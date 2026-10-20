# PROJECT_DERIVED_SELECT_5ITER S2 tools

These scripts independently replay the S1 plaintext source, calculate finite
comparison-cost bounds, and expose the address/data-dependence of a hypothetical
round-aware material interface. They are TEST_ONLY. They create no DCF key,
protocol material, secret share, or secure-runtime adapter.

## Commands

Run from the repository root. Store outputs outside the repository:

```powershell
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/independent_accept.py `
  --out-dir C:\path\outside\repo\s2-independent-run

python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/comparison_cost_bounds.py `
  --out C:\path\outside\repo\comparison-cost-bounds.json

python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/material_access_prototype.py `
  --trace C:\path\to\S1\representative_trace.json --matrix `
  --paired-data-dependent-demo `
  --out C:\path\outside\repo\material-access-prototype.json
```

`independent_accept.py` compiles S1's thin C++ harness against the frozen
`VFSS/include/moe_topk/topk_oracle.h`. Its verifier separately reconstructs
sample order from sample-clique outcomes, then checks pivot identity, pivot
ranks, frozen edges, candidate/rank transitions, cross-layer repeats, stable
mask, and oracle output. It counts each sample clique and its pivot stars as
separate comparison dependency layers.

`comparison_cost_bounds.py` uses exact rational arithmetic for the finite
moment envelope and small-state dynamic program. It cross-checks the dynamic
program against un-memoized sample-path enumeration for every K at n=2..6.
The frozen-matrix values are mathematical upper bounds, not measured times or
simulated estimates.

`material_access_prototype.py` consumes only a plaintext trace and serializes
identifier/count witnesses. It deliberately prints direct pair addresses to
show what a non-oblivious slot lookup would expose. The target-matrix byte
formula is a DCF-sidecar lower bound from the current uCMP serializer; it
excludes shuffle preprocessing, oblivious access, transport framing, and
runtime metadata.
