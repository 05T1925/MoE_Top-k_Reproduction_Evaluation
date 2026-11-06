# M6B-I-PROJECT-SELECT-5ITER-S26 复核报告

日期：2026-10-19  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 1. 结论摘要

| 门 | 状态 | 结论 |
|---|---|---|
| S25 74 文件对象身份 | `PASS_WITH_SCOPE` | 在新 worktree 按 S25 manifest 重建，74/74 哈希匹配，canonical SHA-256 为 `64f32993…8e440d`。|
| 真正异会话独立接收 | `NOT_SATISFIED` | 当前执行者参与过 S19–S25；本轮为新隔离 worktree 的技术复跑。|
| S24→S25 运行时差异 | `PASS_WITH_SCOPE` | 重算 74 文件，只有 3 个测试源不同；运行时代码哈希未变。|
| v2 完整键环正确性 | `PASS_WITH_SCOPE` | 零和刷新在 `R_c` 中对全部随机带保持键和；运行时消费者使用 full-key records。|
| v2 定向功能 | `PASS_WITH_SCOPE` | 新 Release ON/OFF 定向回归、9 个成功 E2E、6 个故障及 6 个 crash-prefix 达到报告范围。|
| P0 完整输出条件 sampler | `OPEN/NO_GO` | 缺少从 `(x_0,L)` 生成完整 score-conversion 本地前缀、party package 其他材料及 lease 状态的 sampler。|
| P1 完整输出条件 sampler | `OPEN/NO_GO` | 同上；`rho_1=-rho_0` 的单方边缘均匀不构成其完整视图模拟。|
| N=128/N=256 DCF 整池与自适应访问 | `OPEN/NO_GO` | 完整输出条件 sampler 未通过，故不向下组合；材料数量只作结构复算。|
| shuffle/inverse/output/crash 联合模拟 | `OPEN/NO_GO` | 没有全视图 simulator；指定 crash-prefix 的证据只支持 no-complete-mask。|
| 攻击或运行时缺陷 | 未发现 | 没有找到攻击或可复现 runtime defect；证明未闭合不能写成安全通过。|
| 正式性能 | `NOT_ALLOWED/NOT_RUN` | M6A V3 仍 OPEN、V4 未获准；本轮未跑 LAN/WAN 矩阵。|

## 2. 被测对象与独立性

