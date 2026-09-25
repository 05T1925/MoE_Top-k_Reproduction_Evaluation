# M5 Protocol III Two-Round Communication Verification

Date: 2026-09-25. **H1 accepted under the user-specified order-of-magnitude criterion; exact Theorem 4.2 logical-cost match is NO.** The user relaxed H1 acceptance after the paper formula was frozen and the initial measurements exposed the mismatch. No paper formula or counting boundary was changed to obtain this verdict. The pre-benchmark paper formula was frozen separately in [M5-H1 paper cost spec](M5_PROTOCOL_III_H1_PAPER_COST_SPEC_2026-09-25.md) before any M5-H1 multi-scale run.

## 1. Scope, repository and environment

Branch `feat/m5-protocol-iii-two-round`, HEAD `e70132c` plus uncommitted H1 test-only benchmark CLI and this note. Release build `build-vfss` (`-O3 -DNDEBUG`), GCC 11.4.0, CMake 3.22.1; Ubuntu 22.04 WSL2 kernel `5.15.167.4-microsoft-standard-WSL2`, x86_64, 13th Gen Intel i7-13620H (16 logical CPUs), 7.6 GiB RAM. P2, P0 and P1 use independent fork+exec address spaces and Unix `socketpair` framed transport on one host; network-stack/LAN/WAN bytes and latency are outside this scope.

## 2. Evidence boundary

**PAPER_DIRECT:** Theorem 4.2 gives two rounds and, for 2+1-party CmpAgg Fselect, `6n ell_prime + 2n ceil(log2 n) + 4np` bits total across online parties, with `ell_prime+p=ceil(log2|H|)`. It lists `2*C(n,2)` DCF.Eval and `2n` DPF.Eval across both parties. Fsort replaces DPF.Eval by DPF.FullEval with no further online/offline communication. Footnote 8's distinct common-mask optimization subtracts `2n ell_prime` bits. **PAPER_DERIVED:** With project `H=F_(2^127-1)`, the capacity-matched parameter substitution is `p=127-ell_prime`. **C-INSTANTIATION:** packed field record, project rank ring, concrete R1/R2 messages, serializations and controller process topology. **D_UNRESOLVED:** author-exact transcript, field-DPF formal proof, input consistency proof, secure ring-to-field conversion and original-order mask output adapter.

The actual application payload is `uint64` (64 bits). Here `p=127-ell_prime` is the theorem-compatible **field-capacity budget** after packing key and payload; unused field capacity is not newly transmitted application data. A direct `p=64` substitution would violate Theorem 4.2's stated `ell_prime+p=127` precondition for these comparison widths. This limits the comparison to a project-field instantiation rather than an exact same-parameter author transcript.

## 3. Paper Theorem 4.2 cost specification

The unoptimized Theorem 4.2 expression across **both** online parties is `6n*ell_prime + 2n*r + 4n*p` logical bits, with `r=ceil(log2 n)` and `ell_prime+p=ceil(log2|H|)`. The separately stated common-mask optimization gives `4n*ell_prime + 2n*r + 4n*p`; it is **NOT_IMPLEMENTED** here. The comparison baseline is therefore the unoptimized expression. The [pre-benchmark paper cost specification](M5_PROTOCOL_III_H1_PAPER_COST_SPEC_2026-09-25.md) preserves this formula independently of measured results.

## 4. Project instantiation and measurement boundary

The measured object is the already-field-shared priority-key / encoded-record two-round core. The program uses real fresh offline dealer material, shared clique CmpAgg, field Beaver masking, field DPF, Fselect or FullEval Fsort, and checks the selected/sorted output against a controller-only oracle. P2 exits before online input release. Each row uses fresh session, fingerprint and material identity. Score input is unique descending for n other than 3/5; n=3/5 use all-equal ties. Payloads include 0, 1, `UINT64_MAX`, 7 and fixed-seed random values. The benchmark seed starts at `0x5f48100000000000+n`; every target/round repetition receives fresh preprocessing.

The paper's routing domain is `Z_n`. This implementation uses `Z_(2^r)` with `r=ceil(log2 n)`; non-power-of-two n has padded local DPF leaves. This is a **C-INSTANTIATION**, not paper-exact domain realization. The field `H=F_(2^127-1)` provides 127 bits of encoding capacity; the application payload is a full `uint64`, encoded through the project's field payload layer.

