# M2 Protocol I paper-compatible shuffle feasibility model reproduction

日期：2026-09-11
阶段三A复核结论：**algebraic ideal-function model = PASS**；阶段三B旧安全模型 Entry Audit：**ENTRY_BLOCKED**。Dealer-preprocessed project candidate 的当前状态见 `docs/decisions/M2_PROTOCOL_I_DEALER_PREPROCESSED_3ROUND_CANDIDATE.md`。

## 1. 基准、工作区和范围

- 仓库：`/Users/wentao.liu/Desktop/moe_plan`
- 阶段二基准：`66c944aac8793d795b73632cc6e5a46219d5b250`
- 分支：`codex/m2-paper-shuffle-feasibility-model`
- 执行开始时 HEAD：`66c944aac8793d795b73632cc6e5a46219d5b250`
- `origin/main`：`2a83b19dead1230a416fa093c8ca2983cdec0f8e`
- 阶段二输入中提到的 `docs/decisions/M2_M3_SHARED_CONTRACT.md` 不存在；实际冻结文件为 `docs/decisions/M2_M3_SHARED_CONTRACT_FREEZE.md`，本阶段按实际 canonical 文件复核。
- 阶段二输入中提到的 `Papers/PAPERS.sha256` 不存在；实际 manifest 为 `docs/PAPERS.sha256`。本次重新执行 `shasum -a 256 -c docs/PAPERS.sha256`，manifest 中 9/9 项均为 `OK`。相关 Agarwal PDF hash 为 `18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c`，Chase shuffle PDF hash 为 `6112f7116ec3d3b100fbb5ca10f058a6a0e3f19f165c5c6a071a48b10c0c48ab`。

本阶段只增加隔离的 `TEST_ONLY` algebra/view model、最小 CTest 注册、设计门文档和本记录。没有修改 M2 现有 8-round secure runtime、M3/M4/M5、共享输入输出契约、基线或本地参考工程。阶段三B复核没有因为模型测试通过而新增 secure candidate。

## 2. 设计和证据边界

设计 owner 为 [`M2_PROTOCOL_I_PAPER_SHUFFLE_FEASIBILITY_MODEL.md`](../decisions/M2_PROTOCOL_I_PAPER_SHUFFLE_FEASIBILITY_MODEL.md)。证据分层如下：

- A：已校验的 Agarwal Protocol I 与 Chase shuffle PDF 中的论文定义；
- B：本地参考工程或现有 VFSS 的静态行为；
- C：项目冻结的 Q20.12、stable tie、logical/padded、原顺序 mask 等工程契约；
- D：本阶段候选模型的代数、三方视图和 causal transcript，不能反写成论文或安全结论。

阶段二的三个 BLOCKED 根因在本模型的理想代数层面成立，但没有闭合真实 secure 入口：

1. `same-permutation algebra`：PASS（仅理想函数和测试 oracle）；当前 PS API 没有 public-list/r/GRank binding 输出。
2. `correlated r/GRank relation`：PASS（仅理想等式）；当前 `ProtocolIUcmpMaterial` 需要完整 `mask_left/mask_right`，没有合格的分布式 offline 生成。
3. `causal transcript`：候选设计成立，但 executable transcript BLOCKED；当前代码仍有独立 masked-key exchange 与 rank reveal，3 barrier 不能从候选表格直接推出。

## 3. TEST_ONLY 模型

文件：[`protocol_i_paper_shuffle_candidate_model_test.cpp`](../../VFSS/tests/moe_topk/protocol_i_paper_shuffle_candidate_model_test.cpp)。

模型使用标准 C++ 独立编译，不链接生产库；CTest target 也没有 `target_link_libraries`，因此没有进入正式 M2 executable、secure library 或 M3 runtime。模型只在测试目标内允许 oracle reconstruction，并明确执行以下信号。这些是 algebraic ideal-function evidence，不是 secure candidate evidence：

- public masked list 与 secret shuffled payload 的复合 permutation 一致；
- public list 中的 `r` 与 GRank local material 逐槽一致；
- 模型对象中的 P0/P1 不持有完整 `r` 或完整复合 permutation，P2 不接触 input、score、key、rank 或完整对象；这不是实际进程视图证明；
- permutation 方向、logical/padded 边界、非二次幂输入、duplicate score 的 stable priority-key；
- 错 permutation、错 r correlation、错 slot metadata、错 shape 和 material reuse 必须失败；
- 候选设计目标计数为 `shuffle 2 + rank 1 = 3`，raw-score candidate path 计数为 `carry 1 + sign 1 + core 3 + reverse/output 2 = 7`；没有实际 transport trace 支撑该候选轮数。

此外，模型的 `padded_n` 对 `logical_n=1` 使用 1，而当前 `protocol_i_make_input_layout` 从 `padded_n=2` 开始；模型的 `key_bits` 也不能直接代替当前项目的 `comparison_bits = 32 + index_bits + 1`。项目级 width/layout conformance 尚未完成。

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

## 7. 阶段三A复核与阶段三B入口结论

| 条件 | 结果 | 证据 |
| --- | --- | --- |
| algebraic ideal-function model | PASS | 设计文档第 2/4 节、candidate model 54 cases |
| distributed secure construction | BLOCKED | 当前 PS 没有 public-list/r/GRank 真实输出契约 |
| executable causal transcript | BLOCKED | 当前 pipeline 仍有 masked-key exchange 和 rank reveal |
| GRank/FSS correlation generation | BLOCKED | uCMP 构造需要完整 mask pair，无分布式生成原语 |
| project width/layout conformance | BLOCKED | `logical_n=1` 的 padding 和 comparison width 与模型不同 |
| TEST_ONLY 关键不变量 | PASS | direct compile、定向 CTest、历史全量 CTest |

因此阶段三B Entry Audit 结论为 **ENTRY_BLOCKED**，不创建 secure candidate 空壳或虚假 executable。阶段三A的 25/25 只作为模型自身的历史证据，不作为本阶段修改后的 secure 通过证据。

不得由本记录推出以下结论：已经实现 Agarwal Protocol I、已经完成论文精确三轮核心、已经完成 secure 三轮协议、已经完成 Protocol I reproduction、已经证明论文泄露完全一致，或已经通过 exact label gate。

## 8. 未决项和后续入口

如需重新进入 Gate B，最小前置项是：给出不掌握完整 `r` 的真实 correlated uCMP/FSS material 生成；为 R1/R2 给出基于实际 PS/OT/Share Translation 的逐槽消息代数；实现可审计的 public list、secret payload 与 GRank 同槽绑定；按实际 send/receive trace 证明最多三个 online barriers；并完成项目 width/layout conformance。当前 8-round 工程路径及其标签继续保留，不因阶段三A模型通过而改名或替换。
