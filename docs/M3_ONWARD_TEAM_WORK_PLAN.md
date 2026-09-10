新的分工应把 M3 当前进度写实，并让搭档负责 M6 的“M2 复用与自适应预处理门”，但不能把 M2 完整图 CmpAgg 当成已经解决 AAV86 自适应预处理。

建议提交到：

```text
docs/M3_ONWARD_TEAM_WORK_PLAN.md
```

不要混入当前有未提交修改的 `m3-grank-runtime`。完成并提交 GRank 后，再从最新 `main` 创建独立文档分支：

```text
docs/m3-onward-team-work-plan
```

以下是完整文档内容：

```markdown
# M3 及后续双人分工计划

状态：执行中  
更新日期：2026-09-07  
主线验收顺序：M3 → M4 → M5 → M6 → M7

## 1. 当前基线

M1/M1.1 已完成并冻结统一 score、tie rule、原顺序 Top-K bit-mask 和 metrics
语义。

M2 已完成并合入 `main`，当前身份是：

```text
M2 Protocol I C-level modular engineering baseline
```

M2 已提供以下可复用组件：

- priority-key 表示与 logical/padded layout；
- DCF-backed uCMP；
- 全对全 CmpAgg；
- session/fingerprint/package binding；
- 有界 framed transport；
- Dealer → Party 0/Party 1 package 传输；
- EMP chosen OT；
- OPV 与 Share Translation；
- Permute+Share；
- 两遍 secret-shared shuffle；
- raw Q20.12 score share 输入适配；
- 原顺序 XOR Top-K mask 输出；
- 通信量、原语调用数和在线轮数计量；
- 双方和三方独立进程测试基础。

这些组件是后续协议的公共工程底座，但不自动证明 Protocol III、AAV86 或其他论文
协议已经实现。

尤其是：

- M2 的 CmpAgg 为固定完整比较图准备全部 edge material；
- M2 没有解决由在线公开状态决定下一轮比较边的自适应预处理；
- M2.16 只完成 Protocol I paper-exact 可行性与泄露审计，没有新增精确协议接口；
- M6 仍必须单独解决 Dealer 在线静默条件下的 exact-edge 自适应预处理。

## 2. 总体分工

| 成员 | 当前主任务 | 后续主任务 |
| --- | --- | --- |
| 角色 A（搭档） | M4 CipherGPT 原生基线 | M6 的 M2 复用审计、自适应预处理设计和安全门 |
| 角色 B（你） | M3 Protocol III 模块化 3 轮 | M5 Protocol III 精确 2 轮、M6 实验 runtime 与统一输出 |
| 双方共同 | 交叉评审、公共语义复核 | M6 安全模型评审、M7 统一实验和论文报告 |

两人可以并行研究和开发，但正式基线验收顺序保持：

```text
M3 → M4 → M5 → M6 → M7
```

允许提前开展后续阶段的来源审计、失败用例和设计研究，但未通过前置门时：

- 不合并依赖尚未冻结接口的 runtime；
- 不把实验原型标为论文精确复现；
- 不用后续协议修改 M1/M2 已冻结语义；
- 不在一个 PR 中混合两个里程碑。

## 3. 角色 B：M3 Protocol III 模块化 3 轮

### 3.1 固定目标

M3 实现：

```text
R1：GRank
R2：masked-rank DPF routing
R3：share-preserving masked multiplication/combine
```

最终输出：

```text
原始输入顺序下长度为 logical_n 的 XOR Top-K mask shares
```

M3 是模块化 3 轮工程基线，不是论文 Theorem 4.2 的精确 2 轮实现。

### 3.2 当前进度

| 子阶段 | 内容 | 当前状态 |
| --- | --- | --- |
| M3.0 | Ubuntu 环境与构建基线 | 已完成 |
| M3.1a | `keyGenDPF`、`evalDPF_Payload` conformance | 已完成于独立分支，待按 PR 状态集成 |
| M3.1b | DPF key 经 Peer 传输后求值不变 | 已完成于独立分支，待按 PR 状态集成 |
| M3.1c | share-preserving multiplication adapter | 已实现并验证于独立分支，待按 PR 状态集成 |
| M3.2 | Protocol III GRank runtime |已完成并通过复检|
| M3.3 | masked-rank DPF routing | 已完成并通过复检|
| M3.4 | secure combine 与原顺序 mask |已完成并通过复检 |
| M3.5 | 三方独立进程完整 E2E | 已完成并通过复检|

分支上的实现和测试只有合并到 `main` 后，才能记为主线已完成。

### 3.3 M3.2：一轮 GRank

当前分支：

```text
m3-grank-runtime
```

输入：

```text
padded priority-key additive shares
ProtocolIPartyPackage
GRank framed fd
```

执行：

1. 校验 `session`、`fingerprint`、party、`n`、`K` 和位宽；
2. 校验 node mask 与完整 edge material 的数量和绑定；
3. 对本地 priority-key share 加 node mask share；
4. 通过 M2 `ProtocolIFramedChannel` 交换 masked keys；
5. 只打开协议允许公开的 masked keys；
6. 调用 `protocol_i_cmpagg_eval_party()`；
7. 将逻辑位置的 rank share 约化到 `Z_(2^rank_bits)`；
8. 返回 additive priority-rank shares；
9. 记录通信量、比较边、uCMP/DCF 调用数和一轮在线通信。

禁止：

- 重构 priority key；
- 重构 rank；
- 返回 padded slot；
- 调用 Protocol I shuffle；
- 调用 rank reveal；
- 把测试 oracle 重构带入 runtime；
- 修改 M2 CmpAgg、transport 或 party package 的冻结行为。

M3.2 退出条件：

- 双方 socket E2E 通过；
- 与 M1 stable-rank oracle 差分一致；
- 重复值、全相等、负值和边界值通过；
- `logical_n=1`、非二次幂输入和 padding 通过；
- package binding 错误显式拒绝；
- one-shot node mask 和 edge material 被消费；
- `online_rounds == 1`；
- 全量相关 CTest 通过；
- 有 Ubuntu 复现记录。

### 3.4 M3.3：DPF routing 第一轮

M3.2 合并后开始。

输入：

```text
rank additive shares
rank-mask shares r_i
DPF party keys
公开目标 rank k
```

执行：

1. 在 `Z_(2^rank_bits)` 中计算 `[rank_i] + [r_i]`；
2. 双方交换并打开 `hat_rank_i`；
3. 对每个逻辑位置 `i` 和目标 rank `k ∈ [0,K)` 计算：

   ```text
   evalDPF_Payload(key_i, hat_rank_i - k)
   ```

4. 输出 additive indicator shares；
5. 记录 DPF key 数量、payload 位宽、离线材料字节数和 R2 通信量。

必须复用已验证的：

- `keyGenDPF`；
- `evalDPF_Payload`；
- Peer/Dealer key transport。

禁止：

- 使用 `evalDPF_EQ` 的 XOR share 直接参与算术乘法；
- 重构 rank、DPF point 或 indicator；
- 使用本地旧 `.bin` key 文件；
- 直接序列化 `DPFKeyPack` 内存布局；
- 把 DPF routing 与 GRank 压成两轮。

### 3.5 M3.4：secure combine

输入：

```text
indicator additive shares
unit payload additive shares
fresh multiplication material
```

执行：

1. 对每个 `(i,k)` 使用独立乘法材料；
2. 通过 masked multiplication adapter 得到 product additive shares；
3. 不公开 product；
4. 沿目标 rank `k` 聚合每个原位置的 mask arithmetic share；
5. 使用 `Z_(2^64)` 加法 share 的最低位生成 XOR mask share；
6. 只返回前 `logical_n` 个原顺序结果。

退出条件：

- 每个重构 mask bit 只能是 0 或 1；
- mask 中恰好有 `K` 个 1；
- 与 M1 oracle 完全一致；
- secure runtime 中不存在 `reconstruct(product)`；
- R3 使用 fresh、one-shot multiplication material；
- adapter 的通信和材料开销进入主结果。

### 3.6 M3.5：完整三方 E2E

建立 Dealer、Party 0、Party 1 独立进程 runner：

```text
Dealer 离线生成并发送材料后退出
Party 0/Party 1 接收输入 shares
R1 GRank
R2 DPF routing
R3 secure combine
输出 XOR mask shares
```

测试至少覆盖：

- 随机输入；
- 重复 score；
- 全相等；
- 正负边界；
- `K=1`；
- `K=n`；
- 非二次幂 `n`；
- 错误 session/fingerprint；
- 截断 package；
- 重复使用 one-shot material；
- socket 分片、超时和 peer 提前退出。

M3 最终交付物：

- Protocol III GRank runtime；
- DPF routing runtime；
- share-preserving combine；
- 三方独立进程 E2E；
- oracle differential；
- 三轮因果关系审计；
- 泄露与输出边界记录；
- Ubuntu 可复现记录。
## M3 当前冻结状态（2026-09-10）

M3 模块化三轮工程基线及其 raw-score 五轮扩展已经完成复检整改。

已完成：

- 安全 raw-score 输入适配；
- Dealer/Party fork+exec 角色隔离；
- logical-n GRank 图；
- DPF conformance CTest 注册；
- raw-score 独立进程 E2E；
- 三轮正式 executable；
- 五轮 raw-score 正式 executable；
- 结构化 MetricsRecord；
- Ubuntu 全新构建验证。

冻结实现：

```text
agarwal_protocol_iii_modular_3round
moe_topk_protocol_iii_raw_score_modular_5round
## 4. 角色 A：M4 CipherGPT 原生基线

