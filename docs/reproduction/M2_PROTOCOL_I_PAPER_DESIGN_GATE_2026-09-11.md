# M2.17 Protocol I 论文设计门复现记录

状态：**BLOCKED**

日期：2026-09-11。

本记录保存阶段二的只读复核边界、证据等级、设计结论和未测项目。它不记录新的运行时实现，不产生新的性能数字，也不授权 push、merge、rebase 或 M2/M3 代码修改。

## 1. 任务与基线

- 目标：闭合 M2 Protocol I 的论文设计门，输出 GO/PARTIAL/BLOCKED 和可审查的解锁条件。
- 工作分支：codex/m2-protocol-i-paper-design-gate。
- 阶段一验证/base revision：2a83b19dead1230a416fa093c8ca2983cdec0f8e。
- 阶段一文档 commit：f5450cd45fdb32afe1201863b6f38898463f738e。
- 本轮进入阶段二时的事实修正 commit：511d43f1cf45825b30dd45b3d1bb52d3840aa3f9。
- origin/main 在复核时为 2a83b19dead1230a416fa093c8ca2983cdec0f8e。
- 远端为 git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git；本轮不 push。

## 2. 阶段一事实修正

阶段一文档中的两处表述已先行修正并单独提交：

1. 删除“docs/decisions/M3_RAW_SCORE_SECURE_ENTRY.md 缺失”的错误说法，改为说明它是前一任务输入文件名错误；实际 M3 raw evidence 是 PROTOCOL_III_MODULAR_3ROUND_DESIGN.md、M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md 及对应 raw pipeline/formal executable/tests。
2. 把共享契约矩阵改为：priority-key/uCMP/CmpAgg、部分 package/transport 可复用；DPF routing、DPF key/material 和 secure combine 是 M3-only，不能写成 M2 依赖。
3. 把 frame/header binding 与 material record binding 分开，避免把 slot/type/width/count 写成每个 frame 的字段。
4. 把阶段一记录中的 HEAD 改成 validation/base revision，并单列 documentation commit。

## 3. 论文文件与完整性

执行 docs/PAPERS.sha256 的 sha256 校验，结果全部通过：

- Agarwal CCS 2024：本地文件 hash 为 18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c。
- Chase Secret-Shared Shuffle：本地文件 hash 为 6112f7116ec3d3b100fbb5ca10f058a6a0e3f19f165c5c6a071a48b10c0c48ab。

已阅读并按页复核：

- Agarwal Table 1、§2.4、§3/§3.1、§4.1、Theorem 4.1、相关 footnote/reference；
- Chase 的 Permute+Share、Share Translation、Secret-Shared Shuffle、两遍组合、视图与 simulator 章节。

论文证据结论：

- Protocol I 是 2+1 拓扑，online cost 为 3 rounds；
- Fsort/Fselect 原生输出是 sorted/selected payload 的 additive shares；
- secure shuffle 需要 public masked list y = π(x)+r；
- r 对任一单方未知，并作为 FSS gate secret parameter；
- conference proof 将具体 shuffle/FSS 细节指向未提供的 full version；
- Chase 两遍 Permute+Share 证明 secret-shared payload 的隐藏组合置换，但不自动提供 public y 和 correlated r。

## 4. 本地参考与当前代码复核

本地 B 级参考检查了：

- Agarwal_TopK/protocol1/route_b/public_masked_shuffle.py：dealer-assisted public masked shuffle functional/test adapter，能够验证代数等式，但不是生产密码协议；
- P1_ROUTE_B0_5_MESSAGE_STATE_MACHINE.md：OFFLINE_PACKAGE、SHUFFLE_OPEN、MASKED_OPEN、RANK_OPEN、COMPLETE 状态；
- B0 security boundary 与 B1 real two-pass OPV/Share Translation/Permute+Share 说明；
- protocol1_ca 的旧 ABI、文件轮询、sleep、.bin artifacts 和动态 CA 边界，确认均不得迁移。

当前 VFSS B/C 级代码复核：

- protocol_i_transport.*：frame/header 绑定 session、fingerprint、n、K、bits/width、sender、receiver、phase、type、sequence、length/offset；
- protocol_i_party_package.*：package/material 绑定 party、slot、stage、edge endpoints、count、one-shot 等记录；
- protocol_i_secret_shared_shuffle.*：forward/reverse 使用 two-pass PS，只有 secret-shared payload output；
- protocol_i_pipeline.cpp：raw adapter 2 + current core 4 + reverse/output adapter 2；
- protocol_i_cmpagg.cpp：当前 masked priority-key exchange/open 后本地 CmpAgg；
- protocol_iii_grank.*、protocol_iii_dpf_routing.*、protocol_iii_secure_combine.*：M3-only raw chain，不是 M2 的 DPF/r material。

