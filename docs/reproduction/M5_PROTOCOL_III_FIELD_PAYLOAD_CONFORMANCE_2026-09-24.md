# M5-D Protocol III field payload conformance closeout

Verified 2026-09-25 on `feat/m5-protocol-iii-two-round`, base
`main@883da9e`; worktree changes remain uncommitted. Scope is field payload
primitives and field-valued DPF output compatibility. No two-round runtime,
Fsort, common-mask optimization, or Theorem 4.2 communication claim.

## Recovery and historical branches

The interrupted M5-D worktree contained only
`VFSS/include/moe_topk/protocol_iii_field.h` and
`VFSS/src/moe_topk/protocol_iii_field.cpp`: implemented, unregistered in
CMake, without a test. The existing build still listed 28 tests. M5-B and
M5-C worktree changes were preserved. The field arithmetic source was tested
first, then its unused constant was removed; all other M5-D code and tests
were added in this resumed session.

| Historical asset | Design intent | Current equivalent / M5-B/C fact | Paper support | Reuse decision |
| --- | --- | --- | --- | --- |
| `origin/M5.0.1-paper-evidence` | §4.2 equations and Theorem boundaries | Current paper reread confirms field, nonzero `s`, inverse beta; M5-C remains modular ring baseline | PAPER_DIRECT for equations; field choice unspecified | REUSE_AS_REFERENCE_ONLY |
| `origin/m5-protocol-iii-field-contract`: `GF(2^64)` | 8-byte field with XOR addition | Current partial M5-D already chose `F_(2^127−1)`; full uint64 payload + key needs more than 64 bits | C-INSTANTIATION | ALREADY_SUPERSEDED |
| Same branch: tagged priority key | Nonzero record binding original index | New `(key||uint64 payload)+1` with 1..62 key bits preserves wider payload; frozen stable priority key unchanged | C-INSTANTIATION | REUSE_WITH_ADAPTATION |
| Same branch: field/ring type separation | No reinterpretation of M3 shares | New distinct `ProtocolIIIField`; M5-C ring path unchanged | PAPER_DERIVED | REUSE_AS_REFERENCE_ONLY |
| Same branch: field DPF output group | Reuse tree, change leaf correction algebra | New Protocol III wrapper; native ring payload remains untouched | PAPER_DERIVED and C-INSTANTIATION | REUSE_WITH_ADAPTATION |
| Same branch: dealer shares, Beaver masks, binding | P0/P1 each get party-local material | New field multiplication and DPF material have party/session/fingerprint/slot; controller integration deferred | PAPER_DERIVED and C-INSTANTIATION | REUSE_WITH_ADAPTATION |
| `origin/docs/m5-protocol-iii-2round-plan` | Field/nonzero/DPF gate before runtime | This M5-D provides those primitives only | Project plan | REUSE_AS_REFERENCE_ONLY |
| `origin/design/m5-protocol-iii-exact-2round` | Future separate two-round path | Field type and payload output needed; old padded-entry and M4-gate statements are superseded by current main/project scope | Project design draft | REUSE_AS_REFERENCE_ONLY |

Historical branches supplied no field runtime code or tests to port. The old
`GF(2^64)` polynomial irreducibility claim was not independently rechecked;
it is not used. The historical M4 predecessor gate is superseded by current
`PROJECT.md`. No historical branch was switched to or cherry-picked.

## Paper and ABI boundary

CCS 2024 PDF §4.2 says modular DPF routing evaluates
`f_(r_i,1)(ŷ_i−target)` then securely multiplies indicator and payload. The
compressed construction requires field `H`, nonzero encoded payloads, random
`0≠s_i∈H`, public `z̃_i=z_i s_i`, and field-output DPF with `β_i=s_i⁻¹`.
Theorem 4.2 assumes `H` encodes key and payload but does not specify a field,
packing, byte order or VFSS adapter. These are **C-INSTANTIATION**, never
PAPER_DIRECT. The final two-round composition remains unimplemented.