角色 A 可与 M3 并行开发 M4，但保持独立分支。

负责内容：

1. 固定 CipherGPT 原始仓库、revision 和许可证；
2. 建立 source-only 可审查边界；
3. 修复非法输入和错误传播；
4. 保证 QuickSelect 每次递归严格收缩；
5. 修复重复值和全相等输入的不终止问题；
6. 保证输出恰好选择 `K` 个位置；
7. 将 original index 与 score 一起通过 shuffle；
8. 按统一 tie rule 恢复原始输入顺序；
9. 增加秘密共享 Top-K bit-mask 输出；
10. 区分 test-only reconstruction 与 secure runtime；
11. 保留 CipherGPT 原生密码学栈和实现身份；
12. 与 M1 oracle 做差分。

禁止：

- 修改 Protocol III runtime；
- 修改 VFSS DPF 或 multiplication adapter；
- 修改 M1 冻结语义；
- 把 CipherGPT-style VFSS adapter 当作原生 CipherGPT；
- 未确认许可证前复制完整参考仓库；
- 修改 `VFSS-baseline/`。

M4 交付物：

- `ciphergpt_native_mask_output`；
- 来源、revision 和许可证记录；
- 终止性与错误传播测试；
- original-index 绑定；
- 统一 mask adapter；
- oracle differential；
- Ubuntu 可复现记录。

