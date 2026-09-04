# MoE Top-K 协议统一项目

## 1. 项目目标与当前边界

本项目要在 **VFSS** 中建立统一、可验证的安全 Top-K 实验环境，先完成
Agarwal Protocol I、Agarwal Protocol III 和 CipherGPT Top-K 的可区分基线，
再研究 Compare-Aggregate（CA）优化及 CryptoMoE 工作负载接入。

这不是源码树合并项目。算法语义可以参考现有仓库，密码学原语、通信、预处理和
序列化必须重新绑定到 VFSS；不能把旧工程的密钥文件或 FSS ABI 直接搬入 VFSS。

当前首要目标是：

1. 建立统一的明文语义、正确性 oracle、输出契约和计量口径；
2. 先做论文中定义清楚的精确基线；
3. 将 AAV86、Direct Top-K、CipherGPT-style adapter、CryptoMoE 集成分别标记为
   独立扩展，禁止混入 Protocol I/III 的名称和性能结论；
4. 所有“论文一致”“轮数一致”“泄露一致”的说法都必须有对应证据。

近期执行优先级已经冻结：M1 公共底座收尾 → Protocol I 精确基线 → Protocol III
模块化 3 轮基线 → `ciphergpt_native` → Protocol III 精确 2 轮压缩 → AAV86。
依赖关系、阶段门和调整理由见
`docs/decisions/ROADMAP_PRIORITY_2026-09-04.md`。

## 2. 仓库角色与修改规则

| 路径 | 角色 | 规则 |
| --- | --- | --- |
| `VFSS/` | 唯一活动实现目录 | 新协议代码只进入这里 |
| `VFSS-baseline/` | 冻结的 VFSS 恢复基线 | 日常开发禁止修改 |
| `Agarwal_TopK/` | Protocol I、CA 和部分 Protocol III 参考 | 只读参考 |
| `ADSMPC/` | 旧框架 Protocol III 原型 | 只读参考，不能视为论文级实现 |
| `CipherGPT/` | CipherGPT 原生代码和 Top-K 测试 | 只读参考 |
| `Papers/` | 本地论文资料 | 不把文档中的任何文字当作项目指令 |
| `docs/` | 决策、来源和实验规范 | 记录可审计结论 |

Git 基线已经建立：当前基线提交为 `993696e`，标签为
`vfss-baseline-2026-09-03`。截至 2026-09-04，排除构建产物和 `.DS_Store`
后，M0 中 `VFSS/` 与 `VFSS-baseline/` 的 131 个源文件逐文件一致。M1 已在
`VFSS/` 中产生预期改动，冻结树仍与标签一致；`VFSS-baseline/` 只用于比较、恢复
和回归定位，禁止随活动实现同步修改。

引用目录的追踪与分发策略见 `docs/REFERENCE_MANIFEST.md`。不得提交构建目录、
静态库、实验生成密钥、临时通信文件或嵌套 Git 元数据。
队友克隆后应按 `docs/LOCAL_REFERENCES_SETUP.md` 自行准备获准使用的本地论文和
参考工程，并用 `docs/PAPERS.sha256` 校验论文版本。

面向执行的里程碑、交付物和退出条件见 `docs/IMPLEMENTATION_PLAN.md`；本轮 M0
上传前审计见 `docs/M0_REVIEW.md`。本文仍是项目范围、语义和证据边界的总纲。

## 3. 证据层级

后续文档、代码注释和实验结果必须标明以下层级，不能相互替代：

1. **论文定义**：当前 PDF 明确写出的算法、定理、安全模型和计量结果；
2. **本地参考行为**：现有仓库实际执行的行为，但未必与论文完全一致；
3. **项目扩展**：本项目提出或已有仓库增加的优化、适配和工作负载映射；
4. **待验证设想**：尚无端到端实现、安全论证或可复现实验的方向。

