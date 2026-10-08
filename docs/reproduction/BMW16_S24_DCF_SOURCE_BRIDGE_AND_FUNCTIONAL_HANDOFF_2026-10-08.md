# BMW16 S24：DCF 源码桥接与功能候选交接报告

日期：2026-10-08<br>
方案：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**<br>
S24 起点：7b781168161a488b156353acf1623100fe59b1f7（S23 文档提交）<br>
被审运行时来源：9210c11a7a877f8fe3687e0509c1722440cf63fe（S22）<br>
分支：codex/m6b-i-bmw16-s24<br>
工作树：C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s24\MoE_Top-k_Reproduction_Evaluation

## 执行摘要

S24 聚焦 S23 留下的首个缺口 DCFCodeEquivalentToBGI15GenEval。独立追踪当前 VFSS keyGenDCF/evalDCF 后，可从源码递推证明重构输出为

$$
F_{a,g}(x)=g\,[x<a]\pmod {2^{64}},
$$

其中输入按 MSB-first 读取、group 为 Z/(2^64)、本方案 groupSize=1。这闭合了**函数正确性解释**，没有闭合完整 party key 的隐私分布桥接。

具体阻断点是内部随机变量的首个直接对应：BGI15 Algorithm 5 为 target child 独立抽取两份 seed correction；VFSS 每层只序列化一个 common scw，并由互补 control bits 门控。若将 BGI 两份 target correction 直接识别为 Tβ·scw，则总有一份为零；BGI 的独立均匀对仅以 2^(1−λ)−2^(−2λ) 概率满足“至少一个为零”。这精确否定**逐字段直接映射**，不构成针对压缩 VFSS party key 的区分攻击。尚无另一种分布保持重编码或 source-specific 单 key 函数隐私归约。

因此：

- DCF_CODE_CORRESPONDENCE：**NO-GO**，不能把 BGI15 Theorem 6 直接转移到当前 ABI。
- SINGLE_KEY_PRIVACY_FOR_ACTUAL_SERIALIZED_KEY：**UNPROVEN**；此结论不等于证明实现不安全。
- ADAPTIVE_FULL_POOL_REDUCTION：**CONDITIONAL**，不能把 S23 的 N_DCF·ε_DCF 草案写成当前安全 PASS。
- EXPERIMENTAL_FUNCTIONAL_HANDOFF：**PASS（有界，运行时身份为 S22；S24 只新跑了定向回归）**。
- SECURE_ALIAS_READY：**NO-GO**。不创建 secure alias。
- 正式 V4/LAN/WAN/六方案性能：**NOT_RUN / NOT_MEASURED**。

S24 没有修改 VFSS runtime、party API 或材料 ABI。新代码只是一份独立 TEST_ONLY 源码转录模型；功能候选仍保持 opt-in 和 EXPERIMENTAL。

## 1. 接收基线、源码与证据身份

| 项目 | S24 核验 |
| --- | --- |
| 起始 HEAD | 7b781168161a488b156353acf1623100fe59b1f7 |
| 起始分支 | codex/m6b-i-bmw16-s24 |
| 起始差异 | 新 worktree 起始干净；本报告/模型/决策及状态同步是在之后新增 |
| main | c3926c68fd14f270faa8b55234311071947fa080 |
| origin/main | c3926c68fd14f270faa8b55234311071947fa080 |
| merge-base | c3926c68fd14f270faa8b55234311071947fa080 |
| S23 身份 | 文档与复核提交，不是新 runtime revision |
| 实际 DCF/Protocol I runtime | S22 9210c11a7a877f8fe3687e0509c1722440cf63fe |
| S24 修改运行时代码 | 否 |
| 本机论文副本 | 当前隔离 worktree 未包含 BGI15 PDF；使用 IACR 官方 PDF 核对定义及算法页，不复制或提交 PDF |

被审源码 SHA-256（S23 索引中登记值；S24 对 worktree 文件复算一致）：

