```markdown
# M5 Protocol III 精确两轮核心实现前检查表

状态：**进行中**  
阶段：**M5.0 前置设计与证明准备**  
基准：`main@2a83b19`  
候选标签：`moe_topk_protocol_iii_exact_2round_candidate`

本检查表用于阻止以下情况：

-在论文边界尚未确认时提前实现；
-把 `Z_(2^b)` 错误当作有限域；
-把零 payload 送入需要逆元的路径；
-把三阶段重新命名成两轮；
-隐藏输入或输出 adapter 的通信轮数；
-修改已经冻结的 M3 路径；
-在 M4 完成前把 M5 实现合入 `main`；
-在证据不足时使用完成态 exact 标签。

只有标记为“合并门”的项目全部完成，M5 实现才可以进入正式合并。

## 1. 仓库与分支基线

- [x] M3 复检整改已关闭。
- [x] M3 模块化三轮实现已在 `main`。
- [x] M3 raw-score 五轮扩展已在 `main`。
- [x] M3 MetricsRecord 已在 `main`。
- [x] `VFSS-baseline/` 保持不变。
- [x] M5 从最新 `main` 创建独立设计分支。
- [x] M5 不在 M3 runtime 文件上原地重构。
- [x] 当前只使用 candidate 标签。
- [ ] M5 设计文档已经由两名成员共同审阅。
- [ ] M5 设计 PR 已合入 `main`。

当前设计文件：

```text
docs/decisions/M5_PROTOCOL_III_EXACT_2ROUND_DESIGN.md
docs/reproduction/M5_PREIMPLEMENTATION_CHECKLIST.md
```

## 2. 路线治理检查

- [x] M5 技术上依赖冻结的 M3，不依赖 M2 paper-exact 完成。
- [x] M2 后续修改应限定在 Protocol I exact shuffle 路径。
- [x] M2 不得静默修改 M5 使用的 M3/CmpAgg/transport 公共接口。
- [x] 当前路线要求 M4 完成后再合并 M5 实现。
- [ ] M4 已完成验收。
- [ ] 或者：路线决策已经正式修改，允许 M5 在 M4 前合并。
- [ ] M5 实现合并门已经由两名成员共同确认。

在 M4 验收或路线修改前，允许：

```text
设计
论文证据整理
代数证明
域 primitive 实验
测试 skeleton
隔离分支 prototype
```

在 M4 验收或路线修改前，禁止：

```text
将 M5 runtime 合入 main
使用完成态 exact 标签
修改冻结的 M3 正式路径
宣称 M5 已完成论文精确复现
```

## 3. 论文证据登记

对应分支：

```text
M5.0.1-paper-evidence
```

必须登记论文中与 Protocol III 两轮核心直接相关的证据。

### 3.1 算法定位

- [ ] 论文标题、版本和来源已确认。
- [ ] Protocol III 所在章节已确认。
- [ ] Protocol III 算法编号已确认。
- [ ] 两轮声明所在页码已确认。
- [ ] 正确性定理所在页码已确认。
- [ ] 安全性定理所在页码已确认。
- [ ] 使用的代数结构所在公式或定义已确认。
- [ ] payload 非零要求所在公式或算法步骤已确认。
- [ ] payload 求逆所在公式或算法步骤已确认。
- [ ] 离线/在线划分所在算法步骤已确认。

证据表：

| 项目 | 论文位置 | 原始符号 | 仓库映射 | 状态 |
|---|---|---|---|---|
| Protocol III 输入 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Protocol III 输出 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Round 1 消息 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Round 1 打开值 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Round 2 消息 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Round 2 输出 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| 有限域 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| 非零 payload | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| payload inverse | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| Dealer 材料 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |
| 泄露边界 | 尚未登记 | 尚未登记 | 尚未冻结 | OPEN |

### 3.2 论文符号映射

- [ ] 每个论文输入符号都有仓库类型映射。
- [ ] 每个论文输出符号都有仓库类型映射。
- [ ] 每个随机变量都有生成方。
- [ ] 每个随机变量都有作用域。
- [ ] 每个随机变量都有是否允许复用的结论。
- [ ] 每个公开值都有公开理由。
- [ ] 每个秘密值都有 share 表示。
- [ ] 每个群、环或域都有明确类型。
- [ ] 没有把论文中的域元素直接映射为未约束的 `uint64_t`。
- [ ] 没有仅凭变量名称推断论文语义。

### 3.3 论文差异登记

- [ ] 论文威胁模型与仓库模型的差异已登记。
- [ ] 论文通信模型与 AF_UNIX 测试环境的差异已登记。
- [ ] 论文输入与项目 priority-key 输入的差异已登记。
- [ ] 论文输出与项目原顺序 mask 输出的差异已登记。
- [ ] 论文离线材料与项目 Dealer package 的差异已登记。
- [ ] 论文轮数与完整工程轮数的差异已登记。

## 4. 输入和输出边界

### 4.1 Paper-core 输入

当前项目候选入口：

```text
padded_n 个 priority-key additive shares
```

检查：

- [x] M5 与 M3 使用同一 priority-key score semantics。
- [x] padding 位置不代表真实输入。
- [x] stable tie-breaking 规则不由 M5 修改。
- [ ] 已证明该入口与论文 Protocol III 输入等价。
- [ ] 如果不等价，转换 adapter 已单独设计。
- [ ] 输入 adapter 的通信轮数已单独计量。
- [ ] 输入 adapter 的泄露边界已单独记录。
- [ ] 输入 adapter 不被隐藏在 paper core 两轮中。

### 4.2 Paper-core 输出

- [ ] 论文核心输出的具体共享表示已确认。
- [ ] 输出仍保持秘密共享。
- [ ] 输出不包含完整 rank。
- [ ] 输出不包含完整选择 mask。
- [ ] 输出不泄露单个元素是否属于 Top-K。
- [ ] 输出可以被项目 mask adapter 消费。
- [ ] 输出 adapter 已独立定义。
- [ ] 输出 adapter 的通信轮数已独立计量。
- [ ] 输出 adapter 不被隐藏在 paper core 两轮中。

### 4.3 项目最终输出

冻结目标：

```text
原输入顺序上的 XOR Top-K mask shares
```

- [x] 最终输出顺序与原输入顺序一致。
- [x] 每方只持有 XOR share。
- [ ] M5 paper-core 输出到 XOR mask share 的转换已证明。
- [ ] 转换过程没有重构完整 mask。
- [ ] 转换使用的一次性材料已定义。
- [ ] 转换的轮数、通信和时间已单独记录。

## 5. 轮数检查

冻结公式：

```text
total_engineering_rounds
  =
