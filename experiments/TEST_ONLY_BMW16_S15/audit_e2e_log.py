#!/usr/bin/env python3
"""Independent consistency checks for the TEST_ONLY C++ process transcript."""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def kv_block(case: str, name: str, end: str) -> dict[str, str]:
    start_marker = name + "={"
    start = case.find(start_marker)
    if start < 0:
        return {}
    payload = case[start + len(start_marker):]
    stop = payload.find(end)
    if stop < 0:
        return {}
    payload = payload[:stop].replace("\\n", "\n")
    result: dict[str, str] = {}
    for line in payload.splitlines():
        if "=" in line:
            key, value = line.strip().split("=", 1)
            result[key] = value
    return result


def next_power_two(n: int) -> int:
    p = 2
    while p < n:
        p *= 2
    return p


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log", type=Path)
    args = ap.parse_args()
    text = args.log.read_text(encoding="utf-8", errors="strict")
    lines = text.splitlines()
    starts = [i for i, line in enumerate(lines) if line.startswith("case n=")]
    cases = []
    failures = []
    for idx, start in enumerate(starts):
        end = starts[idx + 1] if idx + 1 < len(starts) else len(lines)
        block = "\n".join(lines[start:end])
        # The test runner embeds some nested metric files as literal backslash-n
        # text in its one-line case record; normalize only for independent parsing.
        block = block.replace("\\n", "\n")
        head = re.match(r"case n=(\d+) K=(\d+) seed=(\d+) status=(\S+) scope=(\S+) state=(\S+) p0_exit=(\d+) p1_exit=(\d+) p0_slot_audit=(\d+):([0-9a-f]+) p1_slot_audit=(\d+):([0-9a-f]+)", lines[start])
        if not head:
            failures.append(f"line {start + 1}: malformed case header")
            continue
        n, k, seed = map(int, head.group(1, 2, 3))
        status, scope, state = head.group(4, 5, 6)
        p0, p1, dealer = kv_block(block, "party0_metrics", "} party1_metrics={"), kv_block(block, "party1_metrics", "} dealer={"), kv_block(block, "dealer", "}")
        case_errors = []
        if ("exit_code" in p0 and int(head.group(7)) != int(p0["exit_code"])) or (
                "exit_code" in p1 and int(head.group(8)) != int(p1["exit_code"])):
            case_errors.append("exit-code/header mismatch")
        if p0.get("status") != p1.get("status") or p0.get("scope") != p1.get("scope"):
            case_errors.append("party status/scope disagreement")
        if p0 and p1:
            for party, peer in ((p0, p1), (p1, p0)):
                counters_on = int(party.get("dcf_counters_enabled", 0)) != 0
                if counters_on and "dcf_eval_calls" in party and "dcf_eval" in party and int(party["dcf_eval_calls"]) != int(party["dcf_eval"]):
                    case_errors.append("DCF Eval call counter != protocol DCF eval metric")
                if counters_on and int(party.get("dcf_keygen_calls", 0)) != 0:
                    case_errors.append("online party performed DCF keygen")
                if "sent" in party and "recv" in peer and int(party["sent"]) != int(peer["recv"]):
                    case_errors.append("sent/received byte conservation failed")
            if head.group(9) != head.group(11) or head.group(10) != head.group(12):
                case_errors.append("two local slot audit digests/counts differ")
            if n > 1 and status in ("SUCCESS", "ABORT_ALGORITHM_PROBABILITY", "ABORT_ALGORITHM_INVALID"):
                pairs = n * (n - 1) // 2
                padded = next_power_two(n)
                bits = 33 + (padded.bit_length() - 1)
                expected_slots = 2 * padded + 2 + 9 * pairs
                if int(dealer["slots_per_party"]) != expected_slots:
                    case_errors.append("dealer slot coverage identity failed")
                if int(dealer.get("dcf_counters_enabled", 0)) != 0:
                    expected_keygens = 2 * padded + 9 * pairs
                    if int(dealer["dcf_keygen_calls"]) != expected_keygens:
                        case_errors.append("offline DCF keygen count identity failed")
                    expected_expansions = 2 * (2 * padded * 34 + 9 * pairs * bits)
                    if int(dealer["dcf_keygen_node_expansions"]) != expected_expansions:
                        case_errors.append("offline DCF PRG expansion identity failed")
                    if int(dealer["dcf_eval_calls_during_generation"]) != 0 or int(dealer["dcf_eval_node_expansions_during_generation"]) != 0:
                        case_errors.append("T evaluated DCF during offline generation")
            if "claimed_slots" in p0 and (int(p0["claimed_slots"]) != int(head.group(9)) or int(p1["claimed_slots"]) != int(head.group(11))):
                case_errors.append("durable/local one-shot claim count mismatch")
        record = {"n": n, "K": k, "seed": seed, "status": status, "scope": scope,
                  "state": state, "party0": int(head.group(7)), "party1": int(head.group(8)),
                  "party0_logical": int(p0["logical"]) if "logical" in p0 else None,
                  "party0_dcf_eval": int(p0["dcf_eval"]) if "dcf_eval" in p0 else None,
                  "dealer_slots_per_party": int(dealer["slots_per_party"]) if "slots_per_party" in dealer else None,
                  "dealer_keygen_calls": int(dealer["dcf_keygen_calls"]) if "dcf_keygen_calls" in dealer else None,
                  "errors": case_errors}
        cases.append(record)
        if case_errors:
            failures.extend(f"n={n},K={k},seed={seed}: {e}" for e in case_errors)
    report = {"label": "BMW16_S15_INDEPENDENT_E2E_COUNTER_AUDIT_TEST_ONLY",
              "source_log": str(args.log), "case_count": len(cases), "cases": cases,
              "result": "PASS" if not failures else "FAIL", "failures": failures}
    print(json.dumps(report, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
