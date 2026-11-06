# M6B-I-PROJECT-SELECT-5ITER-S23 详细复核报告

日期：2026-10-16
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
结论类型：源码/代数审查，不是新的功能测试、完整安全证明或独立密码学签核。

## 1. 摘要裁决

S23 复核 S18 接收的 S15 默认关闭修复快照，重建 score adapter 的输出 share 合同，并分析给定 Q_b 后的 priority-key record 与 M2CM/v1 前向 shuffle 单方视图。

| 门 | 裁决 | 说明 |
|---|---|---|
| S15 源快照 | PASS | S18 清单 72/72 SHA-256 匹配，canonical hash 一致 |
| 密码学审查独立性 | NOT_SATISFIED | 当前执行者参与 S19–S22 推导/报告，无新非作者 reviewer |
| F_scoreconv 输出 share law | OPEN | 代码有返回值和 correctness 关系，但没有规范 ideal functionality/share-law API |
| DCF correctness 下的 stable key share | CONDITIONAL | 由 R34 运算和 DCF 正确性推出；S23 未重跑原语测试 |
| 固定 Q_b 时的 q_b/key_records | PASS | 从 Q_b 可确定解码，记录就是 (Q_b[i],0,0) |
| 固定 Q_b 的 M2CM forward 子视图 | CONDITIONAL | 理想独立均匀 dealer tape 下有逐方 simulator；不包括 scoreconv 前史 |
| 完整给定 Q_b 的 scoreconv view | OPEN/NO_GO | S22 sampler 未证明 V_pre 在 Q_b 条件下的分布 |
| 两种材料池、自适应链、inverse/output/crash | NOT_RUN / OPEN | 前置条件模拟未闭合，不外推 |
| 真实攻击 / 运行时缺陷 | 未发现 / NONE_REPRODUCED | 只有合同/证明缺口；不修改 runtime |
| 正式性能 | NOT_ALLOWED / NOT_RUN | V3 OPEN、V4 未获准 |

因此，P0、P1 都不能对“本方输入 share、理想 Q_b、L 足以模拟此前 scoreconv 状态和后续 shuffle 全部本地视图”判 PASS。更窄的结果是：Q_b→q_b→key_records 为确定映射；M2CM forward 子协议在理想独立 tape 下可条件模拟；第一个全链断点在 carry/sign view 条件化到 Q_b。

## 2. 源码身份和独立性

被审对象为 S18 锁定的 S15 源码，不是 S23 worktree 当前 main 同路径文件：

| 项 | 值 |
|---|---|
| Worktree / branch | C:/Users/28641/.codex/worktrees/m6b-project-select5-s23/MoE_Top-k_Reproduction_Evaluation / codex/m6b-project-select5-s23 |
| Base | c3926c68fd14f270faa8b55234311071947fa080 |
| S15 source manifest SHA-256 | 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816 |
| S18 receipt manifest SHA-256 | 5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78 |
| 72-file canonical source SHA-256 | c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498 |
| 本轮 hash 复核 | 72 match / 0 mismatch |

实际源目录：C:/Users/28641/.codex/evidence/M6B_I_S18_20261016_02/source。Canonical digest 按 S18/S22 规则复算：UTF-8 ordinal path 排序，每行 path<TAB>lowercase_sha256<LF>。manifest 与核心代码哈希见配套 source/evidence manifest。

独立性披露：执行者参与 S19–S22 推导与报告，故 INDEPENDENT_CRYPTO_REVIEW=NOT_SATISFIED。S22 非作者 reviewer 只签核 carry→sign local pre-output 范围，不含本轮要求的固定输出 share 条件模拟。

本轮只读，无 CMake/build、CTest、E2E、网络 trace 或统计实验；没有改 VFSS、材料、构建物或历史 evidence。S18 功能接收仍是历史 PASS_WITH_SCOPE，不是 S23 新测试。

### 威胁模型与证据类别

沿用已确认模型：可信离线 T 生成双方材料后退出、不与 P0/P1 合谋；在线最多一方半诚实腐化；两方使用私有认证通道。模拟器可输入仅为腐化方自身输入 share、其理想输出 share Q_b、公开参数与获准 L；T 的完整材料/key pair 是 functionality 内部随机性，不交给 simulator。L 包括 masked values、shuffle 后的 endpoint/pivot/comparison transcript、活动迭代 seed、比较次数/通信长度和统一 ABORT。S23 没有从该清单推导出新的 peer-visible 字段；网络逐帧检查未做。

