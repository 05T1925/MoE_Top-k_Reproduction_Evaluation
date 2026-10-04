# M6A-P2-I-E8 修正、干净验证与规模门结果

日期：2026-10-03；隔离 worktree `codex/m6a-p2-i-allpairs-experiment@c3926c68fd14f270faa8b55234311071947fa080` 加未提交差异。本报告接续先于改动保存的 [E8 接收复核](M6A_P2_I_E8_RECEIVER_REVIEW_2026-10-03.md)。E7 报告及原始日志、E1–E6 文件、主工作区、`VFSS-baseline/`、`Papers/` 和参考树均未覆盖。

## 1. 门禁结论

- `E7_TECHNICAL_SMALL_D = PASS`：在已审过的 trusted-offline-T、P0/P1 单方半诚实、完整私有通道条件下，E7 小 D 功能、前向/逆向代数、全池条件性归约及实际消息 DAG 未发现模型内阻断项。下述修正和干净测试完成。
- `E7_INDEPENDENT_ACCEPTANCE = FAIL`：**程序性、不可在本聊天内修复**。本聊天承接 E6/E7 撰写，用户要求的是未参与两阶段的新窗口。技术 PASS 不能偷换成异会话签收。
- 按用户的“独立接收 FAIL 且无法修复则停止规模扩展”门禁，**未扩大 D≤8**，未对 D16/32/64/128 keygen、差分或 E2E 运行。最大本次实测仍为 D=8、r=5；中等规模状态为 `BLOCKED_BY_INDEPENDENT_ACCEPTANCE`，不是 `RESOURCE_LIMITED`，因为尚未进入资源实测。
- `AUTHOR_EXACT = NOT_PROVEN`；M6A 整体完成和 LAN/WAN 性能矩阵均未发生。

## 2. E8 具体修正及验收含义

1. `protocol_i_aav86_small_conformance_test.cpp` 增加两个同 bits/party edge key blob 交换测试，外层 t/a/c/material ID 原样保持。反序列化**确实接受**交换后的包；随后用原 dealer 双份材料和同一 R 在 16 个 TEST_ONLY 比较探针核验，发现交换后的 key 与 mask/边不一致。此处明确区分**结构标签检查**与**内容/数学关系证明**。可信 T、完整发材通道是既定模型，不增设伪 MAC 或 checksum；头文件注释及实施计划收窄了旧表述。现有 conformance 中原始 dealer 材料的跨轮逐边比较核对继续执行。
2. `protocol_i_aav86_small_differential_test.cpp` 增静默 peer 超时及 peer 关闭用例：本次干净日志记录静默等待约 101 ms 后抛错；关闭端在 3 秒 alarm 上界内异常/`SIGPIPE` 退出，未继续协议。持久 claim 在输入依赖在线帧前发生；目录由调用方跨重启保存。同名 session/material ID/party 的旧包仍由 E7 测试验证拒绝。外部删除 claim 文件、恶意对手和计时/缓存侧信道均不在结论内。
3. `protocol_i_aav86_small_e2e_test.cpp` 每例记录实际 P0/P1 序列化 package 字节数与**公开算法 pivot seed**，并核对双方 seed 一致。密码材料随机币来自 OS `getrandom` 与 AES PRG，不记录秘密种子；这是正常安全选择，因而同一输入种子下活跃图和计时不承诺逐字节复演。E2E 保持 T/P0/P1 fork+exec，T exec 后仅有两个离线 package 描述符，父进程等待 T 退出后才发送输入份额。
4. 旧 `protocol_i_aav86_small.cpp` 未修改；前向与每轮 mask/key 关系、同 π 一轮逆路由、一次性 claim、`2r+4` phase 顺序仍是 E7 代码路径。旧 Protocol I/III 接口未改。

## 3. 从源码复核的因果轮与泄露

