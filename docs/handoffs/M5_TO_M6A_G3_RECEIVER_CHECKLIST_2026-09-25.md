# M5 → M6A G3 Receiver Checklist

## 1. Authoritative baseline

Repository:

`https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation`

M5 implementation was merged by PR #25.

M5 implementation merge commit:

`2db7dbf`

The receiver must perform G3 from a clean checkout of the current `main`
that contains this commit, and must record the exact `main` commit actually reviewed.

Do not use:

- developer-local patches
- developer build directories
- uncommitted developer worktrees
- the old M5 feature branch as the M6 development base

---

## 2. Read these documents first

Read in this order:

1. `docs/handoffs/M5_PROTOCOL_III_TO_M6A_HANDOFF_2026-09-25.md`
2. `docs/reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md`
3. `docs/decisions/M5_FIX_F1_SECURE_IO_ADAPTER_DECISION_2026-09-25.md`
4. `docs/reproduction/M5_FIX_F1_SECURE_RAW_SCORE_TO_MASK_E2E_2026-09-25.md`
5. `docs/reproduction/M5_PROTOCOL_III_2ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-25.md`
6. `docs/handoffs/M6A_NEW_CHAT_PROMPT_2026-09-25.md`

---

## 3. Protocol III M6 entrypoint

Production API:

`protocol_iii_raw_score_mask_party`

Header:

`VFSS/include/moe_topk/protocol_iii_raw_score_mask.h`

Input contract:

- P0/P1 hold additive shares in `Z_(2^32)`
- scores use signed 32-bit two's-complement Q20.12
- public `logical_n`
- public `K`
- valid range: `n >= 2`, `1 <= K <= n`

Output contract:

- each party receives `std::vector<std::uint8_t>`
- output length is `logical_n`
- the two vectors are XOR shares of the original-input-order Top-K bit mask

After reconstruction:

`mask[i] = 1`

iff original item `i` belongs to stable Top-K.

Stable priority semantics:

1. score descending
2. original index ascending

---

## 4. Important round boundary

The complete standard raw-score → Top-K-mask pipeline is:

- secure input adapter: 2 online rounds
- Protocol III mask GRank/routing core: 2 online rounds
- output adapter: 0 additional online rounds

Therefore:

`FULL_PIPELINE_ONLINE_ROUNDS = 4`

The separately frozen generic field-valued Protocol III Fselect/Fsort core remains:

`CORE_ONLINE_ROUNDS = 2`

Do not describe the complete raw-score → mask pipeline as a two-round protocol.

---

## 5. Security boundary

The standard mask entrypoint must satisfy:

- `PLAINTEXT_RECONSTRUCTION_BRIDGE = NO`
- `P2_ONLINE_SILENT = YES`
- `RAW_SCORE_PUBLIC = NO`
- `RAW_RANK_PUBLIC = NO`
- `RAW_MASK_PUBLIC = NO`

The controller may create test inputs and cleartext oracle results only for testing.
It must not participate in secure protocol computation.

---

## 6. Clean receiver checkout

The receiver should begin from a clean checkout:

```bash
git clone https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation.git
cd MoE_Top-k_Reproduction_Evaluation

git switch main
git pull --ff-only origin main

git status -sb
git log -5 --oneline

Record:

G3_REVIEWED_MAIN_COMMIT

The working tree must be clean before build and test.

7. Build and reproduction source

Use the commands frozen in:

docs/reproduction/M5_FIX_F1_SECURE_RAW_SCORE_TO_MASK_E2E_2026-09-25.md

Do not request a developer-local patch.

Do not reuse the developer's existing build directory as G3 evidence.

8. Full regression

Run:

ctest --test-dir build-vfss-debug --output-on-failure

Baseline at F1 handoff:

41/41 PASS

If additional tests have been added on main, the total may increase,
but all original M5 regression paths must continue to pass.

Record:

G3_FULL_REGRESSION_RESULT

9. Mandatory F1 receiver verification

Independently verify the standard path:

signed Q20.12 secret shares
→ secure input adapter
→ shared CmpAgg / GRank
→ Protocol III rank routing
→ original-order XOR Top-K mask shares

At minimum test:

n=2
n=3
n=5
n=8

and:

K=1
K=n
middle K

Score cases:

ascending
descending
all equal
mixed duplicates
negative values
zero
positive values
supported boundary values
deterministic random inputs

Verify:

exactly K reconstructed ones
stable tie-breaking
padding never appears in logical output
reconstructed mask equals the clear oracle
10. Process isolation verification

Confirm:

P2, P0 and P1 execute as independent OS processes
P2 distributes only input-independent offline material
P2 exits before online input release
P0/P1 do not inherit the dealer's complete secret state
result collection happens only after protocol outputs are locally formed

Required:

P2_ONLINE_SILENT = YES

11. Cost smoke

Independently reproduce at least:

n=2
n=5
n=8

Verify the documented accounting for:

logical online bits
real wire bytes
offline material bytes
full-pipeline online rounds

This is a G3 smoke check, not the final M6 performance experiment.

12. Failure and one-shot smoke

At minimum verify fail-closed handling for:

wrong session
wrong material ID
malformed/truncated material
early socket close
reuse after success
reuse after partially consumed state

No failing execution may produce valid reconstructed protocol output.

13. Known evidence boundaries

The following are documented limitations and do not by themselves fail G3:

AUTHOR_EXACT = NOT_PROVEN
exact Theorem 4.2 cost match = NO
order-of-magnitude communication match = YES
common-mask optimization = NOT_IMPLEMENTED
field-DPF formal proof = NOT_DONE
persistent replay database = NOT_IMPLEMENTED
native FSS-key serialization remains same-build / same-architecture where documented

G3 verifies the frozen project implementation, not an author-exact binary reproduction.

14. G3 PASS criteria

G3 may be marked PASS only if all of the following hold:

clean receiver checkout builds successfully
full regression passes
the standard Protocol III entrypoint is available from main
raw-score → Top-K-mask E2E reconstruction matches the oracle
stable tie-breaking is correct
independent-process isolation is preserved
P2 remains offline-only
no plaintext reconstruction bridge is required
cost smoke agrees with frozen accounting
failure and one-shot checks fail closed
no developer-local artifact is needed

Then record:

G3_ACCEPTANCE = PASS

15. Receiver acceptance document

If G3 passes, create:

docs/reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md

The document must record:

receiver
reviewed main commit
environment
build commands
regression count
F1 E2E result
process-isolation result
cost smoke result
failure smoke result
known limitations
final G3 verdict
16. M5 closeout after G3 PASS

Only after G3 passes, update the current-status sections of:

PROJECT.md
README.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md

Set:

M5 = COMPLETED

Record:

M5-FIX-F1 = COMPLETED
F2 / G3 = PASS

Continue to preserve:

AUTHOR_EXACT = NOT_PROVEN
exact Theorem 4.2 cost match = NO
order-of-magnitude communication match = YES
common-mask optimization = NOT_IMPLEMENTED
field-DPF formal proof = NOT_DONE

Submit the G3 acceptance and M5 closeout through a PR.

17. Starting M6

Only after the G3/M5-closeout PR has been merged:

git switch main
git pull --ff-only origin main
git switch -c feat/m6a-performance-evaluation

Then follow:

docs/handoffs/M6A_NEW_CHAT_PROMPT_2026-09-25.md

Do not create the official M6 branch from the old M5 feature branch.

18. If G3 fails

Do not begin official M6 performance evaluation.

Record:

reviewed commit
failing command
failing test
observed output
failure category
whether the issue is build, correctness, process isolation, security boundary, cost accounting, or documentation

Return the blocker to the M5 owner before proceeding.
EOFcat > docs/handoffs/M5_TO_M6A_G3_RECEIVER_CHECKLIST_2026-09-25.md <<'EOF'
# M5 → M6A G3 Receiver Checklist

## 1. Authoritative baseline

Repository:

`https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation`

M5 implementation was merged by PR #25.

M5 implementation merge commit:

`2db7dbf`

The receiver must perform G3 from a clean checkout of the current `main`
that contains this commit, and must record the exact `main` commit actually reviewed.

Do not use:

- developer-local patches
- developer build directories
- uncommitted developer worktrees
- the old M5 feature branch as the M6 development base

---

## 2. Read these documents first

Read in this order:

1. `docs/handoffs/M5_PROTOCOL_III_TO_M6A_HANDOFF_2026-09-25.md`
2. `docs/reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md`
3. `docs/decisions/M5_FIX_F1_SECURE_IO_ADAPTER_DECISION_2026-09-25.md`
4. `docs/reproduction/M5_FIX_F1_SECURE_RAW_SCORE_TO_MASK_E2E_2026-09-25.md`
5. `docs/reproduction/M5_PROTOCOL_III_2ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-25.md`
6. `docs/handoffs/M6A_NEW_CHAT_PROMPT_2026-09-25.md`

---

## 3. Protocol III M6 entrypoint

Production API:

`protocol_iii_raw_score_mask_party`

Header:

`VFSS/include/moe_topk/protocol_iii_raw_score_mask.h`

Input contract:

- P0/P1 hold additive shares in `Z_(2^32)`
- scores use signed 32-bit two's-complement Q20.12
- public `logical_n`
- public `K`
- valid range: `n >= 2`, `1 <= K <= n`

Output contract:

- each party receives `std::vector<std::uint8_t>`
- output length is `logical_n`
- the two vectors are XOR shares of the original-input-order Top-K bit mask

After reconstruction:

`mask[i] = 1`

iff original item `i` belongs to stable Top-K.

Stable priority semantics:

1. score descending
2. original index ascending

---

## 4. Important round boundary

The complete standard raw-score → Top-K-mask pipeline is:

- secure input adapter: 2 online rounds
- Protocol III mask GRank/routing core: 2 online rounds
- output adapter: 0 additional online rounds

Therefore:

`FULL_PIPELINE_ONLINE_ROUNDS = 4`

The separately frozen generic field-valued Protocol III Fselect/Fsort core remains:

`CORE_ONLINE_ROUNDS = 2`

Do not describe the complete raw-score → mask pipeline as a two-round protocol.

---

## 5. Security boundary

The standard mask entrypoint must satisfy:

- `PLAINTEXT_RECONSTRUCTION_BRIDGE = NO`
- `P2_ONLINE_SILENT = YES`
- `RAW_SCORE_PUBLIC = NO`
- `RAW_RANK_PUBLIC = NO`
- `RAW_MASK_PUBLIC = NO`

The controller may create test inputs and cleartext oracle results only for testing.
It must not participate in secure protocol computation.

---

## 6. Clean receiver checkout

The receiver should begin from a clean checkout:

```bash
git clone https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation.git
cd MoE_Top-k_Reproduction_Evaluation

