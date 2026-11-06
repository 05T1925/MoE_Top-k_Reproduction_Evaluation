# M6B-I-PROJECT-SELECT-5ITER-S22 详细复核报告

日期：2026-10-16
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
审查对象：S18 锁定的 S15 默认关闭修复源码，canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`

## 1. 裁决摘要

S22 在 S15 实际 score adapter 上重建了 R34 carry→sign 路径。新审查者 `/root/independent_dcf_review` 确认未参与 S19–S21 的证明撰写、推导或决策，并独立核对 72 个 S15 文件哈希。其对**局部 carry→sign pre-output transcript**给出范围受限的 `CONDITIONAL PASS`：在理想 joint RNG tape、固定函数参数 key-only theorem 适用及获准 masked-frame leakage 下，可先替换 carry key、再替换 sign key；同一 key 的双 Eval 和后续本地计算是确定性后处理。候选损失是 `68·epsilon_G + epsilon_RNG_joint`，不含完整协议或输出条件化的证明。

该独立审查没有证明从 carry/sign 本地状态到理想最终 output share 的条件模拟。沿源码继续一步，`q_b` 生成的 priority-key shares 在 `protocol_i_project_derived_select5.cpp:1901–1906` 被包装为 `key_records` 并送入 forward shuffle；没有 `F_scoreconv` 的 share 输出合同，也没有 `Sim_b(x_b,Q_b,L)` 生成与固定理想输出 share 相容的 `q_b/key_records` 条件分布。因此首个未闭合的后续状态就是 **`q_b → key_records → forward shuffle`**。本轮在该处停止，未外推到九层自适应比较、材料池或 shuffle/output/crash 的完整视图。

未发现可复现攻击，也未发现运行时代码缺陷。这里的公开帧关系是待模拟的相关性证据，不是区分攻击。未改 runtime，不构建、不运行测试或 LAN/WAN。

| 门 | S22 裁决 | 范围 |
|---|---|---|
| S15 精确源码身份 | PASS | S18 receipt manifest 的 72 文件逐项核对 |
| reviewer 独立性 | PASS | `/root/independent_dcf_review` 未参与 S19–S21；其密码学结论仅限下一行所述局部引理 |
| R34 carry/sign 源码重建 | PASS | 公式、消息相位、mask、share、DCF α 与端序已复核 |
| P0/P1 carry→sign pre-output view sampler | CONDITIONAL | 非作者独立复核；理想联合随机带、固定参数 key-only theorem 和获准 masked-frame L；`epsilon_RNG_joint` 未量化 |
| P0/P1 给定理想 output share 的 continuation | OPEN/NO_GO | 无 `F_scoreconv` share-output 规则；`q_b/key_records` 未能按固定理想输出 share 条件生成 |
| `q_b → key_records → forward shuffle` | OPEN/NO_GO | S15 源码明确把 priority share 放入 shuffle input；其与理想最终 mask share 的联合条件分布缺少引理 |
| carry→sign 攻击 | NOT_FOUND_NOT_A_PASS | 没有区分器或可利用攻击证据 |
| 两种整池/自适应视图 | OPEN/NO_GO | 按本任务前置规则未继续；仅沿用 S21 的 key-only projection 条件数 |
| shuffle/output/crash 联合视图 | OPEN/NO_GO | 未继续构造完整模拟器；沿用 S18/S21 限定范围 |
| 正式性能 | NOT_ALLOWED / NOT_RUN | V3 OPEN、V4 未获准 |

## 2. 源码对象、工作区与接收者

### 2.1 精确对象

S22 在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-project-select5-s22\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-project-select5-s22`，从 `main@c3926c68fd14f270faa8b55234311071947fa080` 建立。主桌面 checkout 有预存未提交改动，本轮没有触碰它。S15 不在 main；源码来自 S18 外部证据目录 `C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\source`，不是当前 checkout 同路径代码。

