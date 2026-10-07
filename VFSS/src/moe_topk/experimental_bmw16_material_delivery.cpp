#include <moe_topk/experimental_bmw16_material_delivery.h>

#include <moe_topk/experimental_bmw16_stream_store.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::size_t kChunk = 64U * 1024U;
constexpr std::size_t kShellLimit = 64U * 1024U * 1024U;
constexpr std::size_t kManifestBytes = 8+4+1+8+8+4+4+1+8+8+32+8+8+32+8+32+32+32;
constexpr std::array<std::uint8_t, 8> kMagic{'B','M','W','2','0','T','L','S'};
constexpr std::uint8_t kPrepare = 1, kCommit = 2, kAbort = 3;
constexpr std::uint8_t kPrepared = 0x81, kCommitted = 0x82, kRejected = 0xff;
using Clock = std::chrono::steady_clock;
using SslCtx = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
using Ssl = std::unique_ptr<SSL, decltype(&SSL_free)>;
using X509Ptr = std::unique_ptr<X509, decltype(&X509_free)>;
using Digest = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

[[noreturn]] void fail(const std::string& message) {
  throw ProtocolIBmw16MaterialDeliveryError(ProtocolIBmw16MaterialDeliveryError::Kind::Material,message);
}
[[noreturn]] void fail_transport(const std::string& message) {
  throw ProtocolIBmw16MaterialDeliveryError(ProtocolIBmw16MaterialDeliveryError::Kind::Transport,message);
}
void require(bool okay, const char* what) { if (!okay) fail(what); }

bool test_failpoint(const char* name) {
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  const char* configured = std::getenv("MOE_BMW16_TEST_FAILPOINT");
  return configured && std::strcmp(configured, name) == 0;
#else
  (void)name;
  return false;
#endif
}

std::uint64_t elapsed_us(Clock::time_point start) {
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count());
}
void put_u8(std::vector<std::uint8_t>& out, std::uint8_t x) { out.push_back(x); }
void put_u32(std::vector<std::uint8_t>& out, std::uint32_t x) {
  for (int s=24;s>=0;s-=8) out.push_back(static_cast<std::uint8_t>(x>>s));
}
void put_u64(std::vector<std::uint8_t>& out, std::uint64_t x) {
  for (int s=56;s>=0;s-=8) out.push_back(static_cast<std::uint8_t>(x>>s));
}
std::uint8_t get_u8(const std::uint8_t*& p) { return *p++; }
std::uint32_t get_u32(const std::uint8_t*& p) {
  std::uint32_t x=0; for(int i=0;i<4;++i)x=(x<<8U)|*p++; return x;
}
std::uint64_t get_u64(const std::uint8_t*& p) {
  std::uint64_t x=0; for(int i=0;i<8;++i)x=(x<<8U)|*p++; return x;
}

std::vector<std::uint8_t> encode_manifest(const ProtocolIBmw16TlsDeliveryManifest& m) {
  std::vector<std::uint8_t> out(kMagic.begin(), kMagic.end());
  put_u32(out, 1); put_u8(out, m.config.party);
  put_u64(out, m.config.session); put_u64(out, m.config.fingerprint);
  put_u32(out, m.config.n); put_u32(out, m.config.k); put_u8(out, m.config.comparison_bits);
  put_u64(out, m.config.claim_root_device); put_u64(out, m.config.claim_root_inode);
  out.insert(out.end(),m.stream_id.begin(),m.stream_id.end());
  put_u64(out,m.slot_count); put_u64(out,m.shell_bytes);
  out.insert(out.end(),m.shell_sha256.begin(),m.shell_sha256.end());
  put_u64(out,m.sidecar_bytes);
  out.insert(out.end(),m.sidecar_sha256.begin(),m.sidecar_sha256.end());
  out.insert(out.end(),m.own_shell_manifest_sha256.begin(),m.own_shell_manifest_sha256.end());
  out.insert(out.end(),m.peer_shell_manifest_sha256.begin(),m.peer_shell_manifest_sha256.end());
  return out;
}

