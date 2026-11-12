# S65 学姐代码审查交接（一页）

**固定实验原型**：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`。运行时代码来自远端 `m6b-i-project-select5-s64@8debdcfa8e03527decaea13b956e3e18cf941ccd`；本 S65 分支只增加交接文档。该算法借鉴 BB90 的研究思路，实现同一类 Top-K mask 功能，但不是 BB90 五迭代原算法或 BMW16 实现。

## 实际路径与角色

双方各持 signed Q20.12 分数的算术输入份额 → [输入转换、GMW/Beaver](../../VFSS/src/moe_topk/score_to_mask_v3_party.cpp) → [项目自有五迭代 Select](../../VFSS/src/moe_topk/project_derived_select5.cpp) → 同分按**原始索引升序**确定稳定次序 → 逆路由、输出刷新，得到原输入顺序的两份 XOR Top-K bit-mask → [双方分别加密提交](../../VFSS/src/moe_topk/score_to_mask_v3_party.cpp) → [托管服务待两份密文齐备后共同提交](../../VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp) → 双方分别认证取回并解密**本方**份额。两个 API 可以在不同时间返回。

| 角色 | 持有或可见 |
|---|---|
| 离线可信发材方 T | 生成相关 Beaver triples、置换材料、双方签名 package 与仅供托管方的 metadata-only context；发材后退出。 |
| 在线计算方 P0/P1 | 各自的输入份额、材料包、GMW 转录、进程内短暂存在的本方输出份额；各自只持本方收件人私钥。 |
| 在线托管方 A | 已签 context、会话/身份/交易元数据、两份**密文**、到达次序、PENDING/COMMITTED 状态与 receipt；不持双方解密密钥。 |

五次**算法迭代**是 `project_derived_select5.cpp:369` 的 `iteration=0..4` 循环：前四次采样、比较与缩减，末次对剩余候选作精确比较（同文件 `:382-407`）。在线通信另有 `score_to_mask_v3_party.cpp:25-31` 的 READY、Beaver 批次、采样 seed/ACK、公开活动比较结果、输出刷新等消息，之后还有 A 的 submit/receipt/fetch。S64 同一固定输入实测的 party exchange phase：N=128/K=64 为 **88**，N=256/K=128 为 **89**，N=1000/K=500 为 **106**；来源是 S64 逐案收据引用的 `final-n1000.log`（SHA-256 `3ad62b50…0cc2ee`），源码对 N=1000 的上界为 106。五次算法迭代不是五轮网络通信，phase 也不是实测 RTT。

## 从远端干净检出并最小运行（Ubuntu 24.04 x86_64）

依赖：`git cmake ninja-build g++ libeigen3-dev libssl-dev libsqlite3-dev`（须先安装）。以下命令从仓库根目录执行；构建及临时密钥/证书/材料只在 `/tmp`，E2E 驱动自行启动 Dealer、P0、P1、A 四类独立进程。

```bash
git clone --branch codex/m6b-i-project-select5-s65 --single-branch \
  https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation.git s65-review
cd s65-review
git rev-parse HEAD  # 与交接记录中的 S65 提交比对；其父级须为 8debdcfa...
cmake -S VFSS -B /tmp/select5-s65-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL=ON
cmake --build /tmp/select5-s65-build --target \
  moe_topk_score_to_mask_v3_runtime_conformance_test \
  moe_topk_score_to_mask_v3_ft_v3_bounded_conformance_test \
  moe_topk_score_to_mask_v3_ft_v3_process_e2e_test -j 4
/tmp/select5-s65-build/moe_topk_score_to_mask_v3_runtime_conformance_test
/tmp/select5-s65-build/moe_topk_score_to_mask_v3_ft_v3_bounded_conformance_test
/tmp/select5-s65-build/moe_topk_score_to_mask_v3_ft_v3_process_e2e_test --ft-v3-n1000
```

最后一条运行固定分数 `score[i]`（`i mod 10` 为 `0..9`）=`[INT32_MAX,INT32_MIN,-1,0,17,INT32_MAX,-17,4096,-4096,17]`，覆盖极值、负值和同分。它运行 N=128/K=64、N=256/K=128、N=1000/K={1,8,500,999,1000} 及 N=1/K=1，共 8 案。前三个代表规模的预期 mask 均为 `mask[i]=1` 当且仅当 `i mod 10 ∈ {0,4,5,7,9}`；N=1000/K=1 预期仅索引 0 为 1。每案按 `mask[i]=P0_share[i] XOR P1_share[i]` 重构，逐方 share 必须都是 bit，重构值与冻结 C++ oracle 逐位一致且恰有 K 个 1。重构只在 TEST_ONLY 驱动进行；正常运行双方各自取得自己的 share。

如需脱离完整仓库转交**最小可构建源码包**，在已检出的固定 S65 提交上执行：

```bash
git -c core.autocrlf=false archive --format=tar.gz -o select5-s65-source.tar.gz HEAD \
  VFSS docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S65_REVIEW_HANDOFF.md \
  docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_CASES_2026-11-09.json \
  docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-11-09.json
```

包内 `VFSS/` 是该仓库的自足 CMake 构建单元，含本路线及其依赖源码/TEST_ONLY 驱动；不含冻结的 `VFSS-baseline/`。显式关闭 Git 行尾转换，使包内源码字节与 S64 Git blob 清单一致；压缩包本身只在本地生成，不加入 Git。

## 阅读入口、证据和待审点

- 开关与测试目标：[VFSS/CMakeLists.txt](../../VFSS/CMakeLists.txt)；opt-in 头文件：[score_to_mask_v3_gmw.h](../../VFSS/include/moe_topk/score_to_mask_v3_gmw.h)；Dealer 有界采样/签名包：[score_to_mask_v3_gmw.cpp](../../VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp)；上述 Select/party/escrow；冻结 oracle 与独立进程驱动：[score_to_mask_v3_process_e2e_test.cpp](../../VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp)。默认构建关闭实验入口；显式同时开启 GMW-v3、receipt/v2、FTv3 才进入本路径。
- [S64 逐案 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_CASES_2026-11-09.json)：远端源码 fresh Release **28/28 达预期**，26 案得到 share 并匹配 oracle，2 案为人工注入的预期 no-share；本次最小命令只复核其中 8 案，不把继承结果写为本次重跑。N=1000 只证明列出的有限样例通过，不证明所有输入。默认关闭/显式开启、二进制哈希、环境见 [S64 来源清单](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-11-09.json)。
- S63 的 `N≤1000` 单方成功应用/API 视图仅在**理想** FTv3、理想 A token/输出刷新、诚实对端因果执行、静态半诚实腐化与外生 SuccessFence 下有条件结论。真实部署随机源、签名/存储、TLS、收件人加密、协议中止和实际有限超时仍开放；正式 LAN/WAN、RTT 和跨方案性能均 `NOT_MEASURED`，不得称为安全通过。

**请学姐审查三点**：①这套项目自有五迭代定义与稳定、原序 Top-K mask 输出是否符合课题目标？②新增在线托管方只存密文、双方提交后分别取回份额的角色与流程是否可接受？③后续比较是否把它列为独立实验项，而非 BB90 原协议结果？

**可直接转发**：学姐您好，项目自有五迭代 Select 的实验代码和一页运行说明已放在独立 S65 分支。说明内有干净检出、构建及 8 个代表样例的命令，S64 另有 28 案独立功能收据。请重点看算法定义与稳定 mask、托管角色，以及后续是否作为独立实验项。当前不主张完整安全或正式性能结论。
