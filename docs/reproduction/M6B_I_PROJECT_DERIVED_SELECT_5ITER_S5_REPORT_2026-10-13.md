# M6B-I-PROJECT-SELECT-5ITER-S5：S4 接收与 Protocol I 候选复核报告

日期：2026-10-13
路线：`PROJECT_DERIVED_SELECT_5ITER`
组合：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
结论：**N≤128 功能候选复跑通过；不是异人独立接收；联合单方视图安全 OPEN；N>128 runtime NO-GO；正式性能 NO。**

本报告区分代码事实、运行观察和安全证明。它不声称本路线复现 BB90/BMW16，也不转移这些论文的算法、复杂度、概率或安全结论。

## 1. 接收对象、分支与证据身份

| 项目 | 实际身份 |
|---|---|
| S4 原 worktree | `C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation`，branch `codex/m6b-project-derived-5iter-s4`，HEAD `c3926c68fd14f270faa8b55234311071947fa080` |
| S5 隔离 worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s5`，branch `codex/m6b-project-derived-5iter-s5`，HEAD 同为 `c3926c68fd14f270faa8b55234311071947fa080` |
| S4/S5 本地 main、origin/main | 均为 `c3926c68fd14f270faa8b55234311071947fa080` |
| S4 状态 | 未提交改动与未跟踪文件，不是 main 已合并结果 |
| S5 状态 | 独立 worktree，未提交；最终文件清单见下文与 source manifest |

S4 开始前已有 `docs/PAPERS.sha256` working-tree diff 留在原 S4 worktree，未复制该差异。S5 worktree 中此文件为 `c3926c6` 的 baseline 内容、无 diff。S1–S4 报告、决策、实验脚本来自 S4 snapshot；本轮保留原文件，没有覆盖历史。

[S4/S5 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S5_SOURCE_MANIFEST_2026-10-13.json) 列出 S4 全部 36 个 dirty/untracked 文件的状态、长度、SHA-256 和 S5 对照哈希；28 个文件在 S5 中按字节相同。它将报告与代码身份分开登记。

S4 核心文件哈希：

| 文件 | SHA-256 |
|---|---|
| S4 报告 | `0a09f8eb5b9e0bb25239e2079358889c4942f3ca7fe37095a6490b7549475a80` |
| S4 决策 | `ab873eef1c05befcdf89a555156ba9c21c0c1988c87f6e4d718f38a5a2e64cb3` |
| S4 三进程 E2E 源 | `2f9ad122a47b6f4c13da195f0a1d818dd5003a388afa484f318e75e83a0b5b33` |

**独立性限制：**我参与过 S4 的实现/报告，因此本轮虽然以全新外部 build directory 重建和复跑，但不是未参与者的独立签收。`S4_RECEIPT=PASS_REPRODUCED_WITH_SCOPE`；`INDEPENDENT_ACCEPTANCE=NO_GO_NOT_INDEPENDENT`。

## 2. S4 四目标接收复跑

WSL2 Ubuntu 20.04.6，GCC/G++ 9.4.0，CMake 3.28.4，OpenSSL 1.1.1f，Release，`MOE_TOPK_ENABLE_EMP_OT=OFF`。外部构建目录 `/home/lwt2024212936/vfss-m6b-s5-review-build`。在 S5 snapshot 上 fresh rebuild 后复跑 S4 原四目标，均退出码 0：

| S4 目标 | 结果 | fresh 二进制 SHA-256 |
|---|---|---|
| `moe_topk_project_derived_select5_conformance_test` | PASS | `9204a465708daf743e9ef770113a0517d43227993594e78b0c65ac1281b8b9e5` |
| `moe_topk_project_derived_select5_process_e2e_test` | PASS | `5e454600a5d1b1fffd2f0f07d6733a8a88cf14620590cc2c24b015566c0368fc` |
| `moe_topk_m2_ucmp_conformance_test` | PASS | `31169204398bc9668a49d1587617cd970fcb58fcf07ef68cf80770ced1c8b7ee` |
| `moe_topk_m2_parallel_shuffle_conformance_test` | PASS | `cade4a3e6a3587c2e7a7e68e9894f67db185ea6bc1a11fe9e66366ca29be80dc` |

这是 S4 原测试范围的复跑，不将其冒充 S5 扩展功能测试。

## 3. S5 修改：材料、防重领和 TLS

### 3.1 `M6B5/v1` durable lease

T 生成输入无关材料，写入两个 party package。ready record 绑定 package length/SHA-256、session、fingerprint、material ID、logical/padded N、K、comparison width。package 和 ready 使用 exclusive create，落盘内容与目录同步。P0/P1 在受限目录校验 owner/mode、ready 与 package digest 后，以每方独立 `O_EXCL` lease 竞争 claim；lease fsync 完成后才读出/释放 package bytes，读前/读后再次验 digest。

合同为 fail-closed：未发布 ready、wrong binding、package tamper、已存在 lease、claim 后崩溃、DCF/协议错误均不释放第二份 key，不返回 mask。claim 前崩溃保留可领状态；claim 后即使 consumer 崩溃也 burn package ID；两个并发 claim 只能一个成功。它依赖本地 POSIX `O_EXCL`/`fsync` 持久语义、受保护目录和不回滚/删除存储；管理员删除 lease 或从旧备份还原不在合同内。无外部 material service、无在线 Dealer。

包级 lease 防跨进程/重启重领；进程内再按 iteration/purpose/substage-scope/direction/endpoints 查找材料并检测重复边。每个调用和 iteration 使用自己的 one-shot DCF key slot。

**T exit gate 的实际范围：**package ready 发布是独立 API；S5 T/P0/P1 E2E 的父 supervisor 在 `wait_child(T)==0` 后才调用 publish，T 非零退出测试确认 ready 不发布、双方 no-mask。类库 API 本身不验证调用者是不是 supervisor 或 T 是否已退出，因此部署必须由受信的独立 supervisor 以同样顺序调用；该编排没有成为自治 daemon，也不能对任意调用者作更强保证。

### 3.2 `M6S2` streamed slot-store prototype

新增独立原型按 slot 写有界 segment（默认 16 MiB、单槽≤1 MiB），manifest 每条记录 SHA-256 并绑定 session/fingerprint/material ID/N/K/width/party/expected slot count、purpose/iteration/substage/direction/endpoints。manifest 上限 64 MiB；ready 仅在槽数、segment 和 manifest 完成同步后发布；party 以 per-slot `O_EXCL` lease 先烧地址再读 bytes，前后验哈希。

测试通过 unpublished staging、重复地址拒绝、完整槽数、绑定不符、回读、claim 后崩溃、重领、并发单赢家和 ready 后篡改检测。这个原型没有接 Select5 Dealer/runtime；不能称为 N=256 Select 或协议执行。

### 3.3 TLS 1.3 mTLS 与 opt-in

共享 framed transport 增加 TLS 1.3-only client/server context、双方证书验证、peer identity/SAN 校验与 deadline I/O。material ID 非零时用应用帧 v2，绑定 session、fingerprint、material ID、N/K/width、sender/receiver、type/phase/sequence。Select5 runtime 要求 TLS endpoint 和期望 peer identity，无 plaintext fallback；plaintext 仅允许显式 TEST_ONLY harness，TLS/plaintext 混用拒绝。临时测试 CA/证书清理，不存入仓库。部署证书 provisioning、更新和吊销尚未完成。

`MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL` 默认 OFF。Select5、streamed-store、TLS conformance 新目标均在选项开启后注册。共享 transport 编译 TLS helper，因此默认 VFSS build 也需 OpenSSL 1.1.1；默认排序 alias/pipeline 不变。

## 4. 算法、稳定键与 membership 复核

### 4.1 控制状态与精确正确性

控制器身份固定为 `PROJECT_DERIVED_SELECT_5ITER`。节点是 shuffled coordinates，rank 从 1 开始。每个仍未命中的前四迭代对候选 C（`m=|C|`）抽 `s=min(m,ceil√m)` 个不同样本，冻结样本 clique 并恢复样本严格次序；按实际整数参数算 `q_low/q_high` 并选 pivots。之后为两个 pivots 分别连接 `C\S`，用所有 pivot 对候选边准确算 `rx<ry`，按 residual rank 落在小于/等于/区间内/大于的分支更新 candidate 和 residual。第五个迭代若尚未命中 pivot，则对剩余候选生成完整 clique 并按全序精确计算 residual rank。

样本 clique 与 pivot-star 子层端点不重合；当轮图冻结后再比较。跨轮 endpoint pair 可重现，但槽位含 iteration，因此不会复用 key。只要 DCF comparator 对输入给出一致的严格 total order，前四轮每一步保持“目标元素在 C 中且 residual 是其 C 内 rank”的不变量，第五层 complete clique 对任意 random tape 精确选出目标。随机 seed 只影响 cost，不影响算法的正确性事件；无效位、非全序、资源/超时、材料或 transport 错误均 ABORT/no-mask。

输入是 signed Q20.12 32-bit score，降序；同分按 `original_index` 升序。`1≤K≤n≤128`。raw-score adapter 使用两个 34-bit uCMP 阶段构造 packed priority key（含 index bits）；padding score 是 signed minimum、padding index 排在 logical items 后；前向 shuffle、Select、membership、inverse route 后截回 logical N。stable key 唯一时 `key_i≤τ` 用 `1−strict_lt(τ,key_i)`，测试核对原序 mask 二值且恰 K。

随机采样实现从 `getrandom()` 得 64-bit public seed，再按 iteration 派生 seed 并用 MT/Fisher-Yates rejection sampler 生成图。S2 期望 O(n) 证明是假设理想 fresh uniform-without-replacement sampler；不能把其期望界转写为此 64-bit seed/PRNG runtime 的证明。S5 不提出新的期望或高概率成本结论。

### 4.2 DCF 实际 ABI

`protocol_i_ucmp.cpp` 按 `alpha=(m_left−m_right) mod 2^b` 调用 `keyGenDCF(b,64,alpha,1)`。每 party 的 M2UC key 含本方 k 与双方同一 alpha 相关的 g/v correction data。两方交换局部 `x_share+mask_share` 并共同得到 masked `z`；对 `x=(z_left−z_right)` 和 `y=x−2^(b−1)` 各调用一次 `evalDCF`，两方将输出 shares 相加打开 strict compare bit。每次 uCMP 的每 party DCF Eval=2，双方总计 4；key slot 单次消费。uCMP conformance 与 E2E 验证函数 ABI/方向，不代表 key 视图可模拟。

## 5. S5 构建、命令与测试结果

配置 Release 时使用 CMake 3.28.4 wheel wrapper：

```bash
PYTHONPATH=/home/lwt2024212936/.local/cmake-3.28.4 \
  /home/lwt2024212936/.local/cmake-3.28.4/bin/cmake \
  -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-derived-5iter-s5/VFSS \
  -B /home/lwt2024212936/vfss-m6b-s5-review-build \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
