# M6B-I-PROJECT-SELECT-5ITER S54 决策

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **S53 固定来源：`PASS_WITH_SCOPE`。**本轮从 S53 commit `dde7137c8fd185d5725d88980688caacde16da5b`（tree `589f12d813576ba0ba890d2ad6dbfbaa8792fa26`，parent `578ecadfe807748a3e2fada54ca11efe1b00d28c`）开始。S53 manifest 的 4 个交付文件 Git blob 均匹配，记录的 canonical digest 重算一致。三个 Markdown/JSON checkout raw SHA 因 `core.autocrlf=true` 与 S53 登记值不同，但 Git blob 身份一致；未从主工作区替换源码。
- **FTv3 理想 IID 语义材料映射：`PASS_WITH_SCOPE`。**固定源码的数据流把 IID 随机带映射为合法 Beaver triple shares、均匀 Fisher–Yates 置换和独立 one-hot permutation shares。条件于 bounded sampler 的全部位置成功，语义材料仍精确服从该理想联合律；`δ_reject ≤ 1023·2^-154 < 2^-144` 仅用于剥除版本、编码、签名元数据后的语义材料投影。
- **源码随机请求上界：`PASS_WITH_SCOPE`。**N=1000、padded `m=1024`：`G=88,626,688`；单次发材最多 `Qmax=88,786,404` 字节、`Cmax=7,247` 次显式 `RAND_bytes` 调用、每次请求不超过 1 MiB。该上界不涵盖同进程其他调用、其他 invocation、TLS、A token 或 party RNG。
- **真实随机流替换：`CONDITIONAL`。**若实际目标部署在固定 `(P,E_T)` 下，对隐藏初始/重播种熵求平均，并满足包含自适应请求边界、长度和返回状态的整段调用级假设 `Adv_DRBG_T(Qmax,Cmax;E_T)`，则替换完整发材随机流后，issuer 输出及其确定性后处理的计算优势至多该项。该项是待部署方接受/实例化的假设，不是本机或源码已证明的结果。
- **成功条件化：理想 cap-success `PASS_WITH_SCOPE`；真实 `RAND_bytes` 成功条件 `OPEN`。**对 `cap_success`，IID 拒绝指标独立于 Fisher–Yates 接受值、前置 triple 字节和 permutation-share 字节。对真实整次发材成功，必须使用带错误状态的联合随机流游戏；若以联合计算优势 `ε` 比较 `(S_RNG,view)`，其中 `S_RNG` 仅表示所有 RAND 调用成功且 sampler 未 cap-abort，理想成功概率 `p_I≥1−δ_reject`，则 `p_R≥p_I−ε`，条件视图优势为 `2ε/(p_I−ε)`，从而不超过 `2ε/(1−δ_reject−ε)`（需 `ε<1−δ_reject`）。这不条件于签名和文件均发布成功；完整包发布条件还需 `Pr_I[S_issue]≥p_min>ε` 的成功概率下界。当前没有真实 provider/熵源/生命周期资料，无法代入或接受 `ε`，也没有实际 `RAND_bytes` 失败率资料。
- **T 私有 burn marker：`PASS_WITH_SCOPE`（同一未回滚 registry 条件下）。**源码在分配材料前创建并 fsync 以 `(session, invocation)` 为键的私有 marker；同一目录同一 FTv3 profile 的重放被拒。此项不是跨目录、跨版本、备份回滚或多文件原子性保证。采样中止时 marker 保留、材料向量可能已在内存形成且没有显式清零保证。
- **SuccessFence 与真实 timeout 分开。**S53 的理想 SuccessFence 成功应用消息范围仅作为继承项 `PASS_WITH_SCOPE`；真实有限超时筛选视图仍 `OPEN`。S54 不增加 `L_S29`，不声称完整 party-view、安全通过或实际性能。

## 未关闭事项

`DRBG_DEPLOYMENT_PROFILE=OPEN/NOT_AVAILABLE`、`JOINT_RNG_STREAM_BRIDGE=CONDITIONAL`、真实 `RAND_bytes` 成功条件、签名/哈希绑定优势、A token RNG、TLS/mTLS、recipient encryption 与完整 GMW/Select/shuffle/output/escrow party-view 均未关闭。`LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；自然业务 I/O 故障仍为 `NOT_REPRODUCED`；正式 LAN/WAN、RTT、AAV86 与性能 `NOT_RUN/NOT_MEASURED`。

执行者此前参与 S45–S53 的相关实现、证明或交付，不将本轮自审称为独立复核。S54 非作者只读理论/源码复核为 `PASS_WITH_SCOPE`，确认条件界、Q/C 账本、拒绝界、后处理和文件发布范围；reviewer 未构建或测试。该复核不是功能接收或完整密码学证明。无可复现运行时缺陷；未改 runtime/tests；BUILD/TEST=`NOT_RUN`。仅创建本地隔离交付，不推送、不合并、不创建 `secure` alias。
