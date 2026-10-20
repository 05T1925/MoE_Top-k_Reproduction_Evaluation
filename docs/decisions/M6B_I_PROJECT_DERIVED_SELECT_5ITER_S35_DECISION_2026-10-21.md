# M6B-I-PROJECT-SELECT-5ITER-S35 决策记录

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

1. **接收对象。** S34 的 84 文件 canonical snapshot `caa5e162944c20331d6fabded31a419404a37b8228afd6066707b0791bf4a9c4` 作为本轮基线；同时把原清单外 `protocol_i_transport_conformance_test.cpp` 列入 S35 完整源码清单。S35 manifest 记录逐项 source hash、构建与证据身份。
2. **历史接收状态。** S33 原样版本因 S34 复现 SIGPIPE 缺陷而未获运行时独立接收；S34 修复版定向回归通过但此前尚待非作者最终接收。本轮不回写旧记录。S35 版本化修复通过本轮列出的独立构建、transport、metadata-only A 和 oracle E2E，接收状态为 `PASS_WITH_SCOPE`。
3. **A 的条件信任模型。** 用户确认 A 可作为与 T/P0/P1 不合谋的条件受信服务，为原子可见性和身份受限恢复服务，且没有 recipient 解密密钥。A-only 半诚实腐化模型与 A 受信的 P_b 单方腐化模型分开；恶意 A、A+party/T 合谋不包含在此决策。
4. **Metadata-only ABI。** party package format 4 与 signed escrow context format 1 采用独立 purpose/version signature domain。服务配置和 TEST_ONLY launcher 只接受 context 与 T 公钥，不接收 party package 路径/`PackageInfo`。服务 identity/certificate 与 context 绑定，双方 TLS pin 在应用 READY 前检查。以最终 E2E binary `f511f262…494ba` 在 Ubuntu 24.04 WSL root runner（UID 0）完整复跑，退出码 0、195/195 oracle pass；root-only 断言把 A 降到 uid/gid 65534、两份 package 打开均为 `EACCES`，并核对 `/proc` UID/argv。错指纹集成负例只覆盖客户端对 A 的 pin；服务端 party pin 顺序经独立源码审查确认，但同 CA/SAN 错指纹集成负例 `NOT_RUN`。部署环境必须保留同等隔离。
5. **输出/提交语义。** 采用原序 stable Top-K XOR mask shares，signed Q20.12 降序、同分原 index 升序、恰 K 位。真实 SQLite 状态机允许首个 ciphertext 行在 pending 状态先行持久化；第二行与 `COMMITTED` state/receipt 在同一个最终 transaction 中提交。共同逻辑可见点是最终 state+receipt commit，而非两行从未分开提交；两个 API 的 receipt/return 时刻可能不同。commit 前 API 输出为 `⊥`；内部暂存 share 不等于从未形成；pending restart abort，commit 后按身份恢复且不回滚。
6. **可信预处理模拟接口。** 报告冻结 `F_T/v1` 输入/输出与 issuer 生成律。模拟器可调用 setup/signing oracle 获得固定 verifier key 下同分布签名 package/context，不持有 T 私钥。理想独立 tape 下 triples、one-hot permutation shares 的构造关系明确；真实 CSPRNG、签名和 hash 项保留符号，未量化。
7. **安全停止点。** package/READY 接口和 Beaver 单门 masked opening 可以在 `F_T`、静态半诚实和 ideal tape 假设下作候选替换。首个未闭合的全局条件等式是 Select5 首个 `kTagOpenedCompare` 活动结果下，本方完整 Beaver/package/wire/open-share prefix 的 law 与仅用 `(x_b,z_b,public,L_S29)` 的 `Sim_b` 相同。无攻击被发现的声明；没有完整 P0/P1/A simulator，密码学安全保持 `OPEN / NO_GO`。不得扩大 L、不得创建 `secure` alias。
8. **通信与性能。** 正常 trace party 73 phases；N128/N256 final E2E 分别 88/89 phases。算法五次迭代、compare width `W`、exchange phases 与 RTT 独立。自然 I/O fault `NOT_REPRODUCED`；故障注入独立标识。正式 LAN/WAN、RTT 与性能矩阵 `NOT_RUN / NOT_MEASURED`；N>256 `NOT_ADMITTED`。v3 默认仍 OFF，并保持现有 V3→V4 性能前置门。
9. **Provenance。** S33/S34 fresh `libsytorch.a` member names/order/sizes 一致，但 historical archive hashes不一致；三个未变源对象可由绝对 `__FILE__`/debug path 差异解释，transport object 因源码修复变化。旧 manifest/fresh archive byte identity 仍未闭合，provenance 状态 `OPEN`。

## 门禁摘要

| Gate | 状态 |
|---|---|
| S34 start snapshot + supplemental source | `PASS` |
| S35 final source/build identity | `PASS_WITH_SCOPE` |
| default-OFF / explicit-v3-ON / no fallback | `PASS` |
| metadata-only A process/file boundary | `PASS_WITH_SCOPE` |
| client A-pin wrong fingerprint integration | `PASS` |
| server party-pin wrong fingerprint integration | `NOT_RUN`；READY 前校验的源码顺序 `PASS` |
| SIGPIPE/TLS/v2 transport regression | `PASS_WITH_SCOPE`；同时间无关 SIGPIPE 竞态未完全证明 |
| 195 oracle process cases + N128/N256 | `PASS_WITH_SCOPE` |
| natural multi-process I/O error | `NOT_REPRODUCED` |
| F_T setup joint law / CSPRNG advantage | `CONDITIONAL / OPEN` |
| P0/P1 full conditional transcript simulation | `OPEN / NO_GO` |
| A-only simulator and crypto advantage | `OPEN / NOT_QUANTIFIED` |
| S33/S34 archive provenance | `OPEN` |
| SQLite power-loss durability | `NOT_MEASURED` |
| formal LAN/WAN / RTT | `NOT_RUN / NOT_MEASURED` |
| N>256 / secure alias / default v3 | `NOT_ADMITTED / NOT_CREATED / OFF` |

详见 [S35 复核报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S35_REPORT_2026-10-21.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S35_GATES_2026-10-21.json)、[逐案记录](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S35_CASES_2026-10-21.json) 和 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S35_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-21.json)。
