# M6B-I-PROJECT-SELECT-5ITER-S19 条件视图与联合单方视图复核

日期：2026-10-16  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`  
裁决：**UCMP 条件 key-view 引理、两种 schema 的 key-pool hybrid 与 shuffle/output 单方模拟均为 CONDITIONAL；`epsilon_RNG_joint` 未量化。未发现攻击或可复现运行时缺陷。未改运行时代码，未重跑 S18 功能矩阵，未创建 secure alias。**

## 1. 范围、对象与审查者说明

被审查的运行时代码是 S18 独立接收的 S15 默认关闭修复快照，不是当前 checkout 的任意更新文件，也不是把 S18 文档并入 S15 runtime。S15 manifest SHA-256 为 `4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`；逐项重算 72 个源文件得到 72/72 匹配，UTF-8 ordinal 路径 canonical SHA-256 为 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。源码快照外部路径为 `C:\Users\28641\.codex\evidence\M6B_I_S18_20261016_02\source`，以其 72 项清单为唯一运行时对象身份。

S18 工作树基于 `c3926c68fd14f270faa8b55234311071947fa080`，S15 runtime 未提交且未合入 main。S19 在新隔离 worktree、分支 `codex/m6b-project-select5-s19`、同一 base commit 上工作。S18 接收者身份与功能证据只按 S18 报告记录；本报告是对论文和锁定 S15 源码的重新推导，不把自己表述成独立第三方审计，也不把 S18 功能 PASS 扩大到安全 PASS。S19 未构建二进制、未运行 CTest/E2E，因为没有 runtime 修改或新功能缺陷；S18 原测试未重跑。

固定功能语义仍为 signed Q20.12、score 降序、同分 `original_index` 升序、原输入顺序下恰 K 位的共享 Top-K mask。路线是项目派生方案，不是 BB90 或 BMW16 复现。N=128 使用 `M6B5/v3 + M2CM/v1 + M6P2/v1`；N=256 使用 `M6S2/v2`，两套对象下文分开计量。

## 2. S18 定理桥接到 M2B1/v1 的源码核查

本节对照 [Boyle et al., *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*（Microsoft Research 作者稿）](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf) 的 Figure 1、Theorem 2 和 Appendix F.1。为避免 0-based viewer 页序歧义：Figure 1 在 PDF viewer index 12（从 1 计 PDF 第 13 页），Theorem 2 在 viewer index 13（PDF 第 14 页）；Appendix F.1 跨 viewer index 41–45（PDF 第 42–46 页）。定义 2/DCF 定义允许的 key leakage 只有函数输入/输出群；Appendix F.1 通过逐层替换证明固定函数参数下每个 party key 的伪随机性。定理没有直接声明 UCMP、任意 MPC transcript 或 shuffle/output 的安全性。

| 论文/接口量 | S15 实际实现 | 核对结果与范围 |
|---|---|---|
| 安全参数 λ | AES block/key 128 bit | 固定为 128；不主张具体安全位数 |
| 输入位宽 n | `bits`，uCMP 接受 34..53；score adapter 使用 34 | 在定理的 `{0,1}^n` 域内；调用处将输入约束在 ring mask |
| 输出群 G | `Z/(2^64)`，`uint64_t` 加减模 2^64 | 有限 Abelian group，`ell=64≤λ` |
| 函数参数 | `alpha=(mask_left-mask_right) mod 2^w`，`beta=1` | 与 DCF `1[x<alpha]` 接口相符；α 不序列化到 party key |
| 根状态 | `RAND_bytes` 两个 16-byte root seed；评估初始 control=`party` | 与 Fig.1 的随机根 seed 和固定互补初始控制位 `t0_b=b` 相符；真实 RNG 仍由联合假设覆盖 |
| 每层 G | 5 个固定且相异 AES 输入块；保留拼接输出前 514 bit；MSB-first 解析两个 seed、两个 128-bit v 和两 control bit | 对应 `4λ+2=514` bit；`convert_group` 只取 seed 的前 64 bit，舍弃 v 的低 64 bit与论文的 Convert_G 定义一致 |
| correction word | 每层 16-byte seed、8-byte group element、1-byte control（仅低 2 bit 有效）；终端 correction 为 8-byte group element | 有效字段与 Fig.1 相同；高 6 bit 固定为 0 是序列化 padding，不是随机 key 位 |
| 序列化 | header/count 固定，root seed、各 CW、终端 group element 按字节序编码 | `M2B1/v1` 版本化；header、count、party/width、padding 不泄漏 α/β，模拟器必须输出同形状结构化随机 key，而非任意字节串 |
| Eval | 同一 party key 两次评估，输入为 `x=(z_left-z_right) mod 2^w` 和 `y=(x-2^(w-1)) mod 2^w` | 两次 Eval 是同一 key 的相关确定性后处理；不建模成两把独立 key |

源码定位及本快照行号见 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S19_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-16.json`。主要位置：`dcf_bcg21_fig1_aes128_v1.cpp` 的 `expand/expand_children/convert_group/eval/generate/keygen/serialize`（72–100、183–335）；`protocol_i_ucmp.cpp` 的 α、beta、双 Eval 和 party share 公式（27–60、100–129、165–193）；`protocol_i_score_input.cpp` 的 carry/sign 两次 masked-input exchange（79–140）。

