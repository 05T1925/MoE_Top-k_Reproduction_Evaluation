# M6A-P2-I-E21：全对全流式基线的材料边界与配对矩阵

日期：2026-10-05。起点为 E20 交付 `8986a40175f5f8eef56d765fef0cfa863e205985`，E20 独立接收为 `PASS_WITH_EXPLICIT_LIMITS`。这是本项目的实例设计，不是 Agarwal 论文定义或作者精确复现声明。旧 `E20_CLIQUE_SEALED_V1`、`E20_STREAM_AEAD_V1`、E17 内存池以及 E18 draft PR #28 保持各自标签、revision 和原始数值。

## E20 基线逐字段账本

E20 基线调用 AAV86 `protocol_i_aav86_stream_dealer_send`（`r=1`）；Party 在 `protocol_i_e20_baseline_stream_bench_test.cpp::party_main` 只将 `score_materials` 与 `node_mask_shares` 移入全对全 package，边 key 从封存文件逐边读取。独立的 `protocol_i_shuffle_preprocess_party` 执行真实 EMP OT，消费自己的 shuffle 材料。以下字节是每方 D 个元素的规范 payload，不是对象 RSS、网络封装或文件大小：

| T 生成/交付字段 | 每方 payload | 生成时机 | 基线在线消费 | E20 ready 主字段 |
| --- | ---: | --- | --- | --- |
| `forward_sigma`, `forward_tau`, `inverse_sigma`, `inverse_tau` | `4·D·4 = 16D` B | T 离线、全池边 key 前 | 无 | 是 |
| `forward_a`, `forward_e`, `inverse_a`, `inverse_e` | `4·D·8 = 32D` B | T 离线、全池边 key 前 | 无 | 是 |
| `pivot_seed_lo/hi` | 16 B 序列化标签/公开控制状态 | T 离线、全池边 key 前 | 无 | 否，E14/E15 有效载荷定义排除公开控制 seed |
| `node_mask_shares` | `8D` B | T 离线、全池边 key 时 | 全对全 CmpAgg | 是 |
| 两阶段 score mask shares 与 DCF key | `2D·(16+840)` B | T 离线、全池边 key 前 | raw-score adapter | 是 |
| `C(D,2)` 个 uCMP key | `C(D,2)·(24b+24)` B | T 离线逐边交付 | 全对全 CmpAgg | 是 |
| 双方各自的真实 EMP shuffle 状态 | `D·(64+152(2log₂D−1))` B | Party 离线、ready 前 | forward/reverse shuffle | 是，单独分栏 |

T 还临时生成完整 `pi/pi_inverse`、`h/inverse_h` 等 AAV86 中间值以构造上述候选状态；它们没有进入 ready 主字段，但其生成耗时留在 E20 原离线时间。逐字段耗时没有在 E20 原始计时中分表实测，标 `NOT_MEASURED`，不从旧时间扣估计值。D=1024 时未用的四置换/四向量共 **49,152 B/方**、98,304 B/双方；E20 全对全基线每方 T ready payload 554,917,888 B，实际必需的规范 payload 为 554,868,736 B/方。E20 原数值不改。

## E21 目标与格式

新标签 `E21_CLIQUE_MINIMAL_SEALED_V1`。可信非合谋 T 只生成节点 mask shares、两阶段 score 材料和全部 `C(D,2)` 个 uCMP key；不采样、计算、发送或持有 AAV86 permutations、translation vectors、pivot seeds。每方经独立私有完整 T→P 通道流式接收全池 key，以 E20 同一 AES-256-GCM、随机本方盘密钥、固定 slot、AAD、原子发布和持久整份领取机制封存。文件头使用独立格式版本，base 使用独立 magic/version 并只编码必需字段，防止 E20/E21 交叉读取。边密文尺寸不变；base 离线线字节及 ready 有效载荷另记。基线 Party 在 ready 前执行真实 EMP shuffle 预处理；在线消费全对全每一边，T 静默，保留完整 signed Q20.12 raw-score→原序 XOR mask 及 8 因果轮。

新 T ready payload 为 `8D + 2D(16+840) + C(D,2)(24b+24)` B/方；本地 EMP shuffle 按 E15 固定账本另加。任何实际格式与此恒定形状不一致，则正式批次 NO-GO。离线 T→每方流字节包含文件头、逐边序列化 key、base 长度和 base 编码，单列并与 Party 实收相核；文件长度与 ready payload 不混算。

## 验证及正式矩阵门

顺序为：新格式/材料 conformance（含错 party/session/slot/端点、密文交换、截断/篡改、重复领取、T 静默/关闭、peer 失败）→冻结 oracle differential（负分、同分原下标、padding、K=1/n 及中间 K）→独立 T/P0/P1 完整入口 E2E。其后用同一冻结源码和各路线冻结二进制，n=128/256、K=2/8、AAV86 r2–5、LAN/WAN 做 1 预热+5 正式；每次 fresh key/claim，路线同 n/K/profile/rep 输入 seed 与 digest 匹配。新基线与 AAV86 的 n1000/K80/r2–5 同规模 LAN/WAN 也完整重测，不能沿用 E20 旧基线或与 E20 不同日期中位拼接。

沿用 E15 起止钟，离线包含完整 keygen、私有交付、AEAD 封存、EMP OT、ready；在线包含领取、读取/认证/解码及原序 mask。total 逐 run 相加再统计。九指标、每方收发、逐轮 e/v、实际 DCF PRG、材料/文件/OT、环境、原始命令/种子/哈希、RSS、宿主物理可用量与分页窗口分别留存。失败批次整体排除正式中位并完整保留。n≥10⁴ 沿用 E20 资源判定 `RESOURCE_INFEASIBLE/PRECHECK_REJECTED`，九指标 `NOT_MEASURED`，不执行全池 keygen。安全假设、公开活跃图、可信宿主条件与 `AUTHOR_EXACT=NOT_PROVEN` 不因新基线改变。
