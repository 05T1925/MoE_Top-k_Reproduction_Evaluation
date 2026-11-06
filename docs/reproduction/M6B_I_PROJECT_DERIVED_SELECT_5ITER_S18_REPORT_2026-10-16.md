# M6B-I-PROJECT-SELECT-5ITER-S18 独立接收与安全复核报告

日期：2026-10-16
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL
结论：S15 精确快照的独立功能与 default-OFF 接收通过（限本报告复跑范围）；DCF key-only 桥接为 CONDITIONAL；UCMP 条件 transcript、相关整池自适应访问及 shuffle/output 联合单方视图未闭合。未发现可复现攻击或运行时缺陷。

## 1. 身份、范围和独立性

本路线实现的是项目自行推导的稳定 Top-K 功能：signed Q20.12 score 降序、original_index 升序处理同分，输出回到原输入顺序且恰 K 个 1 的秘密共享 mask。它不是 BB90 五迭代算法复现或 BMW16 实现，不继承二者的算法步骤、复杂度、概率或安全结论；也不替代原六方案计划中的 BB90 项。

接收运行时对象是 S15 默认关闭修复后的 72 项源码清单。S15 task commit 为空、未合入 main。S18 使用原始 S15 worktree 的源码构建，而不是用 S17 更新后的 docs 拼接源码。S15 manifest SHA-256 为 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816，72 项 source canonical SHA-256 为 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。本报告配套 manifest 记载逐文件哈希、构建产物和证据日志。独立复核者将 72/72 文件逐一对照清单并重新计算 canonical hash。

S18 在独立 worktree C:\Users\28641\.codex\worktrees\m6b-project-select5-s18\MoE_Top-k_Reproduction_Evaluation，分支 codex/m6b-project-select5-s18，基于 HEAD c3926c68fd14f270faa8b55234311071947fa080 开展。任务起始时 main 与 origin/main 均为该 SHA。S15/S16/S17 都未合入该 checkout。桌面主 checkout 未被写入；测试构建写到 Windows Temp，源副本和短日志写到独立外部 evidence 目录 C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02。

独立接收者代理明确声明未参与 S4–S17 的实现、证明或决策。本轮满足“未参与 S15/S16”的接收条件。S17 接收者参与了 S15/S16，故 S17 的复跑仍是同会话证据，不能替代本次接收。S18 接收仅覆盖 S15 runtime 文件和下述定向复跑，不覆盖 S16/S17 文档增量，也不是完整复跑 S17 的 25-case JSON。

S16 manifest 声明 76 项 canonical 值 63208f02ba8f8b014df41be668d6435aaab29673adbd611f7fbfc1fecb738fbc。S17 worktree 对该 76 项的当前文件差异仅为后续更新的 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/REFERENCE_MANIFEST.md 三份文档；S15 原始 72 项及其构建源未受影响。S14 historical canonical erratum 按 S15/S17 记录保留，不用旧声明值替代当前 S15 文件身份。

## 2. 构建环境和精确复跑

环境为 WSL Ubuntu 20.04.6、GCC/G++ 9.4.0、OpenSSL 1.1.1f、bundled CMake/CTest 3.31.6；Release、BUILD_TESTING=ON、MOE_TOPK_ENABLE_EMP_OT=OFF。系统 CMake 3.16.3 未用于本次构建。独立构建目录：

- ON：C:\Users\28641\AppData\Local\Temp\codex-s18-m6b-s15-receipt\on
- OFF：C:\Users\28641\AppData\Local\Temp\codex-s18-m6b-s15-receipt\off

配置及构建命令：

~~~powershell
wsl.exe -d Ubuntu-20.04 -- bash -lc '/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-select5-s15/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/AppData/Local/Temp/codex-s18-m6b-s15-receipt/on -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON'
wsl.exe -d Ubuntu-20.04 -- bash -lc '/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake --build /mnt/c/Users/28641/AppData/Local/Temp/codex-s18-m6b-s15-receipt/on --target moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test moe_topk_project_derived_select5_conformance_test moe_topk_project_derived_select5_material_store_test moe_topk_project_derived_select5_process_e2e_test moe_topk_streamed_material_slot_store_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test -j2'
wsl.exe -d Ubuntu-20.04 -- bash -lc '/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-select5-s15/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/AppData/Local/Temp/codex-s18-m6b-s15-receipt/off -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF'
wsl.exe -d Ubuntu-20.04 -- bash -lc '/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake --build /mnt/c/Users/28641/AppData/Local/Temp/codex-s18-m6b-s15-receipt/off --target sytorch moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test -j2'
~~~

