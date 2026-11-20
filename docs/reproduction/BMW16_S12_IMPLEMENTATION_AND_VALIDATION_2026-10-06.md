# BMW16 S12：T/P0/P1 C++ EXPERIMENTAL 组合实施与验证

- 日期：2026-10-06
- worktree：`C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s12\MoE_Top-k_Reproduction_Evaluation`
- 分支：`codex/m6b-i-bmw16-s12`
- 接收基线：S11 `43c3c657fb370e73588b161f552b0012b0716d6c`
- 本轮身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 结果：**完整 VFSS C++ 三进程功能路径已运行；party-view 安全门未通过；不具备 secure 身份或正式 V4 性能资格。**

## 1. 本轮做到了什么

是，本轮在独立 T、P0、P1 OS 进程中实际运行了 S11 的完整 VFSS C++ party API：正式 raw-score share adapter、真实 Protocol I shuffle、两个并行 S4-derived Select task、四层自适应比较、uCMP/DCF membership、同置换 inverse shuffle、原序 XOR mask shares，或双方一致的概率 abort。成功 mask 由仅供 TEST_ONLY 的父 harness 重构后与冻结 C++ oracle 逐位核对；party API 不返回 score、threshold、original index、比较位或完整 mask。

本实现名称固定为 `BMW16_S12_CPP_TP0P1_EXPERIMENTAL_TEST_ONLY`，CMake 开关默认 OFF。它是 S11 原 C++ party API 的可追溯扩展，没有在 Python runner 里控制 Select，也没有复制第二份生产选择算法。离线材料、party进程和 API 都还属于本机实验合同；未建立 secure API。

## 2. 基线和接收复核

main/origin/main 均为 `c3926c68fd14f270faa8b55234311071947fa080`。S11 是 main 的后代，本 worktree 从指定 S11 接收提交 `43c3c657fb370e73588b161f552b0012b0716d6c` 开始。S4 算法源提交 `04ed6f8a352ab2277434344094471dff82f83e4b`，`select4r.py` SHA-256 为 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。E20 `8986a40175f5f8eef56d765fef0cfa863e205985` 和 E21 `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4` 只作为接口/工程来源检查；PR #28 draft head `57e239ee820a4208ac606663d877803f0b692206` 未修改。

S11 哈希清单中的原 party API 源身份为：`experimental_bmw16_select_party.cpp` SHA-256 `69880a6867543d8f5fa0cbaedae3fabe768e31c8a5acb0e1d9474c4fa09d52f0`；相应头文件 `746b96573e74f2c12af72c0db705a2b482a8151cf86e14f5fc42bc2fcf4c625c`；S11 的旧线程测试 `7c5d33f31b67ef2e04bf592c410e9dda2a987a4a6fd65f202660771eeabbb34b`。这些历史 hash 保留，S12 修改身份见本报告末尾及工作树文件。

独立源码复核发现 S11 的控制流已在每轮打开结果前冻结两任务当轮边计划；同轮 canonical real pair 反向请求最多一个 key Eval 并缓存；不同轮、不同 task 和 membership 没有跨池复用；stable index 保留在秘密记录中；dummy 分类比较不接触真实 key；selected anonymous handle 不是 party API 返回值，但可由两方共享的 transcript 推导；membership 方向按严格全序实现 `R_i ≥ R_selected`；inverse route 用同一生成器产生的逆向材料；n=1 是明确恒等短路；bundle config 校验 `n/K/pad/index_bits/comparison_bits`。S12 外部 tape 逐层差分和三进程计数进一步核验了这些实现边界。S11 原始 8 success/1 injected-abort 是历史运行，不并入本轮 52/1 统计。

## 3. 外部共同随机 tape：逐层对比 S4 和 C++

`experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py` 为每条测试生成外部 `BMW16_S12_TAPE_V1` 文件。它含 `[task][R1/R3]` 四段、每段 1,024 个 64 位字。CTest C++ 实现读此文件，S4 Python comparator 读 C++ trace 导出的同一 word 序列；两边使用相同 multiply-high bounded mapping 与 pool replacement 抽样次序。比较内容包括：

- 两份任务的 R1 sample/pivots、R2 prefix/suffix/U/residual rank；
- R3 V/x/y/W/residual rank，R4 Reject/Accept 与最终 selected handle；
- R1–R4 两任务逐条边和比较位；
- SUCCESS / probability-abort / invalid-abort 状态类别。