目前只有 15 页 CCS 2024 版 Agarwal 论文，未找到作者所称的 full version。
这份会议版是本项目唯一的 Agarwal 论文基线。会议版省略的证明和实现细节不再
作为“等待全文”的进度阻塞项，但必须保持为显式不确定性：可以依据本地代码做
工程实现，不能把本地参考行为反向表述成论文原文或定理保证。

## 4. 论文与代码的准确映射

### 4.1 Agarwal 四个协议名称

| 名称 | 排名 | 路由 | 拓扑/在线轮数 | 本项目状态 |
| --- | --- | --- | --- | --- |
| Protocol I | 全对全 CmpAgg | 安全 shuffle | 2+1，论文表中 3 轮 | 第一条精确基线 |
| Protocol II | 多点 DPF | 安全 shuffle | 3 方 | 当前不实施 |
| Protocol III | 全对全 CmpAgg | DPF 路由及跨阶段压缩 | 2+1，论文表中 2 轮 | 第二个论文目标；先做 3 轮模块化中间基线 |
| Protocol IV | 多点 DPF | 标准 DPF | 3 方 | 当前不实施 |

这里的“全对全 CmpAgg”是对每一对元素比较并聚合出稳定 rank。论文第 5 节的
AAV86/Compare-Aggregate compiler 是另一条优化路线，不等同于 Protocol I 或
Protocol III，也不能用来替换二者的精确基线。

论文用原始输入下标打破同分值，得到稳定 rank。本项目统一使用 `(score,
original_index)` 作为明确顺序，并在接口边界处理升降序差异。

### 4.2 Protocol I 与安全 shuffle

Protocol I 直接依赖 Chase、Ghosh、Poburinnaya 的 2 方静态半诚实
secret-shared shuffle。`协议1shuffle.pdf` 是当前直接基础资料；
`Agarwal_TopK/protocol1/` 的 B0 是置换矩阵功能参考，不是高效论文 shuffle；
`protocol1_ca/` 中的 B1 才包含两次 Permute+Share、OPV/Share Translation/Benes
方向的实现材料。

迁移时必须分别验证：

- shuffle 后的公开 masked list 与秘密共享 payload 是否保持同一置换；
- 两方各自置换一次的角色、随机性和消息顺序；
- 排名、稳定同分和路由是否与明文 oracle 一致；
- 论文 3 在线轮的统计是否包含本项目新增步骤。

VFSS 当前没有可直接调用的生产级安全 shuffle。`VFSS/ext/FSS/api.cpp` 中的
`MockShuffle` 只是交换首尾元素的 Graphiti 模拟代码，严禁作为 Protocol I
shuffle 或性能数据来源。

### 4.3 Protocol III 与 DPF 路由

`Agarwal_TopK/protocol3_ca/` 只有 DCF conformance 和明文 AAV86 图测试，不是
完整 Protocol III。`ADSMPC/src/protocol3.cpp`、`RankingPhase.h` 和
`routing_dpf.h` 提供了旧框架参考，但包含以下非目标行为：

- Dealer 用明文输入计算 `true_rank`，并据此生成 masked rank；
- 多轮 AAV86 图在 Dealer 端按明文结果更新；
- 两方依靠文件交换、固定 `sleep(1)` 和临时二进制文件同步；
- DPF 指示位与 payload 通过 Beaver 乘法组合，但文件格式绑定旧 FSS 结构；
- 默认编译开关选择 AAV86，而不是论文 Protocol III 的全对全排名。

因此这套代码只能提供局部算法和调用次序参考，不能作为端到端安全性、2 轮在线
复杂度或论文复现的证据。

论文先给出 2 轮的模块化 DPF 路由；它与 1 轮 GRank 组合后是 3 轮。最终
Protocol III 通过跨阶段压缩把整体降为 2 轮，其论证要求 payload 群 `H` 是域且
payload 非零，因为需要乘法掩码及逆元。VFSS 的 `GroupElement` 是 `uint64_t`，
按位宽在 `Z_(2^b)` 上取模，不是域。没有解决表示和逆元条件前，只实现标准 DPF
+ Beaver 模块化路径，并明确标为 3 轮工程中间基线，不使用
`agarwal_protocol_iii_exact` 名称。