| 文件 | SHA-256 |
| --- | --- |
| VFSS/ext/FSS/dcf.cpp | 0EEB1926380726EFC5BE84B4B49E0ACB8C9636D0D6181469EDF505C1921B5DAC |
| VFSS/ext/FSS/include/FSS/dcf.h | 80904E0518BD75772D97984F2D48EEAE7CBD805FD119156AB7B9097082AB3F47 |
| VFSS/ext/FSS/include/FSS/keypack.h | 26597698719B62A6ADECD54194E4A36DB39DB78FEB7F3F0AE30AF13252E991BB |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23 |
| VFSS/src/moe_topk/experimental_bmw16_select_party.cpp | A5F61E04F0BE8A6722A579F918BCE38C94069FD231B235916F1D4AB0A61301CE |
| VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp | E2FCA7497426ACCBFB4B2DE44044C2721C64C8EEA32686F1A3A1E9C62E058F1D |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | BA14008D28964E49965C75E2675E8726193C50991247BDA668F3398A01144BDE |
| VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp | 665C8072BBC3A3FC19CB500720F6AD6214C8942B581998A24F4C8C05F993EF27 |
| VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp | 84BEE8DFF57F8D7B647CF5F07CFE5EED32CEF97A7DB29004CC1726F5FFA98CCA |

S24 新增/同步文件 SHA-256（本报告自身 hash 由仓库外证据索引记录）：

| 文件 | SHA-256 |
| --- | --- |
| PROJECT.md | DCCC5179E2855139CBCF41E399EADB6D800B6EABE18079949EFE610587FD15DE |
| docs/IMPLEMENTATION_PLAN.md | 88910CD121E7FEF26346C9F99361C0779FDBE4ED03DF153D9247DB1E41607EFB |
| docs/decisions/BMW16_S24_DCF_SOURCE_BRIDGE_DECISION_2026-10-08.md | 9A9E89AF4A85F46742BDE1E462E723B63E907F7D8B39E0C411890E0785E8A73B |
| docs/reproduction/BMW16_DERIVED_EXPERIMENTAL_FUNCTIONAL_HANDOFF_2026-10-08.md | EE7BC5F5DBF0C47819A8A23866FD6E41F85BA8303D98C12ECA8059C89DF7C97B |
| experiments/TEST_ONLY_BMW16_DCF_BRIDGE/toy_source_dcf.py | 962F036C6C17EF16536D1F2189AB2064183C7AD1778C6FA59B0E7152B7296577 |
| experiments/TEST_ONLY_BMW16_DCF_BRIDGE/test_toy_source_dcf.py | 3A3B1683811A259A8796E55BE71D1682DE2013EAAF6D9FBBA481A31C1C5D6394 |
| experiments/TEST_ONLY_BMW16_DCF_BRIDGE/README.md | 3BC3BD99D62FCA06E09B592171C037C9E2849E7F59E520437E8070E118FA5E8F |

## 2. BGI15 来源与证据类别

主要纸面依据为 Boyle, Gilboa, Ishai, Function Secret Sharing, EUROCRYPT 2015，IACR 官方 30 页 PDF（2026-10-08 核对）：[官方 PDF](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf)。PDF 未下载到仓库；本 worktree 不含 Papers 副本。

| 证据项 | 原文位置 | 用途 |
| --- | --- | --- |
| FSS 函数隐私定义 | Definition 2，PDF p.7 / 印刷 p.7 | 完整 party key 的函数隐私游戏；同域函数挑战 |
| 比较 DCF KeyGen | Algorithm 5，PDF p.19 / 印刷 p.19 | root、分支扩展及 seed/control/value correction |
| 比较 DCF Eval | Algorithm 6，PDF pp.19–20 / 印刷 pp.19–20 | MSB-first 路径，party 符号和输出 share |
| 正确性及函数隐私 | Proposition 1 / Theorem 6，PDF p.20 / 印刷 p.20 | 只对论文所定义的 scheme 构造引用 |

下文将结论分为 PAPER（论文定义/公式）、SOURCE（当前 C++ 实际行为）、PROJECT_DERIVED（当前 uCMP/Select 组合）、UNPROVEN（尚未建立的密码学桥接）。相同的重构函数不自动意味着完整 key 分布相同。

## 3. 当前实现的逐变量映射

源码位置为 VFSS/ext/FSS/dcf.cpp：convert 60–85、traverseOneDCF 87–126、traversePathDCF 130–148、keyGenDCF 152–284、evalDCF 294–335；uCMP wrapper 为 VFSS/src/moe_topk/protocol_i_ucmp.cpp:5–15；通用 key-pack 结构在 VFSS/ext/FSS/include/FSS/keypack.h:8–22。

