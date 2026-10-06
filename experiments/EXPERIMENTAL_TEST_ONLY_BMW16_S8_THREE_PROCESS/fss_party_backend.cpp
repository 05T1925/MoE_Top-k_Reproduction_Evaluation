// EXPERIMENTAL_TEST_ONLY: offline T material generator and party-local
// Protocol-I shuffle/uCMP endpoint. No production API or secure claim.
#include <FSS/prng.h>
#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_permutation.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/topk_oracle.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <sys/random.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;
using Slot = std::tuple<int, int, int, std::uint32_t, std::uint32_t>;

void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

void entropy(void* out, std::size_t n) {
  auto* p = static_cast<std::uint8_t*>(out);
  while (n) {
    const auto got = ::getrandom(p, n, 0);
    if (got < 0 && errno == EINTR) continue;
    require(got > 0, "getrandom failed");
    p += got; n -= static_cast<std::size_t>(got);
  }
}
std::uint64_t random64() { std::uint64_t x; entropy(&x, sizeof(x)); return x; }
std::uint64_t mask_for(int bits) { require(bits >= 34 && bits <= 53, "unsupported uCMP bits"); return (UINT64_C(1) << bits) - 1U; }
std::uint32_t bounded(std::uint32_t n) {
  require(n != 0, "zero random bound");
  const std::uint64_t limit = UINT64_MAX - (UINT64_MAX % n);
  std::uint64_t x; do { x = random64(); } while (x >= limit);
  return static_cast<std::uint32_t>(x % n);
}
ProtocolIPermutation random_perm(std::uint32_t n) {
  auto p = protocol_i_identity_permutation(n);
  for (std::uint32_t r = n; r > 1; --r) std::swap(p[r - 1], p[bounded(r)]);
  return p;
}
ProtocolIBlock192 random_record(int bits) { return {random64() & mask_for(bits), random64(), random64()}; }
std::vector<ProtocolIBlock192> random_records(std::uint32_t n, int bits) {
  std::vector<ProtocolIBlock192> out(n); for (auto& x : out) x = random_record(bits); return out;
}
std::vector<ProtocolIBlock192> addv(int bits, const std::vector<ProtocolIBlock192>& a,
                                    const std::vector<ProtocolIBlock192>& b) {
  require(a.size() == b.size(), "record add shape"); auto out = a;
  for (std::size_t i=0;i<out.size();++i) out[i]=protocol_i_parallel_record_add(bits,out[i],b[i]);
  return out;
}
std::vector<ProtocolIBlock192> subv(int bits, const std::vector<ProtocolIBlock192>& a,
                                    const std::vector<ProtocolIBlock192>& b) {
  require(a.size() == b.size(), "record sub shape"); auto out = a;
  for (std::size_t i=0;i<out.size();++i) out[i]=protocol_i_parallel_record_sub(bits,out[i],b[i]);
  return out;
}
void put32(Bytes& b, std::uint32_t x) { for(int s=24;s>=0;s-=8)b.push_back(static_cast<std::uint8_t>(x>>s)); }
void put64(Bytes& b, std::uint64_t x) { for(int s=56;s>=0;s-=8)b.push_back(static_cast<std::uint8_t>(x>>s)); }
void put_blob(Bytes& b, const Bytes& x) { put64(b,x.size()); b.insert(b.end(),x.begin(),x.end()); }
std::uint32_t get32(const Bytes& b,std::size_t& o){require(o+4<=b.size(),"truncated u32");std::uint32_t x=0;for(int i=0;i<4;++i)x=(x<<8)|b[o++];return x;}
std::uint64_t get64(const Bytes& b,std::size_t& o){require(o+8<=b.size(),"truncated u64");std::uint64_t x=0;for(int i=0;i<8;++i)x=(x<<8)|b[o++];return x;}
Bytes get_blob(const Bytes& b,std::size_t& o){const auto n=get64(b,o);require(n<=b.size()-o,"truncated blob");Bytes x(b.begin()+o,b.begin()+o+n);o+=n;return x;}
Bytes read_file(const std::string& path) { std::ifstream f(path,std::ios::binary); require(f.good(),"cannot read package"); return Bytes(std::istreambuf_iterator<char>(f),{}); }
void write_file(const std::string& path,const Bytes& b){std::ofstream f(path,std::ios::binary|std::ios::trunc);require(f.good(),"cannot write package");f.write(reinterpret_cast<const char*>(b.data()),b.size());require(f.good(),"package write failed");}

struct KeyEntry { Slot slot; ProtocolIUcmpPartyMaterial p0,p1; };
struct TargetPair { ProtocolIParallelShufflePartyMaterial p0,p1; std::vector<ProtocolIBlock192> full_r; };

