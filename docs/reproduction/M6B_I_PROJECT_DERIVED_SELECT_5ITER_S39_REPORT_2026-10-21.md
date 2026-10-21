# M6B-I-PROJECT-SELECT-5ITER-S39 复核报告

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`
结论：receipt/v1 首个联合模拟点未闭合；receipt/v2 opt-in 候选移除了该回执相关性，但仍等待独立密码学复核，整体安全状态保持 `OPEN_WITH_EXACT_GAP`。

## 1. 裁决摘要

S38 已分析的 peer-envelope G0→G1 替换沿用为前序条件结论，本报告不把它重新算作 S39 的结果。S39 只处理从 H4 本方计算前缀到 A receipt、COMMITTED 响应和一次 committed fetch 的联合分布。

receipt/v1 为确定性值：

```text
R1 = SHA256(
  domain("score-to-mask-v3/escrow/commit/1") || transaction || session || invocation ||
  N || K || circuit_hash || gate_schedule_hash || context_hash || package_set_hash ||
  H(C0) || H(C1))
```

其中 `context_hash` 与 `package_set_hash` 只由签名 metadata-only context 交给 A；二者与 package 和材料生成同源。H4 提供的是 party 前缀的条件采样结论，并未提供上述 hidden context 与 H4 状态的联合条件分布。需要的式子是：

```text
Law(Vpre_b, z_b, L_S29, pkg_b, R1, COMMITTED_b, fetch_b)_real
  ≈ Law(Sim_H4_b(x_b,z_b,public,L_S29,pkg_b), R1, COMMITTED_b, fetch_b)_ideal
```

但 v1 下 party simulator 不能从允许输入恢复 `context_hash`、`package_set_hash` 或 `H(C_(1-b))`，也没有假设能从它们的哈希生成正确的条件分布。碰撞抗性约束的是碰撞查找，不蕴含隐藏性或随机预言机输出均匀；仅凭边缘分布也不能推出固定 `(z_b,L_S29,pkg_b)` 后的联合分布。因此 v1 停在该式，缺口是证明接口/相关性缺口，不是攻击、代码缺陷或不可能性结论。

S39 评估并实现默认关闭的 receipt/v2：最终提交时，A 采样 `U←{0,1}^256` 并将其与 `COMMITTED` 状态在同一 SQL transaction 保存。理想 A 随机源下：

```text
Pr[U=u | x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,ctx_A,Hpkg,H(C0),H(C1)] = 2^-256
```

所以 H4 sampler 可完成本方前缀后，从 A 的真实角色路由响应取得 token；或者在理想服务模拟中为该交易采样一次 `U`，把它 memoize 到 COMMITTED 响应及已提交 fetch。transaction ID 仅由公开 session/invocation 编码产生。COMMITTED/status/transaction 字段是公开或已有服务字段，fetch ciphertext 是本方已生成并提交的密文。固定 32-byte token 与这些字段组成的后处理不会重新引入 v1 的 context/package-set 哈希相关性。

该局部闭合依赖理想 A token 独立均匀。实际服务使用 OpenSSL `RAND_bytes`；把它替换为理想独立随机源的优势保留为 `Adv_DRBG_A(q)`，其中 q 覆盖 A 进程中 token 与可被 parties 观察到的相关 DRBG 输出。该优势没有量化。它不关闭 S38 的密文替换假设、H4 其余条件步骤、F_T/v1 的真实生成器桥、签名、TLS 或输出正确性证明。未参与 S39 推导者的独立密码学复核待完成，因此状态是 `CONDITIONAL / PENDING_INDEPENDENT_CRYPTO_REVIEW`，不是已审通过。

## 2. 固定对象与来源

- S39 起始 worktree：`C:\Users\28641\.codex\worktrees\s39-receipt-v2\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-project-select5-s39`。
- 起始 HEAD 精确为 S38 `2f350b6d79fc88df381347b597cbead4398a7902`；父级 S37 `5c31ebfeaf6eb072421badf31d7354431b984d35`，再前为 S36 `ca5eb001e72c971a7dbaf87d789d4a7fbcba1b9a`，S35 runtime commit `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`。
- S35 85 项规范源清单 canonical SHA-256：`af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`。该基线文件集合和 S38 manifest 中记录一致；本轮对该集合重新计算变更后的 S39 hashes，canonical SHA-256 为 `0c385152e4bcceb64de3d6c19666a5bc8f4b9330adc151800f927865db4df3bb`，完整逐文件哈希见 source/build/evidence manifest。
- S38/S37 原 worktree 未修改；本轮在从 S38 精确提交创建的隔离 worktree 中操作。`VFSS-baseline/` 未改动，未将密钥、材料、证书、构建物或原始大日志放入仓库。
- 证据区分：Protocol I 论文定义不蕴含本项目派生 Select5 是论文算法复现；稳定 Top-K mask 是项目规格；本文列出的 v1/v2 字段与顺序是源码事实；定向 E2E 是功能观察；H4、F_T/v1、理想 A token 与密码优势均是证明假设或待验证桥接。

## 3. 安全游戏、视图与权限

对 `b∈{0,1}` 分别运行同一接口：

```text
Sim_b(x_b, z_b 或 ⊥, public, L_S29, pkg_b, static_b;
      F_T/v1 party route, F_A-ATOMIC/v2 party route)
```

### 3.1 联合变量

| 变量 | 真实来源 | Sim_b 的权限 | 理想目标中的处理 |
|---|---|---|---|
| `x_b` | 本方算术输入 share | 可得 | 输入 |
| `z_b` | F_TOPK-ATOMIC 本方输出 share | 可得 | 输出 share；成功路径为 `N` 个 XOR share bits |
| `L_S29` | 已批准活动泄露 | 可得 | 原样固定，不扩张 |
| `pkg_b` | F_T/v1 签名发给本方的 package | 仅本方 | 输入，保持本方 package 与 H4 状态相关性 |
| `static_b` | 本方 RSA/TLS 私钥与本方配置 | 可得 | 输入；不含 peer 私钥 |
| `Vpre_b` | 在线消息、gate/wire 状态与本方加密前转录 | H4 sampler 产生 | 逐方 H4 成功路径条件视图 |
| `ctx_A` | F_T/v1 签名、只路由给 A | 不可得 | A 内部隐藏状态 |
| `context_hash`,`package_set_hash` | context 的确定性摘要 | 不可得 | A 内部状态；不并入 L_S29 |
| `C_b` | 对 `z_b` 的本方 recipient envelope | 可生成；重复 fetch 缓存 | fetch 返回原密文，不能重新加密替换 |
| `C_(1-b)` | peer 对其输出 share 的 envelope | 不可得 | S38 J_b 仅在 proof-only reduction 中生成/替换，不能授给 Sim_b |
| `R1` | A 从 context、package 与两密文摘要导出 | party 可见结果，但 v1 无 sampler | 不满足 H4 的 hidden-context 联合条件式 |
| `U` | A 在 v2 最终提交生成的随机 32 bytes | 只经真实响应可见 | 服务模拟每交易采样一次并跨响应/fetch 复用 |
| `COMMITTED`, transaction ID | A 状态机；tx 由公开 session/invocation 编码产生 | 可见响应 | 确定性服务后处理 |
| `fetch_b` | 已提交后的 authenticated 本方 fetch | 可见 | `(C_b,U)`；不含 peer ciphertext |

Party simulator 不得得到 `pkg_(1-b)`、`z_(1-b)`、`ctx_A`、T 私钥、peer 材料或未授权随机带。S38 的 `J_b` 是 IND-CPA 归约器的 proof-only 联合采样权限，与 party simulator 的接口分开；不能以 J_b 可生成 context 为理由向 Sim_b 透露 context。T/A 条件受信且与腐化方不合谋。A-only 腐化、party+A/T 合谋不在本轮模型。

视图比较采用对固定输入对的整体联合分布，而不是从无条件边缘推出每个低概率点条件式。目标成功路径至少观察 `(x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,提交响应,fetch_b)`。party simulator 延迟不在 L_S29；A 的 context/hash、ciphertext/hash/length、到达次序与时序、数据库状态和 fetch 记录只属于独立 `L_A`，不合并到 party 泄露。v2 没有新增 L_S29 字段。

## 4. Receipt/v1 首个断点

源码 `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:245-278` 的 v1 receipt 依次哈希 domain、transaction、session、invocation、N/K、circuit hash、gate-schedule hash、context hash、package-set hash、按 party index 排列的 `H(C0),H(C1)`。`score_to_mask_v3_gmw.cpp:575-593` 生成签名 context 并将 package-set hash 绑定其中；A 校验 transaction metadata 与该 context (`score_to_mask_v3_escrow.cpp:343-369`)。`party.cpp:443-453` 的 transaction ID 只编码公开 session/invocation；它不是 v1 断点。

H4 可用条件 law 可记为：

```text
Law(Vpre_b | x_b,z_b,L_S29,pkg_b,static_b) = H4_b(...)
```

但需要构造的下一步为：

```text
Law(ctx_A, package_set_hash, H(C_(1-b)) |
     x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,C_b)
```

S38 的 J_b 可在 reduction-only 游戏中用完整联合生成律运行 A，并证明只替换 `C_(1-b)` 的 IND-CPA 后处理；它没有证明 `H4_b` 能以限制权限生成与隐藏 `ctx_A` 的正确耦合。v1 receipt 是上述隐藏量的确定性函数，所以缺失的 exact step 是：

```text
Law(Vpre_b,z_b,L_S29,pkg_b,R1,COMMITTED_b,fetch_b)_real
  ≈ Law(H4_b(...),R1,COMMITTED_b,fetch_b)_ideal
```

现有 assumption set 没有给出该等式。SHA-256 collision resistance 既不蕴含隐藏性，也不生成条件采样器；没有 ROM/PRF 或条件最小熵假设可引用。本轮不把该证明缺口说成 v1 被攻击或数学上不可能。

## 5. Receipt/v2 规格与实现

### 5.1 版本和职责

- v1 保持现有格式并仍为默认。v2 由 T 签发 context 中已有的 `receipt_format_version=2` 字节签名绑定；context 本身仍是格式 v1，协议/材料包版本、身份、TLS pin 和一次性 lease 不变。
- 构建时必须显式同时启用 GMW v3 和 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON`。v2 未 opt-in 时签发与 context decode 都拒绝 v2；receipt version 3 等未知值拒绝。
- Party 只检查 receipt 是 64 个小写十六进制字符。源码没有让 party 验证其密码学内容；因此 v2 token 在当前条件受信 A 模型下无需 MAC。若以后要求 A 不受信、party 独立验证或公开审计，需要新协议版本的 MAC/签名、密钥路由与对应 `Adv_PRF`/签名证明；不能把裸随机 token 当认证证据。

### 5.2 生成、保存、提交与恢复

当第二份收件人密文已在 A 的 `BEGIN IMMEDIATE` 事务中写入、且状态仍 PENDING 时，A 生成 `U←RAND_bytes(32)`，随后在同一最终 SQL 事务更新 `state=COMMITTED, receipt=U`，最后提交事务 (`score_to_mask_v3_escrow.cpp:315-439`)。第一份 share/PENDING row 可能早已在第一请求自己的事务中提交。若 token RNG 失败，本次 SQL transaction 回滚并返回服务错误；该失败时的客户端/重启行为没有被列入随机源故障 E2E。

正常提交响应格式是状态 byte、transaction 长度、ciphertext 长度、32-byte receipt、transaction 和可选 recipient ciphertext (`:472-480`)。submit 的 COMMITTED 响应带空 ciphertext (`:597-602`)；party request 还经过 TLS 服务证书 pin 与 READY 检查 (`:675-694`)。P0/P1 两个服务线程从同一行取得同一个 token，但真实 API 返回时刻无需相同。一次从已 COMMITTED 状态开始的 party `fetch_b` 返回且仅返回本方密文和同一 token (`:442-469,604-608`)。重复同密文 submit 在已提交状态读取同一个 receipt；同交易、同 party 改密文被拒绝 (`:371-385`)；重复 fetch 保持相同 ciphertext/token；错误 party identity 在 TLS pin/auth 路径拒绝。

token 不在 submit request 中传入。A 内部仍按交易行、context/package-set hash 与两密文 hash 绑定状态；v2 只替换 party 可见 receipt 值，不移除 A 的内部审计字段。transaction/status/receipt/ciphertext 的字段长度与 v1 相同，receipt 仍为 32 bytes（API hex 仍 64 chars），所以不会额外增加已有应用消息长度。A 可见的到达顺序/时序仍属于 L_A。SQLite WAL/FULL 的配置与进程级 restart 测试不证明物理断电或介质耐久。

### 5.3 v2 simulator 与优势项

对任一 `b`，在 H4 和理想 `F_A-ATOMIC/v2` 成功状态机下：

```text
Sim_b:
  1. 接收 x_b,z_b,public,L_S29,pkg_b,static_b；按 H4_b 生成 Vpre_b。
  2. 用公开本方 recipient key 和新鲜本方加密随机性生成 C_b=Enc(pk_b,z_b)，缓存 C_b。
  3. 通过 A 的不透明 role route 走正常成功 commit 服务响应；Sim_b 不读 ctx_A。
     A/理想服务内部只在 COMMITTED 事务生成一次 U；Sim_b 只得到真实 party
     endpoint 应见的 COMMITTED/status/transaction/U。
  4. 对一次 committed fetch 复用缓存 C_b 与相同 U；后续同一调用重试复用同一对值。
```

对理想独立 token `U`，第 3 步的 conditional law 对第 2 步及所有隐藏 setup 量均是均匀分布，故把 `U` 附加在 H4 sampler 后保持联合分布；status 与 transaction 是确定性公开后处理，fetch 是缓存本方 ciphertext 与 `U`。此处没有用 SHA 输出“看似随机”的说法，也没有对 party simulator 开放 J_b 的 peer/context 权限。服务路由是 opaque capability，只负责会话/交易路由，不返回 A context 或数据库内部摘要。

真实 A 使用 OpenSSL `RAND_bytes` 而非数学理想源。v2 回执步骤的可归约优势保留为 `Adv_DRBG_A(q)`，要求该 DRBG 在至多 q 次、包含 token 及 party 可见 TLS/DRBG 输出的联合观察下伪随机；不填安全位数。完整协议总界还需加上既有 `Adv_H4_b`（若有正式构造）、S38 `Adv_Env-IND-CPA` 与最多 `2ε_perm` 的严格置换截断项、`Adv_DRBG_party`、`Adv_signature`、`Adv_TLS`、envelope OAEP/GCM 与 nonce 项、哈希绑定项以及输出正确性前提。各项符号量尚未量化，且 S38 J_b 不由本轮重新证明。receipt/v2 局部引理不是完整安全结论。

## 6. 输出与故障范围

`F_TOPK-ATOMIC` 和原序稳定 Top-K XOR mask 不变。两份密文的第二份 deposit、`COMMITTED` 状态和 receipt 在最终 transaction 的共同逻辑提交点后可恢复；第一份 pending deposit 可先独立存在。双方客户端可能在不同物理时刻收到 receipt 并返回本方 share。一次成功 fetch 只从既有 `COMMITTED` 起点证明。

不声称 v2 证明任意 crash/abort 前缀。测试覆盖的丢回执恢复、pending/precommit abort、服务进程 restart、重复请求和身份拒绝是功能合同观察；API `⊥` 不说明 party 内存未曾形成本方中间 share。自然 I/O 故障本轮未运行；历史状态是 `NOT_REPRODUCED`。随机源失败、物理掉电、存储损坏、恶意 OS、恶意 A、A-only 腐化、A 与 party/T 合谋均未由本轮 theorem 覆盖。

## 7. 定向构建与测试

环境为 WSL Ubuntu 24.04，CMake 3.28.3、Ninja 1.11.1、GCC 13.3.0、OpenSSL 3.0.13；SQLite3 使用系统运行库及外部抽取的头文件。构建目录和原始日志在 `/tmp`，未入库。

| 配置/运行 | 结果 |
|---|---|
| fresh default configure；GMW OFF，receipt/v2 OFF；检查 target help | PASS：没有 v3 target |
| GMW ON、receipt/v2 OFF fresh Release build；runtime conformance | PASS |
| v2 OFF 的 `--receipt-v1-smoke` | PASS；v1 格式与提交后恢复兼容 |
| GMW ON、receipt/v2 ON fresh Release build；runtime conformance | PASS |
| v2 ON 的 `--receipt-v2-only` | PASS：独立进程正常提交、丢回执后服务 restart/fetch、提交前 abort/restart；校验 oracle share、相同 token/ciphertext、重复同密文 submit、改密文拒绝、错误身份拒绝 |
| v2 ON 的 `--receipt-v1-smoke` | PASS |
| v2 ON、GMW OFF 的非法组合 configure | PASS：CMake 明确拒绝 receipt/v2 缺少 GMW opt-in 的配置 |
| 首轮 v2 conformance/E2E | FAIL，随后修复 context decode 仍硬编码 format 1 的接线缺陷并重跑通过；完整失败与通过输出均在外部 append-only log |
| S35 195 案矩阵、N=128/256 的 v2 E2E、全仓测试 | NOT_RUN |
| 正式 LAN/WAN 和物理 RTT | NOT_RUN / NOT_MEASURED |
| 自然 I/O fault | NOT_RUN（历史 NOT_REPRODUCED） |

构建产物 SHA-256：

| 配置 | `libsytorch.a` | conformance binary | process E2E binary |
|---|---|---|---|
| v2 OFF | `c386f1925db131572d3b40026322c82ae21988c72ef98db9377cf30760d4a926` | `a973b7057eb7a49e098f0ac2b4538c8e634d527e222078be01e9d52535aee371` | `a57c86c30cb2327688e4ca846caa53746f2106d63186c93c5f373e866e1df6f6` |
| v2 ON | `15cdf276d28bd3c7df42f986f0919ce7289712a4b90d1adfcbcfc7fa800852a8` | `3404064805d566353aab0cab6d423d807efab7e7d1581e0290ff2831fda79e36` | `05ea0521e76f434df255d9e052c32eb579a685c95e58537fa887db56372770a8` |

上述 CTest 的计时/phase/RSS 是本机诊断和功能记录，不是正式性能或 RTT。manifest 记录外部日志 SHA-256 与实际目标命令/构建配置。

## 8. 审查状态与下一个断点

本报告作者未参与 S37/S38 的推导；S38 reviewer 只复核 S38 的 J_b 子引理，不计作 S39 的独立复核。S39 的 receipt/v2 条件 law、F_T/A 信息权限、`Adv_DRBG_A(q)` 依赖和真实响应字段尚待未参与本轮推导者逐项复核；当前独立审查状态 `PENDING`。

在 v2 局部相关性问题之外，下一处更大的证明断点为：从理想 F_T/v1 的联合材料分布、签名接口和随机 tape，桥接到真实 Dealer 的 Beaver triples、permutation shares、一次性 package、OpenSSL DRBG 与签名实现。所需的是完整调用序列下的条件联合分布/优势归约，包含 package/context 签名及对两方可见输出的路由；本轮未向该点外推。
