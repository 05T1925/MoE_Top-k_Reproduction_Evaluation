#include <moe_topk/protocol_i_aav86_streamed_store.h>
#include <moe_topk/protocol_i_permutation.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool okay,const char* text) {
  if(!okay) throw std::runtime_error(text);
}
template<class F> void rejects(F&& action,const char* text) {
  bool rejected=false;
  try {action();} catch(const std::exception&) {rejected=true;}
  require(rejected,text);
}
std::string directory() {
  std::string pattern="/tmp/m6a20-store-XXXXXX";
  std::vector<char> writable(pattern.begin(),pattern.end());writable.push_back(0);
  require(::mkdtemp(writable.data())!=nullptr,"E20 mkdtemp");
  return writable.data();
}
struct Bundle {
  Bundle()=default;
  Bundle(const Bundle&)=delete;
  Bundle& operator=(const Bundle&)=delete;
  Bundle(Bundle&& other) noexcept:
      c0(std::move(other.c0)),c1(std::move(other.c1)),
      p0(std::move(other.p0)),p1(std::move(other.p1)),
      dir0(std::exchange(other.dir0,{})),
      dir1(std::exchange(other.dir1,{})) {}
  ProtocolIAav86SmallConfig c0,c1;
  ProtocolIAav86StreamedPartyMaterial p0,p1;
  std::string dir0,dir1;
  ~Bundle() {
    if(!dir0.empty()) std::filesystem::remove_all(dir0);
    if(!dir1.empty()) std::filesystem::remove_all(dir1);
  }
};
Bundle make_bundle(std::uint32_t n,std::uint32_t r,std::uint64_t serial) {
  Bundle out;
  out.dir0=directory();out.dir1=directory();
  out.c0={0x120000+serial,0x130000+serial,0x140000+serial,
          n,1,r,0,5000,out.dir0};
  out.c1=out.c0;out.c1.party=1;out.c1.durable_claim_directory=out.dir1;
  std::array<int,2> p0{},p1{};
  require(::socketpair(AF_UNIX,SOCK_STREAM,0,p0.data())==0&&
          ::socketpair(AF_UNIX,SOCK_STREAM,0,p1.data())==0,"E20 sockets");
  std::exception_ptr e0,e1,et;
  std::thread receiver0([&] {
    try {out.p0=protocol_i_aav86_stream_receive_party(out.c0,p0[1],out.dir0);}
    catch(...) {e0=std::current_exception();}
    ::close(p0[1]);
  });
  std::thread receiver1([&] {
    try {out.p1=protocol_i_aav86_stream_receive_party(out.c1,p1[1],out.dir1);}
    catch(...) {e1=std::current_exception();}
    ::close(p1[1]);
  });
  std::thread dealer([&] {
    try {protocol_i_aav86_stream_dealer_send(out.c0,p0[0],p1[0]);}
    catch(...) {et=std::current_exception();}
    ::close(p0[0]);::close(p1[0]);
  });
  dealer.join();receiver0.join();receiver1.join();
  if(et) std::rethrow_exception(et);
  if(e0) std::rethrow_exception(e0);
  if(e1) std::rethrow_exception(e1);
  return out;
}
void flip(const std::string& path,std::uint64_t offset) {
  const int fd=::open(path.c_str(),O_RDWR|O_NOFOLLOW);
  require(fd>=0,"E20 tamper open");
  std::uint8_t value=0;
  require(::pread(fd,&value,1,offset)==1,"E20 tamper read");
  value^=1U;
  require(::pwrite(fd,&value,1,offset)==1,"E20 tamper write");
  ::close(fd);
}
void swap_ciphertext(const std::string& path,std::uint64_t one,
                     std::uint64_t two,std::size_t bytes) {
  const int fd=::open(path.c_str(),O_RDWR|O_NOFOLLOW);
  require(fd>=0,"E20 swap open");
  std::vector<std::uint8_t> a(bytes),b(bytes);
  require(::pread(fd,a.data(),bytes,one)==static_cast<ssize_t>(bytes)&&
          ::pread(fd,b.data(),bytes,two)==static_cast<ssize_t>(bytes),
          "E20 swap read");
  require(::pwrite(fd,b.data(),bytes,one)==static_cast<ssize_t>(bytes)&&
          ::pwrite(fd,a.data(),bytes,two)==static_cast<ssize_t>(bytes),
          "E20 swap write");
  ::close(fd);
}
} // namespace
int main() {
  try {
    auto b=make_bundle(15,2,1);
    const auto d=b.p0.base.padded_n;
    const auto bits=b.p0.base.comparison_bits;
    const auto mask=(UINT64_C(1)<<bits)-1U;
    require(d==16&&b.p1.base.padded_n==d,"E20 padding");
    const auto pi0=protocol_i_compose_permutation(
        b.p0.base.forward_tau,b.p1.base.forward_sigma);
    const auto pi1=protocol_i_compose_permutation(
        b.p1.base.forward_tau,b.p0.base.forward_sigma);
    require(pi0==pi1,"E20 shared permutation algebra");
    rejects([&]{(void)b.p0.read_edge(0,0,1,b.c0.material_id);},
            "E20 unclaimed read accepted");
    auto bad=b.c0;++bad.session;
    rejects([&]{b.p0.claim(bad);},"E20 wrong session claim accepted");
    bad=b.c0;bad.party=1;
    rejects([&]{b.p0.claim(bad);},"E20 wrong party claim accepted");
    b.p0.claim(b.c0);b.p1.claim(b.c1);
    rejects([&]{b.p0.claim(b.c0);},"E20 repeated claim accepted");
    const auto full0=(b.p0.base.node_mask_shares[0]+
                      b.p1.base.node_mask_shares[0])&mask;
    const auto full1=(b.p0.base.node_mask_shares[1]+
                      b.p1.base.node_mask_shares[1])&mask;
    auto k0=b.p0.read_edge(0,0,1,b.c0.material_id);
    auto k1=b.p1.read_edge(0,0,1,b.c1.material_id);
    const auto z0=(UINT64_C(3)+full0)&mask,z1=(UINT64_C(7)+full1)&mask;
    require(k0.eval_strict_lt(z0,z1)+k1.eval_strict_lt(z0,z1)==1U,
            "E20 dealer key/mask consistency");
    rejects([&]{(void)b.p0.read_edge(0,0,1,b.c0.material_id);},
            "E20 repeated edge read accepted");
    rejects([&]{(void)b.p0.read_edge(0,0,0,b.c0.material_id);},
            "E20 noncanonical pair accepted");
    rejects([&]{(void)b.p0.read_edge(0,0,2,b.c0.material_id);},
            "E20 wrong edge ID accepted");
    const auto slots=2U*16U*15U/2U;
    const auto record=(b.p0.disk_bytes()-80U)/slots;
    flip(b.p0.path(),80U+record+32U);
    rejects([&]{(void)b.p0.read_edge(0,0,2,b.c0.material_id+1U);},
            "E20 ciphertext tamper accepted");
    flip(b.p0.path(),0);
    rejects([&]{(void)b.p0.read_edge(0,0,3,b.c0.material_id+2U);},
            "E20 header tamper accepted");
    auto swapped=make_bundle(16,2,2);
    swapped.p0.claim(swapped.c0);
    const auto one_record=(swapped.p0.disk_bytes()-80U)/slots;
    swap_ciphertext(swapped.p0.path(),80U+32U,
                    80U+one_record+32U,one_record-32U);
    rejects([&]{(void)swapped.p0.read_edge(0,0,1,swapped.c0.material_id);},
            "E20 same-spec ciphertext swap accepted");
    auto truncated=make_bundle(16,2,3);
    truncated.p0.claim(truncated.c0);
    require(::truncate(truncated.p0.path().c_str(),80)==0,"E20 truncate fixture");
    rejects([&]{(void)truncated.p0.read_edge(0,0,1,truncated.c0.material_id);},
            "E20 truncated store accepted");
    std::array<int,2> failed{};
    require(::socketpair(AF_UNIX,SOCK_STREAM,0,failed.data())==0,
            "E20 failure socket");
    ::close(failed[0]);
    auto dir=directory();
    auto c=b.c0;c.session+=100;c.material_id+=100;c.durable_claim_directory=dir;
    rejects([&]{(void)protocol_i_aav86_stream_receive_party(c,failed[1],dir);},
            "E20 T close accepted");
    ::close(failed[1]);std::filesystem::remove_all(dir);
    std::cout<<"E20_STORE_CONFORMANCE_PASS algebra=1 binding=1 reuse=1 "
             <<"tamper=1 swap=1 truncate=1 t_close=1\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E20_STORE_CONFORMANCE_FAIL "<<e.what()<<"\n";
    return 1;
  }
}
