# S22 独立接收、统一中止、TLS fail-closed 与联合视图审查

日期：2026-10-07
精确接收起点/本轮被测源码：`c231329f8c952b80df5cee201b056b461b5701b0`
分支：`codex/m6b-i-bmw16-s22`
起始差异：clean；本地 `main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`，与 S21 起点的 merge-base 也是该 revision。按本任务要求，S22 从 S21 精确提交开分支；S21 源码相对于 main 的历史实验提交保留在 ancestry 中。
候选身份：Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 1. 范围与证据等级

本轮在 S21 精确提交上独立重建并复跑，随后对算法状态分类和传输模式增加小范围修正。测试输出、common-tape 矩阵和 binary 均保存在仓库外的 `C:/Users/28641/.codex/artifacts/BMW16_S22_20261007/`，不提交密钥、证书私钥、bundle、构建物或逐次大日志。

证据分为：PAPER（外部原语文献的安全定义）、SOURCE（当前 C++ 实现）、TEST（本机复跑）、PROJECT_DECISION（本轮用户转述的许可）和 UNRESOLVED（仍缺证明）。不得从 TEST 推出密码学安全，也不得将 PROJECT_DECISION 写成密码学结论。

## 2. S21 原提交独立接收

| 项目 | S21 历史报告所述 | 本轮对精确 S21 源码独立重跑 |
|---|---|---|
| Release/party node | S21 报告记录了不同 UID、本地 mTLS 与实验入口 | 从 S21 精确 worktree 独立配置/编译定向目标；targeted CTest 8/8 PASS |
| TLS E2E | S21 原报告记录 n=1/2/3/5/8/64/128/256 与负例 | 新进程 TLS E2E 在外部日志 `s21_exact_tls_e2e.log` 记录完整矩阵及结果；exit 0 |
| common random tape | S21 报告有 S4/C++ 共同 tape | 独立复算 39/39；S21 矩阵 SHA-256：`15f7983541b02d03773ad2c68ec665551263a7b049455b89f52d94872cc19b8c` |
| fail-closed | S21 构造 TLS 流并使用 online mTLS | 本轮基线夹具证明 S21 `ProtocolIFramedChannel` 构造不要求 TLS stream 注册：`BASELINE_CONSTRUCTOR_ACCEPTED_UNREGISTERED_FD raw_fallback_possible=1`；因此 S21 的 TLS 握手存在不等于 framed data path 被 fail-closed 绑定 |

S21 旧代码的正向测试没有覆盖“TLS 模式下 stream 未注册却回落裸 FD”的负例。本轮将其保留为修正前复现；S21 历史结论不回写。

本次新跑的 S21 精确构建二进制 SHA-256 为 `892080d4ae048d7a2b1276e97a483681ea6836efb67f706815d9472c9aa34df2`；S21 targeted CTest 和 TLS E2E 日志分别为 `fa46f322934e0d2c0f8fe63654901ab9b86ed352b31a069164e0dbb1dff553be`、`063aa8f93a8bda8e3b64d1bdf400217e910b3470ede72bfc40a7ccd1739a3457`。这些是重建结果身份，不是 S21 当年报告的原始运行文件。

## 3. S22 修正与源码检查

### 3.1 统一算法中止和工程错误分流

final agreement 三值合同：0 SUCCESS、1 正常算法概率 abort、2 算法不变量/工程错误。正常失败如 R2 bracket 未夹住、U 超限、R3 W 超限，映射为状态 1；双方完成最终交换时均返回 `ABORT_ALGORITHM_PROBABILITY / PEER_AGREED`，party exit 10 且无 mask。工程错误（非法 rank、R1 非唯一 rank、R3/R4 invariant、交集非唯一或状态不一致）映射状态 2，返回 `ABORT_ALGORITHM_INVALID`，设 `engineering_failure`，退出 70 且清除 mask。材料错误 exit 20，通信断连/静默 exit 30 且 `LOCAL_ONLY`；没有对端确认时不宣称双方一致 abort。

新增 TEST_ONLY 的不变量 failpoint 与最终状态不一致 failpoint，分别检查双方不把异常吞为 exit 0/概率 abort；输出文件不得存在。

### 3.2 认证流 fail-closed

增加 `ProtocolITransportMode`：`CallerOwnedFd` 保留旧协议显式裸 FD 行为；`RequireAuthenticatedStream` 要求同一 fd 上预先注册的一次性 TLS byte stream。严格模式缺流、已消费或注册竞争时构造失败，随后不会走 `poll/read/write` 裸 FD 分支。若已注册 stream 却选择旧 FD 模式也拒绝，避免误接。对 strict-mode 构造失败，FD 所有权留给 caller，防止与 TLS owner 双重 close。

