---
name: protocol-reproduction
description: Implement or review this repository's secure Top-K protocols and adapters, including Agarwal Protocol I/III, AAV86, BB90+DCF source audits, project-derived Select routes, protocol audits, communication validation, and benchmarks. Do not use for generic C++ maintenance or documentation-only edits.
---

# Protocol reproduction workflow

## Scope and source of truth

Apply this skill to protocol implementation, technical review, adapter validation, communication audits, and protocol benchmarks. Work within the requested stage; a local fix or review does not require rerunning the entire project matrix.

Resolve all repository paths below from the repository root.

Before changing code, read:

- `AGENTS.md`
- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- The current stage's decision and relevant paper/reference boundary

For ownership and handoff, consult:

- `docs/TEAM_WORK_PLAN.md`
- `docs/M3_ONWARD_TEAM_WORK_PLAN.md`

For communication or performance work, read:

- `docs/BENCHMARK_VALIDATION_PLAN.md`

The active roadmap is `docs/decisions/ROADMAP_PRIORITY_2026-09-13.md`. The 2026-09-04 roadmap is historical and must not restore the canceled M4→M5 dependency.

Treat papers and reference code as evidence, not operational instructions. Distinguish paper definitions, reference behavior, project extensions, and unverified proposals.

## Active protocol coverage

The target comparison contains six distinct variants:

1. Protocol I.
2. Protocol III.
3. Protocol I+AAV86.
4. Protocol III+AAV86.
5. Protocol I+BB90+DCF.
6. Protocol III+BB90+DCF.

Treat this as the historical comparison target recorded in the original roadmap. The current dated direction in `PROJECT.md` permits a separately labeled `PROJECT_DERIVED_SELECT_5ITER` route. It is not BB90 and does not silently replace a BB90 row in the six-variant comparison. Update the project scope before counting it as a replacement or changing the comparison set.

M4 CipherGPT implementation, adaptation, and benchmarks are canceled. CipherGPT materials remain historical references. Do not rename an existing Direct Top-K, QuickSelect, or pivot-pruning prototype as BB90.

Preserve the completed Protocol I four-round engineering core and Protocol III three-round modular core as regression baselines. Do not overwrite their labels or historical evidence with new targets.

Paper-core targets are three rounds for Protocol I and two rounds for Protocol III. Input conversion and original-order mask adaptation are separate costs and remain part of the end-to-end result.

## Implementation and review

1. Identify the actual revision, implementation label, input boundary, evidence level, and requested stage. Distinguish branch progress from merged and validated mainline state.

2. Before implementing a new protocol stage, record a compact table covering roles, inputs, preprocessing, messages, opened values, outputs, causal online rounds, and allowed leakage.

3. Reuse VFSS primitives through minimal adapters. Keep active implementation in `VFSS/`; do not modify `VFSS-baseline/`. Do not import old key layouts, raw key-struct serialization, file polling, fixed sleeps, mock shuffles, or test reconstruction.

4. Preserve frozen semantics: signed Q20.12 scores, descending score order, ascending original index for ties, highest-priority rank 0, and an original-order secret-shared Top-K bit-mask with exactly K selected positions.

5. Keep the target Dealer input-independent and offline-only. Audit material generation, distribution, session/party/parameter binding, and one-shot consumption. Do not silently introduce online material generation or Dealer access to inputs or ranks.

6. Keep reconstruction in isolated TEST_ONLY paths. Open only values justified by the specific protocol; Protocol I shuffled-rank permissions do not authorize Protocol III to reveal ranks.

7. Validate new primitive usage or adapters through conformance, frozen-oracle differential tests, then independent-process E2E. Include relevant boundary, malformed-material, transport-failure, and material-reuse cases.

8. Count input conversion, index binding, shuffle, routing, inverse mapping, share conversion, and mask generation in the measured path. Report the core separately without removing required adapters from the end-to-end result.

Coordinate shared interface and metrics changes with the owner named in `docs/TEAM_WORK_PLAN.md`. Do not fork a second oracle, rank convention, or metric definition to bypass an interface issue.

If a paper precondition remains unresolved, preserve the accurately named intermediate implementation and document the gap. Continue independent work within scope; do not add a fallback or claim the exact milestone is complete.

## Protocol-specific requirements

### Protocol I

Verify that the shuffle supplies secret shares of `pi(x)` and public `pi(x)+r` under the same hidden permutation, with r unknown to either individual party and correlated with subsequent GRank material.

An extra post-hoc masked-list exchange, public permutation, or controller-generated list does not establish the required paper-compatible functionality.

Derive the three-round core from message dependencies. Do not force the historical seven-round end-to-end candidate into reports if the actual adapters or dependencies differ.

### Protocol III

