# M2.15 paper-core alignment reproduction

Status: completed alignment audit; 3-round candidate **not achieved**.

Branch: `m2-protocol-i-raw-score-input`

Implementation label: `m2_protocol_i_raw_score_input_modular_8round_mask_output`

Validated code revision for this record: `a9d9dd61746a71ce02a14ea172e297394bc4e7ad`

Environment: Ubuntu-24.04, Ubuntu 24.04.4 LTS, WSL2 x86_64, GCC 13.3.0,
CMake 3.28.3, Debug, EMP prefix `/tmp/moe_m28_emp.ok9WzQ/prefix`.

## Commands and test counts

EMP-OFF was configured in a fresh `/tmp/moe_m215_off` directory:

```text
cmake -S VFSS -B /tmp/moe_m215_off -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/moe_m215_off -j1
ctest --test-dir /tmp/moe_m215_off -N
ctest --test-dir /tmp/moe_m215_off --output-on-failure
```

EMP-ON was configured in a fresh `/tmp/moe_m215_on` directory:

```text
cmake -S VFSS -B /tmp/moe_m215_on -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m215_on -j1
ctest --test-dir /tmp/moe_m215_on -N
ctest --test-dir /tmp/moe_m215_on --output-on-failure
```

The added alignment target is `moe_topk_m2_paper_core_alignment_test`.
The expected fresh-build discovery is EMP-OFF 13 tests and EMP-ON 19 tests;
the full results below are the results of these commands at the validated
revision.

Explicit E2E commands:

```text
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=2  /tmp/moe_m215_on/moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=8  /tmp/moe_m215_on/moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=2  /tmp/moe_m215_on/moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=8  /tmp/moe_m215_on/moe_topk_m2_protocol_i_modular_e2e_test
```

## Observed results

At this revision EMP-OFF was 13/13 and EMP-ON was 19/19. The four explicit
E2E cases `(128,2)`, `(128,8)`, `(256,2)` and `(256,8)` each exited 0 and
matched the existing test-only oracle reconstruction. The new alignment test
passed in both suites.

Measured round decomposition:

| component | rounds |
| --- | ---: |
| raw score adapter | 2 |
| forward shuffle | 2 |
| masked-key exchange / CmpAgg input | 1 |
| rank reveal | 1 |
| current paper-core-shaped total | 4 |
| reverse mask adapter | 2 |
| current total | 8 |
| requested candidate core | 3 (not achieved) |
| requested candidate total | 7 (not achieved) |

For `(128,2/8)`, each party sent and received 4,192 score-adapter bytes over
the two adapter stages. For `(256,2/8)`, each party sent and received 8,288
score-adapter bytes. CmpAgg uses `n_padded*(n_padded-1)/2` edges and twice as
many raw DCF evaluations per party; the score adapter uses `2*n_padded` uCMP
calls and `4*n_padded` raw DCF calls per party. The existing E2E records retain
the phase-level shuffle, CmpAgg, rank-reveal, reverse, package and per-party
counts; the alignment test adds no protocol traffic.

The paper-compatible public masked-list bytes, a paper-compatible list message
transcript, PRG calls, phase timings, LAN/WAN RTT, bandwidth, repetitions,
formal paper-exact leakage proof, and large-scale performance are
`NOT_MEASURED` or unavailable at this milestone. No retry or failure occurred
in the four explicit E2E cases.

## Decision

Agarwal §4.1 (PDF p.8) defines the shuffle used by Protocol I as producing a
public masked shuffled list in addition to secret-shared shuffled output. Chase
§6.3 (PDF pp.21–22) defines the two-pass secret-shared shuffle output as party
local shares. The current VFSS PS API has only the latter. It cannot express the
public list without a new same-permutation functionality, correlated material,
and message contract. Therefore the independent masked-key exchange remains in
the graph and the accurate current baseline is 4 core / 8 total rounds.

The paper's 3-round statement and the project 7-round candidate remain
separately recorded targets. This record does not claim either one was measured
or achieved.
