# BMW16 S27：实际 DCF 单 key、全池与 shuffle/output 组合复核

日期：2026-10-09
实现身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
审查对象：S26 final HEAD `2818bce20f30719eaccf0bc5df586cce4fe78c84`
S26 runtime repair：`ded433d636a3df929380bdce0ff22f4ce6121453`
报告提交：见本报告所在 revision；没有将报告提交当成 runtime revision。

## 1. 工作树和接收边界

S27 在独立 worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s27\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-bmw16-s27`。起始 HEAD 是用户指定的 S26 final `2818bce20f30719eaccf0bc5df586cce4fe78c84`；其父为 `974c734ca839a2501a2b52c1ff95a4d7de70f895`，它同时是 `origin/codex/m6b-i-bmw16-s26` 指向的提交。起始 `origin/main=main=c3926c68fd14f270faa8b55234311071947fa080`，是 S26 HEAD 的祖先；S27 worktree 沿用 S26 精确源码，未把当前 main 后续内容混入被测 runtime。分支名和提交历史均未改写。

起始工作树没有已跟踪差异；有一个已存在的未跟踪 helper `experiments/TEST_ONLY_BMW16_S27/ideal_dcf_model.py`，起始 SHA-256=`17C3D4CF5A5926DC0FD4571CD51D88F8C928F5665466548CAAA65EF4B213A144`。S27 保留其内容并将它随 TEST_ONLY 证据交付；报告区分它在开工前已存在。没有触碰桌面主工作区、`VFSS-baseline/`、`Papers/`、本地参考树或 PR #28。`Papers/` 不在隔离 worktree；论文没有复制或修改。

S26 不是异会话独立签收。本 S27 报告也不是另一接收聊天对 S26/S27 安全证明的独立复审。S26 报告中历史功能/E2E 数值继续留在 S26 revision，不作为本轮新运行结果。

## 2. 基线源码身份

本轮修改之前，以下运行时源码哈希与 S26 HEAD 相同；本轮没有改动这些文件。

| S26 source | SHA-256 |
|---|---|
| `VFSS/ext/FSS/dcf.cpp` | `0EEB1926380726EFC5BE84B4B49E0ACB8C9636D0D6181469EDF505C1921B5DAC` |
| `VFSS/ext/FSS/include/FSS/dcf.h` | `80904E0518BD75772D97984F2D48EEAE7CBD805FD119156AB7B9097082AB3F47` |
| `VFSS/ext/FSS/include/FSS/keypack.h` | `26597698719B62A6ADECD54194E4A36DB39DB78FEB7F3F0AE30AF13252E991BB` |
| `VFSS/src/moe_topk/protocol_i_ucmp.cpp` | `B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23` |
| `VFSS/src/moe_topk/protocol_i_score_input.cpp` | `012EC8D85DAF19365FA75FACA94D31C2149E4EF6D79A4CDBD137957A65DD7AA7` |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `BA14008D28964E49965C75E2675E8726193C50991247BDA668F3398A01144BDE` |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `A5F61E04F0BE8A6722A579F918BCE38C94069FD231B235916F1D4AB0A61301CE` |
| `VFSS/src/moe_topk/experimental_bmw16_select_adapter.cpp` | `E96104FB82A1A87137B5350EC20BE8B020BD35B1AE0A05B954DE681A4FD63A3A` |

S24 的直接 BGI15 字段映射断点仍保留：VFSS common seed correction 不是 BGI15 对两份独立 target correction 的逐字段同分布变量。S27 的单 key 论证是另一个 source-specific 推导，不把 S24 变成历史错误，也不声称 BGI15 Theorem 6 已经覆盖 VFSS key。

## 3. 核心安全结论

详细定义、递推证明、leakage 与 simulator 见[决策文档](../decisions/BMW16_S27_DCF_AND_COMPOSITION_SECURITY_DECISION_2026-10-09.md)。核心结果如下：

1. 对 `groupSize=1`, `Bin=34..53`, `Bout=64` 的当前 `M2UC v1`，可在理想 `G126` 模型中证明完整 party key 与任意阈值 `α`、payload `β` 独立：一个均匀 root、每层均匀独立 128-bit CW、均匀独立 64-bit v、均匀 g。模拟覆盖完整 serialized key，不只覆盖两方输出和。
2. 终端 `g` 的掩蔽理由是最后一层隐藏 `keep` value 以系数 `±1` 进入 `v_alpha/g`，不是终端 seed scalar 均匀。`s&~3` 清除了两个低位；若声称 convert 的 64-bit seed scalar 本身均匀，该论证不成立。
3. DCF control bits 不属于本证明的隐藏量：腐化方可由共同 `CW` 的 `d_L/d_R` 和自己的 child control 计算对应的 peer control。下一层 PRG 安全只要求高 126-bit seed 在可见 prefix 条件下均匀；keep seed 与当层扩展坐标独立，纠正偏移在固定路径条件下已知。详见决策文档的逐变量映射与 hybrid 归纳。
4. 实际计算结论以 `G:{0,1}^126→{0,1}^382` 安全 PRG 和 cryptoTools AES-CTR 根流安全为前提。若将 `G` 归约到 AES，需明确使用 key `encode(S||00)` 的 restricted-key PRP 假设；标准均匀 128-bit AES PRP 假设不自动蕴含该受限子族结论。优势保留符号项，不编造安全位数。
5. 全池 key 数为 `N_DCF=2p+9·C(n,2)`，每方离线 slot 另有两个 shuffle slot：`2p+2+9·C(n,2)`。同一 uCMP key 的 `eval_strict_lt` 做两次 DCF Eval；它们是完整 key 的确定性后处理。两个 Select task 的每个 task/round/edge 使用新 slot；unused keys 也在 simulator 的初始整池中。
6. 在理想函数 key 分布下，逐 key 对任意固定相关阈值向量成立，故共享 node mask 造成的 alpha 相关性本身不破坏乘积模拟。公开先前 bit 决定后续边不额外破坏模拟，因为 `L` 自身决定访问序列，完整 pool 已先模拟；公开 bit 与本地 Eval share 给出 peer share `bit−local mod 2^64`。
7. 实际 shuffle 给出 source-level project construction 等式：forward `w0=πx1−h`, `w1=πx0+h`, `z=π(x0+x1)+r0+r1`; inverse 对两方 shares `μ0,μ1` 产生 `o0=π⁻¹μ1−h'`, `o1=π⁻¹μ0+h'`。本地 `(σ_b,τ_b)` 条件于 π 的分布独立于 π；forward/inverse 使用独立 shuffle salts。候选 leakage L 明确包含 forward 与 inverse `public_masked`，因为 party 从 round2 消息可计算该值，即使上层丢弃 inverse 字段。
8. 该 shuffle 模拟以 ideal T、均匀 masks/permutations、已批准的公开 L 和 corrupt output share 为前提。L 由 ideal functionality 根据真实输入及 ideal randomness 自行生成，simulator 不获得 real transcript。inverse 阶段必须联合条件采样 output-share 低位、本地 `e` 和 peer round-1 消息；本轮已将构造写入决策文档，仍需要独立接收者复核这一步及各字段与真实 ABI 的对应。

BGI15 source：Definition 2（PDF 页 7）、Algorithm 5/6（PDF 页 18–19）、Theorem 6（PDF 页 20）。论文是 DCF/DPF 原语背景，不是 VFSS 字段安全结论：[作者论文 PDF](https://tzin.bgu.ac.il/~gilboan/publications/DPF-Extended.pdf)。

## 4. 中止概率口径

S16 的 checked 参数脚本从 S16 精确 Git 文件重新执行，不把文件 cherry-pick 到 S27：

```powershell
git show codex/m6b-i-bmw16-s16:experiments/TEST_ONLY_BMW16_S16/abort_bound_recheck.py | py -3 -
```

结果重现 S16 理想均匀无放回样本 union bound：遍历 `n≤256` 所有可达 `M≤512` 为 `1.3105804606611235e-11`；公式-only 到 `n≤10^6` 为 `5.065679989546104e-8`。这些是 S16 参数/公式域结果，不是 BMW16 Theorem 8 的结论，不是 S27 新概率界。S27 代码的实际采样先 XOR 双方各 32-byte `getrandom` contribution，再以公开 `(seed,task,domain,counter)` 计算 SHA-256 counter words 和 rejection sample。随机预言机假设下可条件性使用 ideal sampler 论证；标准模型的裸公开哈希序列不是 keyed PRF，本轮将 `SAMPLER_ABORT_GUARANTEE` 留作 `CONDITIONAL`，没有把理想界当成标准模型实现保证。测试中的强制 probability abort 只是状态负例，不计入自然失败率。

## 5. 本窗口构建与运行证据

运行环境：WSL Ubuntu 24.04、Linux 6.6.87.2-microsoft-standard-WSL2 x86_64、CMake 3.28.3、GCC 13.3.0、OpenSSL 3.0.13。构建目录在仓库外 `/tmp/moe_bmw16_s27_review`。配置实验 adapter ON、DCF counters OFF、TEST_ONLY failpoints OFF：

```powershell
wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s27/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s27_review -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF && cmake --build /tmp/moe_bmw16_s27_review --target moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test --parallel 2'
```

| 直接运行目标 | 结果 | 二进制 SHA-256 |
|---|---|---|
| `moe_topk_bmw16_experimental_party_test` | exit 0；日志有 n=1 shortcut、n=2/3/5/8 与 n=64/K=8 的成功路径，以及 n=5/K=2 的 `TEST_ONLY_FORCED_ABORT_AFTER_SELECT` 无 membership 消费负例。非独立进程 conformance，不检查本轮 raw-score mTLS E2E。 | `7C4F5AEBB489135D038BB3D48C45636B18FA485D789559B170A2100F0A3EA993` |
| `moe_topk_m2_ucmp_conformance_test` | exit 0 | `CDABD354F4D4EFC55A3D582753EE77CAAF1C9F9CF33225B04E71E2DD6342E7DD` |
| `moe_topk_m2_parallel_shuffle_conformance_test` | exit 0 | `FDCCBEC11845ED82568BDF1AE7BF24214FB65588B2A3E416320448775E7760B6` |

三项是定向目标直接执行；`ctest` 在该临时配置没有这些已构建 executable 的注册项，因此不称为 CTest 或全仓回归。没有重跑 common-tape differential 或 T/P0/P1 独立进程 E2E；S26 历史 39-tape/E2E/n=1000 只保留在 S26 报告。

TEST_ONLY 模型命令：

```powershell
py -3 experiments/TEST_ONLY_BMW16_S27/ideal_dcf_model.py --seed 270109 --max-bits 5
py -3 experiments/TEST_ONLY_BMW16_S27/check_toy_source_dcf.py --seed 270109 --max-bits 5
```

两个模型各检查 4,092 个小域 `(b,α,x,β)` 组合并重构 `β[x<α]`；第二个 source-trace run digest=`2e7499b8605156b2f57ba50791750f6991da5aba72d2a7a73a011b466125ce68`。该结果是代数/源转录检查，不是 AES 或密码学安全实验。

复核文件身份：`ideal_dcf_model.py` SHA-256=`17C3D4CF5A5926DC0FD4571CD51D88F8C928F5665466548CAAA65EF4B213A144`（开工前已有的未跟踪文件，内容未改）；`toy_source_dcf.py`=`07FF7C9EAD351034BB742975243D2928A476A9898E0DFA420E7284CB64F28B30`；`check_toy_source_dcf.py`=`5DFD2F4AFE3DB23245005CEC6B8871642EE5BE9C64C3E944BC186B9EB56DF409`。S16 概率脚本从 `codex/m6b-i-bmw16-s16:experiments/TEST_ONLY_BMW16_S16/abort_bound_recheck.py` 直接读取执行，其 blob 内容 SHA-256=`6C4EA102474DC495835809FA78D437BF9D7C7CEA291F8DB7D505355B7A6E1368`；没有把 S16 文件移入当前差异。

party/uCMP/shuffle test stdout 外部保存于 `C:\Users\28641\.codex\artifacts\BMW16_S27_20261009\run_01\`：

| 文件 | SHA-256 |
|---|---|
| `party_test.log` | `0802926199B2F2046DAC8220DE1D0CEE7DB361CF796CA0A45E060A6E15D374B6` |
| `ucmp_test.log` | `19EAF43821A7660EC323A87C8457BF74823BEB296C39F5E01AA8A683AA50F061` |
| `shuffle_test.log` | `19EAF43821A7660EC323A87C8457BF74823BEB296C39F5E01AA8A683AA50F061` |

日志仅含测试配置/计数和退出码；未发现 score share 向量、FSS key、证书私钥、sidecar 或完整 mask。所有 S27 binaries 与临时材料留在仓库外；没有正式时间、网络、RSS/PRG 或 LAN/WAN 指标。

## 6. 门禁

| 门 | 状态 | 范围 |
|---|---|---|
| `DCF_SINGLE_KEY_PRIVACY` | `CONDITIONAL` | 完整 key 的 source-specific 理想扩展器证明；实际 bridge 需要 AES-126 PRG 与 AES-CTR root-stream 假设及独立审查。 |
| `ADAPTIVE_FULL_POOL_VIEW` | `CONDITIONAL` | correlated alpha、全池 unused keys、slot order、自适应公开 bit 与同 key 两 Eval 均已纳入 hybrid；依赖上行单 key 与独立 RNG/collision 条件。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | `CONDITIONAL` | source-level forward/inverse equations 和候选 simulator；依赖 ideal masks/T、准确 leakage L、OS randomness 与独立验证。 |
| `SAMPLER_ABORT_GUARANTEE` | `CONDITIONAL` | S16 ideal uniform-without-replacement math 复跑通过；实际 SHA-256 counter 仅 RO 条件。 |
| `EXPERIMENTAL_FUNCTIONAL_REGRESSION` | `CONDITIONAL` | 当前 runtime 未修改；本窗口定向重建通过，不等于异进程 E2E 接收。 |
| `SECURE_ALIAS_READY` | `UNPROVEN` | 未有另一未参与证明的接收者签收，不创建 secure alias。 |
| 正式 LAN/WAN 性能 | `NOT_RUN` | 不属于本窗口范围，所有正式性能字段 `NOT_MEASURED`。 |

## 7. 文件变更与提交边界

本轮新增 S27 决策/验证文档、两处 dated 状态同步与 TEST_ONLY 模型交接说明/trace checker。DCF、uCMP、shuffle、party runtime、材料 ABI、允许泄露字段、协议轮次和正式性能边界均未更改。`VFSS-baseline/`、Papers、keys、certificates、sidecars、build outputs 和大日志不在 Git 差异。S27 是研究性证明文档，必须交给独立接收者逐条复核后，才可用于任何 secure/security acceptance。
