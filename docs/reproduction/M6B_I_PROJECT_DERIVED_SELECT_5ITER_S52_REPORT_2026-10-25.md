# M6B-I-PROJECT-SELECT-5ITER-S52 详细复核报告

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 执行摘要

本项目实现的是 Protocol I 上项目自有的五迭代稳定 Select。它借鉴 BB90 研究思路，不是 BB90 原算法、BMW16 或 BB90+DCF 复现。功能输入按 signed Q20.12 解释，分数降序、同分按原索引升序，输出原始顺序、恰有 K 个 1 的 Top-K XOR mask shares。托管服务仅保存密文；双方密文共同提交后各取回自己的份额，两个 API 返回时间可以不同。

S52 在 S51 精确提交上重建 default-OFF 与显式 FTv3/receipt-v2/API-v2 Release 配置，新增并运行七个状态/恢复定向案；又在最终测试二进制上顺序运行 N=1000 的五个 K 值和 N=1/128/256 三个兼容 smoke。目标案例均完成 oracle 与提交/取回检查。独立复核指出并帮助定位 escrow ABORT 与未知状态边界；修复后对应定向用例和所有列明功能用例复跑通过。

最重要的安全裁决仍为 `CONDITIONAL/OPEN`：S51 的 `Kpre_3/Kpost_3` 只在环境外生保证合法消息于截止前送达的理想 `SuccessFence` 下组合。真实 API 与 escrow 存在有限 timeout，但没有证明成功样本对 pre-escrow 视图无条件选择偏差，也没有真实环境的成功条件 sampler。S52 的功能和状态修复不关闭这个证明门。

## 2. 来源、工作区与独立性

- 固定基线：S51 commit `1cc1e5202b3a603b55aaae985704933d70db0794`，parent `ff77b9694a1259c3f314f145e06fcb980cda0bfb`，tree `6b1ce4bbcd6a962edeeaa8a5b862dfd15e786021`。S47→S50 ancestry 与 manifest 来源按 S51 固定记录；S51 manifest 的 10 个 Git blob 在该 commit 中逐项复核为 10/10。仓库中四个 S51 文档经 Windows checkout 后有 CRLF 转换；核对时保留 Git blob 身份，并对文本 raw/规范化哈希分别记录，不拿当前已修改运行时代码冒充 S51 源码。
- worktree：`C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s52-receiver/MoE_Top-k_Reproduction_Evaluation`；branch `codex/m6b-i-project-select5-s52-receiver`。开工时干净，基线准确。原主工作区和 S51 历史工作树未动；`VFSS-baseline/` 差异为空。
- 当前执行者参与了此前 S51 工作链，因此不把自己描述为 S51 的非作者。`/root/s51_independent_review` 未参与 S51/S52 runtime 实现或本轮推导。它先只读复核状态分类与源码，再从固定 S51 base 加入 manifest 中四个 S52 源码文件，在仓库外独立目录进行 fresh Release TEST_ONLY 构建并运行 S52 定向 API-v2 与 N=1000 selector。独立 E2E 二进制 SHA-256 与本轮最终 E2E 二进制完全一致；独立 7 个 API 案和 8 个 N=1000/兼容案均 exit 0，oracle/状态结果与逐案 JSON 的 15 案核心字段一致。独立 API 和 N=1000 日志 SHA-256 分别为 `5ab38ce9ce8edfa5ff5f5810dce73d06ef74ae49fce0c7b18e7fdb349c9bda59`、`402d3f05b3a60875b1c21a4dedaded54d5a6cce2c01469657c635db58093af79`。独立接收者未修改仓库文件；初次使用 Windows 挂载路径作为 WSL `TMPDIR` 的运行不能进行测试，改用默认 WSL `/tmp` 后通过。它重新推导了 S52 SuccessFence/实际有限 timeout 的断点，但没有重证 S50/S51 继承子引理，也没有运行密码学实验。该功能接收与理论范围复核分开记录，均不等于完整安全接收。
- S51 历史报告/gates/manifest 保持不变。S48 的 13 个 N≤1000 功能案例只作为继承收据，不改写为 S52 新跑结果。

## 3. 源码审查、缺陷与最小修复

### 3.1 缺陷复现与原因

S51 的 `query_state()` 在 SQLite status 查询无行时返回内部状态 2（ABORT）；服务端 submit 响应分支又将所有非 COMMITTED 状态作为 ABORT 返回。缺失交易行、数据库状态异常与已持久化的 ABORT 因此会走同一 wire status。S51 客户端还将任何 submit callback 异常归为状态未知，不能区分经认证的明确 ABORT 与“请求可能已提交但响应丢失”。这既模糊了交付状态合同，也使测试无法把真实持久化 ABORT 与记录缺失区分。

