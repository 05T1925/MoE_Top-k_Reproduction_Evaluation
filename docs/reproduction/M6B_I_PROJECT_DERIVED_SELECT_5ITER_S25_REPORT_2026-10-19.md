# M6B-I-PROJECT-SELECT-5ITER-S25 复核报告

日期：2026-10-19
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
基线提交：`c3926c68fd14f270faa8b55234311071947fa080`（S24/S25 worktree 的起始提交；候选未合入该提交）
S24 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s24\MoE_Top-k_Reproduction_Evaluation`
S25 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation`
S24 候选 canonical SHA-256：`585d4fd60f8cc04ad4b14bf02730882084cfbb2bd7045d9b2ca36c956fc51438`
S25 测试扩展快照 canonical SHA-256：`64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`

## 1. 裁决摘要

| 门 | 状态 | 结论 |
|---|---|---|
| S24 74 文件对象身份 | `PASS_WITH_SCOPE` | 按 S24 manifest 重算源文件哈希；S24 原 canonical 可复现。 |
| 真正异会话独立接收 | `NOT_SATISFIED` | 当前执行者参与 S19–S24 推导，fresh replay 不能冒充独立接收。 |
| v1 完整 priority-key shares | `PASS_WITH_SCOPE` | 在实际 DCF correctness 下可重构稳定全序键；v1 的局部高位 q 仅因低位 residue 特殊布局成立。 |
| v2 零和完整键重构 | `PASS` | 在 `R_c=Z/(2^c)` 中对每个 rho、含回绕都保持完整键和。 |
| v2 局部 `q′` | `OPEN/NO_GO` | 一般不满足份额右移可组合；源码消费者不使用该局部 q。 |
| v2 定向功能回归 | `PASS_WITH_SCOPE` | 本轮 fresh ON/OFF、原语/材料/transport 测试及 16 个成功、6 个故障、6 个 crash-prefix 执行记录通过各自测试检查。 |
| 实际 DCF 单 key 隐私桥接 | `CONDITIONAL/UNQUANTIFIED` | S25 未由非作者独立重证 BCG21 定理到实际受限 key/辅助输入的归约；AES `T′/Adv` 与联合 RNG 项无量化值。 |
| 输出条件模拟 | `CONDITIONAL/UNQUANTIFIED` | 刷新等式仅适用于不含 rho 的有限前缀视图，并假设 rho 与该视图及前缀泄露联合独立；完整党方视图尚无 sampler。 |
| DCF 池自适应、inverse/output/crash 联合安全 | `OPEN/NO_GO` | 本轮功能证据不证明隐私；没有完整 P0/P1 联合视图模拟。 |
| 攻击/运行时缺陷 | 未发现 | 未找到区分攻击或可复现运行时错误；这不是安全通过。 |
| 正式性能 | `NOT_ALLOWED/NOT_RUN` | M6A V3 仍 OPEN、V4 未授权；本地功能计数不属于性能结果。 |

“独立接收”门未满足是人员关系事实，不因复跑 PASS 改写。S25 仍可作为相同执行者针对 S24 的 fresh build/replay 与数学检查记录，为真正非作者接收者提供可重放证据包。

## 2. 接收对象、差异与执行者关系

S24 候选由 S24 source/build/evidence manifest 锁定的 72 个 S15 基线文件和 2 个新增源文件构成，共 74 个。S24 的 canonical 规则为 UTF-8 字节、相对路径 ordinal 升序，每行 `path<TAB>lowercase_sha256<LF>`。本轮用 S24 声明的 74 个路径重新读取文件：

- 复算 S24 基线 74 文件，canonical `585d4fd6…fc51438`，与 S24 声明值一致。
- S25 当前候选 74 文件中 71 个 SHA-256 与 S24 完全相同；3 个有差异的文件全部是测试源文件，没有运行时代码变更。
- S25 74 文件当前 canonical 为 `64f32993…8e440d`。源码清单与逐文件前后哈希见 source/build/evidence manifest。
- S25 新增的报告/决策/gates/case JSON/manifest 是审计产物，不计入上述 74 个被测候选源码路径。

