# 阶段三N：M2/M3 最终 provenance 冻结与论文精确门复审

日期：2026-09-15
分支：`codex/m2-candidate-ubuntu-validation`
证据级别：C 级项目工程扩展与当前 revision 的 Ubuntu/EMP 实测；不提升为论文结论。

## 1. Executive conclusion

阶段三N在当前实际 HEAD `5bd879e8c4d053b14aae61f19bbaf53529446025` 上重新完成了
EMP-ON fresh build、34 个 CTest 的完整回归、M2/M3 分组回归、三个独立进程 Route A/
candidate focused test，以及 EMP-OFF/EMP-ON production graph 检查。结果如下：

- M2 formal engineering baseline：`GO`，标签
  `m2_protocol_i_raw_score_input_modular_8round_mask_output`，8 个在线因果轮次；
- M2 三轮 candidate：`GO`，标签
  `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`，输出是
  shuffled-domain rank shares，不是完整 mask；
- Priority-key Route A：`GO`，标签
  `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output`，6 轮项目扩展；
- Raw-score Route A：`GO`，标签
  `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output`，8 轮项目扩展；
- M3：`GO`，标签 `agarwal_protocol_iii_modular_3round`，当前 revision 无回归；
  raw-score 5 轮扩展也在当前 revision 上单独复测通过；
- `agarwal_protocol_i_exact_mask_output`：继续
  `M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`；
- 论文原生 Protocol I 三轮 core、7-round total path 和 formal performance benchmark
  均未实现或未测量，不能由当前工程扩展推导。

本阶段没有修改 secure M2/M3 source、`VFSS-baseline/` 或任何参考目录。唯一历史报告
更正是将阶段三M closeout 的实际 `5bd879e` 文档-only 子提交写入阶段三M报告，避免把
阶段三M代码证据提交与当前文档 HEAD 混为一谈。

## 2. Git provenance 与工作区

### 2.1 当前 revision

```text
branch: codex/m2-candidate-ubuntu-validation
HEAD:   5bd879e8c4d053b14aae61f19bbaf53529446025
```

`fc149c2e33df53a272208622da925d0047303121` 与当前 HEAD 存在祖先关系，且
`5bd879e8c4d053b14aae61f19bbaf53529446025` 是 `fc149c2e33df53a272208622da925d0047303121`
的直接子提交：

- `fc149c2`：阶段三M代码/测试证据提交，包含 TEST_ONLY harness 修复、矩阵扩展和
  阶段三L/M报告；
- `5bd879e`：阶段三M文档 closeout，只修改阶段三M报告，不改变代码或测试证据；
- 阶段三L代码基准：`71c161290d6627e8a9521a8733e6061b9afed861`；
- `f752a771e33e8da274e369e9e67695e50cfc676a`：同父 `98be22f...` 的 amend 前 dangling
  commit，不是阶段三L最终 revision；`71c1612` 是其后保留的正式替代 revision。

阶段三N开始前工作区 `git status --short --branch` 仅显示分支且无差异；当时
`git diff --check` 通过，`git ls-files -o --exclude-standard` 无输出。阶段三N交付后，
工作区的四个预期文档修改/新增就是本报告列出的文件；没有构建目录、日志、socket、密钥、
临时 material 或参考工程副本进入仓库。`VFSS-baseline/`、`Papers/`、`Agarwal_TopK/`、
`ADSMPC/`、`CipherGPT/` 均无差异；最终文档差异再次执行 `git diff --check` 通过。所有
本阶段构建和日志均在 Ubuntu `/tmp`。

## 3. 阶段三L/三M报告修正

- 阶段三L已把错误的 `f752a77` final verification revision 改为完整
  `71c161290d6627e8a9521a8733e6061b9afed861`，并保留 amend-before-final 解释；
- 阶段三M原有代码/证据仍绑定 `fc149c2`，其历史复验所依据的前置实现 revision 仍是
  `71c1612`；
- 阶段三M报告本阶段补记 `5bd879e` 已实际落地且是 `fc149c2` 的文档-only 直接子提交；
- 阶段三M历史 `/tmp/moe-stage3m-*` 结果没有被冒充为阶段三N结果。阶段三N所有新测试
  绑定 `/tmp/moe-stage3n-emp-on`、`/tmp/moe-stage3n-emp-off` 和
  `/tmp/moe-stage3n-emp-on-prod`。

## 4. 环境与复现命令