ProtocolIBmw16TlsDeliveryManifest decode_manifest(const std::vector<std::uint8_t>& bytes) {
  require(bytes.size()==kManifestBytes,"BMW16 delivery manifest length");
  require(std::equal(kMagic.begin(),kMagic.end(),bytes.begin()),"BMW16 delivery manifest magic");
  const std::uint8_t* p=bytes.data()+8;
  require(get_u32(p)==1,"BMW16 delivery manifest version");
  ProtocolIBmw16TlsDeliveryManifest m;
  m.config.party=get_u8(p); m.config.session=get_u64(p); m.config.fingerprint=get_u64(p);
  m.config.n=get_u32(p); m.config.k=get_u32(p); m.config.comparison_bits=get_u8(p);
  m.config.claim_root_device=get_u64(p); m.config.claim_root_inode=get_u64(p);
  std::copy_n(p,32,m.stream_id.begin());p+=32;
  m.slot_count=get_u64(p);m.shell_bytes=get_u64(p);
  std::copy_n(p,32,m.shell_sha256.begin());p+=32;
  m.sidecar_bytes=get_u64(p);std::copy_n(p,32,m.sidecar_sha256.begin());p+=32;
  std::copy_n(p,32,m.own_shell_manifest_sha256.begin());p+=32;
  std::copy_n(p,32,m.peer_shell_manifest_sha256.begin());p+=32;
  return m;
}

std::array<std::uint8_t,32> sha256_file(const std::string& path, std::uint64_t expected_bytes) {
  Digest ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free); require(bool(ctx),"BMW16 delivery digest context");
  require(EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"BMW16 delivery digest init");
  const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
  if(fd<0)fail("BMW16 delivery source file open");
  struct stat st{};
  if(::fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<0||static_cast<std::uint64_t>(st.st_size)!=expected_bytes){::close(fd);fail("BMW16 delivery source file shape");}
  std::array<std::uint8_t,kChunk> buf{};std::uint64_t done=0;
  while(done<expected_bytes){const auto want=static_cast<std::size_t>(std::min<std::uint64_t>(buf.size(),expected_bytes-done));
    const auto got=::read(fd,buf.data(),want);if(got<0&&errno==EINTR)continue;
    if(got<=0){::close(fd);fail("BMW16 delivery source truncated");}
    require(EVP_DigestUpdate(ctx.get(),buf.data(),static_cast<std::size_t>(got))==1,"BMW16 delivery digest update");done+=static_cast<std::uint64_t>(got);}
  ::close(fd);std::array<std::uint8_t,32> out{};unsigned n=0;
  require(EVP_DigestFinal_ex(ctx.get(),out.data(),&n)==1&&n==out.size(),"BMW16 delivery digest final");return out;
}

SslCtx make_context(bool server,const ProtocolIBmw16TlsCredentials& c) {
  SslCtx ctx(SSL_CTX_new(server?TLS_server_method():TLS_client_method()),SSL_CTX_free);
  require(bool(ctx),"BMW16 TLS context");
  require(SSL_CTX_set_min_proto_version(ctx.get(),TLS1_3_VERSION)==1,"BMW16 TLS minimum version");
  require(SSL_CTX_use_certificate_chain_file(ctx.get(),c.certificate_pem.c_str())==1,"BMW16 TLS certificate load");
  require(SSL_CTX_use_PrivateKey_file(ctx.get(),c.private_key_pem.c_str(),SSL_FILETYPE_PEM)==1,"BMW16 TLS private key load");
  require(SSL_CTX_check_private_key(ctx.get())==1,"BMW16 TLS certificate/key mismatch");
  require(SSL_CTX_load_verify_locations(ctx.get(),c.trust_bundle_pem.c_str(),nullptr)==1,"BMW16 TLS trust bundle load");
  SSL_CTX_set_verify(ctx.get(),server?(SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT):SSL_VERIFY_PEER,nullptr);
  return ctx;
}

void poll_fd(int fd,short events,int timeout_ms) {
  pollfd p{fd,events,0};int rc;
  do{rc=::poll(&p,1,timeout_ms);}while(rc<0&&errno==EINTR);
  if(rc==0)fail_transport("BMW16 TLS I/O timeout");
  if(rc<0||(p.revents&(POLLERR|POLLNVAL)))fail_transport("BMW16 TLS socket poll error");
  if((p.revents&POLLHUP)&&!(p.revents&events))fail_transport("BMW16 TLS peer closed");
}

void ssl_handshake(SSL* ssl,int fd,bool server,int timeout_ms) {
  for(;;){const int rc=server?SSL_accept(ssl):SSL_connect(ssl);if(rc==1)return;
    const int e=SSL_get_error(ssl,rc);
    if(e==SSL_ERROR_WANT_READ){poll_fd(fd,POLLIN,timeout_ms);continue;}
    if(e==SSL_ERROR_WANT_WRITE){poll_fd(fd,POLLOUT,timeout_ms);continue;}
    fail_transport("BMW16 TLS authenticated handshake failed");}
}