按 S18 receipt manifest 对 S15 72 个路径重算 SHA-256：72/72 匹配、0 mismatch。使用 UTF-8 无 BOM、按路径 ordinal 升序、每行 `path<TAB>lowercase_sha256<LF>` 重新计算 canonical digest，结果仍为 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。S15 manifest SHA-256 `4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`；S18 receipt manifest SHA-256 `5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78`。

### 2.2 信任模型与本轮泄露函数

本轮沿用已确认模型：离线 T 完全可信，完成材料生成后退出且不与任一在线方合谋；P0/P1 至多一方半诚实腐化；在线双方使用私有认证通道。S22 采用本任务明确批准的泄露函数 `L`：打乱坐标 endpoint/position、pivot position、打开的 comparison bits、前向 shuffle 完成后按活动迭代生成并公开的 sample seed、masked values、comparison 次数、通信长度和双方统一 ABORT。seed 不在离线 material/manifest/start binding 中提前披露。本轮 carry/sign 段只涉及 score-adapter masked frames/operands，不含 Select seed、endpoint 或 pivot。

审查者注意到历史 `M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md` 仍记有 exact approval pending；S22 遵循用户本轮明确批准范围，保留历史 audit 原件并记录该文档同步差异。批准公开不等于字段已由 simulator 证明可模拟。

### 2.3 独立性

S22 使用新的只读审查者 `/root/independent_dcf_review`。该审查者明确披露未参与 S19、S20、S21 的证明撰写、推导或决策，满足本任务独立性要求。审查范围严格限于 S15 实际 R34 carry→sign 局部 pre-output transcript sampler、固定参数 key hybrid 和其未量化假设；它没有签核 `Sim_b(x_b,Q_b,L)`、任意自适应状态链、两种全池或 shuffle/output/crash 组合。既有 `/root/s18_independent_receipt` 与 `/root/s12_receipt` 均不作为本轮新 reviewer。

### 2.4 被复核文件

| S15 源文件 | SHA-256 | 用途/行段 |
|---|---|---|
| `VFSS/src/moe_topk/protocol_i_score_input.cpp` | `007b67cb498d714aa26761099680f4a1d0b56acd653fd14453af928a0c977c05` | R34、phase 4/5、carry share、lift、sign frame、q-share，10–16、29–62、90–141 |
| `VFSS/src/moe_topk/protocol_i_ucmp.cpp` | `8b2c68123db86e4ebfa976131895fdd6b383485a01f4d4bd64863a08e6d0aa64` | `alpha_`、β=1、两次同 key Eval 与本地 share，27–60、100–129、165–197 |
| `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp` | `d339c326efd4ab5dd3415f985a5c93b342c5c94cd0f79be86117084ef9a6f31a` | mask 分割与 score stage 生成，907–933、1500–1519 |
| `VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp` | `b2bc67cbe26bd11b2fded7a3f10002970947694e001cbf16f2bbbd45ae83982f` | DCF `Gen/Eval`、AES 扩展与序列化，69–95、148–236、253–299 |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `7576abc41ed2a7aade174940be4519b5de7064d82d0b1306cc606bcedabdba88` | 固定对象；本轮未证明组合视图 |
| `VFSS/src/moe_topk/protocol_i_material_slot_store.cpp` | `82cc87d8483ddaa85008fd14b0d685da5ad50460da4f27e041c12617d9938e28` | 固定对象；本轮未重跑 lease/crash |

## 3. 实际 R34 carry→sign 调用链

### 3.1 环、字节序和份额

`protocol_i_score_input.cpp:10–11` 将 score adapter 定义在
`R34 = Z/(2^34)`，`kScoreMask=2^34−1`。输入 share 是 `uint32_t`，扩展到 `uint64_t` 后所有 carry/sign mask、frame 加法和 lift 均通过该 mask 截断到 R34。frame 按 64-bit word 以 big-endian 编码/解码（行 13–26）；DCF input width 仍为 34，不能与 Select 的 40/41-bit key 混同。

