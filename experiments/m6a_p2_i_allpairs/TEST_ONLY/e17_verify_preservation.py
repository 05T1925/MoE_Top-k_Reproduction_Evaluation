"""Read-only equality check for frozen E15/E16 raw indexes and their copies."""
import csv
import hashlib
import json
import sys
from pathlib import Path

e15, e15_copy, e16, e16_copy = (Path(x) for x in sys.argv[1:5])
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()
def verify(original, copy, index_name, rows):
    index = original / index_name
    assert digest(index) == digest(copy / index_name), index_name
    for relative, expected in rows:
        assert digest(original / relative) == expected, f"original {relative}"
        assert digest(copy / relative) == expected, f"copy {relative}"
    assert {p.relative_to(original).as_posix() for p in original.rglob("*") if p.is_file()} == \
        {relative for relative, _ in rows} | {index_name}, "original inventory"
    assert {p.relative_to(copy).as_posix() for p in copy.rglob("*") if p.is_file()} == \
        {relative for relative, _ in rows} | {index_name}, "copy inventory"
    return {"files":len(rows), "index_sha256":digest(index), "original_and_copy":"MATCH"}
e15_rows = []
for line in (e15 / "e15_raw_complete_index.sha256").read_text().splitlines():
    hash_value, relative = line.split(maxsplit=1)
    e15_rows.append((relative.removeprefix("./"), hash_value.upper()))
e16_rows = [(x["relative_path"], x["sha256"].upper()) for x in csv.DictReader(
    (e16 / "e16_raw_complete_index.csv").open(newline="", encoding="utf-8-sig"))]
assert len(e15_rows)==151 and len(e16_rows)==376
print(json.dumps({"E15":verify(e15,e15_copy,"e15_raw_complete_index.sha256",e15_rows),
                  "E16":verify(e16,e16_copy,"e16_raw_complete_index.csv",e16_rows)},indent=2))