外部 tape 共 39 条差分全部 PASS：n=2..8 的每个 K 共 35 条严格全序输入；n=64 的 K=1、8、32、63 四条输入覆盖重复值、严格次序和 `INT32_MIN/MAX`/重复值组合。另有 n=64 `zero/max/alternating` 三类词流及额外 tape 文件，报告见 `experiments/TEST_ONLY_BMW16_S12/evidence/`；这些属于确定性 TEST_ONLY trace，不是安全随机数测试。C++ 与 Python 的语言内 PRNG seed 不要求相同；同一 tape 文件 SHA-256 才是控制流比较依据。

`n=128` 不运行：S11 runtime 与 S12 bundle 当前将有界域明确限制为 `n≤64`，不是资源测试后缩小后伪报。扩大到 128 需重新审查材料容量、有限参数和 API 合同。本轮 E2E 不声称覆盖 n=128。

## 4. 逐方离线材料和执行隔离

新 bundle v1 中每一方只打包自己的 raw adapter、forward/inverse shuffle、两任务各四轮 Select 和 membership key。manifest 固定 slot ID、stage、task、round 和规范端点；SHA-256 digest 与 `session/fingerprint/party/n/K/padding/index_bits/comparison_bits/nonce/payload_length` 位于 AES-256-GCM AAD。bundle 文件经临时写、`fdatasync`、rename、目录 `fsync` 原子落盘。party 在 key Eval 前先持久 claim bundle/slot；重启后重复 claim 拒绝，代码不补发或重新生成 key。

T 以独立进程运行，只收到公开配置和本机测试凭据，生成两份加密 bundle 并退出。父级 TEST_ONLY harness 等待 T exit 后才生成 raw shares。P0/P1 通过 `fork/exec` 成为 UID 22012/22013，打开不同 0700 目录的本方输入、密钥和 bundle；双方都尝试打开 peer 的 share/key/bundle，均返回 `EACCES/EPERM`。父级 root harness 能读取两边内容并进行冻结 oracle 检查，因此不能纳入腐化 party 视图，也不能代表实际私有网络发材。

本轮 AES key 由 TEST_ONLY root harness 预置；T 知道并使用两方的测试凭据。跨机器身份凭据发行/轮换和生产认证私有传输未交付。AEAD 验证密文字节和 AAD，不单独验证 uCMP key 与其端点差值的数学关系；此关系仍依赖受信任 T 的密钥生成实现。

负例记录在原始 E2E 日志：错接收者 key、错 party、错 session、tag 截断、密文/slot manifest 篡改、错宽度、bundle/slot 重复领取、claim 跨进程退出仍有效、peer closed、peer silent 均被拒绝或由存活方本地 abort。peer 失联不证明对端同意 abort；没有声称测试了硬断电/文件系统损坏，也没有提交密钥/输入 share。

## 5. 三进程 oracle matrix 与失败保留

冻结 C++ oracle differential 是由父级 TEST_ONLY 控制器在两方返回后执行；生产 party 进程没有 oracle 路径。最终 E2E 原始日志有 53 条配置：

- n=1 singleton；n=2..8 每一个 K=1..n 的严格全序输入均有三进程 SUCCESS；
- 额外覆盖重复分数、全相等、负值、`INT32_MIN/MAX`、非二次幂和 K 两端；
- n=64、K=8 走实际非全量抽样路径；
- 一条显式 TEST_ONLY forced probability abort；双方状态和 reason 一致，双方都没有 `mask.share`。

结果是 52 SUCCESS、1 injected abort、0 自然 probability abort、0 观察到的错误 mask。SUCCESS 的所有 52 个输出都通过长度 n、每位 0/1、weight K、oracle 逐位一致。没有自然 abort 的观测不等于零失败概率或任意参数概率保证；共同 tape 的 39 条及 n=64 附加 tape 也均成功。S4 的 BMW16 Theorem 8 和论文比较常数不适用于本项目 sample-bracket 参数/实现。

E2E 中自然随机币来自双方在线各 `getrandom(32)` 后合币；SHA-256 域分离 counter stream 与无偏拒绝取模抽样。`seed` 仅用于测试 fixture/会话，不固定正式分支抽样。注入 abort 是专用测试钩子，在 Select 成功后改为 abort，用于检查无 mask/最终状态相同；它不冒充自然 sampler failure。

## 6. 计数账本和因果消息

n≥2 SUCCESS 因果相位是：raw adapter 2、forward shuffle 2、sampling coin 1、Select 四层 4、membership 本地 Eval 0、inverse shuffle 2、最终状态确认 1，共 **12**。probability abort 跳过 inverse route，为 10。n=1 为 0。以上是本候选真实 framed message exchange phase，不是 Protocol I 原论文三轮核心，也不是安全入口四轮。

n=64 K=8 这次 loopback：

