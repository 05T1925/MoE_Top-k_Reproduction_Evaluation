# S14 DCF theorem bridge: `DCF_BCG21_FIG1_AES128_V1`

Date: 2026-10-15  
Code identity: S14 source manifest, based on `c3926c68fd14f270faa8b55234311071947fa080`; not merged.  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## Evidence categories

- **Paper definition:** Boyle et al., *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*, official full version. Figure 1 and Theorem 2 are on printed pp. 12–13; the DCF security hybrids are Appendix F.1, printed pp. 42–45. The official PDF states that for `G=Z/(2^ell)` with `ell≤lambda`, `Convert_G` takes the first `ell` bits, and Theorem 2 assumes a PRG `G:{0,1}^lambda→{0,1}^{4lambda+2}`. [Microsoft Research paper PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf).
- **S14 code mapping:** the new implementation is a versioned instantiation of that Figure 1 DCF, not a change to the old `dcf.cpp` implementation.
- **Project extension:** AES-128 is used to instantiate the paper’s PRG interface, and the party keys are embedded in the Select5 material schemas described below.
- **Unproved composition:** the DCF theorem does not by itself prove the actual UCMP messages, full correlated package, adaptive Select transcript, or shuffle/output composition.

## Figure 1 / Theorem 2 mapping

Parameters in the conformance/runtime range:

| Item | S14 value | Mapping |
|---|---:|---|
| Security parameter `lambda` | 128 | 16-byte roots; AES-128 key |
| DCF input width `Bin` | 34–53 in DCF conformance; 34, 40, 41 in route material | MSB-first binary tree, unsigned input domain `[0,2^Bin)` |
| Output group | `Z/(2^64)` | additive `uint64_t` arithmetic modulo `2^64` |
| `ell` | 64 | `ell≤lambda`; terminal Convert is the first 64 seed bits, interpreted big-endian |
| DCF function | `f_{alpha,beta}(x)=beta` iff `x<alpha`, else 0 | strict `<`; UCMP separately applies two wrapped-domain evaluations and its signed correction |
| Initial control bits | `(t0,t1)=(0,1)` | Figure 1 initial complementary controls |
| PRG output | first 514 MSB-first bits | `4lambda+2=514` |
| correction word | 128-bit seed CW, 64-bit group CW, two direction control bits | the remaining six wire control-byte bits are canonical zero padding |
| final CW | one element of `Z/(2^64)` | Theorem key size includes the terminal group element |

For a width `w`, Theorem 2's per-party semantic key length is `w(128+64+2)+128+64 = 194w+192` bits. S14's `M2B1/v1` serialization is `57+25w` bytes: a fixed 33-byte ABI/count header, 16-byte root, `w` records of 16-byte seed CW + 8-byte group CW + 1 control byte, and an 8-byte final CW. This encoding is deterministic and injective for canonical keys. It is **not** a uniformly random byte string: the public header is fixed and six control padding bits per level are zero. The simulator for the key-only claim samples a semantic candidate key and applies the same canonical serializer; it must not claim wire-byte uniformity.

At each level, `G(seed)` is implemented as five AES-128 encryptions under the seed key over five distinct fixed blocks whose first seven bytes domain-separate `BCG21F1` and whose final byte is a counter 0–4. The 80 output bytes are truncated to 514 significant MSB-first bits. The `v_L`/`v_R` values are each parsed as 128-bit strings; `Convert_G` consumes their first 64 bits, matching the paper's conversion for a power-of-two group. The earlier S12 `s & notThreeBlock` terminal projection is absent from this ABI.

Generation draws two fresh 16-byte root seeds with `RAND_bytes`; failure throws and has no deterministic or legacy fallback. Runtime key imports require ABI id 1 and exact parameters. The legacy key generator/serializer remain in their previous `M2UC/v1` path.

## Conditional single-key bound

Let `epsilon_G(T_G)` be the PRG advantage for the S14 514-bit expander. For a PRP-secure AES-128 permutation, an explicit conservative bound for the five-block construction is

