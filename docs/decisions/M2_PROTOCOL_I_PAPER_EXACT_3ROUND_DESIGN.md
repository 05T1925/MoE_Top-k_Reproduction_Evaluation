# M2.17 Protocol I 论文设计门：公开 masked-list 与 3-round core

状态：**BLOCKED；设计门完成审查但未授权实现。**

日期：2026-09-11。

本文件是 M2 阶段二的主设计记录。它闭合“是否具备进入论文兼容设计/实现的充分前提”，不是 M2 运行时代码实现，也不是 M3 设计变更。当前已验证实现仍为 m2_protocol_i_raw_score_input_modular_8round_mask_output，其 2 + 4 + 2 = 8 的工程轮数和 C 级证据边界保持不变。

## 1. 证据等级与源边界

本文件严格区分四类证据：

| 等级 | 含义 | 本门中的用途 |
| --- | --- | --- |
| A | 论文明确写出且已按固定文件校验 | 约束论文输入、输出、角色、轮数和 π(x)+r 功能 |
| B | 本地参考工程或当前 VFSS 的可复核行为 | 说明已有接口能做什么，不能反推论文结论 |
| C | 项目为 Q20.12、原顺序 XOR mask、可审计计量而增加的工程契约 | 说明 M2/M3 现有边界和兼容性 |
| D | 缺失、未公开或本轮未验证的证据 | 形成 blocker；不以假设补齐 |

论文基线是 Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf 和 Papers/协议1shuffle.pdf。docs/PAPERS.sha256 校验通过；未把 conference paper 中引用但本地未提供的 full version 当作逐消息实现证据。

## 2. A 级论文结论

### 2.1 输入、输出与角色

Agarwal 论文的 Fsort/Fselect 理想功能以有序阿贝尔群中的加法 shares 为输入，对重构后的排序键和 payload 进行排序或选择，输出排序向量的 additive shares 或选择出的 payload share。其论文输出不是本项目的原输入顺序 XOR Top-K bit-mask，也不是原始 index 的公开列表。

Table 1 对 Protocol I 给出 2+1 拓扑：两名在线参与方和一个离线 dealer；2+1 不是三名在线计算方。表中同时把 Protocol I 的 online cost 标为 3 rounds。该表的协议输出语义仍是论文的 sorted/selected secret shares，不能直接改写为项目的 original-order mask。

### 2.2 排序、同分与比较域

论文的 rank 方向是“比当前值小的元素数量”；stable rank 还按输入位置处理相等值。论文使用有序阿贝尔群和有界的 unsigned comparison promise；uCMP 要求差值满足约束，不提供把任意 Q20.12 signed 值无条件塞入论文域的自动证明。

因此，本项目的 signed Q20.12 语义、降序优先级、原始 index 升序同分、padding dummy、priority rank 0..n-1 和 34-bit 比较域，均属于项目 adapter 或工程扩展，必须有单独 conformance 和 oracle differential 证据。

### 2.3 公开 masked-list

论文 §2.4 与 §4.1 明确描述 secure shuffle 的额外功能：

- 对共享输入应用隐藏置换，形成 secret-shared 的 shuffled payload；
- 同时产生公开 masked shuffled list y = π(x) + r；
- r 是 random private masks，任意一个参与方都不能单独知道；
- 公开数组可直接作为 FSS/GRank 的输入，论文语义不再为此增加通信；
- r 还是 FSS gate 的 secret parameter，因此它不能只是与 public list 同长度的独立随机向量。

论文会议版的 Theorem 4.1 给出三轮结论，但其 proof 将具体 shuffle/FSS 细节指向 full version。故本门可以确认上述功能要求和轮数目标，不能凭 conference paper 补写一份未公开的逐消息、逐材料序列化。

## 3. A 级 Chase shuffle 结论

Chase 等人的 Secret-Shared Shuffle 将 two-party secret-shared shuffle 构造为两个顺序的 Permute+Share。每个 Permute+Share 隐藏一方选择的局部置换，并输出局部 share；组合置换为 π = π1 ∘ π0。文中给出每个 Permute+Share 的消息、Share Translation、视图和 semi-honest simulator，并说明两次顺序调用的构造。

