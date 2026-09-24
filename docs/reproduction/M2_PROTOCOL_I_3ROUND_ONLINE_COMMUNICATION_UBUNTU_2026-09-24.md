# M2 Protocol I three-round C-INSTANTIATION online communication

Date: 2026-09-24

Status: **PROTOCOL LOGICAL MATCH; APPLICATION-WIRE OVERHEAD MEASURED.**

## Evidence scope

The measured object is the current correlated-parallel, three-round Protocol I
`C-INSTANTIATION`. It is not the unpublished Agarwal author-exact transcript.
The purpose of this record is to determine whether the implementation's online
logical communication matches Agarwal Theorem 4.1 for identical
`(n, ell', p)` parameters, and to separately measure its application-level wire
traffic.

The result is:

- protocol logical communication matches Theorem 4.1 exactly at every tested
  parameter point;
- application-level wire communication is higher because of framing,
  fixed-width record storage, and byte-packed rank shares;
- `COST_MATCH != AUTHOR_TRANSCRIPT_PROOF`.

This measurement does not change the author-exact status, the unresolved
dealer/full-`r` interpretation, or the strict M2 G1/G2/G3 gates recorded in the
canonical decision documents.

## Provenance and measured implementation

| Field | Value |
| --- | --- |
| Branch | `feat/m2-chase-c1-c3` |
| HEAD | `5d3f36a5bdf5be821ba86bcd959e4c1b5adae9b1` |
| `origin/main` at measurement | `5d3f36a5bdf5be821ba86bcd959e4c1b5adae9b1` |
| Worktree | dirty; contains the uncommitted C1/C3 implementation and measurement harness |
| Environment | WSL2 Linux `5.15.167.4-microsoft-standard-WSL2`, x86_64 |
| Compiler | GCC/G++ 11.4.0 |
| CMake | 3.22.1 |
| Measurement build | `build-vfss`, Release |
| Regression build | `build-vfss-debug`, Debug |

The implementation is a project `C-INSTANTIATION`: offline P2 creates the
correlated packages and exits before online input; P0 and P1 then execute the
three causal online rounds. P2 package delivery, permutations, masks, and
CmpAgg/DCF key material are offline and are excluded from all online counts in
this record.

## Paper baseline and counting scope

Agarwal Theorem 4.1 gives Protocol I online communication as

```text
4n(ell' + p) + 2n ceil(log2 n) bits
```

across both online parties in total. The two components are:

```text
R1 + R2 shuffle: 4n(ell' + p) bits
R3 rank opening:  2n ceil(log2 n) bits
```

Tables 2 and 3 normalize communication by the number of online parties. For a
balanced two-online-party Protocol I execution, their comparison scope is

```text
[4n(ell' + p) + 2n ceil(log2 n)] / 2.
```

The Theorem 4.1 total must therefore not be compared directly with a Table 2
per-party value.

This record distinguishes:

1. `PAPER_LOGICAL`: the formula above, excluding framing and serialization;
2. `IMPLEMENTATION_LOGICAL`: the mathematical R1/R2 record values and R3 rank
   shares at their actual logical widths;
3. `IMPLEMENTATION_WIRE`: application-level serialized bytes sent through the
   framed channel, including the current frame and storage representation but
   excluding Ethernet, IP, TCP, kernel/network-stack headers, and packet
   retransmission.

Total communication is the sum of sends only:

```text
total online wire = P0 sent + P1 sent.
```

Received bytes are retained only for the integrity checks
`P0 sent == P1 received` and `P1 sent == P0 received`; they are not added to
the total a second time.

## Implementation parameters

The comparison group is `Z_(2^ell'_impl)` with

```text
ell'_impl = 33 + ceil(log2 n).
```

The stable priority key includes the signed Q20.12 score and the original-index
tie-break. `ell'_impl` is the actual comparison-group width; it is not the raw
32-bit score width.

The payload is two independent `uint64_t` lanes:

```text
p_impl = 128 bits.
```

The record storage type is `ProtocolIBlock192`, with three 64-bit lanes:

```text
logical record width B = ell'_impl + 128 bits
storage width              = 192 bits/record.
```

Although original index is encoded into the comparison key for stable ranking,
the record also carries its original-index/payload lane. Both transported
64-bit payload lanes are included in `p_impl`.

| n | `ell'_impl` | logical record bits B | rank logical bits |
| ---: | ---: | ---: | ---: |
| 2 | 34 | 162 | 1 |
| 4 | 35 | 163 | 2 |
| 8 | 36 | 164 | 3 |
| 16 | 37 | 165 | 4 |
| 20 | 38 | 166 | 5 |
| 32 | 38 | 166 | 5 |
| 64 | 39 | 167 | 6 |
| 128 | 40 | 168 | 7 |

Rank shares use `ceil(log2 n)` logical bits. For every benchmark point in this
record the current serialization uses one byte per rank share per record.

## Test method

Each case used independent P2, P0, and P1 processes. P2 generated and delivered
fresh preprocessing material, exited, and was reaped before the controller
released online input. R1, R2, and R3 then used real framed inter-process
messages.

The measured path includes:

- the correlated-parallel shuffle;
- reconstruction of public `y = Pi(x) + r`;
- real `ProtocolIUcmpMaterial`;
- real `protocol_i_cmpagg_eval_party` / `EvalCmpAgg` evaluation;
- R3 rank-share opening;
- final secret-record routing.

The clear oracle is TEST_ONLY and participates only in correctness comparison;
it does not produce protocol messages or routing decisions.

For each `n` in `2, 4, 8, 16, 20, 32, 64, 128`, the harness ran one warm-up
followed by five formal executions. The 40 formal executions each used fresh
preprocessing material. Warm-up results were not included in the statistics.

The measurement command was:

```text
./build-vfss/moe_topk_m2_parallel_shuffle_process_e2e_test --comm-benchmark-128
```

The harness wrote an ephemeral CSV under `/tmp` during the run. That temporary
file is not a durable artifact; the complete long-term core results are
recorded below.

## Logical communication derivation

Let `B = ell'_impl + p_impl`. The implementation messages are:

```text
R1: m0 + m1 = 2nB bits
R2: d0 + d1 = 2nB bits
R3: q0 + q1 = 2n ceil(log2 n) bits.
```

Therefore:

```text
OUR_LOGICAL_TOTAL
  = 4nB + 2n ceil(log2 n)
  = 4n(ell' + p) + 2n ceil(log2 n).
```

At every tested parameter point:

```text
LOGICAL_DELTA = 0 bits
LOGICAL_RATIO = 1.000000.
```

This equality is the principal result of this communication validation.

## Application-wire derivation

For this benchmark range and current serialization only:

- R1 sends one `ProtocolIBlock192` vector in each direction;
- R2 sends one `ProtocolIBlock192` vector in each direction;
- R3 sends one byte-packed rank vector in each direction;
- each of the six application messages has a 48-byte frame.

Thus:

```text
P0 wire = P1 wire = 144 + 49n bytes
OUR_WIRE_TOTAL     = 288 + 98n bytes.
```

This formula must not be generalized beyond the measured range or after a
change to the 48-byte frame, `ProtocolIBlock192`, or the one-byte rank
serialization.

| n | P0 sent (B) | P1 sent (B) | total wire (B) |
| ---: | ---: | ---: | ---: |
| 2 | 242 | 242 | 484 |
| 4 | 340 | 340 | 680 |
| 8 | 536 | 536 | 1,072 |
| 16 | 928 | 928 | 1,856 |
| 20 | 1,124 | 1,124 | 2,248 |
| 32 | 1,712 | 1,712 | 3,424 |
| 64 | 3,280 | 3,280 | 6,560 |
| 128 | 6,416 | 6,416 | 12,832 |

## Wire-overhead decomposition

For the measured range:

```text
WIRE_OVERHEAD_BITS
  = 2304
  + 4n(64 - ell')
  + 2n(8 - ceil(log2 n)).
```

The terms are:

- `2304` bits: six 48-byte application frames;
- `4n(64-ell')` bits: R1/R2 padding from logical records to the fixed 192-bit
  record storage;
- `2n(8-ceil(log2 n))` bits: R3 byte-packing overhead.

The frame includes current application message metadata such as length,
session, sender/receiver, phase/type, and sequence fields. No Ethernet, IP,
TCP, kernel/network-stack, or packet-capture overhead is included.

## Results

| n | ell' | Paper/Our logical (B) | Total wire (B) | Wire overhead | Wire/Paper |
| --: | ---: | --------------------: | -------------: | ------------: | ---------: |
| 2 | 34 | 162.5 | 484 | 197.846% | 2.978462 |
| 4 | 35 | 328 | 680 | 107.317% | 2.073171 |
| 8 | 36 | 662 | 1,072 | 61.934% | 1.619335 |
| 16 | 37 | 1,336 | 1,856 | 38.922% | 1.389222 |
| 20 | 38 | 1,685 | 2,248 | 33.412% | 1.334125 |
| 32 | 38 | 2,696 | 3,424 | 27.003% | 1.270030 |
| 64 | 39 | 5,440 | 6,560 | 20.588% | 1.205882 |
| 128 | 40 | 10,976 | 12,832 | 16.910% | 1.169096 |

As `n` grows, the fixed frame cost and byte-alignment overhead are amortized,
so the application-wire/paper-logical ratio moves toward one. This trend does
not erase the distinction between logical protocol communication and physical
storage/framing.

## Table 2 comparison boundary

```text
TABLE2_DIRECT_COMPARISON = NOT_APPLES_TO_APPLES.
```

Table 2 examples use parameters such as `n=20, ell=8, p=0` and
`n=20, ell=64, p=0`, whereas this implementation at `n=20` uses
`ell'=38, p=128`.

The per-online-party normalization can nevertheless be checked against the
paper formula:

```text
n=20, ell=8, p=0:
  [4*20*8 + 2*20*ceil(log2 20)] / 2
  = 420 bits = 52.5 B, approximately 0.05 KB.

n=20, ell=64, p=0:
  [4*20*64 + 2*20*ceil(log2 20)] / 2
  = 2660 bits = 332.5 B, approximately 0.33 KB.
```

These values agree with the scale of the Table 2 entries and confirm the
normalization boundary. They do not authorize a performance ranking against
the current `n=20` result, whose normalized logical communication is 842.5 B
per party and whose measured application-wire communication is 1,124 B per
party. The parameter meanings differ.

## Determinism, correctness, and regressions

```text
DETERMINISTIC_BYTES = YES
E2E_CORRECTNESS     = PASS
```

For each `n`, the five formal runs had `min == max == mean`; P0 and P1 had
identical sent-byte counts. The harness checked the sent/received equality in
both directions for every round.

Validation performed with this measurement:

| Check | Result |
| --- | --- |
| Release parallel-shuffle process E2E | PASS |
| Release parallel-shuffle conformance | PASS |
| Release communication benchmark, 8 sizes and 40 formal executions | PASS |
| Debug focused Protocol I regression | 9/9 PASS |
| `git diff --check` | PASS |

The focused Debug regression covered priority key, priority DCF, CmpAgg
conformance and process E2E, transport, score input, paper-core alignment,
parallel-shuffle conformance, and parallel-shuffle process E2E.

## Conclusion and limitations

The current three-round Protocol I `C-INSTANTIATION` matches Agarwal Theorem
4.1 exactly at the logical online-communication level for identical
`(n, ell', p)` parameters:

```text
LOGICAL_DELTA = 0
LOGICAL_RATIO = 1.0.
```

The measured application-level wire traffic is higher because of framing,
fixed-width 192-bit record storage, and byte-packed rank shares.

This establishes `PROTOCOL_LOGICAL_MATCH` and separately records
`ENGINEERING_WIRE_OVERHEAD`. It does not establish that the current
construction is Agarwal's unpublished author transcript, does not close the
author-exact security obligations, and is not a LAN/WAN runtime-performance
comparison.
