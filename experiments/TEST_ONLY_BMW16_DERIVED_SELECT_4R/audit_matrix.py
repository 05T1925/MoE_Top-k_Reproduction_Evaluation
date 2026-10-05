"""Emit the requested BMW16 finite-parameter matrix as JSON Lines.

The records are static source/parameter audits.  They are explicitly not
randomized algorithm runs, comparison measurements, or oracle differential
results.
"""

from __future__ import annotations

import json

from audit_reference import build_audit, parse_epsilon


def main() -> int:
    epsilon = parse_epsilon("1/36")
    cases = [(128, 2), (128, 8), (256, 2), (256, 8), (1000, 80)]
    for n, k in cases:
        print(json.dumps(build_audit(n, k, epsilon), sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
