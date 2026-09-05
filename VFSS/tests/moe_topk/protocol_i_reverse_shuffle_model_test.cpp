// TEST_ONLY: algebraic model; not a secure shuffle or runtime primitive.
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
namespace {
enum class Party { P0, P1 };
using Vector = std::vector<std::uint64_t>; using Permutation = std::vector<std::size_t>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Vector apply(const Permutation& p, const Vector& x) { require(p.size()==x.size(),"length"); Vector y(x.size()); std::vector<bool> seen(p.size()); for(size_t i=0;i<p.size();++i){require(p[i]<p.size()&&!seen[p[i]],"invalid permutation");seen[p[i]]=true;y[i]=x[p[i]];} return y; }
Permutation inverse(const Permutation& p) { Vector z(p.size()); (void)apply(p,z); Permutation q(p.size());for(size_t i=0;i<p.size();++i)q[p[i]]=i;return q; }
Vector add(const Vector&a,const Vector&b){require(a.size()==b.size(),"share length");Vector r(a.size());for(size_t i=0;i<a.size();++i)r[i]=a[i]+b[i];return r;}
struct IdealPs { std::mt19937_64 rng{0x4d325053}; std::unordered_set<std::uint64_t> used;
 std::pair<Vector,Vector> call(Party owner,const Permutation&p,const Vector&data,std::uint64_t material){(void)owner;require(used.insert(material).second,"material reuse");Vector owner_share(data.size());for(auto&v:owner_share)v=rng();Vector data_share(data.size());auto permuted=apply(p,data);for(size_t i=0;i<data.size();++i)data_share[i]=permuted[i]-owner_share[i];return {owner_share,data_share};}};
void run(size_t n, const Vector& mask) { Permutation p0(n),p1(n);std::iota(p0.begin(),p0.end(),0);p1=p0;std::mt19937 rng(static_cast<unsigned>(n));if(n>1){std::rotate(p0.begin(),p0.begin()+1,p0.end());std::reverse(p1.begin(),p1.end());} Vector x(n),x0(n),x1(n);for(size_t i=0;i<n;++i){x[i]=UINT64_MAX-i;x0[i]=UINT64_C(0xdeadbeef)+i;x1[i]=x[i]-x0[i];} IdealPs ps;
 auto [a0,a1]=ps.call(Party::P0,p0,x1,1); auto b0=add(apply(p0,x0),a0); auto [c1,c0]=ps.call(Party::P1,p1,b0,2); auto forward1=add(apply(p1,a1),c1); require(add(c0,forward1)==apply(p1,apply(p0,x)),"forward");
 Vector carrier0=apply(p1,apply(p0,mask)), carrier1(n); auto [e1,e0]=ps.call(Party::P1,inverse(p1),carrier0,3); auto f1=add(apply(inverse(p1),carrier1),e1); auto [g0,g1]=ps.call(Party::P0,inverse(p0),f1,4); auto h0=add(apply(inverse(p0),e0),g0); require(add(h0,g1)==mask,"reverse");for(size_t i=0;i<n;++i){require(mask[i]<=1,"bit");require(((h0[i]&1)^(g1[i]&1))==mask[i],"lsb");} bool reused=false;try{ps.call(Party::P0,p0,x,1);}catch(const std::runtime_error&){reused=true;}require(reused,"fresh material"); }
}
int main(){for(size_t n:{size_t(1),3,5,8,17}){run(n,Vector(n));run(n,Vector(n,1));Vector a(n);for(size_t i=0;i<n;++i)a[i]=i&1;run(n,a);}return 0;}
