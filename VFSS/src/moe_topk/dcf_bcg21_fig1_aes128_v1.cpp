#include <moe_topk/dcf_bcg21_fig1_aes128_v1.h>

#include <openssl/aes.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace moe_topk::bcg21_fig1_aes128_v1 {
namespace {

constexpr std::size_t kHeaderBytes = 33;
constexpr std::size_t kSeedBytes = 16;
constexpr std::size_t kGroupBytes = 8;
constexpr std::size_t kBitsPerExpandedTuple = 4 * 128 + 2;

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

std::uint64_t input_mask(std::uint8_t bits) {
  require(bits >= 1 && bits <= 63, "BCG21 DCF input width");
  return (UINT64_C(1) << bits) - 1;
}

std::uint8_t bit_at(const std::array<std::uint8_t, 65>& stream,
                    std::size_t index) {
  return static_cast<std::uint8_t>(
      (stream[index / 8] >> (7U - static_cast<unsigned>(index % 8))) & 1U);
}

void set_bit(std::array<std::uint8_t, 65>& stream, std::size_t index,
             std::uint8_t value) {
  if (value != 0)
    stream[index / 8] |= static_cast<std::uint8_t>(
        1U << (7U - static_cast<unsigned>(index % 8)));
}

std::uint64_t read_bits_u64(const std::array<std::uint8_t, 65>& stream,
                            std::size_t& offset, std::size_t count) {
  require(count <= 64, "BCG21 DCF integer conversion width");
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < count; ++i)
    result = (result << 1U) | bit_at(stream, offset++);
  return result;
}

Seed read_seed(const std::array<std::uint8_t, 65>& stream,
               std::size_t& offset) {
  Seed result{};
  for (auto& byte : result) byte = static_cast<std::uint8_t>(read_bits_u64(stream, offset, 8));
  return result;
}

std::array<std::uint8_t, 16> encrypt_block(
    const Seed& key, const std::array<std::uint8_t, 16>& plaintext) {
  AES_KEY schedule{};
  if (AES_set_encrypt_key(key.data(), 128, &schedule) != 0)
    throw std::runtime_error("AES-128 key setup failed");
  std::array<std::uint8_t, 16> result{};
  AES_encrypt(plaintext.data(), result.data(), &schedule);
  OPENSSL_cleanse(&schedule, sizeof(schedule));
  return result;
}

// Project-specific PRG instantiation for BCG+21's 4*lambda+2-bit G:
// concatenate five AES-128 outputs at distinct fixed domain/counter blocks,
// then retain the first 514 bits in MSB-first order.
std::array<std::uint8_t, 65> expand(const Seed& seed) {
  std::array<std::uint8_t, 80> bytes{};
  for (std::uint8_t counter = 0; counter < 5; ++counter) {
    std::array<std::uint8_t, 16> input{};
    input[0] = 'B'; input[1] = 'C'; input[2] = 'G'; input[3] = '2';
    input[4] = '1'; input[5] = 'F'; input[6] = '1'; input[15] = counter;
    const auto block = encrypt_block(seed, input);
    std::copy(block.begin(), block.end(), bytes.begin() + 16U * counter);
  }
  std::array<std::uint8_t, 65> result{};
  std::copy_n(bytes.begin(), result.size(), result.begin());
  result.back() &= 0xFCU;  // exactly 514 significant MSB-first bits.
  OPENSSL_cleanse(bytes.data(), bytes.size());
  return result;
}

std::uint64_t convert_group(const Seed& value_seed) {
  // Theorem 2's Convert_G is the first ell bits of its lambda-bit input when
  // G = Z/(2^ell), ell <= lambda. This is not another PRG expansion.
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < 8; ++i)
    result = (result << 8U) | value_seed[i];
  return result;
}

struct Child {
  Seed seed{};
  std::uint64_t value = 0;
  std::uint8_t control = 0;
};

