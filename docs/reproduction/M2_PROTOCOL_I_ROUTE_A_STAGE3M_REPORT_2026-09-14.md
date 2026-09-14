# 阶段三M：M2 Route A 最终证据与 M2→M3 交接复检报告

日期：2026-09-14
分支：`codex/m2-candidate-ubuntu-validation`
证据级别：C 级项目工程扩展、当前 revision fresh Ubuntu/EMP 实测；不提升为论文结论。

## 1. Executive conclusion

阶段三M完成了 M2 Route A 的最终证据收口、阶段三L provenance 修正、M2→M3
交接复检和当前 revision 的 fresh 回归验收。

最终结论：

- M2 formal engineering baseline：`GO`，标签和 8 轮口径保持不变；
- M2 三轮 rank-share candidate：`GO`，独立进程和 rank differential 通过；输出仍是
  shuffled-domain rank share，不是完整 mask；
- Priority-key Route A：`GO`，独立 Dealer/P0/P1 进程、大域矩阵、6 轮 trace 和
  logical-domain XOR mask 通过；这是项目扩展；
- Raw-score Route A：`GO`，signed Q20.12 输入适配、独立进程、8 轮 trace 和
  logical-domain XOR mask 通过；这是项目扩展；
- M3 `agarwal_protocol_iii_modular_3round`：`GO`，当前 revision fresh EMP-ON
  11/11 回归通过，M2 没有改变 M3 secure runtime 或契约；
- `agarwal_protocol_i_exact_mask_output`：继续 `BLOCKED / NOT_VERIFIED`；
- 论文原生 3-round core 和论文/项目 7-round total path：没有实现或证明；
- formal performance benchmark、网络 RTT/带宽、在线 PRG 总数：`NOT_MEASURED`。

本阶段发现并修复了一个仅存在于 TEST_ONLY 独立进程 harness 的大包编排问题：当
`n=31` 时 Dealer package 写入 socket 缓冲会阻塞，而旧测试先等待 Dealer 退出，导致
150 秒超时。现已改为 Dealer 与两方并发启动后再等待 Dealer；修复后大域矩阵和完整
CTest 均通过。该修复没有改变 secure runtime、协议消息、轮数或泄露契约。

## 2. Git provenance

### 2.1 当前最终 revision

- Branch：`codex/m2-candidate-ubuntu-validation`
- 阶段三M开始时 HEAD：`71c161290d6627e8a9521a8733e6061b9afed861`
- 阶段三M开始时工作区：仅有本阶段待提交的测试/文档修改；`VFSS-baseline/` 和参考目录无差异。
- 基础提交主题：`feat(m2): complete Route A mask output paths`

阶段三M代码/证据提交已创建为
`fc149c2e33df53a272208622da925d0047303121`（`feat(m2): close Route A stage3M evidence`）。
该提交包含本阶段矩阵修复、阶段三L provenance 修正和本报告初版；随后仅文档的
closeout commit 只会更新本报告中的提交索引，不改变代码或测试证据。

### 2.2 阶段三L 的 `f752a77` 修正

阶段三L 报告原文把 `f752a77` 写成 Final verification revision。审计发现：

- `f752a771e33e8da274e369e9e67695e50cfc676a` 是 dangling commit；
- 它的父提交是 `98be22fe52a251f9a6824b59fd35aa91fc349412`；
- 当前 HEAD `71c161290d6627e8a9521a8733e6061b9afed861` 具有同一父提交、同一作者/时间和同一主题，
  是 amend 后保留的最终 commit；
- 因此 `f752a77` 仅是阶段三L内部验证/历史 amend 前 revision，不是当前最终 revision。

阶段三L报告已将 Final verification revision 改为完整 `71c161290d6627e8a9521a8733e6061b9afed861`，
并保留上述关系说明；不再同时使用两个无法解释的“最终 revision”。

### 2.3 历史报告交叉核对

- 阶段三K报告的 fresh build、18/18 M2 子集和 EMP-ON production build 与其记录的
  `/tmp/moe-stage3k-on`、`/tmp/moe-stage3k-off` 目录一致；它是历史证据，不替代本阶段 fresh 结果。
- 阶段三L报告记录的 34/34 与 `/tmp/moe-stage3l-final`、`/tmp/moe-stage3l-committed`
  一致，但其 revision 字段已按上节修正。
