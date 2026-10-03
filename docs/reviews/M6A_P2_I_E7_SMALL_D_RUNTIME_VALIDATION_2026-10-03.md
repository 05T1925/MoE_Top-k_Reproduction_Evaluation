# M6A-P2-I-E7：小 D 全两两 Protocol I+AAV86 运行时验证与差异记录

日期：2026-10-03。此报告只涉及隔离 worktree `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment`，分支 `codex/m6a-p2-i-allpairs-experiment`，未提交 HEAD `c3926c68fd14f270faa8b55234311071947fa080`。工作区原有 E1–E6 的 19 个未跟踪文件均保留。E7 的 [设计门报告](M6A_P2_I_E7_REVIEW_AND_SMALL_D_RUNTIME_GATE_2026-10-03.md) 先于 secure runtime 源码保存，结论为 `SMALL_D_RUNTIME_GATE = GO`，范围和假设以该报告为准。同一会话承接了 E6，故不能把本次复核称为人员独立签收。

## 1. 实际入口与实现范围

新增独立 `protocol_i_aav86_small_*` 入口；P0/P1 各提供 signed Q20.12 原始 32-bit 字的 `Z_(2^32)` 加法份额，取得原输入顺序 Top-K XOR bit-mask 份额。稳定顺序为分数降序、original index 升序，rank 0 优先；dummy 排在所有真实元素之后。`D=max(2,next_power_of_two(n))`，比较环 `b=33+log2(D)`。当前显式限制 `1≤n≤8`、`D≤8`、`1≤r≤5`，超过即报错。T 离线建立每轮 `C(D,2)` 个 canonical pair 材料及每轮每端点一份 fresh mask，在线 party 只 Eval 活跃边；T 无在线消息描述符。

score adapter 复用已签收 M5 原语；前向 C 式 shuffle 的第一轮 `R[0,a]` 正是全池该轮边 key 的阈值来源；后续轮 fresh `R[t,a]`。CA 控制器只看公开 shuffled handle 上的 local ranks 与公共 pivot seed；其节点/桶列表使用不可变 handle 坐标，未作物理 record 重排。逆路由用独立随机因子但绑定同一 `π^{-1}`，公开 membership 的原序 XOR 份额由一轮 share 路由获得。secure 入口不重构原序明文 mask、selected index 或原始分数。

package 绑定 magic/version/session/fingerprint/material ID/party/n/D/K/r/b、pivot seed、每条边的 t/a/c/id 和 score 材料；读入时验证数量、顺序、宽度、截断和尾随数据。运行前在调用方提供的持久目录执行 `openat(O_EXCL|O_NOFOLLOW)` 并 `fsync` 文件及目录，同一 session/material ID/party 的旧包重载被拒绝。它是半诚实流程和重放拒绝机制，**不是恶意安全认证或 MAC**。64 MiB 单包上限保留，小 D 限制使百万边分配不会发生。

## 2. 实际消息、泄露与轮数

| 顺序 | 相位 | 发送内容/依赖 | 公开内容及可见性 |
|---|---:|---|---|
| 离线 | — | T→P0/P1 各自完整 package；T 退出 | T 知全部 π、R 和 keys；无在线输入/rank/输出通道 |
| score 1 | 4 | 原始分数份额的 carry uCMP | 固定长度 framed 消息 |
| score 2 | 5 | 依赖 carry 的 sign uCMP | 固定长度 framed 消息 |
| CA 0 | 9 | `σ_p(key_share)+a_p` | 单方所收的 peer masked share |
| CA 每轮第 1 步 | 10,12,… | shuffled share 加本轮 `R_p[t,*]`；第 t>0 轮依赖先前 rank/bucket | 整个 D 维 `y_t` 公开，包括不活跃 handle |
| CA 每轮第 2 步 | 11,13,… | 活跃边本地 uCMP Eval，发送局部 rank share | `L_t` 公开；pivot、bucket、图、查表顺序是已公开 ranks/公共币的后处理 |
| 逆路由 | 40 | 公开 membership 的加法 carrier share 经同一 π 的独立逆因子路由 | 双方仅取得原序 XOR 输出份额 |

固定参数、每轮固定 D 维帧长度、abort 时点也对在线方可见；本实现未评估缓存/计时侧信道。真实 `message_trace` 记录相位、每方发送/接收字节；`edge_trace` 记录每条活跃边的 t/a/c/material ID。测试核对 trace 顺序、对方字节对称、槽位与 ID、无重用及首轮 R/key 绑定。实际因果轮为 score 2 + CA `2r+1` + inverse 1 = **`2r+4`**；这仅对本新入口成立，与旧 Protocol I/III 的轮数分开。完整图预留 `r·C(D,2)` 与实际活跃边数分开计量。

