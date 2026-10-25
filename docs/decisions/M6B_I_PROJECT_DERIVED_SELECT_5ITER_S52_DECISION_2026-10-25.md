# M6B-I-PROJECT-SELECT-5ITER-S52 决策

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定

从 S51 固定提交 `1cc1e5202b3a603b55aaae985704933d70db0794` 在隔离 worktree 重建并做 fresh Release 接收。N=1000 的 `K={1,8,500,999,1000}` 五案、N=1/128/256 三个兼容 smoke，以及七个 API-v2/托管定向案均由最终 S52 二进制执行；列明样例均与冻结 oracle 一致，功能状态为 `PASS_WITH_SCOPE`，不代表全输入证明、安全证明或性能通过。

S52 发现并修复 escrow 状态分类边界：数据库状态行缺失曾被解释为 ABORT；服务端对非 COMMITTED 的其余状态也会发送 ABORT。现在缺行或状态值无效会关闭该请求并保持客户端结果未知；只有持久化的 `ABORTED` 状态才发送 ABORT。客户端仅在 TLS 服务身份/帧、响应长度、transaction ID、零 receipt 和空密文均验证后，将 submit 结果标为明确 `ESCROW_ABORTED`。不可确定的 submit 仍为 `COMMIT_STATUS_UNKNOWN`；已知 COMMITTED 后 fetch 失败仍为 `COMMITTED_FETCH_REQUIRED`，两者均不返回 share。明确 ABORT 类型的构造函数改为私有，只允许内置 escrow submit 入口构造，防止任意用户 callback 伪造该状态。

S50 的 `Kpre_3` 与 S51 的 `Kpost_3` 只在 ideal `F_T/v3`、理想 A token RNG、单方静态半诚实、诚实 peer 因果提交、`cap_success` 和外生 `SuccessFence` 下组合。它们不证明按真实有限超时筛选出来的成功转录。首个未闭合条件为：

```text
Law(Vpre_b^3 | Q_b^3, cap_success, Success_actual)
  ≈ Law(Vpre_b^3 | Q_b^3, cap_success)
```

其中 `Success_actual` 取决于客户端/服务端每帧 deadline、A 等待第二次 deposit 的事务 deadline、两方处理进度及实际调度。现有来源没有该等式的证明、部署调度分布或高效条件 sampler。此为模型/证明缺口，不是攻击或不可能性结论；不向 `L_S29` 添加时间、超时、消息活动形状或新状态。

未参与 S51/S52 实现或推导的 `/root/s51_independent_review` 从 S51 精确源码加上 S52 清单中的四个源文件，独立 fresh Release 构建并运行 7 个 API-v2 状态案及 8 个 N≤1000/兼容案；构建二进制与本轮最终测试二进制哈希相同。独立案例与逐案交付核心结果一致。该接收为 `PASS_WITH_SCOPE`，只证明列出的构建/运行结果，不扩为全域正确性、安全或性能通过。reviewer 重新推导并确认上述 timeout 条件分布缺口，但未重证 S50/S51 继承引理。

## 状态边界

- fresh Release、default-OFF 隔离、S52 定向 API 与 N≤1000 列明功能用例：`PASS_WITH_SCOPE`。
- 外生 ideal `SuccessFence` 下的 `Kpre_3 + Kpost_3` 消息视图：继承 `PASS_WITH_SCOPE`；S52 不重写 S50/S51 证明。
- 实际有限超时成功条件化视图、真实 Dealer/DRBG、A token RNG、签名、TLS/mTLS、recipient encryption 与完整 party-view composition：`OPEN`。
- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；自然业务 I/O 故障 `NOT_REPRODUCED`；N>256 安全范围 `NOT_IN_SCOPE`。
- 正式 LAN/WAN、RTT、AAV86 与性能矩阵 `NOT_RUN/NOT_MEASURED`；不创建 `secure` alias，不推送或合并。

完整证据见 [S52 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S52_REPORT_2026-10-25.md)、[逐案结果](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S52_CASES_2026-10-25.json)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S52_GATES_2026-10-25.json) 与 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S52_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-25.json)。
