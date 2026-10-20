# M6B-I S3 工作报告：按 L 复核并推进 N=128 Protocol I 候选

日期：2026-10-12（Asia/Shanghai）  
任务：`M6B-I-PROJECT-SELECT-5ITER-S3`  
算法身份：`PROJECT_DERIVED_SELECT_5ITER`  
组合候选：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF`  
裁决：明文 Select 与 S2 复核 `PASS_WITH_SCOPE`；N=128 已知容量按提议格式可容纳；Protocol I runtime 接入 `NO_GO_FOR_INTEGRATION`。本轮未写 VFSS runtime、未创建 opt-in 入口、未运行协议 E2E 或正式性能矩阵。

## 1. 工作区、来源身份与独立接收边界

| 项目 | 本轮核实 |
|---|---|
| 主 checkout | `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation`；本轮未写入 |
| 主 checkout branch / HEAD | `feat/m6a-performance-evaluation` / `c3926c68fd14f270faa8b55234311071947fa080`，与其 upstream `origin/feat/m6a-performance-evaluation` 一致 |
| 主 checkout 既有用户差异 | 修改 `.agents/skills/protocol-reproduction/SKILL.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md`；未跟踪 M6A 两份决策、source inventory 报告和 `siamjdiscrmath.pdf`。均未改动 |
| S3 实际工作树 | `C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation` |
| 分支 / HEAD | `codex/m6b-project-derived-5iter-s3` / `c3926c68fd14f270faa8b55234311071947fa080` |
| 本地 `main` / `origin/main` / live `origin/main` | 全部为 `c3926c68fd14f270faa8b55234311071947fa080`；live `git ls-remote origin refs/heads/main` 同值 |
| 隔离方式 | S3 分支从此前 S1/S2 worktree 的基线建立；S1/S2 未提交工作仍在分支工作区，未覆盖。未做 commit |
| Python | `Python 3.13.7`；主机 Windows 11（S2 接收证据登记为 `10.0.26200-SP0`） |
| 构建工具 | 本 worktree 未找到构建目录或 `protocol_i_ucmp_conformance_test.exe`；`cmake` 不在 PATH。未执行 C++/DCF build |

进入 S3 前，此隔离 worktree 已有 S1/S2 的 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md` 修改，S1/S2 决策及报告未跟踪，及实验目录未跟踪。它们是继承的脏工作，不是 S3 新建的历史文件；本轮不改写 S1/S2 原始结果或 papers 清单。桌面主 checkout 状态与上述既有差异一致，本轮未写入。

S2 原始接收报告由本轮另一审查线程只读核对源码身份、rank 不变量、边定义与成本条件。本轮还直接检查了外置 `run7/independent_summary.json`、失败 JSONL 与结果文件哈希。审查线程未运行代码；S3 也没有重放 40,490 案例。因此 `S2_RECEIPT=PASS_WITH_SCOPE` 表示对已产生的明文证据和证明链接受，不表示本轮独立复跑/第二份 selector，也不表示安全协议接收。

### 1.1 固定源码与历史证据身份

| 对象 | SHA-256 | 接收用途 |
|---|---|---|
| S1 selector `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/select5iter.py` | `8b8d3dbd087d601a4962fe61c7b35546aa834cebd0ed4736b4e6d5dca204606b` | 唯一被接收的 plaintext selector 源码 |
| S2 verifier `independent_accept.py` | `6dbd5b14a01e6de2796afde4624a953e5456b75c42e11a00ebe6de1f76490883` | 独立重建 sample-clique 顺序、边、状态与 oracle 对照 |
| S2 cost `comparison_cost_bounds.py` | `3810a095f7552ef233da17d15458ef4f24d976b3f024b94acb975f9942e6b40e` | 小规模 DP/路径枚举及有限期望上界 |
| S2 access `material_access_prototype.py` | `3444405e0b1a782619bb4f9c9338e7d0b095f006eb2e6105f9aba06b6bf56a2d` | 先前材料接入阻塞原型 |
| S1 选择来源报告 | `b98ce668897631276740e04fe334412f1578b66ab25b0993f7608c4a819fdab0` | 保留来源 NO-GO，不被 S3 改写 |
| S1 项目派生算法报告 | `09e751bd96f514a3c067dcbd7de503b9005945d359769fa08334f5bc5d47ae43` | Stage A 的 dated 报告 |
| S2 接收报告 | `ac4fb63cdbeabaa861e89e036b6fe1e1b389b46e5012a69f80458a183cfe3653` | S2 原始报告，保持不变 |
| S2 接收决策 | `8e34cc20eba3028f61a8d14c05e81814ab16df2ddc94e39a1d9412397ee6814c` | S2 独立接收和 Stage B NO-GO |
| S2 run7 `independent_summary.json` | `d75f5d0e09e04f953080e9effa08c26c33a1eedfc8784e86f4b8530181defc76` | 40,490 PASS 的原始摘要 |
| S2 run7 `independent_results.jsonl` | `bdf47fc66a1a17b9912b7b4fbc1efbac2b39a089e550455a4de0ec9b3e910751` | 结果文件 |
| S2 run7 `independent_failures.jsonl` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | 空失败文件 |
| S2 run7 `s2_frozen_oracle.exe` | `694c54f1e2b6200a3fc54cd721e3cbfbad9f15cf3d711fc173d0306f2f76c36a` | 已编译冻结 oracle，未在 S3 重编 |