TargetPair make_shuffle_for_pi(const ProtocolIParallelShuffleDealerConfig& cfg,
                               const ProtocolIPermutation& pi) {
  require(cfg.n>=2 && pi.size()==cfg.n,"inverse shuffle dimensions");
  const auto inv0=random_perm(cfg.n), inv1=random_perm(cfg.n);
  const auto sigma0=inv0, sigma1=inv1;
  const auto tau0=protocol_i_compose_permutation(pi,protocol_i_inverse_permutation(sigma1));
  const auto tau1=protocol_i_compose_permutation(pi,protocol_i_inverse_permutation(sigma0));
  const auto a0=random_records(cfg.n,cfg.comparison_bits),a1=random_records(cfg.n,cfg.comparison_bits);
  const auto h=random_records(cfg.n,cfg.comparison_bits),r0=random_records(cfg.n,cfg.comparison_bits),r1=random_records(cfg.n,cfg.comparison_bits);
  const std::vector<ProtocolIBlock192> zero(cfg.n);
  const auto e0=subv(cfg.comparison_bits,subv(cfg.comparison_bits,zero,protocol_i_apply_permutation(tau0,a1)),h);
  const auto e1=addv(cfg.comparison_bits,subv(cfg.comparison_bits,zero,protocol_i_apply_permutation(tau1,a0)),h);
  TargetPair out;
  auto init=[&](ProtocolIParallelShufflePartyMaterial& m,int party,const ProtocolIPermutation& s,
                const ProtocolIPermutation& t,const std::vector<ProtocolIBlock192>& a,
                const std::vector<ProtocolIBlock192>& e,const std::vector<ProtocolIBlock192>& r){
    m.session=cfg.session;m.fingerprint=cfg.fingerprint;m.material_id=cfg.material_id;m.n=cfg.n;m.k=cfg.k;
    m.comparison_bits=cfg.comparison_bits;m.party=party;m.sigma=s;m.tau=t;m.a=a;m.e=e;m.r_share=r;
  };
  init(out.p0,0,sigma0,tau0,a0,e0,r0);init(out.p1,1,sigma1,tau1,a1,e1,r1);
  out.full_r=addv(cfg.comparison_bits,r0,r1);
  for(std::uint32_t i=0;i<cfg.n;++i)for(std::uint32_t j=i+1;j<cfg.n;++j){
    ProtocolIUcmpMaterial key(cfg.comparison_bits,out.full_r[i].word0,out.full_r[j].word0);
    out.p0.edge_materials.push_back(key.export_party_material(0));
    out.p1.edge_materials.push_back(key.export_party_material(1));
  }
  return out;
}

Bytes encode_package(int party,int bits,std::uint32_t n,std::uint32_t k,
                     std::uint64_t session,std::uint64_t fingerprint,
                     const Bytes& first,const Bytes& inverse,const std::vector<KeyEntry>& keys) {
  Bytes out{'S','8','P','K',1,static_cast<std::uint8_t>(party),static_cast<std::uint8_t>(bits),0};
  put32(out,n);put32(out,k);put64(out,session);put64(out,fingerprint);put_blob(out,first);put_blob(out,inverse);put64(out,keys.size());
  for(const auto& e:keys){
    const auto [stage,round,task,left,right]=e.slot;
    out.push_back(static_cast<std::uint8_t>(stage));out.push_back(static_cast<std::uint8_t>(round));
    out.push_back(static_cast<std::uint8_t>(task));out.push_back(static_cast<std::uint8_t>(party));
    out.push_back(static_cast<std::uint8_t>(bits));out.push_back(0);put32(out,left);put32(out,right);
    put64(out,session^UINT64_C(0x8100000000000001));
    put_blob(out,(party==0?e.p0:e.p1).serialize());
  }
  return out;
}

