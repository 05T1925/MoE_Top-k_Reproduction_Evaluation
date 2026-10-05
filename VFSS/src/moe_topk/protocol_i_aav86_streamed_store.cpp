#include <moe_topk/protocol_i_aav86_streamed_store.h>

#include <FSS/prng.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

namespace moe_topk {
namespace {
constexpr std::uint32_t kMagic=UINT32_C(0x4d364132); // M6A2
constexpr std::uint32_t kVersion=1;
constexpr std::size_t kHeaderBytes=80, kRecordHeaderBytes=32, kTagBytes=16;
using Header=std::array<std::uint8_t,kHeaderBytes>;
using RecordHeader=std::array<std::uint8_t,kRecordHeaderBytes>;

void check(bool ok,const char* message) {
  if(!ok) throw std::runtime_error(message);
}
std::uint64_t add(std::uint64_t a,std::uint64_t b) {
  if(b>UINT64_MAX-a) throw std::overflow_error("E20 size addition");
  return a+b;
}
std::uint64_t mul(std::uint64_t a,std::uint64_t b) {
  if(a&&b>UINT64_MAX/a) throw std::overflow_error("E20 size multiplication");
  return a*b;
}
std::uint32_t padded(std::uint32_t n) {
  check(n>=1&&n<=1000,"E20 logical n");
  std::uint32_t d=2;
  while(d<n) d*=2;
  return d;
}
std::uint32_t bits_for(std::uint32_t d) {
  std::uint32_t bits=33;
  for(auto x=d-1U;x;x>>=1U) ++bits;
  return bits;
}
std::uint64_t pairs(std::uint32_t d) { return mul(d,d-1U)/2U; }
std::uint64_t edge_slot(std::uint32_t d,std::uint32_t r,
                        std::uint32_t t,std::uint32_t a,std::uint32_t c) {
  check(t<r&&a<c&&c<d,"E20 canonical edge");
  return add(mul(t,pairs(d)),
             add(mul(a,2U*d-a-1U)/2U,c-a-1U));
}
void put(std::uint8_t* out,std::size_t& at,std::uint64_t value,unsigned bytes) {
  for(unsigned shift=bytes;shift;--shift)
    out[at++]=static_cast<std::uint8_t>(value>>(8U*(shift-1U)));
}
Header header_for(const ProtocolIAav86SmallConfig& c,std::uint32_t party,
                  bool minimal=false) {
  const auto d=padded(c.logical_n),b=bits_for(d);
  check(c.session&&c.fingerprint&&c.material_id&&c.k>=1&&c.k<=c.logical_n&&
            c.iterations>=1&&c.iterations<=5&&party<2&&c.timeout_ms>0,
        "E20 shape and identity");
  const auto slots=mul(c.iterations,pairs(d));
  check(c.material_id<=UINT64_MAX-slots,"E20 material ID range");
  const auto key_bytes=24U*b+57U;
  const auto record_bytes=kRecordHeaderBytes+key_bytes+kTagBytes;
  Header out{};std::size_t at=0;
  put(out.data(),at,kMagic,4);put(out.data(),at,minimal?2U:kVersion,4);
  put(out.data(),at,c.session,8);put(out.data(),at,c.fingerprint,8);
  put(out.data(),at,c.material_id,8);
  put(out.data(),at,c.logical_n,4);put(out.data(),at,d,4);
  put(out.data(),at,c.k,4);put(out.data(),at,c.iterations,4);
  put(out.data(),at,b,4);put(out.data(),at,party,4);
  put(out.data(),at,key_bytes,4);put(out.data(),at,slots,8);
  put(out.data(),at,record_bytes,4);put(out.data(),at,0U,8);
  check(at==out.size(),"E20 header size");
  return out;
}
RecordHeader record_for(std::uint64_t slot,std::uint64_t id,
                        std::uint32_t t,std::uint32_t a,std::uint32_t c,
                        std::uint32_t party) {
  RecordHeader out{};std::size_t at=0;
  put(out.data(),at,slot,8);put(out.data(),at,id,8);
  put(out.data(),at,t,4);put(out.data(),at,a,4);
  put(out.data(),at,c,4);put(out.data(),at,party,4);
  return out;
}
std::array<std::uint8_t,12> nonce_for(std::uint64_t slot) {
  std::array<std::uint8_t,12> nonce{};std::size_t at=4;
  put(nonce.data(),at,slot,8);
  return nonce;
}
void random_bytes(void* out,std::size_t count) {
  auto* bytes=static_cast<std::uint8_t*>(out);
  while(count) {
    const auto got=::getrandom(bytes,count,0);
    if(got<0&&errno==EINTR) continue;
    check(got>0,"E20 OS entropy");
    bytes+=got;count-=static_cast<std::size_t>(got);
  }
}
void bound_socket(int fd,int timeout_ms) {
  check(fd>=0&&timeout_ms>0,"E20 bounded delivery descriptor");
  const timeval limit{timeout_ms/1000,(timeout_ms%1000)*1000};
  check(::setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&limit,sizeof(limit))==0&&
        ::setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&limit,sizeof(limit))==0,
        "E20 bounded delivery socket");
}
void send_exact(int fd,const void* data,std::size_t count) {
  const auto* bytes=static_cast<const std::uint8_t*>(data);
  while(count) {
    const auto sent=::write(fd,bytes,count);
    if(sent<0&&errno==EINTR) continue;
    check(sent>0,"E20 T delivery closed");
    bytes+=sent;count-=static_cast<std::size_t>(sent);
  }
}
void receive_exact(int fd,void* data,std::size_t count) {
  auto* bytes=static_cast<std::uint8_t*>(data);
  while(count) {
    const auto got=::read(fd,bytes,count);
    if(got<0&&errno==EINTR) continue;
    check(got>0,"E20 T delivery truncated/closed");
    bytes+=got;count-=static_cast<std::size_t>(got);
  }
}
void write_exact(int fd,const void* data,std::size_t count) {
  const auto* bytes=static_cast<const std::uint8_t*>(data);
  while(count) {
    const auto wrote=::write(fd,bytes,count);
    if(wrote<0&&errno==EINTR) continue;
    check(wrote>0,"E20 store write");
    bytes+=wrote;count-=static_cast<std::size_t>(wrote);
  }
}
void pread_exact(int fd,void* data,std::size_t count,std::uint64_t offset) {
  auto* bytes=static_cast<std::uint8_t*>(data);
  check(offset<=static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()),
        "E20 store offset");
  while(count) {
    const auto got=::pread(fd,bytes,count,static_cast<off_t>(offset));
    if(got<0&&errno==EINTR) continue;
    check(got>0,"E20 store record truncated");
    bytes+=got;count-=static_cast<std::size_t>(got);offset+=got;
  }
}
using Context=std::unique_ptr<EVP_CIPHER_CTX,decltype(&EVP_CIPHER_CTX_free)>;
std::vector<std::uint8_t> seal(
    const std::array<std::uint8_t,32>& key,const Header& header,
    const RecordHeader& record,std::uint64_t slot,
    const std::vector<std::uint8_t>& plain) {
  const auto nonce=nonce_for(slot);
  Context ctx(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);
  check(bool(ctx),"E20 AES context");
  check(EVP_EncryptInit_ex(ctx.get(),EVP_aes_256_gcm(),nullptr,key.data(),nonce.data())==1,
        "E20 AES init");
  int n=0;
  check(EVP_EncryptUpdate(ctx.get(),nullptr,&n,header.data(),header.size())==1&&
        EVP_EncryptUpdate(ctx.get(),nullptr,&n,record.data(),record.size())==1,
        "E20 AES AAD");
  std::vector<std::uint8_t> encrypted(plain.size()+kTagBytes);
  check(EVP_EncryptUpdate(ctx.get(),encrypted.data(),&n,plain.data(),plain.size())==1&&
            n==static_cast<int>(plain.size()),"E20 AES encrypt");
  int final=0;
  check(EVP_EncryptFinal_ex(ctx.get(),encrypted.data()+n,&final)==1&&final==0,
        "E20 AES finish");
  check(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_GET_TAG,kTagBytes,
                            encrypted.data()+plain.size())==1,"E20 AES tag");
  return encrypted;
}
std::vector<std::uint8_t> open_record(
    const std::array<std::uint8_t,32>& key,const Header& header,
    const RecordHeader& record,std::uint64_t slot,
    const std::vector<std::uint8_t>& encrypted) {
  check(encrypted.size()>=kTagBytes,"E20 encrypted length");
  const auto nonce=nonce_for(slot);
  Context ctx(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);
  check(bool(ctx),"E20 AES context");
  check(EVP_DecryptInit_ex(ctx.get(),EVP_aes_256_gcm(),nullptr,key.data(),nonce.data())==1,
        "E20 AES init");
  int n=0;
  check(EVP_DecryptUpdate(ctx.get(),nullptr,&n,header.data(),header.size())==1&&
        EVP_DecryptUpdate(ctx.get(),nullptr,&n,record.data(),record.size())==1,
        "E20 AES AAD");
  std::vector<std::uint8_t> plain(encrypted.size()-kTagBytes);
  check(EVP_DecryptUpdate(ctx.get(),plain.data(),&n,encrypted.data(),plain.size())==1&&
            n==static_cast<int>(plain.size()),"E20 AES decrypt");
  check(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_TAG,kTagBytes,
                            const_cast<std::uint8_t*>(encrypted.data()+plain.size()))==1,
        "E20 AES tag setup");
  int final=0;
  check(EVP_DecryptFinal_ex(ctx.get(),plain.data()+n,&final)==1&&final==0,
        "E20 store authentication failed");
  return plain;
}
std::string file_name(const ProtocolIAav86SmallConfig& c,bool minimal=false) {
  return std::string(minimal?"clique-e21-":"aav86-e20-")+
         std::to_string(c.session)+"-"+
         std::to_string(c.material_id)+"-p"+std::to_string(c.party)+".sealed";
}
int private_dir(const std::string& path) {
  check(!path.empty(),"E20 private directory");
  if(::mkdir(path.c_str(),0700)<0) check(errno==EEXIST,"E20 directory create");
  const int fd=::open(path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
  check(fd>=0,"E20 private directory open");
  struct stat info{};
  const bool okay=::fstat(fd,&info)==0&&S_ISDIR(info.st_mode)&&
      info.st_uid==::geteuid()&&(info.st_mode&077U)==0;
  if(!okay) {::close(fd);throw std::runtime_error("E20 private directory mode/owner");}
  return fd;
}
} // namespace

