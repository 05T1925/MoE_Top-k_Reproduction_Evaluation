# BMW16 S8：离线材料、三进程组合与匿名自适应图决策

- 日期：2026-10-06
- 状态：`FUNCTIONAL_TEST_ONLY_GO / PARTY_VIEW_SECURITY_NO_GO`
- 方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**
- 实现身份：`EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS`

## 决策

S8 把 S7 的集中式功能路径改成可运行的离线发材、T/P0/P1 分进程实验：T 在任何 party 输入文件创建前产生两方材料并退出；P0、P1 各自读取一个本地加法份额文件，以独立进程交换实际 Protocol I shuffle 消息、四层 Select 的公开比较 transcript、DCF 输出份额，并输出本地 mask share。测试 harness 在进程外重构 mask，仅用于与仓库 C++ `top_k_mask` oracle 核对。

这是一项 **TEST_ONLY 功能实现**，没有通过联合视图安全门。它没有添加 VFSS secure API，不进入生产运行时，不是“安全协议”或正式性能数据，不称为 BB90 实现，也不继承 BMW16 Theorem 8 的概率/比较常数。四个 Select 比较依赖层不等于完整入口四轮。

## 起点、版本关系与来源

| 项目 | 核验结果 |
|---|---|
| S8 工作分支基线 | `main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`；隔离分支 `codex/m6b-i-bmw16-s8` |
| S7 内容来源 | 用户指定 `0dbfc8d99b3e896329611793ef95ea822e241a67`；从该提交选择性 cherry-pick 的本地父提交为 `c0d0e1237037ca9d73496e3ebc9785d8b2c6b4e9`，沿用 S7 的文件树与来源标记 |
| S4 算法来源 | `04ed6f8a352ab2277434344094471dff82f83e4b`；S4 源文件 SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`；本次直接复用 S7 中的 `select4r_s4_source.py`，不维护第二份 Select 源码 |
| S5/S6 | 接收 revision `67c3f302a8564b68616f7d7247c8cee3415ace31`；审计 revision `c4f34e7e8f9fd954637573fe62b8d9b567362298`，保留既有历史结论 |
| S7 独立复核来源 | `0dbfc8d99b3e896329611793ef95ea822e241a67`；S7 的源文件、矩阵、8 条位宽旧错误及 n=8 轨迹未改写 |
| E20 工程参考 | `8986a40175f5f8eef56d765fef0cfa863e205985`；流式实现来源 `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f`；独立接收仅限 E20 自身接口，不转移为本方案安全证据。本次没有复制 E20 runtime |
| 论文 | `Papers/1603.04941v1.pdf`，44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；`Papers/017.pdf`，249 页，SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15`。本轮不把后者或论文定理写成 S8 组合安全证明 |

用户桌面主工作区、PR #28、`VFSS-baseline/`、论文与本地参考树均不在本分支修改范围。

## S7 复核与 S8 修正

| S7 项目 | S8 裁决 |
|---|---|
| 最终功能矩阵 | 独立检查 S7 10/10 成功结果；位宽修正前另有 8 条 `COMPLETED_WRONG/NO_MASK`，其根因是实际 uCMP 需要 `34+b` 而非 `33+b` 位，不把它们算成算法概率失败。S8 使用真实 uCMP 并重新跑自己的矩阵 |
| n=8, K=3 | S7 seed42 集中式轨迹选 original index 7，oracle mask `[0,1,0,0,0,1,0,1]`，R1–R4 `132/44/2/2` 逻辑边、178 Select key slots、712 Select Eval、8 membership slots、28 unused shuffle CmpAgg slots；S7 还为 dummy 边做 FSS，S8 则本地按类别比较 dummy，只为真实边用 FSS，分进程实测 86 Select slots、7 membership slots、372 Eval。S7/S8 计数合同不同，不作性能比较；shuffle handle 也不同，不能把 S7 handle 当常量 |
| 公开原记录端点反例 | 若端点能关联 original ID，比较位确实区分两种分数世界；但它不适用于本实现公开的匿名句柄。S8 没有打开 `original_index` 或 shuffle map |
| 匿名固定图枚举 | 不足以覆盖自适应 transcript；S8 新增精确枚举，模拟两个 Select 实例共享真实 handle，纳入 R1–R4 的逐轮活动图、比较结果、派生状态、选择 handle 与 abort class。它只检验公开 transcript 的有限分布，不包括真实 party key/shuffle/output-share 联合视图 |
| 反向 `eval_strict_lt` | 实际 uCMP 返回 `c=[z_l<z_r]` 的 Z/(2^64) 份额。规范 key 对反向请求本地转换 `(c0,c1) -> (1-c0,-c1)`；只在同一 round/task/cache 域使用。跨轮、跨 Select task、membership 均领取新槽 |
| `D=4096` | 不是 n≤64 每实例活动句柄池的必要大小。S8 用两个共享真实 handle 的 Select task 与 task-local dummy；每个 task 的真实无序边池为 `C(n,2)`，材料 ID 再区分 task、round、stage。S8 不声称这解决了多实例联合隐私 |