实测环境：Ubuntu 24.04.4 LTS（WSL2，x86_64）、GCC/G++ 13.3.0、CMake 3.28.3、
CTest 3.28.3、Ninja 1.11.1、Eigen3 CMake config `/usr/share/eigen3/cmake`、
OpenSSL 3.0.13、`RLIMIT_NOFILE=10240`。EMP-Tool/EMP-OT CMake package 均为 1.0.0，
prefix 为 `/tmp/moe_m28_emp.ok9WzQ/prefix`；没有 fake header、fake library 或 ABI 绕过。

EMP-ON fresh build（`/tmp/moe-stage3n-emp-on`）：

```bash
cmake -S VFSS -B /tmp/moe-stage3n-emp-on -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/share/eigen3/cmake \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/moe-stage3n-emp-on --parallel 1
ctest -N --test-dir /tmp/moe-stage3n-emp-on
ctest --test-dir /tmp/moe-stage3n-emp-on --output-on-failure
```

EMP-OFF fresh production build（`/tmp/moe-stage3n-emp-off`）：

```bash
cmake -S VFSS -B /tmp/moe-stage3n-emp-off -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF -DEigen3_DIR=/usr/share/eigen3/cmake
cmake --build /tmp/moe-stage3n-emp-off --parallel 1
cmake --build /tmp/moe-stage3n-emp-off --target help
```

EMP-ON production graph（`/tmp/moe-stage3n-emp-on-prod`）也使用
`BUILD_TESTING=OFF`、`MOE_TOPK_ENABLE_EMP_OT=ON` 重新构建 candidate production executable。

## 5. M2 五类状态

| 类别 | 固定标签 | 当前状态 | 输出边界 |
|---|---|---|---|
| formal engineering baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | GO，8 轮 | original-order XOR mask |
| 3-round candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | GO，3 轮 | shuffled-domain rank shares |
| priority-key Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` | GO，6 轮 project extension | original-order XOR mask |
| raw-score Route A | `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | GO，8 轮 project extension | original-order XOR mask |
| paper-exact target | `agarwal_protocol_i_exact_mask_output` | BLOCKED / NOT_VERIFIED | 未实现论文原生三轮 mask output |

## 6. 构建结果与 CTest 数量

- EMP-ON `BUILD_TESTING=ON`：177/177 Ninja steps 完成；`ctest -N` 发现 **34** 个测试；
- EMP-OFF `BUILD_TESTING=OFF`：52/52 Ninja steps 完成；target help 只有 `sytorch`、
  `FSS`、`bitpack`、`cryptoTools` 等 production/library targets，没有测试、benchmark、
  conformance 或 Route A harness；
- EMP-ON `BUILD_TESTING=OFF`：69/69 Ninja steps 完成，
  `moe_topk_m2_dealer_preprocessed_3round_candidate` 生产 executable 可构建；target help
  只列 core/backend/library phony targets，不列 CTest targets；
- 没有把 EMP-OFF 中不存在的 EMP-dependent candidate test 误判为构建失败。

## 7. 当前 revision 的测试结果

### 6.1 全量与 M2/M3 focused

```text
full CTest:       34/34 PASS, 0 failed, 37.76 s
M2 focused:      19/19 PASS, 0 failed, 30.05 s
M3/DPF focused:  11/11 PASS, 0 failed, 10.22 s
candidate:         1/1 PASS
priority Route A:  1/1 PASS
raw Route A:       1/1 PASS
M3 raw-score 5-r:  3/3 PASS, 4.52 s
```

M3 focused 的 11 个 target 包括 DPF conformance、GRank、DPF routing、masked multiply、
secure combine、priority-key independent-process E2E、raw-score pipeline/E2E、两个 secure
executable 和 MetricsRecord。raw-score 5-r focused 明确重跑 pipeline、three-process E2E、
secure executable 三个 target；测试断言当前 revision 的 `input_adapter_rounds=2`、
`core_rounds=3`、`total_online_rounds=5`。

### 6.2 M2 candidate 与 Route A

candidate target 启动独立 Dealer/P0/P1，完成 2 个 forward shuffle barrier 加 1 个
masked-list opening barrier，TEST_ONLY 层只对 rank share 做 differential；当前结果 PASS。

Priority-key Route A 独立进程 target 运行 Dealer/P0/P1，解析 `RA6M` result frame，确认
logical output length、非零 rank/reverse transport counters 和 `rounds=6`；输入矩阵覆盖
`n=1,3,5,7,8,11,16,31,127,128,129,256`、`K=1`、`K=n`、中间 K、padding、非 2 次幂、
identity/reverse/random permutation、duplicate/all-equal、negative/extreme signed score 和
stable ties。