## 5. M5：角色 B 主责，角色 A 评审

M5 只有在 M3 三轮基线完成后开始实现。

角色 B 负责：

1. 选择满足论文要求的域，不能将 `Z_(2^b)` 直接当作 field；
2. 定义非零 payload、零值和逆元失败语义；
3. 设计 GRank 与 DPF routing 的跨阶段压缩；
4. 实现 Protocol III 精确 2 轮候选；
5. 分别计量 paper core 和统一 mask adapter；
6. 与 M3 使用相同输入、seed 和 oracle 差分；
7. 建立独立进程 E2E；
8. 审计因果轮数、泄露和 Dealer 行为。

角色 A 负责评审：

- 域与编码边界；
- 非零 payload 条件；
- 两轮消息依赖；
- 是否遗漏 mask adapter 成本；
- 是否错误复用 M2 的 ring-only 接口。

只有论文条件、两轮审计和 E2E 全部通过，才能使用：

```text
agarwal_protocol_iii_exact_2round
```

## 6. M6：AAV86 / Direct Top-K 调整后分工

### 6.1 为什么调整

角色 A 在 M2 已完成或参与了以下底座：

- DCF/uCMP；
- 完整图 CmpAgg；
- party material；
- framed transport；
- session/fingerprint binding；
- 独立进程测试；
- metrics；
- chosen OT、share translation 和 shuffle 扩展。

因此 M6 中与 M2 复用边界、edge material 和 Dealer 行为最相关的部分改由角色 A
主责。

但必须明确：

```text
M2 完整图预处理
≠
AAV86 exact-edge 自适应预处理
```

AAV86 后续比较图依赖前一轮公开的 bucket/rank 状态。若 Dealer 根据在线状态补发
edge keys，安全模型已经改变；若离线为所有可能边准备完整图材料，则可能失去
AAV86 降低比较量的主要意义。

### 6.2 角色 A：M6 预处理与 M2 复用主责

角色 A 负责：

1. 建立 M2 → M6 复用矩阵：

   | M2 组件 | M6 处理 |
   | --- | --- |
   | priority-key 语义 | 直接复用 |
   | uCMP/DCF | 候选复用 |
   | CmpAgg 聚合方式 | 可复用局部接口，不复用固定完整图假设 |
   | party package | 扩展前先审计材料绑定 |
   | framed transport | 直接复用 |
   | session/fingerprint | 直接复用 |
   | metrics | 扩展 edge、vertex、PRG 和 per-round counters |
   | chosen OT/share translation/shuffle | 仅在实际协议需要时复用 |
   | raw-score adapter | 作为统一输入边界复用 |