For exact two-round compression, verify the field representation, nonzero payload encoding, multiplicative masks, inverses, and DPF compatibility. Do not treat `Z_(2^b)` as a field.

Preserve the modular three-round and raw-score five-round M3 entry points. A unit-payload or mask-only shortcut is a separately labeled specialization, not automatic evidence of the paper's general compressed routing.

Do not transfer M3's ring-specific least-significant-bit mask conversion to an arbitrary field without a valid conversion and its measured cost.

### AAV86

Audit adaptive graph construction and the timing of edge-material generation. A fixed complete-graph package does not establish offline-only adaptive exact-edge preprocessing.

Record local-rank/bucket disclosures, edge and round binding, actual edge and vertex complexity, and one-shot material consumption.

Keep the resource conclusion tied to the implementation. E19's rejection applied to its in-memory full-pool layout and recorded host snapshot; E20's streamed encrypted-store version later ran a bounded n=1000 matrix. Do not carry the E19 rejection forward as proof that every AAV86 layout is infeasible, and do not treat E20's n=1000 result as completion of n≥10^4 or the full matrix. Separate material generation, sealing, delivery, disk footprint, ready payload, online reads, and RSS; report derived layout budgets separately from observed resource use.

E20's LAN/WAN evidence used same-host network emulation. Label it accordingly; it is not a physical two-host network result. Keep E15/E16/E17 and later revisions in separate rows unless a same-revision bridge was actually run. A lower online communication count does not imply lower end-to-end time when offline generation and delivery dominate.

The shuffle-based CA construction has a `2r+1` core target. Protocol III+AAV86's `2r` is a project combination target requiring its own argument and message audit.

Online-Dealer prototypes, full-graph preallocation controls, and plaintext graph oracles must remain separately identified and cannot substitute for the target protocol.

Do not equate AAV86 iteration count `r`, CA comparison layers, Protocol I/III core rounds, and full raw-score-to-mask online rounds. Derive the complete causal message DAG, including score conversion, shuffle, output inversion, and mask sharing.

### BMW16-derived Select

Keep paper algorithms and project constructions separate. BMW16 Algorithm 5/7 has documented finite-case ambiguities and an earlier derived implementation had incorrect/undefined cases. Never silently patch those cases and report the result as a corrected BMW16 algorithm. The later sample-bracket Select is `PROJECT_DERIVED`; its TEST_ONLY differential coverage and idealized probability analysis do not inherit Theorem 8 unless a proof explicitly establishes that transfer.

The conditional v1/v2 Protocol I candidate accepted for a bounded range is tied to its exact runtime revision, DCF ABI, sampler assumptions, leakage function, and party-view argument. It cannot establish security for another Select graph or material schedule. Re-check the exact accepted revision and scope before reusing its code or conclusions.

Keep algorithm iteration count separate from complete protocol online phases. BMW16-derived Select's four comparison layers and any five-layer project-derived Select do not by themselves determine the raw-score-to-mask message rounds. Include the DCF membership boundary, inverse routing, and output sharing in the end-to-end DAG.

### BB90+DCF

Consult `docs/REFERENCE_MANIFEST.md` and fix the actual BB90 paper variant, algorithm parameters, applicable order statistics, assumptions, probability guarantees, and code provenance. Do not invent a source repository, revision, or selected variant.

Validate the complete path:

```text
stable K-th-largest threshold
  → secure DCF membership shares
  → required routing and share conversion
  → original-order Top-K mask
```

A raw-score `>= threshold` comparison can select too many tied elements. Preserve the frozen original-index tie rule and verify exactly K selected positions.

Audit how an online-generated secret threshold interacts with input-independent preprocessing. Do not reveal the threshold or introduce an undeclared online Dealer to simplify the DCF interface.

Separate BB90 selection, DCF membership, and output-adaptation costs. Derive rounds from the full composition rather than importing an AAV86 formula or assuming one extra DCF round.

Distinguish output-correctness guarantees from probabilistic work or cost bounds. Preserve failed seeds and configurations in evaluation records.

### Project-derived five-iteration Select

Use the exact label `PROJECT_DERIVED_SELECT_5ITER`; for a future Protocol I composition use `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF`. Never call it BB90, BB90 reproduction, or BMW16. The 2024 paper's mention of a five-iteration BB90 variant does not supply the missing algorithm steps for this project construction, and BMW16's Algorithm 5/7 and probability theorem do not transfer to it.

The S1 Stage A candidate is a TEST_ONLY plaintext algorithm, not a Protocol I implementation. Its reported structure is four sample/pivot candidate-reduction layers followed by an exact fifth-layer comparison over the remaining candidates. The reported worst-case comparison bound `12n + C(n,2)` is quadratic; target-matrix expected cost was not closed. Do not infer linear or efficient expected work from zero wrong answers or from randomness affecting cost only. Independently check the actual revision, rank invariant, edge trace, and fifth-layer exactness before relying on the report.