struct Children {
  Child left;
  Child right;
};

Children expand_children(const Seed& seed) {
  const auto generated = expand(seed);
  std::size_t offset = 0;
  Children result;
  result.left.seed = read_seed(generated, offset);
  result.left.value = read_bits_u64(generated, offset, 64);
  offset += 64;  // discard the low half of v_L; Convert_G uses its MSB prefix.
  result.left.control = static_cast<std::uint8_t>(read_bits_u64(generated, offset, 1));
  result.right.seed = read_seed(generated, offset);
  result.right.value = read_bits_u64(generated, offset, 64);
  offset += 64;  // discard the low half of v_R; Convert_G uses its MSB prefix.
  result.right.control = static_cast<std::uint8_t>(read_bits_u64(generated, offset, 1));
  require(offset == kBitsPerExpandedTuple, "BCG21 DCF PRG parse length");
  return result;
}

struct Node {
  Seed seed{};
  std::uint8_t control = 0;
};

std::uint8_t alpha_bit(std::uint64_t alpha, std::uint8_t bits,
                       std::uint8_t level) {
  return static_cast<std::uint8_t>((alpha >> (bits - level)) & 1U);
}

void put_u64(std::vector<std::uint8_t>& output, std::uint64_t value) {
  for (int i = 7; i >= 0; --i)
    output.push_back(static_cast<std::uint8_t>(value >> (8U * i)));
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& input,
                      std::size_t& offset) {
  require(offset <= input.size() && input.size() - offset >= 8,
          "BCG21 DCF truncated integer");
  std::uint64_t result = 0;
  for (unsigned i = 0; i < 8; ++i) result = (result << 8U) | input[offset++];
  return result;
}

std::uint64_t eval(const PartyKey& key, std::uint64_t x) {
  require(key.party <= 1 && key.input_bits >= 1 && key.input_bits <= 63,
          "BCG21 DCF invalid key");
  require(key.correction_words.size() == key.input_bits,
          "BCG21 DCF correction count");
  require((x & ~input_mask(key.input_bits)) == 0,
          "BCG21 DCF input outside domain");
  Node node{key.root_seed, key.party};
  std::uint64_t value = 0;
  const auto party_sign = key.party == 0 ? UINT64_C(1) : UINT64_C(0) - UINT64_C(1);
  for (std::uint8_t level = 1; level <= key.input_bits; ++level) {
    const auto children = expand_children(node.seed);
    const auto& cw = key.correction_words[level - 1U];
    const auto direction = static_cast<unsigned>(
        (x >> (key.input_bits - level)) & 1U);
    const auto& selected = direction == 0 ? children.left : children.right;
    const auto correction_control = static_cast<std::uint8_t>(
        (cw.control >> direction) & 1U);
    const auto selected_control = static_cast<std::uint8_t>(
        selected.control ^ (node.control & correction_control));
    const auto selected_seed = [&]() {
      Seed result = selected.seed;
      if (node.control != 0)
        for (std::size_t i = 0; i < result.size(); ++i) result[i] ^= cw.seed[i];
      return result;
    }();
    value += party_sign * selected.value;
    if (node.control != 0) value += party_sign * cw.value;
    node = {selected_seed, selected_control};
  }
  value += party_sign * convert_group(node.seed);
  if (node.control != 0) value += party_sign * key.final_correction;
  return value;
}

