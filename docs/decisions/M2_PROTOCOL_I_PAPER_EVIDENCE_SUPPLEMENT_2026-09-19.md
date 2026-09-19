# M2 Protocol I 论文证据补充与交接门

日期：2026-09-19

适用代码基准：`30df4f09836a4ff38c83e87e04e29048f405022c`（本次仅新增文档；最终交接 revision 以本分支提交为准）

前置决策：`docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md`
结论：`DESIGN_BLOCKED`、`IMPLEMENTATION_NO_GO`、`M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`

## 1. 范围与不变项

本补充只整理新增的可定位论文证据、资料缺口和下一位执行者的取证顺序；不修改任何
`VFSS/` 源码、既有实现标签、在线轮数或泄露结论。它不是实现授权，亦不将项目扩展
重命名为论文原生协议。

证据类别沿用仓库规则：A=论文定义，B=本地参考行为，C=项目扩展，D=待验证设想。
“已知 FSS 通常做法”不能补写为 Protocol I 的 A 类 transcript。

## 2. 本次已核验来源

| 来源 | 身份与核验 | 可用范围 |
|---|---|---|
| Agarwal 等，*Secure Sorting and Selection via Function Secret Sharing*，CCS 2024 会议版 | `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf`；SHA256 `18faf63eaa7923eef715a6eb9d5d526fe04dc69700b133c3e94de935f68c01c`；15 页（论文页 3023--3037） | Protocol I 的唯一 A 类定义来源 |
| Gupta 等，*SIGMA: Secure GPT Inference with Function Secret Sharing* | `Papers/Gupta 等 - 2023 - SIGMA Secure GPT Inference with Function Secret Sharing.pdf`；SHA256 `3a2989f1da36bda9e2e9b4b6a2d482e33aeff15b223e18a2d1aa5ebcbf1fea0d` | FSS Gen/Eval 与预处理模型的 B 类对照，不定义 Protocol I |
| 公开检索（2026-09-19） | 作者/机构出版页、ACM/CCS 元数据、IACR/GitHub 检索 | 仅确认公开入口与“未取得 full version”，不把检索未命中当作不存在证明 |

PDF 页码下文采用“PDF p.X / 论文 p.YYYY”。会议版本在 Theorem 4.1 的证明处指向
full version；本仓库和本次公开检索均未取得其正文或独立逐消息 transcript。

## 3. 九项问题的证据矩阵

| # | 结论 | 证据类别与定位 | 仍缺什么 / 是否可解锁 exact |
|---:|---|---|---|
| 1 | Protocol I 是 2+1、Shuffle + CmpAgg、3 online rounds；但三轮的逐消息 sender、receiver、frame、因果依赖未在会议版给出。 | A：Table 1（PDF p.3 / 论文 p.3025）；Theorem 4.1（PDF p.9 / 论文 p.3031）。 | **BLOCKED**：取得 full version/作者确认或其他一手 transcript 前不得实现 paper-exact。 |
| 2 | `r` 是 secure shuffle 输出的私有随机 mask 向量；公开值为 `π(x)+r`，且任何单方不知道全部 `r`。`r` 被作为 FSS gate 的秘密参数使用。 | A：§2.4（PDF p.6 / 论文 p.3028）。 | **PARTIAL / BLOCKED**：具体谁生成、每方持有的份额/keys、何时消费、与组合 permutation 的精确关系未给出。 |
| 3 | 角色层面：P0/P1 是 online parties；P2 是 offline dealer。Dealer 离线发送 correlated randomness 后在线静默。 | A：引言（PDF p.1 / 论文 p.3023）、§2.1（PDF p.5 / 论文 p.3027）、Figure 1/2（PDF p.4--5）。 | **PARTIAL / BLOCKED**：逐轮 party view、公开 carrier、密钥字段和接收者均未确认。 |
| 4 | 论文对 secure shuffle 描述的是一个 list 的 `π(x)+r`；排序功能把 key 与对应 payload 一起排序。 | A：§2.4（PDF p.6 / 论文 p.3028）；Figure 1（PDF p.4 / 论文 p.3026）。 | **PARTIAL / BLOCKED**：同一 `π` 是否以何种 material 明确绑定 score、payload 和项目所需 original index，没有 A 类逐项定义。original-index binding 是当前项目 C 类适配。 |
| 5 | 论文原生最终输出是排序/选择后的 **key + payload additive shares**，不是原始顺序 bit-mask；ranking 的 rank 是中间值。 | A：Fsort Figure 1（PDF p.4 / 论文 p.3026），Fselect Figure 2（PDF p.5 / 论文 p.3027）。 | **已确认原生功能**。项目统一的 original-order XOR Top-K mask 是 C 类输出适配，不能写成论文 Figure 1/2 的原生输出。 |
| 6 | 论文的三轮是 Protocol I/Fsort 高层 online-round 声明；会议版并未定义本项目 raw Q20.12 输入转换、原始 index adapter 或 original-order mask 逆路由。 | A：Table 1、Theorem 4.1；C：项目接口与计量规则。 | **不可免费计入三轮**：任何这类 adapter 必须单列 causal round、通信和泄露，不能借三轮 theorem 归零。会议版不足以给出一个更细的“所有转换均不计入”的普遍定理。 |
| 7 | stable rank 的同分规则已定义：按原始序列中较早的相等值优先。 | A：§3（PDF p.6 / 论文 p.3028）。 | **PARTIAL**：会议版未找到 padding/dummy 语义。项目 `padded_n`、dummy 和“小 original index 优先”的接口约定是 C 类，必须保持与论文分开。 |
| 8 | 2+1 模型的 Dealer 为 input-independent offline 阶段提供 correlated randomness，之后在线静默。 | A：引言（PDF p.1 / 论文 p.3023）、Table 1 caption（PDF p.3 / 论文 p.3025）、§2.1（PDF p.5 / 论文 p.3027）。 | **已确认模型层结论**；但这不自动给出本项目 exact package 的字段、生成算法或 transcript，后者仍受 #1--#3 阻塞。 |
| 9 | 会议版 Theorem 4.1 明确把 proof/完整实例化指向 full version。 | A：Theorem 4.1（PDF p.9 / 论文 p.3031）。 | **PAPER_SOURCE_INCOMPLETE**：截至本次检索，未取得可审计 full version、proof 或 transcript；需作者提供或给出稳定一手链接和 SHA256。 |