## 分方实验合同

角色与数据：

| 角色 | 输入/状态 | 本阶段执行内容 |
|---|---|---|
| T（测试中的离线材料生成进程） | `n,K,比较位宽,session,fingerprint,输出路径`；不接收 score 或 score shares | 生成第一份 Protocol I shuffle 材料、目标为同一隐藏置换逆映射的第二份 shuffle 材料、比较 key pool 和两方 package；进程退出后才创建两份在线输入文件 |
| P0/P1 | 各自一份 signed int32 raw-score 的加法份额，实验拆分环为 Z/(2^34) | 本地稳定键 lift；独立 party 进程；真实 C++ shuffle/uCMP/DCF；交换实际对端 shuffle 消息、每轮冻结 graph 和比较输出 shares；仅比较阶段打开 bit；独立逆路由并输出本地 XOR mask share |
| 测试 harness | 合成用例的 clear scores、种子、两个 process 的编排 | 创建份额/文件，运行 T 后等待其退出，再启动双方；收集结果后仅在 TEST_ONLY 重构 XOR mask 并调用 frozen C++ oracle |

目标信任设定是 T 可信且不与 P0/P1 合谋、在线 P0/P1 半诚实、T 在线静默；不要求 T 擦除。当前是同机实验进程与临时路径，并未建立独立主机/账号隔离、认证 channel 或正式秘密存储，所以该信任设定只是方案合同，不能由进程划分自动实现。

T 对外提供的材料池覆盖为：

```text
Select:    2 tasks × 4 rounds × C(n,2) = 8 C(n,2) slots / party
membership:                         C(n,2) slots / party
shuffle:  forward + inverse shuffle  2 C(n,2) unused CmpAgg slots / party
total serialized uCMP-key objects: 11 C(n,2) / party, plus shuffle state
```

端点 slot tuple 是 `(stage, round, task, lo, hi)`；package/slot 另校验 `party,bits,n,K,session,fingerprint,mask-id`。真实无序句柄为两个 Select 任务共享的 `0..n-1`，dummy/sentinel 属于任务内部公开类别，不占真实 key pool。每个 package 有 `O_CREAT|O_EXCL` 的实验性一次 claim 文件；进程内 key slot 只能使用一次。party helper 启动时把整个池读入进程内 map，不是 E20 式 AEAD 密封流式安全存储。

材料正确性覆盖仅针对当前实现标签与 n≤64 的全边预发池。公开活动图给出匿名端点与 slot 访问轨迹，所以该候选没有声称私有检索。两条相反方向逻辑调用同轮、同 task 共享同一规范槽并用 share-complement 转换；缓存不跨 round/task。keygen 在 T 离线发生，在线无补发/Dealer。

## 稳定键与成员谓词

测试输入为 `s_i ∈ signed-int32`、加法份额 `x_i^0+x_i^1 ≡ s_i (mod 2^34)`。令 `b=max(1,ceil(log2 n))`，party0 在 lift 中加入偏移与 tie-break：

```text
Q(s) = s + 2^31                  in [0, 2^32-1]
R(s,i) = Q(s)·2^b + (n-1-i)
comparison bits = 34+b           (n=64 uses 40)
```

因此升序 R 对应 signed score 升序、同分 original_index 降序；按高优先级取 Top-K 时，R 较大代表 score 较高且同分 original_index 较小。所有真实 `R` 唯一。两 sentinel 及 dummy 在 S4 TEST_ONLY 算法里由公开类别/序号排序；不会作为真实 membership 输出位。

