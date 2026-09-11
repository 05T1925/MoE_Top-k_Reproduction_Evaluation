# M2 Protocol I paper-compatible shuffle feasibility model reproduction

日期：2026-09-11
结论：**FEASIBILITY_GO（仅 Gate A；不代表 secure candidate 或论文精确实现）**

## 1. 基准、工作区和范围

- 仓库：`/Users/wentao.liu/Desktop/moe_plan`
- 阶段二基准：`66c944aac8793d795b73632cc6e5a46219d5b250`
- 分支：`codex/m2-paper-shuffle-feasibility-model`
- 执行开始时 HEAD：`66c944aac8793d795b73632cc6e5a46219d5b250`
- `origin/main`：`2a83b19dead1230a416fa093c8ca2983cdec0f8e`
- 阶段二输入中提到的 `docs/decisions/M2_M3_SHARED_CONTRACT.md` 不存在；实际冻结文件为 `docs/decisions/M2_M3_SHARED_CONTRACT_FREEZE.md`，本阶段按实际 canonical 文件复核。
- 阶段二输入中提到的 `Papers/PAPERS.sha256` 不存在；实际 manifest 为 `docs/PAPERS.sha256`。本次重新执行 `shasum -a 256 -c docs/PAPERS.sha256`，manifest 中 9/9 项均为 `OK`。相关 Agarwal PDF hash 为 `18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c`，Chase shuffle PDF hash 为 `6112f7116ec3d3b100fbb5ca10f058a6a0e3f19f165c5c6a071a48b10c0c48ab`。

本阶段只增加隔离的 `TEST_ONLY` algebra/view model、最小 CTest 注册、设计门文档和本记录。没有修改 M2 现有 8-round secure runtime、M3/M4/M5、共享输入输出契约、基线或本地参考工程。

## 2. 设计和证据边界

设计 owner 为 [`M2_PROTOCOL_I_PAPER_SHUFFLE_FEASIBILITY_MODEL.md`](../decisions/M2_PROTOCOL_I_PAPER_SHUFFLE_FEASIBILITY_MODEL.md)。证据分层如下：

- A：已校验的 Agarwal Protocol I 与 Chase shuffle PDF 中的论文定义；
- B：本地参考工程或现有 VFSS 的静态行为；
- C：项目冻结的 Q20.12、stable tie、logical/padded、原顺序 mask 等工程契约；
- D：本阶段候选模型的代数、三方视图和 causal transcript，不能反写成论文或安全结论。

阶段二的三个 BLOCKED 根因在本模型层面闭合：

1. 使用 `Perm(p, v)[slot] = v[p[slot]]` 固定方向，并验证先 `p0`、后 `p1` 等价于 `pi[slot] = p0[p1[slot]]`；public masked list、secret payload 和 GRank mask 使用同一 slot 对齐。
2. `r = r0 + r1 mod 2^w`，P0/P1 只持有自己的 share，GRank share 直接绑定同一 `r0/r1`；P2 只得到 shape/handle metadata 并在 online 前退出。
3. transcript 明确为 O0 offline package shape、R1 forward local-share bundle、R2 final public-mask/payload shares、R3 rank-share bundle。候选 shuffle 为 2 个 online barriers，和 rank 一轮组合为 3 个 core barriers，没有隐藏第四轮。

## 3. TEST_ONLY 模型

文件：[`protocol_i_paper_shuffle_candidate_model_test.cpp`](../../VFSS/tests/moe_topk/protocol_i_paper_shuffle_candidate_model_test.cpp)。

模型使用标准 C++ 独立编译，不链接生产库；CTest target 也没有 `target_link_libraries`，因此没有进入正式 M2 executable、secure library 或 M3 runtime。模型只在测试目标内允许 oracle reconstruction，并明确执行以下信号：

- public masked list 与 secret shuffled payload 的复合 permutation 一致；
- public list 中的 `r` 与 GRank local material 逐槽一致；
- P0/P1 不持有完整 `r` 或完整复合 permutation，P2 不接触 input、score、key、rank 或完整对象；
- permutation 方向、logical/padded 边界、非二次幂输入、duplicate score 的 stable priority-key；
- 错 permutation、错 r correlation、错 slot metadata、错 shape 和 material reuse 必须失败；
- priority core 计数为 `shuffle 2 + rank 1 = 3`，raw-score candidate path 计数为 `carry 1 + sign 1 + core 3 + reverse/output 2 = 7`。

当前仓库未发现 `VFSS/tests/moe_topk/public_masked_shuffle.py`；已有 reverse-shuffle 测试不包含本阶段的 public-list/GRank correlation/P2 view/causal round 集合，因此本模型提供独立信号，而不是复制已有 functional adapter。

模型不能证明密码学安全、semi-honest simulator、泄露等价、实际独立进程的 offline/online 安全边界、论文 full version 的未公开消息流程或论文精确标签。

## 4. 验证环境

```text
Darwin 24.6.0 x86_64
Apple clang 17.0.0
cmake 4.4.3
ctest 4.4.3
```

