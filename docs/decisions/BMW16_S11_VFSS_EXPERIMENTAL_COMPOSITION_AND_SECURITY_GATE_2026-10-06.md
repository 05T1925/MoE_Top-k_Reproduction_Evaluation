# BMW16 S11：VFSS C++ 实验组合与安全门

- 日期：2026-10-06
- 方案名：Protocol I + BMW16-derived Select + DCF
- 身份：`PROJECT_DERIVED / EXPERIMENTAL`
- 接收对象：S10 `8770c207cd8d0de9ebb1a3a694fe2ddbef3efac3`
- 本 worktree 起点：S10 提交；分支 `codex/m6b-i-bmw16-s11`
- 决议：**功能组合条件性通过；离线发材与单方联合视图门未通过；不得登记为 secure，不启动 V4 正式矩阵。**

## 1. 范围与身份

S11 新增了可调用的 opt-in VFSS C++ party-side 函数，把正式 Protocol I raw-score adapter、同置换前向 shuffle、两份并行的 S4-derived 四层 Select、稳定阈值 membership、同置换逆路由和 XOR bit-mask share 串成一条 API。T 侧也有输入无关的 C++ 离线材料生成函数。新增实现与测试始终显式为 `EXPERIMENTAL`；没有新增 `secure` 名称、生产默认入口或 BB90 标签。

本轮的 C++ party E2E 使用两条并发 party 调用和真实 VFSS uCMP/DCF key、shuffle state machine 与正式 score adapter；冻结 oracle 只由 TEST_ONLY 测试线程在 party 调用返回后核验。它不是 S10 Python party 控制器的重命名。但本轮尚未完成材料序列化/认证私有交付、不同 OS 身份的三进程 T/P0/P1 runner、持久消费账本或完整视图证明。故“完整 VFSS 功能链已可调用”不等于“可信 T 私有发材协议已完成”。

## 2. 起点、来源与保护

| 对象 | 身份/处理 |
|---|---|
| `main` / `origin/main` | `c3926c68fd14f270faa8b55234311071947fa080` |
| S10 | `8770c207cd8d0de9ebb1a3a694fe2ddbef3efac3`；S11 从此精确提交的独立 worktree 起步 |
| S4 Select | `04ed6f8a352ab2277434344094471dff82f83e4b`；被冻结的 `select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| S7/S8/S9 | `0dbfc8d99b3e896329611793ef95ea822e241a67` / `0ac6d5622fc60847f119672456d785ef4700e180` / `a4402b1ef67d359a0d85997a86175497516ab20d`；是 S4 TEST_ONLY 算法接线的历史来源，未改写其 NO-GO 记录 |
| E20 / E21 | `8986a40175f5f8eef56d765fef0cfa863e205985` / `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`；只读工程参考，未移植、未把接收结论挪作本方案证明 |
| PR #28 | draft head `57e239ee820a4208ac606663d877803f0b692206`；未触及 |
| 桌面主工作区 | 原有 M6A 文档、证据、参考 PDF 差异保留；未在主工作区编辑 |
| BMW16 PDF | 44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；论文只支持历史算法来源，不把 Theorem 8 用于 S4 sample-bracket 候选 |

证据类别分开记录：BMW16 论文步骤；S4 代码冻结行为；S10 Protocol I raw adapter/本地参考行为；S11 新增 C++ 端到端拼接与 RNG/计量规则。S11 的 C++ SplitMix 抽样流与 S4 Python `random.Random` 的逐位随机流不同；使用同一 S4 参数、集合更新、四层依赖和比较方向，独立域分离的无放回 Fisher–Yates 只保均匀分布，不声称逐种子复现 Python trace。S4/S10 项目的有限样本概率行为不继承 BMW16 Theorem 8。

## 3. C++ 接口合同

新增头文件 `VFSS/include/moe_topk/experimental_bmw16_select_party.h`，仅在 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON` 时编译。主要接口为：