最终 membership 对每个匿名真实 handle `i` 求 `[R_i >= R_selected]`，自比较由双方本地产生 share 1。规范 DCF key 是 `[R_lo<R_hi]`；对反向 membership 请求做 share complement，所以等值也纳入。若选择结果成功且稳定键唯一，恰 K 项满足谓词。反向 shuffle 对应同一 `π^{-1}`，逐项把 membership shares 路由回原输入顺序；最后各方取加法 bit share 的 LSB 形成 XOR share。本阶段正确性只由 oracle differential 样本确认，未建立共享键 lift 的生产输入 ABI，也未证明 secure 逆路由联合视图。

当前仓库 raw score 路径的生产份额环不是已审查的 Z/(2^34) 输入适配。S8 为保持 signed int32 lift 可解释，在 harness 里直接用 34-bit 随机拆分；不能把它写成现有生产输入 share ABI 已通过 conformance。

## 匿名自适应图：已裁决范围

设真实记录到 handle 的隐藏映射 `π` 均匀，输入上的严格稳定总序固定。对固定真实输入，按 handle 观察的 rank 向量是一个均匀排列。给定公开算法 seed，S4 的抽样、两 Select task 的每轮 graph、已打开比较位、U/V/W、selected anonymous handle、partition abort 和总 abort class 是该 rank 向量与公开 seed 的函数。故在理想均匀隐藏映射假设下，**仅此公开 transcript 的分布对真实输入总序不变**；两个 Select 共享 handle 的关系已纳入枚举，未把每个实例独立洗牌来获得结论。

独立审计器精确枚举 n=2..5 的所有 `n!` handle 排列；对每个 n 使用两个相反的严格输入序、algorithm seeds 42/43。观察到两世界在完整公开 R1–R4 adaptive graph/outcome、派生状态、selected handle 与 abort transcript 上的分布逐项相同；同时以理想均匀输出 share 做了本地输出边际枚举。这是有限实验核验。上述一般 transcript 论证还以实际 shuffle 产生均匀隐藏 π、party 端看不到清晰 π 及无额外相关状态为前提。

这**不闭合**腐化方联合视图：

```text
V_b = (本地输入 shares, 本地 shuffle state/material/messages,
       两实例共享真实 handle 关系, 完整本方 key pool,
       已读/未读槽及访问序列, public adaptive transcript,
       abort class, 本地最终 XOR mask share)
```

缺少对该完整 tuple 的 simulator/hybrid：尤其需要证明同一 mask vector 导致的整池相关 DCF party keys 在自适应选择 query/access 下的多实例隐藏性，并与 shuffle 局部状态、公开 `z_i`、输出 shares 联合；还需有当前 shuffle API 的一方完整 view 保密论证。只证明单个 DCF key 或匿名 transcript 边际不够。协议的许可泄露草案是 `n,K,session、四层比较公开图/位、selected anonymous handle、abort class、Top-K 输出 shares`；此名单尚未被项目隐私目标接收，因此**不得进入 secure 入口**。

具体材料相关性为：uCMP key 对每个规范真实端点 `(i,j)` 由同一 shuffle 的 `r_i,r_j` 生成，key 内阈值 `alpha_ij=(r_i-r_j) mod 2^bits`；在线比较使用相同 `z_i-z_j`。所以所有 task、round、membership 的 party-key pool 虽然由不同 DCF keygen 调用产生，仍共享同一 `r` 向量和匿名句柄标签；离线输入无关并不等于这些 key 在 party view 中统计独立。

## 小规模实际材料与计数

| 样本 | 状态/oracle | 比较宽度 | 逻辑 Select 调用 | Select uCMP slots/party | membership slots/party | DCF Eval 两方合计 | 离线全池槽/party | 包字节/party | 在线消息相位 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| n=2,K=1 极值 | SUCCESS/PASS | 35 | 28 | 6 | 1 | 28 | 11 | 10,641 | 13 |
| n=8,K=3 混合极值 | SUCCESS/PASS | 37 | 180 | 86 | 7 | 372 | 308 | 300,516 | 13 |
| n=64,K=2 固定随机输入 | SUCCESS/PASS | 40 | 19,371 | 5,407 | 63 | 21,880 | 22,176 | 23,139,976 | 13 |

