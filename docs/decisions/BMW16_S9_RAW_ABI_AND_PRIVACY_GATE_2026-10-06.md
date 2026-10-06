# BMW16 S9：raw-score ABI、匿名自适应视图与安全入口门

- 日期：2026-10-06
- 方案身份：`Protocol I + BMW16-derived Select + DCF` / `PROJECT_DERIVED`
- S9 实现身份：`EXPERIMENTAL_TEST_ONLY_BMW16_S9_RAW_ABI_CSPRNG`
- 依据起点：S8 `0ac6d5622fc60847f119672456d785ef4700e180`
- 结论：**S8 实验独立接收；raw-score 路径具备 TEST_ONLY 功能证据；安全入口门 NO-GO。没有修改或创建 VFSS secure runtime。**

## 1. 冻结边界和来源身份

本决策承接 S8，不改写 S1–S8 历史结论，也不把实验结果反写为论文定义。`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY` 是 S4 项目推导的 Select；S9 复用 S8 中从 S7 提交带入的同一份 `select4r_s4_source.py`，没有分叉维护第二份算法。BMW16 Algorithm 5/7、Theorem 8 与 Appendix A Lemma 1 的原文关系和既有推导仍由 S4 决策/报告引用；S9 不继承 Theorem 8 的概率或比较常数。