Dealer 的 `share_mask(full)`（`protocol_i_project_derived_select5.cpp:907–912`）执行：`share0 ← getrandom() mod 2^34`，`share1=full−share0 mod 2^34`。full mask 本身也由新的 `random_u64() & (2^34−1)` 产生（行 922–928、1500–1517）。在理想独立 tape 下，映射 `(full,share0) ↔ (share0,share1)` 是双射，所以两个 additive shares 是独立均匀值；不同 left/right 与 carry/sign stage 使用新随机抽样。DCF party roots 另由 `RAND_bytes` 产生（DCF 文件 253–264），与 mask tape 的**联合独立**仍只由未量化的 `epsilon_RNG_joint` 假设表达，不称为已证明统计独立。

`ProtocolIUcmpMaterial` 令 `alpha=(mask_left−mask_right) mod 2^34`、`beta=1`（`protocol_i_ucmp.cpp:27–60`）。每次 `eval_strict_lt` 计算 `x=(z_left−z_right) mod R34`、`y=(x−2^33) mod R34`，同一把 party DCF key 依次求 `Eval(x)`、`Eval(y)`，再组合成本地 additive result share（100–129；165–197）。两次 Eval 是同一 key 的确定性后处理，不按独立 key 计数。

### 3.2 phase 4 carry

`protocol_i_score_input.cpp:95–102` 的实际 frame 底值为：

- P0：`left_base=0`，`right_base=x0`；
- P1：`left_base=0xffffffff−x1`，`right_base=0`。

双方分别加自己的 carry left/right mask share（R34）后，以 phase 4 交换两个 word。合并值为：

`z^c_L = 0xffffffff − x1 + M^c_L (mod 2^34)`，
`z^c_R = x0 + M^c_R (mod 2^34)`。

因此两次 DCF Eval 输入差为
`z^c_L−z^c_R = 2^32−1−(x0+x1)+alpha_c (mod 2^34)`，其中
`alpha_c=M^c_L−M^c_R`。该式把实际输入 shares、full carry masks 和 DCF 函数参数联系起来；模拟时不能将这些变量各自替换成独立均匀值。

每方本地调用同一 carry party key 两次 Eval，得本地 `c_b`（行 104–108）。源码声明 `carry_open`（行 90），但其余代码无引用、无交换；carry local share 没有作为单独的 opened bit 发送。

### 3.3 lift 与 phase 5 sign

第 109 行是实际状态转移：

`lift_b = (x_b − ((c_b & 3) << 32)) & (2^34−1)`。

phase 5（行 117–124）的 frame 底值为：

- P0：`left_base=0x7fffffff`，`right_base=lift_0`；
- P1：`left_base=0`，`right_base=lift_1`。

各方加 fresh sign mask shares，phase 5 交换 frame。两份 frame 合并为
`z^s_L=0x7fffffff+M^s_L` 和 `z^s_R=lift_0+lift_1+M^s_R`。sign key 的隐藏函数参数仍仅为 masks 的 `alpha_s=M^s_L−M^s_R`，并未把 lift 传进 keygen。随后同一 sign key 两次 Eval，计算 `sign_b` 和
`q_b=C_b−lift_b+((sign_b&3)<<32)`，其中 `C_0=0x7fffffff,C_1=0`；输出 stable-key share `((q_b<<index_bits)+slot_if_P0) mod 2^comparison_bits`（行 128–141）。

sign Eval 输入差为
`z^s_L−z^s_R = 2^31−1−(lift_0+lift_1)+alpha_s (mod 2^34)`。
所以 `alpha_s` 本身不含 lift，但 sign operands 与 lift 存在真实联合相关；固定参数 key replacement 只处理 key hybrid，不能删除该 operand 状态关系。

P0/P1 的真实本地视图均包括：本方 raw input share；本方 carry/sign mask shares 与本方 DCF key bytes；发送及收到的 phase 4/5 frames 和合并 `z`；同 key 的两次 Eval 结果、本地 `c_b/lift_b/sign_b/q_b`；session/purpose/stage/slot 绑定与本地消耗状态。carry local share 不公开，但会影响 `lift_b` 及下一阶段自己发的 frame。P0 的 phase-5 peer-right frame 为 `lift_1+M^s_{1,R}`；P1 对称地看到 `lift_0+M^s_{0,R}`。