四条 configure/build 命令退出码均为 0。ON CTest 定向集合退出码 0、5/5 PASS：DCF_BCG21_FIG1_AES128_V1 conformance、Select5 control conformance、Select5 material store、旧 M2 uCMP、旧 M2 parallel shuffle。复跑命令使用 CTest 3.31.6：

~~~sh
ctest --test-dir /mnt/c/Users/28641/AppData/Local/Temp/codex-s18-m6b-s15-receipt/on -R '^(moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test|moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test)$' --output-on-failure
~~~

OFF CTest 退出码 0、2/2 PASS：旧 M2 uCMP 与 parallel shuffle。

default-OFF API 边界的三个负面检查均按预期拒绝：

- nm -C OFF/libsytorch.a 后 grep BCG21 API 名称：无匹配，grep 退出码 1（预期）。
- OFF CMake target help 后 grep BCG21/Select5：无匹配，grep 退出码 1（预期）。
- 无 opt-in 宏包含 BCG21 公共头的 C++ 探针：编译退出码 1，触发约定的 #error（预期）。

ON 的 DCF/Select5 客户端和 party API 由对应 conformance/E2E 目标实际链接运行。S18 没有另跑独立 ON 外部 API probe；该项单独标记 NOT_RUN。TLS 专项、streamed-store 独立 CTest、完整 25-case S17 E2E、全部 crash prefix、生产存储故障注入均未在 S18 复跑，不能由 S17 同会话结果冒充本轮独立证据。

## 3. 独立功能复核及失败/跳过记录

N=128 BCG21 monolithic 通过命令 moe_topk_project_derived_select5_process_e2e_test --bcg21-monolithic-n128：退出码 0，case 301、K=64、schema 为 M6B5/v3 + M2CM/v1 + M6P2/v1，Select comparison_calls=2148，ORACLE_PASS。

N=256 BCG21 streamed 通过 --bcg21-smoke：退出码 0，12/12 ORACLE_PASS；包括 N=256 K=1、2、8、128、255、256 的路径，N=128 K=1、64、128 和 N=1/2 边界。该 smoke 入口一次运行 12 个成功样本，没有逐案导出的 S18 JSON；逐案名单与汇总数字按独立接收者的运行摘要记录，不能声称复跑 S17 全部 25 行。

独立 TEST_ONLY supervisor 合并两方输出 shares，逐位检查冻结 C++ stable oracle，并验证原序、长度、0/1 值和 weight=K。party 进程只返回自己的 share；明文重构仅在测试 supervisor。

六个 BCG21 故障 case 通过 --bcg21-faults，退出码 0，case 201–206 均为 UNIFORM_ABORT_NO_MASK，覆盖 package 篡改、peer disconnect、错误 K binding、错误 TLS 身份、T 在 ready 前退出、sample seed magic 畸形。

独立运行 --crash-step 5 退出码 0，case 55 为 NO_COMPLETE_MASK：P0 pipe 0 bytes，P1 可有 8 bytes orphan XOR share。该 crash 入口使用 M2UCv1；它只作为既有合同抽查，不能记为 BCG21 专用 crash 证明。S17 历史上 step 1–6 的 6/6 crash-prefix 仍为同会话证据。

一次初始参数选择使用 --n256-k 128，运行的是默认 legacy M2UCv1；发现 ABI 与接收对象不符后，将结果排除并改用 --bcg21-smoke 完成 N=256 BCG21 接收。该修正是测试选择校正，不是算法失败。没有运行时修复；也没有发现需要保留的代码失败样本。随机 seed 按实际代码每活动迭代新取，case 301 与历史 S17 的同号 case 比较调用数不同（S18 2148，S17 1912），case 110 也不同（S18 17207，S17 5372）；这是不同公开采样 seed 导致的功能轨迹差异，不是时间性能、期望界或算法缺陷。

构建与二进制 SHA-256：

| 产物 | 大小 bytes | SHA-256 |
|---|---:|---|
| ON libsytorch.a | 2,606,006 | 8c8bd88124e23faec7f392f6816504684e544554ff7ac2c4c44d14e564a49ff8 |
| OFF libsytorch.a | 2,122,218 | d95e70a34c238a4e2b1f2d67dc66fefbb4573e3331ef2211cdab05113acd57bd |
| ON DCF conformance | 211,920 | 65cc80e3e5100632f148f4a6339185031fe638e64f0feb3bf5215c2feddc2303 |
| ON Select5 process E2E | 652,416 | 6d521461bac45ddbdaf2fab1fa0ed6a2bbfd6960330895834af0fe26e050aa73 |
| ON Select5 control | 312,616 | 754a9550dcc7b22b1e2816ce7772b915f77bf2fb06eb76bf48bc8d477e94385b |
| ON Select5 material store | 571,088 | 6d9ef511ba2ba067abab77791a5e5cce52f60fbbd823890e170b0669970776e3 |
| OFF M2 uCMP | 187,544 | 5cfb02d0f52f96d370657ff8cd7afc270b84026a6598b52ae7d25bbb85925599 |
| OFF M2 parallel shuffle | 309,760 | 963da34acc59ee8457bf90c5cdd165f629d65bedf93f30ccd5a1cfa4f048ad18 |

