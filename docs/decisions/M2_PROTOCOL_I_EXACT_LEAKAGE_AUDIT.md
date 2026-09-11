# M2.17 Protocol I exact leakage audit

状态：**BLOCKED；本审计未授予 exact approval。**

日期：2026-09-11。

本文件是 M2 论文设计门的泄露边界 owner。它比较论文 A 级要求、当前 VFSS 的 B/C 级实际行为和未来候选 D 级功能；候选功能不是已存在实现。

## 1. 结论摘要

论文要求 secure shuffle 同时提供 secret-shared shuffled payload 和公开 masked shuffled list y = π(x) + r。公开 list 中的 r 不能被任一单方单独知道，并且同一 r 还要作为 FSS/GRank 的 secret parameter。

当前 VFSS 的 forward/reverse PS 只证明或实现 secret-shared payload 路径；当前 masked-key opening 是独立的项目扩展，既不是论文 public masked-list，也没有证明使用同一 π 和同一 r。当前材料因此不能通过论文 exact leakage gate。

## 2. 六类视图矩阵

| 视图 | 论文 A 级允许/要求 | 当前 VFSS B/C 事实 | 候选设计 D 级要求 |
| --- | --- | --- | --- |
| P0 | 看到自己的 input shares、自己的随机数、收到的消息和允许的 secret shares；不能单独恢复 π、r、原始 score/index 或另一方 permutation | raw/package/PS shares 与 framed messages；当前没有 public-list 或 GRank-r binding 证明 | 只能看到 local input/material/消息视图；public list 以协议规定方式可见，但不能由其反推 π、原始 index 或 r |
| P1 | 与 P0 对称 | 与 P0 对称；当前 PS 只覆盖 secret-shared payload | 与 P0 对称，且必须通过 simulator/视图证明 |
| P2 | offline dealer 只提供 input-independent correlated material，不能看 input，online 前退出 | package provider 发送 material 后退出；当前 package 没有论文 public-list/GRank correlation 字段 | 生成/分发 π、r、GRank correlation 的 shares；不选 input-dependent 值、不重构、不在线参与 |
| public | 可见 y = π(x)+r 及允许的 shape/metadata；不可见 π、原始 index、未掩码 score/rank | 当前公开的是项目 masked-key vector 和受控 slot/rank reveal；前者不是论文 y，后者需按项目契约单独审计 | 只公开同一隐藏 π 生成的 masked list；slot、phase、width、fingerprint 和 material identity 必须绑定 |
| test-only | 可在 oracle/测试边界重构，不能反映 secure execution leakage | test harness 可验证最终 mask；本地 dealer oracle 可返回 permutation/masks，均不能迁入 secure path | 仅 test-only oracle 可重构指定输出；不得让 secure runtime 暴露 debug transcript 或密钥 |
| persistent | 只保存允许的公共 run metadata；不能保存 raw/key/rank/permutation/r 或 payload | 允许保存 revision、n、K、label、阶段状态、计数等 provenance；禁止持久化敏感向量 | 保持零敏感持久化；失败也不能写入 raw、public-list secret、r、permutation 或完整 payload |

## 3. 字段级差异

| 对象 | 论文 A | 当前 B/C | 候选 D |
| --- | --- | --- | --- |
| raw score | 论文功能以抽象排序键为输入，不公开 raw | P0/P1 持有 Q20.12 raw shares；secure path 不重构 | 继续保持 shares；domain conversion 必须是独立 adapter |
| priority key | 论文不定义项目的 Q20.12 carry/sign adapter | 当前 raw adapter 产生 priority-key shares；masked-key opening 是独立项目阶段 | 若作为 adapter 保留，必须明确不属于论文 public list |
| public list | y = π(x)+r，面向在线 parties 可见 | 无论文意义上的 public list | 必须同时绑定 secret shuffled payload、π 和 GRank 的 r |
| r | 单方未知，作为 FSS secret parameter | node_mask_shares 未证明是 r；M3 DPF masks 不是 M2 的 r | 必须有 correlated-material generation、serialization、consumption 和 negative tests |
| local permutation | 不能泄露 | current PS material 含 local permutation/inverse 的私有材料边界 | 只能保存在对应 party-local material，不进入消息或 public metadata |
| composed permutation | 不能泄露 | current PS algebra可说明 secret-share composition，但没有 public-list binding | 必须证明同一 π 贯穿 public list、secret payload、GRank 和 slot framing |
| rank | stable rank 的方向由论文定义 | current path controlled reveal shuffled slot/rank_P；项目 rank 语义另有说明 | 固定论文 rank、项目 descending rank、tie、padding 和转换 oracle |
| original index | 不得进入 public masked list | secure path 不应公开；旧参考 oracle 可返回，属于 B/test-only | 只作为隐藏 stable binding，不能用于 public mapping |
| selected index | 论文 Fselect 输出 payload share，不要求 public selected index | 项目 output adapter 只能给 mask share | 禁止公开 selected original index |
| final mask | 不是论文原生输出 | 项目 C 级 XOR original-order mask，test-only reconstruct | 作为单独 output adapter 和 leakage gate |
| transport metadata | 论文会议版未提供 VFSS framing schema | 当前 header 绑定 session/fingerprint/n/K/bits/sender/receiver/phase/type/sequence/length/offset | 继续区分 frame/header binding 和 material record binding |

## 4. 当前禁止的 secure leakage

以下对象不得在 secure execution 中打开、发送到未授权角色、持久化或 debug 打印：

