# BMW16 S14：S13 复核、one-shot 材料优化与 n=128 验证报告

- 日期：2026-10-06
- 起始 HEAD：`0eab2e358bf34904fbca542629f8662db9dd62f3`（S13）
- 分支 / worktree：`codex/m6b-i-bmw16-s14` / `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s14\MoE_Top-k_Reproduction_Evaluation`
- 标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 本报告适用性：S14 测试与本机同 host loopback pilot；不属于正式 V4 LAN/WAN 结果。

## 1. 完成摘要

实际复用了 S13 VFSS C++ party API，没有重写 Select 或安全入口。完成了两项工程优化和一个规模扩展：

1. 以一次持久 bundle burn 替代每个 uCMP slot 的逐次 `O_EXCL + fdatasync + directory fsync`；同进程用 mutex/set 审计 slot 唯一性，跨 PID/fork guard 拒绝继续用继承状态。
2. 给 BMW16 专用前向/逆向 shuffle 生成 shuffle-only 材料，移除实验路径从不调用的两池 CmpAgg keys；普通 Protocol I shuffle 默认行为不变。
3. 经 checked capacity preflight 后将实验硬上限从 n≤64 扩至 n≤128；在 n=128/K=2、8、80 做了 S4/C++ common tape differential 和 T/P0/P1 三进程 E2E。

结果：n128 的三个差分均逐层 PASS；n128 三个独立进程样本都产生原序、恰 K 位、逐位等于冻结 oracle 的 mask shares。S14 最终 59 行独立进程矩阵：55 SUCCESS、1 TEST_ONLY 注入概率 abort、1 bundle 篡改、1 意外参数异常、1 peer close；自然算法 abort 为 0，错误 mask 为 0。没有从零次自然 abort 推导理论概率。条件性联合安全证明和生产私有认证发材没有闭合，所以仍 `EXPERIMENTAL`。

## 2. 起点与 S13 独立检查