ON/OFF CTest 原始 LastTest.log 已复制至外部唯一 evidence 目录，哈希列在 source/build/evidence manifest。E2E 的原始 stdout 没有作为日志文件保存；本报告不伪称存在逐案原始日志。外部 Temp build 和日志均不进入 Git。

## 4. 威胁模型、泄露函数与真实字段

模型：离线 T 完全可信，生成双方材料后退出且不合谋；在线最多一方半诚实腐化；执行方间为私有认证通道。恶意 T、T+party 合谋、主动恶意在线方、被攻陷的 OS/root、管理员回滚本地存储均排除。物理墙钟泄露未测量。

记 party b 的完整视图为本方输入 share X_b、本方 package/manifest/segment/lease 状态、本方 DCF keys/mask shares、两次 shuffle local state、发收 TLS 帧、sample seed、从公开比较位导出的候选和访问轨迹、本方 membership carrier、输出 share、PREPARE/COMMIT/ABORT 以及超时/崩溃状态。模拟器不得输入真实 transcript、对方秘密或 test oracle。

获准泄露 L 的实际字段映射如下。静态源码与 S17 的同会话 wire-trace 证据没有显示额外显式 peer payload；S18 未复跑完整网络 trace，故对时序和未测 wall-clock 不作更强断言。

| 字段 | 产生和接收 | 视图分类 | L / 安全处理 |
|---|---|---|---|
| session、fingerprint、N/K、width、party、phase、per-channel sequence、material ID、DCF ABI | 双方 binding/frame header | peer-visible | 会话绑定、材料槽 ID、阶段和长度；不含密钥内容 |
| raw-score adapter 与比较用 masked operands | 双方按用途发帧 | peer-visible | 批准的 masked values；score、threshold、原始下标关联不发送 |
| forward/inverse Y、Q arrays | 本方 shuffle material 产生并发给 peer | peer-visible | 批准的带掩码数组及通信长度；Pi 与原序映射不发送 |
| σ、τ、a、e、r、本方 shares | T 离线生成、owner party 读取 | 本方/T/supervisor local；peer 不读另一方 store | 必须由模拟器按真实条件关系重建；party-scoped OS 文件不是 peer 消息 |
| ready digest、package manifest、segment digest、lease/claim 和 marker 文件 | T/store 与 owner party | owner party/T/supervisor local；没有发现发给 peer 的 package digest payload | local view 的一部分；已用/未用/拒绝槽不能从 view 论证中删除 |
| 活动迭代 sample seed | P0 在 forward shuffle 完成且轮次激活后生成，phase 40+i 发出，P1 校验并 ACK | 双方可见 | 已批准的当前迭代随机量；源代码将 sample generator 调用置于 forward complete 之后，未来轮 seed 不在 package/ready/start metadata |
| 当前 shuffled endpoints/pivot 位置、opened compare bits 和比较数量 | 由 seed、前轮公开 bit 与 controller 确定；对应层发 operand 和 DCF share | 双方可推导/可见 | 作为获准 L；没有额外发送 rank 或 pivot 明文值 |
| 两方局部 DCF/Eval shares | 每方本地执行；结果 share 发给 peer 并相加打开 comparison bit | 本方 key/Eval local，share message peer-visible | opened comparison bit 已获准；联合模拟须保持两次 Eval 共享同一个 key |
| membership operands / carrier | masked operands 发送；本地 membership share 输入 inverse shuffle | operands peer-visible，carrier 本地 | 公开带掩码值获准；membership bit、阈值、完整 mask 不直接发送 |
| PREPARE=1、COMMIT=2、ABORT=0 与 phase/长度 | 双方固定协议状态帧 | peer-visible | 映射到固定通信和统一 ABORT；crash prefix 要模拟 |
| output XOR share | inverse output 后本地返回给调用方 | 本方/TEST_ONLY supervisor；不直接发送完整 mask | 本方 ideal output share 是 simulator 输入；两方完整 mask 不由单方得到 |
| 墙钟时间、调度延迟 | OS/transport 外部表现 | 未测 | NOT_MEASURED；不声称 timing-side-channel 安全 |

