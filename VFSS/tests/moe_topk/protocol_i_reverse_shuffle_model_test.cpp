// TEST_ONLY: algebraic model; not a secure shuffle or runtime primitive.
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>
static std::vector<uint64_t> T(const std::vector<size_t>& p,const std::vector<uint64_t>& x){std::vector<uint64_t> y(x.size());for(size_t i=0;i<x.size();++i)y[i]=x[p[i]];return y;}
static std::vector<size_t> inv(const std::vector<size_t>& p){std::vector<size_t> q(p.size());for(size_t i=0;i<p.size();++i)q[p[i]]=i;return q;}
int main(){std::mt19937_64 r(7);for(size_t n:{size_t(1),3,5,8}){std::vector<size_t>a(n),b(n);std::iota(a.begin(),a.end(),0);std::iota(b.begin(),b.end(),0);std::shuffle(a.begin(),a.end(),r);std::shuffle(b.begin(),b.end(),r);std::vector<uint64_t>x(n);for(size_t i=0;i<n;++i)x[i]=i&1;auto z=T(b,T(a,x));auto y=T(inv(a),T(inv(b),z));if(y!=x)throw std::runtime_error("reverse model");}return 0;}
