# M2.14 Ubuntu reproduction

Implementation label: `m2_protocol_i_raw_score_input_modular_8round_mask_output`.
Code revision used for the validation run: `a259deb8e1876ebdf3629ef3fdc5ca8a2d6c8686` on
`m2-protocol-i-raw-score-input`.
Subsequent commits `d1f7fa8` and this documentation-only update do not change
the validated implementation.

Environment: Ubuntu 24.04 WSL, GNU 13.3, Debug, fixed EMP prefix
`/tmp/moe_m28_emp.ok9WzQ/prefix`.  The raw-input primitive conformance target
and full P2/P0/P1 E2E pass.  The E2E controller distributes only uint32 raw
additive shares; it reconstructs only the test-only final XOR mask.

Additional successful process smokes:

```text
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=2  .../moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=128 MOE_TOPK_M2_E2E_K=8  .../moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=2  .../moe_topk_m2_protocol_i_modular_e2e_test
MOE_TOPK_M2_E2E_N=256 MOE_TOPK_M2_E2E_K=8  .../moe_topk_m2_protocol_i_modular_e2e_test
```

All four commands exited 0.  Score-adapter frames carry two 64-bit masked
operands per padded slot: each stage is `48 + 16*padded_n` bytes in each
direction, so each party sends and receives 4,192 bytes at `n=128` and 8,288
bytes at `n=256` over the two adapter stages.  Score adapter calls are `2n`
uCMP and `4n` raw DCF evaluations per party; CmpAgg remains
`n(n-1)/2` edges and twice that many raw DCF evaluations per party.

Fresh EMP-OFF configuration/build discovered 12 CTests and passed 12/12.
Fresh EMP-ON configuration/build discovered 18 CTests and passed 18/18,
including `moe_topk_m2_chosen_ot_conformance_test`.  The four explicit
correctness smokes above all exited 0.  The documentation-only follow-up
commits are `dc2e12e` and `627cb81`. Timing, PRG, LAN/WAN and benchmark repetitions remain
`NOT_MEASURED`.

Validation commands:

```text
cmake -S VFSS -B /tmp/moe_m214_off_final -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/moe_m214_off_final -j1
ctest --test-dir /tmp/moe_m214_off_final -N
ctest --test-dir /tmp/moe_m214_off_final --output-on-failure

cmake -S VFSS -B /tmp/moe_m214_on_final -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m214_on_final -j1
ctest --test-dir /tmp/moe_m214_on_final -N
ctest --test-dir /tmp/moe_m214_on_final --output-on-failure
```
