# BMW16 S11：VFSS C++ EXPERIMENTAL 组合验证报告

- 日期：2026-10-06
- worktree：`C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s11\MoE_Top-k_Reproduction_Evaluation`
- 分支：`codex/m6b-i-bmw16-s11`
- 起点/接收对象：`8770c207cd8d0de9ebb1a3a694fe2ddbef3efac3`（S10）
- 方案身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 验证结论：C++ party API 的有限功能链实际运行且冻结 oracle 对照通过；T 私有发材、独立 OS 进程 E2E、party-view 联合模拟未通过/未运行；不得命名为 secure。

## 1. 基线与接收审查

S11 worktree 从 S10 精确提交创建，开始时无差异。main/origin/main 为 `c3926c68fd14f270faa8b55234311071947fa080`。S4 Select 来源为 `04ed6f8a352ab2277434344094471dff82f83e4b`，冻结 Python 源 SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。S7 `0dbfc8d99b3e896329611793ef95ea822e241a67`、S8 `0ac6d5622fc60847f119672456d785ef4700e180`、S9 `a4402b1ef67d359a0d85997a86175497516ab20d` 是已有算法/runner 血缘，不修改其历史结论。

S10 报告称其正式 raw adapter 用 11 个 n≤8 样本及一个 n64 样本对照冻结 C++ oracle；S11 本轮阅读了报告、源码与 evidence summary，但**没有独立重跑 S10 的完整 11 行矩阵**。本报告下方只有 S11 新 C++ 运行结果是本轮直接观测。S10 集中控制器的 Python party runner 位于 `experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/party.py`；S11 不把它改名，而新增 VFSS C++ party API。S10 的进程参数、keygen 及原始证据不更改。

E20 `8986a40175f5f8eef56d765fef0cfa863e205985`、E21 `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`、PR #28 draft `57e239ee820a4208ac606663d877803f0b692206` 均只核对身份，没有修改。桌面主工作区的未提交差异保留。VFSS-baseline、Papers、参考工程均不在 S11 差异中。

## 2. 实现内容

新增 opt-in C++ API：

- `protocol_i_bmw16_experimental_material_generate(public_config)`：仅用公开 n/K/session/width 生成 raw-input、两份 shuffle、两个实例各四轮的全真实 pair Select key、独立 membership key；输入无关。
- `protocol_i_bmw16_experimental_raw_score_mask_party(...)`：party 本地调用正式 `protocol_i_raw_score_input_party` adapter，执行实际 shuffle state machine、C++ 两哨兵 S4-derived 四轮选择、uCMP/DCF membership、逆向 shuffle、LSB XOR share 与最终状态交换。
- 每一 Select 轮都先构造两个 partition 的全部当前边，再消费该轮 FSS key并交换当轮比较 share；只有比较 share 打开后才形成后轮边。相同轮/实例中的重复真实边缓存一次 key Eval，但仍按逻辑调用计数。不同轮、不同实例、membership 均为独立 key。
- API 返回状态及本方 mask share，不返回 selected handle、原始 index、score、threshold、比较位或完整 mask。双方实际交换的 uCMP share 相加会打开匿名句柄的真实比较 bit；这是明确泄露。
- `n=1` 为明确单元素恒等路径：输出 P0/P1 的 `1/0` share；不调用 raw-score adapter 或 shuffle。该分支不需要读取 score，因为 K 必为 1。
- 抽样使用固定公开 seed 的 C++ SplitMix64 + rejection-bounded partial Fisher–Yates，两实例/R1/R3 分域。该 sampler 与 S4 Python `random.Random` seeded stream 不逐位相同；无放回均匀性是实现规则，不是密码学随机 coin，也不继承 BMW16 Theorem 8。
- opt-in CMake 开关默认 OFF。接口和 CTest 的实现身份均含 `EXPERIMENTAL`，没有生产协议入口。

T 函数返回内存中的 P0/P1 move-only material 对象，尚无完整的 party-only bundle 序列化、AEAD/AAD、私有 T→party 通道、耐久领取/槽消费或崩溃恢复。party 实验 E2E 是一个进程的两个并发 C++ 线程，**不是**独立 T/P0/P1 进程或不同 OS 身份验证。

## 3. 构建和运行环境

