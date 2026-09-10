# M2/M3 共享契约冻结

- 冻结日期：2026-09-10
- 基线：`origin/main` = `2a83b19dead1230a416fa093c8ca2983cdec0f8e`
- 当前工作分支：`codex/m2-m3-contract-freeze`
- 状态：契约与验证基线已冻结；本变更只增加治理文档，不改变 `VFSS/`、`VFSS-baseline/` 或测试代码。

本文件冻结的是仓库当前实现的依赖边界和互操作契约，不把实现事实写成论文结论。证据分为：论文定义、本地参考行为、项目扩展、待验证设想。没有新鲜运行证据的性能、EMP-ON 结果和生产部署结论保持 `NOT_MEASURED`。

## 1. 当前实现身份

| 标签 | 输入与主链 | 当前轮数含义 | 证据边界 |
| --- | --- | --- | --- |
| `m2_protocol_i_modular_6round_mask_output` | 补齐后的 priority-key additive shares | current PI-shaped core 4 + reverse adapter 2 = 6 | 项目模块化扩展，不宣称论文精确轮数 |
| `m2_protocol_i_raw_score_input_modular_8round_mask_output` | raw score shares + 2-round carry/sign adapter | raw adapter 2 + current PI-shaped core 4 + reverse adapter 2 = 8 | 项目 raw-score 扩展，不宣称论文精确实现 |
| `agarwal_protocol_iii_modular_3round` | padded priority-key additive shares | GRank 1 + DPF routing/secure combine 2 = 3 | Protocol III 的项目模块化实现，不宣称论文原生 2-round |
| `moe_topk_protocol_iii_raw_score_modular_5round` | logical raw score shares | raw adapter 2 + formal modular core 3 = 5 | M3 raw-score 扩展，不把 5 轮写成论文三轮 |

M2 的 forward/reverse shuffle、controlled rank reveal 和独立 masked-key opening 属于 M2 当前 C-level 路径；它们不是 M3 的依赖。M2 的 exact 3-round 设计仍是独立待审边界，不能因 M3 三轮标签而倒推 M2 已完成。

## 2. 两阶段共享语义

### 2.1 输入、排序和 padding

- score 为冻结的 signed Q20.12：32-bit wire width，12 个小数位，scale 为 `4096`。
- raw additive shares 的算术重构语义仅定义为 `raw = (x0 + x1) mod 2^32`；协议执行方不在 secure 路径重构明文 score。
- 排序为 score 降序；同分按原始 index 升序稳定处理；rank `0` 是 top item。
- `logical_n` 是真实输入数，`padded_n` 是协议域长度；padding 使用冻结的低优先级 dummy 语义，不能混入有效 Top-K。
- `K` 必须在有效范围内；有效输出的逻辑 bit 数量为 `logical_n`，重构后恰有 `K` 个 `1`。

### 2.2 输出和角色

- 输出是原始输入顺序下的 logical Top-K bit-mask shares，不是排序后的 carrier，也不是 selected index 或明文 rank。
- 输出 share 按 bit 做 XOR 重构；secure production path 不重构 score、rank、indicator、selected index、product 或最终 mask。重构只允许出现在 test/oracle 路径。
- P0/P1 是在线两方；P2 是与输入无关的离线 provider，不接收 raw shares，完成一次 provider 工作后退出。P2 的角色不能退化成在线 Dealer。

### 2.3 帧、材料和失败语义

- 每个 framed message 绑定 `session`、输入 `fingerprint`、`phase`、`sequence`、`role`、`slot/type`、`width` 和 `count`；接收方必须逐字段校验。
- one-shot material 用过即清理；phase、role、slot、size、sequence 和输入绑定不匹配时 fail closed。
- replay、duplicate、EOF、truncation、trailing bytes、错误 role、错误 size 和超时均不得产生 partial output、明文 fallback、静默吞错或自动降级。
- 禁止 file polling、sleep 同步、无依据 retry、在线 Dealer 依赖，以及用零值替代未测或缺失材料。

## 3. M2 与 M3 的依赖矩阵

