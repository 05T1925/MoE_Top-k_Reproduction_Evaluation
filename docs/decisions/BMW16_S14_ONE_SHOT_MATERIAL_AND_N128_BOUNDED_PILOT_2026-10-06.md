# BMW16 S14：一次性材料领取优化、shuffle-only 发材与 n=128 有界试运行

- 日期：2026-10-06
- 任务：`M6B-I-BMW16-S14`
- 起始提交：S13 `0eab2e358bf34904fbca542629f8662db9dd62f3`
- 分支：`codex/m6b-i-bmw16-s14`（本地隔离 worktree）
- 身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 结论：**有限功能和 n=128 试运行通过；材料路径做了可证明的一次性优化；联合单方视图与生产交付仍 NO-GO；不是 V4 正式验收。**

## 1. 范围、工作区和来源

本轮从 S13 精确提交建立独立 worktree，起始树干净。基线 `main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`。桌面主工作区处于 `feat/m6a-performance-evaluation`，HEAD 与该 main 相同，但有 M6A 文档及 `siamjdiscrmath.pdf` 等本地差异；均留在原工作区，未复制或覆盖。E20=`8986a40175f5f8eef56d765fef0cfa863e205985`、E21=`93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`，PR #28 历史 draft head=`57e239ee820a4208ac606663d877803f0b692206`；没有修改这些分支/PR、`VFSS-baseline/`、论文或本地参考工程。

S4 Select 算法来源仍是 `04ed6f8a352ab2277434344094471dff82f83e4b` 的 `select4r.py`，SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。S14 没有另写 Select。证据分层如下：

| 类别 | S14 中的内容 |
| --- | --- |
| 论文定义 | BMW16 2016 原文和原始定理；本任务不扩张、不移用 Theorem 8 的概率或比较常数。 |
| S4 固定项目推导 | 四层 sample-bracket Select 的固定 Python 源与共同随机 tape 轨迹。 |
| 项目扩展 | VFSS 中的 Protocol I raw-score adapter、shuffle、两份并行 Select、uCMP/DCF membership、逆路由及原序 XOR mask shares。 |
| 待证明/部署 | 相关多 key DCF 全池及 adaptive Eval 的联合模拟；生产 party 凭据和 T 私有认证交付；正式 LAN/WAN 计量。 |

S13 的 56 行 E2E、39 条 tape 差分、8.953595 秒单次在线样本和 4,032 个未用 shuffle CmpAgg key 均保留原身份。S14 独立读代码和重跑，不改写 S13 历史记录。本轮仍只开放用户授权的有界研发与实验测量；V3/V4 的既定阶段依赖和正式性能验收未完成。

## 2. S13 复核和本轮代码变更

复核的实际调用链是 `protocol_i_bmw16_experimental_raw_score_mask_party` → 正式 Protocol I raw-score input adapter → Protocol I forward shuffle → 两个共享真实句柄的 Select task（R1–R4）→ selected stable key membership → 同一置换 inverse shuffle → 两方原序 XOR bit-mask shares。只有 TEST_ONLY 父 harness 重构两份 mask 并调用冻结 oracle。`n=1,K=1` 是明确 identity shortcut；不是一般 Select 成功样本。原 S13 的退出合同保留：SUCCESS=0、算法 abort=10、材料错误=20、通信错误=30、意外错误=70；局部失联仅报告存活方本地 abort。

逐槽持久 claim 的旧路径是每次槽回调做 `O_EXCL` marker、写入、`fdatasync` 与目录 `fsync`。S13 的 n=64/K=8 单次样本将每方 8.84 秒左右花在 claim callback 上（P0 8,841,165 μs / API 8,953,595 μs）。S14 改为：