### 1.2 被接收的算法事实与成本口径

稳定全序为 signed Q20.12 score 降序、`original_index` 升序。四次收窄每轮先 sample clique 再 pivot stars；第五层对剩余候选全 clique 精确闭合。`(C,r)` 的 1-based rank 不变量由 sample clique 和两 pivot 的全候选星边保持；第五层比较确定精确第 r 项。因此选择结果对**所有合法采样 tape** 正确，采样只影响成本。算法迭代数为 5，比较依赖层为 9；这不是五轮/九轮协议保证。

S2 报告的有限矩阵比较上界是项目推导：在每轮从当前候选集合使用新鲜、输入无关、理想 uniform-without-replacement sampler 的条件下，目标矩阵期望逻辑调用为 `O(n)`；没有高概率成本界，最坏 `O(n²)`。N=128、K=2/8 共用的有限上界为期望总比较 `298..5,152`、各轮上界 `[298,294,290,285,3,985]`，joint worst upper `8,308`。这是理论上界，不是 S3 实测均值。S3 没有重跑 S2 的全矩阵 DP 或 40,490 oracle 套件。

## 2. S2 独立接收复核

run7 摘要实测字段：`40,490/40,490` PASS，失败 0；其中 `n≤7` 全排列/全部 K 为 40,319 例；60 个案例确实有五个非空比较层；每个固定输入的 32 个 algorithm seed 产生 32 个不同首轮样本；5 个非法输入被拒绝。原报告记录的 checker run1/run2/run3 错误及失败样本保持在 S2 报告，不混成算法失败，也未被本轮删除。

接收范围只包括 selector/oracle 输出正确、trace edge 规则和不变量推导可复核、成本推导假设明确。S2 verifier 调用 S1 selector 生成 trace 再独立审计 trace，因此不是完全独立的第二个 Select 实现；它也不能证明秘密共享正确或概率安全。

本轮无目标地重跑整个套件不增加接收独立性。独立线程审计加固定源码 hash 是本次选定范围；协议相关的新检查改为泄露投影、一次性槽身份和 membership 方向小探针。

## 3. 新确认的泄露函数 `L_S3`

约定接收方为两个在线方 P0/P1；坐标均是 shuffle 后匿名坐标，除输入适配阶段另行标明。此表登记用户已批准公开项。会话/尺寸/阶段是当前消息配置头里的公共运行上下文。

