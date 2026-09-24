# M5-C Protocol III modular DPF routing closeout (2026-09-24)

## Scope and evidence

This records the existing three-round priority-key Protocol III baseline, extended
with M5-C validation. It does not claim the two-round or field-valued
construction of Theorem 4.2. Evidence labels: **PAPER_DIRECT** is stated by
Agarwal et al., CCS 2024; **PAPER_DERIVED** follows from its DPF definition;
**C-INSTANTIATION** is a project choice; **D-UNRESOLVED** is not established by
the conference paper or this implementation.

The current functionality is an original-input-order, XOR-shared Top-K
membership bit-mask of length logical_n. It is a project specialization using
unit payloads, not the paper's general field-valued Fselect output or Fsort.

## Actual modular pipeline

| Stage | Function / type | Domain and visibility | Online exchange |
| --- | --- | --- | --- |
| Ranking | \`protocol_iii_grank_party\` → \`ProtocolIIIGrankOutput::rank_additive_shares\` | logical_n original-order party shares in Z_(2^rank_bits); never opened | R1: each party's masked comparison-key vector |
| Routing | \`protocol_iii_dpf_routing_party\` → \`ProtocolIIIDpfRoutingOutput::indicator_shares\` | logical_n × k row-major party shares in Z_(2^64) | R2: each party's masked-rank shares; the sum is public |
| Combine | \`protocol_iii_secure_combine_party\` → \`ProtocolIIISecureCombineOutput::xor_mask_shares\` | logical_n original-order XOR bit shares | R3: two masked multiplication openings per matrix cell |
| Controller | \`protocol_iii_secure_core_party\` → \`ProtocolIIISecureCoreOutput\` | validates common session, fingerprint, party, n, k, widths and 3 distinct descriptors | no additional protocol round |

R1 ranking calls the existing \`protocol_i_cmpagg_eval_party\`, with the
existing one-shot uCMP/DCF edge material. No Protocol III copy of CmpAgg exists.
Only logical nodes enter ranking/routing; GRank accepts padded priority-key
input shares but emits exactly logical_n rank shares. Stable rank 0 is highest
priority: signed score descending, original index ascending.

## Mask, point function and combine equations

For item i and party p, let q_i,p be the rank share and rho_i,p the routing
mask share in Z_(2^b), where b=rank_bits. R2 sends
m_i,p=(q_i,p+rho_i,p) mod 2^b. Parties reconstruct only
m_i=(rank_i+rho_i) mod 2^b. Dealer made one standard DPF key pair per logical
item with alpha_i=rho_i and beta_i=1 in Z_(2^64).
For target t in [0,k), each party evaluates its own key at
x_i,t=(m_i-t) mod 2^b. By the DPF point property, the two indicator shares
sum to 1 iff rank_i=t, and 0 otherwise. This is **PAPER_DERIVED** for the
project's power-of-two rank domain.

Dealer additive-shares u_i=1 in Z_(2^64). For each (i,t), fresh
\`MaskedMulMaterial\` produces party shares of indicator_i,t × u_i in the
64-bit ring. Parties sum these shares over t<k, then take each local low bit.
Parity of a ring sum equals XOR of its low-bit shares. Honest execution has
one or zero selected targets per item, so reconstruction equals
1 iff rank_i<k. This is **C-INSTANTIATION**: ring multiplication and a
unit-payload mask specialization, not field multiplication.

## Offline material and lifecycle

| Material | Dealer creation and party visibility | Consumption |
| --- | --- | --- |
| GRank comparison masks, clique uCMP/DCF edges | one share/key per party, bound to session/fingerprint/party/shape/width in \`ProtocolIPartyPackage\` | moved/cleared before R1 I/O; failure after starting R1 cannot retry |
| Routing rho_i shares and DPF keys | one mask share and one key per logical item per party; DPF alpha is full rho_i, beta=1 | \`started\` set before R2 frame attempt; retry rejected after success or partial failure; vectors cleared on success |
| Unit payload shares | one additive share of 1 per item | consumed as input to R3, not itself a one-shot key |
| Multiplication keys/output mask shares | one \`MaskedMulMaterial\` per (item,target) per party | state Fresh→OpenSharePrepared→Consumed; retry after partial R3 exchange is rejected; vector cleared on success |

Each party receives only its own package, DPF keys and multiplication keys.
The full DPF point and both key halves exist only during trusted P2 offline
generation. Local material metadata and all three online framed messages bind
session/fingerprint/party/shape. Persistent uniqueness of session IDs and
offline bundles across process restarts remains a controller responsibility;
there is no durable replay database (**C-INSTANTIATION**).

## Serialization and malformed input

Online frames use \`ProtocolIFramedChannel\`: 48-byte versioned header with
session, fingerprint, sender/receiver, phase/type, n, k and sequence; payload
words are big-endian 64-bit values. The transport conformance test rejects
bad magic/version/reserved bytes/session/fingerprint/sender/sequence/length,
truncated frames and EOF. R1/R2/R3 validate exact payload word counts.

Offline bundles carry version, party, widths, session, fingerprint, n,
padded_n, k and a serialized GRank package. The existing FSS key tail uses
native \`Peer\`/\`Dealer\` ABI. M5-C added a common exact tail-size validator
before every unbounded FSS \`MemBuf\` decode in both priority-key and raw-score
executables and their process harnesses. The priority-key process test passes
roundtrip and rejects truncated/extra tail, wrong version/party/session/width,
and truncated header. Stage APIs reject zero/out-of-range k, wrong rank/DPF
width, wrong payload width, wrong material count and cross-stage metadata.
The native FSS tail is **not a portable canonical byte format**; this is an
explicit homogeneous-host baseline boundary, not paper-exact serialization.
A corrupt native FSS sentinel may terminate the receiving process via the
upstream FSS assertion; it does not produce a valid protocol output.

## Process and causal audit

\`moe_topk_m3_protocol_iii_three_process_e2e_test\` forks and execs P2, P0
and P1 with distinct address spaces and socket endpoints. P2 generates and
sends party-specific material, exits, and the controller waits for its exit
before releasing online priority-key shares. The controller alone holds the
clear test oracle. The two online parties exchange only framed masked values;
raw ranks, indicators and products are never opened.

R1 outbound masked comparison keys are computed locally before peer input.
R2 outbound masked ranks depend on R1 local rank shares; they are computed
before the R2 peer read. R3 outbound masked indicator/payload multiplication
open shares depend on R2 local indicators and are computed before the R3 peer
read. P0 sends then receives and P1 receives then sends to avoid socket
deadlock; P1's outbound value is already determined. Thus the modular
priority-key core has **three causal online rounds**. A process-lifetime
acknowledgement on the test controller channel follows the protocol and is
excluded from protocol round and byte metrics.

Process cases: n=1,2,3,4,5,7,8,127,128,129,256, including duplicate and
boundary scores, non-power-of-two n, k=1, middle k and k=n. The M5-C
in-process test uses the M5-B deterministic case generator and clear oracle:
112 full-chain cases across n=2,3,4,5,8; each executes real priority-key
shares→uCMP/DCF CmpAgg→DPF routing→secure combine→final XOR mask.
It additionally tests cross-stage metadata rejection and one-shot behavior
after a failed exchange. The old M3 process paths and raw-score adapter remain
independently tested.

## Modular baseline communication

These are baseline costs, not Theorem 4.2 costs. Across P0 and P1, with
b_cmp=comparison_bits, b_rank=rank_bits:

| Round | Semantic logical bits | Current 64-bit-word payload bits | Current wire bytes |
| --- | ---: | ---: | ---: |
| R1 GRank | 2n × b_cmp | 128n | 2(48+8n) |
| R2 routing | 2n × b_rank | 128n | 2(48+8n) |
| R3 combine | 4nk × 64 | 256nk | 2(48+16nk) |
| Total | 2n(b_cmp+b_rank)+256nk | 256n+256nk | 288+32n+32nk |

For the measured process case n=5, k=3 (b_cmp=36, b_rank=3):
semantic logical online communication is 4,230 bits; implementation payload
is 5,120 bits; measured wire is R1 176 + R2 176 + R3 576 = **928 bytes**.
The measured P2→P0/P1 offline bundle total is **21,384 bytes** (including
the two 8-byte transport lengths). Local work counts remain separate:
n(n−1)/2 CmpAgg edges, 2 DCF calls per edge per party, n×k DPF evaluations
per party and n×k multiplication calls per party. No Theorem 4.2
communication match is claimed.

## Results and remaining boundary

Debug build: M5-B 112 cases PASS; M5-C 112 cases PASS; independent
priority-key process matrix 14/14 PASS; historical M3 subset 11/11 PASS;
full CTest 28/28 PASS, including Protocol I. No Protocol I runtime or
\`VFSS-baseline/\` change. An existing sanitizer build was unavailable.

**C-INSTANTIATION:** rank shares use Z_(2^rank_bits), not paper Z_n for
n=3,5 and other non-powers of two; honest reconstructed ranks stay in [0,n)
and routing never treats padding-domain points as target ranks. Offline
native-key encoding and the unit-payload ring combine are project choices.
Field-valued payload algebra belongs to M5-D; two-round cross-stage
compression belongs to M5-E/F; Fsort and common-mask work remain deferred.