## 4. P0/P1 continuation sampler：成立部分与边界

### 4.1 对 S21 窄边缘论证的补充

仅写“对端 sign frame 边缘均匀”并不能证明完整视图。S22 将其放进逐阶段 key replacement 中。令 `F^s_{1-b,R}=lift_{1-b}+M^s_{1-b,R}`。在理想 dealer tape 下，`M^s_{1-b,R}` 是与 carry keys/lifts、本方 sign share 独立的 uniform R34 元素，因此对任何固定 carry state：

`Pr[F^s_{1-b,R}=v | carry state, own sign shares] = 2^-34`，对所有 `v∈R34`。

同理，peer left sign frame `F^s_{1-b,L}` 是固定常数加独立 uniform mask share，也均匀。故两项对端 sign frame 可作为 fresh uniform values 生成；其聚合 `z^s` 由两方 frames 相加重建。这给出实际的 continuation frame sampler，不需要模拟器输入对方 carry share。这个事实并不表示 `alpha_s` 与 frames 独立。

更具体地，给定本方输入 share、carry history 和本方 mask shares，P0 收到的 carry frames 为
`(2^32−1−x1+M^c_{1,L}, M^c_{1,R})`，P1 收到
`(M^c_{0,L}, x0+M^c_{0,R})`；各 pair 的两个 fresh peer mask shares 在 ideal tape 下独立均匀。sign frames 同理：P0 收到
`(2^31−1+M^s_{1,L}, lift_1+M^s_{1,R})`，P1 收到
`(2^31−1+M^s_{0,L}, lift_0+M^s_{0,R})`。条件于 carry state，lift 固定而 peer right mask share 仍 fresh uniform，所以 sign-right frame 是精确 uniform；sign-left frame 也精确 uniform。每方自己的两个 outgoing frames 则由自己的 share/lift 和本方 mask shares 确定。对同一阶段同时看两帧时，聚合 operand 与 `alpha` 可能相关；条件 key hybrid 必须按真实 `(alpha,frames)` 联合分布逐点处理，不能声称二者独立。

### 4.2 逐 key 条件 hybrid 的候选步骤

BCG+21 Figure 1/Theorem 2/Appendix F.1 的单方 key-only结论按固定 `(alpha,beta,party)` 给出逐点界。官方作者稿为 [Microsoft Research PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)：DCF 定义/定理见 PDF p.13–14，Appendix F.1 见 PDF pp.42–46。它证明单方 key 的伪随机性，**不直接证明**本 Protocol I 的 output-share-conditioned 完整视图。

在 ideal joint tape 条件下，carry root 与 mask shares 独立。条件于真实 `alpha_c` 和全部非目标-key状态，替换本方 `K^c_b` 为该定理的 final random-key hybrid `U^c_b`，损失至多 `34·epsilon_G`；随后 `c_b/lift_b` 是 `U^c_b` 和 phase-4 `z` 的确定性后处理。替换后仍按真实公式生成 sign-side frame。sign key 的 root 又是独立 fresh draw；在条件于 `alpha_s` 和已经生成的 carry/sign 非目标-key状态后，将 `K^s_b` 替换为 `U^s_b`，再做两次 Eval 和 `q_b` 后处理，损失至多再 `34·epsilon_G`。因此，**若视图目标只是该局部协议产生的本方 transcript 与本地中间结果、并让输出 share 随模拟器生成**，候选 bound 为：

`Delta(V_b, Sim_b^preout) <= 68·epsilon_G + epsilon_RNG_joint(R34 carry/sign; T)`。

