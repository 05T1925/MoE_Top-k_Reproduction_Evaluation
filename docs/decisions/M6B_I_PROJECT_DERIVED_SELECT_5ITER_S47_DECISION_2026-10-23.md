# M6B-I-PROJECT-SELECT-5ITER S47 决策

日期：2026-10-23
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定

1. 固定从 S46 commit `d48a41062ba2f86edbac3bcb3663692a641e95bd`、tree
   `3f70ae5999400781f37f871420a26988afa146c9`、父提交
   `6fd6bed0718be28d677892ca40125d0f52323f0e` 接收。隔离 worktree 初始
   clean；没有用主工作区或其他快照替代。S46 source/build/evidence manifest
   原件保留，S46 报告、decision、gates 和 cases 的 Git blob 身份匹配其记录；
   Windows checkout 的报告 raw SHA 与原登记值不同，但 `git hash-object --path`
   与固定 blob 一致。`VFSS-baseline/` 未改。
2. 接受一个仅通过 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL`
   显式 opt-in 的 F_T/v3 实验候选。必须同时开启 GMW/v3 和 receipt/v2；默认
   OFF；FTv1/FTv2 不扩大到 N>256、不回退到 FTv3。F_T 版本、sampler 版本、
   package format 和 escrow `receipt_format_version` 分开绑定。算法输入、
   signed Q20.12 排序、稳定同分、五迭代 Select 及原序恰 K 位 mask 输出不变。
3. 固定 F_T/v3 profile 为 `(ft=3, sampler=2, lambda=128, retry_cap=7,
   package_format=6)`，context format 2 / receipt format 2。数学最小次数
   `r_min(lambda,m)` 与签名 profile 的固定 cap 7 分开：任一 `m<=1024` 的
   `r_min<=7`；`m=1` sampler 不取随机字，但 package/context 仍绑定 cap 7。
   Fisher–Yates 单步拒绝率 `rho_i<=1023/2^32<2^-22`，最多 1023 个位置，
   `delta_reject<=1023*2^-154<2^-144<2^-128`。该界只适用于去除版本、编码、
   签名元数据后的语义材料投影，不适用于完整签名包或协议 transcript。
4. 资源预检没有遇到本机内存、磁盘、地址空间、包长、gate 或 frame 上限。
   fresh Release FTv3 多进程路径以 N=1000 和 K={1,8,500,999,1000} 完成
   oracle 验证；N=1 profile smoke、FTv3 N=128/256 smoke、FTv1 N=128 和
   FTv2 N=256 兼容 smoke 也通过。结论仅为 `PASS_WITH_SCOPE`，不重写 S46
   的 N=128/256 收据，不构成所有 K、所有 N<=1000 的完整矩阵接收。
5. 非作者 `/root/s45_non_author_review` 未参与 S47 实现或推导，最终只读复核
   retry-cap 规格、reject 界、profile 写入/校验、版本空间、N=1 覆盖、测试门控
   和报告解释；结论 `PASS_WITH_SCOPE`。复核者没有运行 build/test。构建和测试由
   本轮执行者完成；执行者参与 S45/S46 前序实现/审查，不宣称为独立接收者。
   受控发材耗尽测试直接断言 package/context 缺失和私有 burn marker 留存；因未发布
   package，party/READY/lease 流程没有触发，本轮没有单独查询 escrow lease 数据库。
   报告不把该点扩写成进程级 lease 测试。

## 保持开放的门

- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`：当有界采样
  耗尽时实现会在 party package/context/lease/READY 发布前统一中止，T 私有
  burn marker 留存；合法输入下该低概率离线预处理中止尚无负责人产品合同接受。
- `DRBG_DEPLOYMENT_PROFILE=OPEN`、`JOINT_RNG_STREAM_BRIDGE=OPEN`：有界调用量
  不证明真实 OpenSSL provider、熵源、生命周期或整个有序随机流的联合计算安全。
- `PRIVATE_CHANNEL_BRIDGE=OPEN`、`PARTY_VIEW_COMPOSITION=OPEN`：功能验证不关闭
  TLS/认证通道或完整 GMW/Beaver 单方视图证明。
- `SECURE_ALIAS_READY=NOT_READY`、`PERFORMANCE_READY=NOT_READY`。不创建
  `secure` alias，不启动正式 LAN/WAN、RTT、性能或 AAV86 对比。
- 本轮只接收所测 N=1000 和少量兼容点；N>1000 继续 `NOT_ADMITTED`。自然业务
  I/O 故障沿用 S40 `NOT_REPRODUCED`，本轮未重新尝试。

实现、测试命令、资源计算、失败尝试和哈希见 [S47 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S47_REPORT_2026-10-22.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S47_GATES_2026-10-22.json)、[cases](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S47_CASES_2026-10-22.json) 和 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S47_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-22.json)。仅创建本地提交，不推送、不合并。
