#include <moe_topk/experimental_bmw16_material_bundle.h>
#include <moe_topk/experimental_bmw16_select_party.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/topk_oracle.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
using namespace moe_topk;
constexpr uid_t kParty0Uid = 22012;
constexpr uid_t kParty1Uid = 22013;
constexpr std::size_t kChannelCount = 12;  // score2, fwd2, Select4, inverse2, final, coin
constexpr int kExitSuccess = 0;
constexpr int kExitAlgorithmAbort = 10;
constexpr int kExitMaterialAbort = 20;
constexpr int kExitCommunicationAbort = 30;
constexpr int kExitUnexpectedError = 70;

void require(bool ok, const char* what) { if (!ok) throw std::runtime_error(what); }
uid_t party_uid(int party) { return party == 0 ? kParty0Uid : kParty1Uid; }

struct Case { std::vector<std::int32_t> scores; std::uint32_t k; std::uint64_t seed; bool force_abort=false; };

ProtocolIBmw16ExperimentalPartyConfig config_for(const Case& test, int party) {
  const auto n=static_cast<std::uint32_t>(test.scores.size());
  std::uint32_t padded=2; while(padded<n)padded<<=1U;
  std::uint8_t index_bits=1; for(auto x=padded;x>2;x>>=1U)++index_bits;
  const auto session=UINT64_C(0x5331320000000000)+(test.seed<<8U)+(static_cast<std::uint64_t>(test.k)<<4U)+n;
  return {session,session^UINT64_C(0x5331328000000000),n,test.k,padded,index_bits,
      static_cast<std::uint8_t>(33U+index_bits),static_cast<std::uint8_t>(party),30000,
      test.seed,test.force_abort};
}

