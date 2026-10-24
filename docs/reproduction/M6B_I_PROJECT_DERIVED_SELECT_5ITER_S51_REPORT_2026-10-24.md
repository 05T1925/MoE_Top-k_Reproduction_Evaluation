# M6B-I-PROJECT-SELECT-5ITER-S51 详细复核报告

日期：2026-10-24
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论摘要

本轮审查的功能是项目派生的五迭代稳定 Select：输入按 signed Q20.12 score 解释，降序排列，同分按原始输入索引升序；输出原始顺序下恰有 K 个 1 的 Top-K XOR mask shares。它借鉴 BB90 研究思路，但不是 BB90 原算法、BMW16 或 BB90+DCF 的复现。

S50 的 `Kpre_3` 只到本方 output share。S48 正常 API 在收到 escrow COMMITTED receipt 后返回计算期间留在内存的 share；fetch/decrypt 在另一个 recovery API 中。S51 本次任务把要模拟的 party-visible 调用顺序明确为“提交后 authenticated fetch、解密并核对后 API 返回”，因此新增显式版本化 API 实现该调用顺序。S32/S42 历史记录只要求 commit 后不早于共同提交释放 share、并允许之后恢复取回；它们没有要求正常 API 必须等待 fetch。故本轮不把旧 API 描述成违反既有合同的缺陷，也不把 S51 API v2 的严格返回顺序回写为旧合同。

新增的 `score_to_mask_v3_execute_party_v2` 明确要求 fetch callback 与本方 recipient private key，使该版本正常路径成为：完成本方计算并形成本地 share → 加密本方 share → authenticated submit → 收到 COMMITTED receipt → authenticated fetch 本方密文 → 校验 fetch receipt 与提交 receipt 相同 → 用本方私钥/签名 package 解密 → 校验解密 share 等于本地计算 share → 返回 fetched share。提交已确认但 fetch/decrypt/match 失败时，返回 `kRecoveryRequired`，保留 transaction 和 receipt，`local_mask_share` 为空。它不声称此前内存中的局部值被清零；只保证该版本 API 不把该值作为成功结果返回。原 S48 API 是不同调用次序的旧接口，本轮不认定其为安全缺陷。

针对修复后 API 成功应用消息视图，构造了 P0/P1 对称的 `Kpost_3`。在理想 F_T/v3、ideal A 独立 token 随机源、双方是同次调用的合法诚实 endpoint、且成功执行是外生环境前提时，submit/receipt/fetch/解密/API 返回后缀有直接采样器，未见需向 party simulator 交付 peer package/share、A context、T 私钥或 proof-only `J_b` 的步骤。该结论不包含物理 wall-clock 时间视图；也没有证明按真实超时结果筛选的成功样本与 `Kpre_3` 前缀独立。因此，后缀消息投影是 `PASS_WITH_SCOPE`，而将其与 `Kpre_3` 合成“真实超时筛选成功执行”的联合分布仍是 `CONDITIONAL`，不等同完整 party-view 安全通过。

## 2. 固定对象、来源与 L_S29

### 2.1 被测代码与工作区

- 起点：最新 main `c3926c68fd14f270faa8b55234311071947fa080` 的后代 S50 `ff77b9694a1259c3f314f145e06fcb980cda0bfb`，tree `04bda3a66fec7e37a012b91d8aafae91857a49e6`，父提交为 S49 `50bc223c19e24bdf052eacbd47be8df6a1d56d98`。
- 固定前序：S47 `feb5e6677568d649906f46d0e611d5d52349c07f` → S48 `e05b01d5c7e3899297b76a5e0fca212d38d24229` → S49 `50bc223c19e24bdf052eacbd47be8df6a1d56d98` → S50 `ff77b9694a1259c3f314f145e06fcb980cda0bfb`。每个提交均由固定提交对象恢复；S50 manifest 所列 11 个源码 Git blob 均匹配。
- 新分支从固定 S50 链创建。S51 初始工作树干净；没有从桌面主工作区或旧 S51 worktree 复制文件。用户主工作区原有未提交内容未碰；`VFSS-baseline/` 未改。
- S48 原 manifest 的 9 个交付 Git blob 和 S49 原 manifest 的 6 个交付 Git blob 分别在固定 S48/S49 提交上核对。其清单的 `raw_sha256` 取自当时 Windows checkout 字节；与 Git blob 原始 LF 字节的 SHA-256 不总相等（S50 已记录 checkout line-ending 限制）。本轮以提交路径下的 Git blob SHA-1 作为对象身份，并在 S51 manifest 同列 S50 记录值、固定 blob 与本轮工作树字节 SHA-256；不把换行规范化差异误报成源码替换。

### 2.2 找回原始 M6B S29 泄露来源

