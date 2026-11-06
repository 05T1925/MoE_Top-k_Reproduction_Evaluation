# M6B-I-PROJECT-SELECT-5ITER-S24 详细复核与实现报告

日期：2026-10-17
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
任务分支：`codex/m6b-project-select5-s24`
基线 HEAD：`c3926c68fd14f270faa8b55234311071947fa080`（与本工作树登记的 main/origin main 起点相同；未合并）
接收对象：S18 已接收的 S15 默认关闭快照，不是本轮新接收的 S24 修改版
独立性：当前执行者参与 S19–S23 推导，`INDEPENDENT_REVIEW=NOT_SATISFIED`。

## 1. 裁决摘要

| 项目 | 状态 | 本轮结论 |
|---|---|---|
| S18 锁定的 S15 输入快照 | `PASS`（对象身份） | 从其外部 source root 复算 72/72 SHA-256，canonical SHA 仍为 `c4a7df6c…17498`。本轮修改后的候选不是 S18 已接收版本。 |
| `F_scoreconv/v1` 完整键 share contract | `PASS_WITH_SCOPE` | 在 DCF correctness 下，两份额重构为稳定 priority key；单方份额由真实 carry/sign 过程诱导，绝不假设均匀。 |
| v1 局部高位 `q_b` | `PASS_WITH_SCOPE` | v1 两份额低 h 位为 `i` 与 `0`，其和小于 `2^h`，故高位分解恰好可加。当前后续 runtime 使用 full key，不依赖独立 q API。 |
| v2 完整键零和代数 | `PASS` | 在 `R_c` 内逐条相加，重构键不变，包含模回绕。 |
| v2 局部 `q'_b=Q'_b>>h` | `NO_GO` | 不再可重构；提供了 `h=1,c=34` 的确切反例。runtime 不调用/不输出局部 q′。 |
| v2 F_scoreconv 输出条件视图模拟 | `CONDITIONAL/UNQUANTIFIED` | ideal joint RNG 下可由 S22 carry/sign pre-output sampler 推出精确单方 sampler；`epsilon_RNG_joint` 必须纳入本轮新 rho 抽样，尚未量化。本轮 reviewer 独立性不满足。 |
| v2 材料 ABI / feature-OFF 边界 | `PASS`（本地构建范围） | 新增 monolithic M6B5/v4 与 streamed M6S2/v3；ON 有符号/API，OFF 静态库无 helper 符号且无 v2 test target。 |
| v2 功能正确性 | `PASS`（已列测试范围） | 定向 CTest 7/7、streamed store、T/P0/P1 N≤256 E2E 的 7 个成功 case 通过 frozen oracle。 |
| 自适应 DCF 池、inverse/output/crash 联合视图 | `OPEN` | F_scoreconv 单方输出 share 引理不推出这些后续视图。 |
| 整条协议安全、`secure` alias | `NO_GO` | 未证明，也未创建 alias。 |
| 正式性能 | `NOT_ALLOWED/NOT_RUN` | V3 仍 OPEN，V4 未获准；本轮仅为功能与格式诊断。 |

本轮没有发现对 S15 v1 的可复现功能错误或攻击。新增 v2 首次集成时发现 ready 发布器仍拒绝新 package version v4；修正版本校验并在最终回归中验证。所有异常尝试保留于外部 evidence。

## 2. 被审对象与源码身份

S18 manifest：`docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S18_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-16.json`。其中记录：S15 source manifest SHA-256 `4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`，S18 receipt manifest SHA-256 `5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78`，72 文件 canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。S24 从 `C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\source` 按 manifest 路径重新核验：72 文件、0 mismatch；canonical 算法为 UTF-8、路径 ordinal 排序、每行 `path<TAB>lowercase_sha256<LF>`，结果与上值相同。

本 worktree 是 S15 dirty source overlay 的隔离衍生工作树，S24 起点 HEAD 为 `c3926c68…fa080`。它不能称为 S15/S18 已接收源码，也没有提交。与 S15 72 文件 manifest 比较，S24 到目前源码/根文档有 11 个既有路径变化；其中 8 个是运行时/构建/测试文件，3 个是项目状态文档。另新增 v2 enum header、v2 定向测试和本报告列出的 S24 文档。逐文件 S24 最终哈希见配套 source/build/evidence manifest。