void write_all(int fd,const std::uint8_t* data,std::size_t size) {
  while(size){const auto n=::write(fd,data,size);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("fixture write");data+=n;size-=static_cast<std::size_t>(n);}
}
void write_file(const fs::path& path,const std::vector<std::uint8_t>& bytes,mode_t mode=0600) {
  const int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,mode);
  if(fd<0)throw std::runtime_error("fixture file create: "+path.string());
  try{write_all(fd,bytes.data(),bytes.size());if(::fdatasync(fd)!=0)throw std::runtime_error("fixture fdatasync");}
  catch(...){::close(fd);throw;}::close(fd);
}
void overwrite_file(const fs::path& path,const std::vector<std::uint8_t>& bytes) {
  const int fd=::open(path.c_str(),O_WRONLY|O_TRUNC|O_CLOEXEC|O_NOFOLLOW);
  if(fd<0)throw std::runtime_error("fixture file overwrite: "+path.string());
  try{write_all(fd,bytes.data(),bytes.size());if(::fdatasync(fd)!=0)throw std::runtime_error("fixture overwrite fdatasync");}
  catch(...){::close(fd);throw;}::close(fd);
}
void write_text_atomic(const fs::path& path,const std::string& text) {
  auto temp=path;temp += ".tmp";
  const int fd=::open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
  if(fd<0)throw std::runtime_error("result temp create");
  try{write_all(fd,reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
      if(::fdatasync(fd)!=0)throw std::runtime_error("result fdatasync");}
  catch(...){::close(fd);::unlink(temp.c_str());throw;}
  if(::close(fd)!=0){::unlink(temp.c_str());throw std::runtime_error("result close");}
  if(::rename(temp.c_str(),path.c_str())!=0){::unlink(temp.c_str());throw std::runtime_error("result atomic rename");}
  const int dir=::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  if(dir<0)throw std::runtime_error("result parent open");
  const bool synced=::fsync(dir)==0;::close(dir);if(!synced)throw std::runtime_error("result directory sync");
}
std::vector<std::uint8_t> read_file(const fs::path& path) {
  std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("fixture file read: "+path.string());
  return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
std::array<std::uint8_t,32> key_from(const fs::path& path) {
  const auto b=read_file(path);require(b.size()==32,"recipient key length");std::array<std::uint8_t,32> k{};std::copy(b.begin(),b.end(),k.begin());return k;
}
void put_u32(std::vector<std::uint8_t>& b,std::uint32_t x){for(int i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
std::uint32_t get_u32(const std::uint8_t* p){return static_cast<std::uint32_t>(p[0])|(static_cast<std::uint32_t>(p[1])<<8U)|(static_cast<std::uint32_t>(p[2])<<16U)|(static_cast<std::uint32_t>(p[3])<<24U);}
void random_bytes(std::uint8_t* out,std::size_t left){while(left){const auto n=::getrandom(out,left,0);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("fixture OS entropy");out+=n;left-=static_cast<std::size_t>(n);}}
std::vector<std::uint8_t> encode_key(const std::array<std::uint8_t,32>& k){return {k.begin(),k.end()};}
std::string self_path() {
  std::array<char,4096> p{};const auto n=::readlink("/proc/self/exe",p.data(),p.size()-1);if(n<=0)throw std::runtime_error("read executable path");return std::string(p.data(),static_cast<std::size_t>(n));
}
int wait_ok(pid_t pid) { int status=0;if(::waitpid(pid,&status,0)<0)throw std::runtime_error("waitpid");return WIFEXITED(status)?WEXITSTATUS(status):128; }
std::string text_field(const std::string& text,const std::string& key){std::istringstream in(text);std::string line;while(std::getline(in,line)){const auto p=line.find('=');if(p!=std::string::npos&&line.substr(0,p)==key)return line.substr(p+1);}return {};}
std::uint64_t number_field(const std::string& text,const std::string& key){const auto value=text_field(text,key);if(value.empty())throw std::runtime_error("missing metric: "+key);return std::stoull(value,nullptr,0);}
std::uint64_t peak_rss_kb() { struct rusage usage{};if(::getrusage(RUSAGE_SELF,&usage)!=0)throw std::runtime_error("getrusage");return static_cast<std::uint64_t>(usage.ru_maxrss); }
void write_party_result(const fs::path& dir,const std::string& status,const std::string& scope,
                        const std::string& state,const std::string& reason,int exit_code) {
  std::ostringstream out;out<<"status="<<status<<"\nscope="<<scope<<"\nstate="<<state
      <<"\nexit_code="<<exit_code<<"\nabort="<<reason<<"\nuid="<<::geteuid()<<"\n";
  write_text_atomic(dir/"result.txt",out.str());
}
int output_exit_code(const ProtocolIBmw16ExperimentalPartyOutput& out) {
  const std::string status=out.status?out.status:"";
  const std::string scope=out.abort_scope?out.abort_scope:"";
  if(status=="SUCCESS"&&scope=="NONE")return kExitSuccess;
  if(status=="ABORT_ALGORITHM_PROBABILITY"&&scope=="PEER_AGREED")return kExitAlgorithmAbort;
  if(status=="ABORT_MATERIAL"&&scope=="LOCAL_ONLY")return kExitMaterialAbort;
  if(status=="ABORT_COMMUNICATION"&&scope=="LOCAL_ONLY")return kExitCommunicationAbort;
  return kExitUnexpectedError;
}

void run_material_negative_tests() {
  Case c{{0,1},1,0x1299,false};const auto cfg0=config_for(c,0),cfg1=config_for(c,1);
  auto pair=protocol_i_bmw16_experimental_material_generate(cfg0);std::array<std::uint8_t,32> k0{},k1{};random_bytes(k0.data(),32);random_bytes(k1.data(),32);
  ProtocolIBmw16BundleStats st{};auto b0=protocol_i_bmw16_bundle_seal_party(cfg0,0,pair.party0,k0,&st);
  auto b1=protocol_i_bmw16_bundle_seal_party(cfg1,1,pair.party1,k1);
  auto expect_reject=[&](const char* label,auto fn){bool rejected=false;try{fn();}catch(...){rejected=true;}require(rejected,label);std::cout<<"negative="<<label<<" result=REJECTED\n";};
  auto valid=protocol_i_bmw16_bundle_open_party(cfg0,0,b0,k0);(void)valid;
  auto wrong=k0;wrong[0]^=0x80;expect_reject("wrong_recipient_key",[&]{(void)protocol_i_bmw16_bundle_open_party(cfg0,0,b0,wrong);});
  expect_reject("wrong_party_bundle",[&]{(void)protocol_i_bmw16_bundle_open_party(cfg1,1,b0,k0);});
  auto wrong_cfg=cfg0;++wrong_cfg.session;expect_reject("wrong_session",[&]{(void)protocol_i_bmw16_bundle_open_party(wrong_cfg,0,b0,k0);});
  auto short_bundle=b0;short_bundle.pop_back();expect_reject("truncated_aead_tag",[&]{(void)protocol_i_bmw16_bundle_open_party(cfg0,0,short_bundle,k0);});
  auto changed=b0;changed.back()^=1;expect_reject("ciphertext_or_slot_manifest_tamper",[&]{(void)protocol_i_bmw16_bundle_open_party(cfg0,0,changed,k0);});
  auto wrong_width=cfg0;++wrong_width.comparison_bits;expect_reject("wrong_comparison_width",[&]{(void)protocol_i_bmw16_bundle_open_party(wrong_width,0,b0,k0);});

  const auto claimdir=fs::path("/tmp")/("bmw16-s12-claim-"+std::to_string(::getpid()));
  require(::mkdir(claimdir.c_str(),0700)==0,"claim negative mkdir");
  {
    ProtocolIBmw16PersistentClaimStore store(claimdir.string(),cfg0.session,0);store.claim_bundle(st.manifest_sha256);
    expect_reject("duplicate_bundle_claim",[&]{store.claim_bundle(st.manifest_sha256);});
    store.claim_slot(7);expect_reject("duplicate_slot_claim",[&]{store.claim_slot(7);});
  }
  std::error_code ec;fs::remove_all(claimdir,ec);

  const auto crashdir=fs::path("/tmp")/("bmw16-s12-crash-claim-"+std::to_string(::getpid()));
  require(::mkdir(crashdir.c_str(),0700)==0,"crash claim mkdir");
  const pid_t child=::fork();if(child<0)throw std::runtime_error("claim crash fork");
  if(child==0){ProtocolIBmw16PersistentClaimStore store(crashdir.string(),cfg0.session,0);store.claim_slot(77);::_exit(0);}
  require(wait_ok(child)==0,"claim crash child");
  {ProtocolIBmw16PersistentClaimStore store(crashdir.string(),cfg0.session,0);expect_reject("claim_survives_process_exit",[&]{store.claim_slot(77);});}
  fs::remove_all(crashdir,ec);

  int closed_pair[2];require(::socketpair(AF_UNIX,SOCK_STREAM,0,closed_pair)==0,"closed peer socketpair");::close(closed_pair[1]);
  expect_reject("peer_closed_local_abort",[&]{ProtocolIFramedChannel ch(closed_pair[0],{cfg0.session,cfg0.fingerprint,cfg0.n,cfg0.k,cfg0.comparison_bits,0,1,19,9},100);(void)ch.receive();});::close(closed_pair[0]);
  int silent_pair[2];require(::socketpair(AF_UNIX,SOCK_STREAM,0,silent_pair)==0,"silent peer socketpair");
  expect_reject("peer_silent_local_abort",[&]{ProtocolIFramedChannel ch(silent_pair[0],{cfg0.session,cfg0.fingerprint,cfg0.n,cfg0.k,cfg0.comparison_bits,0,1,19,9},30);(void)ch.receive();});
  ::close(silent_pair[0]);::close(silent_pair[1]);
  (void)b1;
}

int run_dealer(int argc,char** argv) {
  // T gets public n/K/session metadata plus recipient delivery credentials, never input shares.
  require(argc==7,"dealer args");const fs::path root=argv[2];Case t{{},static_cast<std::uint32_t>(std::stoul(argv[4])),std::stoull(argv[5]),std::stoi(argv[6])!=0};
  t.scores.resize(static_cast<std::size_t>(std::stoul(argv[3])));std::array<ProtocolIBmw16ExperimentalPartyConfig,2> c{config_for(t,0),config_for(t,1)};
  if(c[0].n==1){std::ofstream report(root/"dealer.metrics",std::ios::trunc);report<<"singleton_no_material=true\ngeneration_us=0\nseal_us=0\ndelivery_us=0\noffline_total_us=0\npeak_rss_kb="<<peak_rss_kb()<<"\n";return 0;}
  const auto start=std::chrono::steady_clock::now();
  const auto generation_start=std::chrono::steady_clock::now();
  auto pair=protocol_i_bmw16_experimental_material_generate(c[0]);ProtocolIBmw16BundleStats st0,st1;
  const auto generation_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-generation_start).count();
  const auto seal_start=std::chrono::steady_clock::now();
  const auto p0=protocol_i_bmw16_bundle_seal_party(c[0],0,pair.party0,key_from(root/"p0"/"aead.key"),&st0);
  const auto p1=protocol_i_bmw16_bundle_seal_party(c[1],1,pair.party1,key_from(root/"p1"/"aead.key"),&st1);
  const auto seal_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-seal_start).count();
  const auto delivery_start=std::chrono::steady_clock::now();
  protocol_i_bmw16_bundle_write_atomic((root/"p0"/"material.bundle").string(),p0);
  protocol_i_bmw16_bundle_write_atomic((root/"p1"/"material.bundle").string(),p1);
  for(int p=0;p<2;++p){const auto uid=party_uid(p);if(::chown((root/ (p?"p1":"p0") /"material.bundle").c_str(),uid,uid)!=0)throw std::runtime_error("chown material bundle");}
  const auto delivery_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-delivery_start).count();
  const auto offline_total_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
  std::ofstream report(root/"dealer.metrics",std::ios::trunc);
  report<<"party0_bundle_bytes="<<st0.envelope_bytes<<"\nparty1_bundle_bytes="<<st1.envelope_bytes
        <<"\nparty0_plaintext_bytes="<<st0.plaintext_bytes<<"\nparty1_plaintext_bytes="<<st1.plaintext_bytes
        <<"\nparty0_manifest_bytes="<<st0.manifest_bytes<<"\nparty1_manifest_bytes="<<st1.manifest_bytes
        <<"\nparty0_ciphertext_bytes="<<st0.ciphertext_bytes<<"\nparty1_ciphertext_bytes="<<st1.ciphertext_bytes
        <<"\nslots_per_party="<<st0.slot_count<<"\naead_metadata_bytes_per_party="<<st0.authenticated_metadata_bytes
        <<"\ngeneration_us="<<generation_us<<"\nseal_us="<<seal_us<<"\ndelivery_us="<<delivery_us
        <<"\noffline_total_us="<<offline_total_us<<"\npeak_rss_kb="<<peak_rss_kb()<<"\n";
  report.close();
  return 0;
}

int run_party(int argc,char** argv) {
  require(argc==9+static_cast<int>(kChannelCount),"party args");const int party=std::stoi(argv[2]);const fs::path dir=argv[3];
  const auto n=static_cast<std::uint32_t>(std::stoul(argv[4]));const auto k=static_cast<std::uint32_t>(std::stoul(argv[5]));
  const auto seed=std::stoull(argv[6]);const bool force=std::stoi(argv[7])!=0;const std::string fault=argv[8+ kChannelCount];
  if(party<0||party>1||::geteuid()!=party_uid(party))throw std::runtime_error("party OS identity mismatch");
  const auto other=dir.parent_path()/(party==0?"p1":"p0");
  for(const char* name:{"aead.key","raw-share.bin","material.bundle"}){
    errno=0;const int probe=::open((other/name).c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(probe>=0){::close(probe);throw std::runtime_error("party read peer-private file unexpectedly succeeded");}
    if(errno!=EACCES&&errno!=EPERM)throw std::runtime_error("party peer-file isolation probe inconclusive");
  }
  std::array<int,kChannelCount> fd{};for(std::size_t i=0;i<fd.size();++i)fd[i]=std::stoi(argv[8+i]);
  if(fault=="unexpected_api")fd[11]=-1;  // Invalid API input must propagate to the process error contract.
  if(fault=="peer_closed"&&party==0&&::close(fd[0])!=0)throw std::runtime_error("TEST_ONLY close channel fault");
  Case test{{},k,seed,force};test.scores.resize(n);const auto c=config_for(test,party);
  ProtocolIBmw16ExperimentalPartyMaterial material;
  std::shared_ptr<ProtocolIBmw16PersistentClaimStore> claims;
  std::uint64_t bundle_open_us=0,bundle_claim_us=0,raw_share_read_us=0;
  if(n>1){
    try {
      const auto open_start=std::chrono::steady_clock::now();
      const auto envelope=protocol_i_bmw16_bundle_read((dir/"material.bundle").string());
      ProtocolIBmw16BundleStats stats;material=protocol_i_bmw16_bundle_open_party(c,party,envelope,key_from(dir/"aead.key"),&stats);
      bundle_open_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-open_start).count();
      const auto claim_start=std::chrono::steady_clock::now();
      claims=std::make_shared<ProtocolIBmw16PersistentClaimStore>((dir/"claims").string(),c.session,static_cast<std::uint8_t>(party));
      claims->claim_bundle(stats.manifest_sha256);
      bundle_claim_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-claim_start).count();
      material.durable_slot_claim=[claims](std::uint64_t id){
        try { claims->claim_slot(id); }
        catch(const std::exception& e) { throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,e.what()); }
      };
    } catch(const ProtocolIBmw16ExpectedFailure&) { throw; }
      catch(const std::exception& e) { throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,e.what()); }
  }
  std::vector<std::uint8_t> raw;
  const auto read_start=std::chrono::steady_clock::now();
  try { raw=read_file(dir/"raw-share.bin");if(raw.size()!=static_cast<std::size_t>(n)*4U)throw std::runtime_error("raw share file length"); }
  catch(const std::exception& e) { throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,e.what()); }
  raw_share_read_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-read_start).count();
  std::vector<std::uint32_t> share(n);for(std::size_t i=0;i<n;++i)share[i]=get_u32(raw.data()+4U*i);
  const std::array<int,2> score{fd[0],fd[1]},fwd{fd[2],fd[3]},inv{fd[8],fd[9]};
  const std::array<int,4> select{fd[4],fd[5],fd[6],fd[7]};
  auto out=protocol_i_bmw16_experimental_raw_score_mask_party(c,std::move(material),share,score,fwd,select,inv,fd[10],fd[11],nullptr);
  const int exit_code=output_exit_code(out);
  std::ofstream report(dir/"result.metrics",std::ios::trunc);report<<"status="<<out.status<<"\nscope="<<out.abort_scope
      <<"\nstate="<<(exit_code==kExitSuccess?"SUCCESS":exit_code==kExitAlgorithmAbort?"AGREED_ALGORITHM_ABORT":
          exit_code==kExitMaterialAbort?"LOCAL_MATERIAL_ABORT":exit_code==kExitCommunicationAbort?"LOCAL_COMMUNICATION_ABORT":"ALGORITHM_INVARIANT_ERROR")
      <<"\nexit_code="<<exit_code<<"\nabort="<<out.abort_reason<<"\nuid="<<::geteuid()<<"\n";
  const auto& m=out.metrics;report<<"logical="<<std::accumulate(m.logical_comparison_calls.begin(),m.logical_comparison_calls.end(),UINT64_C(0))
      <<"\nkey_eval="<<m.ucmp_party_evaluations<<"\ndcf_eval="<<m.dcf_party_evaluations
      <<"\nselect_unique="<<std::accumulate(m.unique_select_slots_consumed.begin(),m.unique_select_slots_consumed.end(),UINT64_C(0))
      <<"\nmembership_slots="<<m.membership_slots_consumed
      <<"\nmessage_phases="<<m.online_message_phases<<"\nsent="<<m.online_bytes_sent<<"\nrecv="<<m.online_bytes_received
      <<"\nclaimed_slots="<<m.durable_slots_claimed<<"\nsampler_prf_words="<<m.sampler_prf_words
      <<"\nonline_time_us="<<m.online_time_us<<"\nunused_cmpagg="<<m.forward_shuffle_cmpagg_slots_unused+m.inverse_shuffle_cmpagg_slots_unused<<"\n";
  report<<"bundle_open_us="<<bundle_open_us<<"\nbundle_claim_us="<<bundle_claim_us
      <<"\nraw_share_read_us="<<raw_share_read_us<<"\nraw_adapter_us="<<m.raw_adapter_time_us
      <<"\nraw_adapter_eval_us="<<m.raw_adapter_eval_time_us<<"\nraw_adapter_exchange_us="<<m.raw_adapter_exchange_time_us
      <<"\nforward_shuffle_us="<<m.forward_shuffle_time_us<<"\nmembership_us="<<m.membership_time_us
      <<"\nforward_shuffle_exchange_us="<<m.forward_shuffle_exchange_time_us
      <<"\nsampling_coin_exchange_us="<<m.sampling_coin_exchange_time_us
      <<"\nmembership_eval_us="<<m.membership_eval_time_us<<"\ninverse_shuffle_us="<<m.inverse_shuffle_time_us
      <<"\ninverse_shuffle_exchange_us="<<m.inverse_shuffle_exchange_time_us
      <<"\nstatus_coordination_us="<<m.status_coordination_time_us<<"\ndurable_slot_claim_us="<<m.durable_claim_time_us
      <<"\nslot_lookup_us="<<m.slot_lookup_time_us<<"\ndurable_claim_io_us="<<m.durable_claim_io_time_us
      <<"\npeak_rss_kb="<<peak_rss_kb()<<"\ninternal_fss_prg_calls=NOT_MEASURED\n";
  for(std::size_t r=0;r<4;++r)report<<"round"<<(r+1)<<"_time_us="<<m.select_round_time_us[r]
      <<"\nround"<<(r+1)<<"_eval_us="<<m.select_eval_time_us[r]
      <<"\nround"<<(r+1)<<"_exchange_us="<<m.select_exchange_time_us[r]<<"\n";
  for(std::size_t r=0;r<4;++r)report<<"round"<<(r+1)<<"_logical="<<m.logical_comparison_calls[r]
      <<"_unique="<<m.unique_select_slots_consumed[r]<<"_dummy="<<m.dummy_related_calls[r]
      <<"_repeat="<<m.repeated_logical_calls[r]<<"_fnv64="<<std::hex<<m.edge_plan_fnv64[r]<<std::dec<<"\n";
  report.close();require(static_cast<bool>(report),"write party metrics");
  if(std::string(out.status)=="SUCCESS")write_file(dir/"mask.share",out.xor_mask_share);
  write_party_result(dir,out.status,out.abort_scope,
      exit_code==kExitSuccess?"SUCCESS":exit_code==kExitAlgorithmAbort?"AGREED_ALGORITHM_ABORT":
          exit_code==kExitMaterialAbort?"LOCAL_MATERIAL_ABORT":exit_code==kExitCommunicationAbort?"LOCAL_COMMUNICATION_ABORT":"ALGORITHM_INVARIANT_ERROR",
      out.abort_reason,exit_code);
  return exit_code;
}

