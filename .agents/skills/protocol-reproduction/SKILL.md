---
name: protocol-reproduction
description: Implement or review secure Top-K protocol stages in this repository, including Agarwal Protocol I/III, CipherGPT, AAV86, DCF/DPF/shuffle/routing adapters, leakage and round audits, or protocol benchmarks. Do not use for generic C++ maintenance or documentation-only edits.
---

# Protocol reproduction workflow

1. Read `PROJECT.md`, `docs/IMPLEMENTATION_PLAN.md`, the relevant decision record, and the
   cited paper/reference boundary before changing code.
2. State the implementation label and evidence level. Never promote reference behavior or a
   project extension to a paper claim.
3. Before implementation, write a compact stage table covering roles, inputs, preprocessing,
   messages, opened values, outputs, causal online rounds, and allowed leakage.
4. Reuse VFSS primitives through minimal adapters. Do not import old key layouts, file polling,
   fixed sleeps, mock shuffles, debug reconstruction, or an online Dealer unless the declared
   model explicitly requires it.
5. Validate in this order: primitive conformance, differential tests against the frozen oracle,
   then independent-process E2E. Keep `test` reconstruction outside `secure` interfaces.
6. Produce the original-order secret-shared Top-K bit-mask. Count index binding, shuffle,
   routing, inverse mapping, share conversion, and mask generation in the measured path.
7. Record revision, label, topology, input/seed, compiler/flags, CPU/OS, network, repetitions,
   correctness, timings, communication, rounds, PRG calls, and comparison edges. Use
   `NOT_MEASURED` for missing observations.
8. If a paper precondition is not met—especially the field/nonzero-payload condition for
   Protocol III compression—stop at the accurately named intermediate baseline and document
   the gap instead of adding a fallback.