S22 将严格模式沿 raw-score、BMW16 R1–R4、sampling、forward/inverse shuffle、final agreement 的实际 channel options 传播。回归测试仍显式执行旧 CallerOwnedFd 分支。

## 4. 本轮验证

环境：WSL Ubuntu 24.04、GCC 13.3、CMake 3.28.3、OpenSSL 3.0.13、Release；完整命令、逐例 JSON 和执行日志见外部目录索引。未启动正式 LAN/WAN 1+5。

| 阶段 | 本轮结果 | 说明 |
|---|---|---|
| S21 精确源码定向 CTest | 8/8 PASS | 仅所列定向 target，不称全仓 CTest |
| S21 exact common tape | 39/39 PASS | 矩阵 SHA-256 如上 |
| S21 exact TLS E2E | PASS | 正/负例在外部原始日志；旧代码存在 strict stream 未绑定缺口 |
| S22 定向 CTest | 8/8 PASS | 含 transport mode、BMW16 party 和 Protocol I 相关回归 |
| S22 common tape | 39/39 PASS | 当前矩阵 SHA-256：`27df4b2299b7052a92d686f643ff4137b6b944761e51db4e770085d7507c39e6` |
| S22 TLS E2E | PASS（有界 n≤256） | n=1 shortcut 与 n=2/3/5/8/64/128/256 success；algorithm abort、材料重领、错身份/静默、工程错误、TLS 缺流和 final status mismatch 均有独立状态/退出码；失败无 mask |
| S22 n=1000/K=80 | PASS，单次功能样本 | fresh streamed materials；T/P0/P1 receive 与两方在线 party 均 exit 0，TEST_ONLY oracle 检查 SUCCESS；不同 UID 文件隔离 PASS |
| 正式性能/网络矩阵 | NOT_RUN | 本地功能 E2E 不并入 V4 或六方案表 |

失败类别观察样本数不是概率证明。S16 对其固定整数规则和理想无放回 sampler 给出的数学失败上界仍与实际 OS/哈希 counter sampler 的计算分布差异分开；本轮未测得新的统计失败界，也不引用 BMW16 Theorem 8。

**抽样分布限制：** 当前在线 coin 是两方各生成 32-byte OS 随机贡献后公开交换并 XOR；合币 seed 因而对双方及 transcript 均公开。随后以 `SHA256(seed || task || domain || counter)` 展开采样 words，再以 rejection sampling 去模偏。由于 seed 公开，这不是秘密密钥 PRF 用法。S16 的上界只适用于理想、独立、均匀的无放回抽样；它不能直接作为当前哈希展开实现的严格概率界。当前实现的精确分布还需要随机 oracle/随机扩展器类假设，或另行证明的采样构造。OS CSPRNG 贡献说明参与方熵的来源，不会把公开 seed 变成秘密，也不能单凭 OS 熵推出哈希流与理想抽样统计距离为零。零自然 abort 样本只报告观察频率，不满足 `≤10^-6` 的理论验收证明。

E2E 输入可由源码中的 `run_tls_suite()` 精确重建：n=2/3/5/8 为固定边界向量/helper，session 分别 `0x20201001/0x20201003/0x20201005/0x20201008`；n=64、128、256 使用 `seeded_scores` 种子/session `0x20206401/0x20212880/0x20225602`，K=`8/128/2`；n=1000/K=80 使用种子/session `0x2020100080`。加法 share split 使用 `mt19937_64(session XOR 0x534841524553)`。T/FSS 材料每例 fresh，由 OS 熵生成。在线 sampler coins 是双方各自 `getrandom(32)` 后经 TLS 交换；当前 E2E 日志不保存这 64 bytes，也不保存其 combined-seed digest，因此能重建输入和材料新鲜性条件，不能逐字节重放当次采样 coin。固定 coin tape 仅用于独立 common-tape 差分，不混入正式 party E2E。此日志粒度缺口已显式登记，不用它主张概率界。

### 外部产物

仓库外目录 `C:/Users/28641/.codex/artifacts/BMW16_S22_20261007/` 保存：S21/S22 两次构建目录、8 项 CTest 日志、两版 TLS E2E 日志、common-tape 矩阵、S21 裸 FD 缺流基线复现和当前 n=1000 运行文件。n=1000 当前日志报告每方 sidecar 4,967,527,664 bytes、每方 shell 2,013,901 bytes，双向离线交付总计 9,941,055,454 bytes，T 离线阶段 427,146,388 μs；这是此单次 localhost 实测，不是正式性能结论。E2E 的 P0/P1 测试性重领各退出 20 且无 mask。提交时另存 `SHA256SUMS.txt` 索引；其中所有秘密材料都由测试 harness 临时生成并留在外部目录，索引只包含文件名、状态和哈希，不包含秘密字节。

### 复跑命令与构建身份

