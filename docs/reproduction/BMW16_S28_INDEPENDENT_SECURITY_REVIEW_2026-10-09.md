# BMW16 S28：独立安全复核与有界功能运行记录

日期：2026-10-09

接收分支：codex/m6b-i-bmw16-s28

身份：Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 1. 被审 revision 与隔离

| 对象 | revision |
|---|---|
| S27 文档接收提交 | 8bbf7119470bbc37f6950a3b425255ed993eaa8d |
| S26 被审最终源码 | 2818bce20f30719eaccf0bc5df586cce4fe78c84 |
| S26 runtime 修复源码 | ded433d636a3df929380bdce0ff22f4ce6121453 |
| main / origin/main / merge-base | c3926c68fd14f270faa8b55234311071947fa080 |

从 S27 精确提交建立隔离 worktree。起始 worktree 干净；桌面主工作区预存改动没有覆盖或纳入。VFSS-baseline、Papers、Agarwal_TopK、ADSMPC、CipherGPT、PR #28 与 S25/S26/S27 原 worktree 未修改。当前工作不含 secure alias，也未向 main 合并。

S27 是文档提交，runtime 源码与 S26 相同。本轮从 S26 runtime 源重建并运行；没有把 S27 文档 revision 说成被测源码 revision。

## 2. 修改前源码身份

S28 首次编辑之前，下列运行时源码与 S26 一致：

| 文件 | SHA-256 |
|---|---|
| VFSS/ext/FSS/dcf.cpp | 0EEB1926380726EFC5BE84B4B49E0ACB8C9636D0D6181469EDF505C1921B5DAC |
| VFSS/ext/FSS/include/FSS/dcf.h | 80904E0518BD75772D97984F2D48EEAE7CBD805FD119156AB7B9097082AB3F47 |
| VFSS/ext/FSS/include/FSS/keypack.h | 26597698719B62A6ADECD54194E4A36DB39DB78FEB7F3F0AE30AF13252E991BB |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23 |
| VFSS/src/moe_topk/protocol_i_score_input.cpp | 012EC8D85DAF19365FA75FACA94D31C2149E4EF6D79A4CDBD137957A65DD7AA7 |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | BA14008D28964E49965C75E2675E8726193C50991247BDA668F3398A01144BDE |
| VFSS/src/moe_topk/experimental_bmw16_select_party.cpp | A5F61E04F0BE8A6722A579F918BCE38C94069FD231B235916F1D4AB0A61301CE |
| VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp | 195162D7061C61F8FE6DE519F052CF553E4DB366090BFA71C279F64EDFBA782F |

E2E harness 最终 SHA-256 为 A54DE809C3B7CBCF838484A2580CA0D8A8A2F2062DB40F718CEA972E44F2F4D0。唯一改动是在 T 子进程 exit 非零时打印隔离 harness 临时目录中的 child log。新 TEST_ONLY checker 的最终 SHA-256 为 1833336F50F4506F4E01B6CA46F5719950863D3B1473CC642E60B44A78E23CC7。两项均未改 party/runtime 或材料 ABI。

## 3. DCF 来源桥接复核

### 3.1 参数、函数和序列化

参数域为 Bin 34..53、Bout 64、groupSize 1。KeyGen 阈值 alpha∈Z/(2^b)，payload beta∈Z/(2^64)；输入 MSB-first。keyGenDCF 的 greaterThan=false 两方输出和为 D_alpha(x)=beta·[x<alpha] mod 2^64。

完整 M2UC v1 party key 是公开头、三段数组长度、该方 root/correction block 数组 k[0..b]、共享终端 g[0] 和共享 v[0..b−1]。Wire 长度为 57+24b bytes。M2UC 未绑定 session、slot、端点、round；这些字段在外层 manifest/AAD。认证封装只保护字段来源/完整性，不证明 KeyGen 确实以该 slot 的 alpha/beta 发材。

源码位置：dcf.cpp:152–188 取两个 root blocks、互补 root control、MSB-first threshold；193–216 对两方状态按 plaintext 0/1/2/3 展开；219–242 计算 v 与累计量；244–266 生成高 126-bit seed correction 和两 control bits；269–284 生成相同的终端 g；87–148、294–335 为 Eval。keypack.h:8–23 记录 Bin/Bout/groupSize/k/g/v。protocol_i_ucmp.cpp:6、13–15 执行两次 Eval及 M2UC 编解码。

