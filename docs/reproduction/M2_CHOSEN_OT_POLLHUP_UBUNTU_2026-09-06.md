# M2 chosen-OT readable-hangup repair

Status: verified dependency-adapter repair.  It does not change Protocol I
messages, output, leakage contract, round count, or implementation label.

The implementation label remains
`m2_protocol_i_raw_score_input_modular_8round_mask_output`; its eight-round
decomposition is unchanged.  This is not paper-exact Protocol I progress and
does not implement `agarwal_protocol_i_exact`,
`agarwal_protocol_i_exact_mask_output`, `paper_3_round_exact`, or
`secure_shuffle_complete`.

## Environment

- Ubuntu 24.04.4 LTS on WSL2, Linux 6.6.87.2-microsoft-standard-WSL2 x86_64.
- GCC/G++ 13.3.0, CMake 3.28.3, OpenSSL 3.0.13, Debug build.
- Pinned `emp-tool v1.0.0-alpha.1` and `emp-ot v1.0.0-alpha.1` local build
  trees:
  `/tmp/moe_m28_emp.ok9WzQ/emp-tool-build` and
  `/tmp/moe_m28_emp.ok9WzQ/emp-ot-build`.
- Observed pre-test limits: soft `10240`, hard `1048576`.  All validation
  below explicitly used soft `RLIMIT_NOFILE=1024`.

## Defect and regression

`EmpBoundedFdIO::wait()` previously treated `POLLHUP` as an immediate peer
failure before considering the requested readiness event.  Linux AF_UNIX stream
sockets may report `POLLIN | POLLHUP` together after a peer writes a final
payload and closes.  Rejecting that combined state prevents an exact read of
already-buffered legal data.

The repaired order is:

1. immediately reject `POLLERR` and `POLLNVAL`;
2. return when the requested event, including `POLLIN`, is present;
3. reject only a remaining HUP-only state as peer failure;
4. reject any other event as an invalid poll event.

The socketpair regression forks a writer, writes 64 deterministic bytes,
closes it, waits for its exit, and then explicitly asserts that `poll()` on the
reader returned both `POLLIN` and `POLLHUP`.  Only after that kernel-state
assertion does it invoke the test-only raw receiver and check the entire
payload.  It is therefore a direct test of the former HUP precedence error.

## A/B and downstream results

At soft limit 1024, an isolated temporary build with the old wait ordering
failed this regression with `chosen OT peer failure`.  The repaired build
passed the regression and the directed suite:

```text
ulimit -n 1024
ctest --test-dir /tmp/moe_m216_chosen_on --output-on-failure \
  -R 'moe_topk_m2_(chosen_ot|opv|share_translation)_conformance_test'
```

Results: chosen-OT passed, OPV passed, Share Translation passed (3/3, 2.28
seconds).  Direct execution of the old binary exited nonzero with the expected
failure; the repaired chosen-OT binary exited zero.

The temporary old/new build comparison is a dependency-adapter conformance
check, not a performance measurement.  Timing, LAN/WAN data, and other
unmeasured fields remain `NOT_MEASURED`.  Before the independent modular E2E
FD-lifecycle repair, an EMP-ON full CTest run reached 18 passing tests and
failed the 19th modular E2E test at `socketpair`; this record makes no full
19/19 claim.  The separate modular lifecycle record contains the later full
matrix result and its scope.

## Interpretation

The former failure is a chosen-OT adapter poll-state error.  The repair drains
valid readable bytes without weakening failures for `POLLERR`, `POLLNVAL`,
HUP-only, timeout, EOF, or invalid events.  It has no effect on the M2 C-level
baseline's score semantics, oracle, role model, secure-path data handling,
round count, or paper-exact status.