```

Build targets：

```bash
cmake --build /home/lwt2024212936/vfss-m6b-s5-review-build --target \
 moe_topk_project_derived_select5_conformance_test \
 moe_topk_project_derived_select5_material_store_test \
 moe_topk_streamed_material_slot_store_test \
 moe_topk_tls13_transport_conformance_test \
 moe_topk_project_derived_select5_process_e2e_test \
 moe_topk_m2_ucmp_conformance_test \
 moe_topk_m2_parallel_shuffle_conformance_test -j2
```

最终运行顺序为上表七个可执行目标依次运行并以 `set -o pipefail` 管线记录到 `/home/lwt2024212936/vfss-m6b-s5-review-build/s5-final-tests-2026-10-13.log`。七个 suite 退出码均为 0。完整命令及 15 条 `CASE_JSON` 收在 [逐案 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S5_E2E_CASES_2026-10-13.json)。

| 最终 S5 二进制 | SHA-256 |
|---|---|
| Select5 controller conformance | `9204a465708daf743e9ef770113a0517d43227993594e78b0c65ac1281b8b9e5` |
| durable package store | `33cb20437860388a04e59f8a2af9faca0fa52f7e2357934b7644050f0bf5d69c` |
| streamed slot-store prototype | `0ce66954f4b5168da7f1e77227463d4be8ba3367a6c6b8e8e2e2db7da9730577` |
| TLS 1.3 mTLS conformance | `7266f81758308ddc17bbbb47ec17b10f9b6fe2a225246e07910f8708ad6ac2bd` |
| T/P0/P1 process E2E | `1943f774ed325e740e023eb779639a9d0eb7de2ae52ef619c1915b17b68a44d5` |
| uCMP/DCF conformance | `31169204398bc9668a49d1587617cd970fcb58fcf07ef68cf80770ced1c8b7ee` |
| parallel shuffle conformance | `fa253556d0c155c1ce56f8274e9732fd9665a2600434a66603056f363f521271` |

成功 oracle E2E 10/10：

| n | K | logical compares | L | online exchanges `9+2L` |
|---:|---:|---:|---:|---:|
| 1 | 1 | 1 | 1 | 11 |
| 7 | 1 | 13 | 2 | 13 |
| 7 | 7 | 13 | 2 | 13 |
| 128 | 1 | 360 | 6 | 21 |
| 128 | 2 | 299 | 3 | 15 |
| 128 | 8 | 724 | 9 | 27 |
| 128 | 64 | 2466 | 9 | 27 |
| 128 | 65 | 2831 | 9 | 27 |
| 128 | 128 | 311 | 4 | 17 |
| 128 | 64, second T seed | 4857 | 9 | 27 |

输入 fixture 含重复值、正负整数、`0x80000000`/`0x7fffffff` signed Q20.12 边界，K 包括边界和 median 附近。N=128 这 7 个观测比较调用数 min/median/max=`299/724/4857`；它只是测试选择下的诊断值，不是期望界、置信区间或性能结果。每例都由 frozen C++ stable oracle 逐位比对原序、长度、bitness、权重 K。

5/5 故障用例均 no-mask：package ready 后篡改；首阶段 peer disconnect；party package seed 不一致；TLS peer identity 错误；T 异常退出且 ready 未发布。包存储测试还覆盖 unpublished staging、wrong K/session、重复领取、tamper、claim 前崩溃仍可领取、claim 后崩溃 burn、并发两进程单赢家。TLS 测试覆盖 TLS1.3 双证、peer SAN、material-bound frame 和错身份拒绝。没有观察到算法错误或 oracle mismatch；注入故障是预期 abort，不作为算法失败统计，也没有重试掩盖失败。

默认 OFF 核验目录 `/home/lwt2024212936/vfss-m6b-s5-default-build` 配置 Release 且 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF`。`target help` 中不包含 Select5、streamed-store 或 TLS conformance 新 target，`default_experimental_targets=0`。第一次误用系统 CMake 3.16.3 被 minimum version 拒绝；第二次未给 CMake wheel 的 `PYTHONPATH` 报 `No module named cmake`；加正确 module 路径后配置/target 检查成功。TLS conformance target 为满足 default-off 被移进实验选项块。第一次整组运行在 30s wait 内只返回中间输出，未登记为最终结果；完整顺序复跑获得退出 0 和 15 条 JSON 记录。S4 期间的历史构建/测试失败修复仍在 S4 报告，S5 未删除。

