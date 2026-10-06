# BMW16 S10：raw-score 正式适配、UID 隔离与安全门裁决

- 日期：2026-10-06
- 方案：`Protocol I + BMW16-derived Select + DCF` / `PROJECT_DERIVED`
- 实现身份：`EXPERIMENTAL_TEST_ONLY_BMW16_S10_FORMAL_RAW_ADAPTER`
- 接收起点：S9 `a4402b1ef67d359a0d85997a86175497516ab20d`
- 状态：**TEST_ONLY 功能候选通过；安全/secure API 门仍 NO-GO；V3/V4 正式验收未完成**

## 决定

S10 在 S9 实验 runner 上新增一个默认关闭的 VFSS 实验适配器；它直接调用仓库正式 `protocol_i_raw_score_input_party`，再把正式 Protocol I priority key shares 映射成 S4 Select 所需的反向稳定键份额。P0、P1、离线 T 使用三个不同 Linux UID 运行。11 个 `n≤8` 冻结样本和一个 `n=64` 非全量抽样样本与仓库冻结 C++ oracle 一致；强制算法 abort、材料 abort、文件访问和材料一次性夹具按本文件报告。

该结论只表示 `TEST_ONLY` 功能和本地账户 ACL 证据。当前材料包没有 AEAD/来源认证；S10 root 测试控制器能访问输入与全池包；在线双方仍公开自适应图、比较位和 selected anonymous handle；同一 shuffle node mask 派生的完整多 key DCF/uCMP pool 与自适应读取仍无联合模拟。因此不创建生产/secure Top-K API，不宣称安全协议，不继承 BMW16 Theorem 8，不称 BB90 原算法实现。Select 的 R1–R4 四个明文依赖层不等于入口在线四轮。

用户明确授权在 V3 完成前推进隔离候选研发。该授权不改变 V3→V4 正式性能验收顺序，也不把 S10 标成 V3/V4 或六方案验收完成。

## 冻结起点与受保护状态

| 对象 | S10 复核状态 | 身份/说明 |
|---|---|---|
| `main` / `origin/main` | 同为 `c3926c68fd14f270faa8b55234311071947fa080` | 桌面工作区所在分支 `feat/m6a-performance-evaluation`；保留其原有工作区差异 |
| S9 接收点 | `a4402b1ef67d359a0d85997a86175497516ab20d` | S10 worktree `codex/m6b-i-bmw16-s10` 从此提交开始；开始时干净 |
| S4 Select | `04ed6f8a352ab2277434344094471dff82f83e4b` | 原算法文件 SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`；本阶段沿用 S9/S8 import 的共享副本，不修改 Select 语义 |
| S4–S9 决策/报告 | 已阅读 | S1–S3 的来源/算法审计，S4 的 TEST_ONLY Select，S5/S6/S7/S8/S9 安全门与原始验证均保留历史身份 |
| E20 | `8986a40175f5f8eef56d765fef0cfa863e205985`，干净 | 只作离线发材/存储/领取工程参考；其 AAV86 安全接收不迁移到本方案 |
| E21 | `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`，有既存文档/证据/脚本未提交差异 | 未改动；相关 worktree 保持原状 |
| PR #28 | head `57e239ee820a4208ac606663d877803f0b692206` | Draft M6A Protocol I+AAV86；本任务未更新或改动 PR |
| PDF 来源 | S9 登记：主论文 44 页、SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；辅助 `017.pdf` 249 页、SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15` | 主论文是 BMW16 算法来源；017 仅交叉核对；均未复制或加入本分支 |

S10 隔离 worktree 位于 `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s10\MoE_Top-k_Reproduction_Evaluation`。桌面主工作区、PR #28、E20/E21、`VFSS-baseline/`、论文及本地参考树均未修改。S10 是用户要求独立接收 S9 的短期分支，未把桌面主工作区的未提交改动混入。

## S9 独立接收复跑

在 S10 worktree 改源码之前，使用 S9 冻结的 `bmw16_s8_backend` 二进制和 S9 原 runner 重跑 11 行 raw-score 矩阵及其 n=64 固定向量：

| 结果 | 复跑 | 结果文件 SHA-256 |
|---|---:|---|
| S9 raw-score matrix | 11/11 SUCCESS、11/11 frozen oracle PASS | `368d23fde67f1747faaa3789895dca5300df99df3b6d927fde62f94cebb17351` |
| S9 n=64 | SUCCESS、oracle PASS | `7e903d6e572f5e8a45f8694f7e02c3f6f7556714ab4f5fd1d16d1c1c34848e02` |
| S9 backend | SHA-256 `0989efa4b2458267f330237ef0027a87812356473b028de64c2c6a14ca3bc3cf` | 与 S9 manifest 一致 |