起始 `main`/`origin/main` 是 `c3926c68fd14f270faa8b55234311071947fa080`，S13 起点为其后代。开始时 S14 worktree 清洁。桌面主工作区在 `feat/m6a-performance-evaluation`、HEAD 同 main，但保留了 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md` 的未提交修改、`siamjdiscrmath.pdf` 和 M6A decisions/reproduction 未跟踪资料；全部原样留存。现有 worktree registry 含 S4、S10–S13、E20/E21 和若干 M6A 试验 worktree；S14 另建，不借用其未提交文件。E20/E21/PR #28 revisions 见 S14 决策来源表；本轮无修改。

S13 来源及历史报告通过源码交叉核对：

- party API 仍是 `protocol_i_bmw16_experimental_raw_score_mask_party`；S14 没有第二份 Select。
- raw score 走仓库正式 `protocol_i_raw_score_input_party` 及 34-bit carry/sign comparator；反转后的稳定 priority key 在 `comparison_bits=33+index_bits` 环中传入 Select。
- key 顺序保持 signed score 降序、`original_index` 升序；真实记录身份随共同隐藏 shuffle 绑定。`n=1,K=1` 走明确快捷分支。
- 两份 Select 在同一个 R1–R4 调度上并行；membership 的方向为稳定 priority 的 `R_i >= R_selected`，自比较本地给 1；inverse 使用同一前向置换对应的 material relation。
- party 只返回 mask shares；重构与 frozen `topk_oracle.h` 比较只在 TEST_ONLY parent controller 完成。
- 退出码/状态仍是 0/10/20/30/70；发生 peer 失联时仅宣告存活方 local abort，不伪称双方 agreement。
- S13 报告的 98.7% claim 占比与 4,032 unused forward/inverse CmpAgg 槽可由旧代码路径解释：逐槽回调触发持久同步；BMW16 runtime 没有消费 shuffle round-3 CmpAgg pool。

历史数据与本轮新数据分开：S13 的 n64/K8 是一次样本，party API 约 8.954 s，其中旧 durable claim callback 约 8.841 s；S14 pilot 是独立 revision 的 1 warmup + 5 run，不把两个统计集合拼成同一正式性能中位数。

## 3. 一次性领取优化与失败合同

### 3.1 逻辑合同

对每方，每一份生成材料的不可复用 identity 在执行开始前持久 claim 一次。claim marker 以排他创建成功后，任何路径（正常完成、预期 abort、意外异常、进程 crash）都不释放该 bundle。进程内 `ProcessSlotClaimSet` 以 mutex + unordered set 对真实消费过的 `(stage,task,round,endpoints,slot ID)` 做唯一检查；created PID 不匹配即拒绝，以免 fork 后两个 child 共用内存 guard。持久 claim 与 in-memory audit 的职责不互相替代：前者跨重启 burn，后者本次进程去重。

正确性论证：一个 pair key 对应一个 uCMP/DCF Eval；一份 party bundle 对应 fresh 随机生成的整池 key。整包持久 claim 使同 bundle 在崩溃后不能重用，因此无需每槽单独 durable write；进程内 set 阻止同一执行中的意外重复 Eval。测试里“claim 后崩溃再取”拒绝，“同进程重复 slot”拒绝，“重复 bundle claim”拒绝。未尝试自动恢复/续跑，失败时废弃剩余材料。

关键代码位置：`experimental_bmw16_material_bundle.h:52-82` 定义 bundle claim 与 PID-bound process guard；`experimental_bmw16_material_bundle.cpp:320-335` 实现 durable marker 调用和进程内唯一性；`experimental_bmw16_select_party.cpp:46-56` 把 manifest lookup 与 process guard 分开计时；`experimental_bmw16_select_party.cpp:732-775` 校验有限域并计算 S14 9-pair-pool 槽数；三进程 harness 在 `experimental_bmw16_3proc_test.cpp:160-206` 检查 duplicate、crash/restart 和并发 bundle claim，在 `:265-305` 为每个 party 先 durable claim 后构造进程 guard。

需要维护的假设/边界：claim store 必须 party-local、耐久且与 bundle identity 唯一对应；备份恢复、复制 bundle 到另一未共享 claim store、跨主机并发 claim 均不由当前本地 helper 覆盖。生产服务要以共识/事务或单主 durable state 保持唯一性，不能把测试文件目录当成生产多机 one-shot 证明。

### 3.2 CmpAgg pool 删除

S14 新增 `include_cmpagg_material`，默认 true。Protocol I generic shuffle 的现有调用保持 true；只有 BMW16 bundle 的前向和逆向发材明确 false。shuffle-only 反序列化标记 `has_cmpagg_material=false`；调用依赖 CmpAgg 的 round 3 明确报错。通用 shuffle conformance / process E2E 仍通过。

full pair pool 数从 11 变 9：8 个 Select task-round pools + membership 1 pool。每方槽数减少 `2*C(n,2)`：n64 去掉 4,032；n128 去掉 16,256。bundle manifest 和 key 数目按 S14 schema 同步，不能留下“未发槽仍可 claim”的影子槽。

实现位置：`protocol_i_parallel_shuffle.h:18-36` 将 `include_cmpagg_material`/`has_cmpagg_material` 加入材料合同并默认 true；`protocol_i_parallel_shuffle.cpp:333-340,369-419,483` 按 profile 生成、编码/解码并拒绝缺少 CmpAgg 的 round3；BMW16 generator 在 `experimental_bmw16_select_party.cpp:732-775` 只选择 shuffle-only profile；bundle schema v2 在 `experimental_bmw16_material_bundle.cpp:175-220` 拒绝旧/多余 CmpAgg 数据。

## 4. 测试顺序、输入和实际结果

### 4.1 环境 / 构建

- WSL Ubuntu 24.04；GCC 13.3.0；CMake 3.28.3；OpenSSL 3.0.13；Release。
- BUILD_TESTING=ON，`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`；fresh/reused isolated build dir `/tmp/bmw16-s14-rebuild`。
- 可用 `/tmp` 空间 990,310,723,584 B，可用内存约 7,560,425,472 B；无额外 ulimit，无 swap。
- CTest final log SHA-256: `0505e6597e3fcee38dc9cb27026653c7fb13bcec6527d9be06e295d2ebd60baa`；build log SHA-256 `0c08aa87477193c4fa34bc3c67be29dad0448d0ff64e38d2f60d2b9b367652a4`。
- 3-process test executable SHA-256：`75f0010adfe4d5792b3adccbd97f2c0d135eeca3751176fcd97fdf2852c2f2cf`。

相关 CTest 6/6 PASS：BMW16 party API、uCMP、score input、transport、parallel shuffle conformance、parallel shuffle process E2E。没有把未构建的其他里程碑测试称为通过。完整 raw output/log 外存归档，见第 8 节。

### 4.2 Common-tape differential

执行 S14 `run_n128_common_tape.py`，固定第一 seed 814000，三个输入分别为：

| n/K | seed | 输入形状 | 结果 |
|---|---:|---|---|
| 128/2 | 814000 | 含 INT32_MIN/MAX 和确定序列分数 | R1–R4/pivots/U/V/W/bit/selected handle 逐层 PASS |
| 128/8 | 814001 | 模 23 的重复分数 | 逐层 PASS |
| 128/80 | 814002 | 固定 seed 的随机 signed 分数 | 逐层 PASS |

矩阵 SHA-256：`4948cee08b88a7208145cebc1eea1a1639046710980a714ac05afcb840e0558e`。Tape、C++ trace 和 differential report 逐案保留在外部 archive；common tape 是相同 sampling words，不是仅用相同整数 seed 比较不同语言 PRNG。

**有限参数/概率审计：**固定 S4 源中没有 `epsilon` 参数；对每个 median 长度 M，R1 样本 `s=min(M,ceil√(64M))`，R1 ranks、`U_cap=min(M,ceil(8M^(3/4)))`、R3 `v=min(u,ceil√(64u²/M))` 和 `W_cap=2ceil√(4M)+1` 都按源代码中的整数 ceil 执行。任意 K wrapper 给 `r=n−K+1`、`N=n+1`、`M=2max(K,n−K+1)`；n=1 另走 shortcut。详细公式及 bracket/rank/window abort 分类见 [S14 决策 §4](../decisions/BMW16_S14_ONE_SHOT_MATERIAL_AND_N128_BOUNDED_PILOT_2026-10-06.md)。这组参数是 `PROJECT_DERIVED` finite rules，S4 source 声明 epsilon 不使用，且 S12/S13 已记录它不满足直接移用 BMW16 Theorem 8 的条件；所以 S14 不报告已证明的高概率成功界或 `≤10^-6` abort 界。C++ 正式 TEST_ONLY coin 是双方各 `getrandom(32)`、交换后 XOR，再用 SHA-256 counter stream 与 rejection-bounded 无放回抽样；common-tape 只用于 differential。自然算法 abort 0/55 是观察值，不是概率证明。

### 4.3 独立进程 E2E

最终 3proc runner 在原 S13 56 case 后加入三组 n128：K2/K8/K80、input seeds 128002/128008/128080，均 SUCCESS。全矩阵 59 行状态分解如下：

| 状态 | 次数 | 输出合同 |
|---|---:|---|
| SUCCESS | 55 | 两 party 状态 0；TEST_ONLY 重构后长度 n、bit、weight K、原序 oracle 都匹配 |
| 测试注入算法概率 abort | 1 | 双方 agreement；两份 mask 均不存在 |
| bundle/material 篡改 | 1 | material error 20；无 mask |
| 意外 API 输入异常 | 1 | unexpected error 70；非成功不写 mask |
| peer close | 1 | 存活方通信状态 30 / LOCAL_ONLY；不宣称双边 ACK |
| 自然算法 abort | 0/55 正常成功路径观察 | 只报观察值；非失败界 |
| COMPLETED_WRONG / 错误成功 mask | 0 | 任一错误成功均会令 runner 失败 |

最终复核 E2E log SHA-256 `85b722044a2a23b9be34b5c43ea97e389ee3373d6f65610baae9be74c4aa94ba`（59 case，exit 0）。此前完整 E2E 日志 `0a7b2e5a2fef48ced1c7bb892c108f67466ffed8d62d7bcd0348b47b6fe8d9c2` 仍保留在 archive。n128 E2E suite 用 `/usr/bin/time -v` 峰值 RSS 523,452 KiB、墙钟 6.71 s。功能测试不建立当前 S4-derived candidate 的有限样本失败率界，也不替代对论文原定理适用条件的证明。

### 4.4 one-shot loopback pilot

每个配置 1 warmup + 5 measured；每次 fresh T materials。摘要 median `[min,max]`：

| 数值 | 64/8 | 128/80 |
|---|---:|---:|
| wall | 0.42035 `[0.39885,0.64759]` s | 1.87339 `[1.60825,3.18062]` s |
| party online API | 47,149 `[44,570,48,711]` μs | 319,935 `[311,627,332,306]` μs |
| T generation/seal+delivery total | 224,847 `[208,744,399,125]` μs | 1,148,157 `[883,984,2,071,848]` μs |
| durable bundle claim | 2,940 `[1,904,10,342]` μs | 39,554 `[1,990,63,954]` μs |
| process slot guard / key slot lookup | 243 `[228,285]` / 15,637 `[13,606,16,439]` μs | 1,114 `[847,1,448]` / 211,800 `[205,071,221,847]` μs |
| actual package / party | 18,654,121 B | 76,701,577 B |
| logical Select / unique slots | 17,568 / 5,622 | 25,855 / 17,945 |
| membership; uCMP / DCF party Eval | 63; 5,813 / 11,626 | 127; 18,328 / 36,656 |
| P0 sent/received; 12 phases | 149,345/149,345 B | 223,833/223,833 B |
| peak party RSS / T RSS | 83,968 / 127,812 KiB | 320,000 / 523,448 KiB |

Pilot per次 JSON 包含 round logical/unique/dummy/repeat/edge digest 及 raw/shuffle/R1–R4/membership/inverse/status 阶段时间。S14 控制器是本机 root TEST_ONLY harness，材料“delivery”是隔离 UID 目录的写入/归属调整；这不是 T 到远程 party 的正式认证私有通道。外层 wall、T offline 和 party online 计时范围不同，不应简单相加。主机 socketpair/loopback 不等同 LAN/WAN。

旧 S13 和 S14 的 n64 输入 profile/无额外资源上限相同，但旧为一次测量，新为 5 次；每轮随机边由 OS CSPRNG 生成，故不作 paired timing 或严格 speedup 结论。S13 的 8.84 s slot claim 与 S14 的 2.94 ms durable bundle claim、243 μs process-local claim 仅支持定位逐槽 fsync 的工程原因。

## 5. 计量和容量核对

独立计量字段严格分开：

- 逻辑 comparison calls 与预发 key slots；
- 真实-真实、dummy-related、重复请求、独立唯一槽；
- 成功 run 实际 consumed slots 与未用预发槽；S14 shuffle CmpAgg unused 为 0，removed slots 单列；
- 每方 uCMP evaluation；每次 uCMP 有两次 DCF Eval，因此报告 2x DCF Eval；
- sampler PRF words 与 FSS AES PRG calls（后者 `NOT_MEASURED`）；
- 每方向字节、12 因果消息 phase、分阶段计时、T 和 party RSS；
- lower-bound bytes（checked arithmetic）与实际 AEAD bundle bytes。

完整容量表和 lower-bound derivation 见 S14 决策文件。脚本拒绝 uint64 overflow，n=128 真实 E2E/pilot 通过；n≥256 仅 `PRECHECK_ONLY/API_CAP_128`，不伪造运行时间、通信或成功率。

## 6. 安全归约审查结论

候选 leakage 是公开的 n/K/session/width/padding、masked shuffled `public_z`、两 task 共用匿名 handle、公开抽样/pivot/U/V/W/R1–R4 图与比较 bit、selected anonymous handle、abort/阶段/slot-access/流量和输出 shares。P0/P1 的完整 local view 还含本地输入 share、shuffle state/message、全部 bundle key（包含未访问槽）、本地 Eval share 与 durable/in-memory claim history。

可以条件性描述 AES-CTR PRNG 的多流替换：cryptoTools 每个 PRNG stream 使用独立 OS seed 并输出 AES-counter stream；若该实现满足所用 AES-PRG 假设，逐 stream hybrid 的优势按实际流/块 query 求和。S14 没有改 keygen PRNG 或强加 slot domain separation，理由是 S13/S14 的关键缺口不是“随机流确定性”，而是下一步所需的协议视图安全定义尚未由仓库具体 DCF 实现或引用定理支持。

第一个缺口：把整份 DCF party key pool（各 `alpha_ij` 共用 shuffle 节点 mask 差、含未用槽）替换为一个只依赖 leakage/output share 的模拟分布，同时保留 party shuffle 辅助状态、公开 masked operands 及由早轮 Eval bit 适应性决定的后续查询。已有单 key FSS 隐私或 AES-GCM 封装不能推出这一条件化 multi-key/adaptive-query 模拟。没有构造成功 simulator，也没有实际两世界区分 witness；因此标记 `NOT_PROVED / NO-GO`，不写“不可行”。完整 hybrid 目标与最小下一步见决策文档第 7 节。

Separate engineering gate：理想私有认证通道前提下的数学组合安全证明，与当前 root 预置 AES-GCM key/本机 UID fixture 下的工程行为是两件事。前者未闭合；后者只在本地测试合同中验证。生产 party 凭据建立与远程 T 交付未做。

## 7. 可复跑命令及环境

从 repo root 执行。主要源码生成 binary 在报告 hash 中固定；完整输出存外部归档：

```powershell
# 资源/容量检查：解析下界，不是性能运行
python experiments/TEST_ONLY_BMW16_S14/capacity_preflight.py