`epsilon_G(T_G) ≤ Adv_AES128^PRP(T_G, q=5) + 10/2^128`,

where `10/2^128` bounds the random-permutation versus independent-random-block collision gap for five distinct block inputs. Truncating to 514 bits is post-processing. This statement assumes AES-128 is a secure PRP for the stated time/query bound and the fixed counter inputs are distinct.

Appendix F.1 replaces one correction word at each of the `w` levels and has a final terminal-conversion hybrid. Here `ell≤lambda` and conversion is exact for uniform seeds, so that final term is zero. Thus the paper's key-only hybrid gives at most `w·epsilon_G` for ideal uniform roots. To transfer that result to the real implementation, define `epsilon_RNG_joint(T;X)` as the computational distinguishing advantage at time `T` between the actual joint random-output transcript for context `X` and the ideal transcript in which the same requested bits are mutually independent uniform bits. `X=slot` includes the two `RAND_bytes` roots (32 bytes total) and the associated `getrandom`-derived mask-share values visible in that party's slot package; this conditional assumption covers any dependence between the Linux `getrandom` and OpenSSL RNG interfaces. It is not a claim that the source proves a statistical-distance bound. The one-key implementation claim is therefore bounded by

`delta_w(T) ≤ w·epsilon_G(T') + epsilon_RNG_joint(T;slot)`.

`T'` is the theorem reduction's adversary time after its polynomial overhead. This is a conditional computational bound, not a concrete numerical security level. It is stated for each fixed `alpha∈{0,1}^w` and `beta∈Z/(2^64)`, and then for efficient deterministic post-processing such as the canonical key serializer and its SHA-256 digest. It does not establish security against a compromised/biased OS RNG, nor does it prove TLS, party-store, or complete protocol security.

The expander buffer copies 65 bytes, but the key parser consumes exactly 514 MSB-first bits: all of the first 64 bytes and the top two bits of the last byte. Four additional bits retained in that byte are unused and ignored; its low two bits are zeroed. This does not change the PRG input tuple or key semantics.

Two independent reviewers who did not implement the S14 DCF code compared the actual PRG input/output layout, control initialization, correction-word layout, terminal conversion, party serialization, and `eval` arithmetic with Figure 1/Theorem 2/Appendix F.1. They found no substantive mismatch in the semantic single-key mapping. The review is conditional on the stated AES-PRP and computational CSPRNG assumptions. It does not upgrade any composition gate below to PASS.

Review provenance: the read-only reviewer tasks were `/root/independent_dcf_review` and `/root/s12_receipt`; neither authored the S14 DCF implementation or bridge. The independently checked implementation source was `dcf_bcg21_fig1_aes128_v1.cpp`, SHA-256 `b2bc67cbe26bd11b2fded7a3f10002970947694e001cbf16f2bbbd45ae83982f`. The second review explicitly checked the revised joint CSPRNG pool term below. Reviewers ran no S14 tests; functional evidence remains separately identified as implementation-run.

The dealer first samples each full mask uniformly, then samples one uniform additive share and defines the other by subtraction. Thus either single party's local mask shares are independent of the full mask differences used as `alpha`; conditioning on those local shares leaves the theorem's fixed-`alpha` key claim applicable pointwise. This closes the local mask-share conditioning for the **key-only** hybrid. It does not cover opened `z` values, comparison bits, or the transcript/output composition.

## Two real material schemas

The two schemas are separate security objects. All counts are per party, unless noted.

| Schema | S14 version | DCF slots | Width split | Shuffle material | Storage evidence |
|---|---|---:|---|---|---|
| N=128 padded monolithic | `M6B5/v3` + `M2CM/v1` + `M6P2/v1` | 41,024 | 40,768 keys at `Bin=40`; 256 score keys at `Bin=34` | 1,280 metadata records; zero legacy `C(128,2)` shuffle DCF keys | measured E2E package 44,985,596 B/party; below 64 MiB cap |
| N=256 padded streamed | `M6S2/v2`, manifest ABI id 1 | 166,528 total records; 163,968 DCF keys | 163,456 keys at `Bin=41`; 512 score keys at `Bin=34` | 2,560 metadata records; zero legacy shuffle DCF keys | measured store 192,644,416 B/party, 12,656,192 B manifest/party, 11 segments; 385,288,832 B both parties total |