本报告的源码逐行描述属于“本地实现行为”；优先键加法正确性属于“项目代数推导”，依赖 DCF correctness；S22 的范围受限 carry→sign 结论属于“既有审查证据”；均匀 share 假设、F_scoreconv/v2 refresh 则分别是被证伪的不匹配理想 law 和“待验证设计”，不是论文定义。没有把论文 theorem 写成整个 Protocol I 安全结论。

## 3. F_scoreconv 实际接口和数学合同

### 输入、R34 中间态与消息

S15 的入口是 VFSS/src/moe_topk/protocol_i_score_input.cpp:79 的 protocol_i_raw_score_input_party。每方输入 uint32_t raw_score_share，合并语义为模 2^32 加法；逻辑输入之外 padding 固定 P0=0x80000000、P1=0（行 86–88）。比较输入环是 R34=Z/(2^34)，由 kScoreBits=34、kScoreMask=2^34−1 定义（行 10–11）。

每槽先做 carry 阶段。P0 本地左/右未加掩码值为 (0,x0)，P1 为 (0xffffffff−x1,0)，再加各自的左右材料 mask share；phase 4 交换 64-bit big-endian masked operands（行 90–103）。本地 DCF 输出 carry share c_b 后：
```tex
lift_b = (x_b - ((c_b & 3) << 32)) & (2^34-1)
```

carry share 本身不作为明文 bit 发给 peer。

sign 阶段以 P0 左常数 0x7fffffff、P1 左常数 0 为输入，并把 lift_b 加到本地 sign mask share；phase 5 交换该 masked frame（行 112–124）。本地 sign share s_b 后，行 131–135 返回：
```tex
q_0 = 0x7fffffff - lift_0 + ((s_0 & 3) << 32)  mod 2^64
q_1 =                 0 - lift_1 + ((s_1 & 3) << 32)  mod 2^64
Q_b = ((q_b << h) + delta_b) mod 2^(33+h)
h=index_bits_for(padded_n); delta_0=i; delta_1=0
```

Q_b 是优先级键加法份额，不是最终 Top-K mask share。S15 头文件将 adapter 注释为 Q20.12 raw-word 到 padded priority-key ring 的窄接口；仓库没有命名为 F_scoreconv 的规范理想 functionality 或 share-law API。

为使合同可审查，S23 将当前代码诱导的候选功能明确写为 F_scoreconv^S15：输入两方 uint32_t additive shares、公开 N/K/padding/索引位宽；可信离线 T 的 carry/sign materials 与其随机 tape 是 functionality 内部预处理，不交给 simulator；功能按 S15 的真实 key-generation、R34 运算和 phase 4/5 消息语义采样一对 Q_0,Q_1，私下分别输出给 P0/P1，并只把批准的 masked frames/长度/统一 abort 纳入 L。输出联合 law 是内部 T tape 与双方实际 Eval 诱导的相关分布 (Q_0,Q_1,L_score)，而非独立 shares；正确性要求 Q_0[i]+Q_1[i]=κ_i（依赖 DCF correctness）。这个 law 不承诺均匀性，且真实 getrandom/FSS RNG 与理想 tape 的距离未量化。该候选定义说明了实际合同应包含哪些内容，但还不是仓库现有 API，也没有给出其完整条件 sampler。

### 正确性不变量（条件于 DCF correctness）

令 r=(x0+x1) mod 2^32。carry DCF correctness 给出组合 carry C=1[x0+x1≥2^32]，因此 lift_0+lift_1=r mod 2^34。sign DCF correctness 给出 S=1[r>2^31−1]。把 r 解读成 signed int32 分数 score（r<2^31 时为 r，否则为 r−2^32），则：
```tex
q_0+q_1 = 0x7fffffff - r + S*2^32 mod 2^33
        = 0x7fffffff - score mod 2^33
Q_0[i]+Q_1[i] = ((0x7fffffff - score_i) << h) + i mod 2^(33+h)
```