2. 写明 AAV86 每轮：
   - 公开值；
   - bucket 状态；
   - 比较图；
   - edge material 生成时机；
   - Dealer 是否在线；
   - one-shot material 消费方式。

3. 解决或明确阻塞：
   - offline-only exact-edge 预处理；
   - Dealer 在线静默；
   - 输入无关材料生成；
   - 自适应 edge 与预生成 DCF key 的绑定；
   - material 数量是否退化为完整图。

4. 建立安全模型标签：
   - offline-only candidate；
   - online-Dealer experimental；
   - full-graph preallocation control；
   - plaintext graph TEST_ONLY oracle。

5. 在设计门通过前只提交：
   - 决策文档；
   - 失败用例；
   - ideal graph oracle；
   - M2 接口复用 conformance；
   - 不含安全声明的实验脚手架。

### 6.3 角色 B：M6 runtime 与统一输出主责

在 M5 完成且 M6 预处理门明确后，角色 B 负责：

1. 实现获批准安全模型下的 AAV86 runtime；
2. 接入 M2 的 uCMP/transport/package/metrics；
3. 实现 AAV86 full-sort 实验目标；
4. 必要时单独实现 Direct Top-K 实验目标；
5. 输出原顺序 XOR Top-K mask shares；
6. 与 M1 oracle 差分；
7. 对 `r=2,3,4,5` 执行统一矩阵；
8. 记录：
   - `e_A(n,r)`；
   - `v_A(n,r)`；
   - 实际 DCF/uCMP 调用；
   - PRG 调用；
   - 离线材料字节数；
   - 每轮通信；
   - 在线因果轮数；
   - LAN/WAN 时间。

Direct Top-K 必须使用独立实现标签，不得与 AAV86 full-sort 混名。

### 6.4 M6 共同退出条件

双方共同确认：

- 安全模型明确；
- Dealer 行为可审计；
- 自适应预处理没有被完整图材料悄悄替代；
- 公开 bucket/graph 泄露已记录；
- 每条 DCF key 与 edge、round、session 绑定；
- one-shot 材料不能跨轮复用；
- 输出与统一 oracle 一致；
- AAV86 和 Direct Top-K 分开计量；
- 实验失败配置标记为 `NOT_MEASURED`。

只有 offline-only、自适应预处理、安全审计和轮数验证全部通过，才能升级论文一致性
标签。否则目标保持：

```text
aav86_ca_experimental
direct_topk_experimental
```

## 7. M7：双方共同负责

角色 A：

- 运行 CipherGPT native 实验；
- 运行或协助运行 M6 的 M2-component 对照；
- 整理 CipherGPT 和 AAV86 预处理结果；
- 检查原生 runtime、安全模型和 topology。

角色 B：

- 运行 Protocol III 三轮与两轮实验；
- 运行 M6 AAV86/Direct Top-K runtime；
- 整理 VFSS 原始结果；
- 检查轮数、通信和离线材料数据。

双方共同：

- 使用相同输入、K、seed 和机器配置；
- 每个配置预热 1 次、正式运行 5 次；
- 汇总 median、min、max；
- 保留 total、per-party、sent 和 received；
- 分开报告 offline 与 online；
- 区分 core protocol 和 mask adapter；
- 将失败配置标记为 `NOT_MEASURED`；
- 完成论文复现报告和结果审计。

## 8. 共享文件规则

以下文件由双方共同维护：

```text
VFSS/include/moe_topk/score_semantics.h
VFSS/include/moe_topk/topk_oracle.h
VFSS/include/moe_topk/metrics.h
docs/decisions/M1_SCORE_SEMANTICS.md
README.md
PROJECT.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
```

规则：

- 两人不得同时修改同一共享文件；
- 冻结语义变更必须建立独立治理 PR；
- 公共 oracle、输入生成和 seed 规则只保留一份；
- 不为不同协议复制另一套 tie rule；
- 协议状态只在代码和测试验收后更新；
- 不把分支完成写成 mainline 完成；
- 不提交论文、密钥、日志、构建目录或本地参考仓库。

## 9. 交叉评审

### 角色 A 评审 M3/M5

重点检查：

- 是否正确复用 M2 CmpAgg；
- 是否存在 rank、indicator 或 product 重构；
- Dealer 是否只在离线阶段参与；
- session/fingerprint 和 one-shot material 是否绑定；
- 三轮或两轮因果关系是否真实；
- 是否改变 M1/M2 公共语义。