这里逻辑比较调用、唯一实际规范 uCMP slot、两方 DCF Eval 与离线全池槽是不同计数。每个实际 uCMP key slot 的 party endpoint 各执行两次 DCF Eval，因此“两方 Eval”=4×(Select slots+membership slots)。本次最终 n64 轨迹的 Select 逻辑调用中，R1=8,010、R2=500、R3=10,843、R4=18；真实-真实边分别为 2,304、126、2,973、4，dummy 相关边为 5,706、374、7,870、14。R1 对每个 task 抽 90 项；单 task 全 M=126 pairwise 会有 `C(126,2)=7,875` 边，而样本边为 `C(90,2)=4,005`，因此该固定样本确实走了抽样而非整域全对比较。n64 实际包大小和消息为单次 TEST_ONLY loopback 观测值，不外推为性能结论。

两份真实 shuffle package 每方共含 `2C(n,2)` 个未使用 CmpAgg slots：n=8 是 56，n=64 是 4,032；已计入上表全池槽/文件字节，未冒充成 uCMP Eval。正式 PRG、离线 T 时间、受保护持久化字节、网络带宽/RTT与 LAN/WAN 数据均为 `NOT_MEASURED`。

## 在线因果与泄露合同

实际 TCP loopback 消息 DAG（n≥2）：

```text
T package generation → T exits
raw shares → local stable-key lift
forward two-round shuffle subprotocol: 2 peer message phases
R1: freeze graph → exchange DCF shares/open result (2 phases)
R2: previous results → freeze graph → exchange/open (2 phases)
R3: previous results → freeze graph → exchange/open (2 phases)
R4: previous results → freeze graph → exchange/open (2 phases)
membership: n−1 DCF slot evals, no peer open
inverse two-round shuffle subprotocol: 2 peer message phases
final status: 1 peer message phase
```

所以 n≥2 是 13 个 loopback message phases，另有本地处理；不是“完整入口 4 轮”。n=1 使用 TEST_ONLY singleton shortcut，不启动 T、无 shuffle/material，只有最终状态交换 1 phase；S4 仍生成四轮本地 dummy-only trace。选择阶段四个比较依赖层数另记为 4。

本实现仅调用 `ProtocolIParallelShuffleParty` 前两轮以形成 `π(x)` additive shares 和 `π(x)+r` masked values；没有调用 `evaluate_cmpagg_prepare_round3/receive_round3`。因此它是本项目测试性两轮 permutation subprotocol，不是仓库完整三轮 CmpAgg/reorder shuffle 的入口或安全复现；全池中该两份 shuffle 包的 CmpAgg edge keys 均未使用，但其实际生成和文件字节已经计入。

公开字段包括两方 shuffle 消息、公开 mask 后 `z`、每轮图/边句柄、比较结果、派生集合、匿名 selected handle 和成功/失败类别；不公开 original ID 或明文 score。P0/P1 会看到彼此比较输出 shares 并能重构比较 bit。测试 harness 明文持有分数/份额并重构输出，故不能混入实际安全视图结论。错误端点/party/轮次、package 截断、重复领取与重复 slot 的本地拒绝经过小夹具；peer silent 测试证明本地无 mask 的结构化 `ABORT_COMMUNICATION`，但缺席 peer 不可能在此用例中确认共同 abort。认证包、网络对端认证、跨会话重放、完整超时恢复和全错误路径的一致 abort 未验证。

在线字段逐项裁决：

| 字段 | 当前可见方 | 目的及安全状态 |
|---|---|---|
| 本地 raw-score shares、本方 shuffle material/state | 对应 party | 本地处理；完整 key-pool/状态联合视图未模拟 |
| forward/inverse shuffle 消息 | P0、P1 对端 | shuffle 状态机所需，必须进入联合 view 证明 |
| public masked `z_i` | 双方 | uCMP Select/membership 输入；单值 masking 不能替代与共享 mask key pool 的联合证明 |
| R1–R4 抽样、pivot、U/V/W、活动边、材料访问地址 | 双方 | 当前公开 adaptive graph 合同；枚举支持 transcript 输入无关，但访问隐私未另做隐藏 |
| DCF output shares 与比较结果 | 双方 | 比较阶段双方重构 bit 推进下一轮；比较 bit 是候选允许泄露 |
| selected anonymous handle | 双方 | membership 星形查询阈值；不含 original index，但与本地 shuffle view 联合仍待证明 |
| membership shares、inverse-route 消息 | 本地份额及对端协议消息 | party 不打开 mask；harness 会在 TEST_ONLY 外部重构 |
| session、成功/失败类别 | 双方 | 包/控制流绑定；失败泄露与一致 abort 仍待安全审查 |

