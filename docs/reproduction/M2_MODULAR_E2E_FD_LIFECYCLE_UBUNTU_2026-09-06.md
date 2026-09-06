# M2 modular E2E file-descriptor lifecycle repair

Status: verified test-harness repair.  It does not change the Protocol I
implementation, output contract, round count, leakage contract, or metrics
boundary.

Implementation label remains
`m2_protocol_i_raw_score_input_modular_8round_mask_output`.

The causal decomposition remains: two raw-score-adapter rounds, two forward
shuffle rounds, one masked-key exchange, one rank reveal, and two reverse-mask
adapter rounds, for eight total rounds.  This is not progress toward
`agarwal_protocol_i_exact`, `agarwal_protocol_i_exact_mask_output`,
`paper_3_round_exact`, or `secure_shuffle_complete`.

## Environment

- Ubuntu 24.04.4 LTS on WSL2, Linux 6.6.87.2-microsoft-standard-WSL2 x86_64.
- GCC/G++ 13.3.0, CMake 3.28.3, OpenSSL 3.0.13, Debug build.
- EMP enabled build used pinned `emp-tool v1.0.0-alpha.1` and `emp-ot
  v1.0.0-alpha.1` local build trees at
  `/tmp/moe_m28_emp.ok9WzQ/emp-tool-build` and
  `/tmp/moe_m28_emp.ok9WzQ/emp-ot-build`.
- Observed shell limits before tightening: soft `10240`, hard `1048576`.
  The commands below explicitly set soft `RLIMIT_NOFILE` to `1024` or `4096`.

## Defect and repair

`run_case()` creates 20 `socketpair`s (40 descriptors): four offline pairs,
two each for forward, reverse, and score stages, plus ten controller/dealer
pairs.  The 30-case default matrix therefore creates 1,200 endpoints over one
controller process.  Before this repair, endpoints inherited by unrelated
children and controller endpoints left after a case were not uniformly closed;
under soft limit 1024 the EMP-ON full CTest run reached the modular E2E test
after 18 passing tests and then failed its `socketpair` check.  This is a test
harness resource-lifecycle error, not a protocol correctness failure.

The harness now has two scoped test-only helpers:

- `FdPool` records both ends of every case socketpair and closes all endpoints
  on normal and exceptional exit.
- `ChildSet` records P0, P1, and P2, reaps successful children, and terminates
  plus reaps unrecovered children during exceptional exit.

Every exec child retains only its role's explicit endpoint list; all unrelated
endpoints are closed before `execv`.  The controller retains its aliases for
the lifetime of the case, then calls `close_all()` before waiting for P0/P1.
This deliberately preserves the existing production framed-transport contract:
that transport rejects a terminal `POLLHUP`, including when final data is
readable.  Closing controller aliases earlier races the test with that existing
contract.  No production transport or Protocol I source was changed.

## Commands and results

EMP-ON build and discovery:

```text
cmake --build /tmp/moe_m216_e2e_fix -j2
ctest --test-dir /tmp/moe_m216_e2e_fix -N
ulimit -n 1024
ctest --test-dir /tmp/moe_m216_e2e_fix --output-on-failure
```

Discovery found 19 tests.  The full EMP-ON suite passed 19/19 at soft 1024,
including chosen-OT, OPV, Share Translation, and modular Protocol I E2E.

EMP-OFF was configured independently and run at the same tightened limit:

```text
cmake -S VFSS -B /tmp/moe_m216_e2e_off_fix -DCMAKE_BUILD_TYPE=Debug \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF
ulimit -n 1024
cmake --build /tmp/moe_m216_e2e_off_fix -j2
ctest --test-dir /tmp/moe_m216_e2e_off_fix --output-on-failure
```

EMP-OFF passed 13/13.  The following direct EMP-ON E2E commands each exited
zero at soft 1024:

```text
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=2 moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=8 moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=2 moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=8 moe_topk_m2_protocol_i_modular_e2e_test
```

The full 30-case modular E2E matrix also passed twice consecutively at soft
1024 and once at soft/hard 4096.  Thus this repaired harness has no observed
dependency on a pre-raised 4096 limit.  `NOT_MEASURED` performance and network
fields in existing M2 records remain unchanged.

## Interpretation

The pre-fix failure is classified as a test-harness FD-lifecycle error.  The
post-fix matrices are evidence that the existing M2 C-level modular baseline
can be reproduced under an explicit soft limit of 1024 in this recorded Ubuntu
environment.  They do not establish paper-exact Protocol I functionality or a
formal paper-level leakage proof.
