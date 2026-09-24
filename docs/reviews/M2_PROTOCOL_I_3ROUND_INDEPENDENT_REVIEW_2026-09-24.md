# M2 Protocol I Three-Round Independent Review

Date: 2026-09-24
Branch: `feat/m2-chase-c1-c3`
HEAD: `5d3f36a5bdf5be821ba86bcd959e4c1b5adae9b1`
Reviewed state: dirty working tree containing the uncommitted correlated-parallel implementation
Verdict: **PASS**

## 1. Review Scope

This review asks only whether the current Protocol I correlated-parallel
`C-INSTANTIATION` has three causal online rounds:

```text
R1 parallel correlated shuffle
R2 public masked shuffled-list opening
LOCAL CmpAgg
R3 rank-share opening
```

A round is causal only if each party's outbound value is determined before it
reads the peer's inbound value for that round. This is not a full security
audit, a proof of the CmpAgg/DCF mathematics, or validation of the unpublished
Agarwal author-exact transcript.

## 2. Reviewed Implementation

Primary files:

- `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`
- `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp`
- `VFSS/tests/moe_topk/protocol_i_parallel_shuffle_three_process_e2e_test.cpp`
- `VFSS/src/moe_topk/protocol_i_transport.cpp`
- `VFSS/src/moe_topk/protocol_i_cmpagg.cpp`
- `VFSS/include/moe_topk/protocol_i_ucmp.h`
- `VFSS/src/moe_topk/protocol_i_ucmp.cpp`
- `VFSS/CMakeLists.txt` for the test registrations

Claim/boundary context:

- `docs/reproduction/M2_PROTOCOL_I_3ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-24.md`
- `docs/decisions/M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md`
- the directly relevant M2 section of `docs/IMPLEMENTATION_PLAN.md`

## 3. Expected Three-Round Schedule

| Stage | Required action | Causal-round status |
| --- | --- | --- |
| R1 | Both parties freeze and exchange correlated-shuffle messages | online round 1 |
| R2 | Both parties freeze and exchange masked shuffled shares; both derive public `y` | online round 2 |
| LOCAL | Each party evaluates its own CmpAgg/DCF material | no communication |
| R3 | Both parties freeze and exchange rank shares | online round 3 |
| post-R3 | Reconstruct public shuffled ranks and route local shares | no communication |

## 4. Actual Message Transcript

Offline, P2 generates and sends one bound package to each online party, then
exits. The three-process harness reaps P2 before releasing online input.

```text
R1 (parallel, one framed record vector in each direction):
  P0 -> P1: m0 = sigma0(x0) + a0
  P1 -> P0: m1 = sigma1(x1) + a1
  local after receive:
    P0: s0 = tau0(m1) + e0
    P1: s1 = tau1(m0) + e1

R2 (parallel, one framed record vector in each direction):
  P0 -> P1: d0 = s0 + r0
  P1 -> P0: d1 = s1 + r1
  local after receive, at both parties:
    y = d0 + d1 = Pi(x) + r

LOCAL (no message):
  P0: q0 = EvalCmpAgg(0, y, party0 edge material)
  P1: q1 = EvalCmpAgg(1, y, party1 edge material)

R3 (parallel, one framed rank vector in each direction):
  P0 -> P1: q0
  P1 -> P0: q1
  local after receive:
    q = q0 + q1; validate ranks; route shuffled shares locally
```

The E2E controller's input delivery, P2 statistics, and post-run result
collection use separate `TEST_ONLY` channels. They are validation plumbing,
not messages in the reviewed online protocol.

## 5. R1 Review

**R1_CAUSAL: PASS.** `prepare_round1()` computes the complete `m` vector from
the local input share and local material. `encode_records(m)` is completed
before `exchange_payload()` is called. Inside the exchange, the immutable
outbound buffer is passed to a sender thread before the receiver reads the peer
frame. No peer-dependent R1 response is sent.

## 6. R2 Review

**R2_CAUSAL: PASS.** After R1 completes,
`receive_round1_prepare_round2()` computes and stores the complete `d` vector.
`encode_records(d)` is completed before the R2 exchange reads its peer frame.

**PUBLIC_Y_AFTER_R2: YES.** `receive_round2()` computes
`public_masked_ = round2_outbound_ + peer_round2` in both processes. There is
no separate `P1 -> P0: y` response and no other R2 message.

## 7. Local CmpAgg Review