- 本阶段所有新结论绑定 `/tmp/moe-stage3m-emp-on`、`/tmp/moe-stage3m-emp-off` 和
  `/tmp/moe-stage3m-emp-on-prod`，不借用旧 build cache 或旧 CTest 结果。

### 2.4 工作区与产物审计

- 仓库内没有新增构建目录、日志、socket、密钥、`.bin` 材料或参考工程副本；
- 所有 build/log 证据均位于 Ubuntu `/tmp` 临时目录；
- `git diff --check`：`PASS`；
- `VFSS-baseline/`、`Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/`、`Papers/`：未修改。

## 3. 环境与命令

运行环境为 Ubuntu 24.04.4 LTS WSL2，x86_64，CPU 为 Intel Core i9-13980HX，
内存约 7.9 GiB 可见，当前 shell `RLIMIT_NOFILE=10240`。

工具和依赖：

- GCC/G++ 13.3.0；
- CMake 3.28.3；CTest 3.28.3；Ninja 1.11.1；
- Eigen3 CMake config：`/usr/share/eigen3/cmake/Eigen3Config.cmake`；
- OpenSSL 3.0.13；
- emp-tool CMake package：`/tmp/moe_m28_emp.ok9WzQ/prefix/lib/cmake/emp-tool/emp-tool-config.cmake`，
  package version `1.0.0`；
- emp-ot CMake package：`/tmp/moe_m28_emp.ok9WzQ/prefix/lib/cmake/emp-ot/emp-ot-config.cmake`，
  package version `1.0.0`；
- EMP archive SHA256：
  `emp-tool.tar.gz = 7f4a2cb169ba0b7fc48ffe89b7615288c41f9377bb0a4d56a3178fe20b66ab46`；
  `emp-ot.tar.gz = 8cfffc340a2014e5ac3b90c6659c0a812a6748e7e24c6ea2b0f5b3e58ba66183`。

EMP-ON fresh configure/build：

```text
cmake -S VFSS -B /tmp/moe-stage3m-emp-on -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/share/eigen3/cmake \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe-stage3m-emp-on --parallel 2
```

EMP-OFF fresh production configure/build：

```text
cmake -S VFSS -B /tmp/moe-stage3m-emp-off -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DEigen3_DIR=/usr/share/eigen3/cmake
cmake --build /tmp/moe-stage3m-emp-off --parallel 2
```

另有 EMP-ON `BUILD_TESTING=OFF` production graph：

```text
cmake -S VFSS -B /tmp/moe-stage3m-emp-on-prod -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/share/eigen3/cmake \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe-stage3m-emp-on-prod \
  --target moe_topk_m2_dealer_preprocessed_3round_candidate --parallel 2
```

## 4. M2 五类状态

| 项目 | 标签 | 输入/输出 | 在线因果轮数 | 阶段三M状态 |
|---|---|---|---:|---|
| M2 formal engineering baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | raw-score shares → original-order XOR mask | 8 | GO，保持原标签 |
| M2 3-round candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | padded priority-key shares → shuffled rank shares | 3 | GO，仍非完整 mask |
| Priority Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` | padded priority-key shares → logical_n XOR mask | 6 | GO，项目扩展 |
| Raw-score Route A | `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | signed Q20.12 raw shares → logical_n XOR mask | 8 | GO，项目扩展 |
| M2 paper-exact target | `agarwal_protocol_i_exact_mask_output` | 论文精确输入/输出 | 3 target | BLOCKED / NOT_VERIFIED |

形式化基线没有被 Route A 改名或改轮数；三轮 candidate 没有被写成 mask-output；Route A
的额外 rank reveal 和 reverse shuffle 均保持显式计数。

## 5. M2 Route A 实现与输出审计

### 5.1 Priority-key Route A

实际 secure 链为：

```text
padded priority-key additive shares
→ R1/R2 forward Permute+Share
→ R3 masked shuffled-list opening + CmpAgg rank shares
→ R4 framed rank-share exchange/reveal
→ local public shuffled rank < K carrier derivation
→ R5/R6 role-swapped reverse Permute+Share
→ logical_n original-order XOR mask shares
```

