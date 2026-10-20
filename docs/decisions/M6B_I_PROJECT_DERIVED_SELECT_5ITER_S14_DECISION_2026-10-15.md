# M6B-I PROJECT_DERIVED_SELECT_5ITER S14 decision

Date: 2026-10-15  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## Decision

1. The exact S12 source snapshot is independently received by a receiver who did not participate in S12 or S4–S12 implementation/route decisions. This receipt covers the 58 paths and hashes in the S12 manifest, not S14 changes.
2. Add `DCF_BCG21_FIG1_AES128_V1` behind a distinct DCF ABI and key encoding. Preserve the legacy `M2UC/v1` DCF behavior and key serialization. New keys are one-shot and bound by the existing session/party/purpose/endpoint address.
3. Use outer monolithic `M6B5/v3` with BCG-only `M2CM/v1` metadata for N≤128. Keep `M6B5/v2` and `M2CP/v1` for the legacy DCF route. Use streamed `M6S2/v2` for BCG keys through N=256. BCG Select5 material must not contain the legacy shuffle edge-key pool.
4. The BCG+21 Figure 1/Theorem 2 mapping for one semantic DCF party key is **conditional**: two reviewers who did not implement the S14 DCF code independently checked the source mapping. The explicit AES-PRP and joint OpenSSL `RAND_bytes`/Linux `getrandom` pseudorandomness assumption, including the party-visible mask-share auxiliary view, is defined in the S14 bridge report. This is not a proof of the actual UCMP masked-operand transcript or complete party view.
5. Static key-pool replacement is **conditional for the key-only projection** under the per-key theorem and one joint CSPRNG stream assumption covering each schema's ordered root-byte outputs. Adaptive-query and shuffle/inverse/output composition remain OPEN; no `secure` alias is allowed.
6. S14 functionally verifies the BCG ABI and selected N≤256 cases with the frozen oracle. S14 implementation/replay is not an independent receipt of S14. A receiver who did not author S14 runtime must review its final source manifest and replay commands before this candidate is considered independently accepted.
7. Preserve the route as project-derived. It does not replace the historical BB90 item or borrow BB90/BMW16 algorithmic, cost, probability, or security claims.
8. Do not run or publish formal LAN/WAN performance. M6A V3 remains OPEN; V4 remains NOT_ALLOWED/NOT_RUN under the existing V3→V4 ordering.

## Version identities

| Layer | S14 identifier |
|---|---|
| Select | `PROJECT_DERIVED_SELECT_5ITER` |
| DCF | `DCF_BCG21_FIG1_AES128_V1`, serialized key `M2B1/v1` |
| N≤128 package | `M6B5/v3`, BCG shuffle metadata `M2CM/v1`, recipient ready marker `M6P2/v1` |
| N=129..256 package store | `M6S2/v2`, BCG ABI id 1, segmented one-shot slot records |
| Deployment status | opt-in `PROJECT_DERIVED/EXPERIMENTAL`; no secure alias |

The exact changed-file list, build identity, case evidence, and unresolved security steps are in the S14 report and machine-readable source manifest.
