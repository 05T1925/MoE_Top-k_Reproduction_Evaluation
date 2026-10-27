# M6B-I PROJECT_DERIVED_SELECT_5ITER S61 决策

日期：2026-10-27（文件名沿用预置草稿日期）
固定对象：S60 commit `07325b076dc31f83439d55d7a59ad2239e0d5fcf`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **固定来源：`PASS_WITH_SCOPE`。**本轮从 S60 精确 commit 开始；parent 为 `0f67aec45d767c4bf530b12e4d548a82d68b6800`，tree 为 `748a4d86bdfbaffc6d401e8d25f3de7a02ed90f2`。S60 tracked tree 与该提交一致；初始工作区另有两份预置未跟踪 S61 草稿，读取后保留并修订。S60 manifest 中 runtime 源码 blob 与本轮行号锚点一致。
- **有限 cut 集合 `E_cut`：`PASS_WITH_SCOPE`。**cut 从成功 package/preflight/lease/channel setup 之后开始，覆盖源码规定的 phase/body-ready、完整应用帧发送/接收、每个 Beaver batch 处理完成、Select5 活动比较结果/状态更新、share 形成及 escrow submit/fetch/API 边界。`FRAME_BODY_READY` 明确覆盖 output-refresh 的 `u` 与 ACK body。集合在 `N≤256` 有限并按公开参数与原 `L_S29` 实例化；任意 CPU 指令点、partial-I/O 字节点、TLS record 内部点和故障注入点不属于该集合。
- **`Kpre-prefix`：`PASS_WITH_SCOPE`。**在 `1≤K≤N≤256`、理想 FTv3 材料且 `ISSUED/cap_success`、一方静态半诚实、理想 `F_TOPK` 输出刷新、既有 `L_S29`、诚实对端因果执行、无 I/O 故障、cut 由外生 event scheduler 指定的范围内，为每个 party 与枚举 source-event cut 给出权限受限 sampler。计算前缀从 S50/S36 的逐门条件核构造，并用“固定 `z_b` 不改变 output-refresh 之前计算前缀 law”的条件等式边缘化；没有把带未来输出 share 的完整成功 transcript 直接截断当作证明。
- **output-refresh 特殊 cut：`PASS_WITH_SCOPE`。**P1 收到 P0 刷新 frame、但尚未形成本地 share 时，模拟器按 fresh ideal refresh 得到的均匀 `u` 条件核采样，不取得未来 `z_1`；P0 的 share 形成、refresh body-ready、send 与 ACK cut 按源码次序处理。只有 share 实际形成之后，才将腐化方自己的 staged `z_b` 纳入其本地模拟状态。
- **成功 escrow 的事件前缀：`PASS_WITH_SCOPE`（继承 S51 ideal kernel 并经本轮增量复核）。**同一 ideal A 状态机、ideal token RNG、honest peer endpoint、外生 `SuccessFence` 下，按因果转移采样 PENDING、COMMITTED、submit/fetch response 和 API return 前缀。A 隐状态不交给 party simulator；诚实对端用自己的同次输入、输出、package 和凭据提交。非作者 reviewer 确认该 event-prefix 扩展不是完整 transcript 截断。它不覆盖 ABORT、UNKNOWN、fetch 失败、响应丢失、restart 或 timeout。
- **未筛选 A+B 完整定理：`OPEN`。**cap-abort 到双方 package/context delivery、start/notice 的 joint kernel仍无负责人批准的合同；中断/失败 response、`COMMIT_STATUS_UNKNOWN`、超时及恢复失败也未纳入本轮 `E_cut`。本轮只解决 `ISSUED` 后的有限协议事件 cut 子目标。
- **不构成完整安全通过。**真实 T/A DRBG、签名、私有信道、recipient encryption、真实有限 timeout/wall-clock、部署持久性、全部协议故障前缀、N>256 安全扩域继续各自 OPEN/NOT_IN_SCOPE。`L_S29`、角色、输入/输出和默认开关未改变。

## 明确适用条件

1. 本方计算输入为 signed Q20.12 shares；目标功能为分数降序、原始索引稳定同分，输出原序恰 K 个 1 的 Top-K XOR mask shares。该项目派生实现不等同 BB90/BMW16 原协议。
2. 本轮条件分布固定完整合法调用元组、有效本方 package、理想材料联合律、公开参数与既有 `L_S29`。对 `L_S29` 不增加 phase、batch、长度、time、cut 或内部 wire 字段。
3. scheduler 只能在预先定义的有限 source-event 标签上选择 cut，且 cut 选择不读取隐藏材料、wire、peer secret 或 A 数据库；本轮不建模按真实耗时/成功与否事后筛选。
4. `share_formed_b`、`ciphertext_submitted_b`、`share_fetched_b`、`api_returned_share_b` 是不同状态；`COMMIT_STATUS_UNKNOWN` 是客户端观察而非 A 的数据库状态。

## 保留的项目确认事项

- FTv3 cap-abort 烧毁本次 invocation 后，是否只允许以新 invocation 重试：`OPEN_CONFIRMATION`。
- 发材失败及双方启动通知规则、通知不可达时是否接受各方看到不同 timeout/UNKNOWN：`OPEN_CONFIRMATION`。
- 安全声明只覆盖发材成功后的正常协议，还是纳入协议级 abort：`OPEN_CONFIRMATION`。既有“算法随机失败数值界暂缓”不等于 cap-abort 合同获批。

## 执行与交付状态

- 不修改 runtime、测试、材料 ABI、构建选项、`L_S29`、输出或角色。
- `BUILD=NOT_RUN`，`TEST=NOT_RUN`；不重跑 S35/S40/S52 矩阵。
- 自然业务 I/O：本轮 `NOT_RUN`，沿用历史 `NOT_REPRODUCED`；人工 kill 证据不外推。
- 正式 LAN/WAN、RTT、AAV86 和性能：`NOT_RUN/NOT_MEASURED`；不创建 `secure` alias，不推送、不合并、不建 PR。
- `VFSS-baseline/` 不变。

独立 reviewer `/root/s61_prefix_review` 未参与 S50–S61 的推导。其只读复核确认 `E_cut` 的明确事件词汇、`z_b` 边缘化、P1 均匀 `u` 核及 S51 ideal Kpost 的因果 event-prefix 扩展；未重推 S36/S50 Beaver 条件材料基础引理，未重新认证 S51 token 基础引理或实际 escrow refinement，也未构建/测试。该复核支持本轮限定增量结论，不称为功能接收或完整安全认证。

详细事件集合、每个 party cut 的条件 sampler 和源代码映射见 [S61 report](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S61_REPORT_2026-10-26.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S61_GATES_2026-10-26.json) 与 [source/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S61_SOURCE_EVIDENCE_MANIFEST_2026-10-26.json)。