```cpp
ProtocolIBmw16ExperimentalMaterialPair
protocol_i_bmw16_experimental_material_generate(public_config);

ProtocolIBmw16ExperimentalPartyOutput
protocol_i_bmw16_experimental_raw_score_mask_party(
    config, party_material, raw_score_share,
    score_fds, forward_shuffle_fds, select_round_fds,
    inverse_shuffle_fds, final_agreement_fd);
```

party 函数只返回状态、该方 XOR bit-mask share 和计数，不返回 selected handle、original index、score、阈值或完整 mask。单元素输入走显式数学恒等分支：唯一元素必选，P0/P1 输出 `1/0` share，不调用 raw adapter 或网络。其它输入按以下因果顺序执行：

1. 正式 `protocol_i_raw_score_input_party` 通过 S10 adapter 生成 S4 方向的稳定复合键 share；
2. `ProtocolIParallelShuffleParty` 两个消息相位生成共享隐藏置换下的公开 masked list；
3. C++ 代码用相同的两哨兵归约构建两个 median partition。R1/R2/R3/R4 每层先冻结两个任务的边批次，再各用一个消息相位交换该层本地比较 share；开出的 bit 同时用于双方下一层控制；
4. 两个 partition 的 `low.accept ∩ high.reject` 必须是唯一真实 handle；独立 membership key 计算 `R_i >= R_selected`，自比较本地置 1；
5. 逆向目标置换的 shuffle 两相位将 membership share 路由回原输入顺序；取每方 share 的 LSB，形成 XOR mask share；
6. 最后一相位交换状态码。算法中止不产生 inverse mask，也不返回部分 share。

在线抽样 seed 在此 `EXPERIMENTAL` 函数配置中是公开固定值，用于复跑；不是安全随机币协议。header 暴露了明确命名的 `test_only_force_probability_abort_after_select` 钩子，只用于验证无 mask abort。此钩子不是自然随机失败，不得用于成功率统计。

### 计量定义

- `logical_comparison_calls[r]`：算法控制流提出的比较请求总数，含 dummy 比较及同轮重复请求。
- `unique_select_slots_consumed[r]`：本轮/本任务首次实际消费的真实—真实 uCMP key 数。跨任务、跨轮 key 独立，不复用。
- `dummy_related_calls[r]`：至少一个端点是公开 dummy/sentinel 的逻辑调用，由公共类别规则本地决定。
- `repeated_logical_calls[r]`：同一轮同一实例重复的真实无序边请求；本实现轮内缓存计算结果，但逻辑请求仍计入调用数。
- `membership_slots_consumed`：成功分支除自比较外消费的 n−1 个 pair key。
- `ucmp_party_evaluations` / `dcf_party_evaluations`：包含 raw-score、Select、membership。每个实际 uCMP key 有两次 DCF Eval/方。
- `forward_shuffle_cmpagg_slots_unused` 与 `inverse_shuffle_cmpagg_slots_unused`：现有 shuffle material 全池预发但在此输出-only shuffle 中未调用的 CmpAgg 槽；本实现诚实计入，未删除。
- `offline_material_slots_per_party` 是 T 生成的 FSS/uCMP key 槽数量：`2*padded_n + 11*C(n,2)`；还需单列 shuffle records/permutation 等非 FSS 槽。
- `primitive_key_and_shuffle_bytes_per_party` 是序列化 primitive key bytes 与两份完整 shuffle serialization 字节之和，不含尚未实现的外层 bundle envelope/slot AAD/认证字段，不能记作最终发材包实测字节。
- `online_message_phases` 是实际 framed exchange 相位。成功 n≥2 为 raw 2 + forward shuffle 2 + Select 4 + inverse 2 + status 1 = 11；算法 abort 跳过 inverse，为 9。它绝不等于完整 secure Protocol I 轮数声明。

## 4. 独立视图审查与精确缺口

### 4.1 不把匿名比较位误判为原记录顺序泄露

