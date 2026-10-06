#!/usr/bin/env python3
"""TEST_ONLY UID boundary replay of the S9 n=2 two-world filesystem witness.

This checks OS file access only. It is not a party-view simulator, channel
authentication test, or cryptographic security proof.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import pwd
import subprocess
import tempfile
from pathlib import Path


def run_user(user: str, *command: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(["runuser", "-u", user, "--", *command], check=False,
                          text=True, encoding="utf-8", errors="replace", capture_output=True)


def can_read(user: str, path: Path) -> bool:
    return run_user(user, "test", "-r", str(path)).returncode == 0


def can_list(user: str, path: Path) -> bool:
    return run_user(user, "ls", "-A", str(path)).returncode == 0


def install_owned(source: Path, target: Path, account: pwd.struct_passwd) -> None:
    result = subprocess.run(["install", "-o", account.pw_name, "-g", str(account.pw_gid),
                             "-m", "0600", str(source), str(target)], check=False,
                            text=True, encoding="utf-8", errors="replace", capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"install failed ({result.returncode}): {result.stderr[-300:]}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--t-user", default="moeaudit")
    parser.add_argument("--p0-user", default="nobody")
    parser.add_argument("--p1-user", default="www-data")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if os.geteuid() != 0:
        parser.error("run this TEST_ONLY audit as WSL root so it can create separate OS-owned files")
    t, p0, p1 = (pwd.getpwnam(name) for name in (args.t_user, args.p0_user, args.p1_user))
    if len({t.pw_uid, p0.pw_uid, p1.pw_uid}) != 3:
        parser.error("T, P0 and P1 must be distinct Linux UIDs")

    # Same P0 input and ideal output share in both worlds, as in S9. Peer
    # shares/material vary with the two opposite clear orders.
    p0_input = b'{"n":2,"k":1,"party":0,"shares":[109,208]}'
    ideal_p0_output = b'{"share":[1,0]}'
    worlds = {"A": ([0, 1], b'{"shares":[4294967187,4294967089]}'),
              "B": ([1, 0], b'{"shares":[4294967188,4294967088]}')}
    rows: dict[str, object] = {}
    with tempfile.TemporaryDirectory(prefix="bmw16_s10_uid_worlds_", dir="/tmp") as name:
        root = Path(name)
        root.chmod(0o711)
        for world, (order, p1_input) in worlds.items():
            world_dir = root / world
            world_dir.mkdir(mode=0o711)
            t_dir, p0_dir, p1_dir = (world_dir / part for part in ("T", "P0", "P1"))
            for path, owner in ((t_dir, t), (p0_dir, p0), (p1_dir, p1)):
                path.mkdir(mode=0o700)
                os.chown(path, owner.pw_uid, owner.pw_gid)

            staged = world_dir / "controller-stage"
            staged.mkdir(mode=0o700)
            (staged / "p0-input").write_bytes(p0_input)
            (staged / "ideal-output").write_bytes(ideal_p0_output)
            (staged / "p1-input").write_bytes(p1_input)
            (t_dir / "p0-package").write_bytes(f"T-material-P0-{world}".encode())
            (t_dir / "p1-package").write_bytes(f"T-material-P1-{world}".encode())
            install_owned(staged / "p0-input", p0_dir / "input.share", p0)
            install_owned(staged / "ideal-output", p0_dir / "ideal-output.share", p0)
            install_owned(staged / "p1-input", p1_dir / "input.share", p1)
            # The T-owned offline package is delivered by this TEST_ONLY root
            # harness into P1's private account directory, never to P0.
            result = subprocess.run(["install", "-o", p1.pw_name, "-g", str(p1.pw_gid),
                                     "-m", "0600", str(t_dir / "p1-package"),
                                     str(p1_dir / "offline.package")], check=False,
                                    text=True, encoding="utf-8", errors="replace", capture_output=True)
            if result.returncode != 0:
                raise RuntimeError(f"P1 package delivery failed: {result.stderr[-300:]}")

            rows[world] = {
                "clear_order_for_fixture_only": order,
                "p0_input_sha256": hashlib.sha256((p0_dir / "input.share").read_bytes()).hexdigest(),
                "ideal_p0_output_sha256": hashlib.sha256((p0_dir / "ideal-output.share").read_bytes()).hexdigest(),
                "p1_input_sha256": hashlib.sha256((p1_dir / "input.share").read_bytes()).hexdigest(),
                "p0_reads_p1_input": can_read(args.p0_user, p1_dir / "input.share"),
                "p0_reads_p1_material": can_read(args.p0_user, p1_dir / "offline.package"),
                "p0_lists_p1_directory": can_list(args.p0_user, p1_dir),
                "p0_reads_t_staging": can_read(args.p0_user, t_dir / "p1-package"),
                "t_reads_p0_input": can_read(args.t_user, p0_dir / "input.share"),
            }
        a, b = rows["A"], rows["B"]
        assert isinstance(a, dict) and isinstance(b, dict)
        result = {
            "label": "BMW16_S10_UID_ISOLATED_TWO_WORLD_OS_ACCESS_TEST_ONLY",
            "worlds": rows,
            "p0_input_identical": a["p0_input_sha256"] == b["p0_input_sha256"],
            "ideal_p0_output_share_identical": a["ideal_p0_output_sha256"] == b["ideal_p0_output_sha256"],
            "p1_inputs_differ": a["p1_input_sha256"] != b["p1_input_sha256"],
            "all_p0_peer_and_t_file_access_denied": all(
                not row[key] for row in (a, b) for key in (
                    "p0_reads_p1_input", "p0_reads_p1_material", "p0_lists_p1_directory",
                    "p0_reads_t_staging", "t_reads_p0_input")),
            "delivery_contract": "root-owned TEST_ONLY local install; not authenticated/private production delivery",
            "security_scope": "filesystem ACL boundary only; does not simulate shuffle, key-pool, transcript, or mask-share views",
        }
    payload = json.dumps(result, sort_keys=True, separators=(",", ":")) + "\n"
    if args.output:
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    return 0 if result["p0_input_identical"] and result["ideal_p0_output_share_identical"] and result["p1_inputs_differ"] and result["all_p0_peer_and_t_file_access_denied"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