input_adapter_rounds
  +
paper_core_rounds
  +
mask_adapter_rounds
```

- [x] `paper_core_rounds = 2` 是 M5 的目标。
- [x] 测试 acknowledgement 不计入协议轮数。
- [x] Dealer preprocessing 不计入在线轮数。
- [x] 本地计算不单独计为通信轮。
- [ ] input adapter 的真实轮数已验证。
- [ ] mask adapter 的真实轮数已验证。
- [ ]完整工程路径总轮数已计算。
- [ ]完整工程路径没有被错误标记为两轮。
- [ ] MetricsRecord 同时记录 core、adapter 和 total rounds。

禁止在上述项目完成前写入：

```text
完整工程路径 = 2 轮
```

## 6. 有限域设计

对应分支：

```text
M5.1.0-field-domain
M5.1.2-field-serialization
```

### 6.1 域选择

- [ ] 已确认论文要求有限域而不是环。
- [ ] 素数 `p` 已选择。
- [ ] `p` 的选择理由已记录。
- [ ] `p` 足以编码全部合法协议值。
- [ ] C++ 存储类型已冻结。
- [ ] 中间乘法类型已冻结。
- [ ] 模约简算法已冻结。
- [ ] canonical representation 已冻结。
- [ ] field identifier 已冻结。
- [ ] wire-format version 已冻结。
- [ ] 域类型与现有 `GroupElement` 明确隔离。
- [ ] 域类型与 `Z_(2^b)` ring share 明确隔离。

### 6.2 运算正确性

必须通过：

- [ ] `0 + x = x`。
- [ ] `x + (-x) = 0`。
- [ ] `1 * x = x`。
- [ ] `(x * y) mod p` 正确。
- [ ] `p - 1 + 1 = 0`。
- [ ] canonical reduction 正确。
- [ ] 非零元素均有逆元。
- [ ] `x * inverse(x) = 1 mod p`。
- [ ] `inverse(0)` 确定性失败。
- [ ] 中间乘法不发生未定义溢出。
- [ ] 随机 differential 与可信大整数 oracle 一致。

### 6.3 序列化

- [ ] 字节序已冻结。
- [ ] 编码长度已冻结。
- [ ] 非 canonical encoding 被拒绝。
- [ ] 大于或等于 `p` 的编码被拒绝。
- [ ] 截断编码被拒绝。
- [ ] 尾随字节被拒绝。
- [ ] field identifier 不一致被拒绝。
- [ ] serialize/deserialize 往返一致。
- [ ] transport 后域元素值和身份保持不变。

## 7. 非零 payload

对应分支：

```text
M5.1.1-nonzero-payload
```

冻结目标：

```text
payload ∈ F_p \ {0}
```

### 7.1 生成

- [ ] payload 由正确角色生成。
- [ ] payload 使用明确的随机源。
- [ ] 固定测试 seed 可复现。
- [ ] production 路径不使用固定 seed。
- [ ] 生成过程保证非零。
- [ ] rejection sampling 无统计偏差。
- [ ] 不通过“零改成一”掩盖生成错误。
- [ ] payload 与 session 绑定。
- [ ] payload 与 stage 绑定。
- [ ] payload 与 slot/index 绑定。
- [ ] payload 与 Party 绑定。

### 7.2 逆元

- [ ] 只允许对已经验证的非零 payload 求逆。
- [ ] 零 payload 在求逆前被拒绝。
- [ ] 逆元算法已经通过 field oracle differential。
- [ ] 逆元失败不会返回零或随机值。
- [ ] 逆元失败不会继续协议。
- [ ] 失败类型和错误信息已冻结。

### 7.3 一次性语义

- [ ] payload/material 初始状态为 `fresh`。
- [ ] Round 1 消费状态已定义。
- [ ] Round 2 消费状态已定义。
- [ ] fully consumed 状态已定义。
- [ ]重复使用被拒绝。
- [ ]跨 stage 使用被拒绝。
- [ ]跨 slot 使用被拒绝。
- [ ]跨 session 使用被拒绝。
- [ ]反序列化材料从正确状态开始。
- [ ]网络重放测试已规划。

## 8. 两轮消息图

对应分支：

```text
M5.2.0-two-round-message-graph
M5.2.1-causality-tests
```

### 8.1 Round 1

- [ ] Party 0 的 Round 1 输入已逐字段定义。
- [ ] Party 1 的 Round 1 输入已逐字段定义。
- [ ] Round 1 消息格式已定义。
- [ ] Round 1 消息长度上限已定义。
- [ ] Round 1 消息绑定 session/fingerprint。
- [ ] Round 1 允许打开的值已定义。
- [ ] Round 1 打开值的泄露理由已记录。
- [ ] Round 1 打开值与论文步骤一致。

### 8.2 Round 2

- [ ] Party 0 的 Round 2 输入已逐字段定义。
- [ ] Party 1 的 Round 2 输入已逐字段定义。
- [ ] Round 2 消息格式已定义。
- [ ] Round 2 消息长度上限已定义。
- [ ] Round 2 消息绑定 session/fingerprint。
- [ ] Round 2 输出 share 类型已定义。
- [ ] Round 2 输出与论文步骤一致。
- [ ] Round 2 后不需要额外 paper-core 通信。

### 8.3 因果证明

- [ ] Round 2 消息依赖对方 Round 1 消息。
- [ ] Round 2 消息不能在 Round 1 接收前生成。
- [ ] Round 1 material 不能用于 Round 2。
- [ ] Round 2 material 不能用于 Round 1。
- [ ]提前 Round 2 被拒绝。
- [ ]重复 Round 1 被拒绝。
- [ ]重复 Round 2 被拒绝。
- [ ]乱序消息被拒绝。
- [ ]协议状态机不允许跳过 Round 1。
- [ ]不存在隐藏的第三轮打开。
- [ ]不存在 core 结束后的额外共享转换通信。

## 9. 泄露边界

- [ ] 所有公开参数已经列出。
- [ ] 所有 Round 1 打开值已经列出。
- [ ] 所有 Round 2 打开值已经列出。
- [ ] 每个打开值都有论文依据。
- [ ] 每个打开值都有模拟或安全解释。
- [ ] 没有打开完整 score。
- [ ] 没有打开完整 priority key。
- [ ] 没有打开完整 rank。
- [ ] 没有打开完整 Top-K mask。
- [ ] 没有公开单个位置的选择结果。
- [ ] padding 不产生额外可区分泄露。
- [ ]非零 payload 不产生可关联标识。
- [ ]错误路径不会输出秘密中间值。
- [ ]日志不会记录 secret share 或完整材料。

泄露登记表：

| 值 | 阶段 | 谁可见 | 必要性 | 论文依据 | 状态 |
|---|---|---|---|---|---|
|公开配置 | Offline | Dealer/P0/P1 |协议配置 |待登记 | OPEN |
| Round 1 打开值 | Round 1 | P0/P1 |待证明 |待登记 | OPEN |
| Round 2 打开值 | Round 2 | P0/P1 |待证明 |待登记 | OPEN |
| paper-core 输出 share | End |各自 Party |项目适配 |待登记 | OPEN |

## 10. Dealer 离线材料

对应分支：

```text
M5.3.0-offline-material
```

- [ ] Dealer material 类型已定义。
- [ ] P0/P1 材料完全分离。
- [ ] Dealer 不接收在线输入。
- [ ] Dealer 在 online input release 前退出。
- [ ] 材料包含 protocol version。
- [ ] 材料包含 session。
- [ ] 材料包含 fingerprint。
- [ ] 材料包含 Party id。
- [ ] 材料包含 `logical_n`。
- [ ] 材料包含 `padded_n`。
- [ ] 材料包含 `K`。
- [ ] 材料包含 field identifier。
- [ ] 材料包含 stage。
- [ ] 材料包含 slot/index。
- [ ] 材料具有 one-shot 状态。
- [ ] 材料长度有上限。
- [ ]错误绑定被拒绝。
- [ ]截断材料被拒绝。
- [ ]尾随材料被拒绝。
- [ ]重复消费被拒绝。

## 11. Transport 检查

对应分支：

```text
M5.3.1-transport-conformance
```

- [ ] 复用经过验证的 framed transport。
- [ ] 不创建无边界裸 socket 消息格式。
- [ ] 每个 frame 有 protocol/stage/session 绑定。
- [ ]消息长度在分配内存前验证。
- [ ] `POLLERR` 可诊断。
- [ ] `POLLNVAL` 可诊断。
- [ ] `POLLHUP` 不会掩盖最后有效 frame。
- [ ]提前关闭可诊断。
- [ ] timeout 可配置。
- [ ]父进程不会保留掩盖 HUP 的 fd 副本。
- [ ] completion acknowledgement 位于协议计量之外。
- [ ] serialization round-trip 通过。
- [ ] Party 0/Party 1 交叉发送后结果不变。

## 12. API 设计检查

计划的 M5 专用接口不得替换 M3 API。

候选文件：

```text
VFSS/include/moe_topk/protocol_iii_exact_2round_field.h
VFSS/include/moe_topk/protocol_iii_exact_2round_material.h
VFSS/include/moe_topk/protocol_iii_exact_2round.h