Ready 文件中包含两方各自 package digest，但它们按源码写入各方独立本地目录；网络 binding 只含固定 schema/version、material ID、N/K、width 与 ABI，不包含 digest。当前复核未发现新增未获准的显式 peer 字段；无需提出新确认问题。

TLS frame header 还带有单调 sequence 计数器与双方认证身份。sequence 由本通道已观察到的帧顺序/数量确定，party identity 是私有认证通道用于身份绑定的字段；它们没有独立于获准 phase、比较次数、通信长度和状态之外的秘密载荷。模拟器应按 L 所确定的消息序列生成对应 sequence；网络墙钟/调度泄露仍 NOT_MEASURED。

## 5. 实际 DCF 与 UCMP 条件桥接

对象为 BCG+21 Figure 1 的 DCF，实际 ABI M2B1/v1，uCMP wrapper 在 34–53 位输入环与 Z/(2^64) 输出域运行。源代码路径为 VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp、protocol_i_ucmp.cpp 和 protocol_i_project_derived_select5.cpp。BCG DCF 与旧 M2UCv1 使用不同头、序列化与 ABI；本次没有改动任何一个版本。

实际映射：

1. KeyGen 用 OpenSSL RAND_bytes 分别取得两个 128-bit root seeds；根控制位为 (0,1)。
2. 每层 G 对同一 AES-128 key 调用五个互异固定 domain/counter block，串接 80 bytes，MSB-first 取前 514 位，解析成左右两个 128-bit seeds、两个 64-bit value 与控制位。
3. CW 包括 128-bit seed correction、64-bit ring correction、左右两个控制位；alpha 的路径位决定 keep/lose 分支；最终 correction 是 64-bit ring 元素。
4. Convert_G 取终端 seed 前 64 bit 并按 big-endian 解释为 Z/(2^64)。序列化键含 M2B1/version/party/width/length header、root、每层 CW 与终端 correction；长度为 57+25w bytes。非随机 header/reserved/control padding 字段不被描述为均匀随机字节。
5. uCMP 令 x=(z_left-z_right) mod 2^w，y=(x-2^(w-1)) mod 2^w；同一个本方 key 在 x、y 上各 Eval 一次，计算 ge=t2-t1 并按 party/wrap 分支改正，发送局部 share 后打开比较 bit。

具体 alpha 关系为 alpha=(mask_left-mask_right) mod 2^w、beta=1；两个 opened operands 满足 z_left=value_left+mask_left、z_right=value_right+mask_right，因此 x 减去 alpha 后才对应未掩码差值。在线腐化方只有自己的 mask shares 与 opened z，不直接持有完整 mask_left/mask_right；然而 S18 没有证明把 party key 和 Eval share 替换为定理模拟分布时，在本方 shares、z_left/z_right、同 key 的第二次 Eval 与 opened bit 条件下仍可使用同一优势界。

论文来源为 Boyle et al., Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation，Microsoft Research full-version PDF，DCF Figure 1 pp. 12–13、Theorem 2 p. 13、Appendix F.1 pp. 41–45，https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf。该 theorem/definition 支撑固定函数参数下的单方 party-key 隐私，不等于完整协议视图模拟。

按五块 AES 扩展的 PRP-to-PRF 逐块 switch，候选单次 G 优势为

~~~text
epsilon_G <= Adv_AES128^PRP(T_prime, q=5) + C(5,2)/2^128
           = Adv_AES128^PRP(T_prime, 5) + 10/2^128.
~~~

前提：五个输入块互异、AES-128 可按 PRP 建模，时间损失计入 T_prime，后续 MSB 截断为确定性后处理。宽度 w 的 key-only 树 hybrid 另有

~~~text
delta_w <= w * epsilon_G + epsilon_RNG_joint.
~~~

epsilon_RNG_joint 是 OS getrandom、OpenSSL RAND_bytes、调用顺序/请求长度、自适应停止与 party-visible 辅助信息的联合随机源优势占位；它没有本轮可核验的数值界或平台归约。不能称为统计独立、无条件均匀、已量化安全位数。此条件项不是 adversarially compromised OS/T 的安全保证。

源码调用族至少包括 dealer/预处理阶段的 Pi 与 sigma permutation、a/h/r 和每槽 mask 的随机抽取（含 rejection sampling 的变长请求）；每个 DCF key 的两次 16-byte RAND_bytes root 请求；以及在线 P0 在每个活动迭代内的 32-byte getrandom sample seed。seed 与 OS/OpenSSL 状态、同一运行中的先前调用、拒绝采样停止点和对 party 可见的 mask/seed 辅助信息需在一个联合区分实验中建模。S18 没有实例化该实验的对手优势或平台熵源下界。

