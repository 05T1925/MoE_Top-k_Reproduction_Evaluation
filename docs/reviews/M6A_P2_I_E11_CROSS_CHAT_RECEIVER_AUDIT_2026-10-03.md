# M6A-P2-I-E11 异会话接收复核

日期：2026-10-03。接收对象为 `codex/m6a-p2-i-e11-benchmark` 中的正式被测提交 `103b76d863e68c2c318998db01073591e6a8fce8`、报告提交 `46009b6b8e10633d114fb00f899879f545c49f48`，以及未提交的 E11 原始记录。本接收会话未参与 E11 实现或原 96 次运行。

## 结论

**E11_RECEIVER_ACCEPTANCE = PASS_WITH_METRIC_FINDINGS。** n=128、K=2/8、r=2..5 的 16 个同主机模拟 LAN/WAN 单元具有可核对的 1 次预热加 5 次正式运行记录；80/80 次正式运行的原序 mask 正确性、阶段消息、边和 PRG 计数在原始记录中一致。TCP 网络整形有 qdisc 流量证据。本次独立构建与代表性 WAN TCP 复跑也通过。

此结论接收 **当前定义下的在线计时与全两两候选功能**，但 E11 的 `offline_time_ms`、`total_time_ms` 和逐阶段接收字节仍有下述计量口径问题；在修正并重新测量之前，不能用这些字段作完整路径效率结论或同路线数值比较。完整 V3、作者精确复现及无条件密码学安全均未完成。

## 独立核验

| 范围 | 结果 |
| --- | --- |
| 源码身份 | 两个本地提交存在；正式原始行均标识 `103b76d...`，被测二进制 SHA-256 `07169745C6306F02EA1CC41D8280109B12069C71AF8AB3BA460125B338D4FF40` 与本地 `/tmp/m6a11-release` 文件一致。最终提交相对正式提交只增改测试、审计和文档，没有改安全协议实现、正式 runner 或已测二进制源码。`git diff ac8af47a..46009b6b --check` 通过。源码工作树哈希与报告一致；本 checkout 启用 `core.autocrlf=true`，Git blob 的 LF 哈希与工作树字节哈希本来不同。 |
| 原始记录 | LAN/WAN 各 48 行，分别 8 次预热和 40 次正式运行；两个 JSONL、两个汇总、校准文件及 32 行容量状态表的 SHA-256 与 E11 报告一致。独立程序复算 96/96 行的参数、槽位和包长、逐轮边/节点/PRG、DCF Eval、通信收支、轮数、时间代数、原始日志哈希与校准引用，错误数 0。16/16 组的五次 median/min/max 与汇总文件一致；LAN/WAN 对应运行的输入摘要及公开 pivot 种子匹配。 |
| 网络 | 校准文件为 LAN RTT 中位 1.2299495 ms、应用吞吐 800.2303 Mbit/s；WAN 为 50.427663 ms、63.2379 Mbit/s。校准后至协议结束的 qdisc 增量分别均为 1,825,026 B、7,393 包。独立在新 namespace 内按 25 ms/100 Mbit/s 运行一例 n128/K2/r3 TCP：oracle 和消息检查通过，在线 524.416 ms，qdisc 从 0 增至 34,672 B、136 包；namespace 退出已清理。该抽样不替代原 96 次正式批次，也不是物理双机 WAN。 |
| 新构建 | Ubuntu 24.04 WSL2、GCC 13.3、Release、EMP OT 关闭，在新 `/tmp/m6a11-receiver-20261003` 目录独立配置并构建五个相关目标。D128 conformance、冻结 oracle differential、三进程 E2E 各 6/6 PASS；同材料 Unix/TCP 等价、frame 等价和 TCP 关闭/静默/截断负面用例通过。另有一例未整形 n128 TCP 通过。未在本次新构建上复跑全部 27 项 CTest、全量 `all` 构建或整套 96 次网络批次。 |

## 必须纠正的计量口径

1. **离线计时起点晚于 T 的 `fork`。** `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp::run_case` 先创建 TCP 连接、启动 P0/P1，再执行 `fork` 启动 T，关闭 parent 不需的描述符之后才设置 `offline_start`。T 可在该时间点之前完成部分启动甚至生成工作。因此当前 `offline_elapsed_ns` 不能声称完整覆盖 T 启动及全部预处理；`total_time_ms=offline+online` 也继承这一缺口。E11 文档中的“含启动”需收窄或移动计时起点，并用新 revision、新原始批次重新测离线和总时间。连接建立时间若继续排除，也要单独写清。旧数据原样保留。
2. **逐阶段接收量由对方发送量推得。** E11 E2E 输出把 `p0_score/core/inverse_received_bytes` 直接填为 P1 对应阶段的发送数，P1 亦然。每方总接收量确由 framed channel 实测，且原始记录的总收发对账成立；但逐阶段接收列不是本方 channel/trace 的独立实测。应从本方实际 `message_trace.received_bytes` 按 phase 汇总，分别验证与对方发送相等，再更新计量契约和原始批次。
3. **`core_time_ns` 包含载体构造。** 计时从 score 返回持续到 inverse 开始，涵盖 AAV86/CA 及公开排名后的本地 carrier 准备。它可以作为本项目的组合核心阶段，但不可直接标作论文纯 CA 核心时间或与另一条路线的不同边界核心相除。完整在线计时不受此命名问题影响；报告和未来基线对照应明确边界，必要时拆出 carrier 适配时间。

## 仍未关闭的范围

- 同路线全对全 Protocol I 的相同 Release、fresh 材料、TCP 网络与完整指标边界尚未建立，当前 `NOT_MEASURED` 正确；不能用 E11 与历史 Debug/core 数值算加速比。
- n≥256 的 24 个 V3 点有 checked 容量和实际预检拒绝记录，没有 keygen 或运行。当前首先触发 `D>128` 硬门；n256/r2 的每方解析包 69,999,474 B 本身也超过 64 MiB。方案仍为全两两，固定 M 未采用。
- E11 报告记录全量 Release `all` 在 `bitpack_test` 链接失败；本接收只确认所需协议目标的新构建成功，未把全量构建写为通过。条件性整池安全归约、允许泄露口径与 `AUTHOR_EXACT=NOT_PROVEN` 保持原样。

本轮只新增本报告；未改动 `VFSS/`、`VFSS-baseline/`、原 E11 报告、正式脚本、原始记录、论文、参考树或主工作区。未暂存、提交、推送、合并或创建 PR。
