# Reference Material Manifest

## Status

This manifest defines the M0-A boundary before Git initialization.  It does
not delete, move, or modify the local reference directories.  It prevents them
from being accidentally staged in the initial project repository.

The active Git baseline will contain only:

- `VFSS/` — active target framework;
- `VFSS-baseline/` — frozen source-level comparison snapshot;
- `PROJECT.markdown`, this manifest, future project documentation, and project
  governance files.

The raw directories below remain local reference material until a source-only
snapshot or an external immutable archive is explicitly approved.

| Local path | Role | Initial Git state | Reason |
| --- | --- | --- | --- |
| `ADSMPC/` | Older Sytorch/FSS prototype for Protocol III and a CipherGPT-style Top-K experiment | Local-only, ignored | Contains a 35 MB build tree and experiment output; source remains available locally |
| `Agarwal_TopK/` | Protocol I main reference, Protocol III algorithmic reference, locked material and benchmark harnesses | Local-only, ignored | Approximately 2.4 GB; includes benchmark packages, archives, static libraries, generated outputs, and nested Git metadata |
| `CipherGPT/` | Native two-party CipherGPT baseline | Local-only, ignored | Contains bundled dependencies, a 57 MB build tree, and generated paper logs/results |

## Required local preservation

The ignored status is a version-control decision, not a disposal decision.
Keep these paths in the shared local workspace:

```text
ADSMPC/
Agarwal_TopK/
CipherGPT/
```

They remain the first reference for code reading, differential testing,
algorithm tracing, and later source extraction.

## Reference entry points

| Implementation | Primary files to consult | Purpose |
| --- | --- | --- |
| Protocol I | `Agarwal_TopK/protocol1_ca/include/protocol1_ca/`, `src/ca.cpp`, `src/aav86.cpp`, tests | Compare-Aggregate, AAV86 planning, rank shares, Direct Top-K |
| Protocol III algorithm | `Agarwal_TopK/protocol3_ca/src/aav86.cpp`, tests | AAV86 graph and DCF conformance reference |
| Protocol III prototype | `ADSMPC/src/protocol3.cpp`, `RankingPhase.h`, `valiant_graph.cpp`, `routing_dpf.h` | Old end-to-end staging reference only |
| CipherGPT native | `CipherGPT/test/Top_K_paper_test.cpp`, `src/globals.cpp`, `src/shuffle.cpp` | Native two-party paper Top-K baseline |
| CipherGPT-style prototype | `ADSMPC/src/ciphergpt_topk_dcf_shuffle.cpp` | Experimental FSS/DCF comparison point; not native CipherGPT |

## Explicit exclusions from ordinary Git history

The following are retained locally but must not enter the first remote Git
history:

- all `build/`, `CMakeFiles/`, object files, static/shared libraries, and
  generated key/package files;
- benchmark outputs, experiment logs, raw result CSVs, and temporary inputs;
- `Agarwal_TopK/protocol1_ca/frozen_b1_stage4c/` and other clean-room runtime
  products;
- compressed dependency/source bundles under `Agarwal_TopK/frozen_inputs/`;
- nested `.git` directories in bundled third-party material.

## Later distribution decision

Before any teammate needs an identical reference copy, choose one documented
method per implementation:

1. source-only snapshot in a reviewed `reference-snapshots/` directory;
2. immutable external archive with SHA-256 and acquisition instructions; or
3. Git LFS only when the material is necessary, licensed for sharing, and too
   large for normal Git.

The decision must record origin, license/redistribution status, revision or
archive hash, included paths, excluded generated paths, and build instructions.
No raw reference directory is force-added merely for convenience.

## Baseline verification

On 2026-09-03, `VFSS/` and `VFSS-baseline/` matched for 131 source files after
excluding generated build files and `.DS_Store`.  Normalized manifest SHA-1:

```text
972fcd23187ecb603b22e37a2fefe608b2cb830b
```
