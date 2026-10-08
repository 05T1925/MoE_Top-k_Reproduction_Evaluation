# TEST_ONLY DCF source transcription

This directory contains a small-word transcription of the ordinary DCF path in
`VFSS/ext/FSS/dcf.cpp`. It is an audit aid only. It is not imported by VFSS,
does not implement AES, does not assert cryptographic security, and is not a
replacement DCF.

Run from the repository root:

```text
python experiments/TEST_ONLY_BMW16_DCF_BRIDGE/test_toy_source_dcf.py
```

The tests fix two independent values: a root-state seed and an expansion-tape
seed. The tape is a finite table of independently sampled toy expansions,
shared by KeyGen and Eval. They exhaust every threshold/input pair for widths
1–5 over four tape choices (5,456 function checks), check non-unit payloads
and the zero function for widths 1–4 (1,360 checks), and check 5,456 more
threshold/input/tape triples for per-level control convergence and share
accumulation. A separate test checks one distribution event that rejects a
*literal* BGI correction-word assignment. These checks establish only that
this transcription follows the source recurrence and that the indicated
variable-wise assignment is not distribution equal. They say nothing about
computational key privacy of the serialized VFSS key.

For the distribution check, BGI15 Algorithm 5 line 7 samples its two target
seed-correction words independently. Under the proposed direct mapping from
the VFSS common seed correction word, the two BGI target correction variables
would be `(0,w)` or `(w,0)` because the target-path control bits are
complementary. At toy width `lambda=4`, the event `c_s^0=0 or c_s^1=0` has
probability 1 under that assignment but `31/256` under independent BGI words.
This is a counterexample to that direct internal-variable identification, not
an attack on VFSS's serialized-key distribution; another encoding or a
source-specific security proof remains possible.

Implementation identity for this audit: the runtime source is unchanged from
S22 commit `9210c11a7a877f8fe3687e0509c1722440cf63fe`; S23 and S24 are review
and documentation/model work. The experiment is kept separate from the
`PROJECT_DERIVED / EXPERIMENTAL` party path.