实现使用 `protocol_i_priority_pipeline_party` 和 `protocol_i_shuffle_reverse_party`。
R4 打开的是 shuffled-domain rank；不打开 permutation、original index、selected original
index 或原始顺序 mask。carrier 由公开 rank 在各 party 本地构造，P0 持有公开 0/1
arithmetic carrier，P1 持有 0，随后由 reverse shuffle 恢复原顺序。最终只取 word-0
奇偶作为 XOR share；这一步是本地算术到 XOR bit 转换，不增加 barrier。

测试矩阵包含：

- `n=1,2,3,5,7,8,11,16,31,127,128,129,256`；
- identity、reverse、random permutation；
- random、duplicate、all-equal、负值/极值和 padding；
- `K=1`、`K=2`、`K=8`（合法时）、`ceil(n/2)`、`K=n`；
- 非二次幂 logical_n、padded domain、stable tie；
- logical 输出长度裁剪、每个 share 的 XOR bit 形状、重构后恰好 K 个 1，以及与 signed int32
  oracle 的逐位置比较。

### 5.2 Raw-score Route A

实际 secure 链为：

```text
raw-score additive shares
→ R0 carry adapter
→ R0b sign adapter
→ R1/R2/R3 priority-key candidate
→ R4 rank reveal
→ local carrier derivation
→ R5/R6 reverse shuffle
→ logical_n original-order XOR mask shares
```

输入语义是冻结的 signed Q20.12 raw word（32-bit wire width，scale=4096）；carry/sign
adapter 使用独立 framed channels 和独立材料，生成 padded priority-key shares，不静默截断
32-bit raw input。测试覆盖负值、equal score、`INT32_MIN/MAX`、dummy、stable original-index tie、
non-power-of-two 和 `K` 边界。

### 5.3 Frame 与 fail-closed

`RA6M`/`RA8M` result frame 均带版本、`material_id`、logical output count、分阶段字节计数、
总轮数和 mask share。输入/材料帧绑定 `session`、`fingerprint`、`padded_n`、`K`、比较位宽、
sender/receiver、phase、type 和 sequence。测试覆盖截断、错误 magic/route identity、错误
material identity、EOF、超时、package party/shape/magic、材料 replay、重复/越界 rank 和
rank permutation gap；异常均 fail closed，不产生 partial output 或明文 fallback。

## 6. 实际轮数与消息审计

### 6.1 Priority Route A 六轮

| Barrier | 方向 | frame/阶段 | 语义 | 依赖 | 计入 |
|---:|---|---|---|---|---|
| R1 | P0↔P1 | forward PS first | 第一次 forward Permute+Share record share | 离线材料和输入就绪 | online |
| R2 | P0↔P1 | forward PS second | 第二次 forward Permute+Share | R1 | online |
| R3 | P0↔P1 | phase 3/type 1 | masked shuffled list opening，随后本地 CmpAgg | R2 | online |
| R4 | P0↔P1 | rank-reveal phase 2/type 1 | shuffled rank share exchange，公开 rank | R3 | online |
| local | 各方本地 | 无 frame | `rank<K` carrier 派生 | R4 | 不增加 barrier |
| R5 | P0↔P1 | reverse PS first | carrier inverse routing 第一轮 | local carrier | online |
| R6 | P0↔P1 | reverse PS second | carrier inverse routing 第二轮 | R5 | online |

Route result frame 使用 `RA6M`、version 1、phase 7、sequence 1；解析器确认 logical_n
长度、非零 rank/reverse sent/received bytes 和 `rounds=6`。在线消息字节由实际 framed
channel counters 读取；没有把 reverse shuffle 当作免费后处理。

### 6.2 Raw-score Route A 八轮

| Barrier | 方向 | frame/阶段 | 语义 | 依赖 | 计入 |
|---:|---|---|---|---|---|
| R0 | P0↔P1 | score phase 4 | carry adapter masked inputs | raw shares | online |
| R0b | P0↔P1 | score phase 5 | sign adapter masked inputs | R0 | online |
| R1 | P0↔P1 | forward PS first | priority-key record share | R0b | online |
| R2 | P0↔P1 | forward PS second | forward shuffle | R1 | online |
| R3 | P0↔P1 | phase 3/type 1 | masked shuffled-list opening + CmpAgg | R2 | online |
| R4 | P0↔P1 | rank-reveal | rank share exchange | R3 | online |
| local | 各方本地 | 无 frame | public rank carrier | R4 | 不增加 barrier |
| R5 | P0↔P1 | reverse PS first | inverse route | local carrier | online |
| R6 | P0↔P1 | reverse PS second | inverse route | R5 | online |

