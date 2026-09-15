# 阶段三O：M2 Protocol I 论文精确三轮设计门与实现可行性报告

日期：2026-09-15
分支：`codex/m2-candidate-ubuntu-validation`
当前最终 HEAD：以阶段三O documentation-only closeout 提交后的 `git rev-parse HEAD` 为准。
证据级别：论文事实为 A；本地参考/当前 VFSS 为 B/C；缺失事实为 D。本文不是 paper-exact 实现报告。

## 1. Executive conclusion

阶段三O完成了阶段三N文档 clean closeout、论文来源核对、Protocol I 事实矩阵、当前
candidate/Route A 对照和 16 项 exact design gate。结论是：

```text
DESIGN_BLOCKED
M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED
paper-native 3-round core: DESIGN_BLOCKED, NOT_IMPLEMENTED
```

原因不是当前 C 级工程代码无法运行，而是会议版论文只给出 Protocol I 的高层功能和
复杂度，未给出足以冻结 paper-compatible 实现的逐消息 transcript、party view、
same-permutation binding、相关 `r` 材料证明和 leakage simulator。论文第 9 页明确将
Theorem 4.1 proof 指向 full version；仓库当前只有 15 页会议版，未找到作者 full version。
因此不允许开始 paper-compatible C++ 实现，也不允许把 candidate 或 Route A 改名。

阶段三N已有运行证据保持不变：M2/M3 工程路径通过，paper exact 不通过。阶段三O没有
修改 `VFSS/` secure source、M3、formal baseline、candidate、Route A、`VFSS-baseline/`
或任何参考目录。

## 2. 阶段三N provenance 与 clean closeout

- 阶段三N代码/测试证据绑定：`fc149c2e33df53a272208622da925d0047303121` 的实现父系，
  结果实际来自 `5bd879e8c4d053b14aae61f19bbaf53529446025` 对应的代码状态；
- 阶段三N文档 closeout：`bcb5dd3a4ea144817d5b7f5e7e673d3f2d214d9a`，仅提交四份文档，
  不改变代码或测试证据；
- 阶段三O文档 closeout：当前 HEAD 的 documentation-only 提交，仅提交本文和本阶段
  decision record；完整 hash 由最终 `git rev-parse HEAD` 固定并在交付回复中列出；
- `git status --short --branch`：clean；`git diff --check`：PASS；
- protected paths (`VFSS-baseline/`, `Papers/`, `Agarwal_TopK/`, `ADSMPC/`, `CipherGPT/`)：无差异；
- `177/177` 是 EMP-ON Ninja build steps，不是测试数量；`34/34` 是 CTest tests；M3
  focused 的 `11/11` 与 raw-score `3/3` 是子集结果。

阶段三O只新增本文和对应 decision record；没有重新运行阶段三N全套测试，因此本阶段
测试状态写 `NOT_REMEASURED`，引用阶段三N的 revision-bound 原始日志，不把历史结果伪装成
阶段三O新测试。

## 3. 论文来源与完整性

### 3.1 Agarwal 会议论文

来源文件：`Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf`

- SHA256：`18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c`，与
  `docs/PAPERS.sha256` 一致；
- 15 页 CCS 2024 会议版；本地未找到作者 full version；
- PDF 提取工具确认 15 pages。第 3 页 Table 1 标明 Protocol I 为 2+1、Shuffle、
  `binom(n,2)`-CmpAgg、3 online rounds；第 6 页 §2.4 定义 public masked shuffle；
  第 8-9 页 §4.1/Theorem 4.1 说明 shuffle-then-reveal 和 3-round 高层组合；
  第 9-10 页描述 DPF routing 的不同路径以及 full-version 细节缺口。

由于会议版省略 formal shuffle instantiation、完整证明和若干失败/边界语义，论文来源
状态固定为：`PAPER_SOURCE_INCOMPLETE`。不能用本地参考工程或当前 VFSS 代码补写 A 类事实。

### 3.2 Shuffle 来源

来源文件：`Papers/协议1shuffle.pdf`，SHA256
`6112f7116ec3d3b100fbb5ca10f058a6a0e3f19f165c5c6a071a48b10c0c48ab`。
该资料用于 Chase/Ghosh/Poburinnaya Permute+Share 的参考行为与调用边界；它不是 Agarwal
论文 Protocol I 的完整 transcript，也不是当前 VFSS secure 证明。`Agarwal_TopK/protocol1_ca/`
仅作 B 类本地参考，不能反向升级为论文事实。