1. 对每方 bundle 做一次持久、原子的 bundle-wide burn claim；`O_EXCL` 创建、文件 `fdatasync` 和父目录 `fsync` 仍在这里执行。
2. bundle 被 claim 后，该进程内用 `ProtocolIBmw16ProcessSlotClaimSet` 的 mutex + set 做每槽唯一性检查；guard 记录创建 PID，拒绝跨 fork 继承后继续使用。
3. 任一进程崩溃/退出后整包作废；重启不能重新 claim 原 bundle。检查或协议中途失败会保留已消耗审计槽，不回滚后重用材料。

正确性理由是“整体销毁的 nonce/一次性材料池”语义：持久 marker 成功意味着该 bundle 永久不可再次领取；单进程中 set 保证每个 slot 在该次使用最多一次。因此不再需要让每一次 Eval 都同步落盘。该结论依赖每个 party 使用同一个受保护、持久的 claim store 作用域，且所有副本共享/绑定同一 bundle identity；当前实现和负例覆盖的是本地 TEST_ONLY store，不是已部署的分布式数据库/高可用 claim 服务。生产化时不能将 bundle 复制到另一个未共享 burn ledger 的存储位置。

前向/逆向 shuffle 只需要 shuffle 的 `sigma/tau/a/e/r_share` 及 raw 输入适配材料；BMW16 实验没有调用该 shuffle 实现的第三轮 CmpAgg。新增 `include_cmpagg_material=false` 的 shuffle-only profile；通用 Protocol I 默认保持 `true`，CmpAgg round3 对 shuffle-only 包显式失败。BMW16 bundle schema/version 升至 S14 v2 并拒绝携带旧配置。两份未使用池从材料公式移除：

```text
S13: 2*padded_n + 2 + 11*C(n,2)
S14: 2*padded_n + 2 +  9*C(n,2)
省去: 2*C(n,2) 个 shuffle CmpAgg key/party
```

这里 `9*C(n,2)` 是两 Select task 的 8 个有序任务池加 1 个 membership 池；不是删除 Select 或 membership 的 key。generic shuffle regression 测试确认默认含 CmpAgg 的既有入口没有被改成 shuffle-only。

## 3. 一次性优化负例和指标边界

最终三进程测试保留了错 recipient key、错 party/session/宽度、截断 tag、密文/manifest 篡改、重复 bundle claim、重复进程内 slot claim、领取后模拟崩溃再用、peer closed、peer silent 等拒绝记录。n=64 的 slot count 从旧方案 22,306 降到 18,274；S13 观察的 4,032 未用 CmpAgg 槽在 S14 预发数与实际计数中均为 0。逐槽唯一审计 hash 由各次 party 运行产生，TEST_ONLY harness 负责将其和 C++ 内部报告计数核对。

当前本地 AES-GCM 测试密钥由 TEST_ONLY root harness 预置并通过不同 UID 目录交付。AAD、recipient/session/config/manifest digest 和包内容认证可检测更改；它们不能证明 dealer 生成的函数 key 与 manifest 端点数学关系正确。数学绑定依赖可信 T 正确执行受审查的 keygen 和 manifest factory。生产 party 身份注册、解封凭据分发、远程 T→party 私密认证通道、轮换/吊销/恢复仍未实现。

## 4. 参数、容量与 n=128 可行域

有界代码接受 `2≤n≤128`（n=1 单独 shortcut），padding `p=next_power_of_two(max(n,2))`，`index_bits=log2(p)`，比较位宽 `b=33+index_bits≤53`。stable real key 的 `0≤key<2^(32+index_bits)`，当前 uCMP 合法非负输入区间为 `<2^(b−1)`；上述 b 令 `2^(b−1)=2^(32+index_bits)`，因此该编码范围可容纳。n=1 不运行这条比较。参数均由配置交叉校验，不静默 clamp。

脚本 `experiments/TEST_ONLY_BMW16_S14/capacity_preflight.py` 用 uint64 checked add/mul/choose2 和当前 key 编码计算下界：每 slot key 序列化 `57+24*b` 字节；raw pool `2*p` 个 34-bit key；Select+membership pair pool `9*C(n,2)`；manifest `8+20*slots` 和 116 字节的 envelope 固定开销。shuffle 状态、容器额外分配等不在解析下界中。

