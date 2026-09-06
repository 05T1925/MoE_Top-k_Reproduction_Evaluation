#pragma once

#include <FSS/group_element.h>
#include <FSS/keypack.h>

#include <cstdint>
#include <utility>

class Dealer;
class Peer;

namespace moe_topk {

constexpr int kMaskedMulRingBits = 64;

enum class OnlineParty : std::uint8_t {
    kParty0 = 0,
    kParty1 = 1,
};

struct MaskedMulOpenShare {
    GroupElement left = 0;
    GroupElement right = 0;
};

// One party's one-time preprocessing material. The state is deliberately kept
// local and is not serialized; received material always starts fresh.
struct MaskedMulMaterial {
    MultKey key{};
    GroupElement output_mask_share = 0;

private:
    enum class State : std::uint8_t {
        kFresh,
        kOpenSharePrepared,
        kConsumed,
    };

    State state = State::kFresh;

    friend MaskedMulOpenShare prepare_masked_mul_open_share(
        GroupElement,
        GroupElement,
        MaskedMulMaterial&);
    friend GroupElement evaluate_masked_mul_share(
        OnlineParty,
        GroupElement,
        GroupElement,
        MaskedMulMaterial&);
};

struct MaskedMulMaterialPair {
    MaskedMulMaterial party0;
    MaskedMulMaterial party1;
};

// Dealer-only preprocessing. input masks and output mask are independent
// uniform ring elements chosen outside this adapter.
MaskedMulMaterialPair generate_masked_mul_material(
    GroupElement left_input_mask,
    GroupElement right_input_mask,
    GroupElement output_mask);

// Online phase, before communication: form this party's shares of the two
// masked values that the runtime will open to both online parties.
MaskedMulOpenShare prepare_masked_mul_open_share(
    GroupElement left_input_share,
    GroupElement right_input_share,
    MaskedMulMaterial& material);

// Online phase, after the runtime has opened the two masked values. Returns an
// additive product share and never communicates or reconstructs the product.
GroupElement evaluate_masked_mul_share(
    OnlineParty party,
    GroupElement opened_left,
    GroupElement opened_right,
    MaskedMulMaterial& material);

// Thin bindings to the repository's existing Peer/Dealer serialization path.
void send_masked_mul_material(Peer& peer,
                              const MaskedMulMaterial& material);
MaskedMulMaterial receive_masked_mul_material(Dealer& dealer);

}  // namespace moe_topk