`RA8M` 使用 version 1、phase 8、sequence 1，携带 carry/sign/candidate/rank/reverse 分阶段
sent/received counters 和 `rounds=8`。测试控制器只在 TEST_ONLY 层接收两方 mask shares
并执行 XOR 重构与 oracle 比较；secure party runtime 不输出原始顺序明文 mask。

## 7. M2 测试与构建原始结果

### 7.1 当前 revision fresh EMP-ON full CTest

命令：

```text
ctest --test-dir /tmp/moe-stage3m-emp-on --output-on-failure
```

结果：`34/34 PASS`，`0 failed`，Total Test time `37.58 sec`。

### 7.2 focused 结果

| 范围 | 命令/CTest target | 结果 |
|---|---|---|
| M2 candidate | `moe_topk_m2_dealer_preprocessed_3round_candidate_test` | 1/1 PASS，4.25s |
| Priority Route A | `moe_topk_m2_dealer_preprocessed_rank_reveal_6round_mask_output_test` | 1/1 PASS，4.22s（含新增大域矩阵） |
| Raw Route A | `moe_topk_m2_raw_score_route_a_8round_mask_output_test` | 1/1 PASS，10.49s |
| M2 回归 | `ctest -R '^moe_topk_m2_'` | 19/19 PASS，最终 full run 中同样通过 |
| M3 回归 | DPF + `moe_topk_m3_*` + masked multiplication | 11/11 PASS，9.04s |

M2 19 项覆盖 priority key、priority DCF、reverse model、uCMP、CmpAgg、transport、process
E2E、score input、paper alignment、candidate、chosen OT、OPV、share translation、Permute+Share、
secret-shared shuffle、formal modular E2E 和两条 Route A 独立进程测试。

### 7.3 CMake build

- EMP-ON `BUILD_TESTING=ON`：fresh configure 通过，177/177 Ninja steps 完成；
- EMP-OFF `BUILD_TESTING=OFF`：fresh configure 通过，52/52 production steps 完成，
  `sytorch`/FSS/cryptoTools 静态库图无测试、benchmark、conformance 或 Route A harness；
- EMP-ON `BUILD_TESTING=OFF`：fresh configure 通过，69/69 steps 完成，
  `moe_topk_m2_dealer_preprocessed_3round_candidate` 构建通过；target help 仅有生产库、
  candidate core 和 candidate executable，无 `_test`/benchmark/conformance target。

注意：EMP-OFF production graph 不定义依赖 EMP-OT 的 candidate executable，因此直接请求
`moe_topk_m2_dealer_preprocessed_3round_candidate` 会得到 `unknown target`；这是预期的
feature gating，不是构建失败。实际 EMP-OFF production build 的 `all` 已通过。

### 7.4 失败复盘

新增大域覆盖的第一次运行在 `n=31` 因 TEST_ONLY controller 顺序错误超时（150.44s）：
controller 先 `waitpid(Dealer)`，Dealer package 写端堵塞，Party 尚未启动读取。修复为
先 spawn Dealer、P0、P1，再等待 Dealer，并未加入 sleep、轮询或重试。修复后：

- Priority Route A focused：1/1 PASS，4.22s；
- fresh full CTest：34/34 PASS，37.58s。

## 8. M3 交接复检

M3 标签仍为 `agarwal_protocol_iii_modular_3round`，priority-key 输入使用 padded_n，
GRank 图按 logical_n，DPF domain 为 `2^rank_bits`，native priority-key online rounds=3；
raw-score extension 仍为 5 轮。M3 没有被改成 Protocol III exact 2-round，也没有为 M2
Route A 改变输入/输出契约。

静态符号审计在 `protocol_iii*` secure runtime 和两条 M3 executable 中没有发现：

- `protocol_i_shuffle_forward_party` / `protocol_i_shuffle_reverse_party`；
- `protocol_i_priority_pipeline_party`；
- M2 Route A headers/symbols；
- `RA6M` / `RA8M` frame；
- M2 reverse carrier 或 raw-score Route A adapter。

M3 secure runtime 的 `reconstruct` 搜索无调用；仅有关于“重构后最低位等价”的注释。当前
EMP-ON M3 11/11 fresh 回归通过，包含 DPF conformance、GRank、DPF routing、masked mul、
secure combine、priority/raw three-process E2E、secure executable 和 metrics record。

