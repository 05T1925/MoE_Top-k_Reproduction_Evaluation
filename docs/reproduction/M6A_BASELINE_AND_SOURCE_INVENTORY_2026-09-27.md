# M6A-P0：基线、接口与资料盘点

盘点日期：2026-09-27
范围：M6A P0。仅核实 Git 基线、M5 G3 接收记录、现有接口和资料来源；不决定 AAV86 算法变体、稳定键编码或 CA 组合设计，不实现 secure runtime。

## 1. Git 起点与工作区保护

| 检查项 | P0 起始值 | 核验 |
| --- | --- | --- |
| git status --short | ?? siamjdiscrmath.pdf | 唯一未跟踪项；按要求原样保留 |
| git status -sb | ## feat/m6a-performance-evaluation | 分支名符合预期；输出没有 ahead/behind 标记 |
| git branch --show-current | feat/m6a-performance-evaluation | 符合预期 |
| HEAD | c3926c68fd14f270faa8b55234311071947fa080 | 与预期基线完全一致 |
| main / origin/main | c3926c68fd14f270faa8b55234311071947fa080 | 两个引用均指向 HEAD |
| 当前 M6 分支与 main 的 merge-base | c3926c68fd14f270faa8b55234311071947fa080 | 当前分支正位于该 main revision |

起始最近五条提交：

1. c3926c6 Merge pull request #27 from 05T1925/docs/m5-g3-closeout
2. ba7efd3 docs: clarify G3 cost smoke counters
3. c72039c docs: refresh M6A kickoff gate after G3
4. 89147bc docs: record M5 G3 receiver closeout
5. 9b3ce37 Merge pull request #26 from 05T1925/docs/m5-g3-receiver-checklist

HEAD 是 PR #27 的 merge commit，第一父提交为 9b3ce3747b1734602e3edf4c644ae1b6da52e8c1，第二父提交为 ba7efd39af5d6cf0d2483c61ee5b2b78fedbca73。也就是说，M5 G3 接收签收已经在当前 main 上，当前 M6 分支与该关闭提交一致。feat/m6a-performance-evaluation 当前没有配置上游跟踪分支；main 与 origin/main 对齐。

根目录 siamjdiscrmath.pdf 实测为 535 页。它是用户指出的 SIAM 书目 PDF，不是 BB90 论文正文，未纳入论文清单或哈希表，仍保持未跟踪状态。P0 新取得的 AAV86 PDF 位于被 .gitignore 忽略的 Papers/，没有进入 Git 状态。

## 2. M5 关闭和 G3 接收结论

接收方签收记录：[M5 → M6A G3 接收报告](../reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md)。

- 接收结论：G3_ACCEPTANCE = PASS；接收方核验日期 2026-09-26。
- 接收所审 revision：main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1。
- 该 revision 的独立干净检出和构建通过，CTest 为 41/41；F1 差分为 140/140，另有独立进程 E2E、成本 smoke 和 fail-closed 检查。以上是接收报告中的历史结果，不是本轮重新运行。
- M5、M5-FIX-F1 已关闭；F2 / G3 = PASS。当前 c3926c6 主线 merge PR #27，包含上述接收签收 revision。
- 标准 Protocol III raw-score-to-mask 入口在接收时可用，输入为 P0/P1 各自的 Q20.12 原始分数加法份额，输出原输入顺序的 XOR mask 份额；P2 在线静默。
- 证据限制保持不变：AUTHOR_EXACT = NOT_PROVEN；Theorem 4.2 精确逻辑成本匹配为 NO；既有 H1 比率约 1.42–1.44、差值 254n bits；common-mask optimization 未实现；field DPF formal proof 未完成；通用 ring-to-field 与 field-to-XOR 转换未实现。M5 工程接收通过不构成作者精确复现证明。

## 3. Protocol I / III 当前接口清单

依据当前 VFSS 头文件、实现和 M5→M6A handoff 静态核对。轮数在此按接口记录的在线因果轮数计；离线发材不计为在线轮。

