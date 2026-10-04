#!/usr/bin/env python3
"""Independent E14 JSONL, log, input-pairing, and statistics audit."""

import argparse
import hashlib
import json
from pathlib import Path
import statistics


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def rows(path):
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]


def check_statistics(source, summary):
    values = json.loads(summary.read_text(encoding="utf-8"))
    assert values["raw_sha256"] == digest(source)
    for name, config in values["configurations"].items():
        parts = name.split("-")
        k = int(parts[1][1:])
        r = int(parts[2][1:]) if len(parts) == 3 else None
        group = [row for row in rows(source) if row["K"] == k and
                 (r is None or row["r"] == r) and not row["warmup"]]
        assert len(group) == 5
        for field, expected in config.items():
            if not isinstance(expected, dict) or set(expected) != {"median", "min", "max"}:
                continue
            actual = [row[field] for row in group]
            assert expected == {"median": statistics.median(actual),
                                "min": min(actual), "max": max(actual)}, (name, field)
    return len(values["configurations"])


def check_row(row, head, route, plan):
    assert row["head"] == head and row["tracked_state"] == "CLEAN"
    assert row["status"] == "PASS" and row["correctness"] == \
        "frozen_oracle_and_exact_K_PASS"
    assert row["input_seed"] == plan[f'k{row["K"]}-rep{row["repetition"]}']
    assert row["raw_log_sha256"] == digest(Path(row["raw_log"]))
    assert row["online_comm_total_bits"] == 8 * (
        row["p0_sent_bytes"] + row["p1_sent_bytes"])
    assert row["online_comm_per_party_bits"] * 2 == row["online_comm_total_bits"]
    assert row["p0_sent_bytes"] == row["p1_received_bytes"]
    assert row["p1_sent_bytes"] == row["p0_received_bytes"]
    assert abs(row["total_time_ms"] - row["offline_time_ms"] -
               row["online_time_ms"]) < 1e-8
    assert row["online_time_ms"] == max(row["online_p0_ms"], row["online_p1_ms"])
    assert row["exit_t"] == row["exit_p0"] == row["exit_p1"] == 0
    if route == "aav86":
        r = row["r"]
        assert row["online_rounds"] == 2 * r + 4
        assert len(row["active_edges_by_round"]) == r
        assert len(row["active_vertices_by_round"]) == r
        assert sum(row["active_edges_by_round"]) == row["comparison_edges_total"]
        assert sum(row["active_vertices_by_round"]) == row["active_vertices_total"]
        assert row["comparison_edges_total"] <= row["reserved_slots_per_party"]
        assert row["offline_material_total_bits"] == 8 * (
            row["t_payload_bytes_p0"] + row["t_payload_bytes_p1"])
        assert row["online_prg_calls_total"] == \
            row["online_dcf_length_doubling_prg_calls_total"]
        stages = ("score", "core", "inverse")
    else:
        assert row["online_rounds"] == 8 and row["comparison_edges_total"] == 8128
        assert row["offline_material_total_bits"] == 8 * sum(
            row[f"{party}_{field}"] for party in ("p0", "p1")
            for field in ("t_payload_bytes", "shuffle_payload_bytes"))
        assert row["offline_ot_communication_total_bits"] == 8 * (
            row["p0_ot_sent_bytes"] + row["p1_ot_sent_bytes"])
        assert row["p0_ot_sent_bytes"] == row["p1_ot_received_bytes"]
        assert row["p1_ot_sent_bytes"] == row["p0_ot_received_bytes"]
        assert row["online_prg_calls_total"] == sum(
            row[f"{party}_{stage}_dcf_prg"] for party in ("p0", "p1")
            for stage in ("score", "pipeline"))
        stages = ("score", "forward", "cmpagg", "reveal", "reverse")
    for party, peer in (("p0", "p1"), ("p1", "p0")):
        assert sum(row[f"{party}_{stage}_sent_bytes"] for stage in stages) == \
            row[f"{party}_sent_bytes"]
        assert sum(row[f"{party}_{stage}_received_bytes"] for stage in stages) == \
            row[f"{party}_received_bytes"]
        for stage in stages:
            assert row[f"{party}_{stage}_received_bytes"] == \
                row[f"{peer}_{stage}_sent_bytes"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-root", type=Path, required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--source-root", type=Path)
    parser.add_argument("--aav-binary", type=Path)
    parser.add_argument("--baseline-binary", type=Path)
    args = parser.parse_args()
    root = args.raw_root
    plan = json.loads((root / "input_plan.json").read_text(encoding="utf-8"))
    pairing = {}
    counts = {}
    for route in ("aav86", "baseline"):
        for profile in ("LAN", "WAN"):
            directory = root / route
            filename = (f"{profile}_runs.jsonl" if route == "aav86" else
                        f"{profile}_baseline_runs.jsonl")
            summary = (f"{profile}_summary.json" if route == "aav86" else
                       f"{profile}_baseline_summary.json")
            source = directory / filename
            items = rows(source)
            assert len(items) == (48 if route == "aav86" else 12)
            assert sum(not row["warmup"] for row in items) == (
                40 if route == "aav86" else 10)
            if args.source_root is not None:
                for row in items:
                    for name, expected in row["source_sha256"].items():
                        assert digest(args.source_root / name) == expected, name
            binary = args.aav_binary if route == "aav86" else args.baseline_binary
            if binary is not None:
                actual_binary = digest(binary)
                assert all(row["binary_sha256"] == actual_binary for row in items)
            for row in items:
                check_row(row, args.head, route, plan)
                key = (profile, row["K"], row["repetition"])
                if key in pairing:
                    assert pairing[key] == (row["input_seed"], row["input_digest"])
                else:
                    pairing[key] = (row["input_seed"], row["input_digest"])
            groups = check_statistics(source, directory / summary)
            counts[f"{route}/{profile}"] = {"rows": len(items), "groups": groups,
                                           "sha256": digest(source)}
    assert len(pairing) == 24
    print(json.dumps({"status": "PASS", "head": args.head,
                      "pairing_groups": len(pairing), "counts": counts},
                     sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