S47–S50 分支链不含 M6B S29 原始决策/报告。通过 Git 对象只读恢复到 snapshot commit `502c9cc68a23b06b3347d34d60e679d98d3f60a4`（parent 是 main `c3926c68fd14f270faa8b55234311071947fa080`，tree `7f4b34b39b10b77fa4cd3c70373be4a6b847dcc9`，commit subject `Codex worktree snapshot: startup-cleanup`）。该 snapshot 的 S29 source manifest 写明来源是 S25 worktree snapshot，候选未提交（`candidate_committed=false`），不是正式 S29 milestone commit；因此来源已找回但有这个治理限定。

被 S36/S42 引用的确为 **M6B 项目派生 S29**，不是另一个 BMW16 S29。原报告 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_REPORT_2026-10-20.md` Git blob 是 `760962e1cdaba1f8459d415da5e3f7b1f6cbe934`，原始字节 SHA-256 是 `aa8dde4bde549c59bffa4ce2f2d2343e52d53912f734d7cc2134c6e15cccf717`；S29 决策、gates、manifest 的对象及 SHA-256 见 S51 source manifest。S36/S42 摘要与 S29 §3.3 一致，六类为：

1. 公开参数与协议绑定信息；
2. shuffle 后获准公开的 endpoint/position；
3. pivot 与比较结果 bit；
4. 仅在相应活动迭代公开的 sample seed；
5. 已批准 masked values；
6. comparison count、通信长度与统一 ABORT。

本轮没有新增 `L_S29` 或 `L_A`。S29 的全视图安全 gate 当时仍是 OPEN/NO_GO；“列表写作 approved L”不是已经通过的完整安全证明。后来的 S31/S32 另行冻结最终输出 share/原子提交正常合同，本轮不从 S29 反推这些后来决定。

### 2.3 证据层级

- **论文定义**：BB90 与相关论文是研究参照，本轮不审出论文等价结论。
- **本地源码事实**：S48 的 party API、escrow DB 状态机、TLS app payload、加密/解密代码由行号锚定。
- **项目派生证明**：S50 的理想 IID FTv3 `Kpre_3` 和本轮 ideal A/token 下的 `Kpost_3`，分别限其模型。
- **待验证假设**：真实 OpenSSL DRBG、A token RNG、Dealer 签名/发材、TLS/mTLS、RSA-OAEP/AES-GCM、真实计时选择及全协议组合仍 OPEN。

## 3. 模型、角色和模拟器权限

固定公开调用元组：

```text
P = (session, invocation, N, K, route,
     circuit_hash, gate_schedule_hash,
     FT_version=3, sampler_version, package_format,
     escrow_context_format, receipt_format=2,
     recipient public-key identities, party TLS identities)
