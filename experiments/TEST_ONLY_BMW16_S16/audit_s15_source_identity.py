#!/usr/bin/env python3
"""Check S15's declared file hashes against its Git source revision/history."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run_git(args: list[str]) -> bytes:
    return subprocess.run(["git", *args], check=True, capture_output=True).stdout


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest", type=Path, help="external S15_SOURCE_AND_BINARY_HASHES.txt")
    ap.add_argument("--revision", required=True, help="S15 declared measured source commit")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    rows = []
    for line in args.manifest.read_text(encoding="utf-8").splitlines()[2:]:
        parts = line.split("  ", 1)
        if len(parts) != 2 or parts[1].startswith("/"):
            continue
        expected, path = parts
        tracked = run_git(["show", f"{args.revision}:{path}"])
        actual = sha256(tracked)
        history = run_git(["log", "--all", "--format=%H", "--", path]).decode().splitlines()
        matching_revisions = []
        for revision in history:
            candidate = subprocess.run(["git", "show", f"{revision}:{path}"],
                                       capture_output=True)
            if candidate.returncode == 0 and sha256(candidate.stdout) == expected:
                matching_revisions.append(revision)
        rows.append({"path": path, "manifest_sha256": expected,
                     "git_revision_sha256": actual,
                     "matches_declared_revision": actual == expected,
                     "matching_git_revisions": matching_revisions,
                     "matches_any_reachable_git_revision": bool(matching_revisions)})
    mismatches = [row for row in rows if not row["matches_declared_revision"]]
    report = {"label": "BMW16_S16_S15_SOURCE_IDENTITY_AUDIT",
              "declared_revision": args.revision,
              "manifest": str(args.manifest),
              "tracked_source_rows": len(rows),
              "matching_declared_revision": len(rows) - len(mismatches),
              "mismatches": len(mismatches),
              "mismatch_hashes_match_any_git_revision": sum(
                  row["matches_any_reachable_git_revision"] for row in mismatches),
              "rows": rows,
              "interpretation": "A mismatch does not identify the bytes used by an old binary; absent a matching tracked revision or frozen source copy, the old binary/source linkage is unverified."}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "RECORDED", "tracked_source_rows": len(rows),
                      "mismatches": len(mismatches),
                      "matches_any_git_revision": report["mismatch_hashes_match_any_git_revision"],
                      "report_sha256": sha256(args.out.read_bytes())}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
