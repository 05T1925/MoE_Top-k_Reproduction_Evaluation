# M6B-I PROJECT_DERIVED_SELECT_5ITER S5 决策记录

日期：2026-10-13
路线：`PROJECT_DERIVED_SELECT_5ITER`
组合：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
基线：S4 未提交快照，基于 `c3926c68fd14f270faa8b55234311071947fa080`
状态：**N≤128 功能候选复跑通过；异人独立接收未通过；联合单方视图安全 OPEN；正式性能 NO。**

## 决策

1. 接收 S4 的可复现功能基线，范围限于当前源快照、Select controller、稳定 Q20.12 K-th oracle、DCF membership、shuffle/inverse route、材料包接口和 TLS 1.3 mTLS 实验传输。S5 执行者曾参与 S4，因此这是技术复跑，不是异人独立签收；`INDEPENDENT_ACCEPTANCE=NO_GO_NOT_INDEPENDENT`。
2. `M6B5/v1` package 使用 SHA-256 ready marker 与本地 POSIX `O_EXCL` per-party durable lease。ready 绑定 package length/digest、session、fingerprint、material ID、logical/padded N、K、width。T 正常退出后由监督端发布 ready；party lease 在读取/释放字节前创建并 fsync。并发仅一方可领；claim 后崩溃会永久 burn package，失败时不返回 mask。
3. runtime 要求 TLS 1.3 mTLS；无静默 plaintext fallback。带 material ID 的 v2 应用帧绑定 session/fingerprint/material ID/N/K/width/party direction/type/phase/sequence。显式 plaintext 仅供 TEST_ONLY harness。
4. 运行范围保持 N≤128。N=128 实际序列化 `60,008,580 B/party`；N=256 当前 `M6B5/v1` 公式得 `245,683,268 B/party`，超过 64 MiB；logical N=1000 pad 至1024 时为 `4,109,070,020 B/party`。`M6S2` streamed slot store 仅是单槽分段存储原型，没有接入 Select5 Dealer/runtime，不提升支持上限。
5. 新 Select5、slot-store 和 TLS conformance targets 仅在 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON` 注册，默认 OFF。无默认排序 alias、无 `secure` alias，不替代 BMW16-derived、AAV86、全对全或 BB90 行。
6. `JOINT_VIEW_SECURITY=OPEN`。Party package 含整池已用/未用 DCF shares，密钥阈值与 mask 有相关性，transcript 自适应选择后续 slot；shuffle package 还有 party-local `sigma/tau`、record masks 和当前两轮 shuffle 未消费的 DCF pool。没有针对完整 P0/P1 view、T 的 `Pi` trust boundary、inverse/output、abort/restart 的 simulator/hybrid。BGI15、BMW16/S31/S32 或其他路线证明不可自动复用。
7. 唯一窄披露待确认字段是 `public_sample_seed`：T 生成后在 package/binding 中对 P0/P1 可见，用于复现 endpoint schedule；它独立于输入，但 S5 重新列举的 L 未显式列出 raw seed。应明确纳入 L 或改成只公开获准的 endpoint transcript。其他已确认字段不重问。
8. S1 BB90 来源 `NO_GO_FOR_IMPLEMENTATION`、BMW16-derived 独立候选、S34 非异会话接收、M6A V3 `OPEN`、V4 `NOT_ALLOWED/NOT_RUN` 均保持。

## 阶段门

| Gate | 裁决 | 范围 |
|---|---|---|
| `S4_RECEIPT` | `PASS_REPRODUCED_WITH_SCOPE` | fresh Release rebuild/rerun；不是异人接收 |
| `INDEPENDENT_ACCEPTANCE` | `NO_GO_NOT_INDEPENDENT` | 执行者参与过 S4 |
| `ALGORITHM_STABLE_TOPK` | `PASS_BOUNDED` | 合法输入 `1≤K≤n≤128`；所有 random tape 由第五层 exact clique 闭合 |
| `DCF_MEMBERSHIP` | `PASS_FUNCTIONAL` | 真实 uCMP ABI；唯一 stable key 上 `1−strict_lt(τ,key_i)`，E2E 恰 K |
| `LEAKAGE_L_MAPPING` | `CONDITIONAL` | raw `public_sample_seed` 需确认/移除 |
| `ADAPTIVE_CONTROL` | `PASS_FUNCTIONAL / SECURITY_OPEN` | opened bits 决定后续 graph，无额外 secret rank mux |
| `MATERIAL_ABI` | `PASS_N128_V1` | 40,640 round-aware Select slots/party |
| `DURABLE_ONE_SHOT_LEASE` | `PASS_LOCAL_FS_CONTRACT` | `O_EXCL`/fsync/digest；管理员删除或回滚旧备份除外 |
| `AUTHENTICATED_TRANSPORT` | `PASS_TLS13_MTLS_FUNCTIONAL` | 错证书、篡改、断连 no-mask 已测；生产证书运营未测 |
| `M6S2_STREAMED_STORE` | `PASS_PROTOTYPE_ONLY` | 分段/manifest/slot lease 测试通过，尚未集成 |
| `FUNCTIONAL_E2E` | `PASS_N128_BOUNDED` | 10 success + 5 fault no-mask cases |
| `JOINT_VIEW_SECURITY` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | 完整 party view 尚无组合模拟 |
| `N>128_RUNTIME` | `NO_GO` | 固定包 cap / runtime bound；M6S2 未接线 |
| `FORMAL_PERFORMANCE_READY` | `NO` | LAN/WAN 未运行；V3/V4 门不变 |

## 下一阶段入口

建议下一任务为 `M6B-I-PROJECT-SELECT-5ITER-S6-INDEPENDENT-SECURITY-REVIEW`。开始前需由未参与 S4 的审查者复跑 S5 receipt；完成 `public_sample_seed` 窄 L 决策；并对真实 `keyGenDCF/evalDCF` local key、mask shares、公开 z/bit、自适应地址、完整 used/unused pool、shuffle、inverse/output、abort/restart 写出 P0/P1 simulator 或 hybrid。若推进 N=256，另需把 M6S2 接到 Dealer 逐槽生成与 party 运行时，并处理超过 1 MiB 的 comparator operand frame。安全证明未完成前保持 EXPERIMENTAL；正式性能依然等待 V3/V4 及性能计划前置门。

详见 [S5 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S5_REPORT_2026-10-13.md)、[逐案 E2E JSON](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S5_E2E_CASES_2026-10-13.json) 和 [S4/S5 文件哈希清单](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S5_SOURCE_MANIFEST_2026-10-13.json)。
