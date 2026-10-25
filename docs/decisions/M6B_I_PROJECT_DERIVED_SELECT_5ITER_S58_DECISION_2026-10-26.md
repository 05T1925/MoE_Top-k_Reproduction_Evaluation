# M6B-I PROJECT_DERIVED_SELECT_5ITER S58 决策

日期：2026-10-26
固定审查对象：S57 `79f61f6298e0da65b7c64ea4e748bee1afd1ebb1`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **S57 基线身份：`PASS_WITH_SCOPE`。**在隔离 worktree 从指定提交开始，HEAD、tree `e18037236c7c58089a8ac58b792adf9716b56b29`、父提交 `66a3db485ffa9ac752a49fd0f8a0f100fcef4cbe` 与提交对象相符。S57 manifest 的 7 个主要源文件、9 个证据输入和 15 个 S55 固定运行时源文件的逐项哈希/blob 均匹配。S56 历史交付摘要可按 manifest 重算。另发现 S57 manifest 对 decision/report 的两个 Git blob ID 记录不正确；本轮以固定 S57 commit 中的实际 blob 为准，在 S58 manifest 保留逐项勘误，不改写历史记录。
- **无成功事件筛选的单方完整视图模拟：`OPEN`。**当前 `F_T/v3`、`F_TOPK` 与 A escrow 分开定义。它们没有规定同一个 invocation 的发行结果、package/context 交付、party 启动/通知、`F_TOPK` 输出、A 状态变化、API 观察和本地 share 是否已形成之间的完整联合状态核。现有成功条件模拟不能覆盖该未筛选分布。
- **正常成功执行：`PASS_WITH_SCOPE`（继承的理想模型结论）。**仅在理想 IID `F_T/v3` 且 `cap_success`、理想 A token RNG、至多一方静态半诚实、A/T 按既定模型条件受信且不与腐化方合谋、诚实对端因果提交、`1≤K≤N≤256`，并由外生 `SuccessFence` 保证合法消息按期处理时，S50 `Kpre₃` 与 S51 `Kpost₃` 支持限定的成功应用消息视图。此结论不说明真实运行中的超时选择、失败前缀、部署概率或可用性。
- **协议级中止：`CONDITIONAL / OPEN`。**理想 `F_T/v3` 已定义 bounded sampler 达上限后向发行 caller 返回 `ISSUANCE_ABORT`、保留理想私有 burn 且不发布 package/context/online lease；但没有与同次 `F_TOPK` 执行/双方启动及 API 错误投影的组合规则。在线验证拒绝、参与方主动中止和连接失败也没有共同的完整失败视图核。
- **底层部署故障：`OPEN`（若不纳入所选隐私定理，可作为可靠性边界单列）。**签名 key 读取、marker/文件 I/O、真实 `RAND_bytes`、签名/序列化/顺序发布、A token RNG、SQLite 提交/回滚、服务重启、TLS 和 timeout 的联合失败分布及其 party projection 未被目标部署资料实例化。把它们排除在定理外不等于证明失败转录无泄露。
- **首次未建模状态：**在有效公开参数和路径的条件下，最早实际运行失败可在 `score_to_mask_v3_gmw.cpp:862–870` 的签名 key 读取/解析发生，早于 `before_material()`。party 模拟器需要的是该私有事件到本次调用的包交付、party 启动/通知和可见结果的联合投影；现有接口没有定义该投影。若把 signer 可用性作为外生前提，首个持久发行状态缺口是 `:492–543` 的 registry/marker 分阶段操作及部分 burn 结果。两者都是证明/接口缺口，不是攻击或不可能性结论。
- **托管状态：`PASS_WITH_SCOPE`（源码状态映射）。**A 内部 `PENDING/COMMITTED/ABORTED` 与 party `COMMIT_STATUS_UNKNOWN` 分开。party 的 share 可在 submit 前已于本进程内形成；只有成功 authenticated fetch、receipt 校验、解密和与本地值比较后，API 才返回本方 share。API 不返回 share 不代表本地从未形成 share。
- **接口候选：`CONDITIONAL / NOT_IMPLEMENTED`。**保留待审名称 `F_TOPK-ISSUE-ESCROW/v1`，用于版本化规定同次发材 outcome、交付/启动、在线执行、A 状态、响应与恢复的联合核。不得把该提案写作已批准合同或安全通过，不扩大 `L_S29`，不向 party simulator 提供 peer secret、A context/数据库状态、T 私钥或测试控制器权限。
- **低概率中止：`OPEN_CONFIRMATION`。**没有新负责人决定。需要单独确认合法输入下 cap abort 是否可接受、是否烧毁本 invocation 后用新 invocation 重试。
- **本轮操作：**未修改运行时代码或测试；`BUILD/TEST=NOT_RUN`。未重跑 S35/S40/S52 矩阵，未运行自然 I/O 故障实验、正式 LAN/WAN、RTT、AAV86 或性能测试；性能 `NOT_MEASURED`。未修改 `VFSS-baseline/`，未创建 `secure` alias，未推送、合并或创建 PR。

## 安全声明分支

1. **成功执行分支：**仅可沿用上述理想 `SuccessFence` 模型下 `PASS_WITH_SCOPE`。若项目选择该范围，失败行为不在定理内；不得称作真实有限 timeout 安全，也不得从旧 E2E 成功推导部署成功概率。
2. **包含协议中止分支：**必须先由版本化 ideal functionality 规定 cap abort、输入/协议拒绝、对端中止、交付/启动门、A 状态和每方 API 观察的联合 law，再证明同一调用的无条件单方视图模拟。当前保持 `OPEN`。
3. **包含部署故障分支：**除上一项外，还需为 signer、marker/file system、RAND/provider、SQLite、TLS/网络与重启定义明确故障切点或经证据支持的部署概率 kernel。现有 S54–S56 局部桥、历史测试次数和本机配置不提供该 kernel 或 `p_min`。当前保持 `OPEN`。

## 负责人待确认项

1. 合法输入下，FTv3 有界拒绝采样以极低概率触发 `ISSUANCE_ABORT`、烧掉本次 invocation 并要求新 invocation 重试，项目是否接受？
2. 若 signer、marker 或文件发布等非协议错误导致发材失败，是否要求双方得到一致且不能误认为成功的失败状态，并且双方都不得启动在线计算？
3. 项目安全声明是否仅覆盖初始化与发材成功后的正常协议隐私，把机器/文件系统/provider 故障列为可靠性边界；还是要求理想功能及证明覆盖这些失败状态？

三项仍为 `OPEN_CONFIRMATION`。不重新询问已确认的正常托管/取回流程，也不改变人工 kill 的证据含义。

## 独立复核

`/root/s58_independent_review` 未参与 S50–S58 的实现或证明推导。本轮只读重新审查发材、A 状态与 party API 的联合状态边界，确认接口缺口及成功/失败范围；未修改文件、构建或测试。该复核是合同/证明边界审查，不是功能接收或完整密码学审查。