- raw score、carry、sign、未掩码 priority key；
- 单边 comparison bit、完整 rank vector、rank share reconstruction；
- original index mapping、selected original index、任一方 local permutation、composed permutation；
- public-list 的 r、GRank secret parameter、node mask share 或 DPF key；
- original-order mask、oracle input、完整 message payload、测试侧 dealer state；
- 可以把 shuffled slot 关联回原输入位置的额外表。

允许的公共对象仅限于冻结契约中的 shape/configuration metadata、阶段状态、计数、失败类别和经审查的 public masked list；项目 D1 的 shuffled slot/rank_P reveal 仍须受独立项目泄露契约约束，不能被误写成论文的 public list。

## 5. 同一置换与同一 r 的审计义务

仅有相同长度、相同排序结果或 test oracle 中的相同顺序，均不能证明同一置换。后续实现必须证明：

1. public list 的第 i 个 slot 与 secret shuffled payload 的第 i 个 slot 使用相同 π；
2. rank/GRank 消费的 r_i 与 public y_i 的 r_i 是同一 correlated material；
3. chunk offset、padding、slot 编号和 reverse adapter 不改变该对应关系；
4. 任一单方视图不能从消息、local material、public list 或错误路径恢复 π、r 或原始 index；
5. P2 的材料生成不依赖输入值，且 P2 在 online 前退出；
6. replay、wrong party、wrong phase、wrong slot、wrong width、wrong count、truncation、trailing bytes、material reuse 均 hard-fail。

当前 package 的 material record 可绑定 party、slot、stage、edge endpoints、count 和 one-shot；transport header 可绑定 session、fingerprint、n、K、bits/width、sender、receiver、phase、type、sequence、length 和 chunk offset。这些字段 binding 本身不等于 public-list/GRank correlation proof。

## 6. 当前 transcript 的泄露含义

当前 raw-score path 的 causal barriers 是：

| 步骤 | 当前对象 | 泄露判断 |
| --- | --- | --- |
| O0 | P2 package 与 exit | input-independent，属于 offline |
| R1 | carry/sign adapter | share-only；不得打开 raw |
| R2 | forward two-pass PS | share-only；不产生 public masked list |
| R3 | masked-key exchange/open | 当前项目公开扩展，不能称论文 y |
| R4 | shuffled slot/rank_P reveal | 受控项目 reveal；不得带原始 mapping |
| R5 | reverse PS 与 original-order mask | share-only；最终 mask 只在 test harness 重构 |

R3 是独立 causal barrier，不能通过字段重命名消失。若将其保留，当前 core 仍是 4 rounds；若把它融合进三轮候选，必须提交新的逐消息 transcript、材料生成和视图证明，不能只改 round counter。

## 7. 论文 core 与项目 output 的边界

论文 Fsort/Fselect 的原生输出是 sorted vector 或 selected payload 的 additive shares。项目需要的是原输入顺序 XOR Top-K bit-mask share。两者之间的稳定同分、rank 方向、padding 和 reverse mapping 都是项目 adapter contract。

因此以下陈述均禁止：

- “当前 original-order mask 就是论文 Fselect 输出”；
- “当前 masked-key opening 就是论文的 y = π(x)+r”；
- “当前 node_mask_shares 就是论文 r”；
- “M3 的 DPF rank mask 可以补齐 M2 的 r”；
- “通过公开 index 或 selected index 可以验证同一置换”。

## 8. 证据缺口与状态

| Gate | 状态 | 缺口 |
| --- | --- | --- |
| 固定论文文件/hash | PASS | 两份本地论文按 docs/PAPERS.sha256 校验通过 |
| public y 的语义 | PASS（论文）/BLOCKED（当前） | 当前实现没有该功能 |
| same-permutation binding | BLOCKED | 没有 public slot、secret payload、GRank material 的统一代数证明 |
| correlated r | BLOCKED | 当前 node masks 没有论文 r 的关联和单方未知性证据 |
| 3-round causal transcript | BLOCKED | 会议版未给完整实现；当前独立 masked-key barrier 仍在 |
| P2 offline-only | PARTIAL | 当前工程边界具备，论文兼容 correlation 尚未实现 |
| leakage matrix | PARTIAL | 当前 C 级边界可列出，候选 public list 未实现 |
| conformance/differential/E2E | BLOCKED | 没有新 primitive，阶段一 24/24 不能替代 |
| M3 regression | PASS（现状） | 当前 M3 未调用 M2 pipeline/shuffle symbols；不解锁 M2 exact |

缺口中同一置换、同一 r 和三轮 transcript 是核心功能/安全缺口，故总状态为 BLOCKED，不是 PARTIAL。

## 9. 标签政策

在所有核心 gate 和测试证据通过前，禁止使用：

- agarwal_protocol_i_exact
- agarwal_protocol_i_exact_mask_output
- paper_3_round_exact
- secure_shuffle_complete

仅允许使用研究候选标签 agarwal_protocol_i_paper_compatible_3round_candidate
和 m2_protocol_i_paper_compatible_core_design。当前已验证 C 级标签
m2_protocol_i_modular_6round_mask_output 与
m2_protocol_i_raw_score_input_modular_8round_mask_output 保持原样。

## 10. 审计结论

当前 M2 C 级 secure path 没有新增可接受泄露，但也没有实现论文所要求的 public masked-list。设计门结论为 **BLOCKED**：不批准 M2 paper-exact 实现、不批准 exact label、不批准以 M3 结果或参考 oracle 替代缺失证明。

重新审查至少需要：独立 primitive、same-permutation algebra、correlated-r material/party view、完整三轮 transcript、六类视图审计、shuffle conformance、negative cases、oracle differential、独立进程 E2E 和 M3 regression。