ProtocolIStreamDeliveryBytes protocol_i_aav86_stream_dealer_send(
    const ProtocolIAav86SmallConfig& config,int party0_fd,int party1_fd) {
  check(party0_fd>=0&&party1_fd>=0&&party0_fd!=party1_fd,"E20 dealer channels");
  const auto h0=header_for(config,0),h1=header_for(config,1);
  bound_socket(party0_fd,config.timeout_ms);
  bound_socket(party1_fd,config.timeout_ms);
  send_exact(party0_fd,h0.data(),h0.size());
  send_exact(party1_fd,h1.data(),h1.size());
  ProtocolIStreamDeliveryBytes sent{h0.size(),h1.size()};
  const auto key_bytes=24U*bits_for(padded(config.logical_n))+57U;
  auto sink=[&](std::uint32_t,std::uint32_t,std::uint32_t,std::uint64_t,
                ProtocolIUcmpPartyMaterial&& k0,
                ProtocolIUcmpPartyMaterial&& k1) {
    const auto b0=k0.serialize(),b1=k1.serialize();
    check(b0.size()==key_bytes&&b1.size()==key_bytes,"E20 key width");
    send_exact(party0_fd,b0.data(),b0.size());
    send_exact(party1_fd,b1.data(),b1.size());
    sent.party0=add(sent.party0,b0.size());
    sent.party1=add(sent.party1,b1.size());
  };
  auto base=protocol_i_aav86_stream_dealer_generate(config,sink);
  for(const auto& item:{std::pair{party0_fd,&base.party0},
                       std::pair{party1_fd,&base.party1}}) {
    const auto bytes=protocol_i_aav86_stream_serialize_base(*item.second);
    check(bytes.size()<=UINT32_MAX,"E20 base length");
    std::array<std::uint8_t,4> size{};std::size_t at=0;
    put(size.data(),at,bytes.size(),4);
    send_exact(item.first,size.data(),size.size());
    send_exact(item.first,bytes.data(),bytes.size());
    auto& count=item.first==party0_fd?sent.party0:sent.party1;
    count=add(count,add(size.size(),bytes.size()));
  }
  return sent;
}

