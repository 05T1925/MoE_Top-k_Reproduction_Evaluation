#!/usr/bin/env python3
"""Two-world same-OS-token input-share disclosure witness; synthetic TEST_ONLY."""
from __future__ import annotations

import hashlib
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path


def split_scores(scores: list[int], seed: int) -> tuple[list[int], list[int]]:
    rng = random.Random(seed)
    left = [rng.getrandbits(32) for _ in scores]
    right = [((score & 0xFFFFFFFF) - share) & 0xFFFFFFFF
             for score, share in zip(scores, left)]
    return left, right


def p0_recover(p0_path: Path) -> list[int]:
    code = (
        "import json,sys; from pathlib import Path; "
        "a=json.loads(Path(sys.argv[1]).read_text()); "
        "b=json.loads(Path(Path(sys.argv[1]).with_name('p1.input.json')).read_text()); "
        "m=(1<<32)-1; "
        "v=[(int(x)+int(y))&m for x,y in zip(a['shares'],b['shares'])]; "
        "print(json.dumps([x-(1<<32) if x&(1<<31) else x for x in v]))"
    )
    result = subprocess.run([sys.executable, "-c", code, str(p0_path)], check=True,
                            capture_output=True, text=True)
    return [int(value) for value in json.loads(result.stdout)]


def main() -> int:
    seed = 109
    worlds = {"A": [0, 1], "B": [1, 0]}
    rows = {}
    with tempfile.TemporaryDirectory(prefix="bmw16_s9_two_worlds_") as directory:
        root = Path(directory)
        left_a, right_a = split_scores(worlds["A"], seed)
        left_b, right_b = split_scores(worlds["B"], seed)
        for name, scores, left, right in (
            ("A", worlds["A"], left_a, right_a),
            ("B", worlds["B"], left_b, right_b),
        ):
            world_dir = root / name
            world_dir.mkdir()
            common = {"n": 2, "k": 1, "session": 9, "party": 0, "shares": left}
            peer = {"n": 2, "k": 1, "session": 9, "party": 1, "shares": right}
            p0_path = world_dir / "p0.input.json"
            p0_path.write_text(json.dumps(common, separators=(",", ":")), encoding="utf-8")
            peer_path = world_dir / "p1.input.json"
            peer_path.write_text(json.dumps(peer, separators=(",", ":")), encoding="utf-8")
            recovered = p0_recover(p0_path)
            rows[name] = {
                "fixture_scores": scores,
                "p0_input_shares_sha256": hashlib.sha256(p0_path.read_bytes()).hexdigest(),
                "peer_input_file_sha256": hashlib.sha256(peer_path.read_bytes()).hexdigest(),
                "same_os_token_recovered_scores": recovered,
                "p0_distinguishing_event_score0_lt_score1": recovered[0] < recovered[1],
            }
        # A one-party XOR output share has the same uniform marginal in both
        # worlds; couple it to the same value for the distinguisher comparison.
        assert left_a == left_b
        assert rows["A"]["same_os_token_recovered_scores"] == worlds["A"]
        assert rows["B"]["same_os_token_recovered_scores"] == worlds["B"]
        assert rows["A"]["p0_distinguishing_event_score0_lt_score1"] != rows["B"]["p0_distinguishing_event_score0_lt_score1"]
        result = {
            "label": "BMW16_S9_SAME_OS_TOKEN_TWO_WORLD_WITNESS_TEST_ONLY",
            "corrupted_party": "P0",
            "threat_scope": "two online party processes run with the same OS token and sibling input files are readable",
            "n": 2,
            "K": 1,
            "input_seed": seed,
            "p0_local_input_identical": left_a == left_b,
            "coupled_ideal_p0_output_share": [1, 0],
            "worlds": rows,
            "distinguishing_event": "read P1 share file, reconstruct both raw scores, test score[0] < score[1]",
            "ideal_p0_view_event_probability": "same event is not available from the ideal output share; equal P0 input/output shares are coupled",
            "verdict": "DISTINGUISHABLE_WITH_PROBABILITY_1_IN_THIS_FILE_ACCESS_MODEL",
            "scope_limit": "This is a same-host OS-access witness. It does not establish a remote-host attack when OS identities and private channels are correctly isolated.",
        }
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