| 字段 | 生成方 | 接收方 | 坐标 / 表示 | 公开时机 | 由已公开字段能否推导 |
|---|---|---|---|---|---|
| `n,K,N,width,session,phase` 运行配置 | 调用方/包头 | P0、P1；离线包由 T 按公开配置生成 | 逻辑与 padded 尺寸、稳定 rank | 启动/包绑定 | 后续所有长度和 padded mask 维度可推导；不暴露 raw score |
| `edge=(left,right)` 比较端点 | 双方依据当前 public schedule 一致生成并确认 | P0、P1 | shuffle 后位置；`left<right` 规范方向 | 比较材料访问前、每个图冻结时 | 可推导逻辑调用集合、图顶点并集、活跃候选规模（图非空时）、边数、下次 slot 地址；不能推导比较结果或原始位置 |
| `pivot_position=(p_x,p_y)` | 秘密比较/选择模块输出后由双方公开 | P0、P1 | shuffle 后位置；有低/高 pivot 角色 | sample clique 后、pivot stars 前 | 可推导 star endpoints 和 pivots 的打乱身份；不直接给出明文 pivot 值或原始位置。与自适应选择规则联用可能泄露 rank 相关信息，需纳入模拟 |
| `key_slot_id`（Select/membership） | 确定性地址函数或 T 的离线目录 | P0、P1 | session/algorithm/iteration/purpose/substage-use/direction/端点绑定；shuffle 后匿名坐标 | 每个 one-shot 材料槽消费前/消费时 | 由地址元组推导槽身份和访问顺序；同一 ID 不得二次调用，跨迭代相同端点必须新 ID |
| `key_slot_id`（raw carry/sign） | T 离线目录 | P0、P1 | pre-shuffle 输入 slot / carry 或 sign purpose；使用 session-scoped opaque ID，不编码 shuffle 位置 | 相应转换 DCF Eval 前 | 可推导输入转换调用位置/阶段；不公开该输入最终到哪个 shuffled endpoint |
| `comparison_count` | 比较调度器 | P0、P1 | 每次迭代及累计 logical calls | 图冻结后/层执行时 | 若端点完整公开，可由端点列表求和；作为压缩计量字段不增加独立信息 |
| `communication_length` | transport/frame 层 | P0、P1 | 每消息 payload/frame 长度 | 每次发送时 | 在固定 ABI 下可由公开端点数与格式推导部分长度；实际可继续揭示阶段/错误路径长度，单独计入 L |
| 统一 `ABORT`/成功状态 | 双方 barrier | P0、P1 | 不含错误原因/rank/阈值 | 失败协调或最终 mask 释放前 | 可揭示运行是否完成，但不得附带本地异常详情；失败时双方都不得输出 mask |

比较结果 DCF shares、masked operands、raw-score、阈值、`original_index → shuffled_position` 映射、秘密 mask 明文均不在已批准 `L_S3`。

### 3.1 公开 transcript 可以推出什么

在本算法活动图中，sample clique 覆盖所有 sample nodes；两个 pivot stars 触及每个非 sample 候选。因此只要 `|C|≥2`，该轮端点并集正好暴露打乱域候选集 `C` 和大小。singleton terminal 轮没有边，端点不指明该单点；不能为方便另发 `terminal_selected_position`。若公开 slot ID 带 `sample_clique` purpose，sample 集合也可由这些槽端点导出。端点、pivot、iteration 和 purpose/substage slot ID 一起允许双方从获批 transcript 计算候选图和访问顺序，不需要单独开放 active-C bitmap。

端点图本身不包含比较结果 share，也不能按代码数据流导出 rank 或 residual `r`。purpose/substage slot ID 用于双方一致区分 sample-clique 与 pivot-star 调度，不需额外发 sample ID 列表。但 pivot 两角色和选择算法的分布会泄露关于目标 rank 的信息；本轮没有量化 mutual information 或构造 simulator，故只把它视为 `L_S3` 明确接受的输入相关泄露，而不宣称 pivot transcript 不泄露 rank 信息。公开 comparison outcome bit 不获批准；若打开，每层每条边的结果 bit 将是 P0/P1 新获知的输入依赖字段，必须保持秘密并由 share-domain control 原语消费。

### 3.2 相同配置的 transcript 对照

`public_view_probe.py` 固定 N=128、K=8、算法 seed `0x5A2`、session `0x50330001` 和一个由 seed `0x51A7` 生成的相同隐藏置换。两个输入是同一 `[0,127]` 多重集的不同排列。对 S1 trace 仅投影 `L_S3` 字段：

| 项 | 输入 A | 输入 B |
|---|---:|---:|
| 每迭代比较计数 | `[298,265,0,0,0]` | `[298,251,24,13,3]` |
| logical calls | 563 | 589 |
| 唯一无序 endpoint pair | 562 | 589 |
| 跨迭代重复调用 | 1 | 0 |
| one-shot slots | 563 | 589 |
| endpoints / pivots / slot ids | 与 B 不同 | 与 A 不同 |
| communication bytes / unified abort | `NOT_MEASURED` / `NOT_RUN` | `NOT_MEASURED` / `NOT_RUN` |

unexpected projected fields 为 `[]`。这是输入顺序改变时，获批自适应图/计数/槽字段预期变化的明文 trace 对照，不是两个 party view、secret shuffle、masked operands、通信或安全攻击模拟。所有样本值、raw index mapping、outcome、rank、阈值均未放入投影。

## 4. 自适应状态与各方计算边界

按获批 L，双方可从公开 endpoint/pivot 记录一致重建：当轮冻结图、`C` 顶点并集、样本/星边角色（结合算法标签与 pivot 字段）、计数和每个 one-shot slot 地址。Dealer 预发完整 pool，不需要知道输入图；在线不请求 Dealer。

