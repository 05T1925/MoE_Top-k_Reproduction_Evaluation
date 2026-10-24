# M6B-I-PROJECT-SELECT-5ITER-S51 决策

日期：2026-10-24
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定

S50 的 `Kpre_3` 仍限于 ideal IID `F_T/v3`、`cap_success`、`1≤K≤N≤256`、一方静态半诚实、无故障并止于本方 output share。S51 从该前缀定义 `Kpost_3`：在同一次调用中，由诚实 endpoint 用自己的 F_T/v3 package、F_TOPK output share 和认证凭据实际向 A 提交第二份密文；A 的真实状态机从 PENDING 进入 COMMITTED 并保存同一 receipt/v2 token；腐化方随后 authenticated fetch 并解密自己的密文，API 才释放 share。

S48 固定源码中的旧 `score_to_mask_v3_execute_party` 在 COMMITTED submit response 后返回本地 share，fetch/decrypt 则由独立恢复 API 提供。S31/S32/S42 记录允许该提交返回顺序，并未规定正常 API 必须等待 fetch；因此不将旧路径裁定为违反既有合同。为落实本任务要求审查的“fetch 后再由 API 返回”视图，S51 增加显式 `score_to_mask_v3_execute_party_v2`：要求 fetch callback 与本方 recipient private key；仅在 fetched receipt 与 submit receipt 相同、密文成功解密且与本地计算 share 相等时返回 `kCommitted`。这是版本化 API 顺序选择，不改写旧合同或泄露。代码审查另发现该新 API 对非 `std::exception` callback 异常未保留恢复状态；S51 将 submit/fetch 边界改为 catch-all，并用人工抛出整数的 E2E 定向验证 `COMMIT_STATUS_UNKNOWN` 或 `COMMITTED_FETCH_REQUIRED` 且不返回 share。这个窄异常处理缺陷与旧 API 顺序选择分别记录。

在“诚实 endpoint 按因果提交且成功调度为执行前提”的抽象应用消息视图中，S51 给出 P0/P1 对称的 `Kpost_3` 条件采样器。receipt/v2 token 在 ideal A 随机源下于 COMMITTED 事务生成一次，持久化并在 submit response、fetch response 和重取中复用；party simulator 只接触本方消息，不接收 peer package/share/credential 或 A context。该后缀结论不包含 wall-clock 侧信道，也不证明对真实计时/超时成功条件化后的前缀分布不发生选择偏差。因此 `KPOST_3_MESSAGE_VIEW=PASS_WITH_SCOPE`，而组合到真实超时筛选分布的 `KPRE3_KPOST3_COMPOSITION=CONDITIONAL`。

## 来源、复核和验证

- 从最新 main `c3926c68fd14f270faa8b55234311071947fa080` 的后代 S50 `ff77b9694a1259c3f314f145e06fcb980cda0bfb` 建隔离分支；S47–S50 固定链、S50 的 11 个源码 Git blobs 和 S48/S49 对应清单已核对。精确值见 S51 source/evidence manifest。
- M6B 的 `L_S29` 原始报告从 Git snapshot `502c9cc68a23b06b3347d34d60e679d98d3f60a4` 恢复，report Git blob `760962e1cdaba1f8459d415da5e3f7b1f6cbe934`，SHA-256 `aa8dde4bde549c59bffa4ce2f2d2343e52d53912f734d7cc2134c6e15cccf717`。这是记录在 S25 candidate manifest 中的 worktree snapshot，不是 S29 milestone commit；manifest 明确 `candidate_committed=false`。S36/S42 引用与该 M6B S29 §3.3 一致。其六类既有泄露保持不变，S29 全视图安全门当时仍 OPEN/NO_GO。
- 执行者参与过 S35–S50 的历史实现、证明或决策。`/root/s51_independent_review` 未参与 S47–S51 实现/推导；复核了 L 来源及范围、条件后缀、诚实对端因果 deposit、A 状态机/token/fetch、P0/P1、API 修正和本报告限定。复核者只读，没有构建或测试；这不是独立功能接收。
- 因增加显式 fetch-before-return API 并修复其 callback 异常边界，运行 fresh Release FTv3/receipt-v2 独立进程 E2E 四案（N=128,K=64）：正常成功、标准异常 fetch、非标准 submit 异常、非标准 fetch 异常。只把非标准异常逸出/破坏恢复状态记为可复现代码缺陷；整数异常均映射到无 share 的未知提交或待恢复状态，服务重启取回与 oracle 通过。反事实 pre-fix 二进制 `b85d121594d9070162f2226785e08048ada3a40137eef74e07ad0617c6ca8d48` 的非标准 submit 注入稳定观察到 P0=134/P1=0、E2E exit 1；最终二进制 `48355b3b3cae05ba4e8d4d9160a020fd8980c57652c9bf9fda41608b9a66dc3c` 四案均 exit 0。OpenSSL 1.1.1f 还复现了最低版本 API 的编译不兼容，改用 `EVP_PKEY_size` 后构建通过；这是单独的兼容性修正，不是协议安全缺陷。注入不是自然 I/O 故障。具体命令、环境、退出码与结果见 case list 和 manifest。

## 门禁与限制

`L_S29_SOURCE=PASS_WITH_SCOPE`；`KPRE_3_N_LE_256=INHERITED_PASS_WITH_SCOPE`；`KPOST_3_MESSAGE_VIEW=PASS_WITH_SCOPE`；`KPRE3_KPOST3_COMPOSITION=CONDITIONAL`（要求 successful schedule 为外生执行前提，或另证成功/超时事件在给定本方条件视图下不产生选择偏差）；`WALL_CLOCK_TIMING_VIEW=OPEN`；真实 Dealer/DRBG、A token RNG、签名、TLS/mTLS、recipient encryption 和完整 party-view 组合均 OPEN。`LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`。自然业务 I/O 故障为 `NOT_REPRODUCED`。N>256 安全扩域 `NOT_IN_SCOPE`；S48 N≤1000 只继承其有限功能案例。正式 LAN/WAN、RTT、AAV86 和性能矩阵 `NOT_RUN/NOT_MEASURED`。不创建 `secure` alias，不推送或合并。

详细证明范围、条件分布、源码映射与输出合同见 [S51 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S51_REPORT_2026-10-24.md)；机器门禁与逐案证据见同目录 JSON。
