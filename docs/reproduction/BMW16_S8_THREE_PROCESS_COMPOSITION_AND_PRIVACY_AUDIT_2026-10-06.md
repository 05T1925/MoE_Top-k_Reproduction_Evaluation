# BMW16 S8：三进程功能组合、材料审计与匿名图隐私复核

- 日期：2026-10-06
- 方案名：**Protocol I + BMW16-derived Select + DCF**
- 身份：`PROJECT_DERIVED`；代码身份：`EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS`
- 结论：**分方功能候选已实际运行；公开匿名 adaptive graph 的有限/条件性输入无关性已复核；完整 party-view security 仍 NO-GO；未创建 secure 入口。**

## 1. 工作区和来源

工作在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s8\MoE_Top-k_Reproduction_Evaluation`、分支 `codex/m6b-i-bmw16-s8` 完成。`main` 与 `origin/main` 起点均为 `c3926c68fd14f270faa8b55234311071947fa080`。S7 指定提交为 `0dbfc8d99b3e896329611793ef95ea822e241a67`；本分支从 main 选择性 cherry-pick S7 文件树，父提交 `c0d0e1237037ca9d73496e3ebc9785d8b2c6b4e9`。所以 S4/S7 选择源码沿用 `experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py`，S8 没有创建未经同步的第二份算法。

| 来源 | 已核验身份 | 在本轮用途/限制 |
|---|---|---|
| BMW16 2016 PDF | 44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23` | 仅保留 S4/S5 论文来源界限；S8 不借 Theorem 8 概率或比较常数 |
| 辅助 PDF `Papers/017.pdf` | 249 页，SHA-256 `3EAE3A93F12AB71EE8C040167F21351FCF3FF54DC423985A04087E3E90CB15` | 仅交叉参考，不作独立证明 |
| S4 | `04ed6f8a352ab2277434344094471dff82f83e4b`；源 SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` | 原 Select 代码曾由 S7 选择性引入；当前 S8 调用同一份源 |
| S5/S6/S7 | `67c3f302a8564b68616f7d7247c8cee3415ace31` / `c4f34e7e8f9fd954637573fe62b8d9b567362298` / `0dbfc8d99b3e896329611793ef95ea822e241a67` | 复核起点；保持 S7 的失败、原始证据和身份标签 |
| E20 | `8986a40175f5f8eef56d765fef0cfa863e205985`，实现源 `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f` | 只审查其输入无关流式发材/一次性领取工程合同；未复制，AAV86 接收结论不转移 |

仓库 `AGENTS.md`、`PROJECT.md`、实施计划、性能计划、S7 决策/报告以及相关 Protocol I shuffle/uCMP 和 C++ oracle 接口均已阅读。没有修改 `VFSS/`、`VFSS-baseline/`、旧 Protocol I/AAV86、E20/E21、论文或本地参考树；没有触碰 PR #28 或桌面主工作区既有差异。

## 2. S7 反例与组合行为复核

S7 最终 10 行矩阵为 10/10 oracle pass；位宽修正前 8 条错误记录是 `COMPLETED_WRONG/NO_MASK`，对应实际 uCMP 最高输入位要求 `34+b` 而不是 `33+b`，不能归为 Select 概率失败。S7 n=8,K=3,algorithm seed=42 的冻结轨迹选中 original index 7，oracle mask 为 `[0,1,0,0,0,1,0,1]`；R1–R4 逻辑比较为 `132/44/2/2` 共 180，独立边审计为 178 个 Select uCMP 槽/712 次两方 DCF Eval、membership 8 槽/32 次两方 Eval，另生成了 28 个未用 shuffle CmpAgg key slots（wire bytes 60,984、Eval=0）。S7 为 dummy/sentinel 逻辑边也生成 FSS key；S8 依据公开类别在本地算 dummy 边，只为真实-真实边领取 uCMP key，因此逻辑调用不变但 key/Eval 计数不同。S7 轨迹属于集中式控制器：控制器看到原始 score/shares、shuffle 映射、比较结果与 selected stable key，并在线按需生成 FSS key、中心化 scatter；因此 S7 不构成分方安全路径。S8 n=8,K=3 的新候选实测为 86 Select slots、7 membership slots、372 两方 DCF Eval；S7/S8 计数合同不同，不可直接作性能优劣比较。E20 的全池存储/AEAD/一次性领取接收也不能证明此处 BMW 图的联合隐私。

S6 n=2 泄露反例需要区分端点语义：能对应原记录 ID 的公开比较位确实区分两种 score 世界；若端点是未知均匀匿名 handle，则不能据此推出相同泄露。S8 的真实 party 进程不打开 original ID 或 shuffle map。它把公开内容限定为匿名 handle 图、比较结果及其派生状态，但在 S8 仍只是**实验允许泄露集合**，尚未由项目隐私策略正式接受。

uCMP 方向继续采用 S7 的真实实现语义 `eval_strict_lt -> c=[z_lo<z_hi]`。对反向边，party share 转换 `(c0,c1) -> (1-c0 mod 2^64, -c1 mod 2^64)`，逻辑调用仍计数；同轮同 task 的规范无序边只求一次。缓存键含 task，材料槽含 stage/round/task/endpoints；跨轮、跨 Select task 和 membership 不复用。独立边审计会重算反向调用数和实际规范槽数。

## 3. 实现路径与实际角色隔离

源码位于 `experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/`。S8 添加 C++ offline/package/uCMP/shuffle helper；Python `party.py` 为单方端点；`run_s8.py` 是明确标记 TEST_ONLY 的测试 harness；Select 源仍从 S7 import。

实际顺序如下：

1. harness 载入固定的 TEST_ONLY score fixture；T C++ 进程收到的只有 n、K、位宽、session/fingerprint 与包输出位置，不接收 score/shares。T 产生材料包并退出。
2. harness 才创建两份各自只含本方 34-bit 加法份额的临时输入文件，然后并发启动 P0/P1 Python 进程。双方各启动本地 C++ child，独立读取本方离线材料。
3. P0/P1 在 TCP loopback 交换现有 shuffle state machine 的前两轮消息，得到同一隐藏匿名真实 handle 域上的加法共享 `π(x)` 和公开 masked values `π(x)+r`。这是 S8 项目衍生的两轮 shuffle 使用：party helper 调用 `prepare_round1`、`receive_round1_prepare_round2`、`receive_round2`，没有调用 `evaluate_cmpagg_prepare_round3/receive_round3`，因此没有运行完整的三轮 CmpAgg/reorder shuffle。Select 代码是 S4 的 S7 版本；每一轮先冻结完整边计划、两方校验计划相同，再执行实际 uCMP party key，交换 share 后重构比较位并决定下一轮。
4. 两个 Select task 共享同一个真实匿名 handle 集合，dummy/sentinel 任务局部命名。selected 值只作为公开匿名 handle。各方对另外 n−1 项使用离线 DCF/uCMP key 计算 `[R_i≥R_selected]`；自比较本地输出 share 1。输出通过独立的、目标置换为 `π^{-1}` 的前两轮 shuffle material 逆路由；它同样没有执行 CmpAgg/reorder 第三轮。
5. party 子进程只写本地 XOR mask share；harness 在进程外重构并调用仓库冻结 C++ `top_k_mask` oracle。它同时持有 clear fixture 与份额，因此这是测试验证器，不是协议角色。

反向 shuffle material 是本实验 C++ 的 `make_shuffle_for_pi(π^{-1})` 自定义 dealer 构造；它依赖 trusted T 持有第一轮 shuffle 的 clear `π`。这是项目扩展，不由 BMW16、E20 或 Protocol I 完整 shuffle 单原语接口自动证明。

### 离线覆盖和一次性槽

| 类别 | 每方预发槽数 |
|---|---:|
| 两个 Select × 四轮 × 每个真实无序 handle pair | `8·C(n,2)` |
| membership 所有可能阈值 handle pair | `C(n,2)` |
| forward + inverse shuffle 里实际未使用的 CmpAgg slots | `2·C(n,2)` |
| 合计 key object | `11·C(n,2)`，另含 shuffle state |

槽绑定 session、party、实例 task、阶段、round、两端点、宽度、mask-id；package 同时含 n、K、fingerprint。测试 helper 加载完整池，并以进程内 `used` 集合拒绝重复 slot；package 另有 `O_CREAT|O_EXCL` + `fsync` 的 `.claimed` 文件拒绝第二次整包领取。keygen 发生在 T，不在线补发。这里的“绑定”和领取只由实验格式、文件系统权限和测试检查保证：包未 AEAD/MAC 密封，harness 可访问临时目录，不能称为安全分发或 E20 等价流式存储。

n=8 时理论/实际序列化池为 `11*C(8,2)=308` 个对象/party，每方包 300,516 bytes，含 56 个 shuffle CmpAgg key objects；n=64 时是 22,176 个对象/party，每方包 23,139,976 bytes，含 4,032 个 shuffle CmpAgg objects。包字节是本次实际生成、写入并 `stat` 的临时文件大小；不是安全持久化容量或正式性能结果。

## 4. 稳定键、成员方向和输出合同

原始 score 为 signed int32 raw Q20.12。实验 harness 以 Z/(2^34) 拆份额，`b=max(1,ceil(log2 n))`；n=64 时 `b=6`、uCMP 输入宽度 `34+b=40`。party0 的局部 lift 加入 signed-order offset 及 index tie-break：

```text
Q(s) = s + 2^31 ∈ [0,2^32−1]
R(s,i) = Q(s)·2^b + (n−1−i)
```

R 较大表示优先级更高：signed score 降序、同分 original_index 升序；每个真实 record 的 R 唯一。两哨兵/虚拟项由 S4 的公开 category/serial 顺序处理，不将 dummy 计入 membership。实际 DCF 比较方向是 `[R_i >= R_selected]`，通过 `1-[R_i<R_selected]` 包含相等；严格反向关系没有被误作成员判断。成功样本恰 K 项为 1，mask 逆路由回原序，TEST_ONLY oracle 再核对。

**输入 ABI 限制：** 本轮 raw score 拆分为 34-bit ring shares，是实验接口；还没有与仓库生产 Protocol I/raw-score 份额域建立无溢出适配证明和 conformance。n=10^6 所需宽度仍超当前 comparator 上限；S8 只把离线实验扩大到 n=64。

API 状态在实验结果中区分成功、算法失败、材料拒绝、通信失败；成功时 party 文件包含 share，失败时写 `no_mask=true` 而不输出零 mask。算法失败与材料/通信错误不作同类计数。本轮负面测试确认一个实现问题：peer timeout 原先发生在错误捕获块外，没有结构化结果；已把 Peer 建立移入同一异常合同，并重跑后确认 P0 写 `ABORT_COMMUNICATION/no_mask`。没有 peer 的静默用例不能声称对端也确认了 abort。

## 5. 匿名自适应图的独立裁决

新审计器 `audit_adaptive_graph.py` 不读取 S8 的实际 FSS key，也不导入 online party；它以 `ExactOrderBackend` 复跑同一 S4 R1–R4 状态机，枚举真实 record 到匿名 handle 的全部均匀置换，对 n=2..5、两个相反严格总序、algorithm seeds 42/43 比较 transcript 分布。Transcript 包括：两 sentinel task 的共用真实 handles、R1–R4 冻结边计划与 outcome hash、派生 U/V/W/pivots、selected anonymous handle、partition abort class 和总 abort 状态；输出 share 另按理想均匀本地 bit-share 边际枚举。结果 4 个 n 组、8 组 world/seed 对比全部相同。

一般的**公开 transcript 单独输入无关论证**是条件性的：固定严格真实总序时，均匀隐藏 π 会使 handle 上的 rank vector 均匀遍历所有排列；固定公开算法随机币后，graph/outcomes/abort/selected handle 是该 rank vector 的确定函数，因此其公开 transcript 分布不依赖原始顺序。该论证要求实际 shuffle 确实提供均匀隐藏 π，且 view 不带入与 π/输入相关的 party-local state。有限枚举支持实现走的是自适应图，不是固定图；它不能证明 shuffle 的上述假设或完整联合安全。

Party-view 门需要对任一腐化方 `b` 模拟：

```text
V_b = (local input shares,
       local shuffle material/state/messages,
       both Selects' common-real-handle relation,
       complete party DCF key pool and read/unread access trace,
       public masked values and adaptive graph/outcomes,
       abort class, final local mask shares)