秘密比较 share 聚合成 pivot rank、由 residual `r` 算出样本 quantile `qL/qH`、按 pivot rank 选择下一候选支路、压缩状态、在第五层选 τ，均不可以通过打开 comparator bits/ranks 实现。当前 runtime 没有对应 secure arithmetic/control/mux 接口。P0/P1 可公开已经被批准的 pivot positions，但要先有一个能在 shares 域选出位置并输出该 approved 字段的模块。当前 S1 Python 直接读 plaintext scores 排序 sample，不能作为 party 控制逻辑。

若任一实现选择公开比较位，额外字段应精确登记为：`cmp_bit(iteration, substage, shuffled_left, shuffled_right)`，生成方为比较双方/运行时，接收方 P0/P1，比较层完成后每条边 1 bit，用途是 rank/candidate update。它不属于 `L_S3`，本轮禁止采用。`residual_rank`/`pivot_rank` 若打开则另是每个 pivot 的整数 rank，接收 P0/P1，发生于每轮 star 后；同样不允许。

### 4.1 需要新增确认或替代 API 的字段

直接复用当前 uCMP 时，双方需要共同输入同一个 masked operand。已有接口在 comparator 前交换 masked local shares 并将其相加重构；当前 Protocol I priority pipeline 也会打开完整 masked-key 列表。S3 批准的 L 未包含以下 operand 值，不能由 endpoint/count/length 推导：

| 新字段 | 观察方 | 时间/坐标 | API 用途 | 状态 |
|---|---|---|---|---|
| `z_carry[slot,operand]` 与 `z_sign[slot,operand]`，每个输入转换阶段每个 padded slot 两个 34-bit operand | P0/P1 | shuffle 之前，输入 slot 为原序位置；每阶段 uCMP Eval 前 | 现有 raw-score Q20.12 转换 DCF Eval | `L_PENDING`；须确认公开带来的字段/复用模拟，或提供 share-only 转换 API |
| `z_left,z_right` | P0/P1 | 已打乱 endpoint pair、iteration/substage；select uCMP Eval 前 | 为自适应比较调用当前 public-input DCF | `L_PENDING`；须确认为获批 L 扩展或换 comparator |
| `z_tau,z_i` | P0/P1 | `(virtual_tau, shuffled_i)` membership endpoint；membership Eval 前 | 计算 `strict_lt(τ,key_i)` | `L_PENDING`；τ 仍需保持秘密，masked operand 也不能直接当作已批准 |

这里的 `z` 是 DCF 的共同 masked ring input，不是明文 key/score。它是否可被视为无信息泄漏必须根据随机 mask 的一次性/复用、候选位置、输入转换坐标和两个 party 的合并视图专门证明；本报告不把它描述成明文值，也不替代项目负责人对新增公开字段的决定。已批准端点、pivot、slot、count、length、统一 ABORT 项不再要求确认。

## 5. N=128 材料与接口准入

### 5.1 预算假设及公式

先给每个可能 `iteration × unordered endpoint pair` 预生成独立 uCMP key，避免在线 Dealer 和数据依赖 keygen。`C(128,2)=8,128`；五迭代全池 `40,640` slot。公开使用地址绑定 `(session, algorithm, iteration, purpose/substage, direction, left, right)`；物理材料池可按 iteration-scoped generic Select purpose 寻址，因为同轮 sample-clique 边只连 sample/sample，pivot-star 边只连 pivot/non-sample，两类端点集合不相交；两 pivot 与同一 non-sample 的边也彼此不同。同一 endpoint pair 因此在同轮至多一次，领取记录可绑定实际 substage；同边跨轮由 iteration 区分并领新 key。材料 descriptor 24 B，含固定 schema 下的 slot/阶段/方向/端点字段。每次调用另计两个新鲜 64-bit mask shares (16 B)，不假设 endpoint mask 跨边/跨迭代复用。slot access 本身获批公开，故采用确定性直接地址，不要求 ORAM；若以后隐藏地址则需重新预算并重做 simulator。

现有 `ProtocolIUcmpPartyMaterial::serialize` 长度从源码可复算为 `D(b)=57+24b`：40-bit comparator key `D(40)=1,017 B`，34-bit输入转换 key `D(34)=873 B`。下表是 proposed round-aware sidecar，不是当前 M2PK bytes，也不是已生成材料：