# S14 fresh/isolated Release build（实验开关显式 opt-in）
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc `
  'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s14/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s14-rebuild -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON && cmake --build /tmp/bmw16-s14-rebuild --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test -j2 && ctest --test-dir /tmp/bmw16-s14-rebuild -R "moe_topk_(bmw16_experimental|m2_ucmp|m2_score_input|m2_transport|m2_parallel_shuffle)" --output-on-failure'

# 59-row independent-process E2E
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc '/tmp/bmw16-s14-rebuild/moe_topk_bmw16_experimental_3proc_test'

# n=128 S4/C++ common tape; output directory must be outside repository
python experiments/TEST_ONLY_BMW16_S14/run_n128_common_tape.py `
  --cpp-test /tmp/bmw16-s14-rebuild/moe_topk_bmw16_experimental_party_test `
  --s4 C:/Users/28641/.codex/worktrees/bmw16-s4/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py `
  --out C:/Users/28641/.codex/artifacts/BMW16_S14_20261006/n128_common_tape_replay `
  --first-seed 814000

# 1 warmup + 5 same-host loopback pilot runs per profile
python experiments/TEST_ONLY_BMW16_S14/run_pilot.py `
  --binary /tmp/bmw16-s14-rebuild/moe_topk_bmw16_experimental_3proc_test `
  --out C:/Users/28641/.codex/artifacts/BMW16_S14_20261006/pilot_replay
```

