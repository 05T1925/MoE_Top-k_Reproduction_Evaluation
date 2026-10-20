# M6B-I-PROJECT-SELECT-5ITER-S4：Protocol I 实验候选复核与功能报告

日期：2026-10-12  
路线身份：`PROJECT_DERIVED_SELECT_5ITER`  
组合标签：`Protocol_I_PROJECT_DERIVED_SELECT_5ITER_DCF_EXPERIMENTAL`  
裁决：**有界 Protocol I 功能候选通过独立进程 oracle E2E；联合视图安全、耐重启防重领、mTLS 和正式性能仍未闭合。不得作为 secure 路径或正式性能结果。**

## 1. 工作区身份与接收范围

本报告在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation` 完成。分支为 `codex/m6b-project-derived-5iter-s4`；`HEAD`、本地 `main`、`origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`。S4 修改未提交。该 checkout 位于 main 同一基础提交上，S4 代码与文档在隔离分支内。

S1/S2/S3 的报告、决策、脚本和测试材料在开始时作为既有未提交/未跟踪工作区内容存在；它们不在上述 HEAD commit 中。没有把报告路径当作已合入 main 的证据，也没有覆盖或删除它们。S3 报告 SHA-256 为 `98f23eeef73f4255e58ca5834fbf560eaecdb5a394fb5d1616198b6a9e13bbc4c`；S3 工具哈希与报告登记一致：`public_view_probe.py` `37d06d0367fcc484492e44218c4d0507cbe88e36860188fa53038939ffe48e18`、`material_budget.py` `e14c52f389fc426637e2d54a41f561c7cccb46b59128c17677a012e20b6f6218`、`membership_direction_probe.py` `76a27dd56fb7f3e7fdf42914ee7cfc9f4727de427e41e59db2c5571a79d1c912`。

本轮对 S3 是源码、工具与报告哈希复核，不是异会话独立接收；`S3_RECEIPT=PASS_WITH_SCOPE`。S3 当时因其 `L_S3` 不含比较 bit 与这些 `z` 值而作出的 NO-GO，在其原始合同下成立；本轮按新确认的泄露规则修订路线合同，不改写 S3 历史。S1 的 BB90 来源 NO-GO 和 S2 的明文 Stage A 证据也继续保留。S34 仍是非 main 分支 schema-v3 技术诊断、非异会话接收；V3 仍 OPEN，正式 V4 仍 NOT_ALLOWED/NOT_RUN。BMW16-derived Select 是独立候选，本轮未复用其算法或安全结论。

`git diff --check` 通过；`git diff --quiet -- VFSS-baseline` 通过（冻结基线无差异）。已有 `docs/PAPERS.sha256` 差异在本轮开始时已存在，S4 未编辑该文件。未改本地论文或参考工程，未将材料、密钥、证书、构建产物或原始大日志加入仓库。

## 2. 项目派生五迭代控制器复核

实现身份仍是 S1 定义的 `PROJECT_DERIVED_SELECT_5ITER`，与 BB90 1990、2024 论文所称版本、BMW16 2016 和 BMW16-derived 项目候选都不同。控制器源码为 `VFSS/src/moe_topk/project_derived_select5.cpp`，公开控制 API 为 `VFSS/include/moe_topk/project_derived_select5.h`。输入控制域使用打乱位置 `0..N-1`；比较回调对冻结边返回 `1{key[left]<key[right]}`。

对候选集 `C`（大小 `m`）和一基 residual rank `r`，前四次收窄使用源码中的参数：

```text
s = min(m, ceil(sqrt(m)))
L = bit_length(N)
w = ceil(sqrt(2*s*L))
mu_floor = floor(s*(r-1)/m)
mu_ceil  = ceil(s*(r-1)/m)
q_low  = clamp(mu_floor-w, 1, s-1)
q_high = clamp(max(q_low+1, mu_ceil+w+1), q_low+1, s)
```

每轮按公开 seed 对 `C` 作均匀无放回抽样；样本大小 `s`。先冻结样本团 `C(s,2)`，其比较位唯一确定样本顺序与 `q_low/q_high` pivots。再冻结两个 pivot 到 `C\sample` 的星边，总数 `2(m-s)`。两层端点集合不相交：第一层边两端都在样本中，第二层恰有一个样本 pivot、另一个非样本候选；两 pivot 之间的边已在样本团中。故同一迭代每个无序 endpoint pair 至多一次。边槽以 `(session, algorithm/version, iteration, purpose, substage-scope, direction, left, right)` 定位；不跨迭代复用。

两 pivot 的精确 rank 可由样本团与其星边直接计数。若目标 rank 命中 pivot 则选中；落在二者之间时保留开区间并减去低 pivot rank；落在低端或高端时保留相应严格前/后集合。候选集和 residual rank 的每步更新只使用此前获批的 endpoint、比较 bit 与公开 seed。第五次对 residual candidate 冻结完整团 `C(m_5,2)`，从每个点相对其它点的胜出数恢复精确总序并选第 `r_5` 项。若先前已选中或候选成为 singleton，后续 trace 仍记五次算法迭代，但不再生成比较边。实现中的异常状态抛出，由实验 party wrapper 统一 no-mask 失败处理。

随机 seed / 样本仅影响调用量：每个有效输入、每个合法 seed tape 都由 rank 不变量与第五层精确比较得到稳定第 K 项；不存在“随机算法选错后视作可接受失败”的分支。错误材料、通信失败或内部不变量失败均不产出 mask。S2 在理想 fresh uniform-without-replacement 模型下的项目期望成本推导可以继续作为成本来源；它不是此轮 E2E 样本均值、尾界、BB90 保证或安全定理。最坏成本仍按 S2 的 `12N + C(N,2)` 上界，为 `O(N²)`；不声称最坏线性。

## 3. 稳定 score、membership 与原序 mask

raw input 沿用现有 `protocol_i_raw_score_input_party()`。对 raw signed Q20.12 的 32-bit 两补码表示，数值越大 priority 越高；项目 key 为：

```text
ordered = raw_score XOR 0x80000000
high = UINT32_MAX - ordered
stable_key = (high << index_bits) | original_index
```

因此越大的 signed score 编成越小的 key；同分以较小 `original_index` 排前。dummy score 为 `INT32_MIN`、index 位于 logical `n` 之后，故排在所有合法输入后。对于任意 `1≤K≤n`，选择 `K`-th smallest stable key 为 `tau`；membership 使用现有 strict-`<` ABI 的精确方向：

```text
selected_i = 1 - strict_lt(tau, key_i) = 1{key_i <= tau}
```

stable key 唯一，所以 padded 域 membership 恰包含 `K` 个 logical item；dummy 不入选。DCF membership 结果保持 additive shares，低位分别投影成 XOR shares 后以逆置换恢复原序。secure runtime 不重构 `tau`、index 或 mask。明文 oracle 重构只在三进程 E2E 控制器中进行，且该控制器在 T/P0/P1 fork 后才创建分数与输入 shares。

## 4. 新确认泄露函数 `L_S4` 与控制流闭合

下表范围是两个协议 parties P0/P1 可见字段。获准值不等同于其 joint-view 安全已证明。

| 字段 | 产生方 → 接收方 | 坐标/域 | 掩码与时机 | 可推导内容 |
|---|---|---|---|---|
| raw-score 转换 `z_carry/z_sign` | P0/P1 → 对方 | 原序 padded slot；各 34-bit ring operand | 每 slot、每 stage 新鲜 additive mask share；uCMP Eval 前合并成公开 masked operand | 公开 masked operand；不单独解出 raw score |
| shuffle round-2 `public_masked_records` | P0/P1 本地形成，双方持有 | shuffle 坐标；word0 40-bit key lane、word1/2 64-bit carrier lanes | `Pi(input_share)+r0+r1`；三个 record lanes 由 fresh random additive shares 掩码 | 公开 masked record；不公开 `Pi` 或 unmasked record |
| Select `z_left/z_right` | P0/P1 → 对方 | 打乱端点 `(left,right)`；40-bit key ring (N=128) | 本次 edge 绑定的两个 fresh masks；两层传输中先打开合成 masked operands | 当前被比较两端的公开 masked 值；不公开 unmasked stable keys |
| 比较结果 bit | P0/P1 的 DCF shares → 对方 | 每个获准打乱无序 edge 一个 bit | 第二个比较 wave message 交换各方 output share 后加法重构为公开 bit | edge 相对次序；可推导 sample order、pivot ranks、candidate、residual rank 和后续图 |
| package binding (schema/material ID/seed/shape) | P0/P1 → 对方 | 不含输入域坐标 | raw-score conversion 前各交换一次 40-byte metadata；session/fingerprint/N/K/bits 同时由 frame header 绑定 | 双方核对相同 package version、material base、public sample seed 和维度；不增加输入相关信息 |
| endpoint、pivot、slot 地址、计数 | 双方一致生成/使用 | 全部在打乱坐标 | 新增 runtime 不单独发送 rank/pivot 明文值；由控制器从 seed 与获批 bit 确定 | 端点并集非 singleton 时给出 active candidate 坐标集合；下一 slot 顺序可计算 |
| membership `z_tau/z_i` | P0/P1 → 对方 | `(virtual_tau, shuffled_i)`；40-bit | `tau`、每个 `key_i` 的 shares 分别加本 slot 新鲜 mask；mask 与 slot DCF 绑定 | 公开 masked operands；membership DCF 输出不公开 |
| communication length / uniform status | 双方 | framing 与状态 barrier | 每个实际比较依赖层有两个 framed exchanges；统一 ABORT byte 为 0，成功 byte 为 1 | 调用数、活跃层数、阶段成功/失败 |

mask 生成实际代码在 `protocol_i_project_derived_select5.cpp`：每个 operand 的 full mask 由 `getrandom()` 取 64-bit 后裁到 `2^b` ring；P0 share 独立均匀采样，P1 share 为 `full_mask-share0 mod 2^b`。left/right、每个 edge、每个迭代、membership slot、raw-score slot/stage 均新抽 mask；没有把 mask share 或 key 跨调用复用。DCF key 对应 `alpha=(mask_left-mask_right) mod 2^b`；因此 DCF key shares、公开 `z` 和被比较秘密输入之间存在协议相关性，单值均匀边际不能替代 joint-view 模拟。

公开控制流不需要新增 rank secret mux：

1. sample clique bits 唯一确定 sample order；公开 seed 与该 order 唯一确定两个 pivot 坐标。
2. pivot-star bits 联同 clique bits 唯一确定两个 pivot 的 exact candidate rank。
3. 公开 `(candidate, residual rank)` 按同一源码分支规则更新；接下来每个 edge/purpose/slot 均可由双方独立计算。
4. 第五层 clique bits 唯一确定目标打乱坐标。它不是原始下标，也不额外发送阈值 key。

这闭合了 S3 的“隐藏比较结果导致无法公开更新”阻塞。代价是双方按每个获批边公开相对序，以及其可导出的 pivot/candidate/residual/后续访问顺序；本项目对该自适应 transcript 的 simulator 尚未建立。T 是可信离线预处理方，发材后退出，不收到 scores、rank、pivot key、`tau` 或在线输出；T 在生成 shuffle correlations 时知道随机 `Pi` 和 masks。P0/P1 的 party messages 不含 `Pi`，但本实验没有为新的 T trust model 证明 T/P0/P1 joint view。若“映射保密”也要求对可信 T 成立，则当前 C-instantiation shuffle 不能满足该强化要求，需替换为 T 不获知合成映射的 preprocessing。

## 5. N≤128 round-aware 材料 ABI 与精确序列化预算

新 package magic/version 为 `M6B5/v1`，固定 `N≤128`；比较槽含 `iteration,purpose,substage_scope,direction,left,right`、两份 fresh mask shares 和一个 `M2UC` party DCF。两个 Select 子层在同一迭代不重边，故使用五个 iteration-scoped pool，每个 `C(N,2)`；实际访问时再验证当前 layer 归属与同一 package 内 slot one-shot。membership 使用独立 `iteration=5,purpose=membership,substage=4,direction=tau<item, virtual endpoint=UINT32_MAX` 命名空间。

`ProtocolIUcmpPartyMaterial` 序列化长度为 `D(b)=57+24b`；`D(40)=1,017 B`，`D(34)=873 B`。N=128、padded N=128 的每方实际 serializer 字节如下，来自三进程测试中的两个完整 party packages，而非估算：

| package 部分 | 每方字节 | 备注 |
|---|---:|---|
| outer M6B5 header/metadata | 52 | session、fingerprint、material base、sample seed、logical/padded N、K |
| 2N raw-score slots | 233,728 | 每 slot `D(34)+40B`，包括两次转换阶段 |
| 5C(N,2) Select pool | 42,956,488 | `8B count + 40,640×(D(40)+40B descriptor/masks)` |
| N membership slots | 135,304 | `8B count + 128×(D(40)+40B)` |
| forward + inverse `M2CP` | 16,682,992 | 每个 8,341,496B；各自含 C(N,2) 个未在此 runtime 消费的 shuffle-rank DCF key |
| 两个 nested blob length | 16 | 每个 shuffle blob 额外 8B |
| **每方完整 M6B5 package** | **60,008,580** | T/P0 与 T/P1 输出一致 |

`64 MiB=67,108,864B`，包限余量 `7,100,284B`；N=128 Select slot 数 `40,640`，低于当前 1,000,000-edge cap。`protocol_i_send_framed_chunks` 数据块为 `1,048,560B`，需 58 frames；若用该 framing 封装完整包，wrapper/header 增加 `58×64=3,712B`，合计 `60,012,292B/party`。E2E 实际使用本地测试 pipe 传递原始 serializer bytes，因此 chunk/TLS 线上发送量是按现有 serializer 精确推算，非观测 wire count。allocator/RSS/峰值、TLS record、mTLS 和远端 T 启动开销为 `NOT_MEASURED`；在线计数不包含 TLS。

每次 uCMP party Eval 内部调用两次 `evalDCF`。N=128 的全池理论槽容量对应五轮 Select 40,640、输入转换 256、membership 128；实际 E2E 只消费端点图调用数，不把全池容量写成 Eval 实测。两个 shuffle package 另带 16,256 个 rank DCF slot，当前两轮 shuffle path 不调用这些槽；它们仍占 package、属于未使用 key view 的安全复核范围。

防重领由 `ProtocolIUcmpPartyMaterial::used_`、Select runtime 的 `(session,fingerprint,material_id_base,party)` 进程级 claim set，以及每轮 edge consumed bitmap共同实现。三进程测试在一次成功后重新 deserialize 同包并再次调用，双方均拒绝且不产生第二个 mask。claim set 不跨进程重启持久化；材料池服务仍必须保证 session/material ID 不会跨重启重发，故 `MATERIAL_ABI` 的耐重启一次性合同为 conditional。

## 6. 消息 DAG、计量定义与 CA 关系

```text
raw signed Q20.12 shares
  → package metadata/schema/seed binding exchange
  → carry z exchange + local DCF
  → sign z exchange + local DCF
  → forward shuffle phase 1 → phase 2
  → for each nonempty frozen Select dependency layer j:
      local z_left/z_right shares exchange
      → local uCMP DCF Eval
      → output-share exchange / public comparison bit
      → public sample/pivot/rank/candidate update
  → batch membership z_tau/z_i exchange
  → local membership DCF Eval; shares stay secret
  → inverse shuffle phase 1 → phase 2
  → final bilateral success status; errors use uniform ABORT/no-mask
  → original-order XOR mask shares
