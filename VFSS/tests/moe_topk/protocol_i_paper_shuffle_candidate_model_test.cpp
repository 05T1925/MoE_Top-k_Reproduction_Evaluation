// TEST_ONLY: paper-compatible algebra and view model.
// This file is not a secure shuffle, a production primitive, or a paper-exact
// Protocol I implementation. It intentionally has no production-library link.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using Word = std::uint64_t;
using Permutation = std::vector<std::size_t>;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Word ring_mask(std::uint32_t bits) {
    require(bits > 0 && bits < 64, "ring width");
    return (Word{1} << bits) - 1;
}

Word add_mod(Word left, Word right, std::uint32_t bits) {
    return (left + right) & ring_mask(bits);
}

Word sub_mod(Word left, Word right, std::uint32_t bits) {
    return (left - right) & ring_mask(bits);
}

template <typename T>
std::vector<T> add_vectors(const std::vector<T>& left,
                           const std::vector<T>& right,
                           std::uint32_t bits) {
    require(left.size() == right.size(), "vector share length");
    std::vector<T> result(left.size());
    for (std::size_t index = 0; index < left.size(); ++index) {
        result[index] = static_cast<T>(add_mod(left[index], right[index], bits));
    }
    return result;
}

template <typename T>
std::vector<T> subtract_vectors(const std::vector<T>& left,
                                const std::vector<T>& right,
                                std::uint32_t bits) {
    require(left.size() == right.size(), "vector subtraction length");
    std::vector<T> result(left.size());
    for (std::size_t index = 0; index < left.size(); ++index) {
        result[index] = static_cast<T>(sub_mod(left[index], right[index], bits));
    }
    return result;
}

