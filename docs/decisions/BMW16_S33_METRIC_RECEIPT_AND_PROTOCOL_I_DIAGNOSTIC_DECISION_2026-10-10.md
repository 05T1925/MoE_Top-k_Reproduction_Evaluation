# S33：S32 计量技术复核与 Protocol I 三路线诊断决策（2026-10-10）

## 决策摘要

1. 技术复核 S32 的源码计数点、四份 schema-v2 历史记录与证据 manifest。未发现改变协议消息、泄露、材料 ABI、一次性消费或输出的改动；schema-v2 仍是 S32 诊断身份，原始文件和哈希不改写。S33 实施者也参与审查，因此不称为独立接收。
2. 新建 `PROTOCOL_I_UNIFIED_RUN_RECORD_SCHEMA_3.json` 和 `protocol_i_unified_diagnostic.py`，为 Protocol I 全对全、Protocol I+AAV86、Protocol I+BMW16-derived Select+DCF 固定九项主指标、采集边界、身份绑定、失败记录和逐次证据哈希。
3. S32 计量比较校准采用同一 TEST_ONLY score/tape 与固定 `getrandom` shim。成功和强制 abort 路径的功能、计数、通信、边迹及 no-mask 状态等价；计量开销只作为本机小样本诊断。
4. 完成 4 个 n/K 配置的三路线配对诊断（n=128/256、K=2/8，AAV86 `r=2`），同一配置三路共用 signed Q20.12 输入与冻结 oracle，各自领取 fresh session/material。结果为 DIAGNOSTIC，不拼接旧 revision 中位数，也不形成正式排名。
5. n=1000/K=80 三路线资源/准入预检中，全对全入口不支持该形状，AAV86 在 D=1024 超过 E16 硬上限；因此联合配对拒绝，未生成协议材料或性能结果。BMW16-derived 符合 conditional_secure_v2 的形状界，但只处于 `RESOURCE_REVIEW_REQUIRED`，当前 RAM/峰值 RSS/网络/时限证据不足以单路线准入。
6. M6A/V3 前置门仍打开。正式 V4 LAN/WAN `1 warmup + 5 repetitions` 为 `READY/NOT_RUN`；schema/sealer 就绪不代表服务器运行或 V4 验收。

## 路线身份与主张边界

BMW16-derived 路线始终是 `Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED`。它不是 BB90 原算法复现，不是 BMW16 Algorithm 7 逐字实现，也不继承二者的轮数、比较常数或概率定理。S31 接收的 `conditional_secure_v2` 仍受原有条件假设和泄露函数 L 限定：可信且不与在线方合谋的离线 T、最多一名半诚实在线方被腐化、私有认证通道、source-specific G126/受限 AES-key 子族 DCF 假设、cryptoTools AES-CTR root-stream PRG、OS CSPRNG，以及至少一个诚实不可预测熵贡献下的 ROM sampler。它不升级为无条件安全、恶意安全或部署安全声明。

BMW16-derived 完整入口按实际消息依赖 DAG 计为 12 个应用阶段：raw-score adapter 2、forward shuffle 2、共同 sampler coin 1、Select R1–R4 4、inverse shuffle 2、final agreement 1。Select 的四层不是完整协议四轮。两条 E16 bridge 路线本次实测完整入口为 8 个应用阶段；各路线轮数只描述当前实测实现，不作为论文核心轮数主张。

## schema-v3 主指标裁决

| 字段 | 正式主口径与计数点 | 单独记录，不混入主字段 |
| --- | --- | --- |
| `offline_time_ms` | T/receiver 启动至双方材料提交并 ready 的同次 wall interval | 输入生成、TEST_ONLY 份额拆分、控制器和 ready 等待分别计时 |
| `offline_material_total_bits` | 在线双方实际持有/消费的预处理 payload 字节总和 ×8 | shell/package/sidecar 文件、AEAD/tag、manifest、TLS application 与 wire 交付分别列出 |
| `online_time_ms` | 输入与材料 ready 至双方最终原序 mask 发布的关键路径 | 控制器辅助、oracle 校验、清理和 party API 子阶段另列 |
| `online_comm_total_bits`、`online_comm_per_party_bits` | 固定应用帧计数；total=`8×(P0_sent+P1_sent)`，per-party=`total/2` | P0/P1 send/receive 用于对端守恒；TLS、TCP/IP wire 可空缺并单列 |
| `online_rounds` | 按实际消息依赖 DAG 的应用阶段计数 | 帧数、函数次数和 Select 四层均不代替完整入口轮数 |
| `online_prg_calls_total` | 在线长度倍增 DCF/PRG 实际扩展调用点计数 | DCF Eval、AES blocks、SHA sampler words、离线 KeyGen/root stream 分列 |
| `comparison_edges_total` | 输入适配、核心和输出适配实际执行的无序安全比较边 | logical/dummy/repeated 请求、材料槽、双方重复 Eval 分列 |
| `total_time_ms` | 同一次尝试的 offline + online | 每行先对账，再统计；禁止相加不同配置的 median |