这里 34 是单个 DCF 的 input width；同一 key 的双 Eval 不再乘 2。`epsilon_G` 仍为 `Adv_AES128^PRP(T′,q=5)+10/2^128` 的候选单 key 项；`T′` 未重导，AES/RNG 优势没有具体安全位数。`epsilon_RNG_joint` 覆盖 getrandom mask/share 与 OpenSSL RAND_bytes roots 的联合 tape，尚未量化。实际 TLS 私有认证信道优势与联合视图未在本轮分析或量化。不得将这些占位项写成统计独立、无条件均匀或具体安全位数。

### 4.3 P0 与 P1 的可执行 sampler 草图

对任一 `b`，若输入是 `(x_b, L_frames)`，其中 L 只含已批准、实际 phase 4/5 公开的 masked frame/operand 数值、相位与长度：

1. 从 phase-4 本方发送帧及本方公开输入分量恢复本方 carry mask shares；用 L 中两侧 phase-4 frames 求 `z^c`。
2. 采样一个 `U^c_b`，用实际 `eval_strict_lt` 对 `z^c` 两次求值，计算 `c_b` 和 `lift_b`。
3. 从 phase-5 本方发送帧减去已知常数/模拟 `lift_b`，重建本方 sign mask shares；L 中 peer phase-5 frames 直接确定公开聚合 `z^s`。不需要采样 peer `lift` 或读取 peer secret。
4. 采样独立 `U^s_b`，执行实际双 Eval，生成 `sign_b` 与 `q_b`；两次 Eval 保持同 key 相关。
5. 输出本地 package/key serialization、已用状态和 phase frame view（仅在其 exact L 字段允许且 message ordering 与源代码一致时）。未执行 pool/lease/crash 模拟。

这说明可为**未条件化的局部 transcript 分布**写出明确候选算法；S22 不是已证明上述 sampler 与某个理想 F 的完整视图同分布。若 simulator 被要求接收已固定的理想 score-output share `Q_b`，目前的步骤 4 会重新生成 `q_b` 而未约束其等于 `Q_b`。

### 4.4 真实 phase-5 相关式不是攻击，但约束模拟

party b 可由获准的两份 sign-left frames 得到 `M^s_L`，并持有自己的 right-mask share `M^s_{b,R}`。其观察到的 peer right frame满足：

`F^s_{1-b,R}+M^s_{b,R}−M^s_L = lift_{1-b}−alpha_s (mod 2^34)`。

此式把隐藏 peer lift、sign α 与公开 frame 绑定；只证明 `F^s_{1-b,R}` 的边缘均匀无法替代条件联合模拟。但它不是一个已知攻击：party 未直接得到 `lift_{1-b}` 或 `alpha_s`，而 key-only replacement 可能在满足定理的条件游戏里消去 key 对 α 的依赖。S22 没有给出利用该式区分输入的算法或优势，因此裁决为“未发现反例，但证明未闭合”，不是被攻破。

### 4.5 独立复核结论与首个后续断点：固定理想 output share 的条件分布

`/root/independent_dcf_review` 的独立只读结论是：P0、P1 的局部 R34 carry→sign pre-output view 均为 `CONDITIONAL PASS`。其确认两个阶段各生成新 masks 和 DCF roots；在 joint-RNG idealization 下，carry 历史不改变 sign roots 的条件分布。按固定 `alpha,beta=1,party` 应用 key-only theorem 后，两次 Eval 和本地结果可作为确定性后处理。独立审查给出的单阶段项是 `34·epsilon_G + epsilon_RNG_joint(prefix)`；carry/sign 两次 replacement 合为 `68·epsilon_G + epsilon_RNG_joint`。联合 RNG 项必须覆盖跨阶段 `getrandom`/`RAND_bytes` 的完整调用前缀，而不是逐调用边缘随机性。`epsilon_G ≤ Adv_AES128^PRP(T′,q=5)+10/2^128`，其中 `T′`、AES 优势和 RNG 项未量化；不得据此报具体安全位数。