运行时核心：

- [protocol_i_score_input.cpp](../../VFSS/src/moe_topk/protocol_i_score_input.cpp)：R34 carry/sign conversion、full-key share 返回和 v2 refresh helper。
- [protocol_i_project_derived_select5.cpp](../../VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp)：package/version/binding、dealer refresh material、monolithic/streamed load、刷新后 full-key records。
- [protocol_i_material_slot_store.cpp](../../VFSS/src/moe_topk/protocol_i_material_slot_store.cpp)：M6S2 schema/manifest/slot binding 与 one-shot lease。
- [protocol_i_score_share_abi.h](../../VFSS/include/moe_topk/protocol_i_score_share_abi.h)：v1/v2 ABI label。
- [protocol_i_score_key_refresh_v2_test.cpp](../../VFSS/tests/moe_topk/protocol_i_score_key_refresh_v2_test.cpp)：环代数、q′ 反例、序列化和错误材料检查。

## 3. F_scoreconv/v1 的实际输出 share 合同

### 3.1 输入、padding、R34 与字节编码

每个 online party 输入 `uint32_t raw_score_share[i]`；两方在 `Z/(2^32)` 相加得到 signed Q20.12 raw word。逻辑输入外的 padding 为 P0 `0x80000000`、P1 `0`，所以 dummy score 为 `INT32_MIN`；因为 K≤logical_n，dummy 排在真实记录之后，不进入 Top-K。score conversion 的 carry/sign 比较宽度固定 34 位：`R34=Z/(2^34)`，输入 operand 以 `& (2^34−1)` 截断。phase 4 carry frame 和 phase 5 sign frame 的每个 word 使用 64-bit big-endian 容器编码，逻辑值仍只在 R34；不能把 64-bit 容器宽度当成 R64 运算。

设 `h=index_bits_for(padded_n)`，`c=33+h`，完整 key 环为 `R_c=Z/(2^c)`。当 N=128 时 h=7、c=40；N=256 时 h=8、c=41。运行时存于 `uint64_t`，序列化为 big-endian 64-bit，协议比较明确只使用 c 位并按 unsigned priority key 顺序比较。h 是公开 padded-size 导出的位宽；非二次幂 logical N 使用 padded N 和原序 index，dummy 使用 padding index。

### 3.2 逐步关系和正确性

对 party `b∈{0,1}` 的局部输入 share 记为 `x_b`。carry 本地 mask frame 的未掩码基值为 P0 `(0,x_0)`、P1 `(0xffffffff−x_1,0)`；两方交换 phase 4 masked words 后各自计算同一 carry DCF 的本方 Eval share `c_b`。之后：

```text
lift_b = x_b − ((c_b & 3) << 32)  mod 2^34
```

sign 本地基值为 P0 `(0x7fffffff,lift_0)`、P1 `(0,lift_1)`，经 phase 5 masked frame 和 sign DCF 得到 `s_b`。代码诱导：

```text
q_0 = 0x7fffffff − lift_0 + ((s_0 & 3) << 32)   (mod 2^64)
q_1 =                    − lift_1 + ((s_1 & 3) << 32)   (mod 2^64)
Q_b[i] = (q_b << h) + (b==0 ? i : 0)            (mod 2^c)
```

最后一步使用 c 位 mask，因此左移和容器溢出均表示 `R_c` 运算，不是饱和算术。只要 carry/sign DCF correctness 成立：

```text
Q_0[i] + Q_1[i] = κ_i
κ_i = ((0x7fffffff − int32(score_i)) << h) + i  (mod 2^c)
```

`κ_i` 的无符号升序等价于 signed score 降序、score 同分时 original_index 升序。内部用户描述的 `Q_b` 是 priority-key 的 additive share，不是 score 值、阈值、rank、输出 mask share，也不是均匀 share。它由实际 DCF key/material 和输入份额共同诱导；v1 API 没有声明独立均匀分布。