默认 CMake 配置首先命中了 `/usr/local/share/eigen3/cmake/Eigen3Config.cmake` 中不兼容的 Eigen 5.0.1，因此停止并改用环境中已有的 Eigen 3.4.1 配置；没有安装依赖、修改版本要求或改变仓库配置。这是环境路径问题，不是候选模型编译错误。

## 5. 执行命令和结果

### 5.1 候选模型直接编译

```bash
MODEL_BUILD="$(mktemp -d /tmp/moe-stage3a-model.XXXXXX)"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  VFSS/tests/moe_topk/protocol_i_paper_shuffle_candidate_model_test.cpp \
  -o "$MODEL_BUILD/moe_topk_m2_paper_shuffle_candidate_model_test"
"$MODEL_BUILD/moe_topk_m2_paper_shuffle_candidate_model_test"
```

结果：通过，输出 `m2_protocol_i_paper_compatible_candidate_model cases=54`。

### 5.2 全新 CMake/CTest build

实际使用全新目录 `/tmp/moe-stage3a-build.3QRsti`，并关闭 EMP-OT：

```bash
cmake -S VFSS -B /tmp/moe-stage3a-build.3QRsti \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
cmake --build /tmp/moe-stage3a-build.3QRsti --target \
  moe_topk_m2_paper_shuffle_candidate_model_test --parallel 2
ctest --test-dir /tmp/moe-stage3a-build.3QRsti \
  -R '^moe_topk_m2_paper_shuffle_candidate_model_test$' \
  --output-on-failure
```

结果：候选 target 构建通过；定向 CTest `1/1` 通过，耗时约 `0.02 sec`。

### 5.3 当前 M2/M3 注册回归

```bash
ctest --test-dir /tmp/moe-stage3a-build.3QRsti \
  -R '^(moe_topk_dpf_conformance_test|moe_topk_m[23]_.*)$' \
  --output-on-failure
```

结果：`20/20` 通过，耗时约 `207.50 sec`。覆盖 DPF conformance、M2 既有 conformance/process-E2E、M3 secure/raw-score/metrics 及新候选模型。

### 5.4 完整 CTest

```bash
ctest --test-dir /tmp/moe-stage3a-build.3QRsti --output-on-failure
```

结果：`25/25` 通过，耗时 `199.07 sec`。

### 5.5 EMP 边界

- EMP-OFF：本记录中的 CMake、构建、候选定向、M2/M3 回归和完整 CTest 均在 `-DMOE_TOPK_ENABLE_EMP_OT=OFF` 下完成。
- EMP-ON：`NOT_MEASURED`。本阶段没有把不可用的 EMP-OT 环境伪装成通过，也没有借用阶段一或阶段二结果。

## 6. 安全和回归检查

- `git diff --check`：通过。
- 新 target 仅注册到测试集合，没有生产库或正式 M2/M3 executable 链接。
- secure runtime/source/include 中没有候选模型符号或测试 reconstruction 函数接入。
- 候选文件没有文件轮询、`sleep`、socket、在线 Dealer、旧 FSS ABI 或在线密钥生成。
- `VFSS-baseline/`、`Papers/`、`Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/` 没有进入差异；构建目录、日志和密钥没有进入版本控制。
- 现有 8-round M2 路径、M3 正式实现、共享契约和 MetricsRecord 统计口径没有修改。

## 7. Gate 结论

| Gate A 条件 | 结果 | 证据 |
| --- | --- | --- |
| 代数、ring/width、padding、score/key | PASS | 设计文档第 2 节、候选模型 54 cases |
| same-permutation proof obligation | PASS | 设计文档第 4 节、staged/direct permutation assertions |
| correlated r/GRank material | PASS | 设计文档第 2.4/3 节、correlation assertions |
| P0/P1/P2 三方视图 | PASS | 设计文档第 3 节、view-shape assertions |
| input-independent offline/P2 boundary | PASS | P2 view 与构造顺序检查 |
| complete causal transcript | PASS | 设计文档第 5 节、O0/R1/R2/R3 表 |
| 无隐藏额外 online barrier | PASS | shuffle 2 + rank 1 的轮次检查 |
| TEST_ONLY 关键不变量 | PASS | direct compile、定向 CTest、完整回归 |

因此本阶段结论为 **FEASIBILITY_GO**，只表示可以进入下一阶段的隔离 secure candidate 实现门（Gate B）。Gate B 尚未执行；Gate C 尚未执行。

不得由本记录推出以下结论：已经实现 Agarwal Protocol I、已经完成论文精确三轮核心、已经完成 secure 三轮协议、已经完成 Protocol I reproduction、已经证明论文泄露完全一致，或已经通过 exact label gate。

## 8. 未决项和后续入口

下一阶段只能另行实现隔离 secure candidate，并至少补齐独立进程 P2/P0/P1、真实 offline/online boundary、primitive conformance、oracle differential、independent-process E2E、secure path 无明文重构、无在线 Dealer 和无文件同步。当前 8-round 工程路径及其标签继续保留，不因本阶段的 FEASIBILITY_GO 改名或替换。