S52 的 `--s52-ft-v3-api-v2-targeted` 中有两个明确注入：

1. 服务端等待第二个 deposit 超时，A 已将交易持久化为 ABORTED；最终客户端在核验 TLS/帧、响应长度、transaction ID、全零 receipt、空 ciphertext 后只将 P0 标为 `ESCROW_ABORTED/kAborted`。P1 被测试控制器安排在提交前退出，故 P1 为 `COMMIT_STATUS_UNKNOWN/kRecoveryRequired`。双方均无 share，A 的重启状态为 `ABORTED_NO_FETCH`。
2. P0 密文进入 PENDING 后，TEST_ONLY 控制器在单独数据库事务中删除该 transaction 和 share 行；服务端查询“无行”后关闭请求，不发送 ABORT。P0 的客户端视图为未知/无 share；P1 是被控制为不提交的对端，也无 share。A 重启 fetch 均拒绝。它是数据库缺行注入，不是自然 IO 错误或攻击。

修复位置与语义：

- [score_to_mask_v3_escrow.cpp](../../VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp)：缺行和 state 不属于 `{PENDING, COMMITTED, ABORTED}` 时抛错；服务端仅对持久化 ABORTED 发送 ABORT，未到 terminal state 时关闭请求；客户端仅在已认证 TLS 会话中完成完整响应、事务 ID、空密文和零 receipt 校验后设置 `definite_abort`。
- [score_to_mask_v3_gmw.h](../../VFSS/include/moe_topk/score_to_mask_v3_gmw.h)：新增不可由普通 callback 伪造的 `ScoreToMaskV3DefiniteEscrowAbort` 私有构造异常，只授权内置 submit 函数构造。
- [score_to_mask_v3_party.cpp](../../VFSS/src/moe_topk/score_to_mask_v3_party.cpp)：收到上述明确异常时返回 `kAborted/ESCROW_ABORTED`，没有 share；其余 submit 异常保留 `COMMIT_STATUS_UNKNOWN/kRecoveryRequired`；COMMITTED 后 fetch 异常保留 receipt 与 transaction，返回 `COMMITTED_FETCH_REQUIRED` 且不返回 share。
- [score_to_mask_v3_process_e2e_test.cpp](../../VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp)：加入 7 案驱动和缺失状态行控制器；所有注入清楚标记为 TEST_ONLY。

这是实现状态分类错误，不是密码学攻击。修复未改变排序、输出、角色、泄露合同、材料格式或托管成功路径。没有测试任意无效枚举值，只由源码检查确认无效值 fail-closed；该细项仍是未覆盖边界。

### 3.2 API v2 调用及返回合同

仓内 `score_to_mask_v3_execute_party_v2` 使用点位于公开头文件、实现文件和 TEST_ONLY E2E；未发现其他仓内消费者。正常顺序为：本地计算份额→加密并 authenticated submit→收到 COMMITTED token→authenticated fetch 本方密文→token 匹配→本方私钥解密→与本地暂存份额匹配→API 返回本方 share。提交异常状态不明时不返回 share；已知 commit 后 fetch 失败时保留恢复信息但不返回 share；只有完成本方取回和校验后返回 share。代码不承诺清零此前进程内暂存值。

外部消费者和已发布 ABI 兼容未验证（`NOT_RUN`）；没有增加旧接口降级包装。

## 4. fresh Release 构建与功能证据

### 4.1 构建环境与隔离

工作区外部构建目录：`C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s52-receiver/s52-env/`。执行环境为 WSL2 Ubuntu 20.04、GCC 9.4.0、CMake 3.31.6、OpenSSL 1.1.1f、SQLite 3.31.1 runtime；SQLite 开发头临时放在该外部目录。构建和临时数据未进入仓库。