定义 `κ_i := ((0x7fffffff - score_i) << h) + i mod 2^(33+h)`。这与分数降序、original_index 升序的 stable priority key 一致；此处 `κ` 是优先级键向量，不是用户的 Top-K 参数 `K`。这里是源码代数推导，依赖原语 correctness；本轮没有执行测试，所以不是新的测试证据。

### 实际 share law 不是均匀

对本方输入 share x_b，源码蕴含：
```tex
Q_0[i] mod 2^h = i
Q_1[i] mod 2^h = 0
(q_0 mod 2^32) = (0x7fffffff - x_0) mod 2^32
(q_1 mod 2^32) = (-x_1) mod 2^32
```

因此以下事件在实际输出中概率为 1：
```tex
E_b := (Q_b[i] mod 2^h == (b==0 ? i : 0))
       && ((Q_b[i] >> h) mod 2^32
           == (b==0 ? 0x7fffffff-x_0[i] : -x_1[i]) mod 2^32)
```

若错误地把理想 Q_b 设为整个 R_(2^(33+h)) 均匀，该事件概率是 2^-(h+32)。这只是“均匀 share ideal law 与代码不匹配”的精确见证，不是对当前路线的攻击。协议诱导 law 可定义为：可信 T 按 S15 carry/sign 材料生成分布发放 one-shot 材料，双方执行现有转换并输出实际 Q_b，且 Q_0[i]+Q_1[i]=κ_i；不声称均匀。真实 getrandom/FSS RNG 与理想独立 tape 的距离仍是未量化的 epsilon_RNG_joint。

### 对外消息

phase 4/5 帧绑定 session、fingerprint、N、K、comparison width、两方身份、phase、协议版本及 material ID；word 编码为 big-endian（S15 protocol_i_score_input.cpp:13–61）。peer 看见 masked operand frames 与通信长度，但不直接收到 c_b、s_b、q_b、完整 Q_b 或原输入下标。Q_b 随后形成本地记录 (Q_b[i],0,0)，第一轮 shuffle frame 再由随机 a_b 掩码。因此本轮未发现超出已批准 masked-value L 的新 peer-visible 字段；逐帧网络检查 NOT_RUN。

## 4. 给定 Q_b 后的 q、key_records 与 forward shuffle

S15 protocol_i_project_derived_select5.cpp:1890–1906 取 adapter 返回 keys，并构造 key_records[i]={keys[i],0,0}。故给定 Q_b 后，q_b=Q_b>>h mod 2^33 和 key record 都是确定值；模拟不需 peer score。

### Shuffle 子协议实际方程

记录群 G=Z_(2^c)×Z_(2^64)×Z_(2^64)，c=33+h。S15 run_two_round_shuffle（protocol_i_project_derived_select5.cpp:618–662）按以下关系工作：
```tex
M_b = sigma_b(Q_b) + a_b                    phase 1
V_b = tau_b(M_(1-b)) + e_b
D_b = V_b + r_b                              phase 2
Y   = D_0 + D_1                              each party sums received frames
```

Dealer 在 protocol_i_parallel_shuffle.cpp:336–363 生成共同 pi、独立均匀 sigma0/sigma1，tau0=pi∘sigma1^-1、tau1=pi∘sigma0^-1，以及均匀独立 a0,a1,h,r0,r1；e0=−tau0(a1)−h，e1=−tau1(a0)+h。仓库 permutation helper 的表示满足 tau_b∘sigma_(1-b)=pi，故：
```tex
V_0 = pi(Q_1) - h
V_1 = pi(Q_0) + h
Y   = pi(Q_0+Q_1) + r_0+r_1
```


### P0/P1 的 forward 单方 sampler

P0 simulator 输入 Q_0 和获准 L 中相关 masked frames/aggregate：
1. 理想 Dealer tape 下采样本地 (sigma0,tau0,a0,e0,r0)。对任意固定 pi，sigma0 与 tau0 分别均匀；a0、r0 独立均匀；h 均匀使 e0 对 P0 的其余局部值边缘独立均匀。
2. 计算本方首帧 M0=sigma0(Q0)+a0。
3. 使用 L 中接收的 M1，计算 V0=tau0(M1)+e0、D0=V0+r0。
4. 使用 L 中 Y 设置收到的对端第二帧 D1=Y−D0；输出本地 V0、Y、帧次序和本地 material view。

