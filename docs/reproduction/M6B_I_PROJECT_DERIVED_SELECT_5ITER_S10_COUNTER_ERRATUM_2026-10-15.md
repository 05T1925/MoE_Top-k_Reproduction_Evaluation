# S10 erratum: S8 comparison-call summary

Date: 2026-10-15  
Affected source: `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_E2E_CASES_2026-10-15.json`  
Input SHA-256: `82f89ea34821a96f8653f4f80449162d0cdb8b5dfb10f8e69c6fee1f5b8d1b53`  
Recomputation: `experiments/PROJECT_DERIVED_SELECT_5ITER_S10/recompute_s8_counters.py`  
Machine output: `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S10_COUNTER_ERRATUM_2026-10-15.json`

## Corrected calculation

The script loads exactly 24 rows, accepts only `ORACLE_PASS` rows for comparison-call statistics, checks that the six other rows are `UNIFORM_ABORT_NO_MASK`, groups by padded N, sorts the observed counters and computes the ordinary median (the mean of the two center values for even cardinality).

| Padded N | Successful rows | K multiplicity | Sorted observed calls | min / median / max |
|---:|---:|---|---|---:|
| 128 | 7 | 1:1, 2:1, 8:1, 64:2, 65:1, 128:1 | 298, 299, 303, 368, 747, 2094, 2479 | 298 / 368 / 2479 |
| 256 | 8 | 1:1, 2:1, 8:1, 64:1, 128:2, 255:1, 256:1 | 600, 605, 607, 626, 765, 10967, 12271, 14723 | 600 / 695.5 / 14723 |

Outcomes in the full JSON: 18 `ORACLE_PASS`, 6 `UNIFORM_ABORT_NO_MASK`.

## Reconciliation

- The S8 report and S8 source manifest print N=128 `298 / 747 / 2479`. Their min and max match the JSON, but `747` is the fifth ordered observation, not the median of seven values. The corrected median is `368`.
- The S8 decision prints N=128 `299 / 1825 / 5340` and N=256 `600 / 676 / 18408`. None of those triples is the result of the specified grouping over the 24-row case JSON. The JSON does not establish which alternate input or filter produced those numbers; their origin remains unexplained and they are unsupported for this 24-case set.
- S9 is an implementer rerun. Its reported run is not independent acceptance and is not substituted for the frozen S8 case JSON in this reconciliation.

This is a correction to the current summary interpretation of one finite functional case file. It does not edit the S8 JSON, S8 report, or S8 decision; those historical records remain byte-for-byte as received. These are observed functional comparison-call counts, not timing, expected cost, a tail bound, theorem, or formal performance result. The route's already proved worst-case bound is unchanged.