静态调用关系中未发现 M3 调用 M2 shuffle、pipeline 或 secret-shared-shuffle symbols。

## 5. 当前与候选轮数

当前 C 级 raw-score path：

1. raw carry/sign adapter：2；
2. forward two-pass PS：2；
3. 独立 masked-key exchange：1；
4. controlled rank reveal：1；
5. reverse/output adapter：2；
6. 总计：2 + 4 + 2 = 8。

候选 paper-compatible path 只能写成设计目标：

1. raw-score adapter：2；
2. paper-compatible 3-round core：3；
3. output adapter：2；
4. 候选总计：2 + 3 + 2 = 7。

候选 7 不是论文原生七轮，也不是当前实现结果。若独立 masked-key barrier 仍存在，则不能声称 core 为 3。

## 6. 设计门审查结果

设计门审查确认必须同时闭合：

- public masked-list 的理想功能；
- public list、secret shuffled payload 和 GRank 所用 r 的同一 permutation binding；
- r 与 FSS/GRank material 的 correlated generation、party view、serialization 和 one-shot；
- P2 input-independent/offline-only/package/exit；
- 全部 sender/receiver、payload、material、view 和 causal barrier；
- rank direction、stable tie、signed/range/uCMP/overflow、padding 和 output adapter；
- P0/P1/P2/public/test/persistent 六类 leakage；
- shuffle conformance、negative cases、oracle differential、独立进程 E2E 和 M3 regression。

现有 VFSS 只满足 C 级工程边界的一部分。当前 node_mask_shares 没有论文 r 的关联证明，当前 public masked-key opening 没有 same-permutation 证明，会议论文也没有可直接复制的完整三轮逐消息 transcript。因此总决定为 **BLOCKED**。

## 7. 选项决定

- 选项 A：在现有 two-pass PS 上扩展 public list/r；暂不 GO，因为需要新增 algebra、view、material 和 3-round 证明。
- 选项 B：新建隔离的 paper-compatible primitive；作为后续设计方向，不在本轮实现。
- 选项 C：保持当前 C 级路径，先把证据和边界写实；本轮选择 C，状态 BLOCKED。

如果后续进入 B 选项，概念文件边界为 protocol_i_paper_shuffle.*、protocol_i_paper_core.* 和 protocol_i_paper_party_package.*；本轮不创建这些 source/header。

## 8. 测试与计量边界

本轮只改 Markdown 设计/审计/复现记录，没有 source、test、CMake、dependency、baseline 或 Papers 变化，因此不执行新的 build/test。阶段一已有 24/24 代码基线结果仅作为历史 B/C evidence，不能充当 public-list 设计证明。

本轮没有产生新的性能计量，EMP-OFF、EMP-ON 和论文精确 3-round 测量均为 NOT_MEASURED。未来实测必须绑定 revision、implementation label、输入/seed、环境、命令、重复次数和原始计数。

## 9. 标签政策

未通过全部 15 项 GO gate 前，禁止 agarwal_protocol_i_exact、agarwal_protocol_i_exact_mask_output、paper_3_round_exact 和 secure_shuffle_complete。

仅可使用研究候选标签 agarwal_protocol_i_paper_compatible_3round_candidate 和 m2_protocol_i_paper_compatible_core_design。现有 m2_protocol_i_modular_6round_mask_output 与 m2_protocol_i_raw_score_input_modular_8round_mask_output 不变。

## 10. 复现结论

阶段二的可复现结论是 BLOCKED，而不是“设计完成即 GO”：

- 论文功能需求已明确；
- 本地参考与 current VFSS 边界已分层；
- 设计选项、目标接口文件、transcript、leakage matrix 和未来测试矩阵已明确；
- 核心 public y/same π/correlated r/3-round proof 尚未存在；
- 本轮不授权 M2 runtime implementation、不升级 exact label、不影响 M3 独立实现。

重新审查的最小入口是：新增 primitive 的设计/代码、完整消息和视图证明、材料负例、oracle differential、独立进程 E2E、M3 regression 以及独立安全复核全部以同一 revision 交付。