Raw-score Route A 独立进程 target 运行 Dealer/P0/P1，解析 `RA8M` result frame，确认 carry、
sign、candidate、rank、reverse counters 均非零且 `rounds=8`；矩阵覆盖
`n=1,2,3,5,7,8,11,16,31,127,128,129,256`、合法 `K=1/2/8/ceil(n/2)/n`、
Q20.12 signed score、`INT32_MIN/MAX`、negative、duplicate/all-equal、padding 和 stable ties。

两个 Route A 测试都在 TEST_ONLY controller 中 XOR 两方 mask share，逐位置匹配冻结的
signed-score stable oracle，并检查重构 mask 恰好有 K 个 1；secure party 进程本身不重构
rank、selected index 或 final mask。

## 8. Route A causal round trace 与泄露

### 7.1 Priority-key Route A（6 轮）

| barrier | sender/receiver | frame/material | payload/opened value | 依赖 | 证据 |
|---|---|---|---|---|---|
| R1 | P0/P1 | forward `Permute+Share`, candidate material | masked share transport；不打开 rank | offline package | candidate counters `>0` |
| R2 | P0/P1 | second forward `Permute+Share` | masked share transport；不打开 rank | R1 | candidate counters `>0` |
| R3 | P0/P1 | masked-list opening | shuffled masked list `y` 公开 | R1/R2 | `public_masked_list` 两方一致 |
| local | each party | CmpAgg/rank evaluation | 不发送；rank 仍为 share | R3 | candidate rank differential 仅 TEST_ONLY |
| R4 | P0/P1 | `RA6M` rank-reveal frame | shuffled-domain rank share 公开，形成 public carrier | local rank | rank counters `>0` |
| local | each party | carrier derivation | 不直接注入 carrier 或重排数组 | R4 | static audit + E2E |
| R5 | P0/P1 | reverse `Permute+Share` first stage | mask share reverse transport | R4 | reverse counters `>0` |
| R6 | P0/P1 | reverse `Permute+Share` second stage | original-order mask share | R5 | result length `logical_n` |

实现和 result frame 强制 `online_rounds=6`。R1/R2/R3 的候选 core counters、R4 的 rank
counters、R5/R6 的 reverse counters 均来自实际运行；当前 frame 没有逐 barrier 的 bit-level
计数器，因此逐 barrier 精确 `sent_bits/received_bits` 为 `NOT_MEASURED`，不能用 aggregate
bytes 伪造。offline package handoff、local computation 和 TEST_ONLY reconstruction 不计入
online causal rounds。

### 7.2 Raw-score Route A（8 轮）

| barrier | sender/receiver | frame/material | payload/opened value | 依赖 | 证据 |
|---|---|---|---|---|---|
| R0 | P0/P1 | carry adapter | carry masked input；不打开 raw score | offline carry material | carry counters `>0` |
| R0b | P0/P1 | sign adapter | sign masked input；不打开 raw score | R0 | sign counters `>0` |
| R1/R2/R3 | P0/P1 | candidate forward/open | shuffled masked list | R0b | forward/candidate counters `>0` |
| R4 | P0/P1 | `RA8M` rank reveal | shuffled rank 公开并形成 carrier | candidate | rank counters `>0` |
| local | each party | carrier derivation | 不发送、不注入原始顺序 | R4 | static audit + E2E |
| R5/R6 | P0/P1 | reverse shuffle two stages | original-order XOR mask shares | R4 | reverse counters `>0` |

`RA8M` frame 强制 `rounds=8` 并携带 carry/sign/forward/rank/reverse aggregate counters。
同样，逐 barrier 精确 bit counters 为 `NOT_MEASURED`；实际 8 轮由 frame assertion、
component counters 和独立进程运行共同证明，不把 reverse shuffle 或 adapter 计为免费。

### 7.3 泄露边界

Route A 公开或可见：configured dimensions、shuffled masked list、shuffled rank、public
shuffled selection carrier、framing/metrics metadata。Route A 不重构 raw score、priority key、
original index、local permutation、selected original index、original-order final mask 或
party-local shares。R4 rank reveal 是 Route A 新增泄露，不能写成论文原生 Protocol I 的泄露；
rank reveal 不能隐藏在 local step，reverse shuffle 不能计为零轮。

上述“没有重构”同时有两层证据：代码静态 audit（secure runtime 无 reconstruct、controller
不注入 carrier）和当前 independent-process runtime 检查；这不是论文安全证明或完整 leakage
simulator 等价证明。

## 9. M3 交接与 secure-path audit