| 组件/方 | 槽公式 | 单槽布局 | bytes/party |
|---|---:|---:|---:|
| 四次迭代+第五层可能 Select edges | `5*C(128,2)=40,640` | `D(40)+24 descriptor+16 fresh masks=1,057` | `42,956,480` |
| raw-score carry/sign 转换 | `2N=256` | `D(34)+40`（现有输入记录尺寸） | `233,728` |
| threshold membership | `N=128` | `D(40)+24 descriptor+16 fresh masks=1,057` | `135,296` |
| 当前包头+N 个节点 mask share/计数 | `40+8N+8` | — | `1,072` |
| **已知 sidecar 合计** | **41,024 DCF key slots** | — | **43,326,576 B = 41.319 MiB** |
| 当前 shuffle retained vectors | 四个 PS material/party | PO 53,760×2 + DO 82,944×2 + own perm/inverse 1,024 | `274,432` |
| **已知逻辑向量总计** | sidecar + shuffle | 不含对象/allocator/OT wire | **43,601,008 B = 41.581 MiB** |

两方 uCMP 每个 slot 各调用两次 DCF Eval，因此最大 `82,048 Eval/party`，两方共 `164,096 Eval`。此数按输入/全 Select pool/membership 的最坏预备池计，不是实际调用数。N=128、K=2/8 的逻辑 Select 成本上界远低于全池容量；K 不改变上述已知字节。完整 sidecar 若与 shuffle 合并发送，现有 chunk framing 的 `chunk_data=1,048,560 B`，分 42 帧，`42×(16+48)=2,688 B` wrapper/header，合计 `43,603,696 B`。该逻辑包低于 64 MiB `23,507,856 B`；TLS record、OT wire 和 in-memory index/allocator 均 `NOT_MEASURED`。

对照 S2：S2 的 key-pool byte 下界指出该固定 M2PK/整包接口不兼容，并显示较大 N 会超字节限。本轮将 N=128 的约 43 MB sidecar 下界补成逐调用 mask/descriptor 格式，并加上已知 shuffle logical vectors 与 frame wrapper。补充后的 known components 仍低于 64 MiB；这不改 S2 报告历史，也不消除 shape incompatibility。40,640 低于 1,000,000 edge cap，但当前 serializer/runtime 只接受恰好 `C(N,2)=8,128` 个无 iteration 字段的 `edge_materials`，所以形状仍会被拒绝。

### 5.2 Shuffle 与输入材料明细

`N=128,T=2` 的 Beneš depth `d=13`，每个 PS preprocessing 有 `dN/T=832` translation；当前 forward/reverse 共四个 PS call/party，按现有计数器为 `6,656` OPV instances 和 `6,656` chosen OT items/party。保留的 PO/DO logical vectors 如表。OT wire 和 dealer key-generation工作区不是序列化向量大小，未测。

raw-score adapter 当前每个 carry/sign stage 交换 `2N` 个 64-bit word，即 `16N=2,048 B` payload/party/stage。两阶段在标准 48 B `ProtocolIFramedChannel` header 下，P0/P1 各 send `4,192 B`、receive `4,192 B`（每阶段一次 send、一次 receive；不含 TLS）。这是现有 adapter 的源码计数，不表示整个候选包的在线 bytes。

当前 shuffle 每次 forward/reverse 由两个 PS handshakes 组成。`d=13,N=128`，每个 DO payload 为 `24N(d+1)=43,008 B`，每条 PS channel 另有 56 B hello。每个方向每方一条 DO 数据 send 加两条 hello，故本地统计的 send/receive 为 `43,120 B`；forward+reverse 合计每方各 `86,240 B` send 与 receive，4 个 PS round。现 API 使用 fd 和自定义 56 B hello；在 tracked `VFSS/include,src,tests` 未检索到 TLS/SSL/mTLS 实现，故不得报告已接入 mTLS。

候选在线总量只能写：

```text
B_send/party = 4,192(raw input) + 86,240(forward/reverse shuffle)
               + B_select_operands + B_public_schedule + B_secret_control
               + B_tau/membership + B_abort + B_transport_TLS
```

未闭合项不是 0；对应 receive 同理。精确 candidate frame count/bytes 在 operands 与控制 API 落定前为 `NOT_MEASURED`。

### 5.3 当前 ABI 的精确阻塞