- `build-off`：Release，`BUILD_TESTING=OFF`；GMW、receipt-v2、FTv2、FTv3、Select5 独立实验选项均关闭。`cmake --build ... --target help` 无 v3/Select5 实验目标；`libsytorch.a` 中未发现 `score_to_mask_v3` 或 `project_derived_select5` 符号。未定义 opt-in macro 的头文件编译探针按 `#error "score-to-mask-v3 is opt-in and must be built explicitly"` 预期拒绝。
- `build-on-prod`：Release，`BUILD_TESTING=OFF`；`MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON`、`...RECEIPT_V2_EXPERIMENTAL=ON`、`...FT_V3_BOUNDED_EXPERIMENTAL=ON`，FTv2 和 TEST_ONLY 关闭。库导出 `score_to_mask_v3_execute_party_v2`，`nm` 未发现 `test_only`/`with_test_random` hook。
- `build-on-test`：Release，`BUILD_TESTING=ON`，用于 conformance 和进程 E2E；测试 hook 只在该 TEST_ONLY 构建中启用。不可把此测试库当 production library。

构建证据 SHA-256：default-OFF `libsytorch.a` `8fd38ea2f5b8f93d66546514f6afc40b9431ec25b828568be8c8333628bcf09c`；opt-in production `libsytorch.a` `496769ed250c2a380ed49c1f9c7434717c8fde3d8e92daeecc54780fc4dd10d63`；TEST_ONLY library `6b5bdb4ef9a0761b194100b61f6c37929469971857df809f3eb6d3cafcedebdc`。最终 E2E 可执行文件 SHA-256 `81a5f05e6a9c75d85ea19c2e59729cd2cc0c9e724b68fae77c0c4333668043b9`。所有构建来自 S52 修复后源码。

### 4.2 实际运行

命令及结果：

```text
moe_topk_score_to_mask_v3_runtime_conformance_test
  exit 0: package signature, triple shares, permutation shares,
          one-shot lease, package faults PASS
moe_topk_score_to_mask_v3_ft_v3_bounded_conformance_test
  exit 0: gates=88,626,688; Qmax=88,786,404; Cmax=7,247;
          issuance_abort PASS
moe_topk_score_to_mask_v3_process_e2e_test --s52-ft-v3-api-v2-targeted
  exit 0: S52_FT_V3_API_V2_TARGETED PASS cases=7
moe_topk_score_to_mask_v3_process_e2e_test --ft-v3-n1000
  exit 0: S47_FT_V3_TARGETED_CASES PASS cases=8
```

逐案 JSON 保存了每方退出码、API 状态码、share 长度、A 恢复状态、oracle、N/K、package 版本、gate 数、phase/frame、payload 和 RSS。为避免把会话标识、package 摘要、密钥、材料或完整输入序列带入仓库，案例文件只保留测试分类和核验字段；完整日志及构建物留在 worktree 外部 `s52-env`，不提交。

**7 个 API-v2/A 状态案（N=128,K=64）**：正常提交、丢失 commit response、标准 fetch callback 异常、非标准 submit throw、非标准 fetch throw、A transaction deadline 后明确 ABORT、PENDING 状态行删除。正常案双方 share 长度 128、oracle 匹配、重启取回通过。两类 callback submit 不确定案均无 P0 share，P1 成功且之后恢复；fetch 异常案 P0 无 share、保留 recovery 信息，重启取回通过。deadline 案 P0 明确 aborted、P1 状态未知、双方无 share、DB 为 `ABORTED_NO_FETCH`。缺行案双方都 unknown/no-share，DB 无记录，之后 fetch 拒绝。

**8 个目标规模/兼容案**：N=1000、K={1,8,500,999,1000} 共 5 案；N=1/K=1、N=128/K=64、N=256/K=128 共 3 案。所有运行由最终 API-v2 二进制执行，dealer 和两方进程成功退出，A COMMITTED，两方 API 仅取回本方份额，输出长度为 N 且与冻结 oracle 一致，restart recovery 通过。输入是确定性的 signed int32 边界/负正值/重复分数模式，包含大量稳定 tie；不是随机全域覆盖或穷尽证明。

N=1000 本轮资源诊断：88,626,688 gates；每方材料 payload 33,366,080 B、package 33,366,516 B；最大 frame payload 1,048,572 B（外层 frame header 另计）；phase 数按 K 为 99、106、106、102、106。两方 RSS（KiB）按 K：1=`940428/940596`，8=`940432/940428`，500=`972172/972172`，999=`940416/940432`，1000=`940688/940444`。独立接收者重复执行时，K=500 的两方 aggregate RSS 为 1,947,464 KiB。以上均为本机功能/资源诊断，不是时间性能或 RTT 结果。

服务端/重启服务 `exit=143` 是 E2E 控制器在用例结束后的受控停止，不计为算法失败。超时及删行用例是人工故障/状态注入，不能写作自然随机崩溃。自然业务读写故障本轮 `NOT_REPRODUCED`。

### 4.3 失败尝试记录

