# M2 CmpAgg three-process E2E — Ubuntu 2026-09-05

## Evidence and scope

- Tested implementation revision: `119e527` (`fix: harden M2 framed process transport`).
- Implementation label: `m2_priority_cmpagg_three_process_e2e`.
- Evidence level: project extension (C), locally tested in independent processes (B); not a
  complete Agarwal Protocol I claim.
- This run is the first real fork+exec P2/P0/P1 CmpAgg data flow. Commit `6b79034` only
  completed the older bounded transport smoke test.
- uCMP is the project's two-evaluation VFSS DCF adapter. Secure shuffle, inverse routing,
  final original-order mask generation, full Protocol I accounting and WAN performance are
  not implemented or are `NOT_MEASURED`.

## Environment and commands

Ubuntu 24.04.4 LTS (Noble), WSL2, x86_64; kernel
`6.6.87.2-microsoft-standard-WSL2`; GCC/G++ 13.3.0; CMake 3.28.3; 32 online CPUs;
available memory at capture: 6.9 GiB. The build was a fresh Debug directory:
`/tmp/moe_m27_final.8kSYej`.

```bash
cmake -S VFSS -B /tmp/moe_m27_final.8kSYej -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/moe_m27_final.8kSYej -j1
ctest --test-dir /tmp/moe_m27_final.8kSYej -N
ctest --test-dir /tmp/moe_m27_final.8kSYej --output-on-failure
```

CTest discovered 11 tests: M1 `4/4`, M2 `7/7`, total `11/11`. The full CTest command
reported `100% tests passed, 0 tests failed out of 11`; total test time was 2.52 seconds.
The process target itself ran ten independent P2/P0/P1 cases and passed every case.

## Topology and ordering

The controller creates all socketpairs before forking. P0 and P1 first block on their
package channels. P2 generates fresh full node masks, additive node-mask shares and one
party-specific DCF/uCMP material per canonical edge `i<j`, sends separate `M2PK` packages
to P0/P1, reports TEST_ONLY package counters, closes its channels and exits. Only after
`waitpid(P2)` returns normal exit does the controller send the two TEST_ONLY online
priority-key-share messages. P0/P1 then exchange one framed masked-key vector and return
additive rank shares to the controller. P2 receives no score, priority key or online input.

The P0/P1 frame is 48-byte fixed-width big-endian header plus an 8-byte value for each
masked key. The direction counters below are actual header+payload bytes. Thus
`online_comm_total_bits = 8 * (P0→P1 + P1→P0)` and
`online_comm_per_party_bits = online_comm_total_bits / 2`.

## Case results

`P2→P0/P1` is the serialized party package, including its `M2PK` header and payload.
TEST_ONLY bytes are listed as `controller→P0/controller→P1; P0-result/P1-result;
P2-stats`.

| n | K | comparison bits | score vector / input seed | P2 seed | edges | P2→P0/P1 bytes | P0→P1 / P1→P0 bytes | TEST_ONLY bytes | reconstructed rank / mask |
| ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | --- | --- |
| 1 | 1 | 34 | `[0]` / 8193 | 4097 | 0 | 48 / 48 | 56 / 56 | 48/48; 64/64; 32 | `[0]` / `[1]` |
| 2 | 1 | 34 | `[INT32_MIN, INT32_MAX]` / 8194 | 4098 | 1 | 953 / 953 | 64 / 64 | 56/56; 72/72; 32 | `[1,0]` / `[0,1]` |
| 2 | 2 | 34 | `[7,7]` / 8195 | 4099 | 1 | 953 / 953 | 64 / 64 | 56/56; 72/72; 32 | `[0,1]` / `[1,1]` |
| 5 | 1 | 36 | `[5,5,1,5,3]` / 8197 | 4101 | 10 | 9530 / 9530 | 88 / 88 | 80/80; 96/96; 32 | `[0,1,4,2,3]` / `[1,0,0,0,0]` |
| 5 | 5 | 36 | `[INT32_MAX,0,INT32_MIN,0,INT32_MAX]` / 8198 | 4102 | 10 | 9530 / 9530 | 88 / 88 | 80/80; 96/96; 32 | `[0,2,4,3,1]` / `[1,1,1,1,1]` |
| 7 | 1 | 36 | `[INT32_MIN,4,4,INT32_MAX,3,2,1]` / 8199 | 4103 | 21 | 19941 / 19941 | 104 / 104 | 96/96; 112/112; 32 | `[6,1,2,0,3,4,5]` / `[0,0,0,1,0,0,0]` |
| 7 | 7 | 36 | all `9` / 8200 | 4104 | 21 | 19941 / 19941 | 104 / 104 | 96/96; 112/112; 32 | `[0,1,2,3,4,5,6]` / `[1,1,1,1,1,1,1]` |
| 11 | 1 | 40 | `[INT32_MIN,11,INT32_MAX,42,11,0,42,3,8,8,19]` / 8203 | 4107 | 55 | 57383 / 57383 | 136 / 136 | 128/128; 144/144; 32 | `[10,4,0,1,5,9,2,8,6,7,3]` / `[0,0,1,0,0,0,0,0,0,0,0]` |
| 11 | 2 | 40 | same fixed vector / 8204 | 4108 | 55 | 57383 / 57383 | 136 / 136 | 128/128; 144/144; 32 | same rank / `[0,0,1,1,0,0,0,0,0,0,0]` |
| 11 | 11 | 40 | `[6,6,6,2,2,0,0,1,1,3,3]` / 8205 | 4109 | 55 | 57383 / 57383 | 136 / 136 | 128/128; 144/144; 32 | `[0,1,2,5,6,9,10,7,8,3,4]` / all `1` |

The `P0→P1` and `P1→P0` values are also the per-party online communication bytes.
For example, the n=5 cases have 176 total online bytes = 1408 bits and 704 bits per
online party. `online_rounds=1` for this foundation. `online_prg_calls_total`, offline
time, offline material time/bit totals, online wall-clock time, total time, WAN/network
measurements, warmups and repeated performance statistics are `NOT_MEASURED`; the package
and frame byte counters above are the raw observed communication evidence.

## Negative and conformance coverage

The process target rejects `n=0`, `K=0`, `K>n`, wrong party, wrong comparison width, missing
or incorrect edge count, duplicate/unordered edge identity, truncated packages and
trailing bytes, and observes a child non-zero exit. The transport target uses controlled
`1`, `2`, `3`, and `7` byte system-call chunks and covers header+payload counters, bad
magic/version/reserved bytes, wrong session/fingerprint/role/sequence, oversize length,
truncated header and payload, payload absolute-deadline timeout, EOF, and preservation of
receive sequence after a rejected header. The full M2 package/transport/process evidence is bounded local IPC
evidence, not a secure shuffle or network performance result.