| 接口 | 输入 → 输出 | 阶段/在线轮数 | 接口边界 |
| --- | --- | --- | --- |
| Protocol I 核心：VFSS/include/moe_topk/protocol_i_parallel_shuffle.h 中的 protocol_i_parallel_shuffle_three_round_party | session/fingerprint/material/party 参数、一次性 party material、ProtocolIBlock192 输入份额、3 个 round fd → core 含 shuffled_share、public_masked_records、public_ranks、sorted_share；另有分轮通信计数 | R1、R2、R3；3 轮 | 三轮 shuffle + C-INSTANTIATION 核心，不接收 Q20.12 raw-score vector，也不直接给出统一原输入顺序 Top-K mask。证据标签为 C-INSTANTIATION，AUTHOR_EXACT 仍 NOT_PROVEN。 |
| Protocol I raw-score 输入适配：VFSS/include/moe_topk/protocol_i_score_input.h 中的 protocol_i_raw_score_input_party | logical_n 个 uint32_t raw-score 加法份额、party package、2 个 stage fd → padded_n 个 uint64_t priority-key 份额 | carry 与 sign 两阶段；2 轮 | 仅处理项目的 Q20.12 raw word 到 priority-key 输入的窄适配，不是通用 ring/field 转换，也不独自输出 mask。 |
| Protocol I mask pipeline：VFSS/include/moe_topk/protocol_i_pipeline.h 中的 protocol_i_priority_pipeline_party | padded priority-key 份额、package、shuffle material、forward/cmpagg/rank-reveal/reverse fd → 原输入顺序 XOR mask share | forward 2 + CmpAgg 1 + rank reveal 1 + reverse 2；6 轮 | 输入已是 priority-key 份额；按现有 API 由调用方组合 raw-score adapter。 |
| Protocol I raw-score → mask 工程组合 | score-input 两轮 + priority pipeline 六轮；历史 E2E 名称为 moe_topk_m2_protocol_i_modular_e2e_test | 总计 8 轮 | 是组合测试入口/工程基线，不是上面的三轮 shuffle 核心。M6A handoff 记录此测试依赖可选 EMP，当前 OFF 构建中不可用；本轮未运行。 |

Protocol III 的当前接口：

| 接口 | 输入 → 输出 | 阶段/在线轮数 | 接口边界 |
| --- | --- | --- | --- |
| 标准 raw-score mask：VFSS/include/moe_topk/protocol_iii_raw_score_mask.h 中的 protocol_iii_raw_score_mask_party | logical_n 个 uint32_t Q20.12 raw-score 加法份额、绑定的一次性 material 和 score/GRank/routing fd → logical_n 个原输入顺序 uint8_t XOR mask share | score adapter 2 + GRank 1 + DPF routing 1 + output 0；总计 4 轮 | 这是标准 raw-score→mask 完整工程入口；P2 在线静默。它是 M5 F1 bit-mask C-INSTANTIATION，不等于通用 field-valued Fselect 的输入/输出适配。 |
| GRank：VFSS/include/moe_topk/protocol_iii_grank.h 中的 protocol_iii_grank_party | padded priority-key additive shares 与本方 package → logical_n 个 Z_(2^rank_bits) additive rank shares | 1 轮 | 只输出 rank shares，不做后续 DPF routing 或最终 mask；不重构 priority key 或 rank。 |
| 两轮 field Fselect/Fsort：M5 handoff 中的 ProtocolIIITwoRoundParty::consume_round2 与 consume_round2_sort | priority-key shares 加已 field-share 的 encoded records → 单个 field record shares 或 rank-order field-record shares | R1/R2；2 轮 | 两轮是 field-valued 核心控制接口，不是 raw-score→原顺序 mask API；调用者承担 key/record consistency。 |

### 全对全图与 AAV86 自适应图

当前 Protocol I CmpAgg 按 n 个元素的全对全边准备资料；protocol_i_cmpagg_eval_party 在 VFSS/src/moe_topk/protocol_i_cmpagg.cpp 中要求 edge material 数为 n(n−1)/2。Protocol III GRank 同样按 logical_n 全对全 CmpAgg 图消费 edge material。当前接口、材料和 round counters 因而对应全对全比较图。

在当前 VFSS 活动实现中没有 AAV86 自适应图或相应 runtime 入口；AAV86 只是 M6A 目标/计量字段和本地参考树中的内容。未来 AAV86 图的 pivot/local-rank 依赖、逐轮确切边集、离线材料生成时机、edge/stage binding 和泄露边界均须在 P1 单独审查，不能由现有 clique 材料推定。

## 4. 论文和本地副本状态

