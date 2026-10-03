# M6A-P2-I-E2：E1 独立复审与固定 M 材料方案审查

- 日期：2026-10-02
- 复审 worktree：`C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment`
- 复审分支：`codex/m6a-p2-i-allpairs-experiment`
- E1 起点 / 当前 HEAD：`c3926c68fd14f270faa8b55234311071947fa080`

## 1. 判定摘要

| 项目 | 判定 | 含义 |
|---|---|---|
| `E1 EXPERIMENT REVIEW` | **PASS_WITH_FINDINGS** | 真实 uCMP/DCF party-key 生成与 TEST_ONLY 单进程材料池消费样本有源码依据；CSV 算术和小规模递归图计数通过独立复算。指标字段、独立 oracle 的覆盖范围和安全边界需要收窄。 |
| `FIXED-M OPTION` | **NO-GO（作为当前紧凑材料方案）** | `M ≥ M_observed` 不能保证任意输入和 pivot randomness 下的覆盖。端点绑定材料若要无超限保证，当前可证明的保守容量是每轮 `C(n,2)`、全协议 `r·C(n,2)`。通用可重绑定材料仍是 `UNPROVEN`。 |
| `P2-I SECURE RUNTIME` | **NO-GO** | E1 没有独立 party 进程、在线 transcript、协议 masked opening、安全 bucket 更新、完整排序、Top-K mask 适配或安全视图证明。 |

本复审只审查已有 E1 文件和接口，不实现协议，不运行 E1 原有测试/二进制，不更改 E1 文件。主工作区的既有差异保持不动。

## 2. 基线、工作区与审阅方法

### 2.1 Git 状态

复审开始时主工作区为：

- 分支：`feat/m6a-performance-evaluation`
- HEAD：`c3926c68fd14f270faa8b55234311071947fa080`
- `git status -sb`：

```text
## feat/m6a-performance-evaluation
 M PROJECT.md
 M docs/IMPLEMENTATION_PLAN.md
 M docs/PAPERS.sha256
 M docs/REFERENCE_MANIFEST.md
?? docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md
?? docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md
?? docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md
?? siamjdiscrmath.pdf
```

这些 tracked/未跟踪内容在本次复审前已存在。主工作区没有被切换、暂存、清理或写入。根目录 `siamjdiscrmath.pdf` 保持原样。

复审开始时实验 worktree 为：

- 分支：`codex/m6a-p2-i-allpairs-experiment`
- HEAD：`c3926c68fd14f270faa8b55234311071947fa080`
- `git status -sb`：

```text
## codex/m6a-p2-i-allpairs-experiment
?? docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md
?? experiments/
```

两个 worktree 都从指定 revision 开始。主工作区中的未跟踪 P0/P1/P2 文档没有复制或移动到实验 worktree。

### 2.2 复核方式

- 独立读了 E1 报告、图计数器、CSV、n=5 边 fixture、材料池 C++ 和实验 CMakeLists；核对了主工作区的项目合同、M1 语义、P0/P1/P2 文档、M5 G3 接收结论、AAV86 与 Agarwal 本地论文、Protocol I uCMP/CmpAgg/priority-key/DCF 源码。
- 没有导入或重跑 E1 的 Python 生成器。单独用只读 Python 计算核对 CSV：156 个 run、546 个逐轮记录、迭代序号、`e_i = pivot-pivot + pivot-element`、run 总边数、样本最大轮边数、容量、已用/未用槽位、比例和宽松逐轮界。结果为 `INDEPENDENT_CSV_AUDIT PASS`。
- 另用独立递归过程对 `n≤32` 的 96 个矩阵 run 逐个枚举活动子问题内的所有无序 pair，并与 CSV 每轮边数比较；检查轮内和跨轮无重复。覆盖 336 个轮记录，结果为 `INDEPENDENT_SMALL_GRAPH_ENUMERATION PASS`。该过程没有使用 `graph_oracle_edges` 或 E1 计数器的 snapshots。它复现了种子和递归语义，因此不是新的随机性证明。
- 对 n=5 fixture 独立检查 8 行边、两轮边数 `[7,1]`、20 个逻辑槽位、8 个已用和 12 个未用。
- 没有重编译或运行 E1 C++ harness，也没有运行仓库测试、基准或完整协议。E1 C++ 的通过状态按报告记为历史实验结果；本复审只确认其调用链和测试边界。

