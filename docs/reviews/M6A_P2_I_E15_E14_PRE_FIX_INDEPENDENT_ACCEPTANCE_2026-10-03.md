# M6A-P2-I-E15：E14 修正前独立接收（2026-10-03）

## 身份与隔离

本聊天没有参与 E14 编写或运行。独立工作树从报告提交 `b521c5e33c4fef1a37f33a63586bb6a201520c5a` 建立在 `codex/m6a-p2-i-e15`；正式被测源码是 `b0e474cfbbcae433405ac61c5240068878ba43ca`。本地 `main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`，亦为候选与 main 的 merge-base。E14 原工作树启动时 tracked 状态清洁；`b0e474c..b521c5e` 仅修改文档和审计/导出脚本，无 VFSS 源码差异。本次接收在任何 E15 源码修正前保存。

证据分层：Agarwal Protocol I/AAV86 的比较图与 shuffle 属论文定义；signed Q20.12、稳定原下标 tie、raw-score 到原序 XOR mask、全两两预留及 EMP-ON 全对全是项目扩展；下述调用与计量边界是 `b0e474c` 源码事实；半诚实方、可信不合谋且在线静默 T、DCF/FSS/PRG/AES 安全及私有完整通道为条件性假设。`AUTHOR_EXACT=NOT_PROVEN`。

## 冻结数据独立核验

只读核对 E14 原树 `experiments/m6a_p2_i_allpairs/TEST_ONLY_E14_RAW/final/` 与仓库外 `C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation_Evidence/M6A/E14_final_complete_b0e474c/`：`sha256sum -c e14_raw_complete_index.sha256` 为 151/151；目录 `diff -qr` 无差异，共 152 文件。索引 SHA-256 为 `7363D64EA6794C49DA563D39FB02DED528E2FABB3C211BC295EFC092037ED796`。早期 `accepted/` 作废批次未进入接收。

独立运行 E14 审计脚本并审读检查项：四份 JSONL 为 AAV86 LAN/WAN 各 48 行、全对全 LAN/WAN 各 12 行，总 120 次，其中 100 次正式；20 组各 5 次正式、24 个配对组。审计检查逐行 `head=b0e474c`、`tracked_state=CLEAN`、源文件和二进制 SHA-256、原始日志哈希、输入 seed 与计划、原序恰 K 位 oracle、分方发送/接收与阶段守恒、offline+online 逐次总时间、统计 median/min/max。该审计输出 `status=PASS`。二进制本机现存副本 SHA-256：AAV86 `E0B8807B1AD9AA47BFAE6272AD2AA02BE375027483736590CC19CB4E9696E3C3`，全对全 `CA0F355DE77705EEE1F004B427872D1A6034ABE7D44CB011AFBF0A475C3CA0B2`，与报告及逐次记录相符。关键源码哈希由审计逐行比对；报告给定的 AAV86 `1A4A3C2BD9FA460CF6FE336068B9B5C353137A556F5E81011E6EC7270F2856D1`、全对全 `A114DA38336079D4E8E4B2CF7085BF51BCAAF11511FF44AA90D6770B87F08FE9` 与共享材料头 `8CF07DD88F3448AFAE39AF889951C820C82A045B938E1494DBEFEBD3546AE1CA` 由该源码检查覆盖。

容量表 32 配置行中，n=128 八点有双网络正式运行；n≥256 的 24 点为 `HARD_CAP_D_GT_128` / `PRECHECK_REJECTED_NO_KEYGEN`，没有性能测量。n=256,r=2 单方解析包 69,999,474 B 超 64 MiB；768 MiB 进程约束未解除。校准文件随索引保存。同主机模拟 LAN/WAN 不等于物理双机网络。

## 代码级计量发现

两条 TEST_ONLY party 入口在接收 T package、完成本地预处理后，**ready 发送之前**调用 `test_only::payload(...)`。该函数遍历所有 score/edge DCF keys；每个 `key_payload_bytes` 调用 `ProtocolIUcmpPartyMaterial::serialize()`，完整复制 k/g/v 到新字节向量，再减固定 33 B 头。AAV86 在 n=128,r=5 每方 40,640 个边槽，另有 score keys；全对全每方 8,128 个边槽，另有 score keys。这是测试诊断扫描，不属于协议预处理。双方 ready 之后控制器才结束 `offline_time_ms`，故该时间**包含**整池重复序列化的 CPU、分配和内存流量；`total_time_ms` 由它派生，同样受影响。该工作发生在在线输入释放前，源码没有让诊断副本进入 secure 在线函数；不能从源码排除缓存、分配器和 CPU 状态对紧随其后的在线计时的间接影响。没有单独实测诊断成本，不能从 E14 时间扣减估算。

材料**数值口径**可另行成立：序列化格式 `9+3*8=33 B` 头，payload 为 `16·|k|+8·|g|+8·|v|`；该构造的 `|k|=bits+1, |g|=1, |v|=bits`，故每 DCF key 为 `24·bits+24 B`（34 bit 时 840 B），不依赖随机币。n=2 手算、计数器及 n=128 审计支持材料、T package/本地 shuffle/EMP OT 分栏。在线长度倍增 PRG 计数位于 DCF `traverseOneDCF`；不能等同 DCF Eval、AES 调用或比较边。分方应用层在线字节守恒；在线轮数为 AAV86 完整入口 `2r+4`、全对全完整入口 8，均是项目实现因果轮而非论文 III 结论。

## 分项接收判定

| 项目 | 判定 |
| --- | --- |
| E14 `final/` 原始完整性、身份、正式/预热隔离、统计和配对 | PASS |
| 原序 mask 功能及 n≤128 有界正确性证据 | PASS_WITH_LIMITS |
| 非时间材料、在线通信、PRG、边、轮数计数 | PASS_WITH_SCOPE（仅这两条入口及已说明的编码/计数层） |
| `offline_time_ms` | FAIL_AS_PROTOCOL_TIME |
| `online_time_ms` | INCONCLUSIVE_FOR_COMPARISON（不能排除前置诊断污染） |
| `total_time_ms` 与九指标整体最终签收 | FAIL / NOT_COMPLETE |
| E14 LAN/WAN 总时间胜负、速度结论 | WITHDRAW_PENDING_E15_REMEASUREMENT |
| n≥256 | NOT_MEASURED；24 个预检拒绝有效 |

E14 原始行与报告保持冻结，不改写、不以估算扣除诊断成本。E15 将另立计量合同、实现标签、源码提交与原始目录；本聊天不能异会话签收自己产生的 E15 数据。