本执行者参与过 S19–S24 的算法/证明推导。故 `INDEPENDENT_RECEIPT=NOT_SATISFIED` 且 `INDEPENDENT_CRYPTO_REVIEW=NOT_SATISFIED`。本轮只可称为“隔离 worktree fresh replay / 同一执行者的技术复核”。S18 的 S15 功能接收既不覆盖 S24 v2，也不能补足本轮独立性。

### S25 对 S24 的测试文件增量

| 文件 | S24 SHA-256 | S25 SHA-256 | 增量 |
|---|---|---|---|
| `VFSS/tests/moe_topk/protocol_i_score_key_refresh_v2_test.cpp` | `9a2c92b03c2117ffe3539341be201b0a553d0f6e79834e2e4bd663b0ab4cb92a` | `d4cf7d44f862118875c86a0d1252919f6fc78341d4e7e7ed7f348dc245e5a00d` | 增加错误 party 与缺失 refresh-slot 拒绝用例。 |
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_material_store_test.cpp` | `32bb24c273a41de47cecbfa9665b78285d36aef292be352a0590ec92f169a2c3` | `c4f1359784414c5f15fa5627c717e3d042487fd8b419ebab707ca6a878e097f8` | 增加 v2 wrong-session/wrong-party 在 lease 前拒绝、收件方字节与 durable replay 检查。 |
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp` | `10a019af62dd04c7a6dc84fecfe76c230f52ea3162ec5b677d4fcee084b0e55b` | `00822853dd462be6fd06d4d8ea1c00f746e278b109e1fb41078f9ac3bd02583c` | 增加 `--score-share-v2-acceptance`，覆盖 N=1、边界 K、N=128/256、fault 与六个 crash-prefix，并由 TEST_ONLY supervisor 差分 oracle。 |

这些改动只扩大测试面，没有 runtime/API/材料格式变化。未改 S24 或历史证据文件。

## 3. 功能与稳定键语义复核

### 3.1 精确环和 v1 share/record 语义

源码路径：

- `VFSS/src/moe_topk/protocol_i_score_input.cpp:64–75`：校验 Q20.12 输入 share、padding、session/party 与输入材料。
- 同文件 `:79–141`：carry/sign score conversion；R34 carry/lift/sign 路径，`lift=(x−((carry&3)<<32)) mod 2^34`，然后组合成 score key share。
- 同文件 `:131–135`：将 sign 输出映射到 `q_b`，生成 `Q_b=(q_b<<h)+residue_b mod 2^c`，其中 P0 的低 h 位为 index `i`、P1 为 0。
- 同文件 `:156–170`：v2 helper 对完整 `c` 位 share 执行 `(Q_b+rho_b) & ring_mask`，拒绝长度错误或超出 ring 的 rho。
- `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:2022–2045`：score conversion 后消费完整 `Q′_b`，形成 `{Q′_b,0,0}` records 并交给 forward shuffle；没有局部 q′ 的下游消费者。

对 logical input N，算法使用 `padded_n` 并定义 `h=index_bits_for(padded_n)`、`c=33+h`、`R_c=Z/(2^c)`。容器使用 `uint64_t` 只是存储宽度；逻辑值经 `2^c−1` mask/高位约束编码。N=128 时 h=7、c=40；N=256 时 h=8、c=41。序列化整数为 64-bit big-endian，但其有效值受 c 位约束；不能把 64-bit 容器误称为数学环 `R_64`。

令 `score_i` 为 signed Q20.12 的 32-bit 有符号整数值。DCF correctness 条件下，重构键为：

```text
κ_i = ((0x7fffffff − score_i) << h) + i  (mod 2^c)
Q_0[i] + Q_1[i] = κ_i (mod 2^c)
```

无符号 key 升序等价于 score 降序，并以原输入下标升序稳定打破平分。这里“准确”是实际 DCF correctness 和输入域契约下的代数结论，不是安全证明或随机失败概率结论。

一般加法 share 写作 `Q_b=2^h q_b+r_b`，则两个局部商重构只在 `r_0+r_1<2^h` 时等于总键高位。v1 结构固定 `r_0=i,r_1=0` 且 `i<padded_n≤2^h`，所以进位为 0，局部 q 可组合。该性质不是对任意加法 share 的通用规则。S24/S25 的运行时后续实际使用 full key，不依赖独立 q API。