void make_private_dir(const fs::path& path,uid_t uid) {
  if(::mkdir(path.c_str(),0700)!=0)throw std::runtime_error("mkdir private party dir");
  if(::chown(path.c_str(),uid,uid)!=0)throw std::runtime_error("chown private party dir");
  if(::chmod(path.c_str(),0700)!=0)throw std::runtime_error("chmod private party dir");
  const auto claim=path/"claims";if(::mkdir(claim.c_str(),0700)!=0||::chown(claim.c_str(),uid,uid)!=0||::chmod(claim.c_str(),0700)!=0)throw std::runtime_error("create private claim dir");
}
void root_write_owned(const fs::path& path,const std::vector<std::uint8_t>& b,uid_t uid) {
  write_file(path,b);if(::chown(path.c_str(),uid,uid)!=0)throw std::runtime_error("chown private fixture file");
}

void run_case(const Case& test,const std::string& exe,const std::string& fault="none") {
  require(::geteuid()==0,"three-process E2E must run under WSL root to drop distinct UIDs");
  const auto root=fs::path("/tmp")/("bmw16-s12-"+std::to_string(::getpid())+"-"+std::to_string(test.seed));
  if(::mkdir(root.c_str(),0700)!=0)throw std::runtime_error("create root fixture dir");
  auto cleanup=std::unique_ptr<void,std::function<void(void*)>>(reinterpret_cast<void*>(1),[root](void*){std::error_code ec;fs::remove_all(root,ec);});
  if(::chmod(root.c_str(),0711)!=0)throw std::runtime_error("open fixture parent traversal");
  const auto p0=root/"p0",p1=root/"p1";make_private_dir(p0,kParty0Uid);make_private_dir(p1,kParty1Uid);
  std::array<std::uint8_t,32> key0{},key1{};random_bytes(key0.data(),32);random_bytes(key1.data(),32);
  root_write_owned(p0/"aead.key",encode_key(key0),kParty0Uid);root_write_owned(p1/"aead.key",encode_key(key1),kParty1Uid);

  // T is a separate exec and exits before test input shares are generated.
  const pid_t dealer=::fork();if(dealer<0)throw std::runtime_error("fork T");
  if(dealer==0){const auto n=std::to_string(test.scores.size());const auto k=std::to_string(test.k);const auto seed=std::to_string(test.seed);const std::string force=test.force_abort?"1":"0";::execl(exe.c_str(),exe.c_str(),"--dealer",root.c_str(),n.c_str(),k.c_str(),seed.c_str(),force.c_str(),nullptr);::_exit(127);}
  require(wait_ok(dealer)==0,"T process failed");
  if(fault=="corrupt_bundle"&&test.scores.size()>1){
    for(const auto& dir:{p0,p1}){auto bytes=read_file(dir/"material.bundle");require(!bytes.empty(),"bundle tamper fixture empty");bytes.back()^=0x40;overwrite_file(dir/"material.bundle",bytes);}
  }

  std::mt19937_64 rng(test.seed^UINT64_C(0x7231327368617265));std::array<std::vector<std::uint8_t>,2> shares;
  for(auto score:test.scores){const auto x=static_cast<std::uint32_t>(score),a=static_cast<std::uint32_t>(rng());put_u32(shares[0],a);put_u32(shares[1],x-a);}
  root_write_owned(p0/"raw-share.bin",shares[0],kParty0Uid);root_write_owned(p1/"raw-share.bin",shares[1],kParty1Uid);

  std::array<std::array<int,2>,kChannelCount> channels{};for(auto& pair:channels)if(::socketpair(AF_UNIX,SOCK_STREAM,0,pair.data())!=0)throw std::runtime_error("party socketpair");
  std::array<pid_t,2> children{};
  for(int party=0;party<2;++party){
    const pid_t pid=::fork();if(pid<0)throw std::runtime_error("fork party");children[party]=pid;
    if(pid==0){
      const int side=party;for(std::size_t i=0;i<channels.size();++i)for(int s=0;s<2;++s)if(!(s==side))::close(channels[i][s]);
      if(::setgroups(0,nullptr)!=0||::setgid(party_uid(party))!=0||::setuid(party_uid(party))!=0)::_exit(126);
      const auto ps=std::to_string(party);const auto ns=std::to_string(test.scores.size());const auto ks=std::to_string(test.k);const auto ss=std::to_string(test.seed);const std::string fsarg=test.force_abort?"1":"0";
      std::vector<std::string> args{exe,"--party",ps,(party?p1:p0).string(),ns,ks,ss,fsarg};
      for(const auto& pair:channels)args.push_back(std::to_string(pair[party]));
      args.push_back(fault);
      std::vector<char*> av;for(auto& a:args)av.push_back(a.data());av.push_back(nullptr);::execv(exe.c_str(),av.data());::_exit(127);
    }
  }
  for(auto& pair:channels)for(auto fd:pair)::close(fd);
  const int e0=wait_ok(children[0]),e1=wait_ok(children[1]);
  require(fs::exists(p0/"result.txt")&&fs::exists(p1/"result.txt"),"party structured result missing");
  const auto text0=read_file(p0/"result.txt"),text1=read_file(p1/"result.txt");
  const std::string r0(text0.begin(),text0.end()),r1(text1.begin(),text1.end());
  const auto metrics0_bytes=fs::exists(p0/"result.metrics")?read_file(p0/"result.metrics"):std::vector<std::uint8_t>{};
  const auto metrics1_bytes=fs::exists(p1/"result.metrics")?read_file(p1/"result.metrics"):std::vector<std::uint8_t>{};
  const std::string metrics0(metrics0_bytes.begin(),metrics0_bytes.end()),metrics1(metrics1_bytes.begin(),metrics1_bytes.end());
  std::ifstream dm(root/"dealer.metrics");std::string dealer_metrics((std::istreambuf_iterator<char>(dm)),{});
  const auto status0=text_field(r0,"status"),status1=text_field(r1,"status");
  const auto scope0=text_field(r0,"scope"),scope1=text_field(r1,"scope");
  const auto state0=text_field(r0,"state"),state1=text_field(r1,"state");
  const auto code0=static_cast<int>(number_field(r0,"exit_code")),code1=static_cast<int>(number_field(r1,"exit_code"));
  require(e0==code0&&e1==code1,"party exit code/result contract mismatch");
  require(status0==status1&&scope0==scope1&&state0==state1,"party status disagreement");
  const auto expected_code=[&](const std::string& status,const std::string& scope,const std::string& state){
    if(status=="SUCCESS"&&scope=="NONE"&&state=="SUCCESS")return kExitSuccess;
    if(status=="ABORT_ALGORITHM_PROBABILITY"&&scope=="PEER_AGREED"&&state=="AGREED_ALGORITHM_ABORT")return kExitAlgorithmAbort;
    if(status=="ABORT_MATERIAL"&&scope=="LOCAL_ONLY"&&state=="LOCAL_MATERIAL_ABORT")return kExitMaterialAbort;
    if(status=="ABORT_COMMUNICATION"&&scope=="LOCAL_ONLY"&&state=="LOCAL_COMMUNICATION_ABORT")return kExitCommunicationAbort;
    if(status=="ERROR_UNEXPECTED"&&scope=="PROCESS_ERROR"&&state=="UNEXPECTED_EXCEPTION")return kExitUnexpectedError;
    if(status=="ABORT_ALGORITHM_INVALID"&&scope=="PEER_AGREED"&&state=="ALGORITHM_INVARIANT_ERROR")return kExitUnexpectedError;
    throw std::runtime_error("invalid party structured state contract");
  };
  require(code0==expected_code(status0,scope0,state0),"party0 status/exit code classification");
  require(code1==expected_code(status1,scope1,state1),"party1 status/exit code classification");
  if(status0.rfind("ABORT_ALGORITHM_",0)==0)require(text_field(r0,"abort")==text_field(r1,"abort"),"algorithm abort reason disagreement");
  require(r0.find("uid=22012")!=std::string::npos&&r1.find("uid=22013")!=std::string::npos,"party UID not isolated");
  if(test.scores.size()>1&&(status0=="SUCCESS"||status0=="ABORT_ALGORITHM_PROBABILITY"||status0=="ABORT_ALGORITHM_INVALID")){
    require(number_field(metrics0,"dcf_eval")==2U*number_field(metrics0,"key_eval"),"party0 DCF/uCMP accounting");
    require(number_field(metrics1,"dcf_eval")==2U*number_field(metrics1,"key_eval"),"party1 DCF/uCMP accounting");
    require(number_field(metrics0,"sent")==number_field(metrics1,"recv")&&number_field(metrics1,"sent")==number_field(metrics0,"recv"),"cross-party byte accounting");
    const bool success=status0=="SUCCESS";require(number_field(metrics0,"message_phases")== (success?12U:10U),"phase accounting");
    const auto pair_count=static_cast<std::uint64_t>(test.scores.size())*(test.scores.size()-1U)/2U;
    const auto expected_claims=2U*config_for(test,0).padded_n+1U+number_field(metrics0,"select_unique")+
        (success?number_field(metrics0,"membership_slots"):0U)+(success?1U:0U);
    require(number_field(metrics0,"claimed_slots")==expected_claims,"durable slot accounting identity");
    const auto dslots=number_field(dealer_metrics,"slots_per_party");
    require(dslots==2U*config_for(test,0).padded_n+2U+11U*pair_count,"bundle slot coverage identity");
  }
  const bool mask0=fs::exists(p0/"mask.share"),mask1=fs::exists(p1/"mask.share");
  if(status0=="SUCCESS"){
    require(mask0&&mask1,"SUCCESS missing one or both party masks");
    const auto m0=read_file(p0/"mask.share"),m1=read_file(p1/"mask.share");require(m0.size()==test.scores.size()&&m1.size()==m0.size(),"mask share shape");
    std::vector<std::uint8_t> mask(m0.size());for(std::size_t i=0;i<mask.size();++i)mask[i]=m0[i]^m1[i];
    std::vector<std::uint32_t> words;for(auto x:test.scores)words.push_back(static_cast<std::uint32_t>(x));
    require(mask==top_k_mask(words,test.k),"frozen oracle differential");require(std::accumulate(mask.begin(),mask.end(),0U)==test.k,"mask weight");
  }else require(!mask0&&!mask1,"non-success published a mask");
  if(fault=="unexpected_api")require(e0==kExitUnexpectedError&&e1==kExitUnexpectedError&&
      status0=="ERROR_UNEXPECTED"&&status1=="ERROR_UNEXPECTED","unexpected exception was accepted as normal");
  if(fault=="peer_closed")require(e0==kExitCommunicationAbort&&e1==kExitCommunicationAbort&&
      status0=="ABORT_COMMUNICATION"&&status1=="ABORT_COMMUNICATION"&&
      scope0=="LOCAL_ONLY"&&scope1=="LOCAL_ONLY","peer close was reported as bilateral agreement");
  if(fault=="corrupt_bundle")require(e0==kExitMaterialAbort&&e1==kExitMaterialAbort&&
      status0=="ABORT_MATERIAL"&&status1=="ABORT_MATERIAL","bundle tamper did not produce typed material abort");
  std::cout<<"case n="<<test.scores.size()<<" K="<<test.k<<" seed="<<test.seed<<" status="<<status0
      <<" scope="<<scope0<<" state="<<state0<<" p0_exit="<<e0<<" p1_exit="<<e1
      <<" fault="<<fault<<" forced_abort="<<(test.force_abort?1:0)<<" p0_uid=22012 p1_uid=22013"
      <<" party0_metrics={"<<metrics0<<"} party1_metrics={"<<metrics1<<"}"
      <<" dealer={"<<dealer_metrics.substr(0,dealer_metrics.find_last_not_of('\n')+1)<<"}\n";
  // Deliberately retain only metrics and digests in /tmp; never print shares or keys.
  std::error_code ec;fs::remove_all(root,ec);
}
}