### 4.4 AAV86 / CA / Direct Top-K

AAV86 compiler 从一次初始 shuffle 开始，随后按已公开的局部 rank 自适应地产生
下一轮比较图。会议论文给出 2+1 方、`2κ+1` 轮的结论，但当前本地实现存在关键
预处理边界：具体 uCMP/DCF 密钥与某一条边的 mask difference 绑定，而后续边只有
在前一轮结果公开后才知道。

当前 `protocol1_ca` 能支持的证据是：

- 公共比较图在离线包生成前已知时，可生成 exact-edge CA 材料；
- B1 全对全 Protocol I 路径和若干 AAV86/Direct Top-K 原型已有测试记录；
- 动态第二轮使用在线 Dealer 的路径改变了论文“输入无关离线、Dealer 在线静默”
  的模型；
- 尚未证明一个 offline-only、exact-edge、可递归自适应的完整 AAV86 包生成算法。

在解决一般性预处理问题前，AAV86 和 Direct Top-K 统一标记为实验扩展；不得隐藏
在线 Dealer，不得报告为 Theorem 5.1 的实现，也不得用固定完整图预留等局部补丁
冒充一般算法。

### 4.5 CipherGPT

CipherGPT 论文的 Top-K 是 shuffle 后递归的 modified QuickSelect：比较结果在
shuffle 后公开，最终返回最大的 `K` 个值共享。它与全对全 rank 路线不同，必须
保留为独立基线。

仓库中 `CipherGPT/src/globals.cpp::Top_K_paper` 和
`CipherGPT/test/Top_K_paper_test.cpp` 已经存在，旧文档中
`CIPHERGPT_CODE_IMPLEMENTED=false` 的描述已经过期。但当前实现仍有待修正项：

- 轮数被限制为 `2*log2(N)`，达到上限后可能返回少于 `K` 个结果；
- `K>N` 和输入长度错误只打印后返回，调用方可能继续运行；
- 没有稳定同分规则；论文伪代码使用 `>= pivot`，全相等输入存在不收缩风险；
- 测试会重构 selected index 并生成明文 mask，这只能属于验证模式。

因此分成两条线：

1. `ciphergpt_native`：修正并运行原生 CipherGPT Top-K，保留其原生密码学栈；
2. `ciphergpt_style_vfss_adapter`：只在统一契约稳定后，用 VFSS 原语实现相同的
   高层流程，名称和性能结果与原生实现严格分开。

### 4.6 CryptoMoE

CryptoMoE 使用的是 CipherGPT Top-K，而不是新的 Top-K 密码学原语。它对本项目的
直接价值是定义应用工作负载：`m` 个 token、`n` 个 expert、router Top-k，以及
每个 expert 固定容量 `t` 的 confidence-aware Top-t dispatch。

第一阶段只接入“单 expert 的候选 Top-t dispatch”，不先实现完整 MoE。以下问题
必须在集成前解决：

- ineligible 候选不能简单把 score 设为 0，因为合法 router score 也可能量化为
  0；必须定义可证明正确的 eligibility 编码和 dummy 顺序；
- `t=2mk/n` 是吞吐/精度折中，会丢弃超额 token 并填充不足容量，不能与精确
  Top-K oracle 混为一谈；
- 当前 CA transcript 会公开位置、pivot、局部 rank、bucket 大小、边界和选中
  位置；尚未证明其泄露与 CryptoMoE 允许的泄露等价；
- Direct Top-K 的比较量并非一般性的 `O(km)`，最坏情况和实测边数必须单独报告。

### 4.7 基础和条件性资料

- `FSS基础.pdf`：用于团队理解 FSS、DPF、DCF、correction word 和 full-domain
  evaluation；它是学习笔记，不是规范。笔记中的 DCF 分段公式存在与文字描述不符
  的记号，落地时以论文和 VFSS conformance test 为准。