Note: output paths above refer to new directories in external raw evidence storage. `run_pilot.py` requires an empty output directory. In this worktree the WSL build used the absolute S14 worktree path; the command is not meant to imply the desktop main checkout was built.

## 8. 证据及哈希

原始输出不提交到 Git，外部归档为：

```text
C:\Users\28641\.codex\artifacts\BMW16_S14_20261006
```

其中 `wsl/` 保存 fresh build、final CTest、3proc matrix、resource preflight 和早期调试失败；`n128_common_tape/` 保存每个 tape、trace 和 differential；`pilot_final/` 保存环境、warmup/五次 raw log 和完整 `pilot.json`。外部归档逐文件 SHA-256 manifest：`SHA256SUMS.txt`，manifest SHA-256 `8899b04952e8d6d287a7d74804f9bc1eaece02e52174c1254eb38cf3736360ef`，64 项逐项重验通过。没有把 key、解封凭据、输入 share 或完整 mask 放入提交日志。

| 证据 | SHA-256 |
|---|---|
| S4 `select4r.py` | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| n128 common-tape matrix | `4948cee08b88a7208145cebc1eea1a1639046710980a714ac05afcb840e0558e` |
| final E2E log | `0a7b2e5a2fef48ced1c7bb892c108f67466ffed8d62d7bcd0348b47b6fe8d9c2` |
| final 3proc binary | `75f0010adfe4d5792b3adccbd97f2c0d135eeca3751176fcd97fdf2852c2f2cf` |
| CTest final log | `0505e6597e3fcee38dc9cb27026653c7fb13bcec6527d9be06e295d2ebd60baa` |
| final build log | `0c08aa87477193c4fa34bc3c67be29dad0448d0ff64e38d2f60d2b9b367652a4` |
| final pilot.json | `efb2f35cb5db002354555335c0b47a66da29bd9c00e2e48e662eb4e0a1118158` |
| checked capacity JSON | `a26203fe555eed7072e84847eff30f56dadb859da112896259b71d2ef9365495` |
| archive manifest | `8add3cc793f06eafd69176a829f389b144f9013edf9866e3663db05a44e94c7f` |

