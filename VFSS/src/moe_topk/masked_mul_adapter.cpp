#include <moe_topk/masked_mul_adapter.h>

#include <FSS/comms.h>
#include <mult.h>

#include <stdexcept>

namespace moe_topk {
namespace {

void set_ring_metadata(MultKey& key) {
    key.Bin = kMaskedMulRingBits;
    key.Bout = kMaskedMulRingBits;
}

void require_ring_metadata(const MultKey& key) {
    if (key.Bin != kMaskedMulRingBits || key.Bout != kMaskedMulRingBits) {
        throw std::invalid_argument(
            "masked multiplication material must use the 64-bit ring");
    }
}

int mult_eval_party(OnlineParty party) {
    switch (party) {
        case OnlineParty::kParty0:
            return 0;
        case OnlineParty::kParty1:
            return 1;
    }
    throw std::invalid_argument("online party must be party 0 or party 1");
}

}  // namespace

MaskedMulMaterialPair generate_masked_mul_material(
    GroupElement left_input_mask,
    GroupElement right_input_mask,
    GroupElement output_mask) {
    auto keys = MultGen(left_input_mask, right_input_mask, output_mask);
    set_ring_metadata(keys.first);
    set_ring_metadata(keys.second);

    const auto output_mask_shares =
        splitShare(output_mask, kMaskedMulRingBits);

    MaskedMulMaterialPair result;
    result.party0.key = keys.first;
    result.party0.output_mask_share = output_mask_shares.first;
    result.party1.key = keys.second;
    result.party1.output_mask_share = output_mask_shares.second;
    return result;
}

MaskedMulOpenShare prepare_masked_mul_open_share(
    GroupElement left_input_share,
    GroupElement right_input_share,
    MaskedMulMaterial& material) {
    require_ring_metadata(material.key);
    if (material.state != MaskedMulMaterial::State::kFresh) {
        throw std::logic_error(
            "masked multiplication material open share was already prepared");
    }

    material.state = MaskedMulMaterial::State::kOpenSharePrepared;
    return {
        left_input_share + material.key.a,
        right_input_share + material.key.b,
    };
}

GroupElement evaluate_masked_mul_share(
    OnlineParty party,
    GroupElement opened_left,
    GroupElement opened_right,
    MaskedMulMaterial& material) {
    require_ring_metadata(material.key);
    const int party_index = mult_eval_party(party);
    if (material.state != MaskedMulMaterial::State::kOpenSharePrepared) {
        throw std::logic_error(
            "masked multiplication material is not ready or was already consumed");
    }

    material.state = MaskedMulMaterial::State::kConsumed;
    return MultEval(party_index, material.key, opened_left, opened_right) -
           material.output_mask_share;
}

void send_masked_mul_material(Peer& peer,
                              const MaskedMulMaterial& material) {
    require_ring_metadata(material.key);
    peer.send_mult_key(material.key);
    peer.send_ge(material.output_mask_share, kMaskedMulRingBits);
}

MaskedMulMaterial receive_masked_mul_material(Dealer& dealer) {
    MaskedMulMaterial material;
    material.key = dealer.recv_mult_key();
    material.output_mask_share = dealer.recv_ge(kMaskedMulRingBits);
    require_ring_metadata(material.key);
    return material;
}

}  // namespace moe_topk
