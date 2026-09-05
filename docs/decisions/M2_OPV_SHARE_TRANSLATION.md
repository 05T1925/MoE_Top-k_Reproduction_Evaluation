# M2.9 EMP OPV and Share Translation conformance

Status: **implemented and validated as
`m2_emp_opv_share_translation_conformance` (A/B/C; no shuffle claim).**

## Evidence boundary

| layer | evidence and use |
| --- | --- |
| A | Chase's OPV/Share Translation functionality and static semi-honest model in `协议1shuffle.pdf`; the resulting role separation and fresh-material requirement are retained. |
| B | `real_opv_internal.cpp` supplies only the GGM tree layout, MSB-first path handling, batched layer order, and its hard-failure style. |
| C | The EMP IKNP adapter supplies the connected-fd framing, counters, bounded deadline, stage binding, OpenSSL SHA-256 expansion, and explicit implementation limits. |
| D | Permute+Share composition, both shuffle passes, inverse routing, complete shuffle security, paper round equivalence, and Protocol I Top-K remain unimplemented. |

## OPV contract

FVO generates one fresh 128-bit root for every instance, derives a full binary
GGM tree, and owns all `T` 192-bit leaves.  PO supplies only one puncture index
per instance and receives every other leaf as `optional`; the puncture is
always `nullopt`. `T >= 2` is a power of two, depth is `log2(T)`, and a batch
uses exactly `batch_count * depth` chosen-OT items.

Child and leaf expansion use disjoint SHA-256 domains
`M2-OPV-CHILD-v1` and `M2-OPV-LEAF-v1`.  This is a C engineering choice using
the approved OpenSSL dependency; it is not a claim that a local B1 helper is
paper code.  At each level FVO sends `slot 0 = XOR(right children)` and
`slot 1 = XOR(left children)`.  An MSB-first puncture bit of zero therefore
obtains the right sibling, and a bit of one obtains the left sibling.  PO
expands known sibling subtrees and cancels their matching contribution from
the next level XOR.

The OT preamble has an explicit OPV stage identity (`OPV1`), distinct from the
standalone IKNP identity.  It binds role, session, fingerprint, material ID,
item count and stage before EMP runs.  No values are opened.  Counters cover
EMP traffic only (the preamble is excluded); causal-round counters are carried
from the bounded EMP channel.

## Share Translation contract

One translation of length `T` invokes one batched OPV with `batch_count=T`
and `puncture[i]=pi[i]`.  FVO locally derives

```text
b[i] = sum_j V[i][j]       a[j] = sum_i V[i][j]
```

and PO locally derives

```text
row_i = sum_(j != pi[i]) V[i][j]
col_i = sum_(q != i) V[q][pi[i]]
delta[i] = row_i - col_i = b[i] - a[pi[i]].
```

Each of the three 64-bit words is added/subtracted modulo `2^64`.  Production
has no helper that jointly accepts FVO and PO output.  Only the fork/exec
TEST_ONLY controller receives both outputs and checks this equation.

## Conformance scope and non-claims

Roles run as independent `--role=fvo`/`--role=po` processes over a controller
created `socketpair`; the controller gives FVO only public configuration and
PO configuration plus punctures/permutation.  Results return on a separate
TEST_ONLY channel.  Matrices cover OPV `T=2,4,8,16,32`, batches `1,3,T`, and
edge/interior/generated punctures.  Translation covers `T=2,4,8,16` with
identity, reverse, cycle, pair-swap, and fixed-seed permutations.

This does **not** implement `permute_share_complete`, `secure_shuffle_complete`,
`agarwal_protocol_i_exact`, or `paper_3_round_exact`.  LAN/WAN performance is
`NOT_MEASURED`.

M2.10 consumes the component through batched local permutations and rejects
every unexpected missing leaf before row/column aggregation; see
`M2_PERMUTE_SHARE.md`.