## 4. 可追溯论文事实矩阵

| 事实 | 论文来源 | A 类可确认内容 | 当前 VFSS 对应/差异 | 作者确认 |
|---|---|---|---|---|
| 输入 | p.3 Table 1；p.6 §2.4 | additive secret-sharing 的 n 个输入，输入群 `[L]`/`G` | 项目使用 signed Q20.12、priority key、padded_n | 需要确认 raw-score 是否原生 |
| 输出 | p.3 Table 1；p.8 §4.1 | `Fsort/Fselect` 的共享输出；selection 可为 payload/value | 项目统一要求 original-order XOR mask | 需要确认 mask 是否原生 |
| 总轮数 | p.3 Table 1；p.9 Theorem 4.1 | Protocol I 为 3 online rounds | candidate=3，Route A=6/8 | 需逐消息映射 |
| 角色 | p.3 Table 1 footnote | 2+1：两在线方 + offline correlated-randomness dealer | 项目 Dealer/P0/P1 | paper party view 仍不完整 |
| R1 高层 | p.2 §1.1；p.8 §4.1 | shuffle input before ranking | candidate forward shuffle | exact message 未给 |
| R2 高层 | p.2 §1.1；p.6 §3.1；p.8 §4.1 | one-round CmpAgg ranking | candidate CmpAgg core | exact frame/edge material 未给 |
| R3 高层 | p.2、p.8-9 §4.1 | reveal stable shuffled ranks; clear routing | Route A additionally reveals rank and reverse-shuffles | paper output adapter 未明 |
| shuffle 数学定义 | p.6 §2.4 | public `y = π(x)+r`; π permutation，r random private masks unknown to any one party | 当前 Route A 有 shuffled masked list，但 material binding 不是论文证明 | 需确认 same permutation |
| public masked-list | p.6 §2.4；p.8 §4.1 | public masked shuffled array feeds FSS gate with r as secret parameter | candidate/Route A public list为工程 frame | 需给精确域/编码 |
| score/payload/index binding | p.8 §4.1 只说 input list/attached payload | 未明确写出 original-index binding 或 record encoding | 项目 priority key 绑定 index | 必须作者确认 |
| permutation visibility | p.6 §2.4；p.8 §4.1 | 单方不能知道完整 secret permutation；公开 masked list | P0/P1 当前各自 material/view 是项目定义 | 需逐方 view |
| `r` 定义 | p.6 §2.4 | n 个 random private masks | 项目有 share/material id | 生成、分发、消费相关性未证明 |
| `r` 与 permutation | p.6 §2.4 | 同一 shuffle functionality 输出 π(x)+r | 当前工程 correlation 仅 C 类 | 必须作者确认 |
| Dealer offline | p.3 footnote；p.9 Theorem 4.1 | offline correlated randomness；dealer 可被消除但代价变化 | 项目 Dealer input-independent/offline/silent | paper exact transcript 缺失 |
| rank 是否公开 | p.2 §1.1；p.8 §4.1 | shuffled ranks 可安全 reveal | Route A R4 公开 shuffled rank | 公开对象和受众需确认 |
| selected index | p.8 §4.1 | routing in clear for secret-shared values，不等于公开 original index | 项目不公开 selected/original index | 需确认 routing carrier |
| original-order mask | p.8 routing定义是排序/selection payload | 未看到原始顺序 XOR mask 定义 | 项目额外 mask adapter | 必须分开计量 |
| stable ties | p.6 §3 stable rank | equal values按原始出现顺序 stable rank | 项目 index tie-break 一致 | padding/dummy 仍需确认 |
| padding/dummy | available conference pages | `UNKNOWN_FROM_AVAILABLE_PAPER` | 项目内部 padded_n，dummy 不输出 | 必须作者确认 |
| security model | p.3 Table 1 | semi-honest，single-party corruption | 项目测试/工程边界为两在线方 + offline Dealer | exact party model需确认 |
| leakage set | p.6 §2.4；p.8 §4.1 | masked list与shuffle后的stable ranks用于 routing | Route A rank reveal/carrier是项目公开对象 | exact simulator缺失 |
| metrics | p.3 Table 1 | asymptotic online/offline bits、PRG calls、rounds | 项目当前逐barrier bits/PRG未测 | 论文低阶项需 full version |
| adapter rounds | p.9 Table/Theorem 4.1 | 3 rounds指论文 core；未列项目 mask adapter | 项目 raw adapter/reverse shuffle额外计入 | 必须确认原生输出边界 |
| proof/transcript | p.9 Theorem 4.1 | proof referred to full version | 本地没有 full version | 必须补作者材料 |