审查者指出仓库历史 `M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md` 尚记录“未确认确切 leakage approval”。本轮以用户在 S22 请求中再次明确的已批准字段为本轮 `L`：shuffle 后 endpoint/position、pivot position、comparison bits、活动迭代 seed、masked values、comparison/communication count 与统一 ABORT。此项是历史文档同步差异，不改变安全定理范围，也不把获准字段当作模拟证明。若代码出现超出该 `L` 的新 peer-visible 字段，仍须另行处理；本轮没有发现或验证新的此类字段。

审查者没有验证固定理想输出 share 的完整 simulator。沿 S15 调用链，下一状态为：`protocol_i_score_input.cpp:129–141` 产生 `keys[i]`；`protocol_i_project_derived_select5.cpp:1901–1906` 将其写入 `key_records[i].word0` 并传给 `run_two_round_shuffle`。随后的 shuffle share 再进入 `shuffled_keys`（行 1911–1913），被后续自适应比较使用（行 1921 起）。因此，所需的首个新引理不是“每个比较槽安全”，而是对给定 `(x_b,Q_b,L)` 生成正确分布的 `q_b/key_records` 及 forward-shuffle 本地视图，并维持它与理想最终 mask share 的条件分布。S22 没有证明这一点，也没有声称它不可能成立。

S18/S20 文档规定 simulator 目标输入包含 `(X_b, ideal_output_share_b, L)`，但实际审阅到的材料没有定义 score adapter 中间 `q_b/keys_b` 的 ideal functionality，也没有规定理想 share 是否允许 simulator 选择/随机化、输出 share 与 L 的联合分布。局部 sampler仅从 `(x_b,L)` 生成 `q_b`，不能直接证明它在给定任意合法 `Q_b` 后输出正确的条件分布 `View_b | Q_b,L`。

可执行的下一条证明义务是先定义 `F_scoreconv` 的 share 输出约定，再证明以下至少一项：

- simulator 可在理想功能约定下选择腐化方的加法 share，并由功能给诚实方补足，使步骤 4 的模拟 q 成为 ideal output；或
- 给出 `Sim_b(x_b,Q_b,L)`，条件生成 carry/sign key 与 frames，证明其分布等于真实视图条件分布。若走 rejection sampler，需证明对所有合法 `Q_b` 的接受概率下界和条件分布；仅称“top 2 bits 应均匀”不够。AES/PRG 与 RNG 项也要进入该条件界。

这一步不证明最终 Top-K output mask share 的联合分布。它是 S22 当前可以精确指出的首个未闭合定义/分布步骤，而非已证伪步骤。没有复现实验能构成攻击见证；phase-5 相关式是模拟条件约束，不是攻击见证。S22 未继续到 adaptive pool 或整个 shuffle/output/crash 模拟。

## 5. S20/S21 结论在 S22 的处理

