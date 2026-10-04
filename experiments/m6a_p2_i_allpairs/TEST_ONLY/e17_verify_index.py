"""Read-only verifier for the separate E17 raw evidence index."""
import csv
import hashlib
from pathlib import Path
import sys

root=Path(sys.argv[1])
index=root/(sys.argv[2] if len(sys.argv)>2 else "e17_raw_complete_index.csv")
rows=list(csv.DictReader(index.open(newline="",encoding="utf-8-sig")))
assert len(rows)==len({x["relative_path"] for x in rows})
for x in rows:
    path=root/x["relative_path"]
    assert path.is_file() and path.stat().st_size==int(x["bytes"])
    assert hashlib.sha256(path.read_bytes()).hexdigest().upper()==x["sha256"]
assert {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()} == \
    {x["relative_path"] for x in rows} | {index.name}
print(f"PASS files={len(rows)} index_sha256={hashlib.sha256(index.read_bytes()).hexdigest().upper()}")
