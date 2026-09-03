# MOE Top-K Protocol Unification Project

## 1. Project baseline

This repository preserves four implementation lines and uses **VFSS as the
only active target framework**.  The work is a controlled migration and
evaluation project, not a bulk merge of source trees.

| Directory | Role | Modification policy |
| --- | --- | --- |
| `VFSS/` | Active, unified implementation | The only directory for new protocol code |
| `VFSS-baseline/` | Frozen VFSS source snapshot | Do not edit during ordinary development |
| `Agarwal_TopK/` | Reference implementations for Protocol I and the Protocol III algorithmic reference | Read-only reference |
| `ADSMPC/` | Older Sytorch/FSS integration prototypes for Protocol III and a CipherGPT-style Top-K experiment | Read-only reference |
| `CipherGPT/` | Original CipherGPT two-party secure GPT inference baseline | Read-only reference |

### Frozen VFSS rule

`VFSS-baseline/` is an in-repository comparison baseline.  On 2026-09-03 it
matches `VFSS/` exactly for 131 source files after excluding generated build
files and `.DS_Store` (normalized manifest SHA-1:
`972fcd23187ecb603b22e37a2fefe608b2cb830b`).

When Git is initialized:

1. Commit this snapshot before any VFSS protocol work.
2. Tag that commit, for example `vfss-baseline-2026-09-03`.
3. Require a dedicated review for any future change under `VFSS-baseline/`.
4. Never use the baseline as a convenient source of unreviewed patches; use it
   only for comparison, recovery, and regression diagnosis.

The complete M0-A tracking boundary is maintained in
`docs/REFERENCE_MANIFEST.md`.

### Repository architecture

The root repository separates development authority, frozen recovery state,
governance, and local-only reference material:

```text
moe_plan/
  VFSS/                     active implementation; sole target for new code
  VFSS-baseline/            immutable in-repository VFSS source snapshot
  docs/                     tracked decisions, manifests, paper mapping, specs
    REFERENCE_MANIFEST.md    tracking and distribution policy for references
  AGENTS.md                 tracked project-local implementation constraints
  PROJECT.markdown          project objective and non-negotiable constraints
  .gitignore                prevents generated output and raw references staging

  ADSMPC/                   local-only Protocol III / experimental reference
  Agarwal_TopK/             local-only Protocol I/III reference package
  CipherGPT/                local-only native CipherGPT reference package
```

`VFSS/`, `VFSS-baseline/`, and governance documentation form the initial
remote Git baseline.  The three raw reference directories intentionally remain
in the local workspace but outside ordinary Git history; their provenance and
future source-only/archive distribution decisions are controlled by
`docs/REFERENCE_MANIFEST.md`.

Future directories are created only when their owner and retention policy are
clear:

```text
VFSS/include/moe_topk/      public protocol contract and stable interfaces
VFSS/src/moe_topk/          common runtime and protocol implementations
VFSS/tests/moe_topk/        unit, differential, E2E, and benchmark tests
docs/decisions/             append-only architecture/security/accounting decisions
scripts/                    reproducible build, test, and benchmark runners
artifacts/                  ignored local output; never a source-of-truth
```

No source code belongs in `artifacts/`, and no generated result becomes a
benchmark claim until its configuration and raw output are recorded in `docs/`.

## 2. Objective and non-objectives

### Objective

Provide a common VFSS-hosted environment in which Protocol I, Protocol III,
and a clearly labelled CipherGPT Top-K adapter can be tested with a shared
input/output contract, correctness oracle, security review checklist, and
performance schema.

### Non-objectives for the first migration stage

- Do not replace the complete CipherGPT HE+OT framework with VFSS.
- Do not merge or overwrite VFSS's vendored `ext/FSS` with the ADSMPC FSS tree.
- Do not reuse binary DCF/DPF keys or serialized preprocessing files across
  reference projects and VFSS.
- Do not claim paper fidelity, privacy equivalence, or comparable performance
  until the exact papers, security models, and measurement rules are supplied
  and checked.

## 3. Source map and reuse boundaries

### Protocol I

The primary reference is `Agarwal_TopK/protocol1_ca/`.  It supplies the
Compare-Aggregate runtime, AAV86 planning, rank-share composition, threshold
mask generation, Direct Top-K checks, and substantial test/benchmark harnesses.
The frozen `Agarwal_TopK/protocol1/` source must remain untouched.

Reuse its algorithmic control flow and test semantics; rebind cryptographic
operations and transport to VFSS.  Its locked FSS and runtime packages are not
the VFSS runtime ABI.