证据标签沿用任务定义：`PAPER_DIRECT`、`PAPER_DERIVED`、`PROJECT_CONTRACT`、`EXPERIMENT_OBSERVED`、`ANALYTICAL_BOUND`、`UNPROVEN`、`NOT_MEASURED`。

### 2.3 E1 主张与证据等级

| E1 主张 | 证据标签 | 独立复审结论 |
|---|---|---|
| AAV86 固定轮数随机完整排序的 pivot / clique / bucket 递归 | `PAPER_DIRECT`、`PAPER_DERIVED` | 对 AAV86 §3.1 / Theorem 3.1 与 Agarwal CCS’24 §5.3 / Algorithms 1–2 原文页复核；代码实现为 TEST_ONLY 明文模拟。 |
| Q20.12 signed score 降序、original index 升序及前 K mask oracle | `PROJECT_CONTRACT`、`EXPERIMENT_OBSERVED` | 只证明 fixture/oracle 语义映射；没有安全 mask 输出。 |
| 156 runs、546 轮行、各个 `e_i` 和 M5 fixture 边数 | `EXPERIMENT_OBSERVED` | CSV 算式和 n≤32 的递归小图矩阵样本独立通过；n>32 图本身未在 E2 重跑。 |
| 每轮 `C(n,2)` 与全两两 `r·C(n,2)` | `ANALYTICAL_BOUND` | 在简单无向边、活动子问题分割、每轮每 pair 至多一项的前提下成立；是保守界/本实验池公式，不是 AAV86 期望复杂度的推论。 |
| 仓库 uCMP/DCF party-specific keygen 与序列化、消费 | `EXPERIMENT_OBSERVED` | 调用链源码确认真实 `keyGenDCF` 和 party 导出；C++ harness 的通过值来自 E1 报告，本复审未重跑 harness。 |
| 图 digest、party isolation、持久 one-shot、adaptive privacy 和协议安全 | `UNPROVEN`、`NOT_MEASURED` | E1 没有实现或测量这些边界。 |
| compact fixed-M 对任意自适应图的材料覆盖 | `UNPROVEN` | `M_observed` 只为样本观察值；无 endpoint rebind 构造或超限概率证明。 |

## 3. 实际读取的证据及 SHA-256

哈希针对复审时实际读取的路径。P0/P1/P2 等未跟踪文件只从主工作区读取。

### 3.1 项目文档与论文

| 路径 | SHA-256 |
|---|---|
| 主工作区 `AGENTS.md` | `A260F872D0A9058FE9F0B2B94DC78C06272DCBBA7C0D28488CCCEE7BF34897AA` |
| 主工作区 `PROJECT.md` | `33B66C6BD6CCE8FCEAB817FB68C17BEF2A4E58194A39B41AA4D48AD79D2C05D5` |
| 主工作区 `docs/IMPLEMENTATION_PLAN.md` | `F5C2D1DCD6587D763EFBD8DBF3D6CF8F9A7014EDB93145A7678ADF6876DD49E8` |
| 主工作区 `docs/BENCHMARK_VALIDATION_PLAN.md` | `DBFAA8A850541A4A8BFA5DAE708804CF2DB4C44F82A9D4E3402E87AB10198756` |
| 主工作区 `docs/M3_ONWARD_TEAM_WORK_PLAN.md` | `F2252FCCCF646091270CE2B584DA6D8518712CB6467B077AFB28E5AD2136D3E6` |
| 主工作区 `docs/decisions/M1_SCORE_SEMANTICS.md` | `5920A00EAA41CC04712E0338F82305EE255730E944AB38293E9C65C70FBA426D` |
| 主工作区 `docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md` | `21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0` |
| 主工作区 `docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md` | `C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC` |
| 主工作区 `docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md` | `B637F31009F329391D1CC92CC959E251E64A0AAC45C5FBB96CFEE0D471B93EFE` |
| 主工作区 `docs/reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md` | `C3598C62EDC0D431E5C1159A979683602157175050963F1E3F2F20A905562DD6` |
| 主工作区 `docs/handoffs/M5_TO_M6A_G3_RECEIVER_CHECKLIST_2026-09-25.md` | `32FB81AC68DAC74197836A7EE89E544A64E0F705D10A6264F6CDF35B0EE16D30` |
| 主工作区 `Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf` | `322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83` |
| 主工作区 `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf` | `18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C` |

