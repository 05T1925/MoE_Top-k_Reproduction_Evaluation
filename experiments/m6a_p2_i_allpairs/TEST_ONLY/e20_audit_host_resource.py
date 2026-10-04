#!/usr/bin/env python3
"""Join E20 raw run clocks with the independent Windows resource sampler."""

import argparse
import csv
import json
from pathlib import Path
import statistics


def main():
    p = argparse.ArgumentParser()
    p.add_argument("root", type=Path)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    root = args.root.resolve()
    with (root / "host_resource_samples.csv").open(newline="") as f:
        samples = list(csv.DictReader(f))
    samples = [(int(x["utc_ms"]), int(x["available_physical_bytes"]),
                float(x["page_reads_per_sec"]), float(x["page_writes_per_sec"]),
                int(x["vmmem_working_set_bytes"])) for x in samples]
    if any(a[0] >= b[0] for a, b in zip(samples,samples[1:])):
        raise RuntimeError("host monitor timestamps not monotonic")
    rows=[]
    for path in sorted(root.rglob("runs.jsonl")):
        for line in path.read_text().splitlines():
            run = json.loads(line)
            lo = run["started_utc_ns"]//1000000
            hi = (run["finished_utc_ns"]+999999)//1000000
            window = [x for x in samples if lo <= x[0] <= hi]
            row = {"raw_path": path.relative_to(root).as_posix(),
                   "run_id":run["run_id"], "implementation":run["implementation"],
                   "status":run["status"], "source_revision":run["source_revision"],
                   "sample_count":len(window)}
            if window:
                row.update(min_available_physical_bytes=min(x[1] for x in window),
                           median_page_reads_per_sec=statistics.median(x[2] for x in window),
                           max_page_reads_per_sec=max(x[2] for x in window),
                           median_page_writes_per_sec=statistics.median(x[3] for x in window),
                           max_page_writes_per_sec=max(x[3] for x in window),
                           max_vmmem_working_set_bytes=max(x[4] for x in window))
            else:
                row.update({key:"NOT_MEASURED" for key in
                    ("min_available_physical_bytes", "median_page_reads_per_sec",
                     "max_page_reads_per_sec", "median_page_writes_per_sec",
                     "max_page_writes_per_sec", "max_vmmem_working_set_bytes")})
            rows.append(row)
    if not rows:
        raise RuntimeError("no E20 raw run records")
    with args.output.open("x", newline="") as f:
        writer=csv.DictWriter(f,fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"E20_HOST_RESOURCE_AUDIT rows={len(rows)} samples={len(samples)}")


if __name__ == "__main__":
    main()
