# M6B-I-PROJECT-SELECT-5ITER-S31 决策

日期：2026-10-20
路线名：`PROJECT_DERIVED_SELECT_5ITER`
被审对象：S25 manifest 快照，canonical SHA-256 `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`

## 决策

1. **S25 成功输出 share law**：实际输出是每方的 `inverse.result.shuffled_share[i].word1 & 1`。逆 shuffle 的 `e0/e1` 含随机 `h.word1` 的相反项；`r_share` 用于 public-masked lane，不是输出 pad。`word1` 按 `uint64_t` 模 `2^64` 运算；`word0` 才按 `comparison_bits` 掩码。理想独立随机带下，`h` 的低位使每方 share 均匀，且 XOR 重构为 `M_runtime`。这是对源码 `±h`、`shuffled_share` 与 LSB 的实现推导，不是为模拟方便而设定；生产 RNG 联合优势仍未量化。S27 有限功能收据不替代全输入/全有效材料正确性证明。`F_TOPK-C` 不合格，不采用。

2. **S25 历史中止语义**：`UNRESOLVED_BY_EVIDENCE`。S25 本地 share 在 PREPARE/COMMIT/PUBLISH 前形成。步骤 5–6 crash probe 只读到 P1 的 8 字节长度头，声明 payload 长度 `n=7`，没有读取 payload；P0 管道为 0 字节。历史 `NO_COMPLETE_MASK` 不表示已发布 share，也不能推出原子无输出或 all-or-nothing。

3. **v3 中止合同**：负责人已为新 v3 选择 `F_TOPK-ATOMIC`。新增 escrow A 将 P0/P1 的 recipient-encrypted share 记录作为一个耐久、线性化事务提交。提交前双方理想输出都是 `⊥`；commit 转移同时使两份 share 各自可恢复；commit 后任一进程崩溃不回滚输出。后续物理 fetch 可异步恢复，不是理想输出事件。该合同仅适用于新 v3，不追溯改写 S25。

4. **A 的信任与泄露边界**：A 保证原子 commit、持久保存及恢复可用性，不能解密 shares，recipient decryption keys 不交给 A。新增 `L_A` 至少包含公开版本/session、固定密文长度、deposit/commit/abort 状态、到达顺序与时序。P0/P1 仍按原 `L_S29`；但全系统元数据接收者和参与方集合增加 A，故不能称整体泄露/腐化边界完全不变。A Byzantine、A 与腐化方合谋、或 A 的耐久性失效均不在当前合同内。

5. **v3 电路/材料**：继续评估离线可信 T 直接生成的一次性 XOR-shared Boolean Beaver triples over `GF(2)` 与固定 GMW 电路，无在线 Dealer、无 OT。`G=63N+N²W+C(N,2)(2W−1)+N²`，`W=32+log2(N)`。P0/P1 只公开批准的活动比较结果，inactive comparator、permutation、membership、raw score wire 保持秘密。P0/P1 成功输出维持均匀 XOR shares 和 `L_S29`，新 A 只见 `L_A`。

6. **TEST_ONLY**：单文件中心化原型使用 S25 `topk_oracle.h` 对照稳定排序，检查比较、五迭代控制、candidate/rank、membership、逆路由、输出 shares、triples 一次消费及材料错误/重领。12 个逻辑输出前缀检查 F_TOPK-ATOMIC 提交前双 `⊥`、commit 时同时逻辑可用和 commit 后恢复取回。它不证明 S25 中止行为、分布式 escrow 原子性、runtime conformance、双进程 E2E 或安全。

7. **v2/v3 路线**：不重试 v2 完整 sampler；保留 S30 的“证明缺口、非不可能性”结论。当前继续 v3 规格与 TEST_ONLY；其消除 S29/S30 条件分布断点仍只是有待证明的构造理由，尚无完整 GMW/Dealer/A simulator。

8. **准入**：独立技术规格复核和 TEST_ONLY 正确性已通过，准入下一阶段默认关闭、显式版本化的 opt-in runtime 开发；S31 本身不接入运行时代码。该阶段准入不是安全通过，仍需 P0/P1/A 完整 view simulator/hybrid、A 的 crash/recovery correctness，以及量化密码学优势项。`VFSS/` 运行时代码不变，无 `secure` alias。

## 独立审查状态

S31 独立 reviewer 确认未参与 S19–S30 的证明撰写、决策或实现，也未编写本原型。首轮提出的 `word1` 模数与 READY 预检问题已修正；后续指出的正常本地 share return/恢复 fetch、`W+51` 单一 commit 阶段、提交前 `⊥` 与暂存 party view、READY 固定形状泄露映射问题均已在报告、决策与 gates 中修订，并完成最终只读技术规格复核 `PASS`。技术复核、S27 功能收据和密码学证明分别记录，互不替代。

## 状态表

| Gate | 状态 |
|---|---|
| S25 identity | `PASS`，74/74 hashes match |
| S27 functional receipt | `PASS_WITH_SCOPE`，历史证据；S31 不重跑 |
| TEST_ONLY functional model | `PASS_TEST_ONLY`，最终 F_TOPK-ATOMIC 原型运行收据以 manifest 为准 |
| S25 success-share implementation law | `CONDITIONAL_IMPLEMENTATION_DERIVATION`（理想随机带下） |
| S25 historical abort semantics | `UNRESOLVED_BY_EVIDENCE` |
| v3 abort/output contract | `OWNER_SELECTED_F_TOPK-ATOMIC` |
| A escrow / `L_A` | `SPECIFIED; NEW TRUST AND METADATA BOUNDARY` |
| P0/P1 end-to-end simulator | `OPEN/NO_GO` |
| Modular composition | `OPEN/NO_GO` |
| v3 formal security | `NOT_PROVEN` |
| v3 technical spec review | `PASS_TECHNICAL_REVIEW_ONLY` |
| v3 next-phase opt-in runtime admission | `ADMITTED; IMPLEMENTATION_NOT_STARTED; SECURITY_OPEN` |
| N=128 / N=256 runtime and performance | `NOT_RUN` / `NOT_MEASURED` |
| M6A V3→V4 | 本轮未触及；既有状态为 V3 `OPEN`、V4 `NOT_ALLOWED/NOT_RUN` |

详情见 [S31 工作报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_REPORT_2026-10-20.md)、[S31 gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_GATES_2026-10-20.json) 和 [source/test/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_SOURCE_TEST_EVIDENCE_MANIFEST_2026-10-20.json)。

## 最终路线决策

**score-to-mask-v3 规格与 TEST_ONLY 原型通过，准入下一阶段默认关闭、显式版本化的 opt-in runtime 开发。** 采用负责人选择的 `F_TOPK-ATOMIC` escrow 合同；S31 不实现 runtime。P0/P1/A 完整安全模拟证明、量化优势项和 crash/recovery 论证仍是安全门，不把本轮功能验证或技术复核写成安全通过；不宣称 v2 不可能；不运行正式 LAN/WAN 性能矩阵。