### 角色 B 评审 M4

重点检查：

- QuickSelect 是否严格收缩；
- 重复值和全相等是否终止；
- 是否始终返回恰好 `K` 个位置；
- original index 是否跨 shuffle 保持；
- 原生代码、工程扩展和统一输出是否清楚区分。

### 双方评审 M6

重点检查：

- adaptive edge 是否依赖在线公开值；
- Dealer 是否根据在线状态补发材料；
- 完整图预留是否被错误称作 exact-edge；
- M2 复用是否只复用接口而没有继承错误假设；
- AAV86 与 Direct Top-K 是否使用独立标签；
- 比较量下降是否包含额外预处理和通信成本。

## 10. 分支建议

### 角色 B：M3

```text
m3-grank-runtime
m3-dpf-routing
m3-masked-combine
m3-protocol-iii-3round-e2e
```

### 角色 A：M4

```text
m4-ciphergpt-source-audit
m4-ciphergpt-failure-tests
m4-ciphergpt-native-topk-fix
m4-ciphergpt-mask-output
```

### M5

```text
m5-protocol-iii-field-contract
m5-protocol-iii-2round-runtime
m5-protocol-iii-2round-e2e
```

### M6

```text
m6-aav86-m2-reuse-audit
m6-aav86-adaptive-preprocessing
m6-aav86-runtime
m6-direct-topk-experimental
```

每个 PR 只处理一个可以独立验证的目标。

## 11. 合并顺序

1. 合并 M3 DPF conformance；
2. 合并 M3 share-preserving multiplication adapter；
3. 合并 M3 GRank runtime；
4. 合并 M3 DPF routing；
5. 合并 M3 secure combine；
6. 合并 M3 三方 E2E、审计和复现记录；
7. 确认 M3 达到退出条件；
8. 合并 M4 CipherGPT native baseline；
9. M3/M4 稳定后实现并合并 M5；
10. 角色 A 可提前合并不改变 runtime 的 M6 复用与预处理审计；
11. M5 完成且 M6 设计门通过后合并 M6 runtime；
12. 双方共同执行 M7。

## 12. 当前立即任务

### 角色 B

1. 修复并通过 `moe_topk_m3_grank_test`；
2. 运行 GRank oracle differential；
3. 运行相关 M1/M2/M3 回归；
4. 提交并创建 `m3-grank-runtime` PR；
5. 审阅并集成 DPF conformance 与 multiplication adapter；
6. 从最新 `main` 开始 M3 DPF routing；
7. 暂不实现 Protocol III 两轮压缩。

### 角色 A

1. 继续完成 M4 CipherGPT source/revision/license 审计；
2. 建立重复值、全相等和非法 K 失败测试；
3. 修复 QuickSelect 终止与错误传播；
4. 绑定 original index 并实现统一 mask；
5. 建立 M2 → M6 组件复用矩阵；
6. 单独研究 AAV86 adaptive exact-edge 预处理；
7. 不在设计门前声称 M6 paper-exact。

## 13. 完成原则

并行开发不表示无序合并。

双方必须保证：

- 不修改对方负责的协议实现；
- 不复制或分叉公共语义；
- 不绕过阶段门；
- 不把 test-only reconstruction 带入 secure runtime；
- 不把 M2 工程扩展错误写成其他论文的证明；
- 不把构建物、论文、密钥、日志或本地参考仓库提交到 Git；
- 每项结论都有 commit、测试和复现命令；
- 主线状态始终可审计。
```

手动提交流程：

```bash
# 先完成并提交当前 m3-grank-runtime 的代码，不要带着未提交修改切分支
git status

git switch main
git pull --ff-only origin main
git switch -c docs/m3-onward-team-work-plan
```

创建：

```text
docs/M3_ONWARD_TEAM_WORK_PLAN.md
```

建议在 `README.md` 的文档入口增加：

```markdown
- [M3 及后续双人分工计划](docs/M3_ONWARD_TEAM_WORK_PLAN.md)
```

检查并提交：

```bash
git diff --check
git status --short
git diff --name-only

git add docs/M3_ONWARD_TEAM_WORK_PLAN.md README.md
git commit -m "docs: update team plan for M3 and later milestones"
git push -u origin docs/m3-onward-team-work-plan
```

PR 标题：

```text
docs: update team plan for M3 and later milestones
```

PR 目标为 `main`，差异最好只包含：

```text
README.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md
```