## 3. 验证环境、命令和原始结果

环境：Windows 主机上的 WSL Ubuntu-24.04；CMake Debug 构建在 WSL `/tmp/m6a7-build`，`MOE_TOPK_ENABLE_EMP_OT=OFF`；源码 revision 为上述 HEAD 加本报告列出的未提交 E7 差异。测试的输入 share 使用源码中固定的 `std::mt19937_64` 种子；T 材料使用系统 `getrandom`，秘密随机种子不记录，因此活跃边的逐次值不能字节级复演，但每次真实 trace 已保存。下表的时间是这一次本机调试构建和进程调度下的观测值，不是 LAN/WAN benchmark。

实际构建命令：

```sh
cmake -S VFSS -B /tmp/m6a7-build -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m6a7-build --target moe_topk_m6a7_aav86_small_conformance_test moe_topk_m6a7_aav86_small_differential_test moe_topk_m6a7_aav86_small_e2e_test -j 4
```

按要求依次执行三项，三个可执行文件的 stdout 分别保存在 `experiments/m6a_p2_i_allpairs/TEST_ONLY/` 下：

| 阶段 | 结果 | 本次原始日志 SHA-256 |
|---|---|---|
| `moe_topk_m6a7_aav86_small_conformance_test` | PASS，24 组；首轮/跨轮 mask-key、同 π 前逆、exact K/dummy、错误方/参数/轮/槽/截断/尾随/超限与单 key 重用拒绝 | `9BA6C3183027C3B7B476934D068990284AC96217B47104B25F09ECDBE823DD13` |
| `moe_topk_m6a7_aav86_small_differential_test` | PASS，57 组；冻结 `top_k_mask` oracle，含 n=1、2..8、全同分、非二次幂、INT32_MIN/MAX、K=1/n、r=1..5；同进程与 fork 后旧包重放拒绝 | `9D0D2D62D1E595856062AA2E11638C8901EF0432E1F1AE696E12A0F3FCD4003C` |
| `moe_topk_m6a7_aav86_small_e2e_test` | PASS，50 组；T/P0/P1 均 fork 后 exec 为独立角色，T 的新地址空间仅获公开参数和离线 package 描述符；输出由测试 harness 重构并对照 oracle | `1A31D6DC4440876C0BAE961D46CF6E77A7BC0BC22C328C0CEBFA35D3B9E3A856` |

上述日志分别是 `e7_conformance_test_raw_2026-10-03.txt`、`e7_differential_test_raw_2026-10-03.txt`、`e7_e2e_test_raw_2026-10-03.txt`。differential 日志逐配置列出实际 `t:a:c:material_id` 与每个 phase 的双方字节；E2E 日志逐配置列出预留/活跃/Eval/字节/轮数/时间/进程峰值内存和边集合 digest。

| 本次计量 | differential 57 组求和 | 三进程 E2E 50 组求和或最大值 |
|---|---:|---:|
| 每方预留 CA key 槽 | 2980 | 2730 |
| 实际活跃 CA 边 | 776 | 708 |
| 每方 score DCF Eval | 1368 | 1200 |
| 每方 CA DCF Eval | 1552 | 1416 |
| 每方全路径 DCF Eval | 2920 | 2616 |
| P0/P1 各发送字节 | 58848 / 58848 | 52800 / 52800 |
| 每次因果轮 | `2r+4` | `2r+4` |
| 离线 T 发材墙钟时间求和 | NOT_MEASURED | 166919004 ns |
| 每例双方在线墙钟较大值求和 | NOT_MEASURED | 136105288 ns |
| 在线 party 峰值 RSS 最大值 | NOT_MEASURED | 4352 KiB |
| T 峰值 RSS、PRG/AES 细分成本、LAN/WAN | NOT_MEASURED | NOT_MEASURED |

峰值 RSS 是每个 party 子进程 `getrusage(RUSAGE_SELF).ru_maxrss` 的最大值；离线时间包括 T 子进程调度、生成、序列化与发材，不是纯密码学计算时间。按配置逐行数据比合计更适合复核。`r·C(D,2)` 为解析容量公式；表中预留槽是各个已运行配置公式的求和，不应当当作 n=10^5/10^6 的实测。