## 9. Secure / leakage audit

### Secure path

- M2 Route A secure party path 不重构 score、priority key、rank share、selected index 或
  final mask；rank reconstruction 只发生在明确的 Route A rank-reveal 泄露边界，用于公开
  shuffled-domain rank；最终 mask reconstruction 只发生在 TEST_ONLY controller；
- 不使用 MockShuffle、旧 FSS ABI、旧 key layout、文件轮询、固定 sleep、在线 Dealer fallback
  或 `std::reverse` 代替 secure reverse shuffle；executable 中的 `std::reverse` 仅生成测试/CLI
  permutation 参数，secure path 调用的是 `protocol_i_shuffle_*`；
- P2 只在离线阶段生成并发送 input-independent package，Party 收到材料后才开始输入阶段，
  P2 不连接在线 rank/reverse channels；
- frame、material one-shot consumption、shape/party/phase/sequence/identity 校验均 fail closed。

### Leakage

Priority Route A 明确额外公开 shuffled-domain rank permutation 和 carrier；不公开 permutation
映射、original index、selected original index、原始顺序 mask 或 score。Raw Route A 继承同一
rank reveal，并额外包含 carry/sign adapter 的 masked exchanges。该泄露是项目 Route A 契约，
不是论文精确泄露证明；论文 exact leakage simulator、same-permutation public masked list 和
correlated material 仍未完成。

## 10. Metrics provenance

本阶段实际测得并可追溯的项目验证数据：CTest 计数/时间、Ninja step/build 成功、Route A
result frame 的分阶段 sent/received bytes、rounds、logical output count、正确性和测试矩阵。
原始日志路径：

- `/tmp/moe-stage3m-emp-on/configure.log`
- `/tmp/moe-stage3m-emp-on/build-full-v2.log`
- `/tmp/moe-stage3m-emp-on/ctest-full-v2.log`
- `/tmp/moe-stage3m-emp-on/ctest-route6-matrix-v2.log`
- `/tmp/moe-stage3m-emp-on/ctest-route8-final.log`
- `/tmp/moe-stage3m-emp-on/ctest-candidate-final.log`
- `/tmp/moe-stage3m-emp-on/ctest-m3-final.log`
- `/tmp/moe-stage3m-emp-off/configure.log`、`build-all.log`、`target-help.log`
- `/tmp/moe-stage3m-emp-on-prod/configure.log`、`build-route-a.log`、`target-help.log`

以下字段没有在本阶段形成可信、统一的正式 benchmark 采集，因此必须保持
`NOT_MEASURED`：

- offline/online/total wall-clock benchmark（CTest 用时不是协议性能 benchmark）；
- 网络 RTT、带宽、跨主机吞吐；
- formal offline material total bits 的统一论文口径；
- online communication per-party 的正式 benchmark 表；
- online PRG calls total；
- 重复次数/预热后的性能分布；
- 论文精确 3-round core 的性能和通信；
- 7-round total path 的任何性能或通信。

## 11. 修改文件与提交

阶段三M直接修改：

1. `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp`
   - 增加 `<utility>`；
   - 将 Route A 独立进程 harness 改为 Dealer/P0/P1 并发启动后再等待 Dealer，避免大 package
     socket backpressure 死锁；
   - 增加 `n=16,31,127,128,129,256` 的 Priority Route A matrix cases。
2. `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3L_REPORT_2026-09-13.md`
   - 将 final verification revision 从 dangling `f752a77` 修正为最终 `71c1612`；
   - 增加 amend-before-final provenance 解释。
3. `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3M_REPORT_2026-09-14.md`
   - 本阶段完整 reproduction/evidence report。

阶段三L/前置 Route A 代码仍来自提交：

`71c161290d6627e8a9521a8733e6061b9afed861 feat(m2): complete Route A mask output paths`

本阶段不修改前置 Route A 实现文件、M3 secure runtime 或冻结基线。代码/证据提交为
`fc149c2e33df53a272208622da925d0047303121`；提交前已运行 `git diff --check`，并确认
差异只包含上述三文件；随后已创建仅文档的 closeout commit，
不改变代码或测试证据。

## 12. A-J 完整回答