## 5. Benchmark methodology and reproduction commands

Reproduce and validate all measured rows with:

```bash
cmake --build build-vfss --target moe_topk_m5f_two_round_process_e2e_test -j4
python3 VFSS/tests/moe_topk/protocol_iii_communication_benchmark.py \
  ./build-vfss/moe_topk_m5f_two_round_process_e2e_test \
  /tmp/m5h1-protocol-iii-comm-release-verified.csv
ctest --test-dir build-vfss-debug --output-on-failure
python3 -c 'import csv,sys; sys.path.insert(0,"VFSS/tests/moe_topk"); import protocol_iii_communication_benchmark as b; rows=list(csv.DictReader(open("/tmp/m5h1-protocol-iii-comm-release-verified.csv",newline=""))); b.validate(rows,b.DEFAULT_SCALES,2); print("VALIDATION_PASS",len(rows))'
```

The driver reruns all 10 sizes at two repetitions, validates each oracle-checked process row, totals, message-width formulas, target/mode determinism and the *unadjusted* theorem delta, and emits `VALIDATION_PASS`. The initial direct run and the maintained-driver rerun each produced 80 successful rows. The received/sent frame counters and output correctness are checked inside the independent-process harness. The full Debug CTest result recorded during the H1 benchmark stage was 39/39 PASS; this documentation-only closeout does not count that historical run as a new execution.

Each invocation emits a CSV header and eight rows: three Fselect targets (0, floor(n/2), n−1) and one full Fsort, each repeated twice. The maintained driver is `VFSS/tests/moe_topk/protocol_iii_communication_benchmark.py`; its second full run wrote an ephemeral raw CSV to `/tmp/m5h1-protocol-iii-comm-release-verified.csv`, which is not a repository artifact. All 80 rows passed oracle, session/material, causal-event and sender/receiver byte checks. The tables below record the identical per-n value across all targets, modes and repeats; full CSV can be regenerated by the command above.

## 6. Fselect R1/R2 message schema

Each party's R1 application body is `n` comparison-ring shares stored in 8 bytes each, followed by `n` field multiplication opening pairs `(d,e)` stored as two canonical 16-byte field elements. R2 body is `n` masked-rank shares stored in 8 bytes each, followed by `n` masked encoded-record field shares stored as 16-byte elements. Every message also has a 44-byte application binding header and a 48-byte framed-channel header. Fsort uses the exact same messages as Fselect, with `k=n,target=0` as canonical bound configuration; its n per-item FullEval calls occur locally after R2.

## 7. Fsort R1/R2 message schema

Fsort uses the exact same R1 and R2 messages as Fselect, with `k=n,target=0` as the canonical bound configuration. Its `n` per-item FullEval calls per party occur locally after R2; there is no third online message. The measured Fsort wire and logical counts equal Fselect at each n, so `FSORT_EXTRA_ONLINE_COMM=0`.

## 8. Derived implementation communication formula

Let `ell_prime=comparison_bits=33+ceil(log2 n)`, `r=rank_bits=ceil(log2 n)`, `D=2^r`. The **implementation** logical protocol payload across both online parties is:

```text
R1 = 2n(ell_prime + 2*127) bits   [masked key, field d, field e]
R2 = 2n(r + 127) bits             [masked rank, masked field record]
TOTAL = 2n*ell_prime + 2n*r + 6n*127 bits.
```

The two 127-bit field values in R1 are from independent Beaver multiplication openings; R2 transmits a third field value per item per party. These are all counted as protocol payload, not hidden in engineering overhead. Binding metadata, result reports and P2 packages are excluded from online logical communication.

Wire formulas measured at every scale, **per online party**: `R1=92+40n` bytes, `R2=92+24n` bytes. Both-party online wire is `368+128n` bytes. In R1 each party's `40n` body bytes comprise `8n` comparison storage and `32n` field storage; R2's `24n` comprise `8n` rank storage and `16n` field storage. Per message, 44 bytes are application metadata and 48 bytes are framing. The ring shares have unused high storage bits; 16-byte field encoding stores 127 logical bits in 128. Thus wire overhead over logical-bit equivalent is, for both parties, `R1:184 + 2n(66-ell_prime)/8` bytes and `R2:184 + 2n(65-r)/8` bytes. These fractions denote bit-equivalent overhead; wire itself is byte-integral. The P2 bundle lengths exclude their two 8-byte outer delivery prefixes. Controller result reports are not online protocol frames and are not counted in the wire totals.