### 3.2 v2 zero-sum refresh 的正确性与 q′ 边界

逐槽在 `R_c` 中：

```text
rho_0 ← U(R_c^N)
rho_1 = −rho_0 mod 2^c
Q′_b = Q_b + rho_b mod 2^c
Q′_0 + Q′_1 = Q_0 + Q_1 = κ mod 2^c
```

以上等式对每条随机带成立，包含模回绕；实现使用 ring mask，不做饱和或有符号算术转换。两方 `Q′` 仍相互相关，并非独立 share pair。对任意一个 party，在 ideal uniform `rho_b` 下，其单方 `Q′_b` 边缘为 `R_c^N` 均匀；这不推出它与其它 party-visible preprocessing 的联合分布可模拟。

局部右移反例仍然成立：h=1、c=34、Q0=Q1=0、rho0=1、rho1=2^34−1，则重构完整键 0，但 `Q′0>>1 + Q′1>>1 = 2^33−1 mod 2^33`。因此 `q′` 不可作为高位加法 share。S25 搜索实际消费者，确认 Select5 直接消费完整 `(Q′_b,0,0)`；没有找到重新局部移位/窄化路径。该不兼容是 API/数学范围边界，不是当前运行时错误。

v2 的材料/绑定：

- N≤128：M6B5/v4 monolithic package；party package 头、session/fingerprint/N/K/width/party/score-share ABI 校验，rho 为每 padded record 一个 8-byte big-endian 字段；N=128 精确格式增量 `8 + 8·128 = 1,032 B/party`。
- N=256：M6S2/v3 streamed package；每个 padded record 一个 purpose=5 refresh slot，每 slot payload 8 B，按 session、party、用途、端点、score-share ABI 一次性寻址/claim；manifest entry 76 B，N=256 新增 2,048 B payload 与 19,456 B manifest 描述，共 21,504 B/party 布局增量。文件系统分配开销和峰值 RSS 未测。
- S25 复核了 M6B5/v4 ready/package 版本兼容、wrong party/session、缺失 slot、损坏/篡改、重领与 lease 前拒绝的测试范围；不把这些格式测试写成密码学 key-pool 证明。

源码中两种 schema 的逻辑材料项可分别拆为 `5·C(N,2)` Select DCF 项、`10N` shuffle metadata 项（forward/inverse 各 5 个 N 长数组）、`2N` carry/sign DCF 项、`N` membership DCF 项以及 v2 新增 `N` refresh 项。N=128：Select 40,640、shuffle metadata 1,280、carry/sign 256、membership 128、refresh 128，合计 42,432 个逻辑条目；M6B5/v4 把它们放在 monolithic package 的结构化段中，refresh 部分是 128 个 share 字段，不是单独的 M6S2 lease slot。N=256：Select 163,200、shuffle metadata 2,560、carry/sign 512、membership 256、refresh 256，M6S2/v3 总计 166,784 个按地址寻址的 slot。数量由 `protocol_i_project_derived_select5.cpp` 的 Select/score/shuffle/membership/refresh writer 和 `streamed_slot_count()` 逐项相加；这是结构计数，不是整池隐私证明。

实测布局观察与格式推导分开：N=128 v2 E2E package 为 44,986,628 B/party；N=256 v2 E2E staged store 共 385,331,840 B，两方各 192,665,920 B，manifest 各 12,675,648 B，共 11 segments。N=256 每方 21,504 B 的 refresh payload+manifest 是新增布局推导，不能从整包观察值中误认为峰值 RSS。峰值 RSS 与生产存储开销仍 `NOT_MEASURED`。

## 4. 输出条件模拟：等式的精确作用域

### 4.1 不能把 share 边缘均匀说成 full-view 模拟

S24 的 `V_pre` 应准确理解为 S22 carry/sign 范围内的本方 score-conversion 局部前缀视图：本方输入 share、本地 carry/sign materials 与 masks、两次阶段的本地 Eval 状态、phase 4/5 已有 masked frames、旧 `Q_b`。它明确不包括新 refresh `rho_b`，也不等于完整 party package/lease、全部已用和未用 DCF pool、Select 后续 transcript、shuffle 本地 permutation state 或输出发布状态。