void ssl_write_all(SSL* ssl,int fd,const std::uint8_t* p,std::size_t n,int timeout_ms) {
  while(n){const int ask=static_cast<int>(std::min<std::size_t>(n,static_cast<std::size_t>(std::numeric_limits<int>::max())));
    const int rc=SSL_write(ssl,p,ask);if(rc>0){p+=rc;n-=static_cast<std::size_t>(rc);continue;}
    const int e=SSL_get_error(ssl,rc);if(e==SSL_ERROR_WANT_READ){poll_fd(fd,POLLIN,timeout_ms);continue;}
    if(e==SSL_ERROR_WANT_WRITE){poll_fd(fd,POLLOUT,timeout_ms);continue;}fail_transport("BMW16 TLS write failed");}
}
void ssl_read_exact(SSL* ssl,int fd,std::uint8_t* p,std::size_t n,int timeout_ms) {
  while(n){const int ask=static_cast<int>(std::min<std::size_t>(n,static_cast<std::size_t>(std::numeric_limits<int>::max())));
    const int rc=SSL_read(ssl,p,ask);if(rc>0){p+=rc;n-=static_cast<std::size_t>(rc);continue;}
    const int e=SSL_get_error(ssl,rc);if(e==SSL_ERROR_WANT_READ){poll_fd(fd,POLLIN,timeout_ms);continue;}
    if(e==SSL_ERROR_WANT_WRITE){poll_fd(fd,POLLOUT,timeout_ms);continue;}fail_transport("BMW16 TLS peer closed or read failed");}
}

int connect_tcp(const ProtocolIBmw16TlsEndpoint& ep,int timeout_ms) {
  addrinfo hints{};hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;
  addrinfo* result=nullptr;const auto service=std::to_string(ep.port);
  if(::getaddrinfo(ep.host.c_str(),service.c_str(),&hints,&result)!=0)fail_transport("BMW16 delivery DNS resolution");
  int fd=-1;
  for(auto* it=result;it;it=it->ai_next){fd=::socket(it->ai_family,it->ai_socktype|SOCK_CLOEXEC,it->ai_protocol);if(fd<0)continue;
    const int old=::fcntl(fd,F_GETFL,0);(void)::fcntl(fd,F_SETFL,old|O_NONBLOCK);
    int rc=::connect(fd,it->ai_addr,it->ai_addrlen);
    if(rc==0)break;
    if(errno==EINPROGRESS){pollfd p{fd,POLLOUT,0};do{rc=::poll(&p,1,timeout_ms);}while(rc<0&&errno==EINTR);
      int error=0;socklen_t size=sizeof(error);
      if(rc>0&&::getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&size)==0&&error==0)break;}
    ::close(fd);fd=-1;
  }
  ::freeaddrinfo(result);if(fd<0)fail_transport("BMW16 delivery TCP connect");return fd;
}

Ssl make_client_ssl(SSL_CTX* ctx,int fd,const std::string& identity) {
  Ssl ssl(SSL_new(ctx),SSL_free);require(bool(ssl),"BMW16 TLS client session");
  require(SSL_set_fd(ssl.get(),fd)==1,"BMW16 TLS client fd");
  require(SSL_set_tlsext_host_name(ssl.get(),identity.c_str())==1&&SSL_set1_host(ssl.get(),identity.c_str())==1,"BMW16 TLS server identity setup");
  return ssl;
}
Ssl make_server_ssl(SSL_CTX* ctx,int fd) {
  Ssl ssl(SSL_new(ctx),SSL_free);require(bool(ssl),"BMW16 TLS server session");
  require(SSL_set_fd(ssl.get(),fd)==1,"BMW16 TLS server fd");return ssl;
}
void check_peer_identity(SSL* ssl,const std::string& identity) {
  X509Ptr cert(SSL_get1_peer_certificate(ssl),X509_free);
  require(bool(cert),"BMW16 TLS peer certificate absent");
  if(X509_check_host(cert.get(),identity.c_str(),identity.size(),0,nullptr)!=1||SSL_get_verify_result(ssl)!=X509_V_OK)
    fail_transport("BMW16 TLS peer certificate identity/chain rejected");
}

void send_frame(SSL* ssl,int fd,const std::uint8_t* data,std::size_t size,int timeout) {ssl_write_all(ssl,fd,data,size,timeout);}
void receive_frame(SSL* ssl,int fd,std::uint8_t* data,std::size_t size,int timeout) {ssl_read_exact(ssl,fd,data,size,timeout);}
void send_u8(SSL* ssl,int fd,std::uint8_t value,int timeout){send_frame(ssl,fd,&value,1,timeout);}
std::uint8_t receive_u8(SSL* ssl,int fd,int timeout){std::uint8_t x=0;receive_frame(ssl,fd,&x,1,timeout);return x;}

