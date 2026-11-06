# M6B-I-PROJECT-SELECT-5ITER-S31 工作报告

日期：2026-10-20
路线：`PROJECT_DERIVED_SELECT_5ITER`（Protocol I 项目派生五迭代稳定 Select；不是 BB90/BMW16 原算法复现）
范围：冻结 S25 成功输出 share 观察、明确 S25 崩溃探针边界；提出可审查的 score-to-mask-v3 GMW/Beaver 规格；建立隔离的 TEST_ONLY 可执行功能模型并做小规模正确性验证。
准入结论：负责人已为新 v3 选择 `F_TOPK-ATOMIC`，该选择只定义 v3 目标，不追溯改变 S25。TEST_ONLY 正确性通过且独立技术规格审查通过，因此准入下一阶段默认关闭、显式版本化的 opt-in runtime 开发；S31 本身未实现 runtime。密码学安全仍 `OPEN/NO_GO`，没有安全 PASS。

## 1. 裁决摘要

| 项目 | S31 状态 | 结论 |
|---|---|---|
| 被审对象 | `PASS` | 使用 S25 固定快照，canonical SHA-256 为 `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`；74/74 manifest 文件哈希匹配。 |
| S25 功能接收 | `PASS_WITH_SCOPE`（历史收据） | 仅继承 S27 对同一 S25 manifest 的限定功能接收；本轮没有重跑 S25 矩阵。 |
| 成功 share law | `F_TOPK-U` 的实现诱导分布 `CONDITIONAL` | 代码输出是逐位 0/1 的本地 share；在理想独立随机带下，每方 share 均匀，XOR 重构为本地实际 `M_runtime`。这来自逆 shuffle 的随机 pad 与 LSB 运算推导，不是为了模拟方便的假设。真实 OS/DRBG 联合随机性优势未量化；`M_runtime` 对所有合法输入及所有有效随机材料等于稳定 Top-K 的全域证明未由 S27 有限收据替代。 |
| S25 历史中止语义 | `UNRESOLVED_BY_EVIDENCE` | S25 本地 share 在最终状态交换前形成；步骤 5–6 探针只读到 P1 的 8 字节长度头，未读 payload。`NO_COMPLETE_MASK` 不能推出原子无输出或 all-or-nothing。 |
| v3 中止/崩溃合同 | `OWNER_SELECTED_F_TOPK_ATOMIC` | 负责人选择双方逻辑输出经耐久 escrow 同一原子提交；提交前双方为 `⊥`，提交后两方 share 同时可恢复。该新边界不改变 S25 历史合同，增加 escrow 可用性信任及 `L_A` 元数据观察。 |
| GMW/Beaver TEST_ONLY | `PASS_TEST_ONLY` | 独立单文件模型编译并通过：比较、五迭代控制、完整小规模路径、membership、inverse routing、输出 share law、材料拒绝与一次性 triples。它不是双进程/网络实现，也不是密码学证明。 |
| P0/P1/A 新版本安全模拟 | `OPEN/NO_GO` | 未构造经证明的 party-view sampler；Beaver/GMW 通用定理尚未实例化到具体电路、T 材料、公开比较结果、A escrow 视图和 crash/recovery 前缀。 |
| opt-in runtime 下一阶段准入 | `ADMITTED_NEXT_PHASE; IMPLEMENTATION_NOT_STARTED` | 独立技术规格复核和 TEST_ONLY 正确性均通过，满足进入默认关闭、显式版本化实验 runtime 阶段的门槛。本轮不实现 runtime；该准入不是安全 PASS，P0/P1/A 模拟证明、escrow crash/recovery 论证与优势量化仍开放。 |
| 正式性能 | `NOT_MEASURED` | 未运行 LAN/WAN 正式矩阵；本轮只记录测试程序诊断耗时。 |

## 2. 被审对象、证据分类和范围

被审快照为 `C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation`。S25 manifest SHA-256 为 `c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb`，manifest 的 74 个文件逐个复算全部匹配。canonical SHA-256 为用户指定值。S25 基础 HEAD 是 `c3926c68fd14f270faa8b55234311071947fa080`；候选本身包含未提交变化，因此审查对象是 manifest 固定文件集合，不是只看 HEAD。

本轮用于重新核验 S25 身份的 PowerShell 命令（PowerShell 7.6.5；按 manifest 的 `s25_sha256` 逐项比对，并以 ordinal path 顺序重算 canonical hash）：

```powershell
$s25='C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation'
$mp=Join-Path $s25 'docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json'
$m=Get-Content -Raw -LiteralPath $mp | ConvertFrom-Json
$lines=[System.Collections.Generic.List[string]]::new(); $mismatches=0
foreach($f in $m.files){
  $actual=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $s25 $f.path)).Hash.ToLowerInvariant()
  if($actual -ne $f.s25_sha256){$mismatches++}
  $lines.Add("$($f.path)`t$actual")
}
$arr=$lines.ToArray(); [Array]::Sort($arr,[System.StringComparer]::Ordinal)
$bytes=[System.Text.UTF8Encoding]::new($false).GetBytes(([string]::Join("`n",$arr)+"`n"))
$digest=[Convert]::ToHexString([System.Security.Cryptography.SHA256]::Create().ComputeHash($bytes)).ToLowerInvariant()
"FILE_COUNT=$($m.files.Count) MISMATCHES=$mismatches CANONICAL_SHA256=$digest"
```

结果：`FILE_COUNT=74 MISMATCHES=0 CANONICAL_SHA256=64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。

S27 的 `PASS_WITH_SCOPE` 是针对同一快照的历史功能接收。本轮没有重跑 S25/S27 的构建、CTest、原语、材料池或三进程矩阵。S31 TEST_ONLY 程序是新增隔离文件，不能追溯改变 S25 身份或 S27 收据。

| 证据类别 | S31 用法与边界 |
|---|---|
| 论文定义 | 没有把论文中的 BB90、BMW16 或 GMW 文献步骤/定理当作五迭代 Select 的证明。没有论文定义本项目的 v3 全流程。 |
| 本地运行时行为 | 只引用 S25 快照中的 Protocol I、score-input、Select5、shuffle、材料和 E2E crash probe 的具体代码。 |
| 项目派生推导 | 稳定优先键、五迭代控制图、Top-K mask、GMW 电路、Beaver gate 预算和消息 DAG 均属于本项目扩展。 |
| 待验证设想 | v3 的半诚实安全、T preprocessing 联合模拟、随机源优势、精确泄露映射、全域功能正确性、崩溃合同选择及资源可行性均未由本轮功能测试证明。 |