门禁拆分：

- DCF_SINGLE_KEY_PRIVACY=CONDITIONAL，仅在论文构造与 M2B1/v1 字段映射、AES-128 PRP、上述联合随机源假设及参数范围成立时，支持 key-only projection；独立接收通过不等于独立证明通过。
- UCMP_SINGLE_CALL_VIEW=CONDITIONAL/OPEN：代码两个 Eval 共用一把 key；要从 theorem 的 key-only simulator 生成真实调用视图，还需证明给定本方 input share、本方 correlated masks、公开 z operands 与 opened bit 后的条件分布仍可替换。S17 给出候选而非完整游戏归约，S18 接收者也没有建立该条件引理。
- 该 theorem 不直接覆盖跨调用共享 mask/material auxiliaries、同池已用/未用槽、比较位选择后续地址或 shuffle/output transcript。

## 6. 材料池及自适应访问

分别记录两套 schema，不能将其当成同一个安全对象：

| N / schema | DCF 槽 | 位宽分布 | 独立功能样本 | 静态 key-only hybrid 系数 | 其余状态 |
|---|---:|---|---|---|---|
| 128 / M6B5/v3 + M2CM/v1 + M6P2/v1 | 41,024 | 40,768×40-bit + 256×34-bit | case 301：2,148 Select 调用，另有 3N raw conversion/membership DCF 槽 | 1,639,424 epsilon_G + epsilon_RNG_joint(schema) | shuffle metadata 1,280，另计 |
| 256 / M6S2/v2 | 163,968 | 163,456×41-bit + 512×34-bit | case 110：17,207 Select 调用，另有 3N raw conversion/membership DCF 槽 | 6,719,104 epsilon_G + epsilon_RNG_joint(schema) | shuffle metadata 2,560，另计 |

实际每个 sample case 的 used count = Select 比较调用 + 3N；其余槽仍在本方 package/store 中。S18 没有重跑完整 S17 case accounting，未活动/失败路径的消费数 NOT_MEASURED。两种 schema 的上述计数和 key-only 式只是静态 projection，不是 adaptive full-view 定理。

完整自适应模拟还需证明：对获准 seed 与已打开比较位确定的地址顺序，每次 hybrid 换 key 后 opened bits 条件分布保持一致；未用、被拒绝和崩溃留下的槽可按 schema 的本地方视图生成；durable one-shot lease/status 不引入未建模 peer 信息。S15/S17 未给出这一整池 transcript reduction，故 ADAPTIVE_POOL_VIEW=OPEN。

## 7. M2CM/v1 shuffle、逆路由和输出

实际 dealer 关系按源码为：

~~~text
Pi uniform permutation
sigma0,sigma1 independent uniform permutations
tau0 = Pi o inverse(sigma1); tau1 = Pi o inverse(sigma0)
a0,a1,h,r0,r1 independent uniform vectors over record group G^N
e0 = -tau0(a1)-h
e1 = -tau1(a0)+h
Y_b = sigma_b(X_b)+a_b
S_b = tau_b(Y_(1-b))+e_b
Q_b = S_b+r_b
Z = Q0+Q1 = Pi(X0+X1)+r0+r1
~~~

record group 为 Z/(2^w)×Z/(2^64)×Z/(2^64)，按代码 ring arithmetic。运行时对 forward 使用 Pi、对 inverse 使用 Pi^{-1} 调用两轮 shuffle metadata 生成逻辑，各方向均有单独的 sigma/tau/a/e/r 本地材料。N=128 使用 monolithic M6B5/v3 + M2CM/v1 + M6P2/v1；N=256 使用 streamed M6S2/v2，目录/segment/lease 实现和访问记录不同，必须分别模拟。

Select 的公开比较位和 seed 导致后续 candidate/endpoint/slot 地址自适应。membership 在 masked operands 上运行；membership share 形成 carrier 并送入 inverse shuffle；最终每方只返回本方 inverse output share。PREPARE/COMMIT 是固定 1-byte 1/2，ABORT 是固定 0；本地 output markers 不直接作为 peer payload。

P_b simulator 的目标输入只能是 (X_b, ideal_output_share_b, L)：