## 6. 比较成本、材料容量及轮数

候选 C 大小 m、样本大小 `s=min(m,ceil√m)` 时，前四轮每个 active iteration 产生 `C(s,2)` 样本 clique 边及 `2(m−s)` pivot-star 边。它们当轮端点不相交；同轮 set 去重。跨轮可出现相同无序端点对，但 iteration-bound slot 不同。第五层费用为 `C(|C4|,2)`。每边是一条方向固定的 `left<right` 有向请求、一个逻辑比较/无序关系、一个 key slot；每个 local comparator 两次 `evalDCF`，双边总计四次。

最坏 Select 调用为 `12N+C(N,2)`：padded N=128 为 9,664；256 为 35,712；1024 为 536,064，故最坏复杂度为 O(N²)。runtime E2E 未记录跨层 unique-edge union，`NOT_MEASURED`，不将 call count 冒充 unique edges。每 party 序列化 key slots=`7C+3N`：2N score、5C Select、N membership、2C M2CP shuffle；最后两池现有两轮 shuffle 不消费但 package 仍含其 key。

单个 uCMP key `D(b)=57+24b` bytes，M2CP 包 `M(N,b)=56+80N+C(N,2)(8+D(b))`；M6B5/v1 逐字段公式：

```text
b = 33 + log2(N)
M6B5 = 52 + 2N(D(34)+40)
      + [8 + 5C(D(b)+40)]
      + [8 + N(D(b)+40)]
      + 2[M(N,b)+8]
```