令 s_j 为 party j 当前 state、t_j=lsb(s_j)，alpha 位 a_i 按 MSB-first 读入；keep=a_i、lose=1−a_i。AES key 是 s&~3 的高 126 bits。AES 输出 0/1 是左右 child block，控制位取各 child LSB；输出 2/3 经 convert 截为 64-bit value。CW 高位是两方 losing child seed XOR；两个 control correction 分别为两侧 child control XOR 加目标 bit 校正。sign epsilon=(-1)^t1。

当前生成等式在 Z/(2^64) 为：

\[
v_i=\epsilon(-A_i-V_{0,lose}+V_{1,lose}+[keep=1]\beta),
\]
\[
A_{i+1}=A_i-V_{1,keep}+V_{0,keep}+\epsilon v_i.
\]

Eval 对 x 的每位走相同 branch，应用 CW control correction，累加 party sign_j·(V_child+t_previous v_i)，party0 sign=+1，party1 sign=−1；terminal 项为 convert_64(s_final&~3)+lsb(s_final)g，再按 party sign 输出。 uCMP 使用同一 key 对 x=(zl−zr) mod 2^b 与 y=(x−2^(b−1)) mod 2^b 运行两次 DCF Eval，按 party 符号合成 strict-less arithmetic share。其应用正确性要求真实键差落在半环不歧义范围；稳定键宽度和 dummy 类别比较约定满足该接口前提。

S24 的字段直映断点仍有效：M2UC shared seed correction 不逐字段等于 BGI15 两份独立 target correction。S28 没有以相同功能或测试通过作为论文算法等价证据。

### 3.2 完整 key 的理想单方分布

S27 的关键递推独立复算如下。把每个隐藏 node state 的有效 AES seed 看成 126 bits，control 为低位；理想 G126 独立输出 child seed/control 与 value。给定被腐化方 root、其自身扩展和已有 serialized prefix：

- losing hidden child seed 的高 126 bits 一次性遮蔽 CW 高位；
- 两个隐藏 child control 以独立位遮蔽两个 CW correction bit。公开 CW 后该方可计算 peer child control，所以这两位不被宣称为秘密；
- losing value 以单位系数遮蔽当前 v_i；
- keep seed 是同轮 G 的独立坐标，经过由控制位/CW决定的已知 XOR correction 后仍均匀，支持下一层归纳；
- 最后一个 hidden keep value 以 ±1 进入终端 g。终端 seed 的 convert 输出不能用作“64-bit uniform”论据，因为 s&~3 已清除控制位。

结果是在理想 G126 模型中，完整单方 key（不是只看输出和）由固定头、均匀 128-bit root、b 个均匀 128-bit CW、b 个均匀 64-bit v、一个均匀 64-bit g 构成；参数 alpha/beta 不改变此分布。两个 party keys 的 correction/value/terminal 字段相同，因此不能称两方 key 独立。

测试覆盖口径修正：最初 checker 计入有效 seed 和 control，但没有枚举 serialized root block 中不参与 AES/control 的第二低位。此位虽不影响归纳，却在 key 字节串中存在。S28 将它纳入 root tuple 后重跑：4 种阈值/payload组合，两 party，各 25,165,824 个条件化 ideal-tape case；每方 tuple 为 1,024 种，每种频数 6,144。该修正只完善理想 toy checker，未改 DCF。

这仍不是 AES 安全证明。实际 AES key 限制在 encode(S||00) 的 126-bit 子族。计算性桥接需假设该 restricted-key family 对四个固定 AES 输入具备 PRP/PRG 安全；一般均匀 128-bit AES PRP 声明本身不推出它。若采用 restricted-key PRP，4 点 PRP/随机函数切换项至多 6/2^128，AES 优势保留为符号；并需 cryptoTools AES-CTR root stream 与 OS getrandom 假设。安全位数没有编造。

### 3.3 根随机带和全池槽数

实际 KeyGen 每次从 FSSConfig::prngs[omp_get_thread_num()] 取两个 128-bit blocks。T 的当前发材调用是串行，因此 KeyGen 消耗 q0=2N_DCF、其他线程 q_t=0。seed_fss_once 初始化 256 条 cryptoTools PRNG，OS 为每条取 seed；SetSeed 预填每条 256-block buffer。该 65,536-block 启动计算不是 keygen 已消费 root blocks；active stream 在 buffer 用尽时会 refill。S28 没有运行时 PRG/AES 计数器，q_t 和 AES 工作量是源代码布局推导。