## 9. Symbolic paper-versus-implementation comparison

The frozen **base** paper formula, after substituting `p=127-ell_prime`, is `2n*ell_prime + 2n*r + 4n*127` bits. The measured/derived implementation formula is `2n*ell_prime + 2n*r + 6n*127`. **Delta = 2n*127 = 254n bits** for every n. The project sends three field values per item per party across R1/R2; the theorem expression has capacity for two after accounting for comparison and rank terms. This identifies a concrete transcript-cost gap; it does not prove which message can safely be eliminated. The common-mask optimized theorem formula is still lower by an additional `2n*ell_prime`; that optimization is not implemented. Reinterpreting field-opening shares as metadata would be an invalid accounting change. No Theorem 4.2 logical-cost match or author-exact claim follows.

## 10. Benchmark scales and logical communication

| n | ell_prime | r | paper logical bits | implementation logical bits | delta bits | impl/paper | overhead | wire bytes | offline bundle bytes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 34 | 1 | 1156 | 1664 | 508 | 1.439446 | 43.945% | 624 | 2858 |
| 3 | 35 | 2 | 1746 | 2508 | 762 | 1.436426 | 43.643% | 752 | 7102 |
| 4 | 35 | 2 | 2328 | 3344 | 1016 | 1.436426 | 43.643% | 880 | 13076 |
| 5 | 36 | 3 | 2930 | 4200 | 1270 | 1.433447 | 43.345% | 1008 | 21532 |
| 8 | 36 | 3 | 4688 | 6720 | 2032 | 1.433447 | 43.345% | 1392 | 56992 |
| 16 | 37 | 4 | 9440 | 13504 | 4064 | 1.430508 | 43.051% | 2416 | 240984 |
| 20 | 38 | 5 | 11880 | 16960 | 5080 | 1.427609 | 42.761% | 2928 | 388452 |
| 32 | 38 | 5 | 19008 | 27136 | 8128 | 1.427609 | 42.761% | 4464 | 1002696 |
| 64 | 39 | 6 | 38272 | 54528 | 16256 | 1.424749 | 42.475% | 8560 | 4137640 |
| 128 | 40 | 7 | 77056 | 109568 | 32512 | 1.421927 | 42.193% | 16752 | 17000552 |

## 11. Fselect logical and online wire communication

Targets 0, floor(n/2), n−1 and both repetitions have the same values. The table reports both online directions, then their send sum; receives are integrity checks and are not added twice.

| n | R1 logical bits | R2 logical bits | P0→P1 R1 B | P1→P0 R1 B | P0→P1 R2 B | P1→P0 R2 B | total wire B |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1152 | 512 | 172 | 172 | 140 | 140 | 624 |
| 3 | 1734 | 774 | 212 | 212 | 164 | 164 | 752 |
| 4 | 2312 | 1032 | 252 | 252 | 188 | 188 | 880 |
| 5 | 2900 | 1300 | 292 | 292 | 212 | 212 | 1008 |
| 8 | 4640 | 2080 | 412 | 412 | 284 | 284 | 1392 |
| 16 | 9312 | 4192 | 732 | 732 | 476 | 476 | 2416 |
| 20 | 11680 | 5280 | 892 | 892 | 572 | 572 | 2928 |
| 32 | 18688 | 8448 | 1372 | 1372 | 860 | 860 | 4464 |
| 64 | 37504 | 17024 | 2652 | 2652 | 1628 | 1628 | 8560 |
| 128 | 75264 | 34304 | 5212 | 5212 | 3164 | 3164 | 16752 |

## 12. Fsort logical and online wire communication

These are separate full-Fsort process executions through `consume_round2_sort()` and field FullEval; they are not repeated Fselect calls. They match the corresponding Fselect messages at each n.