| logical N | padded N | bits | C | per-party slots | worst compares | package / party | 64 MiB |
|---:|---:|---:|---:|---:|---:|---:|---|
| 128 | 128 | 40 | 8,128 | 57,280 | 9,664 | 60,008,580 B, exact serialized | 余量 7,100,284 B |
| 256 | 256 | 41 | 32,640 | 229,248 | 35,712 | 245,683,268 B, formula | 超 178,574,404 B |
| 1,000 | 1,024 | 43 | 523,776 | 3,669,504 | 536,064 | 4,109,070,020 B, formula | 超 4,041,961,156 B |

N128 package 若用当前 1,048,560B chunk payload，则推导 58 frames、60,012,292B including chunk envelope/header；S5 E2E 由 T 本机 staged file 提供材料，没有实测 T→party package wire，因此这些是 framing 公式而非 observed bytes。N≤128 每个 comparator wave 小于 1 MiB max payload；N=1024 的全图 operands `16*C=8,380,416B`，需 transport streaming/chunking。M6S2 manifest N256 预计 `229,248×76+64=17,422,912B`，低于 manifest cap；N1024 manifest `278,882,368B`，超 64 MiB。即使 N256 M6S2 manifest 估算可装，Dealer/runtime 未接线且无 N256 E2E，仍 NO-GO。