namespace {
constexpr std::uint32_t kCliqueBaseMagic=UINT32_C(0x45323143); // E21C
constexpr std::uint32_t kCliqueBaseVersion=1;
constexpr std::uint32_t kScoreKeyBytes=24U*34U+57U;
std::mutex clique_dealer_mutex;

void append(std::vector<std::uint8_t>& out,std::uint64_t value,unsigned count) {
  for(unsigned i=count;i>0;--i)
    out.push_back(static_cast<std::uint8_t>(value>>(8U*(i-1U))));
}
std::uint64_t take(const std::vector<std::uint8_t>& in,std::size_t& at,
                   unsigned count) {
  check(count<=8&&at<=in.size()&&count<=in.size()-at,"E21 base truncated");
  std::uint64_t value=0;
  for(unsigned i=0;i<count;++i) value=(value<<8U)|in[at++];
  return value;
}
std::vector<std::uint8_t> clique_base_encode(
    const ProtocolIAav86SmallPartyMaterial& m) {
  const auto d=padded(m.logical_n);
  check(m.iterations==1&&m.padded_n==d&&m.party<2&&
            m.node_mask_shares.size()==d&&m.edge_keys.empty()&&
            m.forward_sigma.empty()&&m.forward_tau.empty()&&
            m.inverse_sigma.empty()&&m.inverse_tau.empty()&&
            m.forward_a.empty()&&m.forward_e.empty()&&
            m.inverse_a.empty()&&m.inverse_e.empty()&&
            m.pivot_seed_lo==0&&m.pivot_seed_hi==0,
        "E21 base contains unexpected AAV86 state");
  std::vector<std::uint8_t> out;
  append(out,kCliqueBaseMagic,4);append(out,kCliqueBaseVersion,4);
  append(out,m.session,8);append(out,m.fingerprint,8);
  append(out,m.material_id,8);append(out,m.logical_n,4);
  append(out,d,4);append(out,m.k,4);append(out,m.iterations,4);
  append(out,m.comparison_bits,4);append(out,m.party,4);
  append(out,d,4);
  for(auto word:m.node_mask_shares) append(out,word,8);
  for(std::uint32_t stage=1;stage<=2;++stage) {
    const auto& items=stage==1?m.score_materials.carry_materials:
                               m.score_materials.sign_materials;
    check(items.size()==d,"E21 base score count");
    append(out,d,4);
    for(std::uint32_t slot=0;slot<d;++slot) {
      const auto& item=items[slot];
      check(item.slot==slot&&item.stage==stage&&
                item.material.party_id()==m.party&&
                item.material.comparison_bits()==34,"E21 base score shape");
      const auto key=item.material.serialize();
      check(key.size()==kScoreKeyBytes,"E21 score key length");
      append(out,slot,4);append(out,stage,4);
      append(out,item.left_mask_share,8);
      append(out,item.right_mask_share,8);
      out.insert(out.end(),key.begin(),key.end());
    }
  }
  check(out.size()<=16U*1024U*1024U,"E21 base maximum");
  return out;
}
ProtocolIAav86SmallPartyMaterial clique_base_decode(
    const std::vector<std::uint8_t>& in,
    const ProtocolIAav86SmallConfig& config) {
  check(in.size()<=16U*1024U*1024U,"E21 base maximum");
  std::size_t at=0;
  check(take(in,at,4)==kCliqueBaseMagic&&
            take(in,at,4)==kCliqueBaseVersion,"E21 base magic/version");
  ProtocolIAav86SmallPartyMaterial m;
  m.session=take(in,at,8);m.fingerprint=take(in,at,8);
  m.material_id=take(in,at,8);m.logical_n=take(in,at,4);
  m.padded_n=take(in,at,4);m.k=take(in,at,4);
  m.iterations=take(in,at,4);
  const auto received_bits=take(in,at,4),received_party=take(in,at,4);
  check(received_bits<=UINT8_MAX&&received_party<=1,
        "E21 base numeric binding");
  m.comparison_bits=received_bits;m.party=received_party;
  const auto d=padded(config.logical_n);
  check(m.session==config.session&&m.fingerprint==config.fingerprint&&
            m.material_id==config.material_id&&m.logical_n==config.logical_n&&
            m.padded_n==d&&m.k==config.k&&m.iterations==1&&
            config.iterations==1&&m.comparison_bits==bits_for(d)&&
            m.party==config.party,"E21 base binding");
  check(take(in,at,4)==d,"E21 node count");
  m.node_mask_shares.reserve(d);
  for(std::uint32_t a=0;a<d;++a) m.node_mask_shares.push_back(take(in,at,8));
  auto& score=m.score_materials;
  score.session=m.session;score.fingerprint=m.fingerprint;
  score.party=m.party;score.n=d;score.k=m.k;
  score.comparison_bits=m.comparison_bits;
  for(std::uint32_t stage=1;stage<=2;++stage) {
    check(take(in,at,4)==d,"E21 score count");
    auto& items=stage==1?score.carry_materials:score.sign_materials;
    for(std::uint32_t slot=0;slot<d;++slot) {
      check(take(in,at,4)==slot&&take(in,at,4)==stage,
            "E21 score slot/stage");
      const auto left=take(in,at,8),right=take(in,at,8);
      check(at<=in.size()&&kScoreKeyBytes<=in.size()-at,
            "E21 score key truncated");
      std::vector<std::uint8_t> raw(in.begin()+at,in.begin()+at+kScoreKeyBytes);
      at+=kScoreKeyBytes;
      auto key=ProtocolIUcmpPartyMaterial::deserialize(raw);
      check(key.party_id()==m.party&&key.comparison_bits()==34,
            "E21 score key binding");
      items.emplace_back(slot,static_cast<std::uint8_t>(stage),
                         left,right,std::move(key));
    }
  }
  check(at==in.size(),"E21 base trailing data");
  return m;
}
} // namespace

