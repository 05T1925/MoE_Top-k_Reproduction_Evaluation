# M5 Protocol III 论文精确两轮核心设计

状态：**M5.0 协议边界冻结草案**  
实现状态：**尚未开始**  
合并状态：**设计文档允许审阅；实现代码暂不得合入 `main`**  
基准提交：`main@2a83b19`  
设计分支：`design/m5-protocol-iii-exact-2round`

## 1. 文档目的

M5 的目标是复现论文 Protocol III 的精确两轮在线核心，同时保持仓库中
已经冻结的 M3 模块化三轮实现不变。

M5 不是对 M3 文件的原地重构，而是一条独立实现路径：

```text
M3：
agarwal_protocol_iii_modular_3round
GRank → DPF routing → secure combine
3 online rounds

M5 candidate：
moe_topk_protocol_iii_exact_2round_candidate
paper-exact Round 1 → paper-exact Round 2
2 paper-core online rounds
```

在域条件、非零 payload、两轮消息因果关系、泄露边界和测试矩阵全部
通过审查之前，M5 不得使用完成态标签。

预留但暂时禁止使用的完成态标签：

```text
agarwal_protocol_iii_exact_2round
```

当前唯一允许使用的实验标签：

```text
moe_topk_protocol_iii_exact_2round_candidate
```

## 2. M5 与其他里程碑的关系

### 2.1 对 M3 的依赖

M5 依赖已经冻结的 M3 能力：

- priority-key share 语义；
-稳定 Top-K 的 tie-breaking 规则；
- Dealer、Party 0、Party 1 角色模型；
- session/fingerprint 绑定；
- framed transport；
- DPF 本地及 Peer/Dealer transport conformance；
- MetricsRecord；
- oracle differential；
-独立 `fork()` + `exec()` 测试结构。

M5 不得修改 M3 正式路径：

```text
protocol_iii_grank
protocol_iii_dpf_routing
protocol_iii_secure_combine
protocol_iii_secure_core
agarwal_protocol_iii_modular_3round
```

M3 继续作为 M5 的正确性和性能对照基线。

### 2.2 对 M2 的依赖

M5 不依赖 M2 paper-exact shuffle 是否已经完成，因此 M2 当前整改不阻塞
M5.0 至 M5.4 的设计、证明和测试准备。

如果 M2 后续必须修改以下公共接口，应先提交独立接口变更说明：

- `ProtocolIUcmpPartyMaterial`；
- `ProtocolIPartyPackage` 的既有字段语义；
- `ProtocolIFramedChannel`；
- DPF key transport；
- masked multiplication adapter。

M2 不得静默改变 M5 所依赖的 M3/CmpAgg/transport 语义。

### 2.3 M4 合并门

依据当前项目路线：

```text
M4 完成
  → M5 实现允许进入正式合并
```

因此：

- M5.0 至 M5.4 可以并行完成；
- M5 实验代码可以存在于隔离分支；
- M5 实现暂不得合入 `main`；
- 如果需要在 M4 前合并 M5 实现，必须先修改并审阅路线决策；
- 不得以“代码已经能运行”为理由绕过 M4 合并门。

## 3. 协议边界

M5 必须区分三个层次：

```text
raw-score input adapter
        ↓
Protocol III paper core
        ↓
project original-order mask adapter
```

### 3.1 论文核心入口

M5 paper core 的项目集成入口暂时冻结为：

```text
padded_n 个 priority-key additive shares
```

该入口与 M3 三轮正式实现保持一致。

如果论文算法要求另一种共享表示，必须增加显式转换层，并单独记录：

- 输入表示；
- 输出表示；
-通信轮数；
-通信字节；
-离线材料；
-泄露边界。

不得把转换通信隐藏在 paper core 的两轮计量中。

### 3.2 论文核心出口

M5 paper core 的输出必须保持秘密共享。

具体代数表示在 M5.1 冻结，候选形式包括：

- selection indicator shares；
- rank-derived selection shares；
-论文规定的非零 payload shares；
-其他可被项目 mask adapter 消费的共享表示。

禁止在 runtime 中：

-重构完整 rank；
-重构 Top-K mask；
-公开单个元素是否被选择；
-公开与原始 score 对应的排序信息。

### 3.3 项目最终输出

项目最终需要保持现有输出契约：

```text
原输入顺序上的 XOR Top-K mask shares
```

paper core 到该输出的转换属于项目 mask adapter，不得自动计入论文两轮核心。

## 4. 轮数口径

M5 必须同时报告以下指标：

```text
paper_core_rounds
input_adapter_rounds
mask_adapter_rounds
total_engineering_rounds
```