相关旧路径回归的实际命令：

```sh
cmake --build /tmp/m6a7-build --target moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_m5_fix_f1_raw_score_mask_test moe_topk_m5_fix_f1_process_e2e_test moe_topk_m5g_two_round_fsort_test -j 4
ctest --test-dir /tmp/m6a7-build --output-on-failure -R 'moe_topk_m6a7_aav86_small_|moe_topk_m2_parallel_shuffle_(conformance|process_e2e)_test|moe_topk_m5_fix_f1_(raw_score_mask|process_e2e)_test|moe_topk_m5g_two_round_fsort_test'
git diff --check
```

结果：8/8 CTest PASS、`git diff --check` PASS。E7 untracked 源文件另以行尾空白扫描核对，未发现行尾空白；`git diff --check` 本身只覆盖已跟踪差异。测试顺序为 conformance → differential → 三进程 E2E；上述合并 CTest 作为最后回归确认。

## 4. 功能、安全与尚缺证据的分界

**小规模功能验收：PASS（限定 D≤8/r≤5）。** 数据与 trace 证明此源码实例在列出的 24+57+50 组输入上的输出、材料查找、轮数和字节计数满足合同；不能替代穷举或正式密码学证明。首次 differential 运行发现递归 `nodes.push_back` 后使用失效引用，已改为事先保存桶数，随后重跑通过。初版 E2E 只 fork，会继承测试 harness 的明文输入内存；已改为每个角色 fork 后立即 exec，T 的执行映像只接收公开参数及两条离线发材描述符，重新完成全部 50 组。这些均是 E7 范围内的修复，未触及旧 Protocol I/III 行为。

**条件性安全结论：** 只在设计门所列 FSS-IND（单份、选阈值、独立新鲜币）、AES PRG/OS 熵、私有有序完整通道、T 可信且不合谋、P0/P1 单方半诚实等假设下主张显式 transcript 的模拟归约；关键 peer rank 消息为 `L-q_p`，全池 keys 与同 π 前逆材料必须联合考虑。源码测试不证明这些密码学假设，亦不证明侧信道、恶意安全或 T 合谋安全。

**作者精确复现：`AUTHOR_EXACT = NOT_PROVEN`。** 这里的可信 T 全池、稳定键、padding、持久 package 与 XOR 输出是项目组合；未将其写成 AAV86/Agarwal 作者代码或论文证明。

**尚未完成：** Protocol III+AAV86、D>8 的容量/分片策略、n=10^5/10^6 及 LAN/WAN 五次正式性能矩阵、T 峰值内存与 PRG/AES 细分计量、侧信道分析、另一个人的独立签收。因此 M6A 整体验收未完成，不能由此次小 D 结果外推。

## 5. E7 文件差异

本次只在隔离 worktree 中修改或新增下列 12 个文件；E1–E6 未跟踪文件原样保留，主工作区、`VFSS-baseline/`、`Papers/`、本地参考树未写入，未暂存/提交/推送/创建 PR。

| 文件 | 用途 |
|---|---|
| `docs/reviews/M6A_P2_I_E7_REVIEW_AND_SMALL_D_RUNTIME_GATE_2026-10-03.md` | 实现前的设计门、全池 hybrid、联合置换与 DAG |
| `docs/reviews/M6A_P2_I_E7_SMALL_D_RUNTIME_VALIDATION_2026-10-03.md` | 本报告、计量与限制 |
| `docs/IMPLEMENTATION_PLAN.md` | 同步当前全两两材料项目决策、小 D 轮数和未完成项 |
| `VFSS/CMakeLists.txt` | 新源码与三项测试目标 |
| `VFSS/include/moe_topk/protocol_i_aav86_small.h` | 独立接口、材料与 trace/计量类型 |
| `VFSS/src/moe_topk/protocol_i_aav86_small.cpp` | 离线 T、序列化/claim、CA 运行时及同 π 逆路由 |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_conformance_test.cpp` | package、代数、mask/key 和负面用例 |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_differential_test.cpp` | oracle 差分、真实边/消息 trace 与重启重放拒绝 |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | T/P0/P1 独立进程 E2E 与逐配置计量 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_conformance_test_raw_2026-10-03.txt` | conformance 原始 stdout |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_differential_test_raw_2026-10-03.txt` | differential 逐配置原始 stdout |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e7_e2e_test_raw_2026-10-03.txt` | E2E 逐配置原始 stdout |