`rho_b` 在实际实现中由可信离线 T 在 ready 前生成并写入本方材料：M6B5/v4 将其放在 monolithic package 字段中，M6S2/v3 将其作为 purpose=5 slot 预置到 streamed store；运行时 claim 后才执行 score conversion。因而 party 的完整材料/初始 view 已包含本方 rho（以及 package/manifest/lease 信息），它不是在线转换后才新抽取且此前不可见的随机变量。S24 的 `V_pre` 是为了写条件等式而定义的缩小视图；不能把它不含 rho 的定义当作真实完整 party view。

对腐化方 b、固定正概率的 `(x_b,L_prefix)`，若 `rho_b` 在 `V_pre,L_prefix` 条件下仍均匀独立，且 `V_pre` 包含旧 `Q_b`，才有：

```text
Pr[V_pre=v, Q′_b=q′ | x_b,L_prefix]
  = Pr[V_pre=v | x_b,L_prefix] · 2^(−cN).
```

其计算是：给定 `V_pre=v` 后，满足 `Q′=q′` 的唯一刷新向量为 `rho=q′−Q_b(v) mod 2^c`，该向量概率 `2^(−cN)`。这只证明刷新份额与该前缀视图的条件独立，且不构造 `V_pre | x_b,L_prefix` sampler。

这里的 `q′` 是理想 `F_scoreconv/v2` 输出给该方的 priority-key share，不是最后的 Top-K mask share。分别对腐化 P0、P1，候选模拟顺序相同但是两个独立的单方实验：给定本方 `x_b`、本方理想 `Q′_b` 和获准的 scoped `L_prefix`，先从基准的 `V_pre | x_b,L_prefix` 分布采样 v（其中要含旧 `Q_b(v)`），再计算唯一的本地 rho `Q′_b−Q_b(v)`。P0 的 rho 是 dealer 原始随机向量；P1 的 rho 是其环上相反数，单方边缘同为均匀。不能把两个单方 sampler 拼接成一个双方联合 sampler：`rho_1=−rho_0` 是双方相关约束，且 T+party 合谋不在腐化模型内。

将 `rho_b` 放回本地 view 后，正确联合 law 是：

```text
Pr[V_pre=v, rho_b=r, Q′_b=q′ | x_b,L_prefix]
 = Pr[V_pre=v | x_b,L_prefix] · 2^(−cN)
   · 1[q′=Q_b(v)+r mod 2^c].
```

给定 ideal output share `q′` 的候选步骤是先从 `V_pre | x_b,L_prefix` sampler 采样 v，再置 `rho=q′−Q_b(v)`。它在该独立均匀假设下恢复 `(V_pre,rho,Q′)` 的联合分布。若 `V_pre` 还要包含 dealer 生成的其它 key/material、共享 PRG tape 派生量、package manifest/lease/restart 状态，则必须将这些变量一并纳入并证明相应条件分布；现有 S24 推导未提供这一全视图 sampler。

### 4.2 两方与随机带条件

对 P0，`rho_0` 来源于 T 生成的随机量；对 P1，`rho_1=−rho_0` 在环上是均匀边缘，但两方 rho 相关。单方模拟可以在 trusted T 不合谋、被模拟 view 只含本方 package 的模型下考虑自己的 rho；如果完整 P0/P1 package 或 T+party 视图被并入，零和关系立即相关，且 T 与 party 合谋在本路线模型之外。

实现用随机源的源码事实与假设边界：score masks/refresh shares 与 shuffle 的 `random_bytes` 使用 Linux `getrandom`；FSS/DCF root 随机量使用 OpenSSL `RAND_bytes`；部分 shuffle PRNG seed 从 `getrandom` 初始化。源码只表明调用不同 API，不证明输出联合统计独立。把它们替换为 ideal joint tape 是一个条件化理想模型；具体 `epsilon_RNG_joint^(v2)` 需覆盖刷新、输入 masks、其它 package materials、shuffle tape、seed 以及党方可见辅助信息，当前未量化，也无具体安全位数。

公开 sample seed 仍由 P0 在前向 shuffle 完成后按活动迭代生成并发送；S25 E2E 使用 `verify_sample_seed_wire_order` 检查在线记录中的顺序。它不进入 refresh rho package/manifest，也不作为 rho 或 DCF alpha。此顺序测试是协议执行 trace 检查，不扩展为完整联合安全模拟。