R1–R4 打开的比较结果确实是匿名句柄上的真实稳定键严格顺序；这是更强 transcript。若一个图端点是原始 index，比较位可直接泄露记录顺序；但当前接口中端点是共享隐藏置换输出槽，P0/P1 各自只持有 shuffle 的局部 `sigma/tau`，没有单独得到总置换。因而不能仅从“比较位等于真实 `<`”推出任一方知道原 index 的秩。

在抽象的理想均匀隐藏置换模型中，固定严格全序被重标号为随机匿名秩序；若图生成、两实例共享句柄、selected handle、abort 和 mask 逆路由都对 real-handle 双射等变，匿名比较 transcript 的边际分布可以按随机双射模拟。这是有希望的引理方向，不是由 n≤8 测试证明的一般定理。

### 4.2 当前安全门仍为 NO-GO 的确切原因

本轮没有完成将上述理想 shuffle 替换为仓库实际 shuffle party 联合视图的 hybrid。缺少的命题是：对任一腐化方，给定其 raw input share、整个本方 shuffle material/state、输入适配 transcript、全体共享节点 mask 生成的相关 uCMP/DCF key pool（包括未用槽）、打开的 masked list 与自适应真实比较位/图、两实例 selected handle、abort 和最终原序 mask share，可由规定理想输出及明示匿名 transcript 泄露模拟；且差异受具体 PRG/DCF 多密钥安全优势界控制。

不能用“每份 DCF key 单独安全”替代：需确认 DCF 定义对一组独立 keygen、相关阈值 `alpha_ij=r_i-r_j`、同池自适应选取、公开 masked operands 和返回位支持 hybrid。S11 代码没有证明这条多实例/相关输入定理，也没有给出仓库 FSS 所依赖 PRG 的可引用优势界。当前 frame header 是会话/字段校验，不是 T 端点认证或对方身份认证。

另外，S11 的 T 函数同时返回 P0/P1 两方完整材料对象；没有 party-specific AEAD 包格式、密钥配置接口、T→party 私有认证通道、崩溃耐久的一次性领取账本。进程测试在同一进程的两个线程执行，不能关闭 S9/S10 文件/OS 隔离问题，也不能把 harness root 权限视为协议安全。

### 4.3 n=2 反例结论

只观察固定匿名句柄上的一条比较 bit，且隐藏置换均匀时，比较方向在所有严格顺序下各以 1/2 出现；这本身不是可区分事件。n=2 的 `selected anonymous handle` 与这一 bit 在最大值选择中相互确定，但其标签仍随隐藏置换随机变化。S11 不重复 S6 将“已知原始端点”泄露例套到匿名端点的推断。

仍需建立包含本方 shuffle state、两实例共同 handle、相关 key pool 和输出 share 的实际分布。S11 有限测试未枚举此完整联合视图，也未构造证明实际 shuffle 的混合链。因此准确状态为 `NOT_PROVED`，而不是“已找到匿名 transcript 泄露反例”或“不可能安全”。

## 5. 材料与执行状态

T generator 当前只接受公开 `n,K,session,fingerprint,width,seed`，产出两份 move-only C++ materials；Select 对每任务/轮/真实 pair 单独 keygen，membership 单独 keygen，forward/inverse 各有 CmpAgg pool。party key 对象在进程内只能 eval 一次，shuffle state machine 有进程内 claim。**这些是进程内约束，不是跨重启持久一次性语义**。外层发材包完整性、保密、party 认证、换槽/重放/崩溃测试均未实现。

最小下一步接口补丁：

1. 冻结 versioned party bundle schema，列出 session、party、n/K、pad/index/comparison bits、stage/task/round/canonical endpoints、mask shares、key bytes、唯一 slot ID；分别序列化完整 party-only bundle；
2. T 通过 P0/P1 独立凭据保护的 AEAD channel 发包，AAD 覆盖全部字段；端点/阈值数学关系仍须由 T 生成合同保证，AEAD 只保护来源/完整性；
3. bundle 原子落盘和持久 one-shot claim，覆盖 claim 期间崩溃和重复领取；
4. 在 T、P0、P1 独立 OS 身份的三进程 runner 中调用本 C++ API；party 输入和 party-only package 不通过共同 root controller；
5. 证明/独立审查一个明确的 combined-view simulator，或由项目所有者批准扩大理想功能泄露到匿名完整 transcript，并证明本地 shuffle/FSS 实例符合其模拟条件。

