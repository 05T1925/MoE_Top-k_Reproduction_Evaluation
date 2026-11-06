# M6B-I-PROJECT-SELECT-5ITER-S21 详细复核报告

日期：2026-10-16
对象：S18 锁定的 S15 72 文件默认关闭修复快照
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 1. 裁决摘要

| 门禁 | S21 裁决 | 本轮范围 |
| --- | --- | --- |
| 源码身份 | PASS | S15/S18 manifest、72 文件哈希及 canonical digest 重算 |
| 独立密码学审查 | PASS_WITH_SCOPE | 非作者只读复核 carry→sign 断点表达；非完整安全签核 |
| 固定参数单 key / 单次调用 | CONDITIONAL | 固定 α/β/party 与完整 pre-Eval 辅助状态；依赖 AES-128 PRP 和理想联合随机带 |
| P0 carry→sign 条件模拟 | OPEN/NO_GO | 缺少可由本方 input share、理想 output share 和 L 生成的联合 continuation sampler |
| P1 carry→sign 条件模拟 | OPEN/NO_GO | 同一首个断点；逐方分析，不能由 P0 结论替代 |
| carry→sign 攻击 | NOT_FOUND_NOT_A_PASS | 未找到区分器；证明缺口不等于攻击 |
| N=128/N=256 key-only pool projection | CONDITIONAL | 槽数及 Σw 重新核算；仅 key 投影 |
| 自适应全池 | OPEN/NO_GO | carry→sign 引理未闭合；失败路径 slot ledger 未测 |
| shuffle/inverse/output/crash | OPEN/NO_GO | 未完成共同置换、帧、输出 share、lease/restart 的联合模拟 |
| 运行时缺陷 | NOT_FOUND | 只读源审；不表示安全通过 |
| 构建/测试 | NOT_RUN | 无运行时代码改动，不重跑 S18 功能套件 |
| 正式性能 | NOT_RUN / NOT_ALLOWED | V3 OPEN、V4 未获准；无 LAN/WAN 矩阵 |

路线仍是项目派生、功能范围受限的 EXPERIMENTAL 实验候选，不是 BB90 或 BMW16 实现。S18 功能接收只覆盖 S15 精确快照，不接收本报告安全结论。

## 2. 接收对象、审查者与 provenance

### 2.1 固定源码对象

S18 接收的 S15 源码目录：
C:/Users/28641/.codex/evidence/M6B_I_S18_20261016_02/source

- S15 source manifest SHA-256：4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816
- S18 receipt manifest SHA-256：5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78
- 文件数：72；本轮匹配：72/72；不符：0
- canonical source SHA-256：c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498
- S20 当前报告 SHA-256：9668c67d37ecaed9558638f7a59300f7a3c27da99b088c8ddd38d156a48aac89
- S20 较早草稿 SHA-256：cf8654e9fc985a2eeddb24549f66d19f4a4b0106c778b3c4efa2f5c7a8e47885

S21 审查者确认核对了 score adapter SHA-256 007b67cb498d714aa26761099680f4a1d0b56acd653fd14453af928a0c977c05 及 S20 当前报告 hash；较早草稿不是最终复核对象。

### 2.2 审查独立性

非作者复核者 /root/s12_receipt 声明未参与 S19/S20 证明推导或撰写，也未撰写 S20 报告；但曾作为非作者只读审阅 S20 final bridge/report。因此本次是同一审查者对该证明线索的延续复核，不是新的独立第二意见。其早前 S12 功能接收不涉及 S19/S20 证明推导。S21 只读公式/文本复核，不构建、不测试、不改 runtime。根作者曾撰写 S20，作者自审不计独立签核。复核者检查的三份草案 hash 记录在配套 source/evidence manifest。复核后，作者修正了独立性披露，并补写 §5.1 的源码到论文 PRG 接口映射；该新增桥接说明由本轮作者直接核对论文和源代码，但未由该复核者单独复核。carry→sign 的数学结论没有改变。最终文档 hash 以 manifest 为准。非作者复核范围到 carry→sign 条件视图及其数学表达，不包括完整协议证明或新增 §5.1 的 AES 实现映射。

### 2.3 关键源码身份