- 主机：Windows 11；构建 shell：WSL Ubuntu 24.04
- CMake 3.28.3；GCC 13.3.0；Python 3.12.3
- T materials 在每个 case 中新生成；底层 shuffle/FSS 使用系统 `getrandom`，本轮没有固定或记录 T 侧随机 seed。固定 raw shares 和 algorithm seed 可复跑功能输入，但匿名置换改变时逐轮图/FNV 指纹、adaptive edge 总数和在线字节允许变化；证据文件绑定本次实际输出。
- 初次误用默认 WSL Ubuntu 20.04（CMake 3.16.3），配置在 `cmake_minimum_required(3.17)` 处失败；未改源码，随后明确指定 Ubuntu 24.04 成功构建。
- 构建目录：`/tmp/bmw16-s11-build`（不在仓库）

实际命令：

```bash
wsl.exe -d Ubuntu-24.04 -- bash -lc \
  'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s11/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s11-build -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON'
wsl.exe -d Ubuntu-24.04 -- bash -lc \
  'cmake --build /tmp/bmw16-s11-build --target moe_topk_bmw16_experimental_party_test -j2'
wsl.exe -d Ubuntu-24.04 -- bash -lc \
  'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s11/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s11-default-off -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=OFF -DBUILD_TESTING=ON && ctest --test-dir /tmp/bmw16-s11-default-off -N'
wsl.exe -d Ubuntu-24.04 -- bash -lc \
  'ctest --test-dir /tmp/bmw16-s11-build --output-on-failure -R "moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_ucmp_conformance_test|moe_topk_bmw16_experimental_party_test"'
wsl.exe -d Ubuntu-24.04 -- bash -lc \
  '/tmp/bmw16-s11-build/moe_topk_bmw16_experimental_party_test | tee /tmp/bmw16-s11-party-run.txt'
```

CTest 实际结果：4/4 PASS：

- `moe_topk_bmw16_experimental_party_test`
- `moe_topk_m2_score_input_conformance_test`
- `moe_topk_m2_parallel_shuffle_conformance_test`
- `moe_topk_m2_ucmp_conformance_test`

