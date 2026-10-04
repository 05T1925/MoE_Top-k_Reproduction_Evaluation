"""Freeze the separate E17 evidence tree with a one-time SHA-256 index."""
import csv
import hashlib
from pathlib import Path
import sys

root=Path(sys.argv[1]).resolve()
index=root/(sys.argv[2] if len(sys.argv)>2 else "e17_raw_complete_index.csv")
if index.exists():raise RuntimeError("refusing to replace E17 index")
files=sorted(p for p in root.rglob("*") if p.is_file())
with index.open("x",newline="",encoding="utf-8") as stream:
    writer=csv.writer(stream)
    writer.writerow(("relative_path","sha256","bytes"))
    for p in files:
        writer.writerow((p.relative_to(root).as_posix(),hashlib.sha256(p.read_bytes()).hexdigest().upper(),p.stat().st_size))
print(f"indexed={len(files)} sha256={hashlib.sha256(index.read_bytes()).hexdigest().upper()}")