路径均相对于固定 S15 source 根目录。

| 文件 | SHA-256 | 本轮定位 |
| --- | --- | --- |
| VFSS/src/moe_topk/protocol_i_score_input.cpp | 007b67cb498d714aa26761099680f4a1d0b56acd653fd14453af928a0c977c05 | 34-bit ring、carry phase 4、lift、sign phase 5、q share |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | 8b2c68123db86e4ebfa976131895fdd6b383485a01f4d4bd64863a08e6d0aa64 | α 构造、两次同 key Eval、输出 share |
| VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp | d339c326efd4ab5dd3415f985a5c93b342c5c94cd0f79be86117084ef9a6f31a | masks/key 生成、score material stage |
| VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp | b2bc67cbe26bd11b2fded7a3f10002970947694e001cbf16f2bbbd45ae83982f | M2B1/v1 Gen/Eval、RAND_bytes、key ABI |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | 7576abc41ed2a7aade174940be4519b5de7064d82d0b1306cc606bcedabdba88 | shuffle 源码身份；本轮未证明联合视图 |
| VFSS/src/moe_topk/protocol_i_material_slot_store.cpp | 82cc87d8483ddaa85008fd14b0d685da5ad50460da4f27e041c12617d9938e28 | slot/store 源码身份；本轮未重测 crash/lease |

## 3. 实际 carry→sign 数据流

固定 S15 的 protocol_i_score_input.cpp 第 10–11 行设 kScoreBits=34、kScoreMask=2^34−1。因此本节所有输入、加减、mask、frame 和 lift 都在 R34=Z/(2^34)；Select key 的 40/41 位与本节无关。

对原始 share x_b，padding share 由 P0 取 0x80000000、P1 取 0。第 90–108 行构造并交换 carry operand frames，消息 phase 为 4：

- P0 的 carry-left 底值为 0，P1 的底值为 0xffffffff−x_1。
- P0 的 carry-right 底值为 x_0，P1 的底值为 0。
- 每方加本地 carry left/right mask share，按 R34 截断，双方交换完整 word frames。
- 每方从两份 frames 得到相同 masked operands，并调用本方 carry DCF material 的 eval_strict_lt。

protocol_i_ucmp.cpp 第 100–129、165–197 行把 z_left−z_right 与减去半环后的 y 交给同一 party DCF key 做两次 Eval；不能将它们视为两个独立 key。

score adapter 第 90 行声明 carry_open，但后续没有读写或打开它。每方持有本地 carry share c_b。第 109 行只取其低两位形成：
lift_b = x_b − 2^32 (c_b mod 4)  mod 2^34。

phase 5 的 sign frames 在第 112–124 行构造：

- sign-left 底值 C_0=0x7fffffff、C_1=0。
- sign-right 的本地底值是 lift_b。
- 每方分别加新的 sign left/right mask shares，再交换 phase-5 frames。
- 两方 sign DCF material 的 α_s 由生成器中 α=mask_left−mask_right mod 2^34 得到，β=1。sign 的同 key 双 Eval 后，本方 sign share 参与 q_share（第 128–134 行）。

## 4. P0/P1 条件视图与首个断点

### 4.1 获准泄露 L 与逐方视图目标

本轮沿用已确认的 L：前向 shuffle 完成后、当前活动迭代才公开的 sample seed；打乱坐标下当前端点和 pivot 位置；实际打开的比较位；masked operands；本轮 slot/access 标识、比较调用数和通信长度；双方统一 ABORT。seed 不预置未来轮次，不披露 DCF α/key、mask shares、原始分数、pivot 值、原始下标映射或完整 mask。S21 没有发现 L 之外的新 peer-visible 字段，也没有运行或观察输入相关墙钟 trace。

对 b∈{0,1}，真实 carry 阶段视图至少包含：

- 本方原始输入 share x_b 及 padding；
- 本方 carry/sign mask shares、party DCF keys 与材料 metadata；
- 本方发送的 z-frame、收到的 peer frame，以及两帧组成的公开 masked operands；
- 本方 carry 两次 Eval 的结果与本地聚合 share c_b；c_b 并未打开；
- lift_b；它是本地状态而非新增公开字段；
- phase-5 两个 sign operand frames（双方均可见）、sign local key/Eval share；
- 当前消息相位、session、material ID、调用数及后续 q/share 状态。

