# M2.11 Ubuntu validation

Label: `m2_emp_two_pass_shuffle_roundtrip_conformance`; Ubuntu 24.04.4,
GCC 13.3.0, CMake 3.28.3, OpenSSL 3.0.13 and the pinned EMP prefix.

The M2.11 target was configured and built in a fresh Debug directory with the
pinned EMP prefix, then executed through its CTest entry. The controller forks
and execs two role processes for every test case; the role processes rendezvous
on an explicit barrier before deterministic TEST_ONLY shares are generated.

```bash
cmake -S /mnt/c/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation/VFSS \
  -B /tmp/moe_m211_full -DCMAKE_BUILD_TYPE=Debug \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe_m211_full \
  --target moe_topk_m2_secret_shared_shuffle_conformance_test -j1
cd /tmp/moe_m211_full
timeout 300s ./moe_topk_m2_secret_shared_shuffle_conformance_test
```

The command exited 0 on 2026-09-06. It covers all requested `(N,T)` shapes
`(2,2)`, `(4,2)`, `(4,4)`, `(8,2)`, `(8,8)`, `(16,4)`, `(16,16)`, `(64,4)`,
and `(256,16)`, each with independently derived `pi0`/`pi1` and four record
forms: general 192-bit, zero, word0-only, and an LSB carrier. The carrier
assertion reconstructs its bit by XORing the local word0 LSBs and verifies that
words 1/2 reconstruct to zero.

Fresh Debug CTest results on 2026-09-06 were 11/11 with EMP disabled (2.89 s)
and 16/16 with EMP enabled (9.69 s); the M2.11 case was test 16 and took
3.22 s. These durations are CTest wall times, not protocol benchmarks.
LAN/WAN and performance remain `NOT_MEASURED`.