void send_file(SSL* ssl,int tls_fd,const std::string& path,std::uint8_t file_kind,
               std::uint64_t bytes,int timeout,std::uint64_t& wire) {
  const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)fail("BMW16 delivery source open");
  std::array<std::uint8_t,kChunk> data{};std::uint64_t seq=0,done=0;
  while(done<bytes){const auto count=static_cast<std::size_t>(std::min<std::uint64_t>(kChunk,bytes-done));
    std::size_t got=0;while(got<count){const auto rc=::read(fd,data.data()+got,count-got);if(rc<0&&errno==EINTR)continue;if(rc<=0){::close(fd);fail("BMW16 delivery source truncated while sending");}got+=static_cast<std::size_t>(rc);}
    std::vector<std::uint8_t> h;put_u8(h,file_kind);put_u64(h,seq);put_u32(h,static_cast<std::uint32_t>(count));
    if(test_failpoint("delivery_reorder_chunks")&&seq==1)h[8]^=1U;
    if(test_failpoint("delivery_corrupt_sidecar_chunk")&&file_kind==1&&seq==0&&count>0)data[0]^=0x80U;
    send_frame(ssl,tls_fd,h.data(),h.size(),timeout);send_frame(ssl,tls_fd,data.data(),count,timeout);wire+=h.size()+count;
    done+=count;++seq;
    if(test_failpoint("delivery_truncate_shell")&&done<bytes&&done>=kChunk){::close(fd);fail("TEST_ONLY truncated BMW16 TLS shell");}
    if(test_failpoint("delivery_truncate_sidecar")&&done<bytes&&done>=kChunk){::close(fd);fail("TEST_ONLY truncated BMW16 TLS sidecar");}
  }
  std::uint8_t extra=0;const auto more=::read(fd,&extra,1);::close(fd);if(more!=0)fail("BMW16 delivery source grew while sending");
}

struct TempFile {
  std::string path, final_path;
  int fd=-1;
  bool published=false;
  bool committed=false;
  std::uint64_t device=0,inode=0;
  TempFile()=default;
  TempFile(const TempFile&)=delete;
  TempFile& operator=(const TempFile&)=delete;
  TempFile(TempFile&& other) noexcept
      : path(std::move(other.path)),final_path(std::move(other.final_path)),fd(other.fd),
        published(other.published),committed(other.committed),device(other.device),inode(other.inode) {
    other.fd=-1;other.path.clear();other.published=false;
  }
  TempFile& operator=(TempFile&& other) noexcept {
    if(this!=&other){if(fd>=0)::close(fd);if(!path.empty())(void)::unlink(path.c_str());if(published&&!committed)remove_owned();
      path=std::move(other.path);final_path=std::move(other.final_path);fd=other.fd;published=other.published;committed=other.committed;
      device=other.device;inode=other.inode;other.fd=-1;other.path.clear();other.published=false;}return *this;
  }
  ~TempFile(){if(fd>=0)::close(fd);if(!path.empty())(void)::unlink(path.c_str());if(published&&!committed)remove_owned();}
  void keep_published() noexcept { committed=true; }
  void close_sync(){require(fd>=0,"BMW16 receive temp fd");if(test_failpoint("receiver_fsync"))fail("TEST_ONLY injected receiver fdatasync failure");if(::fdatasync(fd)!=0)fail("BMW16 receive file fdatasync");if(::close(fd)!=0){fd=-1;fail("BMW16 receive file close");}fd=-1;}
  void publish_readonly(){
    require(fd<0&&!published,"BMW16 receive publish state");
    if(::link(path.c_str(),final_path.c_str())!=0)fail("BMW16 receive no-clobber publish");
    struct stat st{};if(::lstat(final_path.c_str(),&st)!=0||!S_ISREG(st.st_mode)){(void)::unlink(final_path.c_str());fail("BMW16 receive published inode");}
    device=static_cast<std::uint64_t>(st.st_dev);inode=static_cast<std::uint64_t>(st.st_ino);published=true;
    if(::chmod(final_path.c_str(),0400)!=0)fail("BMW16 receive final chmod");
    const auto parent=std::filesystem::path(final_path).parent_path();int d=::open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(d<0)fail("BMW16 receive directory open");
    if(::fsync(d)!=0){::close(d);fail("BMW16 receive directory fsync after publish");}
    if(::unlink(path.c_str())!=0){::close(d);fail("BMW16 receive temp unlink");}
    path.clear();
    const bool ok=::fsync(d)==0;::close(d);if(!ok)fail("BMW16 receive directory fsync after temp unlink");
  }
  void remove_owned() noexcept {struct stat st{};if(::lstat(final_path.c_str(),&st)==0&&static_cast<std::uint64_t>(st.st_dev)==device&&static_cast<std::uint64_t>(st.st_ino)==inode)(void)::unlink(final_path.c_str());published=false;}
};

