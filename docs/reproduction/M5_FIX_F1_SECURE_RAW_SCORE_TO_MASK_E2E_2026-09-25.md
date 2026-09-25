# M5-FIX-F1 secure Q20.12 shares → original-order Top-K mask

Date: 2026-09-25. Base HEAD `3904afb461597d6a142f3d534c6686fb377bb4ec`, branch `feat/m5-protocol-iii-two-round`, implementation label `m5_fix_f1_raw_score_mask_4round`. **F1 additions are uncommitted in this worktree; this is not a frozen G3 receiver revision.** Environment: Ubuntu 22.04 under WSL2 Linux 5.15.167.4, x86_64, GCC 11.4.0, CMake 3.22.1, Debug CMake build, `MOE_TOPK_ENABLE_EMP_OT=OFF`. Test fixture inputs use fixed case seeds; the offline dealer obtains 128 private OS `getrandom` seed bits and expands them with the bundled AES PRNG independently. All communication/offline counts below are raw process counters from one run per configuration, not latency benchmarks.

## Requirement and implemented boundary

The authoritative blocker is [H2 F1](../reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md): the verified field core did not provide the project's secure Q20.12-share input to original-order shared Top-K mask output or account for its adapters. The accepted [F1 decision](../decisions/M5_FIX_F1_SECURE_IO_ADAPTER_DECISION_2026-09-25.md) uses a bit-mask specialization requiring no field record. This is a **C-INSTANTIATION**: the M5-E/G general field Fselect/Fsort remains unchanged, and its arbitrary-payload ring→field conversion remains unimplemented. The new full standard path never claims the paper's general field-valued Fselect or author-exact raw-score protocol.

Production entrypoint: `protocol_iii_raw_score_mask_party` in `VFSS/include/moe_topk/protocol_iii_raw_score_mask.h`. Each party supplies only its own length-`n` additive `uint32_t` shares of signed two's-complement Q20.12 words, the party-bound material, and four independent stage sockets. It returns length-`n` original-input-order `uint8_t` XOR shares. `n>=2` and `0<K<=n` (n=1 is rejected); reconstructed mask has exactly K ones by stable score DESC/index ASC. Signed raw score range is `[-2^31,2^31-1]` scaled by `2^-12`; padding is filled inside the existing score adapter and omitted from GRank/routing outputs.

The companion `protocol_iii_raw_score_mask_serialize_bundle` / `deserialize_bundle` provides version 1, party/session/fingerprint/material/shape/width bound offline material. Its score portion uses carry/sign uCMP/DCF material; GRank uses the frozen M2 package codec; routing uses rank-mask shares plus native DPF party keys. Embedded native FSS keys are same-build/same-architecture only. `started` is set before the first online attempt, including a failed attempt. A new process can deserialize a copied bundle again if the controller reuses its material ID: persistent replay prevention remains the controller's responsibility and is **not** claimed here.

## Causal stages and algebra

| Stage | Online messages (both parties) | Party-local result | Security status |
| --- | --- | --- | --- |
| A1 carry | one framed outbound per party, each `2p` masked 34-bit values | score carry shares | P2 silent; no raw score opening |
| A2 sign | one framed outbound per party, each `2p` masked 34-bit values | padded priority-key additive shares | same frozen M3 score adapter |
| B GRank | one framed outbound per party, each `n` masked comparison keys | `n` original-order rank shares in `Z_(2^r)` | shared `protocol_i_cmpagg_eval_party`, raw rank unopened |
| C routing | one framed outbound per party, each `n` masked rank shares | row-major `n×K` additive DPF indicator shares in `Z_(2^64)` | masked rank public; raw rank private |
| D mask | **no online message** | per-item XOR bit share from local row sum parity | `Z_(2^64)→Z_2` additive homomorphism |

Here `p=padded_n`, `r=ceil(log2 n)` and `ell'=33+index_bits`. For each item `i`, `m_i=(rank_i+r_rank[i]) mod 2^r` is opened; DPF key `f_(r_rank[i],1)` is evaluated at `(m_i-t) mod 2^r` for `t=0..K-1`. Reconstructed indicator is 1 exactly when `rank_i=t`. Each party sums its row in the **64-bit ring** and takes LSB. XOR of local LSBs equals `rank_i<K`, with no combine opening. Do not apply this low-bit rule to `F_(2^127-1)` field shares. The standard complete path has **four** causal rounds (2 input + 1 ranking + 1 routing); the separately frozen general field Fselect/Fsort core still has **two**. There is no post-routing protocol frame in the standard path.