1. 从 L 重建 public binding、获准的 masked operands、seed、端点/pivot、opened bits、消息长度与固定 phase；不能读取真实对端 transcript。
2. 每槽以 BCG+21 的 key-only simulator 替代 party key，再对同一 key 保留双 Eval 相关性。只有在第 5 节所述条件 UCMP 引理成立后，此步才能推广到 real invocation。
3. 按公开 seed/bit 状态重建当前地址；生成本方 used、unused、rejected slot、manifest/segment/lease transcript。两个 schema 分开运行，不能将 MONOLITHIC 与 STREAMED store 状态混为一谈。
4. 对每个 shuffle 方向使用本方 input share/理想 output share 和 L 模拟 local sigma/tau/a/e/r 与发收 arrays；应验证共同 Pi/Pi^-1、两方向 metadata、membership carrier、公开 Y/Q 和最终 share 条件分布在同一条件游戏下吻合。
5. 生成同长 PREPARE/COMMIT 或 ABORT/crash transcript。step 5 的本地孤立 share 取腐化方 ideal output share；还需证明该 share 与所有前置 shuffle/DCF/local lease state 的联合分布。

单方向 shuffle 的局部观察量有可行的边缘构造：从 L 给的本方 Y 可设 a_b=Y_b−sigma_b(X_b)，从公开 Q 和抽样的本地 S 可设 r_b=Q_b−S_b、e_b=S_b−tau_b(Y_peer)。但是这只给出边缘条件关系；它没有证明双向共用 Pi/Pi^-1、Select/Membership 载荷、输出 bit 和失败/重启信息条件下的整个 party view。

**首个未闭合 hybrid**：把真实 UCMP 输入的相关 DCF party key/Eval-share，替换为基于固定函数 key-only theorem 产生的独立 party key，同时条件化于本方输入 share、本方 mask/material auxiliary state、公开 z operands 与 opened compare bit。缺少的具体命题是：对实际 alpha=(mask_left−mask_right) mod 2^w、同一 key 的 x/y 两次 Eval，以及代码里 z 由双方 mask shares 加秘密共享 operand 得到的生成过程，theorem 的 key-only 分布在该条件辅助输入下仍可模拟，且误差项仍为声明的 delta_w。随后还须证明 per-schema 多键池及共同置换的双向 shuffle/output 联合条件分布。现有材料没有该条件独立性/auxiliary-input 归约。

未发现可重放区分攻击；这是“未发现反例，但证明未闭合”。不把有限 E2E、单变量边缘均匀或“字段已在 L”视为通过证据。N=128 和 N=256 的首个 UCMP hybrid 相同；后续 pool/store hybrid 必须依各自 schema 分别完成。

## 8. 输出/故障合同

成功时本地 party 仅返回本方 XOR share。测试中完整 mask 由隔离 TEST_ONLY supervisor 合并检查。crash-prefix 的既有 6/6 结果与本轮抽查的 case 55 只支持 no-complete-mask：某些后期崩溃可留下 P1 的 8-byte 本地 orphan share。该孤立 share 位于本方本地状态/结果管道，不意味着对端获取完整 mask；但在 restart/retry、OS 权限和联合 transcript 中仍需由 simulator 建模。两方文件/进程不能据此声称原子同时发布，也没有证明任意崩溃下 all-or-nothing。

## 9. 门禁与未运行项

| 门禁 | S18 裁决 | 证据与限制 |
|---|---|---|
| S15_INDEPENDENT_RECEIPT | PASS_WITH_SCOPE | 非作者接收者；72/72 hash；fresh ON/OFF；见本报告复跑矩阵 |
| DEFAULT_OFF_API_BOUNDARY | PASS_WITH_SCOPE | OFF 无 BCG 符号/目标、头文件按预期拒绝；ON API 独立 probe NOT_RUN |
| DCF_SINGLE_KEY_PRIVACY | CONDITIONAL | BCG+21 key-only 定理映射 + AES-PRP + epsilon_RNG_joint 未量化 |
| UCMP_SINGLE_CALL_VIEW | OPEN/CONDITIONAL_CANDIDATE | 双 Eval 同 key；条件 mask/z/open-bit simulator 引理未证明 |
| POOL_AND_ADAPTIVE_ACCESS | OPEN | 两 schema 静态 key-only projection 可复算，adaptive transcript 未证明 |
| SHUFFLE_OUTPUT_COMPOSITION | OPEN | M2CM/v1 双向共同 Pi/Pi^-1、membership 与 output share 联合条件分布缺证明 |
| CRASH_ABORT_OUTPUT_VIEW | CONDITIONAL_FUNCTION_OPEN_PRIVACY | BCG faults 6/6 no-mask；case55 旧 ABI no-complete-mask；不是原子/all-or-nothing |
| N≤256_FUNCTIONAL_E2E | PASS_WITH_SCOPE | N128 monolithic、N256 BCG21 streamed 12 smoke；不是全 25-case 再接收 |
| N>256_ADMISSION | NO_GO | 未扩大；沿用既有 N=257 拒绝记录 |
| SECURE_ALIAS_READY | NO_GO | UCMP→adaptive pool→shuffle/output joint view 未闭合 |
| V3_V4_PERFORMANCE_STATUS | V3 OPEN; V4 NOT_ALLOWED/NOT_RUN | 未运行正式 LAN/WAN 矩阵 |

