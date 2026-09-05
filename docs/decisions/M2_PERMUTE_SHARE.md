# M2.10 single-pass Permute+Share conformance

Status: `m2_emp_single_pass_permute_share_conformance` is a component
conformance result only. A is the Chase ideal PS functionality; B is limited
to local B1 layout/order; C is the VFSS/EMP/OpenSSL implementation boundary;
D remains two-pass shuffle, inverse routing, and complete Protocol I.

`apply(pi,x)[i]=x[pi[i]]`. Public Benes layout contains only N, T, D and wire
groups, where `D=2*(log2(N)/log2(T))-1`. Private decomposition remains with
the permutation owner. Counts are `D*N/T` translations, `D*N` OPVs and
`D*N*log2(T)` chosen OTs.

PO supplies only pi and local permutations; DO supplies only x and owns full
OPV views. Offline is one batched translation invocation. Online DO sends one
bounded framed payload containing m, inter-layer c values and fresh OpenSSL
random mask w. No values are opened. The only online causal round is one;
offline OT counters are reported separately. TEST_ONLY reconstruction checks
`share_po + share_do == apply(pi,x)`.

This is not a two-pass shuffle, inverse routing, secure Protocol I, or
`agarwal_protocol_i_exact`. LAN/WAN and performance results are `NOT_MEASURED`.