真实协议中给定本地材料及 M1 后，对端独立均匀 r1 使 D1、Y 在 G^n 均匀，故该模拟在理想 tape 模型下精确。

P1 simulator 对称：采样 (sigma1,tau1,a1,e1,r1)，算 M1/V1/D1；取 L 中 M0 与 Y，并令 D0=Y−D1。对端独立均匀 r0 给出相同的条件分布。两方 simulator 都不取 peer 的未掩码 Q，也不把共同 pi 作为输入。

此证明只覆盖 M2CM/v1 forward shuffle 子协议，并假定 L 确实包含该时序下批准公开的 masked values。实现使用 OS getrandom 和 FSS PRNG；其联合偏差没有量化，所以落到实现时仍是 CONDITIONAL/UNQUANTIFIED。它没有模拟 carry/sign key、材料池、lease/restart、inverse shuffle、membership、mask output 或 crash。

## 5. 首个条件分布断点

对腐化方 b，需模拟的完整 score-conversion 前史是：
```tex
V_b^pre | (x_b, Q_b, L_score)
```

其条件概率为：
```tex
Pr[V=v | x_b=x,Q_b=q,L=l]
 = Pr[V=v,Q_b=q,L=l | x_b=x] / Pr[Q_b=q,L=l | x_b=x]
```

（只对正概率条件定义）。

S22 的 sampler 先生成 pre-output view V，再由同一 carry/sign local DCF shares 得出 Q；它没有接收 q，也没有证明 key replacement 在 Q=q 条件下仍有效。不能把同一 key 的两次 Eval、carry/sign 两个 DCF key、mask shares 或 dealer tape 当作独立样本重抽。由未条件化 computational indistinguishability 也不能直接推出指定输出 share 下的条件分布不可区分；缺少事件概率下界或条件混合引理。

所以第一个全链断点是 scoreconv 的 output-conditioned local view，而不是 q→key_records，也不是固定 Q 下的 forward shuffle。没有找到真实区分攻击；这里是当前证明不覆盖所要求条件分布。尚未定义的 F_scoreconv share law 是合同缺口：至少要明确 ideal 输入、输出 shares、share 随机分布、T preprocessing 的内部角色和与 L 的 joint law。

S22 的范围限定结论保持原状：非作者 reviewer 对 P0/P1 carry→sign local pre-output transcript，在 ideal joint RNG tape 与 fixed-parameter BCG+21 key-only bridge 下给出条件性复核；候选项为 68·epsilon_G + epsilon_RNG_joint，T′ 与 epsilon_RNG_joint 未量化。S23 不推翻该局部结论，也不将它扩大到固定 Q_b 条件事件。

## 6. 两种 schema、后续组合和 crash

- N=128：M6B5/v3 + M2CM/v1 + M6P2/v1。只推导了 M2CM forward 子协议；没有重算所有 carry/sign/Select/membership/shuffle 已用、未用、拒绝槽或 durable lease。
- N=256：M6S2/v2。未重算 segment/slot/pool 或 restart 路径；不从 N=128 外推。
- adaptive Select、inverse shuffle、membership、最终 mask share、PREPARE/COMMIT、ABORT/crash 没有组合模拟。S18 历史指定故障证据只支持 no-complete-mask；部分 crash prefix 可能留下 P1 的 8-byte 本地 XOR share，不代表原子无输出/all-or-nothing；S23 未复跑这些用例。
- 五个算法迭代、最多九个明文比较依赖层和完整 raw-score→mask exchange 数分别记录；本轮未重算端到端 DAG，轮数 NOT_CLOSED。

## 7. 最小版本化改造方向（未实现）

候选 `F_scoreconv/v2`：先按 S15 生成 priority-key shares `Q_0,Q_1∈R_c^n`，再加入独立的一次性零和刷新份额 `rho_0←U(R_c^n)`、`rho_1=−rho_0`，输出 `Q'_b=Q_b+rho_b`。因此逐槽保持 `Q'_0[i]+Q'_1[i]=κ_i`。若 `rho_0` 与 scoreconv 的完整预输出视图及其他 T 材料独立，则对固定 `x_b,v,q'` 有

```tex
Pr[V_b=v,Q'_b=q' | x_b] = Pr[V_b=v | x_b] / |R_c|^n
```