### 2.1 可复算优势项

Appendix F.1 对宽度 w 的单个 party key 使用 w 个逐层 PRG hybrid，再用终端 Convert_G hybrid。这里 `G` 取 514 bit，`Z/(2^64)` 为 2 的幂且 `64≤128`，因此 Convert_G 是 128-bit seed 的前 64 bit；在理想均匀 seed 下，转换为均匀群元素的额外统计误差为 0。每次 PRG 扩展使用同一 seed 作为 AES-128 key，查询 5 个不同固定 plaintext；用 PRP/PRF switching 得到单次 PRG 项

`epsilon_G(T') <= Adv_AES128^PRP(T', q=5) + C(5,2)/2^128 = Adv_AES128^PRP(T',q=5) + 10/2^128`。

因此固定函数、理想独立随机 tape 下，单 key key-only 项为 `w * epsilon_G`，不是只乘一次 `epsilon_G`。S18 两个池的系数是所有 key 输入位宽之和，复算如下；同 key 双 Eval 不再乘 2，因为它们已作为一个 key 的确定性后处理纳入区分器。

| N / schema | score 转换 key | Select key | membership key | 总 DCF key | M2CM shuffle metadata logical slots（另计） | key 输入位宽和 Σw |
|---|---:|---:|---:|---:|---:|---:|
| 128 / `M6B5/v3 + M2CM/v1 + M6P2/v1` | 256 × 34-bit | 40,640 × 40-bit | 128 × 40-bit | 41,024 | 1,280 | 256×34 + 40,768×40 = **1,639,424** |
| 256 / `M6S2/v2` | 512 × 34-bit | 163,200 × 41-bit | 256 × 41-bit | 163,968 | 2,560 | 512×34 + 163,456×41 = **6,719,104** |

全池式在第 5 节列出。`T'` 包含构造混合挑战及运行 DCF 对手的多项式开销。公式不是测得性能数字，也不给出 concrete advantage：`Adv_AES128^PRP` 的时间界和最关键的 `epsilon_RNG_joint` 均未实例化。

S18 已记录的成功样本作容量核对（不是 S19 新复跑）：N=128 case 301 有 2,148 个 Select 调用和固定 `3N=384` 个 score/membership key，合计 2,532 个已消费 DCF slot、38,492 个未消费 slot；N=256 case 110 有 17,207 个 Select 调用和 `3N=768` 个固定 key，合计 17,975 个已消费 slot、145,993 个未消费 slot。成功路径无重复 slot；S18 abort/crash path 的逐项 consumed-slot 计数为 `NOT_MEASURED`。M2CM metadata slots 不属于 DCF key pool，也不进入 `Σw`。

## 3. RNG 与条件辅助输入

对安全替换所需的最小 RNG 假设定义如下，而不是把系统调用名直接等同于理想随机：对给定 schema 和公开参数，实际执行中整段有序随机 tape（离线排列、shuffle `a/e/r`、每个 mask 及其 additive shares、每个 DCF 的两个 `RAND_bytes` roots、逐轮 `getrandom` sample seed、Fisher–Yates rejection sampler 的拒绝与停止点）与独立均匀理想 tape 的 party-view 分布差异至多 `epsilon_RNG_joint(schema,T)`。该实验中的区分者可见本任务获准的 L 和腐化方全部本地辅助材料。它是外部、平台相关且目前未量化的联合假设；不是 `RAND_bytes` 与 `getrandom` 已统计独立、无条件均匀或具有某个已知安全位数的证明。

