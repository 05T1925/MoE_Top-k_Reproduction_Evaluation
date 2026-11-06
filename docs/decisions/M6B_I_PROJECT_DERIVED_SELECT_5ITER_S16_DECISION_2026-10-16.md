# M6B-I-PROJECT-SELECT-5ITER-S16 决策记录

日期：2026-10-16  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决策

- S15 默认关闭修复源码快照共 72 项，manifest 内 canonical hash 为 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`；S16 在新 worktree 逐项复核 72/72 文件和 manifest digest，并从该快照重建 ON/OFF Release。
- 本执行者撰写了 S15 默认关闭修复，故 S16 接收门记为 `NOT_INDEPENDENT`。S16 记录的是隔离 worktree 的同会话技术复跑，不冒称异会话独立接收；S15 需要尚未参与修复的接收者再次核验。
- `DEFAULT_OFF_API_BOUNDARY=PASS`（同会话技术复核）：OFF archive 未含 BCG API 符号，未启用 API 头探针按预期编译失败、OFF target help 不含 Select5/BCG target，旧 M2 uCMP 与 shuffle regression 通过；ON 外部 API 探针成功编译、链接、运行。未发现 S15 修复的可复现构建/API 缺陷，不改 runtime。
- N≤256 功能路径在 fresh S16 Release 二进制上复跑 13 个 frozen-oracle 成功样例、6 个 uniform abort/no-mask 样例、N=128 monolithic、N=256/N=257 预检，以及 6 个 crash-prefix 样例。崩溃步骤 5/6 可留下一个 party 的 8-byte 局部 share，但没有完整 mask；合同仍是 `no-complete-mask`，不是原子发布/all-or-nothing。
- `DCF_SINGLE_KEY_PRIVACY=CONDITIONAL`。BCG+21 Figure 1/Theorem 2 对固定 `(alpha,beta)` 的单 key 语义分布与 M2B1/v1 可作条件映射，优势含 AES-128 PRP 和未量化的 `epsilon_RNG_joint`；S16 复核不是独立证明接收，且该定理只覆盖 party-key 单 key。
- `UCMP_SINGLE_CALL_VIEW=CONDITIONAL` 仅限给定腐化方本次 invocation 输入 shares、真实打开的 masked operands/比较 bit、单个 DCF key 与两次本地 Eval 的局部 transcript。完整 raw input 到该 invocation 的连接尚不能代替 shuffle 引理。
- `POOL_AND_ADAPTIVE_ACCESS=CONDITIONAL` 仅是 DCF key/Eval 投影。全池 symbolic key 项可按实际槽数累加；要覆盖逐轮公开 sample seed 决定的自适应访问，`epsilon_RNG_joint` 的上下文必须覆盖 Dealer 产生完整 package 的随机带及在线逐轮 seed 的有序联合随机带。S15 package-only 表达式不得解释为已证明完整协议 transcript。
- `SHUFFLE_OUTPUT_COMPOSITION=OPEN`、`CRASH_ABORT_OUTPUT_VIEW=OPEN`（隐私模拟层面）、`SECURE_ALIAS_READY=NO_GO`。首个未闭合分布是实际 M2CM/v1 前向 shuffle 的同一方联合视图：`(sigma_b,tau_b,a_b,e_b,r_b)`、收到的两个 masked-record frame、shuffle share、公开 masked aggregate 的联合分布及其与后续公开比较 transcript 的条件关系，尚无仅以腐化方原始输入 share、理想输出 share 和获批 L 为输入的模拟引理。逆路由、membership output share、PREPARE/COMMIT、崩溃后 orphan share 也未纳入完整模拟。
- 保持 `N>256=NO_GO`，M6A V3=OPEN、V4/正式 LAN-WAN=NOT_ALLOWED/NOT_RUN。五个算法迭代、最多九个比较依赖层与实测完整消息 exchange 分开记录；实验路线不替换六方案中的 BB90，不称 BB90/BMW16。

## 最小后续任务

由一位未参与 S15 修复和 S16 报告的接收者，接收 S16 source/build manifest 并重跑 default-OFF/API 和功能目标；随后仅聚焦一项证明交付：针对实际 M2CM/v1 两轮 shuffle 消息与本地材料字段，分别构造 P0/P1 模拟器，证明其在给定该方输入 share、理想 output share 和批准 L 时与真实联合分布不可区分；将所得 shuffle 引理接到已条件化的单次 UCMP/整池 hybrid，再补逆路由和 orphan-share/no-complete-mask 阶段。独立复核未通过前不得升级安全门。