ProtocolIStreamDeliveryBytes protocol_i_clique_minimal_dealer_send(
    const ProtocolIAav86SmallConfig& config,int party0_fd,int party1_fd) {
  check(config.iterations==1&&party0_fd>=0&&party1_fd>=0&&
            party0_fd!=party1_fd,"E21 dealer shape/channels");
  const auto h0=header_for(config,0,true),h1=header_for(config,1,true);
  bound_socket(party0_fd,config.timeout_ms);
  bound_socket(party1_fd,config.timeout_ms);
  std::lock_guard<std::mutex> guard(clique_dealer_mutex);
  std::array<std::uint64_t,2> seed{};
  random_bytes(seed.data(),sizeof(seed));
  FSSConfig::prngs[0].SetSeed(osuCrypto::toBlock(seed[0],seed[1]));
  ProtocolIAav86SmallDealerOutput base;
  const auto d=padded(config.logical_n),b=bits_for(d);
  const auto mask=(UINT64_C(1)<<b)-1U;
  auto init=[&](ProtocolIAav86SmallPartyMaterial& m,std::uint32_t party) {
    m.session=config.session;m.fingerprint=config.fingerprint;
    m.material_id=config.material_id;m.logical_n=config.logical_n;
    m.padded_n=d;m.k=config.k;m.iterations=1;
    m.comparison_bits=b;m.party=party;
    m.score_materials.session=config.session;
    m.score_materials.fingerprint=config.fingerprint;
    m.score_materials.party=party;m.score_materials.n=d;
    m.score_materials.k=config.k;m.score_materials.comparison_bits=b;
    m.node_mask_shares.reserve(d);
  };
  init(base.party0,0);init(base.party1,1);
  const auto score_mask=(UINT64_C(1)<<34)-1U;
  for(std::uint32_t stage=1;stage<=2;++stage)
    for(std::uint32_t slot=0;slot<d;++slot) {
      std::array<std::uint64_t,4> words{};
      random_bytes(words.data(),sizeof(words));
      const auto left=words[0]&score_mask,right=words[1]&score_mask;
      const auto left0=words[2]&score_mask,right0=words[3]&score_mask;
      ProtocolIUcmpMaterial material(34,left,right);
      auto k0=material.export_party_material(0);
      auto k1=material.export_party_material(1);
      auto& v0=stage==1?base.party0.score_materials.carry_materials:
                        base.party0.score_materials.sign_materials;
      auto& v1=stage==1?base.party1.score_materials.carry_materials:
                        base.party1.score_materials.sign_materials;
      v0.emplace_back(slot,stage,left0,right0,std::move(k0));
      v1.emplace_back(slot,stage,(left-left0)&score_mask,
                      (right-right0)&score_mask,std::move(k1));
    }
  std::vector<std::uint64_t> full(d);
  for(std::uint32_t a=0;a<d;++a) {
    random_bytes(&full[a],sizeof(full[a]));full[a]&=mask;
    std::uint64_t share0=0;random_bytes(&share0,sizeof(share0));share0&=mask;
    base.party0.node_mask_shares.push_back(share0);
    base.party1.node_mask_shares.push_back((full[a]-share0)&mask);
  }
  send_exact(party0_fd,h0.data(),h0.size());
  send_exact(party1_fd,h1.data(),h1.size());
  ProtocolIStreamDeliveryBytes sent{h0.size(),h1.size()};
  const auto key_bytes=24U*b+57U;
  for(std::uint32_t a=0;a<d;++a)
    for(std::uint32_t c=a+1;c<d;++c) {
      ProtocolIUcmpMaterial material(b,full[a],full[c]);
      const auto k0=material.export_party_material(0).serialize();
      const auto k1=material.export_party_material(1).serialize();
      check(k0.size()==key_bytes&&k1.size()==key_bytes,
            "E21 edge key width");
      send_exact(party0_fd,k0.data(),k0.size());
      send_exact(party1_fd,k1.data(),k1.size());
      sent.party0=add(sent.party0,k0.size());
      sent.party1=add(sent.party1,k1.size());
    }
  for(const auto& item:{std::pair{party0_fd,&base.party0},
                       std::pair{party1_fd,&base.party1}}) {
    const auto bytes=clique_base_encode(*item.second);
    std::array<std::uint8_t,4> size{};std::size_t at=0;
    put(size.data(),at,bytes.size(),4);
    send_exact(item.first,size.data(),size.size());
    send_exact(item.first,bytes.data(),bytes.size());
    auto& count=item.first==party0_fd?sent.party0:sent.party1;
    count=add(count,add(size.size(),bytes.size()));
  }
  return sent;
}