Chase 的功能输出是：

- P0/P1 各自得到随机 shares；
- 组合后的数据按隐藏置换排列；
- 单方视图不应恢复另一方局部置换或组合置换。

Chase 的论文并未在当前使用的功能定义中给出“同时向双方公开 π(x)+r、且该 r 已与后续 FSS/GRank material 绑定”的输出。其 Secret-Shared Shuffle 的 two-pass transcript 不能仅通过把某个 share 重命名为 public_masked_list 而获得 Agarwal 的公开 masked-list 功能。

## 4. B/C 级本地参考与当前 VFSS

### 4.1 本地参考

Agarwal_TopK/protocol1/route_b/public_masked_shuffle.py 是 dealer-assisted 的公开 masked shuffle functional/test adapter：dealer 可接收 clear rows，返回 public masked rows、shares、permutation 和 masks，测试验证代数等式。这是 B 级 oracle/test evidence，不是生产密码协议；其 clear-row dealer 边界不能迁移到 VFSS。

本地 B1 参考记录了 Permute+Share -> masked-open -> rank-open 的状态和真实两遍调用，但它使用旧 ABI、旧 artifacts 或测试侧 material；也不能证明当前 VFSS 的论文精确性。protocol1_ca 中的文件轮询、sleep、.bin 材料和动态 CA 依赖均留在参考边界，禁止迁入活动实现。

### 4.2 当前 VFSS 已有能力

当前 protocol_i_secret_shared_shuffle 的 forward 使用两次 PS，返回 secret-shared payload；reverse 使用逆置换和两次 role-swapped PS。其接口没有 public masked list、同置换证明或 GRank-correlated r 的输出/材料契约。

当前 M2 pipeline 的可复核 causal path 是：

1. raw score adapter：carry 和 sign，共 2 rounds；
2. forward secret-shared shuffle；
3. 独立 masked-key exchange/open；
4. CmpAgg 本地计算；
5. shuffled slot/rank_P controlled rank reveal；
6. reverse shuffle 和 original-order mask adapter，共 2 rounds。

因此当前 core 是 4 rounds，raw-score 总路径是 2 + 4 + 2 = 8。当前 node_mask_shares 只是项目 package 中的 mask material，尚未证明为论文 r，也未证明与公开 list 使用相同 π。

M3 的 protocol_iii_grank、DPF routing、secure combine 属于 M3-only 链路；M2 不使用 DPF routing/key/material，也不能用 M3 的三轮标签倒推 M2 论文门已通过。共享的只是经明确拆分后的 transport/header 约束、priority-key/uCMP/CmpAgg 类型和计量字段，不能把现有 M2 package instance 直接视为 M3 package。

## 5. 论文 core 与项目 adapters

必须保留三层身份：

| 层 | 目标 | 身份 |
| --- | --- | --- |
| 论文 core | secret-shared shuffle 产生 π(x)+r，并由同一 r 驱动 FSS/GRank | A 级目标；当前 D |
| raw-score adapter | signed Q20.12 raw shares 转为项目 priority-key shares | C 级项目 adapter，2 rounds |
| output adapter | shuffled rank/carrier 转为原输入顺序 XOR Top-K mask | C 级项目 adapter，当前 2 rounds |

只有在论文 core 本身被实现和审查后，才可以讨论 2 + 3 + 2 = 7 的项目候选总轮数；不能把它写成论文原生的七轮，也不能把当前 2 + 4 + 2 = 8 改名为 paper-exact。

## 6. 目标理想功能：public masked-list

后续实现必须先冻结以下可审查的理想功能。设 P0/P1 持有同一批记录的 additive shares，记录包含排序键、稳定同分所需的隐藏 index binding 和项目需要的 payload；逻辑长度为 n，协议工作长度可为 padded_n。

理想功能应：