S15 源码中 Select5 与 shuffle 使用 `getrandom`；BCG21 DCF roots 使用 OpenSSL `RAND_bytes`。`random_bounded` 采用 rejection loop，因此 tape 请求数量可以变化；sample seed 在 forward shuffle 完成之后、实际活动 iteration 开始时由 P0 生成。seed 不在包、manifest、binding 或 ready metadata 中。S19 将随机源差异统一留在 `epsilon_RNG_joint`，未将其移到 AES 项中或按 key 隐式重复计数。运行时序证据沿用 S18，S19 未做逐帧重放或平台 RNG 评估。

## 4. 核心结果：M2B1/v1 单 key 的条件 UCMP 视图引理

### 4.1 视图与模拟目标

对腐化方 `b∈{0,1}`，令 `W_b` 表示调用前的本方输入份额、本方两侧 mask shares、key slot/session/purpose/iteration/address、其它已经交付给该方的非 key 材料和本地状态。线上并非只传一个合并后的 `Z`：Select/score/membership 各方先发送自己的 masked operand-share 数组并收到 peer 数组；两数组都在双方本地 view 中，逐坐标相加才得到公开 ring 值 `Z=(z_left,z_right)`。令 `alpha=(M_left-M_right) mod 2^w`；`x=(z_left-z_right) mod 2^w`、`y=(x-2^(w-1)) mod 2^w`。同一 party key `K_b` 产生 `v_x=Eval_b(K_b,x)`、`v_y=Eval_b(K_b,y)`，uCMP 返回

`u_0 = 1 - (v_y-v_x) mod 2^64`,  
`u_1 = -(v_y-v_x + [y>=2^(w-1)]) mod 2^64`。

opened comparison `c=(u_0+u_1) mod 2^64∈{0,1}`。按 DCF correctness，对每个合法 root tape，c 是输入关系的确定函数，不取决于 DCF root/CW 随机性。P0/P1 的真实局部视图包含本方 K、两次 Eval、`u_b`、收到的 `u_(1-b)` 和 c；对端 share 满足 `u_(1-b)=c-u_b mod2^64`。

模拟目标是 `(W_b,K_b,local_operand_frames,peer_operand_frames,Z,v_x,v_y,u_b,u_(1-b),c)`。条件 UCMP 引理只把调用上下文固定；其安全归约的 distinguisher 可把给定的非 key 辅助状态作为 advice，但模拟器最终只收到该方输入 share、理想 output share 与获准 L，不收到真实 transcript、peer secret、隐藏 α 或明文 oracle。全协议模拟器如何由这些输入生成 `W_b` 与逐方 operand frames 在第 7 节给出。

### 4.2 证明步骤

1. 固定任意合法 `alpha,beta=1,party b` 及任意本方辅助状态 `w`、masked operands Z 与比较位 c。虽然 Z 与 α 相关（`z_left-z_right = input_difference + alpha mod 2^w`），Appendix F.1 的 key-only hybrids 对每个固定 α、β、party 及非均匀 distinguisher 给同一上界；证明不要求 Z 与 α 独立。
2. 在理想独立 tape 下，给定 α，DCF roots 独立于本方 additive mask shares、shuffle/lease 前态和 sample seed。Correction words 由 α 和 DCF roots 计算；α 与 key 的代数关联并未被忽略，而是由逐 α 的 DCF key theorem 处理。形式上先把 `(alpha,W_b,local/peer operand frames,Z,c)` 作为非 key 旁信息固定；若某字段是随机变量，则逐其支持点应用相同的均匀界再取期望。这里不能把包含 K 本身的 package digest 当作固定 advice；它作为 K 的确定性后处理随 key 一起进入区分游戏。
3. 对任意固定 (α,w,Z,c)，把该条件上下文编入非均匀区分器 advice。定理将真实 `K_b` 替换成同版式随机结构化 key `U_b`，差异至多 `w*epsilon_G`。对 (α,w,Z,c) 的真实联合分布取期望，不增大这个逐点上界；再加整段 tape 替换差异 `epsilon_RNG_joint`。
4. 在 U_b 上按源码对同一 key 计算 x/y 两次 Eval，得到 v_x、v_y、u_b；不把两次 Eval 当成独立随机量。由 c 构造接收的 party share `u_(1-b)=c-u_b`。真实执行中由正确性恰有该等式，所以收到的 peer message 不增加 key 条件项。
5. 序列化 header、party id、width、key counts 和控制字节 padding 固定；模拟器只随机化合法 key 的 seed/group/control 字段。它输出的不是任意 uniform bytes，而是 Theorem 2 最终 random-key hybrid 的同格式结构化字段。