std::string random_suffix(){std::array<std::uint8_t,16> b{};std::size_t done=0;while(done<b.size()){auto n=::getrandom(b.data()+done,b.size()-done,0);if(n<0&&errno==EINTR)continue;if(n<=0)fail("BMW16 receive OS entropy");done+=static_cast<std::size_t>(n);}static constexpr char h[]="0123456789abcdef";std::string s;for(auto x:b){s.push_back(h[x>>4]);s.push_back(h[x&15]);}return s;}

TempFile create_temp(const std::string& final_path){
  struct stat st{};if(::lstat(final_path.c_str(),&st)==0)fail("BMW16 receive destination already exists");if(errno!=ENOENT)fail("BMW16 receive destination inspection");
  TempFile f;f.final_path=final_path;f.path=final_path+".incoming."+std::to_string(::getpid())+"."+random_suffix();
  f.fd=::open(f.path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(f.fd<0)fail("BMW16 receive temp create");return f;
}

std::array<std::uint8_t,32> receive_file(SSL* ssl,int fd,TempFile& temp,std::uint8_t file_kind,std::uint64_t bytes,int timeout,std::uint64_t& wire){
  Digest ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);require(bool(ctx),"BMW16 receive digest context");require(EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"BMW16 receive digest init");
  std::array<std::uint8_t,kChunk> data{};std::uint64_t seq=0,done=0;
  while(done<bytes){std::array<std::uint8_t,13> h{};receive_frame(ssl,fd,h.data(),h.size(),timeout);wire+=h.size();
    const std::uint8_t* p=h.data();const auto kind=get_u8(p);const auto got_seq=get_u64(p);const auto len=get_u32(p);
    const auto expected=static_cast<std::uint32_t>(std::min<std::uint64_t>(kChunk,bytes-done));
    require(kind==file_kind&&got_seq==seq&&len==expected,"BMW16 delivery chunk order/phase/length");
    receive_frame(ssl,fd,data.data(),len,timeout);wire+=len;
    std::size_t written=0;while(written<len){const auto n=::write(temp.fd,data.data()+written,len-written);if(n<0&&errno==EINTR)continue;if(n<=0)fail("BMW16 receive disk write");written+=static_cast<std::size_t>(n);}
    require(EVP_DigestUpdate(ctx.get(),data.data(),len)==1,"BMW16 receive digest update");done+=len;++seq;
  }
  temp.close_sync();std::array<std::uint8_t,32> digest{};unsigned size=0;require(EVP_DigestFinal_ex(ctx.get(),digest.data(),&size)==1&&size==digest.size(),"BMW16 receive digest final");return digest;
}

std::array<std::uint8_t,132> marker_bytes(const ProtocolIBmw16TlsDeliveryManifest& m){
  std::vector<std::uint8_t> b{'B','M','W','1','6','R','D','Y'};
  // S19 marker format uses little-endian integers.
  for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(1U>>(8U*i)));
  for(auto x:{m.config.session,m.config.fingerprint})for(unsigned i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>(x>>(8U*i)));
  for(auto x:{m.config.n,m.config.k})for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(x>>(8U*i)));
  b.insert(b.end(),m.stream_id.begin(),m.stream_id.end());
  b.insert(b.end(),m.config.party==0?m.own_shell_manifest_sha256.begin():m.peer_shell_manifest_sha256.begin(),
                    m.config.party==0?m.own_shell_manifest_sha256.end():m.peer_shell_manifest_sha256.end());
  b.insert(b.end(),m.config.party==1?m.own_shell_manifest_sha256.begin():m.peer_shell_manifest_sha256.begin(),
                    m.config.party==1?m.own_shell_manifest_sha256.end():m.peer_shell_manifest_sha256.end());
  require(b.size()==132,"BMW16 TLS ready marker shape");std::array<std::uint8_t,132> out{};std::copy(b.begin(),b.end(),out.begin());return out;
}