```

每个字段的绑定位置须按 S48 package/context/request parser 解释；版本空间彼此独立。party transaction id 是 `s2m-v3-<session hex>-<invocation hex>`，是公开绑定的确定函数。

对 `b∈{0,1}`，沿用 S50 输入：

```text
q_b^3 = (x_b, z_b, L_S29, pkg_b^3, static_b, P)
Vpre_b^3 is sampled separately, conditioned on q_b^3 and cap_success.
```

`Kpre_3^b` 只得到本方输入、理想本方输出 share、本方 FTv3 package、公开参数、既有泄露、本方静态状态与本方真实前缀；S50 的独立复核结论只对 `1≤K≤N≤256`、ideal IID F_T/v3 且发材 `cap_success` 有效。它不把 peer package 或 Dealer tape交给本方模拟器。

在 S51 的后缀执行中：

| 角色 | 可用状态 | 不转交给被模拟 party 的内容 |
|---|---|---|
| 腐化方 simulator `Sim_b` | `q_b^3`，以及 Kpre3 产生后作为其当前状态的 `Vpre_b^3`、本方 TLS/RSA 静态凭据、自己的加密/通信随机带、真实格式的 A 服务响应 | peer 的 package、share、凭据；A-only context/状态；T 私钥；reduction-only `J_b` |
| 诚实 endpoint `P_(1-b)` | 自己同次调用的 `x_(1-b), z_(1-b), pkg_(1-b)^3`、自己的 TLS/RSA 凭据和公开参数 | 不与 `Sim_b` 共享其材料或 share |
| 托管方 A | 已签名 metadata-only context、T 公钥、mTLS client identity、两份密文及其 hash/length、DB 状态与到达顺序 | 不持有任何 share 解密私钥；A 的上下文/内部状态不交给 `Sim_b` |
| 离线 T / ideal `F_T/v3` | 同一调用的关联材料、两个各自 package、只给 A 的 context；生成后退出 | T 私钥与联合生成随机带不交给在线方 |

A 是基线模型中的条件受信且不与任一方合谋的服务；A-only corruption、A+party 合谋不属于本定理。A 的 context、package-set digest、对端密文/hash/长度、PENDING/COMMITTED、交易顺序和 A 的内部时序归 `L_A`/受信服务内部视图。本方请求等待时间可以被观察；本轮成功消息投影不把它追加进 `L_S29`，真实 wall-clock 侧信道另列 OPEN。

## 4. 实际状态机与 party-visible 字段

### 4.1 本方与 A 的真实因果顺序

1. `score_to_mask_v3_execute_party_v2` 在已完成 GMW/Select、inverse route、output refresh 后持有本地 `local_output`；其仅在本方进程内存中，代码不在这里写 share 文件。这个内部值仍属于该腐化进程的静态/动态 view，API 修复没有声称它在提交前不存在或被清零。
2. 本方用 `encrypt_share(local_output, package, party, own_public_key)` 构造自己的 recipient envelope `C_b`，随后以 package 绑定的 `transaction_id` authenticated submit。submit plaintext envelope含：magic `S2MA`、envelope version、operation、party、reserved byte、session、invocation、circuit hash、gate-schedule hash、本方 package hash、transaction id 长度/字节、ciphertext 长度/字节。
3. 每条 escrow TLS 连接在认证 server identity 与本方 client certificate 后都返回固定应用 READY `{ 'A','R','E','A','D','Y',1 }`（7 bytes），随后请求/响应使用 escrow request phase。TLS record 字节不在这里的应用 payload 计数或证明式中。
4. A 验本方证书指纹、party 字段与 signed context 绑定。第一次有效 deposit：`BEGIN IMMEDIATE`，写 transaction 元数据和一份 share，状态为 PENDING，并提交该次 SQLite 事务。两份密文不是在同一个首次 SQL transaction 同时插入。
5. 诚实 endpoint 以自己的输入/输出/package/凭据加密并提交第二份 envelope。第二次 deposit 的 `BEGIN IMMEDIATE` 内插入第二个 share、检测两行完整、生成 token、把 transaction 更新为 COMMITTED 并写入 token，再提交事务。receipt/v2 token 在该调用中只生成一次。
6. 任何先到的 submit 处理线程都等到 A 查询到 COMMITTED（或超时进入 ABORT；超时不属于本轮理想成功路径）。故两方收到响应的时刻可以不同；没有物理同时返回保证。
7. submit response `make_response` 字段为 `status || u16(tx_len) || u32(cipher_len) || receipt[32] || tx || ciphertext`。成功 submit 的 `status=COMMITTED`、`cipher_len=0`。固定 tx 长度 72 bytes 时，其应用 payload 为 111 bytes；party API 把 32-byte token 编成 64 个小写 hex 字符。
8. commit 后，本方开第二条 authenticated TLS 连接，收到同一 7-byte READY，再请求本方 fetch。A 仅查询并返回本方 recipient row 的 ciphertext 与同一 token；fetch response 的 payload 是 `1+2+4+32+72+|C_b|=111+|C_b|` bytes。fetch 请求不带密文，使用同一 transaction/调用 package 绑定。
9. 本方检查 fetch token 等于 submit token；用本方 private key 和签名 package identity 校验 RSA public-key fingerprint、RSA-OAEP-SHA256/MGF1-SHA256 解包 AES key，并按 package 中 session/invocation/party/circuit/schedule 为 AES-256-GCM AAD 解密；检查 bits 与本地 `local_output` 完全相同后，API 把**fetch 解出的** share 放入返回对象。

应用帧中 A 返回的字段和内容都可由消息格式、公开调用绑定、本方 `C_b` 与一个 token `U` 生成。party-visible view 包含：本方生成并发出的 `C_b`、submit/fetch 的 READY 与消息响应、transaction id、COMMITTED 状态、receipt/token、本方 fetch 回来的 `C_b`、解密检查结果、最终 API share。它不包含 `C_(1-b)`、peer hash、A context、package-set digest、A 数据库状态或 A 内部到达顺序。消息长度是 view；对 N、密钥长度及本方 `C_b` 条件固定后，本方请求/响应长度是确定值。A 的内部计时/到达次序保留在 `L_A`，而本方的 wall-clock 等待观测不在上述离散 message projection 中。

### 4.2 显式版本化 API 顺序与 callback 异常修正

固定 S48 源码在提交 receipt 通过格式检查后直接返回 `local_output`；`score_to_mask_v3_escrow_fetch` 与 `score_to_mask_v3_decrypt_recipient_share` 由独立恢复 API 提供，原 E2E 在 party worker 返回之后才调用它们做 recovery 检查。S31/S32/S42 的历史记录将 COMMITTED 作为逻辑共同可领取点，并允许随后独立 fetch/recovery；没有找到“每个正常 API 必须等到 fetch/decrypt 后才返回”的既有要求。因此这是旧 API 的既定顺序，不能称为原合同缺陷。S51 为本轮要求审查的 fetch 后缀显式新增 v2 API 顺序。

修复后显式入口 `score_to_mask_v3_execute_party_v2` 强制 fetcher 和本方 private key 参数；成功结果来自 fetched envelope。错误状态精确区分：

- submit 结果不明：`COMMIT_STATUS_UNKNOWN`，无 share、无已知 receipt；之后按 transaction recovery；
- receipt 格式异常：`COMMIT_RECEIPT_INVALID`，无 share；
- COMMITTED 已确定但 fetch、receipt match 或解密/内容匹配失败：`COMMITTED_FETCH_REQUIRED`，保留 tx 与 submit receipt，API 不给 share；
- 正常成功：只有 fetch receipt match、recipient decryption 与 computed share 相同才 `kCommitted`。

该候选把原 `score_to_mask_v3_execute_party` 函数签名迁移为显式 `_v2` 名称，没有保留旧名称的兼容包装；本仓库内相关调用点均已迁移。外部消费者的源码/ABI 兼容性本轮未运行，单列为 `NOT_RUN`。此项是实验接口迁移状态，不影响下述理想模型模拟范围。

独立 reviewer 随后指出，submitter/fetcher 是 `std::function`，调用方可抛出任意异常类型；`catch(std::exception)` 不会捕获例如 `throw 17`，会使新 API 逸出而非返回规定的恢复状态。S51 将这两个 callback 边界改为 `catch(...)`：submit 异常映射为无 share 的 `COMMIT_STATUS_UNKNOWN`；确认 COMMITTED 后 fetch/decrypt/match 异常映射为无 share 的 `COMMITTED_FETCH_REQUIRED`，保留 transaction/receipt。新增 TEST_ONLY 整数异常独立进程用例并验证恢复。可复现缺陷只指新 v2 callback 异常边界；旧 API 的提交后返回顺序不认定为缺陷。未增加旧接口静默回退。

最终修复前，以同一 TEST_ONLY submit 回调 `throw 17` 做了反事实复现：回调已先完成 A 的 COMMITTED submit，随后非标准异常逃出 `catch(std::exception)`，P0 进程以 134 退出，P1 以 0 退出，E2E 报 `party E2E failed: P0=134 P1=0`；该失败不能走到设计的 `COMMIT_STATUS_UNKNOWN` 恢复状态。恢复 catch-all 后，最终二进制同一用例以 E2E exit 0 完成，P0 返回无 share 的 `COMMIT_STATUS_UNKNOWN`，重启 fetch 恢复双方 share 并匹配 oracle。完整 pre-fix 与 post-fix 记录见 case JSON。该对照是人工 callback 异常注入，不是自然 I/O 故障。

### 4.3 可核验的源码映射

| 行为 | 固定源码锚点 | 对本轮论证的用途 |
|---|---|---|
| S48 旧 API 的已记录顺序：submit COMMITTED 后返回 `local_output`，fetch 单独用于恢复 | `VFSS/src/moe_topk/score_to_mask_v3_party.cpp:522,821,853-854`（固定 S48 Git blob）及 S32/S42 决策/报告 | 区分旧接口合同与 S51 显式 v2 fetch-before-return 顺序，不判为旧合同缺陷 |
| 新 API/版本、输出状态、fetch callback | `VFSS/include/moe_topk/score_to_mask_v3_gmw.h:19,150-153,194-196,279-291` | 固定版本化接口、无 share 恢复状态与调用参数 |
| 形成 envelope、submit、提交状态处理、receipt 比较、recipient 解密、仅在成功后返回 fetched share | `VFSS/src/moe_topk/score_to_mask_v3_party.cpp:831-891` | 本方 transcript 的因果顺序与 API 释放点 |
| escrow READY/fetch/status 操作码 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:31-36,103-115` | 固定服务消息阶段和固定 READY 内容 |
| receipt/v2 token 来源、只在第二份 share 后创建及 COMMITTED 更新 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:245-262,315-428` | 理想 A 随机源接口和 token 一次生成/事务位置 |
| RSA key size API 与声明的 OpenSSL >=1.1.1 最低版本兼容 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:755` | 用 `EVP_PKEY_size` 替代在 OpenSSL 1.1.1f 编译失败的 `EVP_PKEY_get_size`；仅为构建兼容，不是加密安全论据 |
| recipient-only fetch、response 序列化、restart pending 处理、认证服务处理 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:440-483,534-536,563-619` | 逐字段映射 A 状态机和本方可见响应；restart pending 属故障范围，不纳入主定理 |
| 请求 READY/submit/fetch 客户端消息处理 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:642-702,853-884` | 双方各自的 A 连接及 response parser |
| S51 正常、标准 fetch 异常、非标准 submit/fetch 异常驱动，重启取回与 oracle | `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:868-912,999-1230,1280-1465,1935-1975` | 四个受影响路径的功能证据；TEST_ONLY failpoint 不当成自然故障 |