M3 保持 `agarwal_protocol_iii_modular_3round`：

```text
priority-key shares (padded_n)
  -> GRank(logical_n graph)
  -> DPF routing (domain 2^rank_bits)
  -> secure combine
  -> original-order logical_n XOR Top-K mask
```

secure core 强制 3 个 online rounds；raw-score 扩展固定为两轮 carry/sign adapter 加三轮
core，即 5 轮。M3 当前 revision focused 11/11 与 raw-score 5-r focused 3/3 通过。

静态符号/调用审计确认 M3 不调用 M2 Route A reverse-shuffle、rank-reveal、`RA6M`、`RA8M`
或 raw-score carry/sign adapter symbols；不解析 M2 Route A frame，不依赖 public carrier，
使用自己的 package/material/serialization。M3 secure runtime 未发现实际 `reconstruct(...)`
调用，也不重构 rank、DPF index、selected index 或 final mask。该结论为
`STATIC_AUDIT_PASS` 加当前测试通过，不等同于论文 Protocol III exact 2-round 证明。

M3 3-round trace 由当前运行的 component metrics 组成：GRank=1、DPF routing=1、secure
combine=1；raw-score 当前运行明确为 input adapter=2、core=3、total=5。精确每条消息的
bit-level trace 未由现有 MetricsRecord 单独导出，故该字段为 `NOT_MEASURED`。

## 10. M2 paper-exact Protocol I gate

| Gate | 论文要求 | 当前证据 | 状态 | 缺口 |
|---|---|---|---|---|
| 三轮 core transcript | 论文逐消息三轮、party view、依赖边界 | 会议版与当前报告无足够逐消息 transcript | BLOCKED/UNKNOWN | 需论文/作者材料 |
| same-permutation binding | masked list、rank、index 由同一隐藏置换绑定 | Route A 有工程 permutation material，但无论文等价证明 | BLOCKED | 需形式化 binding |
| correlated material `r` | `r` 的生成、分发、消费和与 permutation 的相关性 | 本地工程有 material id/share contract，相关性证明缺失 | BLOCKED | 需协议定义与证明 |
| public masked-list | 精确公开对象与语义 | 当前是项目 Route A shuffled masked list | BLOCKED | 需论文语义确认 |
| secure rank computation | 不重构 rank/index 完成核心 | 项目 candidate secure runtime + TEST_ONLY differential | BLOCKED/PROJECT PASS | 缺论文 exact transcript |
| leakage equivalence | 与论文允许公开集合等价 | R4 rank reveal 改变泄露 | BLOCKED | 需 leakage simulator |
| offline Dealer model | 输入无关、离线、静默 Dealer | 当前 independent-process Dealer 满足项目边界 | PROJECT PASS | 论文模型仍 UNKNOWN |
| stable ties/padding | 论文是否定义当前 tie/padding | signed stable oracle 与 padding 已通过项目矩阵 | PROJECT PASS / PAPER UNKNOWN | 需论文边界确认 |
| primitive conformance | 原语 ABI 与输入输出一致 | DPF/OT/shuffle/adapter conformance 通过 | PROJECT PASS | 不构成 paper proof |
| differential | 与冻结 oracle 一致 | M2/M3 independent-process differential 通过 | PROJECT PASS | exact leakage 仍缺 |
| independent-process E2E | Dealer/P0/P1 与三轮 core 真实运行 | candidate/Route A/M3 当前 revision 通过 | PROJECT PASS | 不是 paper-native core |
| metrics/provenance | revision、label、round、输入、原始计数 | Stage3N logs 与 frame counters 已冻结 | PROJECT PASS | per-barrier bit counts NOT_MEASURED |

结论：`M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`。当前 Route A 是含 rank reveal 与 reverse
shuffle 的 6/8 轮工程扩展；三轮 candidate 不是完整 mask；不能宣称
`agarwal_protocol_i_exact_mask_output`、论文原生三轮 core 或 7-round total path。

## 11. Blocker closure 与下一阶段建议

最小缺失事实：论文三轮的准确 transcript、sender/receiver、public masked-list 语义、
same-permutation binding、相关 `r` 生成/分发/消费、稳定同分与 padding 边界、允许公开对象、
安全模型和原始输出定义。能由 VFSS 补齐的是独立 paper-compatible primitive、材料状态机、
transport trace、conformance、differential、independent-process E2E 和 exact leakage
simulator；必须由论文或作者确认的是上述论文事实与泄露等价关系。