DCF 构造桥接仍依赖 S22/S24 所述 BCG21 Figure 1/Theorem 2/Appendix F.1 到实际 `M2B1/v1` 的条件映射。候选单 key 项 `epsilon_G ≤ Adv_AES128^PRP(T′,q=5)+10/2^128` 不是本轮重新证明的全协议界；`T′`、总 query 范围和跨槽联合随机带优势未量化。TLS 1.3 mTLS 测试验证的是功能/身份/绑定 conformance，不是信息论信道优势；`epsilon_TLS` 也保持 `CONDITIONAL/UNQUANTIFIED`。

### 4.3 Forward wire projection 与完整视图的差别

给定完整重构 key `κ`，v2 refresh 不改变它。按 S24 描述的 M2CM 前向 shuffle 方程，理想独立 uniform pad 可让指定 masked wire frame 在条件于 key 时呈均匀分布。该推理最多支持受限的 forward **wire projection**；它不采样本地 `sigma/tau/a/e/r` 与共同 `Pi/Pi⁻¹` 的完整相关状态，也不处理 package 中其它 slots、后续 comparison bits、adaptive address、inverse route、membership、最终 output share、PREPARE/COMMIT、crash/restart 或孤立的 8-byte XOR share。

因此，当前最先需要补齐的是一个无需真实 transcript 的 `V_pre | x_b,L_prefix` 生成器及其与完整 M6B5/v4 或 M6S2/v3 单方材料/lease view 的联合分布证明；即便该步成立，下一道证明仍是从一次 UCMP 的同 key 两次 Eval/打开比较 bit到自适应整池的条件模拟。未找到可利用区分器；这是未闭合证明，不是攻击报告。

## 5. 功能接收复跑

### 5.1 构建环境和配置

- Worktree：S25 独立路径；分支 `codex/m6b-project-select5-s25`；起点 HEAD `c3926c68fd14f270faa8b55234311071947fa080`。
- WSL Ubuntu 20.04 / Linux 6.6.87.2-microsoft-standard-WSL2；GCC/G++ 9.4.0；CMake 3.31.6；OpenSSL 1.1.1f；Unix Makefiles；Release；`BUILD_TESTING=ON`。
- CXX flags：`-Wno-write-strings -Wno-unused-result -maes -Wno-ignored-attributes -march=native -Wno-deprecated-declarations -O3 -DNDEBUG -std=gnu++17 -fopenmp`。
- ON：`MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON`；OFF 为 `OFF`。
- Build dirs：`/mnt/c/Users/28641/.codex/tmp/m6b_s25_on_v2` 与 `/mnt/c/Users/28641/.codex/tmp/m6b_s25_off_v2`，均在仓库外。
- CMakeCache SHA256：ON `96c31b65c84f3b0a8e41a95d29236d6e589f0d195c89f0631459f73d5904a766`；OFF `4c13f2ce85321b503fe3a33030afe175746ff187a44e8f2b68e2c3fc104ae7f8`。
- `libsytorch.a`：ON 2,621,398 B，SHA256 `8a837ab74680ec815740ecdb0b2aba95f105afcad3e7b55fbb6d69364b755ac9`；OFF 2,122,218 B，SHA256 `650efb7d286b343db53e0e1b50f6d4e62ee2d544427c371631ba2c0b1cb66d0b`。
- 详细 target binary hashes、源码清单、logs hashes 见 manifest。

### 5.2 命令与结果

Release ON 配置：

```sh
cmake -S VFSS -B /mnt/c/Users/28641/.codex/tmp/m6b_s25_on_v2 -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
```

Release OFF 同样配置但 build dir 为 `m6b_s25_off_v2` 且选项为 `OFF`。ON 定向 CTest：

```sh
ctest --test-dir /mnt/c/Users/28641/.codex/tmp/m6b_s25_on_v2 --output-on-failure \
  -R 'moe_topk_score_key_refresh_v2_test|moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test|moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test'
```

结果 `7/7 PASS`。OFF 旧路径：

```sh
ctest --test-dir /mnt/c/Users/28641/.codex/tmp/m6b_s25_off_v2 --output-on-failure \
  -R 'moe_topk_m2_ucmp_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test'
```

