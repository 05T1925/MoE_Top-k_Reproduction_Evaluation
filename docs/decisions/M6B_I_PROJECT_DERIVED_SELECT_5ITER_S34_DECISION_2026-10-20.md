# M6B-I-PROJECT-SELECT-5ITER-S34 决策记录

日期：2026-10-20  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策摘要

S34 精确接收并核验 S33 的最终 84 文件快照（canonical SHA-256 `7e0f2ab6ae03ae402a739e2f5e97a09aa44a4fb3a373294f860ebb6f441c3df9`）。独立源码审查复现了 S33 SIGPIPE helper 在成功写期间吞并发外部 SIGPIPE 的缺陷，作了最小 transport 修复并补上回归。修复后本轮正常 trace、三项通信故障注入、v3 runtime/generic transport 定向 CTest、default-OFF 边界和 v2 TLS13 专用 target 均通过。完整 195 案矩阵未重跑。

S34 接收结论：`PASS_WITH_SCOPE`（精确 S33 接收、所列定向功能和通信边界）；本轮新的 v3 transport 修复只在所列回归范围内验证。自然多进程 I/O 故障 `NOT_REPRODUCED`。密码学安全 `OPEN/NO_GO`，正式 LAN/WAN 与 RTT `NOT_RUN/NOT_MEASURED`，N>256 `NOT_ADMITTED`。v3 默认关闭，不创建 `secure` alias。

## 来源分类与独立性

- **论文定义：**没有证据表明此项目派生路径等于 BB90 五迭代算法，也不是 BMW16 或 BB90+DCF 实现/性能结果。
- **本地参考/规格：**S31 的 `F_TOPK-U` 与 `F_TOPK-ATOMIC` 作为目标合同；S31 是规格和 TEST_ONLY 审查，不是 S32 runtime review。
- **项目派生算法：**`PROJECT_DERIVED_SELECT_5ITER`、全 pair GMW/Beaver-v3、在线 escrow A 是本地源码实现事实；算法迭代、比较依赖层和传输 phase 分开计量。
- **测试观察：**仅支持本报告列出的 oracle/多进程/故障边界，不构成模拟证明。
- **待证明假设：**离线 T 的理想联合随机带、非合谋边界、逐方输出条件模拟、A 视图/恢复模拟、密码学优势和真实存储耐久性。

S34 接收者未参与 S32 runtime 实现；和 S31 的规格审查是不同角色。S34 在 S33 最终快照上独立复核，发现 SIGPIPE defect 后只改 S34 隔离工作树。原 S32、S33 worktree 与用户主工作区均未清理、重置、覆盖或提交。

## 经负责人确认的角色假设与现实差异

负责人确认：可以把在线 A 建模为与 T/P0/P1 不合谋的**条件受信**提交/恢复服务；A 负责逻辑提交可见性和身份受限恢复，不持有 recipient 解密密钥。该确认不是恶意 A 安全证明，也不证明服务可用性、断电持久性或多方合谋安全。

但 S33 TEST_ONLY `escrow_worker` 接收 P0 package 文件路径并调用完整 package validator，实际进程因而可读取 triples 和 permutation shares。该 harness 的 A view 超出原定仅元数据与密文的 `L_A`，所以实现证据不能直接套用被确认的 metadata-only A 模型。继续准入前，S35 应改成 T 签发 metadata-only escrow context，并以进程/文件权限证明 A 不能读取任何 party package；另一个可选模型是显式把 A 获得的完整 P0 包纳入泄露，但这会扩大信任边界，不应默认采用。

## 输出、提交及材料合同

输入是两份 32-bit additive shares，模 `2^32` 重构后按有符号 Q20.12 分数降序、原始 index 升序稳定排序；输出为原输入顺序、恰有 K 个 1 的两方 XOR mask shares。单次成功执行目标 law：`R ← {0,1}^N`，P0 得 `R`，P1 得 `R XOR M`。中止时双方 API 不返回完整本方输出 share，已使用的 one-shot package/lease 不复用。A 提交前没有 API share 返回；逻辑提交后可分别恢复两份密文，但两个 service receipt 与 API return 不保证物理同时发生。

代码中的物理存储提交更细：首次 party submit 在独立 `BEGIN IMMEDIATE … COMMIT` 写入一个 pending ciphertext；第二次 submit 在另一个 transaction 写入第二 ciphertext，并在同一最终 transaction 把 state/receipt 更新为 committed。fetch 仅在 state=1 时可用。因此它实现了一个**逻辑可见性线性化点**，但两行 ciphertext 并非从未提交到共同 transaction 中。S31/S32 文案若要求“两个密文行由同一个 SQL transaction 一起插入”，则源码不满足该字面合同；本决策不把它改写成已满足。准入合同须由负责人明确采用 logical-visibility atomicity，或后续版本改变数据库状态机。

## 安全审查停止条件

逐方 simulator/hybrid 未能按 S31 规定的输入闭合。P0/P1 simulator 的首个精确断点为 T 包中的 `sig[64]`：真实 package 在任何在线 READY/frame 前由固定 T Ed25519 private key 对 package header/material 签名；规定 sampler 仅有本方 input share、理想 output share、公开参数与获准 leakage，没有 T signing oracle 或可生成对应固定公钥有效签名的输入。签名验证可检验这一差异。此处即停止向后组合；它是模拟接口缺口，不是签名伪造攻击。

A 方向还存在上述 TEST_ONLY 完整 P0 package 暴露，首个超范围字段是 package body 的 triple/permutation material。P0/P1 的 Beaver openings、线份额、活动比较结果、shuffle/inverse、输出刷新、加密提交和 abort prefix 尚无完整条件联合 sampler；不能仅凭 Beaver 定理或 E2E 外推。A 的重启、元数据和密文视图也没有完整模拟。更详细的 role/DAG、材料账本、理想输出/泄露和停止条件见 S34 报告与 gates。

## 最小版本化修正建议

1. 增加受信离线 `F_T`/模拟接口：向 P0/P1 sampler 提供经签名并绑定 session/invocation 的 party package，或提供一个不泄漏 T 私钥的理想 signing/preprocessing oracle；不得把私钥塞进在线方。
2. A 服务采用无 party-package 路径的 metadata-only启动参数；OS ACL 测试验证 A 进程读不到材料包。为测试恢复保留该隔离。
3. 明确 atomicity 为“最终 state/receipt commit 是唯一 fetch 可见性点”，并将两次 deposit 的 pending durability、超时 abort、重复请求及 restart 分别纳入 `F_TOPK-ATOMIC`；若需要同一 SQL transaction 写入两行，需独立协议/API 版本设计。
4. 证明 T 生成的全局 triples、one-hot permutation shares 及其随机带的联合条件分布；再由未参与 S34 的审查者逐方检查完整 party/A transcript simulator。
5. 量化 DRBG、Ed25519、RSA-OAEP/AES-GCM、SHA-256、TLS 及 leakage/timing 组合优势项；不预设任何安全位数。

不在 S34 创建 security alias，也不宣布密码学 PASS。正式性能仍受 V3→V4 前置门约束；本轮本地 exchange phase 不是 RTT。

详见 [S34 复核报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S34_REPORT_2026-10-20.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S34_GATES_2026-10-20.json)、[case results](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S34_CASES_2026-10-20.json) 和 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S34_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-20.json)。