- Boneh 等 2023：Poplar/private heavy hitters、incremental/extractable DPF 和
  恶意客户端检查的背景资料；不是当前 Protocol I/III 的直接依赖。
- `协议2shuffle.pdf`（Ruffle）：3 方诚实多数、恶意安全和公平/GOD 的条件性
  路线，不可直接替换当前 2+1 半诚实 shuffle。当前 PDF 首页还带有出版占位符，
  若未来采用，应先确认正式版本和安全模型。

## 5. 统一语义契约

迁移前必须冻结最小契约：

- 公开参数：`n`、Top-K 数量 `K`、score 位宽与有符号/定点解释、payload 形状、party 拓扑、
  实验模式；
- 选择语义：最大的 `K` 个元素，同分按原始下标确定唯一顺序；
- 统一安全输出：原始输入顺序下的 `m` 个秘密共享比特
  `([z_1]^B, ..., [z_m]^B)`，其中 `z_j∈{0,1}` 且 `Σz_j=K`；
- 掩码为 1 的元素集合必须精确等于 Top-K，不要求集合内部排序；
- 默认不公开：完整 rank、单边比较位、selected index、明文校验数据和调试转录；
- `test` 模式可使用固定向量和显式重构；`secure` 模式使用新鲜随机性，不能进入
  测试专用公开路径；
- selected payload 可以作为内部中间值或额外诊断输出，但不能代替统一 Top-K
  bit-mask；若 VFSS 内部不是布尔共享，必须明确实现并计量到 `[z]^B` 的转换。

统一实验接口写为：

```text
([z_1]^B, ..., [z_m]^B) <- TopK(([x_1]^A, ..., [x_m]^A), K)
```

例如 `X=(0.2,0.9,0.4,0.8), K=2` 时，重构后的掩码必须是
`Z=(0,1,0,1)`。统一 bit-mask 是团队比较接口；若论文原生协议返回值共享或排序
结果，报告名称必须标明 `mask_output` 适配，并把 paper core 与输出适配开销分别
保留，主比较使用两者之和。

所有实现都必须保留原始位置：

- CipherGPT shuffle 时绑定元素与原始索引，QuickSelect 后逆映射并生成 `m`-bit
  共享掩码；
- Protocol I 的 shuffle 与 Protocol III 的 DPF 路由最终也必须恢复到原始输入
  位置；
- shuffle、索引绑定、逆映射、共享类型转换和掩码生成产生的时间与通信全部计入
  对应阶段，不能作为测试脚本的免费后处理。

CryptoMoE 的容量限制、dummy、drop 语义位于工作负载适配层，不修改精确 Top-K
核心 oracle。

### 5.1 长期统一测试矩阵

为避免 Top-K 数量与 AAV86 迭代次数混淆，本文固定用 `K` 表示 Top-K 数量，
用 `r` 表示 AAV86 迭代次数。

| 输入数量 `n` | Top-K 数量 `K` | AAV86 迭代 `r` |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

- Agarwal 与 CipherGPT 的输入位长统一为 32 位；signedness、定点 scale 和随机
  输入分布必须随原始结果一并记录；
- 同一比较组使用同一批明文输入、相同 `(n,K)`、相同网络环境和一致的正确性
  oracle；
- 先用论文或仓库已有实验数据做逻辑回归，但这些历史数据不能替代当前统一环境
  下的性能实测；
- 对无法完成的大规模点如实标记失败原因或 `NOT_MEASURED`，不得用缩小参数、
  外推或旧数据填充。

### 5.2 长期统一指标与输出字段

每次正式运行必须逐项记录：

