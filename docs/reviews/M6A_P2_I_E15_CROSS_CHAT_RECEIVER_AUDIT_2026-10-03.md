# M6A-P2-I-E15 异会话接收复核（2026-10-03）

## 范围与结论

本聊天未参与 E15 实现和 120 次正式批次的编写或运行。只读复核以被测源码 `346a92326e81aa3ab573c162439968503e792354`、事后报告提交 `40a1ccb450447c00ce604a08b058cbe5c0255e88` 为准；两者之间没有 VFSS 源码差异。

`E15_PROTOCOL_I_N128_DATA = PASS_WITH_FINDINGS`：接受 D=128、K=2/8、AAV86 r=2..5 与匹配 EMP-ON 全对全基线的同主机模拟 LAN/WAN 有界数据包，作为以后同功能、同参数六方案对照的 Protocol I 来源。此结论不表示 n≥256 已测、Protocol III+AAV86 已实现、M6A/V3 总验收通过或作者精确复现已证明。没有重跑 120 次网络实验或密码学安全证明。

## 独立核验

- `TEST_ONLY_E15_RAW/corrected/e15_raw_complete_index.sha256` 的 151 项逐项 SHA-256 匹配；索引哈希为 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`。原始目录和仓库外 `E15_corrected_346a923` 各 152 个文件，逐文件哈希差异为 0。
- 四份 JSONL 共 120 行，其中正式 100 行。逐行 HEAD、tracked CLEAN、PASS、原序恰 K 位状态、逐次 `offline+online=total`、在线通信 total/per-party 关系及 120 个原始日志哈希均匹配。每行登记的相关源码哈希与当前未改动的被测源码文件一致；两路线的二进制 SHA-256 已对 `/tmp/m6a15-emp-release/` 现存文件核对。
- 独立按 `profile/route/K/r` 从正式行复算 20 组；九指标各 median/min/max 共 540 项，与 `e15_unified_protocol_i.csv` 一致。24 组 `profile/K/repetition` 中两路线和四个 AAV86 r 的输入 seed/digest 均一致。作废的 E15 `final/` 与 E14 批次未进入此次统计。
- 正式 runner 为两条入口设置 `MOE_TOPK_M6A_E15_BENCH`。源码在该模式下不于 ready 前遍历/序列化材料；固定形状材料量和 OT 计数在 secure 在线计时结束后计算。E15 诊断模式保留对真实生成材料的逐字段对照。计时边界修正限于 TEST_ONLY；被测源码之后未改变 secure 路径。

## Findings 与使用口径

1. `offline_material_total_bits` 在正式行由 D、r、比较位宽和已验证的固定序列化布局**精确推导**，并经隔离的真实材料诊断检查；正式行没有逐 key 再测一次长度。后续表格应标 `DERIVED_FROM_VALIDATED_FIXED_LAYOUT`，不写成逐次物理字节测量；若 key 格式或材料形状改变，必须重做诊断及合同审查。
2. LAN `K=2,r=4` 的五次正式在线时间为 48.153、41.073、70.288、75.156、21.359 ms，波动明显。原始行保留，未删离群值；速度比较只表述为此次五次样本的中位数，不给总体或跨机器保证。后续若要解释该点的异常，可做额外诊断批次，但不得改写本批数据。
3. n≥256 的 24 个配置仅有 checked 容量与 `HARD_CAP_D_GT_128` 预检拒绝；在线时间、通信及峰值 `NOT_MEASURED`。n=256,r=2 的解析单方包 69,999,474 B 已超过现行 64 MiB 包限制。大规模性能不能从 n=128 外推。
4. 测试为单主机 WSL2 内的真实 TCP 加 netem 模拟 LAN/WAN；安全结论继续依赖原 DCF/FSS、PRG、可信不合谋离线 T、半诚实在线方与通道假设，`AUTHOR_EXACT=NOT_PROVEN`。

`Protocol III+AAV86 DESIGN_GATE = NO-GO` 的 E15 文档只是一份设计审查；本接收不将其升级为不可行性证明。下一阶段应把隐藏布局、每轮图/材料、local-rank 泄露、III field/DPF 路由和原序 XOR mask 分成可独立证伪的构造义务，在 GO 前不作 secure runtime 或 2r 声明。正式统一入口是 `protocol_iii_raw_score_mask_party`；其 F1 路线已有 `Z_(2^64)` DPF 指示份额到 XOR mask 的特化，因此通用 field Fselect/Fsort 的 field→XOR 转换不应被直接列成该 bit-mask 路线的必需阻塞。应优先审查隐藏 handle 上的 AAV86 rank shares 能否接入 F1 ring DPF、再经安全逆路由得到原序 mask；通用 field 路线仍作为独立候选。
