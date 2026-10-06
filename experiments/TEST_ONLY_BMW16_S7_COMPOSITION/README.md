# TEST_ONLY BMW16 S7 composition experiment

This directory contains a centralized plaintext test harness. Its fixed label is
Protocol I + BMW16-derived Select + DCF, PROJECT_DERIVED / TEST_ONLY_CENTRAL_OPEN.

It is not secure Protocol I, BB90, an implementation of BMW16 Theorem 8, or an
independent T/P0/P1 E2E. Do not connect it to VFSS production entry points.
The harness opens the shuffled identity mapping and uCMP result shares; comparison
keys are generated on demand inside the test service. S4's sample-bracket
Select source was copied from the S4 commit listed in the S7 decision report.

Build from a Windows checkout using WSL Ubuntu 24.04:

    wsl -d Ubuntu-24.04 -- bash -lc "cmake -S '/mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s7/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_S7_COMPOSITION' -B /tmp/m6b_i_bmw16_s7_build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/m6b_i_bmw16_s7_build --target bmw16_s7_fss_testonly -j2"

Run the retained differential matrix, which compares only in the separate test
controller against the frozen C++ Top-K oracle:

    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/run_validation.py --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --output docs/reproduction/evidence/BMW16_S7_TEST_ONLY_MATRIX.json

Run a traceable single composition and its independent edge-list audit:

    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/compose_testonly.py --scores-json experiments/TEST_ONLY_BMW16_S7_COMPOSITION/vectors/mixed_extremes_n8_k3.json --k 3 --input-seed 11 --algorithm-seed 42 --shuffle-seed 77 --fss-seed 99 --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --trace --output docs/reproduction/evidence/BMW16_S7_E2E_N8_K3_SEED42.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/audit_edges.py docs/reproduction/evidence/BMW16_S7_E2E_N8_K3_SEED42.json --output docs/reproduction/evidence/BMW16_S7_EDGE_AUDIT.json

Audit the actual uCMP key range and the leakage/capacity alternatives:

    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/key_ucmp_conformance.py --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --output docs/reproduction/evidence/BMW16_S7_UCMP_KEY_CONFORMANCE.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/audit_s7.py --output docs/reproduction/evidence/BMW16_S7_VIEW_AND_NAMESPACE_AUDIT.json

See docs/reproduction/BMW16_S7_COMPOSITION_AND_SECURITY_AUDIT_2026-10-06.md
for current results, proof limits, metrics and blockers.
