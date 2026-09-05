#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace moe_topk {
using ProtocolIPermutation = std::vector<std::uint32_t>;
void protocol_i_validate_permutation(const ProtocolIPermutation& permutation);
ProtocolIPermutation protocol_i_identity_permutation(std::uint32_t n);
ProtocolIPermutation protocol_i_inverse_permutation(const ProtocolIPermutation& permutation);
// apply(pi,x)[i]=x[pi[i]]; compose(outer,inner) applies inner then outer.
ProtocolIPermutation protocol_i_compose_permutation(const ProtocolIPermutation& outer, const ProtocolIPermutation& inner);
template <typename T> std::vector<T> protocol_i_apply_permutation(const ProtocolIPermutation& pi, const std::vector<T>& values) {
  protocol_i_validate_permutation(pi); if (pi.size()!=values.size()) throw std::invalid_argument("permutation input shape"); std::vector<T> out(values.size()); for(std::size_t i=0;i<pi.size();++i) out[i]=values[pi[i]]; return out;
}
ProtocolIPermutation protocol_i_test_permutation(std::uint32_t n, std::uint64_t seed);
}  // namespace moe_topk