**CMPAGG_BETWEEN_R2_R3_LOCAL_ONLY: YES.** The network entry point calls
`receive_round2()`, then `evaluate_cmpagg_prepare_round3()`, then the R3
exchange. The local call chain is:

```text
protocol_i_cmpagg_eval_party
  -> ProtocolIUcmpPartyMaterial::eval_strict_lt
  -> evalDCF
```

It operates on public masked keys and pre-distributed party-local material.
The reviewed call chain contains no socket, file, callback, Dealer, or other
peer I/O.

## 8. R3 Review

**R3_CAUSAL: PASS.** `evaluate_cmpagg_prepare_round3()` completes and masks the
entire local rank-share vector before `encode_ranks(q)` and the R3 exchange.
The R3 payload is only that rank-share vector. `receive_round3()` reconstructs
and validates public shuffled ranks, then performs local share routing.

**POST_R3_ONLINE_MESSAGE: NO.** The reviewed protocol entry point returns after
local routing. Each of its three round descriptors has already carried exactly
one frame in each direction; no fourth protocol exchange exists.

## 9. Dealer Review

**P2_ONLINE_SILENT: YES.** In the three-process harness P2 retains only its two
package descriptors and the test statistics descriptor. It does not retain any
R1/R2/R3 descriptor. The controller calls `waitpid(P2)` successfully before it
sends either online input share. P2 therefore cannot send, receive, or answer
an online request in this execution.

## 10. Hidden Fourth-Round and Transport Check

**HIDDEN_FOURTH_ROUND: NO.** The online entry point contains exactly three
calls to `exchange_payload()`, using three distinct connected full-duplex file
descriptors. Each call performs one `ProtocolIFramedChannel::send()` and one
`receive()` per party. A frame consists of a 48-byte header plus its already
fixed payload; header/payload writes, `poll`, buffering, EOF handling, and
`POLLIN|POLLHUP` handling do not create an acknowledgement or response frame.

## 11. Three-Process E2E Review

**THREE_PROCESS_E2E_VALIDATES_CURRENT_STRUCTURE: YES.** The harness uses
`fork()` plus `execv()` to run independent P2, P0, and P1 processes. P0 and P1
execute the production three-round entry point over separate R1/R2/R3 socket
pairs. The test checks cross-direction sent/received byte equality for every
round, exact per-round wire sizes, public `y` equality, real CmpAgg output,
three-round accounting, and P2 exit before online input. Its normal cases cover
`n=2,4,8` (including two `n=8` values of `k`).

The test does not emit a syscall event trace. The outbound-before-inbound fact
is established by the reviewed state-machine source and exercised by the
independent-process run; it is not inferred only from the declarative
`online_rounds` metric.

## 12. Tests Executed

```text
cmake --build build-vfss --target \
  moe_topk_m2_parallel_shuffle_conformance_test \
  moe_topk_m2_parallel_shuffle_process_e2e_test -j2

./build-vfss/moe_topk_m2_parallel_shuffle_conformance_test
./build-vfss/moe_topk_m2_parallel_shuffle_process_e2e_test --report
./build-vfss/moe_topk_m2_parallel_shuffle_process_e2e_test --comm-benchmark
```

Results:

- conformance: **PASS**;
- independent-process E2E: **PASS** for `n=2,4,8` and `n=8,k=8`;
- communication benchmark: **PASS** for `n=2,4,8,16,20,32,64`, with one
  warm-up plus five fresh formal runs per size;
- representative `n=8`: 5,296 logical bits; P0/P1 each sent 536 wire bytes;
  total 1,072 wire bytes; deterministic across formal runs.

## 13. Findings and Verdict

No blocking or nonblocking finding was found within the requested causal-round
scope.

```text
R1_CAUSAL                              PASS
R2_CAUSAL                              PASS
PUBLIC_Y_AFTER_R2                      YES
CMPAGG_BETWEEN_R2_R3_LOCAL_ONLY        YES
R3_CAUSAL                              PASS
POST_R3_ONLINE_MESSAGE                 NO
P2_ONLINE_SILENT                       YES
TOTAL_CAUSAL_ROUNDS                    3
HIDDEN_FOURTH_ROUND                    NO
REVIEW_VERDICT                         PASS
```

## 14. Evidence Boundary

```text
Three-round C-INSTANTIATION review PASS
!=
Agarwal author-exact transcript proven
```

This verdict does not change the recorded author-exact, formal-simulation,
Dealer/full-`r`, or strict M2 G1/G2/G3 evidence status.
