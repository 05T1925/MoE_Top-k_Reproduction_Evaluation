# M6A-P2-I-E8 异会话接收复核

日期：2026-10-03。接收者为负责规划和复检的当前聊天，未在 E6–E8 执行聊天中编写或修改运行时代码。本报告只接收下列精确源码状态；它不代表另一位研究人员的密码学同行评审。

## 结论

| 门 | 接收结果 |
|---|---|
| E7/E8 异会话技术接收 | **PASS_WITH_EXPLICIT_LIMITS**：E8 同聊天自审造成的程序性 FAIL 已由本次异会话复核补足；E8 原报告保留为历史记录。 |
| 小规模 Protocol I+AAV86 功能 | **PASS**：仅 `D≤8, 1≤r≤5` 的独立入口与已审测试矩阵。 |
| 单方半诚实安全 | **CONDITIONAL**：依赖 E7 明列的单份 DCF 选阈值隐私、独立 keygen 随机性、AES/PRG、可信非合谋离线 T 和私有完整通道；源码测试不证明这些密码学假设。公开 masked list、shuffled local rank、pivot、bucket、活跃图、帧长度和 abort 时点必须列入泄露口径。 |
| `D=16/32/64/128`、正式性能、Protocol III | **NOT_TESTED / NOT_MEASURED**；本次不批准将解析容量或 E8 小规模计量外推。 |
| 作者精确复现 | **NOT_PROVEN**；本实现是全两两预处理的项目扩展。 |

这项接收允许下一执行阶段先做 keygen 前容量与资源预检，再按有界规模逐级扩展。它不改变 M6A 整体未完成的状态，也不批准固定 M 或在线 T。

## 接收对象与工作区

隔离 worktree：`codex/m6a-p2-i-allpairs-experiment`，`HEAD=c3926c68fd14f270faa8b55234311071947fa080`。接收时已有 E1–E8 文档、实验文件和 E7/E8 源码差异均未暂存。本次新增本报告，并在隔离 worktree 的 `docs/IMPLEMENTATION_PLAN.md` 登记接收结果；没有改动 `VFSS/`、`VFSS-baseline/`、论文、参考树或主工作区。主工作区仍为 `feat/m6a-performance-evaluation@c3926c68`，原有文档差异和 `siamjdiscrmath.pdf` 保留。

接收源码 SHA-256 与 E8 验证报告登记值逐项一致：

| 文件 | SHA-256 |
|---|---|
| `VFSS/include/moe_topk/protocol_i_aav86_small.h` | `92ABFFED0FEF9D0F1CA393D9D1E9B607ECC4596DFEE41F1DE13F6BE3B5C50836` |
| `VFSS/src/moe_topk/protocol_i_aav86_small.cpp` | `296C25CADA425FDDA1A4ABF329FF9147B3732590E2AB0E993A4E07731DCBF85B` |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_conformance_test.cpp` | `D4DB5D01A5725244A993BEB49B629C0E81CAEED5FDDD40E4A85D71B6C9DDE120` |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_differential_test.cpp` | `218E3DD7B4A1642205C8229185147DF81D77EA1F3160491A2CEFC1EFB230AD4B` |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | `344CB6DE9726CF91F8B0241391B16C435C0392CC662D52A2923CA40E542D56DD` |

## 独立核验

1. 对照 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、E6/E7/E8 设计与复核报告及当前源码，确认 E8 的 key-blob 互换发现被如实收窄为结构校验边界。保持相同规格、交换 key blob 后，反序列化接受；测试路径用双份 dealer 材料检出错误数学关系。生产代码不声称 checksum 或外层标签能认证 key 与 mask 的关系。可信 T 与完整交付通道仍是必要假设。
2. 核对全池每轮 canonical pair 取材、首轮 mask 与前向 masked list 共用、同一隐藏置换的独立逆路由、在线阶段无 T，以及 `exchange_words` 的消息顺序。`2r+4` 是该独立 raw-score→原序 XOR mask 入口的已实现因果轮数；旧 Protocol I/III 入口不能继承该值。代码仍硬限制 `D≤8`，64 MiB 只在序列化时限制，故大 D 扩展必须先加入 keygen 前预检。
3. 在新的 WSL `Ubuntu-24.04` Debug 构建目录 `/tmp/m6a8-receiver-20261003` 配置并构建三项新路径目标与五项相关旧路径回归目标。配置参数为 `-DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF`。执行 `ctest --test-dir /tmp/m6a8-receiver-20261003 --output-on-failure -R 'moe_topk_m6a7_aav86_small_|moe_topk_m2_parallel_shuffle_(conformance|process_e2e)_test|moe_topk_m5_fix_f1_(raw_score_mask|process_e2e)_test|moe_topk_m5g_two_round_fsort_test'`，结果 **8/8 PASS、0 FAIL**。其中三项新路径 conformance、oracle differential、独立进程 E2E 分别通过。`git diff --check` 为 exit 0。

本次没有生成 D>8 key，没有复跑正式 LAN/WAN benchmark，也没有测试 T 峰值内存或 PRG/AES 细分。E8 记录的 D16/32/64/128 包字节是解析预测，不能作为资源实测。

## 下一阶段的前置条件

执行聊天可继续使用同一隔离 worktree。第一步在任何全池 `reserve`、keygen 或序列化前做 checked arithmetic 的槽数、包字节和保守进程内存预算预检；错误要在分配前有界失败。随后 D16→32→64 逐级进行 conformance、oracle differential 和 T/P0/P1 独立进程 E2E；D128 仅在预检和前一档真实资源计量允许时尝试。将已用活跃边与每方全池预留分别计量；实测与公式、Debug 与正式性能、协议核心与 raw-score 总入口分开记录。每次改动后重新核对源码身份；本报告的 PASS 不自动覆盖后续修改。