| n | p / uCMP 位宽 | slots/方 | 解析包下界/方 | 两方下界 | 入口状态 |
|---:|---:|---:|---:|---:|---|
| 64 | 64 / 39 | 18,274 | 18,494,340 B | 36,988,680 B | 已实测 |
| 128 | 128 / 40 | 73,410 | 76,087,396 B | 152,174,792 B | 已完成有界实测 |
| 256 | 256 / 41 | 294,274 | 312,136,740 B | 624,273,480 B | PRECHECK_ONLY，API 不支持 |
| 1,000 | 1,024 / 43 | 4,497,550 | 4,987,338,528 B | 9,974,677,056 B | PRECHECK_ONLY，API 不支持 |
| 10,000 | 16,384 / 47 | 449,987,770 | 542,225,036,988 B | 1,084,450,073,976 B | PRECHECK_ONLY |
| 100,000 | 131,072 / 50 | 44,999,812,146 | 57,464,659,444,756 B | 114,929,318,889,512 B | PRECHECK_ONLY |
| 1,000,000 | 1,048,576 / 53 | 4,499,997,597,154 | 6,070,495,802,256,900 B | 12,140,991,604,513,800 B | PRECHECK_ONLY |

除 64/128 外，均只由 checked 容量算术给出，不是预发、落盘或性能实测。此 O(n²) 全池在百万规模达到 PB 量级；位宽适配不代表入口可运行。K 不影响全池槽数，但表列的 target K 覆盖两端和中间 rank。

预检环境：WSL Ubuntu 24.04；`/tmp` 可用空间 990,310,723,584 B、可用内存约 7,560,425,472 B，无额外 ulimit、无 swap 使用。n=128 fresh bundle 实测 76,701,577 B/party（下界 76,087,396 B）；party 峰值 RSS 约 320,000 KiB、T 峰值约 523,448 KiB。E2E suite `/usr/bin/time` 峰值 523,452 KiB、耗时 6.71 s。这些是在本机 loopback/单机多 UID harness 下的有界结果，不能外推到 LAN/WAN。

## 5. n=128 算法差分与 E2E

共同 tape 是外部 TEST_ONLY 文件，保证 Python S4 与 C++ 消费同一无放回抽样选择，而非把 Python `random.Random` 与 C++ SplitMix 相同 seed 误当成同一轨迹。S4 frozen SHA-256=`8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。n=128 的 K=2/8/80 三案均逐层比较抽样、pivot、U/V/W、R1–R4 edge plan/结果、状态和 selected handle，全部 PASS；未发现差异。其含极值的 K=2 向量覆盖 INT32_MIN/MAX；K=8 有重复分数模式，K=80 为固定 seed 随机向量。矩阵 SHA-256=`4948cee08b88a7208145cebc1eea1a1639046710980a714ac05afcb840e0558e`。

三进程 fresh-material E2E 在 S13 56 行基础上新增 n=128/K=2、8、80，输入种子分别 `128002`、`128008`、`128080`；均 SUCCESS，mask 原序、长度、bit 域、weight K 和冻结 C++ oracle 逐位相同。全矩阵 59 行：55 SUCCESS、1 TEST_ONLY 强制算法 abort、1 篡改材料、1 意外输入异常、1 peer close；自然抽样 abort 0，错误 mask 0。自然失败样本只有 55 个真实成功算法输入的有限观察；注入 abort 不计作随机失败率数据，不能得出概率上界。

S4 固定源 `select4r.py`（SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`）没有使用 `epsilon`；下面的抽样规模与 cap 是项目有限参数规则。S12/S13 已记录这套 sample-bracket 参数不适用 BMW16 Theorem 8。因此本轮**没有可报告的高概率成功界或 `≤10^-6` abort 界**；这是当前候选的算法概率门缺口，不由 0 次失败补上。