| 因果层 | phase | 输入与下一步依赖 | 可见数据 |
|---|---:|---|---|
| score carry | 4 | 原始双方加法份额 | 固定 D 帧 |
| score sign | 5 | carry 打开值 | 固定 D 帧 |
| 前向 C share | 9 | score 稳定键份额 | 对方 masked shuffle share |
| 每轮 masked-open | `10+2t` | 初轮接前向；后轮接上一轮公开 rank/桶 | 全 D 的 `y_t` |
| 每轮 rank reveal | `11+2t` | 本轮公开 y、公共 pivot、活跃边本地 DCF Eval | 按 handle 的 local rank L；图、查表顺序、桶由 L 与公开币确定 |
| 同 π 逆路由 | 40 | 最后一轮公开 full order 的 membership | 最终各方原序 XOR share |

每阶段按代码真实 `exchange_words` send/receive 次序检查，而非只引用预填的 `metrics.causal_rounds`。此**独立 API**的 CA 核心为 `1+2r`，两轮 score 和一轮只输出 share 的逆路由后为 `2r+4`。它不适用于旧 Protocol I/III 入口。显式公开还包括参数、帧长度、pivot/bucket/活跃边、abort 时点；通道侧信道未审。FSS 单份选阈值隐私、独立 keygen 币、AES PRG/OS 熵及非合谋 T 是安全归约的**假设**，源码测试不能证明它们，也不能把归约写成 Agarwal 作者证明。

## 4. 干净验证记录

环境：WSL `Ubuntu-24.04`、GNU C/C++ 13.3.0、OpenMP 4.5；新建 `/tmp/m6a8-clean-20261003`，`CMAKE_BUILD_TYPE=Debug`、`MOE_TOPK_ENABLE_EMP_OT=OFF`。构建目录不在仓库。源码身份为本报告首段 HEAD 加下列 SHA-256：

| E8 验证输入 | SHA-256 |
|---|---|
| `protocol_i_aav86_small.h` | `92ABFFED0FEF9D0F1CA393D9D1E9B607ECC4596DFEE41F1DE13F6BE3B5C50836` |
| `protocol_i_aav86_small.cpp` | `296C25CADA425FDDA1A4ABF329FF9147B3732590E2AB0E993A4E07731DCBF85B` |
| `protocol_i_aav86_small_conformance_test.cpp` | `D4DB5D01A5725244A993BEB49B629C0E81CAEED5FDDD40E4A85D71B6C9DDE120` |
| `protocol_i_aav86_small_differential_test.cpp` | `218E3DD7B4A1642205C8229185147DF81D77EA1F3160491A2CEFC1EFB230AD4B` |
| `protocol_i_aav86_small_e2e_test.cpp` | `344CB6DE9726CF91F8B0241391B16C435C0392CC662D52A2923CA40E542D56DD` |

实际命令（依次）：

```sh
test ! -e /tmp/m6a8-clean-20261003 && cmake -S VFSS -B /tmp/m6a8-clean-20261003 -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m6a8-clean-20261003 --target moe_topk_m6a7_aav86_small_conformance_test -j 4
/tmp/m6a8-clean-20261003/moe_topk_m6a7_aav86_small_conformance_test
cmake --build /tmp/m6a8-clean-20261003 --target moe_topk_m6a7_aav86_small_differential_test -j 4
/tmp/m6a8-clean-20261003/moe_topk_m6a7_aav86_small_differential_test
cmake --build /tmp/m6a8-clean-20261003 --target moe_topk_m6a7_aav86_small_e2e_test -j 4
/tmp/m6a8-clean-20261003/moe_topk_m6a7_aav86_small_e2e_test
cmake --build /tmp/m6a8-clean-20261003 --target moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_m5_fix_f1_raw_score_mask_test moe_topk_m5_fix_f1_process_e2e_test moe_topk_m5g_two_round_fsort_test -j 4
ctest --test-dir /tmp/m6a8-clean-20261003 --output-on-failure -R 'moe_topk_m6a7_aav86_small_|moe_topk_m2_parallel_shuffle_(conformance|process_e2e)_test|moe_topk_m5_fix_f1_(raw_score_mask|process_e2e)_test|moe_topk_m5g_two_round_fsort_test'
```