N_DCF=2p+9C，其中 C=n(n−1)/2：2p 是 raw carry/sign；8C 是两个 Select task ×四个比较层；C 是 membership。offline_material_slots_per_party=2p+2+9C，额外两个是 forward/inverse shuffle slot。n=1 走 shortcut，不调用此 generator；因此下表只列 n≥2：

| n | p | C | N_DCF | q0=2N_DCF |
|---:|---:|---:|---:|---:|
| 2 | 2 | 1 | 13 | 26 |
| 3 | 4 | 3 | 35 | 70 |
| 5 | 8 | 10 | 106 | 212 |
| 8 | 8 | 28 | 268 | 536 |
| 64 | 64 | 2,016 | 18,272 | 36,544 |
| 128 | 128 | 8,128 | 73,408 | 146,816 |
| 256 | 256 | 32,640 | 294,272 | 588,544 |

每一 comparator key 用两次 Eval，不是两个 key。源码显示每 task/round/canonical pair 生成新 key；membership 使用另一新 key；没发现跨 task/round/edge/membership 重用。

全池 hybrid 在根流替换后，条件于完整但相关的 alpha/beta vector 逐槽替换每把 corrupt party key 的 b 个隐藏侧 G 输出。single-key 归纳对每个固定参数成立；其它槽可由 reduction 以固定参数向量产生，unused keys 在 online transcript 之前即作为完整池生成。实际打开 bit 下 local Eval share 是本地 key 和公开 operand 的确定后处理，peer share 是 bit−local mod 2^64。自适应后续槽只依 L 中已产生的 transcript/coin/order 选择。ideal functionality 自己计算并交付 L；模拟器不能拿 real execution L，否则会循环假设。

界记为：

\[
Adv_{pool}\le Adv_{root}((q_t)_t)+N_{DCF}bAdv^{aux}_{G126}
 + {2N_{DCF}b\choose2}/2^{126}.
\]

其中 G126 假设需要覆盖固定参数、其它状态和前缀作为 auxiliary input；最后项是排除 2N_DCF b 个状态输入碰撞的保守 birthday 项。本式是条件界，不提供具体安全位数。

## 4. 泄露 L 与 shuffle/output

候选 L 包括公开 session/config/channel；raw adapter 的 carry/sign masked operands；forward/inverse public_z 与 masked vectors；anonymous comparison endpoints 和 opened bits；sampling/pivot/U/V/W 匿名位置；selected anonymous handle；slot/key IDs、访问顺序、比较次数、phase/frame length；双方 XOR 的 sampler seed；统一算法 abort 状态及公开 transcript 可推测的 abort 阶段。L 不包含 raw score、original_index、真实 index↔anonymous handle 映射、真实 rank、selected stable key 或完整明文 mask。

每方 view 包含本方 raw input share、完整已用/未用材料 key pool、local shuffle factors、self/peer message frames、本地 Eval share、最终 output share 与本地状态。材料/TLS/OS/不变量故障不等于概率 abort；断连无 ACK 时仅报告 LOCAL_ONLY。

shuffle 源码生成 π、σ0、σ1，τ0=π∘σ1⁻¹、τ1=π∘σ0⁻¹；独立均匀 a0,a1,h,r0,r1；e0=−τ0(a1)−h、e1=−τ1(a0)+h。party b 发 m_b=σ_b(x_b)+a_b，再发 q_b=τ_b(m_peer)+e_b+r_b。故 q0−r0=πx1−h、q1−r1=πx0+h，合并 z=π(x0+x1)+r0+r1。inverse 以 fresh factor/mask 和 π⁻¹处理 membership shares。

给定 corrupt party 的理想 output share y_b，inverse simulator 对每 record 抽均匀 peer round1 frame m_peer，再抽本方中间 w_b，使 word0 LSB=y_b、其余有效位均匀；置 e_b=w_b−τ_b(m_peer)，选 fresh r_b，构造 q_b=w_b+r_b，peer q 由公开和 z−q_b 得到。真实 output share 就是 LSB(w_b)；fresh h/peer a/peer r 提供必要的 conditional uniformity。这模拟的是 output-share 与 messages 的联合条件分布，不交付 π、对方输入映射或完整 mask。

两 Select task 共享同一匿名真实句柄；模拟器不获 π。该为项目构造的条件模拟不借 Protocol I 论文 shuffle 定理，且要求 ideal T、独立随机 masks/permutations、OS 熵和 L 完整。有限枚举仅找反例，不是一般安全证明。

## 5. 抽样 abort

双方各从 getrandom 取 32-byte contribution，交换后 XOR；对 seed/task/domain/counter 运行公开 SHA-256 counter 与 rejection sampling。task/domain/counter 分离，双方 contribution 在抽样前取得。S16 历史脚本从原 revision 重执行：理想无放回模式 n≤256 的最大 union bound 为 1.3105804606611235e−11，公式域至 n≤10^6 为 5.065679989546104e−8。随机预言机模型且至少一个诚实 OS entropy contribution 时可条件性套用。标准模型裸 SHA-256 counter 不是 keyed PRF，本项目没有证明其抽样流与均匀无放回相近。自然 abort、注入 abort、工程错误、材料错误和 peer 错误分别记录；0 次自然 abort 仅是本轮观察。

## 6. 构建、命令与证据

环境为 WSL Ubuntu 24.04、Linux 6.6.87.2-microsoft-standard-WSL2 x86_64、CMake 3.28.3、GCC 13.3.0、OpenSSL 3.0.13。Release 采用实验 adapter ON、DCF counters OFF、TEST_ONLY failpoints OFF。E2E 使用另一个专用 TEST_ONLY failpoint-ON 构建。二进制和结果都留在 /tmp 与 .codex/artifacts。

构建命令：

    wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s28/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s28_review -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF && cmake --build /tmp/moe_bmw16_s28_review --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test --parallel 2'
    wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s28/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s28_test -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON && cmake --build /tmp/moe_bmw16_s28_test --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test --parallel 2'
    wsl.exe -d Ubuntu-24.04 -- /tmp/moe_bmw16_s28_review/moe_topk_m2_ucmp_conformance_test
    wsl.exe -d Ubuntu-24.04 -- /tmp/moe_bmw16_s28_review/moe_topk_m2_parallel_shuffle_conformance_test
    wsl.exe -d Ubuntu-24.04 -- /tmp/moe_bmw16_s28_review/moe_topk_bmw16_experimental_party_test
    py -3 experiments/TEST_ONLY_BMW16_S28/ideal_dcf_one_level_check.py
    py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s28_review/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out C:\Users\28641\.codex\artifacts\BMW16_S28_20261009\run_01\common_tape
    wsl.exe -d Ubuntu-24.04 -u root -- /tmp/moe_bmw16_s28_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s28_test/moe_topk_bmw16_experimental_party_node --tls

| 验证目标 | 结果与身份 |
|---|---|
| moe_topk_m2_ucmp_conformance_test | exit 0；binary SHA-256 18cd538f1e33a0177e1fb4a95f14b0670e3e0ee7498adf8b2d79197cd58ea950 |
| moe_topk_m2_parallel_shuffle_conformance_test | exit 0；binary SHA-256 1d29afb194c62d9c23cd777ac9af09b9e8bc5831d167010def631d5953ac3692 |
| moe_topk_bmw16_experimental_party_test | exit 0；n=1 shortcut、n=5/8、n=64 样本与 abort negative；binary SHA-256 f2250060f8c30ae4cd34b97d72b638708491841ea2e7d3f82b1b78a0df134942 |
| ideal_dcf_one_level_check.py | 25,165,824 ideal tape case PASS；1024 tuple 各频数6144；不是密码学证明 |
| common-tape differential | 39/39 PASS；matrix SHA-256 ED383DF5CB2B8D1C5B5A6601B95B8130A8C04931AC032DA0E6513A52CF824F86 |
| fresh T/P0/P1 TLS E2E | exit 0；n≤256；成功日志 SHA-256 C1999EC049525B9787F7B54139277C63DF16D8362D3E4F83E337CCF33A15780C |

E2E 正常配置：n=1/K=1 shortcut；n=2/K=1；n=3/K=2；n=5/K=5；n=8/K=4；n=64/K=8；n=128/K=128；n=256/K=2。fresh material，每例的隔离 oracle 检查长度 n、0/1、恰 K 和原序逐位一致。负例包括错身份/party/session/stream、重放、坏/截断 sidecar、T prepare/commit fault、receiver publish/fsync/silent、peer close/silent、注入 algorithm abort、工程 fault、missing TLS stream、最终状态不一致和 mask publish failure；失败无 mask，失联无 ACK 只记存活方 LOCAL_ONLY。不同 UID 与 loopback TCP+mTLS 是同主机试验，不是跨主机安全部署/性能。自然概率 abort 0 次，不用注入样本估计它。

首个 E2E 用 failpoint-OFF Release node，harness 调用的 test-only t-local-test-only 不存在，child exit=64；usage 表明是构建开关不匹配，尚未进入协议。换专用 TEST_ONLY 构建后运行成功。这个失败保留，不计作功能失败或成功样本。

| 二进制 | SHA-256 |
|---|---|
| Release party-node | 5d8033b29f6e8aa9b8cc2ccff0262564b342aab8f19b1be2d7f24baf5a89d52e |
| TEST_ONLY party-node | bea403fb8ee8e39460fde3542354e289af25e2c91193f4d4c0038a7cb0949b5c |
| TEST_ONLY E2E harness | acf4520d9a65911bd26b5deca8aa3ee277ffe8ce6bc4a2fab327ee089109260d |

其他外部文件：party_conformance.log SHA-256 A728422C67680797DAE386CB408B8BA0D2817FDB48CB587967FCFE37073AF976；ucmp_conformance.log 与 shuffle_conformance.log 均为 D6A604A05C5461E56CD71451B3B212F0BF0D7E7B0487C483D9D6A51208759666；failpoint-OFF 初次失败 log SHA-256 5133B2799844E188793012968EF0483A595AB2F2C392EAA3FC656DCD049FB356；child usage 诊断 log SHA-256 E9EBC36D44162BB46353F1024CFC7D567CB9B6E0960E51A41DFBFC628CEA2694。完整 common tape 子文件均保存在外部 run_01 路径。报告不复制 raw tape、shares、materials 或身份私密数据。

外部证据目录递归 SHA-256 索引：`C:\Users\28641\.codex\artifacts\BMW16_S28_20261009\sha256_index_run_01.txt`，205 项，索引文件 SHA-256 `DB9E84DF8E8B2E6FC1E36AC788B69D60743429D8C2CD518D93A234D5EC9F76F1`。索引覆盖 run_01 下逐次结果与复跑文件；不把这些大日志或秘密材料放入 Git。

full CTest NOT_RUN，只执行上述定向 binary。S26 复核曾记录完整 Release 构建被无关的冻结外部目标 `bitpack_test` 阻断，链接错误为 `undefined reference to bitpack::mod(unsigned long, int)`，导致 38 项 CTest Not Run；S28 未重跑全仓构建，故这是历史构建障碍，不能算本轮复现或通过。runtime/material ABI 未变，所以本轮不生成 n=1000 多 GB fresh materials；最终 S28 n=1000 NOT_RUN，S22/S26 历史结果仅属各自 revision。正式 LAN/WAN、总时间、PRG 调用实测、九指标一律 NOT_MEASURED。

## 7. 门禁和下一接收

| 门 | 结果 |
|---|---|
| DCF_SINGLE_KEY_PRIVACY | CONDITIONAL：G126 source-specific proof + restricted AES/AES-CTR/OS assumptions |
| ADAPTIVE_FULL_POOL_VIEW | CONDITIONAL：related thresholds、unused keys、same-key two Eval、adaptive L 下的 pool hybrid |
| SHUFFLE_OUTPUT_COMPOSITION | CONDITIONAL：source equation 与 output-share conditioned inverse simulator |
| SAMPLER_ABORT_GUARANTEE | CONDITIONAL：理想无放回/ROM 下；standard-model SHA-256 counter 未证明 |
| FUNCTIONAL_REGRESSION | PASS（有界至 n=256；不代表安全或全规模） |
| SECURE_ALIAS_READY | UNPROVEN；无 secure alias |

下一接收者应复核 G126 auxiliary-input 假设、restricted-key AES 条件、碰撞项、线程 q_t、ideal F 生成 L 的方向、所有 message 字段、inverse output-share conditional simulator 和真实 OS randomness。production credentials/remote deployment、标准模型 sampler、正式 V4 顺序与 LAN/WAN 矩阵继续独立。用户要求的并行 S26 接收任务已在另一线程完成：报告提交 32f7f0b 并推送到独立分支；Draft PR 未创建，GitHub connector/browser 流程不可用。该候选没有合并到 main。