| 指标 | 观测值 |
|---|---:|
| 各方状态 / oracle | SUCCESS / 两方 XOR mask 与 oracle 相等，weight 8 |
| Select logical calls | 17,757；逐层 7,310 / 452 / 9,984 / 11 |
| Select unique key slots consumed | 5,557；dummy logical calls 12,200；重复请求 0 |
| Membership / uCMP Eval / DCF Eval | 63 / 5,748 / 11,496 每方 |
| phases；双方发送/接收 | 12；各 150,857 / 150,857 bytes |
| 每方材料槽 | 22,306 预发；5,750 claimed；4,032 个 shuffle CmpAgg 未用 |
| 每方 bundle | 22,770,793 bytes；AEAD header+tag 116；manifest 446,128 |
| T offline | 355,042 μs，本次单样本，含生成/序列化/写盘 |
| party API online | 约 10.34 s，本次单机 loopback 样本 |
| sampler SHA-256 words | 328/方 |
| 其他 FSS/PRG 内部调用、峰值内存、LAN/WAN、正式五次统计 | `NOT_MEASURED` |

这只是单机冒烟，不放进 V4 性能表。计量核对 `DCF Eval=2×uCMP party Eval`、双方 sent/peer received 对齐、claimed slots 与实际 raw/Select/membership/shuffle base 槽相符、预发槽等于 `2p+2+11*C(n,2)`。同轮 logical reverse request 计入 logical calls但仅消费一个 canonical key；跨阶段 key 不共享。

后续与 Protocol I 全对全、Protocol I+AAV86 使用同一 [comparison run schema v1](BMW16_COMPARISON_RUN_SCHEMA_V1.json)：统一输入身份、revision、输出状态、九个主指标、逐阶段因果相位/双方字节、逻辑边与真实/dummy 边、材料槽、DCF Eval、PRG、环境和命令。每次尝试都保留一个记录；概率 abort、材料 abort、通信 abort 与注入测试 abort 分开计数并保留发生阶段及失败成本。测试未观测/当前未计量的值用 `NOT_MEASURED`，绝不填 0。此 schema 是后续共用记录合同；本轮原始 E2E 仍以日志记录，并未声称所有字段均已接入 JSON benchmark runner。

## 7. 代码、安全视图和边界

| 审查项 | 结论 |
|---|---|
| C++ 算法身份 | S11 party API 原地扩展；S4 exact trace 范围是上述 39 tape cases，不代表任意 tape 的形式证明 |
| 成功输出功能 | 52 个 process E2E 例逐位 oracle 正确 |
| abort 合同 | 注入概率 abort 双方一致，无 mask；未观测自然 probability abort |
| 独立 OS 身份 | WSL 单机不同 UID 通过本地文件访问夹具；不等于多主机隔离 |
| AEAD/claim | 本地 bundle 功能和指定负例通过；recipient credential provisioning/生产 transport 未建 |
| party-view security | **NO-GO / NOT_PROVED** |
| V4 五次 LAN/WAN / 六方案 | `NOT_STARTED` |

本候选把匿名比较位、graph、pivot/U/V/W、masked operands、abort 和可由 transcript 推导的 selected anonymous handle 均纳入泄露草案。P0/P1 各自的混合论证仍需同时模拟 raw input share/adapter、真实 shuffle local state、两 task 共享句柄、共享节点 mask 所诱导的相关阈值、整池已用和未用 FSS/uCMP key、自适应选取和公开比较位、abort、selected handle、inverse route 和 output share。当前首个缺少的步骤是将实际 shuffle party view 和相关阈值的多 key、自适应 FSS pool 一起换为模拟器；仓库没有提供可直接引用的该联合安全定理/优势界。独立 keygen、单 key DCF 隐私、枚举、AES-GCM 和 oracle correctness 都不补这一缺口。本轮没有证明匿名 transcript 必然可区分；结论是“未证明”，不是“不可能”。

最准确的下一技术任务：针对现有两 task 共用真实句柄、共享节点 mask 的实际 ProtocolI uCMP/DCF pool，证明或实现一个覆盖全部本方 key（含未使用槽）和打开自适应比较位的联合多 key simulator/hybrid，并独立审查其与真实 shuffle view 的组合；若不能，必须缩小/隐藏 transcript 或调整预处理 key 接口，然后重新计量。生产 party 证书/PSK provisioning 和 T→party 私有认证传输是另一必要工程前置条件。

## 8. 命令和环境

环境：Windows 11 主机；WSL Ubuntu 24.04；CMake 3.28.3；GCC 13.3.0；OpenSSL 3.0.13；Windows Python 3.13.5，NumPy 2.3.5。构建目录 `/tmp/bmw16-s12-build` 不在仓库。S4 source 使用其 S4 独立 worktree 只读路径。

