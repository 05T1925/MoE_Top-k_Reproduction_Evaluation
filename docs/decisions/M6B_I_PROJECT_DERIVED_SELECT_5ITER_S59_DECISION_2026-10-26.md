# M6B-I PROJECT_DERIVED_SELECT_5ITER S59 决策

日期：2026-10-26
固定审查对象：S58 `4ae16569317f4526699ccb0e55a156eafcd03a1e`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **S58 来源接收：`PASS_WITH_SCOPE`。**隔离工作树的 HEAD/tree/parent 与指定提交一致。S58 manifest 的 7 项主源码、9 项证据输入、15 项固定 S55 runtime 源码逐项复核匹配；S58 五文件交付规范摘要复算匹配。S57 历史 manifest 的勘误继续保留，不修改历史记录。
- **完整单方无条件视图模拟：`OPEN`。**S50 `Kpre₃` 和 S51 `Kpost₃` 只给理想发材成功、理想 A token RNG、诚实对端按因果提交、`N≤256` 且外生 `SuccessFence` 下的成功应用消息视图。它们没有定义未筛选分布中 cap-abort 到 package 交付/双方启动/通知的联合路由，也没有覆盖每个 escrow 中止、未知响应与恢复切点。
- **首次协议级接口断点：`ISSUANCE_ABORT → D_b`。**理想 `F_T/v3` 可向离线发行调用者报告 sampler cap 耗尽并保留 private burn，但没有同一 invocation 的 `D_b=(package delivery, party start, party notice)` 联合规则；`F_TOPK` 独立定义也没有该门。因此当前不具备合法权限的 `K_abort^b` 来生成两方可见完整视图。本结论是接口/合同未定义，不是攻击或不可能性。
- **部署故障的首次源码断点：签名 key 读取/解析。**`score_to_mask_v3_gmw.cpp:862–870` 在 marker 注册及材料生成前读取 signer key；错误并未进入理想 `F_T/v3`。若把 signer 可用性外生固定为成功，则随后 marker 的多阶段 O_EXCL/write/fsync/close/dir-fsync 结果 (`:472–543`) 是首个持久状态分支缺口。部署错误不能与理想 cap-abort 混称。
- **错误码精度：`CONDITIONAL / OPEN`。**源码 marker durability 失败也报带 `ISSUANCE_ABORT` 的异常 (`score_to_mask_v3_gmw.cpp:542`)，而 bounded sampler 耗尽另在 `:925` 报同样状态前缀。必须按产生原因区分协议 cap-abort 与部署 marker failure；共享前缀不证明二者同分布。
- **重放分支：**同一 marker 的 `EEXIST` 重放拒绝位于 `score_to_mask_v3_gmw.cpp:510–513`，也使用 `ISSUANCE_ABORT` 前缀，但它是 one-shot burn/replay 分支，不是随机 cap exhaustion；该消息字符串不能替代原因字段或状态模型。
- **A 状态及 party API 映射：`PASS_WITH_SCOPE`（源码静态映射）。**A 的 `NO_TX/PENDING/COMMITTED/ABORTED`、token/事务状态与 party 的 `ESCROW_ABORTED/COMMIT_STATUS_UNKNOWN/COMMIT_RECEIPT_INVALID/COMMITTED_FETCH_REQUIRED/COMMITTED` 分开；本地 share 可先于 submit 形成，API 仅在 fetch、receipt、解密及本地一致性检查成功后返回它。该映射不构成失败视图证明，也不证明数据库故障/回滚/耐久性。
- **`F_TOPK-ISSUE-ESCROW/v1`：`CONDITIONAL / NOT_APPROVED / NOT_IMPLEMENTED`。**本轮给出 proof-only 的状态和权限规格，供负责人决策及后续独立复核；不改变 runtime、角色、输出或 `L_S29`。它不会自动证明目标部署故障分布。
- **SuccessFence 成功视图：`PASS_WITH_SCOPE`（继承）。**范围仍是理想 IID FTv3、cap-success、理想 A token RNG、单方静态半诚实、诚实 peer 因果提交、`1≤K≤N≤256`、外生合法消息按期完成。真实有限超时/墙钟成功筛选保持 `OPEN`。
- **负责人合同问题：均 `OPEN_CONFIRMATION`。**低概率 cap-abort 后烧掉 invocation 并以新 invocation 重试；发材失败时是否要求双方一致失败且不得启动、通知不可达时是否接受 timeout/UNKNOWN；安全声明只覆盖初始化/发材成功后的隐私，还是纳入协议级 abort 状态。既有正常 escrow/输出合同和人工 kill 含义不重复询问。
- **本轮执行：**未改 runtime 或测试；`BUILD=NOT_RUN`、`TEST=NOT_RUN`。未运行 S35/S40/S52 历史矩阵、自然 I/O 故障实验、LAN/WAN/RTT/AAV86 或性能矩阵；`PERFORMANCE=NOT_MEASURED`。`VFSS-baseline/` 未修改；未创建 `secure` alias；未推送、合并或创建 PR。