该复跑还原的是 S9 的 TEST_ONLY carry/sign lift 移植，不等于本轮正式 VFSS adapter E2E。S10 的新路径另行调用官方 raw-score party 函数并重新做 conformance 和 oracle differential。完整 S9 运行 JSON 保留在本机 `%TEMP%`，文件哈希列入 S10 证据摘要，没有把大 JSON 放入 Git。

## 输入与稳定键适配

原始输入仍为 signed Q20.12 二补码原始 32-bit word 两方加法份额，满足 `raw=(x0+x1) mod 2^32`。S10 每个 party worker 调用正式 `protocol_i_raw_score_input_party`，输入正式原始份额和材料；测试控制器不自行计算 carry、sign 或 Select key。

令 `q = raw xor 0x80000000`、`b=max(1,ceil(log2 n))`。正式 Protocol I priority key 为

```text
P_i = (UINT32_MAX - q_i) * 2^b + original_index_i
```

它按项目稳定优先级升序排列。BMW16/S4-derived Select 使用反向顺序。新 opt-in adapter 令

```text
C   = UINT32_MAX * 2^b + (n - 1)
S_i = C - P_i = q_i * 2^b + (n - 1 - original_index_i)
```

并在 `Z/(2^(33+b))` 内变换 shares：P0 加公开常数 `C`，P1 加 0，再各自减 priority-key share。对真实元素 `q∈[0,2^32−1]`、`index∈[0,n−1]`，`0≤S_i≤2^(32+b)−1`，低 b 位编码原始 index 的逆序 tie break，高位编码 signed score 顺序；INT32_MIN/MAX、重复值和全相等均不碰撞。n≤64 的本轮最大宽度为 `33+6=39` 位。

适配器只包装正式 input party API 的输出；仍是实验功能入口。它没有把 Select、公开比较图、阈值 membership 或逆路由接进 VFSS secure runtime。两个正式输入转换阶段复用一条 full-duplex socket，但传给 `ProtocolIFramedChannel` 的两个 fd 必须独立：channel 析构会关闭自己拥有的 fd。S10 用 `dup()` 给 carry 和 sign 阶段各自一个 descriptor，避免 sign exchange 在第一阶段结束后使用已关闭 fd。

## T / P0 / P1 的执行隔离结果

实际 WSL 身份：T=`moeaudit` UID 1000；P0=`nobody` UID 65534；P1=`www-data` UID 33。实验 root harness 负责创建 ext4 `/tmp` 临时目录、分方安装文件与运行 oracle；它是 TEST_ONLY 编排器，不属于协议角色，也能读取测试输入、双方输出与包，故不能把此实验称作正式私有发材部署。

- T 使用独立 UID 运行 offline generator，只输入公开 `n,K,bits,session,fingerprint` 与两个 T 输出路径。generator 子进程完成并退出后，root TEST_ONLY harness 才创建 raw share 输入文件。
- 输入先落在 root-only controller stage；T 不能读取该目录或复制到 P0/P1 的输入份额文件。party 输入、结果和 package 分别落在独立 owner 的 `0700` 目录；两个 P 之间互不可遍历。party 在启动时设 `umask(077)`。
- 对 n≥2 的每个最终 E2E 样本，P0/P1 对对方输入文件、离线材料文件和结果文件的 `test -r` 均失败；两方不能列出对方 party 目录；两方不能读取 T stage；T 不能读取后置创建的 controller/party input 文件。
- `/proc` 在本 WSL 配置下允许对端读取 Python 子进程 command line。每个 party 的 argv 只有本方输入/package 路径、公开参数和 socket/测试标志，不带对方文件路径、share words 或 FSS key bytes；但对端可看到本方文件路径/PID，这是实际本地 process-metadata 暴露，不能写成“进程参数完全不可见”。分方文件权限仍阻止读取内容。
- 本地 package 交付是 root TEST_ONLY harness 使用 `install` 将离线 T 输出复制到 P 私有目录。该步骤没有 AEAD、T 签名或材料内部端点关系认证，不是可部署的 T→P 私有认证通道；通过的只是 party 间 OS 文件访问边界和包字段 conformance。