定义：

```text
total_engineering_rounds
  =
input_adapter_rounds
  +
paper_core_rounds
  +
mask_adapter_rounds
```

冻结结论：

```text
paper_core_rounds = 2
```

尚未冻结：

```text
input_adapter_rounds
mask_adapter_rounds
total_engineering_rounds
```

如果复用当前 raw-score 输入适配和原顺序 mask 输出适配，必须分别验证它们
各自的真实通信因果关系，然后才能计算完整工程轮数。

不得直接把完整 raw-score 到原顺序 mask 的 executable 标记为“两轮”。

## 5. 两轮消息因果关系

M5.2 必须给出逐字段消息图。当前只冻结两轮的因果约束，不冻结具体算法字段。

### 5.1 离线阶段

```text
Dealer
  ├── material_0 → Party 0
  └── material_1 → Party 1

Dealer successful exit
        ↓
online input release
```

Dealer 必须在在线输入释放前完成：

-随机性生成；
-非零 payload 生成；
-域元素检查；
-离线 key/material 生成；
-session/fingerprint/stage/slot 绑定；
-序列化和发送；
-成功退出。

Dealer 不得接收在线 secret input。

### 5.2 Round 1

```text
Party 0:
  input_share_0
  + offline_material_0
  → round1_message_0

Party 1:
  input_share_1
  + offline_material_1
  → round1_message_1

round1_message_0 ↔ round1_message_1
        ↓
allowed Round 1 opened/shared state
```

Round 1 消息只能依赖：

-本方输入 share；
-本方离线材料；
-公开配置；
-本方本地随机状态。

### 5.3 Round 2

```text
Round 1 received/opened state
        +
remaining one-shot offline material
        +
local secret state
        ↓
Round 2 message

round2_message_0 ↔ round2_message_1
        ↓
paper-core output shares
```

Round 2 消息必须真实依赖对方的 Round 1 消息。

验收要求：

```text
在接收并验证 Round 1 之前，Party 无法生成合法 Round 2 消息。
```

如果 Round 2 消息可以在 Round 1 前完整生成，则需要重新审查该过程究竟是一轮、
两轮，还是预处理被错误计入在线协议。

### 5.4 禁止隐藏通信

paper core 两轮内部禁止隐藏：

-输入转换通信；
-输出 mask 转换通信；
-测试 completion handshake；
-控制器 acknowledgement；
-Dealer 在线通信；
-额外 reconstruct；
-错误恢复重试通信。

测试专用控制消息必须位于计量区间之外并明确标注 `TEST_ONLY`。

## 6. 参与方与威胁模型

参与方：

```text
Dealer
Party 0
Party 1
```

当前安全模型继承仓库已冻结的半诚实模型，除非论文证据明确要求更强模型。

M5 当前不声称提供：

-恶意安全；
-主动攻击检测；
-可验证计算；
-抗串谋 Dealer；
-网络匿名性；
-侧信道安全。

如果论文的假设与仓库现有模型不同，必须在实现前单独记录差异。

## 7. 公开参数与允许公开值

候选公开参数：

- `session`；
- `fingerprint`；
- `logical_n`；
- `padded_n`；
- `K`；
- priority-key bit width；
- field identifier；
- field modulus；
- payload encoding version；
-协议版本；
- Party id；
- timeout；
- implementation label。

以下内容默认不允许公开：

- raw scores；
-单方 score shares；
-完整 priority keys；
-完整 ranks；
-单个元素是否属于 Top-K；
-最终完整 mask；
-非零 payload 的未掩码关联关系；
-可将中间值关联回原输入位置的信息。

每个计划打开的中间值必须在实现前记录：

| 阶段 | 打开值 | 接收方 | 是否论文允许 | 模拟理由 | 状态 |
|---|---|---|---|---|---|
| Round 1 | 尚待论文逐式确认 | P0/P1 | OPEN | OPEN | 阻塞实现 |
| Round 2 | 尚待论文逐式确认 | P0/P1 | OPEN | OPEN | 阻塞实现 |

在该表完成前，不得开始正式 runtime。

## 8. 域表示阻塞门

论文中只要使用非零元素逆元，M5 就必须工作在真正的有限域中。

禁止将以下环冒充为域：

```text
Z_(2^b)
```

原因：

```text
Z_(2^b) 中并非所有非零元素可逆。
```

M5.1 必须选择并冻结：

```text
F_p
```

其中 `p` 为经过审查的素数。

必须证明：

```text
∀ x ∈ F_p, x ≠ 0:
x * inverse(x) = 1 mod p
```

并明确：

```text
inverse(0) → deterministic rejection
```