结果 `3/3 PASS`。另外独立执行 `moe_topk_streamed_material_slot_store_test` 输出 `segmented stream, slot binding, durable one-shot, crash, race, integrity PASS`；TLS 独立测试输出 `TLS 1.3 mTLS frame binding and peer identity conformance PASS`。

ON 定向 target 集：`sytorch`、`moe_topk_score_key_refresh_v2_test`、`moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test`、`moe_topk_project_derived_select5_conformance_test`、`moe_topk_project_derived_select5_material_store_test`、`moe_topk_m2_ucmp_conformance_test`、`moe_topk_m2_score_input_conformance_test`、`moe_topk_m2_parallel_shuffle_conformance_test`、`moe_topk_streamed_material_slot_store_test`、`moe_topk_project_derived_select5_process_e2e_test`、`moe_topk_tls13_transport_conformance_test`。复跑 target 的单项命令形式为 `cmake --build <build-dir> --target <target> -j2`；本轮保存的构建证据是对应 CTest/独立可执行输出、产物哈希与最终完整构建失败日志，未保留 shell history 中逐个 target 的原始命令行。

API/默认关闭检查：feature-ON external include/link probe 成功；OFF 头文件编译按约定失败（没有 `protocol_i_refresh_full_priority_key_shares_v2` 声明），OFF `libsytorch.a` 中符号 absent、ON 符号存在，OFF target help 中 v2 test target absent。ON symbol scan count=3。探针源码与对象 SHA 见 manifest。

全量 `cmake --build ... -j2` 退出 2：`VFSS/ext/bitpack/tests/test.cpp` 的 `bitpack::mod(unsigned long,int)` undefined reference。该错误在第三方 ext/bitpack test 链接；本轮未改该依赖。定向目标已构建并通过。不能把“全仓 Release build 成功”写入结论。

E2E 命令（WSL TEST_ONLY supervisor，root 只负责隔离目录/进程管理，party 降权运行）：

```sh
wsl -d Ubuntu-20.04 -u root -- bash -lc \
  'cd /mnt/c/Users/28641/.codex/tmp/m6b_s25_on_v2 && ./moe_topk_project_derived_select5_process_e2e_test --score-share-v2-smoke'
wsl -d Ubuntu-20.04 -u root -- bash -lc \
  'cd /mnt/c/Users/28641/.codex/tmp/m6b_s25_on_v2 && ./moe_topk_project_derived_select5_process_e2e_test --score-share-v2-acceptance'
```

外部只追加证据目录：`C:\Users\28641\.codex\evidence\M6B_I_S25_20261019_01`、`..._02`。对应 raw logs 的 SHA256/字节数写入 manifest。repo case JSON 有意剔除 session、fingerprint、material ID、input-share seed 和原始 frames；原始日志未复制进仓库。

### 5.3 逐案范围

机器可读全表为 `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_E2E_CASES_2026-10-19.json`。共记载：

- 16 个成功运行：S24 smoke 中 1 个 v1 对照与 6 个 v2，再加 S25 acceptance 中 9 个 v2；每个成功样例由 TEST_ONLY supervisor XOR 合并 shares，与冻结 C++ stable oracle 逐位比对，并检查原序、长度、二值、weight=K。检查代码在 E2E 测试 `oracle_mask(scores,k)`、逐 i 的 `mask0[i]^mask1[i]` equality 和 `weight==k`。
- S25 v2 成功矩阵新增 N=1/K=1 padded singleton，N=2 的 K=1、2，N=7 K=1/7（重复分数、signed Q20.12 边界和 padding fixture），N=128 K=1/64/128，N=256 K=128 streamed。N=256 单个 K 并不代表全部边界 K。
- 6 个故障：包篡改、peer disconnect、wrong K binding、TLS wrong identity、T 在 ready 前退出、malformed sample seed。双方退出失败状态且不发送编码 mask。
- 6 个 v2 crash-prefix：步骤 1–4 双方输出字节均为 0；步骤 5–6 P1 留下 8-byte 本地 XOR share，P0 为 0；均判 `NO_COMPLETE_MASK`。这不是原子无输出/all-or-nothing，也不代表本地 orphan share 不会留存。
- smoke 还复核 N=256 monolithic 在 generation 前拒绝、N=257 streamed preflight 拒绝。
- 测试的 scores 固定 fixture 包含 signed 极值、负值、0 和重复项；覆盖的是这些有限输入与执行种子，不证明所有输入或概率界。