1. 由 offline material 确定或等价地产生一个隐藏的组合置换 π；
2. 在同一个 π 下生成 secret shares of π(x)；
3. 生成公开数组 y_i = π(x)_i + r_i；
4. 使同一 r 成为后续 GRank/DCF/FSS 的 secret parameter；
5. 保证 r 对任何单一参与方都不是可重构明文；
6. 让公开方只能看到协议允许的 public masked list 和 shape/phase metadata，看不到原始 index、原始 score、未掩码 rank、π 或任一局部置换；
7. 明确 dummy/padding、slot 对齐、域和溢出处理；padding 不得把 n-1-paper_rank 等未经证明的变换当成 rank 语义；
8. 约束 session、fingerprint、party、stage、slot、width、count、one-shot material 和失败/重放/复用行为。

公开 list 的可见性不等于公开 permutation。公开 list 的每个 slot 必须与 secret payload slot 和 GRank material 共同绑定；没有原始 index 的 public mapping，也不能从公开 list 逆推出原始顺序。

## 7. 同一置换绑定

最小代数义务是证明：public list 的每个 y_i、secret shuffled payload 的每个 share，以及后续 GRank 所消费的 r_i，都使用完全相同的组合 π = π1 ∘ π0，并且 slot 顺序没有在 framing、padding、chunking 或 reverse adapter 中改变。

Chase 的 two-pass algebra 可以证明 secret share 输出使用同一组合置换；它不能单独证明 public π(x)+r 输出。当前 VFSS 的 PS return value、transport frame 和 package record 均没有 public-list slot 与 GRank mask 之间的可验证 binding。因此以下做法都不能过门：

- 只让两个数组长度相同；
- 由 controller 或 test oracle 另采一个 permutation；
- 由 P2 生成 input-dependent public list；
- forward PS 完成后再交换一个无法证明同置换的 list；
- 把当前 masked-key frame 改名为 public list；
- 以公开 original index 或 selected index 帮助 reverse mapping。

## 8. r 与 GRank material 的关联

论文要求 r 既参与 public masking，又是 FSS gate 的秘密参数。后续 preprocessing 必须给出材料生成算法、party-local serialization 和 consumption proof，说明：

- P2 不看 raw shares，不选择 input-dependent permutation，不在线参与；
- P0/P1 各自只收到自己的 shares/material，任何一方不能单独恢复 r；
- public list 与 GRank material 使用同一个 r，而非两个独立随机向量；
- material 与 session/fingerprint/phase/slot/width/count/one-shot 绑定；
- 错误、超时、EOF、重复消费、trailing bytes、角色错配均 hard-fail；
- secure path 不打印、持久化或重构 r、permutation 或 raw/key。

当前 node_mask_shares 没有上述论文关联证明。M3 DPF rank masks 和 secure-combine mask 也不是 M2 的 r，不能作为替代证据。

## 9. rank、tie、padding 与输出适配

论文 rank 方向、stable tie 规则和项目的 descending priority rank 必须先写成相同输入域上的 oracle contract，再实现。尤其不能把论文的 ascending rank 通过全局 n-1-rank 变换“看起来”改为项目 rank，而不证明 padding dummy、stable index 和 signed domain 仍保持等价。

项目最终输出仍是原输入顺序的 XOR Top-K bit-mask share，只能由 test harness 重构。它不是论文 Fsort/Fselect 的原生输出，因而必须作为独立 output adapter 及独立 leakage/test gate 记录。

## 10. 当前与候选消息因果 transcript

### 10.1 当前 VFSS transcript（B/C）

| 因果屏障 | 当前行为 | 计数 |
| --- | --- | ---: |
| O0 | P2 发送 input-independent packages，随后退出 | offline |
| R1 | raw carry/sign adapter | 2 |
| R2 | forward two-pass secret-shared PS | 2 |
| R3 | 独立 masked-key exchange/open | 1 |
| R4 | shuffled slot/rank reveal | 1 |
| R5 | reverse two-pass PS + output mask | 2 |
| 合计 | 当前 raw-score path | 8 |