## 5. 论文 core 与当前路径对照

| 项目 | 论文 Protocol I | 3-round candidate | Priority Route A | Raw Route A |
|---|---|---|---|---|
| 输入 | shared ordered values/payloads | padded priority-key shares | same + Route A material | signed Q20.12 + 2 adapter rounds |
| R1 | secret shuffle + public masked list | forward PS stage 1 | same | carry/sign precede core |
| R2 | one-round CmpAgg rank | forward PS stage 2 | same | same |
| R3 | rank/reveal and clear routing high-level | masked-list opening + local rank share | same, then R4 reveal | same, then R4 reveal |
| rank public | high-level shuffled stable rank reveal | no rank output; shares remain | yes, R4 | yes, R4 |
| reverse shuffle | `UNKNOWN_FROM_AVAILABLE_PAPER` for mask adapter | none | R5/R6 | R5/R6 |
| output | Fsort/Fselect shared output | shuffled rank shares | logical_n original-order XOR mask | logical_n original-order XOR mask |
| preprocessing | input-independent correlated randomness | project package/material | project package + reverse material | project package + carry/sign |
| `r` correlation | paper functionality requires it | project candidate assumption | project material, not paper proof | project material, not paper proof |
| identity | A paper claim | C candidate | C 6-round extension | C 8-round extension |

数字相同不代表同构：candidate 的 3 轮是工程 candidate 的 forward shuffle/opening/rank
share transcript；Route A 的完整 mask 额外加入 rank reveal 与两次 reverse shuffle；raw
Route A 再加入 carry/sign。没有 paper transcript 和 material proof，不能把 candidate 3 轮
写成论文三轮。

## 6. 16 项 exact design gates

| Gate | 状态 | 结论 |
|---|---|---|
| 1. 论文来源充分性 | BLOCKED | `PAPER_SOURCE_INCOMPLETE`，缺 full-version transcript/proof |
| 2. 输入输出兼容性 | BLOCKED | 论文 payload/value output 与项目 XOR mask adapter 未冻结为同一接口 |
| 3. same-permutation binding | BLOCKED | score/payload/index/rank 的同置换绑定没有论文证据 |
| 4. correlated material `r` | BLOCKED | 生成/分发/消费与 permutation correlation 未证明 |
| 5. party view | BLOCKED | Dealer/P0/P1 每轮看到的完整字段未给出 |
| 6. offline Dealer | PASS-C / UNKNOWN-A | 项目模型满足；论文细节仍需确认 |
| 7. rank privacy | BLOCKED | 论文允许 shuffled rank reveal，但受众/精确泄露未冻结 |
| 8. leakage equivalence | BLOCKED | Route A R4 public rank/carrier 改变泄露，simulator 未完成 |
| 9. stable ties/padding | BLOCKED | stable rank有论文定义；dummy/padding未知 |
| 10. round accounting | BLOCKED | 3轮高层声明明确，逐 barrier 与 adapter 边界不明确 |
| 11. secure output | BLOCKED | paper-native Fsort/Fselect 与 original-order XOR mask未分离证明 |
| 12. primitive availability | BLOCKED | VFSS 无已证明等价的 production paper shuffle/public-list primitive |
| 13. primitive conformance | PASS-C / NOT_REMEASURED | 工程原语已有证据，exact paper primitive尚未定义 |
| 14. oracle differential | PASS-C / NOT_REMEASURED | 当前项目 oracle通过，不能替代 exact oracle |
| 15. independent-process E2E | PASS-C / NOT_REMEASURED | candidate/Route A通过，不是 paper-native core |
| 16. formal leakage evidence | BLOCKED | 只有静态/运行审计，无论文等价 simulator |