NOT_RUN：S17 全部 25 case 重跑；全部 crash-prefix 的 BCG21 专用版本；独立 TLS conformance；streamed store 独立 CTest；完整 pool 失败路径 used-slot 计数；墙钟时序审计；生产 OS/store 故障条件。功能计数不是时间性能或复杂度界。

## 10. 复跑入口、证据和冻结范围

source/build/evidence manifest 含 S15 manifest 与 72 项哈希、S18 复算摘要、环境、构建命令、关键二进制 SHA、CTest 日志 SHA、E2E 结果和限制。S18 原始 ON/OFF CTest LastTest.log 位于外部 evidence 目录的 receipt 子目录；构建目录仍在 Windows Temp。S17 原始 JSON/报告保留原位且哈希不变。S18 没有修改任何运行时代码、测试源码、BB90/S8/S17 历史证据、VFSS-baseline、论文、密钥或材料包。

在 S18 worktree 执行 git diff --check 与冻结路径检查后，报告命令和结果追加到本段及 manifest。S18 修改仅限本报告、决策、gate JSON、source/build/evidence manifest 以及 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/REFERENCE_MANIFEST.md 的当前状态同步。


## 11. 迭代、比较依赖层和在线交换计数边界

三类计数分开记录：算法控制器执行五个 iteration；明文比较依赖波次最多九层；端到端消息交换由实际阶段 DAG 决定，不能用前两项代替，也不能将其称为 Agarwal 论文核心三轮。S15 runtime metric 源码中的当前计数表达式是 9 + 2×active_compare_layers + active_sample_rounds + 1；本次只复跑比较调用总数和部分 smoke，不独立审计每一个 endpoint 的完整在线消息交换，也没有为每个 case 留存 C/A 的计数拆分，因此 S18 的实际端到端 exchange 数标记 NOT_MEASURED。代码中的计数公式不表示论文轮数，也不单独证明消息 DAG 轮次正确。

## 12. 最终文件与差异检查

S18 未发现可复现 runtime 缺陷，未修改 VFSS 或测试源码。git diff --check 退出码 0；冻结路径 VFSS-baseline/ 的 diff 为空。任务 worktree 差异仅含 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/REFERENCE_MANIFEST.md 三份状态文档，以及 S18 决策、报告、gate JSON 和 source/build/evidence manifest。桌面主 checkout 保持原样。外部构建和 evidence 文件未进入 Git。

source/build/evidence manifest SHA-256：5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78。该 manifest 含 72 项 S15 源码哈希、构建/测试二进制 SHA、ON/OFF CTest 日志 SHA、外部 evidence 路径、独立接收范围和未运行清单；不自包含 manifest 或报告自身哈希。最终报告 SHA-256 由交付记录单独给出。

## 13. P0/P1 的分方模拟目标与首个断点

安全目标不是“某个字段各自均匀”，而是对 b∈{0,1}，比较下列 Real_b 与 Sim_b 分布。Real_b 的视图包含本方 X_b、party key 与 mask/package/store state、双向 shuffle material 与本地 shares、全部发送/接收帧、按 seed/bit 生成的访问轨迹、本地 membership carrier、output share 和 crash/lease transcript。Sim_b 的输入严格限于 X_b、本方理想 output share O_b 和获准泄露 L；它不得输入真实 transcript、X_(1-b)、完整 mask 或明文 oracle。目标是 Real_b ≈c Sim_b，按具体 S15 ABI/schema 与 AES/RNG 条件参数化。

