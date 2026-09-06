#include <moe_topk/protocol_i_share_translation.h>

#include <cerrno>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using moe_topk::ProtocolIBlock192; using moe_topk::ProtocolIOpvConfig;
void require(bool v,const char* m){if(!v)throw std::runtime_error(m);} void close_fd(int& f){if(f>=0){::close(f);f=-1;}}
void pair(int a[2]){require(::socketpair(AF_UNIX,SOCK_STREAM,0,a)==0,"socketpair");}
void write_all(int f,const void* d,std::size_t n){auto p=static_cast<const std::uint8_t*>(d);while(n){const auto k=::write(f,p,n);if(k<0&&errno==EINTR)continue;require(k>0,"write");p+=k;n-=static_cast<std::size_t>(k);}}
void read_all(int f,void* d,std::size_t n){auto p=static_cast<std::uint8_t*>(d);while(n){const auto k=::read(f,p,n);if(k<0&&errno==EINTR)continue;require(k>0,"read");p+=k;n-=static_cast<std::size_t>(k);}}
void keep(int a,int b,int c){for(int f=3;f<256;++f)if(f!=a&&f!=b&&f!=c)::close(f);} std::string self(const char* a){char p[4096]{};const auto n=::readlink("/proc/self/exe",p,sizeof(p)-1);if(n>0){p[n]=0;return p;}return a;}
pid_t spawn(const std::string& e,const std::vector<std::string>& s){const auto p=::fork();require(p>=0,"fork");if(!p){std::vector<char*> a{const_cast<char*>(e.c_str())};for(const auto& x:s)a.push_back(const_cast<char*>(x.c_str()));a.push_back(nullptr);::execv(e.c_str(),a.data());_exit(127);}return p;}void wait_ok(pid_t p){int s{};require(::waitpid(p,&s,0)==p&&WIFEXITED(s)&&WEXITSTATUS(s)==0,"role failure");}
ProtocolIOpvConfig cfg(std::uint32_t t,std::uint64_t m){return {0x53540000+t,0x5354f000+t,m,t,1,15000};}
std::vector<std::string> args(const char* role,int peer,int out,int in,const ProtocolIOpvConfig& c){return {std::string("--role=")+role,"--peer",std::to_string(peer),"--out",std::to_string(out),"--in",std::to_string(in),"--session",std::to_string(c.session),"--fingerprint",std::to_string(c.fingerprint),"--material",std::to_string(c.material_id),"--t",std::to_string(c.vector_length)};}
ProtocolIOpvConfig parse(int ac,char** av){ProtocolIOpvConfig c{};for(int i=0;i+1<ac;i+=2){const std::string k(av[i]),v(av[i+1]);if(k=="--session")c.session=std::stoull(v);else if(k=="--fingerprint")c.fingerprint=std::stoull(v);else if(k=="--material")c.material_id=std::stoull(v);else if(k=="--t")c.vector_length=std::stoul(v);}c.batch_count=1;c.timeout_ms=15000;return c;}
int role_main(int ac,char** av){try{require(ac==16,"role args");const std::string r(av[1]);const int peer=std::stoi(av[3]),out=std::stoi(av[5]),in=std::stoi(av[7]);const auto c=parse(ac-8,av+8);keep(peer,out,in);if(r=="--role=fvo"){const auto x=moe_topk::protocol_i_share_translation_fvo(c,peer);write_all(out,&x.counters,sizeof(x.counters));write_all(out,&x.chosen_ot_items,sizeof(x.chosen_ot_items));write_all(out,x.a.data(),x.a.size()*sizeof(ProtocolIBlock192));write_all(out,x.b.data(),x.b.size()*sizeof(ProtocolIBlock192));}else if(r=="--role=po"){std::vector<std::uint32_t> pi(c.vector_length);read_all(in,pi.data(),pi.size()*sizeof(std::uint32_t));const auto x=moe_topk::protocol_i_share_translation_po(c,peer,pi);write_all(out,&x.counters,sizeof(x.counters));write_all(out,&x.chosen_ot_items,sizeof(x.chosen_ot_items));write_all(out,x.delta.data(),x.delta.size()*sizeof(ProtocolIBlock192));}else return 2;return 0;}catch(...){return 1;}}
ProtocolIBlock192 sub(ProtocolIBlock192 a,const ProtocolIBlock192& b){return {a.word0-b.word0,a.word1-b.word1,a.word2-b.word2};}
void run(const std::string& exe,const ProtocolIOpvConfig& c,const std::vector<std::uint32_t>& permutation){int ot[2]{},fo[2]{},po[2]{},input[2]{};pair(ot);pair(fo);pair(po);pair(input);const auto f=spawn(exe,args("fvo",ot[0],fo[1],-1,c));const auto p=spawn(exe,args("po",ot[1],po[1],input[1],c));close_fd(ot[0]);close_fd(ot[1]);close_fd(fo[1]);close_fd(po[1]);close_fd(input[1]);write_all(input[0],permutation.data(),permutation.size()*sizeof(std::uint32_t));::shutdown(input[0],SHUT_WR);moe_topk::ProtocolIChosenOtCounters fs{},ps{};std::uint64_t fi{},pi{};read_all(fo[0],&fs,sizeof(fs));read_all(fo[0],&fi,sizeof(fi));read_all(po[0],&ps,sizeof(ps));read_all(po[0],&pi,sizeof(pi));std::vector<ProtocolIBlock192>a(c.vector_length),b(c.vector_length),d(c.vector_length);read_all(fo[0],a.data(),a.size()*sizeof(*a.data()));read_all(fo[0],b.data(),b.size()*sizeof(*b.data()));read_all(po[0],d.data(),d.size()*sizeof(*d.data()));const auto depth=c.vector_length==2?1:c.vector_length==4?2:c.vector_length==8?3:4;require(fi==c.vector_length*depth&&pi==fi,"translation item count");require(fs.sent_bytes==ps.received_bytes&&fs.received_bytes==ps.sent_bytes,"translation counters");for(std::uint32_t i=0;i<c.vector_length;++i)require(d[i]==sub(b[i],a[permutation[i]]),"translation formula");wait_ok(f);wait_ok(p);close_fd(fo[0]);close_fd(po[0]);close_fd(input[0]);}
std::vector<std::uint32_t> permutation(std::uint32_t t,int mode){std::vector<std::uint32_t> p(t);std::iota(p.begin(),p.end(),0);if(mode==1)std::reverse(p.begin(),p.end());else if(mode==2)for(std::uint32_t i=0;i<t;++i)p[i]=(i+1)%t;else if(mode==3)for(std::uint32_t i=0;i+1<t;i+=2)std::swap(p[i],p[i+1]);else if(mode==4){std::mt19937 g(0x4d3239+t);std::shuffle(p.begin(),p.end(),g);}return p;}
void invalid(){int x[2]{};pair(x);bool bad=false;try{(void)moe_topk::protocol_i_share_translation_po({1,2,3,4,1,1},x[0],{0,0,2,3});}catch(...){bad=true;}require(bad,"duplicate permutation");bad=false;try{(void)moe_topk::protocol_i_share_translation_po({1,2,4,4,1,1},x[0],{0,1,2,4});}catch(...){bad=true;}require(bad,"range permutation");close_fd(x[0]);close_fd(x[1]);}
}  // namespace
int main(int ac,char** av){try{if(ac>1)return role_main(ac,av);::signal(SIGPIPE,SIG_IGN);invalid();const auto e=self(av[0]);std::uint64_t m=100;for(const auto t:{2U,4U,8U,16U})for(int mode=0;mode!=5;++mode)run(e,cfg(t,m++),permutation(t,mode));return 0;}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