### 3.3 v1 的 q 分解为何成立、其边界

实际 v1 有：

```text
Q_0[i] mod 2^h = i
Q_1[i] mod 2^h = 0
```

对任意加法 share 写 `Q_b=2^h q_b+r_b`，则高部重构为

```text
floor((Q_0+Q_1 mod 2^c)/2^h)
 = q_0+q_1+floor((r_0+r_1)/2^h) mod 2^33.
```

v1 `r_0=i`, `r_1=0`, `0≤i<padded_n≤2^h`，所以最后的 carry 为 0，`Q_b>>h` 可组合成目标高部。一般 share 不保证如此。现有 Select5 runtime 在 `protocol_i_project_derived_select5.cpp` 把完整 key share 放到 `word0` (`{keys[i],0,0}`)，再用于完整 c 位 shuffle/比较/membership；未发现后续独立 q 输入。因此该 q 公式是 v1 的数学解码性质，不是 Select5 的另一个运行时传输边界。

S18 的历史功能接收覆盖的是 v1 S15 路径，不覆盖 S24 v2。S24 新增了 v1 稳定键/局部商的有限环回归，但没有把测试当论文定理或完整安全证明。

## 4. v2 零和刷新：正确性、q′ 断点和输出功能

### 4.1 完整 priority key 保持不变

对每条 padded record，T 生成：

```text
rho_0 ← U(R_c)
rho_1 = −rho_0 mod 2^c
Q'_b = Q_b + rho_b mod 2^c
```

于是逐项：

```text
Q'_0 + Q'_1 = Q_0 + Q_1 + rho_0 + rho_1 = κ_i mod 2^c.
```

这对随机 tape 的所有值都保持模环正确性；没有 float、饱和、取整或 score 编码变化。score 和 original index 的稳定次序、dummy 次序以及后续 full-key DCF/Select/membership 的明文功能不变。单方 `Q'_b` 在 ideal uniform rho 下是 R_c 均匀，但两方份额仍由 `Q'_1=κ−Q'_0` 相关；这不是独立 share pair。

### 4.2 刷新后的逐方右移不再可组合

令 `Q'_b=2^h q'_b+r'_b`，重构高部时有 `γ=floor((r'_0+r'_1)/2^h)`。v1 固定 γ=0；v2 的均匀低位可能 γ=1。因此：

```text
q'_0+q'_1 = floor((Q'_0+Q'_1)/2^h) − γ mod 2^33.
```

最小反例取 `h=1,c=34,Q_0=Q_1=0,rho_0=1,rho_1=2^34−1`。完整键之和仍为 0；但 `Q'_0>>1=0`、`Q'_1>>1=2^33−1`，两个局部 q′ 相加不是 0。此为确定性代数不兼容，不是安全攻击，也不是统计失败。S24 不通过额外公开 carry、rank 或 plaintext 的方式“修复”。如果未来某个接口真的要求 q，需单独设计新 ABI，例如在高位环 `R_33` 中刷新 q、再固定低位 residue `i/0` 重组完整键；这会改变输出份额结构、材料和证明，不在本轮实现。

### 4.3 v2 输出条件模拟（条件证明，不是独立签核）

定义 v2 score-conversion 功能的输出为双方私有完整键 share `Q'_b∈R_c^N`，满足逐槽和为 κ；功能内部以独立均匀 `rho_0` 刷新，令 `rho_1=−rho_0`。本方理想输出是自己的 `Q'_b`；L 是项目已批准的 masked score-conversion/forward-shuffle 字段，不含对方输入或完整 mask。T 的材料生成视为功能内部 preprocessing，不交给 simulator。

对 b=0 和 b=1 分别令 `V_b` 为 S22 范围内的本方 carry/sign pre-output 本地 view（本方输入 share、本地 carry/sign materials 与 masks、局部 Eval 结果、phase4/5 masked frames、本地 Q_b）。在理想 fresh joint tape 假设下，ρ_b 与 V_b 条件独立且均匀；P1 的 `−rho_0` 同样是均匀边缘。对任意有正概率的 `(x_b,L)`、任意 v 与 `q'`：