线上最大有序 exchange 数按实际 L 为 `9+2L`：9 个 adapter exchanges（binding1、score input2、forward shuffle2、membership1、inverse2、status1）加每个非空比较 layer 两个 exchanges，L≤9，故≤27。Select 算法 trace 有 5 个迭代槽，比较依赖层上限 9；若完整图能满足 CA 假设，通用 `2k+1` 可条件代入 k=9 得19，但该 candidate 未实例化 CA。Protocol I 论文核心3轮是独立论文目标，不是完整路径或本候选结果。

WSL 资源快照：`df -B1 /home` 可用 `1,009,491,193,856B`；`MemAvailable=7,189,032kB`。这不是 Select5 RSS；vector allocator 峰值、Dealer 峰值、磁盘峰值、正式耗时、WAN/TLS wire 计量均 `NOT_MEASURED`。本轮不运行 LAN/WAN 矩阵。

## 7. 实际 L、party view 与联合模拟缺口

| 字段/状态 | 观察者/坐标 | 复核 |
|---|---|---|
| session/fingerprint/material ID/N/K/width/schema | T/P0/P1，binding/package | 参数与身份绑定，不含分数 |
| public sample seed | T 生成，P0/P1 package/binding 可见 | 输入无关但不是当前 L 逐项列出的 raw 字段；唯一待窄确认项 |
| endpoints/pivots/slot/iteration/purpose/direction | P0/P1，shuffle coordinates | 可由 seed 与比较 bits 确定；原 index→shuffle 对应仍不发送 |
| `z_carry/z_sign`, `z_left/z_right`, `z_tau/z_item` | 双方 | 每槽 fresh `getrandom` mask shares 后双方合出 `z=x+m_l+m_r mod 2^b`；z 对双方公开，score/key/tau 原值不发 |
| comparator bit、比较数、消息长度、状态 | 双方；L_S4 字段 | bit 公开后可确定 sample order、pivot rank、candidate/residual 和后续 slot 顺序；失败不返回 output mask |
| shuffle material/view | T 与 party-specific view | T 知道 Pi/full masks/两方 package；party package 含本方 sigma/tau、record masks、整池 key；双方看 public masked records |
| result/abort/restart | party | 成功通过 final barrier 后才返回本方 XOR share；注入故障无 mask。断连时双方错误文本/时间可不同，应用结果均失败/no-mask |
| TLS identity/frame | direct peer / network | 对端校验身份；外部网络可见时序/长度，TLS 加密应用 payload |