## Cost definition and measured counts

Logical online bits across P0+P1: A carry `4p·34`, A sign `4p·34`, B `2n·ell'`, C `2n·r`, D `0`; total `272p+2n(ell'+r)`. Wire bytes across P0+P1 from the existing 48-byte framed transport and 8-byte storage words: A carry `96+32p`, A sign `96+32p`, B `96+16n`, C `96+16n`; total `384+64p+32n`. Logical bits are not wire bytes. Offline bytes are measured serialized score package, GRank package, rank-mask/DPF material and 132 bytes total application+length metadata across two bundles. Local work per party: `2p` score uCMP (`4p` underlying DCF Eval), `C(n,2)` GRank uCMP (`2C(n,2)` DCF Eval), `nK` DPF Eval, `n(K-1)` local ring additions and `n` parity extractions. No field operations occur in this bit-only specialization.

`./build-vfss-debug/moe_topk_m5_fix_f1_process_e2e_test --cost-smoke` printed the following raw counters. The input case seeds are respectively 104, 101, 102, 103, 105 and 106 for the displayed `n=2,3,5,8,16,20` rows. One process run per row, no warmup, no timing claim. `DCF` and `DPF` are across both online parties. Stage wire columns are carry/sign/GRank/routing. Stage offline columns are score/GRank/routing/metadata.

| n | K | p | Logical bits | Wire bytes | Offline bytes | DCF Eval | DPF Eval | Stage wire bytes | Stage offline bytes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2 | 1 | 2 | 684 | 576 | 9470 | 20 | 4 | 160,160,128,128 | 7184,1922,232,132 |
| 3 | 1 | 4 | 1310 | 736 | 20614 | 44 | 6 | 224,224,144,144 | 14368,5670,444,132 |
| 5 | 2 | 8 | 2566 | 1056 | 48844 | 104 | 20 | 352,352,176,176 | 28736,19076,900,132 |
| 8 | 8 | 8 | 2800 | 1152 | 83452 | 176 | 128 | 352,352,224,224 | 28736,53144,1440,132 |
| 16 | 8 | 16 | 5664 | 1920 | 293908 | 608 | 256 | 608,608,352,352 | 57472,232912,3392,132 |
| 20 | 10 | 32 | 10424 | 3072 | 497712 | 1016 | 400 | 1120,1120,416,416 | 114944,377756,4880,132 |

The process harness verifies each row's stage sums equal total wire/offline bytes and P0 sent bytes equal P1 received bytes. Offline bytes include bundle length prefixes, but controller input/result test traffic is excluded from protocol wire metrics. For `n=3,5,20`, honest ranks stay in `[0,n)` while the DPF input ring has 4, 8 and 32 points respectively; this is the project's documented `Z_(2^r)` C-INSTANTIATION, not paper-exact `Z_n`. The stage table is a full standard-function engineering cost baseline, **not** a Theorem 4.2 cost match for general field payloads.

## Correctness, process and failure evidence