int main(int argc,char** argv) {
  try {
    if(argc>1&&std::string(argv[1])=="--dealer")return run_dealer(argc,argv);
    if(argc>1&&std::string(argv[1])=="--party"){
      try { return run_party(argc,argv); }
      catch(const ProtocolIBmw16ExpectedFailure& e) {
        if(argc>=4)write_party_result(fs::path(argv[3]),"ABORT_MATERIAL","LOCAL_ONLY","LOCAL_MATERIAL_ABORT",e.what(),kExitMaterialAbort);
        std::cerr<<"party-local-material-abort\n";return kExitMaterialAbort;
      }
      catch(const ProtocolITransportError& e) {
        if(argc>=4)write_party_result(fs::path(argv[3]),"ABORT_COMMUNICATION","LOCAL_ONLY","LOCAL_COMMUNICATION_ABORT",e.what(),kExitCommunicationAbort);
        std::cerr<<"party-local-communication-abort\n";return kExitCommunicationAbort;
      }
      catch(const std::exception& e) {
        if(argc>=4)write_party_result(fs::path(argv[3]),"ERROR_UNEXPECTED","PROCESS_ERROR","UNEXPECTED_EXCEPTION",e.what(),kExitUnexpectedError);
        std::cerr<<"party-unexpected-error="<<e.what()<<"\n";return kExitUnexpectedError;
      }
      catch(...) {
        if(argc>=4)write_party_result(fs::path(argv[3]),"ERROR_UNEXPECTED","PROCESS_ERROR","UNEXPECTED_EXCEPTION","non-standard exception",kExitUnexpectedError);
        std::cerr<<"party-unexpected-error=non-standard exception\n";return kExitUnexpectedError;
      }
    }
    require(argc==1,"no arguments expected");const auto exe=self_path();
    run_material_negative_tests();
    run_case({{INT32_MIN},1,1201,false},exe);
    run_case({{INT32_MIN,INT32_MAX},1,1202,false},exe);
    run_case({{INT32_MIN,INT32_MAX},2,1208,false},exe);
    run_case({{INT32_MIN,0,INT32_MAX},1,1209,false},exe);
    run_case({{7,-3,7},2,1203,false},exe);
    run_case({{7,-3,7},3,1210,false},exe);
    run_case({{4,-9,4,INT32_MAX},1,1211,false},exe);
    run_case({{4,-9,4,INT32_MAX},4,1212,false},exe);
    run_case({{INT32_MIN,INT32_MAX,0,-1,INT32_MAX},3,1204,false},exe);
    run_case({{INT32_MIN,INT32_MAX,0,-1,INT32_MAX},1,1213,false},exe);
    run_case({{INT32_MIN,INT32_MAX,0,-1,INT32_MAX},5,1214,false},exe);
    run_case({{9,9,-2,-2,0,INT32_MIN},3,1215,false},exe);
    run_case({{9,9,9,9,9,9,9,9},4,1205,false},exe);
    run_case({{9,9,9,9,9,9,9,9},1,1216,false},exe);
    run_case({{9,9,9,9,9,9,9,9},8,1217,false},exe);
    run_case({{5,-6,17,3,-20,0,2},7,1218,false},exe);
    std::vector<std::int32_t> scores64;for(int i=0;i<64;++i)scores64.push_back((i*17)%23-11);
    run_case({scores64,8,1206,false},exe);
    run_case({{12,1,9,-4,2},2,1207,true},exe);
    run_case({{12,1,9,-4,2},2,1219,false},exe,"corrupt_bundle");
    run_case({{12,1,9,-4,2},2,1220,false},exe,"unexpected_api");
    run_case({{12,1,9,-4,2},2,1221,false},exe,"peer_closed");
    // Exhaust every requested K for each small finite size with a strict total
    // order. Separate cases get fresh T material and independent party execs.
    for(std::uint32_t n=2;n<=8;++n){
      std::vector<std::int32_t> ordered;ordered.reserve(n);
      for(std::uint32_t i=0;i<n;++i)ordered.push_back(static_cast<std::int32_t>(13U*i)-31);
      for(std::uint32_t k=1;k<=n;++k)
        run_case({ordered,k,UINT64_C(200000)+100U*n+k,false},exe);
    }
    return 0;
  }catch(const std::exception& e){std::cerr<<"BMW16 S12 3-process TEST_ONLY failure: "<<e.what()<<'\n';return 1;}
}