允许的 simulator 只能使用本方 input share、理想 output share 和获准泄露 L。不能将真实 transcript、对方 input、明文 oracle 或对方私密 share 当成输入。获准 L 中的 masked values/比较位仅覆盖源码实际公开的字段；carry c_b 不是打开比较位。phase-5 operand frames 即使属于已批准公开的 masked values，也仍需模拟其联合条件分布。

### 4.2 sign-frame 条件关系

设完整 sign left/right masks 为 M_L=A_0+A_1、M_R=B_0+B_1（均在 R34），本方右 mask share 为 B_b。公开常数 C_0+C_1=0x7fffffff。交换左帧后双方可计算：
M_L = F^s_{0,L}+F^s_{1,L}−(C_0+C_1) mod 2^34。

对方右 sign frame 为 F^s_{1−b,R}=lift_{1−b}+B_{1−b}。由于 α_s=M_L−M_R，真实联合视图满足：
F^s_{1−b,R}+B_b−M_L
= lift_{1−b}+B_{1−b}+B_b−M_L
= lift_{1−b}−α_s mod 2^34。

这不是已知攻击：腐化方不因此直接得到 lift_peer 或 α_s。它是需要模拟的联合关系，涉及未打开 peer carry share 的 continuation、已公开 frames 和 sign key 的隐藏参数。单 key key-only 伪随机性不能直接推出此条件联合分布在替换 key 前后相同。

### 4.3 狭窄的边缘均匀事实

在理想独立 dealer mask tape 下，若只条件于 carry state/lift，sign right mask shares 的分布使 B_{1−b} 在 R34 均匀；因此 F^s_{1−b,R}=lift_{1−b}+B_{1−b} 的窄边缘均匀。这不等于完整 phase-5 view 中仍均匀。左帧使 M_L 可恢复，本方知道 B_b，sign key 的参数为 α_s=M_L−M_R；必须条件化到这些相关量后证明，而不能把窄边缘结论扩展到包含 key、两帧和本地 mask 的整个视图。

mask 独立只在理想 joint RNG 计算中成立。实际 masks 使用 getrandom，M2B1/v1 root seeds 使用 RAND_bytes；S20 的 ε_RNG_joint 是未量化的外部联合随机源假设。S21 不把实现描述为统计独立或无条件均匀。

## 5. S20 推导逐步审查

S20 固定参数单 key 步骤仅在其限定范围内保留 CONDITIONAL。S20 采用的候选单 key 项为 ε_G ≤ Adv_AES128^PRP(T′, q=5) + 10/2^128；宽度为 w 的投影按 w·ε_G 计。S21 未独立重做完整归约运行时间 T′ 的推导，也不把该项视为端到端协议优势。实际 key roots 使用 RAND_bytes、share/mask 使用 getrandom，ε_RNG_joint 未量化；真实 TLS/channel 优势项同样未量化：

### 5.1 论文构造到冻结实现的桥接范围

本轮对照了 Boyle et al. 的 Microsoft Research 作者稿（55 页 PDF）：Figure 1 在论文第 13 页、Theorem 2 在第 14 页、Appendix F.1 在第 42–46 页。Theorem 2 的 DCF 构造以 `G:{0,1}^λ→{0,1}^{4λ+2}` 为 PRG 接口；Appendix F.1 的单方 key 证明对固定 `b, α, β` 逐层替换 correction-word 相关扩展，并单独处理 terminal `Convert_G`。冻结实现的 `expand()` 以 5 个不同固定 AES 输入块拼接 640 bit，再保留 MSB 前缀 514 bit；`convert_group()` 对 `G=Z/(2^64)` 取 128-bit seed 的前 64 bit，正好对应论文 `|G|=2^ℓ, ℓ≤λ` 的 `Convert_G` 规则。实现按 MSB-first 取输入位；party key 序列化存 root seed、每层 correction word、final correction 及固定结构头。固定结构头和控制字节的 padding 是合法编码约束，不能把任意字节串当论文 hybrid 的均匀 key。