另用 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=OFF` 配置，并执行 `ctest -N`：列出 41 个测试，实验 party 测试目标不存在，确认实验路径可显式关闭且默认配置不接入该目标。

Direct C++ E2E 使用真实 `ProtocolIUcmpPartyMaterial` DCF Eval、S10 正式 adapter、两条 shuffle state machine 和 TEST_ONLY 的冻结 C++ `top_k_mask` oracle。每条用例每次新生成 key material。原始紧凑运行输出在 `/tmp/bmw16-s11-party-run.txt`；最终提交证据副本与哈希见第 8 节。

## 4. S11 逐例结果

S11 input shares 由 `std::mt19937_64(seed XOR 0x7261777368617265)` 拆分成 `Z/(2^32)`；party API 的抽样 seed 单独列出。`SUCCESS` 用两方 XOR mask 与仓库冻结 C++ oracle 逐位比较，并检查 n 长度和 weight K。强制 abort 用例在 Select 返回后注入，区分于自然算法失败；验证双方状态一致且 mask 为空。

| n | K | input（signed raw） | algorithm seed | 结果 | 关键计量 |
|---:|---:|---|---:|---|---|
| 1 | 1 | `INT32_MIN` | singleton | SUCCESS | 常量 share 分支；无网络/材料 |
| 2 | 1 | `[-8, 3]` | 4358 | SUCCESS/oracle PASS | 28 logical；每轮 12,12,2,2；6 Select slots，1 membership；11 相位 |
| 3 | 2 | `[6,-7,4]` | 4357 | SUCCESS/oracle PASS | 28 logical；每轮 12,12,2,2；16 Select slots，2 membership；11 相位 |
| 5 | 1 | `[INT32_MIN,INT32_MAX,0,-1,INT32_MAX]` | 4353 | SUCCESS/oracle PASS | 130 logical；每轮 90,36,2,2；33 Select slots，4 membership |
| 5 | 3 | 同上 | 4354 | SUCCESS/oracle PASS | 54 logical；每轮 30,20,2,2；38 Select slots，4 membership |
| 5 | 5 | 同上 | 4355 | SUCCESS/oracle PASS | 130 logical；每轮 90,36,2,2；33 Select slots，4 membership |
| 8 | 4 | `[9,9,9,9,9,9,9,9]` | 4356 | SUCCESS/oracle PASS | 130 logical；每轮 90,36,2,2；86 Select slots，7 membership |
| 5 | 2 | `[12,1,9,-4,2]` | 4359 | `ABORT_ALGORITHM_PROBABILITY`，注入 | 无 mask；9 相位；不是自然概率失败 |
| 64 | 8 | `score[i]=(17*i mod 23)-11` | 4360 | SUCCESS/oracle PASS | 非全量 R1：M=114，sample=86；17,748 logical，5,588 Select slots，63 membership；11 相位 |

自然随机概率 abort：本固定矩阵观察到 0 个。它只说明这些种子，不提供成功率界。没有筛掉失败种子；唯一 abort 是显式注入。S11 本轮没有记录 algorithm-native 自然概率失败样本。

### n=64 成本拆分

`n=64,K=8`：`r=64−8+1=57`，`N=65`，`low_pad=0`，`high_pad=49`，两个实例有效 M=114，median cut h=57，R1 `s=min(114,ceil(sqrt(64·114)))=86`，确实进入非全量抽样分支。本次运行 logical calls R1/R2/R3/R4=`7310/452/9984/2`，和为 17,748；dummy-related=`5148/326/6686/0`，同轮同实例重复请求均 0。unique Select FSS 槽 `5588`，membership 槽 `63`，raw adapter uCMP key `128`，总 uCMP party eval `5779`，DCF Eval/party `11558`，两方合计 `23116`。

每方预发 FSS key 槽数 `2*padded_n + 11*C(64,2) = 128 + 22,176 = 22,304`。其中 Select `16,128`、membership `2,016`、两份 shuffle CmpAgg `4,032`、raw score `128`。本实现计入但在线未用的 shuffle CmpAgg 槽为 `4,032/party`。`primitive_key_and_shuffle_bytes_per_party=22,175,120` 是序列化 primitive-key bytes 加两份 shuffle package 的精确长度；仍不含未实现的 raw mask-share/slot metadata 与外层认证 bundle，不能当作最终离线包大小。

记录到每方本地 loopback socket 的发送/接收均为 150,705 bytes，11 个 framed phases；这不是网络吞吐/延迟或 LAN/WAN 结果。PRG 次数、离线/在线独立计时、OS 身份隔离、CPU 峰值及最终 bundle bytes 均 `NOT_MEASURED`。

各轮逻辑边 plan 另带一个有序输入 ID 编码的 64-bit FNV-1a 指纹用于复跑比对；它不是密码学 hash，也不替代逐边审计。完整运行逐例文件 hash 用 SHA-256。

## 5. 单方联合视图、安全门

### 可观察字段

两方都知道公开 n/K、公开固定 algorithm seed、同一 shuffled slot namespace、四轮逻辑图（双方本地由相同 transcript 重建）、公开 shuffle masked values、每轮打开的真实比较位、U/V/W 及 pivot 派生状态、两任务交集得到的匿名 selected handle、算法最终 abort class，以及自己的 raw shares、party-local shuffle material、全部本方 DCF/uCMP key pool、读取轨迹和原序 mask share。API 不返回 selected handle，但两方都从打开的 bits 可以在本地推导并用它索引 membership。T 生成器在内存里暂时持有双方 materials。

### 裁决

固定原 index endpoint 的直接顺序泄露不适用于匿名共享 shuffle handle。对“严格 total order 经独立均匀隐藏双射重新贴匿名标签”，公开边、比较结果、两实例共享句柄及选择句柄的分布存在可模拟的置换等变方向。n=2 单条比较位边际为 1/2，不能单独构造区分事件。本轮不把 S6/S7 的原记录端点结论推广到匿名图。

但有限枚举/边际等变也没有关闭完整单方视图。待证引理必须联合覆盖：本地输入份额和正式 adapter 消息、本地 `sigma/tau` 与 shuffle messages、共享真实 handle 关系、整个相关节点 mask key pool（used/unused）、公开 masked operands、adaptive edge/value transcripts、abort、selected handle 与 inverse mask output shares。特别需要说明由同一随机节点 mask 产生的 `alpha_ij=r_i-r_j` 相关阈值，能否用仓库 FSS/DCF PRG 假设对整个多 key pool 作 hybrid，并覆盖自适应取用。现有 S11 实现与单 key DCF 安全定义没有给出这个定理/优势界。

因此 `PARTY_VIEW_SECURITY=NO-GO/NOT_PROVED`，缺口精确为“实际 Protocol I shuffle state 与相关全池 DCF/uCMP transcript 的联合模拟引理”；这不是已证明不可能，也不是通过测试获得安全结论。另一个独立 NO-GO 是 T 目前同时持有两方材料的内存返回对象，没有逐方 AEAD/AAD 发包、私有凭据交付、持久 one-shot claim。普通 frame 标签仅校验字段，不是 party-authenticated package。

## 6. 因果相位与端到端总轮数

```text
raw score shares
  ├─ carry input exchange / Eval     [phase 1]
  ├─ sign input exchange / Eval      [phase 2]
  ├─ forward shuffle share exchange  [phase 3]
  ├─ forward shuffle mask exchange  [phase 4]
  ├─ Select R1 open bits             [phase 5]
  ├─ Select R2 open bits             [phase 6]
  ├─ Select R3 open bits             [phase 7]
  ├─ Select R4 open bits             [phase 8]
  ├─ membership DCF Eval             [local; no exchange]
  ├─ inverse shuffle share exchange  [phase 9]
  ├─ inverse shuffle mask exchange  [phase 10]
  └─ status agreement               [phase 11]