### 10.2 论文兼容候选 transcript（D/C）

候选只能是以下目标，不是当前实现事实：

| 因果屏障 | 必须产生的结果 | 目标计数 |
| --- | --- | ---: |
| O0 | P2 提供与 input 无关、同时关联 π/r/GRank 的 package，随后退出 | offline |
| R1 | public masked-list shuffle 的第一部分 | 1 |
| R2 | public masked-list 与 secret shuffled shares 完成，且同一 π/r 可证明 | 1 |
| R3 | GRank/rank reveal 达到论文三轮边界，不能另藏一个 masked-key barrier | 1 |
| A | 项目 reverse/output adapter | +2 |

必须给出每一条消息的 sender/receiver、payload 语义、输入/输出视图、依赖的 material、公开对象和 causal barrier。论文未公开的 full-version 细节不能用猜测填入；若实现采用不同 transcript，必须标为 project extension，而不是 paper-exact。

## 11. 设计选项与决策

| 选项 | 说明 | 论文支持 | 主要风险 | 本门决定 |
| --- | --- | --- | --- | --- |
| A | 在现有 two-pass PS 上做最小扩展，追加/融合 public list 与 correlated r | 需要新增代数、视图和材料证明；Chase 本身不足 | 可能仍多一个 causal barrier，无法证明 3 rounds | 暂不 GO |
| B | 新建 paper-compatible primitive，独立冻结两轮 shuffle、第三轮 GRank 和材料 | 可把 paper core 与项目 adapters 隔离 | 新协议、新 conformance、P2 package 和安全审查成本高 | 可作为后续设计方向 |
| C | 保持当前 C 级工程路径，只完成证据闭合 | 与现状一致，不冒充论文实现 | 不提供 paper-exact 功能 | **本门选择；BLOCKED** |

选择 C 不表示论文目标被否定，而是明确当前证据不能授权实现或标签升级。M3 继续沿现有独立链路推进，不借用未闭合的 M2 exact 结论。

## 12. 实现隔离边界

本轮不新增或修改源代码、测试、CMake、依赖、baseline 或 Papers。若后续 B 选项获批准，概念上应隔离为最小的新 primitive 文件：

- protocol_i_paper_shuffle.*
- protocol_i_paper_core.*
- protocol_i_paper_party_package.*

概念接口只能先冻结为功能契约，不能在本轮直接变成 ABI：

- paper_shuffle_preprocess(config, party_material, correlated_grank_material) -> one-shot package；
- paper_shuffle_online(party, input_share, one-shot package) -> secret_shuffled_share + public_masked_list + binding metadata；
- paper_core_rank(public_masked_list, correlated_grank_material) -> shuffled rank shares；
- paper_output_adapter(shuffled rank shares, output material) -> original-order XOR mask shares。

每个接口都必须 hard-fail 于 wrong role、wrong phase/sequence、wrong session/fingerprint、wrong slot/width/count、truncation、trailing bytes、EOF、timeout、replay 或 material reuse；不得 retry、fallback、online Dealer、文件同步或 secure reconstruction。实际参数和 wire format 必须在独立安全审查后再冻结。

这些文件必须与当前 protocol_i_secret_shared_shuffle.*、当前 CmpAgg pipeline、M3 protocol_iii_* 分开。不能在当前 PS return struct 上塞入未审查的 public-list 字段，不能把 test/dealer oracle 变成 secure path，不能通过重命名已有 frame 消除一轮。

frame/header binding 与 material record binding 也必须分开：

- frame/header：session、fingerprint、n、K、bits/width、sender、receiver、phase、type、sequence、payload length，以及分块时的 ordered chunk offset；
- material record：party、slot、stage、edge endpoints、material count、one-shot 和 correlated-material identity。

类型可以复用，已有 M2 package instance 不能直接复用于 M3；M2 使用 padded_n 图，M3 使用 logical_n/独立 GRank 图，node mask、edge material、count 和 serialization 必须各自生成、校验和测试。

## 13. 未来测试矩阵