| 变量/行为 | BGI15 公式与角色 | VFSS 实现 | 等价判断及边界 |
| --- | --- | --- | --- |
| 输入和函数 | Alg.5/6 的 threshold a、输入 x，输出 g iff x<a | keyGenDCF(...idx,payload)，greaterThan=false | 递推输出函数相同；仅函数层 |
| 根 seed/control | 每方起始 tree state | 线程 PRNG 取两个 block；s[0]=(s[0]&~1)^((s[1]&1)^1)；后续 AES seed 用 &~3 | root control 互补；seed/control packed 在 block 中 |
| 左/右 seed/value 扩展 | G(s) 含两个子 seed、控制位和值 | 对 state&~3 用 AES 加密 counter 0/1/2/3；前两块作左右 child，后两块作左右 value | 需要把受限 AES-key 的有效截断扩展视为 PRG；不能仅由接口名推定 |
| 输出宽度 | G 要求 2λ+2log|G|+2 bits | 每个 AES 扩展计算 4×128 bits；child 的两组 seed/control 有效 254 bits，value 两组各取 64 bits，共 382 有效 bits | 当 λ=126, |G|=2^64 时公式为 382 bits；多余原始 AES 位丢弃 |
| target prefix | Alg.5 令路径 prefix 控制位互补 | keep = idx>>(Bin-1-i)；MSB-first | 位顺序吻合比较树 |
| seed CW | Alg.5 有左右/两方 correction 数据 | scw=(si[0][lose]^si[1][lose])&~3，同一 scw 存入双方 k[i+1]；低两位另存 tL/tR | 状态递推可逐式核对；BGI 的两份 target seed-CW 不可直接逐字段映射 |
| control CW | 调整目标边互补、非目标边收敛 | tL=(lsb(si0L^si1L)^keep^1)；tR=(lsb(si0R^si1R)^keep)；Eval 取入边位 | 源码状态不变量与 DCF 功能构造相符 |
| seed 更新 | 每方在 control=1 时异或相应 correction | sβ←siβ_keep XOR (Tβ ? (scw XOR t_keep) : 0) | target control xor=1；离开目标 prefix 后状态收敛 |
| value CW | BGI 每层以 group 加法校正 value share | σ=(-1)^t1，v_i=σ(-vα−v0_lose+v1_lose+[keep=1]g)；更新 vα=vα−v1_keep+v0_keep+σv_i | 模 2^64 的功能递推已由式子闭合；完整 key 分布另论 |
| Eval node share | BGI 按 control 校正并用 party 符号 | ct=AES(state&~3,2*keep)；每层累加 (+1/-1)·(q(ct_value)+T·v_i) | SERVER0 正号，SERVER1 负号 |
| terminal g | BGI 的 Eval 叶项/累计值 | g0=q(s1&~3)-q(s0&~3)-vα，若 lsb(s1) 则取负；Eval 加 q(leaf)+T·g0 后乘 party 符号 | 代数抵消给出严格阈值函数 |
| group | 有限 Abelian group | uint64_t 加减溢出环 | Z/(2^64)；不是 XOR 输出 |
| uCMP 调用 | 非 BGI15，属于项目组合 | α=(ml−mr) mod2^b，x=(zl−zr) mod2^b，y=x−2^(b−1)；同一 key 对 x,y 各 Eval 一次，P1 对 y≥2^(b−1) 修正 | 合法范围下重构 [left < right]，应视为 PROJECT_DERIVED 的 wrapper 语义 |
| 参数和完整 party key | Theorem 6 覆盖整个 Gen 输出 key | Bin=34..53,Bout=64,groupSize=1,payload=1；M2UC/version1/party/Bin/Bout/groupSize/3 lengths/k[b+1]/g[1]/v[b]；长度 57+24b bytes | 873–1,329 bytes/party。安全游戏必须覆盖序列化全字段，不能只看 k 或 Eval 输出 |

### 功能递推

每层 Eval 的两方重构项为

$$
q_0(x_i)-q_1(x_i)+(T_0-T_1)v_i.
$$

在阈值前缀上 T_0−T_1=(-1)^(T_1)=σ，所以累加量是 vα。若首次偏离阈值的位使 keep=1，lose-left 项结合先前累计量留下 payload g；若 keep=0 留下 0。首个失配后 seed/control 收敛，余下节点项相消。精确命中阈值时 terminal g0 叶项抵消累计值。因此输出 g[x<a]；输出宽度 64 时在 Z/(2^64) 取值。