KeyPair generate(std::uint8_t bits, std::uint64_t alpha,
                 std::uint64_t beta, const Seed& root0, const Seed& root1) {
  const auto mask = input_mask(bits);
  require((alpha & ~mask) == 0, "BCG21 DCF alpha outside input domain");
  KeyPair pair;
  pair.party0.party = 0;
  pair.party1.party = 1;
  pair.party0.input_bits = pair.party1.input_bits = bits;
  pair.party0.root_seed = root0;
  pair.party1.root_seed = root1;
  pair.party0.correction_words.resize(bits);
  pair.party1.correction_words.resize(bits);
  Node nodes[2]{{root0, 0}, {root1, 1}};
  std::uint64_t value_alpha = 0;
  for (std::uint8_t level = 1; level <= bits; ++level) {
    const auto expanded0 = expand_children(nodes[0].seed);
    const auto expanded1 = expand_children(nodes[1].seed);
    const auto alpha_i = alpha_bit(alpha, bits, level);
    const bool keep_left = alpha_i == 0;
    const Child& keep0 = keep_left ? expanded0.left : expanded0.right;
    const Child& keep1 = keep_left ? expanded1.left : expanded1.right;
    const Child& lose0 = keep_left ? expanded0.right : expanded0.left;
    const Child& lose1 = keep_left ? expanded1.right : expanded1.left;
    const auto sign = nodes[1].control == 0 ? UINT64_C(1) : UINT64_C(0) - UINT64_C(1);
    CorrectionWord cw;
    for (std::size_t i = 0; i < cw.seed.size(); ++i)
      cw.seed[i] = static_cast<std::uint8_t>(lose0.seed[i] ^ lose1.seed[i]);
    cw.value = sign * (lose1.value - lose0.value - value_alpha);
    if (!keep_left) cw.value += sign * beta;
    value_alpha = value_alpha - keep1.value + keep0.value + sign * cw.value;
    const auto left_control = static_cast<std::uint8_t>(
        expanded0.left.control ^ expanded1.left.control ^ alpha_i ^ 1U);
    const auto right_control = static_cast<std::uint8_t>(
        expanded0.right.control ^ expanded1.right.control ^ alpha_i);
    cw.control = static_cast<std::uint8_t>(left_control | (right_control << 1U));
    pair.party0.correction_words[level - 1U] = cw;
    pair.party1.correction_words[level - 1U] = cw;
    const auto keep_control = keep_left ? left_control : right_control;
    for (unsigned party = 0; party < 2; ++party) {
      const auto& keep = party == 0 ? keep0 : keep1;
      nodes[party].seed = keep.seed;
      if (nodes[party].control != 0)
        for (std::size_t i = 0; i < nodes[party].seed.size(); ++i)
          nodes[party].seed[i] ^= cw.seed[i];
      nodes[party].control = static_cast<std::uint8_t>(
          keep.control ^ (nodes[party].control & keep_control));
    }
  }
  const auto sign = nodes[1].control == 0 ? UINT64_C(1) : UINT64_C(0) - UINT64_C(1);
  const auto final_cw = sign * (convert_group(nodes[1].seed) -
                                convert_group(nodes[0].seed) - value_alpha);
  pair.party0.final_correction = final_cw;
  pair.party1.final_correction = final_cw;
  return pair;
}

}  // namespace

PartyKey::~PartyKey() {
  volatile std::uint8_t* root = root_seed.data();
  for (std::size_t i = 0; i < root_seed.size(); ++i) root[i] = 0;
  for (auto& cw : correction_words) {
    volatile std::uint8_t* seed = cw.seed.data();
    for (std::size_t i = 0; i < cw.seed.size(); ++i) seed[i] = 0;
    cw.value = 0;
    cw.control = 0;
  }
  final_correction = 0;
}

KeyPair keygen(std::uint8_t bits, std::uint64_t alpha, std::uint64_t beta) {
  Seed root0{}, root1{};
  if (RAND_bytes(root0.data(), static_cast<int>(root0.size())) != 1 ||
      RAND_bytes(root1.data(), static_cast<int>(root1.size())) != 1) {
    std::fill(root0.begin(), root0.end(), 0);
    std::fill(root1.begin(), root1.end(), 0);
    throw std::runtime_error("BCG21 DCF CSPRNG failure");
  }
  auto result = generate(bits, alpha, beta, root0, root1);
  OPENSSL_cleanse(root0.data(), root0.size());
  OPENSSL_cleanse(root1.data(), root1.size());
  return result;
}

