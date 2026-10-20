# M6B-I-PROJECT-SELECT-5ITER-S32 决策记录

日期：2026-10-20
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

按 S31 已接收的规格实现默认关闭、显式版本化的 v3 功能运行时。S32 的功能与故障恢复结果准许保留为 opt-in 实验实现；这不是密码学安全批准，也不允许创建 `secure` alias。

运行时保留五次算法迭代与 `F_TOPK-U`：对冻结的 signed Q20.12 输入，按分数降序、原始索引升序稳定选择恰好 K 项，返回原输入顺序的两方 XOR-shared mask。单次成功执行的份额 law 为 `R ← {0,1}^N` 均匀，P0 得到 R，P1 得到 `R XOR M`；任一方都不收到完整 mask。

失败合同采用 S31 的 `F_TOPK-ATOMIC`。A 将两份收件人密文、密文摘要和共同提交回执放入 SQLite WAL `synchronous=FULL` 的一个线性化事务。持久提交之前两个 Party API 都不给出 share；提交后该事务不可回滚，Party 可在服务重启后只取回各自密文。若执行方失去提交回执但不能确定 A 是否提交，API 返回 `RECOVERY_REQUIRED`、不给出本地 share，并保留事务 ID；这不是 ABORT。一个份额已暂存但另一个份额未到达的事务，在 A 重启或事务超时后被耐久标为 ABORTED，双方不能 fetch。

## 新参与方和依赖边界

在线拓扑新增 escrow 服务 A。T 仍是输入无关的离线可信预处理方，生成包后退出；在线不重连 T。A 仅收到 TLS 认证 Party 发来的收件人加密 share，持有密文和交易元数据，不持有解密私钥。A 会看到 session/invocation、N/K、双方身份、密文长度与哈希、事务提交/中止及访问时序；该观察按 S31 的单独 `L_A` 记录，不得将该拓扑继续描述为仅有“两方在线、Dealer 离线”。

运行时只在 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON` 时编译并注册 v3 API/目标。该选项默认 OFF；v2/DCF schema 不参与新路径，失败时没有降级路径。A 的耐久线性化提交使用 SQLite3，因此新增 SQLite3 开发依赖只在 opt-in 开启时查找；密码、签名、收件人加密和 TLS 使用 OpenSSL API，没有自制原语。

## 接收范围

- S25 精确候选源快照在其原 worktree 中重新核验 74/74，canonical SHA-256=`64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。当前 S32 worktree 仅对 CMake 和共享 TLS/framing transport 三个文件有实现增量；其余 71 个快照文件仍逐字节匹配。
- S27 对 S25 的功能接收 `PASS_WITH_SCOPE` 为历史接收证据，不作为 S32 新功能测试结果。
- S31 TEST_ONLY 规格、决策、报告与 gates 哈希核对通过；S31 独立技术审查批准了规格进入 opt-in runtime 阶段。该审查不等于本轮实现审查或安全证明。
- S32 专项运行时 conformance 通过；独立进程 E2E 共 195 案通过。测试实际覆盖 N={1,2,3,4,5,8,128,256}；这不是对所有整数 `1≤N≤256` 的穷举运行承诺。N=128 和 N=256 资源点已真实运行。
- 默认 OFF 符号、目标和头文件边界通过；开启配置下 v3 targets 可构建。旧 Select5 control、score-key refresh v2 和 TLS transport 定向回归通过。
- 通过范围是功能正确性、one-shot 材料消耗、事务输出门控和所列故障恢复行为。完整党视图模拟、预处理联合分布、密码学优势和 A 的恶意/系统级容错仍未证明。

## 安全与性能状态

P0、P1、A 的完整模拟与组合证明均为 `OPEN/NO_GO`；T 生成的 Beaver triple shares 与 permutation shares 的联合分布证明仍为 `OPEN`。OpenSSL CSPRNG/DRBG、Ed25519、RSA-OAEP/AES-GCM 和 TLS 1.3 的本地构造优势项及运行参数均 `NOT_QUANTIFIED`。本轮没有可复现区分器，也没有把功能测试当成安全 PASS。

形式化 gate、错误尝试和未运行项见 [S32 gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S32_GATES_2026-10-20.json) 与 [S32 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S32_REPORT_2026-10-20.md)。S32 未运行正式 LAN/WAN 矩阵；E2E 时间仅为本地诊断值。M6A V3→V4 gate 独立保持原状态，不由本轮功能接收改变。

## 后续准入

允许在明确 opt-in 构建中继续做代码审查、接口完善和密码学证明工作。不得默认启用、发布为 secure、创建安全 alias、将 N 的支持范围外推到未实测点，或将此路线称作 BB90/BMW16 复现。下一项安全准入材料须包含 P0/P1/A output-conditioned 全视图 simulator/hybrid、T preprocessing 的联合分布论证、SQLite crash/commit/recovery 合同证明，以及量化的随机数和认证通道优势项，并由未参与实现的技术接收者独立审查。
