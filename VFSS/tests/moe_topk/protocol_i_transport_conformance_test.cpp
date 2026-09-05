#include <moe_topk/protocol_i_transport.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdexcept>
using namespace moe_topk; namespace {void ok(bool x){if(!x)throw std::runtime_error("transport conformance");} ProtocolIFrameConfig c(int s,int r){return {7,9,5,3,36,(std::uint8_t)s,(std::uint8_t)r,1,1};}}
int main(){int fd[2];ok(socketpair(AF_UNIX,SOCK_STREAM,0,fd)==0);auto p=fork();ok(p>=0);if(!p){close(fd[0]);try{ProtocolIFramedChannel x(fd[1],c(1,0));ok(x.receive()==std::vector<std::uint8_t>({1,2}));x.reset_counters();x.send({3});_exit(x.sent_bytes()>0?0:2);}catch(...){_exit(3);}}close(fd[1]);ProtocolIFramedChannel x(fd[0],c(0,1));x.send({1,2});x.reset_counters();ok(x.receive()==std::vector<std::uint8_t>({3}));ok(x.sent_bytes()==0&&x.received_bytes()>0);int st;ok(waitpid(p,&st,0)==p&&WIFEXITED(st)&&WEXITSTATUS(st)==0);return 0;}