- `M2PK v2` 只编码 lexicographic `C(N,2)` 一层并强校验端点，无 iteration/substage/purpose/direction 字段。`protocol_i_priority_pipeline_party` 也要求全图 `C(N,2)`，会打开完整 masked-key 列表和 shuffled rank；不能调用为自适应五迭代协议。
- 新接口需要单独版本化 sidecar，使用本任务的 canonical slot address，执行每 key 一次消费、跨轮 fresh 和 duplicate/replay reject。T 按 `(session,N,K,algorithm schema)` 生成完整输入无关 pool 后退出；不拿到 input/raw score、rank、pivot key、τ 或在线结果，不作为在线 Dealer。
- 新 L 已允许端点/槽公开，因此不再把 ORAM 当作 N=128 必要 blocker；但 whole-pool adaptive-query transcript 和 used/unused key 的一方视图仍须单独证明，S31 对另一 Select 的 pool simulator 不自动适用。

### 5.4 当前 uCMP/DCF ABI 与可复用安全前提

`ProtocolIUcmpMaterial` 的源码行为是对比较 ring `b∈[34,53]` 计算 `alpha=(mask_left-mask_right) mod 2^b`，调用 `keyGenDCF(b,64,alpha,1)`；每 party 的 `eval_strict_lt` 对两个变换后的输入各运行一次 DCF Eval，因此两方合计每逻辑 uCMP 比较有四次 DCF Eval。`ProtocolIUcmpPartyMaterial` 的 M2UC v1 serializer 固定 `Bout=64,groupSize=1`，序列化长度 `D(b)=57+24b`。单个内存对象的 `used_` 防止同对象重复 Eval；serialized key 不自带 session/iteration/edge 身份，也没有持久 replay registry。现 M2PK 由外层绑定 session/fingerprint 和单层 endpoint，不提供跨迭代 purpose 地址。

因此可复用的候选底层假设仅限：同一 DCF 版本/输出域、同一 `b`、同一 `alpha` 方向、正确生成的一方 key 和独立 one-shot 查询时，单 key DCF 函数可以调用同一来源证明。将这些单 key 结论复合到输入相关 graph、整池 used/unused keys、公开 masked `z` operands、secret rank control、inverse shuffle 和 abort 仍需新 proof。新 sidecar 必须在外层严格校验 `(session,algorithm,iteration,purpose,substage claim,endpoint,direction)` 并防止重复导入/重领，不能仅依赖 M2UC 内存 `used_`。

## 6. membership、原序 mask 与失败路径

### 6.1 稳定 key 与方向

当前 `protocol_i_priority_key` 做：

```text
ordered = raw_score XOR 0x80000000
high    = UINT32_MAX - ordered
key     = (high << index_bits) | original_index
```

对 N=128，`index_bits=7`、打包 key 为 39 bit；comparison ring 为 `33+7=40` bit。数值越小表示 signed score 越大；同分时 index 越小，故恰为稳定 Top-K 次序。padding 使用 `INT32_MIN` 且 index≥logical n，排在全部 logical item 后。

令 τ 为第 K 小打包 key；严格 ABI 是 `strict_lt(left,right)=[left<right]`，所以 membership 查询顺序为 `(τ,key_i)` 并取补：`M_i=1-strict_lt(τ,key_i)=[key_i≤τ]`。当 `key_i=τ` 时 strict 比较为 0、M 为 1。稳定 key 唯一，logical mask 恰 K 位；dummy 不满足 `key_i≤τ`。加法分享低位投影到 `Z_2` 等于 XOR，因此若 DCF 输出按 `Z_(2^64)` additive share，取各自 LSB 可形成 XOR shares；这一映射尚未通过该组合的 runtime test。

新建 `membership_direction_probe.py` 对 4 个边界/重复 fixture 的所有 K 共 27 项检查验证 packed key 单调方向、稳定 index 和 complement/count 等式。结果为代数 `PASS`，明确标注不运行 DCF。活动 checkout 中已有 `protocol_i_ucmp_conformance_test.cpp`，其测试涵盖 34–53 bit 下 mask/boundary cases；本轮没有构建或运行该 C++ 测试，不能将 Python 算式检查写成 DCF conformance。

### 6.2 输出与 ABORT 合同

拟议流程为：隐藏 forward shuffle 后形成稳定 key share → 每轮 Select 输出秘密 `(C,r)` 状态 → 第五层得到秘密 `τ` share → padded N 个位置批量算 membership shares → 每 party 仅放 membership share 的 LSB 到 carrier → 使用同一 shuffle 的 inverse route → 只释放原输入顺序 XOR shares。TEST_ONLY controller 可重构验证 mask 重量为 K；P0/P1/Dealer secure path 不得重构阈值或 mask 明文。