行号以本轮 source manifest 对应的 S51 文件字节为准；修复前行号专指固定 S48 Git commit，不是主工作区文件。

## 5. Kpost_3 构造与联合 law

### 5.1 因果 sampler

对 `b∈{0,1}`，对象是三个独立执行部件的交互组合
`Sim_b || HonestEndpoint_(1-b) || A`，不是给 `Sim_b` 添加一个 peer-success oracle：

- 理想 `F_T/v3` 只把 `pkg_b` 交给本方、`pkg_(1-b)` 交给诚实方、`ctx_A` 交给 A；理想 `F_TOPK` 分别把 `z_b` 和 `z_(1-b)` 交给对应 party endpoint。每个 endpoint 只持有自己的输入/输出/package/凭据。
- `Sim_b` 只负责腐化端的本方消息并接收该端真实 API 可见的响应；它没有调用 `HonestEndpoint` 私有状态、读取其输入/输出或替它构造第二份 deposit 的接口。
- `HonestEndpoint_(1-b)` 是理想执行环境中的诚实 party 程序，使用自己的理想输出与 package 执行同一份 API；A 从 `F_T/v3` 获得 context 后运行真实 escrow 状态机。双方调用成功与否由同一成功调度前提约束，而不是由模拟器强制“成功”。

在该交互接口下，后缀按源码因果顺序为：

