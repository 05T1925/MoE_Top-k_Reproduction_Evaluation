# M2.11 two-pass secret-shared shuffle roundtrip

Implementation label: `m2_emp_two_pass_shuffle_roundtrip_conformance`.

Four distinct offline material IDs drive forward pass 1/2 and reverse pass
1/2. Each material is move-only, binds its full PS configuration (session,
fingerprint, offline material ID, online message ID, N, T, owner and timeout),
and is consumed by one online call. PO material also retains the exact owner
permutation. No offline OPV/OT is invoked from an online API.

Forward evaluates `PS(P0,pi0,P1,x1)` followed by `PS(P1,pi1,P0,b0)`; reverse
uses freshly preprocessed inverse permutations in the opposite owner order.
TEST_ONLY checks forward composition and reverse roundtrip for all three words
of every record. The CTest controller fork/execs one P0 and one P1 per case;
the role processes synchronize at a barrier before generating their TEST_ONLY
shares. Carrier tests use arithmetic 0/1 in word0 and local LSB-to-XOR
conversion only; it adds no communication or round.

Evidence: Chase functionality is A; B1 order/layout is B; EMP/VFSS framing and
counting are C. Two-pass composition is a project conformance component, not
`secure_shuffle_complete`, complete Protocol I, rank reveal, or Top-K mask.
Padding for non-power-of-two N is not implemented. LAN/WAN and performance are
`NOT_MEASURED`.

M2.12 now consumes this composition through a priority-key-input E2E. That
changes neither the evidence level nor its non-power-of-two limitation.