```text
Pr[V_b=v, Q'_b=q' | x_b,L]
  = Pr[V_b=v | x_b,L] · Pr[rho_b=q'−Q_b(v) | V_b=v,x_b,L]
  = Pr[V_b=v | x_b,L] / 2^(cN).
```

所以 `V_b ⟂ Q'_b | (x_b,L)`，且 `Pr[V_b=v | x_b,Q'_b=q',L]=Pr[V_b=v | x_b,L]`。逐方 simulator：

1. 采样与 S22 carry/sign pre-output sampler 同分布的 `V_b`，其中含该 sampler 生成的 Q_b；
2. 取输入给定的理想完整键 share `Q_b^*`，计算本地 refresh share `rho_b=Q_b^*−Q_b mod2^c`；
3. 用 `Q_b^*` 作为本地完整 priority-key output，继续只模拟 S22/L 覆盖的 score-conversion frames。

该步骤没有把两次同 key Eval 当作独立调用；它把“旧视图到 Q_b 的全部相关性”留在单个 V_b 样本中，再用独立一次性 rho 打断输出 share 与 V_b 的条件相关性。若真实刷新 tape 与旧材料/OS 输出相关，此等式不再由代数保证。真实实现用 `getrandom` 字节生成 rho、mask/share，DCF roots 使用 OpenSSL `RAND_bytes`；二者联合计算优势 `epsilon_RNG_joint^(v2)` 未量化。S22 的候选 `68·epsilon_G + epsilon_RNG_joint` 不能不加说明地原样套用；若把 carry 与 sign 的两次 34 项 hybrid 分别应用到每个 padded record，朴素候选项会是 `68·padded_n·epsilon_G + epsilon_RNG_joint^(v2)`，但 S22 对 34 项的向量/逐记录量词及总查询数需要非作者重新核准，故 S24 不把该式登记为已证明界。v2 至少需要以覆盖 rho 与全部 party-visible 辅助材料的新版 `epsilon_RNG_joint^(v2)` 重陈。继承的单 key 项 `epsilon_G ≤ Adv_AES128^PRP(T′,q=5)+10/2^128` 中 `T′`/实际优势未量化。实际 TLS channel 项 `epsilon_TLS` 也没有安全量化。

### 4.4 `q′ → key_records′ → forward shuffle` 的有界延伸

v2 下 `q'_b=Q'_b>>h` 是本地 Q′ 的确定函数，因此给定理想 Q′ 可以计算它并放入本方 view；但 §4.2 的低位反例表明它不再是可重构高位，故 runtime 不把它作为 Select 输入。实际后继记录是

```text
key_records'_b[i] = (Q'_b[i], 0, 0).
```

给定理想 Q′ 后这是确定映射。S23 对“固定任意本方 Q_b 的 M2CM/v1 forward-only 子视图”给出过理想独立 Dealer tape 下的逐方 sampler：它按本方 shuffle material 生成本地 `sigma/tau/a/e/r` 状态，借获准 L 中收到的 masked frame 与 masked aggregate `Y` 补齐 peer-visible/transcript 量。把固定输入从 Q_b 换成 Q′ 不改变该 shuffle 方程；若 scoreconv rho tape、shuffle tape 和本方其他所需随机量在 ideal joint tape 下相互独立，且 L 包含 S23 sampler 所需的获准 masked values，可在上面 simulator 后串接这个 sampler。P0、P1 均适用。

因此，`q′/key_records′` 的确定映射和 M2CM forward-shuffle 本地 view 可到 **CONDITIONAL**，假设是 S22/S23 各自局部 sampler、批准 L 包含相应 masked shuffle frame、以及新版 joint RNG idealization。此结论不模拟完整 party package 的其他未用/已用 DCF pool，也不通过 S23 之外的非作者复核。沿消息链的下一个明确 OPEN 点是 forward shuffle 后第一条 sample-selected/adaptive DCF comparison：其 opened bit、同一 key 的两次 Eval、related slot pool 与未来访问地址需要条件联合模拟。故不能把前向 shuffle 子引理推广到五迭代 Select。