把 5 个 AES 输出块替换成 5 个独立均匀块时，PRP/PRF switching 项为 `5·4/(2·2^128)=10/2^128`；余项仍是针对随机 AES-128 key、5 个不同明文块的 `Adv_AES128^PRP(T′,q=5)`。这是将项目 AES-CTR 式扩展接到论文 PRG 接口的单次调用级候选界，不是 BCG+21 本身给出的 AES 定理，也未包括 `RAND_bytes` 与 `getrandom` 的联合分布替换。S21 未重新推导归约运行时间 `T′`，所以只接受该项作为 S20 的条件性候选表达；具体实现级优势仍 `CONDITIONAL/UNQUANTIFIED`。

1. 固定 α、β、party 后，论文给出单方 key-only 分布结论；若目标 roots 在给定 α 后独立于完整 pre-Eval 辅助状态 A，则逐支持点的界可对 (α,A) 取期望。
2. 同一 key 的 Eval(x)、Eval(y) 与 local output share 是该 key、固定输入的确定性后处理。对单调用投影替换一次 key，不应误作两个独立 key。
3. c−u_b 的 received share 采样只适用于完整比较结果 c 已打开，且由固定 (α,z) 下 DCF correctness 决定的 UCMP 调用。
4. carry 的 c_b 未打开；其低两位进入 lift_b 和 phase-5 operand。不能用第 3 步把 peer share 当作 c−u_b，也不能把 Eval 后 lift/sign frame 放进独立于目标 key 的 pre-Eval 辅助状态。

首个未闭合的是从 H_pre-Eval 单 key replacement 到 H_continuation 后继采样的步骤。证明者必须说明：替换 carry key 后如何同时采样本方 carry local share、lift、本方/peer phase-5 frames、sign key 条件关系和后续 transcript，使其在 simulator 限定输入下与真实分布相同或有可证明距离。S20 没有给出该 sampler 或等式。该步骤目前是未证明，不是已证伪；S21 没有给出区分器或可利用攻击。

非作者复核者同意：只用 sign frame 的窄边缘均匀性不能证明完整 phase-5 view 仍均匀。sign-key-first/carry-key-second 两-key sampler/hybrid 是一个待证方向，不是已有结论。

## 6. 两种材料池的范围

按 S15 生成器/槽构造和 S20 槽分类重算宽度权重；以下仅是 key-only projection：

| schema | 34-bit score | 40/41-bit Select | 40/41-bit membership | 总槽数 | Σw |
| --- | ---: | ---: | ---: | ---: | ---: |
| N=128：M6B5/v3 + M2CM/v1 + M6P2/v1 | 2×128=256 | 5×C(128,2)=40,640（40-bit） | 128（40-bit） | 41,024 | 1,639,424 |
| N=256：M6S2/v2 | 2×256=512 | 5×C(256,2)=163,200（41-bit） | 256（41-bit） | 163,968 | 6,719,104 |

候选 key-only 累加式仍是：
- N=128：1,639,424·ε_G + ε_RNG_joint(schema,T)
- N=256：6,719,104·ε_G + ε_RNG_joint(schema,T)

逐槽式要求目标 roots 在给定各槽 α 向量及非 key 辅助 tape 后 fresh、条件独立；固定顺序 hybrids 要能在固定其他槽时应用；未使用材料与 package/segment digest 只能是被替换 key pool 的后处理。该式不覆盖 carry→sign continuation、比较位驱动的 adaptive address transcript、跨阶段 mask/key 条件关系、abort/crash 槽访问或实际 channel advantage。失败路径 consumed/rejected/unused slot ledger 仍 NOT_MEASURED。两种 schema 不合并为一个证明对象。

## 7. 停止继续组合的理由及其余门禁

任务规定只有 carry→sign 条件模拟成立后，才把该引理扩到自适应状态、材料池和 shuffle/output。前置未通过，因此 S21 不把之后步骤外推为已证明：

