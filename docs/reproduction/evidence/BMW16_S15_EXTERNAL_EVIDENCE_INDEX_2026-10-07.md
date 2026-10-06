# BMW16 S15 外部原始证据索引

本索引对应 [S15 独立验证报告](../BMW16_S15_INDEPENDENT_ACCEPTANCE_AND_N256_DIAGNOSTICS_2026-10-07.md)。全部逐次 trace、tape、进程输出与 pilot JSON 留在仓库外：

```text
C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\
```

该目录递归清单为 `SHA256SUMS.txt`，SHA-256=`be7089865be7068f2101fb47ad5abbab9527935b2967cd22c997c32cdfcde9ea`。清单覆盖下表文件、每次 pilot 的 warmup/run 原件、共同 tape 和 trace，也包括被纠正的 harness 运行记录；密钥、raw input share、mask share 和构建物不在该归档中。S14 原始归档清单位于 `C:\Users\28641\.codex\artifacts\BMW16_S14_20261006\SHA256SUMS.txt`，SHA-256=`8899b04952e8d6d287a7a74804f9bc1eaece02e52174c1254eb38cf3736360ef`，64 项文件校验通过。

## 独立接收 S14

| 证据 | SHA-256 | 说明 |
|---|---|---|
| `independent-s14/3proc-final.log` | `26cbe246c59a2362bcd98edbea75cd42622c32208adafdc6cd0b20b7f4d20629` | S14 源重建后的 59 行三进程代表矩阵独立复跑 |
| `independent-s14/n128-common-tape/matrix.json` | `dae49383e868a29d7f629382024d368ab7ca7bc63d980e2a695aef05f0d70ce6` | n=128，K=2/8/80 三例中间轨迹 differential |
| S14 原归档 `SHA256SUMS.txt` | `8899b04952e8d6d287a7a74804f9bc1eaece02e52174c1254eb38cf3736360ef` | 64 个冻结源/报告/原始输出逐项验证 |

S14 测试归档与历史 S14 自报值分离；S15 不改写历史结果。

## S15 被测构建身份

- Git source revision：`2c6937116fbbc170e7a254f4449a3a6411b0cb70`，branch `codex/m6b-i-bmw16-s15`。
- Release，`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`，`MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON`。
- WSL2 Ubuntu 24.04 x86_64，GCC 13.3.0，CMake 3.28.3，OpenSSL 3.0.13。
- 三进程 TEST_ONLY binary SHA-256：`e30e1a60b10ec57fc0c592823634b84a29d032988ce8dab8733cd458c17d8544`。
- Party/conformance binary SHA-256：`765601fe6f2008831db6bca89fc2965d1f559e6a9c30258e24df4f34eb27f287`。
- S15 实现源码和二进制 hash 清单：`S15_SOURCE_AND_BINARY_HASHES.txt`，SHA-256=`5ddc5ba251aadb76a3d56d7c57efd98b55ffd9778716adcfbfe306bd21a3ebfa`。
- 起始源码冻结表（修改前）：`source-freeze.txt`，SHA-256=`5868b74b21905eba3db29948b18702c6ee835d9ddfa16fe8fff58fca69edafe0`。

## 当前 revision 复跑记录

