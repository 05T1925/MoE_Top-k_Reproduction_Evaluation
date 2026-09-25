# M5-F Protocol III two-round process boundary decision

Status: accepted M5-F project process instantiation on
`feat/m5-protocol-iii-two-round`, HEAD `8f18354` plus uncommitted M5-F diff.

M5-E's algebra, comparison-key/field-share input boundary, selected-field-share
output, and two online messages remain unchanged. The process controller
starts independent `execv` P2/P0/P1 roles. P2 distributes one bound local
bundle to each party through offline sockets and exits; the controller releases
party-local online input shares only after `waitpid(P2)` succeeds and both
bundles have decoded. P0/P1 then exchange exactly one framed message per
party in R1 and R2. Test-only status and result sockets are not protocol
rounds and carry no value back into the online computation.

A bundle binds protocol/version/field, party, session, fingerprint, material
identity, logical/padded shape, `k`, target and widths. It uses canonical field
serialization and the existing same-build/same-architecture CmpAgg FSS package
serialization. Material identity uniqueness is enforced by the active
controller; no persistent restart-safe replay database exists. An online
party moves its material into one state and never retries after an exchange
failure. The M3 three-round baseline and Protocol I remain unchanged.

This is C-INSTANTIATION process framing and lifecycle, not an author-exact
transcript. The selected record remains field-shared; no secure conversion
from arbitrary ring payload shares or original-order bit-mask output adapter
is implied. Functional and causal evidence is in
[the M5-F process closeout](../reproduction/M5_PROTOCOL_III_TWO_ROUND_PROCESS_E2E_2026-09-25.md).