```text
Kpost_3^b(q_b^3,Vpre_b^3; A, HonestEndpoint_(1-b), SuccessFence):
  1. 从 `q_b^3` 得到自己的 z_b、pkg_b、public tuple 和本方凭据；不读 peer/A/T 隐藏值。
  2. 按接收的实现格式和理想本方加密随机带生成 C_b=Enc(pk_b,z_b; r_b)，
     t=transaction_id(pkg_b)；party-local view 保留 C_b。
  3. 在本方认证 channel 上发 submit(t,b,C_b)。
  4. 诚实 party 程序独立从自己收到的 z_(1-b)、pkg_(1-b)、凭据和公开参数
     生成 C_(1-b)，通过其真实 authenticated endpoint 发第二个 deposit；该步骤由
     `HonestEndpoint` 自己执行，Kpost/Sim_b 不接收也不代造该 deposit。
  5. A 按真实状态机将 first deposit 写成 PENDING；second deposit 到达后，在
     同一 second-deposit SQL transaction 中持久保存第二个密文、COMMITTED 和
     唯一 U。若已给定固定成功调度，A 对本方 submit 回复 COMMITTED(t,U)。
  6. 本方另开 authenticated fetch(t,b)。A 返回本方 C_b 与已存的同一个 U；不返回
     peer ciphertext。Kpost 验证 receipt 相等并运行本方解密/分享比较。
  7. 返回完整本方应用消息 projection 和 `API_b=z_b`；P0/P1 分别执行同一个
     party-index 参数化算法。
```

模拟器实际可见 A 的本方 READY/response/fetched row；它不拿 context、peer ciphertext、peer share/package/credential、A DB 状态、T key 或 proof-only `J_b`。测试控制器虽能观测 DB，但它不是 party simulator 的输入。诚实 peer 路由来自 ideal `F_T/v3` 与 `F_TOPK` 给诚实 endpoint 的本方输出；peer 的第二份提交是消息因果，而非 `peer_deposit_success` oracle。

### 5.2 条件分布

令 `C_b` 表示腐化方本地生成的 recipient envelope，`U∈{0,1}^{256}` 是 A 在 COMMITTED 时的 receipt/v2 token。固定成功路径 data view 包括本方 submit/response 与 fetch/response。对于理想、独立且在 commit 时只采样一次的 A 随机带，有：

```text
Law(U | q_b^3, Vpre_b^3, C_b, ctx_A, C_(1-b), valid_deposits, commit)
  = Uniform({0,1}^256).
```

这条关系来自 ideal A 随机带独立且 token 在两份有效 deposit 已验证后才采样；不是由 SHA-256 碰撞抗性推出。它是联合条件 law，不是仅有边缘分布陈述。`ctx_A` 和 `C_(1-b)` 出现在数学条件中用于说明独立性，却不被交给 `Sim_b`。

给定 `U` 后，真实后处理是确定的：

```text
R_submit,b = Encode(COMMITTED, t, no-ciphertext, U)
R_fetch,b  = Encode(COMMITTED, t, C_b, U)
API_b      = Dec(sk_b, C_b, pkg_b) = z_b
```

协议每条 TLS connection 的 READY 固定，request/response frame lengths由公开 tuple、本方 envelope 长度及固定 token 长度确定。模拟器先抽一个 U，再把同一个 U 用于提交响应、fetch 响应及重复 fetch，不独立重抽。A 内部的 `C_(1-b)`、哈希、package-set/context、PENDING/COMMITTED、第二份到达顺序和服务端时间是 `L_A` 内部状态，不作为 party simulator input。于是针对固定 `q_b^3,Vpre_b^3,C_b` 且 successful completion 被当成给定成功执行前提时，应用数据后缀的模拟是精确的；不存在首个不能采样的 peer ciphertext/receipt/fetch 值。

本方自己的加密不需要 IND-CPA 替换：`z_b` 与本方公钥可用，模拟器运行同一个随机化 envelope 算法即可；fetched 是 A 原样返回之前存储的 `C_b`。peer ciphertext 不在 party-visible projection，故本引理不以它做 IND-CPA hybrid。真实 RSA-OAEP/AES-GCM 的优势仍需单独证明，不能从这里推导。