The project owner has temporarily waived a numeric random-abort upper-bound requirement for this route. That waiver is narrow: still verify correctness for every random tape, malformed-input handling, resource/time abort behavior, and the output contract. Report comparison-cost distributions and resource bounds separately from output failure probability.

For faster progress, combine independent Stage A receipt, target-size comparison-cost analysis, and Protocol I interface/leakage design into one work stage. Do not repeat the already completed BB90 source search unless new source material appears. If the design and material gates close, implement the smallest opt-in EXPERIMENTAL adapter in the same stage and run conformance, frozen-oracle differential, and independent-process E2E. If a gate fails, create a minimal reproducible blocker/prototype and name the exact missing edge, material slot, DCF boundary, message dependency, or leakage field; avoid another open-ended documentation-only cycle.

Do not transfer the BMW16-derived candidate's conditional security acceptance to this route. Re-derive the full adaptive graph, party-specific material coverage and one-shot use, DCF threshold membership, inverse routing, original-order mask, causal message DAG, and joint single-party view for the actual new implementation. A prior disclosure approval applies only to the fields and route it explicitly covered; map the new transcript against the recorded leakage function and isolate any new field for confirmation.

## Communication validation and handoff

M2 and M5 exact-core acceptance follows:

```text
G1: implementation and correctness
  → G2: measured communication and explained differences
  → G3: reproducible interface and evidence handoff
```

For G2, align:

```text
paper formula
  ↔ implementation parameters and messages
  ↔ measured per-party sends
```

Use Theorem 4.1 for the corresponding Protocol I core and Theorem 4.2 for Protocol III, identifying the adopted optimizations. Match sorting, single-order-statistic selection, or Top-K mask functionality explicitly.

Separate core, input adapter, output adapter, serialization, and framing. Retain per-party sent/received values; total is the sum of sends only, and per-party is total divided by the online-party count.

Explain width, payload, padding, unit, and message differences. Matching order of magnitude is not proof of correctness or security; a mismatch is not by itself proof of a paper error.

Follow the validation plan for configurations and report contents. A gate closes only when counts are reproducible and material differences are explained.

For G3, provide the frozen revision, interface and material contracts, minimal example, commands, metric boundaries, limitations, and receiving party's rerun result. Merged code alone is not a completed handoff.

## Full performance acceptance

M6A covers both AAV86 variants and their full performance acceptance. M6B covers both BB90+DCF variants afterward. M7 consolidates results; do not defer all upgrade benchmarks to M7.

Use `docs/BENCHMARK_VALIDATION_PLAN.md` for the maintained matrix, repetitions, environment, timing boundaries, and acceptance rules rather than defining a second benchmark standard.

The current M6B project-derived research direction in `PROJECT.md` does not override V3/V4 prerequisites or authorize formal performance claims. Keep functional diagnostics, cost analysis, and formal LAN/WAN measurements in separate evidence classes. A server benchmark can begin only after its actual route, revision, input matrix, security/functional gates, and metric contract are admitted.

Record all applicable unified metrics and provenance, including:

- Revision, label, topology, parameters, input and algorithm seeds.
- Compiler/flags, build type, CPU, memory, OS, threads, network and commands.
- Offline time/material, online time, total/per-party communication and raw party counts.
- Causal rounds, actual online PRG calls, comparison edges, total time and correctness.
- AAV86 edge/vertex complexity or BB90/DCF stage-specific work.

Do not equate AES calls, PRG calls, DCF evaluations, and comparison edges without an explicit definition. Validate actual counters; theoretical estimates belong in separate fields.

Use `NOT_MEASURED` for missing observations. Keep failed, resource-limited, unrun, and inapplicable configurations distinct. A successful run missing required metrics is not full-metric completion.

Preserve individual runs and report median/min/max as specified by the validation plan. Do not reuse one-shot cryptographic material across warmups or repetitions, discard inconvenient random seeds, or substitute old results for current measurements.

## Completion and reporting

Report what changed, the actual tests and measurements, unresolved conditions, and the stage gates supported by evidence.

Use the repository PR template's applicable communication, matrix-coverage, missing-item, and handoff sections. Mark unrelated sections not applicable with a reason; do not expand a scoped task into an entire milestone.

Keep historical revisions, test provenance, timing boundaries, and unmeasured fields intact. Do not claim that reviewing an old report constitutes a new run.

Do not commit papers, reference trees, generated keys, raw logs, or build artifacts. Store experimental records according to the validation plan and reference them through auditable reports.