void validate_manifest(const ProtocolIBmw16TlsDeliveryManifest& m,const ProtocolIBmw16ExperimentalPartyConfig& expected,const std::string& claim_root){
  require(m.config.party==expected.party&&m.config.session==expected.session&&m.config.fingerprint==expected.fingerprint&&
          m.config.n==expected.n&&m.config.k==expected.k&&m.config.comparison_bits==expected.comparison_bits&&
          m.config.claim_root_device==expected.claim_root_device&&m.config.claim_root_inode==expected.claim_root_inode,
          "BMW16 TLS delivery identity/config mismatch");
  require(m.config.n>1&&m.config.k>0&&m.config.k<=m.config.n,"BMW16 TLS delivery dimensions");
  const auto root=protocol_i_bmw16_claim_root_identity(claim_root);
  require(root.device==expected.claim_root_device&&root.inode==expected.claim_root_inode,"BMW16 TLS claim-root identity mismatch");
  const auto pairs=static_cast<std::uint64_t>(m.config.n)*(m.config.n-1U)/2U;
  require(pairs<=std::numeric_limits<std::uint64_t>::max()/9U,"BMW16 TLS slot count overflow");
  const auto slots=9U*pairs;require(m.slot_count==slots,"BMW16 TLS slot count mismatch");
  const auto per=57U+24U*m.config.comparison_bits+16U;
  require(slots<=(std::numeric_limits<std::uint64_t>::max()-164U)/per,"BMW16 TLS sidecar length overflow");
  require(m.sidecar_bytes==164U+slots*per,"BMW16 TLS sidecar length mismatch");
  require(m.shell_bytes>0&&m.shell_bytes<=kShellLimit,"BMW16 TLS shell size limit");
}

struct Connection {
  int fd=-1;SslCtx ctx{nullptr,SSL_CTX_free};Ssl ssl{nullptr,SSL_free};
  Connection()=default;Connection(const Connection&)=delete;Connection&operator=(const Connection&)=delete;
  Connection(Connection&& other) noexcept : fd(other.fd),ctx(std::move(other.ctx)),ssl(std::move(other.ssl)){other.fd=-1;}
  Connection& operator=(Connection&& other) noexcept {
    if(this!=&other){if(ssl)SSL_shutdown(ssl.get());ssl.reset();if(fd>=0)::close(fd);
      fd=other.fd;ctx=std::move(other.ctx);ssl=std::move(other.ssl);other.fd=-1;}return *this;
  }
  ~Connection(){if(ssl)SSL_shutdown(ssl.get());ssl.reset();if(fd>=0)::close(fd);}
};

Connection connect_tls(const ProtocolIBmw16TlsCredentials& c,const ProtocolIBmw16TlsEndpoint& ep){
  Connection x;x.fd=connect_tcp(ep,c.io_timeout_ms);x.ctx=make_context(false,c);x.ssl=make_client_ssl(x.ctx.get(),x.fd,ep.server_dns_identity);
  ssl_handshake(x.ssl.get(),x.fd,false,c.io_timeout_ms);check_peer_identity(x.ssl.get(),ep.server_dns_identity);return x;
}

void client_prepare(Connection& conn,const ProtocolIBmw16TlsCredentials& credentials,
                    const ProtocolIBmw16TlsEndpoint& endpoint,const ProtocolIBmw16TlsDeliveryManifest& m,
                    const ProtocolIBmw16TlsDeliveryFiles& files,
                    const std::array<std::uint8_t,132>& ready_marker,
                    std::uint64_t& sent,std::uint64_t& received){
  auto wire=encode_manifest(m);
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  if(test_failpoint("delivery_wrong_party"))wire[12]^=1U;
  if(test_failpoint("delivery_wrong_session"))wire[13]^=1U;
  if(test_failpoint("delivery_wrong_stream"))wire[54]^=1U;
#endif
  send_u8(conn.ssl.get(),conn.fd,kPrepare,credentials.io_timeout_ms);send_frame(conn.ssl.get(),conn.fd,wire.data(),wire.size(),credentials.io_timeout_ms);sent+=wire.size()+1;
  send_file(conn.ssl.get(),conn.fd,files.shell_path,0,m.shell_bytes,credentials.io_timeout_ms,sent);
  // File kind is embedded in each framed chunk; reset to sidecar with a distinct channel byte.
  // Sidecars are read in bounded chunks and never accumulated in T's RAM.
  send_file(conn.ssl.get(),conn.fd,files.sidecar_path,1,m.sidecar_bytes,credentials.io_timeout_ms,sent);
  send_frame(conn.ssl.get(),conn.fd,ready_marker.data(),ready_marker.size(),credentials.io_timeout_ms);sent+=ready_marker.size();
  const auto ack=receive_u8(conn.ssl.get(),conn.fd,credentials.io_timeout_ms);received+=1;
  if(ack!=kPrepared)fail("BMW16 receiver did not prepare authenticated package");
  (void)endpoint;
}

}  // namespace

