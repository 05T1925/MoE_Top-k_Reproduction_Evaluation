# M6B-I PROJECT_DERIVED_SELECT_5ITER S11 decision

Date: 2026-10-15  
Decision: retain `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`; do not create a `secure` alias.

## Bound source and inherited evidence

S11 reviews the 58-file S8 snapshot received in S10, identified by S8 manifest SHA-256 `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb` and S11 canonical source digest `17f1827077bc3f9dbf6038688bbda466162c4fe53e6436d34abb09703725f957`. S8 was based on `c3926c68fd14f270faa8b55234311071947fa080` and was not merged to main. S10, not S11, is the independent functional receipt: it rebuilt Release, ran seven directed targets, checked default-OFF registration, and ran 24 T/P0/P1 cases (18 oracle-checked success, 6 abort/no-mask). S11 performed source/theorem review only; it ran no build or tests and changed no runtime code.

S10 counter erratum remains authoritative for corrected S8 comparison counts. Neither S8/S10 reports nor their raw case records are rewritten.

## DCF bridge decision

The current `keyGenDCF/evalDCF` source maps to the optimized DCF construction in Boyle et al., EUROCRYPT 2021, Definition 2, Definition 3, Figure 1, Theorem 2, and Appendix F.1. The bridge is conditional on the exact restricted-key fixed-counter AES expansion being a PRG, the dealer AES-counter stream, OS CSPRNG, and authenticated-channel assumptions. S8's randomized complementary initial control bits require the additional derivation documented in the S11 report; that extension has not received independent cryptographic review.

Consequently:

- single-key privacy: `CONDITIONAL_BRIDGE_ESTABLISHED`;
- correlated full-pool key simulation: `CONDITIONAL_PASS_DCF_KEY_LAYER` for each package schema separately;
- adaptive DCF address simulation: `CONDITIONAL_PASS_DCF_KEY_LAYER` under public current-round seed/transcript and one-shot no-rollback assumptions;
- full shuffle/output composition and whole P0/P1 view: `OPEN`.

The N=128 `M6B5/v2` monolithic pool and N=256 `M6S2` streamed pool are distinct security objects and must remain separately analyzed. The concrete pool inventory and symbolic hybrid bound are in the S11 report.

## Whole-view blockers

1. The approved leakage function has not explicitly enumerated all observed forward/inverse 24-byte record arrays, per-party package/ready digests, and lease records emitted by the runtime.
2. The N=128 staging interface places both party packages and both hashes under one directory/UID boundary. A same-UID P0 process can open the P1 package; deployment isolation is not enforced by the current API.
3. Output-share publication is not atomic across P0/P1 crashes after final-status exchange.

These are concrete interface/deployment proof gaps. No DCF distinguisher was found; that does not close the joint-view proof. **未发现反例，但证明未闭合。** Replacing DCF would not address these blockers, so S11 does not introduce a second DCF ABI.

## Required next work

Create a versioned party-scoped package delivery contract, enumerate the exact fields in the approved leakage function without exposing prohibited values, and define an atomic output/no-output release protocol. Obtain independent review of the restricted-key PRG mapping and randomized initial-control-bit extension. Preserve one-shot leases and fail-closed restart behavior. Only then reevaluate the full simulators.

## Milestone boundaries

- S8 functional evidence is independently received by S10; S11 is not a new receipt.
- Route identity remains project-derived, not BB90 or BMW16.
- `N>256` remains `NO_GO`.
- M6A V3 remains `OPEN`; V4 and formal LAN/WAN performance remain `NOT_ALLOWED/NOT_RUN` under the existing V3-to-V4 prerequisite.
- No secure alias, runtime change, formal performance run, or change to `VFSS-baseline/` was made.

## Primary evidence

- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S11_REPORT_2026-10-15.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S11_SOURCE_MANIFEST_2026-10-15.json`
- S10 report, S8 manifest, and counter erratum, with hashes recorded in the S11 manifest.
- Boyle et al., “Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation,” EUROCRYPT 2021, official full version: https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf
