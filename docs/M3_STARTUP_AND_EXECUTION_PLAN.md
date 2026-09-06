# M3 启动与执行计划

## 1. 目的与状态基线

本文是 M3 的独立启动文件，用于压缩此前 M1.1、M2、Protocol III DPF、乘法 adapter 与 M3 接口讨论，并作为后续实现、复检、测试和合入的执行清单。

状态基线固定为：

```text
main@6c72ec8a18a44c1d3d441758017b9807fe1dc090
6c72ec8 docs: sync M1 M2 M3 progress and branch map
```

本文描述的“已完成”只表示已有仓库证据支持；尚未实现的 M3 项目必须继续按计划项处理，不得把设计目标、前置原语验证或候选轮数写成已经完成的论文精确实现。

### 1.1 已冻结事实

- M1.1 公共正确性底座已完成；Ubuntu 验收四项 CTest 为 4/4。
- M2 已合入 `main`。
- M2 的实际成果是 C 级模块化 Protocol I 基线 `m2_protocol_i_raw_score_input_modular_8round_mask_output`，实际在线因果轮数为 8，不是论文精确 3 轮实现。
- M2 最终验证为 EMP-OFF 13/13、EMP-ON 19/19；四组 `(128,2)`、`(128,8)`、`(256,2)`、`(256,8)` E2E 均 exit 0。
- DPF conformance 的 44 组验证已进入 `main`，但 `moe_topk_dpf_conformance_test` 尚未注册到 CTest。
- 乘法 adapter 位于 PR #4 / `design/protocol-iii-mul-adapter`；在本基线复检时，该分支相对 `main` 落后 54 个提交、领先 2 个提交。
- PR #4 同步 `main` 时预计重点冲突文件为 `VFSS/CMakeLists.txt` 和 `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`。
- M3 采用双层交付：3 轮核心与 raw-score 5 轮端到端包装必须分开命名、测试和计量。

---

## 2. 阶段 0：VMware 统一测试环境

目标：从最新 `main` 建立统一、可复现的 Ubuntu 24.04 测试基线，不在旧 PR #4 分支上审计 M1/M2。

### 执行清单

- [ ] 从最新 `origin/main` 建立临时复检分支 `review/m1-m2-unified-validation`。
- [ ] 复检分支初始 HEAD 必须与当时最新 `origin/main` 一致。
- [ ] VMware 当前若未发现 EMP，则在仓库外安装冻结版本：
  - `emp-tool@97f3359`
  - `emp-ot@03acb04`
- [ ] 使用 `docs/decisions/M2_CHOSEN_OT_DEPENDENCY.md` 中记录的 archive SHA-256 校验实际归档；hash 不一致时停止。
- [ ] EMP 源码、安装目录和构建目录均不得提交到仓库。
- [ ] 构建目录放在 `/tmp`。
- [ ] 使用 `Debug`。
- [ ] 默认 `-j1`，避免 40GB VMware 虚拟磁盘和内存压力。
- [ ] 先运行 EMP-OFF 完整 CTest，预期 13/13。
- [ ] 再运行 EMP-ON 完整 CTest，预期 19/19。
- [ ] EMP-ON 全绿后执行四组 M2 E2E：`(128,2)`、`(128,8)`、`(256,2)`、`(256,8)`。
- [ ] 四组 E2E 必须全部 exit 0。

任何失败先保存：

1. 失败 target/test 名称；
2. 精确命令；
3. 退出码；
4. 最后错误输出；
5. 当前 revision；
6. EMP-OFF / EMP-ON 配置；
7. 构建目录。

完成失败记录前不得直接修改协议实现。

---

## 3. 阶段 1：M1.1 逻辑复检

目标：确认 M3 继续复用的 score、rank、mask、oracle 和 metrics 语义没有漂移。

### 冻结语义

逐项确认并记录：

- [ ] score 是 32-bit two's-complement Q20.12，scale=12；
- [ ] Top-K 按 score 降序，同分按 `original_index` 升序；
- [ ] `rank=0` 表示最高优先级；
- [ ] 合法请求为非空且 `1 <= K <= n`；
- [ ] oracle 输出为原始顺序 bit-mask；
- [ ] mask 长度为 `n`、每位为 0/1、恰有 K 个 1；
- [ ] metrics 的 total/per-party 可由同一原始记录派生；
- [ ] 未测量字段保持 `NOT_MEASURED`；
- [ ] `VFSS-baseline/` 仍与冻结标签 `vfss-baseline-2026-09-03` 一致。

### 测试

复跑 M1 四项 CTest：

- [ ] `moe_topk_m1_oracle_test`
- [ ] `moe_topk_m1_cmpagg_test`
- [ ] `moe_topk_m1_metrics_test`
- [ ] `moe_topk_m1_dcf_conformance_test`

补充或确认：

- [ ] 重复值；
- [ ] 全相等；
- [ ] 负值；
- [ ] `INT32_MIN/MAX`；
- [ ] `K=1`；
- [ ] `K=n`；
- [ ] 非 2 次幂 `n`。

只有发现可复现错误才建立独立修复 PR；不得为了 M3 修改冻结语义。

---

## 4. 阶段 2：M2 Protocol I 逻辑复检

按真实数据流审计：

```text
raw score shares
→ 2-round carry/sign widening
→ priority-key shares
→ 2-round forward shuffle
→ 1-round masked-key/CmpAgg
→ 1-round shuffled rank reveal
→ 2-round reverse mask
→ original-order XOR mask shares
```

当前总在线因果轮数：

```text
2 + 2 + 1 + 1 + 2 = 8
```

### 检查重点

- [ ] P2/Dealer 在输入释放前完成离线工作并退出；
- [ ] P2 在线阶段无连接或消息；
- [ ] secure 路径不重构 raw score；
- [ ] secure 路径不重构 comparison bit；
- [ ] secure 路径不重构原始位置 rank；
- [ ] secure 路径不重构 selected index；
- [ ] secure 路径不重构最终 mask；
- [ ] package/material 一次性消费；
- [ ] party、session、fingerprint、长度或材料错误时 fail-closed；
- [ ] EOF/timeout fail-closed；
- [ ] 不允许明文 fallback 或部分成功结果；
- [ ] `protocol_i_raw_score_input_party` 契约一致；
- [ ] `protocol_i_cmpagg_eval_party` 契约一致；
- [ ] framed transport 契约一致；
- [ ] 最终 original-order XOR mask 契约一致。

M2 当前实现始终报告为：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

其证据等级固定为 C 级模块化工程基线。论文 3 轮只作为未完成目标，不得把当前 8 轮路径简称为论文 3 轮实现。

只有 EMP-OFF、EMP-ON 和四组显式 E2E 再次全绿后，才允许 M3 runtime 接入。

---

## 5. 阶段 3：重整 PR #4 与 M3 原语测试

使用现有分支：

```text
design/protocol-iii-mul-adapter
```

### 同步策略

- [ ] `fetch origin` 后将最新 `origin/main` merge 到该分支；
- [ ] 不改写共享远程历史；
- [ ] 不 force-push；
- [ ] 保留 `main` 上全部 M1/M2 target 和 CTest。

重点冲突文件：

```text
VFSS/CMakeLists.txt
docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md
```

### CMake 要求

处理冲突时保留全部 M2 目标，并加入/保留：

- [ ] `moe_topk_dpf_conformance_test` 的 CTest 注册；
- [ ] `moe_topk_masked_mul_adapter_test`；
- [ ] masked multiplication adapter 的 CTest 注册。

### 文档要求

Protocol III 设计文档必须同时保留：

- [ ] M2 已完成、M3 可启动的交接状态；
- [ ] 乘法 adapter 的证据。

更新 Ubuntu 复现记录到新的 `main` 基线，并明确 revision、环境与实际测试数量。

预期统一 CTest 数量：

```text
EMP-OFF: 15/15
EMP-ON:  21/21
```

更新 PR #4 前确认差异仍只包含 adapter、测试注册和相关 Protocol III 文档，不混入无关 M1/M2 实现修改。

---

## 6. 阶段 4：补齐 M2 → M3 rank-share 接口

当前 M2 pipeline 只公开最终 XOR mask，M3 不能从中取得 GRank rank shares；不得通过测试重构或从最终 mask 反推 rank。

新增最小 M3 专用接口。

### `ProtocolIIIGrankConfig`

至少包含：

```text
session
fingerprint
party
n
comparison bits
timeout
```

### `ProtocolIIIGrankOutput`

至少包含：

```text
original-position additive rank shares
single-round communication counters
edge count
DCF evaluation count
```

不得包含明文 rank 或 selected index。

### `protocol_iii_grank_party(...)`

- [ ] 接受 prepared priority-key shares；
- [ ] 接受 party-specific CmpAgg material；
- [ ] 复用 M2 uCMP/CmpAgg；
- [ ] 通过一次 masked-key exchange 返回 original-position additive rank shares；
- [ ] 只计 1 个在线 GRank 因果轮次；
- [ ] 复用 M2 package binding、framed transport、timeout/EOF 和 fail-closed 语义；
- [ ] 不复制第二套 score comparison 或 rank 实现。

CmpAgg 只覆盖真实 `logical_n` 个元素。DPF routing 单独嵌入最小 `2^rank_bits` 域，不引入 dummy score。

进入 routing 前至少通过：

- [ ] 与 M1 oracle 的 rank differential；
- [ ] party/material/session/fingerprint 错配；
- [ ] EOF；
- [ ] timeout；
- [ ] 重复材料；
- [ ] 材料不足；
- [ ] 重复值、全相等、负值、`INT32_MIN/MAX`；
- [ ] 非 2 次幂 `n`。

---

## 7. 阶段 5：M3 双层实现

## 7.1 三轮核心

实现标签：

```text
agarwal_protocol_iii_modular_3round
```

输入为 prepared priority-key arithmetic shares，输出为 original-order XOR Top-K mask shares。

### R1：GRank

```text
prepared priority-key arithmetic shares
→ protocol_iii_grank_party(...)
→ original-position additive rank shares
```

### R2：masked rank + DPF routing

公开均匀随机掩码后的 rank：

```text
hat_y_i = y_i + r_i mod 2^rank_bits
```

使用 `evalDPF_Payload` 得到目标 rank indicator additive shares。

要求：

- [ ] DPF 域为最小 `2^rank_bits` 域；
- [ ] 非 2 次幂只产生 DPF domain padding，不引入 dummy score；
- [ ] 不得调用 `evalDPF_EQ` 参与算术乘法；
- [ ] 不公开 DPF point、indicator 或 selected index。

### R3：share-preserving multiplication

使用 PR #4 的 share-preserving multiplication adapter：

```text
indicator arithmetic shares
× unit/payload arithmetic shares
→ product arithmetic shares
```

要求：

- [ ] 使用独立一次性 multiplication material；
- [ ] material 复用必须失败；
- [ ] secure 路径不得调用 `reconstruct(...)`；
- [ ] 不重构 rank；
- [ ] 不重构 DPF index；
- [ ] 不重构 selected index；
- [ ] 不重构最终 mask；
- [ ] 最终 arithmetic bit share → XOR bit share 不增加额外在线轮次。

正式记录：

```text
paper_core_online_rounds = 3
```

该标签表示项目的 Protocol III 模块化 3 轮核心，不得写成 Theorem 4.2 的精确 2 轮实现。

## 7.2 Raw-score 五轮包装

实现标签：

```text
m3_protocol_iii_raw_score_input_modular_5round_mask_output
```

路径：

```text
raw Q20.12 score arithmetic shares
→ protocol_i_raw_score_input_party
→ prepared priority-key arithmetic shares
→ agarwal_protocol_iii_modular_3round
→ original-order XOR Top-K mask shares
```

轮数：

```text
raw-score adapter = 2
Protocol III core = 3
total              = 5
```

报告必须同时保留：

```text
paper_core_online_rounds    = 3
unified_total_online_rounds = 5
```

不得把五轮端到端路径简称为论文三轮实现。

---

## 8. 阶段 6：统一验证与收尾

按顺序增加并运行：

### Primitive

- [ ] DPF 44 组 conformance；
- [ ] DPF Peer/Dealer 传输；
- [ ] DPF 独立进程传输；
- [ ] multiplication adapter 39 组；
- [ ] multiplication material 一次性状态检查；
- [ ] arithmetic bit share → XOR bit share。

### GRank 与 routing

- [ ] GRank 与 M1 oracle differential；
- [ ] scalar payload DPF routing；
- [ ] rank 0 / rank `n-1`；
- [ ] DPF domain padding；
- [ ] Top-K indicator aggregation；
- [ ] 完整 Top-K mask differential。

### 三独立进程 E2E

- [ ] P2 / Dealer、P0、P1 为三个独立进程；
- [ ] P2 在线阶段无连接和消息；
- [ ] 不用固定 sleep 代替协议同步；
- [ ] 不用 `.bin` 文件代替在线消息；
- [ ] secure runtime 不使用 oracle reconstruction。

显式运行：

```text
(128,2)
(128,8)
(256,2)
(256,8)
```

### 边界输入

- [ ] 重复值；
- [ ] 全相等；
- [ ] 负值；
- [ ] `INT32_MIN/MAX`；
- [ ] `K=1`；
- [ ] `K=n`；
- [ ] `n=1`；
- [ ] `n=3`；
- [ ] `n=5`；
- [ ] `n=127`；
- [ ] `n=128`；
- [ ] `n=129`。

### Fail-closed

- [ ] 错误 party；
- [ ] party/material 错配；
- [ ] session/fingerprint 错配；
- [ ] 错误长度；
- [ ] 材料不足；
- [ ] 材料复用；
- [ ] EOF；
- [ ] timeout；
- [ ] 空输入；
- [ ] `K=0`；
- [ ] `K>n`。

任何错误均不得 fallback 到明文、返回部分成功结果或重用随机性。

---

## 9. 最终验收要求

### Correctness

- [ ] mask 长度为 `n`；
- [ ] 每位重构为 0/1；
- [ ] mask 总和为 K；
- [ ] 与统一 M1 oracle 完全一致。

### Security-path invariants

- [ ] secure 路径不重构 raw score；
- [ ] secure 路径不重构 comparison bit；
- [ ] secure 路径不重构完整 rank；
- [ ] secure 路径不重构 DPF point/index；
- [ ] secure 路径不重构 selected index；
- [ ] secure 路径不重构最终 mask；
- [ ] P2 在线阶段无连接或消息。

### Round accounting

三轮核心和五轮包装必须分开记录：

```text
agarwal_protocol_iii_modular_3round
paper_core_online_rounds = 3
```

以及：

```text
m3_protocol_iii_raw_score_input_modular_5round_mask_output
paper_core_online_rounds    = 3
unified_total_online_rounds = 5
```

不得通过命名省略 raw-score adapter 的两轮成本。

### Communication accounting

分别记录：

- [ ] offline material bytes/bits；
- [ ] P0 sent/received；
- [ ] P1 sent/received；
- [ ] total/per-party communication；
- [ ] phase-level communication；
- [ ] GRank edge count；
- [ ] DCF evaluation count；
- [ ] DPF key/material count；
- [ ] multiplication material count。

未测数据保持 `NOT_MEASURED`。

### Repository hygiene

提交前必须：

```bash
git diff --check
git diff --quiet vfss-baseline-2026-09-03 -- VFSS-baseline
git status --short
```

确认 `VFSS-baseline/`、论文、本地参考工程、EMP 源码/安装目录、密钥、Dealer material dump、`.bin` 临时材料、日志、core dump 和构建物不进入提交。

---

## 10. 分支顺序

按顺序使用短生命周期分支：

1. `review/m1-m2-unified-validation`
2. 现有 `design/protocol-iii-mul-adapter` / PR #4
3. `m3/grank-runtime`
4. `m3/dpf-routing-core`
5. `m3/raw-score-process-e2e`

职责边界：

- `review/m1-m2-unified-validation`：M1.1/M2 统一复检、环境冻结、本启动文档；不实现 M3 runtime。
- `design/protocol-iii-mul-adapter`：PR #4 重整、adapter、DPF/adapter CTest 注册及相关文档。
- `m3/grank-runtime`：rank-share 接口、GRank differential 与 fail-closed tests。
- `m3/dpf-routing-core`：Protocol III 3-round core、DPF routing、multiplication integration、core metrics。
- `m3/raw-score-process-e2e`：两轮 raw-score adapter 接入、五轮 wrapper、三进程 E2E、标准规模 E2E 与最终报告。

---

## 11. 立即停止条件

任一阶段出现以下情况时，立即停止后续编码，先形成可复现证据和设计结论：

1. M1/M1.1 基线测试不全绿；
2. M2 EMP-OFF 或 EMP-ON 基线测试不全绿；
3. M2 四组标准 E2E 任一失败；
4. rank-share 接口需要改变冻结的 score/rank/mask 语义；
5. M3 需要复制第二套 comparison/rank 语义才能继续；
6. 三轮核心需要额外在线因果轮次；
7. secure 路径必须重构秘密值才能继续；
8. DPF routing 必须公开 DPF point、rank 或 selected index；
9. multiplication adapter 必须重构 operands/product；
10. P2 必须重新进入在线阶段；
11. 一次性材料无法 enforce；
12. fail-closed 错误必须降级到明文才能继续；
13. 论文证据、项目扩展和本地参考行为的标签发生冲突；
14. 实际实现无法继续使用既定证据等级和实现标签准确描述。

发生停止条件后先记录：

```text
revision
branch
exact command
input/config
expected behavior
actual behavior
exit code
last error output
security/round implication
```

再决定独立修复 PR、设计变更或证据等级调整。

---

## 12. 执行原则

整个 M3 实现期间持续遵守：

1. 论文定义、项目扩展、本地参考行为分层记录；
2. 先通过 primitive conformance，再进入协议集成；
3. 先通过 correctness，再开始性能结论；
4. 测试层可以重构用于 differential，secure runtime 不可以；
5. offline material 与 online traffic 分开计量；
6. online round 只按真实因果依赖计数；
7. build、依赖和大文件留在仓库外；
8. 不 force-push 共享历史；
9. 一个短生命周期分支只完成一个明确阶段；
10. 任何无法由仓库证据支持的结论都不得写成“已完成”。

M3 的最终目标不是仅让单独模块可运行，而是形成一个可审计、可测试、可计量的完整链路：

```text
raw score shares
→ 2-round priority-key preparation
→ 1-round GRank
→ 2-round modular DPF routing
→ original-order XOR Top-K mask shares
```

并始终明确：

```text
Protocol III core = 3 online rounds
raw-score unified path = 5 online rounds
```
