# M2.13 Ubuntu reproduction

Revision: pending commit on `m2-protocol-i-modular-6round`.

Environment: Ubuntu 24.04, GNU 13.3, Debug, `BUILD_TESTING=ON`,
`MOE_TOPK_ENABLE_EMP_OT=ON`, EMP prefix `/tmp/moe_m28_emp.ok9WzQ/prefix`.

```text
cmake -S VFSS -B /tmp/moe_m213_dev -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m213_dev -j2 --target moe_topk_m2_protocol_i_modular_e2e_test
ctest --test-dir /tmp/moe_m213_dev -R '^moe_topk_m2_protocol_i_modular_e2e_test$' --output-on-failure
```

Actual result: `1/1` passed; target elapsed `9.77 s`, CTest total `10.09 s`.
The test runs logical `n={1,2,3,4,5,7,8,11,16,17,31}` and deduplicated
`K={1,n,ceil(n/2)}` cases, with random/equal/INT32_MIN/min-max/order styles
and identity/reverse/pair/cycle/random permutations. It asserts P2 exit before
input, P0/P1 offline readiness, oracle mask, padding/dummy behavior, material
consumption, rank permutation and rounds/edge/DCF/package metrics.

Not measured: benchmark repetitions, PRG calls, phase wall-clock times, LAN/WAN
RTT and bandwidth. No performance extrapolation is made.
