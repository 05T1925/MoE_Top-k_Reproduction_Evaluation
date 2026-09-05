# M2.10 Ubuntu validation

Label: `m2_emp_single_pass_permute_share_conformance`. Ubuntu 24.04.4,
GCC 13.3.0, CMake 3.28.3, OpenSSL 3.0.13 and the pinned M2.8 EMP prefix.
Validated implementation revision: `cfc408d`.

```bash
cmake -S "$workspace/VFSS" -B /tmp/moe_m210_off -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/moe_m210_off -j1
ctest --test-dir /tmp/moe_m210_off --output-on-failure
cmake -S "$workspace/VFSS" -B /tmp/moe_m210_on -DCMAKE_BUILD_TYPE=Debug \
  -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m210_on -j1
ctest --test-dir /tmp/moe_m210_on --output-on-failure
```

Expected counts are OFF 11 and ON 15. Performance, LAN and WAN remain
`NOT_MEASURED`.
