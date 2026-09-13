---
name: protocol-reproduction
description: Implement or review this repository's secure Top-K protocols and adapters, including Agarwal Protocol I/III, AAV86 and BB90+DCF upgrades, protocol audits, communication validation, and benchmarks. Do not use for generic C++ maintenance or documentation-only edits.
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

The shuffle-based CA construction has a `2r+1` core target. Protocol III+AAV86's `2r` is a project combination target requiring its own argument and message audit.

Online-Dealer prototypes, full-graph preallocation controls, and plaintext graph oracles must remain separately identified and cannot substitute for the target protocol.

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