| 字段 | 统一含义 | 单位/口径 |
| --- | --- | --- |
| `offline_time_ms` | 生成在线阶段使用的全部预处理材料 | ms |
| `offline_material_total_bits` | Dealer 或离线生成方提供给在线执行的全部预处理材料之和 | bits |
| `online_time_ms` | 输入就绪至统一 Top-K mask 完成 | ms |
| `online_comm_total_bits` | 在线方实际发送字节折算后的总和 | bits，原始审计字段 |
| `online_comm_per_party_bits` | `online_comm_total_bits / 在线方数量` | bits，统一主报告字段 |
| `online_rounds` | 有因果依赖的在线交互轮数 | rounds |
| `online_prg_calls_total` | 所有在线方长度倍增 PRG 调用总数 | calls |
| `comparison_edges_total` | 实际执行的无序比较边总数 | comparisons |
| `total_time_ms` | `offline_time_ms + online_time_ms` | ms |

同时保留每一方的 `sent_bits` 和 `received_bits` 诊断值。统一表以 per-party 在线
通信为展示口径，但不得删除 total 和分方原始计数。论文公式若写明 “across both
online parties”，先记录到 total 字段，再派生 per-party 字段，不直接混用。

AAV86 报告还必须记录实际 `e_A(n,r)`（比较边数）和 `v_A(n,r)`（参与比较的节点
复杂度），并把实测值与 Theorem 5.1 的离线材料、在线通信和 DCF.Eval/PRG 理论项
并列，不能只报告渐近复杂度。

轮数口径分开记录：

- Protocol I + AAV86：团队验收值为 `2r+1`，与当前会议版 Theorem 5.1 一致；
- Protocol III + AAV86：团队目标值为 `2r`。当前会议版没有给出该组合的独立
  theorem，因此实现前必须补齐组合协议、预处理时序和泄露论证；未完成前不能把
  `2r` 标成论文实测或已证明结论。

## 6. VFSS 迁移原则

1. `VFSS/ext/FSS` 是唯一有效 FSS 实现；信任其内部代码，不复制或重写原语。
2. 只为当前协议确实需要的 DCF、DPF、乘法和 shuffle 建最小适配；不要预先创建
   大而全的 `domain/runtime/algorithm` 抽象。
3. 每个适配器先用边界向量做 conformance test，再进入协议。
4. 所有预处理在 VFSS 下重新生成；不读取 ADSMPC/CipherGPT 的原始 key struct。
5. 传输使用 VFSS 通信层；文件轮询和固定等待只保留为旧实现证据，不迁移。
6. 不加入降级路径、启发式终止、特殊输入补丁或事后修正；遇到语义缺口先解决
   一般算法和证明条件。
7. 只有第一次实现确实需要时才创建：

```text
VFSS/include/moe_topk/      已稳定的最小公开接口
VFSS/src/moe_topk/          当前协议实现与必要适配
VFSS/tests/moe_topk/        oracle、conformance、differential、E2E
docs/decisions/             影响语义、泄露或计量的决策
scripts/                    可复现构建和实验入口
artifacts/                  被忽略的本地产物，不作证据源
```

## 7. 修订后的实施顺序

### M0：证据与命名基线

- 采用本文的证据层级和协议命名；
- 记录论文 PDF、参考代码入口、已知偏差和可复现环境；
- 把已有文档中的过期状态逐项校正，不从旧报告直接继承“完成”结论；
- 保持 VFSS baseline 冻结。

### M1：公共正确性底座

状态：M1 与 M1.1 已完成并合入 `main`。统一语义冻结为 32 位二补码 signed
fixed-point、scale=12、数值降序、同分 original index 升序；M1.1 已在 Ubuntu 24.04.4
干净 Debug 构建中通过四项 CTest，并补齐正式 metrics provenance。该收尾不改变冻结
语义；网络与性能仍为 `NOT_MEASURED`，详见 `docs/M1_1_UBUNTU_HANDOFF.md`。

- 实现 32 位输入、稳定同分和原始位置 `m`-bit mask 的明文 oracle；
- 建立随机、重复值、全相等、负数、`K=1`、`K=n`、非 2 次幂 `n` 和位宽边界
  向量，并验证 `z_j∈{0,1}`、`Σz_j=K`；