void protocol_i_bmw16_tls_deliver_pair(const ProtocolIBmw16TlsCredentials& dealer_credentials,
    const std::array<ProtocolIBmw16TlsEndpoint,2>& endpoints,
    const std::array<ProtocolIBmw16TlsDeliveryManifest,2>& manifests,
    const std::array<ProtocolIBmw16TlsDeliveryFiles,2>& files,
    const std::array<std::uint8_t,132>& pair_ready_marker,ProtocolIBmw16TlsDeliveryStats* out_stats){
  const auto started=Clock::now();ProtocolIBmw16TlsDeliveryStats stats{};
  std::array<Connection,2> c;
  try{
    for(std::size_t i=0;i<2;++i){
      require(manifests[i].config.party==i,"BMW16 TLS party ordering");
      require(sha256_file(files[i].shell_path,manifests[i].shell_bytes)==manifests[i].shell_sha256,"BMW16 T shell digest mismatch");
      require(sha256_file(files[i].sidecar_path,manifests[i].sidecar_bytes)==manifests[i].sidecar_sha256,"BMW16 T sidecar digest mismatch");
      require(marker_bytes(manifests[i])==pair_ready_marker,"BMW16 T pair marker mismatch");
      c[i]=connect_tls(dealer_credentials,endpoints[i]);
      client_prepare(c[i],dealer_credentials,endpoints[i],manifests[i],files[i],pair_ready_marker,stats.bytes_sent,stats.bytes_received);
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
      if(i==0&&test_failpoint("crash_t_tls_after_prepare0"))::_exit(71);
#endif
    }
    for(std::size_t i=0;i<2;++i){send_u8(c[i].ssl.get(),c[i].fd,kCommit,dealer_credentials.io_timeout_ms);++stats.bytes_sent;
      const auto ack=receive_u8(c[i].ssl.get(),c[i].fd,dealer_credentials.io_timeout_ms);++stats.bytes_received;
      if(ack!=kCommitted)fail("BMW16 party failed durable package commit");
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
      if(i==0&&test_failpoint("crash_t_tls_after_commit0"))::_exit(71);
#endif
    }
  }catch(...){for(auto& conn:c)if(conn.ssl){try{send_u8(conn.ssl.get(),conn.fd,kAbort,dealer_credentials.io_timeout_ms);}catch(...){}}throw;}
  stats.elapsed_us=elapsed_us(started);if(out_stats)*out_stats=stats;
}