| n | R1 logical bits | R2 logical bits | P0→P1 R1 B | P1→P0 R1 B | P0→P1 R2 B | P1→P0 R2 B | total wire B |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1152 | 512 | 172 | 172 | 140 | 140 | 624 |
| 3 | 1734 | 774 | 212 | 212 | 164 | 164 | 752 |
| 4 | 2312 | 1032 | 252 | 252 | 188 | 188 | 880 |
| 5 | 2900 | 1300 | 292 | 292 | 212 | 212 | 1008 |
| 8 | 4640 | 2080 | 412 | 412 | 284 | 284 | 1392 |
| 16 | 9312 | 4192 | 732 | 732 | 476 | 476 | 2416 |
| 20 | 11680 | 5280 | 892 | 892 | 572 | 572 | 2928 |
| 32 | 18688 | 8448 | 1372 | 1372 | 860 | 860 | 4464 |
| 64 | 37504 | 17024 | 2652 | 2652 | 1628 | 1628 | 8560 |
| 128 | 75264 | 34304 | 5212 | 5212 | 3164 | 3164 | 16752 |

## 13. Ratio and constant-factor analysis

Across all ten sizes, the implementation/paper **logical-bit** ratio is `1.421927–1.439446`; its overhead is `+42.193%–+43.945%`. The exact symbolic delta is `254n` bits. Both expressions are linear in n for fixed widths, with the same comparison-width and rank-width terms, so `ASYMPTOTIC_ORDER_MATCH=YES` and `ORDER_OF_MAGNITUDE_MATCH=YES`; `CONSTANT_FACTOR_EXACT_MATCH=NO`. These ratios use the paper's **unoptimized** formula. They do not certify an author-exact transcript or an identical functional payload budget.

At `n=128`, the paper's logical cost is 77,056 bits and this implementation sends 109,568 logical bits: `1.421927×`, or `+42.193%`. Its measured online wire is 16,752 bytes = 134,016 bits, about `1.739203×` the paper logical cost. The 1.74× figure mixes protocol bits with serialization/framing and is **not** the logical-cost comparison criterion.

## 14. Wire overhead breakdown

The measured online wire counts **both online parties' sends**, never sends plus receives. Each of the four online frames (P0/P1 in R1/R2) has 44 bytes of application binding metadata and 48 bytes of transport framing. Each party sends `92+40n` bytes in R1 and `92+24n` bytes in R2; together they send `368+128n` bytes. The `368` fixed bytes are four times `44+48`. R1 body uses 8 bytes per comparison-ring share plus two 16-byte field encodings per item. R2 body uses 8 bytes per rank share plus one 16-byte field encoding per item. Logical widths count `ell_prime`, `r` and 127 bits instead of those fixed storage widths.

Thus `WIRE_BYTES > LOGICAL_PROTOCOL_BYTES` reflects application metadata, framing and canonical/fixed-width storage. Theorem 4.2's communication formula concerns logical protocol bits; it excludes those engineering bytes, offline dealer bundles and controller-only result reports.

## 15. Measured offline material (separate from online)

Each entry is a serialized party bundle length, excluding one 8-byte delivery prefix for each party. Both Fselect and Fsort use one fresh same-shaped bundle per execution, so bytes are identical at fixed n. These are *measured*, not the theorem's `dominated by` offline-key formula.

| n | P0 bundle B | P1 bundle B | both bundles B | delivery prefixes B |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 1429 | 1429 | 2858 | 16 |
| 3 | 3551 | 3551 | 7102 | 16 |
| 4 | 6538 | 6538 | 13076 | 16 |
| 5 | 10766 | 10766 | 21532 | 16 |
| 8 | 28496 | 28496 | 56992 | 16 |
| 16 | 120492 | 120492 | 240984 | 16 |
| 20 | 194226 | 194226 | 388452 | 16 |
| 32 | 501348 | 501348 | 1002696 | 16 |
| 64 | 2068820 | 2068820 | 4137640 | 16 |
| 128 | 8500276 | 8500276 | 17000552 | 16 |

## 16. Local computation and padded-domain effect

