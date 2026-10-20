#!/usr/bin/env python3
"""Exact known-component budget and schema-compatibility probe for padded N=128."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


def dcf_party_key_bytes(bits: int) -> int:
    # ProtocolIUcmpPartyMaterial::serialize(): 9-byte header, 24-byte counts,
    # (bits+1) 16-byte blocks, one 8-byte G, and bits 8-byte V elements.
    return 57 + 24 * bits


def budget(n: int = 128) -> dict[str, Any]:
    if n != 128:
        raise ValueError("S3 budget fixture is padded N=128")
    padded_n, bits, index_bits = 128, 40, 7
    one_pair = math.comb(padded_n, 2)
    select_slots = 5 * one_pair
    slot_addresses = {
        (iteration, "select_compare", "strict_lt", left, right)
        for iteration in range(1, 6)
        for left in range(padded_n)
        for right in range(left + 1, padded_n)
    }
    if len(slot_addresses) != select_slots:
        raise AssertionError("iteration-scoped one-shot slot identity collision")
    input_slots = 2 * padded_n
    membership_slots = padded_n

    # Conservative proposed record: 24-byte injective slot descriptor plus
    # two fresh 64-bit mask shares for this exact comparison (no mask reuse).
    select_record = dcf_party_key_bytes(bits) + 40
    input_record = dcf_party_key_bytes(34) + 40
    membership_record = dcf_party_key_bytes(bits) + 40
    select_bytes = select_slots * select_record
    input_bytes = input_slots * input_record
    membership_bytes = membership_slots * membership_record
    # Reproduce the existing 40+8N+8 package prefix/mask-vector budget.
    # Per-call masks for every Select edge and membership call are counted in
    # their records above; this node-mask vector is the existing package field.
    mask_share_count = padded_n
    mask_share_bytes = 8 * mask_share_count
    bundle_header_bytes = 48  # 40-byte header/config + 8-byte carry/sign count
    known_bundle_bytes = (
        select_bytes + input_bytes + membership_bytes + mask_share_bytes + bundle_header_bytes
    )

    # Existing secret-shuffle retained vector payload, using the current
    # N=128,T=2, d=2 log_T(N)-1 construction. This excludes C++ object,
    # allocator, base-OT/Chosen-OT wire and transient-generation overhead.
    t = 2
    depth = 2 * (padded_n.bit_length() - 1) // (t.bit_length() - 1) - 1
    translations = depth * padded_n // t
    po_payload = (
        depth * padded_n * 4  # Beneš combined layer permutations
        + translations * t * 4  # local permutations
        + depth * padded_n * 24  # delta
        + padded_n * 4  # PO permutation
    )
    do_payload = 2 * depth * padded_n * 24 + padded_n * 24  # a,b,w
    shuffle_retained_bytes = 2 * po_payload + 2 * do_payload + 2 * padded_n * 4
    opv_instances = 4 * translations * t
    chosen_ot_items = opv_instances * (t.bit_length() - 1)

    # Existing package validator requires exactly one C(N,2) layer; it has no
    # iteration/purpose namespace, so a five-stage pool cannot be encoded.
    existing_package_edge_count = one_pair
    current_edges_cap = 1_000_000
    frame_cap = 64 * 1024 * 1024
    max_frame_payload_bytes = 1 << 20
    wire_header_bytes = 48
    chunk_envelope_bytes = 16
    chunk_data_bytes = max_frame_payload_bytes - chunk_envelope_bytes
    framed_chunks = math.ceil(known_bundle_bytes / chunk_data_bytes)
    framed_envelope_bytes = framed_chunks * chunk_envelope_bytes
    framed_wire_header_bytes = framed_chunks * wire_header_bytes
    framed_message_bytes = known_bundle_bytes + framed_envelope_bytes + framed_wire_header_bytes
    combined_logical_bytes = known_bundle_bytes + shuffle_retained_bytes
    combined_chunks = math.ceil(combined_logical_bytes / chunk_data_bytes)
    combined_framed_bytes = combined_logical_bytes + combined_chunks * (
        chunk_envelope_bytes + wire_header_bytes
    )
    return {
        "label": "PROJECT_DERIVED_SELECT_5ITER",
        "status": "KNOWN_COMPONENT_BUDGET_ONLY",
        "n": n,
        "k_matrix": [2, 8],
        "k_affects_known_material_bytes": False,
        "padded_n": padded_n,
        "comparison_bits": bits,
        "index_bits": index_bits,
        "pairs_per_complete_layer": one_pair,
        "max_select_slots": select_slots,
        "injective_iteration_scoped_slot_addresses": len(slot_addresses),
        "slot_address_identity_fields": ["session(header)", "algorithm(schema)", "iteration", "purpose", "direction", "left", "right"],
        "substage_namespace_policy": "usage claim records substage; one physical slot per iteration/pair is sound because sample-clique and pivot-star edge sets are disjoint",
        "raw_score_conversion_slots": input_slots,
        "membership_slots": membership_slots,
        "total_known_dcf_slots_per_party": select_slots + input_slots + membership_slots,
        "dcf_evals_per_party_max": 2 * (select_slots + input_slots + membership_slots),
        "dcf_party_key_bytes_b40": dcf_party_key_bytes(bits),
        "dcf_party_key_bytes_b34": dcf_party_key_bytes(34),
        "proposed_slot_record_bytes_b40": select_record,
        "proposed_slot_descriptor_bytes": 24,
        "fresh_left_right_mask_shares_per_select_slot_bytes": 16,
        "select_pool_bytes_per_party": select_bytes,
        "raw_score_material_bytes_per_party": input_bytes,
        "membership_material_bytes_per_party": membership_bytes,
        "existing_package_node_mask_share_count": mask_share_count,
        "per_party_mask_share_bytes": mask_share_bytes,
        "proposed_bundle_header_bytes": bundle_header_bytes,
        "known_bundle_bytes_per_party": known_bundle_bytes,
        "known_bundle_mib_per_party": known_bundle_bytes / (1024 * 1024),
        "framed_bundle_transfer": {
            "frame_payload_cap_bytes": max_frame_payload_bytes,
            "wire_header_bytes_per_frame": wire_header_bytes,
            "chunk_envelope_bytes_per_frame": chunk_envelope_bytes,
            "chunk_data_capacity_bytes": chunk_data_bytes,
            "frame_count_for_known_bundle": framed_chunks,
            "chunk_envelope_total_bytes": framed_envelope_bytes,
            "wire_header_total_bytes": framed_wire_header_bytes,
            "known_bundle_plus_frame_envelopes_and_headers_bytes": framed_message_bytes,
            "known_bundle_plus_shuffle_logical_bytes": combined_logical_bytes,
            "combined_frame_count_if_sent_as_one_bundle": combined_chunks,
            "combined_bundle_plus_frame_envelopes_and_headers_bytes": combined_framed_bytes,
            "tls_record_and_transport_overhead": "NOT_MEASURED",
        },
        "shuffle_parameters": {
            "subpermutation_size": t,
            "benes_depth": depth,
            "translation_count_per_ps": translations,
            "po_payload_bytes_each": po_payload,
            "do_payload_bytes_each": do_payload,
            "retained_logical_vector_bytes_per_party": shuffle_retained_bytes,
            "opv_instances_per_party": opv_instances,
            "chosen_ot_items_per_party": chosen_ot_items,
            "offline_ot_wire_bytes": "NOT_MEASURED",
        },
        "known_material_plus_shuffle_vector_payload_bytes_per_party": known_bundle_bytes
        + shuffle_retained_bytes,
        "known_combined_mib_per_party": (known_bundle_bytes + shuffle_retained_bytes) / (1024 * 1024),
        "adaptive_control_material_bytes": "NOT_MEASURED_NO_IMPLEMENTED_API",
        "secure_pivot_and_threshold_selection_material_bytes": "NOT_MEASURED_NO_IMPLEMENTED_API",
        "full_party_material_total": "NOT_CLOSED",
        "serialized_lookup_index_bytes": "NOT_MEASURED; proposed address is direct from session/iteration/purpose/endpoints",
        "current_package_schema": {
            "required_edge_count": existing_package_edge_count,
            "required_select_edge_slots": select_slots,
            "iteration_namespace": False,
            "current_edge_count_cap": current_edges_cap,
            "current_package_byte_cap": frame_cap,
            "pool_below_edge_count_cap": select_slots <= current_edges_cap,
            "current_serializer_accepts_five_stage_pool": False,
            "rejection_reason": "M2PK shape is exactly C(N,2) with no iteration/purpose key namespace",
        },
        "proposed_known_bundle_under_nominal_frame_cap": known_bundle_bytes <= frame_cap,
        "proposed_known_bundle_with_chunk_envelopes_under_nominal_frame_cap": framed_message_bytes <= frame_cap,
        "admission": "NO_GO_UNTIL_CONTROL_AND_MEMBERSHIP_MATERIALS_ARE_DEFINED",
        "caveat": (
            "No DCF keys were generated. The per-call fresh-mask descriptor format is a "
            "conservative budget model, not a runtime package. The known bundle omits "
            "adaptive-control material, offline OT wire, and in-memory lookup allocations. "
            "The per-party package cap applies to the logical package; the chunked handoff "
            "adds the measured-in-source 16-byte envelope and 48-byte frame header per chunk."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = budget()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "result": result["admission"],
                "select_slots": result["max_select_slots"],
                "known_bundle_bytes_per_party": result["known_bundle_bytes_per_party"],
                "known_bundle_mib_per_party": result["known_bundle_mib_per_party"],
                "framed_bundle_bytes": result["framed_bundle_transfer"][
                    "known_bundle_plus_frame_envelopes_and_headers_bytes"
                ],
                "frame_count": result["framed_bundle_transfer"]["frame_count_for_known_bundle"],
                "combined_framed_bytes": result["framed_bundle_transfer"][
                    "combined_bundle_plus_frame_envelopes_and_headers_bytes"
                ],
                "shuffle_retained_vector_bytes_per_party": result["shuffle_parameters"][
                    "retained_logical_vector_bytes_per_party"
                ],
                "current_schema_accepts": result["current_package_schema"][
                    "current_serializer_accepts_five_stage_pool"
                ],
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