AAV86 本地作者托管版为 9 页；读取 PDF 第 5–6 页文本，核对 §3.1、Theorem 3.1：`t=ceil(n^(1/k))`、均匀无放回选择 `t−1` 个 pivot、pivot 与所有元素（包括其他 pivot）比较、把非 pivot 分为 `t` 个块并递归 `k−1` 轮；定理是期望比较数上界。印刷页 506–507。Agarwal CCS’24 本地会议版为 15 页；读取 PDF 第 12 页 §5.3 / Algorithms 1–2，核对相同 pivot-clique / pivot-non-pivot 图和 CA local-rank 改写。没有把会议版之外的 full version 内容补写为已知。

### 3.2 运行时代码

| 路径 | SHA-256 |
|---|---|
| 主工作区 `VFSS/include/moe_topk/protocol_i_ucmp.h` | `7E444F0188AFA2FAF0CE8A1FA97AF7D821C54FE82FB46CEFD1999AEA4C798D33` |
| 主工作区 `VFSS/src/moe_topk/protocol_i_ucmp.cpp` | `B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23` |
| 主工作区 `VFSS/include/moe_topk/protocol_i_cmpagg.h` | `D50B7B84A3EEF299D1F1C23A4D342C7F95003C28DD0F72CD8BD56FD0E3878390` |
| 主工作区 `VFSS/src/moe_topk/protocol_i_cmpagg.cpp` | `5183E371ADC7D06DF6E93097921B2C303B1AC5A8C24B883CA2330855521FCA4F` |
| 主工作区 `VFSS/include/moe_topk/protocol_i_priority_key.h` | `8C345675877FDD101E0B5918250FF7E7B6879031161CD38B7FE141CE96D0E338` |
| 主工作区 `VFSS/ext/FSS/dcf.cpp` | `1E62CF39A5985E761F5BB7E18E20884835DFE6AC08A8BF55F69E34B1B1B3E0E8` |

### 3.3 E1 文件

| 实验 worktree 路径 | SHA-256 |
|---|---|
| `docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md` | `0F54A121DC2CFFA70719C735F577C66022C2ED019706415CBC5D9A87347537A4` |
| `experiments/m6a_p2_i_allpairs/aav86_graph_counter.py` | `09DCC72DF6BC68C83046CBB3F7C9AF8B13642BEA720036B92804F6BE5423B402` |
| `experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv` | `5346AE011201C1F88F3B7199BB86DE7C1B4F761151F3FB269C86F9E99AF9F622` |
| `experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv` | `87A660C55B47FBCD1AAAE6C4E025798E8E4B6853968DC6D27DF635A704B46D78` |
| `experiments/m6a_p2_i_allpairs/material_pool_test.cpp` | `8F6F36C5408A55A09CB9754C7076C2C27EADD0DDD539B7D5297949A9DEF0E159` |
| `experiments/m6a_p2_i_allpairs/CMakeLists.txt` | `B99BCA22E068E462FB222E83AD5A79BE9E4A92E79447E46EB6880BD317C77195` |

## 4. AAV86 图计数器复核

### 4.1 论文对象与计数公式

- `PAPER_DIRECT`：AAV86 §3.1 / Theorem 3.1 是固定轮数 `r` 的随机**完整排序**，期望比较数 `E(n,r)≤c(r)n^(1+1/r)`。Agarwal CCS’24 §5.3 / Algorithm 1 展示该比较图，Algorithm 2 以 CA local-rank 取代公开全部比较结果。
- `PAPER_DERIVED`：对大小 `m`、剩余深度 `d` 的活动子问题，`t=ceil(m^(1/d))`，`q=t−1`。本轮边是 pivot clique 加 pivot—non-pivot 完全二部边；该子问题边数 `C(q,2)+q(m−q)`。`d>1` 时非 pivot 按 pivot 全序放入 `q+1=t` 个桶并递归到下一轮。
- `PROJECT_CONTRACT`：脚本把分数映射为 `score` 降序、`original_index` 升序；这只是明文 TEST_ONLY 排序 oracle，不是论文的 Q20.12 编码或安全稳定排序。
- `d=1` 时 `t=m,q=m−1`，该**活动子问题内部**所有无序 pair 都至少包含一个 pivot，故该子问题图是完全图。最终一轮若有多个活动子问题，是这些子问题各自的 clique；它不等于原始 `n` 个节点上的全局完全图。