## 4. SIGMA 对照的正确用法

SIGMA §2.2--2.4（PDF p.3）说明通用 FSS 预处理模型：相关随机性可在输入已知前生成，
`Gen` 产生 material，online `Eval` 由两方使用。它支持“FSS 常见离线 Gen / 在线 Eval”
这一背景理解，且 SIGMA 有公开实现。它**不能**证明 Agarwal Protocol I 的三个 causal
round、`π/r` material、party view 或 inverse routing；这些仍必须来自 Protocol I 的一手
资料。

## 5. 对现有 M2/M3 的影响

下列标签、实现和历史测试状态均不因本补充改变：

| 路径 | 固定标签 | 当前身份 |
|---|---|---|
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | C 类工程基线 |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | candidate；不是完整 mask |
| M2 priority / raw Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` / `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | C 类扩展；含额外泄露/adapter |
| M3 modular / raw | `agarwal_protocol_iii_modular_3round` / `moe_topk_protocol_iii_raw_score_modular_5round` | 与 Route A 隔离的工程基线/扩展 |

阶段三N的 Ubuntu/EMP 测试记录仍是祖先代码的历史证据，详见
`docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md`。本次没有运行 C++、
CTest 或性能测试，故本补充的运行验证为 `NOT_REMEASURED`，所有性能字段保持
`NOT_MEASURED`。

## 6. 唯一允许的后续顺序

1. 获取 full version、作者提供的算法/附录，或可校验的一手逐轮 transcript；记录 URL、
   获取日期、SHA256 和页/算法编号。
2. 先写新的 A/B/C/D evidence table：逐轮 sender→receiver、字段、公开值、每方 view、
   material producer/recipient/consumption、`π/r` 生命周期，以及 key/payload/index 的
   same-permutation 证明。
3. 单独决定原生 key+payload shares 到项目 mask 的适配：若需要输入转换或逆路由，明确
   它们在 paper core 之外，并量化新增轮数、通信和泄露。
4. 仅在 1--3 全部可审计后，新建独立 package/frame/material identity；先做
   conformance、oracle differential、独立进程 E2E 和 leakage/round audit。

在此之前，不得用 `RA6M`/`RA8M`、M3 DPF material、测试层明文重构或本地参考代码来填补
paper transcript 空白。

## 7. 给作者/导师的最小问题集

请索取或确认：

1. Protocol I 的 full version、附录或逐轮 transcript（含三轮的 sender/receiver）。
2. secure shuffle 中 `r` 的生成者、P0/P1 各自得到的 material、`r` 与 `π` 的组合及消费点。
3. key、payload、original index 是否由同一 secret permutation 绑定；若论文未定义 index，
   请明确项目 adapter 的正确组合方式。
4. stable tie 的完整编码、padding/dummy 的定义（若有）以及这些是否影响 round/leakage。
5. Fsort/Fselect 原生 key+payload 输出转成原始顺序 Top-K mask 是否属于论文计数；若否，
   推荐的额外协议/轮数/泄露口径是什么。