git switch main
git pull --ff-only origin main

git status -sb
git log -5 --oneline

Record:

G3_REVIEWED_MAIN_COMMIT

The working tree must be clean before build and test.

7. Build and reproduction source

Use the commands frozen in:

docs/reproduction/M5_FIX_F1_SECURE_RAW_SCORE_TO_MASK_E2E_2026-09-25.md

Do not request a developer-local patch.

Do not reuse the developer's existing build directory as G3 evidence.

8. Full regression

Run:

ctest --test-dir build-vfss-debug --output-on-failure

Baseline at F1 handoff:

41/41 PASS

If additional tests have been added on main, the total may increase,
but all original M5 regression paths must continue to pass.

Record:

G3_FULL_REGRESSION_RESULT

9. Mandatory F1 receiver verification

Independently verify the standard path:

signed Q20.12 secret shares
→ secure input adapter
→ shared CmpAgg / GRank
→ Protocol III rank routing
→ original-order XOR Top-K mask shares

At minimum test:

n=2
n=3
n=5
n=8

and:

K=1
K=n
middle K

Score cases:

ascending
descending
all equal
mixed duplicates
negative values
zero
positive values
supported boundary values
deterministic random inputs

Verify:

exactly K reconstructed ones
stable tie-breaking
padding never appears in logical output
reconstructed mask equals the clear oracle
10. Process isolation verification