## 3. S25 成功输出合同

### 3.1 从运行时代码得到的实际 share law

仓库统一目标是原输入顺序下长度为 `n`、每位为 0/1、权重为 `K` 的秘密共享 mask。S25 的输出代码没有把完整 mask 返回任一方：score/key 和 membership 经过逆 shuffle 后，每方 API 返回 `inverse.result.shuffled_share[i].word1 & 1`。

S25 的逆 shuffle Dealer 先独立抽样随机记录向量 `h`，并把 `−h`、`+h` 分别放进 P0/P1 的 `e` 材料；每方的 `shuffled_share` 为路由后的本地输入 share 加本地 `e`，最终 API 返回该 `shuffled_share.word1` 的最低位。对 `word1` 的某个输出位置，精确字宽是 `uint64_t`，故可写为 `w0=v0−h (mod 2^64)`、`w1=v1+h (mod 2^64)`；仅 `word0` 按 `comparison_bits` 掩码。由于只取最低位，`LSB(w0) XOR LSB(w1)=LSB(v0+v1)=M_runtime_i`。`r_share` 另用于第二轮 public-masked lane；它不进入返回的 `shuffled_share`，因此不作为本 share-law 推导依据。若将 `getrandom()` 输出建模为相互独立的理想均匀随机字，则每个位置的本地 share 含一个新鲜、均匀的 `h.word1` 最低位；对固定输入及实际重构 mask `M_runtime`，得到：

```text
z0_i <- {0,1} uniformly
z1_i = z0_i XOR M_runtime_i
P0 returns z0; P1 returns z1
```

因此每方 share 的边缘分布均匀，`z0 XOR z1 = M_runtime`。这一推导依赖 `h` 与输入无关、其 `word1` 低位在理想随机带中均匀；生产 `getrandom()` 的整体伪随机/故障优势项未量化。`e0/e1` 的 `±h` 生成在 S25 `protocol_i_parallel_shuffle.cpp:352-360`，本地 `shuffled_share` 的形成与返回在 `:518-531`，mask 输出位见 `protocol_i_project_derived_select5.cpp:2249-2254`；S25 E2E controller 的长度前缀后写 share payload 见 `...three_process_e2e_test.cpp:389-391`。

**成功理想功能目标记为 `F_TOPK-U`，但仅在以下明确范围内冻结：**实现诱导的 share-law 形式是均匀 XOR shares；目标重构值是稳定 Top-K mask。S27 是同一快照的有限功能接收，不等价于“对每个合法输入和每组有效 preprocessing/随机 tape，`M_runtime=M_topk`”的全域证明，也不量化真实 RNG 优势。因此 S31 不把全域功能正确性或生产密码学安全标为 PASS。`F_TOPK-C`（向一方交付完整 mask）不符合秘密共享输出目标，不是候选。

稳定顺序在 S25 项目 priority key 中实现：`raw ^ 0x80000000` 将 signed Q20.12 映射为 unsigned 顺序，再对 score 高位取反并将 original index 放低位；升序 key 等价于 signed score 降序、原始 index 升序。源码见 S25 `protocol_i_priority_key.h:20-29`、`score_semantics.h:7-23`、`topk_oracle.h:23-49`。这属于项目输入/排序语义，不能写成论文结论。

### 3.2 P0/P1 理想输入输出与 simulator API

对 `b∈{0,1}`，成功功能为 `F_TOPK-U`，并由新 v3 原子 escrow 外层 `F_TOPK-ATOMIC` 发布：

```text
F_TOPK-U(x0, x1, public_params=(n,K,Q20.12,route,session,version), L)
  x = (x0 + x1) mod 2^32          // 每项算术共享
  M = stable_topk_mask(x,K)       // score 降序，原 index 升序，原序 mask
  z0 <- uniform {0,1}^n
  z1 = z0 XOR M
  stage z0 for P0 and z1 for P1 in durable escrow A
  if both authenticated deposits commit atomically:
      make z0 and z1 durably retrievable to their respective recipients
      output z_b to each P_b in the same ideal transition
  else before commit: output ⊥ to both
  disclose L_S29 to P0/P1; disclose only L_A metadata to escrow A
```

`n>0`、`1≤K≤n`、32-bit 二补码 Q20.12、输入长度及 session/version 是公开绑定。新 v3 的非法参数、包错配、资源预检失败和提交前 crash 均在原子提交点之前统一输出 `(⊥,⊥)`；这里的 `⊥` 指没有 API 可见输出，不表示本方 view 中尚未形成暂存 share。已消费的材料不可重领。原子提交后双方输出 share 被耐久分配，任一进程 crash 不回滚；接收方可在恢复后从 A 重取本方加密记录。线性化事件是 A 的持久原子 commit，而非两个操作系统 syscall 在同一时刻完成。若 A 在 commit 前不能完成，则超时/故障为双方 `⊥`；commit 后的 A liveness/durability 是新增信任假设。

该合同新增服务 A 的观察范围 `L_A`：协议/version、session/invocation、固定长度的收件人加密 share、双方 deposit 是否到齐、原子 commit/abort、请求顺序与时序元数据。P0/P1 仍仅按既有 `L_S29` 建模，A 不解密 payload；全系统参与方集合及元数据接收者因此扩大，不能称为完全不变的全局泄露/腐化边界。

每方的模拟器候选接口为：

```text
Sim_b(input_share_b, prescribed_output_b ∈ {z_b, ⊥}, public_params, L_S29)
  -> complete local view_b, including provisional share, A deposit/receipt and crash prefix
Sim_A(public_params, L_A)
  -> A's ciphertext/metadata/commit view (no plaintext share)
```

其中 party simulator 只能拿本方 `x_b`、功能输出（成功时本方 `z_b`、提交前失败时 `⊥`）、公开参数和 `L_S29`；不能额外拿 peer 输入、完整 mask `M`、内部 `Q/Q′`、rho、carry/sign、DCF key、Dealer tape、另一方 package、真实 transcript 或未公开选中坐标。提交前即使 `z_b^F=⊥`，真实 party view 仍可能含暂存的本地 wire share、密文及 pending deposit；证明须在不拿 `z_b` 的条件下采样这些内部值。`Sim_A` 独立使用新增 `L_A`，不得获得明文 `z0/z1`。任何 party-local view 字段只能由 sampler 自己生成。

P0/P1 的 `L_S29` 字段边界保持：公开参数/绑定；已批准的 shuffle 端点或位置；pivot 与公开 comparison result bit；forward shuffle 完成后每个活动迭代才公开的 sample seed；既有定义允许的 masked values；comparison count；communication length；uniform ABORT。Beaver `d/e` 及其本地 shares 属于真实 party view，v3 必须由 simulator 联合生成；是否完全由原 `L_S29` 模拟取决于新 proof，不把它们作为新增 party 输出字段。`Q′`、rho、carry/sign shares、Dealer tape、DCF keys 和完整 mask 均不加入 `L_S29`。A 的新增 `L_A` 不回流给 P0/P1，也不抹除它作为系统元数据观察者的事实。

### 3.3 S25 中止/崩溃前缀事实与合同选择

S25 代码先构造每方本地 `output_share`，随后分别执行 durable `.prepared`、PREPARE 交换、durable `.commit`、COMMIT 交换、durable `.published`，最后才从 API 返回。API 返回后测试子进程先写 8 字节长度，再写 share payload。具体代码在 S25 `protocol_i_project_derived_select5.cpp:2249-2277` 和测试 `protocol_i_project_derived_select5_three_process_e2e_test.cpp:389-391,762-793`。

| Crash 探针前缀 | 本地代码/探针观察 | 可作出的结论 |
|---|---|---|
| step 1：材料 claim 后 | 可已 claim 一次性材料；输出尚未产生 | 没有该探针下的输出证据；材料状态需进入理想 abort view。 |
| step 2：本地 output share 形成、prepared marker 后 | P0 崩溃；P0 stack/local state 中有本地 share。P1 也在执行对应本地路径 | 本地孤立 share 与 API 返回分开；不能称为原子无输出。 |
| step 3：收到 peer PREPARE 后 | P0 已见 PREPARE，尚未通过后续 COMMIT/PUBLISH；P1 可能处在相邻阶段 | 有协议阶段观测和本地 share，无完整 mask 结论以外的原子保证。 |
| step 4：本地 commit marker 后、peer COMMIT 前 | P0 可能持有本地 share，peer 可能继续或中止 | 是否某方 API 返回由真实进程时序决定；不从 marker 推出双方一致结果。 |
| step 5：收到 peer COMMIT 后、P0 写 `.published` 前 | P0 注入崩溃；P1 可以完成 `.published` 和 API return。测试子进程正常路径会先写长度头再写 payload，但 crash probe 只读取 P1 管道的 8 字节长度头，声明长度 `n=7`，没有读取/核验 payload；P0 管道 0 字节。 |
| step 6：P0 写 `.published` 后、P0 API return 前 | P0 已有 durable published marker，但注入崩溃阻止 P0 API return；P1 可已返回。探针同样只读到 P1 的 8 字节长度头（声明 `n=7`），没有读 payload；P0 管道 0 字节。 |
| 所有受测 crash rows | 原记录结论为 `NO_COMPLETE_MASK` | 只代表探针没有观察到双方完整 payload；不表示“8 字节 share 已发布”，不表示原子无输出，也不表示 all-or-nothing。 |

v3 选定合同下的逻辑失败前缀如下（这是新规格，不是 S25 实测）：

| v3 阶段 | 可能的 party-local 状态 | `F_TOPK-ATOMIC` 输出/恢复合同 |
|---|---|---|
| READY/输入/电路/逆路由前后、A 尚未收到记录 | 任一方可能已有本地输入 share、GMW wire shares、最终暂存 `z_b`；材料状态按 one-shot lease 记录 | A commit 前 API 均无输出；若失败则双方理想输出 `⊥`，已消费材料 burn。内部暂存 share 必须进入该方 view，而不是删除或称“无输出”。 |
| 只有一个 deposit 到 A | A 有一份收件人密文；双方可能仍持有本地暂存 share | 不发布任一份；超时/取消为统一 pre-commit ABORT，双方为 `⊥`；A 清理 pending ciphertext，party view 保留 deposit/状态帧。 |
| 两份 deposit 到齐、A durable commit 前 | A 持有两份不可解密的 pending 密文 | 仍不发布任一份；crash/事务失败都走双方 `⊥`。A 必须恢复一致的 pending/abort 状态，不能单方返回。 |
| A durable atomic commit | 两个 recipient ciphertext 已耐久绑定到同一 transaction id | 同一线性化转移将两份逻辑输出 share 分配给 P0/P1；A 向两方回执。正常 API 在回执后各返回本地已持有的 share；回执传递延迟不改变理想提交状态。 |
| commit 后、任一 API 回执/return 前 crash | 可能只有一方物理上收到回执；另一份密文仍 durable | 不回滚为 `⊥`；崩溃方恢复后从 A 取回本方 share。此处单方物理收件进度不等于单方逻辑输出，边界依赖 A 的耐久线性化信任。 |
| 正常返回或 crash 后恢复 | 每方最多获得本方 share | P0/P1 从不单独得到完整 mask；重放/重取受 session 与一次性事务/访问策略约束。 |

S31 不把旧 `no-complete-mask` 扩写为 P0/P1 原子发布语义。负责人已对**新 v3**选择以下合同；该选择不追溯改写 S25 的实际失败语义：

1. `F_TOPK-PREFIX` 不选用：它允许一方已返回而另一方为 `⊥`，弱于负责人选定的合同。
2. **`F_TOPK-ATOMIC`（v3 选定）**：A 的耐久 escrow 对两份收件人加密 share 执行一个线性化原子 commit。commit 前双方逻辑输出均为 `⊥`；commit 转换中两份输出同时成为各自可恢复的 share；commit 后进程 crash 不回滚。单个客户端稍后完成物理 fetch 是恢复/交付过程，不是理想输出事件。需要新增 A 的原子性与持久可用性信任边界，不能只靠本地 durable marker。

状态：v3 合同 `OWNER_SELECTED`；S25 历史合同 `UNRESOLVED_BY_EVIDENCE`。没有把更弱合同选为原协议合同。

## 4. score-to-mask-v3 GMW/Beaver 规格（设计，不是安全结论）

### 4.1 输入电路、排序键和 Select5 控制

令 `n` 为逻辑长度，`N=2^ceil(log2(max(n,2)))`，当前设计上限 `N≤256`。P0/P1 持有各自 32-bit additive share，重构模 `2^32`。输入转换中双方各自独立抽样本方 `32N` 位 mask `r_b`，并同时将 `u_b=x_b XOR r_b` 发给对方；本方持有 `r_b`，收方持有 `u_b`，从而每方的私有 additive share 成为 Boolean XOR sharing。随后用 `63N` 个 Beaver AND 计算 `x0+x1 mod 2^32`，不可把原始 additive shares 直接 XOR 当作和。padding 槽 P0 输入 `0x80000000`、P1 输入 `0`，即 signed `INT32_MIN`；original index 仍为追加的高位索引，且 `K≤n`，所以 padding 不可进入前 K。

电路给每个槽构造 `W=32+log2(N)` 位唯一 priority key：

```text
ordered = raw XOR 0x80000000
score_hi = bitwise_not(ordered)
key = score_hi || little_endian(original_index)
```

其无符号升序对应 signed Q20.12 降序和 original index 升序。算法先在稳定顺序 oracle 的语义下选择第 K 个 key，再形成 mask。

Select 部分沿用 S25 `project_derived_select5.cpp:341-506` 的项目控制图：最多四个采样 bracket 迭代，`s=min(m,ceil_sqrt(m))`；窗口 `ceil_sqrt(2*s*bit_length(N))`；据剩余 rank 计算 `q_low/q_high`，先对 sample clique 比较并排序，再对两个 pivot 与非 sample 候选做 star 比较，公开获准 bits 后更新 pivot rank、candidate 和 residual rank。第五个算法迭代对剩余 candidate 做 exact clique；若提前 singleton/选中，仍补齐 5 个 trace record。算法迭代数固定为五；每个迭代含比较依赖的子层，活动比较波数最多 9。它们均不等于 online RTT 数。

### 4.2 Boolean Beaver gate 规格和精确预算

全部非线性门是 `GF(2)` 上 AND，XOR/NOT/free public constant 不耗 triple。T 为每个 gate id `g` 生成独立 `a_g,b_g←{0,1}`、`c_g=a_g∧b_g`，再按 XOR additive sharing 将 `a,b,c` 分给 P0/P1。在线双方对每个 AND 交换两位局部 masked differences：`d_i=x_i XOR a_i`、`e_i=y_i XOR b_i`；重构公开 `d,e` 后用标准 Beaver 公式得到 XOR share：

```text
z0 = c0 XOR (d AND b0) XOR (e AND a0) XOR (d AND e)
z1 = c1 XOR (d AND b1) XOR (e AND a1)
```

每 triple 固定绑定 session、party、协议版本、完整 circuit/gate schedule hash 与唯一 gate id；成功或失败后均不可重领、跨 session/version 重放，进程重启不复用。所有 gate 的三份 Boolean share 必须恰好消费一次。

| 电路段 | 每 gate 数 | AND gates |
|---|---:|---:|
| 将两方 32-bit additive input shares 相加，N 槽 | 32 generate + 31 carry AND | `63N` |
| 依 Dealer XOR-shared one-hot permutation 矩阵前向路由 key | 每输出 bit 与每输入位置一门 | `N²W` |
| 对 N 个 key 的全部无向 pair 预计算严格比较 share | 每对 `W` generate + `W−1` borrow AND | `C(N,2)(2W−1)` |
| 把 shuffled membership 逆路由回原序 | 每输出与每 shuffled 位置一门 | `N²` |
| **总计 G** |  | **`63N + N²W + C(N,2)(2W−1) + N²`** |

比较 borrow recurrence 对 little-endian key 的逐位严格比较：`g_i=(¬x_i)∧y_i`、`eq_i=¬(x_i XOR y_i)`、`b_0=g_0`、`b_i=g_i XOR (eq_i∧b_(i−1))`；两个乘积项互斥，最终 `b_(W−1)=[x<y]`。活动比较结果只在 S25 对应 sample/pivot/final graph edge 被需要时公开；其他 pair comparator wire 保持秘密并仅用于后续 membership。不得公开整个排序或所有比较表。

| N | W | score add | forward route | all-pairs comparator | inverse route | **G = triple 数** | 每方 packed triples `ceil(3G/8)` | 每方 permutation shares `ceil(N²/8)` | 每方 packed package payload |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 | 39 | 8,064 | 638,976 | 625,856 | 16,384 | **1,289,280** | 483,480 B | 2,048 B | **485,528 B** |
| 256 | 40 | 16,128 | 2,621,440 | 2,578,560 | 65,536 | **5,281,664** | 1,980,624 B | 8,192 B | **1,988,816 B** |

Byte预算是假设 triples 按 `a,b,c` 位打包、permutation 每个 matrix bit 每方一位；不含 manifest、MAC/signature、TLS、对齐和容器头，属于理论数，不是实际包测量。GMW 每一 gate layer 的每方差分 payload 为 `ceil(2*g_layer/8)` bytes；活动比较公开 payload 为每方 `ceil(E_layer/8)`；在线消息字节尚未测量。

建议的 v3 资源准入硬条件：`N≤256`、`G≤5,281,664`；v3 package 固定头/manifest/signature 最多 64 KiB，含该元数据的每方 package 总长不得超过 2 MiB；单个在线 gate-batch frame 不得超过 1 MiB；电路构建器以 checked arithmetic 计算所有 wire/triple/frame 的最大实际内存。每方在消费 one-shot lease 前本地完成预检；READY 握手只发送固定形状的既有 `READY` 或统一 `ABORT` 状态，不传本地内存数值、管理员阈值或失败原因。任一方预检失败即走已批准 `L_S29` 的统一 ABORT，避免另一方无限等待。任何条件不满足即 uniform pre-online ABORT，不自动缩小 N、不换 v2/DCF schema、不复用材料。N=256 的已知 packed payload 是 1,988,816 B，元数据上限 65,536 B，总和 2,054,352 B，小于 2,097,152 B；这是设计上限，不是实测资源接收。

### 4.3 T 预处理、域分离和 package/lease 合同

可信离线 Dealer `T` 在不知道输入时用 CSPRNG 生成 triples、均匀随机 permutation `π`、其 N×N one-hot matrix 的每方 XOR shares；T 可知 `π` 和 triples，但 T 不在 P0/P1 半诚实腐化模型中、不得与其中一方合谋、不得收到在线输入或 transcript。若项目不接受这一 T 信任假设，需另行设计分布式 preprocessing，不能沿用本规格的安全结论。所有材料生成完毕、签发两份固定包并发布 ready 后，T 退出，不在线补发、轮询或跨运行复用。

建议 CSPRNG label 分域：`s2m-v3/triple/a`、`/triple/b`、`/triple/a-share`、`/triple/b-share`、`/triple/c-share`、`/permutation/fisher-yates`、`/permutation/xor-share`、`/manifest`、party-local `/input-share` 与 `/output-pad`。每次派生绑定 protocol id、circuit hash、session、invocation、party、gate/slot id、purpose；sample seed 使用既有 session/fingerprint/iteration 派生合同。随机 rejection sampling 必须无偏。域分离仅给出 PRF/CSPRNG 混合证明，不自动产生信息论独立；优势项必须由实际 DRBG/query bound 量化。

`score-to-mask-v3` package schema 绑定 `version=3`、协议名、circuit hash、logical/padded N、K、Q20.12、party id、session/invocation、L schema version、gate schedule hash、triple count、payload length、bit order、SHA-256 digest、T signature/认证信道身份。两方 READY 交换各自 manifest digest；任一不匹配即拒绝并进入已选 abort prefix。每个 party 使用原子/耐久 one-shot lease 在开始首个门前消费，记录 `ready→claimed→spent/aborted`；任何失败均 burn 本 session 包。v2 DCF package 的 magic/version/digest、stream-slot 地址和 rho 字段不能被 v3 parser 接受；无降级路径。

建议的显式版本化 API 类型（仅规格名，本轮未在 runtime 中声明）：

```text
ProtocolIScoreToMaskV3Config {
  session, invocation, fingerprint, logical_n, padded_n, K,
  score_abi=Q20.12_i32, route=PROJECT_DERIVED_SELECT_5ITER,
  output_contract=F_TOPK_ATOMIC_V1, leakage_schema=L_S29_V1,
  escrow_schema=A_ESCROW_V1, circuit_hash, timeout, resource_caps
}
ProtocolIScoreToMaskV3DealerOutput { package_p0_v3, package_p1_v3 }
ProtocolIScoreToMaskV3PartyInput { party, additive_score_share[n], package_v3, endpoints }
ProtocolIScoreToMaskV3PartyOutput { local_mask_share[n], commit_receipt }
// returned/recoverable only after A's atomic commit; no complete mask, no Q/Q′
ProtocolIScoreToMaskV3EscrowRecord { recipient, sealed_share[n], session, invocation }

protocol_i_score_to_mask_v3_dealer_generate(config) -> DealerOutput
protocol_i_score_to_mask_v3_party(config, PartyInput, escrow_A)
  -> PartyOutput after durable atomic commit, recoverable read after commit,
     or uniform pre-commit ABORT (⊥ to both)
```

API 必须走单独 opt-in feature/version tag；正常 v2 函数签名和默认构建保持不变。v3 parser 只接受 v3 magic 与 circuit hash，v2/v4、错版本或缺字段均拒绝；不得因 v3 包缺失静默运行 v2。v3 配置还必须显式绑定 A 的 escrow public identity、durability/linearizability contract version、recipient encryption key ids 与恢复策略；没有该 A 或其合同不匹配时拒绝启动。

TEST_ONLY 原型的 FNV-1a checksum 只检出测试字节翻转，不具备密码学完整性；它未实现建议的 SHA-256/T signature/网络认证。

### 4.4 在线消息 DAG、declassification 和轮数推导

消息均使用带 session、party、phase、circuit hash 和固定 public shape 的有序认证 frame；不把错误帧细节映射为秘密相关分支。每个 GMW AND depth 批次中，两方并行发送该层 `d/e` shares；各自重构 masked values 并更新 wires。输入 share、READY、sample seed、活动比较 output、inverse routing 和最终发布严格依赖下图：

```text
T packages + READY/digest check; each side preflights locally and sends only READY or uniform ABORT
  -> both parties exchange private-input XOR masks
  -> 32 dependent AND layers: 32-bit additive-share addition
  -> 1 AND layer: forward one-hot routing
  -> W dependent AND layers: all-pairs key comparator table (still secret)
  -> up to 4 (seed/ack RTT + active comparison-open RTT(s)) adaptive Select rounds
       each of iterations 0..3: disclose 32-byte seed only after forward shuffle,
       open only exact sample-clique outcomes, derive pivots, then open pivot-star outcomes
       exact fifth layer: open only remaining-candidate clique outcomes
  -> local membership wires M_i=[key_i <= kth-key]
  -> 1 AND layer: inverse one-hot routing to original input order
  -> P0 fresh output pad r; P0 sends fixed-N u=r XOR m0; P1 computes z1=u XOR m1
  -> P0 and P1 seal z0=r and z1 to their recipient keys and deposit fixed-shape records at A
  -> A performs one durable atomic transaction: commit both records and return the same commit receipt to both, or commit neither and return uniform ABORT
  -> after receipt each API returns its already-held local z_b; pre-commit failure is (⊥,⊥)
  -> after a post-commit crash, the recipient may recover its own sealed record from A
```

此处 `m0 XOR m1` 是 GMW mask output wire。P0 持有 `z0=r`；P1 持有 `z1=u XOR m1=r XOR M`，于是两份 share XOR 为 M，且任一方单独只见均匀 share。P0→P1 的 refresh frame 是固定 `N` bits，不向 P1 直接发送完整 M。双方将各自 share 用收件人公钥封装后交 A；A 看见固定长度密文、身份/会话元数据及到达顺序/时序，不看明文 share。A 的线性化 durable commit 同时使两份记录对各自收件人可恢复，并向双方返回事务回执；正常 API 在收到回执后返回各自已持有的本地 share。A fetch 只用于进程丢失本地状态后的恢复，不计入正常 DAG 上界。commit 后不能回滚或重新变成理想 `⊥`。commit 前本地可能已有暂存 `z_b`，但不作为 API 输出。这不是对旧 S25 PREPARE/COMMIT/PUBLISH 的证明。

消息长度边界：READY/ABORT 固定形状且不携带本地资源详情；32N-bit 输入 share、每一固定 Beaver gate layer、N-bit output refresh、两份固定长度收件人密文和 A commit receipt 均只依赖公开 N/circuit version；Beaver layer payload 为每方 `ceil(2*g_layer/8)` bytes。活动比较公开 frame 为每方 `ceil(E_layer/8)` bytes，`E_layer` 和 edge endpoints 由当轮 L 已公开的 seed/此前 comparison bits 决定；每个活动 seed 帧 payload 固定 32 bytes，原 graph control ack payload 为 9 bytes。活动波数/边数可能依赖前序已公开结果，但不得再由私有分数以 L 未记录的方式影响 frame count/shape。A 收到密文后看到双方到达顺序/时间，并计入新增的 `L_A`。所有 frame metadata/TLS overhead 是固定 per frame，本轮不估算实际字节。

GMW AND-depth 为 `32 + 1 + W + 1 = W+34` 层；上界 9 个 active comparison-open 波、4 个 active sample seed/ack RTT、输入 share exchange 1 RTT、READY 1 RTT（仅交换固定形状 READY 或统一 ABORT，不携带本地资源详情）、output refresh 1 RTT、A escrow atomic commit/receipt 1 个服务轮。没有额外 PREPARE 消息。escrow 服务轮是第三方服务阶段，不能被误标成 party-to-party RTT；下式是当前 DAG 的因果通信阶段/RTT-equivalent 上界：

```text
R_max = 1 READY(preflight) + 1 input-share + (W+34) Beaver layers
        + 9 comparison-open + 4 sample-seed/ack
        + 1 output-refresh + 1 A-escrow atomic-commit/receipt
      = W + 51
N=128 (W=39): 90 RTT upper bound
N=256 (W=40): 91 RTT upper bound
```

这是规格 DAG 推导的上界，不是运行时观测或性能数据。正常 API 在收到 A commit receipt 后返回本地暂存 share，因此只计一次 A commit/receipt 服务轮；A fetch 是进程丢失本地状态后的恢复路径，不属于正常完成，不计入该上界。若未来实现改为每次正常返回都从 A fetch，必须将其依赖轮单独加入 DAG。样本/控制层可少于上界，且抽象 RTT 口径不含 TCP/TLS 握手、传输重试和系统调度。五个算法迭代、比较依赖层 `W`、消息轮数 `R_max` 分别记录。

### 4.5 每阶段 view、leakage 和正确性不变量

| 阶段 | party-local view | 对外 leakage | 必须维持的不变量 |
|---|---|---|---|
| READY/package | 本方 manifest、triple shares、permutation shares、lease 状态、本地 resource preflight | 公开版本/维度/包长/digest；仅固定形状 READY 或既有统一 ABORT，不公开本地资源详情 | 包与 session/party/circuit 绑定；T 不收输入；失败在 lease 前同步；lease 一次消费。 |
| 输入分享/score add | 本方 additive input、随机 XOR masks、GMW gate shares 和 masked `d/e` | 固定 `N` 的消息长度；不公开 raw score/sign | wire XOR 等于两个 additive shares 的模 `2^32` 和。 |
| key/forward route | priority-key wires、secret permutation share、triple shares | permutation 本身不公开；seed 只在 forward 完成后按迭代公开 | key 唯一；forward route 等于同一秘密置换的作用。 |
| comparator/precompute | 每个 pair 的 secret borrow wire、used triples、d/e frames | 不公开 inactive pair bits；只按 L 打开活动 edge bit | strict total order；bit 精确为 `key_left<key_right`。 |
| Select control | 已公开 sample seed、候选/remaining rank/pivots 及获准 comparator bits | 仅 S29 批准的活动图、pivot/result/计数/长度 | candidate 保留目标 rank；更新 residual rank；第五层 exact。 |
| membership/inverse | 本地 tau-derived membership wire、逆路由 shares | 不公开 tau 以外的新 secret；原序 mask 仍共享 | membership 恰含排序前 K；逆路由回 original index。 |
| output/abort/escrow | 本地 GMW output wire share、pad、recipient ciphertext、A receipt、fetch/recovery 状态 | P0/P1: 最终 local share 或统一 pre-commit ABORT；A: `L_A` 的长度、commit/abort、顺序/时序 | XOR 重构 stable mask；commit 前 API 均无输出（内部暂存 share 仍属 view），commit 同时分配两份 share；receipt 后正常 API 返回本地暂存 share；commit 后耐久恢复且不回滚；lease 不重用。 |

GMW `d/e` messages 和本地 triples 虽然不是 `F` 输出，但都是真实完整 view；simulator 必须逐批产生它们、包括未被选择的 pair wires、未公开的 comparator bits、整个固定 triple package、manifest/lease、transport frame 及 crash 前缀。不能以“Beaver triple 标准安全”省略这些联合分布义务。

## 5. P0/P1 simulator 与安全义务

S31 没有构造出可审查的完整 simulator，故 P0、P1 都不是 security PASS。下一轮固定协议后，分别证明：

```text
Sim_b(x_b, z_b, public_params, L_S29)
  ≈ View_b(real GMW+T preprocessing, full schedule, output/abort prefix)
```

至少包含以下 hybrids/义务：

1. **T package**：从本方 `a/b/c` triple shares、secret permutation matrix shares、manifest、digest、ready、used/unused gate records 的正确联合分布采样。每个 gate 的 `c=a∧b`，两方 share 相关；不能让 simulator 拿到 T 的 full tape。T 与腐化方合谋明确排除。
2. **输入转换**：只给 `x_b` 时，生成本方 bit shares、peer 的 masked input message 及 input-sharing transcript；不能从控制器重构 secret input。
3. **每层 Beaver**：在随机独立 triples 假设下，模拟本方 triple shares、masked `d/e` frame、打开值和 output wires；记录电路 wire dependencies、所有 gate id 和 exactly-once 状态。混合实际 CSPRNG 时需 PRF/DRBG 查询界。
4. **比较/自适应 Select**：只由 `L_S29` 的 sample seed、active edge bits/lengths 生成 seed/ack 和已开放结果；模拟器不可获得未公开 all-pairs comparator table。对候选集、pivot 排名和下一层 edge set 的自适应性逐迭代归纳。
5. **membership/inverse**：在选中位置由 L 推导或允许公开的条件下，生成 local membership wires、secret permutation shares、inverse-route Beaver transcripts；不把完整 mask、tau 以外位置或 mask 内部 wire 放入模拟器输入。
6. **output/abort**：以指定 `z_b` 生成 output refresh、本方 recipient ciphertext、A receipt 和 crash prefix；证明 commit 前不向任一方释放记录，commit 时两方输出同一线性化转移，commit 后任一方 crash 都不撤销对方或自身的 durable retrieval entitlement。需覆盖 A 重启/恢复与 pending transaction 清理。
7. **escrow A**：独立证明 A 只能看到 `L_A` 与不可区分的 recipient ciphertext；其被信任保证 durable linearizable commit、但不持有 recipient decryption key。若允许 A Byzantine 或与任一方合谋，现规格不适用，需新增 corrupt-A 功能/模拟器。

