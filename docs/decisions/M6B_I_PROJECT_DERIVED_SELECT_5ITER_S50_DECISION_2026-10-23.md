# M6B-I-PROJECT-SELECT-5ITER-S50 决策

日期：2026-10-23
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定与状态

从 S49 固定源码及理想 F_T/v3 semantic-material joint law 出发，为 N≤256 构造 `Kpre_3`：以 S36 H4 的 package-conditioned Beaver/比较条件 sampler 为核心，利用 S49 两方语义材料联合律，并用确定性映射生成 FTv3 新 wire 格式。

在既有 S36/S42 `L_S29` 摘要下，新增 layer/batch/count、phase/sequence/frame length 是公开调用元组和已批准 active seed/outcome/endpoint/comparison-count/communication-length 的确定函数。最大 padded `m=256` route layer 为 2,621,440 gates，小于 4,193,824 cap；N≤256 内没有数据相关拆批。未扩大 `L_S29` 或 `L_A`。

非作者 reviewer 复核修订后的共同语义参数 `P_sem` 条件 law 后，确认本方 peer-material 条件核、P0/P1 sampler 和新增字段映射成立，且无前缀反例。因此 `KPRE_3_N_LE_256=PASS_WITH_SCOPE`、新增 schedule 字段映射=`PASS_WITH_SCOPE`：仅限理想 IID F_T/v3、`cap_success`、新鲜单次 package、一方静态半诚实、无故障成功 GMW transcript，范围 N≤256，止于本方 output share 形成。Reviewer 初审指出过 FTv1/FTv3 profile 被同写入 `P` 的记号问题；报告已改为版本中立的 `P_sem` 比较材料律，并以 `P_3` 编码 FTv3 消息，复核者确认修正有效。S42 escrow suffix 的 FTv3 迁移未重证，完整 party-view 仍 OPEN。

## 不变项和禁止外推

- 输入、稳定排序、原序 Top-K mask、角色、托管合同、`L_S29` 与 `L_A` 不变。
- 只引用 S49 `δ_reject≤1023·2^-154<2^-144` 在 semantic-material projection 的范围；不适用于 full package/transcript，也不当作实际部署失败概率。
- DRBG/party RNG、签名材料发行、TLS、escrow suffix、A-only corruption、crash-prefix、物理掉电、N>256 安全扩域和正式性能门继续 OPEN/NOT_IN_SCOPE。
- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`，无新负责人记录；不再询问已确认的 escrow/输出合同。
- 本轮仅证明与治理文档变更。`BUILD/TEST=NOT_RUN`，正式 LAN/WAN/RTT/AAV86/performance `NOT_RUN/NOT_MEASURED`，自然业务 I/O 故障沿用 `NOT_REPRODUCED`。
- 非作者密码学 reviewer `/root/s40_independent_receiver` 未参与 S47–S50 实现或推导，复核范围含条件材料 law、P0/P1、字段到 `L_S29` 映射、S49 δ边界及范围措辞；首轮记号意见已修订并获复核确认。未构建/测试，也未改文件。
- 未发现可复现 runtime 缺陷；不改运行时代码/测试，不修改 `VFSS-baseline/`，不建 `secure` alias，不推送/合并。

字段映射、模拟器权限、review 状态与精确来源见 S50 report、gates 与 source/evidence manifest。