## 拟冻结的定理范围

当前可沿用的唯一成功定理是 S50/S51 的 SuccessFence 条件成功视图；失败分支的隐私声明尚未冻结。S59 提议把下一阶段证明拆为两项，不能混写：

1. **成功定理 `T_success`：**接受理想发材成功并以 `SuccessFence` 保证合法应用消息及时处理；`N≤256`、静态半诚实一方、T/A 条件受信且不合谋。只对成功应用消息 view 组合 S50/S51；不覆盖 cap-abort、实际 timeout 或部署异常。
2. **abort-aware 定理候选 `T_abort`：**把 `F_T/v3` cap-abort、协议验证拒绝、诚实端点的协议中止，以及明确建模 cut 产生的 A 数据库 `PENDING/COMMITTED/ABORTED` 与各方观察 `COMMIT_STATUS_UNKNOWN`、恢复响应纳入未筛选联合 party view；这些分别是不同随机变量，不能将 `UNKNOWN` 作为 A 的数据库状态。由版本化组合功能及确定性 fault-cut 或显式概率核规定它们。该模型未获负责人确认、未实现、未独立证明，当前为 `OPEN`。真实 TLS/网络断连、deadline、服务重启属于 C 类部署环境事件；只有在明确建模成抽象 fault cut 时才讨论为理想状态分支，不由 `T_abort` 自动覆盖。

签名器/文件系统/RAND/provider/SQLite/TLS/机器与介质故障、以及真实 deadline/调度如果要纳入，必须增加目标部署联合 kernel；在取得决定前既不暗中纳入 `T_abort`，也不宣称这些故障视图已被隐私证明覆盖。

## 待负责人确认

1. 合法输入下，FTv3 bounded sampler 极低概率耗尽上限并烧掉本次 invocation 后，是否接受以**新 invocation**重新发材？此采样合同与“五迭代算法数值随机失败上限”的既有豁免不是同一决策。
2. signer/marker/文件发布等发材故障时，是否要求双方获得一致且不能误认为成功的失败结果、两方都不得启动？如果一方无法收到通知，是否允许该方观察 timeout/`COMMIT_STATUS_UNKNOWN`？
3. 安全声明是否限定为初始化与发材成功后的正常协议隐私、将底层故障列作可靠性边界；还是要求 ideal functionality 与证明覆盖协议级 abort 状态？

## 独立复核

`/root/s59_independent_review` 是本轮新委派的非作者审查者，未参与 S50–S59 推导。其只读核验固定 S58 源码/manifest，并复核 S59 的状态链、模拟权限、首个接口断点及候选功能；特别提示 marker durability failure、EEXIST replay refusal 与 sampler cap-abort 虽可共用错误字符串但必须按 cause 分开，且实际 transport failure 归部署环境范围，不能和 abstract protocol cut 混列。未编辑、未构建、未测试；该审查是证明边界/合同复核，不是功能接收，也不构成完整密码学证明。最终报告/门禁记录其审查范围及剩余限制。