环境为 WSL Ubuntu 24.04、GCC 13.3、CMake 3.28.3、OpenSSL 3.0.13，Release。源码为本 worktree 的 S22 差异（起点 `c231329f8c952b80df5cee201b056b461b5701b0`）；构建位于仓库外的 `build-s21-receive`（目录名沿用 S21 接收阶段，但 cache source 指向 S22 worktree）。定向复跑命令如下，`$ART` 指向上文外部证据目录：

```sh
cmake --build "$ART/build-s21-receive" --target moe_topk_bmw16_experimental_party_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_m3_grank_test moe_topk_m3_dpf_routing_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test -j2
ctest --test-dir "$ART/build-s21-receive" --output-on-failure -R '^(moe_topk_bmw16_experimental_party_test|moe_topk_m3_grank_test|moe_topk_m3_dpf_routing_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_transport_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_parallel_shuffle_process_e2e_test)$'
"$ART/build-s21-receive/moe_topk_bmw16_experimental_party_node_e2e_test" --tls
"$ART/build-s21-receive/moe_topk_bmw16_experimental_party_node_e2e_test" --tls-n1000
```

最后两条是该 E2E harness 的标准矩阵入口与单独 n=1000/K=80 入口；测试产生的临时材料、输入和逐次日志只放在 `$ART`。构建目录沿用 `build-s21-receive` 名称，但其 `CMakeCache.txt` 的 `CMAKE_HOME_DIRECTORY` 指向本 worktree 的 S22 源码，配置为 Release 且 `BUILD_TESTING=ON`。当前 party-node executable SHA-256 为 `b41673548359aea4e5ee4072e4aa4eb87cc85ae577452356eb2aed0e0c5c553d`；E2E harness SHA-256 为 `892080d4ae048d7a2b1276e97a483681ea6836efb67f706815d9472c9aa34df2`。common-tape `matrix.json` SHA-256 为 `27df4b2299b7052a92d686f643ff4137b6b944761e51db4e770085d7507c39e6`。首次 S22 targeted CTest 日志为 `b25df891ddbcff881a18cb059401cd4c7f5fb4a6cda1f7b4f07ee6d8824b47e6`；最终复核日志 `s22_final_ctest.log` SHA-256 为 `e41328c4f963b177d78e3e2b7cfbac5f69859d9824007bc87b36108f32cf7d89`，同为 8/8 PASS。S22 TLS E2E 汇总日志为 `77750c6aeef5e4508999386a1893f3506d261176c239c554f1877083f4273d92`，n=1000 独立日志为 `a41dae081c35e9047b1efa3f8934fbd6f089a66e3692faf416bca0931afabda8`。精确文件清单和全部 repo source 哈希见仓库外 `SHA256SUMS.txt`；提交前更新该索引，避免索引自指。

## 5. 批准的公开信息与完整视图

本轮用户转述学姐答复，批准 P0/P1 之间公开匿名化后端点/比较 bit、pivot 位置、masked operands/public_z、置换后 selected position、实际 slot ID 与次序、比较次数、通信长度以及一致算法中止状态。S21 原文称这些仍待批准；该许可于 S22 才报告，属于项目泄露合同更新，不能改写成 S21 当时事实。

未获准的内容：raw score、original_index、原始输入位置到置换 handle 的映射、完整明文 mask。key ID 可编码 session、stage/task/round、规范 shuffled endpoints，不得含原始位置/分数。代码中的 raw carry/sign local array slot 仅为本地方便寻址，未进入在线公开端点或 key ID。

完整 P0/P1 view 含：本方 input share、本方 shuffle sigma/tau/a/e/r_share 和消息、全部本方 key（含未用槽）、shared node mask 导出的相关 DCF thresholds、本地 Eval share、peer Eval share、opened bit、两个 Select task 的共享匿名 handle、R1–R4 自适应边和 pivot/U/V/W、sample/key IDs、selected handle、abort、membership、inverse、输出 share 及 TLS frame 长度/状态。

## 6. 联合视图裁决

### H0→H1：随机带

`seed_fss_once` 用 OS `getrandom` 为 256 个 cryptoTools PRNG 提供根 seed；`keyGenDCF` 每次取 2 个 block，节点扩展由 AES 四块接口完成。替换为独立均匀 tape 的条件优势记作 `Adv_PRG(Q)`。代码已有的 DCF counters 不能代替全协议 PRG 调用数，Q 当前 `NOT_MEASURED`；SHA-256 sampler PRF words 和 FSS AES blocks 必须分列。

### H1→H2：相关自适应多 key