### 4.2 独立计数结果与适用范围

`INDEPENDENT_CSV_AUDIT PASS` 核对了全部 546 行 / 156 个 run。所有行满足：轮编号完整、`e_i = pivot_pivot_edges + pivot_element_edges`、`run_total_edges=Σe_i`、`M_observed_sample_max=max_i(e_i)`、`pool_capacity=r·C(n,2)`、`pool_slots_used=Σe_i`、`pool_slots_unused=capacity−used`、`pool_used_ratio=used/capacity`（`n=1` 按 0 处理），以及 `e_i≤C(n,2)` 数值关系。

独立重建并逐 pair 枚举了矩阵中 `n≤32` 的 96 个 run（336 个逐轮记录）；这些样本的轮内和跨轮边重复均为 0。独立 fixture 检查得到 `n=5,r=2,e=[7,1]`、容量 20、使用 8、未用 12。

复核边界：大于 32 的边计数没有在本轮重新执行递归随机生成；其具体 `e_i` 仍为 `EXPERIMENT_OBSERVED`。本轮验证了其 CSV 算术一致性和不超过宽松容量，而不是独立重现每条大 n 图。没有从三个输入/pivot seed 推导任何概率尾界。

### 4.3 代码与 CSV 中的计数器发现

1. **`graph_oracle_edges` 的 oracle 范围较窄（MINOR）**：脚本第 147–154 行先接收同一 `AAV86GraphCounter` 生成的 `snapshots`，再穷举 snapshot 上的 pair，检查“至少一端是本轮 pivot”这一边定义。它能独立核对 edge-list 枚举与给定 snapshots 的关系；它不独立验证 pivot 抽样、递归子问题、bucket 归属和完整递归图。报告宜称其为“snapshot 条件下的穷举边 oracle”。本轮另对 n≤32 的矩阵样本独立重建递归并枚举，补足了小规模证据；不补足大 n 的随机图重跑。
2. **排序和 bucket 是明文逻辑（NOTE）**：第 110–120 行读取 `Record.stable_key`，`sorted` / `bisect_left` 直接用明文 score 与原始下标决定 pivot 顺序和桶；`topk_mask` 也直接映射原始下标。这只核验图计数器，不是 CA/local-rank、安全 bucket 更新或 Protocol I runtime。
3. **`same_round_recursive_edges` 不是实测计数（MINOR）**：第 38 行 dataclass 默认初始化为 0，代码没有后续赋值。当前算法把子问题递归放到后续轮次，因此这个值只能解释为预设的结构性零值，不能作为一次计算出的指标。建议删除此列或改名/注明 `STRUCTURAL_ZERO`。
4. **`empty_buckets` 不包含最终层桶（MINOR）**：第 112–122 行每个非平凡子问题都会创建桶，但 `depth==1` 在递归桶之前直接返回排序结果；因此最后一轮创建的空桶没有递归调用 `_add_subproblem`，不会计入 `empty_buckets`。现值表示“实际递归访问的空子问题调用数”，不是所有轮次创建的空 bucket 总数。它不影响 `e_i`、图或 pool 容量，需收窄列名/解释。
5. **CSV 重复检查字段是方法标签，不是矩阵内逐 run 的检查结果（MINOR）**：`run_matrix` 将小 n 标为 `ENUMERATED_0`、大 n 标为 `PROVED_BY_PIVOT_REMOVAL`，但矩阵循环本身没有根据该 run 的 `edges_by_round` 做重复检测；小规模原脚本 self-test 会检查另一组小样本，大规模由递归不变量作分析论证。本轮对矩阵中 n≤32 的 96 个 run 独立检查了重复；对大 n，唯一性来自下面的结构论证，而不是该 CSV 字段执行了证明。

`PAPER_DERIVED / ANALYTICAL_BOUND`：同一轮的活动子问题来自不相交桶，因此每个原始端点只在一个活动子问题内；每个子问题按简单无向图生成边，故每轮 `e_i≤C(n,2)`。跨轮方面，一旦某端点成为 pivot，它不会进入后续桶；先前比较边至少有一个端点是 pivot，所以同一端点 pair 不会在后续轮重复。因而当前算法下全部轮次边的并集也不重复。这个结构结论不把期望复杂度变成最坏情况的紧界；比逐轮 `C(n,2)` 更紧的统一 worst-case bound 仍为 `UNKNOWN`。

