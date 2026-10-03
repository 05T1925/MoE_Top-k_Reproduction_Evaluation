# M6A-P2-I-E14 统一指标来源与计量门（2026-10-03）

## 身份和证据类别

E12 正式被测源码为 `8ad0725c73716ac37c96d417136958f932233d61`，事后报告为 `c86f5bd50a938be8550d41ca99712d5859f9a1ae`。E13 文档检查点为 `2b48ef6c9ae3d4a25e0812a08a85c1d6d6b2dd56`。本记录在 E14 改代码前形成。E12 `accepted/` 与仓库外副本是冻结历史，不追加新字段。

论文定义是 Agarwal Protocol I 的比较与 shuffle 框架；signed Q20.12、原下标稳定同分、raw-score→原序 XOR mask、全两两按轮预发、可信且在线静默的 T、EMP-ON 全对全工程基线均为本项目实例。`AUTHOR_EXACT=NOT_PROVEN`。源码事实与密码学假设分别列示，不能由计数测试推出安全证明。

## 同一指标合同

| 字段 | AAV86 来源、阶段和当前状态 | EMP-ON 全对全来源、阶段和当前状态 | 单位及判定 |
| --- | --- | --- | --- |
| `offline_time_ms` | TEST_ONLY 控制器在角色启动前至 T 退出且 P0/P1 ready，MEASURED | 同边界，MEASURED | ms；TCP 预连接另列 |
| `offline_material_total_bits` | E12 仅以双方序列化 package 长度填列；须分离有效载荷与封装，现为 **NOT_MEASURED（统一定义）** | E12 缺双方离线生成的 shuffle/OT 留存状态；**NOT_MEASURED** | bits；双方在线实际持有的离线有效载荷之和，排除协议标签、容器容量、离线临时对象及在线消息 |
| `online_time_ms` | 双方 secure 入口实测较大者，MEASURED | 同边界，MEASURED | ms |
| `total_time_ms` | 每次 offline + online，DERIVED | 同左，DERIVED | ms；先逐次相加再统计 |
| `online_comm_total_bits` | 双方在线 `sent_bytes` 之和乘 8，DERIVED 自 trace | 双方 score/shuffle/cmpagg/reveal 的发送量之和乘 8，DERIVED | bits；只合计发送，接收对账 |
| `online_comm_per_party_bits` | total/2，DERIVED | 同左，DERIVED | bits |
| 每方发送/接收 | score、CA 组合、inverse 的本方 trace，MEASURED | score、forward、cmpagg、reveal、reverse 的本方计数，MEASURED | application bytes，含实现帧/控制字节；非 TCP/IP 链路字节 |
| `online_rounds` | 消息 DAG：score 2 + CA 2r+1 + inverse 1 = 2r+4，源码阶段/trace 核验 | score 2 + pipeline 6 = 8，源码阶段核验 | 因果轮；不是 send 次数 |
| `online_prg_calls_total` | E12 的 `dcf.cpp` thread-local 计数只覆盖 `evalDCF` 的长度倍增扩展；其他路径核查后才能升级为统一总数，当前 **NOT_MEASURED（统一定义）** | E12 未读取同一 DCF 计数；**NOT_MEASURED** | calls；两方运行时实际扩展之和，不以 Eval、AES 或边数代替 |
| `comparison_edges_total` | 各轮公开活跃无序边之和，MEASURED | 实际全对全无序边 `D(D−1)/2`，MEASURED/源码核验 | edges；双方求值不重复计边 |
| AAV86 `e_t`,`v_t` | 逐轮活跃图；`v_t` 为该轮边端点去重，MEASURED | 不适用 | count；跨轮可重复节点，最终逐轮求和 |
| AAV86 DCF/uCMP | score 与 CA 各自 Eval 和 uCMP 路径，MEASURED | score + cmpagg DCF Eval，MEASURED | calls；不冒充 PRG |
| 预留槽位/消费边 | `r·C(D,2)` 与逐轮实际活跃边，分别 DERIVED/MEASURED | `C(D,2)` 与全对全消费，分别 DERIVED/MEASURED | pairs per party |

## 离线材料对象账本及拟采用定义

统一主字段只数 **ready 屏障时双方为本次在线执行实际持有、会被在线路径读取的密码/路由有效载荷**，按固定宽度的规范编码计数；不数 C++ 对象大小、vector capacity、传输帧头及离线已销毁的 EMP OT 种子/中间值。每个实际保留字段按实际 vector 元素遍历，重复保留的字段如在线分别读取则分别计数。非秘密的路由/置换字段仍属于在线预处理材料，但与密钥子项分栏。T package 的序列化/分发字节另记；EMP OT 离线 P0/P1 发送量另记，不能与材料留存量相加。

- AAV86：T package 内的四组 forward/inverse 置换、四组 mask/translation words、`rD` 节点 mask shares、score 两阶段 mask shares 与 DCF key payload、`r·C(D,2)` 个 DCF key payload。public pivot seed 属公开控制输入，另列；package header、pair 标签、长度字段、key header 不入有效载荷。
- 全对全：T package 内的 `D` 节点 mask shares、`C(D,2)` 个 DCF key payload、两阶段 score mask shares 与 DCF key payload；双方独立留存的 shuffle material 包含 own/own-inverse permutations、各 PO 的 permutation、Benes layer permutation 和 `delta`，各 DO 的 `a,b,w`。`local_permutations` 仅用于离线 OPV，不在在线路径读取；离线 EMP OT 中间块不计为在线留存。应逐字段报告 PO/DO 份额、T package 份额及 OT 双方发送/接收，并对 n=2 小实例手算。
- DCF key payload 是实际 `ProtocolIUcmpPartyMaterial::serialize()` 中的 `k/g/v` 字节；现格式有固定 33-byte 结构头，计量只取实际序列化长度减该头。此为逻辑有效载荷长度，不等于对象 RSS 或含元数据的文件长度。

## 在线 PRG 路径核验门

`VFSS/ext/FSS/dcf.cpp::traverseOneDCF` 在每次 seed→双子 seed 扩展时递增 thread-local 计数。`evalDCF` 从 score 的 carry/sign 和 CA/全对全 cmpagg 进入该点；同一线程内重置一次、逐阶段读取差值，跨 P0/P1 由控制器求和。`protocol_i_permute_share_online_*` 是已预处理的向量加减与路由，未见在线 OPV/OT 调用；`protocol_i_opv_emp.cpp` 的 SHA256 子种子树和 `protocol_i_chosen_ot_emp.cpp` 的 EMP OT 在 ready 前的预处理发生。AAV86 `choose_pivots` 使用公开 seed 的 `osuCrypto::PRNG`，属于公开控制随机流，应单列调用/字节，不能混称 DCF 的 seed→双子 seed 扩展。仍须核实其他在线路径后，才能给统一字段 `MEASURED`。

## 正式数据门

先使有效载荷计数、OT 通信和两条入口的 DCF 扩展计数在小实例可人工复算，并通过 conformance→冻结 oracle differential→独立进程 E2E 与字段守恒。门未闭合时新 `n=128` LAN/WAN 正式批次为 **NO-GO**；E12 已接收时间/在线通信保持历史有效，但不拼接成 E14 完整指标。若门通过，使用新实现标签、独立忽略目录和与 T 可见运行标签独立的输入种子及 OS fresh shares，重新测相同输入的两条路线。