这段推导加上 TEST_ONLY 小字宽逐层轨迹测试支持当前实现的**函数语义**。它不能证明任何 party key 的分布隐私。

## 4. 第一个未闭合变量关系：精确事件

BGI15 Alg.5 在 target edge 相关步骤独立均匀抽取

$$
c_s^{0,target},c_s^{1,target}\leftarrow\{0,1\}^{\lambda}.
$$

最直接的 VFSS 变量识别为

$$
c_s^{\beta,target}=T_\beta\cdot scw,\quad T_0\oplus T_1=1.
$$

因此该映射下事件 E=[c_s^{0,target}=0 或 c_s^{1,target}=0] 的概率是 1；若 BGI 的两值独立均匀，

$$
Pr[E]=2·2^{-\lambda}-2^{-2\lambda}.
$$

当 toy λ=4 时后者为 31/256，差为 225/256。这就是 DCFCodeEquivalentToBGI15GenEval 的**第一条无法按字面成立的变量关系**。

限定：当前 serialized key 使用 common packed correction word，BGI 内部变量未必需要逐项显式出现在该序列化中。此反例仅说明一种朴素变量命名会改变 distribution；没有证明两种压缩 key 分布可区分，也没有证明当前 scheme 不安全。仍需要以下之一：

1. 对完整 serialized ProtocolIUcmpPartyMaterial 给出 source-specific single-key 函数隐私归约，精确覆盖 k/g/v、根随机流和 Eval；
2. 构造从 VFSS key 到 BGI key 的可验证、分布保持编码；
3. 改用独立 versioned BGI15 Gen/Eval key ABI，并重做 uCMP/material/conformance/E2E。

S24 未改 key ABI，因为没有证明某个变量映射修复足以关闭真实安全门。

## 5. PRG 与整池归约边界

- **节点扩展假设**：要假设 AES_encode126(seed)(0,1,2,3) 经保留字段截取后，对 126-bit seed 族构成输出 382-bit 的安全 PRG。AES key 的低两位固定/保留，不能不加说明地代入均匀 128-bit AES-key 假设。源码可数每层 KeyGen 两次 ecbEncFourBlocks、Eval 一次 ecbEncTwoBlocks；这不是全路径 PRG 或 AES block 的实测计数。
- **根随机流**：seed_fss_once 使用 OS getrandom 为各 stream 初始化 128-bit 种子；cryptoTools PRNG 以 AES counter 扩展。每次 DCF keyGenDCF 消费两个 blocks（32 bytes）。需把该流替换为独立均匀带时，优势保留符号项 Adv_PRG(Q_root)，不报告无依据安全位数。
- **单 key 条件**：只有当前完整 party key 在给定允许辅助输入时有 ε_DCF 隐私，逐 key hybrid 才能作为前提。把所有其他 key/阈值作为辅助输入、挑战 key 独立 KeyGen、已打开输出与后续 transcript 一致地模拟，当前尚无有效归约。
- **数量口径**：令 C=binom(n,2)，N_DCF=2p+9C 是 DCF KeyGen pair 调用数；Select/membership stream sidecar slots 为 9C；offline_material_slots_per_party=2p+2+9C 另含 raw 与两个 shuffle stage。相同 uCMP slot 的两个 Eval 不等于两份 DCF key；跨用途/round 使用独立 key。不能将三种计数混用。
- **整池结论**：只有 key 隐私 ε_DCF、随机流可替换、并且 simulator 能从腐化方输入/理想输出与候选泄露 L 生成同一 transcript 条件分布时，逐槽 hybrid 才可条件性写作 N_DCF·ε_DCF + Adv_PRG(Q_root)+Adv_PRG(Q_node)。实际 ε_DCF 尚未建立；公开 transcript/输出 share 和相关阈值又必须联合模拟，所以该式目前不是安全结论。
- **shuffle 组合**：S24 不重做 Protocol I shuffle 的 σ,τ,a,e,r 模拟；ProtocolIForwardInverseViewSimulator_L 仍为独立未证项。不能从 DCF 源码桥接推出 forward/inverse shuffle、两个 Select 共用 handle、mask 输出与 abort 的单方联合 view 安全。

## 6. 功能候选接口及适用范围