These are **code-derived call counts**, not instrumented PRG/AES-call measurements. Shared `protocol_i_cmpagg_eval_party` traverses `C(n,2)` edges per party. Its `ProtocolIUcmpPartyMaterial::eval_strict_lt` invokes `evalDCF` twice per edge; hence this implementation calls `4*C(n,2)` DCF Eval across both parties versus Theorem 4.2's `2*C(n,2)`. Fselect calls one field DPF Eval per item per party, matching the theorem's `2n` *call count*. Fsort calls one field FullEval per item per party; each walks all `D=2^r` project domain leaves. Routing aggregation makes n field multiplications/additions per party for Fselect and n² of each per party for Fsort. Field multiplication preparation/finish and internal DPF arithmetic are excluded from those routing counts. Actual local PRG calls and total field operation counts are `NOT_MEASURED`.

| n | edges/party | DCF Eval/party | Fselect DPF Eval/party | Fsort FullEval/party | D | FullEval leaves/party | padding D/n | Fsort routing mul/party |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1 | 2 | 2 | 2 | 2 | 4 | 2/2 | 4 |
| 3 | 3 | 6 | 3 | 3 | 4 | 12 | 4/3 | 9 |
| 4 | 6 | 12 | 4 | 4 | 4 | 16 | 4/4 | 16 |
| 5 | 10 | 20 | 5 | 5 | 8 | 40 | 8/5 | 25 |
| 8 | 28 | 56 | 8 | 8 | 8 | 64 | 8/8 | 64 |
| 16 | 120 | 240 | 16 | 16 | 16 | 256 | 16/16 | 256 |
| 20 | 190 | 380 | 20 | 20 | 32 | 640 | 32/20 | 400 |
| 32 | 496 | 992 | 32 | 32 | 32 | 1024 | 32/32 | 1024 |
| 64 | 2016 | 4032 | 64 | 64 | 64 | 4096 | 64/64 | 4096 |
| 128 | 8128 | 16256 | 128 | 128 | 128 | 16384 | 128/128 | 16384 |

For n=3,5,20 the FullEval domain has 4,8,32 leaves rather than the logical 3,5,20 ranks. The project rank ring remains `Z_(2^r)` rather than paper `Z_n`; this enlarges local work without changing online message-body size beyond the `r`-bit rank width. All non-power-of-two cases pass the same output oracle and no padded output slot is emitted.

## 17. FullEval and padded-domain effect

Fselect invokes `n` field DPF.Eval calls per party, or `2n` across both, matching the paper's DPF.Eval call structure. Fsort invokes `n` field DPF.FullEval calls per party, or `2n` across both. Each FullEval traverses `D=2^r` leaves, so local leaves per party are `nD`; for `n=3,5,20`, respectively `D=4,8,32`. This local-computation padding does not add an online message or logical communication. Wrapper-specific field work is a project C-INSTANTIATION; actual PRG/AES calls remain `NOT_MEASURED`.

## 18. Two-round causal verification

For every measured Fselect and Fsort run: P2 exited before input release; P0/P1 prepared each outbound before reading that round's peer message; one R1 and one R2 protocol frame were sent per party; no post-R2 protocol frame was sent. Independent process outputs matched the clear test-only oracle. Communication was deterministic over two repetitions and independent of Fselect target. `FSORT_EXTRA_ONLINE_COMM=0` in this implementation.

## 19. Non-power-of-two cases

For `n=3,5,20`, the project routes over padded domains of 4, 8 and 32, respectively. Its rank shares and DPF inputs use `Z_(2^r)`; the paper states `Z_n`. The measured Fselect and Fsort outputs passed the same clear test-only oracle, but this does **not** erase the domain difference. The larger FullEval leaf counts are local work; there is no extra online protocol frame.

## 20. Theorem 4.2 alignment matrix

| Property | Paper / project comparison | H1 result |
| --- | --- | --- |
| Online causal rounds | 2 / 2 for Fselect and Fsort | Exact round match: YES |
| Post-R2 protocol frames | No further online communication / 0 observed | Hidden third round: NO |
| Logical online scale | Unoptimized theorem / measured implementation | Order-of-magnitude and asymptotic-order match: YES |
| Logical online exact cost | Paper substitution `2n*ell_prime+2n*r+4n*127` / implementation `2n*ell_prime+2n*r+6n*127` | Exact match: NO; delta `254n` bits |
| Fsort online communication | FullEval replaces Eval locally / same measured R1 and R2 | Extra online communication: 0 |
| Ranking DCF Eval calls | `2*C(n,2)` across both / `4*C(n,2)` code-derived | Same `Theta(n^2)` work; exact call count: NO |
| Fselect DPF Eval calls | `2n` across both / `2n` code-derived | Structural call-count match: YES |
| Fsort DPF FullEval calls | `2n` across both / `2n` code-derived | Structural call-count match: YES; padded local leaves |
| Routing domain | `Z_n` / `Z_(2^r)` | C-INSTANTIATION |
| Common-mask optimization | Optional paper formula / not implemented | Excluded from H1 comparison |
| Author-exact transcript | Not supplied by this evidence | NOT_PROVEN |