### 5.3 成功条件与 Kpre_3 组合限制

S50 证明：

```text
Law(Vpre_b^3 | q_b^3, cap_success) = Kpre_3^b(q_b^3).
```

S51 后缀结论给出：

```text
Law(Vpost_b^3 | q_b^3, Vpre_b^3, cap_success, SuccessFence)
  = Kpost_3^b(q_b^3,Vpre_b^3; A, HonestEndpoint, SuccessFence).
```

若 `SuccessFence` 是证明环境给定的“双方合法 authenticated deposit 均被调度为成功，deadline 不触发”的非选择性前提，则可按核的链式分解合成 `Kpre_3^b ; Kpost_3^b`，得到限定的成功应用消息 view。若定理改指从真实运行中事后筛出“刚好在 timeout 之前成功”的样本，则还需证明或构造：

```text
Law(Vpre_b^3 | q_b^3, cap_success, Success_escrow)
  = Law(Vpre_b^3 | q_b^3, cap_success)
```

或给出其等价高效条件 sampler。S50 没有证明该等式；真实活动 Select 的比较数量、处理/认证时间、调度与 A 的 finite timeout 可能共同影响 `Success_escrow`。本轮不把 `SuccessFence` 偷换成此等式，也不把超时状态加进 `L_S29`。因此 success-message suffix 可通过，真实超时成功筛选下的整条联合视图仍为 `CONDITIONAL`。本轮不论证 crash/abort-prefix 隐私。

## 6. P0/P1、泄露字段及 S42 迁移

| 变量/字段 | P0/P1 对应生成 | 本方是否可见 | 泄露/模拟归属 |
|---|---|---:|---|
| `pkg_b`, `z_b`, `Vpre_b` | 同次理想 FTv3/F_TOPK + S50 Kpre3 | 是 | Kpre3 输入/view，S50 范围 |
| 本方 `C_b` | 本方公钥加密自己的 z，随机带在本方 view | 是，发送与 fetch 都见 | view 而不是新 L；由已知 z/公钥重算 |
| peer `pkg`, `z`, TLS/RSA credentials, `C_(1-b)` | ideal router 交给 honest endpoint 生成与提交 | 否 | endpoint 与 A 内部；不传给 Sim_b |
| `t` | 同一 session/invocation 的确定 id | 是 | 公开参数/绑定确定后处理 |
| `READY` | 每次 TLS service connection 固定 7-byte frame | 是 | 固定消息 DAG；不增加泄露 |
| `PENDING`, A ctx/package-set、双方 hash/长度/arrival order | A DB / service 内状态 | 否（除本方等候后响应） | `L_A`/可信服务内部；不并入 `L_S29` |
| `COMMITTED` 与 receipt token | 双份认证 deposit 成功后固定状态；U 一次均匀抽样 | 是 | 成功 transcript 由 Kpost 采样；通信长度已有类别，U 不塞入 L |
| fetch 本方 ciphertext/token | A 按本方 identity 返回自己先前的 C_b、同一个 U | 是 | 确定性服务响应；不含 peer ciphertext |
| API share 与时间 | 成功时返回 Dec(C_b)=z_b；两边返回时刻可能不同 | share 是；物理时间可观察 | share 是 F_TOPK 已有 output；wall-clock model OPEN，不当作 S29 泄露已批准 |

P0/P1 的状态机同构：party id、证书、package hash、recipient key、监听端口、加密随机带均按 b 置换；服务器对每一份只验证自己绑定的 identity并且 fetch 只返回该 party row。若 P0 先到，则 P0 的服务线程等待，P1 的第二次 deposit 使 A commit；反之交换 P0/P1。A arrival order/时间属于 `L_A`；数据投影的 `COMMITTED(t,U)` 和 `fetch(C_b,U)` 不依赖到达顺序。两方 API 不保证物理同时返回。

S42 可复用的是抽象数据后缀结构：本方加密、独立生成的一次 token、诚实 peer endpoint 因果 deposit、A 状态机 commit、recipient-only fetch 与确定后处理。不可直接迁移的是原实现接口事实和 FTv1 package/context 绑定；S51 以 S48 FTv3 context format 2、package format 6、receipt/v2 及本轮显式 API v2 顺序重新映射。S42 不曾证明旧 S48 API 在返回前执行 fetch，本报告不将这一行为归给 S42。

## 7. 优势项、失败范围与独立复核

在上述理想成功模型中，S50 `Kpre_3` 与本轮 `Kpost_3` 的数据 transcript sampler 是 exact law；后续向真实系统的证明必须分别保留符号项，不能将它们折叠成未经证明的数值：

```text
Adv_total ≤ Adv_FT/Dealer + Adv_DRBG^T(Qmax,Cmax;E_T)
          + Adv_party_rng(Q_party,C_party;E_P)
          + Adv_A_token_rng(32 bytes;E_A)
          + Adv_signature + Adv_TLS/mTLS
          + Adv_RSA-OAEP + Adv_AES-GCM + Adv_hash/context-binding
          + Adv_success-selection/timing.
```

