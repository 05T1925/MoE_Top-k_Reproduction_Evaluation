#include <moe_topk/protocol_i_opv.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using moe_topk::ProtocolIBlock192;
using moe_topk::ProtocolIOpvConfig;
void require(bool v, const char* m) { if (!v) throw std::runtime_error(m); }
void close_fd(int& fd) { if (fd >= 0) { ::close(fd); fd = -1; } }
void pair(int x[2]) { require(::socketpair(AF_UNIX, SOCK_STREAM, 0, x) == 0, "socketpair"); }
void write_all(int fd, const void* data, std::size_t n) { auto p = static_cast<const std::uint8_t*>(data); while (n) { const auto k = ::write(fd,p,n); if(k<0&&errno==EINTR)continue; require(k>0,"write"); p+=k;n-=static_cast<std::size_t>(k); } }
void read_all(int fd, void* data, std::size_t n) { auto p = static_cast<std::uint8_t*>(data); while (n) { const auto k = ::read(fd,p,n); if(k<0&&errno==EINTR)continue; require(k>0,"read"); p+=k;n-=static_cast<std::size_t>(k); } }
void close_except(int a, int b, int c) { for(int fd=3;fd<256;++fd) if(fd!=a&&fd!=b&&fd!=c) ::close(fd); }
std::string self(const char* argv0) { char p[4096]{}; const auto n=::readlink("/proc/self/exe",p,sizeof(p)-1); if(n>0){p[n]=0;return p;} return argv0; }
pid_t spawn(const std::string& x, const std::vector<std::string>& v) { const auto p=::fork(); require(p>=0,"fork"); if(!p){std::vector<char*> a{const_cast<char*>(x.c_str())};for(const auto& s:v)a.push_back(const_cast<char*>(s.c_str()));a.push_back(nullptr);::execv(x.c_str(),a.data());_exit(127);}return p; }
void wait_ok(pid_t p) { int s{}; require(::waitpid(p,&s,0)==p&&WIFEXITED(s)&&WEXITSTATUS(s)==0,"role failure"); }
ProtocolIOpvConfig config(std::uint32_t t,std::uint32_t b,std::uint64_t m) { return {0x4d323900+t,0x6d323900+b,m,t,b,15000}; }
std::vector<std::string> role_args(const char* role,int peer,int result,int input,const ProtocolIOpvConfig& c) { return {std::string("--role=")+role,"--peer",std::to_string(peer),"--result",std::to_string(result),"--input",std::to_string(input),"--session",std::to_string(c.session),"--fingerprint",std::to_string(c.fingerprint),"--material",std::to_string(c.material_id),"--t",std::to_string(c.vector_length),"--batch",std::to_string(c.batch_count)}; }
ProtocolIOpvConfig parse(int argc,char** argv) { ProtocolIOpvConfig c{}; for(int i=0;i+1<argc;i+=2){const std::string k(argv[i]),v(argv[i+1]);if(k=="--session")c.session=std::stoull(v);else if(k=="--fingerprint")c.fingerprint=std::stoull(v);else if(k=="--material")c.material_id=std::stoull(v);else if(k=="--t")c.vector_length=std::stoul(v);else if(k=="--batch")c.batch_count=std::stoul(v);}c.timeout_ms=15000;return c; }
int role_main(int argc,char** argv) {
  try { require(argc==18,"role arguments"); const std::string role(argv[1]); require(role.rfind("--role=",0)==0,"role"); const int peer=std::stoi(argv[3]),result=std::stoi(argv[5]),input=std::stoi(argv[7]); const auto c=parse(argc-8,argv+8); close_except(peer,result,input);
    if(role=="--role=fvo"){const auto r=moe_topk::protocol_i_opv_full_vector_owner(c,peer);write_all(result,&r.counters,sizeof(r.counters));write_all(result,&r.chosen_ot_items,sizeof(r.chosen_ot_items));for(const auto& row:r.leaves)write_all(result,row.data(),row.size()*sizeof(ProtocolIBlock192));}
    else if(role=="--role=po"){std::vector<std::uint32_t> p(c.batch_count);read_all(input,p.data(),p.size()*sizeof(std::uint32_t)); const auto r=moe_topk::protocol_i_opv_puncture_owner(c,peer,p);write_all(result,&r.counters,sizeof(r.counters));write_all(result,&r.chosen_ot_items,sizeof(r.chosen_ot_items));for(const auto& row:r.leaves)for(const auto& v:row){const std::uint8_t ok=v.has_value();write_all(result,&ok,1);if(ok)write_all(result,&*v,sizeof(*v));}}
    else return 2; return 0;
  } catch (...) { return 1; }
}
void run_case(const std::string& exe,const ProtocolIOpvConfig& c,const std::vector<std::uint32_t>& punctures) {
  int ot[2]{},fr[2]{},pr[2]{},pin[2]{};pair(ot);pair(fr);pair(pr);pair(pin);
  auto fa=role_args("fvo",ot[0],fr[1],-1,c);auto pa=role_args("po",ot[1],pr[1],pin[1],c);
  const auto f=spawn(exe,fa); const auto p=spawn(exe,pa);
  close_fd(ot[0]);close_fd(ot[1]);close_fd(fr[1]);close_fd(pr[1]);close_fd(pin[1]);
  write_all(pin[0],punctures.data(),punctures.size()*sizeof(std::uint32_t));::shutdown(pin[0],SHUT_WR);
  moe_topk::ProtocolIChosenOtCounters fc{},pc{};std::uint64_t fi{},pi{};read_all(fr[0],&fc,sizeof(fc));read_all(fr[0],&fi,sizeof(fi));read_all(pr[0],&pc,sizeof(pc));read_all(pr[0],&pi,sizeof(pi));
  std::vector<ProtocolIBlock192> full(static_cast<std::size_t>(c.batch_count)*c.vector_length);read_all(fr[0],full.data(),full.size()*sizeof(ProtocolIBlock192));
  const auto expected=static_cast<std::uint64_t>(c.batch_count)*(c.vector_length==2?1:c.vector_length==4?2:c.vector_length==8?3:c.vector_length==16?4:5);require(fi==expected&&pi==expected,"OPV item count");require(fc.sent_bytes==pc.received_bytes&&fc.received_bytes==pc.sent_bytes,"OPV crossed counters");
  for(std::uint32_t i=0;i<c.batch_count;++i)for(std::uint32_t j=0;j<c.vector_length;++j){std::uint8_t ok{};read_all(pr[0],&ok,1);if(j==punctures[i])require(!ok,"puncture nullopt");else{require(ok,"known leaf");ProtocolIBlock192 got{};read_all(pr[0],&got,sizeof(got));require(got==full[static_cast<std::size_t>(i)*c.vector_length+j],"OPV differential");}}
  wait_ok(f);wait_ok(p);close_fd(fr[0]);close_fd(pr[0]);close_fd(pin[0]);close_fd(pin[1]);
}
void invalid_inputs(){int x[2]{};pair(x);bool threw=false;try{(void)moe_topk::protocol_i_opv_full_vector_owner({1,2,3,1,1,1},x[0]);}catch(...){threw=true;}require(threw,"T=1");threw=false;try{(void)moe_topk::protocol_i_opv_puncture_owner({1,2,4,3,1,1},x[0],{0});}catch(...){threw=true;}require(threw,"non-power T");threw=false;try{(void)moe_topk::protocol_i_opv_puncture_owner({1,2,5,2,2,1},x[0],{0});}catch(...){threw=true;}require(threw,"puncture count");close_fd(x[0]);close_fd(x[1]);}
}  // namespace
int main(int argc,char** argv){try{if(argc>1)return role_main(argc,argv);::signal(SIGPIPE,SIG_IGN);invalid_inputs();const auto exe=self(argv[0]);std::uint64_t material=1;for(const auto t:{2U,4U,8U,16U,32U})for(const auto b:{1U,3U,t}){std::vector<std::uint32_t> p(b);for(std::uint32_t i=0;i<b;++i)p[i]=(i*3+b-1)%t;run_case(exe,config(t,b,material++),p);}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