## 5. uCMP 材料池与安全边界

### 5.1 已由源码支持的部分

- `EXPERIMENT_OBSERVED`：E1 harness 调用仓库 `ProtocolIUcmpMaterial(bits, mask_left, mask_right)`；实现将 `alpha=(mask_left−mask_right) mod 2^bits` 传给 `keyGenDCF(bits,64,alpha,1)`，再以 `export_party_material(0|1)` 导出不同 party 的真实 DCF key 序列化字节。它不是 mock key。这里“party-local key”仅表示按 party 区分的底层 key blob。
- `EXPERIMENT_OBSERVED`：`eval_strict_lt` 以两个 `evalDCF` 调用构造严格小于的 Boolean share；实验 bits 为 36，仓库原语接受 34–53 bits。比较函数/DCF 原语此前已有自己的 conformance；本轮未重跑该测试，也未从本次材料池实验推出通用输入范围证明。
- `EXPERIMENT_OBSERVED`：`generate_pool` 为每个 iteration、endpoint 生成一次 full mask，并将 additive shares 放入两方 pool；同轮所有关联端点 pair 的 key 使用这一相同端点 mask，因此邻边共享节点 mask。跨 iteration 从确定性 `mt19937_64` 继续取新的 mask/key 输入。该 PRNG 是 TEST_ONLY 确定性设置，不是生产随机源或密钥独立性证明。
- `PROJECT_CONTRACT / EXPERIMENT_OBSERVED`：PartyPool 的内存表按 `(iteration, endpoint_a, endpoint_b)` 查找；请求另外检查 version/session、party、bits、canonical endpoint 次序、invocation=0、已绑定 graph digest/edge membership、缺槽位和该进程内的重复消费。池 envelope 写入 session、party、n/r/bits、mask shares、槽位迭代/端点/invocation 和 key bytes。

### 5.2 绑定、one-shot 与序列化限制

- DCF key 本体没有 session、端点、graph digest 或 invocation 字段；它的数学参数包含掩码差，key header记录 party/bits。端点/session/iteration/invocation 的关联由实验的 `PartyPool` 表和 request 检查维护，不是 key 本身的密码学认证。误把一份 key blob 交给其他端点时，DCF 不会识别“端点标签错误”；正确性依赖 wrapper 路由。
- `bind_graph` 在单进程内存中保存 FNV64 字符串及允许边集合。FNV64 是非密码学标签，只能帮助发现本地调用不一致；不提供抗碰撞、真实性、完整性、保密或外部 transcript 绑定。pool `serialize()` 不包含 `graph_bindings_` 和 `consumed_`，也没有对应的完整 pool restore 流程。key 的序列化/反序列化会生成新的 party-material 对象，used 标志并不随 key bytes 持久化；`take()` 的 replay 拒绝仅在当前 PartyPool 实例的内存状态中成立。不能据此声称跨进程/重启的 durable one-shot 或防重放。
- E1 报告的 party envelope 是自定义 serializer 对象长度，不包含网络/TLS/framing；也不包含 graph allowlist/digest、消费状态、socket/transcript、完整 CA 消息或实际交付开销。因此它是 pool envelope 的序列化长度，不是完整协议通信量。
- harness 在同一进程创建并保留双方 party key、完整 mask、priority、输入 shares 和测试 oracle；它用完整 `z_a/z_b` 直接调用两方原语，并做 test-only share 重构。未实现真实 P0/P1 独立 OS 进程、T 离线交付后退出、在线消息 transcript、masked opening 传输、局部 rank 消息或安全 bucket 更新。它证明不了半诚实视图、T 不合谋假设下的安全性，也没有跑完整 AAV86 sorter/Top-K mask。

### 5.3 字节、容量、计时