| 腐化方 | 单方向候选构造（仅候选，不是通过证明） | 真实 party-key 角色 | 适用的存储对象 | 未闭合项 |
|---|---|---|---|---|
| P0 | S0(X0,O0,L)：按 L 中 Y0 取 uniform sigma0，反解 a0=Y0−sigma0(X0)；对每个方向取 tau0 与 local masked-share S0，再令 r0=Q0−S0、e0=S0−tau0(Y1)。inverse 方向须让本地输出坐标的 word1 LSB 与 O0 一致，其余分量按真实条件分布采样。 | party=0，root control 初值 0；每个 DCF key 的两次 Eval 在同 key 上模拟。 | N=128 M6B5/v3+M2CM/v1+M6P2/v1；N=256 M6S2/v2。 | 尚未证明以 fixed-function key-only simulator 替换时，上述 shuffle/input/mask/O0 条件分布仍保持；共同 Pi 与 Pi^-1、adaptive bits/addresses 和 membership carrier 的联合条件分布未导出。 |
| P1 | S1(X1,O1,L)：按 L 中 Y1 取 uniform sigma1，反解 a1=Y1−sigma1(X1)；对每个方向取 tau1 与 local masked-share S1，再令 r1=Q1−S1、e1=S1−tau1(Y0)。inverse 方向以 O1 约束输出 share bit，再按剩余真实条件分布采样。 | party=1，root control 初值 1；其 DCF key/Eval share 与 P0 方向不同，不能将 S0 的函数代码不加变换照搬。 | 分 schema 结论同上；M6B5/v3 的 ready/manifest 与 M6S2/v2 的 segment/lease 分开模拟。 | 同样缺少对本方 key、mask、公开 z、打开 bit 与 shuffle metadata 的联合辅助输入模拟，以及 crash/restart orphan-share 的联合分布证明。 |

这里的 uniform 候选采样是待证明分布构造，不代表已验证可从 (X_b,O_b,L) 采样出 Real_b 的全部关联变量。具体首个 H0→H1 替换是调用 DCF key-only theorem 时把本方真实 M2B1/v1 key 换成模拟 key；需要 theorem 在由 alpha、mask、opened z operand、另一次同-key Eval 及 opened comparison bit 条件化后仍适用。当前材料未给出这种辅助输入/条件分布引理。若此步不能证明，后续全池 hybrid 和 shuffle/output hybrid 即无可继承的前提。
S17 同会话 25-case E2E JSON 的历史 SHA-256 为 29566a9f1e459a5826a1910411a420c2d455e99bde7766af389a91b45e26c252；S18 没有改写或重跑该文件。


## 14. 精确源码定位

为便于另一位 reviewer 在 S15 快照重放，主要代码位置如下（行号以 S15 source snapshot 为准）：

- AES-5-block G 展开、514 位解析、Convert_G 与树递推：VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp:72-100、183-251；RAND_bytes root seeds：253-280；M2B1 serialization/deserialize：282-335。
- alpha = (mask_left-mask_right) mod 2^w、beta=1 及双 Eval 的 uCMP：VFSS/src/moe_topk/protocol_i_ucmp.cpp:27-60、100-129、165-193。
- Pi/sigma/tau/a/e/r 的 Dealer 生成：VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp:337-373；本地方向的 Y、S、Q 与 public aggregate：504-531。
- M6B5 package ready/digest、party package claim/lease：VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:1629-1776；初始 package binding：1854-1882。
- endpoint operands、DCF output shares 与 opened comparison bits：VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:1920-2010；当前迭代 seed phase/ACK 与 sample reconstruction：2013-2063；membership operands/carrier/inverse output share：2073-2114；prepare/commit/publish/abort markers：2118-2142。
- 传输 frame header 中的 session/fingerprint/phase/sequence/material ID 与验证：VFSS/src/moe_topk/protocol_i_transport.cpp:52-116、345-389。
- OFF compile gate 的公共头和 CMake option：VFSS/include/moe_topk/dcf_bcg21_fig1_aes128_v1.h:3 起；VFSS/CMakeLists.txt:25、78、276、690。


## 15. 失败与重试台账

- 源码冻结准备阶段曾从 S17 当前 worktree 尝试按 S15 文件清单复制；在 docs/IMPLEMENTATION_PLAN.md 发现 S17 后续变更时停止。该部分外部临时目录 M6B_I_S18_20261016_01 保留为失败的准备尝试，没有据此构建或运行测试。随后改从原始 S15 worktree 获取并逐文件哈希验证的 72 项快照，形成当前 evidence 目录 M6B_I_S18_20261016_02。
- 证据打包时第一次 canonical 计算使用了与清单不同的路径排序/字节规范，得到不匹配值；没有文件校验 mismatch。改为 UTF-8 ordinal 路径排序、path<TAB>lowercase SHA-256<LF>、UTF-8 no-BOM 重新计算后与 manifest 的 c4a7df6c… 完全一致。source 文件逐项仍为 72/72。
- 一次 N=256 选择开关 --n256-k 128 运行到 legacy M2UCv1，因此排除为 BCG21 接收证据；使用 --bcg21-smoke 重跑后得到 12/12 ORACLE_PASS。
- 所有 ON/OFF configure/build 和已运行 CTest/E2E 退出码均为 0。OFF 符号/目标 grep 返回 1、头文件探针返回 1 均为预期拒绝，不是运行失败。没有 runtime patch，因此无修复前后测试状态需要覆盖。