ProtocolIBmw16TlsDeliveryStats protocol_i_bmw16_tls_receive_once(int listening_socket,
    const ProtocolIBmw16TlsCredentials& party_credentials,
    const ProtocolIBmw16ExperimentalPartyConfig& expected_config,const std::string& claim_root,
    const ProtocolIBmw16TlsDeliveryFiles& destination,const std::array<std::uint8_t,32>& wrapping_key){
  const auto started=Clock::now();ProtocolIBmw16TlsDeliveryStats stats{};
  pollfd ready{listening_socket,POLLIN,0};
  int poll_rc;do{poll_rc=::poll(&ready,1,party_credentials.io_timeout_ms);}while(poll_rc<0&&errno==EINTR);
  if(poll_rc<=0)fail_transport(poll_rc==0?"BMW16 TLS accept timeout":"BMW16 TLS listener poll failed");
  const int fd=::accept4(listening_socket,nullptr,nullptr,SOCK_CLOEXEC);if(fd<0)fail_transport("BMW16 TLS accept failed");
  Connection conn;conn.fd=fd;conn.ctx=make_context(true,party_credentials);conn.ssl=make_server_ssl(conn.ctx.get(),fd);
  auto flags=::fcntl(fd,F_GETFL,0);if(flags>=0)(void)::fcntl(fd,F_SETFL,flags|O_NONBLOCK);
  ssl_handshake(conn.ssl.get(),fd,true,party_credentials.io_timeout_ms);check_peer_identity(conn.ssl.get(),party_credentials.peer_dns_identity);
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  if(test_failpoint("receiver_silent")) {
    // A bounded socket wait models an authenticated but silent peer. Do not
    // consume PREPARE or send a rejection; the sender must hit its own timeout.
    pollfd silent{fd,0,0};int rc;
    do{rc=::poll(&silent,1,party_credentials.io_timeout_ms);}while(rc<0&&errno==EINTR);
    if(rc<0)fail_transport("TEST_ONLY silent-peer poll failed");
    fail_transport("TEST_ONLY silent receiver timed out without replying");
  }
#endif
  std::uint64_t recv_wire=0,send_wire=0;
  try{
    if(receive_u8(conn.ssl.get(),fd,party_credentials.io_timeout_ms)!=kPrepare)fail("BMW16 TLS expected PREPARE");++recv_wire;
    std::vector<std::uint8_t> raw(kManifestBytes);receive_frame(conn.ssl.get(),fd,raw.data(),raw.size(),party_credentials.io_timeout_ms);recv_wire+=raw.size();
    auto m=decode_manifest(raw);validate_manifest(m,expected_config,claim_root);
    struct stat root_st{};if(::stat(claim_root.c_str(),&root_st)!=0||root_st.st_uid!=::geteuid()||(root_st.st_mode&0777)!=0700)fail("BMW16 TLS claim root owner/mode");
    if(std::filesystem::path(destination.shell_path).parent_path()!=claim_root||
       std::filesystem::path(destination.sidecar_path).parent_path()!=claim_root||
       std::filesystem::path(destination.pair_ready_path).parent_path()!=claim_root)fail("BMW16 TLS destination must be inside claim root");
    auto shell=create_temp(destination.shell_path);auto sidecar=create_temp(destination.sidecar_path);auto marker=create_temp(destination.pair_ready_path);
    const auto shell_digest=receive_file(conn.ssl.get(),fd,shell,0,m.shell_bytes,party_credentials.io_timeout_ms,recv_wire);
    const auto sidecar_digest=receive_file(conn.ssl.get(),fd,sidecar,1,m.sidecar_bytes,party_credentials.io_timeout_ms,recv_wire);
    std::array<std::uint8_t,132> transmitted{};receive_frame(conn.ssl.get(),fd,transmitted.data(),transmitted.size(),party_credentials.io_timeout_ms);recv_wire+=transmitted.size();
    if(shell_digest!=m.shell_sha256||sidecar_digest!=m.sidecar_sha256)fail("BMW16 TLS package digest mismatch");
    const auto expected_marker=marker_bytes(m);if(transmitted!=expected_marker)fail("BMW16 TLS pair-ready marker mismatch");
    auto shell_bytes=protocol_i_bmw16_bundle_read(shell.path);
    auto opened=protocol_i_bmw16_bundle_open_stream_shell_party(expected_config,expected_config.party,shell_bytes,wrapping_key);
    if(opened.stats.manifest_sha256!=m.own_shell_manifest_sha256||opened.stream.stream_id!=m.stream_id||
       opened.stream.sidecar_sha256!=m.sidecar_sha256||opened.stream.ucmp_slot_count!=m.slot_count||
       opened.stream.sealed_sidecar_bytes!=m.sidecar_bytes)fail("BMW16 TLS shell/manifest mismatch");
    auto store=ProtocolIBmw16StreamedUcmpSlotStore::open_party(sidecar.path,expected_config,expected_config.party,
                                                              wrapping_key,m.stream_id,m.sidecar_sha256);
    if(store->slots()!=m.slot_count||store->file_bytes()!=m.sidecar_bytes)fail("BMW16 TLS sidecar shape mismatch");
    auto marker_bytes_vec=std::vector<std::uint8_t>(transmitted.begin(),transmitted.end());
    {std::size_t done=0;while(done<marker_bytes_vec.size()){const auto n=::write(marker.fd,marker_bytes_vec.data()+done,marker_bytes_vec.size()-done);if(n<0&&errno==EINTR)continue;if(n<=0)fail("BMW16 TLS ready marker write");done+=static_cast<std::size_t>(n);}}
    marker.close_sync();
    send_u8(conn.ssl.get(),fd,kPrepared,party_credentials.io_timeout_ms);++send_wire;
    const auto decision=receive_u8(conn.ssl.get(),fd,party_credentials.io_timeout_ms);++recv_wire;
    if(decision==kAbort)fail("BMW16 T aborted paired delivery");if(decision!=kCommit)fail("BMW16 TLS invalid commit decision");
    if(test_failpoint("receiver_publish"))fail("TEST_ONLY injected receiver publish failure");
    shell.publish_readonly();sidecar.publish_readonly();marker.publish_readonly();
    send_u8(conn.ssl.get(),fd,kCommitted,party_credentials.io_timeout_ms);++send_wire;
    shell.keep_published();sidecar.keep_published();marker.keep_published();
  }catch(...){try{send_u8(conn.ssl.get(),fd,kRejected,party_credentials.io_timeout_ms);++send_wire;}catch(...){}throw;}
  stats.bytes_received=recv_wire;stats.bytes_sent=send_wire;stats.elapsed_us=elapsed_us(started);return stats;
}

}  // namespace moe_topk