KeyPair keygen_for_test(std::uint8_t bits, std::uint64_t alpha,
                        std::uint64_t beta, const Seed& root0,
                        const Seed& root1) {
  return generate(bits, alpha, beta, root0, root1);
}

std::uint64_t eval_share(const PartyKey& key, std::uint64_t input) {
  return eval(key, input);
}

std::size_t serialized_size(std::uint8_t bits) {
  require(bits >= 1 && bits <= 63, "BCG21 DCF input width");
  return kHeaderBytes + kSeedBytes + static_cast<std::size_t>(bits) * 25U + kGroupBytes;
}

std::vector<std::uint8_t> serialize(const PartyKey& key) {
  require(key.party <= 1 && key.input_bits >= 1 && key.input_bits <= 63 &&
              key.correction_words.size() == key.input_bits,
          "BCG21 DCF key shape");
  std::vector<std::uint8_t> output{'M','2','B','1',1,key.party,key.input_bits,64,1};
  put_u64(output, static_cast<std::uint64_t>(key.input_bits) + 1U);
  put_u64(output, static_cast<std::uint64_t>(key.input_bits));
  put_u64(output, static_cast<std::uint64_t>(key.input_bits));
  output.insert(output.end(), key.root_seed.begin(), key.root_seed.end());
  for (const auto& cw : key.correction_words) {
    require((cw.control & 0xFCU) == 0, "BCG21 DCF invalid control bits");
    output.insert(output.end(), cw.seed.begin(), cw.seed.end());
    put_u64(output, cw.value);
    output.push_back(cw.control);
  }
  put_u64(output, key.final_correction);
  require(output.size() == serialized_size(key.input_bits),
          "BCG21 DCF serialization size");
  return output;
}

PartyKey deserialize(const std::vector<std::uint8_t>& encoded) {
  require(encoded.size() >= kHeaderBytes && encoded[0] == 'M' &&
              encoded[1] == '2' && encoded[2] == 'B' && encoded[3] == '1' &&
              encoded[4] == 1 && encoded[7] == 64 && encoded[8] == 1,
          "BCG21 DCF key header/version");
  PartyKey key;
  key.party = encoded[5];
  key.input_bits = encoded[6];
  require(key.party <= 1 && encoded.size() == serialized_size(key.input_bits),
          "BCG21 DCF key length/identity");
  std::size_t offset = 9;
  const auto seed_count = get_u64(encoded, offset);
  const auto group_cw_count = get_u64(encoded, offset);
  const auto control_count = get_u64(encoded, offset);
  require(seed_count == static_cast<std::uint64_t>(key.input_bits) + 1U &&
              group_cw_count == key.input_bits && control_count == key.input_bits,
          "BCG21 DCF key correction counts");
  std::copy_n(encoded.begin() + static_cast<std::ptrdiff_t>(offset), 16,
              key.root_seed.begin());
  offset += 16;
  key.correction_words.resize(key.input_bits);
  for (auto& cw : key.correction_words) {
    std::copy_n(encoded.begin() + static_cast<std::ptrdiff_t>(offset), 16,
                cw.seed.begin());
    offset += 16;
    cw.value = get_u64(encoded, offset);
    require(offset < encoded.size(), "BCG21 DCF missing control bit");
    cw.control = encoded[offset++];
    require((cw.control & 0xFCU) == 0, "BCG21 DCF control padding");
  }
  key.final_correction = get_u64(encoded, offset);
  require(offset == encoded.size(), "BCG21 DCF trailing bytes");
  return key;
}

std::array<std::uint8_t, 16> aes128_encrypt_block_for_test(
    const Seed& key, const std::array<std::uint8_t, 16>& plaintext) {
  return encrypt_block(key, plaintext);
}

}  // namespace moe_topk::bcg21_fig1_aes128_v1
