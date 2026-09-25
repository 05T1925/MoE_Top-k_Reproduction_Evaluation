# M5-D Protocol III field payload decision

Status: M5-D project instantiation, validated 2026-09-25 on
`feat/m5-protocol-iii-two-round` based on `main@883da9e`. This decision does
not assign Theorem 4.2 or author-exact status to a runtime.

## Evidence and decision

- **PAPER_DIRECT** (CCS 2024, §4.2 and Theorem 4.2): compressed DPF routing
  uses a field `H`, nonzero plaintext payloads, uniform `s_i ∈ H*`, public
  `z̃_i=z_i·s_i`, and DPF output payload `β_i=s_i⁻¹`. The theorem assumes `H`
  can encode both key and payload. The paper's modular routing uses DPF
  output `1` and Beaver multiplication; it is a separate three-round
  composition with GRank.
- **PAPER_DERIVED**: for exactly one matching rank, local field aggregation
  of `z̃_i · DPF_i(ŷ_i-target)` yields additive shares of the selected encoded
  payload. For zero plaintext payload, a nonzero injective encoding is needed
  before publishing a multiplicatively masked value.
- **C-INSTANTIATION**: `H=F_(2^127−1)`, 128-bit storage, 16-byte big-endian
  canonical encoding, uint64 payload offset `+1`, optional `(key||payload)+1`
  packing, field Beaver material and Protocol-III-specific field DPF output
  correction. The conference paper fixes none of these engineering choices.
- **D-UNRESOLVED**: the conference version refers to a full version for the
  complete construction; no claim of author-exact field representation or
  exact security proof for this specific field DPF wrapper is made. The
  two-round schedule, secure ring-to-field input adapter, selected-record to
  original-order mask adapter, process E2E and cost match remain later work.

## Chosen algebra and bounds

`p = 2^127−1 = 170141183460469231731687303715884105727` is a Mersenne prime.
Every canonical element satisfies `0 <= x < p`; every nonzero element has an
inverse. Addition/subtraction are modulo `p`. Multiplication uses four exact
64-bit product limbs, `unsigned __int128` partial products, and the identity
`2^127 = 1 (mod p)` for reduction. Inversion uses exponent `p−2`, rejecting
zero. The toolchain is GCC 11.4; portability to compilers without
`unsigned __int128` is not claimed. Wire bytes are fixed-width big-endian,
with `x>=p`, wrong length and malformed material rejected, never reduced on
decode.

A standalone payload `u ∈ [0,2^64−1]` encodes as `u+1`; even `u=0` becomes
nonzero and `u=2^64−1` remains representable. Decode accepts only
`[1,2^64]`. An optional key/payload record packs as
`((uint128)key<<64 | payload)+1` with declared key width `1..62`; overflow or
out-of-width key is rejected. The current M5-B/C test sizes fit; a future
runtime must reject shapes requiring more than 62 key bits or use multiple
field elements. `ProtocolIBlock192` is not a field element and is not packed here. The
field-selected payload remains secret shared; plaintext decode is test-only
until a secure output adapter exists.

Alternatives considered: `Z_(2^64)` is not a field; a 61-bit prime cannot
hold a full uint64 payload; historical `GF(2^64)` is a valid candidate in
principle, but its single element cannot pack a full uint64 payload together
with a project priority key. The historical draft's specific polynomial and
irreducibility assertion were not revalidated or adopted.

## DPF and party boundary

Native `GroupElement` and `evalDPF_Payload` operate in `Z_(2^bout)`. Their
party outputs cannot be reduced separately modulo `p` to obtain field shares:
a pair summing to zero modulo `2^64` may sum to `2^64` in `F_p`. The new
Protocol III wrapper reuses native DPF tree keys and traversal and recomputes
one leaf correction in `F_p`. It never passes a field payload through native
`GroupElement` and does not modify `VFSS/ext/FSS/` or Protocol I.

At the designated point, native control bits differ by `+1` or `−1`; the
field correction makes the reconstructed leaf equal arbitrary `β∈F_p`.
Off-point the tree seeds/control bits agree and the field shares cancel.
A separate AES-based leaf expansion with rejection supplies a canonical full
field value. This is a project DPF output-group instantiation; its
cryptographic/security and constant-time review remain separate from
functional conformance.

P2 generates `s`, `β=s⁻¹`, DPF tree/correction, field Beaver triple and
additive shares. P0 receives only `[s]_0`, its DPF key and its triple shares;
P1 receives only the matching party-1 material. Neither party material has
full `s` or scalar `s⁻¹`. Public masked operands are `d=z−a`, `e=s−b`;
the masked product is `z̃=z·s`. Each field multiplication material is one-shot,
with fresh → started → consumed lifecycle. DPF evaluation may be repeated
locally within one future session (as needed for multiple target ranks), but
cross-session reuse of its associated rank/payload masks is forbidden.
Session, fingerprint, slot and party are encoded in the field DPF and
multiplication material and checked before use. A full M5-E controller and
transport frame will enforce the cross-stage one-shot bundle lifecycle.

The rank group remains `Z_(2^rank_bits)`, including the non-power-of-two
`Z4`/`Z8` embeddings for `n=3`/`n=5`; it is distinct from payload field `H`.
The existing three-round ring-based original-order Top-K mask path remains the
frozen M3/M5-C baseline.