只有以下证据全部绑定到同一 revision、label、输入/种子、环境、命令、重复次数和原始计数，才可重新审查：

1. shuffle conformance：置换组成、share reconstruction、public π(x)+r 方程、slot/padding/域边界；
2. correlated-material negative cases：替换 r、错 slot、错 permutation、错 phase、错 party、重复消费和 truncation 必须失败；
3. oracle differential：论文理想功能、公开 masked-list oracle、GRank rank/tie/padding 和项目 output adapter；
4. independent-process E2E：P2/P0/P1 进程、package/exit、socket frame、public list、rank reveal、reverse mask，全链路无 test reconstruction；
5. M3 regression：确认不调用 M2 pipeline/shuffle symbols，M3 raw/GRank/DPF/secure-combine 的输入输出和三轮标签不变；
6. leakage audit：逐 party view、public view、test-only view 和 persistent artifacts 的正反例。

当前阶段不执行这些新测试，因为没有实现变更；阶段一已记录的 24/24 代码基线结果只能作为历史 B/C evidence，不能充当 public-list 设计证明。

## 14. 15 项 GO gate

以下条件必须全部满足才能从 BLOCKED 进入 GO；任一缺失至少为 PARTIAL，若缺少核心功能或安全证明则为 BLOCKED：

1. 固定论文文件与 hash；
2. 明确 Fsort/Fselect 输入输出、角色和原生输出；
3. 明确 π(x)+r 的公开可见性与 r 的单方未知性；
4. 明确同一组合置换的代数绑定；
5. 明确 r 与 GRank/FSS material 的关联；
6. 明确 P2 offline-only、package 和 exit；
7. 完整列出所有 online causal barriers；
8. 证明 core 恰为 3 rounds，且无隐藏第四轮；
9. 固定 signed/range/uCMP/overflow 域；
10. 固定 rank direction、stable tie 和 padding；
11. 固定 public、P0、P1、P2、test、persistent leakage；
12. 固定项目 output adapter 与论文原生输出的边界；
13. 完成 paper-compatible primitive 的 conformance；
14. 完成 oracle differential 与独立进程 E2E；
15. 完成 M3 regression、reproduction record 和独立安全复核。

本轮 3、4、5、7、8、13、14、15 尚未满足核心或实现证据，因此结论不是 PARTIAL，而是 BLOCKED。

## 15. 标签与实现授权政策

在上述 gate 全部满足前，禁止使用：

- agarwal_protocol_i_exact
- agarwal_protocol_i_exact_mask_output
- paper_3_round_exact
- secure_shuffle_complete

可以使用的候选研究标签仅为：

- agarwal_protocol_i_paper_compatible_3round_candidate
- m2_protocol_i_paper_compatible_core_design

当前已验证标签继续保持：

- m2_protocol_i_modular_6round_mask_output：历史 C 级 priority-key 输入路径；
- m2_protocol_i_raw_score_input_modular_8round_mask_output：当前 raw-score C 级路径。

## 16. 结论与解锁条件

本门结论是 **BLOCKED**。原因是论文必需的 public masked-list、同一 hidden permutation、correlated r/GRank material 和三轮完整 causal transcript 尚未在当前 VFSS 或本地可用论文证据中闭合。这个结论只阻止 M2 paper-exact 实现和 exact label，不阻止已冻结 M2 C 级基线或 M3 独立实现继续验证。

允许另行审查明确标注为 project candidate 的 Dealer-preprocessed 路径，但 P2
offline 知道完整 `r` 属于项目扩展安全模型，不是本门对 Agarwal exact Dealer view
或论文泄露等价性的批准。该路径必须独立满足自己的 candidate conformance、
differential、独立进程和 leakage gate。

下一次只有在形成独立的 paper-compatible primitive 设计、逐消息 transcript、材料/视图证明、15 项 gate 的 conformance/differential/E2E 证据并复核 M3 regression 后，才重新审查 GO。未经这些证据，不得以代码改名、补一个字段、增加测试 oracle 或借用 M3 三轮结果解锁。