`S49 δ_reject≤1023·2^-154<2^-144` 仍只适用于理想 IID、cap_success、剥除版本/签名/编码的语义材料投影；S51 不把它加到完整 package、signed transcript 或完整线上 view 距离中。未获得目标部署的 provider、熵源和生命周期证据；此处所有真实优势未量化。

`/root/s51_independent_review` 未参与 S47–S51 实现或证明撰写；独立重推了 S29 来源/字段映射、q/Vpre 条件 law、诚实 peer 路由、A PENDING→COMMITTED/token/fetch、P0/P1 对称性和 timeout selection 限制，并指出非标准 callback 异常边界。Reviewer 只读，没有构建或运行测试；构建和 E2E 由本轮执行者生成，不称为独立功能验收。审查者还确认 `EVP_PKEY_size` 是 OpenSSL 1.1.1-compatible 拼写，属于构建兼容性而非密码学证明；最终核对也发现并要求修正 manifest 中 party.cpp 的一项陈旧 raw hash。

## 8. 运行证据与未运行项

- 最终执行环境：WSL2 Ubuntu 20.04；GCC 9.4.0、CMake 3.31.6、Unix Makefiles、OpenSSL 1.1.1f；系统有 SQLite runtime 3.31.1，但无开发头文件/链接名。`libsqlite3-dev 3.31.1-4ubuntu0.7` 仅下载并解包到 worktree 外的 `s51-env/sqlite`，`.so` 链接指向系统 runtime；未安装系统包或改仓库外系统配置。构建产物存于隔离 worktree 的兄弟目录 `s51-env/build-ftv3`，未进入仓库。
- 构建尝试记录：第一次指向 `/usr/include` 配置，因无 `sqlite3.h` 失败；第二次用临时解包的 SQLite 开发包完成配置；首次编译在 `score_to_mask_v3_escrow.cpp:755` 因 OpenSSL 1.1.1f 不提供 `EVP_PKEY_get_size` 而失败，暴露该源码与 CMake 最低 OpenSSL 版本不兼容。将其替换为 1.1.1/3.x 兼容的 `EVP_PKEY_size` 后从 Release 目录重建成功。这项修正只证明 API/版本构建兼容，不证明 RSA 密码安全。
- 最终 fresh Release TEST_ONLY 配置：GMW/v3 ON、receipt/v2 ON、FTv3 bounded ON、FTv2 bounded OFF、`BUILD_TESTING=ON`；仅构建目标 `moe_topk_score_to_mask_v3_ft_v3_process_e2e_test`。最终二进制 SHA-256 `48355b3b3cae05ba4e8d4d9160a020fd8980c57652c9bf9fda41608b9a66dc3c`。
- 成功的 fresh configure/build 命令为：

```bash
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s51-review/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s51-review/s51-env/build-ftv3 -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=OFF -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL=ON -DSQLite3_INCLUDE_DIR=/mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s51-review/s51-env/sqlite/extracted/usr/include -DSQLite3_LIBRARY=/mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s51-review/s51-env/sqlite/extracted/usr/lib/x86_64-linux-gnu/libsqlite3.so
cmake --build /mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s51-review/s51-env/build-ftv3 --parallel 2 --target moe_topk_score_to_mask_v3_ft_v3_process_e2e_test
```

- 正常 FTv3/receipt-v2 E2E：N=128,K=64，Dealer/P0/P1 exit 0，escrow/restart service 以 SIGTERM 退出 143；两方 API `BOTH_COMMITTED`，oracle PASS，重启后双方 recipient-only fetch/re-fetch 及相同 token PASS，88 个 peer exchange phase，最大应用 payload 159,860 bytes。两方 RSS 约 23 MiB。均为功能/资源诊断，不是性能数据。
- 标准 fetch callback `std::runtime_error`：P0 exit 11、无 share、`COMMITTED_FETCH_REQUIRED` 并保留 tx/token，P1 exit 0；重启后 P0/P1 recipient fetch，token 一致，XOR 重构匹配 oracle，恰 64 个 1。
- 非标准 fetch callback `throw 23`：P0 exit 11、无 share、`COMMITTED_FETCH_REQUIRED` 且保留 tx/token；P1 exit 0；服务重启后两方取回成功，XOR 与 oracle 一致。非标准 submit callback 在 A 实际提交成功后 `throw 17`：P0 exit 11、`COMMIT_STATUS_UNKNOWN`、无 receipt/share；P1 exit 0；重启恢复两方 shares，token 与 XOR/oracle 校验通过。后两者验证 catch-all 映射，不是自然 I/O 故障。
- 未重跑 S48 的 13 案、S40/S35 矩阵或任何 N=1000 诊断；未单独运行 GMW conformance 或 oracle 全矩阵。四个新二进制 E2E（正常、标准 fetch 异常、非标准 submit、非标准 fetch）是本轮受影响目标。
- `BUILD/TEST` 在代码修复后为已执行上述精确范围；正式 LAN/WAN/RTT/AAV86/performance 未运行/未测。自然业务 I/O 故障沿用 `NOT_REPRODUCED`。目标真实 DRBG 部署 profile、真实 TLS/recipient-encryption cryptographic bridge、crash-prefix、physical power-loss、A-only corruption、N>256 security proof 均不在本轮已证范围。