进入 paper-compatible C++ 的必要 gate 1、3、4、5、7、8、10、12、15、16 未全部通过，
因此总决策为 `DESIGN_BLOCKED`，不是 `DESIGN_GO`，更不是 `IMPLEMENTATION_GO`。

## 7. Blocker closure

### 7.1 必须由论文/作者确认

1. Protocol I 三个 causal online rounds 的逐消息划分、sender/receiver 和 barrier 依赖；
2. public masked-list 的数学域、编码和是否逐位置公开；
3. `r` 如何生成、分发、消费，以及是否与隐藏 permutation 同一材料绑定；
4. P0/P1/Dealer 各自看到哪些 masked list、rank、payload、index、carrier；
5. paper-native 输出是 rank share、sorted payload、selected payload 还是 original-order mask；
6. 三轮是否包含输入转换、payload/index binding、reverse routing 或只指 protocol core；
7. stable ties 的确切 index 语义和 padding/dummy 规则；
8. 论文允许泄露集合与 semi-honest party corruption view；
9. full version 中被省略的 shuffle instantiation、proof、低阶计量项。

### 7.2 可由 VFSS 自行实现，但必须先获设计批准

- paper-compatible package/material state machine；
- 与论文 transcript 对应的 public-list shuffle primitive；
- secure record-preserving inverse routing（若论文输出边界确实需要）；
- exact frame schema、barrier trace、失败语义；
- primitive conformance、paper-native differential、独立 Dealer/P0/P1 E2E；
- leakage simulator 和 paper-vs-project 输出适配分离；
- stable ties、padding、K/n 边界测试。

### 7.3 最小新增工程面（估计，不是已实现）

- 新 primitive：`paper_public_masked_shuffle`，输入/输出绑定同一 π、r、record fields；
- 新 material/package：permutation share、r shares、FSS gate parameter、payload/index binding；
- 新 frame：独立 paper identity、phase/sequence/material id、per-barrier sent/received bits；
- 新 conformance：same-permutation、public-list reconstruction、r correlation、role view；
- 新 differential：paper-native payload/value oracle 与项目 mask adapter 分离；
- 新 E2E：独立 Dealer/P0/P1，Dealer online silent；
- 新 leakage simulator：分别模拟 P0、P1、Dealer 的 view；
- 预期 online rounds：论文 core 仍需确认是否 3；任何 mask/reverse adapter 必须额外计数，
  不能预先写成 3 或 7。

这些是下一阶段设计输入，不是当前实现承诺。

## 8. M3 与既有路径交接

M3 继续固定为 `agarwal_protocol_iii_modular_3round`，raw-score 扩展为
`moe_topk_protocol_iii_raw_score_modular_5round`。M3 不调用 M2 Route A rank reveal、
reverse shuffle、`RA6M`/`RA8M` 或 M2 raw-score adapter。阶段三O没有修改 M3，也没有使用
M3 DPF material 替代 Protocol I paper material。

## 9. 测试、原始证据与未测量项

阶段三O没有修改 C++，因此测试均为 `NOT_REMEASURED`；阶段三N结果仍可追溯于：

- `/tmp/moe-stage3n-emp-on/ctest-full.log`：34/34 CTest；
- `/tmp/moe-stage3n-emp-on/ctest-m2.log`：M2 19/19；
- `/tmp/moe-stage3n-emp-on/ctest-m3.log`：M3/DPF 11/11；
- `/tmp/moe-stage3n-emp-on/ctest-candidate.log`：candidate 1/1；
- `/tmp/moe-stage3n-emp-on/ctest-route6.log`：Priority Route A 1/1；
- `/tmp/moe-stage3n-emp-on/ctest-route8.log`：Raw Route A 1/1；
- `/tmp/moe-stage3n-emp-on/ctest-m3-raw5.log`：M3 raw 5-round 3/3；
- `/tmp/moe-stage3n-emp-on/build.log`：177/177 Ninja build steps；
- `/tmp/moe-stage3n-emp-off/build.log`：52/52 EMP-OFF production steps；
- `/tmp/moe-stage3n-emp-on-prod/build.log`：69/69 EMP-ON production steps。

仍为 `NOT_MEASURED` 或 `UNKNOWN_FROM_AVAILABLE_PAPER`：逐 barrier bits、online PRG total、
正式性能/RTT/带宽、论文 exact leakage simulator、paper-native E2E、padding/dummy paper
语义、full-version 低阶计量和任何 7-round path。