| 项目 | E1 值 | 复审口径 |
|---|---:|---|
| 单个 party DCF key | 921 bytes | bits=36；从该 serializer 可按 `33 + 16(bits+1) + 8(1+bits)` 复算。报告的 921 是 n=5 fixture 的实测输出口径；不作为其他 bits 的物理测量。 |
| 每方 pool envelope | 18,963 bytes；双方共 37,926 bytes | n=5、r=2、bits=36 的自定义 `PartyPool::serialize()`；不含网络 framing/TLS，且不含 graph binding/消费状态。复审按源码结构复算长度。 |
| 离线生成时间 | 0.240075 ms；0.138949 ms | 两个单次值，不平均；计时包围 test-only 全池 mask-share、20 个 keygen 和序列化过程。仅 n=5、r=2、bits=36、单进程/确定性 PRNG。 |
| 本地 wrapper + uCMP eval | 0.065826 ms；0.073332 ms | 两个单次值，计时包围 8 条活跃边的单进程本地调用，不含真实网络/进程调度/CA bucket 更新。 |
| DCF eval 调用计数 | 112 | 按源码调用结构推算：8 边×2 方×每次 2 个 evalDCF=32；独立 validation pool 20 项×2 方×2=80。不是 profiler 测得的 CPU 调用 trace。 |

报告列出的时间值没有配套保存的 stdout/log artifact；本复审未重跑二进制，因此保留为 `EXPERIMENT_OBSERVED` 的两次单样本报告值，不作统计性能结论。其他 n 和 comparison bits 的物理 key bytes、RSS、分配峰值、网络传输、端到端 latency 均为 `NOT_MEASURED`。 E1 的 CMakeLists 声明 CMake 3.17 最低版本；E1 报告称本机 CMake 3.16.3 未能配置，随后通过直接 g++ 命令运行 harness，并报告 `MATERIAL_POOL_TEST PASS`。该环境/编译结果只按 E1 报告记录，本复审没有重新构建。

容量定义：

```text
N_pool(n,r) = r · C(n,2) logical slots per party
both parties together = 2 · r · C(n,2) party-key blobs
M_observed = max_i e_i within a particular sampled run
pool_slots_used = Σ_i e_i
```

这是 E1 的“每 pair、每 iteration 一个 endpoint-bound 槽位”实验契约。它为所有可能图边及每轮不同 mask 差预留材料，不等于实际活跃比较数，也不是所有可能材料设计的紧下界。当前 DCF/uCMP CmpAgg 入口要求 `C(n,2)` 条全对全 edge material，E1 使用独立 wrapper 消费活跃边，不等于生产 CmpAgg 支持 AAV86 动态子集。

| 样本 | `e_i` | `M_observed` | 每方全对两轮/多轮容量 | 本样本活跃槽 / 未用槽 |
|---|---|---:|---:|---:|
| n=5,r=2 固定 fixture | 7, 1 | 7 | 20 | 8 / 12 |
| n=32,r=3，输入/pivot seed 42/1042 | 90, 78, 24 | 90 | 1,488 | 192 / 1,296 |
| n=256,r=4，seed 7/1007 | 762, 1,065, 1,027, 694 | 1,065 | 130,560 | 3,548 / 127,012 |
| n=10,000,r=5，seed 1/1001 | 59,979; 59,718; 62,989; 72,260; 54,001 | 72,260 | 249,975,000 | 308,947 / 249,666,053 |
| n=100,000,r=2，seed 42/1042 | 31,549,914; 33,896,022 | 33,896,022 | 9,999,900,000 | 65,445,936 / 9,934,454,064 |

所有 `M_observed` 是有限 seed 样本的轮最大值，不是概率界或 worst-case bound。三组种子没有给出可接受超限概率。AAV86 的期望比较复杂度 `E[e_A]≤c(r)n^(1+1/r)` 也不推出固定 M 的无超限保证。

自定义 serializer 的容量推导可以从源码单独复算，但不是物理分配或实测：单 key 长度 `L_key(b)=57+24b`；每方 envelope 为 `63 + 8nr + r·C(n,2)·(20+L_key(b))` bytes。n=10^6、r=5、b=53 得 `3,372,496,667,500,063` bytes/party（约 3.37 PB 十进制），是 `ANALYTICAL_BOUND` 的 serializer 长度，不是实际创建的 pool/RSS/磁盘需求测量。

## 6. 固定 M 分情形分析

### 6.1 计数型 M：只限制比较次数