这个组合 sampler **只延伸到 M2CM forward 子协议**，不模拟所有 Select DCF slots、未使用材料、后续公开比较位导致的自适应访问、inverse shuffle 的共同置换状态、membership、完整输出 mask share、PREPARE/COMMIT、restart 或 crash。故 `F_SCORECONV_OUTPUT_VIEW=CONDITIONAL/UNQUANTIFIED`、`FORWARD_SHUFFLE_LOCAL_VIEW=CONDITIONAL`，整协议 `JOINT_PARTY_VIEW=OPEN`。S24 执行者参与 S19–S23；上式是可复核的设计推导，不是满足要求的非作者密码学签核。

## 5. 版本化运行时和材料成本

### 5.1 接口、材料绑定与默认关闭

新增 `ProtocolIScoreShareAbi::{S15InducedV1,FullKeyZeroSumRefreshV2}`。v2 只能和 BCG21 v1 DCF candidate 组合；旧 v1 package 字节布局不变。在线 phase-3 binding 继续使用原有 M6B5 header/version 字段（v2 固定为 version 5），并在既有两个 ABI/reserved 字节中将 score-share ABI marker 置为 `2,0`；帧长度不增加。该值与 scores 无关，是双方配置的公开版本参数；不公开 rho、Q、score 或 q，也不新增数据相关泄露或消息 phase。rho 只在 T 的离线 package/store 内，不在线发送。报告将这个 version marker 明确登记为本次 binding transcript 的 public configuration 字段，而不是隐含为秘密安全结论。

monolithic v2 package header 使用 M6B5/v4；在 padded N 条记录外新增 8-byte count 和每条一个 8-byte `rho_b`。N=128 精确增加 `8+8·128=1,032 B/party`；这是 serializer 结构推导，不是时间/性能测量。package 内 u64 big-endian，运行时仅接收 `rho_b∈R_c`。

streamed v2 使用 M6S2/v3；旧 slot count `5·C(N,2)+13N` 增加 N 个 purpose=5、每 padded record 一个、方向/子层固定的刷新 slot；v2 N=256 因而新增 256 个有 session、party、purpose、端点绑定和 one-shot lease 的 8-byte slots，即 2,048 B/party payload。每个 manifest entry 的固定描述为 76 B（6×u32 address + segment u32 + offset/length 两个 u64 + SHA-256 32 B），新增描述布局为 `76·256=19,456 B/party`；payload+描述 21,504 B/party 是 schema 推导，不含文件系统 inode、lease file、block allocation 等。

N=256 v2 E2E 实际 function-run 观测：`staged_store_bytes=385,331,840`，两 party store 各 192,665,920 B，manifest 各 12,675,648 B，11 segments。它只描述测试 store 布局，不是正式容量/峰值 RSS 或网络性能结果。N=128 v2 E2E package 每方 44,986,628 B；刷新额外 1,032 B 来自格式精确结构推导。完整生产峰值内存、磁盘预分配与跨主机成本 `NOT_MEASURED`。

### 5.2 E2E 确认的错误与修复

首次 v2 E2E 失败：ready marker 发布端只允许 M6B5/v2 与 v3，拒绝新 M6B5/v4 header。失败在 v2 dealer-ready 阶段，可复现；不是 oracle 期望问题。最小修复使 `publish_material_ready` 对照 `(dcf_abi,score_share_abi)` 求得的确切 package version，再跑 E2E 通过。旧格式拒绝/兼容由定向测试覆盖。首个失败日志仍在外部 append-only evidence，不覆盖。

## 6. 功能验证、命令与失败记录

### 6.1 构建环境/命令

构建在 WSL Ubuntu 20.04 的外部目录完成；工作树无 build output。编译器 GCC/G++ 9.4.0，CMake 3.31.6，OpenSSL 1.1.1f，Release，Generator Unix Makefiles。启用配置：

