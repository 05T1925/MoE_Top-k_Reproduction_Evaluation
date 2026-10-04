"""E17 n=128 bridge on the frozen E16 binaries; raw output stays outside Git."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import subprocess

HEAD = "7515aac64e8c7779895017d7c735285cb44dc336"
BINARIES = {
    "aav86": "A1648F24DA0C6DDD88A3A6B561700A2907ECE57125068D25A9F210E6E013AF79",
    "baseline": "A07737585DC14ABABCDD8DF915FBC69FC11C26F61C75C89D52B5B4399B8C3A69",
}
METRICS = ("offline_time_ms", "offline_material_total_bits", "online_time_ms",
           "total_time_ms", "online_comm_total_bits", "online_comm_per_party_bits",
           "online_rounds", "online_prg_calls_total", "comparison_edges_total")
def require(ok, reason):
    if not ok:
        raise RuntimeError(reason)
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()
def fields(line):
    result = {}
    for item in line.split()[1:]:
        key, value = item.split("=", 1)
        result[key] = [int(x) for x in value.split(",") if x] if key in (
            "active_by_round", "vertices_by_round", "ca_prg_by_round") else (
            value if key == "transport" else int(value))
    return result
def gitdir(repo):
    marker=(repo/".git").read_text().strip()
    require(marker.startswith("gitdir: "),"worktree git marker")
    path=marker[8:]
    if len(path)>3 and path[1:3]==":/":
        path="/mnt/"+path[0].lower()+"/"+path[3:]
    return str(Path(path).resolve())
def source_hash(repo, file):
    expected = subprocess.check_output(["git",f"--git-dir={gitdir(repo)}",
                                        f"--work-tree={repo}","show",f"{HEAD}:{file}"])
    actual = (repo / file).read_bytes()
    require(actual.replace(b"\r\n",b"\n") == expected,
            f"source differs from frozen E16: {file}")
    return hashlib.sha256(actual).hexdigest().upper()
def row(case, route, profile, k, r, rep, seed, calibration, common):
    d, pairs, bits = 128, 8128, 40
    require(case["n"] == case["d"] == d and case["k"] == k, "shape")
    require(all(case[f"{p}_exit"] == 0 for p in ("t","p0","p1")), "role exit")
    require((case["p0_bytes"] == case["p1_received_bytes"] and
            case["p1_bytes"] == case["p0_received_bytes"]) if route == "aav86" else
            (case["p0_sent_bytes"] == case["p1_received_bytes"] and
             case["p1_sent_bytes"] == case["p0_received_bytes"]), "direction conservation")
    require(case["online_max_ns"] == max(case["online_p0_ns"],case["online_p1_ns"])
            if route == "baseline" else case["online_max_elapsed_ns"] == max(
                case["online_p0_ns"],case["online_p1_ns"]), "online clock")
    if route == "aav86":
        require(case["r"] == r and case["reserved"] == r*pairs, "reserved slots")
        require(case["package_bytes_p0"] == case["package_bytes_p1"] ==
                114+d*(1844+8*r)+r*pairs*(81+24*bits), "package length")
        require(sum(case["active_by_round"]) == case["active"] and
                sum(case["vertices_by_round"]) == case["active_vertices"] and
                sum(case["ca_prg_by_round"]) == case["ca_prg_per_party"], "round vectors")
        require(len(case["active_by_round"]) == r and
                all(q == 2*e*bits for q,e in zip(case["ca_prg_by_round"],case["active_by_round"])), "PRG round definition")
        require(case["online_prg_total"] == 2*(case["score_prg_per_party"]+
                case["ca_prg_per_party"]+case["inverse_prg_per_party"]), "PRG stages")
        require(case["rounds"] == 2*r+4, "rounds")
        for p in ("p0","p1"):
            require(sum(case[f"{p}_{s}_bytes"] for s in ("score","core","inverse")) == case[f"{p}_bytes"] and
                    sum(case[f"{p}_{s}_received_bytes"] for s in ("score","core","inverse")) == case[f"{p}_received_bytes"], "stage communication")
        t_bytes = (48+8*r)*d+2*d*(16+840)+r*pairs*(24*bits+24)
        require(case["t_payload_bytes_p0"] == case["t_payload_bytes_p1"] == t_bytes, "material payload")
        offline, online = case["offline_elapsed_ns"],case["online_max_elapsed_ns"]
        sent0,sent1 = case["p0_bytes"],case["p1_bytes"]
        material,prg,edges = 16*t_bytes,case["online_prg_total"],case["active"]
        digest = None
    else:
        require(case["input_seed"] == seed and case["edges"] == pairs and
                case["rounds"] == 8, "baseline shape")
        require(case["dcf_eval_per_party"] == 4*d+2*pairs, "baseline DCF work")
        for p in ("p0","p1"):
            require(sum(case[f"{p}_{s}_sent_bytes"] for s in ("score","forward","cmpagg","reveal","reverse")) == case[f"{p}_sent_bytes"] and
                    sum(case[f"{p}_{s}_received_bytes"] for s in ("score","forward","cmpagg","reveal","reverse")) == case[f"{p}_received_bytes"], "baseline stage communication")
        t_bytes = 8*d+2*d*(16+840)+pairs*(24*bits+24)
        shuffle = d*(64+152*(2*7-1))
        for p in ("p0","p1"):
            require(case[f"{p}_t_payload_bytes"] == t_bytes and
                    case[f"{p}_shuffle_payload_bytes"] == shuffle, "baseline material")
        offline, online = case["offline_ns"],case["online_max_ns"]
        sent0,sent1 = case["p0_sent_bytes"],case["p1_sent_bytes"]
        material = 16*(t_bytes+shuffle)
        prg = sum(case[f"{p}_{s}_dcf_prg"] for p in ("p0","p1") for s in ("score","pipeline"))
        edges,digest = pairs,case["input_digest"]
    result = dict(common, profile=profile, route=route, n=128, K=k, r=r if route=="aav86" else None,
                  repetition=rep, warmup=rep==0, status="PASS", input_seed=seed,
                  input_digest=digest, network_rtt_median_ms=calibration["rtt_median_ms"],
                  network_throughput_mbps=calibration["throughput_mbps"],
                  offline_time_ms=offline/1e6, online_time_ms=online/1e6,
                  total_time_ms=(offline+online)/1e6,
                  offline_material_total_bits=material,
                  online_comm_total_bits=8*(sent0+sent1),
                  online_comm_per_party_bits=4*(sent0+sent1),
                  online_rounds=case["rounds"], online_prg_calls_total=prg,
                  comparison_edges_total=edges, exit_t=case["t_exit"],
                  exit_p0=case["p0_exit"],exit_p1=case["p1_exit"],
                  peak_t_kib=case["peak_t_kib"],peak_p0_kib=case["peak_p0_kib"],
                  peak_p1_kib=case["peak_p1_kib"])
    return result
def main():
    p=argparse.ArgumentParser()
    for name in ("source-root","output-dir","input-plan","calibration","aav86-binary","baseline-binary"):
        p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--profile",choices=("LAN","WAN"),required=True)
    args=p.parse_args()
    repo=args.source_root.resolve()
    source_files=("VFSS/src/moe_topk/protocol_i_aav86_small.cpp",
                  "VFSS/include/moe_topk/protocol_i_aav86_small.h",
                  "VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp",
                  "VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp",
                  "VFSS/tests/moe_topk/protocol_i_e14_material_metrics.h",
                  "VFSS/ext/FSS/dcf.cpp","VFSS/CMakeLists.txt")
    hashes={f:source_hash(repo,f) for f in source_files}
    bins={"aav86":args.aav86_binary,"baseline":args.baseline_binary}
    for route,b in bins.items(): require(sha(b)==BINARIES[route],f"frozen binary {route}")
    plan=json.loads(args.input_plan.read_text())
    require(len(plan)==12 and len(set(plan.values()))==12, "input plan")
    cal=json.loads(args.calibration.read_text())
    require(cal["profile"]==args.profile and (0.5 <= cal["rtt_median_ms"] <= 5 if args.profile=="LAN" else 35 <= cal["rtt_median_ms"] <= 70),"network calibration")
    out=args.output_dir
    out.mkdir(parents=True,exist_ok=True)
    raw=out/f"{args.profile}_n128_bridge_runs.jsonl"
    require(not raw.exists(),"refuse overwrite")
    common=dict(source_revision=HEAD,source_sha256=hashes,bridge_label="E17_E16_REVISION_N128",
                input_plan_sha256=sha(args.input_plan),calibration_sha256=sha(args.calibration),
                timing_contract="E15 post-online material accounting; offline before fork through T exit and both ready; online=max secure party calls",
                measurement_layer="framed application bytes; wire overhead NOT_MEASURED",
                rlimit_as_bytes=805306368)
    records=[]
    with raw.open("x") as stream:
        for route in ("aav86","baseline"):
            for r in ((2,3,4,5) if route=="aav86" else (0,)):
                for k in (2,8):
                    for rep in range(6):
                        seed=plan[f"k{k}-rep{rep}"]
                        serial=(72 if args.profile=="LAN" else 73)*1000000+k*10000+r*100+rep
                        cmd=[str(bins[route]),"bench","128",str(k)]
                        if route=="aav86":cmd.append(str(r))
                        cmd += [str(seed),str(serial)]
                        env=os.environ.copy()
                        env["MOE_TOPK_M6A_E15_BENCH"]="1"
                        if route=="aav86":
                            env["MOE_TOPK_M6A_E11_TRANSPORT"]="tcp"
                            env["MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED"]=str(0xA170000+r*1000+rep)
                        else:env["MOE_TOPK_M2_E12_TRANSPORT"]="tcp"
                        run_id=f"{args.profile}-n128-{route}-k{k}-r{r}-rep{rep}"
                        log=out/f"{run_id}.log"
                        try:
                            result=subprocess.run(cmd,env=env,text=True,stdout=subprocess.PIPE,
                                                  stderr=subprocess.STDOUT,timeout=120)
                            log.write_text(result.stdout)
                            require(result.returncode==0,f"exit {result.returncode}")
                            case=next(fields(s) for s in result.stdout.splitlines() if s.startswith(
                                "E12_AAV86_CASE " if route=="aav86" else "E12_BASELINE_CASE "))
                            entry=row(case,route,args.profile,k,r,rep,seed,cal,common)
                            if route=="aav86":
                                meta=next(fields(s) for s in result.stdout.splitlines() if s.startswith("E12_BENCH_META "))
                                require(meta["input_seed"]==seed,"input seed")
                                entry["input_digest"]=meta["input_digest"]
                            entry.update(run_id=run_id,command=cmd,binary_sha256=BINARIES[route],
                                         raw_log=str(log),raw_log_sha256=sha(log))
                        except Exception as exc:
                            if not log.exists():log.write_text(str(exc)+"\n")
                            entry=dict(common,run_id=run_id,status="FAILED",reason=str(exc),command=cmd,
                                       raw_log=str(log),raw_log_sha256=sha(log))
                        stream.write(json.dumps(entry,sort_keys=True)+"\n");stream.flush();os.fsync(stream.fileno())
                        print(run_id,entry["status"],flush=True)
                        require(entry["status"]=="PASS",f"stop failed {run_id}")
                        records.append(entry)
    grouped={}
    for profile in (args.profile,):
        for k in (2,8):
            for rep in range(6):
                inputs={(x["input_seed"],x["input_digest"]) for x in records if x["K"]==k and x["repetition"]==rep}
                require(len(inputs)==1,f"paired input {profile,k,rep}")
            for route in ("aav86","baseline"):
                for r in ((2,3,4,5) if route=="aav86" else (0,)):
                    sample=[x for x in records if x["K"]==k and x["route"]==route and (x["r"] or 0)==r and not x["warmup"]]
                    require(len(sample)==5,"formal sample")
                    grouped[f"{profile}-{route}-k{k}-r{r}"]={m:{"median":statistics.median(x[m] for x in sample),"min":min(x[m] for x in sample),"max":max(x[m] for x in sample)} for m in METRICS}
    (out/f"{args.profile}_n128_bridge_summary.json").write_text(json.dumps(dict(source_revision=HEAD,
        runs=len(records),formal=sum(not x["warmup"] for x in records),raw_sha256=sha(raw),groups=grouped),indent=2,sort_keys=True)+"\n")
if __name__=="__main__":main()