功能接口与角色合同见 [BMW16-derived 实验功能交接](BMW16_DERIVED_EXPERIMENTAL_FUNCTIONAL_HANDOFF_2026-10-08.md)。其中：

- T 输入只含公开 session/fingerprint/n/K/宽度等配置，在线 raw score/share 生成前完成离线 KeyGen、party package/slot 发布并退出；不能得到在线 score、rank 或结果。
- P0/P1 接正式 raw-score uint32_t 加法份额，在现有 party API 中运行 adapter、同一隐藏置换 shuffle、两份并行 Select 的 R1–R4、DCF-backed membership、同置换 inverse，输出各自原序 XOR mask share。
- TEST_ONLY 控制器才可构造冻结输入、启动角色并重构 shares 核 oracle。Party 过程不重构原始 score、original_index 到匿名位置的映射、selected key 或完整明文 mask；按候选泄露合同公开的比较位对双方可见。
- mTLS 和离线 package/one-shot 属既有 runtime 交付合同；S24 未重测/未新验收这些合同。概率 ABORT、工程异常、材料错误、通信失败应分开，失联时不宣称双方达成 PEER_AGREED。
- 支持形状不能替代已测规模：当前接口代码可表示到 n≤2^20 且宽度不超 53 bit；S23 独立三进程接收实际到 n≤256。S22 曾在其历史源码/材料身份运行过一次 n=1000,K=80；这不是 S24 复跑或性能数据。

## 7. 本轮实测与可复跑命令

环境：WSL Ubuntu 24.04；GCC 13.3.0；CMake 3.28.3；OpenSSL 3.0.13；Release；CTest 与 TEST_ONLY failpoint 均打开，BMW16 adapter opt-in 显式打开。构建目录 /tmp/moe_bmw16_s24_build；全部二进制、命令日志和测试日志在仓库外 C:\Users\28641\.codex\artifacts\BMW16_S24_20261008。

配置：

~~~bash
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s24/MoE_Top-k_Reproduction_Evaluation/VFSS \
  -B /tmp/moe_bmw16_s24_build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON
~~~

定向构建：

~~~bash
cmake --build /tmp/moe_bmw16_s24_build --target \
  moe_topk_m1_dcf_conformance_test \
  moe_topk_m2_priority_dcf_conformance_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_bmw16_experimental_party_test -j2
~~~

定向 CTest：

~~~bash
ctest --test-dir /tmp/moe_bmw16_s24_build \
  -R 'moe_topk_(m1_dcf_conformance|m2_priority_dcf_conformance|m2_ucmp_conformance|bmw16_experimental_party_test)' \
  --output-on-failure
~~~

结果：4/4 通过，CTest 汇总耗时 0.66 秒。它不是全量 CTest，也没有重跑 E2E。TEST_ONLY 转录：

~~~bash
python3 experiments/TEST_ONLY_BMW16_DCF_BRIDGE/test_toy_source_dcf.py
~~~

结果：5/5 test 方法通过；检查 5,456 个 threshold/input/tape 输出、1,360 个 payload/zero function 输出、5,456 条逐层 control convergence/share accumulation trace，以及 λ=4 correction 事件。该模型固定根 seed 与有限独立 toy expansion table，不调用 VFSS/AES，不连接 party 代码；只佐证转录的源码代数和直接字段映射反例。

下列结论是**本窗口新跑**：Release 定向 C++ build、4 项 CTest、toy model 5 项。下列是**引用的历史独立接收**：S23 从 S22 runtime tree 的 common-tape、不同 UID TLS/non-TLS bounded E2E 到 n≤256。下列**没有运行**：S24 S4 differential、S24 三进程 E2E、n=1000 fresh materials、n≥1000 容量运行、全仓 CTest、正式网络/性能矩阵。没有把 S22 的 n=1000 历史单次运行当作 S24 证据。

二进制 SHA-256：

| 二进制 | SHA-256 |
| --- | --- |
| moe_topk_m1_dcf_conformance_test | 8e628d0499f9ef4eef5641777211517bb815b29cf2013a0c41e2580daeceb797 |
| moe_topk_m2_priority_dcf_conformance_test | f5ed9d71a653d174328f311a3dfeb64bbb76b83ff8a3f7a08d74babb63031b1a |
| moe_topk_m2_ucmp_conformance_test | aa1128d9dba085833e0e45bd10b5403f4e19060ca76b19c761b047113d359261 |
| moe_topk_bmw16_experimental_party_test | baed52f5e8144bc075bd2e5b62f1cc00f450858cf4d2ff08f0fcec9fdd4aecbf |

