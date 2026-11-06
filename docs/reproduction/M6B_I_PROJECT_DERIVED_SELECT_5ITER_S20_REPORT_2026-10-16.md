# M6B-I-PROJECT-SELECT-5ITER-S20 密码学条件视图反向复核

日期：2026-10-16
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL
结果：源快照身份复核通过；本轮作者自审及非作者密码学审查均未发现可复现攻击或运行时代码缺陷。非作者审查将固定 α 的单 key/单次 UCMP 结论评为有条件成立，并确认 key-only 池界的算术；同时指出 carry→sign continuation 是首个未闭合步骤。因此自适应全池与 shuffle/output/crash 联合单方视图仍为 OPEN/NO_GO，路线保持 EXPERIMENTAL。

## 1. 审查者身份与边界

本报告作者撰写了 S19 条件视图推导，因此作者自审本身不是独立签核。之后由未参与 S19 撰写的密码学审查者 `/root/s18_independent_receipt` 完成只读复核；该审查者此前只参与 S18 功能接收，未参与 S19 证明撰写或 S20 自审。独立复核的逐项裁决见第 6.1 节。此审查确认有限范围的条件推导，并未签核整池或完整协议安全。

当前对象是 S18 独立接收的 S15 默认关闭修复运行时快照。S18 的接收只覆盖 S15 精确源码与其报告明确限定的功能证据，不覆盖 S19/S20 安全推导。S19 本身是条件安全推导草案，没有改运行时代码或执行测试。

## 2. 冻结对象、哈希和审查方法

S15 精确源码根目录为：

C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\source

锁定清单：S18 source/build/evidence manifest SHA-256 为 5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78。清单内列出 S15 72 个文件的相对路径和 SHA-256。S20 重新逐文件读取，并按 UTF-8、斜杠标准化、路径 ordinal 排序、每行 path + TAB + lowercase SHA-256 + LF 复算 canonical 值：

- 文件数：72。
- SHA-256 不匹配：0。
- 复算 canonical SHA-256：c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。
- 与 S15/S18 声明值相同。
- S15 未提交且未合入 main；运行时 identity 仍是外部冻结快照，不是 S20 分支当前文件。

核心运行时文件及哈希：

| 文件 | SHA-256 | 核查入口 |
| --- | --- | --- |
| VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp | b2bc67cbe26bd11b2fded7a3f10002970947694e001cbf16f2bbbd45ae83982f | expand、convert_group、generate、keygen、serialize/deserialize |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | 8b2c68123db86e4ebfa976131895fdd6b383485a01f4d4bd64863a08e6d0aa64 | alpha、beta、双 Eval、party share 和 ABI |
| VFSS/src/moe_topk/protocol_i_score_input.cpp | 007b67cb498d714aa26761099680f4a1d0b56acd653fd14453af928a0c977c05 | carry/sign masked input |
| VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp | d339c326efd4ab5dd3415f985a5c93b342c5c94cd0f79be86117084ef9a6f31a | Select compare、seed、membership、inverse、output |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | 7576abc41ed2a7aade174940be4519b5de7064d82d0b1306cc606bcedabdba88 | permutation 与 sigma/tau/a/e/r 生成和两阶段执行 |
| VFSS/src/moe_topk/protocol_i_material_slot_store.cpp | 82cc87d8483ddaa85008fd14b0d685da5ad50460da4f27e041c12617d9938e28 | party-scoped read、lease、streamed store |

