# M6A-P2-I-E8 接收复核（修改实现前冻结）

日期：2026-10-03。审查对象仅为隔离 worktree 的 E7 小 D 实现。**来源限制：本聊天承接 E6/E7 的撰写上下文，不能充当用户要求的“未参与撰写的新聊天窗口”签收。** 下文重新按源码复核技术合同；`E7_INDEPENDENT_ACCEPTANCE = FAIL`（仅因异会话独立性这一程序条件未满足），`E7_TECHNICAL_ACCEPTANCE_BEFORE_FIX = PASS_WITH_FINDINGS`。发现均非既定可信 T/完整通道/单方半诚实模型内的已证实泄密或 Top-K 功能反例。实现修正后可重判技术门；正式异会话签收仍需真正的新窗口。

## A. 工作区证据冻结

branch `codex/m6a-p2-i-allpairs-experiment`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`。起始完整 `git status --short --untracked-files=all`：

```text
 M VFSS/CMakeLists.txt
 M docs/IMPLEMENTATION_PLAN.md
?? VFSS/include/moe_topk/protocol_i_aav86_small.h
?? VFSS/src/moe_topk/protocol_i_aav86_small.cpp
?? VFSS/tests/moe_topk/protocol_i_aav86_small_conformance_test.cpp
?? VFSS/tests/moe_topk/protocol_i_aav86_small_differential_test.cpp
?? VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp
?? docs/decisions/M6A_PROTOCOL_I_AAV86_ALL_PAIRS_DESIGN_GATE_2026-10-02.md
?? docs/decisions/M6A_PROTOCOL_I_AAV86_E6_COMPOSITION_SECURITY_AND_RUNTIME_CONTRACT_2026-10-02.md
?? docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md
?? docs/reviews/M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md
?? docs/reviews/M6A_P2_I_E4_CA_SOURCE_AND_SHUFFLE_COMPATIBILITY_2026-10-02.md
?? docs/reviews/M6A_P2_I_E5_AAV86_CA_SPEC_AND_REFERENCE_2026-10-02.md
?? docs/reviews/M6A_P2_I_E7_REVIEW_AND_SMALL_D_RUNTIME_GATE_2026-10-03.md
?? docs/reviews/M6A_P2_I_E7_SMALL_D_RUNTIME_VALIDATION_2026-10-03.md
?? experiments/m6a_p2_i_allpairs/CMakeLists.txt
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/README_TEST_ONLY.md
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_control_E6_TEST_ONLY.py
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_control_verification_E6_TEST_ONLY.json
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_counts_E5_v2_TEST_ONLY.csv
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_runs_E5_v2_TEST_ONLY.csv
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_conformance_test_raw_2026-10-03.txt
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_differential_test_raw_2026-10-03.txt
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_e2e_test_raw_2026-10-03.txt
?? experiments/m6a_p2_i_allpairs/TEST_ONLY/verify_aav86_ca_control_E6_TEST_ONLY.py
?? experiments/m6a_p2_i_allpairs/aav86_graph_counter.py
?? experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv
?? experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv
?? experiments/m6a_p2_i_allpairs/material_pool_test.cpp
```

E7 文件以内容哈希识别，不能只用 HEAD：

| 文件（E7 起点） | SHA-256 |
|---|---|
| `M6A_P2_I_E7_REVIEW_AND_SMALL_D_RUNTIME_GATE_2026-10-03.md` | `A6A63C1669800F98BC3745D60F1D7C3F8840BB88FE64F6490E9A8ABE48ACADD0` |
| `M6A_P2_I_E7_SMALL_D_RUNTIME_VALIDATION_2026-10-03.md` | `58AD7C9E16ED1BF2FED29C9D07EA6397EAA1D773A2ED9611F47449F3B08509F6` |
| `protocol_i_aav86_small.h` | `26CB2733BCAC2EEB6649FA36D8797E6528DF38B1A1F73CBD61A5809834DA18BD` |
| `protocol_i_aav86_small.cpp` | `296C25CADA425FDDA1A4ABF329FF9147B3732590E2AB0E993A4E07731DCBF85B` |
| `protocol_i_aav86_small_conformance_test.cpp` | `A74728C9E336F452B4C6F77BFBCE6EE4CC978096581071034F1E22210F9D8972` |
| `protocol_i_aav86_small_differential_test.cpp` | `5A9E0DFABC97040A83BE4A02F1ABAC4097E03225F6FC98C9B7C6B4CE42172664` |
| `protocol_i_aav86_small_e2e_test.cpp` | `C986A4CA624CF249E6155433352538875C0C54C5407670989AC4C3271F68EC95` |
| `e7_conformance_test_raw_2026-10-03.txt` | `9BA6C3183027C3B7B476934D068990284AC96217B47104B25F09ECDBE823DD13` |
| `e7_differential_test_raw_2026-10-03.txt` | `9D0D2D62D1E595856062AA2E11638C8901EF0432E1F1AE696E12A0F3FCD4003C` |
| `e7_e2e_test_raw_2026-10-03.txt` | `1A31D6DC4440876C0BAE961D46CF6E77A7BC0BC22C328C0CEBFA35D3B9E3A856` |

已读 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 的 M6A 节、E6/E7、score adapter、uCMP/DCF、permutation、framed transport、E7 入口与三项测试。AAV86 递归图和 Agarwal CA 公式是论文定义；E6/E7 测试是本地实现事实；可信 T 全池、稳定 Top-K、D padding、同 π 逆路由是项目扩展；FSS/PRG/通道安全是**假设**，不能标记为作者证明。

## B. 重新推导的关键合同

1. **节点 mask、key 与首轮 y。** `protocol_i_aav86_small.cpp:287–304` 对每个 t 先取独立 D 维 `full=R_t`，每端点一次分成 R0/R1，再逐 canonical pair 以 `ProtocolIUcmpMaterial(bits,full[a],full[c])` 生成 key。`keyGenDCF` 每次从已用 OS 熵初始化的单线程 PRNG 状态抽下一份币；这是源码生成关系，伪随机性是条件假设。`protocol_i_aav86_small.cpp:565–589` 前向形成两份 shuffle share，t=0 的首次 masked-open 正加同一数组 `node_mask_shares[0,a]`；没有第二套首轮 R。
2. **整池单方视图。** 假定单份 DCF key 对任意选定阈值在独立币下不可区分，包括序列化和任意本地 Eval 的后处理。按 t/a/c 混合替换全部 `r·C(D,2)` 个 key；归约在挑战前采样并固定全部相关 R、双方 share、π、y、其他 keys，故这些辅助量不依赖挑战位。挑战 key 的阈值从 `R_a−R_c` 换为 0 时，腐化方本地 rank share q_p 可变；对方 rank 揭示消息必须同步模拟为 `L−q_p mod 2^log2(D)`。H0 的该消息与真实执行逐位相同，hybrid 后公开 L 及后续图保持不变。活跃边选择是已公开 L/公共 pivot 币的函数，本地选择性 Eval 仍为后处理；未活跃 key 也在包里，不能省掉。此论证还需 score adapter 的单方 key/掩码安全与私有、有序、完整通道。若单份 key 泄露 alpha，则 `y_a−y_c−alpha` 给出键差，归约直接失败。AES/PRG 安全并非 C++ 测试结论。
3. **π 与输出。** 依 `apply(π,x)[a]=x[π[a]]`，前向两消息代入 E7 的 σ/τ/a/e 可得两方 share 和为 `π(x)`；逆路由 γ/δ/a'/e' 使用 fresh 随机币而目标固定 `π⁻¹`。membership 以 `(m,0)` 输入时，P0 发 `γ0(m)+a'0`、收到 `a'1`，最终 share 为 `−h'`；P1 发 `a'1`、收到 P0 消息，最终 share 为 `π⁻¹(m)+h'`。两份最低位 XOR 为原槽 m。对任意候选 π，固定一方的 σ、τ、γ、δ 后，另一方两套随机置换因子各有唯一解，单方联合视图不确定 π。score adapter 的 dummy 是 INT32_MIN 且下标≥n，稳定键低位令任意真实 INT32_MIN 先于 dummy；K≤n 得前 n 槽恰 K 个 1。这里是项目代数/功能结论。
4. **T 拓扑。** E7 E2E 测试先 fork，子进程 `close_except` 后 exec；T exec 参数仅公开 shape/session 与两个 package fd（`protocol_i_aav86_small_e2e_test.cpp:178–200`）。父进程 `wait_ok(dealer)` 后才拆分并发送原始输入份额（同文件 203–216）。在线 score/core/inverse fd 只在 P0/P1 的 keep 集合内。`ProtocolIFramedChannel` 对每次 send/receive 设 poll deadline，peer EOF/POLLHUP 报错；写端遇对方关闭也可能由 SIGPIPE 直接终止，仍为有界失败而非优雅异常。
5. **真实依赖 DAG。** score carry phase 4 → sign phase 5 → C 前向 share phase 9 → 对 t=0..r−1：masked-open phase `10+2t` → 本地活跃 Eval → rank reveal phase `11+2t` → 根据公开 L 决定下层控制 → 末轮公开 full order → 一轮逆路由 phase 40。源码按此顺序调用 `exchange_words`，每个调用一帧 send 和一帧 receive，故审过的新入口 CA 为 `1+2r`，全路径为 `2+(1+2r)+1=2r+4`；`metrics.causal_rounds` 的预填值本身不是证据。E7 tests 的 phase/bytes trace 为这一源码路径的执行佐证。

