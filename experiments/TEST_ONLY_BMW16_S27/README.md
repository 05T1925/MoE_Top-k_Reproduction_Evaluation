# BMW16 S27 TEST_ONLY DCF models

These scripts are isolated algebra/conformance aids. They are not imported by VFSS and do not implement AES, a cryptographic simulator, or a secure protocol.

## Reproduce

From the repository root:

```powershell
py -3 experiments/TEST_ONLY_BMW16_S27/ideal_dcf_model.py --seed 270109 --max-bits 5
py -3 experiments/TEST_ONLY_BMW16_S27/check_toy_source_dcf.py --seed 270109 --max-bits 5
```

The first model follows the groupSize=1 `dcf.cpp` recurrence, constructs the complete M2UC-v1-shaped party key, and checks reconstructed values over all domains through 5 bits. The second model records each party's small-word evaluation transitions and checks the source-shaped recurrence over every alpha, x, and payload in `{0,1,255}` for widths 1–5.

The 382-bit expansion tape is ideal and fixed for each run. These checks can expose transcription or algebra errors; they do not prove AES/PRG security, the real DCF key distribution, multi-key security, shuffle privacy, or sampler probability.

## S27 run record

The S27 review ran both commands with seed `270109` and `--max-bits 5`. Each checked 4,092 `(width, alpha, x, payload)` cases. The source-trace model's trace digest was `2e7499b8605156b2f57ba50791750f6991da5aba72d2a7a73a011b466125ce68`.
