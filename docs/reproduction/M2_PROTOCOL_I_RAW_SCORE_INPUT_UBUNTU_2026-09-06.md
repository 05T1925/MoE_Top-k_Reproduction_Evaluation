# M2.14 Ubuntu reproduction

Implementation label: `m2_protocol_i_raw_score_input_modular_8round_mask_output`.
Code revision: `1a6af9232971a741b4ec0ab0e5bc0b9b72204b5d` on
`m2-protocol-i-raw-score-input`.

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

Fresh EMP-OFF configuration discovered 12 CTests.  Fresh EMP-ON configuration
discovered 18; after its complete build, full CTest ran 17/18 successfully.
The sole failure was the pre-existing `moe_topk_m2_chosen_ot_conformance_test`
with `test read`; the new score-input conformance and raw-input modular E2E
both passed.  EMP-OFF full build/run is `NOT_MEASURED`.  Timing, PRG, LAN/WAN
and repetitions remain `NOT_MEASURED`.
