# M6B-I-PROJECT-SELECT-5ITER S48 决策

日期：2026-10-23
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定

1. 接收对象固定为 S47 本地提交 `feb5e6677568d649906f46d0e611d5d52349c07f`，tree
   `7c494b5cfcc67843410e683556a1792074d0ca1e`，父提交
   `d48a41062ba2f86edbac3bcb3663692a641e95bd`。新建隔离 worktree 起始 clean；S47
   manifest 的 19 个 source-delta Git blob 在修改前逐项匹配，manifest 中 recorded
   raw-SHA canonical 摘要重算匹配。当前 Windows checkout 有 5 个 S47 新文档的换行
   raw-SHA 差异；Git blob 身份未变。S48 只改 TEST_ONLY 测试代码和交付文档，不改
   runtime。S47 原 worktree 最终只读状态为 clean；桌面主工作区最终检查有未提交/未跟踪历史内容，S48 未在主工作区执行写操作。`VFSS-baseline/` 无差异，S47 状态以只读检查为准。
2. S48 对 S47 的 N=1000 FTv3 候选独立复跑 13 个多进程 oracle 案例：S47 固定定义
   的 10 案由 S48 最终 E2E 二进制重新执行，其中包括 N=1000、K={1,8,500,999,1000}
   五案，以及 N=1、FTv3 N=128/256、FTv1/receipt-v1 N=128 和 FTv2/receipt-v2 N=256
   兼容 smoke；另增加 3 个固定 seed/固定结构 N=1000 输入。所有案中 Dealer、P0、P1
   退出码为 0，冻结 C++ oracle、双方 COMMITTED、各自 share 返回和 escrow 重启取回
   均通过。接收范围为 `PASS_WITH_SCOPE`，不是全 N/K 矩阵或安全结论。
3. fresh Release default-OFF 库未注册 v3/Select5 target，也未导出 v3/Select5 符号；
   未定义 opt-in 的头文件拒绝编译。显式 GMW+receipt/v2+FTv3 Release 可构建；缺失
   GMW 或 receipt/v2 的负配置按预期失败。production archive 不含 TEST_ONLY 随机源
   hook。上述构建和符号结论仅对应本轮记录的 Release 配置。
4. FTv3 的 retry cap 7、版本参数、N=1000 资源公式和原输出语义保持 S47 规格。确定性
   达到重试上限时，conformance 观察到唯一私有 burn marker、重放被拒，package/context
   与在线 escrow/lease 路径均未创建。该测试只覆盖离线 Dealer 预发布失败；A、P0、P1
   和在线数据库进程没有启动，因此不声称检查了在线 DB 表中的零行。有效输入下低概率
   `ISSUANCE_ABORT` 是否可接受仍是 `OPEN_CONFIRMATION`。
5. 透明代理的无注入正常 N=1000 路径成功：Dealer/P0/P1 均 exit 0，A 为 COMMITTED、
   一笔交易与两条 share row，双方取回各自 share；服务 exit 143 是测试在成功后发
   SIGTERM 的预期清理。最后业务 phase 结束后的 TLS unexpected EOF 单独记录为
   `post_success_teardown_eof`，不是业务 frame 失败。自然业务读写故障为
   `NOT_REPRODUCED`。
6. N=1000 功能/资源诊断未遇到本机 RAM、磁盘、package 或 payload 限制。最大观测
   payload 为 1,048,572 B；56 B Protocol-I framed header 另加在 payload 外，故最大
   协议帧为 1,048,628 B（TLS record 层开销未计）。4 B payload 空隙来自固定 batch
   gate 数及 bit packing，不是外层 56 B header。RSS、目录占用和 exchange phase 只作
   功能/资源诊断，不能写成正式性能、RTT 或论文结果。

## 继续开放的门

- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`。
- `DRBG_DEPLOYMENT_PROFILE=OPEN`、`JOINT_RNG_STREAM_BRIDGE=OPEN`；S47 的有限请求界
  及拒绝概率仅在理想 IID tape / 语义材料投影范围成立，不覆盖部署随机流或完整签名
  package/transcript。
- `PRIVATE_CHANNEL_BRIDGE=OPEN`、`PARTY_VIEW_COMPOSITION=OPEN`、
  `SHUFFLE_OUTPUT_ESCROW_COMPOSITION=OPEN`。功能复跑不是全视图安全证明。
- `N_GT_1000=NOT_ADMITTED`；`SECURE_ALIAS_READY=NOT_READY`。
- 正式 LAN/WAN、RTT、性能矩阵和 AAV86 对比均未运行/测量；保持 V3→V4 前置门。

非作者接收者 `/root/s40_independent_receiver` 未参与 S47 实现或证据生成；此前只参与
S40–S42 的只读审查。本轮独立 fresh-build 并复跑最终更新后的 bounded conformance、
FTv3 N=1000 定向案例、额外案例和无注入帧 trace。reviewer 未独立复跑 FTv1/v2 兼容
smoke；该两案由本轮执行者用最终 S48 二进制运行。执行者参与 S45/S46 既往工作，不
自称为独立接收者。

详细的命令、环境、逐案记录和哈希见 [S48 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_REPORT_2026-10-23.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_GATES_2026-10-23.json)、[cases](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_CASES_2026-10-23.json) 与 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-23.json)。仅本地提交，不推送、不合并、不创建 PR。