即理想模型下 `V_b ⟂ Q'_b | x_b`。对应的单方候选 sampler 是：先用 S22 范围内的 pre-output sampler 在 `x_b,L_score` 下生成 `(V_b,Q_b)`，再设 `rho_b=Q'_b−Q_b mod 2^c`；若该刷新份额和本方其他材料未在视图中造成额外相关性，此步骤给出正确的 `(V_b,Q'_b,rho_b)` 条件分布。该推导是版本化设计条件，不是对当前 ABI 的证明或独立审查结论。刷新后的 `Q'` 直接作为 shuffle word0；不得再从 `Q'` 解码 q。

候选 schema 新版本：N≤128 用 M6B5/v4 + M2CM/v1 + M6P2/v1；N=256 用 M6S2/v3。每个 slot/party 增加绑定 session/party/purpose 的一次性 `R_c` 刷新材料；`c=33+h≤41` 时可按一个 uint64 word 序列化，刷新 payload 下界为每方 `8N` 字节：N=128 为 1024 B，N=256 为 2048 B。完整 package/framing/峰值资源 `NOT_MEASURED`。它改变离线 ABI、材料账本与输出 share law，不增加在线消息阶段；不在本轮实现。实施前先证明刷新随机量对完整 dealer/party 视图的独立性和一次性消费，并检查刷新后自适应访问的条件模拟；实施后依次做 share-sum/refresh conformance、冻结 oracle differential、N=1/小规模全 K/N=128/256 的独立 T/P0/P1 E2E，再覆盖错版本/错绑定/重领/损坏材料/断连/abort-no-mask 与 default-OFF。仍需非作者独立安全复核。

## 8. 执行证据、状态和下一步

源文件复核的可复跑 PowerShell 逻辑如下（S23 实际执行了等价的一次性脚本；结果 72/72、0 mismatch、canonical digest 相同）：
```powershell
$manifestPath = 'C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\S15_SOURCE_MANIFEST.json'
$sourceRoot = 'C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\source'
$m = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$files = @($m.final_source_set.files)
$lines = [System.Collections.Generic.List[string]]::new()
$bad = @()
foreach ($f in $files) {
  $p = Join-Path $sourceRoot ($f.path -replace '/', '\')
  $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLower()
  if ($h -ne $f.sha256) { $bad += $f.path }
  $lines.Add("$($f.path)`t$h`n")
}
$ordered = [string[]]$lines.ToArray()
[Array]::Sort($ordered, [StringComparer]::Ordinal)
$canonical = [string]::Concat($ordered)
$utf8 = [System.Text.UTF8Encoding]::new($false)
$canonicalHash = [Convert]::ToHexString(
  [Security.Cryptography.SHA256]::HashData($utf8.GetBytes($canonical))
).ToLower()
```

本轮实际做了：72 文件 hash/canonical 复核；只读检查 score adapter、uCMP、Select5 key-record 点、M2CM dealer/runtime 方程；生成 S23 决策、报告、gates、manifest 并更新项目状态。`git diff --check` 通过；仅显示三个根文档的 LF→CRLF 转换提示。CMake/build/CTest/E2E/网络 trace/材料容量/正式性能均 NOT_RUN 或 NOT_MEASURED。

可声称：S18 接收的精确 S15 snapshot 有历史 N≤256 PASS_WITH_SCOPE 功能证据；在 DCF correctness 下，score adapter 两方输出相加为稳定 priority key；给定一方 Q_b 时 q/records 确定；理想 Dealer tape 下固定 Q 的 M2CM forward 子视图可模拟。

不可声称：Q_b 均匀；phase4/5 完整 view 可在固定 Q_b 下模拟；两个材料池/自适应访问/inverse/output/crash 已安全；整个 Protocol I 安全、secure alias ready，或该路线是 BB90/BMW16；本轮运行过测试或正式性能矩阵。

下一项最小工作：冻结 F_scoreconv API 与 share-law（协议诱导非均匀 share，或另立 refresh 版本），为 P0/P1 构造真实 V_pre | x_b,Q_b,L_score 条件 sampler/混合界，并由未参与 S19–S23 的密码学审查者独立签核。该门关闭前停止向 adaptive full-pool 和全协议组合外推。