Slot formulas: Select `5*C(N,2)`; score conversion `2N`; membership `N`; shuffle metadata `10N`. Therefore N=128: `40,640+256+128=41,024` DCF keys and `1,280` metadata records. N=256: `163,200+512+256=163,968` DCF keys and `2,560` shuffle metadata records. The monolithic BCG package uses a metadata-only `M2CM/v1` encoding and its runtime rejects nonempty legacy forward/inverse edge-key pools. This prevents silently embedding a second unused `M2CP` full-edge pool. Legacy `M6B5/v2`/`M2CP/v1` remains distinct.

At N=256, source-derived offline work is 67,016,960 AES block calls for width-41 keys plus 174,080 for 512 width-34 keys, total 67,191,040 AES calls, and 327,936 fresh 16-byte root draws. These are source-derived operation counts, not wall-clock measurements. Online leases are exact-address, durable one-shot records. Segment rollover is file layout only; all slots are generated offline before Select starts. `M6S2/v2` manifest address/digest entries are deterministic post-processing of the canonical key representation.

## Composition gates and first missing hybrid

For either corrupted party `P_b`, the key-only candidate simulator can sample a fresh semantic key for each logical slot, serialize it canonically, compute the same local digest, and then produce the party-local slot/lease events. For N=128, the ideal-root pool hybrid is `Q40·40·epsilon_G + Q34·34·epsilon_G = 1,639,424·epsilon_G`; for N=256 it is `Q41·41·epsilon_G + Q34·34·epsilon_G = 6,719,104·epsilon_G`. Transfer from the ideal tape to the actual joint `getrandom`/`RAND_bytes` schedule adds one schema-wide term: `epsilon_RNG_joint(T;M6B5/v3,N128)` or `epsilon_RNG_joint(T;M6S2/v2,N256)`. The DCF-root parts of those schedules request 1,312,768 bytes for N=128 and 5,246,976 bytes for N=256, respectively; the same term also includes the associated `getrandom`-generated local mask-share and metadata randomness, including variable rejection-sampler draws. This joint term avoids multiplying a marginal per-slot claim without a composition assumption. These are symbolic reductions, not concrete numeric security levels. They assume trusted T and one-shot slots. The S14 runtime requests a fresh `keygen()` root pair per slot; no seed is reused across segments or addresses.

Adaptive selection of later slot addresses after already-approved public seeds/endpoints/comparison bits is deterministic post-processing of the public transcript and a pre-generated static pool. That observation alone does not simulate the whole view. Specifically, the first **unclosed** full-view hybrid is the step that replaces the actual joint distribution of (a party's mask-share package and shuffle state, the opened masked operands and comparison bit, the selected slot key/evaluation share, and later adaptive state) by independently simulated values conditioned only on approved leakage `L`, the ideal stable Top-K output, and that party's own input share. The FSS key theorem addresses the local DCF key for fixed `alpha,beta`; it does not by itself supply this UCMP-mask/opened-bit/shuffle-output conditional simulator.

No concrete single-party attack was found in the review. **The full view proof is not closed.** The next proof action is to state and prove one lemma for a single real UCMP invocation that jointly includes its local mask shares, BCG key, opened `z_left/z_right` and comparison bit, under the approved leakage and fixed ideal output. Then repeat that replacement across the exact slot counts above, including unused slots and lease/restart events, and compose with the actual forward/inverse shuffle and output-share state. Until that proof is reviewed independently, `CORRELATED_POOL_SIMULATION`, `ADAPTIVE_QUERY_SIMULATION`, `SHUFFLE_OUTPUT_COMPOSITION`, and `JOINT_PARTY_VIEW` remain OPEN; no secure alias.