ProtocolIAav86StreamedPartyMaterial::~ProtocolIAav86StreamedPartyMaterial() {
  if(fd_>=0) ::close(fd_);
  OPENSSL_cleanse(key_.data(),key_.size());
}
ProtocolIAav86StreamedPartyMaterial::ProtocolIAav86StreamedPartyMaterial(
    ProtocolIAav86StreamedPartyMaterial&& other) noexcept { *this=std::move(other); }
ProtocolIAav86StreamedPartyMaterial& ProtocolIAav86StreamedPartyMaterial::operator=(
    ProtocolIAav86StreamedPartyMaterial&& other) noexcept {
  if(this!=&other) {
    if(fd_>=0) ::close(fd_);
    OPENSSL_cleanse(key_.data(),key_.size());
    base=std::move(other.base);fd_=other.fd_;other.fd_=-1;
    key_=other.key_;OPENSSL_cleanse(other.key_.data(),other.key_.size());
    header_=other.header_;disk_bytes_=other.disk_bytes_;
    plaintext_payload_bytes_=other.plaintext_payload_bytes_;
    record_bytes_=other.record_bytes_;key_bytes_=other.key_bytes_;
    claimed_=other.claimed_;other.claimed_=false;
    read_slots_=std::move(other.read_slots_);
    path_=std::move(other.path_);
  }
  return *this;
}