下一阶段若继续，应新建独立 paper-compatible design branch/decision record，先冻结三轮
transcript、party view、material contract、leakage table 和失败语义，再实现新 primitive。
不得改名当前 Route A、复用 M3 DPF material、隐藏 rank reveal/reverse adapter、或改写
formal 8-round baseline。若无法取得论文缺失事实，建议保持当前状态并转入 M3/后续任务。

## 12. 未测量项目

以下均为 `NOT_MEASURED`：formal wall-clock performance benchmark、跨主机网络 RTT/带宽、
重复/预热后的性能分布、统一论文口径 offline/online total bits、逐 barrier 精确
sent_bits/received_bits、online PRG calls total、论文 exact 3-round core 性能/通信、
7-round path 的任何性能/通信、完整论文 leakage simulator 等价证明。

## 13. 修改文件、提交与原始证据

本阶段修改文件：

1. `docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md`（本报告）；
2. `docs/decisions/M2_M3_FINAL_HANDOFF_2026-09-15.md`；
3. `docs/decisions/M2_PROTOCOL_I_EXACT_NEXT_GATE_2026-09-15.md`；
4. `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3M_REPORT_2026-09-14.md`（补记已落地的
   `5bd879e` 文档-only closeout provenance）。

代码/测试证据提交仍为 `fc149c2e33df53a272208622da925d0047303121`；当前文档-only closeout
为其直接子提交 `5bd879e8c4d053b14aae61f19bbaf53529446025`。阶段三N文档修改完成后应创建新的
Stage3N documentation commit；本报告不伪造 commit hash。

原始证据路径：

- `/tmp/moe-stage3n-emp-on/configure.log`
- `/tmp/moe-stage3n-emp-on/build.log`
- `/tmp/moe-stage3n-emp-on/ctest-list.log`
- `/tmp/moe-stage3n-emp-on/ctest-full.log`
- `/tmp/moe-stage3n-emp-on/ctest-candidate.log`
- `/tmp/moe-stage3n-emp-on/ctest-route6.log`
- `/tmp/moe-stage3n-emp-on/ctest-route8.log`
- `/tmp/moe-stage3n-emp-on/ctest-m2.log`
- `/tmp/moe-stage3n-emp-on/ctest-m3.log`
- `/tmp/moe-stage3n-emp-on/ctest-m3-raw5.log`
- `/tmp/moe-stage3n-emp-off/configure.log`
- `/tmp/moe-stage3n-emp-off/build.log`
- `/tmp/moe-stage3n-emp-off/target-help.log`
- `/tmp/moe-stage3n-emp-on-prod/configure.log`
- `/tmp/moe-stage3n-emp-on-prod/build.log`
- `/tmp/moe-stage3n-emp-on-prod/target-help.log`

## 14. A-J 完整回答

**A.** 当前实际最终 revision 是 `5bd879e8c4d053b14aae61f19bbaf53529446025`。
**B.** `fc149c2` 是代码/测试证据提交；`5bd879e` 是其直接文档-only 子提交。
**C.** `f752a77` 是同父 amend 前 dangling revision；阶段三L已正确改为正式的 `71c1612`。
**D.** 是，M2 formal baseline 标签和 8 轮未变。
**E.** 是，candidate 在当前 revision fresh EMP-ON 上 1/1 PASS，输出仍为 rank-share。
**F.** 是，priority-key Route A 独立进程 1/1 PASS，`RA6M`/counters 确认 6 轮。
**G.** 是，raw-score Route A 独立进程 1/1 PASS，`RA8M`/counters 确认 8 轮。
**H.** 是，两个 Route A 输出均为 `logical_n` 原始顺序 XOR Top-K bit-mask；重构只在 TEST_ONLY。
**I.** 是，M3 当前 revision 无回归；M3 focused 11/11、raw-score 5-r focused 3/3 通过。
**J.** 不可以。论文 Protocol I exact gate 尚未全部有证据，必须继续
`BLOCKED / NOT_VERIFIED`。

## 15. 最终状态表

| 项目 | 状态 |
|---|---|
| M2 formal engineering baseline | GO，8-round project baseline |
| M2 3-round rank-share candidate | GO |
| M2 priority-key Route A | GO，6-round project extension |
| M2 raw-score Route A | GO，8-round project extension |
| M3 modular 3-round | GO，仅限 modular project baseline |
| M2 paper-exact Protocol I | BLOCKED / NOT_VERIFIED |
| paper-native 3-round core | NOT_IMPLEMENTED / BLOCKED |
| project 7-round path | NOT_IMPLEMENTED / RESEARCH_BLOCKED |
| formal performance benchmark | NOT_MEASURED |