```

尚未闭合的具体步骤：

1. 当前 shuffle 实现没有在本轮完成与 S8 整体 transcript 相符的一方完整 view 定理/模拟器。
2. 单 DCF key 的 one-party 隐藏性即便成立，也未推出共享 mask vector 上所有 endpoint/task/round/member keys 的 joint distribution 隐藏；需对完整相关 pool 与自适应访问做 hybrid，不能只逐 key 套用。
3. 需要把公开 masked `z_i`、每轮被选择的 key 地址、比较 shares/outcomes 与真实 shuffle 局部状态共同纳入模拟；再证明 inverse route 得到的本地 mask share 符合理想输出份额边际。
4. 错误/超时下的双方一致 abort 与认证端点/session 绑定未完成；这虽然不影响无错误成功样本，但不满足生产 API 合同。

因此 `PARTY_VIEW_SECURITY=NO_GO`。允许泄露候选暂列为 `n,K,session、匿名 public masked values、每层匿名 graph/比较位、selected anonymous handle、abort class、最终本地 mask share`；是否与现有项目隐私目标兼容，还需安全接收审查。

## 6. 验证结果

### 6.1 冻结小矩阵与额外 n=64 样本

小矩阵 score fixture 由 `run_s8.py --matrix` 固定；share seed 从 11 起、algorithm seed 从 42 起，按行号各加 1。shuffle permutation 使用 C++ `getrandom`，所以种子能固定 score/share/algorithm inputs，但不固定 shuffle 秘密随机性。

| 行 | n,K | 样本说明 | 结果 |
|---|---:|---|---|
| n1_min_k1 | 1,1 | `INT32_MIN`，singleton shortcut | SUCCESS / oracle PASS |
| n2_extremes_k1 | 2,1 | `INT32_MIN/MAX`；运行离线包负面 conformance | SUCCESS / oracle PASS |
| n2_equal_k2 | 2,2 | 相等分数、K=n | SUCCESS / oracle PASS |
| n3_all_equal_k2 | 3,2 | 全相同 | SUCCESS / oracle PASS |
| n5_repeat_mixed_k1 | 5,1 | 重复、正负、int32 极值、K=1 | SUCCESS / oracle PASS |
| n5_repeat_mixed_k3 | 5,3 | 同输入中间 K | SUCCESS / oracle PASS |
| n5_repeat_mixed_kn | 5,5 | 同输入 K=n | SUCCESS / oracle PASS |
| n8_repeat_k1 | 8,1 | 重复值、K=1 | SUCCESS / oracle PASS |
| n8_mixed_extremes_k3 | 8,3 | 混合极值和重复、两实例交集 | SUCCESS / oracle PASS |
| n8_all_equal_k4 | 8,4 | 全同分、中间 K | SUCCESS / oracle PASS |
| n8_boundary_k8 | 8,8 | 边界值、K=n | SUCCESS / oracle PASS |
| n64_k2_seed2026 | 64,2 | fixture `n64_seed20261006.json`，有效域 126；R1 样本每 task 90 项，小于 126 全域 | SUCCESS / oracle PASS |

结果为 12/12 成功，oracle 12/12 pass；没有观测到算法 ABORT、错误答案或半输出。这个有限样本不推出失败率。n64 fixture score 由 Python `random.Random(20261006)` 生成，份额输入 seed=67，算法 seed=101；OS 随机 shuffle 仍会改变具体 handle、R2/R3 图与计数。

### 6.2 小规模实际材料 conformance

n=2 专项检查均为 true：不同 party package 被拒绝；40-byte 截断包被拒绝；不存在 round 和 endpoint 槽被拒绝；实际首个槽返回 share、重复消费同槽被拒绝、第二次领取同包被拒绝；peer silent 时存活端写结构化 `ABORT_COMMUNICATION` 且不产生 mask；另用两个已连接 party 进程令 P1 领取 P0 package，双方均记录 `ABORT_MATERIAL` 且 `no_mask=true`。这覆盖了一个材料错误的一致分类路径，但没有证明所有通信/材料错误的一致 abort。包 AEAD/MAC、网络认证、跨会话重放、成功连接后篡改也未验证。

最终 12 个功能样本没有触发 S4 `PROJECT_RANDOM_FAILURE_PATH`，本轮也没有用注入方式伪造算法随机失败；因此算法失败后的双方共同 ABORT/no-mask 控制流尚无实际 E2E 见证。peer-silent 只验证仍存活一方能记录通信 ABORT，不验证掉线方确认。连接中的错误 party package 测试验证了双方 `ABORT_MATERIAL` 一致。包 session/fingerprint 是自描述字段，端点没有由外部可信协商值校验 session，也没有在线认证 handshake；错 session/跨会话包测试未运行。

### 6.3 逐轮计数与独立边复算

`audit_party_transcripts.py` 从 serialized P0/P1 trace JSON 独立重算，不 import Select/uCMP/backend。它检查每条边在所属轮冻结、计划在结果到达前后的位置、每轮比较/真实边/dummy 边、规范 key 槽、反向 share-complement、跨轮重复、membership 星形边、两方 DCF Eval 公式、P0/P1 trace 相等与 13/1 个 loopback causal phases。矩阵 11 行 + n64 共 12/12 审计通过。

代表性 n=8,K=3 计数：

| Round | 逻辑比较 | real-real | dummy-related | 实际规范 uCMP slots/party |
|---|---:|---:|---:|---:|
| R1 | 132 | 56 | 76 | 56 |
| R2 | 44 | 28 | 16 | 26 |
| R3 | 2 | 2 | 0 | 2 |
| R4 | 2 | 2 | 0 | 2 |
| 合计 | 180 | 88 | 92 | 86 |

上表没有将 dummy 或重复 logical call 折算成 DCF Eval；可逐边核查数据保存于矩阵 JSON 与独立审计 JSON。n=8 的 membership 7 slots/party；全池 308 slots/party；实际 93 slots/party，使两方总 DCF Eval 为 372。

n=64,K=2 单次轨迹：

| Round | 逻辑比较 | real-real | dummy-related | 实际 uCMP slots/party |
|---|---:|---:|---:|---:|
| R1 | 8,010 | 2,304 | 5,706 | 2,304 |
| R2 | 500 | 126 | 374 | 126 |
| R3 | 10,843 | 2,973 | 7,870 | 2,973 |
| R4 | 18 | 4 | 14 | 4 |
| 合计 | 19,371 | 5,407 | 13,964 | 5,407 |

另有 membership 63 slots/party，实际每 party 共 5,470 个 uCMP 槽、10,940 个 party-local DCF Eval；双方共 21,880 Eval。全池 22,176 slots/party，未用 Select key slots 10,721、membership slots 1,953、shuffle CmpAgg slots 4,032。每方包 23,139,976 bytes。双方 loopback 每方向约 0.753 MB，在线单方计时约 1.40 s（仅该次 synthetic TEST_ONLY run；不能作正式性能数）。

S4 样本括区是 `PROJECT_DERIVED`，S8 不证明其一般渐近线性比较量，也不声称这些实数与 BMW16/BB90 常数匹配。R3 在 n64 固定样本中有 10,443 个逻辑边，展示必须实报逐轮量；有限样本不能替代最坏路径证明。

### 6.4 带构建环境的复跑命令

环境：Windows host Python 3.13.7、NumPy 2.3.5；WSL Ubuntu 24.04.4、Python 3.12.3、CMake 3.28.3、GCC 13.3.0。Release helper 在 `/tmp/m6b_i_bmw16_s8_build/bmw16_s8_backend`；构建产物没有进入仓库。

```powershell
wsl.exe -d Ubuntu-24.04 -- cmake -S '/mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s8/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS' -B /tmp/m6b_i_bmw16_s8_build -DCMAKE_BUILD_TYPE=Release
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/m6b_i_bmw16_s8_build --target bmw16_s8_backend -j2
python -m py_compile experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/party.py experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_adaptive_graph.py experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_party_transcripts.py
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s8_build/bmw16_s8_backend' --matrix --output docs/reproduction/evidence/BMW16_S8_THREE_PROCESS_MATRIX.json
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s8_build/bmw16_s8_backend' --scores-json experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/n64_seed20261006.json --k 2 --case-id n64_k2_seed2026 --input-seed 67 --algorithm-seed 101 --output docs/reproduction/evidence/BMW16_S8_N64_K2_SEED2026.json
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_party_transcripts.py docs/reproduction/evidence/BMW16_S8_THREE_PROCESS_MATRIX.json docs/reproduction/evidence/BMW16_S8_N64_K2_SEED2026.json --output docs/reproduction/evidence/BMW16_S8_PARTY_TRACE_EDGE_AUDIT.json
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_adaptive_graph.py --max-n 5 --output docs/reproduction/evidence/BMW16_S8_ADAPTIVE_GRAPH_EXACT_ENUMERATION.json
git diff --check
```

本轮 bring-up 失败记录单列于 `BMW16_S8_DEVELOPMENT_FAILURES.json`：一次 P0/P1 graph JSON tuple/list 表示不同导致拒绝继续、一次 peer timeout 位于异常捕获块外、一次 n64 fixture 尾随字面 `\\n` 导致 JSON 解析失败、一次边审计器最初把 n=1 trace 当作无比较层。这些均已各自修复，复跑矩阵/审计通过；它们不是被筛掉的算法失败种子。最终算法样本没有失败种子。

## 7. 指标和限制

| 指标 | S8 本轮字段 |
|---|---|
| 算法逻辑 Select 比较 | `select_logical_comparison_calls`，每方独立相同逻辑 trace；不等同 DCF Eval |
| 实际真实边/规范 slot | `real_real_calls`、`fss_material_slots`、`select_party_uCMP_key_slots_consumed`；同轮反向 share complement 仍留在逻辑计数 |
| DCF Eval | 每方 2 Eval/槽、两方合计 4 Eval/槽；membership 自比较本地无需槽 |
| 离线 key 槽/字节 | `11*C(n,2)`/party 与实测文件长度；含未用 shuffle CmpAgg |
| 离线/在线时间 | 离线 T 时间 `NOT_MEASURED`；在线 loopback elapsed 在 JSON 仅作诊断，不作性能矩阵 |
| 双方收发 | JSON 中 peer payload bytes；loopback 序列化字节，不是 LAN/WAN 链路计量 |
| 因果层/总阶段 | Select comparison layers=4；n≥2 完整入口 `13` request-response phases；n=1 shortcut `1` |
| PRG、真实网络、总时间、安全 abort 率 | `NOT_MEASURED` |

可复现性边界：Python 输入/算法随机种子固定；shuffle 隐藏置换由 OS entropy 生成，所以 handle transcript 和依赖 shuffle 的 U/V/W/计数是一次实际结果，不承诺按命令逐比特相同。没有把临时 package、key material、明文输入日志或构建产物提交。

## 8. 门禁

| 门 | 判定 | 依据 |
|---|---|---|
| `FUNCTIONAL_E2E` | `PASS_EXPERIMENTAL_TEST_ONLY` | 12/12 分方执行与 C++ oracle 一致；不等于安全协议 |
| `PARTY_VIEW_SECURITY` | `NO_GO` | 公开匿名 transcript 输入无关获得条件性论证与 n≤5 精确枚举；完整 shuffle + correlated adaptive DCF pool + access + output-share 联合模拟缺失 |
| `OFFLINE_ONLY_MATERIAL` | `PASS_EXPERIMENTAL_CONTRACT_ONLY` | T 无输入，先发全池并退出，P0/P1 无在线 keygen；包保护/持久化/联合隐私未过安全审查 |
| `ORIGINAL_ORDER_MASK` | `PASS_TEST_ONLY` | 每个成功 fixture 的重构 mask 与稳定 C++ oracle 同序、恰 K 一致；重构只在外部 harness |
| `PERFORMANCE_READY` | `NO` | 非安全 loopback 样本，不满足 LAN/WAN、production domain、跨会话和正式指标门 |

下个安全阶段的最小阻塞不是“再写一般性 NO-GO 文档”，而是给出可审查的 `BMW16-S8-JOINT-POOL-SHUFFLE` simulator/hybrid：固定任一腐化方 local shares，联合覆盖本地 shuffle state/message、共同真实 handle、同 mask 向量的完整 key pool、adaptive 访问/未访问槽、公开 transcript、abort 和最终 mask shares。若无此引理，唯一可推进替代项是实现/审查隐藏图或安全私有 key-slot 查询并重新列出通信和材料成本；不得把它们作为零成本假设。

## 9. 原始证据索引

- `docs/reproduction/evidence/BMW16_S8_THREE_PROCESS_MATRIX.json`：11 个 n≤8 case 与 party traces、逐轮边及在线/材料计数。
- `docs/reproduction/evidence/BMW16_S8_N64_K2_SEED2026.json`：n64 实际分方 run、selected anonymous handle、mask/oracle、package 与逐轮计数。
- `docs/reproduction/evidence/BMW16_S8_PARTY_TRACE_EDGE_AUDIT.json`：独立不导入算法实现的 12/12 边/轮因果复算。
- `docs/reproduction/evidence/BMW16_S8_ADAPTIVE_GRAPH_EXACT_ENUMERATION.json`：n=2..5 全置换的公开自适应 transcript 分布审计。
- `docs/reproduction/evidence/BMW16_S8_DEVELOPMENT_FAILURES.json`：bring-up 阶段的实现/审计工具失败和修复轨迹。
- `docs/reproduction/evidence/BMW16_S8_SHA256SUMS.txt`：源码、当前构建二进制、fixture 和结构化证据哈希。

本地提交 revision、最终差异检查与禁止路径审计在交付回复中给出；不推送、不合并、不更新 PR。