Confirm:

P2, P0 and P1 execute as independent OS processes
P2 distributes only input-independent offline material
P2 exits before online input release
P0/P1 do not inherit the dealer's complete secret state
result collection happens only after protocol outputs are locally formed

Required:

P2_ONLINE_SILENT = YES

11. Cost smoke

Independently reproduce at least:

n=2
n=5
n=8

Verify the documented accounting for:

logical online bits
real wire bytes
offline material bytes
full-pipeline online rounds

This is a G3 smoke check, not the final M6 performance experiment.

12. Failure and one-shot smoke

At minimum verify fail-closed handling for:

wrong session
wrong material ID
malformed/truncated material
early socket close
reuse after success
reuse after partially consumed state

No failing execution may produce valid reconstructed protocol output.

13. Known evidence boundaries

The following are documented limitations and do not by themselves fail G3:

AUTHOR_EXACT = NOT_PROVEN
exact Theorem 4.2 cost match = NO
order-of-magnitude communication match = YES
common-mask optimization = NOT_IMPLEMENTED
field-DPF formal proof = NOT_DONE
persistent replay database = NOT_IMPLEMENTED
native FSS-key serialization remains same-build / same-architecture where documented

G3 verifies the frozen project implementation, not an author-exact binary reproduction.

14. G3 PASS criteria

G3 may be marked PASS only if all of the following hold:

clean receiver checkout builds successfully
full regression passes
the standard Protocol III entrypoint is available from main
raw-score → Top-K-mask E2E reconstruction matches the oracle
stable tie-breaking is correct
independent-process isolation is preserved
P2 remains offline-only
no plaintext reconstruction bridge is required
cost smoke agrees with frozen accounting
failure and one-shot checks fail closed
no developer-local artifact is needed

Then record:

G3_ACCEPTANCE = PASS

15. Receiver acceptance document

If G3 passes, create:

docs/reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md

The document must record:

receiver
reviewed main commit
environment
build commands
regression count
F1 E2E result
process-isolation result
cost smoke result
failure smoke result
known limitations
final G3 verdict
16. M5 closeout after G3 PASS

Only after G3 passes, update the current-status sections of:

PROJECT.md
README.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md

Set:

M5 = COMPLETED

Record:

M5-FIX-F1 = COMPLETED
F2 / G3 = PASS

Continue to preserve:

AUTHOR_EXACT = NOT_PROVEN
exact Theorem 4.2 cost match = NO
order-of-magnitude communication match = YES
common-mask optimization = NOT_IMPLEMENTED
field-DPF formal proof = NOT_DONE

Submit the G3 acceptance and M5 closeout through a PR.

17. Starting M6

Only after the G3/M5-closeout PR has been merged:

git switch main
git pull --ff-only origin main
git switch -c feat/m6a-performance-evaluation

Then follow:

docs/handoffs/M6A_NEW_CHAT_PROMPT_2026-09-25.md

Do not create the official M6 branch from the old M5 feature branch.

18. If G3 fails

Do not begin official M6 performance evaluation.

Record:

reviewed commit
failing command
failing test
observed output
failure category
whether the issue is build, correctness, process isolation, security boundary, cost accounting, or documentation

Return the blocker to the M5 owner before proceeding.