| 项目 | 冻结身份 | S9 用途和边界 |
|---|---|---|
| main / origin/main | `c3926c68fd14f270faa8b55234311071947fa080` | S8 从此基线分出；S9 按用户要求在 S8 提交上建立隔离分支，不重置或改动桌面主工作区 |
| S8 起点 | `0ac6d5622fc60847f119672456d785ef4700e180` | 本 worktree `codex/m6b-i-bmw16-s9` 的初始 HEAD |
| S4 算法原文件 | `04ed6f8a352ab2277434344094471dff82f83e4b:experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py`；SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` | S4 原始项目推导来源，不称 BB90、不声称作者原实现 |
| S7/S8 复用副本 | `experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py`；S7/S8 Git blob SHA-256 `d9a4bba123cfdccf01b9ff3451c6622b41d63bcf90cf1709c66a3af7c4fcd3bb`；S9 checkout 原始字节 SHA-256 `39757567f0107be82859e1123d06c370f9f2e057e1d175242ae2cfbcae06476c` | S9 实际 import 的唯一副本；两者差异是 Git LF blob 与 Windows checkout CRLF 字节，不是第二份实现 |
| S5 / S6 / S7 | `67c3f302a8564b68616f7d7247c8cee3415ace31` / `c4f34e7e8f9fd954637573fe62b8d9b567362298` / `0dbfc8d99b3e896329611793ef95ea822e241a67` | 保留历史决策和证据 |
| E20 | `8986a40175f5f8eef56d765fef0cfa863e205985`；实现源 `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f` | 阅读离线流式发材、AEAD、OS 身份和一次性领取合同；未移植其 AAV86 专属 runtime/接收结论 |
| BMW16 主 PDF / 辅助 PDF | 44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23` / 249 页，SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15` | 前者是主要算法来源；后者只作交叉核对。均未纳入 Git |

来源复核说明：桌面 `Papers/017.pdf` 只读 `Get-FileHash`、`certutil -hashfile` 和仓库 `docs/PAPERS.sha256` 三者一致为 `...71EEA8C040...`。S8 报告曾记录为 `...71EE8C040...`，属于报告字符串转录差异；S9 采用文件与 manifest 的实测 hash，不据此改变论文来源等级。fitz 只读页数为 249 和 44；`Papers/017.pdf` 仍不是独立证明来源。

源码复核说明：`git show <S4>:.../select4r.py` 的原文件 SHA 为 `8644...`；S7 提取为共享源文件后的 Git blob 为 `d9a4...`，S9 Windows checkout 文件字节 SHA 为 `3975...`（CRLF 转换）。S9 实际运行的是 S7/S8 副本，并未在本轮修改它；以上不同 SHA 是来源文件/路径及行尾表示差异，不代表算法源重新分叉。

修改只在隔离实验代码与 S9 文档/证据索引内；未触及 `VFSS/`、`VFSS-baseline/`、旧 Protocol I/AAV86、E20/E21、PR #28、论文、桌面主工作区差异或参考树。S9 源码与构建二进制哈希见验证报告和 `BMW16_S9_SHA256SUMS.txt`。

## 2. raw-score ABI 的实验接线

仓库正式输入约定是两个 `uint32_t` 向量，满足 `x=(x0+x1) mod 2^32`，按二补码解释 signed Q20.12；稳定优先级在原输入索引上定义，不能用 shuffle slot 代替该索引。正式接口位于 `protocol_i_raw_score_input_party`，输出现有 Protocol I priority-key 环中的份额。

S9 在 TEST_ONLY 后端内逐步复用了同一转换等式，但没有调用正式 party adapter、正式 framed transport 或正式 package。对每个补齐槽，令 `a=x0`、`b=x1`：

1. carry/uCMP 阶段两方局部输入相加后分别为 `MAX−b` 和 `a`（未持有对应项的一方贡献 0）；严格比较得到 `c=[MAX−b<a]`，即 32 位加法 carry。随后各方计算其份额上的 `a+b−c·2^32 (mod 2^34)`。
2. sign/uCMP 阶段比较 `INT32_MAX` 与该 34 位 lift，得到 signed sign bit；使用它将二补码 raw word 映到 `ordered=raw xor 0x80000000`。
3. 正式 Protocol I priority key 为 `P_i=(UINT32_MAX−ordered_i)·2^b+original_index`，其中 `b=max(1,ceil(log2 n))`。BMW16 Select 使用相反的严格次序，S9 由公开常数 `C=(UINT32_MAX)·2^b+(n−1)` 得 `S_i=C−P_i=ordered_i·2^b+(n−1−original_index)`。

对 `n≤64`，实验 uCMP 比较位宽为 `33+b`，原始 carry/sign 比较固定 34 位；n=64 时 `b=6`、Select 比较宽 39 位。原始合法 signed 边界映射到无符号 `ordered` 的 `[0,2^32−1]`，索引域是 `[0,n−1]`；因此该拼接没有合法值碰撞，索引 tie-break 始终是原始 index。S9 扩展仓库现有 `moe_topk_m2_score_input_conformance_test`，真实调用正式 `protocol_i_raw_score_input_party`，两方线程通过两条 socketpair framed channel 执行 carry/sign 两阶段，并核对 n=1/2/3/5/8 的极值、重复值 key shares 与 frozen priority-key oracle；该 conformance target 通过。它不是独立进程 secure composition。S9 另有一份 carry/sign 等式移植到隔离 worker，并以 frozen C++ Top-K oracle 做端到端 differential。正式函数 conformance 与移植组合 differential 证据层不同；S9 没有将测试移植直接改成生产 secure API。

FSS 源码检查：vendor `osuCrypto::PRNG` 默认构造确实处于未 keyed 状态；S9 backend 在任何 `keyGenDCF` 前先调用 `protocol_i_parallel_shuffle_dealer_generate`，该函数先以 Linux `getrandom` 为 `FSSConfig::prngs[0..255]` 各自 `SetSeed`。因此此 TEST_ONLY dealer 的 DCF keygen 使用 OS entropy 播种后的流；conformance 单元测试则显式用固定 test seed 初始化。此检查不替代生产熵源/并发/重启/跨会话审计，也不为 parties 单方视图提供证明。

n=1 走已有 TEST_ONLY singleton shortcut，不调用 T 或 carry/sign 材料；其唯一 mask 位在此有限域正确，但不构成一般协议的材料接收证据。`n≥2` 的 TEST_ONLY runner 调 T 先生成和写出两方全池，再创建 raw input share 文件并启动两个 party 进程；记录显示 T 子进程已返回、online `keygen=false`。这验证实验控制流，尚不验证有隔离身份的安全发材。

## 3. 随机性、匿名图和单方视图命题

### 3.1 允许泄露草案（未被项目隐私决策接收）

待审的理想接口草案公开 `n,K,session`、四层匿名 handle 图、比较位、由此导出的抽样/pivot/U/V/W、选中匿名 handle 及 abort 类别；仅各方本地输出原序 mask share。原始记录的 original index 不属于公开 handle，两个 Select 实例共享同一批真实 handle，每个实例的 dummy/sentinel 用 task-local 身份。该草案不是既有 Protocol I 泄露定理，也未证明满足项目最终隐私目标。

对**纯公开 transcript**，可以给出置换不变性论证：固定任一严格全序的记录序列 `σ`，让隐藏映射 `π` 从 `S_n` 均匀抽样；算法若在所有真实 handle 上置换等变、抽样币与输入独立、两实例共用同一个 `π`、task dummy 身份独立，则所有 adaptive 边、边结果、派生集合、选中 handle 与 abort 都是输入顺序经均匀重命名后的函数。因此 transcript 的分布不依赖 `σ`。此论证仅覆盖规定的公开字段，不能自动模拟局部 shuffle 状态、FSS keys、访问槽或最终 share。

独立枚举器对两个相反输入序、固定腐化方全零 shares、同一共享匿名句柄置换、两个 Select task、R1–R4 自适应边/结果、选中 handle 和 abort 做全排列检查：n=2、3、4、5 的所有 `n!` handle 排列在两个固定 algorithm seed 下 transcript multiset 均相同，P0/P1 理想输出份额边际也相同。有限枚举仅支持上面的公开 transcript 命题，**不包含本地 shuffle state/messages、相关全池 key 或自适应访问历史，不能代替一般模拟证明**。

### 3.2 可复现同 OS 身份攻击

实际 TEST_ONLY harness 在同一 OS 用户/令牌下运行 T、P0、P1，输入 shares 与 party package 放在同一临时目录。S8/S9 的 `chmod 0600` 在同一身份下只表示 owner-only，不隔离同身份进程。S9 的 party 诊断选项由 P0/P1 自己尝试读取 sibling input/share 和 sibling package；两个 party 均实际读取成功。在 raw-score 模式下这直接使另一方 share 可见并可重构整份 raw 输入。

保存的两世界夹具：`n=2,K=1`；World A score `[0,1]`，World B `[1,0]`；P0 输入 share 文件在两个世界字节相同，理想 P0 输出 share 也耦合成相同 `[1,0]`。同 OS token 的 P0 读取 P1 share 后恢复各自 score，事件 `score[0] < score[1]` 在 A 为真、B 为假，区分优势为 1。这个反例只针对当前同主机可读 sibling 文件的执行面；它不推出不同 OS 身份/主机且私有通道时也必泄漏。

### 3.3 完整相关 key pool 仍缺一般混合

每个 uCMP pair key 的阈值由同一 shuffle 节点 mask 派生，形如 `α_ij=(r_i−r_j) mod 2^b`。不同 edge/round/task/membership 的 key 是独立 keygen 调用，但阈值共用同一个 r 向量；S9 没有证明完整 party-key pool 与公共自适应访问、局部 shuffle material/messages、公开 masked z、abort 和输出 mask share 联合可模拟。不能把“每一个 DCF key 单独安全”或 n≤5 transcript 枚举当作多 key 组合证明。

完整腐化方视图需至少覆盖：本地输入 shares 与 raw lift、forward/inverse shuffle 状态和收发、同两任务真实 handle 关系、该方整个 uCMP/DCF pool（已读/未读槽）、公开图/比较输出/selected handle、错误/abort 和输出 mask share。没有完整 simulator/hybrid，远端独立身份模型下 `PARTY_VIEW_SECURITY` 仍 `NO_GO`。

### 3.4 OS、包和发材

目前 package 为 S9 自定义序列化文件，含 session/fingerprint、party、stage/round/task/endpoint/width/mask 绑定字段及 party DCF key；party backend 本地校验字段并用 `.claimed` O_EXCL 标记单次消费。它**不是** AEAD：外层标签没有认证 key 内隐含的端点数学关系，也不能防止能写入文件的主体替换 package。双方间 loopback TCP 也没有在 S9 中提供身份认证/机密性。E20 明确要求 OS 身份隔离/私有 T→P 完整交付，且同 uid 的目录权限不足以隔离；该合同适用的最小需求可以沿用，但其 AAV86 包布局不能直接复用为 BMW S9 全池。

要得到可审查的离线私有发材，需要独立 P0/P1 OS 主体或独立主机、T→P0/P1 私有完整通道、每方本地持有 AEAD 解封密钥、认证完整 package/slot manifest、session/party/task/round/endpoint/width/material-id 绑定、原子一次性消费，以及输入前 T 退出的可验证编排。所有在线比较材料须在不知输入和未来 adaptive 图时预先生成。当前代码没有这个发材通道，故 `OFFLINE_PRIVATE_MATERIAL=NO_GO`。

## 4. S9 实验内容及概率边界

`--joint-csprng-coins` 仅在 TEST_ONLY runner 中将两方 `secrets.token_bytes(32)` 贡献 XOR、SHA-256 域分离后送入 HMAC-SHA256 counter stream；每个 task/round 使用独立域，通过 rejection `randbelow` 和 partial Fisher–Yates 无放回抽样。理想独立均匀贡献下 XOR 种子均匀；这里没有 commit/reveal，因此不提供恶意方抗偏置，也没有认证信道。OS CSPRNG/HMAC-PRF 的计算安全假设与算法抽样失败概率分开陈述。

S4-derived sample-bracket 的失败概率没有从 BMW16 Theorem 8 借用，也没有给出 `≤10^-6` 证明。S9 固定矩阵的成功频率只是在已测样本上的观察值。`--test-only-force-algorithm-abort` 是在选择结果之后人为注入 `ABORT_ALGORITHM_PROBABILITY` 的无 mask 测试夹具，不是自然随机失败种子，不能估计失败率。自然算法失败率在这批样本中观察为 0；该观察不是概率上界。算法失败、材料拒绝和通信错误分别输出不同状态；掉线方未收到 ACK 时不宣称双方确认了一致 abort。

## 5. 计量口径和全池覆盖

计数严格区分逻辑比较、边/去重、真实边、dummy 相关边、每方材料槽和底层 DCF Eval。S9 不跨 round/task/membership 缓存 FSS key；仅同轮反向请求可由已验证的 Z/(2^64) 份额补数本地复用，逻辑比较仍计数。未用 shuffle CmpAgg keys 没有移除，计入 T 输出和 package 字节。

对于一个 n 节点实例，离线池在各 Select task 各预发四轮全部无序真实 handle pair，另预发 membership 全部无序 pair；两份 forward/inverse shuffle package 还各自含 CmpAgg pair pool。令 `C=binom(n,2)`、`P=next_power_of_two(n)`，原始输入转换另有 `2P` 个 raw uCMP slots/party。报告槽数是 package 当前序列化内容的**实际逻辑槽数**，不是 Eval 数、时间或安全覆盖证明。n=64 实测：`8C=16,128` Select pool、`C=2,016` membership pool、`2C=4,032` 未用 shuffle CmpAgg slots、`2P=128` raw carry/sign slots，共 `22,304` 槽/party。在线实际 Select 消费 5,350 key slots，membership 消费 63 slots；raw 输入阶段消费 128 slots。uCMP 每方每槽 2 次 DCF Eval，双方便是 4 次，因此 n=64 的 Select+membership 两方 DCF Eval 为 `4×(5,350+63)=21,652`，raw 输入为 `4×128=512`，合计 22,164。逻辑 Select 边为 R1/R2/R3/R4=`8,010/500/11,362/2`，合计 19,874；真实—真实边=`2,118/126/3,104/2`，dummy 相关边=`5,892/374/8,258/0`。n=64 pool 文件实测 `22,723,604` bytes/party。

这不是最坏情况/渐近覆盖证明，也不是 benchmark。Select 候选每层根据前轮公开结果从有限输入域选边，而每 task 四轮的离线全对池使所有可能的规范真实 pair 可查询；双 task 使用共同真实句柄和 task 区分的槽位。dummy 边在逻辑图计数内，但明文 dummy/dummy 结果本地生成；真实/dummy 边也由公开 sentinel 类型规则决定，不持有真实记录 key。该覆盖论证只说明当前实验表中的地址可查询；不能弥补 pool 联合安全和完整材料认证缺口。

完整 raw-score→mask DAG 在验证报告列出。n≥2 实测 message phases 是 direct legacy S8 13；raw-score adapter 加 2；joint coin 加 1，所以 S9 raw+n64+joint 为 16。选择器恰有 R1–R4 四层；它们不是入口在线轮数。DCFs、shuffle、membership、逆路由、协调分别计数；在线时间、offline T 时间、生产网络、PRG、正式总时延和 LAN/WAN 均为 `NOT_MEASURED`。

## 6. 门禁

| Gate | 判定 | 依据 |
|---|---|---|
| `S8_INDEPENDENT_ACCEPTANCE` | `PASS_EXPERIMENTAL` | 在 S9 worktree 独立重建运行 11/11 S8 matrix、n=64 一例；边审计 12/12 |
| `RAW_SCORE_ABI` | `PASS_FORMAL_ADAPTER_CONFORMANCE_AND_EXPERIMENTAL_FUNCTIONAL / SECURE_COMPOSITION_NOT_ACCEPTED` | 官方 adapter 真实 socketpair conformance PASS；S9 carry/sign 移植与冻结 C++ oracle 11/11+n64 PASS，另有 INT32 边界 n5；未接入 Select 生产/secure runtime |
| `OFFLINE_PRIVATE_MATERIAL` | `NO_GO` | 同 OS token 对方 package/input 可读；无私有 AEAD 完整发材链路 |
| `ADAPTIVE_TRANSCRIPT_VIEW` | `PASS_FINITE_PUBLIC_TRANSCRIPT_ONLY / FULL_VIEW_NO_GO` | n2–5 全置换枚举与置换等变论证只覆盖公开 transcript |
| `FULL_POOL_KEY_VIEW` | `NO_GO` | 同 r 节点 mask 相关的整个 party DCF/uCMP pool 与 adaptive access 的联合 simulator 缺失 |
| `SAME_PI_INVERSE_MASK` | `PASS_EXPERIMENTAL_FUNCTIONAL` | 样本的两阶段 shuffle/逆路由和 XOR mask 外部冻结 C++ oracle differential 通过；party 不重构，TEST_ONLY verifier 重构 |
| `AGREED_ABORT` | `PASS_TEST_ONLY_INJECTED / PRODUCTION_NOT_ACCEPTED` | 注入概率 abort 时双方状态/no-mask 一致；同 OS file leak 和 transport/channel error 全覆盖未解决 |
| `SMALL_N_SECURE_RUNTIME` | `NOT_IMPLEMENTED / NO_GO` | 没有新增 `VFSS/` secure API；隔离实验保留 TEST_ONLY 标签 |
| `PERFORMANCE_READY` | `NO` | Loopback 冒烟与包字节不是正式网络矩阵；安全门未过 |

## 7. 允许进行下一步的精确条件

下阶段不得以“再写一份安全不足报告”替代工程验证。最小可测条件是：把 P0/P1 放在不可互读的不同 OS 身份或主机；通过 T→party 私有、认证、AEAD 完整材料通道交付绑定全池；确保输入后 T 关闭且不能补发；然后对当前真实 shuffle 实现给出包括本地状态/消息、同 r 相关全 pool、 adaptive key 访问、abort 和 mask share 的单方 simulator/hybrid。需保留同一套冻结样本并运行两世界视图测试、篡改/错绑定/重放/重复消费/对端失联测试。只在这些条件形成可审查的条件性 GO 后才可创建正式 secure 入口；正式性能矩阵仍须等待既有 M6A/V4 前置条件，并重新测量。