- S25 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-project-select5-s25`，起点 HEAD `c3926c68fd14f270faa8b55234311071947fa080`，候选未提交。
- S26 隔离 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s26\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-project-select5-s26`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`。
- S25 manifest：`M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json`。复核时逐条复制 manifest 列出的 74 个文件，不复制整个 S25 dirty tree。S25 canonical `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d` 与 S24 `585d4fd60f8cc04ad4b14bf02730882084cfbb2bd7045d9b2ca36c956fc51438` 分别复算一致。
- 重新读取 S24 worktree 的同一 74 个路径后，确认仅以下测试源文件发生变化：

| 文件 | S24 SHA-256 | S25 SHA-256 | 变化 |
|---|---|---|---|
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_material_store_test.cpp` | `32bb24c273a41de47cecbfa9665b78285d36aef292be352a0590ec92f169a2c3` | `c4f1359784414c5f15fa5627c717e3d042487fd8b419ebab707ca6a878e097f8` | 增加错 session/party、收件方字节与 durable replay 检查。|
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp` | `10a019af62dd04c7a6dc84fecfe76c230f52ea3162ec5b677d4fcee084b0e55b` | `00822853dd462be6fd06d4d8ea1c00f746e278b109e1fb41078f9ac3bd02583c` | 增加 v2 acceptance E2E。|
| `VFSS/tests/moe_topk/protocol_i_score_key_refresh_v2_test.cpp` | `9a2c92b03c2117ffe3539341be201b0a553d0f6e79834e2e4bd663b0ab4cb92a` | `d4cf7d44f862118875c86a0d1252919f6fc78341d4e7e7ed7f348dc245e5a00d` | 增加错 party 与缺失 refresh slot 拒绝用例。|

执行者参与 S19–S25 的实现、设计、测试或证明推导，故 `INDEPENDENT_RECEIPT=NOT_SATISFIED` 和 `INDEPENDENT_CRYPTO_REVIEW=NOT_SATISFIED`。S18 的独立接收对象是 S15 72 文件快照，不能覆盖 S24/S25 的 v2 候选。S25/S24 原始工作树没有修改。

## 3. v1/v2 share 与材料合同

### 3.1 代码路径

- `VFSS/src/moe_topk/protocol_i_score_input.cpp:64–75` 验证 party、session、N/K、位宽、输入 share 和 carry/sign 材料绑定。
- 同文件 `:90–124` 构造 carry masked operands，打开 DCF carry share 并计算 `lift=(x-((carry&3)<<32)) mod 2^34`，再构造 sign masked operands、打开 sign share。
- 同文件 `:125–141` 由 sign/lift 生成 priority-key share `Q_b`。
- 同文件 `:156–169` 执行 v2 完整键刷新：检查 shape/高位，计算 `(Q_b+rho_b) & (2^c-1)`。
- `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:390–405` 用 OS `getrandom` 读取随机字节；`:401–405` 的 `random_u64()` 是该 API 的包装。`keygen()` 中实际 DCF root 通过 `RAND_bytes` 生成，见 `VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp:253–261`。
- `protocol_i_project_derived_select5.cpp:1446–1459` 在 Dealer 生成 carry/sign 材料后生成 v2 零和 rho；`:1628–1639` 为 streamed layout 写 purpose=5 refresh slots。
- 单体格式将 `rho` 作为 package 字段序列化/读取，见 `:1171–1178`、`:1280–1290`；流式路径在 Select 前 claim purpose=5 槽，见 `:1089–1101`。
- 运行时 `:2022–2045` 先完成 raw-score adapter，再加完整 refresh share，然后直接构造 `{keys[i],0,0}` records 进入 forward shuffle。S26 源码搜索未发现 v2 的 `Q′>>h`、截断或窄化 consumer。v1 的旧 `q` 构造仍保留在输入 adapter，不等同于 v2 对刷新后 shares 做局部右移。

### 3.2 环与稳定键

令 `h=index_bits_for(padded_n)`、`c=33+h`、`R_c=Z/(2^c)`。N=128 时 `(h,c)=(7,40)`；N=256 时 `(h,c)=(8,41)`。字段存为 `uint64_t`，但逻辑值通过 `2^c−1` ring mask/范围检查限制；64-bit 容器不把环变成 `R_64`。序列化为 8-byte big-endian word，读写函数为 `put_u64/get_u64`（同文件 `:441–480`）。

对合法 signed Q20.12 原始 score 的 32-bit 表示 `score_i`，实际重构键为：

```text
κ_i = ((0x7fffffff - score_i) << h) + i  mod 2^c
Q_0[i] + Q_1[i] = κ_i  mod 2^c
```

在 DCF correctness 下，无符号升序对应 score 降序，低 h 位的 original index 使同分稳定。S25 v2 采用 `rho_0[i] ← U(R_c)`、`rho_1[i]=−rho_0[i]` 和 `Q′_b[i]=Q_b[i]+rho_b[i]`，所以对每条 tape 都有 `Q′_0+Q′_1=κ`，包含模回绕。该等式是代数/功能合同，不证明 key 或完整 party view 的隐私。

v2 局部商不构成通用加法 share：若 `Q′_b=2^h q′_b+r′_b`，则 `q′_0+q′_1` 可能带有 `r′_0+r′_1` 的进位。因此禁止把 `Q′_b>>h` 当作可重构高位份额。S26 所审 runtime 实际传递 full key，未发现该错误 consumer。

### 3.3 两种材料 schema（不合并）

由源码的 `streamed_slot_count(n)=5·C(n,2)+13n` 及 v2 的额外 `+n` 重新计算：

| Schema | Select DCF | Shuffle metadata | Carry/sign DCF | Membership DCF | Refresh | 合计 | 存储边界 |
|---|---:|---:|---:|---:|---:|---:|---|
| N=128 `M6B5/v4` | 40,640 | 1,280 | 256 | 128 | 128 | 42,432 个逻辑条目 | monolithic package；refresh 为每个 padded record 一个 8-byte 字段，不是独立 M6S2 lease slot。|
| N=256 `M6S2/v3` | 163,200 | 2,560 | 512 | 256 | 256 | 166,784 个 one-shot slot | purpose=5 的每条 refresh slot 为 8-byte payload，purpose/session/party/参数绑定。|

N=128 v2 E2E 观测 package 为每 party 44,986,628 bytes。N=256 E2E 观测每 party store 为 192,665,920 bytes、manifest 12,675,648 bytes、11 segments；双 party store 合计 385,331,840 bytes。它们是该功能运行的存储观察，不是性能结果；峰值 RSS 与生产存储开销 `NOT_MEASURED`。Schema 的 used/unused slot 自适应安全混合未通过本轮前置门，不能把成功执行次数外推为整池证明。

## 4. 完整 party view 的条件 sampler

### 4.1 定义

设 `G=R_c^N`。本报告中的受限 `V_pre,b` 专指不含刷新 `rho_b` 的 score-conversion 本地前缀：本方输入 share 与 padding、本方 carry/sign DCF party material 和 local mask shares、carry/sign local Eval 输出、阶段 4/5 masked operand frames、旧 priority-key share `Q_b` 及对应阶段消息。它不包括完整 M6B5/M6S2 party package、其他 Select/shuffle/membership key slots、package/manifest digest、one-shot lease/restart 历史、forward shuffle 本地 permutation/mask arrays、Select 后续 transcript、inverse/membership、最终 output share 和 crash 状态。

完整在线 party view 另含：T 为该 party 生成的离线材料与所有未用/已用 slots、刷新 share `rho_b`、绑定头/manifest/segments/lease 记录、本地输入和 DCF Eval shares、peer frames、当前逐轮 seed/endpoints/pivots/comparison bits、masked values、membership/inverse 状态、输出 XOR share、PREPARE/COMMIT/ABORT 及重启/崩溃本地残留。`rho_b` 在 T 发布 ready/package/slot 后、执行 party 开始协议前即为该 party 本地材料的一部分；它不是 score conversion 完成后才首次出现。

### 4.2 条件等式及其边界

对单独腐化方 `b∈{0,1}`，固定输入 share `x_b` 和已公开的前缀泄露 `L_prefix`。若理想模型中刷新向量在条件于 `(V_pre,b,L_prefix,x_b)` 后仍为均匀独立 `G`，且 `V_pre,b` 包含旧 `Q_b`，则 `Q′_b=Q_b+rho_b` 给出：

```text
Pr[V_pre=v, Q′_b=q′ | x_b,L_prefix]
  = Pr[V_pre=v | x_b,L_prefix] / |G|