每阶段任一方遇到错误 key binding、重复领用、timeout/disconnect、异常图、内存拒绝或 internal invariant failure，应让双方进入同一公开 `ABORT` 状态并且双方不返回 mask。现有组件的异常多为本地 throw；本路线没有可审查的全程 barrier/一致 no-mask 结果。不能把一次局部失败当“允许算法失败”后仍输出错误阈值。

## 7. 轮数、消息 DAG 与可复用范围

| 层次 | 计数 | 状态与原因 |
|---|---:|---|
| 算法迭代 | 5 | 四轮收窄 + 一轮候选精确闭合 |
| 明文比较依赖层 | 9 | 四组 sample clique → pivot stars（8 层）+ final candidate clique（1 层） |
| CA 通用条件映射 | `2×9+1=19` | 仅在本自适应控制图满足对应 compiler 定理全部前提时；S3 未证明适用 |
| Protocol I 论文核心 | 3 | 原论文参考目标，不是本组合实测/定理 |
| 现有 priority engineering core | 6 | forward 2 + 固定 CmpAgg 1 + rank reveal 1 + reverse 2；不是新路线 |
| 现有 raw-score→mask engineering path | 8 | 现有 raw adapter 2 + 现有 pipeline 6；不是新路线 |
| 本组合 Protocol I core / 完整 raw-score→原序 mask | `NOT_CLOSED` | control、masked operands、membership 和 ABORT barrier 尚无 ABI，不声称 11/14 或其它总轮数 |

**真实因果 DAG**：

```text
raw Q20.12 shares
  → carry masked-input exchange / local DCF
  → sign masked-input exchange / local DCF
  → forward secret shuffle (PS round 1 → PS round 2)
  → iteration i=1..4:
      public sample graph → sample-clique compare layer
      → secret sample order/rank shares and qL/qH pivot mux [MISSING]
      → approved shuffled pivot positions + derived pivot-star graph
      → pivot-star compare layer
      → secret pivot rank, branch, candidate/rank update [MISSING]
  → final candidate-clique compare layer
  → secret exact rank select / τ share [MISSING]
  → N DCF membership calls (operand path/L pending)
  → inverse secret shuffle (PS round 3 → PS round 4)
  → joint success/ABORT/no-mask barrier [MISSING]
  → original-order XOR shares
```

九层比较只表示 comparator 之间的 data dependence；每层送 masked operands、协议交互及 secure control 都会有 wire phases。不能用 nine layers 或 CA 条件数代替 complete path。

S31 可作为来源线索引用的仅有同类 DCF 单 key premise（须再次确认 M2UC ABI、domain、function direction/output 和一次性调用完全一致）及指标定义。S31 的 whole-pool/adaptive-query/shuffle/output joint simulator、安全结论与其路线特定 L 不转移；S32 计量口径可参考，但结果数据不是本路线结果。S1 BB90 论文结论及 BMW16-derived 证明均不继承。

## 8. S3 实际运行和复现

运行命令（证据写到仓库外）：

```powershell
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/public_view_probe.py --out C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S3_2026-10-12\public_view_probe.json
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/material_budget.py --out C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S3_2026-10-12\n128_material_budget.json
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S3/membership_direction_probe.py --out C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S3_2026-10-12\membership_direction_probe.json
```

S3 probe hashes：

| File | SHA-256 |
|---|---|
| `public_view_probe.py` | `37d06d0367fcc484492e44218c4d0507cbe88e36860188fa53038939ffe48e18` |
| `material_budget.py` | `e14c52f389fc426637e2d54a41f561c7cccb46b59128c17677a012e20b6f6218` |
| `membership_direction_probe.py` | `76a27dd56fb7f3e7fdf42914ee7cfc9f4727de427e41e59db2c5571a79d1c912` |
| `README.md` | `ee6a26592ef64eb2ec1feeeb2ff6615326f645a3769d8d9dcf93fe96f89e070b` |
| `public_view_probe.json` | `8c3f8f70ea8c7bce1da0f1ae77442ce1ea0fc64b2a28b012d390bf83dc1f5ca2` |
| `n128_material_budget.json` | `c5e02cd156a996afdbe91aac98fc7e924ff7873d75aefa4433454e5bf6371de0` |
| `membership_direction_probe.json` | `844d18056fa0ba49c0f9d28559d8dc399c88646e97650a2d5112c329fa79cda2` |

以上哈希对应已运行脚本与外置 JSON；后续若再修改任何脚本必须重跑并更新。本轮没有把 DCF keys、shares、sidecar、certificates、build outputs 或原始大日志放进仓库。

