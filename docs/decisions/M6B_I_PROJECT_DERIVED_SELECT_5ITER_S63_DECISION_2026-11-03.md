# M6B-I PROJECT_DERIVED_SELECT_5ITER S63 决策

日期：2026-11-03
固定基线：S62 commit `c9c87799b0f27931249d5b82edce37a5f0f1814c`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **理想模型下正常成功视图扩至 `1≤K≤N≤1000`：`PASS_WITH_SCOPE`。**在理想 FTv3 发材成功、理想输出刷新与 A token RNG、一方静态半诚实、诚实对端按因果执行、T/A 与腐化方不合谋、固定既有 `L_S29`、理想外生 `SuccessFence`，并且事件 cut 固定为本决策列明的完整源码事件边界时，FTv3 拆批可逐批使用 S50 的 H4 条件 kernel。`layer/batch/count/phase/sequence/frame length` 均为公开调用元组和既有 `L_S29` 的确定后处理，不增加泄露。非作者只读复核已通过此扩域论证。
- **证明结论不等于实际实现安全通过。**S63 扩展的是理想模型中的 Kpre/Kpost 与外生成功事件 cut；S48/S52 的 N=1000 测试仍只是有限功能证据。真实 T/A 随机源、签名/文件发布、recipient encryption、TLS/mTLS、SQLite、实际有限超时筛选和完整实际部署优势仍 `OPEN`。
- **协议级中止联合视图：`OPEN`。**固定 S62 快照内没有负责人书面决定 cap-abort 后是否以新 invocation 重试、发材失败通知/双方启动门、通知不可达时的 party 观察，或安全定理是否覆盖协议级中止。不得把 `PAIRED_GENERIC_ABORT` 当作已批准泄露；仅当负责人确认该观察属于既有 `L_S29` 后才可采用。生产 issuer 与 party API 未提供共同的启动/通知屏障；TEST_ONLY 控制器不能代表生产路由。
- **真实有限超时、部署故障和性能：继续独立开放。**SuccessFence 是外生及时成功条件，不是 wall-clock/网络保证；不启动 LAN/WAN、RTT 或 AAV86 矩阵。

## 新增证明边界

`N=1000` 时 `m=1024`、`W=42`、gate 数 `88,626,688`。层拆批为 input 1、carry 31、forward route 11、compare-generate 6、borrow 41、inverse 1，共 91 个 AND batch exchange；READY、最多 4 个 seed exchange、最多 9 个活动比较 wave 和输出刷新给出最多 106 个应用 phase。逐批 H4 归纳、事件 cut 定义、P0/P1 顺序和消息元数据映射见 [S63 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S63_REPORT_2026-11-03.md)。这不是五个算法迭代等于五轮网络通信的主张。

## 验证与状态

- 本轮只读源审查与证明；runtime/test、`VFSS-baseline/`、材料 ABI、输出、角色、`L_S29`、默认开关均未修改。
- BUILD/TEST=`NOT_RUN`；S35/S40/S48/S52 历史矩阵不重跑；正式性能=`NOT_RUN/NOT_MEASURED`。
- 非作者 `/root/s63_independent_review` 未参与 S50–S63，独立检查批次数、相位/帧映射、逐批条件核、输出 share 时点、Kpost 扩域及 S62 中止合同；只读，未编辑、构建或测试。该意见是理想模型证明范围复核，不是功能接收或实际部署安全签核。

## 未关闭门

`LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；cap-abort 到 package/context、双方 start/notice 的协议接口仍 OPEN；A 的 ABORT/UNKNOWN/fetch-recovery 联合核 OPEN；真实有限超时、T/A DRBG、签名/发布/存储、TLS/mTLS、recipient encryption、完整实际 party-view 组合 OPEN；`N>1000` 不在范围；正式性能未准入。没有创建 `secure` alias，不推送、不合并。