最初一次状态查询将相对路径用于仓库根而非隔离 worktree 父目录；后续 OFF 符号命令因 PowerShell 在 WSL 命令外层展开变量而没访问构建目录。这两次是命令装配错误，没有执行测试，也不算代码失败。改用绝对 WSL 路径后 conformance、符号检查、头文件 OFF probe 均按预期完成。独立接收者第一次尝试将 Windows 挂载路径设为 WSL 临时目录，不能用于执行目标测试；清除该设置、使用 WSL `/tmp` 后独立 API-v2 与 N=1000 selector 均成功。S51 runtime 的预修复日志直接显示 timeout 案 P0/P1 都被归为 `COMMIT_STATUS_UNKNOWN`，尽管 A 已有 `ABORTED_NO_FETCH`。missing-row 错误由 S51 固定源码的 `query_state`/响应分支确认，S52 用删行注入验证修复后的 fail-closed/no-share 行为；本轮未用预修复 runtime 运行该新注入。预修复和最终原始日志 SHA 及最终二进制身份见 manifest/cases。

## 5. 实际 timeout/状态可见性

| 阶段 | 源码边界 | 时间限制/处理 | party 可见结果 |
|---|---|---|---|
| Protocol-I peer framed channel | `protocol_i_transport.h:60-62`, `.cpp:184-198,337-375,378-445,480-533` | 默认 options 2000 ms；逐次 send/receive 建立 frame deadline，header 与 payload 共用该 frame deadline；TLS handshake/WANT_READ/WANT_WRITE 在同一帧 deadline 内推进；下一帧重新计时。调用者可传别的值，E2E worker 用 15000 ms；没有整个 Select 调用的统一 wall-clock deadline。 | timeout、EOF、frame/phase/identity error 以异常进入 API 的相应中止/恢复边界；时间长度本身不加入 `L_S29`。 |
| A 等第二份 deposit | `score_to_mask_v3_gmw.h:92`, `score_to_mask_v3_gmw.cpp:855-856,1408-1409`, `score_to_mask_v3_escrow.cpp:590-607` | signed context 默认 30000 ms，上限 60000 ms；PENDING 后每 20 ms 轮询；deadline 到达时对仍 PENDING 的事务执行 durable abort，再查状态。只有数据库已为 state 2 才发送 ABORT。 | 经 TLS pin、frame、transaction ID 和 ABORT 内容验证后 P0 `ESCROW_ABORTED`；P1 尚未提交时可能只得到本地未知/恢复态。 |
| A 接收 TLS/request/response frame | `score_to_mask_v3_escrow.cpp:572-586,696-717` | server 端 framed TLS channel 使用 context transaction timeout；client 端每个 frame 使用 60000 ms；TLS handshake 与该次 frame 操作共用 deadline。每次下一帧重置 deadline，不构成整个 RPC/global deadline。 | 未认证/超时/EOF/格式错/DB 缺行均不能产生确定 ABORT；客户端 `COMMIT_STATUS_UNKNOWN`。已 COMMITTED 后 fetch 失败为 `COMMITTED_FETCH_REQUIRED`。 |
| TCP/DNS connect | `score_to_mask_v3_escrow.cpp:512` `connect_tcp()` | 源码未设置单独的非阻塞 connect/DNS 总期限；frame deadline 从 channel I/O 开始计。 | 连接阶段 wall-clock 仍可能受平台/解析/网络影响；没有部署上界证据。 |

显式业务 transaction timeout 注入不是自然读写错误。状态 API 结果、进程是否获得 share、服务 DB 状态和服务物理退出时刻是不同观察量；无 share 不等于进程内从未形成局部临时 share。S52 不证明物理同时返回、任意 crash-prefix 隐私、数据库掉电耐久或崩溃后完整安全。

## 6. SuccessFence 与有限 timeout 的安全组合

S50/S51 前置结论可写成：对 `b∈{0,1}`，在 ideal IID `F_T/v3`、发材 `cap_success`、单方静态半诚实、A/T 不与腐化方合谋、诚实 peer 用自身材料和份额按真实因果发送第二份 deposit，并且环境外生承诺双方合法消息在截止前处理时，`Kpre_3` 的 pre-escrow message view 与 `Kpost_3` 的 authenticated submit/COMMITTED token/fetch/decrypt/API-return 消息后缀可以按既有条件 sampler 组合。S51 独立 reviewer 已复核该**理想消息视图**范围；S52 非作者 reviewer 重新推导并确认以下有限 timeout 断点，但没有重证 S50/S51 子引理。该理想组合只继承，不把本轮功能数据当成证明。