没有运行 `protocol_i_ucmp_conformance_test`：本 worktree 没有该测试的构建二进制，且没有 CMake 在 PATH；本任务因安全接口门失败，不伪造编译命令成功。没有运行 conformance → oracle differential → independent-process E2E。未运行 LAN/WAN 性能矩阵；性能计量均 `NOT_MEASURED`。

## 9. 门禁与下一步

| Gate | S3 | 判定说明 |
|---|---|---|
| `S2_RECEIPT` | `PASS_WITH_SCOPE` | 精确源码/run7 哈希及独立审查线程复核；未重跑全量套件，也不是第二实现 |
| `LEAKAGE_L_MAPPING` | `PARTIAL; L_PENDING_MASKED_OPERANDS` | 获批 endpoint/pivot/slot/count/length/ABORT 已逐字段映射；raw input/select/membership `z` operands 需要确认或改 API |
| `ADAPTIVE_CONTROL` | `NO_GO` | rank aggregation、sample quantile/pivot mux、secret `(C,r)` branch/update、τ share 缺失；不能公开 cmp/rank 绕过 |
| `MATERIAL_COVERAGE` | `NO_GO_FOR_RUNTIME; N128_KNOWN_LAYOUT_FITS` | proposed logical bundle 43,326,576 B/party；当前 M2PK shape 不兼容；控制材料/OT wire/索引未闭合 |
| `DCF_MEMBERSHIP` | `ALGEBRA_PASS; API_NO_GO` | `1−strict_lt(τ,key_i)` 正确且 mask 重量 K；τ share、masked z 与 DCF actual runtime conformance 未闭合 |
| `MESSAGE_DAG` | `PARTIAL / NOT_CLOSED` | 5 iterations、9 compare dependency layers、已知复用模块 2+2+2+2 等分栏；新的 control/wire/barrier 未定 |
| `FUNCTIONAL_E2E` | `NOT_RUN` | 新路线无 runtime；no-mask、replay、disconnect/error cases 未执行 |
| `JOINT_VIEW_SECURITY` | `NO_GO` | pivot/endpoint adaptive transcript、masked z、pool/unused keys、shuffle/inverse、ABORT 未有双方 simulator |
| `FORMAL_PERFORMANCE_READY` | `NO` | V3/V4 门原样保留；本轮无正式 benchmark |

接续任务应以 `PROJECT_DERIVED_SELECT_5ITER` 为唯一算法身份，先评审/实现 share-domain adaptive control 与 DCF 输入边界，再建 versioned sidecar/T/P0/P1 原型。若要使用已批准 endpoint L，可用全池逐 edge 独立 one-shot key 和直接公开寻址；不得复用 key。新 L 只待确认本报告 §4.1 列出的 `z` operands 字段，不重复询问 endpoint/pivot/slot/count/length/ABORT。原语通过后按 conformance → frozen oracle differential → T/P0/P1 E2E 顺序；密码学视图关闭前仍只能标 `EXPERIMENTAL`。S34 非异会话接收、BB90 S1 NO-GO、M6A V3 OPEN/V4 未运行、BMW16-derived 独立路线及正式性能门全部保持。

## 10. S3 新建/修改文件

S3 直接新建：

1. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S3_DECISION_2026-10-12.md`
2. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S3_REPORT_2026-10-12.md`
3. `experiments/PROJECT_DERIVED_SELECT_5ITER_S3/README.md`
4. `experiments/PROJECT_DERIVED_SELECT_5ITER_S3/public_view_probe.py`
5. `experiments/PROJECT_DERIVED_SELECT_5ITER_S3/material_budget.py`
6. `experiments/PROJECT_DERIVED_SELECT_5ITER_S3/membership_direction_probe.py`

本轮保留 S1/S2 原始报告与决策，不覆盖历史结果。未改任何 `VFSS/`、`VFSS-baseline/`、`docs/PAPERS.sha256` 或 S1/S2 测试源码。

同步修改的治理总览文件：

7. `PROJECT.md` — 追加 S3 L、N=128 预算和 NO-GO 状态；S1/S2 历史保留。
8. `docs/IMPLEMENTATION_PLAN.md` — 增加 S3 gate 行与接口/材料复审记录。
9. `docs/REFERENCE_MANIFEST.md` — 增加 dated S3 预算、L 与 ABI 记录。

S1/S2 原始报告、决策和测试文件没有修改。桌面主 checkout 的已有差异及 `docs/PAPERS.sha256` 在隔离 worktree 中继承的旧差异也未由 S3 修改。