域设计必须记录：

-素数 `p`；
-选择 `p` 的理由；
-C++ 存储类型；
-中间乘法宽度；
-模约简算法；
-加法和减法的 canonical representation；
-序列化字节序；
-wire format version；
-从整数/rank 到域元素的编码；
-从域元素回到共享输出的条件；
-是否需要 rejection sampling；
-常数时间要求；
-与现有 `GroupElement`/`uint64_t` 的隔离方式。

在这些内容完成之前，不允许让现有 `GroupElement` 隐式承担有限域语义。

## 9. 非零 payload 语义

M5.1.1 必须单独冻结非零 payload 规则。

生成要求：

```text
payload ∈ F_p \ {0}
```

禁止：

-生成零 payload；
-将零值静默替换成一；
-逆元失败后继续执行；
-跨 session 复用 payload；
-跨 stage 复用 payload；
-跨 slot 复用 payload；
-两方持有可直接恢复完整 payload 的不当材料。

必须定义：

```text
encode_nonzero_payload(...)
decode/validate_payload(...)
inverse_nonzero_payload(...)
```

失败语义必须是确定性的协议拒绝，例如：

```text
invalid_argument
runtime_error
explicit protocol error
```

不得通过产生全零输出或随机输出掩盖失败。

## 10. 离线材料约束

每份 Party material 至少绑定：

```text
protocol version
session
fingerprint
party
logical_n
padded_n
K
field identifier
field modulus/version
stage
slot/index
one-shot state
```

材料状态至少区分：

```text
fresh
round1_consumed
round2_consumed
fully_consumed
```

必须拒绝：

-错误 Party；
-错误 session；
-错误 fingerprint；
-错误维度；
-错误 field；
-错误 stage；
-错误 slot；
-截断数据；
-尾随数据；
-重复使用；
-乱序使用；
-零 payload；
-不可逆 payload。

序列化反序列化必须保留所有安全绑定信息。

## 11. 与 M3 的兼容边界

M5 允许复用：

- framing 和 timeout 模式；
- session/fingerprint 校验方式；
-独立进程 runner；
- Dealer 离线生命周期；
- MetricsRecord schema；
- oracle；
- DPF transport 中已经验证的安全序列化路径。

M5 不允许通过修改 M3 来获得两轮结果。

如需适配现有组件，应新增 M5 专用 adapter，而不是改变：

```text
protocol_iii_grank_party()
protocol_iii_dpf_routing_party()
protocol_iii_secure_combine_party()
protocol_iii_secure_core_party()
```

M3 和 M5 必须能够在同一提交中同时构建和测试。

## 12. 正确性目标

对于同一组合法输入，经过必要的项目适配后，M5 最终输出必须与以下结果一致：

```text
stable Top-K oracle
```

并与冻结的 M3 基线进行 differential：

```text
reconstruct_for_test(M5 mask shares)
  ==
reconstruct_for_test(M3 mask shares)
  ==
stable_topk_oracle(scores, K)
```

`reconstruct_for_test` 只能存在于测试控制器，不得进入 runtime。

tie-breaking 必须继承当前仓库冻结语义，不得由有限域编码或 payload 随机性改变。

## 13. 测试计划

计划新增：

```text
moe_topk_m5_field_conformance_test
moe_topk_m5_nonzero_payload_test
moe_topk_m5_two_round_causality_test
moe_topk_m5_transport_conformance_test
moe_topk_m5_oracle_differential_test
moe_topk_m5_three_process_e2e_test
moe_topk_m5_metrics_record_test
```

### 13.1 域测试

覆盖：

- `0`；
- `1`；
- `p - 1`；
-随机合法域元素；
-加减法边界；
-乘法中间溢出；
-canonical reduction；
-序列化往返；
-非零元素求逆；
-零元素求逆拒绝。

### 13.2 payload 测试

覆盖：

- payload 永不为零；
-固定 seed 可复现；
-不同 slot 绑定；
-不同 stage 绑定；
-错误 field 拒绝；
-零 payload 拒绝；
-重复消费拒绝；
-逆元恒等式；
-序列化后非零及绑定语义保持不变。

### 13.3 两轮因果测试

必须证明：

- Round 1 前不能消费 Round 2 material；
-未收到对方 Round 1 时不能生成合法 Round 2；
-重复发送 Round 1 被拒绝；
- Round 2 重放被拒绝；
-错误 session/fingerprint 被拒绝；
-对方提前关闭产生可诊断错误；
-父进程不保留掩盖 HUP 的 fd 副本；
-测试 acknowledgement 不计入协议轮数。

### 13.4 正确性矩阵

