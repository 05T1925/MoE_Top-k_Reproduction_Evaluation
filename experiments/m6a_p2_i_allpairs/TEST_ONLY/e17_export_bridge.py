"""Export the separate E17 n=128 1+5 bridge summary; refuse overwrite."""
import csv
import json
from pathlib import Path
import sys

root=Path(sys.argv[1])
dest=root/"unified_nine_metrics_n128_bridge.csv"
if dest.exists():raise RuntimeError("refusing overwrite")
metrics=("offline_time_ms","offline_material_total_bits","online_time_ms",
         "total_time_ms","online_comm_total_bits","online_comm_per_party_bits",
         "online_rounds","online_prg_calls_total","comparison_edges_total")
columns=["profile","route","n","K","r","source_revision","formal_runs","warmup_runs"] + [f"{m}_{s}" for m in metrics for s in ("median","min","max")]
rows=[]
for profile in ("LAN","WAN"):
    summary=json.loads((root/profile/f"{profile}_n128_bridge_summary.json").read_text())
    for name,values in summary["groups"].items():
        _,route,k,r=name.split("-")
        row=dict(profile=profile,route=route,n=128,K=int(k[1:]),r=int(r[1:]) if route=="aav86" else "",
                 source_revision=summary["source_revision"],formal_runs=5,warmup_runs=1)
        row.update({f"{m}_{s}":values[m][s] for m in metrics for s in ("median","min","max")})
        rows.append(row)
assert len(rows)==20
with dest.open("x",newline="",encoding="utf-8") as stream:
    writer=csv.DictWriter(stream,fieldnames=columns)
    writer.writeheader();writer.writerows(rows)
print(f"wrote {len(rows)} groups to {dest}")