`BMW16_S10_UID_ISOLATED_TWO_WORLD_OS_ACCESS_TEST_ONLY` 重新构造 S9 的 `n=2,K=1` 两世界访问设定。World A/B 的 P0 本地份额文件 hash 相同，理想 P0 输出 share 文件 hash 相同；P1 输入/材料不同。两世界中 P0 均不能读取 P1 input/package、不能列出 P1 目录或读取 T stage；T 不能读取 P0 input。此为文件系统访问实验，不模拟完整 P0/P1 协议视图，也不测试远程认证信道。

## 完整 party-view 安全裁决

本候选的在线测试 transcript 对两方都公开 `R1–R4` 匿名边、比较位、派生 pivot/U/V/W、算法状态、selected anonymous handle 和中止类别；最终只要求双方输出本地原序 mask share。实际 party 代码也会把 `selected_handle` 放进最终对等消息/测试结果。该泄露函数尚未经项目隐私目标批准，也没有 S9 所需的全视图模拟器。

| 视图项 | S10 现状 | 裁决 |
|---|---|---|
| 本地 raw shares 与原序 key shares | 各方只从本地输入文件读本方 share；正式输入阶段内部 shares 不明文重构 | 本地文件隔离通过；root TEST_ONLY harness 仍持有 clear fixture/splits |
| Shuffle 状态/消息和共享真实 handle | 同两实例共享真实 handle；公开 masked value/transcript；未对 local shuffle state+messages 建模拟 | 未证明 |
| 自适应图、比较位、U/V/W、selected handle、abort | 两方可见；属于实验泄露 | 未证明满足项目隐私目标 |
| 全量本方 DCF/uCMP key pool | key pool 在相同 shuffle-node masks 上相关；各 edge/task 有独立 keygen，但阈值不独立 | 没有覆盖整池、访问/未访问槽、自适应访问的 hybrid |
| DCF membership 和逆路由 | 测试功能有结果；同置换逆路由后由 runner 重构 oracle | 功能通过不构成一方视图证明 |
| 输出份额 | party 本地结果文件受 UID ACL 保护；root runner 读取重构 | secure API 不存在 |

精确阻塞是“基于当前 Protocol I shuffle 实现，在一个 semi-honest party 的 view 中，联合模拟 correlated multi-key DCF/uCMP pool、adaptive slot access/transcript、shuffle messages/state、abort 和最终 XOR output share”的引理缺失；并且 offline handoff 尚需对 session/party/stage/task/round/endpoint/width/mask/slot 的加密认证包格式。S10 不据此宣称不可能，也不把 `n≤5` transcript enumeration、单 DCF key 安全或文件 ACL 当作联合证明。

## 门禁

| Gate | 结论 |
|---|---|
| `S9_INDEPENDENT_ACCEPTANCE` | PASS_REPRODUCED：S9 原路径 11/11 + n64 rerun 通过 |
| `RAW_SCORE_ABI` | PASS_CONFORMANCE_AND_TEST_ONLY_ADAPTER：官方 adapter CTest 1/1；S10 composed n≤8、n64 调用正式 party function |
| `EXECUTION_IDENTITY_ISOLATION` | PASS_LOCAL_UID_ACL_EXPERIMENT：UID/file/result/T-stage access checks；不是远程通道/正式部署接收 |
| `OFFLINE_PRIVATE_MATERIAL` | NO_GO：本地 root staging 可供测试；未有 T→P AEAD/来源认证和完整数学绑定认证 |
| `FUNCTIONAL_COMPOSITION` | PASS_TEST_ONLY：矩阵 + n64 与 frozen C++ oracle 一致；不代表成功概率上界或密码学正确性证明 |
| `ADAPTIVE_TRANSCRIPT_VIEW` | FINITE_PUBLIC_VIEW_ONLY：保留 S9 有限证据；joint party view 未证明 |
| `FULL_POOL_KEY_VIEW` | NO_GO：依赖相关 key pool 的多 key 自适应模拟仍缺 |
| `AGREED_ABORT` | PASS_TEST_ONLY_INJECTED_AND_MATERIAL：强制概率 abort 双方一致且无 mask；材料错误双向一致且无 mask；peer silent 只证明存活方本地返回通信 abort |
| `SMALL_N_SECURE_RUNTIME` | NOT_IMPLEMENTED / NO_GO：无生产 secure API |
| `PERFORMANCE_READY` / `V3` / `V4` / 六方案 | NO：不运行正式 LAN/WAN 矩阵，不将 loopback 数字登记为正式性能 |