| 步骤 | S4 项目代码的精确规则（`m=M` 为 median partition 长度，`h=m/2`） | 证据身份/失败处理 |
|---|---|---|
| 任意 K 到两个 median 实例 | `r=n-K+1`，`N=n+1`，`low=max(0,N−2r)`，`high=max(0,2r−N)`，`M=N+low+high=2·max(K,n−K+1)`，所以 M 总为偶数且至少 2。每个实例输入 M 个项，真实项和 dummy/sentinel 类别由固定包装规则给出；n=1 走单元素 shortcut。 | `PROJECT_DERIVED`，由代码代数直接得出；未改变两哨兵来源。 |
| R1 抽样和 rank bracket | `s=min(M,ceil√(64M))`，不放回均匀抽 `s` 项。若 `s=M`，`qL=M/2`、`qH=M/2+1`（一基）；否则 `a=ceil√(16s)`、`qL=max(1,floor((s+1)/2)−a)`、`qH=min(s,floor((s+2)/2)+a)`。只比较 S 内全部无序 pair。 | 参数/取整 `PROJECT_DERIVED`；样本秩非唯一或 q 不可用时 `ABORT_R1_SAMPLE_RANK_INVALID`，不补排序。 |
| R2 两 pivot 与 U | 将每个当前输入项分别与 R1 两 pivot 比较；得到真实前缀/后缀及 bracket 集 U。仅 `c≤h≤cy` 才继续。`U_cap=min(M,ceil(8M^(3/4)))`；若 `|U|>U_cap` 则 abort。R3 residual rank `q=h−c`。 | 集合/控制流为 S4-derived；bracket 缺失 `ABORT_R2_MEDIAN_NOT_BRACKETED`，超 cap `ABORT_R2_U_OVERSIZE`。 |
| R3 V 与 W | `v=min(u,ceil√(64u²/M))`，不放回抽 V；比较 V 内所有无序 pair 及 V×(U\V)。虚拟边界允许无左/右 pivot；`gap=ceil√(4M)`，`W_cap=2·gap+1`。若 q=0 由 R2 exact residual endpoint 直接完成该任务；否则必须满足 `0≤i≤q<j≤u+1`、`1≤qW≤|W|` 且 `|W|≤W_cap`。 | 项目 finite-window 规则；边界/rank 不闭合时 `ABORT_R3_RESIDUAL_RANK_INVALID`，W 超 cap `ABORT_R3_W_OVERSIZE`。`u<2`/q 越界属于内部不变量错，标 `COMPLETED_WRONG`，不伪作概率 abort。 |
| R4 最终 W | 比较 W 内全部无序 pair，按 qW 输出 Partition 的 Reject/Accept。两个 median Partition 成功并由两哨兵归约交集得到唯一真实目标后才报告 Select 成功。 | 四轮控制是 S4 项目实现；任何最终集合/交集不唯一都不得输出错误 SUCCESS。 |
| 失败概率 | 当前 C++ 非 tape 路径各方经 `getrandom(32)` 贡献、在线 XOR 后由 SHA-256 counter stream + rejection-bounded partial Fisher–Yates 采样；task 与 R1/R3 域分开。共同随机 tape 仅 TEST_ONLY。 | OS CSPRNG 贡献和 PRF 假设描述的是实现抽样分布，不是算法成功概率定理。S14 E2E 自然 abort `0/55` 只作为有限样本观察。 |

整数 ceil 用 checked/整型算法实现；R3 样本请求由 `min(u,...)` 限定，不会超过 U。当前 finite domain 是 `n≤128`，`n>128` party config 拒绝；不重试、不做明文排序或 oracle fallback。上述 cap 的线性比较成本是 S4-derived 参数设计意图，S14 不把它夸写成已验证的全参数成功概率证明。

## 6. same-host loopback pilot（非正式性能）