```sh
cmake -S VFSS -B /mnt/c/Users/28641/.codex/tmp/m6b_s24_on -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build /mnt/c/Users/28641/.codex/tmp/m6b_s24_on -j2
ctest --test-dir /mnt/c/Users/28641/.codex/tmp/m6b_s24_on --output-on-failure \
  -R 'moe_topk_score_key_refresh_v2_test|moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test|moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test'
/mnt/c/Users/28641/.codex/tmp/m6b_s24_on/moe_topk_streamed_material_slot_store_test
```

ON E2E：

```sh
wsl -d Ubuntu-20.04 -u root -- bash -lc \
  'cd /mnt/c/Users/28641/.codex/tmp/m6b_s24_on && ./moe_topk_project_derived_select5_process_e2e_test --score-share-v2-smoke'
```

root 仅由 TEST_ONLY supervisor 用于按 party uid 创建隔离目录和进程；运行中的 P0/P1 降权为独立 OS 用户。没有 root party 被当作 party isolation 证据。

OFF 配置：

```sh
cmake -S VFSS -B /mnt/c/Users/28641/.codex/tmp/m6b_s24_off -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF
cmake --build /mnt/c/Users/28641/.codex/tmp/m6b_s24_off --target sytorch -j2
nm -C /mnt/c/Users/28641/.codex/tmp/m6b_s24_off/libsytorch.a | grep protocol_i_refresh_full_priority_key_shares_v2
cmake --build /mnt/c/Users/28641/.codex/tmp/m6b_s24_off --target help | grep score_key_refresh_v2
```

最后两条 grep 无输出并通过否定检查；OFF 静态库缺 helper 符号且没有 v2 单元测试 target；ON 静态库存在 helper，ON E2E 能链接并执行。

### 6.2 定向结果

| 目标 | 结果 | 证据边界 |
|---|---|---|
| DCF_BCG21 Figure1 v1 conformance | PASS | 原语实现无改动；用于验证刷新仍接收该候选输出。 |
| Select5 controller conformance | PASS | 含 v1/v2 ABI binding 与 full-key path。 |
| Select5 material-store conformance | PASS | slot schema/lease。 |
| 新 `score_key_refresh_v2_test` | PASS | h=1…8、模边界、signed 极值/重复 score 稳定 key、rho=0/1/max/max−1、v1 q、v2 zero-sum、local-q counterexample、package roundtrip/truncation/downgrade。 |
| M2 uCMP、score input、parallel shuffle conformance | PASS | 3/3。 |
| Streamed material slot store | PASS | segmented stream、slot binding、durable one-shot、crash、race、integrity。 |
| feature ON/OFF | PASS | OFF symbol/target 缺失，ON symbol/runtime 存在。 |
| v2 T/P0/P1 E2E | PASS（7 successful cases） | 每例均由 TEST_ONLY supervisor XOR 合并双方 shares；独立 stable oracle逐位相等，原序、长度、0/1 和 weight=K 均核验，one-shot replay 拒绝。 |
| v2 错包/损坏 | PASS（所测范围） | 单元测试拒绝越界 rho、长度错、截断包、downgrade package version；store test 覆盖 lease/replay。专门的 v2 错 party/session E2E `NOT_RUN`。 |
| TLS13 独立 conformance | `NOT_RUN`（S24 targeted） | E2E 实际走 TLS；没有单独重跑 TLS suite，因为代码未改 TLS。 |
| 攻击/密码学实验 | `NOT_RUN` | 功能 differential 不是隐私证明。 |

成功 E2E case id：500 v1 N=7 K=3；501/502 v2 N=7 K=3（不同材料/运行）；504 v2 N=7 K=1；505 v2 N=7 K=7；507 v2 N=128 K=64（M6B5/v4）；503 v2 N=256 K=128（M6S2/v3）。logical N=7 时 padded N=8，覆盖 dummy。共同 fixture 含 signed Q20.12 边界 `INT32_MIN`, `INT32_MAX`, `-1`, `0` 与重复分值；T/P0/P1 成功输出逐位 oracle PASS。v1/v2 的 N=7,K=3 分别对同一个冻结 stable oracle 和同一 score fixture 校验，不以输出 shares 相等作为标准。

