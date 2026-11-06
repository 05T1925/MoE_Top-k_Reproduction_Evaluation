# M6B-I-PROJECT-SELECT-5ITER-S26 决策

日期：2026-10-19  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决定

1. S25 的精确 74 文件候选在 S26 隔离 worktree 中按逐文件 SHA-256 重建，canonical SHA-256 为 `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。与 S24 的 `585d4fd60f8cc04ad4b14bf02730882084cfbb2bd7045d9b2ca36c956fc51438` 相比，独立重算确认仅 3 个测试源文件改变；运行时代码未改。
2. 本执行者参与 S19–S25 的实现、设计或证明推导，故 `INDEPENDENT_RECEIPT=NOT_SATISFIED`、`INDEPENDENT_CRYPTO_REVIEW=NOT_SATISFIED`。S26 的构建和测试是隔离的新技术复跑，不是异会话接收或非作者密码学签核。
3. `F_scoreconv/v2` 在完整 `R_c=Z/(2^c)` 中逐随机带保持稳定 priority key：`rho_0` 均匀、`rho_1=-rho_0`、`Q'_b=Q_b+rho_b`，故 `Q'_0+Q'_1=κ`，包括模回绕。S26 源码复核确认 v2 后续消费者使用完整 `(Q'_b,0,0)` 记录，没有将 `Q'_b>>h`、截断或窄化后的值重新带入比较路径。
4. S24/S25 的刷新等式只给出一个受限前缀结论。若 `V_pre` 排除 `rho_b`，且刷新带在给定 `x_b,V_pre,L_prefix` 后仍均匀独立，则 `Q'_b` 对该 `V_pre` 条件均匀。实际 party 在 ready/package 领取时已持有 `rho_b`；完整视图还包含 `rho_b`、相关 DCF/UCMP 与 shuffle 材料、包/manifest、lease 状态和后续 transcript。因此该等式不等于完整输出条件 sampler。
5. 对固定理想 score-conversion 输出 share `q'_b`，只有在可从 `(x_b,L)` 采样完整的未刷新本地前缀/材料状态 `X_b` 后，才可取 `rho_b=q'_b-Q_b(X_b) mod 2^c` 并重建完整视图。S25 的单 key/key-only 论证没有提供该联合条件 sampler，也没有覆盖实际党方完整 package。P0、P1 的 `FULL_OUTPUT_CONDITIONAL_SAMPLER` 均为 `OPEN/NO_GO`；没有找到攻击或可复现运行时缺陷。
6. 因完整 score-conversion sampler 未闭合，本轮不将其组合成 adaptive DCF pool 或 Protocol I shuffle/inverse/output/crash 安全证明。N=128 `M6B5/v4` 单体包与 N=256 `M6S2/v3` 流式池保持不同安全对象；仅独立复核布局数值并完成有限功能回归。
7. 保持 `EXPERIMENTAL`，不创建 `secure` alias，不扩大到 N>256。M6A V3 仍 OPEN、V4 未获准；正式 LAN/WAN 性能矩阵 `NOT_ALLOWED/NOT_RUN`。

## 实际复跑

- Release feature-ON 新建目录构建：目标构建成功；定向 CTest `7/7 PASS`。
- Release feature-OFF 新建目录构建：旧路径目标构建成功；定向 CTest `3/3 PASS`。
- 独立 streamed-store 与 TLS 1.3 mTLS conformance 均 PASS；ON API probe 可编译、OFF API probe 按合同拒绝，OFF 静态库无 v2 helper 符号且 OFF 目标列表无 v2 测试目标。
- v2 acceptance E2E：9 个成功样例逐例由 TEST_ONLY supervisor 重构 XOR mask 并比对冻结 oracle；6 个故障场景双方 ABORT/no-mask；6 个 crash-prefix 无完整 mask。步骤 5–6 留下 P1 的 8-byte 本地 XOR share，不表示原子无输出或 all-or-nothing。
- E2E preflight 拒绝 N=256 monolithic 与 N=257 streamed。完整 ON 全量构建本轮未重跑；S25 原始证据记录它在无关 `bitpack::mod(unsigned long,int)` 处以 exit 2 失败，targeted protocol build/test 成功。

复跑命令、逐案 JSON、构建/原始外部日志哈希见 S26 报告、E2E cases 与 source/build/evidence manifest。初次错误命令及其更正保留在外部 evidence 中，不计作功能失败。

## 下一项最小工作

由一名未参与 S19–S25 的密码学审查者先定义并证明或反驳 `S_b(x_b,L) -> (V_pre, party-local non-rho materials, lease state)` 的真实条件分布 sampler；要求同时覆盖 carry/sign 的本地 Eval shares、masked operand frames、旧 `Q_b`、包中的相关材料和失败/重启状态。若该 sampler成立，再分别固定 N=128 与 N=256 schema、审查 rho 的条件独立性并推导固定理想 `Q'_b` 下完整 party-view sampler。未完成前不得外推到自适应池或 shuffle/output/crash。