每个 profile 1 次 warmup + 5 次 measured，输入与 seed 固定、每次 T 新发材料、在线 sampler 用 OS CSPRNG；S14 的 S4 抽样边会因此变化。运行器通过 TEST_ONLY 父进程 oracle 检查每个成功 mask。下表 median `[min,max]` 仅描述这个本机版本：

| 指标 | n=64,K=8 | n=128,K=80 |
|---|---:|---:|
| 外层 wall time | 0.42035 `[0.39885,0.64759]` s | 1.87339 `[1.60825,3.18062]` s |
| party online | 47,149 `[44,570,48,711]` μs | 319,935 `[311,627,332,306]` μs |
| T 离线总计 | 224,847 `[208,744,399,125]` μs | 1,148,157 `[883,984,2,071,848]` μs |
| bundle open/party | 42,175 `[39,996,45,036]` μs | 205,943 `[171,933,223,091]` μs |
| durable bundle claim | 2,940 `[1,904,10,342]` μs | 39,554 `[1,990,63,954]` μs |
| process-local slot claim / 普通 lookup | 243 `[228,285]` / 15,637 `[13,606,16,439]` μs | 1,114 `[847,1,448]` / 211,800 `[205,071,221,847]` μs |
| 每方实际 bundle / 预发 slots | 18,654,121 B / 18,274 | 76,701,577 B / 73,410 |
| Select logical / unique slot | 17,568 / 5,622 | 25,855 / 17,945 |
| membership / party uCMP / DCF Eval | 63 / 5,813 / 11,626 | 127 / 18,328 / 36,656 |
| P0 sent / received | 149,345 / 149,345 B | 223,833 / 223,833 B |
| party / T peak RSS | 83,968 / 127,812 KiB | 320,000 / 523,448 KiB |

各 round 的逻辑、unique、dummy、repeat、边 hash，以及 raw adapter、shuffle、R1–R4、membership、inverse、状态各阶段计时保存在每个 raw run 里。按五次分别取 median 后，n64 的 R1/R2/R3/R4 逻辑中位数为 7,310/452/9,804/4，n128 为 10,302/636/14,905/7。由于每轮各自取 median，不能要求它们之和等于“总逻辑比较”中位数。消息因果 phase 数为 12：raw adapter 2、forward shuffle 2、coin 1、Select 四层 4、inverse shuffle 2、最终状态 1。PRG 字段只计 sampler PRF words（中位数 326 / 371）；FSS 库 AES PRG 调用数仍 `NOT_MEASURED`。

与 S13 的一次旧样本采用相同 n=64/K=8/input seed 和同一资源上限，但不具严格配对性（S13 是单次、随机边由另一份 OS coin 生成，且实现 revision 不同）。旧 durable slot callback 8.84 s 与新整包 claim 2.94 ms/进程内 unique check 243 μs 支持“移除逐槽 fsync 是主要开销点”；不以此给正式加速比。pilot 的 T delivery 是 root TEST_ONLY harness 在本机生成/封包/写入隔离 UID 目录，非生产网络交付。WAN/LAN RTT、可部署凭据、真实跨机吞吐、正式九指标完整性均未测，所有相关 V4 字段仍 `NOT_MEASURED`。

## 7. 联合单方 view：hybrid 尝试和第一个缺口

泄露草案与 S13 相同：公开 n/K/session/width/padding、shuffle 后 masked `public_z`、两 task 共用匿名真实句柄空间、抽样/pivot/U/V/W、R1–R4 边和打开的比较位、selected anonymous handle、abort 类别、流量/槽访问及各方输出 mask share。单方 view 还含自己的 raw input share、shuffle 局部 `sigma/tau/a/e/r_share` 与消息、整份解封 party bundle（已用和未用 keys）、claim 轨迹和 FSS Eval share。该 leakage 尚未获隐私目标独立签收。