VFSS/src/moe_topk/protocol_iii_exact_2round_field.cpp
VFSS/src/moe_topk/protocol_iii_exact_2round_material.cpp
VFSS/src/moe_topk/protocol_iii_exact_2round.cpp
```

检查：

- [ ] Config 类型只包含公开配置。
- [ ] PartyMaterial 类型只包含本方材料。
- [ ] Output 类型只包含本方输出 shares。
- [ ] Metrics 类型区分 offline/core/adapter。
- [ ] Round 1 和 Round 2 状态显式表示。
- [ ] API 不接收两方 shares。
- [ ] API 不返回完整重构值。
- [ ] runtime 不包含 `TEST_ONLY reconstruct`。
- [ ] material 通过 move/状态机保证 one-shot。
- [ ] M5 类型名称不与 M3 类型混淆。
- [ ] candidate label 不冒充完成态 exact 标签。
- [ ] M3 与 M5 可以同时链接和运行。

## 13. 正确性与 differential

对应分支：

```text
M5.4.0-oracle-differential-plan
```

必须验证：

```text
TEST_ONLY reconstruct(M5 output)
  ==
TEST_ONLY reconstruct(M3 output)
  ==
stable_topk_oracle(scores, K)
```

检查：

- [ ] M5 与 stable Top-K oracle 一致。
- [ ] M5 与 M3 三轮基线一致。
- [ ] tie-breaking 一致。
- [ ] padding 不影响 logical 输出。
- [ ] `K=1` 正确。
- [ ] `K=n` 正确。
- [ ]全相等输入正确。
- [ ]重复 score 正确。
- [ ] signed boundary 正确。
- [ ] Q20.12 boundary 正确。
- [ ]非二次幂 `n` 正确。
- [ ]固定 seed 的随机测试可复现。
- [ ]测试 reconstruct 只存在于控制器。
- [ ] production/runtime 文件不包含 reconstruct。

## 14. 测试矩阵

对应分支：

```text
M5.4.1-three-process-test-plan
```

计划 target：

```text
moe_topk_m5_field_conformance_test
moe_topk_m5_nonzero_payload_test
moe_topk_m5_two_round_causality_test
moe_topk_m5_transport_conformance_test
moe_topk_m5_oracle_differential_test
moe_topk_m5_three_process_e2e_test
moe_topk_m5_metrics_record_test
```

### 14.1 基础矩阵

- [ ] `n=1, K=1`
- [ ] `n=3, K=2`
- [ ] `n=3, K=3`
- [ ] `n=5, K=3`
- [ ] `n=7, K=1`
- [ ] `n=8, K=8`
- [ ] `n=127, K=2`
- [ ] `n=128, K=2`
- [ ] `n=128, K=8`
- [ ] `n=129, K=2`
- [ ] `n=256, K=2`
- [ ] `n=256, K=8`

### 14.2 负向测试

- [ ] invalid Party id
- [ ] wrong session
- [ ] wrong fingerprint
- [ ] wrong field identifier
- [ ] wrong protocol version
- [ ] wrong logical/padded dimensions
- [ ] zero payload
- [ ] non-canonical field value
- [ ] inverse of zero
- [ ] truncated material
- [ ] trailing material
- [ ] material reuse
- [ ] stage mismatch
- [ ] slot mismatch
- [ ] Round 2 before Round 1
- [ ] duplicate Round 1
- [ ] duplicate Round 2
- [ ] peer early exit
- [ ] frame timeout
- [ ] oversized message

## 15. 三进程 E2E

- [ ] Dealer 使用独立 `exec`。
- [ ] Party 0 使用独立 `exec`。
- [ ] Party 1 使用独立 `exec`。
- [ ]每个测试 case 恰好一个 Dealer exec。
- [ ]每个测试 case 恰好两个 Party exec。
- [ ] Dealer 完成并退出后才发送在线输入。
- [ ]控制器不向 Party 传递明文 score。
- [ ] Dealer 不持有在线输入。
- [ ]任一 Party 不持有两方输入 shares。
- [ ]父进程及时关闭协议 fd 副本。
- [ ] E2E 严格执行 Round 1 → Round 2。
- [ ] paper-core 完成后才执行项目 mask adapter。
- [ ] oracle 验证位于测试控制器。
- [ ]测试控制消息不计入协议通信。
- [ ] `strace -f -e trace=process` 提供角色隔离证据。

## 16. MetricsRecord

- [ ] exact candidate implementation label 已记录。
- [ ] Git revision 已记录。
- [ ] `n` 和 `K` 已记录。
- [ ] input seed 已记录。
- [ ] input distribution 已记录。
- [ ] field identifier/version 已记录。
- [ ] offline time 已记录。
- [ ] Round 1 time 已记录。
- [ ] Round 2 time 已记录。
- [ ] paper-core online time 已记录。
- [ ] input adapter time 已单独记录。
- [ ] mask adapter time 已单独记录。
- [ ] offline material bits 已记录。
- [ ] Party 0 sent/received bits 已记录。
- [ ] Party 1 sent/received bits 已记录。
- [ ] paper-core rounds = 2。
- [ ] input adapter rounds 已单独记录。
- [ ] mask adapter rounds 已单独记录。
- [ ] total engineering rounds 已记录。
- [ ] payload generation count 已记录。
- [ ] inversion count 已记录。
- [ ] correctness status 已记录。
- [ ]环境信息已记录。
- [ ]无法可靠测量的指标使用 `NOT_MEASURED`。
- [ ]没有以估算值冒充测量值。

## 17. 正式 executable 门

计划候选 executable：

```text
moe_topk_protocol_iii_exact_2round_candidate
```

在完成态标签启用前：

- [ ]域实现通过审查。
- [ ]非零 payload 通过审查。
- [ ]两轮因果测试通过。
- [ ] transport conformance 通过。
- [ ] oracle differential 通过。
- [ ] M3 differential 通过。
- [ ]独立三进程 E2E 通过。
- [ ] MetricsRecord 通过。
- [ ]论文证据表完成。
- [ ]泄露表完成。
- [ ]两名成员完成代码复审。
- [ ] M4 合并门解除。
- [ ]完成态标签经过单独决策批准。

在这些条件完成前，禁止创建：

```text
agarwal_protocol_iii_exact_2round
```

## 18. 回归要求

每个 M5 实现分支必须保证：

- [ ] M1 tests 通过。
- [ ] M2 tests 通过。
- [ ] M3 tests 通过。
- [ ] DPF 44-case conformance 通过。
- [ ]两个 M3 正式 executable 通过。
- [ ] M3 MetricsRecord test 通过。
- [ ] `git diff --check` 无输出。
- [ ]没有修改 `VFSS-baseline/`。
- [ ]没有提交论文 PDF。
- [ ]没有提交密钥。
- [ ]没有提交日志。
- [ ]没有提交构建产物。
- [ ]工作区在提交后保持干净。

## 19. 分支完成门

### `M5.0.0-boundary-freeze`

- [ ]设计文档完成。
- [ ]本检查表完成初版。
- [ ] M5/M3/M4 边界完成审阅。
- [ ]只修改 Markdown。

### `M5.0.1-paper-evidence`

- [ ]论文证据表完成。
- [ ]输入输出符号映射完成。
- [ ]两轮声明有精确出处。
- [ ]域、payload 和 inverse 有精确出处。

### `M5.1.0-field-domain`

- [ ]域 primitive 完成。
- [ ] field conformance 全部通过。
- [ ]没有修改现有 ring 语义。

### `M5.1.1-nonzero-payload`

- [ ]非零采样完成。
- [ ]逆元完成。
- [ ]零值和复用失败语义完成。

### `M5.1.2-field-serialization`

- [ ] wire format 完成。
- [ ]本地和 transport 往返测试通过。

### `M5.2.0-two-round-message-graph`

- [ ] Round 1/2 消息字段冻结。
- [ ]状态机冻结。
- [ ]泄露表冻结。

### `M5.2.1-causality-tests`

- [ ]提前、重放、乱序测试全部通过。
- [ ]没有隐藏第三轮。

### `M5.3.0-offline-material`

- [ ] Dealer material 完成。
- [ ]全部绑定和 one-shot 检查通过。

### `M5.3.1-transport-conformance`

- [ ] framing、timeout、HUP 和截断测试通过。

### `M5.4.0-oracle-differential-plan`

- [ ] oracle/M3 differential 方案冻结。

### `M5.4.1-three-process-test-plan`

- [ ]独立进程 E2E 方案冻结。
- [ ]完整测试矩阵冻结。

### `M5.5.x` 实现阶段

- [ ] M4 合并门已解除。
- [ ] Round 1 runtime 完成。
- [ ] Round 2 runtime 完成。
- [ ] E2E 完成。
- [ ] MetricsRecord 完成。
- [ ]正式标签完成审查。

## 20. 当前阻塞项

以下项目阻塞 M5 正式实现：

1. 论文 Protocol III 两轮算法证据尚未逐项登记。
2. 有限域及素数尚未冻结。
3. 非零 payload 编码尚未冻结。
4. Round 1 打开值尚未冻结。
5. Round 2 输出表示尚未冻结。
6. 两轮因果图尚未完成证明。
7. 泄露登记表尚未完成。
8. mask adapter 的真实轮数尚未验证。
9. M4 尚未完成，正式实现合并门未解除。

## 21. 下一步

完成本检查表初版后进入：

```text
M5.0.1-paper-evidence
```

该阶段只允许：

-阅读并登记论文证据；
-建立论文符号到仓库类型的映射；
-冻结输入、输出、公开值和两轮因果关系；
-确认有限域、非零 payload 和 inverse 的论文依据；
-更新设计文档和检查表。

该阶段禁止：

-编写兜底协议实现；
-把现有三轮代码改名为两轮；
-将 `Z_(2^b)` 当作有限域；
-使用完成态 exact 标签；
-将 M5 runtime 合入 `main`。
```