至少覆盖：

```text
n=1, K=1
n=3, K=2
n=3, K=3
n=5, K=3
n=7, K=1
n=8, K=8
n=127, K=2
n=128, K=2
n=128, K=8
n=129, K=2
n=256, K=2
n=256, K=8
```

数据包括：

-重复 score；
-全相等；
-非二次幂；
- `K=1`；
- `K=n`；
- signed boundary；
- Q20.12 boundary；
-随机但固定 seed 的输入。

### 13.5 独立进程 E2E

正式 E2E 必须使用：

```text
Dealer process
Party 0 process
Party 1 process
```

并通过 `fork()` + `exec()` 证明角色隔离。

必须满足：

```text
Dealer exit
  → online input release
  → Round 1
  → Round 2
  → report
```

## 14. MetricsRecord

M5 至少记录：

```text
implementation_label
git_revision
n
K
input_seed
input_distribution
field identifier
field modulus/version
offline_time_ms
online_time_ms
offline_material_total_bits
per-party sent/received bits
paper_core_rounds
input_adapter_rounds
mask_adapter_rounds
total_engineering_rounds
payload/inversion primitive counts
correctness_status
environment
```

不能可靠测量的字段必须记录：

```text
NOT_MEASURED
```

禁止使用理论估算值冒充运行时测量值。

paper core、input adapter 和 mask adapter 的通信与时间必须分项记录。

## 15. 分支规划

M5 后续分支统一使用：

```text
M5.x.x-xxxxx
```

建议顺序：

```text
M5.0.0-boundary-freeze
M5.0.1-paper-evidence
M5.1.0-field-domain
M5.1.1-nonzero-payload
M5.1.2-field-serialization
M5.2.0-two-round-message-graph
M5.2.1-causality-tests
M5.3.0-offline-material
M5.3.1-transport-conformance
M5.4.0-oracle-differential-plan
M5.4.1-three-process-test-plan
M5.5.0-round1-runtime
M5.5.1-round2-runtime
M5.5.2-three-process-e2e
M5.5.3-formal-executable
M5.5.4-metrics-and-benchmark
M5.6.0-review-closeout
```

其中 M5.5.x 实现分支在 M4 验收前不得合入 `main`。

## 16. M5.0 决策记录

| ID | 决策 | 状态 |
|---|---|---|
| M5-D0-01 | M5 是独立两轮路径，不原地修改 M3 | 已冻结 |
| M5-D0-02 | M3 三轮实现继续作为 differential baseline | 已冻结 |
| M5-D0-03 | paper core 单独报告为 2 轮 | 已冻结 |
| M5-D0-04 |输入和输出 adapter 不计入 paper core 两轮 | 已冻结 |
| M5-D0-05 |完整工程轮数在 adapter 因果关系验证后确定 | 已冻结 |
| M5-D0-06 |当前只允许 candidate 标签 | 已冻结 |
| M5-D0-07 |M4 完成前 M5 实现不得合入 main | 已冻结 |
| M5-D0-08 |具体有限域及素数选择 | OPEN，M5.1 阻塞项 |
| M5-D0-09 |论文 Round 1 打开值 | OPEN，需论文证据 |
| M5-D0-10 |论文 Round 2 输出表示 | OPEN，需论文证据 |
| M5-D0-11 |非零 payload 编码 | OPEN，M5.1.1 阻塞项 |
| M5-D0-12 |mask adapter 的真实轮数 | OPEN，需单独验证 |
| M5-D0-13 |完整工程路径总轮数 | OPEN，不得提前声明 |

## 17. M5.0 完成条件

M5.0 只有在以下条件全部满足后才能关闭：

-论文算法、定理、公式和页码证据已经登记；
- paper core 输入边界已经确认；
- paper core 输出边界已经确认；
-两轮的定义和计量口径已经冻结；
-公开参数清单已经冻结；
-允许公开的每个中间值已经登记；
-威胁模型差异已经登记；
-M3 对照路径和禁止修改范围已经冻结；
-M4 合并门已经登记；
-所有 OPEN 项已经分配到后续 M5.x.x 分支。

M5.0 完成不表示 M5 协议已经实现或论文精确性已经成立。

## 18. 当前下一步

下一分支：

```text
M5.0.1-paper-evidence
```

目标：

-逐条登记论文 Protocol III 的算法步骤；
-记录定理、公式、页码；
-确认输入、输出和公开值；
-确认论文为何需要非零 payload；
-确认逆元所在代数结构；
-将论文两轮映射成可验证的消息因果图；
-不得编写兜底 runtime；
-不得声明 exact 2-round 已完成。
```