在这些接口和引理完成前，保持 `EXPERIMENTAL`；不要再只增加抽象安全结论文档，也不要创建 secure alias。

## 6. 失败状态与输出合同

- `SUCCESS`：仅当两个 task 成功且交集为唯一真实 handle；逆路由后返回每方 n 位 share。TEST_ONLY verifier 检查 XOR 重构为 oracle mask、二值且 weight=K。
- `ABORT_ALGORITHM_PROBABILITY`：sample-bracket 的 R2 bracket/U window 或 R3 W window 越界；执行最终状态相位；无 mask。
- `ABORT_ALGORITHM_INVALID`：秩唯一性、边界/基数或交集不变量异常；无 mask。
- frame/material 异常当前通过 C++ exception 交给调用者；此 API 尚无跨进程一致的 `ABORT_MATERIAL`/`ABORT_COMMUNICATION` watchdog。调用者不得用零 mask 代替无输出。
- S11 固定运行观察到 0 个自然算法概率 abort；一个强制 abort 用例通过无 mask 检查。不能据此声称概率界。C++ sampler 与 S4 Python seeded stream 不同，S4 的 Theorem 8 不适用。

## 7. 门禁

| 门 | 判定 | 证据/限制 |
|---|---|---|
| full VFSS experimental function path | `PASS / EXPERIMENTAL` | 正式 raw adapter→shuffle→C++ Select×2→membership→inverse→mask share 实际 CTest/oracle |
| Select four comparison layers | `PASS` | 同一层先构图并冻边，再交换/打开结果；每个 n≥2 成功 case 有 11 个完整入口 exchange phases |
| stable Kth functionality | `PASS_FOR_TESTED_CASES` | n=1,2,3,5,8,64 固定输入，对 oracle 逐位一致；小规模边界/重复分数；不表示安全 |
| T→party authenticated private delivery | `NO-GO` | bundle serializer、AEAD/AAD、私有交付、耐久一次性消费缺失 |
| adaptive transcript + full correlated-pool party view | `NO-GO / NOT_PROVED` | 缺少实际 shuffle 和 correlated multi-key DCF 的联合 hybrid；没有把匿名边际误称为完整视图证明 |
| independent OS-process E2E | `NOT_RUN` | 本轮两并发线程执行 C++ party API；S10 的 Python 多进程记录仍只是历史证据 |
| V4 formal LAN/WAN / six-scheme result | `NOT_STARTED` | 安全门、发材接口、完整独立进程计量合同未接收 |

## 8. 计量/因果消息账本

成功 n≥2 的在线相位数为 11。原始输入适配 carry/sign 两相位；前向 shuffle 两相位；Select R1/R2/R3/R4 四个依赖层；membership 是本地 DCF/uCMP Eval；逆路由两相位；最后状态一致一相位。Select 四层不意味着安全 Protocol I 入口四轮。

本轮 test 记录每层逻辑请求、dummy 边、同轮重复、unique uCMP slots、双方 DCF Eval、实际发送/接收 frame bytes、material key slot 数、shuffle 未用 CmpAgg 槽。实际 PRG 调用、离线生成时间、party-exclusive authenticated bundle 字节、CPU 峰值/内存、网络环境与协议吞吐 `NOT_MEASURED`。S11 smoke 结果不进 V4 表。

## 9. 修改与复跑

新接口与测试在 [S11 验证报告](../reproduction/BMW16_S11_CPP_EXPERIMENTAL_COMPOSITION_VALIDATION_2026-10-06.md) 列出。验证命令、Ubuntu 24.04/CMake 3.28.3/GCC 13.3、运行逐 case 原始计数和文件哈希随报告记录。构建在 WSL `/tmp/bmw16-s11-build`，不在仓库提交构建产物。