**A. 当前最终 revision 的准确 hash 是什么？**
阶段三M复验所依据的前置最终 revision 是
`71c161290d6627e8a9521a8733e6061b9afed861`；包含本阶段代码/证据的提交是
`fc149c2e33df53a272208622da925d0047303121`。之后的 closeout 若仅修改本报告，不改变
该代码 revision 的测试含义。

**B. `f752a77` 与 `71c1612` 的关系是什么？**
`f752a77` 是同一父提交 `98be22f` 上的 amend 前 dangling commit；`71c1612` 是同主题、
同作者/时间的最终 amend commit。阶段三L报告已修正，不再把 `f752a77` 写成最终 revision。

**C. M2 formal baseline 是否仍保持原标签和 8 轮？**
是。标签仍为 `m2_protocol_i_raw_score_input_modular_8round_mask_output`，8 轮不变。

**D. 三轮 candidate 是否通过独立进程和 rank differential？**
是。当前 revision fresh EMP-ON candidate focused 为 1/1 PASS；测试启动独立 Dealer、P0、P1，
并在 TEST_ONLY 层对 shuffled rank shares 做 differential。输出仍不是完整 mask。

**E. Priority-key Route A 是否真正通过独立进程，实际是否 6 轮？**
是。独立 Dealer/P0/P1 运行，`RA6M` frame 解析确认非零 rank/reverse counters 和 `rounds=6`；
扩大到 `n=256` 后 focused 仍 1/1 PASS。

**F. Raw-score Route A 是否真正通过独立进程，实际是否 8 轮？**
是。独立 Dealer/P0/P1 运行，`RA8M` frame 解析确认 carry/sign/candidate/rank/reverse counters
和 `rounds=8`；focused 1/1 PASS。

**G. 最终输出是否为 logical_n 原始顺序 XOR Top-K bit-mask？**
是。两个 Route A frame 的 mask share 长度等于 logical_n；TEST_ONLY controller 对两方 share
执行 XOR 后逐位置匹配冻结 signed-score stable oracle，且恰好 K 个 1。

**H. secure 路径是否避免 rank/index/final-mask 重构？**
是，但 Route A 明确允许并计入 R4 的 shuffled-domain rank reveal；secure path 不重构 selected
index、original index 或 final mask，最终 mask 重构仅在 TEST_ONLY harness。

**I. M3 是否在当前 revision 上通过无回归复检？**
是。fresh EMP-ON M3 focused 11/11 PASS，完整 CTest 34/34 PASS；M3 符号和契约边界未被 M2
Route A 改动污染。

**J. 当前是否可以宣称论文 Protocol I 三轮精确复现？**
不可以。论文 exact label 仍 `BLOCKED / NOT_VERIFIED`；public masked-list 同置换/相关材料、
完整论文 transcript、精确泄露证明和 3-round mask-output gate 尚未满足。

## 13. 当前阻塞与下一阶段建议

当前唯一协议级阻塞仍是论文精确 Protocol I：需要独立完成论文兼容的 shuffle/public-list
功能、相关预处理材料证明、完整消息流和泄露模拟器，再按 conformance、oracle differential、
独立进程 E2E 和 metrics gate 验收。不得把当前 Route A 的 6/8 轮重命名为 3/7 轮。

建议下一阶段：

1. 保持本阶段 `RA6M`/`RA8M`、M2 candidate 和 M3 contract 标签冻结；
2. 继续维护大域独立进程矩阵与 frame negative tests；
3. 若推进论文 exact，先更新 decision record 和实施计划，明确 public masked-list、same
   permutation、Dealer view、rank visibility、inverse routing 和 leakage proof，再实现代码；
4. formal performance benchmark 在统一采集器、重复/预热、网络配置和 per-party counters
   完整接通前继续写 `NOT_MEASURED`。

## 14. 状态表

| 项目 | 状态 |
|---|---|
| M2 formal engineering baseline | GO |
| M2 3-round rank-share candidate | GO |
| M2 priority-key Route A 6-round mask output | GO |
| M2 raw-score Route A 8-round mask output | GO |
| M3 modular 3-round | GO，仅限模块化工程基线 |
| M2 paper-exact Protocol I | BLOCKED / NOT_VERIFIED |
| 论文原生 3-round core | NOT_IMPLEMENTED / BLOCKED |
| 论文/项目 7-round total path | NOT_IMPLEMENTED / RESEARCH_BLOCKED |
| formal performance benchmark | NOT_MEASURED |