- S15/S18 的 72-file source identity 已重新计算，S18 的功能 `PASS_WITH_SCOPE` 仍仅覆盖原 S15 功能快照，不扩展为本轮安全接收。
- S20 的固定 `(alpha,beta,party)` 单 key/key-only投影步骤，在其假设下仍作为 `CONDITIONAL` 候选；同 key 双 Eval 仍是后处理，不单独乘 Eval 次数。
- S21 的实际 R34、更正后的 phase 4/5 frame 与 lift 公式均由源码支持；S22 的非作者审查把“局部、pre-output carry→sign sampler”评为 CONDITIONAL，但未关闭 S21 所要求的固定理想 output share simulator。
- S21 记录的完整 continuation OPEN 仍成立；此处不是发现 S21 的 source error 或攻击。
- N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` 与 N=256 `M6S2/v2` 的池仍是不同对象。S21 key-only projection 的槽数/Σw沿用为条件历史证据；S22未重算全池、未检查 abort/crash consumed-slot，也未把局部 sampler延伸成 adaptive pool证明。
- M2CM共同 Π/Π⁻¹、membership、inverse route、output shares、lease/restart、PREPARE/COMMIT/ABORT/crash仍OPEN。S18指定案例只支持 no-complete-mask；不表示原子同时发布或 all-or-nothing。

## 6. 测试、失败、修改和未运行

- S15 72/72 源码哈希校验与canonical hash：PASS。复跑命令为读取S18 receipt manifest，逐文件 SHA-256 后按路径ordinal排序计算UTF-8无BOM `path<TAB>sha256<LF>`摘要；详情在配套S22 manifest。
- 源码文本检查：phase4/phase5、R34、同 key 双 Eval、mask shares/alpha/root生成器字段直接核对。没有执行协议。
- 独立 build、CTest、oracle/E2E、carry-sign trace、构造攻击实验、adaptive pool、shuffle/output/crash：`NOT_RUN`。无 runtime change，因此不重复 S18 功能矩阵。
- 未发现可复现 runtime defect；未改 C++、ABI、材料 schema 或 DCF key。
- `git diff --check` 对 tracked 差异通过（exit 0）；另对 S22 新写的 3 份 Markdown 扫描，未发现尾随空白。16 个携带的 S18–S21 历史文档与 S21 worktree 逐文件 SHA-256 相同。`VFSS/`、`VFSS-baseline/`、论文与参考目录无本轮变更；差异中不含证书、密钥、材料包、构建物或原始大日志。
- 文档生成尝试记录：一次 `apply_patch` 因上下文文本不匹配而未应用；一次 PowerShell manifest 更新尝试对不存在的顶层 reviewer 字段赋值并发出属性错误，脚本仍写出了 JSON。随后改为更新已存在的 review 节点，重新核对 72 个源码哈希、16 个历史文件哈希并解析 gates/manifest JSON。上述文档工具错误没有触及 runtime 或原始 evidence。
- 正式 LAN/WAN：`NOT_RUN/NOT_ALLOWED`。M6A V3 仍 OPEN、V4 未获准。

## 7. 本轮结论与唯一最小后续任务

**可以声称：**S18接收的S15快照具有限定范围内的功能证据；S22独立源码复核了R34 carry→sign的实际消息与状态关系；在理想 fresh joint tape 和固定参数 key-only 条件成立时，S22给出了可供密码学审查者逐步检查的 P0/P1 pre-output continuation sampler 及 `68·epsilon_G+epsilon_RNG_joint` 候选界。

**不能声称：**carry→sign 已有经独立签核的 ideal-output-share-conditioned simulator；局部 sampler 已组合成自适应完整视图；两种材料池、shuffle/output/crash安全通过；secure、生产安全、BB90或BMW16复现；完整Protocol I轮数或正式性能就绪。

**下一任务最小范围：**定义 `F_scoreconv` 的 share 输出语义，并证明 `Sim_b(x_b,Q_b,L)` 可按固定理想输出 share 条件生成 `q_b/key_records`、M2CM forward-shuffle 的腐化方本地视图以及第一步自适应输入；由未参与 S19–S22 的密码学审查者复核本报告 §4.1–4.5。需要明确 simulator 是否能选取腐化方 share，或给出条件分布/接受率下界。仅当该条件引理关闭后，才继续到一个实际自适应 Select slot；不得同时宣称 N=128/N=256 整池或 shuffle/output 关闭。



## 8. 版本化备选方向（未实施、待验证）

若 S15 的 `Sim_b(x_b,Q_b,L)` 条件分布最终无法证明，最小研究方向可评估新建 `PROJECT_DERIVED_SELECT_5ITER_SCORE_SHARE_V2`：在 `key_records` 进入 forward shuffle 前，对 priority-key shares 做一次 fresh one-time additive rerandomization，并把 rerandomization purpose/slot/session binding 纳入新 party-material schema。它会改变每方材料、share 状态和 forward-shuffle 输入视图，也可能影响后续比较 frame；必须重新证明稳定键重构不变、联合输出-share模拟成立，重新核算两种 schema 容量/访问顺序并跑 conformance/oracle/E2E。单纯“加随机掩码”不保证条件模拟成立；未完成这些证明前不得实现为安全修补，也不得复用旧 key/package。本段是待验证设想，不是攻击修复或 S22 的通过结论。