ProtocolIAav86StreamedPartyMaterial protocol_i_stream_receive_impl(
    const ProtocolIAav86SmallConfig& config,int dealer_fd,
    const std::string& private_directory,bool minimal) {
  check(dealer_fd>=0&&config.party<2,"E20 party delivery channel");
  check(!minimal||config.iterations==1,"E21 clique iteration");
  const auto expected=header_for(config,config.party,minimal);
  bound_socket(dealer_fd,config.timeout_ms);
  Header received{};receive_exact(dealer_fd,received.data(),received.size());
  check(received==expected,"E20 delivery header binding");
  const auto d=padded(config.logical_n),b=bits_for(d);
  const auto slots=mul(config.iterations,pairs(d));
  const auto key_bytes=24U*b+57U;
  const auto record_bytes=kRecordHeaderBytes+key_bytes+kTagBytes;
  const auto file_bytes=add(kHeaderBytes,mul(slots,record_bytes));
  check(file_bytes<=static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()),
        "E20 file address range");
  const int directory=private_dir(private_directory);
  const auto final=file_name(config,minimal),partial=final+".partial";
  int fd=::openat(directory,partial.c_str(),
                  O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
  if(fd<0) {::close(directory);throw std::runtime_error("E20 new material file");}
  ProtocolIAav86StreamedPartyMaterial result;
  try {
    result.fd_=fd;result.header_=expected;
    result.key_bytes_=key_bytes;result.record_bytes_=record_bytes;
    result.read_slots_.assign(slots,false);
    random_bytes(result.key_.data(),result.key_.size());
    check(::posix_fallocate(fd,0,static_cast<off_t>(file_bytes))==0,
          "E20 material allocation");
    check(::lseek(fd,0,SEEK_SET)==0,"E20 material seek");
    write_exact(fd,expected.data(),expected.size());
    std::vector<std::uint8_t> raw(key_bytes);
    for(std::uint32_t t=0;t<config.iterations;++t)
      for(std::uint32_t a=0;a<d;++a)
        for(std::uint32_t c=a+1U;c<d;++c) {
          const auto slot=edge_slot(d,config.iterations,t,a,c);
          const auto id=config.material_id+slot;
          receive_exact(dealer_fd,raw.data(),raw.size());
          auto key=ProtocolIUcmpPartyMaterial::deserialize(raw);
          check(key.party_id()==config.party&&key.comparison_bits()==b,
                "E20 delivered key shape");
          const auto record=record_for(slot,id,t,a,c,config.party);
          const auto encrypted=seal(result.key_,expected,record,slot,raw);
          write_exact(fd,record.data(),record.size());
          write_exact(fd,encrypted.data(),encrypted.size());
        }
    std::array<std::uint8_t,4> size{};receive_exact(dealer_fd,size.data(),size.size());
    std::uint32_t base_size=0;
    for(auto byte:size) base_size=(base_size<<8U)|byte;
    check(base_size>=64&&base_size<=16U*1024U*1024U,"E20 base package length");
    std::vector<std::uint8_t> base_bytes(base_size);
    receive_exact(dealer_fd,base_bytes.data(),base_bytes.size());
    result.base=minimal?clique_base_decode(base_bytes,config):
        protocol_i_aav86_stream_deserialize_base(base_bytes,config.party,config);
    struct stat actual{};
    check(::fstat(fd,&actual)==0&&actual.st_size==static_cast<off_t>(file_bytes),
          "E20 full material length");
    check(::fdatasync(fd)==0,"E20 material sync");
    check(::linkat(directory,partial.c_str(),directory,final.c_str(),0)==0,
          "E20 material already exists");
    check(::unlinkat(directory,partial.c_str(),0)==0&&::fsync(directory)==0,
          "E20 material publish sync");
    result.path_=private_directory+"/"+final;
    result.disk_bytes_=file_bytes;
    result.plaintext_payload_bytes_=add(
        add(mul(mul(2U,d),16U+840U),
            mul(d,minimal?8U:48U+8U*config.iterations)),
        mul(slots,24U*b+24U));
    ::close(directory);
    return result;
  } catch(...) {
    ::unlinkat(directory,partial.c_str(),0);
    ::close(directory);
    throw;
  }
}

