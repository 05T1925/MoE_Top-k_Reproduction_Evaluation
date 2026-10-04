"""TEST_ONLY, allocation-free F1 bundle shape preflight.

These are layout-derived bytes, never timing, RSS or measured nine metrics.
The formulas follow protocol_iii_raw_score_mask_package.cpp,
protocol_i_party_package.cpp and the native DPF key width contract.
"""

import json


PACKET_LIMIT = 64 * 1024 * 1024


def shape(n, k):
    if n < 2 or not 1 <= k <= n:
        raise ValueError("F1 public shape")
    d = max(2, 1 << (n - 1).bit_length())
    comparison_bits = 33 + (d - 1).bit_length()
    rank_bits = (n - 1).bit_length()
    if comparison_bits > 53 or rank_bits > 20:
        raise ValueError("F1 width")
    pairs = n * (n - 1) // 2
    point_bytes = 1 if rank_bits <= 8 else 2 if rank_bits <= 16 else 4
    dpf_key_bytes = 16 * (rank_bits + 1) + 2 * point_bytes + 16
    score_key_serialized = 24 * 34 + 57  # 33-byte header + 840-byte payload
    edge_key_serialized = 24 * comparison_bits + 57
    score_section = 2 * d * (25 + score_key_serialized)
    grank_section = 48 + 8 * n + pairs * (24 + edge_key_serialized)
    routing_section = n * (8 + dpf_key_bytes)
    party_bundle = 58 + score_section + grank_section + routing_section
    # Same effective-payload boundary as E17: online-retained shares and
    # native FSS key payloads, excluding application metadata/headers.
    effective_score = 2 * d * (16 + 840)
    effective_grank = 8 * n + pairs * (24 * comparison_bits + 24)
    effective_routing = routing_section
    return {
        "n": n, "k": k, "padded_d": d,
        "comparison_bits": comparison_bits, "rank_bits": rank_bits,
        "planned_full_clique_edges": pairs,
        "party_bundle_bytes_derived": party_bundle,
        "party_score_section_bytes_derived": score_section,
        "party_grank_section_bytes_derived": grank_section,
        "party_routing_section_bytes_derived": routing_section,
        "two_party_effective_material_bits_derived": 16 * (
            effective_score + effective_grank + effective_routing),
        "bundle_under_64_mib": party_bundle <= PACKET_LIMIT,
        "process_rss_status": "NOT_MEASURED",
        "formal_lan_wan_batch_status": "NOT_RUN",
        "nine_metrics_status": "NOT_MEASURED",
    }


def main():
    # M5 G3 receiver's real serialized small-case smoke, both bundles plus
    # two 8-byte IPC length prefixes. Reject layout drift before larger shape.
    for n, k, observed_bytes in ((2, 1, 9470), (5, 2, 48844), (8, 8, 83452)):
        assert 2 * shape(n, k)["party_bundle_bytes_derived"] + 16 == observed_bytes
    for n in (128, 256):
        for k in (2, 8):
            print(json.dumps(shape(n, k), sort_keys=True))


if __name__ == "__main__":
    main()