## 21. Known differences and limitations

The implementation pays one additional 127-bit field share **per item per online party** relative to the unoptimized theorem substitution; the two 127-bit R1 Beaver opening components and the R2 127-bit field component remain counted as logical protocol values. The shared uCMP realization invokes two DCF.Eval calls per edge per party, versus the theorem's one-call-per-edge-per-party cost term. Neither difference is reclassified as transport overhead. The optional common-mask optimization is not implemented.

This report measures the already-field-shared two-round core. Secure ring-to-field conversion, original-order Top-K mask adaptation, author-exact transcript equivalence and a formal field-DPF security proof remain outside H1's verified claim. Wire measurements use Unix socketpairs on one host and exclude LAN/WAN stack effects. Offline bundle bytes are separate from online logical bits. The count comparison is not a new functional or security proof.

## 22. Data provenance and reproducibility

The ten scales are `2,3,4,5,8,16,20,32,64,128`; each has two repetitions of three Fselect targets and one Fsort, for 80 independent-process rows. The exact build, benchmark and CTest commands are in §5. The Python driver recomputes and validates the message formulas, round count, offline sums and paper/implementation delta before writing the raw CSV. Table 10's ratios and percentages were calculated from that CSV as `implementation_bits/paper_bits` and `(ratio-1)*100`, rather than copied from a verbal estimate. Fselect and Fsort have equal online logical and wire totals at every measured scale. The raw CSV is an ephemeral `/tmp` output; rerun the command to regenerate it.

## 23. H1 acceptance verdict

- `M5_H1_COMMUNICATION_PASS=YES` under the user-specified `ACCEPTANCE_CRITERION=ORDER_OF_MAGNITUDE_MATCH`.
- `THEOREM_4_2_ROUND_MATCH=YES`; `FSELECT_ONLINE_ROUNDS=2`; `FSORT_ONLINE_ROUNDS=2`; `POST_R2_PROTOCOL_FRAME_COUNT=0`.
- `THEOREM_4_2_STRUCTURAL_MATCH=PARTIAL / MATCH_WITH_C_INSTANTIATION`.
- `THEOREM_4_2_LOGICAL_COMM_SCALE_MATCH=YES`; `ASYMPTOTIC_ORDER_MATCH=YES`.
- `THEOREM_4_2_LOGICAL_COMM_EXACT_MATCH=NO`; `THEOREM_4_2_EXACT_COST_MATCH=NO`; implementation/paper logical ratio `1.42–1.44`, approximately `+42%–+44%`.
- `LOCAL_COMPUTE_ASYMPTOTIC_ORDER_MATCH=YES`; `LOCAL_COMPUTE_EXACT_CALL_COUNT_MATCH=NO` for DCF.Eval.
- `FSORT_EXTRA_ONLINE_COMM=0`; `COMMON_MASK_OPTIMIZATION=NOT_IMPLEMENTED`.
- `AUTHOR_TRANSCRIPT_PROOF=NOT_PROVEN`; `AUTHOR_EXACT=NOT_PROVEN`.

当前实现在线通信与论文 Theorem 4.2 保持相同渐进数量级和两轮结构，但存在约 1.42–1.44 倍的常数因子开销，因此通过项目的数量级通信验收，不构成精确成本复现。

## 24. Stage boundary

`M5_OVERALL_STATUS=IN_PROGRESS`. The next stage is `M5-H2 independent review`: independently audit the extra field share and the shared uCMP DCF-call factor without altering the frozen paper specification or H1 counts. This H1 document does not complete M5 or assert an author-exact implementation.