- 若 `M` 表示**每轮**比较上限，当前可证明的保守保障要求 `M≥C(n,2)`；若 `M<C(n,2)`，就只能接受特定输入/随机性下可能超限，或承认并定义 abort。`M` 本身没有端点和掩码差，不会生成对应 pair 的 key。
- 若 `M` 表示**整套协议**比较次数上限，保守容量为 `r·C(n,2)`；不能把每轮的 `M_observed` 当作总数。E1 的全两两材料容量是每方 `r·C(n,2)`，样本使用为 `Σe_i`。
- 固定 M 可以定义资源预算/拒绝条件，但不是 offline preprocessing 方案。超过 M 时当前没有生产级安全处理；不得请求 T 在线补发、静默退化或输出部分有效 mask。必须先有 fail-closed、无部分结果和 abort 泄露处理定义。
- 若声称概率型 M，尚缺输入分布、pivot randomness 下的尾界、目标失败概率与超限行为；目前为 `UNPROVEN`。

**判定：**作为计数或 abort policy 可被定义；作为“有 M 次比较就保证离线 key 覆盖”的主张，`NO-GO`。

### 6.2 端点绑定型材料池

一项真实 DCF/uCMP key 对应特定掩码差。若在线图在离线前未知，key 还必须有正确 endpoint pair、iteration、party、bits 和 one-shot 归属；同一轮同一节点的掩码需在所有 incident edges 上保持一致，跨轮 mask freshness 则需要相应的轮次材料。仅改变 slot number 不能把 alpha 为一组 mask 差生成的 key 变成另一对端点的 key。

在“任意合法输入、任意 AAV86 pivot randomness 均要覆盖”的语义下，当前可复核的保守容量是：

```text
per round: C(n,2) endpoint-pair candidates per party
whole protocol: r · C(n,2) iteration-bound slots per party
```

它与 E1 全两两池同阶。`M_observed` 低于该值只说明样本图稀疏，不证明未覆盖端点永不出现。若 graph endpoint/槽位访问序列对在线方可见，需把泄露纳入协议；若图本来就是 CA 公开输出，则也仍需说明哪些方看到图和查表模式。E1 的 FNV64/内存 allowlist 不构成安全图绑定。

**判定：**全两两候选池可作为明确的 `EXPERIMENTAL_ALL_PAIRS_OFFLINE_POOL` 对照；使用紧凑固定 M 替代它并仍保证所有图边，`NO-GO`。更紧的 endpoint-bound worst-case 容量 `UNKNOWN`。

### 6.3 可重绑定通用材料

要在图确定后把预生成材料安全地绑定到任意 endpoint pair，必须给出与 `keyGenDCF(bits,64,mask_a−mask_b,1)` 分布相容的实际相关随机性构造，并证明：端点掩码差正确、共享节点 mask 的多边联合分布安全、对自适应图访问安全、party view/T view 合规、材料 one-shot/domain separation、查表访问模式的泄露边界。不能靠重命名 endpoint、重排 M 个槽位或忽略每对 mask difference 实现重绑定。

E1 没有这样的原语/构造；仓库 uCMP keygen 是输入掩码差绑定的 DCF keygen。也没有多边联合安全、adaptive lookup simulation 或重放/刷新证明。

**判定：**通用可重绑定材料在理论上是否可能，当前 `UNRESOLVED`；对本项目而言，没有构造与证明前不得实现或把它当作 M 方案，当前实施判定 `NO-GO`。

## 7. 发现分级与建议的精确措辞

| 等级 | 发现 | 对 E1 结论的影响 |
|---|---|---|
| `MAJOR`（runtime 边界） | C++ 池在单进程同时持有双方 key、全掩码和 oracle；slot/graph/one-shot 依赖可变内存 wrapper。pool serializer 不含 graph binding 或 consumed 状态，跨进程恢复/重放安全未实现。 | 不推翻“真实底层 party-specific DCF key 曾在 TEST_ONLY harness 生成/消费”的窄结论；阻止将此实验当作协议集成、party isolation 或安全预处理证据。 |
| `MINOR` | `graph_oracle_edges` 以同一生成器 snapshots 为条件，不覆盖完整递归构造；matrix CSV 的重复检查字符串是方法标签，而非矩阵循环内逐 run 检验。 | 需收窄 E1 的 oracle/CSV 字段措辞。独立复审的 n≤32 矩阵递归枚举补充了 96 个小样本。 |
| `MINOR` | `empty_buckets` 不统计最后一轮建立但未递归的桶；`same_round_recursive_edges` 从未写入，固定为默认 0。 | 这两个字段不参与 `e_i`、slot 数或容量结论；不可称为实测字段。 |
| `NOTE` | 大 n 的图未在本轮重新生成；计时仅有报告中的两次单样本值，未见原始 stdout/log artifact。 | 大 n `e_i` 和两次 timing 仍按 E1 的实验观察报告，不能说本复审独立复跑/性能复现。 |
| `NOTE` | 36-bit 物理样本只有 n=5/r=2；测试计时、921-byte key 和 envelope 均不含真实线上交付/网络。 | 不外推其他 n、bits、RSS、网络或端到端 latency。 |