- 后续 Select 的逐轮 seed/endpoint/比较位可以作为获准 L 的公开前缀；仍须证明 score key shares、active state 和 threshold/membership carrier 与此前视图一致生成。
- N=128 的 M6B5/v3 + M2CM/v1 + M6P2/v1 与 N=256 的 M6S2/v2 自适应完整视图均 OPEN/NO_GO。
- M2CM 共同 Π/Π⁻¹、forward/inverse frames、membership carrier、selected handle、最终 output shares、lease/restart、PREPARE/COMMIT/ABORT/crash prefix 未被本轮逐方完整模拟。不能用 shuffle mask 边缘均匀或 S18 有限 crash 用例替代。
- S18 只支持指定场景的 no-complete-mask。部分 crash prefix 可留下 P1 的 8-byte XOR share；孤立 share 在重启/重试时是否泄露额外信息尚未闭合。没有声称 atomic publish/all-or-nothing。

## 8. 假设与证据类别

- 论文定义：Boyle et al., Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation，Microsoft Research 作者稿：Figure 1 PDF p.13，Theorem 2 PDF p.14，Appendix F.1 PDF pp.42–46。它用于单 key/key-only 结果，不自动证明 UCMP continuation、adaptive pool 或 Protocol I shuffle/output。官方作者稿：https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf
- 本地参考行为：逐式结论来自固定 S15 的实际 C++ 源码与 M2B1/v1 ABI。
- 项目推导：R34 变量关系、phase-5 条件帧等式、两种槽数及 Σw 是对固定实现的分析结果。
- 待验证设想：sign-key-first/carry-key-second continuation sampler，尚未证明。
- 模型：可信离线 T 生成材料后退出且不与在线方合谋；在线至多一方半诚实；双方使用私有认证通道。密码学前提还包括 AES-128 PRP，以及 RAND_bytes 与 getrandom 联合输出可被某个明确计算分布/优势项替换。ε_RNG_joint 尚未量化；TLS/channel 项本轮未量化，也未给安全位数。
- 本轮未发现新的对端可见字段；未观察输入相关墙钟 transcript。

## 9. 执行、失败与未运行记录

- 固定源码复核：72/72 PASS，canonical digest 匹配。
- 首次源码读取路径漏写 moe_topk 子目录，命令返回路径不存在；修正路径后完成读取。该失败是命令路径错误，不是源码或构建失败。
- 首次 manifest 检查脚本使用错误的 JSON 字段路径，未比较任何文件；随后按 S18 receipt manifest 真实结构读取清单，完成 72/72 校验。
- 独立 reviewer 的初稿把 score ring 写成 2^53；S21 按实际 kScoreBits=34 更正为 R34。reviewer 确认公式改正、证明断点不变。
- 初版 S21 新文档写入了字面量 `\\n`，导致 gates 不能作为 JSON 解析；随后将新文件正规化为实际换行并以 PowerShell `ConvertFrom-Json` 验证。该问题只影响本轮新写文档，不涉及旧报告或运行时。
- 首次更新验证字段时把字段写到不存在的 gates `validation` 对象，并在哈希命令重复传入 `-Algorithm`；PowerShell 报错后停止使用错误结果，改为写入 gates 根字段并用默认 SHA-256 重算 manifest。另一次 whitespace-scan 脚本因变量插值语法报解析错误，未执行检查；修正后扫描通过。以上均为文档/验证脚本错误，不是协议测试失败或源码差异。
- Build/CTest/E2E：NOT_RUN；无运行时代码修改，不重复 S18 全量功能证据。
- 正式 LAN/WAN：NOT_RUN；V3 OPEN、V4 NOT_ALLOWED。
- 未改 VFSS-baseline、论文、证书、密钥、材料包、构建产物或历史原始日志。

## 10. 最终可声明范围

可以声明：S18 独立接收的 S15 精确快照具有 PASS_WITH_SCOPE 功能证据；S21 核对了 34-bit carry→sign 实际源码关系；单 key/单次 UCMP 和两种 key-only pool projection 只能在明确 AES/理想随机源前提下 CONDITIONAL 讨论。可以准确写“未发现攻击，但证明未闭合”。

不能声明：完整 DCF+UCMP continuation 已模拟；N=128/N=256 adaptive 整池安全；shuffle/inverse/output/crash 联合单方安全；secure 或生产安全；BB90 或 BMW16 复现；五个算法迭代等于协议轮数；正式性能就绪。