## 10. 修改文件与提交

本阶段新增/修改：

1. `docs/reproduction/M2_PROTOCOL_I_STAGE3O_PAPER_GATE_2026-09-15.md`；
2. `docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md`。

阶段三N clean closeout：`bcb5dd3a4ea144817d5b7f5e7e673d3f2d214d9a`。阶段三O文档
closeout：当前 HEAD 的 documentation-only 提交；本阶段没有代码提交。

## 11. 下一阶段建议

在作者/导师确认清单获得答复前，建议 `NO-GO` paper-compatible C++，继续维护当前
M2/M3 工程路径。答复齐全后，先创建独立 design record 和 transcript review，再实现
primitive；不得从 Route A 复制后删除阶段、不得复用 M3 material、不得修改现有标签。

## 12. 最终状态表

| 项目 | 状态 |
|---|---|
| M2 formal engineering baseline | GO，8-round project baseline |
| M2 3-round rank-share candidate | GO |
| M2 priority-key Route A | GO，6-round project extension |
| M2 raw-score Route A | GO，8-round project extension |
| M3 modular 3-round | GO，仅限 modular project baseline |
| M2 paper-exact Protocol I | BLOCKED / NOT_VERIFIED |
| paper-native 3-round core | DESIGN_BLOCKED，NOT_IMPLEMENTED |
| project 7-round path | NOT_IMPLEMENTED / RESEARCH_BLOCKED |
| formal performance benchmark | NOT_MEASURED |

## 13. A-O 完整回答

**A.** 是。阶段三N已在 `bcb5dd3` 形成 clean committed revision；阶段三O文档将产生新的 clean documentation-only revision。

**B.** 当前阶段三O文档提交后的最终 HEAD 以最终 `git rev-parse HEAD` 为准，交付回复列出完整 hash。

**C.** 是。阶段三N报告明确区分 `fc149c2` 代码/测试证据、`5bd879e` closeout、`bcb5dd3` 阶段三N文档提交和阶段三O documentation-only closeout；阶段三O只引用这些边界。

**D.** `177/177` 是 Ninja build steps；`34/34` 是 CTest tests。

**E.** `m2_protocol_i_raw_score_input_modular_8round_mask_output`，8 轮。

**F.** `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`，输出 shuffled-domain rank shares，不是完整 mask。

**G.** `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output`，6 轮，输出 logical_n 原始顺序 XOR mask。

**H.** `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output`，8 轮，输出 logical_n 原始顺序 XOR mask。

**I.** M3 为 `agarwal_protocol_iii_modular_3round`，并独立于 M2 Route A；raw-score 扩展是 5 轮。

**J.** 不可以，仍为 `M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`。

**K.** 最小原因是会议版缺少可实现、可证明、可模拟的三轮逐消息 transcript 和 `π/r`/泄露契约；当前 Route A 还增加了 rank reveal 与 reverse adapter。

**L.** 必须确认三轮阶段、public masked-list、r correlation、party view、native output、adapter 是否计入、ties/padding、leakage 和 full-version proof。

**M.** VFSS 可以实现新 package/material/frame、conformance、differential、independent E2E 和 leakage simulator，但不能自行定义论文缺失语义。

**N.** 不允许。当前决策是 `DESIGN_BLOCKED`，不是 `IMPLEMENTATION_GO`。

**O.** 最短路径是先向作者/导师发送下方确认清单；获得回答后先冻结独立 design record，再实现最小 primitive，最后通过 conformance → differential → independent-process E2E → leakage gate。

### 作者/导师确认清单

1. Protocol I 的三个 online causal rounds 分别是哪三个消息阶段？
2. `y = π(x)+r` 中 `r` 如何生成、分发、消费？是否与 π 的材料相关？
3. P0/P1 是否看到完整 shuffled rank，看到哪些 masked list 字段？
4. score、payload、original index 是否必须由同一隐藏 π 绑定？
5. 论文原生输出是 rank share、sorted payload、selected payload 还是 original-order mask？
6. 三轮是否包含输入转换、payload/index binding 和 reverse routing？
7. stable ties、padding、dummy 的确切定义是什么？
8. Dealer 是否完全 offline、input-independent、online silent？
9. full version/proof/transcript 是否可提供？