```

| 轮数口径 | 数量 | 本轮解释 |
|---|---:|---|
| 算法 trace iterations | 5 | 四次 rank-safe shrink + 第五次 exact candidate closure；早停仍有五条 trace entry |
| 明文 comparator dependency layers | 最大 9 | 四次 sample clique/pivot stars 各 2 层 + final clique 1 层；实际依赖已冻结，不等于本次 run 的 active count |
| 通用 CA 条件式 | 若定理前提成立，`2×9+1=19` | 仅说明 conditional formula；本 runtime 不调用 CA，也未证明自适应公开比较图满足 compiler theorem |
| Protocol I 论文 core | 3 | 论文基线计数，不是本 candidate 的实现/安全结论 |
| 本 candidate 不含 raw-score adapter 的核心交换 | `7+2L` | package binding 1 + forward 2 + Select `2L` + membership 1 + inverse 2 + final status 1 |
| 完整 raw-score→原序 mask | `9+2L` | 在核心交换上加 carry/sign 输入转换 2；`L` 是实际非空 Select comparator layers |

本报告绑定的 N=128 K=2 E2E 样本 `L=2`、完整 13 个交换轮；K=8 样本 `L=8`、完整 25 个交换轮。它们不是固定轮数保证：该算法可到最多 9 层（完整最多 27 个 framed exchanges），seed/输入决定活跃图。CA 的 19 与 candidate runtime 13/25 计数含义不同，不能相加或互换。

N=128 E2E 精确 framed byte counter（每方 sent 与 peer received 对称；不含 TLS）：

| K | logical comparisons | active Select layers L | 完整交换轮 | binding | raw input | forward | Select | membership | inverse | final status | 每方 send=receive 总数 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 2 | 298 | 2 | 13 | 88 | 4,192 | 6,240 | 7,344 | 2,096 | 6,240 | 49 | 26,249B |
| 8 | 777 | 8 | 25 | 88 | 4,192 | 6,240 | 19,416 | 2,096 | 6,240 | 49 | 38,321B |

这些数字是固定本轮 fixture 和记录的公开 sample seed 的功能计数，不是速度、带宽性能、算法期望均值或正式 benchmark。正式 V4 LAN/WAN 矩阵未运行。

## 7. 原语、oracle、E2E 与失败路径

环境：Ubuntu 20.04.6 WSL、GCC 9.4.0、CMake 3.28.4、Unix Makefiles、Release；构建目录位于仓库外 `/home/lwt2024212936/vfss-m6b-s4-build-make`。运行目标和顺序：

```bash
cmake --build /home/lwt2024212936/vfss-m6b-s4-build-make --target moe_topk_project_derived_select5_conformance_test
cmake --build /home/lwt2024212936/vfss-m6b-s4-build-make --target moe_topk_project_derived_select5_process_e2e_test
cmake --build /home/lwt2024212936/vfss-m6b-s4-build-make --target moe_topk_m2_ucmp_conformance_test
cmake --build /home/lwt2024212936/vfss-m6b-s4-build-make --target moe_topk_m2_parallel_shuffle_conformance_test
./moe_topk_project_derived_select5_conformance_test
./moe_topk_project_derived_select5_process_e2e_test
./moe_topk_m2_ucmp_conformance_test
./moe_topk_m2_parallel_shuffle_conformance_test
```

结果：`PROJECT_DERIVED_SELECT_5ITER C++ control + DCF membership PASS`。控制测试全枚举 `n≤8` 所有 strict order/所有 K、不同算法 seed 和冻结边审计；membership DCF conformance 覆盖 `tau<key_i` complement 方向、重复/边界 raw 分数、padding 和恰 K mask。独立进程 E2E 在 T、P0、P1 三个 POSIX processes 中运行，覆盖 `(n,K)=(1,1),(7,1),(7,7),(128,2),(128,8)`；controller 对每项 XOR mask 与冻结 `top_k_mask()` oracle 比较，全部相等且重量恰 K。分数夹具含 `INT32_MIN/MAX`、正负边界、零、重复值和稳定 original-index tie break。

本次最终三进程测试记录（T 在生成包时以 OS CSPRNG 生成并公开 sample seed；T shuffle 与 DCF masks 同样由 OS CSPRNG 生成。每次执行的 seed/比较图可能变化；下表绑定本报告所列 executable hash 对应的一次运行）：

| padded/logical n | K | public sample seed | comparisons | active layers | exchange rounds | 结果 |
|---:|---:|---:|---:|---:|---:|---|
| 2/1 | 1 | 4180309495781720709 | 1 | 1 | 11 | oracle PASS |
| 8/7 | 1 | 18403234542365329511 | 14 | 3 | 15 | oracle PASS |
| 8/7 | 7 | 4916874122803212232 | 13 | 2 | 13 | oracle PASS |
| 128/128 | 2 | 11458164936844887143 | 298 | 2 | 13 | oracle PASS |
| 128/128 | 8 | 10547386099598296138 | 777 | 8 | 25 | oracle PASS |

另有三种有意失败的进程用例：P0 package magic 被篡改，导入前失败并通过公开 `protocol_i_project_derived_select5_abort_no_mask()` 与 P1 统一 no-mask；P0 package 内 sample seed 被改成另一个仍有效的值，package-binding exchange 在读入 raw-score 前发现两方 metadata 不一致并统一 abort；P1 关闭第一条在线输入通道后发送统一 ABORT，双方不返回 mask。成功用例结束后双方在同一进程重新导入同一个 `session/material_id`，package claim 拒绝第二次调用；测试只保留第一次 oracle mask，replay call 不返回 mask。跨重启 replay 未测且当前进程级 ledger 无法阻止，见上节。

过程中的开发失败均保留在本报告：首次编译因 shuffle helper 是另一翻译单元的匿名符号而失败，随后改用新 API 的 framed exchange；首个 E2E 计量断言把 uCMP 调用数错当底层 `evalDCF` 数，修正为每个 uCMP 两次 DCF Eval；随后又将 membership 比例按 logical n 而非 padded n 计算，修为 `2×padded_N`；一次 CMake 多 target 调用写法不被该 build driver 接受，随后逐目标构建。最终原语、控制、membership、五个正常 E2E 和三个失败 E2E 均通过，无被删除失败样本。

最终 executable SHA-256：

| binary | SHA-256 |
|---|---|
| `moe_topk_project_derived_select5_conformance_test` | `03dbcaa56026380fa5159062ba436985ebc2263acbad6cb4eda798e802727d45` |
| `moe_topk_project_derived_select5_process_e2e_test` | `75021c0f678c88fcfab188017dd9ee920cdf2444cee1806cb6f59afdc95308e5` |
| `moe_topk_m2_ucmp_conformance_test` | `b4ebc06bd734d9bcb9766e7c7d999653e7cb8a2d67cf09130567f698ba900304` |
| `moe_topk_m2_parallel_shuffle_conformance_test` | `61f87e10fabad170ef41dd2389e6929a349867e98764758104702d2fb6d7100a` |

## 8. 安全复用范围与未闭合义务

- 代码 ABI 确认为当前 `ProtocolIUcmpPartyMaterial`：`b=40`（Select/membership）、`Bout=64`、`groupSize=1`、函数严格方向由 `eval_strict_lt(z_left,z_right)` 给出；raw score adapter 是 `b=34`。方向、边界和 project-stable mapping 已由 DCF conformance 与 E2E 验证。该函数性核对不等同于新的密码学定理。
- 可复用到此处的只是相同 key ABI 下单次 DCF/uCMP 的原语合同。不能复用 BMW16-derived 的 whole-pool/adaptive-query/shuffle/output simulator，也不能复用 S31/S32 其他图的 joint-view 或计量通过结论。
- 新 L 明确公开比较 bits、endpoint/pivot、count/length 和 masked operands。仍缺：对 full adaptive transcript 与全池 used/unused DCF key material 的 P0/P1 simulator；shuffle public masked records 和 inverse routing 的联合模拟；T 对 `Pi`/mask 的受信任视图界定；ABORT/timeout/restart/篡改 material 与输出的组合证明；跨重启 one-shot durable ledger；当前 fd framing 到 mTLS/TLS transport 的 authenticated binding。
- 因此功能路径是 opt-in experimental API 和独立测试 target；没有 secure alias，没有接到默认/活动排序 pipeline，没有进入 `VFSS-baseline/`。不声称半诚实协议安全、恶意安全、无条件安全或生产安全。

## 9. 门禁结论

| Gate | S4 结论 | 依据 / 保留项 |
|---|---|---|
| `S3_RECEIPT` | `PASS_WITH_SCOPE` | 报告/工具 hash 与工作区核对；S3 artifacts 非 main commit、非异会话接收 |
| `LEAKAGE_L_MAPPING` | `PASS_FOR_P0_P1_WITH_TRUSTED_T_SCOPE` | 新增 bits/masked-z 入 L；T 知道随机 Pi，明确限于受信任离线 T |
| `MASKED_OPERAND_REVIEW` | `PASS_CODE_REVIEW_AND_FUNCTIONAL_CONFORMANCE` | 每类 z 的实际份额公式、mask ring 与 DCF alpha 绑定已核对；joint-view 仍 open |
| `PUBLIC_CONTROL_DERIVATION` | `PASS` | 获批 endpoints/bits/seed 可确定性重建 pivots、ranks、C、residual 与 slot 次序 |
| `MATERIAL_ABI` | `PASS_N128_SIZE / CONDITIONAL_REPLAY` | 实际包 60,008,580B <64MiB；同进程防重领通过，耐重启 ledger 未实现 |
| `DCF_MEMBERSHIP` | `PASS_BOUNDED` | C++ DCF conformance + 稳定 key、padding、exact-K oracle E2E |
| `MESSAGE_DAG` | `PASS_FUNCTIONAL / TLS_OPEN` | DAG 与实际 `9+2L` 交换计数一致；mTLS 未集成 |
| `FUNCTIONAL_E2E` | `PASS_BOUNDED` | T/P0/P1 independent-process fixtures 与三种 fail-closed 用例通过 |
| `JOINT_VIEW_SECURITY` | `OPEN` | 新图/泄露、全池材料、trusted-T view、shuffle/output/abort composition 未证明 |
| `FORMAL_PERFORMANCE_READY` | `NO` | 未运行 LAN/WAN；V3/V4 前置门不变；在线 bytes 只是本地功能计数 |

实现身份结论：`Protocol_I_PROJECT_DERIVED_SELECT_5ITER_DCF_EXPERIMENTAL` 仅表示明文正确性和有界协议功能候选，不是安全 Select、Protocol I 论文精确三轮实现、BB90 或 BMW16。进入正式安全审查前，先补完整 L simulator、T trust contract、跨重启 material lease/anti-replay、authenticated transport 与 failure composition；正式 LAN/WAN 仍须等 V3/V4 门。

## 10. S4 文件和源码身份

S4 源码 SHA-256：

| 文件 | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `340803c56c81765722ed04a836cbbb094e13235917ff09a96c1712cc8131103b` |
| `VFSS/include/moe_topk/project_derived_select5.h` | `19177983f603beddd2cc237d7ca9ee10cea8c49e7b8bed87f71afbaF2a90fe75` |
| `VFSS/src/moe_topk/project_derived_select5.cpp` | `ecfa56c3d12f8a8183db680d9197bcaf75a18761ba1a4e078a300ca273f2651e` |
| `VFSS/tests/moe_topk/project_derived_select5_control_test.cpp` | `56291adf3904670f26f53766c42d7353c0430a41cf618222c58af2aa0fcfa695` |
| `VFSS/include/moe_topk/protocol_i_project_derived_select5.h` | `174b3fdacde85101b6fc19ad74dd11f445df1030c4282067df64d0bcf6510356` |
| `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp` | `5d9e981a8613e6ef20ad31cf4ed73ae91dcb3b54f043bc515e79f262d512f6d0` |
| `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h` | `0843c6717885f12421b37fed1285d6216c800fb590a97ca0984aa1287254d476` |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `f5b767130bf6d93fde58151629ef6f209257b7a53daF30cd4843c20cfdc01010` |
| `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp` | `2f9ad122a47b6f4c13da195f0a1d818dd5003a388afa484f318e75e83a0b5b33` |

