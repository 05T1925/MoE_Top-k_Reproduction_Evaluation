# M6A-P2-I-E15：Protocol I 两路线正式计时合同（2026-10-03）

本合同先于 E15 TEST_ONLY 代码修改。E14 `final/` 的计数和原始记录保留；E14 的 offline/total 时间不能作为纯协议时间，也不用于新速度结论。E15 只重测 Protocol I+AAV86 全两两与同路线 EMP-ON 全对全，保持 signed Q20.12、原下标稳定 tie、原序 XOR mask、可信离线且在线静默 T、D≤128、64 MiB 包和 768 MiB 进程限制。Protocol III secure runtime 本阶段只审查设计。

| 观察量 | E15 边界 |
| --- | --- |
| `offline_time_ms` | 从控制器启动 T/P0/P1 前至 T 成功退出且双方完成 package 接收和本地协议预处理并发送 ready；连接创建/TCP 校准单列。此区间只运行协议所需生成、序列化、分发、本地 shuffle/EMP OT 与必要同步。 |
| `online_time_ms` | 双方 secure raw-score→原序 mask 入口的实测时间取较大者；输入夹具生成、TEST_ONLY oracle、结果编码和汇总不在内。 |
| `total_time_ms` | 每一正式行先加 `offline_time_ms + online_time_ms`，再取五次 median/min/max。 |
| `offline_material_total_bits` | ready 时双方实际在线持有并读取的有效载荷；T package、party 本地 shuffle 分栏；OT 发送/接收另列，不混入留存量。 |
| 在线计数 | 发送总量为双方 sent 之和；received 校验守恒。DCF 长度倍增 PRG 与 Eval、AES、边分别记；边以无序比较为单位。轮数按因果 DAG。 |

为使 ready 前不再发生整池诊断：E15 正式 party 路径按冻结材料结构的**恒定形状公式**取得有效载荷字节，不访问任何 DCF key，不调用 `serialize`，不扫描 edge pool 或 shuffle vectors。公式所需字段为 D、r、comparison_bits 和四个已生成的 OT 字节计数（固定 4 项）。DCF 规范 `serialize` 头 33 B，`k=bits+1` 个 16 B block、`g=1` 个 8 B word、`v=bits` 个 8 B word，故实际 payload `24·bits+24 B`。score key 固定 34 bit，即 840 B。生成、反序列化及 secure 使用中的形状检查保证该格式；若格式变更须更新并重新验证合同。

T package 每方：全对全 `8D + 2D(16+840) + C(D,2)(24b+24)` B；AAV86 `(48+8r)D + 2D(16+840) + r·C(D,2)(24b+24)` B。全对全本地 shuffle 每方 `D(64+152d)` B，其中 `d=2log₂D−1` 为 Benes 层数：两组 own/own-inverse，双向各两份 PO/DO 的 permutation/layers/delta/a/b/w 按实际在线留存布局计。n=2 得 T 4,280 B、本地 432 B；AAV86 r=1 得 T 4,376 B。n=128 全对全 T 8,218,112 B、本地 261,120 B。此为代码格式推导，不是以旧实验值回填。正式路径仅用 checked 整数算术；不以固定 n 的查表或 sample patch 替代形状检查。

隔离诊断测试仍可对真正生成的材料逐字段遍历及逐 key `serialize`，将其与恒定公式在 n=2、D16、n=128 的不同随机材料上逐项比对。诊断运行不进入正式 120 次统计。若任何形状或随机币产生可变长度，正式批次 NO-GO，改用精确计数设计并重新冻结。正式路径的材料计数计算在协议时间边界内仅为恒定整数运算与四个 OT 计数读取，记录这一极小观测成本；不事后扣减。TEST_ONLY 原序 mask 校验和统计在计时外。计数和轨迹收集若位于 secure 函数内则仍随协议在线计时，并如实解释，不改 secure 消息或泄露。

执行门：conformance → 冻结 oracle differential → T/P0/P1 独立进程 E2E；再 TCP/Unix 同材料、关闭/静默/截断有界失败及旧 I/III 回归。正式 E15 使用新的源码提交、实现标签和忽略目录，n=128、K=2/8、r=2..5、LAN/WAN；每配置 1 预热+5 正式，计 120/100 次。每次 fresh T 材料，同 K/rep 的输入 seed 在两路线/四 r 配对且与 T 可见 serial 分离；重新校准 TCP，逐行保存 revision、binary/source 哈希、环境、命令、种子、九指标、分方/分阶段、OT、e_t/v_t、DCF/PRG、峰值、预热与失败。n≥256 只记录 checked 容量和真实预检拒绝，时间/峰值写 `NOT_MEASURED`。E15 原始索引与仓库外完整副本独立保存；本聊天只做技术审计，不自称异会话最终接收。