S31 server runbook 曾把 `sidecar + shell` 精确交付文件字节作为材料主字段候选，S32 已按 PROJECT 主口径采集在线方持有的 payload。schema-v3 将该口径冻结，封装与传输开销单列；不会重标或批量改写 S32 schema-v2 记录。S31 runbook 对 comparisons 的逻辑请求口径也由 schema-v3 的实际无序边口径取代，dummy/repeat/材料预留另列。

对 BMW16-derived，审计恒等式为：

- `offline_material_total_bits = 8 × (T_payload_P0 + T_payload_P1)`；
- `P0_sent = P1_received`、`P1_sent = P0_received`，`online_comm_total_bits = 8 × (P0_sent + P1_sent)`，per-party 等于 total/2；
- 两方 DCF Eval counter 各自等于 runtime DCF Eval；在线 PRG calls 为两方 `dcf_eval_node_expansions` 之和，AES block 与 SHA sampler word 分开；
- comparison edges 为 raw adapter 已执行 uCMP + 四层 Select 的唯一真实边 + membership 边；dummy、重复逻辑请求及材料槽不混加；
- `online_rounds` 由双方同意的阶段 counter 与消息 DAG 核对；同一 attempt 的 `total_time_ms = offline_time_ms + online_time_ms`（微秒计时误差容差 0.002 ms）。

所有未测项保存 JSON `null` 并标 `NOT_MEASURED`，禁止用零或容量推算代替。正式 attempt 必须记录网络模式、RTT、吞吐、拓扑、测量方法、warmup/formal 身份、revision/工作树差异/二进制/build-options、输入与 oracle 身份、失败原因以及逐次 raw evidence manifest 哈希。FORMAL campaign 校验要求 V3 机器可读 PASS，并恰有每路线一次 warmup、五次正式结果；本地 collector 只接受 DIAGNOSTIC，正式 sealer 只校验服务器侧已采集的 18 条记录和网络证据。

## 修改和阶段门

S33 仅修改主工作树中的 TEST_ONLY benchmark harness、schema/collector 和治理文档；E16 all-pairs/AAV86 的计量 bridge 在另一个 worktree，仅修改 TEST_ONLY harness。没有修改 `VFSS/` secure 算法路径、`VFSS-baseline/`、密钥、证书、sidecar、论文或参考树。逐次数据与复核结论见 `docs/reproduction/BMW16_S33_TECHNICAL_METRIC_REVIEW_AND_DIAGNOSTIC_REPORT_2026-10-10.md`。

| 门 | 判定 |
| --- | --- |
| S26 独立功能接收 | 历史 `DRAFT_PR_READY=PASS`，仅代表默认关闭的实验功能候选可供 Draft PR 评审 |
| S31 conditional_secure_v2 接收 | 沿用既有 CONDITIONAL_ACCEPTED；S33 未重开安全证明 |
| S32 计量源码/原始记录技术复核 | PASS |
| schema-v3 与本地诊断 collector / 审计器 | PASS；正式服务器端采集尚未运行 |
| 三路线小规模配对诊断 | PASS，4 组 × 3 路 = 12 条 SUCCESS |
| n=1000/K=80 三路线预检 | A/B PRECHECK_REJECTED；C RESOURCE_REVIEW_REQUIRED；未运行且 NOT_MEASURED |
| PR #29 的 S32 内容已推送 | YES；远端 head=`32cd7952cccdacc5a1a5b4fcca245f7acf594049` |
| PR #29 reviewer approval | NO / NOT_APPROVED；Copilot 因 requester quota limit 未完成 review |
| PR #29 已合入 main | NO / NOT_MERGED；PR 仍为 Open |
| S33 是否已推送到 PR #29 | NO；S33/S34 留在本地技术分支，没有推送 |
| M6A/V3 | OPEN |
| 正式 V4 LAN/WAN | READY/NOT_RUN，等待 V3 关闭 |

## S34 技术复核状态更正（2026-10-12）

本 S33 记录由 S33 实施者参与撰写和审查，准确身份为 S32 计量技术复核及 schema-v3 诊断交付，不是独立接收。S34 在外部证据目录复核了 22 个 campaign；选定的 4 个 n=128/256、K=2/8 配对 campaign 和 3 个身份/准入验证 campaign 通过增强后的 schema-v3 audit。其余 15 个早期探索/失败 campaign 的 JSON、manifest 和哈希保持原样，排除在最终 12 条配对诊断之外。

S34 修正了两个计量工具问题：preflight 不再将 n>1000 的 BMW route 报为 READY；campaign audit/seal 现在核验配置与 attempt 的 n/K、输入/oracle SHA、seed、command identity 和 fresh-session/material serial。当前 n=1000/K=80 BMW 单路线为 `RESOURCE_REVIEW_REQUIRED / NOT_RUN`，不是已准入。V3 仍 OPEN，V4 正式 LAN/WAN 仍禁止启动。完整证据见 `docs/reproduction/BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md`。