构建：

```powershell
wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s12/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s12-build -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON'
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/bmw16-s12-build --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test -j2
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /tmp/bmw16-s12-build -R '^moe_topk_bmw16_experimental_party_test$' --output-on-failure
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/bmw16-s12-build --target moe_topk_m2_ucmp_conformance_test moe_topk_m2_score_input_conformance_test -j2
wsl.exe -d Ubuntu-24.04 -- bash -lc "ctest --test-dir /tmp/bmw16-s12-build -R 'moe_topk_m2_ucmp_conformance_test|moe_topk_m2_score_input_conformance_test' --output-on-failure"
```

这些 CTest 中，EXPERIMENTAL party unit test、既有 Protocol I uCMP conformance 和正式 raw-score input conformance 均通过；三进程 harness 单独以 root 调用并由其 fork 到 party UIDs。

完整 T/P0/P1 E2E（必须 root 启动 harness，以便 drop 到两个固定无特权 UID）：

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s12-build/moe_topk_bmw16_experimental_3proc_test
```

全部 rank 外部随机 tape 差分：

```powershell
py experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/bmw16-s12-build/moe_topk_bmw16_experimental_party_test --s4 C:/Users/28641/.codex/worktrees/bmw16-s4/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py --out experiments/TEST_ONLY_BMW16_S12/evidence/common_tape_all_ranks
```

另在 `/tmp/bmw16-s12-default-off` 用 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=OFF` 和 `BUILD_TESTING=ON` 配置 CMake，检查 `ctest -N | grep -i bmw16` 无输出；因此没有默认注册/编译 S12 target。

## 9. 哈希与原始证据

最终二进制：

| Artifact | SHA-256 |
|---|---|
| `moe_topk_bmw16_experimental_3proc_test` | `df47e79dc4d25298d54197e6616096f07c7a764b2e2b37f5de2cc57be714c7b7` |
| `moe_topk_bmw16_experimental_party_test` | `3839a86fd355406c62381af370e4af6cb76b728dfea56e63f234bb3e7949ed35` |
| E2E raw output | `1ef43dfc5bf04fbe19fceecf57ba2674a4535f191eaab43f07605356b2111c09` |
| small-rank/edge C++ output | `8f48aae0527ef0d64f356d282a8f8d31cf99b84cfc999a27f383456fc56f296e` |
| external tape matrix manifest | `d3a9966c636d1076a4cda576a46aade14bdc30c01414a07b37224e5b66607a4b` |
| S4 frozen source | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |

逐文件最终源码、文档、证据与二进制 SHA-256 清单见 [S12 SHA256SUMS](evidence/BMW16_S12_SHA256SUMS.txt)。原始运行计数保存在 [T/P0/P1 E2E evidence](evidence/BMW16_S12_TP0P1_E2E_2026-10-06.txt)、[TEST_ONLY trace matrix](evidence/BMW16_S12_TEST_ONLY_TAPE_MATRIX_2026-10-06.txt) 与 `experiments/TEST_ONLY_BMW16_S12/evidence/`。原始日志不含 key、score shares 或完整 mask。

## 10. 门禁结论

1. `S11_INDEPENDENT_ACCEPTANCE`: 本轮接收了明确提交，复核报告与 API/旧计量边界一致；本轮结果独立重跑。
2. `S4_CPP_TRACE_EQUIVALENCE`: `PASS_FOR_39_TESTED_TAPES`; 35 个 n=2..8 全 K 和四个 n=64 profile，非一般性证明。
3. `TP0P1_FUNCTIONAL_E2E`: `PASS_FOR_52_SUCCESS_CASES`; one forced abort separate; 0 wrong mask observed。
4. `PARTY_ONLY_LOCAL_BUNDLE_AND_DURABLE_CLAIM`: `PASS_FOR_LOCAL_TEST_CONTRACT`; 生产身份建立/网络分发未完成。
5. `DISTINCT_OS_IDENTITY`: `PASS_FOR_WSL_UID_FILE_ACCESS_TEST`; root harness 不计 party view。
6. `FULL_PARTY_VIEW_SECURITY`: `NO-GO / NOT_PROVED`; 相关整池自适应 FSS+真实 shuffle hybrid 缺失。
7. `FORMAL_V4_PERFORMANCE`: `NOT_STARTED`; 小规模 loopback 数字不进入正式性能表。

未修改 `VFSS-baseline/`、论文 PDF、密钥、构建物或本地参考树；无推送、合并或 PR 更新。