条件性归约见 [S22 决策](../decisions/BMW16_S22_APPROVED_LEAKAGE_ABORT_AND_JOINT_VIEW_2026-10-07.md)。核心是固定已批准 transcript L，逐 slot hybrid；其它阈值相关 key、完整未用池与本方 shuffle 辅助信息作为单 key FSS game 的辅助 state；挑战 key 的两次 Eval 是完整 party key 的确定后处理；本方收到的 peer share 由公开 b 与本方 s 决定为 `b−s mod 2^64`。因此若 `dcf.cpp` 的完整 serialized party key确实满足 BGI15 comparison-FSS function privacy，且每槽 KeyGen randomness可按 PRG hybrid 独立化，则在 N 个槽上损失至多 `N·ε_DCF + Adv_PRG(Q)`。本结论是条件性证明草案，独立 reviewer 尚未确认源码与 Theorem 6 construction 的精确等价，故门为 CONDITIONAL。

参考：[Boyle, Gilboa & Ishai, “Function Secret Sharing,” EUROCRYPT 2015, Definition 2 and Theorem 6](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf)。该文 Definition 2 将安全游戏定义为单方完整 key 对函数的不可区分性，Theorem 6 给出 comparison-function FSS；它不自动证明项目 `dcf.cpp` 的实现对应关系，也不单独覆盖实际 shuffle。

### H2→H3：shuffle 和输出

S22 决策给出基于实际 dealer 方程的条件模拟。关键关系是 `tau0=pi∘sigma1⁻¹`、`e0=−tau0(a1)−h`，所以 P0 的本地 forward 置换对 `(sigma0,tau0)` 在给定 pi 下仍独立均匀；inverse 使用 pi⁻¹ 和独立随机 tape，联合边际同样不含 pi。给定 L 中双方各轮 masked frame，本地 a/e/r 可由线性等式条件采样；对 inverse 输出，本方 ideal bit share 是本地 `y.word0` 最低位，模拟器先按该位条件抽 y，再解出 e/r，得到精确条件分布。该论证显式条件化消息和 output share，不只依赖置换边际。

H2→H3 仍为 CONDITIONAL：适用要求 L 精确包含 raw adapter 两个 masked operand frames、forward/inverse 两轮三 word records 及 public sum、Select transcript 与 status；还依赖 FSS hybrid 先替换整池 key。代码接口和公式对应由本轮源码审查支持，但尚未由独立密码学审查者逐式签收。没有发现两世界 distinguisher；不把条件性模拟写成 secure PASS。

## 7. 门禁结论

| 门 | 结论 |
|---|---|
| `FUNCTIONAL_ENTRY` | PASS：本轮独立构建、common-tape、n≤256 及 n=1000/K=80 独立进程 TLS E2E；n=1000 为单次本机功能样本 |
| `UNIFIED_ABORT` | PASS：正常抽样失败状态 1、工程错误状态 2 分流；peer failure LOCAL_ONLY；失败无 mask |
| `TLS_FAIL_CLOSED` | PASS：RequireAuthenticatedStream 缺流不降级；旧 FD 只在显式 legacy mode |
| `APPROVED_LEAKAGE_CONTRACT` | PROJECT_DECISION_PASS：用户转述的项目许可已写入；不等同于安全证明 |
| `MULTIKEY_ADAPTIVE_VIEW` | CONDITIONAL：N·ε hybrid 草案成立须确认实际 DCF 单 key theorem correspondence 和 PRG调用条件 |
| `SHUFFLE_OUTPUT_COMPOSITION` | CONDITIONAL：给出局部置换、masked-frame、inverse output-share simulator；待独立逐式审查源码对应 |
| `SECURE_ALIAS` | NOT_CREATED：仍 EXPERIMENTAL |
| `DEPLOYMENT` | CONDITIONAL：S21 本地配置/测试 CA 不等于生产证书注册/跨主机运维验收 |
| `PERFORMANCE_READY` | NOT_RUN：无正式 LAN/WAN/V4 资格 |

## 8. 复跑命令、文件与哈希

当前 S22 n=1000 命令为外部构建目录中的 `moe_topk_bmw16_experimental_party_node_e2e_test --tls-n1000`；日志末行为 `N1000_TLS_E2E_PASS`。精确源码/证据 hash 清单在仓库外 `SHA256SUMS.txt`，包含本轮 17 个修改文件、关键未修改 DCF/uCMP 源、S21/S22 二进制和原始日志。任何 loopback 时间或容量估计都不写入正式性能表；正式 LAN/WAN 时间、全路径 PRG、正式峰值 RSS 等缺项为 `NOT_MEASURED`。

本地差异仅涉及 S22 规定的 transport、score/shuffle/Select wiring、party-node 和相关定向测试，以及本决策/验证文档、计划状态同步。`VFSS-baseline/`、Papers、证书私钥、sidecar、bundle、build tree、原始大日志和本地参考工程均不在 Git 差异中。不推送、不合并、不更新 PR #28。