```

这只是 S11 C++ experiment 的实测消息依赖。它打开比较 bit，使用公开固定 seed，材料由测试 T 内存对象提供，也没有完成 secure output abort 协调；不能叫作安全 Protocol I 轮数。正式网络层中一个 framed exchange 算作一因果消息相位，V4 的计量合同需在独立接收后冻结。

## 7. 门禁结论与下一动作

| 门 | 判定 | 具体依据 |
|---|---|---|
| S10 源码/报告一致性接收 | `PARTIAL_REVIEWED` | 读取源与报告；本轮没有重跑完整 S10 矩阵 |
| VFSS callable full functional path | `PASS / EXPERIMENTAL` | 新 C++ party API 从 raw shares 到原序 XOR mask share |
| Select four-layer causal schedule | `PASS` | R1→R4 每层一 exchange；下一边在本层 bit 打开后生成 |
| stable Kth oracle differential | `PASS_FOR_TESTED_CASES` | 7 个 n≥2 SUCCESS case 逐位 oracle PASS；n=1 恒等分支；1 个注入 abort 无 mask |
| natural failure behavior | `INCOMPLETE` | 本轮无自然 abort；已有项目失败类别无兜底，需保留新 RNG 失败种子持续采样 |
| authenticated offline party bundles | `NO-GO` | 缺 versioned party-only schema、AEAD/AAD、分方通道和持久领取/消费接口 |
| full joint party-view security | `NO-GO / NOT_PROVED` | 缺相关阈值多 key pool 与实际 shuffle/adaptive transcript 的组合 simulator |
| independent T/P0/P1 OS process E2E | `NOT_RUN` | 本轮为一个进程两个 party threads，不能计为进程隔离 |
| secure VFSS API | `NOT_CREATED` | 只有 opt-in EXPERIMENTAL API |
| official V4 LAN/WAN / six-scheme performance | `NOT_STARTED` | 安全、发材、独立进程与指标门未接收 |

最小后续任务：先定义和验证 P0/P1 party-only encrypted bundle schema/one-shot store，再提供单方联合视图所需的 shuffle + correlated multi-key DCF simulation lemma。安全证明之前，可在隔离 EXPERIMENTAL API 上补三进程 harness，但不能把它计作 secure E2E 或正式性能矩阵。

## 8. 复核文件、源码和二进制哈希

仓库内本报告及决策记录代码与缺口。原始小型运行输出保存在本报告所引证据文件。构建二进制放在 `/tmp/bmw16-s11-build/moe_topk_bmw16_experimental_party_test`，只报告哈希，不进 Git。

| 文件 | SHA-256 |
|---|---|
| `VFSS/include/moe_topk/experimental_bmw16_select_party.h` | `746b96573e74f2c12af72c0db705a2b482a8151cf86e14f5fc42bc2fcf4c625c` |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `69880a6867543d8f5fa0cbaedae3fabe768e31c8a5acb0e1d9474c4fa09d52f0` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | `7c5d33f31b67ef2e04bf592c410e9dda2a987a4a6fd65f202660771eeabbb34b` |
| binary `/tmp/bmw16-s11-build/moe_topk_bmw16_experimental_party_test` | `99268a41163d032cd9c76dbdc2d8071849cbe1a5480d975308cd4d63a7707f10` |
| output `docs/reproduction/evidence/BMW16_S11_CPP_PARTY_RUN.txt` | `55a058e157ae895342a0fe00a07db07f9a73dc6f63502d8718c40598357d1153` |

完整 S11 文件 hash、S10 adapter/backend 来源 hash 见 [BMW16_S11_SHA256SUMS.txt](evidence/BMW16_S11_SHA256SUMS.txt)。

本轮不运行正式 LAN/WAN matrix；V4 性能表的离线/在线时间、PRG 调用、最终 party bundle bytes、独立 OS process 通信、网络 latency、吞吐和失败率字段均 `NOT_MEASURED`。