| hybrid | 替换步骤和假设 | S14 结论 |
|---|---|---|
| H0 | 实际 T 生成、两 party 的 share、shuffle state、全部 party bundle、adaptive graph/Eval/transcript、abort 与输出 share。 | 起始真实执行。 |
| H0→H1 | 按每个 `FSSConfig::prngs[tid]` 的 OS 独立 seed，以 AES-CTR PRG 安全性将顺序随机流替换成同长度均匀 tape；优势应按实际 stream/块数求和。S14 没有为了这一论证另加 slot domain separation。 | **条件性可写**；需要明确 cryptoTools AES PRG 的安全假设、OS seed 熵和使用上界。不是当前缺口的关闭。 |
| H1→H2 | 联合替换整池 DCF party keys；阈值是共享节点 mask 的差 `alpha_ij`，不同 pair/task/round pool 有代数相关；party 视图含未使用 key；online public operands 和 Eval 查询由同一 masked list 生成，后续边由先前 opened bits 自适应选择。 | **第一个未闭合 hybrid。** 仓库没有精确覆盖“相关 threshold 向量 + 本地 shuffle 辅助输入 + 整池（含未用 key）+ 公开相关 query + adaptive Eval transcript”的 multi-key/auxiliary-input DCF party-view simulator/优势定理。单 key 隐私描述不足以自动证明该步；也没有两世界区分 witness 证明其不成立。 |
| H2→H3 | 以 Protocol I shuffle 的理想共同置换/消息 simulator 替换局部随机置换、状态和 masked list；保持两个 Select task 的共同真实 handle 绑定。 | **未完成。** 代数关系 `tau0=pi∘sigma1^-1`、`tau1=pi∘sigma0^-1` 只能说明局部置换边际，不能模拟 `a/e/r_share`、CmpAgg 辅助状态、公开 `public_z` 与输出。 |
| H3→H4 | 在上述 view 模拟条件下按 Select transcript leakage 继续模拟两实例、membership、逆路由、算法 abort 与 output share。 | **条件性组合接口；未证。** 依赖 H1→H2、H2→H3 及 abort/output-share 模拟。 |

这是具体证明义务，不是“不可能”结论。最小下一步是针对当前 `protocol_i_ucmp.cpp` / `ext/FSS/dcf.cpp` 给出可引用的多 key correlated-threshold、auxiliary-input、adaptive-query 安全定义和可复核 reduction；如只能证明随机阈值场景，需说明当前 shuffle 对阈值/查询给出的条件分布。随后单独证明 shuffle 的完整本地 view 及逆路由 output share。增加 domain separation 可清晰化多实例 PRG 混合，但本轮没有证据表明“只加 domain separation”能补上 DCF simulator，故未擅自加入。

## 8. 分项门禁和后续

| 门 | 结论 |
|---|---|
| S13 源/API/旧失败复核 | `PASS_WITH_BOUNDARY`；历史报告未改写 |
| 一次性 claim 优化 | `FUNCTIONAL_PASS_FOR_LOCAL_BUNDLE_BURN_MODEL`；依赖持久 claim store 与 process-local guard；生产存储另审 |
| shuffle-only 发材 | `PASS_FOR_BMW16_PATH`；通用 Protocol I 默认仍含 CmpAgg |
| n=128 conformance/common-tape/E2E | `PASS_FOR_K=2/8/80_TESTED_CASES` |
| O( n² ) capacity preflight | `CHECKED`；仅 64/128 真正运行，其余仅算术下界 |
| conditional party-view security | `NO_GO / FIRST_GAP_H0_TO_H1_POOL_VIEW` |
| production T credential/private delivery | `NO_GO / NOT_IMPLEMENTED` |
| V4 LAN/WAN / 六方案性能完成 | `NOT_READY / NOT_STARTED` |

下次最有价值的唯一安全构造不是重复匿名图枚举，而是写出并审核精确 DCF key-generation 的 party-view 多 key定理/模拟接口；如果现有 key distribution不能支持该接口，替换原语后再连接 shuffle hybrid。S14 研发许可不改变 V3/V4 正式验收顺序。