- Differential target `moe_topk_m5_fix_f1_raw_score_mask_test`: **140/140 PASS**, n=2/3/4/5/8, K=1/n/middle when applicable; descending, ascending, all equal, mixed duplicates, min/max signed words, zero, negative/positive, same-integer distinct Q20.12 fractions and fixed-seed random. Each case uses real score-share adapter, DCF/uCMP CmpAgg, native DPF routing and local output adapter. Selected count is exactly K. For representative cases, priority-key reconstruction matches the frozen M3 key encoding and final reconstruction matches the frozen M3 five-round mask bit for bit. Clear reconstruction occurs only in test code.
- Independent process target `moe_topk_m5_fix_f1_process_e2e_test`: n=3/K=1 all equal, n=5/K=2 signed boundaries/duplicates, n=8/K=8 boundary; **3/3 PASS**. `--cost-smoke` adds n=2/16/20 for six measured cases, **6/6 PASS**. P2 generates/distributes material only, exits, and is waited for **before** the controller releases each P0/P1 private score-share vector. The party processes are separate `fork+exec` address spaces. Party CLI contains only public metadata; P2 material RNG and controller input-share RNG seeds are not supplied to P0/P1. No test-memory oracle, clear rank, clear indicator, clear mask or file polling enters the secure party runtime.
- Version/party/session/fingerprint/material ID/shape/width/length mutation, wrong-party bundle, truncated and extra bytes: rejected. Same material object reuse after success rejected; partial score-exchange failure sets `started`, and retry is rejected. `--failure-smoke` closes P1's carry socket after input release: both parties exit nonzero and neither produces a valid mask. Existing framed transport tests cover malformed phase/session frames. A persistent cross-process replay database and malicious-dealer proof are out of scope.
- Full Debug CTest after the new targets: **41/41 PASS**, including all previous 39 M3/M5/Protocol I/H1 targets. No Protocol I runtime or `VFSS/ext/FSS/` source was changed. Temporary ASan/UBSan build under `/tmp/m5f1-vfss-asan` ran the new differential, process and failure-smoke targets with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`; all exited 0. `detect_leaks=0` follows the established H2 sanitizer environment; address/UB checks were active.

## Reproduction and receiving-party commands

Prerequisites: Ubuntu 22.04 x86_64 with GCC 11, CMake/Ninja, Eigen3 and this repo's bundled VFSS dependencies. The current F1 source is **uncommitted** on `feat/m5-protocol-iii-two-round` at base `3904afb`; a fresh clone/fetch of this SHA alone will **not** contain F1. The delivering worktree has generated `/tmp/m5_fix_f1_review_3904afb.patch`, containing tracked edits plus all eight new F1 files. Its exact SHA256 is in the companion `/tmp/m5_fix_f1_review_3904afb.sha256` checksum file and is also conveyed with the stage report; the patch contains this document, so the hash cannot be embedded in it without a self-reference. It was independently `git apply --check` tested and applied to a clean local clone of `3904afb`. This provisional same-host patch permits a receiver to execute tests now without violating the no-commit rule. Formal G3 handoff should later pin a user-authorized checkpoint commit; do not claim acceptance of base SHA alone.

Provisional clean-clone source preparation on a host that has the patch:

```bash
git clone <repository-url> MoE_Top-k_Reproduction_Evaluation
cd MoE_Top-k_Reproduction_Evaluation
git fetch origin feat/m5-protocol-iii-two-round
git switch --detach 3904afb461597d6a142f3d534c6686fb377bb4ec
sha256sum -c /tmp/m5_fix_f1_review_3904afb.sha256
git apply --check /tmp/m5_fix_f1_review_3904afb.patch
git apply /tmp/m5_fix_f1_review_3904afb.patch
git diff --check
```

Once an F1 checkpoint is authorized and available, replace the patch application with `git switch --detach <F1_CHECKPOINT_SHA>` and record the exact SHA. Build and verify either source state with:

```bash
cmake -S VFSS -B build-vfss-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build build-vfss-debug -j4
ctest --test-dir build-vfss-debug --output-on-failure
./build-vfss-debug/moe_topk_m5_fix_f1_raw_score_mask_test
./build-vfss-debug/moe_topk_m5_fix_f1_process_e2e_test
./build-vfss-debug/moe_topk_m5_fix_f1_process_e2e_test --cost-smoke
./build-vfss-debug/moe_topk_m5_fix_f1_process_e2e_test --failure-smoke
```

On the current shared worktree, run the same commands starting from the `cmake` line. Expected outputs include `140` differential cases, `F1_PROCESS_PASS` for the process/cost rows, `F1_FAILURE_SMOKE_PASS early_close no_valid_mask=1`, four online rounds for the full path, and all CTest targets passing. The failure smoke intentionally logs two party errors. The receiver must record checkout SHA or exact patch hash, build flags, exit codes, raw counters, any optional-dependency exclusions, and independent verdict. This delivering agent's self-runs are **not F2/G3 acceptance**.

Optional sanitizer rerun:

```bash
cmake -S VFSS -B /tmp/m5f1-vfss-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m5f1-vfss-asan --target moe_topk_m5_fix_f1_raw_score_mask_test moe_topk_m5_fix_f1_process_e2e_test -j4
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/m5f1-vfss-asan/moe_topk_m5_fix_f1_raw_score_mask_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/m5f1-vfss-asan/moe_topk_m5_fix_f1_process_e2e_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/m5f1-vfss-asan/moe_topk_m5_fix_f1_process_e2e_test --failure-smoke
```

**Status boundary:** engineering F1 standard input/output path and full cost are implemented and self-verified. The field-valued Fselect/Fsort core remains separate and two rounds. `AUTHOR_EXACT=NOT_PROVEN`, `F2/G3=NOT_ACCEPTED`, `M5=IN_PROGRESS`; no M6A runtime or performance work was performed. A receiver-checkable frozen F1 revision must be created only on later user authorization.
