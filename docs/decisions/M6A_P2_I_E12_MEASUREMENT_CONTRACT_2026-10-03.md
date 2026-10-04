# M6A-P2-I-E12：n=128 比较的计量边界

日期：2026-10-03。此决策只约束隔离 TEST_ONLY benchmark harness 的计量，
不改变 Protocol I+AAV86、旧全对全 Protocol I 的安全消息或密钥格式。
方案仍为可信、非合谋、在线静默 T 的每轮全两两预发；固定 M 不采用。

## 时间

- `transport_setup_ms`：本机测试 harness 建立所需 fd/TCP 连接的包围时间，
  在角色启动之前结束；不进入 `offline_time_ms` 或 `total_time_ms`。
- `offline_time_ms`：建立连接后、首次 fork P0/P1/T **之前**起表；
  T 发材并退出、P0/P1 完成材料接收及 ready 屏障后止表。
  T 生成、序列化、分发和从 T 退出到双方 ready 的屏障时间分别计量。
  子区间未覆盖角色启动与并行的 party 预处理，不能简单相加代替包围时间。
- `online_p0_ms`、`online_p1_ms`：各方在接收测试输入后调用 secure
  入口直至返回的时间。`online_time_ms` 为较大值，不相加；
  测试输入发送、oracle、结果收集均不计入。
- `total_time_ms = offline_time_ms + online_time_ms`，只在这两项均实测时派生。
  本地 TCP 建连另列，不藏入总时间。
- AAV86 `combination_p{0,1}_ms` 是 score 之后至 inverse 之前的
  CA 加本地输出 carrier 阶段。相邻时间戳单独测 `ca_p{0,1}_ms`
  （首轮 shuffle、逐轮 CA 和公开排序 flatten）与
  `carrier_p{0,1}_ms`（由公开排序构造共享输出载体），二者和
  等于组合阶段。旧 E11 `core_p{0,1}_ms` 指组合阶段，不能改名
  作为作者论文纯 CA 核心的历史测量。
- 旧全对全基线的 score 与完整 pipeline 分段分别实测；旧入口尚无
  与 AAV86 CA 同语义的纯论文核心时间，记 `NOT_MEASURED`。

## 工作量与通信

每个 party 直接从本方 framed channel 的 message trace 聚合
score、组合阶段、inverse 的**实际接收**与发送量；同方阶段和
等于总量，并与对方同阶段发送量交叉对账。旧全对全基线记录
score、forward、cmpagg、reveal、reverse 的各方实际收发量。
`online_comm_total_bits = 8·(P0 发送字节 + P1 发送字节)`；
接收用于校验，不再加进总通信。qdisc 字节另作网络作用证据。

旧全对全的 EMP OT 预处理在 P0/P1 离线阶段执行，T 为 score 与
所有比较边生成 fresh package，仍无在线 T。其每方 package 字节可测，
party 本地 OT 材料的完整大小目前 `NOT_MEASURED`；因此基线完整
离线材料位数不得由 package 字节冒充。基线 DCF/PRG 长度倍增计数
尚无同一可信观测点，写 `NOT_MEASURED`。两路径的完整时间、
应用层在线通信和因果轮数可同口径并列；不同 shuffle/离线角色
应在解释时明确。

基线 TEST_ONLY 控制器没有把 `input_seed` 直接传给 T；T 的 exec 参数
使用另一运行标签与公开形状，不能直接复用输入 seed 作为材料标签。
但 E12 的 `input_seed` 与该标签都由公开的 K/重复编号日程确定，不能
据此主张 T 无法推知这批合成输入。AAV86 TEST_ONLY 控制器还以可由
serial 重现的 PRNG 生成一方分数份额；该夹具不证明输入份额在角色
视图中的隐私。上述限制针对测试输入生成，不改变 secure 入口以任意
外部输入份额运行时的条件性安全前提，亦不改变 E12 计时和通信原值。
历史 `d62f437` 基线 LAN 试跑在此隔离修正前发生，虽功能通过，
仍列为无效试跑并保留原始文件；正式对照须在修正后的新提交重跑。

E11 原始数据保持其旧起点和推导接收口径，不与 E12 同名新记录
直接合并；E12 重测有独立 HEAD、二进制、原始 JSONL、校准和哈希。
安全声明仍依赖既有 DCF/FSS、PRG、可信 T、半诚实方和完整通道
假设；`AUTHOR_EXACT=NOT_PROVEN`。