ProtocolIAav86StreamedPartyMaterial protocol_i_aav86_stream_receive_party(
    const ProtocolIAav86SmallConfig& config,int dealer_fd,
    const std::string& private_directory) {
  return protocol_i_stream_receive_impl(config,dealer_fd,private_directory,false);
}
ProtocolIAav86StreamedPartyMaterial protocol_i_clique_minimal_receive_party(
    const ProtocolIAav86SmallConfig& config,int dealer_fd,
    const std::string& private_directory) {
  return protocol_i_stream_receive_impl(config,dealer_fd,private_directory,true);
}

ProtocolIUcmpPartyMaterial ProtocolIAav86StreamedPartyMaterial::read_edge(
    std::uint32_t iteration,std::uint32_t a,std::uint32_t c,
    std::uint64_t material_id) {
  check(fd_>=0&&claimed_,"E20 sealed store unclaimed/closed");
  struct stat actual{};
  check(::fstat(fd_,&actual)==0&&actual.st_size==static_cast<off_t>(disk_bytes_),
        "E20 sealed store length changed");
  Header on_disk{};
  pread_exact(fd_,on_disk.data(),on_disk.size(),0);
  check(on_disk==header_,"E20 sealed store header changed");
  const auto d=padded(base.logical_n);
  const auto slot=edge_slot(d,base.iterations,iteration,a,c);
  check(material_id==base.material_id+slot,"E20 edge material ID");
  check(slot<read_slots_.size()&&!read_slots_[slot],"E20 edge reused");
  read_slots_[slot]=true;
  const auto expected=record_for(slot,material_id,iteration,a,c,base.party);
  const auto offset=add(kHeaderBytes,mul(slot,record_bytes_));
  check(add(offset,record_bytes_)<=disk_bytes_,"E20 edge offset");
  RecordHeader received{};
  pread_exact(fd_,received.data(),received.size(),offset);
  check(received==expected,"E20 edge record binding");
  std::vector<std::uint8_t> encrypted(key_bytes_+kTagBytes);
  pread_exact(fd_,encrypted.data(),encrypted.size(),offset+kRecordHeaderBytes);
  auto plain=open_record(key_,header_,received,slot,encrypted);
  auto key=ProtocolIUcmpPartyMaterial::deserialize(plain);
  check(key.party_id()==base.party&&key.comparison_bits()==base.comparison_bits,
        "E20 decoded key binding");
  return key;
}

void ProtocolIAav86StreamedPartyMaterial::claim(
    const ProtocolIAav86SmallConfig& config) {
  check(!claimed_&&config.session==base.session&&
            config.fingerprint==base.fingerprint&&
            config.material_id==base.material_id&&
            config.logical_n==base.logical_n&&config.k==base.k&&
            config.iterations==base.iterations&&config.party==base.party,
        "E20 claim binding/reuse");
  protocol_i_aav86_stream_claim(config);
  claimed_=true;
}

ProtocolIAav86SmallOutput protocol_i_aav86_stream_party_from_store(
    const ProtocolIAav86SmallConfig& config,
    ProtocolIAav86StreamedPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int,2>& score_fds,
    const std::vector<int>& core_fds,int inverse_fd) {
  auto read=[&](std::uint32_t t,std::uint32_t a,std::uint32_t c,std::uint64_t id) {
    return material.read_edge(t,a,c,id);
  };
  // The secure core makes the durable claim before its first callback.
  material.claimed_=true;
  return protocol_i_aav86_stream_party(config,std::move(material.base),read,
                                       raw_score_share,score_fds,core_fds,inverse_fd);
}
} // namespace moe_topk