实际有限 timeout 运行还筛选了成功事件。令 `Q_b^3` 为本方获准理想输入/输出、本方 package/静态状态、公开参数和原 `L_S29`；`Vpre_b^3` 是本方完整 pre-escrow 前缀；`S_actual` 表示这次真实运行在两方 peer frames、A transaction deadline、submit response、COMMITTED fetch 等步骤都没有超时且完成 API。若要声称实际成功运行的完整视图可由 S50/S51 模拟，至少需要以下条件分布等式/计算不可区分版本，或直接给出权限正确的 sampler：

```text
Law(Vpre_b^3 | Q_b^3, cap_success, S_actual)
  ≈ Law(Vpre_b^3 | Q_b^3, cap_success).
```

现无可用部署 scheduler/network/OS/TLS 延迟联合模型，没有条件 sampler，也没有论证 deadline 成功与 `Vpre` 的本方处理进度/活动消息/隐藏状态条件独立。仅凭成功 E2E、单个字段边缘分布或理想 `SuccessFence` 均不能推出该等式。这里标 `OPEN`，不等于发现攻击或证明不可能。不得把耗时、timeout、active message shape 或状态加入 `L_S29` 来制造闭合。

最小可执行下一步有两条**证明/规格**路线：

1. 保持既有泄露合同，仅主张外生 `SuccessFence` 抽象中的成功消息视图，并将真实 timeout/墙钟筛选明确排除在安全定理之外；或
2. 如要主张真实有限 deadline 条件化视图，先定义部署可测的网络/调度和截止策略，再构造 `S_actual` 条件 sampler 或证明前述联合分布差距可忽略。若最终需要公开时间/状态作泄露，另作版本化合同并由负责人明确批准；本轮未添加该字段。

## 7. 门禁状态和未覆盖事项

- `S51_SOURCE_RECEIPT`、`DEFAULT_OFF_ISOLATION`、`API_V2_TARGETED_FUNCTIONAL_RECEIPT`、`N_LE_1000_LISTED_FUNCTIONAL_CASES`、`ESCROW_ABORT_UNKNOWN_STATE_CLASSIFICATION`：`PASS_WITH_SCOPE`。
- `IDEAL_SUCCESSFENCE_KPRE_KPOST_COMPOSITION`：继承 `PASS_WITH_SCOPE`，严格限 S50/S51 模型及应用消息视图。
- `REAL_FINITE_TIMEOUT_SUCCESS_CONDITIONED_VIEW`、`WALL_CLOCK_TIMING_SELECTION`：`OPEN`，首个断点为上一节条件 law。
- 真实 Dealer/DRBG、A token RNG、签名、TLS/mTLS、RSA-OAEP/AES-GCM recipient encryption、hash binding 与完整 Select/GMW/shuffle/output/escrow party-view：继续 `OPEN`。目标 provider、部署密钥生命周期和可量化优势项本轮未验证。
- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；N>256 安全 `NOT_IN_SCOPE`；自然业务 I/O 故障 `NOT_REPRODUCED`；外部 API/ABI `NOT_RUN`。
- 正式 LAN/WAN、RTT、AAV86 和性能矩阵 `NOT_RUN/NOT_MEASURED`；不创建 `secure` alias、不推送、不合并。

## 8. 可用声明与后续工作

可以说：**“项目自有五迭代稳定 Select 已在 S48 列明的 N≤1000 功能样例及 S52 新跑的 N=1000 五个 K 值中，完成冻结 oracle 对照；S52 另对 API-v2 提交/恢复状态运行七个定向独立进程案。”**这不是全输入正确性证明，也不是 S48 全矩阵重跑、密码学安全或性能通过。

阻止“完整安全论证完成”的门包括真实 timeout 成功条件化、真实离线 Dealer/DRBG、A token RNG、签名/材料发行绑定、TLS/mTLS、recipient encryption、完整单方 GMW/Select/shuffle/output/escrow composition，以及 N>256 的安全扩域。`ISSUANCE_ABORT` 合法输入低概率合同仍待负责人确认。N>256 资源/安全扩域、正式性能比较和 AAV86 对比应分别立项，继续遵守项目 V3→V4 性能前置门。

唯一待确认产品问题：**合法输入下，离线材料准备若以极低概率达到拒绝采样上限并返回发材中止、本次调用作废且需用新 invocation 重试，是否可接受？**不重复询问已确认的托管与正常交付合同。