## 9. 门状态、可写声明与下一步

| 门 | 状态 | 范围 |
|---|---|---|
| S47–S50 fixed source receipt | `PASS_WITH_SCOPE` | commit/blob/manifest 核验，详见 manifest；S29 来源限定另列 |
| 原始 M6B `L_S29` provenance | `PASS_WITH_SCOPE` | Git snapshot 复原并核对 S36/S42；snapshot 不是正式 S29 milestone，S29 全视图门仍 OPEN |
| S50 `Kpre_3` | `INHERITED_PASS_WITH_SCOPE` | ideal IID FTv3，cap_success，N≤256，成功 GMW pre-escrow view；独立复核继承 |
| v2 API fetch-before-return ordering | `PASS_WITH_SCOPE` | S51 显式版本化的 API 顺序；四条受影响 FTv3/receipt-v2 process E2E；不追溯为旧 API 缺陷 |
| callback non-standard exception handling | `PASS_WITH_SCOPE` | catch-all 修正；整数异常的 submit/fetch 两条独立进程用例，无 share 并可恢复；其他异常后缀仍不穷尽 |
| OpenSSL minimum API build compatibility | `PASS_WITH_SCOPE` | 在 OpenSSL 1.1.1f 上的 fresh Release build；`EVP_PKEY_size` 编译兼容只属于构建证据 |
| `Kpost_3` ideal application message view | `PASS_WITH_SCOPE` | 固定本方 pre-view，ideal A token RNG，诚实 endpoint 因果 deposit，双方在 timeout 前成功，N≤256 |
| `Kpre_3+Kpost_3` conditioned on ex-post runtime success | `CONDITIONAL` | 还需证明成功/timeout 筛选不偏置 `Vpre_b`，或给其条件 sampler；成功调度必须作为外生前提才可精确组合 |
| wall-clock timing / timeout-selection view | `OPEN` | party可观察提交等待时间；未扩张 L；缺 deployment/scheduler条件分布 |
| ideal A token local law | `PASS_WITH_SCOPE` | 沿用 S39/S40 的独立理想 A RNG 条件均匀结论；真实 RAND_bytes 未桥接 |
| real Dealer / FTv3 DRBG / signatures | `OPEN` | 真实 joint distribution 与优势项未闭合 |
| TLS/mTLS / recipient crypto / binding | `OPEN` | 只复核代码路径与 E2E 功能，未证明/量化优势 |
| complete GMW/Select/shuffle/output/escrow party-view | `OPEN` | 本轮只到成功应用 API 数据后缀，完整 composition 未通过 |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` | 未有新负责人接受记录 |
| natural business I/O fault | `NOT_REPRODUCED` | 本轮只有一个明确人工 post-commit fetch failpoint |
| `N>256` security | `NOT_IN_SCOPE` | S48 的 N≤1000 13个功能案例只为历史功能收据 |
| build/test | `PASS_WITH_SCOPE` | 只有 fresh release FTv3 target 与四案；非全仓/非全矩阵 |
| formal LAN/WAN, RTT, AAV86, performance | `NOT_RUN` / `NOT_MEASURED` | 后置独立门 |
| secure alias / push / merge | 未开放 / `NOT_PERFORMED` | 未创建、未推送、未合并 |

可写：**“项目自有五迭代稳定 Select 在 S48 列明的 N≤1000 有限功能用例中实现并匹配冻结 oracle；S51 另独立执行 N=128,K=64 的 receipt/v2 fetch-before-return 正常与人工恢复案例。”**这不说明全输入正确性、N≤1000 安全证明或性能通过。

安全门未关闭的最短后续工作：先由负责人接受/拒绝低概率 `ISSUANCE_ABORT` 合同；定义 timeout 的安全抽象（成功为外生保证，或将 time/success-selection 形式化并证明/模拟）；再把 `Kpost_3` 绑定到真实 T/RNG/TLS/recipient crypto 与完整静态半诚实 GMW/Select/shuffle/output/escrow hybrid。若选择对真实成功运行做条件化，就需要直接给出上式中的 `Law(Vpre_b^3 | q_b^3,cap_success,Success_escrow)` sampler 或证明成功选择不依赖隐藏前缀；不能把时间字段增列到 `L_S29` 来绕过。

唯一未确认产品问题：**合法输入下，离线材料准备若以极低概率达到拒绝采样上限并返回发材中止、本次调用作废且需用新调用重新准备，是否可接受？**不重复询问已确认的 escrow/正常输出合同。