建议最终安全优势项写成 `Adv_b ≤ Adv_GMW + Adv_DRBG(q) + Adv_AEAD/HPKE + Adv_A_auth + Adv_package_hash/sign + Adv_model`；可信 T 直接生成 Beaver triples，当前设计无 `ε_OT` 项。A 的原子可用性/持久性是功能信任假设，不是从加密安全推导出来的。GMW 的半诚实定理只可能给出基于该电路/输入分享/输出定义的通用构造结论；`Adv_DRBG(q)`、联合 RNG、A 加密与认证、package integrity、每个公开 bit 的确切 `L_S29` 映射、`L_A` 处理及组合损失都还没有被量化。状态 `CONDITIONAL/UNQUANTIFIED`。

本轮没有可复现区分器，也没有证明 v2 不可能安全；失败类别是“证明缺口”和“依赖未量化”，不是攻击。

## 6. TEST_ONLY 原型与正确性验证

新增 `VFSS/tests/moe_topk/score_to_mask_v3_gmw_test_only.cpp`。文件头要求显式 `MOE_TOPK_ENABLE_S31_TEST_ONLY` 宏；它没有加入 CMake/runtime target。使用真实 S25 `topk_oracle.h` 作为冻结 stable oracle；S25 控制图逻辑作为隔离测试模型。测试控制器持有两方 shares、为每个 Beaver gate 临时重构 `d/e` 和输出只用于验证；这些中心化行为是 TEST_ONLY 禁止进入 party runtime。

| 检查 | 本轮结果 |
|---|---:|
| 8-bit Boolean strict comparator 穷举 | 65,536 个 `(x,y)` 对 |
| Select5 全部 total orders 与所有合法 K | 362,879 个 case，n=1..8 |
| 完整小型电路对 S25 stable oracle | 5,936 个 case；n≤4 的 domain `{INT32_MIN,-4096,-1,0,4096,INT32_MAX}` 全排列赋值、所有 K；n=5..8 各有 tie/boundary 向量、所有 K |
| candidate/residual rank、sample/pivot、active open bits | 每个完整 case 检查 trace 和顺序 |
| membership、inverse routing、原序输出 mask | 每个完整 case 独立逐项对照 |
| 每 gate Beaver triple 消费 | 所有完整路径和 package 解码回路都要求全部 gate 恰好一次 |
| 材料拒绝 | 截断、payload 翻转、错误 session、错误 party、重领、duplicate gate、cross-session triple 共 7 类拒绝 |
| 输出 share law | n=1..8 对所有 bitmask 和所有 uniform pad 值穷举 87,380 组合 |
| N=128/256 gate 与 packed package 预算 | 2 个规格预算点逐项断言 |
| 输出/故障前缀模型 | 12 个 TEST_ONLY 逻辑 prefix，验证 F_TOPK-U XOR law 和 F_TOPK-ATOMIC 的 commit/检索权限状态；“fetch/crash”枚举只是模型状态标签，没有执行实际 fetch、进程崩溃或重启；不是 S25 crash 证据或密码学/分布式原子性证明 |

测试包使用教学性 FNV-1a checksum、`std::mt19937_64` seed 和单进程中心化 gate engine；它验证电路功能和材料索引规则，不验证 CSPRNG、密码学 package authenticity、双进程协议、网络消息、TLS、性能或安全模拟。完整 32-bit score 值没有穷举；全域 correctness 仍须形式推导/更大范围 differential。

直接编译/运行命令及结果见第 10 节与 machine-readable evidence。一次诊断耗时 3,952 ms，仅为该测试 harness 的计时，不是协议性能。

## 7. v2 与 v3 选择、实现与迁移边界

| 路线 | 这轮可执行性 | 输出/泄露 | 当前决策 |
|---|---|---|---|
| v2 proof-only | 可继续作为独立研究；S30 已在完整 output-conditioned sampler 处停止 | 维持 v2 外层功能与原 L；不得给模拟器附加 `Q′/rho/DCF keys`，不得公开 `Q′` 子功能 | 不把 S30 的证明缺口说成不可能性；但当前没有可以直接执行的完整 proof-only sampler，短期不作为运行时准入路线。 |
| score-to-mask-v3 GMW/Beaver | 有可运行的中心化功能模型和包含第三方 A 的固定 gate/DAG 设计；双进程/runtime 尚未实现 | P0/P1 保持 `F_TOPK-U` 与原 `L_S29`；新 A 参与方看到新增 `L_A` 元数据；不得增添公开 `Q′`、raw rank 或完整 mask。新 escrow commit 是输出/中止架构变更。 | 负责人已选 v3 `F_TOPK-ATOMIC`，协议规格独立技术复核 `PASS`，TEST_ONLY 通过；准入下一阶段 opt-in runtime 开发。P0/P1/A 安全证明和 crash recovery 仍是独立安全门。 |

v3 消除 S29/S30 score-conversion 条件分布断点的**架构理由（条件性）**：不再把 `rho_b` 作为影响 Q′、Dealer DCF keys、ready package 的旧式独立前缀变量；改为对输入 shares 和 secret permutation shares 直接运行一个固定 GMW circuit，AND triple 是 T 独立输入生成、逐 gate 条件于 session/circuit 的联合 sharing，再按 Beaver masking 逐层生成 online transcript。若完整 GMW simulation theorem 与所列条件全部满足，新的 proof 可直接覆盖 package+online transcript 的联合分布，而无需假设 v2 的 `rho | 其他材料` 条件独立。S31 只给出这个构造蓝图和功能模型，尚未证明 triples/schema/frames 的具体 simulator，也没有证明泄露完全保持，因此“断点已消除”状态仍是 `CONDITIONAL/NOT_PROVEN`。