## 安全和工程决定

1. **不创建 secure API。** 匿名图的边际均匀性只关闭一个过宽的泄露推断，不能替代联合 party-view simulator。
2. **离线发材合同仅在实验内成立。** T 不收到输入、生成完退出；party package 是未加密临时文件、由 harness 共享文件路径，不能算作安全分发/AEAD 存储。E20 的 AAV86 接收不转移到此处。
3. **一致 abort 仍未完全闭合。** 成功路径两方状态一致；一个已连接的错误-party package 测试现已确认双方均写 `ABORT_MATERIAL/no_mask`。算法失败时同 seed/同打开 transcript 应进入同一 final status，但本轮没有触发该路径；peer silence 只验证存活端输出结构化无 mask。丢包/对端退出后不能保证一个已断开的 peer 收到一致 abort acknowledgment。
4. **概率目标未证明。** 11 个 n≤8 矩阵样本 + 1 个 n64 样本均成功，但 algorithm RNG 是 Python `random.Random` 固定 seed；不是安全随机源。零失败样本不给 abort 上界，不能复用 BMW16 定理，也没有 `≤10^-6` 证明。
5. **当前输入 ABI 有差异。** 实验输入份额在 Z/(2^34)，需要与生产 raw-score shares 的域转换/overflow 语义 conformance 和证明。
6. **范围只到 n=64。** n64 pool 经本地生成验证；n>64 的 dealer/package 限制、生产材料容量、用户已有包上限及 53-bit comparator 边界尚需另审。n=10^6 的 `34+b` 仍超当前比较 API。

## 门禁

| 门 | S8 判定 |
|---|---|
| `FUNCTIONAL_E2E` | `PASS_EXPERIMENTAL_TEST_ONLY`：11/11 n≤8 + 1/1 n64 与冻结 C++ oracle 一致；P0/P1 各自进程、T 独立离线进程先退出 |
| `PARTY_VIEW_SECURITY` | `NO_GO`：公开匿名自适应图 transcript 的输入无关性获有限枚举和条件性论证；完整 shuffle + key pool/access + output shares 联合模拟缺失 |
| `OFFLINE_ONLY_MATERIAL` | `PASS_EXPERIMENTAL_CONTRACT_ONLY`：在线无 keygen/Dealer，全候选边预发且一次性消费；不是加密/隔离存储，完整相关池安全性未证明 |
| `ORIGINAL_ORDER_MASK` | `PASS_TEST_ONLY`：每成功样本 XOR mask 与 frozen C++ oracle 同序一致；harness 才重构校验 |
| `PERFORMANCE_READY` | `NO`：只有 loopback 冒烟计时/字节，不能进入正式 LAN/WAN 或六方案比较 |

进入正式 secure 入口前的最小阻塞是 `BMW16-S8-JOINT-POOL-SHUFFLE` 引理：在不额外泄漏比许可列表更多信息的前提下，对任意两份输入及固定单方 score shares，证明/实现可模拟上述完整 tuple，特别是同 mask 相关整池 DCF keys、公开自适应查询、shuffle party view 与 output mask share 的联合分布；若无法证明，改用可审查的隐藏图/秘密索引机制并重新计算轮数与材料，不把 ORAM/隐藏图成本视为零。

## 后续决策

保留本 TEST_ONLY 候选以支持安全审查与容量改进；不登记为已完成六方案、不修改生产 Protocol I/AAV86 入口、不开始正式性能矩阵。安全审查后若通过，应另开独立里程碑做异会话接收、生产域输入适配、材料完整性/私有分发、可靠一致 abort 和 n≥128 起统一 LAN/WAN 指标。