- 先为 Protocol I 实际需要的 VFSS DCF、共享转换和 shuffle 接口建立
  conformance test，不提前迁移 Protocol III 专用 DPF；
- 实现 Protocol I 的全对全 CmpAgg rank 核心并与 oracle 差分；
- 固定 5.2 节字段、计时边界和错误语义，禁止打印错误后继续。

### M2：Protocol I 精确基线

- 从 Chase shuffle 的 B1 材料迁移最小安全 shuffle 路径；
- 接入全对全 CmpAgg、稳定 rank 和 payload 路由；
- 恢复到原始输入顺序，输出统一的秘密共享 Top-K bit-mask；
- 用独立进程完成 2+1 角色 E2E；
- 通过重复值、payload 对齐和 secure/test 输出边界检查后，才使用
  `agarwal_protocol_i_exact` 名称。

### M3：Protocol III 模块化 3 轮基线

- 复用 M1 的全对全 CmpAgg，把 GRank 明确计为 1 个在线轮次；
- 使用 VFSS 标准 DPF 与秘密共享乘法实现 2 轮路由；
- 移除旧原型中的明文 `true_rank` Dealer、文件同步、固定等待和调试重构；
- 输出统一的原始顺序秘密共享 bit-mask，并完成小规模独立进程 E2E；
- 在未做跨阶段压缩前只使用 `agarwal_protocol_iii_modular_3round` 名称。

### M4：CipherGPT 原生基线

- 在 CipherGPT 原生框架修复 `Top_K_paper` 的终止、基数、同分和错误传播；
- 将元素与原始索引绑定，逆映射并输出统一的秘密共享 bit-mask；
- 保留 CipherGPT 原生两方密码学栈，并先完成 source-only/许可证边界审查；
- 使用 M1 同一批 32 位输入完成小规模正确性基线；
- 性能报告明确其运行时和 topology，不把它伪装成 VFSS 实现。

### M5：Protocol III 精确 2 轮压缩

- 只在 M3 的 3 轮路径正确且阶段审计稳定后开始；
- 明确选择满足论文条件的域表示和非零 payload 编码；
- 实现 GRank 与 DPF 路由的跨阶段压缩，验证在线因果轮数为 2；
- 分别记录 paper core 与统一 mask adapter 的开销和轮数；
- 只有代数条件、消息流、泄露和差分/E2E 都通过后，才使用
  `agarwal_protocol_iii_exact_2round` 名称。

### M6：AAV86 / Direct Top-K 实验

- 复用已经稳定的精确基线原语，不反向改变 M2/M3/M5 的实现身份；
- 先解决输入无关、Dealer 在线静默条件下的自适应 exact-edge 预处理；
- 对 `r=2,3,4,5` 完整记录 `e_A`、`v_A`、PRG 调用、在线轮数和全部统一指标；
- 在 LAN/WAN 下分别寻找比较量下降与轮数上升的实测拐点；
- Direct Top-K 保持独立实验标签；未通过一般算法和安全审查前只使用
  `aav86_ca_experimental`、`direct_topk_experimental` 名称。

### M7：统一实验

- 每个配置预热 1 次、正式运行 5 次，保存原始输出并报告 median/min/max；
- 主表展示 `online_comm_per_party_bits`；同时保存 total、每方 sent 和 received；
- 完成 5.1 节固定 `(n,K,r)` 矩阵，无法运行的点标记 `NOT_MEASURED`；
- 分组比较，不做混合排行榜：
  - B0 与 B1：只比较 shuffle backend；
  - Protocol I 与 Protocol III 3 轮：统一 CmpAgg，比较 shuffle 与 DPF routing；
  - Protocol III 3 轮与 2 轮：统一功能，比较跨阶段压缩的轮数与代数代价；
  - Protocol I/III 与 CipherGPT native：统一输入、输出和指标，明确 topology 与运行时；
  - Protocol I B1 全对全与 AAV86 full-sort：比较排名算法；
  - AAV86 full-sort 与 Direct Top-K：使用相同输入共享、计划和 transcript 口径；
  - CipherGPT native 与未来 VFSS adapter：并列报告，不混同实现身份。