结果：新路径 conformance 24 组、oracle differential 57 组另加 2 个失败关闭用例、fork+exec E2E 50 组全部 PASS；相关 CTest **8/8 PASS**。冻结 oracle、n=1/非二次幂/全同分/INT32_MIN/MAX/K=1 或 n/r=1..5 的 E7 矩阵未减少。输入份额测试种子由源码固定，公开 pivot seed 每例写入 E2E 原始日志；密码随机币不导出。E8 原始 stdout 留在 `experiments/m6a_p2_i_allpairs/TEST_ONLY/`，**不得提交**：

最终 `git diff --check` exit 0；E8 新增/修改的未跟踪头文件、测试和报告另用行尾空白扫描，零匹配。起点 E7 两份报告与三份原始日志的 SHA-256 复核不变；主工作区原有状态不变。

| 日志 | 本次 SHA-256 |
|---|---|
| `e8_clean_conformance_raw_2026-10-03.txt` | `9BA6C3183027C3B7B476934D068990284AC96217B47104B25F09ECDBE823DD13` |
| `e8_clean_differential_raw_2026-10-03.txt` | `6AD23770C3FE807FBDE262C99636031EC211D9A058A4F79A5DC731F91A0E596C` |
| `e8_clean_e2e_raw_2026-10-03.txt` | `B83C42EFC4262FD5E645616F15DAC53EA09B819659775148CD2BE3A6818C44B5` |

本次 E2E 50 例求和：每方预留 2730 key、活跃 661 边、score/CA/总 DCF Eval 为 1200/1322/2522，每方发送 52800 字节，每方离线包总字节 3140190；离线发材等待墙钟求和 174524559 ns、每例在线较慢 party 墙钟求和 127360501 ns、观测最大 party `ru_maxrss` 4352 KiB。逐例 r、D、pivot seed、包字节、活跃边 digest、真实轮数在日志中。T 峰值 RSS、PRG/AES 细分计数、LAN/WAN 正式计量均为 `NOT_MEASURED`。这些是小 D Debug 工程试运行，不能外推。

## 5. 未执行的中等 D 容量预检与最短路径

供真正独立窗口继续的**解析布局**（还未作为运行时预检代码）：uCMP 单方 blob 在 b 位的字节数为 `57+24b`；每条边外层 24 字节；两轮 score 每 D 端点共 `1796D` 字节；固定 header/计数器 114 字节，四个置换/向量及 r 个节点 mask 共 `D(48+8r)`。因此每方序列化包长度

```text
B(D,r) = 114 + D·(1844+8r) + r·D(D−1)/2·(81+24b),  b=33+log2 D.
```

此式在 E7/E8 D≤8 的实际包字节总和上复核一致；**解析预测不是大 D 实测或内存安全保证**。

| D, r=5 | 预留 key/方（解析） | 包字节/方（解析） | 本任务状态 |
|---:|---:|---:|---|
| 16 | 600 | 611658 | 未 keygen；独立门未过 |
| 32 | 2480 | 2523042 | 未 keygen；独立门未过 |
| 64 | 10080 | 10372050 | 未 keygen；独立门未过 |
| 128 | 40640 | 42547506 | 仅公式低于 64 MiB；双份在内存中的峰值未测，禁止据此直接运行 |

真正新窗口的最短路径：先以 E8 起点哈希独立复核并签收；在 `dealer_generate` **任何 keygen/`reserve` 前**加入溢出安全的 `B(D,r)`、槽数和内存预算预检；按 D16→32→64 有界运行，D128 在预检及实测资源允许后再尝试。每个规模先 conformance/differential，再 fork+exec E2E；记录每轮边、实际包字节、在线通信、DCF、时间和峰值。不得换固定 M、在线 T 或 Protocol III，也不得把解析容量当实测。

## 6. 本轮修改文件

E8 新增：本接收复核、本结果报告、三份 `e8_clean_*_raw_2026-10-03.txt`。E8 修改：`docs/IMPLEMENTATION_PLAN.md`、`VFSS/include/moe_topk/protocol_i_aav86_small.h`、三个 `VFSS/tests/moe_topk/protocol_i_aav86_small_*_test.cpp`。`VFSS/src/moe_topk/protocol_i_aav86_small.cpp` 与 `VFSS/CMakeLists.txt` 未由 E8 修改；它们仍是 E7 未跟踪/已修改起点。无提交、推送、合并或 PR。