诊断比较调用数、层数、交换数及 package bytes 在 case JSON 中仅作为该次功能执行的计数/布局观察。不得称性能、期望成本、复杂度上界或正式容量门通过。峰值 RSS、生产文件系统开销和跨主机材料服务 `NOT_MEASURED`。

## 6. 失败、重试和未运行项

- 全量 ON build 在 ext/bitpack test 链接失败；记录保留在外部 `full_on_build.log`，没有通过重试掩盖。定向目标构建与 ON 7 项 CTest 通过。
- 初始系统 CMake/Ninja 配置不可用（系统 CMake 版本不支持本项目配置；Ninja generator 不存在），转用 bundled CMake 3.31.6 与 Unix Makefiles。此项是工具/配置尝试失败，不是测试失败。
- 初始 API probe 曾因 cryptoTools include 路径及 PowerShell/WSL 路径变量传递不正确失败；修正命令后 ON probe 通过、OFF probe 按预期编译拒绝。最终输出日志和对象哈希均保留；早期命令行错误作为过程记录，不算功能回归失败。
- 本任务没有重跑 S24 已有全部历史 case；重跑 smoke 7 case 并新加 9 success、6 failure、6 crash-prefix。对每项命令及原始 log identity 见 case JSON 与 manifest。
- 未运行：大规模全随机 tape 正确性证明、正式概率成功率实验、对抗性密码分析、non-author S24 receipt、密码学独立审查、完整 P0/P1 view simulation、所有 N=256 K 边界、N>256、生产 peak RSS/cross-host delivery、LAN/WAN 正式矩阵。

## 7. 最终安全结论与下一步

### 当前可声称

- 项目派生 v2 full-key zero-sum refresh 在 `R_c` 上保持 priority key 重构；测试覆盖的 M6B5/v4 和 M6S2/v3 功能路径与冻结 oracle 相符，feature-OFF 下 v2 API/目标不可用。
- 在 ideal independent refresh tape 与 S22 carry/sign prefix sampler 已成立的前提下，`Q′_b` 对其限定的、不含 rho 的 `V_pre` 前缀具有条件均匀关系。数学等式可复算，但本执行者不是独立审查者，具体随机源和 AES/TLS 项未量化。
- 指定测试故障范围中的统一 abort/no-complete-mask；部分 crash 可留下 P1 的孤立 XOR share。

### 当前不能声称

- 不能称为 BB90/BMW16 复现、不能继承其轮数/复杂度/概率保证。
- 不能声称 S24 真正异会话独立接收、完整 F_scoreconv party-view 模拟已闭合、DCF 单 key/整池自适应安全已独立证明，或 M2CM inverse/membership/output/crash 联合视图安全通过。
- 不能称为 `secure`、恶意安全、生产就绪、所有合法输入都由本有限测试证明正确，或将本地计数作为性能数据。
- M6A V3 仍 OPEN；V4 未获准，因此正式 LAN/WAN 矩阵 `NOT_ALLOWED/NOT_RUN`。

### 下一项最小工作

交给一名未参与 S19–S25 实现与证明的密码学审查者，在 S25 source manifest 的精确运行时身份上独立：

1. 先复核 `V_pre` 完整字段清单与刷新 rho 的条件独立性，将 party 本地材料包、其它未使用/已使用预处理状态、lease/restart 状态纳入同一条件分布；给出 P0、P1 的 `V_pre | x_b,L_prefix` sampler 或明确指出它不存在/不由当前定理推出。
2. 只有该引理通过后，按 schema 分别审查 M6B5/v4 与 M6S2/v3 的第一条自适应 DCF/UCMP 调用。不要从此直接外推到 inverse/output/crash。
3. 要求把 `epsilon_RNG_joint`、AES reduction `T′/Adv`、TLS channel 优势分别正式实例化或继续标 `UNQUANTIFIED`。

在这项非作者复核完成以前保持 EXPERIMENTAL、不创建 alias。没有代码缺陷时无需为证明缺口再改 runtime。