std::size_t next_power_of_two(std::size_t value) {
    require(value > 0, "logical_n must be positive");
    std::size_t result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

std::uint32_t bits_for_count(std::size_t count) {
    require(count > 0, "count must be positive");
    std::uint32_t bits = 0;
    for (std::size_t value = count - 1; value != 0; value >>= 1) {
        ++bits;
    }
    return bits == 0 ? 1 : bits;
}

struct Shape {
    std::size_t logical_n;
    std::size_t padded_n;
    std::uint32_t key_bits;
    std::uint32_t payload_bits;
    std::uint32_t rank_bits;
    std::uint64_t material_id;

    bool operator==(const Shape& other) const {
        return logical_n == other.logical_n && padded_n == other.padded_n &&
               key_bits == other.key_bits &&
               payload_bits == other.payload_bits &&
               rank_bits == other.rank_bits &&
               material_id == other.material_id;
    }
};

Shape make_shape(std::size_t logical_n, std::uint64_t material_id) {
    const auto padded_n = next_power_of_two(logical_n);
    const auto index_bits = bits_for_count(logical_n);
    const auto key_bits = 32U + index_bits;
    return {logical_n,
            padded_n,
            key_bits,
            key_bits + index_bits + 1U,
            bits_for_count(padded_n),
            material_id};
}

void validate_permutation(const Permutation& permutation, std::size_t size) {
    require(permutation.size() == size, "permutation length");
    std::vector<bool> seen(size, false);
    for (const auto value : permutation) {
        require(value < size && !seen[value], "invalid permutation");
        seen[value] = true;
    }
}

template <typename T>
std::vector<T> permute(const Permutation& permutation,
                       const std::vector<T>& values) {
    validate_permutation(permutation, values.size());
    std::vector<T> result(values.size());
    for (std::size_t slot = 0; slot < values.size(); ++slot) {
        result[slot] = values[permutation[slot]];
    }
    return result;
}

Permutation compose_after_p0_then_p1(const Permutation& p0,
                                      const Permutation& p1) {
    validate_permutation(p0, p0.size());
    validate_permutation(p1, p0.size());
    Permutation result(p0.size());
    for (std::size_t slot = 0; slot < result.size(); ++slot) {
        result[slot] = p0[p1[slot]];
    }
    return result;
}

Permutation make_permutation(std::size_t size,
                             int kind,
                             std::uint64_t seed) {
    Permutation result(size);
    std::iota(result.begin(), result.end(), 0);
    if (kind == 0) {
        return result;
    }
    if (kind == 1) {
        std::reverse(result.begin(), result.end());
        return result;
    }
    std::mt19937_64 generator(seed);
    std::shuffle(result.begin(), result.end(), generator);
    return result;
}

std::uint64_t priority_key(std::int32_t score,
                           std::size_t original_index,
                           const Shape& shape) {
    require(original_index < shape.logical_n, "original index");
    const auto index_bits = shape.key_bits - 32U;
    const auto raw = static_cast<std::uint32_t>(score);
    const auto ordered = raw ^ UINT32_C(0x80000000);
    const auto high = UINT32_MAX - ordered;
    const auto key = (static_cast<Word>(high) << index_bits) |
                     static_cast<Word>(original_index);
    return key & ring_mask(shape.key_bits);
}

std::vector<std::int32_t> make_scores(std::size_t logical_n) {
    std::vector<std::int32_t> scores(logical_n, 0);
    for (std::size_t index = 0; index < logical_n; ++index) {
        const auto pattern = static_cast<std::int32_t>(
            (static_cast<int>(index % 3) - 1) * 4096);
        scores[index] = pattern;
    }
    scores[0] = std::numeric_limits<std::int32_t>::max();
    if (logical_n > 1) {
        scores[1] = std::numeric_limits<std::int32_t>::min();
    }
    if (logical_n > 2) {
        scores[2] = 4096;
    }
    if (logical_n > 3) {
        scores[3] = 4096;
    }
    return scores;
}

struct PayloadRecord {
    Word key;
    std::size_t original_index;
    bool dummy;
};

Word encode_payload(const PayloadRecord& record, const Shape& shape) {
    const auto index_bits = shape.key_bits - 32U;
    const auto dummy_bit = record.dummy ? (Word{1} << index_bits) : 0;
    return (record.key << (index_bits + 1U)) | dummy_bit |
           static_cast<Word>(record.original_index);
}

struct PartyMaterial {
    Shape shape{};
    Permutation local_permutation;
    std::vector<Word> r_share;
    std::vector<Word> grank_mask_share;
    std::vector<std::size_t> slot_ids;
    bool consumed = false;
};

struct P2View {
    Shape shape{};
    bool saw_input = false;
    bool saw_complete_permutation = false;
    bool saw_complete_r = false;
    bool exited_before_online = true;
};

struct CandidateResult {
    Shape shape{};
    std::vector<Word> public_masked_list;
    std::vector<Word> public_mask_share0;
    std::vector<Word> public_mask_share1;
    std::vector<Word> shuffled_payload_share0;
    std::vector<Word> shuffled_payload_share1;
};

void validate_material(const PartyMaterial& material, const Shape& shape) {
    require(material.shape == shape, "material shape");
    validate_permutation(material.local_permutation, shape.padded_n);
    require(material.r_share.size() == shape.padded_n, "r share length");
    require(material.grank_mask_share == material.r_share,
            "GRank correlation does not use the same r share");
    require(material.slot_ids.size() == shape.padded_n, "slot metadata length");
    for (std::size_t slot = 0; slot < shape.padded_n; ++slot) {
        require(material.slot_ids[slot] == slot, "slot metadata");
    }
}

PartyMaterial make_party_material(const Shape& shape,
                                  const Permutation& local_permutation,
                                  const std::vector<Word>& r_share) {
    validate_permutation(local_permutation, shape.padded_n);
    require(r_share.size() == shape.padded_n, "material r length");
    PartyMaterial material;
    material.shape = shape;
    material.local_permutation = local_permutation;
    material.r_share = r_share;
    material.grank_mask_share = r_share;
    material.slot_ids.resize(shape.padded_n);
    std::iota(material.slot_ids.begin(), material.slot_ids.end(), 0);
    return material;
}

P2View make_p2_view(const Shape& shape) {
    return {shape, false, false, false, true};
}

std::vector<Word> make_mask_share(const Shape& shape,
                                  std::uint64_t seed,
                                  Word offset) {
    const auto mask = ring_mask(shape.key_bits);
    std::vector<Word> result(shape.padded_n);
    for (std::size_t slot = 0; slot < result.size(); ++slot) {
        result[slot] =
            (offset + seed + static_cast<Word>(slot + 1) * UINT64_C(0x13579B)) &
            mask;
        if (result[slot] == 0) {
            result[slot] = static_cast<Word>(slot + 1);
        }
    }
    return result;
}

template <typename T>
std::pair<std::vector<T>, std::vector<T>> split_vector(
    const std::vector<T>& values,
    std::uint32_t bits,
    std::uint64_t seed) {
    std::mt19937_64 generator(seed);
    const auto mask = ring_mask(bits);
    std::vector<T> left(values.size());
    std::vector<T> right(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        left[index] = static_cast<T>(generator() & mask);
        right[index] = static_cast<T>(sub_mod(values[index], left[index], bits));
    }
    return {left, right};
}

CandidateResult run_candidate(Shape shape,
                              const std::vector<Word>& key_values,
                              const std::vector<Word>& payload_values,
                              PartyMaterial& p0,
                              PartyMaterial& p1,
                              const P2View& p2) {
    require(!p0.consumed && !p1.consumed, "one-shot material reuse");
    require(!p2.saw_input && !p2.saw_complete_permutation &&
                !p2.saw_complete_r && p2.exited_before_online,
            "P2 view boundary");
    require(key_values.size() == shape.padded_n, "key vector length");
    require(payload_values.size() == shape.padded_n, "payload vector length");
    validate_material(p0, shape);
    validate_material(p1, shape);

    const auto forward = permute(p0.local_permutation, key_values);
    const auto shuffled_keys = permute(p1.local_permutation, forward);
    const auto payload_forward = permute(p0.local_permutation, payload_values);
    const auto shuffled_payload =
        permute(p1.local_permutation, payload_forward);
    const auto full_r =
        add_vectors(p0.r_share, p1.r_share, shape.key_bits);
    const auto public_masked_list =
        add_vectors(shuffled_keys, full_r, shape.key_bits);

    const auto payload_shares =
        split_vector(shuffled_payload, shape.payload_bits, 0x5041594C4F4144ULL);
    const auto public_shares =
        split_vector(public_masked_list, shape.key_bits, 0x5055424C4943ULL);

    p0.consumed = true;
    p1.consumed = true;
    return {shape,
            public_masked_list,
            public_shares.first,
            public_shares.second,
            payload_shares.first,
            payload_shares.second};
}

struct Fixture {
    Shape shape;
    std::vector<Word> keys;
    std::vector<Word> payload;
    PartyMaterial p0;
    PartyMaterial p1;
    P2View p2;
};

Fixture make_fixture(std::size_t logical_n,
                     int permutation_kind,
                     std::uint64_t seed) {
    const auto shape = make_shape(logical_n, seed);
    const auto scores = make_scores(logical_n);
    std::vector<Word> keys(shape.padded_n, ring_mask(shape.key_bits));
    std::vector<Word> payload(shape.padded_n);
    for (std::size_t index = 0; index < logical_n; ++index) {
        const auto key = priority_key(scores[index], index, shape);
        keys[index] = key;
        payload[index] = encode_payload({key, index, false}, shape);
    }
    for (std::size_t index = logical_n; index < shape.padded_n; ++index) {
        payload[index] = encode_payload(
            {keys[index], 0, true}, shape);
    }

    const auto p0 =
        make_permutation(shape.padded_n, permutation_kind, seed + 11);
    const auto p1 =
        make_permutation(shape.padded_n, permutation_kind == 0 ? 2 : 3,
                         seed + 29);
    const auto r0 = make_mask_share(shape, seed + 101, UINT64_C(0x123456));
    const auto r1 = make_mask_share(shape, seed + 211, UINT64_C(0x654321));
    return {shape,
            keys,
            payload,
            make_party_material(shape, p0, r0),
            make_party_material(shape, p1, r1),
            make_p2_view(shape)};
}

void verify_expected_output(const Fixture& fixture,
                            const CandidateResult& result) {
    const auto composite = compose_after_p0_then_p1(
        fixture.p0.local_permutation, fixture.p1.local_permutation);
    const auto shuffled_keys = permute(composite, fixture.keys);
    const auto shuffled_payload = permute(composite, fixture.payload);
    const auto full_r = add_vectors(fixture.p0.r_share,
                                    fixture.p1.r_share,
                                    fixture.shape.key_bits);
    const auto expected_public =
        add_vectors(shuffled_keys, full_r, fixture.shape.key_bits);
    require(result.public_masked_list == expected_public,
            "public masked list");
    require(add_vectors(result.public_mask_share0,
                        result.public_mask_share1,
                        fixture.shape.key_bits) == expected_public,
            "public list shares");
    require(add_vectors(result.shuffled_payload_share0,
                        result.shuffled_payload_share1,
                        fixture.shape.payload_bits) == shuffled_payload,
            "shuffled payload shares");
    require(fixture.p0.grank_mask_share == fixture.p0.r_share &&
                fixture.p1.grank_mask_share == fixture.p1.r_share,
            "GRank local correlation");
    require(fixture.p0.r_share != full_r && fixture.p1.r_share != full_r,
            "a party received complete r");

    std::vector<std::size_t> real_order(fixture.shape.logical_n);
    std::iota(real_order.begin(), real_order.end(), 0);
    std::sort(real_order.begin(), real_order.end(),
              [&](std::size_t left, std::size_t right) {
                  return fixture.keys[left] < fixture.keys[right];
              });
    for (std::size_t rank = 1; rank < real_order.size(); ++rank) {
        const auto left = real_order[rank - 1];
        const auto right = real_order[rank];
        require(fixture.keys[left] <= fixture.keys[right],
                "priority-key order");
    }
}

void run_case(std::size_t logical_n, int permutation_kind, std::uint64_t seed) {
    auto fixture = make_fixture(logical_n, permutation_kind, seed);
    const auto result = run_candidate(fixture.shape,
                                      fixture.keys,
                                      fixture.payload,
                                      fixture.p0,
                                      fixture.p1,
                                      fixture.p2);
    verify_expected_output(fixture, result);
    require(fixture.p0.consumed && fixture.p1.consumed,
            "material was not consumed");
}

template <typename Function>
void expect_failure(Function&& function, const char* message) {
    bool failed = false;
    try {
        function();
    } catch (const std::exception&) {
        failed = true;
    }
    require(failed, message);
}

void verify_view_shape() {
    auto fixture = make_fixture(5, 2, 0x5348415245ULL);
    const auto composite = compose_after_p0_then_p1(
        fixture.p0.local_permutation, fixture.p1.local_permutation);
    const auto full_r = add_vectors(fixture.p0.r_share,
                                    fixture.p1.r_share,
                                    fixture.shape.key_bits);
    require(fixture.p0.local_permutation != composite,
            "P0 unexpectedly stores complete permutation");
    require(fixture.p1.local_permutation != composite,
            "P1 unexpectedly stores complete permutation");
    require(fixture.p0.r_share != full_r && fixture.p1.r_share != full_r,
            "party unexpectedly stores complete r");
    require(!fixture.p2.saw_input && !fixture.p2.saw_complete_permutation &&
                !fixture.p2.saw_complete_r &&
                fixture.p2.exited_before_online,
            "P2 view is not input-independent");
}

void verify_negative_cases() {
    {
        auto fixture = make_fixture(5, 2, 0x525348415245ULL);
        fixture.p1.grank_mask_share[0] =
            add_mod(fixture.p1.grank_mask_share[0], 1, fixture.shape.key_bits);
        expect_failure(
            [&] {
                (void)run_candidate(fixture.shape,
                                    fixture.keys,
                                    fixture.payload,
                                    fixture.p0,
                                    fixture.p1,
                                    fixture.p2);
            },
            "wrong r correlation was accepted");
    }
    {
        auto fixture = make_fixture(5, 2, 0x534C4F54ULL);
        std::swap(fixture.p1.slot_ids[0], fixture.p1.slot_ids[1]);
        expect_failure(
            [&] {
                (void)run_candidate(fixture.shape,
                                    fixture.keys,
                                    fixture.payload,
                                    fixture.p0,
                                    fixture.p1,
                                    fixture.p2);
            },
            "wrong slot metadata was accepted");
    }
    {
        auto fixture = make_fixture(5, 2, 0x5348415045ULL);
        fixture.p1.shape.padded_n += 1;
        expect_failure(
            [&] {
                (void)run_candidate(fixture.shape,
                                    fixture.keys,
                                    fixture.payload,
                                    fixture.p0,
                                    fixture.p1,
                                    fixture.p2);
            },
            "wrong shape was accepted");
    }
    {
        auto fixture = make_fixture(5, 2, 0x5045524DULL);
        fixture.p1.local_permutation[0] = fixture.p1.local_permutation[1];
        expect_failure(
            [&] {
                (void)run_candidate(fixture.shape,
                                    fixture.keys,
                                    fixture.payload,
                                    fixture.p0,
                                    fixture.p1,
                                    fixture.p2);
            },
            "invalid permutation was accepted");
    }
    {
        auto fixture = make_fixture(3, 1, 0x52455553ULL);
        (void)run_candidate(fixture.shape,
                            fixture.keys,
                            fixture.payload,
                            fixture.p0,
                            fixture.p1,
                            fixture.p2);
        expect_failure(
            [&] {
                (void)run_candidate(fixture.shape,
                                    fixture.keys,
                                    fixture.payload,
                                    fixture.p0,
                                    fixture.p1,
                                    fixture.p2);
            },
            "material reuse was accepted");
    }
}

void verify_round_accounting() {
    constexpr std::uint32_t raw_carry_rounds = 1;
    constexpr std::uint32_t raw_sign_rounds = 1;
    constexpr std::uint32_t candidate_core_rounds = 3;
    constexpr std::uint32_t reverse_mask_adapter_rounds = 2;
    constexpr std::uint32_t current_core_rounds = 4;
    require(candidate_core_rounds == 2 + 1,
            "candidate core decomposition");
    require(candidate_core_rounds != current_core_rounds,
            "candidate overwrote current core accounting");
    require(raw_carry_rounds + raw_sign_rounds +
                candidate_core_rounds + reverse_mask_adapter_rounds ==
                7,
            "candidate raw-score path");
}

}  // namespace

int main() {
    try {
        std::size_t cases = 0;
        for (const auto logical_n :
             {std::size_t{1}, std::size_t{2}, std::size_t{3},
              std::size_t{5}, std::size_t{7}, std::size_t{8}}) {
            for (const auto permutation_kind : {0, 1, 2}) {
                for (const auto seed : {UINT64_C(11), UINT64_C(29),
                                        UINT64_C(47)}) {
                    run_case(logical_n, permutation_kind, seed);
                    ++cases;
                }
            }
        }
        verify_view_shape();
        verify_negative_cases();
        verify_round_accounting();
        std::cout << "m2_protocol_i_paper_compatible_candidate_model"
                  << " cases=" << cases << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "candidate model failure: " << error.what() << '\n';
        return 1;
    }
}