仓库外原始日志 SHA-256：

| 证据 | SHA-256 |
| --- | --- |
| cmake_configure_clean.log | 2471a5ad6730f0f735d3763ce71d5e870e917cc298343ae2c44e4db0ddb0c451 |
| build_targeted.log | 66dd71d87e346bcfab60861bc81882158bd6e569ddd5775be96ea27bb9043dab |
| ctest_targeted.log | 95f9dd9a7a58618dd990760ce99d9ecbe804f3a00f9f5664b712a38cf3348fdb |
| toy_test.log | 6e3f94a6d31e29455795c2c8bfce2c4ca28af14c06168fad3b25c6d52bcee6b5 |

CTest 与 toy 日志无输入 shares、party keys、凭据、材料 sidecar 或明文 mask。未记录敏感随机带。

## 8. 门禁和下一项工作

| Gate | S24 结论 | 原因 |
| --- | --- | --- |
| DCF_CODE_CORRESPONDENCE | **NO-GO（定理直接转移）** | 函数递推对应，但完整 Gen/Eval/key 分布不能由变量名类比；直接目标 c_s 映射有精确 support 差异 |
| SINGLE_KEY_PRIVACY_FOR_ACTUAL_SERIALIZED_KEY | **UNPROVEN / NO-GO** | BGI Definition 2 作用于完整 party key；需要 k/g/v 全字段的 source-specific 归约或分布保持编码 |
| ADAPTIVE_FULL_POOL_REDUCTION | **CONDITIONAL** | 必要条件是实际单 key ε_DCF、PRG 替换、给定同一合法 L 的输出感知自适应模拟；均未被本轮全部闭合 |
| EXPERIMENTAL_FUNCTIONAL_HANDOFF | **PASS（有界、继承 S23 的 E2E）** | S24 新 build 的 DCF/uCMP/party 定向 CTest 4/4；S23 历史独立进程 n≤256。S24 未重跑 E2E |
| SECURE_ALIAS_READY | **NO-GO** | 以上 DCF/full-pool 及 shuffle/output simulator 仍未闭合 |
| PERFORMANCE_READY | **NOT_READY** | 无正式性能计量；S24 CTest 时间不是性能数据 |

最小后续任务不是扩随机测试，而是二选一：

1. 针对真实 packed k/g/v key 给出 Definition 2 下的完整单方分布归约，覆盖给定相关辅助阈值和所有未用槽；或
2. 在独立标签下实现 BGI15 Gen/Eval 的 versioned serialized ABI，先做逐变量 differential，再重做材料、uCMP、stream store 和有界 E2E。

无论 DCF 选项如何，ProtocolIForwardInverseViewSimulator_L 仍需单独闭合。完成上述证明前保持 EXPERIMENTAL，不启用 secure alias，不把研究诊断放入 V4/六方案正式性能表。

## 9. S24 修改文件

1. docs/decisions/BMW16_S24_DCF_SOURCE_BRIDGE_DECISION_2026-10-08.md：源码桥接、分布断点与门禁决策。
2. docs/reproduction/BMW16_S24_DCF_SOURCE_BRIDGE_AND_FUNCTIONAL_HANDOFF_2026-10-08.md：本报告。
3. docs/reproduction/BMW16_DERIVED_EXPERIMENTAL_FUNCTIONAL_HANDOFF_2026-10-08.md：现有候选接口合同与证据分层。
4. experiments/TEST_ONLY_BMW16_DCF_BRIDGE/toy_source_dcf.py：无生产密码学依赖的 DCF 小字宽递推转录。
5. experiments/TEST_ONLY_BMW16_DCF_BRIDGE/test_toy_source_dcf.py：输出、逐层状态、share 以及分布事件检查。
6. experiments/TEST_ONLY_BMW16_DCF_BRIDGE/README.md：模型范围与复跑说明。
7. PROJECT.md：当前方案摘要与 M6B 现状同步至 S24。
8. docs/IMPLEMENTATION_PLAN.md：新增 S24 进度记录并修正当前方案的过期 S16 摘要。

未修改 VFSS/ runtime、VFSS-baseline/、论文、本地参考树、密钥/证书、构建物或原始大日志。
