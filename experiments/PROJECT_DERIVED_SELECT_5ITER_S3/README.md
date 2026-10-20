# PROJECT_DERIVED_SELECT_5ITER S3 probes

These scripts are TEST_ONLY design probes. They create no DCF keys, shares,
certificates, or protocol transcript, and do not change the VFSS runtime.

```powershell
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/public_view_probe.py `
  --out C:\path\outside\repo\s3-public-view.json

python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/material_budget.py `
  --out C:\path\outside\repo\s3-n128-material-budget.json

python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/membership_direction_probe.py `
  --out C:\path\outside\repo\s3-membership-direction.json
```

`public_view_probe.py` applies one fixed synthetic hidden permutation to two
different 128-element inputs with the same K and algorithm seed. It projects
S1 plaintext edge traces onto the public-field allowlist confirmed for S3.
The output is not a simulation of P0/P1 views; masked DCF inputs, secure state
updates, communication lengths, and ABORT exchange are explicitly not observed.

`material_budget.py` computes the exact known DCF sidecar components for a
conservative proposed 24-byte slot descriptor, fresh per-call operand masks,
and the current uCMP serialization, then
separately accounts for the current Protocol I shuffle's retained vector
payload at N=128,T=2. It reports unknown adaptive-control material as
NOT_MEASURED and refuses admission until that interface is defined. It does
not instantiate or serialize a party package.

`membership_direction_probe.py` checks only the algebraic mapping from strict
`left < right` to `key_i <= tau` and the stable 40-bit priority-key order for
small Q20.12 fixtures. Actual uCMP/DCF conformance remains NOT_RUN.
