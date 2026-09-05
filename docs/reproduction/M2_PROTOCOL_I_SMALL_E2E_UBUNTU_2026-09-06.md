# M2.12 Ubuntu validation

Label: `m2_protocol_i_priority_key_input_small_e2e`. Use Ubuntu 24.04, Debug,
`-j1`, and the pinned EMP prefix:

```bash
cmake -S VFSS -B /tmp/moe_m212_off -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/moe_m212_off -j1
ctest --test-dir /tmp/moe_m212_off -N
ctest --test-dir /tmp/moe_m212_off --output-on-failure

cmake -S VFSS -B /tmp/moe_m212_on -DCMAKE_BUILD_TYPE=Debug \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m212_on -j1
ctest --test-dir /tmp/moe_m212_on -N
ctest --test-dir /tmp/moe_m212_on --output-on-failure
```

On 2026-09-06, fresh Debug validation passed 11/11 with EMP OFF (2.45 s) and
17/17 with EMP ON (9.08 s). The new E2E test was test 17 and took 0.30 s.
These are CTest wall times, not protocol benchmarks. LAN/WAN, bandwidth, PRG
call count and protocol performance are `NOT_MEASURED`.