- 每个数字标记为历史记录、当前实测、理论值或 `NOT_MEASURED`。

CryptoMoE 保留为 M7 之后的工作负载接入：先冻结 eligibility、dummy、容量和允许
公开的 routing transcript，再选择已经验收的 Top-K 后端。

## 8. 验收门槛

### 正确性

1. 原语 conformance 通过；
2. 每条协议与统一 oracle 做差分测试；
3. 独立进程 E2E 通过；
4. 检查 `m`-bit、二值性、`Σz_j=K`、稳定同分、原始位置、payload 对齐和重复执行；
5. 安全模式不依赖重构中间值。

### 安全性

每一阶段记录输入、输出、接收方、公开元数据、秘密共享、打开值、随机性来源、
持久化文件和 Dealer 在线行为。改变 party 模型、泄露或预处理时机必须单独决策，
不能只改实现标签。

### 性能

每条记录至少包含：Git revision、实现标签、party 拓扑、`n/K/r`、位宽、padding、
线程数、CPU/内存/OS/编译器/flags、网络带宽与 RTT、重复次数、离线材料、离线
时间、在线时间、total/per-party/分方通信、在线轮数、PRG 调用、比较边数、总耗时
和 correctness status。字段名和口径以 5.2 节为准。

## 9. 当前已确认状态

- M0 和 M1 已同步远端；`VFSS/` 只包含 M1 的预期差异，冻结 baseline 未改；
- M1 已冻结 32 位二补码 fixed-point（scale=12）、统一随机范围、稳定同分、
  oracle、CmpAgg、metrics 和 DCF 边界映射；
- Protocol I 的 B0/B1 和 CA 测试材料较完整，但 B0 不是高效论文 shuffle，CA
  不是 Protocol I 本体；
- Protocol III 没有可直接迁移的完整论文级实现，ADSMPC 只能作为旧原型参考；
- VFSS 有 DCF/DPF 等原语，但没有可用的真实安全 shuffle；
- AAV86 的动态图预处理与 offline-only Dealer 模型尚未闭合；
- CipherGPT 原生 Top-K 代码已经存在，但边界语义和终止逻辑尚未达到基线要求；
- 团队主线已经调整为 Protocol I → Protocol III 3 轮 → CipherGPT native →
  Protocol III 2 轮 → AAV86；
- 所有基线和扩展的统一交付输出已经固定为原始顺序下的秘密共享 Top-K bit-mask；
- 固定测试矩阵、AAV86 `r=2..5` 和统一指标字段已经成为长期验收约束；
- CryptoMoE 目前应作为工作负载和系统语义来源，而不是第四种 Top-K 原语；
- Agarwal full version 不再列为待获取材料，会议版之外的细节保持“未知”。

## 10. 后续需要团队明确的输入

M0、M1 和 M1.1 已完成；以下问题影响后续里程碑：

1. 目标机器、LAN/WAN 条件和最终需要复现的表格；
2. 首个 MoE 模型的 `m/n/k/t`、score 编码和 payload 形状；
3. 是否把 selected payload 作为 bit-mask 之外的附加诊断输出；
4. CryptoMoE 集成允许公开哪些 routing transcript；
5. 是否需要把 Ruffle/恶意安全 3PC 纳入后续独立路线；
6. 是否为 Protocol III 压缩路由引入域表示，还是只保留标准 DPF + Beaver 路径；
7. CipherGPT 原生代码的来源、revision、许可证和 source-only 共享方式。

这些选择不阻塞 M1.1、Protocol I 或 Protocol III 模块化 3 轮基线。CipherGPT
可以提前做资料与失败用例整理，但主线合并顺序不变。不能提前声称 AAV86、
CryptoMoE 或 Protocol III 2 轮压缩已经满足论文安全模型。