| 能力 | M2 当前路径 | M3 当前路径 | 冻结判断 |
| --- | --- | --- | --- |
| Q20.12、signed score、stable tie、oracle | 使用 | 使用 | 共享语义，可兼容 |
| priority-key、uCMP、CMpAgg、DPF/transport 基础设施 | 使用 | 使用对应组件 | 共享组件，但阶段和 frame binding 仍须校验 |
| raw-score carry/sign adapter | M2 raw 入口使用 | M3 raw extension 调用同一入口语义 | 共享 2-round adapter；总轮数不可混写 |
| M2 forward/reverse secret-shared shuffle | 使用 | 不调用 | 不属于 M3 依赖 |
| M2 shuffled-rank reconstruction/rank reveal | 使用/审计为 M2 泄露边界 | 不调用 | 不得带入 M3 |
| M2 reverse carrier adapter | 使用 | 不调用 | 不得带入 M3 |
| M2 pipeline 与独立 masked-key-after-forward-shuffle phase | 使用 | 不调用 | 不得带入 M3 |
| M3 GRank masked CmpAgg exchange | 不使用 | 自有 1-round GRank 阶段 | M3 独立阶段，不称为 M2 stage |
| M2 formal MetricsRecord implementation | 无同级闭环可复用 | 自有 `protocol_iii_metrics_record.*` factory | 只共享字段/Measurement/provenance schema |

M3 secure core 的固定链为 `GRank -> DPF routing -> secure combine`；priority-key 版本为 3 轮，raw-score 版本在其前面增加 2 轮输入适配。静态审计确认 M3 入口和 `protocol_iii_*` 主链不调用 `protocol_i_shuffle_forward_party`、`protocol_i_shuffle_reverse_party`、`protocol_i_priority_pipeline_party`、M2 shuffle/reverse API 或 M2 pipeline。

## 4. 泄露与度量边界

- M2 的当前 C-level 记录保留其既有 controlled shuffled-rank reveal、masked-key opening 和 reverse adapter 事实；后续修复不能把这些历史事实改写为“已满足论文精确泄露边界”。
- M3 secure path 只允许最终原始顺序 mask 作为输出；不以 `true_rank`、selected index、明文校验数组或明文 score 作为协议输出。
- 轮数必须按阶段记录：M2 raw 为 `2 + 4 + 2`，M3 priority-key 为 `1 + 1 + 1`，M3 raw 为 `2 + 1 + 1 + 1`。不能用 label 替代 phase evidence。
- M3 使用自己的 `protocol_iii_metrics_record.*` factory；共享的是 `MetricsRecord` 的字段、`Measurement` 状态和 provenance schema。正式指标必须绑定 revision、实现标签、输入/种子、环境、命令、重复次数和原始计数；缺失项写 `NOT_MEASURED`。

## 5. 兼容规则与变更门槛

1. M2 priority-key package 只有在 session、fingerprint、phase、role、width、count、one-shot slot 和 output semantics 全部相同，并且明确属于 priority-key 入口时，才可与 M3 priority-key core 对接。
2. M2 raw-score adapter 与 M3 raw extension 的兼容只覆盖冻结的 2-round Q20.12 输入转换；不能把 M3 raw 的 5 轮重新标为 M3 三轮，也不能把 M2 的 8 轮缩写成论文轮数。
3. M2 shuffle、reverse shuffle、shuffled-rank、reverse carrier 和 M2 pipeline 的 material/package 不得直接接入 M3。
4. 改变输入含义、输出顺序、角色、轮数、泄露、padding、预处理时机、frame schema、指标边界或错误处理时，必须先新增/更新决策文档，再同时更新实施计划、conformance、oracle differential、独立进程 E2E 和指标 provenance。
5. 任何不匹配均 fail closed；禁止兼容层吞掉字段、自动猜测阶段、静默降级到明文或旧 shuffle 路径。

## 6. 联合回归基线

本冻结对应的当前 EMP-OFF fresh baseline 为 24/24 CTest 全通过，覆盖 M1、M2 和 M3；M3 的 11 个测试必须与 M2 的 9 个测试和 M1 的 4 个测试分开报告。EMP-ON 因当前主机缺少 pinned `emp-tool`/`emp-ot` 配置，保持 `NOT_MEASURED`，不得借用历史 Ubuntu 结果填充。

后续 M2 修复的最低回归门槛是：保留本文件的共享语义，重新运行 M2 相关 conformance、oracle differential/对照、独立进程 E2E，并确认 M3 的 primitive、three-process E2E、secure executable、raw-score executable、metrics/provenance 测试仍通过。任何 M3 依赖矩阵变化都必须在本文件和对应复现记录中留下证据。

## 7. 未决项

- 请求中指定的 `docs/decisions/M3_RAW_SCORE_SECURE_ENTRY.md` 在当前 checkout 不存在；本次不伪造该文档，也不把本冻结记录冒充为它的替代品。
- M2 exact 3-round paper alignment、严格泄露审计和 EMP-ON 当前环境仍不是本次治理任务的完成项。
- 本次只冻结契约和验证基线；没有实现 M2 三轮修复，也没有修改历史复现记录中的旧 revision 或旧结论。