## C. Findings、影响与最小修正

| ID / 严重度 | 位置 | 复现及影响 | 最小修正 |
|---|---|---|---|
| F1 / P2 文档与测试合同 | `protocol_i_aav86_small.cpp:333–345,389–404`；E7 验证报告 §1/§3 | 找到两个同 bits、同 party 的 edge key blob，保持各自 t/a/c/ID 字段和长度，只交换 blob。解析逐字段通过，因为 key blob 没有自己的端点、R 或 alpha 证明。若两方包同时错配，可造成错误比较；在可信 T 与完整通道模型内，这不是对恶意 T/网络的承诺缺失，而是“内容绑定/错包拒绝”说法过宽。 | 增负面用例确认交换后的包被**接受**，把合同收窄为结构标签/规格校验；另用测试路径的双份 dealer 材料及 R 验证 key/mask 一致性。不得称普通 checksum 为认证。 |
| F2 / P2 规模门 | `protocol_i_aav86_small.cpp:37–45,108–116,226–306,310–344` | D>8 直接拒绝；现仅在序列化末尾检查 64 MiB，若放宽 D 会先 keygen/分配。 | 用确切布局公式和溢出检查在 keygen 前预检每方字节、槽数与资源预算，然后有界扩到 D16/32/64，D128 仅预检安全后探测。 |
| F3 / P2 验证覆盖 | E7 三项测试及原始日志 | 没有保持 peer 打开但静默的 timeout 负面用例，未直接复验异常终止时的有界退出；E2E 未记录 public pivot seed 或每方离线包字节。 | 增隔离 timeout/abort 用例；逐配置保存 public pivot seed、实际序列化长度及测试输入种子。密码材料种子不输出。 |
| F4 / P1 程序性 | 本聊天来源 | 本聊天参与 E6/E7，无法满足用户所需异会话接收身份。技术复核无法消除此事实。 | 交付源码和可复跑证据；由真正未参与 E6/E7 的新窗口复核并签收，报告始终标明这一缺口。 |

持久 claim 的目录由调用方管理，必须是跨进程重启保留、同一 party/session/material ID 共享的受控目录。`protocol_i_aav86_small.cpp:447–466,542` 在任何输入依赖的在线 frame 前 `openat(O_EXCL|O_NOFOLLOW)`、写 marker 并 fsync 文件与目录；同名 claim 在重启后 fail closed。磁盘删除/目录替换、恶意 party、T 合谋、侧信道不在模型。超时依赖 transport 的单帧 deadline，不覆盖操作系统永久阻塞或任意 I/O 系统故障。

**处置顺序：**本报告先保存；再加入 F1 的失败证明测试与合同修正、F3 负面测试及规模预检。技术门修复验证后才进入中等 D。独立签收状态仍按首段保持 `FAIL`，不得在后续报告偷换为人员独立 PASS。