### Protocol III

`Agarwal_TopK/protocol3_ca/` is the algorithmic reference: it contains an
AAV86 graph test and a DCF conformance test, not a complete end-to-end Protocol
III application.  `ADSMPC/src/` provides the old-framework prototype for the
larger pipeline: DCF ranking fragments, Valiant/AAV86 bucket updates, and DPF
routing.

Port the graph/state-machine semantics and reimplement the runtime binding in
VFSS.  File polling, fixed sleeps, debug reconstruction, and raw key-file
formats in ADSMPC are reference behavior, not target architecture.

### CipherGPT

`CipherGPT/` remains a first-class baseline, not an afterthought and not a
Protocol I/III variant.  It is a two-party GPT framework with SEAL/HE for
linear operations and EMP/OT/VOLE components for nonlinear operations.  Its
paper Top-K test performs two Benes-style shuffles, MSB-based comparisons, and
selection-index recovery before producing a mask.

CipherGPT has two planned tracks:

1. **Native baseline track:** preserve and run its original implementation and
   record results with its native cryptographic stack.
2. **VFSS algorithm-adapter track:** after the common contract is stable,
   implement only the high-level Top-K flow in VFSS using VFSS primitives.
   This result must be named `CipherGPT-style Top-K adapter`, not original
   CipherGPT, because the cryptographic backend and party model differ.

No performance chart may silently compare the native baseline and the adapter
as the same implementation.

## 4. Architecture direction

New functionality belongs under a dedicated `moe_topk` namespace/module inside
`VFSS`, with these dependency directions:

```text
apps / benchmark runner
            |
protocol orchestrators (protocol1, protocol3, ciphergpt_adapter)
            |
common domain + algorithm plans ------ VFSS runtime adapters
            |                                 |
      clear oracle / tests              DCF, DPF, transport, preprocessing
```

Suggested layout:

```text
VFSS/
  include/moe_topk/
    domain.h          # request, output, roles, metrics
    algorithm.h       # graph and planning interfaces without FSS types
    runtime.h         # VFSS crypto/transport adapter interfaces
  src/moe_topk/
    common/
    runtime/
    protocol1/
    protocol3/
    ciphergpt_adapter/
  tests/moe_topk/
    unit/
    differential/
    e2e/
    benchmark/
```

`common/` and `algorithm/` must not include raw FSS key types or networking
objects.  `runtime/` owns VFSS-specific DCF/DPF, transport, preprocessing, and
serialization.  Protocol directories orchestrate phases and may not duplicate
cryptographic helpers, metrics, or cleartext oracles.

## 5. Canonical protocol contract

The team must agree on this contract before migrating either protocol:

- Public configuration: `n`, `k`, score bit width, signed/fixed-point
  interpretation, payload shape, party topology, and experiment mode.
- Canonical selection: largest `k` scores; ties are deterministically broken by
  the original index.  Any protocol with opposite ranking direction must adapt
  at its boundary.
- Secure output: secret-shared selection mask and/or selected payload shares.
  `k` may be public.  Clear ranks, per-edge comparison bits, clear selected
  indices, plaintext verification data, and debug transcripts are not default
  secure outputs.
- Clear oracle: uses the same score interpretation and tie rule, but is kept
  outside the secure execution path.
- Modes: `test` may use deterministic seeds and controlled reconstruction;
  `secure` must use fresh randomness and reject test-only disclosure paths.

The precise paper-required output and allowed public transcript remain pending
until the papers are reviewed.  This contract is the conservative project
baseline, not a claim about the papers.

## 6. Runtime compatibility constraints

VFSS and ADSMPC share a Sytorch ancestry but their vendored FSS trees have
diverged in DCF, DPF, communication, config, keypack, and API files.  VFSS also
contains additional FSS variants not present in ADSMPC.  Therefore:

- VFSS `ext/FSS` is authoritative for all new code.
- New code may use a small compatibility adapter only where a needed primitive
  has a demonstrably equivalent VFSS API.
- Regenerate all preprocessing under VFSS; never deserialize legacy key files
  into VFSS key structs.
- Do not make an ADSMPC-only header such as `FSS/select.h` a hidden project
  dependency.  Either remove an unused dependency or expose a reviewed VFSS
  adapter with its own unit test.

## 7. Overall work plan

### M0 — repository and specification baseline

- Initialize Git with the active `VFSS/`, frozen `VFSS-baseline/`, governance
  documentation, and the reviewed ignore rules.