诊断计数（仅测试执行轨迹，不是性能）：N=128/K=64 case507 为 2,133 比较调用、9 个明文比较依赖层、该测试 harness 记录 32 个在线 exchange；N=256/K=128 case503 为 16,818 调用、9 层、32 exchanges。算法五迭代、最多九层和该次实际消息计数是三个不同概念；32 只适用于该 harness case，不是所有输入的通用协议轮公式。

失败记录：① 首次 Ninja generator 配置失败（本机无 Ninja），改用 Unix Makefiles；② 初次 CTest 在全部测试目标构建前运行，两个目标为 Not Run，构建所有目标后同命令最终 7/7 PASS；③ 首次 v2 ready publisher 拒绝 v4 package，修复后最终 E2E PASS；④ S24 最后一次直接运行错用了不存在的 executable 名，退出 127，按 CMake 实际 target `moe_topk_project_derived_select5_process_e2e_test` 构建并重跑成功。所有失败/成功输出均在外部 evidence 的唯一 S24 run 目录中保留。

## 7. 门禁边界与安全结论

### 条件通过的范围

在 ideal joint independent tape、S22 的 carry/sign local pre-output sampler 与 fixed-parameter BCG+21 DCF key bridge 均适用的条件下，F_scoreconv/v2 的**单方、score-conversion 前置局部视图**在给定本方输入 share、输出 share Q′ 和已批准 L 时具有上述精确 simulator。新增 rho 取值应纳入 joint tape 的 query/运行时间统计；现有 `epsilon_RNG_joint` 不可不经扩展沿用。`epsilon_G ≤ Adv_AES128^PRP(T′,q=5)+10/2^128` 和 `T′` 是 S22 的条件论证对象；本轮没有给出量化优势或安全位数。

### 仍 OPEN 的范围

- N=128 M6B5/v4 与 N=256 M6S2/v3 是不同材料 schema；S24 只核验新增 refresh material 的格式/调用，不重算两个 schema 的全体 used/unused/rejected slot 和失败/crash 访问轨迹。
- DCF key 单次、同 key 两次 Eval、相关 key/mask pool、公开比较 bit 后自适应地址；刷新不能替代这些证明。
- M2CM/v1 共同 Pi/Pi⁻¹、forward/inverse shuffle、membership、最终 original-order output share 和它与 scoreconv/Select 的联合分布没有模拟。
- 错误/崩溃合同仅沿用历史指定用例 `no-complete-mask`；可能有单方孤立 8-byte XOR share。不是原子无输出/all-or-nothing。S24 没有重跑 crash prefix。
- TLS channel 的可组合优势、getrandom 与 OpenSSL RAND_bytes 的联合优势、实际 AES hybrid 的 T′/查询界未量化。
- 完整 raw-score→mask 消息 DAG不因五算法迭代/九比较层而自动确定；本轮观测的 32 exchanges 是特定功能 E2E 计数。其他参数/case 全 DAG `NOT_CLOSED`。

因此没有攻击结论，也没有整协议安全通过。当前可称“PROJECT_DERIVED 五迭代 Select 的版本化实验实现，经所列范围的明文/多进程功能 oracle 验证”；不可称 BB90、BMW16、secure、恶意安全、生产安全或正式性能已通过。

## 8. 复跑与交接

详细 source hash、ON/OFF build configuration、二进制 SHA-256、外部 evidence log SHA-256、S24 与 S15 的差异文件列表见配套 `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S24_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-17.json`。machine-readable gate 见配套 `...S24_GATES_2026-10-17.json`。case summary 见配套 `...S24_E2E_CASES_2026-10-17.json`。

最终 `git diff --check` 通过；`VFSS-baseline/` 无差异；密钥、TLS证书、材料包、build outputs 和原始大日志均不进入仓库差异。未提交、未推送、未合并、未更新 PR。