外部论文证据使用 Boyle 等作者维护的 Microsoft Research PDF：[Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。核对 Figure 1（PDF 第 13 页）、Theorem 2（第 14 页）和 Appendix F.1（第 42–46 页）。定理证明的是每个固定函数参数下单方 DCF key 的伪随机性；不会自动证明任意 MPC transcript、UCMP 双 Eval、相关多 key pool 或 Protocol I shuffle/output。定理将非均匀 adversary 用于固定 b、α、β 的逐点 hybrid；当每个目标 key 与条件化的非 key 侧信息在给定 α 后独立时，固定 α 的界可逐点用于该侧信息并取期望。该量词转换不覆盖与目标 key roots 相关的 auxiliary input。

## 3. 定理桥接与可复算界

实际新 ABI 为 M2B1/v1：

- DCF input width 为 34..53；UCMP 设 β=1；output group 是 Z/(2^64)。
- keygen 以两个 16-byte root seeds 建两方 key；初始控制位等于 party id。
- 每层 G 将同一个 128-bit seed 作为 AES-128 key，对 5 个互不相同的固定 128-bit block 求值，拼接后保留 514 个 MSB-first bits，即 4λ+2；字段解析成左右子 seed、左右 128-bit v 和两个 control bits。
- Convert_G 对 Z/(2^64) 取 128-bit seed 的前 64 bits；在理想均匀 seed 下输出严格均匀群元素，不加转换统计误差。
- Key serialization 固定头、计数与高位 control padding；最终 random hybrid 必须是相同结构的合法 key 字段分布，不能把任意字节串称为 Theorem 2 的 U。
- UCMP 的 alpha=(mask_left-mask_right) mod 2^w、β=1。x=(z_left-z_right) mod 2^w；y=(x-2^(w-1)) mod 2^w。相同一把 party DCF key 依次 Eval(x)、Eval(y)。party 输出由 t2-t1 和 party-specific 常数修正产生；两份输出相加得到公开 comparison bit。

令 εG = Adv_AES128^PRP(T′, q=5) + 10/2^128。第二项是 5 个 128-bit permutation outputs 与 5 个独立均匀块之间的 switching bound。单个宽度 w 的 party key 由 Appendix F.1 的 w 个 PRG hybrids 替换，代价 w·εG。两个 Eval 共用同一 key，属于同一随机 key 的确定性后处理，不能把它们误作独立 key；相应优势不额外乘 2。

### 3.1 UCMP 条件视图：P0 与 P1

P0 与 P1 分别持有各自输入份额、对应 party key、mask shares、槽/用途/方向/session/iteration 元数据、发送和接收的 masked operand-share frames、合并后的 z、本地两次 Eval share、对方 result share 以及打开位 c。P0/P1 的 control bit 和 uCMP 常数项不同，模拟时 party header 与输出公式必须分别实例化。

本轮自审认可下列**单次调用的条件证明结构**，范围仅到一次 key replacement：

1. 固定 α、β=1、party b 和调用前非 key 状态 A。A 可与 α、z 和本方 mask shares 相关，但不能包含目标 K_b 或由其计算出的后续状态。这里 z 的两个端点和本地/对端 masked operand frames 都在 DCF Eval 前产生。
2. 在理想联合随机 tape 下，给定 α，目标 DCF root bytes 独立于 mask shares、输入份额、slot 元数据和 pre-Eval operand frames。α 与本方 shares 之间可以有关联；逐个固定 α 后应用 Theorem 2 的统一界，再对 A 的条件分布取期望，不要求 α 与 A 在边缘上独立。
3. opened comparison bit c 由 DCF correctness 确定为目标输入关系，对所有 root tapes 相同。条件化 c 不额外偏置目标 key。此点依赖被比较两方的 α、x 关系确实等于该协议要输出的确定布尔函数。
4. 用一个 U_b 替换同一 key；按真实代码在 x/y 两点 Eval，生成本地 t1/t2、u_b。接收 share 按 correctness 关系 c-u_b 生成。整个 Eval pair 是对一个 key challenge 的后处理。点wise key-only bound 为 w·εG。
5. 不得把未来候选/下一层 operand frame 固定为与当前 key 独立的 advice。它们是当前调用后的协议 continuation；要证明整段视图，应让其作为当前 hybrid 的后处理，或逐槽使用因果顺序证明。

因此，S19 单次 UCMP 条件引理没有被本轮找到无效 algebra step；其结论只能写成：理想独立随机 tape 和 AES-128 PRP 条件下，固定 α、party、β 的 M2B1/v1 key-only pseudorandomness 可提升到该调用**预 Eval**辅助输入和由正确性决定的开位。该单次结论不等于端到端 Protocol I 安全。

S19 使用的联合随机源差异 εRNG_joint 仍是外部假设：它必须覆盖 dealer getrandom、OpenSSL RAND_bytes、全部 mask/份额/置换/shuffle pad、逐活动 iteration sample seed 与 rejection sampler 的完整自适应请求顺序，并允许 adversary 观察获准 L 与本方辅助材料。该量未量化，也不代表 RAND_bytes 与 getrandom 统计独立、无条件均匀或有具体安全位数。

### 3.2 对“取期望”步骤的条件

该步骤对每个固定 α 都有同一个统一上界 w·εG，故对任何在给定 α 后与 target root 独立的 A 分布取期望，不改变上界。α 可以是本方 masks 的函数，也可与 z 确定相关。禁止的情况是把包含 target key、或由 target Eval 产生的后续数据，塞入“固定 auxiliary input”再套同一定理。S19 将 W_b 定义为调用前状态，因此单次调用的这个风险可避免；整池证明必须按 causal order 确保每槽前态只含之前已处理的槽。

## 4. 两种 DCF pool 与自适应 slot

重新按 S19 的每类槽位数量和位宽重算：

| N / schema | score key | Select key | membership key | 总 DCF slots | Σ 输入位宽 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 128 / M6B5/v3 + M2CM/v1 + M6P2/v1 | 256×34 | 40,640×40 | 128×40 | 41,024 | 256×34 + 40,768×40 = 1,639,424 |
| 256 / M6S2/v2 | 512×34 | 163,200×41 | 256×41 | 163,968 | 512×34 + 163,456×41 = 6,719,104 |

Select slot 容量对应 5·C(N,2)；score conversion 为每个 padded item 两个 DCF slots；membership 为每个 padded item 一个。M2CM 的 shuffle metadata 不是 DCF key，未加进 Σw。S18 报告成功 case 301、case 110 的消费数仅是成功用例数据；abort/crash/拒绝路径逐槽消费仍 NOT_MEASURED。

在 ideal tape 下，如果全池每个 key 的 root draws 互不重用且与所有 dealer masks 独立，那么条件于完整 α 向量及非 key tape，可按预先固定的 slot 顺序作 hybrid。每个 slot j 的替换量 wj·εG，求和为 Σj wj·εG。最后把真实联合随机 tape 换为理想 tape，加 εRNG_joint；若把外部网络观察也纳入游戏，另有 εchannel。N=128 与 N=256 的加权项分别为 1,639,424·εG 与 6,719,104·εG。

自适应访问地址在公开前缀下由活动 seed、已经打开的 comparison bits 和当前候选状态确定；seed 按实际 runtime 在 forward shuffle 后、当前 iteration 激活时才发送。comparison bit 依 DCF correctness 不依 roots。若每个 key slot 只消费一次，地址顺序是该公开前缀的后处理，不引入新 key challenge。N128 monolithic 包的未用 key 字节、N256 streamed segments 的未 lease 字节和 durable lease 前缀都必须在最终一方视图 simulator 中出现；前述仅在 ideal-tape/key-only projection 上给条件论证，S20 不声称 abort/crash 完整访问账本已实测。

## 5. M2CM/v1 shuffle、inverse 与 output share

实际 dealer 对给定共同置换 Pi 采样独立均匀 sigma0、sigma1，再设 tau0=Pi∘sigma1⁻¹、tau1=Pi∘sigma0⁻¹；采样独立均匀 a0/a1、h、r0/r1，设 e0=tau0(a1)-h、e1=tau1(a0)+h。Party b 发送 Y_b=sigma_b(X_b)+a_b，计算 S_b=tau_b(Y_(1-b))+e_b，再发送 Q_b=S_b+r_b。由定义可复算：

- S0=Pi(X1)-h；
- S1=Pi(X0)+h；
- S0+S1=Pi(X0+X1)；
- Q0+Q1=Pi(X0+X1)+r0+r1。

对固定 Pi，腐化方 b 的 sigma_b 均匀；tau_b 是 Pi 与另一方独立均匀 sigma 的复合，故 tau_b 也均匀，且与 Pi、本方 sigma_b 独立。forward/inverse 若使用独立新 sigma 和 pads，即使一方向使用 Pi、另一方向使用 Pi⁻¹，本地完整 permutation tuple 的条件分布仍不依赖共同 Pi。这是逐方向、逐方的条件边缘，不自动说明所有 record data、DCF masks 和 transcript 联合分布。

S19 的 P0/P1 simulator 框架可按如下方式成立：接收本方输入份额 X_b、理想输出 share O_b 与批准 leakage L；按 L 给出的 masked operand-share、Y/Q frame 和 opened bits 逐时序生成消息；本地 fresh mask shares 从本方 frame 减去已模拟本方 input/output share 得到；前向本地 Y/Q pad 可按均匀 a/h/r 的条件分布生成；active seed 只在批准 phase 生成；同一 U_b 对当前 UCMP 两点 Eval；membership bit 不打开；inverse local word1 的最低位条件为理想 O_b，其余 63 位由 fresh h 的均匀 pad 模拟；PREPARE/COMMIT/ABORT/crash 状态依固定 phase prefix 生成。P0/P1 的符号、初始 control 和 share 方向要分别实例化。

本轮没有从 M2CM 等式中得到区分器。S19 描述的 simulator 仍应由非作者形式化成一个联合条件概率实验。最需要审查的首个组合步骤不是单 key 定理，而是：逐槽 hybrid 中，后续 masked frame 与本地 mask shares、先前 key-derived shares 之间的条件分布，是否可按 L 所给字段和仅本方输入/理想输出 share 生成，并且在改变先前 key 后仍保持同分布。若把 L 定义成仅 aggregate z 而不含线上分别发送/接收的 masked operand-share frame，S19 的 frame-conditioned simulator 不适用。S19 明确采用更强的 wire-level L；它必须作为理想泄露输出精确定义，不能把“真实 transcript”对象直接交给 simulator。

崩溃合同仅是已测 prefix 内 no-complete-mask。晚期 crash 可让 P1 留下 8-byte 本地 XOR share；它必须作为该腐化方自己的输出 share 纳入 view。孤立 share 不等于完整 mask。没有原子同时发布证据，也没有 all-or-nothing 结论；任意管理员回滚、生产 store、跨主机 durability 和输入相关墙钟观察不在本轮范围内。

## 6. 逐门裁决

| 门 | S20 状态 | 本轮复核范围与限制 |
| --- | --- | --- |
| S15 source identity | PASS | 72/72 hashes，canonical SHA-256 与 S18一致；对象仍是外部 S15 source snapshot |
| S19 non-author cryptographic review | COMPLETED_WITH_OPEN_FINDING | `/root/s18_independent_receipt` 未参与 S19；对单调用/key-only projection 给出 CONDITIONAL，对 carry→sign 后续联合分布指出 OPEN 首个断点；只读、未构建/测试 |
| DCF_BCG21 bridge | CONDITIONAL | Figure 1/Theorem 2/Appendix F.1 到 M2B1/v1 字段映射；AES PRP 与 ideal tape 假设 |
| DCF single-key party view P0 | CONDITIONAL | 非作者确认固定 α 后对独立于 target roots 的完整 pre-Eval 辅助状态逐点套用 key-only 界可取期望；依赖理想联合 tape/AES PRP |
| DCF single-key party view P1 | CONDITIONAL | 非作者复核同一单调用结构；party-1 初始互补控制位与接收 share 需按实际分布处理，结论不扩展到 continuation |
| key-only pool projection N128 | CONDITIONAL | M6B5/v3 + M2CM/v1 + M6P2/v1；Σw=1,639,424；非作者复核算术，成立需槽根独立/理想 tape；不是自适应 transcript 证明 |
| key-only pool projection N256 | CONDITIONAL | M6S2/v2；Σw=6,719,104；非作者复核算术，成立需槽根独立/理想 tape；不是自适应 transcript 证明 |
| adaptive full-pool view N128/N256 | OPEN/NO_GO | 后续 masked frames 与本方状态在 key replacement 后的条件分布未由 sampler/hybrid 闭合；失败消费逐槽账本 NOT_MEASURED |
| shuffle/inverse/output P0/P1 | OPEN/NO_GO | 有 M2CM 代数及 sampler 框架；共同 Π/Π⁻¹ 下与 frames、DCF masks、output share、crash/lease 的联合条件分布未证明 |
| crash/abort joint view | OPEN/NO_GO | 仅继承 S18 指定案例中的功能性 no-complete-mask 证据；孤立本地 share、重启/重试的完整联合分布未模拟，不称原子发布 |
| reproducible attack | NOT_FOUND_NOT_A_PASS | 本轮未构造攻击；不是密码学证明 |
| runtime defect | NOT_FOUND | 本轮只读，未发现可复现缺陷 |
| N>256 admission | OPEN/NO_GO | 未扩大 |
| secure alias | OPEN/NO_GO | 自适应 pool 与 shuffle/output 联合证明未闭合，且 RNG/channel 优势未量化 |
| formal performance / V3→V4 | NOT_RUN | V3 OPEN；V4 未获准；LAN/WAN NOT_RUN |

### 未量化项

- εRNG_joint(schema,T)：整个有序 getrandom/RAND_bytes tape 及本方可见辅助信息与独立 uniform tape 的区分优势；平台、请求序列、时间损失和具体优势均未实例化。
- AES PRP 项：未选定具体攻击时间 T′，因此 εG 和池总优势不是 concrete security level。
- εchannel：理想私有认证信道模型可取 0；实际 TLS/外部观察者项未量化。对腐化 party 的 plaintext peer frames 本身仍由 L 覆盖。
- abort/crash 消费槽分布、外部 timing、管理员/root 或跨主机 durable storage：NOT_MEASURED/NOT_RUN。

### 6.1 非作者密码学复核：范围与裁决

审查者：`/root/s18_independent_receipt`（独立 reviewer agent）；利益/参与关系：参与过 S18 功能接收，但未参与 S19 证明撰写、S20 作者自审或 S4–S17 的实现/证明/决策。本轮只读，没有运行构建、测试或正式性能矩阵，也没有修改文件。审查者独立复算 S15 外部快照 72/72 哈希与 canonical 摘要；对象身份与 S20 结果一致。审查者检查的 S19 报告 SHA-256 为 `27380f69ac4fdd71a6d7c0f98be097e8a74a89367d3ece054473bdfe17441f8c`，S20 报告初稿 SHA-256 为 `cf8654e9fc985a2eeddb24549f66d19f4a4b0106c778b3c4efa2f5c7a8e47885`。

非作者裁决如下：

1. **M2B1/v1 单 key 与单次 UCMP：CONDITIONAL。** 固定 `(α, β, party)` 后，若目标 key roots 在给定 α 后独立于完整 pre-Eval 辅助状态，Appendix F.1 的逐点 key-only 界可对该辅助状态的条件分布取期望。相同 key 的两次 Eval 是同一 key 的确定性后处理，不产生第二个独立 key 项；若打开的 `c` 是固定 `(α,z)` 下由 correctness 决定，可用 `c-u_b` 构造接收 share。该结论不覆盖目标 Eval 生成的 continuation，并依赖 AES-128 PRP 与理想联合随机 tape。建议界仍为 `ε_G ≤ Adv_AES128^PRP(T′, q=5) + 10/2^128`；`ε_RNG_joint` 未量化。
2. **key-only 池投影：CONDITIONAL。** 非作者复算 N=128 的 41,024 槽、`Σw=1,639,424`，及 N=256 的 163,968 槽、`Σw=6,719,104`。该界可用于独立理想 roots 条件下的 key-only projection，不自动覆盖在线 transcript、失败/崩溃槽消费或自适应查询。
3. **首个无效/未闭合推导步骤：全池 continuation。** S15 carry DCF 的本地 share 不会在该阶段打开；`lift = x - ((carry & 3) << 32)` 随后进入 sign 阶段 operand frame。该 sign frame 依赖此前 key 派生且未公开的本地 share。S19 以“已打开比较 bit 不依赖 roots，因此未来地址是公开前缀的后处理”不能覆盖 carry→sign 帧；只有真正已打开的比较位能作为该论据中的公共前缀。要闭合必须给出替换 carry key 后的 `LeakageSampler_b(X_b,O_b,L)`，证明 carry shares、下一阶段 sign frames 与本地 masks 的联合条件分布正确。
4. **M2CM/inverse/output/crash：OPEN/NO_GO。** 固定共同 `Π` 时，`σ_b` 与 `τ_b=Π·σ_(1-b)^−1` 的各自边缘均匀可核对；该边缘事实并不模拟共同 `Π/Π^-1` 下的 Y/Q frames、DCF carrier、最终 output share、lease 与 crash prefix 的联合条件分布。没有发现区分器，但完整模拟缺少分布引理。
5. **结论边界。** 这是完成的非作者密码学审查，审查过程门禁可记 `PASS_WITH_SCOPE`；证明结论按范围分别为单 key/单次调用 CONDITIONAL、两个 key-only pool CONDITIONAL、adaptive pool 与 shuffle/output/crash OPEN。没有通过整池或端到端安全签核。

### 6.2 两种池的条件优势界与失败路径限制

审查者复核的 key-only projection 界分别为：N=128 `1,639,424·ε_G + ε_RNG_joint`；N=256 `6,719,104·ε_G + ε_RNG_joint`。若把实际信道观察纳入，另需明确定义/量化 `ε_channel`；理想私有认证信道模型下可单独置零。该式要求每槽目标 roots 在给定 α 向量及非 key preprocessing tape 条件下独立、fresh 且不重用，并要求被替换池后的 package/manifest digest 仅为池后处理。成功轨迹的槽数不能代替 abort/crash 各 prefix 的 consumed/unused/rejected slot 账本；该账本仍 `NOT_MEASURED`。

## 7. 本轮检查命令与结果

工作树：C:\Users\28641\.codex\worktrees\m6b-project-select5-s20\MoE_Top-k_Reproduction_Evaluation
分支：codex/m6b-project-select5-s20
基线 HEAD：c3926c68fd14f270faa8b55234311071947fa080
运行时变更：无。

实际检查：

1. PowerShell 解析 S18 manifest，对外部 source root 中 72 个文件逐项调用 Get-FileHash -Algorithm SHA256；将路径 slash-normalize 后以 StringComparer.Ordinal 排序，拼接 path + TAB + lowercase hash + LF 并 SHA-256：更正后的命令 exit 0，72/72 匹配，canonical c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。
2. Get-FileHash 重新读取 S18 manifest、S19 report/gates/source manifest；哈希记录见配套 manifest。
3. rg/Get-Content 针对冻结快照审阅 DCF、UCMP、score input、Select5、parallel shuffle 与材料 store 的实际源函数；没有执行程序。
4. 正式/功能测试：NOT_RUN（无 runtime 变更，按任务不重跑 S18 矩阵）。
5. 构建/二进制：NOT_RUN / NOT_MEASURED。
6. 正式 LAN/WAN：NOT_RUN；V3 OPEN，V4 NOT_ALLOWED。
7. diff whitespace、冻结目录、禁止文件检查在交付前执行并记入 manifest。

失败尝试保留：最初复跑源哈希脚本时把 `snapshot_root` 误读为 S18 manifest 顶层字段；实际字段位于 `receipt_object.snapshot_root`。该错误使 72 个路径组装全部失败，未产生任何有效哈希比较。修正 JSON 字段路径后重新逐文件计算，得到 72/72 匹配及声明 canonical 值。该项是检查脚本路径错误，不是源文件哈希不匹配。

复核者可用 S18 manifest 与 external snapshot 重复第 1 项。论文页码和定理文本请对照上文链接中的作者稿；S20 不添加或改写本地论文副本。

## 8. 下一步最小可执行证明义务

非作者审查已经完成；下一步不是重复审查，而是由证明作者或新证明负责人给出可供进一步独立检查的完整联合 sampler/game。使用 S18 manifest 固定的六个核心运行时源文件和两个精确材料 schema，完成以下工作：

1. 将每个 UCMP slot 形式化为 (alpha_j, masks_j, frames_j, K_b,j, Eval pair, c_j) 联合随机变量，逐方证明固定 alpha_j 后 target roots 与 pre-call state 条件独立。
2. 明确哪些分布量进入 L，给出一个不读取 real transcript 的 leakage sampler；证明本地 mask share 可以由 simulator 以正确条件分布产生，特别检查 carry output 到 sign-frame 的后续依赖。
3. 针对 N128、N256 分别以固定槽序逐槽作 key hybrid；写出对每个 slot 的 reduction advice、执行时间 T′ 和失败前缀如何停止/消耗 slot；复算优势项。
4. 完整模拟两次 Eval 的传输 shares、opened c、自适应地址、未使用材料和 lease markers。
5. 证明 common Pi/Pi^-1 与独立 sigma/tau/a/e/r 的 P0、P1 联合条件分布；逆路由本地输出 share 条件于 O_b；对指定 crash prefix 说明 orphan 8-byte share 的模拟。
6. 对上面新写的 sampler/game 再做一次独立审查，并报告审查者身份、利益关系、裁决和范围。此项完成前，secure alias 仍为 NO_GO。

## 9. 历史与名称边界

S1/S2 对 2024 论文五迭代 BB90 的来源结论未改变；本路线是项目派生稳定 Select，未证明与 BB90 或 BMW16 等价。S18 功能接收不等于 S20 安全通过。S20 的非作者复核只对单次 UCMP 条件结论及 key-only 池界给出 CONDITIONAL，并确认 continuation 与 shuffle/output 首个缺口仍 OPEN。算法五个迭代、最多九个明文比较依赖层、完整 raw-score→mask 在线交换继续分开；S20 不重算端到端 exchange count。N≤256 的功能证据只继承 S18/S15 的既定范围，不扩展为新 revision 的功能接收。

## 10. S20 对 S19 推导步骤的逐项裁决与首个未闭合跳转

### 10.1 单次 UCMP 条件替换

在**单次调用、调用前状态固定**的范围内，本轮逐式反查 S19 的 key-only 提升，没有找到数学反例。可核验对象为：

- S15 `dcf_bcg21_fig1_aes128_v1.cpp` 的 `generate()`、`eval()`、`serialize()` 与 `deserialize()`；
- `protocol_i_ucmp.cpp` 的 `ProtocolIUcmpMaterial` 构造、`eval_strict_lt()` 和 `ProtocolIUcmpPartyMaterial::eval_strict_lt()`；
- score adapter 的 carry/sign operand construction 和各方 frame 写入。

BCG+21 Definition 2 的理想算法只输出单方 key，默认泄露为输入/输出群；Appendix F.1 的 Hybrid `Hyb_0…Hyb_{n+1}` 对每个固定 `b, alpha, beta` 给出同一 key-only 界，并以完全随机、与参数无关的结构化 key 为末态。作者稿相应原文可在 [Microsoft Research PDF 的定义与定理位置](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf) 及 Appendix F.1 逐页复核。故若 `A_b` 是 key 生成前的状态，并满足 `K_b ⟂ A_b | alpha`，则对任意支持点 `(alpha,a)` 逐点替换、再对 `(alpha,A_b)` 的联合分布取期望是合法的；这里不要求 alpha 与 masks、输入份额或 z 边缘独立。`c` 必须是固定 `(alpha,z)` 下由 correctness 确定的布尔值，不能是评估后的另一个 key-dependent 泄露。

对相同 `K_b` 的 `Eval(x), Eval(y)`，两个结果、`u_b` 和接收 share `c-u_b` 都是 `(K_b,x,y,c)` 的确定性后处理。因此对**这个单调用投影**使用一次 key replacement 足够，不应另加两次独立 PRG/key 项。它仍依赖 S19 的理想随机 tape 替换假设和 AES PRP 假设；`epsilon_RNG_joint` 没有量化，S20 也没有给出 concrete advantage。

非作者审查者对上述单次调用范围作了条件性确认，但未审查/确认完整 continuation。适用的严谨前提必须写成按目标槽 `j` 条件于 `alpha_j` 和完整 pre-Eval 辅助状态 `A_{b,j}`，且排除目标 key bytes 与任何由该 key Eval 产生的 continuation。若某份本地 digest/manifest 包含目标 key，则它只可作为完整池混合中对 pool 的后处理；不得作为“独立于目标 key 的辅助状态”固定后再套用单 key 定理。

### 10.2 第一个未闭合的联合混合跳转

第一个未闭合处不是已发现的 DCF 区分器，而是 S19 从单槽 key-only hybrid 推到真实全协议视图时的 `H_pool^0 → H_pool^1` 继续分布论证：替换槽 `j` 后，必须重新生成或证明保持同分布的后续槽前态与后续线上 leakage，包括 carry/sign adapter 后继 operand frames、本方 mask/material shares、下一调用的 `(alpha,z)`、活动 seed/endpoint transcript、输出 share 和 crash/lease 前缀。S19 把“future transcript 是 continuation 而不是固定 advice”列为要求，却没有给出一个完整的 `LeakageSampler_b(input_share_b, ideal_output_share_b, public_params; coins)`，也没有证明该 sampler 在真实 hybrid 与目标 key 已替换的 hybrid 中对这些联合量产生相同条件分布。

因此，当前证据只能支持如下分层裁决：

| 推导层 | S20 非作者复核裁决 | 缺口 |
|---|---|---|
| 实际 M2B1/v1 单方 key-only 分布 | `CONDITIONAL` | Appendix F.1 的逐固定参数 bound 可提升到独立于目标 roots 的 pre-Eval advice；依赖理想联合随机 tape 与 AES-128 PRP |
| 单次 UCMP：同 key 双 Eval 与 opened bit | `CONDITIONAL`，非作者已复核该范围 | 必须在固定正确性决定的 `c` 后构造 `c-u_b`；限单次调用投影 |
| 全池 used/unused key projection | `CONDITIONAL`，仅 key-only 投影 | `Σw·epsilon_G` 算术与槽数经复核；需条件独立 roots 与理想 tape；完整 continuation sampler 及 abort/crash consumed-slot ledger 仍缺失/`NOT_MEASURED` |
| 自适应访问与整个协议 transcript | `OPEN/NO_GO` | 当前公开前缀决定地址这一事实不足以证明后续 L 的条件分布在 key replacement 下不变 |
| M2CM forward/inverse/output/crash 联合视图 | `OPEN/NO_GO` | 单方向 `sigma_b,tau_b` 边缘均匀不推出 `(Y,Q,Pi,Pi^-1,DCF masks,output share,lease/crash)` 联合条件分布；S19 的采样步骤仍是框架而非闭合的概率实验 |

**最小可执行后续工作：**先形式化 carry→sign 的变量关系和两种实验中的条件分布，构造 `LeakageSampler_b(input_share_b, ideal_output_share_b, L; coins)`，证明它能在目标 carry key 替换前后正确生成 carry 本地 share、lift、后续 sign operands/masks 和 wire frames。然后按固定 slot 顺序将该 continuation 扩展到 N=128 与 N=256 的 used/unused pool、adaptive access、lease/crash prefix；最后将真实 M2CM 的共同 Π/Π⁻¹、Y/Q、membership、inverse route、output share 和 abort/crash 联合进去。符号分布/小有限域枚举仅用于发现反例，不替代计算安全证明。这个结果仍须由非作者复核。

### 10.3 两种池的算术与适用边界

按 S19 槽位定义独立复算：

- N=128：`2N=256` 个 34-bit score key，`5*C(128,2)=40,640` 个 40-bit Select key，`N=128` 个 40-bit membership key；总数 `41,024`，`Σ_j w_j=1,639,424`。
- N=256：`2N=512` 个 34-bit score key，`5*C(256,2)=163,200` 个 41-bit Select key，`N=256` 个 41-bit membership key；总数 `163,968`，`Σ_j w_j=6,719,104`。

在以下**显式额外前提**下，key-only projection 的逐槽求和式为 `Σ_j w_j·epsilon_G + epsilon_RNG_joint(schema,T)`：各槽 roots 使用不重用且条件独立的理想随机 tape；所有 slot 类型确实都是表中位宽的 M2B1/v1 key；固定顺序 hybrids 可将别的槽作为 reduction 的随机辅助状态；未访问字节与 package/segment digest 仅为已替换 key pool 的后处理；one-shot lease/失败 prefix 不创建额外 key 或重放。该式不能单独证明真实 transcript 的自适应联合安全。对 abort/crash 各 prefix 的已用、拒绝、剩余槽枚举是 `NOT_MEASURED`，不可由两个成功样例外推。

### 10.4 输出与失败边界

沿用 S18 已测案例的 scoped `no-complete-mask`：某些后期 crash prefix 可留下 P1 的 8-byte 本地 XOR share；这不是完整 mask。S20 未证明该孤立 share 在所有 crash/restart/retry 和本机管理员视图下的分布，也未证明原子同时发布或 all-or-nothing。任意生产存储、恶意 root、T 与在线方串通和输入相关墙钟观察仍排除或未测。