建议将 E1 报告的相关句子精确调整为：

1. “小规模 oracle”改为“对 generator 已生成的 snapshots 穷举 pair，核对 pivot-edge predicate；递归独立性由另外的测试/审查支持”。
2. 将 `empty_buckets` 解释为“非最终深度实际递归访问到的空子问题”，或另行增加真正遍历最终桶的计数；将 `same_round_recursive_edges` 标记为 `STRUCTURAL_ZERO`/非实测。
3. 将 CSV `ENUMERATED_0` / `PROVED_BY_PIVOT_REMOVAL` 说明为方法/论证类别，不写成每个矩阵行都运行了重复检查。
4. 将“key 绑定端点/session/图”改为“TEST_ONLY pool wrapper 以端点/session/图标签路由并拒绝不匹配请求；底层 DCF key 本身只含 DCF key 与 party/bits，graph digest 没有密码学认证”。
5. 将“one-shot”限定为“当前进程 PartyPool 实例中的一次消费”；持久化 replay、pool restore、独立进程传递均 `NOT_IMPLEMENTED`。

未修改 E1 原报告或任何 E1 文件；上述为独立复审 errata 建议，不覆盖原记录。

## 8. M5/G3 与项目状态边界

主工作区 `PROJECT.md` 和接收签收记录将 M5 G3 记为 PASS，并保留 `AUTHOR_EXACT = NOT_PROVEN`。这次 E1 复审不改变 M5 历史结论，也不把工程接收变成作者精确复现证明。当前 M6A 仍需分别完成 Protocol I+AAV86 与 Protocol III+AAV86；本报告只处理 Protocol I 预处理设计审查，没有评估或推进 Protocol III 的 `2r` 目标。

## 9. 未测项与后续门槛

仍为 `NOT_MEASURED` 或 `UNPROVEN`：

- AAV86 大 n 计数的第二实现重跑、比 `C(n,2)` 更紧的统一 worst-case bound、所需的超限失败概率；
- 输入分布与 pivot randomness 下的 `M` 尾概率、可接受失败概率和超限处理；
- DCF key 的跨边联合隐私、共享节点掩码相关性、自适应图/lookup 模拟；
- 图、party pool、session/round/endpoint 的认证绑定，以及跨进程 one-shot/replay protection；
- 独立 P0/P1 进程、T 的离线视图/退出/擦除、无合谋假设下的模拟安全证明；
- masked input opening、local-rank reveal、secure bucket update、完整 AAV86 sort、原顺序 Top-K mask 适配和端到端轮数/通信；
- 除 n=5、bits=36 外的真实 key bytes、RSS、pool allocation、网络 framing/TLS、运行时间。

建议继续把全两两路线保留为隔离对照，并维持 `P2-I SECURE RUNTIME = NO-GO`。如要重新评估固定 M，先提交数学上明确的方案说明：M 的作用域、keygen 输入/联合分布、端点掩码差如何在 adaptive edge 确定后匹配、查询模式泄露、超限 abort 行为、one-shot/刷新、概率或最坏情况保证，以及 party/T 安全视图。缺任一项时，不能以三个种子或 `M_observed` 进入 runtime 实现。

## 10. 文件差异与执行结果

本次新增文件仅为：

- `docs/reviews/M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md`（本报告）

没有修改实验 worktree 中已有的 E1 报告、Python、CSV、fixture、C++ harness 或 CMakeLists；没有改主工作区文件、`VFSS/`、`VFSS-baseline/`、论文、本地参考树、密钥或构建物。未暂存、提交、推送、合并或创建 PR。

只读复核结果：`INDEPENDENT_CSV_AUDIT PASS`；`INDEPENDENT_SMALL_GRAPH_ENUMERATION PASS`；`FIXTURE_AUDIT PASS`。未运行仓库测试或 E1 C++ harness。
