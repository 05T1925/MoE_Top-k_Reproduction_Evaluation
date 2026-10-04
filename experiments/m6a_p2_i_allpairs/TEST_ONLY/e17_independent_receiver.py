"""Read-only E17 verification of E16 raw artifacts; no E16 audit imports."""
import csv
import hashlib
import json
import math
import re
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(sys.argv[1]).resolve()
F = ROOT / "final_v2"
HEAD = "7515aac64e8c7779895017d7c735285cb44dc336"
METRICS = (
    "offline_time_ms", "offline_material_total_bits", "online_time_ms",
    "total_time_ms", "online_comm_total_bits", "online_comm_per_party_bits",
    "online_rounds", "online_prg_calls_total", "comparison_edges_total",
)
def check(ok, note):
    if not ok:
        raise AssertionError(note)
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()
def close(a, b):
    return math.isclose(float(a), float(b), rel_tol=1e-11, abs_tol=1e-5)

index = list(csv.DictReader((ROOT / "e16_raw_complete_index.csv").open(newline="", encoding="utf-8-sig")))
check(len(index) == 376, "index count")
check(len({x["relative_path"] for x in index}) == 376, "duplicate index path")
check({p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*") if p.is_file()} ==
      {x["relative_path"] for x in index} | {"e16_raw_complete_index.csv"},
      "unindexed or missing raw file")
for x in index:
    p = ROOT / x["relative_path"]
    check(p.is_file() and p.stat().st_size == int(x["bytes"]) and sha(p) == x["sha256"].upper(), f"index {p}")
plan = json.loads((F / "input_plan.json").read_text())
plan_sha = sha(F / "input_plan.json")
status = list(csv.DictReader((F / "configuration_status_32.csv").open(newline="")))
check(len(status) == 32, "shape count")
preflight = (ROOT / "gate/e16_final_preflight_3g.log").read_text()
accepted = {(int(n),int(r)) for n,r in re.findall(r"E16_PREFLIGHT_ACCEPTED n=(\d+) r=(\d+)",preflight)}
rejected = {(int(n),int(r),why) for n,r,why in re.findall(
    r"E16_PREFLIGHT_REJECTED n=(\d+) r=(\d+) reason=([A-Z0-9_]+)",preflight)}
check(accepted == {(256,r) for r in range(2,6)}, "actual accepted preflight")
check(rejected == {(n,r,"HARD_CAP_D_GT_256") for n in (1000,10000,100000,1000000)
                   for r in range(2,6)}, "actual first rejection")
gate = Counter()
for x in status:
    n, k, r = map(int, (x["n"], x["K"], x["r"]))
    d = 1 << max(1, (n-1).bit_length())
    b = 33 + (d.bit_length()-1)
    pairs = d*(d-1)//2
    slots = r*pairs
    package = 114 + d*(1844+8*r) + slots*(81+24*b)
    budget = 8*package + 64*1024*1024
    for col, val in (("padded_D",d),("pairs_per_round",pairs),("reserved_slots_per_party",slots),("party_package_bytes_formula",package),("dealer_budget_bytes_formula",budget)):
        check(int(x[col]) == val, f"shape {n,k,r} {col}")
    expected = "NOT_RUN_E16_NO_BRIDGE" if n == 128 else "MEASURED" if n == 256 else "PRECHECK_REJECTED"
    check(x["aav86_status"] == expected, f"status {n,k,r}")
    if n >= 1000:
        check(x["first_e16_reject_gate"] == "HARD_CAP_D_GT_256" and package > 192*1024*1024 and budget > 2*1024**3, f"first gate {n,k,r}")
    gate[expected] += 1

runs = []
grouped = defaultdict(list)
paired = defaultdict(dict)
log_count = 0
calibrations = []
for r in range(2,6):
    for route in (["aav86", "baseline"] if r == 2 else ["aav86"]):
        base = F / f"r{r}" / route
        for profile in ("LAN", "WAN"):
            name = f"{profile}_{'baseline_' if route == 'baseline' else ''}runs.jsonl"
            file = base / name
            lines = file.read_text().splitlines()
            check(len(lines) == 12, f"row count {file}")
            calibration = base / f"{profile}_calibration.json"
            c = json.loads(calibration.read_text())
            check(c["profile"] == profile and c["rtt_samples"] == 20 and c["throughput_mbps"] > 0, f"calibration {calibration}")
            calibrations.append((profile,c["rtt_median_ms"],c["throughput_mbps"]))
            for line in lines:
                x = json.loads(line)
                k, rep = x["K"], x["repetition"]
                check(x["n"] == x["D"] == 256 and k in (2,8) and rep in range(6), "dimensions")
                check(x["head"] == HEAD and x["tracked_state"] == "CLEAN", "source identity")
                check(x["binary_sha256"] == ("A1648F24DA0C6DDD88A3A6B561700A2907ECE57125068D25A9F210E6E013AF79" if route == "aav86" else "A07737585DC14ABABCDD8DF915FBC69FC11C26F61C75C89D52B5B4399B8C3A69"), "binary identity")
                check(x["profile"] == profile and x["status"] == "PASS" and x["correctness"] == "frozen_oracle_and_exact_K_PASS", "run status")
                check(all(x[f"exit_{p}"] == 0 for p in ("t","p0","p1")), "party exit")
                check(x["input_plan_sha256"].upper() == plan_sha and x["input_seed"] == plan[f"k{k}-rep{rep}"], "input plan")
                check(x["warmup"] == (rep == 0), "warmup")
                check(x["rlimit_as_bytes"] == 3*1024**3, "process limit") if route == "aav86" else None
                check(close(x["online_time_ms"],max(x["online_p0_ms"],x["online_p1_ms"])), "online max")
                check(close(x["total_time_ms"],x["offline_time_ms"]+x["online_time_ms"]), "time identity")
                check(x["p0_sent_bytes"] == x["p1_received_bytes"] and x["p1_sent_bytes"] == x["p0_received_bytes"], "direction conservation")
                check(x["online_comm_total_bits"] == 8*(x["p0_sent_bytes"]+x["p1_sent_bytes"]), "total communication")
                check(x["online_comm_per_party_bits"] == 8*x["p0_sent_bytes"] == 8*x["p1_sent_bytes"], "party communication")
                check(x["online_prg_calls_total"] == x["online_dcf_length_doubling_prg_calls_total"], "PRG definition")
                check(x["other_aes_prg_breakdown"] == "NOT_MEASURED", "AES scope")
                check(x["comparison_edges_total"] <= (r if route == "aav86" else 1)*32640, "active edges")
                check(x["online_rounds"] == (2*r+4 if route == "aav86" else 8), "rounds")
                if route == "aav86":
                    check(x["r"] == r and x["reserved_slots_per_party"] == r*32640, "slots")
                    check(x["package_bytes_p0"] == x["package_bytes_p1"] == 114+256*(1844+8*r)+r*32640*(81+24*41), "real package")
                    check(sum(x["active_edges_by_round"]) == x["comparison_edges_total"] and sum(x["active_vertices_by_round"]) == x["active_vertices_total"], "edge/vertex vectors")
                    check(len(x["active_edges_by_round"]) == len(x["ca_prg_by_round_per_party"]) == r, "round vectors")
                    check(all(0 <= v <= 256 and 0 <= e <= v*(v-1)//2 and p == 2*e*41 for v,e,p in zip(x["active_vertices_by_round"],x["active_edges_by_round"],x["ca_prg_by_round_per_party"])), "e_A/v_A/DCF length doubling")
                    check(sum(x["ca_prg_by_round_per_party"]) == x["ca_prg_per_party"], "CA PRG")
                    check(x["online_prg_calls_total"] == 2*(x["score_prg_per_party"]+x["ca_prg_per_party"]+x["inverse_prg_per_party"]), "PRG stages")
                    stages=("score","core","inverse")
                else:
                    check(x["reserved_pairs_per_party"] == 32640, "baseline slots")
                    check(x["online_prg_calls_total"] == 2*(x["p0_score_dcf_prg"]+x["p0_pipeline_dcf_prg"]), "baseline PRG")
                    stages=("score","cmpagg","forward","reveal","reverse")
                for p in ("p0","p1"):
                    check(sum(x[f"{p}_{s}_sent_bytes"] for s in stages) == x[f"{p}_sent_bytes"], f"{p} send stages")
                    check(sum(x[f"{p}_{s}_received_bytes"] for s in stages) == x[f"{p}_received_bytes"], f"{p} receive stages")
                raw_name = Path(x["raw_log"]).name
                log = base / raw_name
                check(log.is_file() and sha(log) == x["raw_log_sha256"].upper(), f"log sha {log}")
                first = log.read_text().splitlines()[0]
                check(first.startswith("E12_AAV86_CASE " if route == "aav86" else "E12_BASELINE_CASE ") and "t_exit=0" in first, f"log content {log}")
                observed = dict(token.split("=",1) for token in first.split()[1:])
                for field, value in (("n",256),("k",k),("d",256),("p0_exit",0),("p1_exit",0)):
                    check(int(observed[field]) == value, f"log {field} {log}")
                offline_field = "offline_elapsed_ns" if route == "aav86" else "offline_ns"
                online_field = "online_max_elapsed_ns" if route == "aav86" else "online_max_ns"
                check(close(int(observed[offline_field])/1e6,x["offline_time_ms"]) and close(int(observed[online_field])/1e6,x["online_time_ms"]), f"log times {log}")
                check(int(observed["p0_bytes" if route == "aav86" else "p0_sent_bytes"]) == x["p0_sent_bytes"], f"log bytes {log}")
                check(close(x["network_rtt_median_ms"],c["rtt_median_ms"]) and close(x["network_throughput_mbps"],c["throughput_mbps"]), "calibration mapping")
                for src, digest in x["source_sha256"].items():
                    check(sha(Path(__file__).resolve().parents[3] / src) == digest.upper(), f"source hash {src}")
                pair_key=(profile,k,rep)
                check(route+str(r) not in paired[pair_key], "duplicate pair")
                paired[pair_key][route+str(r)]=(x["input_seed"],x["input_digest"])
                runs.append(x)
                log_count += 1
                if rep > 0:
                    grouped[(profile,route,k,r if route == "aav86" else None)].append(x)

check(len(runs) == log_count == 120 and len(paired) == 24, "runs/pairs")
check(all(len(v)==5 and len(set(v.values()))==1 for v in paired.values()), "paired seeds and digests")
check(len(grouped)==20 and all(len(v)==5 for v in grouped.values()), "formal groups")
summary=list(csv.DictReader((F / "unified_nine_metrics.csv").open(newline="")))
check(len(summary)==20, "summary groups")
for y in summary:
    route=y["route"]
    key=(y["profile"],route,int(y["K"]),int(y["r"]) if route=="aav86" else None)
    vals=grouped[key]
    check(len(vals)==5 and y["head"]==HEAD and int(y["formal_runs"])==5 and int(y["warmup_runs"])==1, f"summary metadata {key}")
    for m in METRICS:
        series=[x[m] for x in vals]
        for stat, expected in (("median",statistics.median(series)),("min",min(series)),("max",max(series))):
            check(close(y[f"{m}_{stat}"],expected), f"summary {key} {m} {stat}")
print(json.dumps({"result":"PASS","indexed_files":len(index),"runs":len(runs),"verified_logs":log_count,"formal_correct":sum(not x["warmup"] for x in runs),"paired_input_groups":len(paired),"five_run_groups":len(grouped),"metric_statistics":len(grouped)*len(METRICS)*3,"configuration_status":dict(gate),"calibration_lan_rtt_ms_range":[min(v for p,v,_ in calibrations if p=="LAN"),max(v for p,v,_ in calibrations if p=="LAN")],"calibration_wan_rtt_ms_range":[min(v for p,v,_ in calibrations if p=="WAN"),max(v for p,v,_ in calibrations if p=="WAN")]},indent=2))
