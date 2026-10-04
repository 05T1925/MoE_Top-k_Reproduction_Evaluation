#!/usr/bin/env python3
"""Create a complete per-file SHA-256 index for an external E20 evidence root."""

import argparse
import csv
import hashlib
from pathlib import Path


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest().upper()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("--name", default="e20_evidence_index.csv")
    args = parser.parse_args()
    root = args.root.resolve()
    target = root / args.name
    if target.exists():
        raise RuntimeError("index already exists; preserve frozen raw evidence")
    files = sorted(path for path in root.rglob("*") if path.is_file())
    if not files or any(path.is_symlink() for path in files):
        raise RuntimeError("empty evidence or unsupported symlink")
    with target.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=("relative_path", "bytes", "sha256"))
        writer.writeheader()
        for path in files:
            writer.writerow({"relative_path": path.relative_to(root).as_posix(),
                             "bytes": path.stat().st_size, "sha256": digest(path)})
    print(f"E20_EVIDENCE_INDEX files={len(files)} index_sha256={digest(target)}")


if __name__ == "__main__":
    main()