迁移边界：下一阶段新增的入口必须显式名为 `score-to-mask-v3-gmw-test/experimental` 一类版本化实验入口；要求 v3 manifest、circuit hash、`F_TOPK-ATOMIC` 输出 contract version、`L_S29` 和 `A_ESCROW_V1` schema version、session-bound package 及 A identity/key binding。v2 API、材料 schema、rho 与 DCF ABI 完全拒绝，不作 fallback。回滚只切回独立 v2 experimental target，不复用/重放 v3 package；secure alias 不得创建。

## 8. gates 与其他状态

机器可读状态见 `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_GATES_2026-10-20.json`。

| Gate | P0 | P1 | 理由 |
|---|---|---|---|
| `END_TO_END_SIM` | `OPEN/NO_GO` | `OPEN/NO_GO` | 功能 TEST_ONLY 执行不等于完整 party-view sampler。 |
| `MODULAR_COMPOSITION_VALID` | `OPEN/NO_GO` | `OPEN/NO_GO` | 不把内部 Q′ 作为子功能输出；未有保持原 outer functionality/L 的组合证明。 |
| `CURRENT_INTERFACE_SECURITY` | `OPEN/NO_GO` | `OPEN/NO_GO` | 现 v2 条件分布缺口不继承 v3；v3 也无具体 proof。 |
| `VERSIONED_REDESIGN_REQUIRED` | `CONDITIONAL` | `CONDITIONAL` | 新选定的原子输出/中止合同需要带 A escrow 与版本化接口；不代表 v3 runtime ready。 |

M6A V3→V4 的状态单独记录为“本轮未触及、无状态变化”：现有治理文件记载 M6A V3 `OPEN`，V4 与正式 LAN/WAN `NOT_ALLOWED/NOT_RUN`。S31 不改变 M6A 门。

## 9. 独立审查与审查边界

本轮 reviewer 已确认未参与 S19–S30 证明、决策或实现，也未编写原型。首轮指出 `word1` 模数和 READY 预检同步，均已修正；其后要求区分正常本地 share return 与 A 的故障恢复 fetch、明确提交前 `⊥` 不代表 party view 无暂存 share、将 DAG 合并为单一 commit/receipt 阶段，并使 READY 只传固定形状 READY 或既有统一 ABORT。以上修订已同步报告、决策和 gates；最终只读技术复核结果为 `PASS`。审查者还要求刷新证据清单中的报告、决策和 gates 哈希；清单已更新。12 个 crash 前缀仍只是原型逻辑 entitlement 模型，不是分布式原子性或进程恢复测试。该检查属于**技术规格复核**；S27 的 S25 功能接收属于**功能接收**；P0/P1/A simulator/theorem 仍属**密码学证明**。三者不互相代替。

技术规格复核通过只满足下一阶段 opt-in 实验 runtime 的准入条件，不替代 P0/P1/A 密码学证明或关闭 security gate。独立 reviewer 对安全准入仍建议在完整证明和 crash/recovery 论证前保持 NO_GO；本报告将“进入实验 runtime 阶段”和“安全接受”分开记录。

## 10. 本轮命令、结果、文件与未运行项

编译命令（PowerShell，产物在 `%TEMP%`，不入仓）：

```powershell
$s25 = 'C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation'
$out = Join-Path $env:TEMP 's31_score_to_mask_v3_gmw_test_only.exe'
g++ -std=c++17 -O2 -Wall -Wextra -Werror `
  -DMOE_TOPK_ENABLE_S31_TEST_ONLY=1 `
  -I "$s25/VFSS/include" `
  VFSS/tests/moe_topk/score_to_mask_v3_gmw_test_only.cpp -o $out
& $out
```

环境：Windows PowerShell 7.6.5；Windows 10 Home China build 26200；MinGW-w64 `g++ 8.1.0`；C++17；`-O2 -Wall -Wextra -Werror`。最终 F_TOPK-ATOMIC 版本编译/运行 exit code `0`，输出 `S31 TEST_ONLY GMW/Beaver functional prototype PASS`；计数：`comparator_pairs=65536`、`select_total_orders=362879`、`full_pipeline_cases=5936`、`all_beaver_gates_one_shot=true`、`material_fault_cases=7`、`abort_prefix_cases=12`、`output_share_cases=87380`、`budget_cases=2`。诊断耗时 `3952 ms`，不是性能结果。最终 TEST_ONLY 源 SHA-256 `AB8BB143E31B30BD7588C7E7F4A0A4A586E28B90E2602B12EF8574E977976144`，临时二进制 SHA-256 `557D74CDB023A9E59F248817C8A110AB3E01C7497871844E992C4015F94E8450`。

本轮修改/新增文件仅限：

- `VFSS/tests/moe_topk/score_to_mask_v3_gmw_test_only.cpp`：隔离的 TEST_ONLY GMW/Beaver 电路功能模型和检查。
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_REPORT_2026-10-20.md`：本报告。
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_DECISION_2026-10-20.md`：负责人/路线决策记录。
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_GATES_2026-10-20.json`：机器可读门禁。
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S31_SOURCE_TEST_EVIDENCE_MANIFEST_2026-10-20.json`：来源、测试、命令、环境和哈希 manifest。

没有修改任何 `VFSS/` runtime 源文件、`VFSS-baseline/`、论文、参考工程、key/cert、材料包或构建产物；没有创建 secure alias；没有运行 S25/S27 功能矩阵、conformance/E2E runtime、独立进程验证、正式性能矩阵、N=128/256 实际大电路、M6A V3/V4。

本轮最终路线：**v3 规格与 TEST_ONLY 原型通过，准入下一阶段默认关闭、显式版本化的 opt-in runtime 开发；S31 不接入运行时代码。** 新实现必须保留 `F_TOPK-ATOMIC` 与 A escrow 边界，并继续将 P0/P1/A 完整 view 证明、量化优势和 crash/recovery 论证设为独立安全门。此阶段准入不是 runtime 已实现、不是安全 PASS，也不声称 v2 不可能。