| 证据 | SHA-256 | 结果/范围 |
|---|---|---|
| `ctest-s15-relevant-final.log` | `58903b2639353d94693c54c93f26ce47f89511cb060feef65412cd03e73d5eaa` | 6/6 相关测试通过 |
| `party-conformance-final.stdout.log` | `2e738872c8b4aa815ce2788ee3e0653aff883dde33f9c6dffc851c57b39c99b8` | party/bundle/uCMP 夹具，包括新空 claim store 重放见证；stderr 单独保存 |
| `n256-common-tape-revision/matrix.json` | `ba165fe6441d134d967efd7dd44149e74bdae0f00f485e45073b45f68e171afb` | n=256、K=2/8；用 S4 冻结原始 Git blob SHA `8644bf71…f3fdfbab`，两条中间轨迹 PASS |
| `e2e-s15-revision-final.stdout.log` | `1726bf0520306249d8ebc3ff954bf2fd82ab87ec24eade65ca03e20ab721e16d` | 61 条分方 E2E 的机器可审计标准输出 |
| `e2e-s15-revision-final.stderr.log` | `9adfbb83373252596d9f1e59af93ca18fed167f667667595127b1789558cb5e6` | 与标准输出分离的子进程错误/负例诊断 |
| `e2e-audit-revision-final.json` | `46017dadf6079970441b0d09a66136f80ac05d2d717f1a5c76f57024b3d35a14` | 独立边/状态/成本复算，PASS；57 SUCCESS + 各 1 个 forced algorithm/material/unexpected/peer-close |
| `pilot-revision-final3/pilot.json` | `8da80393e9e21097af444a022310074b594cd2a0036dd0337b41b79b92fc9224` | 四配置 1+5 pilot，20 次 measured 均 SUCCESS；包含各次 attempt raw hash、stage、I/O、计数及 RSS summary |
| `capacity_preflight_final.json` | `af86090d5c1b1612298ffb9af8014b2dce65e2c420d5234ba16bbec170ff1048` | n=64..1e6 checked 64-bit slots/package lower bounds；非实测字段明确说明 |
| `ideal_abort_bound_final.json` | `aff8d18987d2cc8ff2f5fc68445bd3d48f40492a3c6cc9eae5888368e8e3fea5` | 实际 S4 项目参数、rounding slack 和分段最大值检查；不继承 BMW16 Theorem 8 |

## 被保留、但不作为最终通过证据的执行记录

- `ctest-s15-very-final.log` 是一次错误地调用全量 CTest 的记录：临时 build 只编译本轮相关 targets，36 个其他历史 target 因 executable 不存在而显示 Not Run。正式验收只采用上述 `ctest-s15-relevant-final.log` 的 6 项完整相关集合。
- `e2e-s15-revision-final.log` 是曾把 stdout/stderr 合并后得到的运行记录，负例 stderr 可与 stdout 指标行交错；其独立审计器拒绝该文件。该原件保留但不作最终 E2E 证据，最终以分开的 stdout/stderr 和 audit JSON 为准。
- `n256-common-tape-revision-final*` 目录保留了因传入 S7 行尾副本/相对路径导致的冻结源身份检查失败；这不是算法轨迹差异。最终 matrix 从 Git 对象导出的原始 S4 blob（SHA `8644bf71…f3fdfbab`）重跑通过。
- `pilot`、`pilot-final`、`pilot-revision-final`、`pilot-revision-final2` 保留了计量摘要逐步补齐前的 1+5 原始 attempt。最终报告只引用 schema 完整的 `pilot-revision-final3/pilot.json`，历史 attempt 没有被改写或删除。

## 精确复跑命令

```powershell
cmake --build /tmp/bmw16-s15-rebuild --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test --parallel 4
ctest --test-dir /tmp/bmw16-s15-rebuild -R "moe_topk_bmw16|moe_topk_m2_(ucmp|score_input|transport|parallel_shuffle)" --output-on-failure
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc 'cd /tmp/bmw16-s15-rebuild && ./moe_topk_bmw16_experimental_3proc_test > <external>/e2e.stdout.log 2> <external>/e2e.stderr.log'
python experiments/TEST_ONLY_BMW16_S15/audit_e2e_log.py <external>/e2e.stdout.log
python experiments/TEST_ONLY_BMW16_S15/run_n256_common_tape.py --cpp-test /tmp/bmw16-s15-rebuild/moe_topk_bmw16_experimental_party_test --s4 <exact-S4-Git-blob-export> --out <new-external-dir>
python experiments/TEST_ONLY_BMW16_S15/run_pilot.py --binary /tmp/bmw16-s15-rebuild/moe_topk_bmw16_experimental_3proc_test --out <new-external-dir>
python experiments/TEST_ONLY_BMW16_S15/capacity_preflight.py
python experiments/TEST_ONLY_BMW16_S15/ideal_abort_bound.py
```

原始 party 和 pilot 日志经过字符串检查，没有发现 score/share、key、credential 或完整 mask 字段；mask 仅由 TEST_ONLY 控制器在 E2E 内重构以与冻结 oracle 对比。仓库只保存工具、协议代码、决策与报告，不保存外部原始日志、bundle、key、tape matrix dump 或构建物。