被测代码 hash、report commit revision 在提交前追加到 evidence index。所有正式 WAN/LAN 和 FSS AES PRG 数值均为 `NOT_MEASURED`。

## 9. 门禁

| 门 | S14 结论 |
|---|---|
| `S13_SOURCE_AND_METRIC_REVIEW` | `PASS_WITH_EXPERIMENTAL_BOUNDARY` |
| `ONE_SHOT_MATERIAL_CLAIM` | `PASS_FOR_LOCAL_BUNDLE_BURN_CONTRACT` |
| `SHUFFLE_ONLY_BMW16_MATERIAL` | `PASS`; generic Protocol I CmpAgg default retained |
| `N128_COMMON_TAPE_DIFFERENTIAL` | `PASS` 3/3 |
| `N128_INDEPENDENT_PROCESS_E2E` | `PASS` 3/3 cases; total 59 case matrix has 55 SUCCESS |
| `CONDITIONAL_SECURITY` | `NO-GO / correlated multi-key adaptive DCF view + complete shuffle simulator missing` |
| `PRODUCTION_DELIVERY` | `NO-GO / local preprovisioned TEST_ONLY credentials only` |
| `PERFORMANCE_READY / V4` | `NOT_READY / same-host diagnostic only` |

不推送、不合并、不更新 PR #28。报告源 revision 与当前 S14 本地提交身份见附属 evidence index。