- Keep raw `ADSMPC/`, `Agarwal_TopK/`, and `CipherGPT/` directories local-only
  in the initial repository baseline.  Do not accidentally commit their build
  trees, benchmark packages, static libraries, archives, nested Git metadata,
  or generated protocol material.
- Use `docs/REFERENCE_MANIFEST.md` to record reference roles and later choose a
  source-only snapshot, immutable external archive, or approved LFS workflow
  when a teammate needs an identical reference package.
- Record source provenance, exact paper versions, dependency versions, and
  runnable entry points.
- Create a decision log for changes that affect semantics, leakage, or metric
  accounting.

### M1 — common VFSS foundation

- Define the canonical contract and a clear oracle.
- Add deterministic small test vectors: random, duplicates, negative values,
  `k=1`, `k=n`, non-power-of-two `n`, and supported bit-width boundaries.
- Add one metrics record schema shared by all VFSS-hosted implementations.
- Add primitive-level VFSS DCF/DPF smoke tests needed by the protocol paths.

### M2 — parallel Protocol I and Protocol III migration

- **Protocol I owner:** migrate AAV86 plans, Compare-Aggregate, rank-share
  composition, and Direct Top-K/threshold selection through the common runtime.
- **Protocol III owner:** migrate Valiant/AAV86 graph and bucket state first,
  then DCF ranking and DPF routing through the same common runtime.
- Both owners add differential tests against the clear oracle before claiming
  end-to-end completion.

### M3 — integration and review

- Run reference-versus-VFSS differential tests on small cases.
- Audit every opened value, message, seed, temporary file, and role transition.
- Confirm that output direction, tie handling, padding, and party-role mapping
  are identical to the approved contract.

### M4 — CipherGPT evaluation and adapter

- Stabilize the native CipherGPT `Top_K_paper_test` baseline first.
- Map its shuffle, comparison, selection, and output stages to the canonical
  contract.
- Implement the VFSS adapter only after M1-M3 are stable, with separate target
  names, configuration, and result labels.

### M5 — reproducible performance evaluation

- Use a single runner format where possible, but retain a field identifying the
  native runtime and topology.
- Store raw run metadata and summarized results separately.
- Report protocol-level and primitive-level costs separately.

## 8. Test, security, and performance gates

### Correctness gates

1. Unit-test graph generation, bucket updates, ordering, and selection.
2. Differential-test every implementation against the canonical clear oracle.
3. Run two-party/three-role end-to-end tests with independent processes.
4. Check output cardinality, stable ties, payload alignment, and repeatability.

### Security-review gates

For every migrated phase, document inputs, outputs, recipient roles, public
metadata, secret shares, opened values, randomness source, and persisted files.
Test-only reconstruction must be explicit and unavailable in secure mode.

### Performance-accounting gates

Every record must include at least:

- implementation label and Git revision;
- protocol/adapter name and party topology;
- `n`, `k`, score/ring width, padding policy, and thread count;
- CPU, memory, OS, compiler, flags, network bandwidth/latency, and repetitions;
- offline material, preprocessing communication/time, online communication,
  online rounds, active time, wall time, and correctness status.

Existing numbers are reference evidence, not yet a single comparable leaderboard.
For example, Protocol I summaries explicitly distinguish paper-core, extension,
and project-total accounting, and mark strict offline/online accounting as not
paper-faithful.  CipherGPT's CSV uses a different two-party metric split.
Preserve both raw schemas and normalize only with documented transformations.

## 9. Collaboration rules

- `main` must remain buildable for the declared supported environment.
- Use feature branches: `feat/vfss-common`, `feat/vfss-protocol1`,
  `feat/vfss-protocol3`, `feat/ciphergpt-adapter`, and `bench/...`.
- A pull request must state affected protocol steps, tests added, output/leakage
  impact, and metric-accounting impact.
- Reference-tree edits require a separate, explicitly justified review.
- No benchmark headline is accepted without the raw configuration and output.

## 10. Inputs still required for paper-faithful planning

Before paper compliance or final benchmarking claims, attach or register:

1. exact PDF/version for Protocol I, Protocol III, and CipherGPT;
2. algorithm, theorem, and security-model references relevant to each path;
3. target machine/network settings and required comparison table/figure;
4. definition of offline, setup, online, rounds, and communication for the
   intended report;
5. any required MOE model shape, routing semantics, and payload format.

Until then, the project may establish engineering equivalence to its clear
oracle and reference behavior, but must not overinterpret that as proof of
paper conformance.
