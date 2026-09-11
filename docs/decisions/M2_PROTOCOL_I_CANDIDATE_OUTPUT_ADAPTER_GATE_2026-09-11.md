# M2 Protocol I candidate runtime and output-adapter entry gate

日期：2026-09-11

阶段三B基准：`0d14b5433bf855346905741e0e6f2eb7de6509c0`

状态：**ENVIRONMENT_BLOCKED**

本记录只负责阶段三C的验证门和 adapter 入口门，不新增 adapter 实现。当前结论为：

- `CORE_RUNTIME_BLOCKED`：阶段三B只有 source-authored evidence，真实 EMP-ON 编译、链接、运行尚未测量；
- `ADAPTER_ENTRY_BLOCKED`：candidate 输出是 shuffled-domain additive rank shares，不是 selection carrier shares，且 core 未达到 `CORE_RUNTIME_GO`；
- 不允许输出 `SECURE_CANDIDATE_GO`、7-round 或完整 M2 unified path。

## 1. 阶段三B证据口径

| 证据项 | 当前状态 | 说明 |
| --- | --- | --- |
| candidate source authored | PASS | 独立 package/core/app/TEST_ONLY controller 已存在 |
| real dependency compilation | NOT_MEASURED | EMP-ON graph 未生成 |
| real link | NOT_MEASURED | 未到达链接阶段 |
| runtime conformance | NOT_MEASURED | 未运行 candidate CTest |
| public-y/correlated-r differential | NOT_MEASURED | TEST_ONLY 代码存在但未执行 |
| fork+exec E2E | NOT_MEASURED | Dealer/P0/P1 未启动 |
| three-round runtime trace | NOT_MEASURED | 没有真实 R1/R2/R3 counter |
| secure candidate accepted | BLOCKED | 缺少上述运行证据 |

OpenSSL declaration stub、单文件 syntax check 和 EMP-OFF 旧回归只能说明静态边界或
既有路径状态，不能升级为 candidate 编译、链接或运行证据。

## 2. 环境门结果

当前执行面为 macOS Darwin 24.6.0、AppleClang 17、CMake 4.4.3。只发现 Eigen
3.4.1 配置，没有发现：

- `emp-tool` CMake package；
- `emp-ot` CMake package；
- `*/include/openssl/crypto.h`。

fresh EMP-ON 配置命令：

```text
cmake -S VFSS -B <build-on> -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
```

实际在 `VFSS/CMakeLists.txt:279` 的
`find_package(emp-tool 1.0 CONFIG REQUIRED)` 失败。没有未经授权安装系统包、
Homebrew package 或修改全局开发环境。

## 3. Rank share 与 selection carrier 入口审查

阶段三B secure core 的输出字段是：

```text
public_masked_list
shuffled_rank_share
```

它没有输出：

```text
selection_bit_share[i] = [rank[i] < K]
```

`rank_share[i]` 到 `selection_bit_share[i]` 是非线性转换，不能通过单方判断、截断、
低位检查、P0 单方赋值、打开 rank 不计轮数、Top-K oracle 或事后修正伪造。两轮
reverse shuffle 只能恢复顺序，不能免费完成 rank-to-bit。

因此当前没有满足 adapter 输入契约的 secure shuffled selection carrier shares。

## 4. 路线审查

### 路线A：公开 shuffled rank

如果后续明确接受公开 rank 泄露，需要新增一轮 rank-share exchange/reconstruction，
再形成 public shuffled carrier，然后执行两轮 reverse shuffle：

```text
raw-score adapter 2
+ candidate rank-share core 3
+ rank reveal 1
+ reverse shuffle 2
= 8 online rounds
```

该路线不能标为 7 轮，也不能称论文精确改进。当前阶段未实现。

### 路线B：安全 rank-to-bit

当前没有已验证的真实 primitive、offline material、party-local 输入契约、消息流或
selection conformance。若新增比较或 opening，必须按真实 transport trace 增加 causal
barrier。当前状态：`BLOCKED`。

### 路线C：真正的 7-round path

要求三轮 core 本身在结束时已经输出 selection carrier，且不能把当前 R3 masked-list
opening 同时当作 rank reveal。现有阶段三B输出不满足该条件，必须重新设计 modified
shuffle/ranking core。当前状态：`BLOCKED`。

## 5. Adapter Entry Gate

| 条件 | 状态 | 原因 |
| --- | --- | --- |
| `CORE_RUNTIME_GO` | BLOCKED | EMP-ON 缺失 |
| secure shuffled selection carrier 输入 | BLOCKED | 当前只有 rank shares |
| 无额外 rank reveal/comparison barrier | BLOCKED | 尚无 rank-to-bit primitive |
| 两轮 reverse 输入输出契约 | NOT_STARTED | 入口未通过，不创建空壳 |
| logical/padded/dummy 语义 | NOT_STARTED | 入口未通过 |
| logical_n 长度 XOR Top-K mask | NOT_STARTED | 没有 carrier 输入 |
| 不修改正式 M2/M3 | PASS | 本阶段未修改 secure runtime |

最终 adapter 结论：**ADAPTER_ENTRY_BLOCKED**。本阶段不新增
`protocol_i_dealer_candidate_output_adapter.*`，不新增 full-path test，不把
TEST_ONLY carrier 接入 secure party。

## 6. Ubuntu 验证入口

在已具备 EMP-ON pinned 依赖、OpenSSL development headers/library、Eigen 3.4.x 的
Ubuntu 24.04 环境中，执行以下命令；不得用安装动作替代证据：

```bash
stage3c_build="$(mktemp -d /tmp/moe-stage3c-build.XXXXXX)"
cmake -S VFSS -B "$stage3c_build" -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/path/to/eigen-3.4.x/share/eigen3/cmake
cmake --build "$stage3c_build" -j2 \
  --target moe_topk_m2_dealer_preprocessed_3round_candidate_core \
  moe_topk_m2_dealer_preprocessed_3round_candidate \
  moe_topk_m2_dealer_preprocessed_3round_candidate_test
ctest --test-dir "$stage3c_build" -R moe_topk_m2_dealer_preprocessed_3round_candidate_test \
  --output-on-failure
ctest --test-dir "$stage3c_build" -R 'moe_topk_m2_|moe_topk_m3_|moe_topk_dpf_' \
  --output-on-failure
```

必须留存 candidate package/conformance、correlated-r/public-y differential、
fork+exec、peer-exit/timeout、FD lifecycle、R1/R2/R3 trace、实际轮数和原始计数。
只有这些证据全部通过，才可重新审查 `CORE_RUNTIME_GO`；之后仍需先解决
rank-to-selection carrier，才能重新审查 `ADAPTER_ENTRY_GO`。

## 7. 允许后续继续的边界

当前 M2 正式路径
`m2_protocol_i_raw_score_input_modular_8round_mask_output`、M3 主线和后续 M4/M5
不受本阶段阻塞影响。论文 exact 复现仍为 `NOT_VERIFIED/BLOCKED`。本阶段只关闭
证据和入口门，不改变任何正式路径的标签或轮数。