Native VFSS `GroupElement=uint64_t`; native DPF `keyGenDPF` correction and
`evalDPF_Payload` use wraparound arithmetic then `mod(out,bout)`. Output group
is `Z_(2^bout)`. The conformance test reconstructs native ring output and
shows that independent conversion of its two shares into `F_p` does not in
general reconstruct the same function. `DPF_FIELD_COMPATIBILITY` is
`NEW_WRAPPER_REQUIRED`; `CORE_FSS_CHANGE_REQUIRED=NO`.

## Implemented field layer

- `H=F_(2^127−1)`; storage `unsigned __int128`; canonical 16-byte big-endian.
- Exact four-limb multiplication with Mersenne reduction; `inv(0)` rejects.
- Plain uint64 payload `u` → `u+1`, including zero and `UINT64_MAX`.
- Optional packed `key||payload` record, key width 1..62, no truncation;
  192-bit Protocol I block rejected as a single field element.
- Dealer rejection-samples uniform nonzero `s`; P0/P1 receive additive shares.
- Dealer field Beaver triple supports shared `z·s`: public `d=z−a`, `e=s−b`,
  local product shares `c_p+d·b_p+e·a_p+[p=0]d·e`. Product opening produces
  public `z̃`; the test does not inject a clear product into the party path.
- Field DPF reuses the native tree and computes a new field leaf/correction.
  Reconstructed output is `β` at `alpha`, `0` elsewhere, with field addition.
  Per-party key wire includes tag/version, party, domain bits,
  session/fingerprint/slot, canonical correction, correction bits and seeds.
- Beaver material wire is fixed width with tag/version, metadata and three
  canonical field shares. Truncation, oversize, wrong tag and noncanonical
  field encodings reject. Started/consumed material cannot serialize as fresh.

The field-selected value remains secret shared. Field payload input shares
used by the primitive conformance tests are generated in a test controller;
a secure conversion from existing ring/raw-score shares is **not** yet a
production API. A full P2/P0/P1 field two-round process path is deferred.
The old M3/M5-C independent process path is unchanged.

## Tests and results

Environment: GCC 11.4, CMake Debug build `build-vfss-debug`; fixed seeds.

| Command / test | Result |
| --- | --- |
| `ctest --test-dir build-vfss-debug --output-on-failure -R 'moe_topk_m5d_'` | 4/4 PASS |
| Field arithmetic | 5,000 independent double-and-add multiplication oracle cases; 256 inverse cases; boundaries, zero inverse, canonical serialization PASS |
| Field payload | 3,000 randomized uint64 and packed record roundtrips; zero/max, mask/inverse, invalid widths PASS |
| Field multiplication | 2,000 randomized share-preserving Beaver products; replay/binding/malformed wire PASS |
| Field DPF | Native ring gap, beta 0/1/2/p−1 and random, alpha boundaries and 128 random points, 63-bit domain boundary, key roundtrip/malformed wire, shared key/payload selection for all five ranks at n=5 (including selected zero and UINT64_MAX) PASS |
| M3/M5-B/C and M5-D focused CTest | 17/17 PASS |
| `ctest --test-dir build-vfss-debug --output-on-failure` | 32/32 PASS, including the previous 28/28, historical M3, independent-process M3 and Protocol I |
| `ctest --test-dir /tmp/m5d-vfss-sanitized --output-on-failure -R 'moe_topk_m5d_'` | ASan/UBSan 4/4 PASS |

No performance or communication results were measured for the new field path:
`NOT_MEASURED`. `rank` still uses `Z_(2^rank_bits)` rather than paper `Z_n`
for non-power-of-two `n`: explicit C-INSTANTIATION. M5-E must bind the new
field material to an independent two-round controller and transport,
validate the secure input/output adapters, and audit the causal transcript.
