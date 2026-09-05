# M2.9 Ubuntu OPV / Share Translation validation

Implementation label: `m2_emp_opv_share_translation_conformance`.

Validated on Ubuntu 24.04.4 LTS x86_64, GCC 13.3.0, CMake 3.28.3 and OpenSSL
3.0.13.  EMP dependencies are the pinned prefix recorded in
`M2_CHOSEN_OT_DEPENDENCY.md`; no dependency selection was repeated for M2.9.

Validated source revision: `15b1153` (implementation plus final negative-test
coverage; this documentation commit follows it). Commands were:

```bash
cmake -S "$workspace/VFSS" -B /tmp/moe_m29_off -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/moe_m29_off -j1
ctest --test-dir /tmp/moe_m29_off --output-on-failure

cmake -S "$workspace/VFSS" -B /tmp/moe_m29_on -DCMAKE_BUILD_TYPE=Debug \
  -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m29_on -j1
ctest --test-dir /tmp/moe_m29_on --output-on-failure
```

The OFF configuration discovers/passes 11 tests.  The enabled configuration
discovers/passes 14: the original eleven plus chosen-OT, OPV and Share
Translation conformance.  The M2.9 role tests record crossed FVO/PO byte
counters and verify no opened values; performance, LAN and WAN measurements
remain `NOT_MEASURED`.