### Agarwal CCS ’24

书目：Amit Agarwal 等，Secure Sorting and Selection via Function Secret Sharing，ACM CCS 2024，pp. 3023–3037，15 页，DOI 10.1145/3658644.3690359。MIT DSpace 将仓库对象标为 final published version，并列出 1.16 MB PDF；出版会议记录也确认 CCS ’24。[MIT DSpace 记录](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)；[ACM DOI](https://doi.org/10.1145/3658644.3690359)。

- 实际本地文件：Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf。
- 页数：15；文件大小：1,249,281 bytes。
- 实测 SHA-256：18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c。
- 哈希与 docs/PAPERS.sha256 中已有 Agarwal CCS ’24 哈希完全一致；该清单原先记录的 basename 较短，缺少“via Function Secret Sharing”。P0 已把该行路径修正为当前实存文件名，哈希值未变。
- 本阶段只确认它是 CCS ’24 会议版，不将会议版未包含的证明或细节视为已知，也不展开算法推导。

### AAV86 原始论文

书目候选经作者论文目录、作者托管 PDF 和 Tel Aviv University 机构记录交叉确认：Noga Alon、Yossi Azar、Uzi Vishkin，Tight Complexity Bounds for Parallel Comparison Sorting，27th FOCS 1986，pp. 502–510，9 页，DOI 10.1109/SFCS.1986.57。[Princeton 作者托管 PDF](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf)；[Princeton 作者论文目录](https://web.math.princeton.edu/~nalon/PDFS/publications.html)；[Tel Aviv University 机构记录](https://cris.tau.ac.il/en/publications/tight-complexity-bounds-for-parallel-comparison-sorting)；[IEEE DOI](https://doi.org/10.1109/SFCS.1986.57)。

- P0 前 Papers/ 和仓库文件名检索未发现 AAV86 原文；作者托管地址可访问，故按要求保存到 Git 忽略目录。
- 本地副本：Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf。
- 获取日期：2026-09-27；作者托管 PDF；PDF 内 IEEE 1986 标识，实际 9 页，对应书目 pp. 502–510。
- 文件大小：1,063,055 bytes；实测 SHA-256：322f1bd761a987fd09e6b59b3a3ae77d6e4b1ca9dcb1c2765e45ac4a2a7e2b83。
- 该文件位于 .gitignore 忽略的 Papers/；未将 PDF 正文加入 Git，仅更新来源清单与哈希登记。

### BB90（仅做 M6B 资料状态核对）

参考清单登记的书目为 Béla Bollobás、Graham Brightwell，Parallel Selection with High Probability，SIAM Journal on Discrete Mathematics 3(1):21–31 (1990)，DOI 10.1137/0403003。[SIAM 出版页面](https://epubs.siam.org/doi/10.1137/0403003)。

- Papers/ 现存文件中未发现 BB90 正文 PDF；仓库搜索也未核实到另一份 BB90 正文副本。因此本地正文状态为未取得，页数和哈希均未登记。
- 根目录 siamjdiscrmath.pdf 为 535 页书目 PDF，不是该论文正文；按要求保留原样且不作为 BB90 来源。
- 当前未核实到作者代码或已确认的 BB90 实现来源；不据此断言公开世界不存在。P0 未下载或分析 BB90。

## 5. Agarwal full version、作者代码及本地参考树

- Agarwal 会议版来源可确认；本次盘点未核实到经作者、作者机构、出版方或正式 artifact 页面确认的 full version。PROJECT.md 既有状态“尚未取得作者所称 full version”与本次检索相符；这只是本次未核实到，不表示公开世界不存在。
- 本次没有找到可由论文作者、作者机构、出版方或正式 artifact 来源确认、且明确对应该论文的作者发布代码。不得把参考树中同名实现标为作者实现。
- 当前机器上的 Agarwal_TopK/、ADSMPC/、CipherGPT/ 三个本地参考目录均存在并被 .gitignore 忽略；Agarwal_TopK 下有 protocol1_ca 和 protocol3_ca 的 AAV86 源码/测试路径。三个目录根部均无 .git 元数据，故本次不能核验其上游 URL、revision 或许可证是否与作者发布版本对应。
- PROJECT.md 的当前文本称本 checkout 未安装上述三个参考树，与本机实际目录状态不一致。P0 记录该差异；没有移动、删除或修改参考目录。
- 可定位的本地 AAV86 参考代码入口包括 Agarwal_TopK/protocol1_ca/src/aav86.cpp、protocol1_ca/src/ca.cpp、protocol3_ca/include/aav86.h、protocol3_ca/src/aav86.cpp 与 protocol3_ca/tests/aav86_test.cpp。它们仅登记为本地参考行为；P0 未执行或审查正确性、安全性和性能。

## 6. 用户提供的历史资料

- C:\Users\28641\Downloads\parallel_shuffle_3round_reference.py 存在，大小 13,291 bytes，SHA-256 为 404a90c600d9b9c97b4fc17957c4aa67df027435b7e673cffe9d6c3cead8779e。
- 仅静态读取其开头说明，脚本自称是“newly derived reference construction”，并明确不是恢复的 Agarwal 作者代码；所述模型含 2+1 preprocessing/dealer 与其自身的测试说明。故分类为“历史设计线索 / 用户提供的本地参考脚本”，不作为 AAV86 自适应图、离线边材料、安全证明或实际验证行为的证据。本轮没有执行该脚本。
- 本轮当前消息没有可访问的聊天截图附件或本地截图路径；截图状态为不可核验，未把聊天截图作为证据。

## 7. P1 继续审查所需的精确材料

P0 找到足以开展 P1 阅读的论文与代码位置，但以下内容应在 P1 由论文原文、项目冻结语义和当前源码逐项对照：

1. AAV86 原文：Alon–Azar–Vishkin FOCS 1986，§2.3（printed pp. 505–506）和 §3.1 / Theorem 3.1（printed pp. 506–507）；再核对 §3.2–§3.6（printed pp. 507–509）中适用的前提、复杂度与证明范围。对应 PDF 页为 4–8。
2. Agarwal CCS ’24 会议版：§5.1–§5.4、printed pp. 3033–3035（PDF pp. 11–13），重点定位其 AAV86 算法描述、Valiant 到 CA 的转换以及 Theorem 5.1；不要把 theorem 或会议版没有给出的内容外推为已知。
3. 当前稳定 Top-K 项目语义：docs/decisions/M1_SCORE_SEMANTICS.md、VFSS/include/moe_topk/score_semantics.h、VFSS/include/moe_topk/topk_oracle.h；并核对 Protocol I priority-key adapter 和 Protocol III mask 输出边界。
4. 当前协议代码入口：本报告第 3 节所列 Protocol I shuffle/input/pipeline 和 Protocol III raw-score mask/GRank 头文件与实现，以及 docs/handoffs/M5_PROTOCOL_III_TO_M6A_HANDOFF_2026-09-25.md、docs/handoffs/M5_TO_M6A_G3_RECEIVER_CHECKLIST_2026-09-25.md。
5. 本地参考行为：第 5 节列出的 AAV86 源文件与测试、各自可核验的来源/版本记录；核实之后再决定其证据用途。用户脚本仅提供历史设计线索。

P0 没有选择 AAV86 算法变体、稳定键编码、Protocol III + AAV86 轮数/CA 组合、安全协议或离线材料设计。

## 8. 尚未核实的资料缺口与阶段判断

- Agarwal full version 和可归属作者的代码：本次未核实到。
- 当前三个大型参考树的上游 revision/许可证：目录存在，但本地没有根级 Git 元数据，来源链未闭合。
- 本轮聊天截图：没有可访问文件，无法核验。
- M6A 自适应图的精确边生成/预处理时机、edge/stage binding、local-rank/bucket 泄露边界和 Protocol III 组合轮数：均留待 P1，P0 不推断。
- M6A 运行时、conformance、oracle differential、独立进程 E2E 和性能矩阵：本轮没有执行，也没有新的实测数据。

进入 P1 “AAV86 算法与稳定 Top-K 语义决策”的证据基础：**可以开始 P1 的原文阅读、语义对照和设计审查**。依据是已确认的 9 页 AAV86 原文、15 页 CCS ’24 会议版、冻结的项目 score/tie/mask 语义、实际 VFSS 接口和通过的 M5 G3 接收交接。该证据基础不支持作者精确复现声明，也不允许直接实现；P1 仍须完成算法步骤、稳定同分映射、CA 变换及两条协议组合的独立论证。