```

理由是给定 `V_pre=v` 后唯一满足 `Q′=q′` 的向量为 `rho=q′−Q_b(v) mod 2^c`，其理想概率为 `|G|^-1=2^(-cN)`。该式只对不含 rho 的投影视图成立；它不证明 `V_pre | x_b,L_prefix` 可由允许模拟器生成。

完整元组的正确条件 law 在理想独立刷新假设下应为：

```text
Pr[X=x, rho_b=r, Q′_b=q′ | x_b,L]
 = Pr[X=x | x_b,L] · |G|^-1 · 1[q′=Q_b(x)+r mod 2^c]
```

其中 `X` 必须包含所有不含 rho 的本地前缀、party-local DCF/UCMP/shuffle/membership 材料、manifest/package/lease 状态及相应的已用/未用槽。给定理想 `q′` 的候选 sampler 是：先由 `SimBase_b(x_b,L)` 按真实条件 law 生成 `X`，再令 `rho_b=q′−Q_b(X)`，继而重新编码 package/manifest/lease 与后续消息。若 `SimBase_b` 不存在或未经证明，代数反推 rho 不能补出完整模拟器。

将 `rho` 错当成与完整 party view 独立有一个最小数学反例：令 `Q_b=0`、`W=rho_b` 且 `rho_b` 均匀，则 `Q′_b=rho_b`，所以 `Pr[Q′=q′|W=r]=1[q′=r]`，而非 `1/|G|`。这不是运行时攻击；它说明不含 rho 的边缘等式不能直接套用到包含 rho 的完整视图。正确完整 sampler 仍可在已证明的 base sampler 存在时通过 `rho=q′−Q_b(X)` 生成。

### 4.3 P0/P1 裁决与首个断点

对 P0，真实刷新材料是均匀 `rho_0`；对 P1，`rho_1=−rho_0` 在该单方边缘上也均匀。二者是两个单方模拟游戏；不能把两个单方 sampler 拼成彼此独立的双方材料，因为零和约束相关，且可信 T 与任一在线方合谋被排除。`rho_b` 的理想均匀性只处理刷新量本身。

S25/S24 没有给出 `SimBase_b(x_b,L)`。既有 BCG21 key-only 桥接（仍 `CONDITIONAL/UNQUANTIFIED`）不自动生成同一 score conversion 中相关的 carry/sign DCF keys、mask shares、local Eval state、masked frame、opened comparison bits 与旧 `Q_b` 的联合条件分布；更不生成整份 party package/lease view。故 P0、P1 在同一第一步骤均不能被完整模拟。这里没有发现可利用区分器；结论是 theorem/proof coverage 不足，不是协议被攻破。

实际 `getrandom` 为 rho、score masks 及 shuffle 随机带提供字节；BCG DCF roots 用 OpenSSL `RAND_bytes`。两个 API 的源码调用并不能推出其联合随机带与 party-visible 辅助信息统计独立。`epsilon_RNG_joint`、AES reduction 的 `T′/Adv`、TLS channel 项未量化，均保持 `CONDITIONAL/UNQUANTIFIED`，不报告安全位数。

## 5. 后续组合停止位置

因 P0/P1 完整 `F_scoreconv/v2` 输出条件 sampler 未闭合，按任务要求停止在 score-conversion 输出视图门：本轮不声称 N=128 `M6B5/v4`、N=256 `M6S2/v3` 的已用/未用 DCF pool 自适应访问、同 key 两次 Eval/公开比较位、membership、inverse shuffle、最终 mask shares、lease/restart、PREPARE/COMMIT、ABORT/crash 的安全组合通过。两个 schema 的静态槽计数已分开重算，但没有将 E2E 的活动比较调用计数当作池安全 hybrid 或概率界。

崩溃合同只支持指定测试前缀中的 `NO_COMPLETE_MASK`。步骤 1–4 双方输出字节均为 0；步骤 5–6 P0 为 0、P1 留下 8-byte 本地 XOR share。它不是原子无输出，不是 all-or-nothing；单方残留 share 必须进入未来 simulator 的 local view 和重启/重试状态。

下一道最小证明义务是让未参与者给出两个 schema 上可复核的 `SimBase_b(x_b,L)`，并精确定义理想 preprocessing 如何生成完整 party projection。该引理通过后，才进入两次相关 Eval 与第一条自适应 slot 的 hybrid；不要直接跳到整池或 shuffle/output 安全。

## 6. 本轮构建和回归

### 环境与命令

在 WSL Ubuntu 20.04 / Linux 6.6.87.2-microsoft-standard-WSL2，新建仓库外 build dirs：`/mnt/c/Users/28641/.codex/tmp/m6b_s26_on_20261019_01` 与 `_off_20261019_01`。GCC/G++ 9.4.0、OpenSSL 1.1.1f、bundled CMake/CTest 3.31.6、Unix Makefiles、Release、`BUILD_TESTING=ON`。target flags 为 `-Wno-write-strings -Wno-unused-result -maes -Wno-ignored-attributes -march=native -Wno-deprecated-declarations -O3 -DNDEBUG -std=gnu++17 -fopenmp`。

```sh
cmake -S VFSS -B /mnt/c/Users/28641/.codex/tmp/m6b_s26_on_20261019_01 -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake -S VFSS -B /mnt/c/Users/28641/.codex/tmp/m6b_s26_off_20261019_01 -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF
```

ON target build 成功（exit 0）：`sytorch`、`moe_topk_score_key_refresh_v2_test`、`moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test`、`moe_topk_project_derived_select5_conformance_test`、`moe_topk_project_derived_select5_material_store_test`、旧 M2 uCMP/score-input/parallel-shuffle、streamed store、三进程 E2E、TLS 1.3。OFF 构建 `sytorch` 和 3 个旧 M2 uCMP/score-input/parallel-shuffle 目标成功（exit 0）。完整 ON build 本轮未跑；S25 先前全量构建失败仍保留为历史证据，失败点为无关 `VFSS/ext/bitpack/tests/test.cpp` 对 `bitpack::mod(unsigned long,int)` 的 unresolved reference，S25 log SHA-256 `98bcb46ad4066c56dabc84cc59b427b0968c40b75b8b249ed0753b599e88cc46`。

定向 CTest 使用 CMake 3.31.6 的 bundled CTest：ON `7/7 PASS`，OFF `3/3 PASS`。另运行 standalone streamed-store（durable one-shot、crash、race、integrity PASS）和 TLS 1.3 mTLS conformance（peer identity/frame binding PASS）。外部 API probe ON 可编译，OFF 在未定义实验宏的正常 feature-OFF 使用方式下按预期编译拒绝；`nm` 检查 ON 静态库存在 v2 helper、OFF 不存在；CMake target help 中 ON 有 v2 test target、OFF 没有。

三进程 E2E 命令：

```sh
cd /mnt/c/Users/28641/.codex/tmp/m6b_s26_on_20261019_01
./moe_topk_project_derived_select5_process_e2e_test --score-share-v2-acceptance
```

exit 0，记录 9 个成功（N=1、2、7、128 与 N=256 streamed K=128）、6 个故障 abort/no-mask、6 个 crash-prefix。TEST_ONLY supervisor XOR 重构并逐位对照冻结 C++ stable oracle，检查原序、长度、二值和 weight=K。Acceptance 日志的 preflight JSON 同时记录 N=256 monolithic `REJECTED_BEFORE_GENERATION`、N=257 streamed `REJECTED`。N=256 此运行成功并不外推到所有 K 或所有输入。

### provenance 与保留的失败尝试

- Fresh ON `libsytorch.a` SHA-256 `fd6130d17019b5cf4c23548e43705d189e7786cacda35f8dc58e3930bc250479`；OFF library、CMakeCache、每个测试二进制及外部日志 SHA-256 见 manifest。
- S25 与 S26 同为 CMake 3.31、GCC 9.4 Release，target flags 一致；新旧 binary hash 不同，`libsytorch.a` 中可见各自的绝对 source-root 字符串。未证明所有 byte 差异都只由路径引起，故把 S26 构建视为独立 build identity，不宣称与 S25 binary 可互换。
- 初次 build 命令因 PowerShell 展开 Bash 变量而以 exit 127 失败；已保留 `build_*_targets.log`，改为直接 CMake 路径后 ON/OFF target builds 均通过。
- 首次系统 CTest 3.16 不支持所用 `--test-dir` 语义，实际在源码目录显示 `No tests were found`；该空跑不计入通过。用 bundled CTest 3.31.6 对真实 ON/OFF build dir 重跑后 7/7、3/3 通过。
- 首次 OFF API probe 错误地手工定义实验宏，导致本应拒绝的 probe 编译成功；保留日志，移除 OFF 宏后重跑，ON compile pass、OFF expected reject、符号/target 检查 pass。
- 没有运行 S25 16-success 全矩阵、M6A LAN/WAN、所有 K/N 配置、全输入所有随机 tape 证明、攻击者自适应模拟、生产 peak RSS/跨主机存储；这些均 `NOT_RUN/NOT_MEASURED`。S26 仅复跑任务指定范围。

## 7. 当前可声称与不能声称

可声称：S25 74 文件候选被精确复建；S26 的同一执行者在新的隔离 Release 构建中完成上述 v2 定向功能回归；零和 refresh 在目标环保持完整稳定键重构；有限的 N≤256 E2E 样例匹配冻结 oracle；测试中的 crash-prefix 不发布完整 mask。

不能声称：本轮异会话独立接收、非作者密码学签核、完整 P0/P1 输出条件模拟、DCF 相关整池/自适应访问安全、shuffle/inverse/membership/output/crash 联合安全、BB90/BMW16 实现、任意输入密码学安全、atomic no-output、`secure`/生产就绪或性能就绪。路线维持 EXPERIMENTAL；V3 OPEN，V4 未授权，正式矩阵不允许运行。

## 8. S26 文档交付与文件变更清单

本任务实际新增或更新的仓库文件如下。S25 的 74 文件候选（含运行时代码）是本轮接收基线，并非 S26 代码改动；没有修改 `VFSS/` 运行时。S25 manifest 原件从原 S25 worktree 逐字节复制，仅为使接收对象可复核；没有改写其内容。

| 文件 | 操作 | 用途 |
|---|---|---|
| `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json` | 原件复制 | 固定 S25 74 文件路径、哈希、构建及历史证据指针。|
| `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S26_DECISION_2026-10-19.md` | 新增 | 记录 S26 身份、裁决、范围和下一步。|
| `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S26_REPORT_2026-10-19.md` | 新增 | 本详细复核报告。|
| `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S26_GATES_2026-10-19.json` | 新增 | 独立接收、v2、完整 sampler、材料池及性能门禁。|
| `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S26_E2E_CASES_2026-10-19.json` | 新增/更正摘要 | 记录 9 成功、6 fault、6 crash-prefix；摘要成功数与逐例结果一致。|
| `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S26_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json` | 新增 | 锁定 S25/S26 源码身份、文档后置 overlay、fresh build、二进制与外部日志哈希。|
| `PROJECT.md` | 追加 S26 状态 | 不改写 S18–S25 历史状态。|
| `docs/IMPLEMENTATION_PLAN.md` | 追加 S26 状态 | 记录本阶段功能和安全门结果。|
| `docs/REFERENCE_MANIFEST.md` | 追加 S26 来源记录 | 记录 S25 快照与 S26 条件 sampler 边界。|

S25 清单的 canonical 身份是在 S26 文档 overlay 前确认的。S26 manifest 将三个计划/参考文档的 S25 哈希与 S26 更新后哈希分栏保存；此后 S25 的 74 文件源身份不因状态更新而重定义。所有构建目录、二进制、原始日志和失败尝试都保存在仓库外的 `C:\Users\28641\.codex\tmp\` 或 `C:\Users\28641\.codex\evidence\M6B_I_S26_20261019_01\`。