`public_sample_seed` 的窄待决和处理建议见决策记录；已确认的 endpoints/pivots/slot/bits/length/masked operands/uniform abort 不重问。

密码视图审查实际 `protocol_i_project_derived_select5.cpp`、`protocol_i_parallel_shuffle.cpp` 与 `protocol_i_ucmp.cpp`。单 key 由 `alpha=(m_left−m_right) mod 2^b` 生成；local DCF `k` 与相关 `g/v` correction material、mask shares、双方打开 z、结果 bit 一起进入自适应多 key transcript。后续 edge/address/candidate 由先前 opened bit 选择。party 读入完整本方 package，模拟器不能只生成已用 keys；还必须覆盖未用 Select/membership/score keys、M2CP 两份未消费 shuffle DCF 池、local `sigma/tau`、masked records、forward/inverse route、output share、T 的 Pi boundary、ABORT/timeout/重启状态。

S5 没有为这些关系构造 P0/P1 simulator 或逐 hybrid，也没找到 concrete distinguisher。准确结论是没有证明、未检出区分器；不能用“masked z 单独均匀”、BGI15 不同 key 结果、BMW16/S31/S32 条件论证或允许公开 endpoints 替代组合证明。T 是 trusted offline input-independent party，看到 Pi/keygen masks/packages 后退出，不看 score/rank/threshold/output；T+party collusion在安全范围之外。TLS 与 lease解决传输/重放边界，不证明密码 view。`JOINT_VIEW_SECURITY=OPEN/NO_GO_FOR_SECURE_CLAIM`。

## 8. 门禁与未决工作

| Gate | 裁决 |
|---|---|
| `S4_RECEIPT` | `PASS_REPRODUCED_WITH_SCOPE` |
| `INDEPENDENT_ACCEPTANCE` | `NO_GO_NOT_INDEPENDENT` |
| `ALGORITHM_STABLE_TOPK` | `PASS_BOUNDED`，`1≤K≤n≤128`；随机带影响 cost、不影响正确性 |
| `DCF_MEMBERSHIP` | `PASS_FUNCTIONAL`，实际 ABI、stable order 与 `1−strict_lt` |
| `LEAKAGE_L_MAPPING` | `CONDITIONAL`，raw `public_sample_seed` 窄确认未完成 |
| `DURABLE_ONE_SHOT_LEASE` | `PASS_LOCAL_FS_CONTRACT`，不涵盖管理员删除/存储回滚 |
| `AUTHENTICATED_TRANSPORT` | `PASS_TLS13_MTLS_FUNCTIONAL`，生产证书运营未测 |
| `N=128` | bounded process E2E pass |
| `N>128` | `NO_GO_FOR_RUNTIME`，M6S2 未接入 |
| `JOINT_VIEW_SECURITY` | `OPEN / NO_GO_FOR_SECURE_CLAIM` |
| `FORMAL_PERFORMANCE_READY` | `NO`，V3 OPEN / V4 NOT_ALLOWED、NOT_RUN |
| BB90/BMW16/S34 | BB90 S1 NO-GO；BMW16-derived 独立；S34 非异会话接收 |

最小下一步：未参与 S4 的审查者按 source manifest、binary hashes、case JSON 复跑；只确认/移除 raw sample seed；针对完整单方 view 补 simulator/hybrid。N256 若继续则把 streamed store 接 Dealer/runtime、加入>1MiB operand framing、重算资源后做 N256 E2E。正式性能仍须待 V3/V4 与 benchmark plan 前置门。

## 9. 文件变更清单