因此，在准确的随机源条件下，**给定调用前状态和获准 masked operands/开 bit 的单次相关 UCMP 视图**满足 `CONDITIONAL` key replacement。没有要求 peer key 与 K_b 独立，也没有假定同一 key 两次 Eval 独立。若不对整段 tape 作理想化，结论增加 `epsilon_RNG_joint`，该量目前未量化。

### 4.3 P0 与 P1 的专门化

两方的证明共享上述逐点定理，但不能省略 party 标签：P0 的生成状态从 `t0=0` 开始，P1 从 `t0=1` 开始；模拟 key 的 header 固定为相应 party；`u_0` 和 `u_1` 的常数修正不同。`Sim_b` 对两个输入点使用一个 `U_b`，计算两个评估 share，并用获准开位 c 生成另一 share。如此分别覆盖 P0 和 P1 的本地 DCF key、Eval pair、发送/接收 share 和 slot metadata。

这是一条针对实际 M2B1/v1 ABI 的条件归约，不是通用 DCF 定理对任意 key layout 的自动转移。适用边界为 AES-128 的五点 PRG、`G=Z/(2^64)`、`beta=1`、`w=34..53`、全新一次性 key、可信离线 T 和本方不与 T 合谋。

## 5. 全池与自适应地址：分别裁决两种 schema

记 schema 内 DCF slots 为 `j=1..m`，每个 `w_j` 为其 DCF input bits。条件于完整 α 向量、全部本方 mask shares 和非 key dealer state，各 key 的 roots 来自独立理想 tape。对 key 按固定 slot 顺序作 hybrids；每步由前节逐点 bound 替换一个 party key。把所有本地输出、未使用文件、manifest/hash、slot lease、错误和访问轨迹视作已替换 key pool 的后处理。所有 `w_j` 项求和后：

`Adv_pool(schema,T) <= (sum_j w_j) * [Adv_AES128^PRP(T',q=5)+10/2^128] + epsilon_RNG_joint(schema,T)`。

| 腐化方视图/材料 schema | 已用、未用与地址规则 | 全池界 | 状态 |
|---|---|---|---|
| P0 或 P1；N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` | 单一 monolithic party package 包含全部 41,024 slots；Select address=`(purpose,iteration,substage,direction,left,right)`；未访问 slot 仍在该方包内，重复 slot 由消费状态拒绝；package claim/digest 本地可见 | `1,639,424 * epsilon_G + epsilon_RNG_joint(M6B5/v3)` | CONDITIONAL |
| P0 或 P1；N=256 `M6S2/v2` | 163,968 slots 在 party-scoped streamed segments；manifest 给 address/segment/offset/length/digest；逐槽 durable lease 先于读取；访问顺序由当前 seed 和已公开 comparison bits 导出，未访问槽仍在本地 segments | `6,719,104 * epsilon_G + epsilon_RNG_joint(M6S2/v2)` | CONDITIONAL |

自适应性处理的关键是顺序：sample seed 仅在前向 shuffle 完成后且 iteration 激活时公开；当前样本、候选状态、后续 endpoints/pivots 和 slot 地址是此前获准 seed/比较 transcript 的确定函数。正确性使打开的 comparison bit 不依赖 DCF roots。故固定公开 transcript 后，访问顺序不再是目标 key 根随机带的选择事件；整池先 hybrid、再按 transcript 重放 slot/lease 可覆盖适应性。future seed 没有写在 offline package、manifest、binding 或 ready metadata 中，模拟器按实际 phase 顺序生成，而不提前读取未来 seed。

这里的“池”包含所有 key 字节，而不只是被调用 key：monolithic 包中的未用 keys 和 streamed store 中未 lease 的 segment bytes 一并替换；拒绝路径只产生实际已创建的 claim/lease 前缀。当前状态仍是条件证明，因为 `epsilon_RNG_joint` 未量化，且 full party-view 宏观证明没有另一位非作者审查。

## 6. 获准泄露 L 与线上/本地视图映射

| 字段 | 产生方与可见者 | 坐标/时点 | 是否在 L；模拟处理 |
|---|---|---|---|
| session、fingerprint、N/K、bit width、ABI、material ID、party/phase/sequence | P0/P1 交换 binding/frame header；双方可见 | 固定公共参数及每帧 header | 是，协议绑定元数据；由配置和消息 DAG 重建 |
| raw-score adapter 的 carry/sign masked operand-share 数组及逐坐标和 | P0/P1 各自发送/接收数组后双方本地可见；数组按固定输入 slot 次序 | shuffle 前，frame phase 4/5；局部份额与合并 ring 值分开记录 | 本地出/入 frame 和由两者相加得到的 masked value 均纳入已批准 masked-value L 与通信长度；不把原始 score 或 shuffled-position 映射公开 |
| forward shuffle 的 `Y_b`、`Q_b` 和其聚合 masked records | P0/P1 互相发送并可计算 aggregate | phase 20/21，shuffle 坐标 | 是，按已批准 masked values；模拟器给定本方输入与 frames 生成 `a/e/r` 和本地 share |
| 本方 `sigma/tau/a/e/r`、DCF/mask package、M6P2 ready/package digest、M6S2 segment manifest/lease | 仅本方进程、可信 T/supervisor 的对应本地存储可见；不作为 peer payload | 离线、claim/read、重启时 | 不是新增 peer leakage；属于模拟器输出的本地 view，按 schema 生成并重算本地 digest/lease |
| 公开 sample seed、端点、pivot 位置、opened comparison bit、比较次数 | 当前 iteration 活动后双方可见；Select controller 根据共同 transcript 更新 | seed phase `40+i` 在 forward 完成后；comparison endpoint/result 在对应 wave | 是，先前批准；按时间 DAG 逐轮重放。seed 不得预先存在 |
| Select DCF 的 local/peer operand-share 数组、合并后的 `z_left/z_right`、两次本地 Eval 输出 share、peer Eval share | 各 operand/result frame 对端可见；双方分别由 operand-share 求和得到 z，result-share 求和得到 opened bit | 每个 compare wave operand phase `10+2l`、result phase `11+2l` | 两边 operand-share frames、masked z 与 opened bit 均按已批准 masked-value/comparison L 逐字段记录；Sim 使用同一 U 生成 local Eval/share，再用 `c-u_b` 生成接收 share |
| membership masked operands、membership local result/carrier | masked frame 对双方可见；local carrier 仅本方 | phase 60 后进入 inverse route | masked values/长度在 L；membership bit 不打开；Sim 本地计算 carrier |
| inverse shuffle frames 与输出 XOR share | frames 为带掩码值，对端可见；最终 share 仅由本地 API/本地 crash artifact 持有 | phase 22/23，随后本地 prepare/commit/publish | frames 纳入 masked values；output share 是理想输出 share 输入 |
| PREPARE=1、COMMIT=2、统一 ABORT=0 | 双方在线进程可见；store marker 是本地状态 | phase 61/62/63 | 状态/长度已获准；固定字节按 success/failure prefix 模拟 |
| 输入相关墙钟时间、文件系统管理员视图、T 与 party 合谋 | 未有 S18 逐帧/墙钟证据；T+party 合谋被模型排除 | NOT_MEASURED / out of model | 不作为通过结论；管理员回滚/root 和网络外部 timing observer 不在当前证明 |

没有发现新的 peer-visible 明文字段。这里把“带掩码值”按实际线上字节解释为各方发送的 masked operand-share frame、接收的 peer frame 和双方由两者相加得到的 masked ring value；这是既有批准字段的精确线级展开，不增加 seed、明文 score、pivot value 或 raw-index 到 shuffle-position 的映射。raw-score adapter 的数组位置是固定的原始输入 slot 结构；它不公开 score。S19 没有请求重复确认已批准字段。

## 7. Protocol I forward/inverse/output 的 P0/P1 模拟器

本节给出在前述 DCF pool hybrid 之后的条件模拟。群为 `G = Z/(2^comparison_bits) × Z/(2^64) × Z/(2^64)`。泄露输入 L 包含实际 peer-visible 的 masked `Y/Q` record frames、masked operand frames、approved opened bits、逐活动 seed/endpoint/pivot/slot trace、长度和统一状态；模拟器不输入真实 transcript 对象，而是按 L 的已批准字段逐 phase 生成一个同分布的 transcript。L 的数值是 ideal leakage 的输入，不是偷渡未模拟的真实消息。

### 7.1 shuffle dealer 代数

源码 Dealer 对每个 direction 使用隐藏置换 `Pi_d`、独立均匀 `sigma_0,sigma_1`，令 `tau_0=Pi_d o sigma_1^{-1}`、`tau_1=Pi_d o sigma_0^{-1}`；采样均匀 `a_0,a_1,h,r_0,r_1`，并设 `e_0=-tau_0(a_1)-h`、`e_1=-tau_1(a_0)+h`。线上 `Y_b=sigma_b(X_b)+a_b`，`S_b=tau_b(Y_(1-b))+e_b`，`Q_b=S_b+r_b`。于是

`S_0=Pi_d(X_1)-h`, `S_1=Pi_d(X_0)+h`, `S_0+S_1=Pi_d(X_0+X_1)`，

`Q_0+Q_1=Pi_d(X_0+X_1)+r_0+r_1`。

对固定共同 `Pi_d`，每个腐化方本地 `sigma_b` 均匀；`tau_b=Pi_d sigma_(1-b)^-1` 也均匀且独立于 `Pi_d` 和本方 `sigma_b`。forward 与 inverse 使用独立的本地 sigma 和 fresh masks，即使 dealer 对 inverse 使用同一个 `Pi^-1`，单方观察到的 local `(sigma,tau)` 联合分布仍是乘积均匀，无法从该本地排列 tuple 识别共同 Pi。`h` 使本地 `S_b` 均匀；`r_b` 使自己/peer 的 `Q` frames 不暴露另一方 S share。

### 7.2 显式 `Sim_b(X_b,O_b,L)`

以下对 b=0、b=1 分别执行。公式里的 `2b-1` 给出 P0 的 `-h` 与 P1 的 `+h`；模拟器不需要知道、也不采样真实 peer secret 或真实 Pi。

1. **离线包与 DCF pool。** 按对应 schema 的公开 descriptor 生成完整本方本地包。每个 mask share 依 DCF key slot 关联的 masked operand 条件分布生成；每个 `M2B1/v1` key 取同 party/width 的结构化随机 key U。N128 生成 monolithic bytes、ready digest 和 package-level claim 状态；N256 生成所有 M6S2 slot bytes、segment/manifest offsets/digests，并仅在 L 的访问时序上创建对应 durable leases。unused/rejected slots 也生成，lease 状态只按已访问 prefix 变化。
2. **raw-score conversion。** 输入本方 raw Q20.12 share。对每个 carry/sign slot，L 分别给出本方 outbound operand-share frame 和 peer inbound frame；本方 mask share 由 `outbound - local_input_component (mod ring)` 直接确定，不需要采样或获知 peer raw share。把两份 frame 相加还原公开 masked z，再以对应 U 同 key 计算 x/y local Eval share，按实际 score-input wrapper 得到本方 carry/sign 与 priority-key share。carry/sign 没有作为 comparison bit 打开；两份 operand-share frame 和合并 z 纳入 L。
3. **forward shuffle。** 令 `X_b` 为上一步本方 priority-key shares。取均匀本地 `sigma_b,tau_b`，从 L 读本方 Y frame 并置 `a_b=Y_b-sigma_b(X_b)`。若本方 Q frame 为 `Q_b`，按真实条件分布取均匀 `S_b`、令 `r_b=Q_b-S_b`、`e_b=S_b-tau_b(Y_(1-b))`。由此模拟本方 local shuffled share 与全部 local shuffle materials；对 P0/P1 分别可按 `S_b=Pi(X_(1-b))+(2b-1)h` 验证边缘均匀性。公开 Q aggregate 从 L 的 Q frames 相加得到，不另加秘密。
4. **Select seeds、边图和 UCMP。** forward shuffle 完成后才处理 L 中实际活动的 32-byte seed；每个 iteration 仅当 controller 进入该轮时消费 seed，依据已公开前缀重建该轮 sample/endpoint/pivot/address。对每个地址加载 U 与本方 masks；local/peer operand-share frames 直接从 L 读取，本方 outbound frame 必须与已模拟本方 shuffled share 及本方 mask share 一致，再相加得到 z。计算同 U 两次 Eval 及 `u_b`；将收到的结果 share 生成成 `c-u_b mod2^64`。opened c 取 L 已批准值，候选/边状态按真实公开规则更新。inactive future seed 不存在、不放入包。
5. **membership。** 依 L 中 membership masked operands 和其 τ/item slot 取对应 U，计算本方 `lt_share` 与 `carrier[i]={0,1-lt_share,0}`（实际 party-specific 模式按源码选择）。Membership bit 不打开；carrier 是下一阶段本方状态。
6. **inverse shuffle。** 以 carrier 为本方 `X_b`，从新一套本地 uniform `sigma/tau` 和 L 的 inverse Y/Q frames 构造 `a/e/r`。对进入 output share 的逻辑项，按理想输出 share `O_b[i]` 取本方 inverse `S_b[i].word1` 均匀条件于其 LSB=`O_b[i]`；其余 record words 和未输出 padded positions 按真实均匀条件分布采样。此约束与真实 `S_b=Pi^-1(C_(1-b))+(2b-1)h` 相符，因为 fresh uniform h 使本方 inverse record share 在给定低位输出 share 后其余位仍均匀。
7. **成功、ABORT、崩溃/重启。** 按 L phase 输出 fixed PREPARE/COMMIT/ABORT bytes 与消息长度。已发生访问的 slot 生成一次性 lease；崩溃重启不得恢复已消费 key。若 crash prefix 已生成本地 share，则 simulator 使用 `O_b` 生成孤立本地方 share/local artifact；peer 仍拿不到完整 mask。不得模拟为双方原子 publish。

`Sim_0` 使用 party=0 的 key header、初始 control 0、uCMP share 公式 P0 及 shuffle 的 `-h/+h` 方向；`Sim_1` 使用 party=1 的对应字段和相反 share 公式。两者均不拿 peer raw input、真实 package、真实 transcript、完整 mask 或 oracle 作输入。

### 7.3 组合为什么只到条件结论

给定 6.2 的实际 masked wire leakage，raw-score、shuffle、Select、membership、inverse 和 output 端除 DCF root key 外均由本方 input/output shares、L 和独立 fresh pads 生成。uniform `a/h/r` 让本地 shuffle records 与 common permutation 的关系可按上面的线性等式消去；uniform DCF-key hybrid 后，Eval share、candidate、address、membership carrier 均是 U 与公开 L 的后处理。故 P0/P1 的联合单方视图可以按 `RNG hybrid → per-slot DCF key hybrids → party-local shuffle/output pad sampling → abort/lease prefix` 顺序条件模拟。以理想私有认证通道为模型时，通道项为 0；若要把结论具体化到 TLS 1.3 实例，还需加上未在本轮量化的 `epsilon_channel`（含认证/机密性失败及相关实现事件），并把外部可观察时序单独处理。`epsilon_RNG_joint` 不吸收 TLS 假设。

在每个 schema 与腐化方 `b` 的该模拟顺序下，统一界写为

`Adv_view_b(schema,T) <= (sum_j w_j) * epsilon_G + epsilon_RNG_joint(schema,T) + epsilon_channel(schema,T)`。

理想私有认证通道定义下 `epsilon_channel=0`；对应实际 TLS 1.3 运行环境的优势尚未量化，故不能把该条件界解释为当前部署的具体安全位数。`sum_j w_j` 按上表分别为 1,639,424 与 6,719,104。该式是相对于强 L 的计算不可区分界；不把 L 自身对原始输入的泄露纳入零知识结论。

这不是“两份输出原子同时发布”的定理：输出合同只支持 no-complete-mask。S18 的 crash-step 5 证据表明后期 prefix 可在 P1 留下 8-byte 本地 XOR share；在 ideal functionality 中该孤立份额作为腐化方自己的随机 output share 给 simulator，peer 无完整输出。其余 abort/crash prefix 只按 S18 已测范围引用；S19 没有把功能 crash 测试外推为任意进程/存储故障保证。

### 7.4 Simulator 可检查性边界

此模拟器把已批准 leakage L 当作理想功能明确输出的 wire-level 数值（包括两方各自发送的 masked operand-share frames 以及它们的合并值），而不是把“真实 transcript”传给模拟器再原样转发。证明对象因此是相对于该强 L 的不可区分性；若项目将来把 L 收窄为只含聚合 masked z、不含独立发送的份额 frame，则当前构造必须重做，不能把 frame 分解隐含地塞进 L。模拟器使用 L 中的输入/输出帧数值约束自身本地随机材料，再以定理替换 key；它不证明 L 自身对原始输入无泄露，也不证明主动对手安全。

## 8. 门禁裁决

| 门禁 | S19 状态 | 证据与限制 |
|---|---|---|
| `S18_SOURCE_IDENTITY` | PASS | 72/72 文件和 canonical hash 匹配；只确认 S15 runtime 身份，不扩大接收范围 |
| `S18_FUNCTIONAL_RECEIPT` | PASS_WITH_SCOPE（继承） | 只引用 S18 报告中限定的 ON/OFF 与功能证据；S19 未重跑，不能扩大到完整 S17 25-case 矩阵 |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL | M2B1/v1 到 BCG+21 Fig.1/Appendix F.1 的字段映射；AES-128 PRP；`epsilon_RNG_joint` 未量化 |
| `UCMP_CORRELATED_CONDITIONAL_VIEW` P0/P1 | CONDITIONAL | 对固定 α/β/party 逐点 key-only 替换；辅助状态取平均；同一 key 双 Eval 后处理；open bit 由 correctness 固定。运行时根随机源仍依联合假设 |
| `POOL_AND_ADAPTIVE_ACCESS` N=128 | CONDITIONAL | 41,024 slots、Σw=1,639,424，含未用槽及 monolithic claim/view |
| `POOL_AND_ADAPTIVE_ACCESS` N=256 | CONDITIONAL | 163,968 slots、Σw=6,719,104，含 M6S2 未 lease 段、逐槽 lease 与 restart prefix |
| `SHUFFLE_OUTPUT_COMPOSITION` P0/P1 | CONDITIONAL | M2CM/v1 dealer 等式、单方 local permutation 边缘与 fresh pads 条件模拟；依赖完整 L 的 masked frames 和输出 share idealization |
| `JOINT_PARTY_VIEW` | CONDITIONAL | 上述 hybrid 顺序给出；尚未被另一位非作者密码学 reviewer 独立复核；墙钟/恶意 root/跨主机 store 不在结论内 |
| `NON_AUTHOR_CRYPTO_REVIEW` | OPEN | 尚无另一位未撰写本论证的密码学审查者签核定理量词、条件化步骤、池混合与共享 Pi/Pi^-1 的模拟 |
| `CRASH_ABORT_OUTPUT_VIEW` | CONDITIONAL | 只按已有 no-complete-mask 案例；允许 orphan local share；不声称 atomic/all-or-nothing |
| `REPRODUCIBLE_ATTACK` | 未发现 | 没有构造出区分器；此事实不等于通过经验攻击搜索 |
| `RUNTIME_DEFECT` | 未发现 | 本轮源码复核未发现可复现代码/合同错误；未改代码 |
| `N>256_ADMISSION` | OPEN/NO_GO | 范围未扩大 |
| `SECURE_ALIAS_READY` | OPEN/NO_GO | S19 继续保留 EXPERIMENTAL；RNG 假设未量化且 joint proof 未获另一 reviewer 审核 |
| `V3_V4_PERFORMANCE_STATUS` | NOT_RUN | M6A V3 OPEN，V4 未获准；正式 LAN/WAN 未运行 |

## 9. 失败、未运行和交付边界

- S19 未发现新的失败样本，未修改或覆盖 S18 原始日志、case JSON、报告、source manifest 或 build manifest。
- S19 未运行 fresh build、CTest、oracle differential、E2E、TLS 或 crash-prefix。S18 的 5 ON CTest、2 OFF CTest、12 个 BCG21 smoke、1 个 N128 monolithic success、6 个 abort 及历史 crash-step 5 是继承证据，不是本轮重跑。S18 自己未覆盖完整 S17 25-case 矩阵的限制仍保留。
- S19 二进制 hash、S19 E2E、S19 RNG 来源量化、墙钟侧信道、生产 OS/store 和多机 durable store：`NOT_RUN/NOT_MEASURED`。
- S19 无 runtime 变更，因此没有“修复后回归”或新的功能身份。S18 决策、报告、gates 与 source/build manifest 以逐字节相同副本带入本隔离分支，以使状态文档引用可复核且不再悬空；另新增/更新 S19 决策、报告、gate、manifest 和三个状态文档。S18 原 worktree 与证据未改动。
- 五个算法迭代、最多九层明文比较依赖波次与完整 Protocol I 在线交换数继续分开。S18 报告中完整逐例 exchange split 为 `NOT_MEASURED`；S19 没有计算或声称新轮数/性能。

## 10. 下一步最小必要工作

请一位未撰写本报告模拟论证的密码学 reviewer 对 `Sim_b` 的逐方条件分布、Appendix F.1 的非均匀量词提升、Σw advantage、M6B5/v3 与 M6S2/v2 全池 replacement、共同 Pi/Pi^-1 与 inverse output share 条件分布逐项签核；另由平台负责人定义并量化 `epsilon_RNG_joint` 的调用序列、side information、平台和 adversary time。审核应特别检查两方各自发送与接收的 masked operand-share frames 是否均在 L 内。若 reviewer 发现 L 不包含某个实际 peer-visible masked frame，则先精确修订 L/消息合同再复核。无需重做已接收的 S12/S14 功能阶段。

本轮 git checkout 基于 `c3926c68fd14f270faa8b55234311071947fa080` 的隔离分支。提交前检查命令与文件哈希见配套 manifest。无推送、合并、PR 操作；`VFSS-baseline/`、论文、密钥、材料包和本机原始构建证据均不在差异中。