int dealer(int argc,char**argv){
  require(argc==9,"dealer args: n k bits session fingerprint output0 output1");
  const auto n=static_cast<std::uint32_t>(std::stoul(argv[2])),k=static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto bits=std::stoi(argv[4]);const auto session=std::stoull(argv[5]),fp=std::stoull(argv[6]);
  require(n>=2&&n<=64&&k>=1&&k<=n&&bits>=34&&bits<=53,"S8 dealer finite domain");
  const ProtocolIParallelShuffleDealerConfig c{session,fp,session^UINT64_C(0x8100000000000001),n,k,static_cast<std::uint8_t>(bits)};
  auto first=protocol_i_parallel_shuffle_dealer_generate(c);
  const auto pi=protocol_i_compose_permutation(first.party0.tau,first.party1.sigma);
  const auto pi_inv=protocol_i_inverse_permutation(pi);
  auto ci=c;ci.material_id=session^UINT64_C(0x8200000000000001);
  auto inverse=make_shuffle_for_pi(ci,pi_inv);
  const auto first0=protocol_i_parallel_shuffle_serialize_material(first.party0);
  const auto first1=protocol_i_parallel_shuffle_serialize_material(first.party1);
  const auto inv0=protocol_i_parallel_shuffle_serialize_material(inverse.p0);
  const auto inv1=protocol_i_parallel_shuffle_serialize_material(inverse.p1);
  std::vector<KeyEntry> entries;
  for(int task=0;task<2;++task)for(int round=1;round<=4;++round)
    for(std::uint32_t i=0;i<n;++i)for(std::uint32_t j=i+1;j<n;++j){
      const auto fullmask=mask_for(bits);
      ProtocolIUcmpMaterial key(bits,(first.party0.r_share[i].word0+first.party1.r_share[i].word0)&fullmask,
                                  (first.party0.r_share[j].word0+first.party1.r_share[j].word0)&fullmask);
      entries.push_back({Slot{0,round,task,i,j},key.export_party_material(0),key.export_party_material(1)});
    }
  for(std::uint32_t i=0;i<n;++i)for(std::uint32_t j=i+1;j<n;++j){
    const auto fullmask=mask_for(bits);
    ProtocolIUcmpMaterial key(bits,(first.party0.r_share[i].word0+first.party1.r_share[i].word0)&fullmask,
                                (first.party0.r_share[j].word0+first.party1.r_share[j].word0)&fullmask);
    entries.push_back({Slot{1,0,0,i,j},key.export_party_material(0),key.export_party_material(1)});
  }
  const auto p0=encode_package(0,bits,n,k,session,fp,first0,inv0,entries);
  const auto p1=encode_package(1,bits,n,k,session,fp,first1,inv1,entries);
  write_file(argv[7],p0);write_file(argv[8],p1);
  (void)::chmod(argv[7],S_IRUSR|S_IWUSR);(void)::chmod(argv[8],S_IRUSR|S_IWUSR);
  std::cout<<"T_OFFLINE_OK n="<<n<<" K="<<k<<" bits="<<bits<<" select_slots="<<8ULL*n*(n-1)/2
           <<" membership_slots="<<1ULL*n*(n-1)/2<<" shuffle_cmpagg_slots_per_party="<<2ULL*n*(n-1)/2
           <<" party0_bytes="<<p0.size()<<" party1_bytes="<<p1.size()<<"\n";
  return 0;
}

struct Loaded { int party=0,bits=0;std::uint32_t n=0,k=0;std::uint64_t session=0,fp=0;
  ProtocolIParallelShufflePartyMaterial fwd,inv;std::map<Slot,ProtocolIUcmpPartyMaterial> keys;std::set<Slot> used; };
