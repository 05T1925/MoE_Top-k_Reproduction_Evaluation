#!/usr/bin/env python3
"""Read-only E13 audit of the frozen E12 accepted batch; no E12 audit imports."""

import argparse
import collections
import csv
import hashlib
import json
import pathlib
import re
import statistics

MEASURED_HEAD = "8ad0725c73716ac37c96d417136958f932233d61"
BINARIES = {
    "aav86": "805E2B5B96653ED6E5C1A0C2428A1A0ECB41A9A1DC5C9D6ABBD2975579578904",
    "baseline": "C767797CD712C2DCDA8D522AC1E9F7AD13E71E4DE35B1A8AE10DE4BB913A6456",
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def accepted_path(raw_root, recorded):
    marker = "/accepted/"
    normalized = str(recorded).replace("\\", "/")
    if marker not in normalized:
        raise ValueError(f"not an accepted path: {recorded}")
    return raw_root / normalized.split(marker, 1)[1]


def audit(raw_root, source_root):
    errors = []
    count = collections.Counter()

    def expect(condition, label):
        if not condition:
            errors.append(label)

    index = (raw_root / "e12_raw_index.sha256").read_text().splitlines()
    expect(len(index) == 151, f"index count {len(index)}")
    for entry in index:
        match = re.fullmatch(r"([a-f0-9]{64})  (.+)", entry)
        expect(match is not None, "index format")
        if match:
            path = accepted_path(raw_root, match.group(2))
            expect(path.is_file() and sha256(path) == match.group(1).upper(),
                   f"index hash {path}")
    count["indexed_files"] = len(index)

    all_rows = {}
    for implementation in ("aav86", "baseline"):
        for profile in ("LAN", "WAN"):
            stem = profile + ("_baseline" if implementation == "baseline" else "")
            directory = raw_root / implementation
            raw_file = directory / f"{stem}_runs.jsonl"
            rows = [json.loads(line) for line in raw_file.read_text().splitlines()]
            all_rows[implementation, profile] = rows
            expect(len(rows) == (48 if implementation == "aav86" else 12),
                   f"{implementation}/{profile} row count")
            summary = json.loads((directory / f"{stem}_summary.json").read_text())
            expect(summary["head"] == MEASURED_HEAD and
                   summary["raw_sha256"] == sha256(raw_file),
                   f"{implementation}/{profile} summary identity")
            calibration = directory / f"{profile}_calibration.json"
            cal = json.loads(calibration.read_text())
            qdisc = []
            for phase in ("before", "after_calibration", "after"):
                text = (directory / f"{profile}_qdisc_{phase}.txt").read_text()
                match = re.search(r"Sent (\d+) bytes (\d+) pkt", text)
                expect(match is not None, f"{implementation}/{profile} qdisc {phase}")
                if match:
                    qdisc.append((int(match.group(1)), int(match.group(2))))
            if len(qdisc) == 3:
                expect(qdisc[0][0] < qdisc[1][0] < qdisc[2][0],
                       f"{implementation}/{profile} qdisc progression")
                count[f"{implementation}_{profile}_qdisc_protocol_bytes"] = (
                    qdisc[2][0] - qdisc[1][0])

            groups = collections.defaultdict(list)
            seen_ids = set()
            for row in rows:
                run = row["run_id"]
                expect(run not in seen_ids, f"{run} duplicate")
                seen_ids.add(run)
                k, repetition = row["K"], row["repetition"]
                rounds = row.get("r")
                expect(row["head"] == MEASURED_HEAD and
                       row["tracked_state"] == "CLEAN" and
                       row["binary_sha256"] == BINARIES[implementation],
                       f"{run} source identity")
                expect(row["status"] == "PASS" and
                       row["correctness"] == "frozen_oracle_and_exact_K_PASS" and
                       row["n"] == row["D"] == 128 and
                       row["profile"] == profile, f"{run} correctness/shape")
                expect(row["warmup"] == (repetition == 0) and
                       0 <= repetition <= 5, f"{run} repeat")
                expect(row["calibration_sha256"] == sha256(calibration) and
                       row["network_rtt_median_ms"] == cal["rtt_median_ms"] and
                       row["network_throughput_mbps"] == cal["throughput_mbps"],
                       f"{run} calibration")
                for source, expected in row["source_sha256"].items():
                    path = source_root / source
                    expect(path.is_file() and sha256(path) == expected,
                           f"{run} source {source}")
                log_path = accepted_path(raw_root, row["raw_log"])
                expect(log_path.is_file() and
                       sha256(log_path) == row["raw_log_sha256"],
                       f"{run} raw log hash")
                if log_path.is_file():
                    log = log_path.read_text()
                    expect(str(row["input_seed"]) in log and
                           str(row["input_digest"]) in log and
                           "t_exit=0 p0_exit=0 p1_exit=0" in log,
                           f"{run} raw log input/exits")
                    fields = dict(part.split("=", 1) for part in
                                  log.splitlines()[0].split() if "=" in part)
                    raw_to_json = (
                        {"offline_elapsed_ns": "offline_time_ms",
                         "online_p0_ns": "online_p0_ms",
                         "online_p1_ns": "online_p1_ms",
                         "online_max_elapsed_ns": "online_time_ms",
                         "p0_bytes": "p0_sent_bytes",
                         "p1_bytes": "p1_sent_bytes",
                         "reserved": "reserved_slots_per_party",
                         "active": "comparison_edges_total",
                         "rounds": "online_rounds",
                         "score_eval_per_party": "score_dcf_eval_per_party",
                         "ca_eval_per_party": "ca_dcf_eval_per_party",
                         "total_eval_per_party": "dcf_eval_per_party",
                         "p0_received_bytes": "p0_received_bytes",
                         "p1_received_bytes": "p1_received_bytes",
                         "package_bytes_p0": "package_bytes_p0",
                         "package_bytes_p1": "package_bytes_p1"}
                        if implementation == "aav86" else
                        {"offline_ns": "offline_time_ms",
                         "online_p0_ns": "online_p0_ms",
                         "online_p1_ns": "online_p1_ms",
                         "online_max_ns": "online_time_ms",
                         "p0_sent_bytes": "p0_sent_bytes",
                         "p1_sent_bytes": "p1_sent_bytes",
                         "p0_received_bytes": "p0_received_bytes",
                         "p1_received_bytes": "p1_received_bytes",
                         "edges": "comparison_edges_total",
                         "rounds": "online_rounds",
                         "dcf_eval_per_party": "dcf_eval_per_party",
                         "p0_package_bytes": "package_bytes_p0",
                         "p1_package_bytes": "package_bytes_p1"})
                    for raw_key, json_key in raw_to_json.items():
                        expect(raw_key in fields, f"{run} log field {raw_key}")
                        if raw_key in fields:
                            factor = 1_000_000 if json_key.endswith("_ms") else 1
                            expect(abs(float(fields[raw_key]) -
                                       float(row[json_key]) * factor) < 0.05,
                                   f"{run} log/JSON {raw_key}")
                    if implementation == "aav86":
                        for raw_key, json_key in (
                                ("active_by_round", "active_edges_by_round"),
                                ("vertices_by_round", "active_vertices_by_round"),
                                ("ca_prg_by_round", "ca_prg_by_round_per_party")):
                            observed = [int(value) for value in
                                        fields[raw_key].split(",") if value]
                            expect(observed == row[json_key],
                                   f"{run} log/JSON {raw_key}")
                if implementation == "aav86":
                    expect(row["exit_t"] == row["exit_p0"] == row["exit_p1"] == 0,
                           f"{run} JSON exits")
                expect(abs(row["online_time_ms"] -
                           max(row["online_p0_ms"], row["online_p1_ms"])) < 1e-6,
                       f"{run} online max")
                expect(abs(row["total_time_ms"] - row["offline_time_ms"] -
                           row["online_time_ms"]) < 1e-6, f"{run} total time")
                expect(row["online_comm_total_bits"] ==
                       8 * (row["p0_sent_bytes"] + row["p1_sent_bytes"]),
                       f"{run} communication total")
                expect(row["p0_sent_bytes"] == row["p1_received_bytes"] and
                       row["p1_sent_bytes"] == row["p0_received_bytes"],
                       f"{run} cross-party communication")
                phases = (("score", "core", "inverse") if implementation == "aav86"
                          else ("score", "forward", "cmpagg", "reveal", "reverse"))
                for party, peer in (("p0", "p1"), ("p1", "p0")):
                    for direction in ("sent", "received"):
                        expect(sum(row[f"{party}_{phase}_{direction}_bytes"]
                                   for phase in phases) ==
                               row[f"{party}_{direction}_bytes"],
                               f"{run} {party} {direction} phase sum")
                    for phase in phases:
                        expect(row[f"{party}_{phase}_sent_bytes"] ==
                               row[f"{peer}_{phase}_received_bytes"],
                               f"{run} {party} {phase} peer accounting")
                if implementation == "aav86":
                    edges = row["active_edges_by_round"]
                    vertices = row["active_vertices_by_round"]
                    prg = row["ca_prg_by_round_per_party"]
                    expect(len(edges) == len(vertices) == len(prg) == rounds and
                           sum(edges) == row["comparison_edges_total"] and
                           sum(vertices) == row["active_vertices_total"],
                           f"{run} graph arrays")
                    expect(all(0 <= e <= 8128 for e in edges) and
                           all(0 <= v <= 128 for v in vertices) and
                           all(p == 80 * e for p, e in zip(prg, edges)),
                           f"{run} graph/PRG bounds")
                    expect(row["reserved_slots_per_party"] == 8128 * rounds and
                           row["score_dcf_eval_per_party"] == 512 and
                           row["ca_dcf_eval_per_party"] == 2 * sum(edges) and
                           row["dcf_eval_per_party"] == 512 + 2 * sum(edges),
                           f"{run} slots/DCF")
                    expect(row["score_prg_per_party"] == 17408 and
                           sum(prg) == row["ca_prg_per_party"] and
                           row["online_prg_calls_total"] == 2 * (
                               row["score_prg_per_party"] +
                               row["ca_prg_per_party"] +
                               row["inverse_prg_per_party"]),
                           f"{run} PRG totals")
                    expect(row["core_rounds"] == 2 * rounds + 1 and
                           row["online_rounds"] == 2 * rounds + 4,
                           f"{run} rounds")
                    for party in ("p0", "p1"):
                        expect(abs(row[f"combination_{party}_ms"] -
                                   row[f"ca_{party}_ms"] -
                                   row[f"carrier_{party}_ms"]) < 1e-6,
                               f"{run} CA/carrier timing")
                else:
                    expect(row["online_rounds"] == 8 and
                           row["comparison_edges_total"] == 8128 and
                           row["reserved_pairs_per_party"] == 8128 and
                           row["dcf_eval_per_party"] == 16768,
                           f"{run} baseline work")
                    expect(row["online_prg_calls_total"] == "NOT_MEASURED" and
                           row["offline_material_total_bits"].startswith(
                               "NOT_MEASURED"), f"{run} baseline missing metrics")
                key = f"n128-k{k}" + (
                    f"-r{rounds}" if implementation == "aav86" else "")
                groups[key].append(row)
            for key, group in groups.items():
                expect(len(group) == 6 and
                       sorted(item["repetition"] for item in group) == list(range(6)),
                       f"{implementation}/{profile}/{key} repetitions")
                formal = [item for item in group if item["repetition"] > 0]
                saved = summary["configurations"][key]
                expect(saved["formal_runs"] == 5 and
                       saved["warmups_excluded"] == 1,
                       f"{implementation}/{profile}/{key} summary size")
                for metric, stats in saved.items():
                    if not isinstance(stats, dict) or not {
                        "min", "median", "max"
                    }.issubset(stats):
                        continue
                    values = [item[metric] for item in formal]
                    expect(abs(stats["min"] - min(values)) < 1e-6 and
                           abs(stats["median"] - statistics.median(values)) < 1e-6 and
                           abs(stats["max"] - max(values)) < 1e-6,
                           f"{implementation}/{profile}/{key}/{metric} statistics")
            count[f"{implementation}_{profile}_all"] = len(rows)
            count[f"{implementation}_{profile}_formal"] = sum(
                item["repetition"] > 0 for item in rows)
            count[f"{implementation}_{profile}_groups"] = len(groups)

    for profile in ("LAN", "WAN"):
        aav = {(item["K"], item["repetition"], item["r"]): item
               for item in all_rows["aav86", profile]}
        for base in all_rows["baseline", profile]:
            for r in (2, 3, 4, 5):
                peer = aav[base["K"], base["repetition"], r]
                expect(base["input_seed"] == peer["input_seed"] and
                       base["input_digest"] == peer["input_digest"],
                       f"{profile}/K{base['K']}/rep{base['repetition']}/r{r} input")

    capacity = list(csv.DictReader(
        (raw_root / "e12_v3_capacity_status.csv").open(newline="")))
    expect(len(capacity) == 32, "capacity row count")
    for row in capacity:
        n, r = int(row["n"]), int(row["r"])
        domain = max(2, 1 << (n - 1).bit_length())
        expect(int(row["D"]) == domain and
               int(row["slots_per_party"]) == r * domain * (domain - 1) // 2,
               f"capacity shape n{n}/r{r}")
        if n >= 256:
            expect(row["actual_preflight_reason"] == "HARD_CAP_D_GT_128" and
                   row["LAN_status"] == row["WAN_status"] ==
                   "PRECHECK_REJECTED_NO_KEYGEN",
                   f"capacity rejection n{n}/r{r}")
    count["capacity_rows"] = len(capacity)
    count["capacity_rejected"] = sum(int(row["n"]) >= 256 for row in capacity)
    return count, errors


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-root", type=pathlib.Path, required=True)
    parser.add_argument("--source-root", type=pathlib.Path, required=True)
    args = parser.parse_args()
    counts, errors = audit(args.raw_root.resolve(), args.source_root.resolve())
    print(json.dumps({"counts": counts, "errors": errors}, indent=2, sort_keys=True))
    raise SystemExit(1 if errors else 0)


if __name__ == "__main__":
    main()
