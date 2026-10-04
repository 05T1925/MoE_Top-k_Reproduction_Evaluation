"""Read-only independent check of the E17 same-revision n=128 bridge."""
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import sys
from collections import defaultdict

root=Path(sys.argv[1])
e16_plan=Path(sys.argv[2])
plan=json.loads(e16_plan.read_text())
metrics=("offline_time_ms","offline_material_total_bits","online_time_ms",
         "total_time_ms","online_comm_total_bits","online_comm_per_party_bits",
         "online_rounds","online_prg_calls_total","comparison_edges_total")
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest().upper()
def close(a,b):return math.isclose(float(a),float(b),rel_tol=1e-11,abs_tol=1e-5)
all_rows=[]
paired=defaultdict(set)
groups=defaultdict(list)
for profile in ("LAN","WAN"):
    base=root/profile
    calibration=base/f"{profile}_calibration.json"
    cal=json.loads(calibration.read_text())
    assert cal["profile"]==profile and cal["rtt_samples"]==20
    rows=[json.loads(s) for s in (base/f"{profile}_n128_bridge_runs.jsonl").read_text().splitlines()]
    assert len(rows)==60
    summary=json.loads((base/f"{profile}_n128_bridge_summary.json").read_text())
    assert summary["runs"]==60 and summary["formal"]==50 and summary["raw_sha256"]==digest(base/f"{profile}_n128_bridge_runs.jsonl")
    for x in rows:
        route,k,rep,r=x["route"],x["K"],x["repetition"],x["r"] or 0
        assert x["status"]=="PASS" and x["source_revision"]=="7515aac64e8c7779895017d7c735285cb44dc336"
        assert x["n"]==128 and x["profile"]==profile and x["warmup"]==(rep==0)
        assert x["input_plan_sha256"]==digest(e16_plan) and x["input_seed"]==plan[f"k{k}-rep{rep}"]
        assert x["calibration_sha256"]==digest(calibration) and close(x["network_rtt_median_ms"],cal["rtt_median_ms"])
        assert x["rlimit_as_bytes"]==805306368 and all(x[f"exit_{p}"]==0 for p in ("t","p0","p1"))
        assert close(x["total_time_ms"],x["offline_time_ms"]+x["online_time_ms"])
        assert x["online_rounds"]==(2*r+4 if route=="aav86" else 8)
        assert x["online_comm_total_bits"]==2*x["online_comm_per_party_bits"]
        log_name=x["raw_log"]
        if os.name=="nt" and log_name.startswith("/mnt/c/"):
            log_name="C:/"+log_name[len("/mnt/c/"):]
        log=Path(log_name)
        assert log.is_file() and digest(log)==x["raw_log_sha256"]
        first=log.read_text().splitlines()[0]
        assert first.startswith("E12_AAV86_CASE " if route=="aav86" else "E12_BASELINE_CASE ")
        assert "t_exit=0" in first and "p0_exit=0" in first and "p1_exit=0" in first
        paired[(profile,k,rep)].add((x["input_seed"],x["input_digest"]))
        if not x["warmup"]:groups[(profile,route,k,r)].append(x)
        all_rows.append(x)
    assert len(summary["groups"])==10
    for name, stats in summary["groups"].items():
        _,route,k,r=name.split("-")
        sample=groups[(profile,route,int(k[1:]),int(r[1:]))]
        assert len(sample)==5
        for m in metrics:
            seq=[x[m] for x in sample]
            for stat,val in (("median",statistics.median(seq)),("min",min(seq)),("max",max(seq))):
                assert close(stats[m][stat],val),(name,m,stat)
assert len(all_rows)==120 and len(groups)==20 and all(len(v)==1 for v in paired.values()) and len(paired)==24
print(json.dumps({"result":"PASS","runs":len(all_rows),"formal_correct":100,
                  "paired_input_groups":len(paired),"five_run_groups":len(groups),
                  "metric_statistics":len(groups)*len(metrics)*3},indent=2))