S5 新增治理/报告文件：本报告、S5 decision、S5 E2E JSON、S5 source manifest。同步文档为 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/REFERENCE_MANIFEST.md`。S5 对 S4 快照的源/测试改动为：`VFSS/CMakeLists.txt`、`VFSS/include/moe_topk/protocol_i_score_input.h`、`VFSS/include/moe_topk/protocol_i_transport.h`、对应两份 `.cpp`、`VFSS/include/moe_topk/protocol_i_project_derived_select5.h`、其 `.cpp`、`VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp`。S5 新增 `VFSS/include/moe_topk/protocol_i_material_slot_store.h`、`VFSS/src/moe_topk/protocol_i_material_slot_store.cpp`、`VFSS/tests/moe_topk/protocol_i_material_slot_store_test.cpp`、`VFSS/tests/moe_topk/protocol_i_project_derived_select5_material_store_test.cpp`、`VFSS/tests/moe_topk/protocol_i_tls13_transport_conformance_test.cpp`。承接自 S4 且不在本轮修改的文件及所有 S4/S1–S3 历史工件由 source manifest 列出；`VFSS-baseline/`、Papers、参考工程无变化。

### 9.1 S5 源码 SHA-256

| 文件 | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `22aa598c4a16ccce30691e6e9a6883801c180af1ee752c8eda017bf31c621026` |
| `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h` | `0843c6717885f12421b37fed1285d6216c800fb590a97ca0984aa1287254d476` |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `f5b767130bf6d93fde58151629ef6f209257b7a53daf30cd4843c20cfdc01010` |
| `VFSS/include/moe_topk/protocol_i_score_input.h` | `61eca4c20e21a86c76593c2ee2bc22d887d78d33df28e44c7b879a171c2d09af` |
| `VFSS/src/moe_topk/protocol_i_score_input.cpp` | `007b67cb498d714aa26761099680f4a1d0b56acd653fd14453af928a0c977c05` |
| `VFSS/include/moe_topk/protocol_i_transport.h` | `87d479df29e06ea635c6fd8ead7b9295620fe869aa8a7e1d4195ddcfd4d5fd0e` |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | `54c2c7f5a803bc4f7ce44774e83ff79b674ca1fc05a748efeaa788a191a93dfd` |
| `VFSS/include/moe_topk/project_derived_select5.h` | `19177983f603beddd2cc237d7ca9ee10cea8c49e7b8bed87f71afbaf2a90fe75` |
| `VFSS/src/moe_topk/project_derived_select5.cpp` | `ecfa56c3d12f8a8183db680d9197bcaf75a18761ba1a4e078a300ca273f2651e` |
| `VFSS/include/moe_topk/protocol_i_project_derived_select5.h` | `698f0c0d27d4c4a1fd5b4c7aac1b5ef37d829963bbd4b29942dc4dda0378e862` |
| `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp` | `a4a5f1c63afd4bc9b621b4a46a203df62fcc924c0a7fa222b6595395f478f325` |
| `VFSS/include/moe_topk/protocol_i_material_slot_store.h` | `18cce441cf4f719d720ddab39f603e2c76b5b82621994d3248c78c91a8564690` |
| `VFSS/src/moe_topk/protocol_i_material_slot_store.cpp` | `d84c14d93ed58e1baf8a18737a6387074d288dd0d1fcefc96a3f8dd096d66792` |
| `VFSS/tests/moe_topk/project_derived_select5_control_test.cpp` | `56291adf3904670f26f53766c42d7353c0430a41cf618222c58af2aa0fcfa695` |
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_material_store_test.cpp` | `574cfa48f06b52aa7a2cf141465a72f51236ce0aa243d9a1b523e8b693bf294a` |
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp` | `51ad8c34c5b1170585b7cadceb9e3a158d7ece3f7e5106e8177083d8cbaa365d` |
| `VFSS/tests/moe_topk/protocol_i_tls13_transport_conformance_test.cpp` | `c8adb49eac16967f48d24ec5358bc11d6f60ae2425feb69c6290aa05409dac63` |
| `VFSS/tests/moe_topk/protocol_i_material_slot_store_test.cpp` | `4c2ce29ef13ab5d1614283aaabe5b176e6c6133b5f069233b5541a9c633715ff` |

S5 decision SHA-256 为 `7eae3cac642397859e020430f2490685f7854d9d94f5aeefcba7607031d0d3c5`；逐案 JSON 为 `56142221b7acfe35f784109fea763864489b71237659e299cef42708ede5207d`；S4/S5 source manifest 为 `feb5001cb89e334eaea85f765f05920645ac4762d0e510198a527fe2be7cb079`。本报告自身哈希在回复中单独给出。