Loaded load(int party,const std::string&path){
  const auto b=read_file(path);require(b.size()>=32&&b[0]=='S'&&b[1]=='8'&&b[2]=='P'&&b[3]=='K'&&b[4]==1&&b[5]==party,"party package header");
  std::size_t o=8;Loaded x;x.party=party;x.bits=b[6];x.n=get32(b,o);x.k=get32(b,o);x.session=get64(b,o);x.fp=get64(b,o);
  x.fwd=protocol_i_parallel_shuffle_deserialize_material(get_blob(b,o),party);
  x.inv=protocol_i_parallel_shuffle_deserialize_material(get_blob(b,o),party);
  const auto count=get64(b,o);require(count<=25000,"pool count");
  for(std::uint64_t i=0;i<count;++i){require(o+6<=b.size(),"slot header");int stage=b[o++],round=b[o++],task=b[o++];require(b[o++]==party&&b[o++]==x.bits&&b[o++]==0,"slot party/width binding");auto lo=get32(b,o),hi=get32(b,o);require(get64(b,o)==(x.session^UINT64_C(0x8100000000000001)),"slot mask binding");auto material=ProtocolIUcmpPartyMaterial::deserialize(get_blob(b,o));
    require(lo<hi&&hi<x.n&&material.party_id()==party&&material.comparison_bits()==x.bits,"slot binding");
    const Slot s{stage,round,task,lo,hi};require(x.keys.emplace(s,std::move(material)).second,"duplicate slot");}
  require(o==b.size()&&x.n==x.fwd.n&&x.n==x.inv.n&&x.bits==x.fwd.comparison_bits&&x.session==x.fwd.session&&x.fp==x.fwd.fingerprint,"package trailing/binding");return x;
}
void claim_file_once(const std::string& path) {
  const auto claim_path=path+".claimed";
  const int fd=::open(claim_path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,S_IRUSR|S_IWUSR);
  require(fd>=0,"offline party package already claimed or claim create failed");
  const char marker[]="BMW16_S8_TEST_ONLY_CONSUMED\n";
  const auto count=::write(fd,marker,sizeof(marker)-1);
  require(count==static_cast<ssize_t>(sizeof(marker)-1),"claim marker write failed");
  require(::fsync(fd)==0,"claim marker fsync failed");
  ::close(fd);
}
std::vector<ProtocolIBlock192> read_records(std::istringstream&in,std::uint32_t n){std::vector<ProtocolIBlock192>v(n);for(auto&x:v)require(static_cast<bool>(in>>x.word0>>x.word1>>x.word2),"record input");return v;}
void print_records(const char*tag,const std::vector<ProtocolIBlock192>&v){std::cout<<tag<<' '<<v.size();for(const auto&x:v)std::cout<<' '<<x.word0<<' '<<x.word1<<' '<<x.word2;std::cout<<'\n'<<std::flush;}
int endpoint(int argc,char**argv){
  require(argc==4,"endpoint args: party package");const int party=std::stoi(argv[2]);claim_file_once(argv[3]);auto x=load(party,argv[3]);
  ProtocolIParallelShufflePartyConfig cf{x.session,x.fp,x.fwd.material_id,x.n,x.k,static_cast<std::uint8_t>(x.bits),static_cast<std::uint8_t>(party),10000};
  std::unique_ptr<ProtocolIParallelShuffleParty> fwd,inv;std::string line;
  while(std::getline(std::cin,line)){if(line.empty())continue;std::istringstream in(line);std::string op;in>>op;
    if(op=="QUIT"){std::cout<<"Q\n"<<std::flush;break;}
    if(op=="FWD1"){std::uint32_t n;in>>n;require(n==x.n,"FWD1 n");auto v=read_records(in,n);fwd=std::make_unique<ProtocolIParallelShuffleParty>(cf,std::move(x.fwd));print_records("R",fwd->prepare_round1(v));}
    else if(op=="FWD2"){std::uint32_t n;in>>n;require(n==x.n&&fwd,"FWD2 n/state");print_records("R",fwd->receive_round1_prepare_round2(read_records(in,n)));}
    else if(op=="FWD3"){std::uint32_t n;in>>n;require(n==x.n&&fwd,"FWD3 n/state");auto r=fwd->receive_round2(read_records(in,n));print_records("F",r.shuffled_share);print_records("Z",r.public_masked_records);}
    else if(op=="CMP"||op=="MEM"){int round=0,task=0;std::uint32_t lo,hi;std::uint64_t zl,zr;if(op=="CMP")in>>round>>task>>lo>>hi>>zl>>zr;else{in>>lo>>hi>>zl>>zr;round=0;task=0;}require(static_cast<bool>(in),"comparison input");const Slot s{op=="CMP"?0:1,round,task,lo,hi};auto it=x.keys.find(s);require(it!=x.keys.end()&&x.used.insert(s).second,"missing or replayed key slot");std::cout<<"C "<<it->second.eval_strict_lt(zl,zr)<<'\n'<<std::flush;}
    else if(op=="INV1"){std::uint32_t n;in>>n;require(n==x.n,"INV1 n");auto v=read_records(in,n);auto ic=cf;ic.material_id=x.inv.material_id;inv=std::make_unique<ProtocolIParallelShuffleParty>(ic,std::move(x.inv));print_records("R",inv->prepare_round1(v));}
    else if(op=="INV2"){std::uint32_t n;in>>n;require(n==x.n&&inv,"INV2 n/state");print_records("R",inv->receive_round1_prepare_round2(read_records(in,n)));}
    else if(op=="INV3"){std::uint32_t n;in>>n;require(n==x.n&&inv,"INV3 n/state");auto r=inv->receive_round2(read_records(in,n));print_records("I",r.shuffled_share);}
    else throw std::runtime_error("unknown endpoint command");
  }
  return 0;
}
int oracle(int argc,char**argv){require(argc>=4,"oracle n k scores...");const auto n=std::stoul(argv[2]),k=std::stoul(argv[3]);require(argc==4+static_cast<int>(n),"oracle score count");std::vector<std::uint32_t>s(n);for(std::size_t i=0;i<n;++i)s[i]=static_cast<std::uint32_t>(std::stoll(argv[4+i]));auto m=top_k_mask(s,k);std::cout<<"O";for(auto b:m)std::cout<<' '<<static_cast<unsigned>(b);std::cout<<'\n';return 0;}
}

int main(int argc,char**argv){try{require(argc>=2,"missing role");const std::string role=argv[1];if(role=="--dealer")return dealer(argc,argv);if(role=="--party")return endpoint(argc,argv);if(role=="--oracle")return oracle(argc,argv);throw std::runtime_error("unknown role");}catch(const std::exception&e){std::cerr<<"S8_TEST_ONLY_BACKEND_ERROR: "<<e.what()<<'\n';return 2;}}
