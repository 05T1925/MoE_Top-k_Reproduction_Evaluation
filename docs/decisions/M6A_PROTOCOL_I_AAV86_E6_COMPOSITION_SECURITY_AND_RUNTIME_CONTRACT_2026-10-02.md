# M6A-P2-I-E6：全两两 Protocol I+AAV86 构造、安全视图与实现门

日期：2026-10-02。判定对象是**项目工程变体**：可信、离线静默且不与 P0/P1 合谋的 T；P0/P1 单方半诚实；每轮固定域全部端点对预生成独立 uCMP 材料，在线只使用 CA 活跃边。固定 M 不在范围内。本文不是 Agarwal 作者精确复现、底层密码学安全证明或独立复审通过记录。

## 0. 工作区、依据与证据身份

隔离 worktree：`C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment`，分支 `codex/m6a-p2-i-allpairs-experiment`，起始 HEAD `c3926c68fd14f270faa8b55234311071947fa080`，符合预期。开始时 E1–E5 文档、实验源码和 CSV 均为未跟踪文件，本次不覆盖。主工作区 `feat/m6a-performance-evaluation` 的既有 tracked 修改为 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md`，另有既有未跟踪 P0/P1/P2 文档和 `siamjdiscrmath.pdf`；本次不写主工作区。生产代码、基线、论文、参考树只读。

证据分层：AAV86 FOCS 1986 §3.1/Theorem 3.1（印刷 pp.506–507，本地 PDF pp.5–6；[IEEE DOI](https://doi.org/10.1109/SFCS.1986.57)、[作者托管 PDF](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf)）与 Agarwal CCS 2024 §2.4、§5.2–5.4/Algorithms 1–2（会议版印刷 pp.3033–3035 / 本地 PDF pp.11–13；[DOI](https://doi.org/10.1145/3658644.3690359)、[MIT DSpace](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)）是论文定义；E1–E5 和 E6 TEST_ONLY 是本地参考行为；可信 T、全池、稳定键、padding、原序 XOR mask 是项目扩展；整池多 key 隐私、组合 shuffle 与逆路由等仍是待证明条件。会议版未被当作 full version；`AUTHOR_EXACT=NOT_PROVEN`。

输入文件的实测 SHA-256（除注明外均取本 worktree，生产源码均为上述 HEAD；主工作区未跟踪 P1/P2 文档只读）：

| 输入 | SHA-256 |
|---|---|
| `AGENTS.md` | A260F872D0A9058FE9F0B2B94DC78C06272DCBBA7C0D28488CCCEE7BF34897AA |
| `PROJECT.md` | 296C72BE64707BA2592B7E3BD93A51D6CCA961580564C2EABCD8983E9433169A |
| `docs/IMPLEMENTATION_PLAN.md` | E75FAA039DA3090322B8BBAF8F1BA74B6B5EAD5323EC8B8CC0C538FE0AF61F7D |
| `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md` | 0E933B3E63F3080C32BB2FFEC67E3E9A8DAA6ECE5166A006F983293BF4B542 |
| 主工作区 P1 决策 `M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md` | 21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0 |
| 主工作区 P2 决策 `M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md` | C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC |
| E1 `M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md` | 0F54A121DC2CFFA70719C735F577C66022C2ED019706415CBC5D9A87347537A4 |
| E3 `M6A_PROTOCOL_I_AAV86_ALL_PAIRS_DESIGN_GATE_2026-10-02.md` | CE43F664BADC664A918C919EEDA889B6586C8EA224B0CF0D512712A914906965 |
| E2 `M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md` | 307B80ABC789F2C1EB0614992A4E2C4938A7AB51BB3BCB089951D713C6C23ED9 |
| E4 `M6A_P2_I_E4_CA_SOURCE_AND_SHUFFLE_COMPATIBILITY_2026-10-02.md` | 3312ED84FD5EDEE2F3766C791186CABF297BB73E0565BD06E93E2FCA588E35FD |
| E5 `M6A_P2_I_E5_AAV86_CA_SPEC_AND_REFERENCE_2026-10-02.md` | 7C7A35B66E785EDBA4ADD2F9FF204C5F319077F5242669E0423C9C875513624D |
| E5 `aav86_ca_reference_TEST_ONLY.py` | 6FA205CDD1067C58203103F85F4C0F90781345FBC68559C92EBB6F6FB808F073 |
| E1 `aav86_graph_counter.py` | 09DCC72DF6BC68C83046CBB3F7C9AF8B13642BEA720036B92804F6BE5423B402 |
| E5 `aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv` | CC73AF0CC615938AE5852D58CAD3070960186F41F56501BC0B9C07B5FCE22256 |
| 本轮用户 E6 任务文本 | BAFB86F10B212758FB4B6690158D7E9B5FC260E204D06FFCC2D7E9281CE992F8 |
| `protocol-reproduction/SKILL.md` | 6759FBB53C9DB00662C3F83CCDBC259EE7CBAA89A040ED5342DDA96D544A35A7 |
| 主工作区只读 `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf` / CCS’24 会议版 15 页 | 18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C |
| 主工作区只读 `Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf` / 作者托管 9 页 | 322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83 |

起点未跟踪 E1–E5 产物的**完整清单**还包括下列未在上表单列的文件；这些起点哈希本轮结束复核不变：

| 既有文件（均在 `experiments/m6a_p2_i_allpairs/` 下） | SHA-256 |
|---|---|
| `CMakeLists.txt` | B99BCA22E068E462FB222E83AD5A79BE9E4A92E79447E46EB6880BD317C77195 |
| `TEST_ONLY/README_TEST_ONLY.md` | 0828F6AC7D0B16E1AC4C8E170F8F64FFED2A5CD117442692CBEB86E8BBF13910 |
| `TEST_ONLY/aav86_ca_counts_E5_v2_TEST_ONLY.csv` | F7451C956CCCB7F5E68CC1919BDC488FF6ACE4B78C7EE08F2A63B3B016C054A9 |
| `TEST_ONLY/aav86_ca_runs_E5_v2_TEST_ONLY.csv` | 75400E10300D985FB19EAB9CCEFC20AAE687CCA9A44160AE9E7D96A5F662DC58 |
| `aav86_graph_counts.csv` | 5346AE011201C1F88F3B7199BB86DE7C1B4F761151F3FB269C86F9E99AF9F622 |
| `aav86_n5_r2_test_edges.csv` | 87A660C55B47FBCD1AAAE6C4E025798E8E4B6853968DC6D27DF635A704B46D78 |
| `material_pool_test.cpp` | 8F6F36C5408A55A09CB9754C7076C2C27EADD0DDD539B7D5297949A9DEF0E159 |

E1–E5 起点另外五份文档与 E1 计数器、E5 reference/fixture 已在上表列出。E4 的 local-rank 转录错误保持历史原样，按 E5 的更正公式审查。Agarwal full version/作者代码本次未重新检索，沿用 E5 的“本次盘点未核实到”边界，不能据此断言公开世界不存在。

关键源码行及 SHA-256：

| 源码 | 行/功能 | SHA-256 |
|---|---|---|
| `protocol_i_priority_key.h` | 15–33 signed 翻转、降序、原下标注入；helper 限 n≤1e6 | 8C345675877FDD101E0B5918250FF7E7B6879031161CD38B7FE141CE96D0E338 |
| `protocol_i_score_input.cpp` | 45–54 配置；64–68 dummy；104–112 密钥份额 | 282D44D2632D92BD22D70FF64495FF7B65630D1CC15ACCB959A1E9D0642E9AD8 |
| `protocol_i_ucmp.cpp` | 5–13 DCF 阈值、两次 eval、party 输出；14–15 无持久 claim 的序列化 | B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23 |
| `protocol_i_cmpagg.cpp` | 24–56 只接受完整 clique，rank 左右累加 | 5183E371ADC7D06DF6E93097921B2C303B1AC5A8C24B883CA2330855521FCA4F |
| `protocol_i_parallel_shuffle.cpp` | 26–47 百万边 cap；298–353 dealer；442–503 两轮公开 list 加第 3 轮 clique rank | 8D0E488B6348CAA6BC55D8F6652198E97FC6ED69381D5B367091D71B77DCD75D |
| `protocol_i_secret_shared_shuffle.cpp` | 39–119 正反各两轮；仅 share 输出 | 4B83CED81802CD1C1EECD484C96CFA1866F8601131D052C34A672A8223C6121C |
| `protocol_i_pipeline.cpp` | 27–39 full package；52–61 layout；63–126 六轮 clique 到 mask | 0FD331AD19637415CD83307DE96AE6DD3C4D3DE21D0DD34858B5143847E94D06 |
| `protocol_i_party_package.cpp` | 11–12、35–48、138–244 64 MiB/百万边上限与完整包 | E3D31C04DCD979A882E8FA37C75C80C243494A75848CA4B00946842A0088E216 |
| `protocol_i_transport.cpp` | 14–17、102–111、168–213 单帧 1 MiB、session/phase/序号检查；无密码学 MAC | 3DE029FED495F7CCC499E3BF08C1D9F4F9FA6BAE0836E349ACFA8C6BCCF51EB6 |
| `VFSS/ext/FSS/dcf.cpp` | 121–175 keygen 从线程 PRNG 取种子；254–295 eval；尚无本项目组合安全定义 | 1E62CF39A5985E761F5BB7E18E20884835DFE6AC08A8BF55F69E34B1B1B3E0E8 |

相应 header 已读：`protocol_i_score_input.h`、`protocol_i_ucmp.h`、`protocol_i_cmpagg.h`、`protocol_i_parallel_shuffle.h`、`protocol_i_secret_shared_shuffle.h`、`protocol_i_pipeline.h`、`protocol_i_party_package.h`、`protocol_i_transport.h`。其 SHA-256 依次为 `8103866D657721F604DE3CB7A475824830581FE1D64A243A21B17972C6CEB0D1`、`7E444F0188AFA2FAF0CE8A1FA97AF7D821C54FE82FB46CEFD1999AEA4C798D33`、`D50B7B84A3EEF299D1F1C23A4D342C7F95003C28DD0F72CD8BD56FD0E3878390`、`A88FFF48B9B2F1F423EFDAECEA9B8F89C01F25C91BFE1A0AEA4F358604DA672D`、`7C739B2B7DE07791AE746C1E5B942C267F332D7058F465FBE9CBD04A1EDCF197`、`4871BD423ABD71758661B6668249D4D1268EFA240C8894658F804DC411DCAC6F`、`9258F0F141E8D7F0346559458D2B280255633AE5BECBA41A00BCFE0D83867AA9`、`710171E0D5E526C9A2FEE9A24ADAFB67176A9824285C4BE10A93546C3B1FAFDF`。

## 1. CA 信息限制与定向验证

新增的 `aav86_ca_control_E6_TEST_ONLY.py` 仅含整数 shuffled handles、公开 pivot、按 handle 标记的已打开 local rank、depth、公共随机币。它据 `q=ceil(m^(1/d))-1` 均匀无放回抽 pivot；pivot 按其互异 local rank 排序；非 pivot 的 local rank 是 bucket 下标；公开图为 pivot clique 加 pivot/nonpivot 全连接。输出 pivot 顺序、bucket、非空下一层子问题和 canonical 边。不存在 score、priority、original_index、明文比较字段或 E5 导入。单节点直接携带，空桶跳过；末轮 q=m−1，桶仅有一名 nonpivot，不再递归。

`verify_aav86_ca_control_E6_TEST_ONLY.py` 是隔离的外部驱动：先让 E5 oracle 生成明文 local ranks 和期望 trace，再按同一公开币顺序喂给控制器；逐节点核对 pivot、bucket、边、子问题、合并排序与 K=1/中间/n mask。实际执行：1224 小规模穷举、120 随机、12 同分/INT32 边界、1 个 n=5/r=2 fixture、1 个 handle 重标号、8 个新增 padded sentinel 样例，检查 3050 个图节点，均 PASS。结果见 `aav86_ca_control_verification_E6_TEST_ONLY.json`。此驱动使用明文 oracle；它不是 MPC、密码学证明或 secure runtime 测试。局部 rank 足以唯一确定本图的控制状态：pivot 邻接全子问题，故其 rank 是本层全序位置；nonpivot 只邻接 pivot，故其 rank 恰是之前 pivot 的个数。若没有唯一全序，pivot rank 可重复，控制器会拒绝；稳定键合同排除此情形。

## 2. 冻结的域、键与端点合同

选 `D=padded_n=max(2,next_power_of_two(logical_n))` 作为**整条 CA 图、shuffle、节点 mask、边 key 和逆路由共同的域**；`1≤n≤1,000,000`、`1≤K≤n`。选择 D 的理由是现有 layout、score adapter 和 reverse carrier 都以 D 为输入；若选择 n 图而保留 D shuffle，须公开/维护隐藏置换后真实节点集合、另设 n↔D handle map 与分段 inverse，当前无接口或安全证明。D 方案的代价是 dummy 参加 pivot、桶、rank、活跃图和全池；E5 原 `logical_n` 图计数不再是本方案的成本保证。E6 padded TEST_ONLY 只检小例。

设 `b_i=log2(D)`，`b=33+b_i`，比较环 `R=Z_(2^b)`。真实记录原槽 `j<n` 的 signed 32-bit raw（Q20.12 只影响数值解释，不改位级顺序），`u=raw XOR 0x80000000`，`q=0xffffffff-u`，`key=(q<<b_i)|j`。dummy 原槽 `n≤j<D` 取 raw=`0x80000000` 即 INT32_MIN，同一式子注入各自 j。所有 j 互异；同分按 j 升序；得分高者 q 小。即使真实 score 为 INT32_MIN，其 j<n 也小于全部 dummy j≥n，因此所有真实键小于全部 dummy 键。键落在 `[0,2^(32+b_i)-1]=[0,2^(b-1)-1]`，故任意两键的整数差严格落在 `(-2^(b-1),2^(b-1))`，满足当前 uCMP 的半环无歧义前提。n=1→D=2、b=34；n=100000→D=131072、b=50；n=1000000→D=1048576、b=53；均在 uCMP 34..53 范围。n 非二次幂、INT32_MAX/MIN、全同分亦满足上式。

这是 score adapter 在 `protocol_i_score_input.cpp:64–68,104–112` 已采用的 padding/key 形式；`protocol_i_priority_key.h` 的 helper 在 D>1e6 时拒绝调用，不能直接用它处理最大 n 的 D，须以 adapter/layout 合同为准。现有 score adapter 的 carry/sign 安全和宽化 E2E 仍是独立验证义务；此处只给位级排序证明。handle 是隐藏置换后的**槽位** `a∈[0,D)`，绝不是原下标 j。公开局部重排 `ρ_t` 必须同时作用于 record shares、handle 标签、每轮 endpoint masks/edge lookup 的逻辑坐标、最终 membership carrier 的逆栈；键低位仍为 shuffle 前 j。不得用局部槽位重新打破同分。

## 3. 条件性离线构造与比较正确性

下述是新 AAV86 专用接口的目标伪代码，**尚未在生产路径实现**：

```text
T.Setup(session, version, n, D, K, r, b, security_label):
  sample a hidden uniform initial permutation π and correlated forward/inverse materials
  sample input-independent public pivot seed; bind it to session/version
  for t in 1..r:
    for a in 0..D-1:
      sample R[t,a] uniformly in Z_(2^b), independently over (t,a)
      sample R0[t,a] uniformly; set R1[t,a] = R[t,a]-R0[t,a] mod 2^b
    for 0<=a<c<D, in lexicographic canonical order:
      alpha[t,a,c] = R[t,a]-R[t,c] mod 2^b
      (key0,key1) = keyGenDCF(b,64,alpha,1) with fresh keygen coins
      bind both keys to (session,version,D,K,r,b,t,a,c,material_id)
  distribute each party its own masks, keys, shuffle/inverse material and package manifest
  durably claim/consume session and material IDs before any online release
  T exits online message topology; no input/open/rank/output recipient socket
Online(t):
  for each current public handle a: y[t,a] = open(x_share[t,a]+R_share[t,a]) mod 2^b
  for each active canonical edge (a,c): use exactly key[t,a,c] on (y[t,a],y[t,c])
  add party shares as in §4, open local ranks once for this iteration
  update public pivot/bucket/graph state; carry corresponding secret record shares
```

每轮 masks 对**该轮 endpoint** 独立，因而同一 endpoint 出现在多边时引用同一个 R；不能为边重新抽不相容的 mask。不同轮、session、(a,c) 的 keygen 币须独立；公开 pivot seed 要独立于 π、mask、keys，双方以公共 subproblem path/counter 和无偏抽样取本层 pivots，既不依赖明文值也不请求 T 在线指导。E6 Python `random.Random` 仅供 TEST_ONLY 对照，不是生产随机性合同。重启后一次性状态由**持久材料池所有者**负责，内存 `used_` 标志和 C-INSTANTIATION 进程本地 `claimed_material` 不够。T 可保留离线状态且无需擦除，但不得收到任何在线 transcript。缺包、身份不符、重复 claim、超容量、异常或 peer abort 只 fail closed；错误类型/时机和访问流量列入泄露合同，不能静默换 key 或在线向 T 补料。

进程/消息边界必须可检查：T 独立于两在线方，完成离线 package 交付与 ACK 后关闭到 P0/P1 的在线可写/可读通道；P0/P1 的在线状态机只持有相互连接的 fd，不接受 T sender/receiver 的 rank、masked-open 或输出帧，也不将日志/遥测上传给 T。现有 `ProtocolIFramedChannel` 的 header 检查 session、指纹、phase、type、序号和双方角色，但没有密码学 MAC，且没有 AAV86 的 t/edge/material ID；所需绑定须通过专用外层协议和受控通道落实，不能把现有 frame 检查称为认证证明。

参数表：

| 对象 | 取值/绑定 | 当前接口与缺口 |
|---|---|---|
| score shares | 双方 `Z_(2^32)`，Q20.12 signed；原槽 j | `protocol_i_raw_score_input_party` 2 轮；需与新 package 脱钩 |
| key/mask ring | `Z_(2^b)`，`b=33+log2 D` | uCMP 支持 34..53 |
| rank shares | 至少 `Z_(2^b)`，每图 degree < D < 2^b | 当前 CmpAgg 使用 64-bit 累加、再按环重构 |
| graph endpoint | shuffled handle `a,c∈[0,D)`，canonical `a<c` | 当前 CmpAgg/包只能完整 clique，需稀疏 selector |
| package | session、fingerprint、version、party、n、D、K、r、b、公开 pivot seed、t、a、c、material ID、one-shot ledger | 当前 package 缺 r/t/edge ID/版本/seed 及持久 claim |
| shuffle binding | `π` 同时决定份额、公开首轮 y、CA handle、inverse | 当前三轮 C 类只绑 clique，需分离 |

## 4. uCMP 代数与 CA rank

以下正确性仅依赖 DCF 原语功能 `F_alpha(z)=1[z<alpha]` 的两方加法 share 正确性。源码 `protocol_i_ucmp.cpp:5–13` 令 `N=2^b,H=N/2,alpha=R_a-R_c (mod N), x=y_a-y_c (mod N), z=x-H (mod N)`。两方合计 `ge=F_alpha(z)-F_alpha(x)+1[z≥H]`，`lt=1-ge`；party 1 单独加公开 correction，party 0 单独给常数 1。因为 `x=(key_a-key_c+alpha) mod N`，且两合法键的整数差 d 在 `(-H,H)`，按 `alpha` 两侧可能环绕的四种区间逐一化简得 `ge=1[d≥0]`，故 `lt=1[key_a<key_c]`；d=0 时 ge=1、lt=0，虽然稳定键合同实际排除相等。这里结果在 `Z_(2^64)` 合并；公开 y 和阈值均在 `Z_(2^b)`，不能把 masked y 当 signed 数直接比较。

对 canonical `a<c`，源码 `protocol_i_cmpagg.cpp:43–55` 将 `1-lt` 加到 a，`lt` 加到 c。唯一键下，a 得 `1[key_a>key_c]`、c 得 `1[key_a<key_c]`，即各自较小键在前的邻居数。图为 AAV86 pivot 图时，非 pivot 只连接 q 个 pivot，local rank 唯一给出桶编号；pivot 与本图所有其他节点连接，其 rank 是全子问题位置。以固定公开 pivot 币和 depth 递归，末轮 q=m−1，得到完整稳定全序。此证明是代数/算法正确性；DCF key 隐私、随机币实现、active-edge 安全仍另审。

## 5. 完整单方视图与可证明范围

允许公开的项目 transcript 暂按最宽可核验版本列出：公共 `n,D,K,r,b,session/version`；每轮公开随机 pivot 币及在 shuffled handle 上的 pivot、bucket、local ranks、活跃 edge set、访问顺序、每阶段消息长度与 abort；每轮 masked vector `y_t`；最后 shuffled 全序/rank。若项目最终要隐藏 bucket 大小、访问流量或错误时机，当前动态图协议必须另加 oblivious 控制、定长 padding 和新的轮数/成本，此稿不替其证明。

| 参与方 | 完整应计视图 | 可公开/禁止事项 |
|---|---|---|
| P0 | 自己的 `Z_(2^32)` score shares、已知输入槽标签、score adapter key shares、自己的 `σ_0,τ_0` 或对应专用 shuffle 因子及 offline messages、自己全部 `R0[t,a]` 和全池 DCF/uCMP keys、材料 manifest/ledger、前向/反向各阶段本地/peer 消息、全部 `y_t`、自己的比较/rank shares、公开 local ranks/pivots/buckets/活跃查表序列/流量/错误/abort、最终 XOR mask share | 不得见原值、完整 π、对方 mask share、原槽↔shuffled handle、明文 selected index、原序重构 mask |
| P1 | 同上，换为自己的输入份额、`σ_1,τ_1,R1`、party1 key、输出份额；含其收到的所有在线消息与失败状态 | 与 P0 同边界 |
| T | 全部离线 score-independent keygen 币、`π,σ,τ,R,R0,R1`、双份 package、material ID、分发/ACK/失败元数据及保留状态 | **没有**输入份额、`y_t`、rank、pivot/bucket 在线 transcript、输出份额的接收路径；T 与一方事后合谋不在模型内 |

**单方条件性 masked-list 引理。** 固定任一在线方的合法离线视图，若未见对方 fresh `R_{1-p}[t,a]`，且其整池 keys/所有辅助材料在该视图下不泄露对方 mask 的可识别关系，则每个 `y_t[a]=x_t[a]+R[t,a]` 由对方未知均匀 mask one-time pad；对任意向量 x，同一轮整个 y 向量均匀。一个节点 mask 被多条边共享**不改变 y 向量本身**的均匀性；但同一 mask 差进入多个 key、而该方又能评估 key 于共同 y，联合视图不由此引理覆盖。跨轮 fresh masks 是必要条件；若复用，`y_t[a]-y_s[a]` 会显露输入差。T 知完整 R，故条件只针对非合谋 P0/P1，且 T 必须收不到 y。

**逐边 hybrid 的准确前提。** 需要 VFSS DCF 的定义保证：对任意自适应选择的阈值向量 `alpha_e=R_a-R_c`、给定该方 mask shares、已持有其他 keys、公开 y 与合法 eval 输出/公开 rank 的辅助信息，独立 keygen 币生成的单方 keys 可逐边模拟或不可区分；模拟还须保持每边本地 eval shares 与最终 rank reveal 的一致性。这比单个 DCF 正确性或“隐藏 alpha”更强。若有适用的多实例、选择性求值、辅助输入安全定理，且随机数源/序列化满足它，可按 lexicographic edge 顺序、轮次顺序替换完整 `r·C(D,2)` keys，再处理自适应使用子集；因为所有槽位事先固定，选择本身不会向 T 发请求。但当前可见 `keyGenDCF(b,64,alpha,1)`、`evalDCF`、key bytes 只给功能与布局，未提供上述联合定义/模拟器或其与共享 mask 的证明；本报告**不宣称 hybrid 成立**。`ProtocolIUcmpPartyMaterial::serialize` 只含 party/bits/DCF arrays，缺 session/t/edge ID，反序列化后 `used_` 可重置；必须由新外层受保护的包和持久 ledger 补足。

源码 `VFSS/ext/FSS/dcf.cpp:121–130` 从 `FSSConfig::prngs[tid]` 获取种子；C dealer `protocol_i_parallel_shuffle.cpp:114–123` 以 `getrandom` 为 256 个线程 PRNG 初始化一次。新工厂如果绕开该入口，还必须显式保证安全初始化。这些是可核实现象，不自动证明不同边的 keygen 币在并发/重启/跨 session 中满足上述独立性，也没有提供完整 key 分布的安全定理。需按实际调用线程、seed 生命周期和序列化后的联合视图独立审计。

具体失败反例说明证明义务：若单方从其 key 或跨边相关性恢复任一 `alpha_{a,c}=R_a-R_c`，则公开 `y_a-y_c-alpha_{a,c}=key_a-key_c (mod 2^b)`，泄露**数值差**而不只是已批准的次序/rank。若同一 R 跨轮复用，公开两个 masked opens 的差直接给出两轮秘密记录差。若该方恢复完整 π，公开全 rank 立刻映回每个原槽的稳定顺序及被选原下标。这些分别是 key 隐私、freshness 和 same-π 隐藏证明不能省略的可检验反例，不主张当前源码已经发生这种泄露。

**自适应访问引理（条件性）。** 在通过 §1 的控制合同、相同公开 pivot 币、完整 rank reveal、确定性 DFS/规范 pair 序和无私有错误分支前提下，下一层子问题、访问边、查表顺序、理论消息长度是已公开 ranks/币/状态的确定性函数，故“预发整池、只消费活跃槽”不会因 T 在线补料增加独立泄露。但当前稀疏 selector、固定错误语义和流量封装均不存在；material lookup 的计时/缓存/内存缺页可能暴露更多，需在小规模实现审查中明确模型。公开错误类型/abort 若依赖秘密一致性检查，也不是上述确定性函数，须统一 fail closed 或单列泄露。

**随机 shuffle 的对称性与缺口。** 对固定互异键全序和独立均匀隐藏 π，shuffled 全 rank 向量是 `D!` 种 rank 排列中的均匀一个；若 n 个真实与 D−n 个 dummy 由公开 rank 识别，真实 rank 到真实 shuffled slots 的对应在所有 n! 排列中均匀。CA transcript 是 rank 排列、公开 pivot 币、公共 D/r 的确定性函数，因此在**只给这些公共量**的分布上不依赖原始 score 或稳定原槽顺序。然而单方实际还持有自己的 permutation 因子、输入/记录份额、mask shares、全池 keys、online shuffle messages 和 output share；必须证明在**这些条件下** π 仍不向其暴露原槽映射。C-INSTANTIATION dealer 的 `τ0=π∘σ1^{-1}`、`τ1=π∘σ0^{-1}`（依仓库 permutation compose 约定，见 cpp:303–308），若 σ0/σ1 独立均匀，仅给单方自身 `σ_p,τ_p` 时 π 仍均匀；但加入 `a,e,r_share`、两轮消息、全池相关 keys、最终 mask share 后的联合条件分布尚无模拟证明。故不能以“rank 向量均匀”推出实际 P0/P1 视图安全；这正是 `ADAPTIVE_TRANSCRIPT_LEAKAGE` 的阻塞项。

## 6. 同一 π、首轮 masked list 与原序输出

| 当前路径 | 已核实功能 | 对 E6 的边界 |
|---|---|---|
| 两轮 `SecretSharedShuffle` | 两次 Permute+Share 串行，返回 `π(x)` 的份额；另两轮 reverse 绑定其同一材料对象里的本方 inverse | 不直接给 `π(x)+r` 公开 list；后接 mask exchange 至少再有一个因果层，无法把它冒充融合的论文初始 shuffle |
| 三轮 `C-INSTANTIATION` | dealer 选 π、σ0/1、τ0/1；R1/R2 后有 `shuffled_share` 和 `public_masked_records`；R3 固定 clique rank reveal | 对本类，代数可核：`s0=τ0(σ1(x1)+a1)+e0`、`s1=τ1(σ0(x0)+a0)+e1`；`e0=-τ0(a1)-h`、`e1=-τ1(a0)+h`，故 `s0+s1=π(x0+x1)`，R2 public=`s0+s1+r0+r1=π(x)+R1`。但 dealer 直接生成完整 clique keys、n≤约 1414 的百万边 cap、没有 CA 每轮图/逆路由接口；此函数不能直接当 AAV86 实现 |
| 六轮 fixed-clique pipeline | 先两轮 SecretSharedShuffle，再额外 exchange 打开 masked keys、全图 CmpAgg、rank reveal、两轮 reverse | 输出 logical_n XOR mask 的历史工程功能；其 `π` 属于两轮 shuffle，公开 list 与它分阶段形成；不满足本设计所需 `2r+1` 初始融合或动态 CA |

因此最小候选是**新命名的 AAV86 专用组合合同**，复用 C 的 R1/R2 代数/记录群运算与独立 uCMP eval，但把 C 的固定 clique keygen、R3 rank state machine 拆开；另生成与**该 C 初始 π**严格绑定的逆路由材料。不能用另一套两轮 shuffle 的 inverse 替代。论文 §2.4 的公有 list、秘密 payload 与 r 同置换功能需对新组合做单方模拟；源码代数只证明功能相等。

形式化坐标：令 `x[j]` 是先注入 j 的 D 个 priority records，`z[a]=x[π^{-1}(a)]` 为初始隐藏 shuffle 后份额，首轮 `y_1[a]=z[a]+R[1,a]`。当前公开局部重排 `ρ_t` 应作为两个在线方共同维护的可逆 handle 标签置换；每轮 record share 在 handle a 上始终属于同一逻辑输入 `π^{-1}(ρ_t^{-1}(a))`，该轮 mask/key 用**当前 a**。终局排序函数 `s:sorted_position→current_handle` 从公开 CA local ranks/buckets 得出。形成 current-handle 的公开 bit `m[a]=1[sort_position(a)<K]`；P0 取其 mod-2 share 为 m，P1 为 0（或在更宽加法环中置 m/0）。沿公共 `ρ_r^{-1},…,ρ_1^{-1}` 撤销局部布局，再用**初始 π 的逆安全路由**得到原槽加法份额 `c_0[j]+c_1[j]=m[π(j)]`。取两份 word 的最低位得 XOR shares，因为 `parity(c_0+c_1)=parity(c_0) XOR parity(c_1)`；截取 j<n。由于 dummy 永排真实之后、K≤n，`sum_{j<n}m[π(j)]=K`，且无 dummy 被选。secure 路径只公开 shuffled rank/布局，不能重构原槽→handle、score、selected index 或原序 mask。现有 C 路径没有这个 inverse 证明/材料；公式是待实现的接口义务。

## 7. 逐消息因果 DAG 与计量

下表是**拟议最小消息合同**，并非当前可运行协议。每个箭头均是 P0↔P1 双向受绑定帧交换；T 仅在离线阶段向两方分别交包、ACK 后退出。

| 因果层 | 发送→接收/字段 | 依赖与打开值 | 材料/泄露 |
|---|---|---|---|
| 离线 O | T→P0/P1：session/version/D/K/r/b、各自 score/shuffle/inverse、`r·C(D,2)` key 槽、mask shares、完整 manifest；双方 ACK | 在线输入无关；不计在线轮 | T 见全部离线量与 ACK；持久 claim 后不得复用 |
| 输入 A1,A2 | P0↔P1：score carry masked operands、sign masked operands | 原始 `Z_(2^32)` shares → D 个 stable key shares | 当前 adapter 两个串行轮；各层打开 masked carry/sign 值 |
| 核心 S1 | P0↔P1：`σ_p(x_p)+a_p` | key shares 就绪 | 初始 shuffle 第一因果层 |
| 核心 S2 / iter1 open | P0↔P1：`s_p+R_p[1,*]` | S1 peer frame；公开 `y_1=π(x)+R_1` | 首轮 D 个 masked records；同一 π |
| iter t rank，t=1..r | P0↔P1：图内各 handle 的 local-rank shares（规范帧） | 已公开 `y_t`、活跃边；本地 DCF eval，打开 rank | 公开 rank、pivot/bucket/graph/访问/流量；消耗活跃边 key |
| iter t+1 open，t<r | P0↔P1：`x_{t+1,p}+R_p[t+1,*]` | 上轮 rank 决定的公共布局/图；公开 `y_{t+1}` | fresh masks；无在线 T；空/单节点位置也须有规范帧或明示长度 |
| 输出 V1,V2 | P0↔P1：专用 inverse route 的两级消息 | 最后 rank 已公开、本地 m 已生成并撤销公共布局 | 同一 π 的独立 inverse 材料；原序 mask 保持 secret |
| 失败 F | P0↔P1：规范 abort/状态同步（若需要） | 任一校验失败 | 若增独立因果帧，额外计轮；当前 `NOT_SPECIFIED` |

若 C 初始两轮与 iter1 masked opening融合、每轮 rank reveal 一轮、之后每轮 masked opening 一轮、公共控制零通信，则核心轮数 `2+r+(r-1)=2r+1`，这是**条件性工程 DAG**，与 Agarwal theorem 的论文目标同数，但此稿尚无满足其安全假设的新接口。已知 raw adapter 再 +2、假设同 π inverse 为两轮再 +2，且无额外转换/错误同步时端到端算式 `2r+5`；由于 inverse 及失败合同未闭合，实际端到端 `NOT_PROVEN / NOT_MEASURED`。若采用现有两轮 SecretSharedShuffle 后另开 y1，则核心至少 `2r+2`，是项目工程变体。旧 fixed-clique 六轮 pipeline + 两轮 raw adapter = 八轮，不能记为 AAV86。代码的 C-INSTANTIATION 三轮是**单次 clique 核心**，不是 r 轮 CA。所有线上通信字节、时延、计算、离线真实 keygen/交付时间、峰值内存均 `NOT_MEASURED`。

容量必须分开：论文 `e_A` 是随机活跃图的实际边数（AAV86 Theorem 3.1 仅给期望比较量）；本项目预留每方 `r·C(D,2)` **key 槽**，两方合计 `2r·C(D,2)`；每条活跃边当前 uCMP 对同一 DCF key 作两次 `evalDCF`，所以实际在线 DCF 调用每方 `2·e_A`，不能拿 E5 observed maximum 当上界。全图预留不意味着全图在线求值。

| logical n | D | C(n,2) 逻辑域/每轮/每方 | C(D,2) 选定域/每轮/每方 | r=2 全池/每方 |
|---:|---:|---:|---:|---:|
| 100,000 | 131,072 | 4,999,950,000 | 8,589,869,056 | 17,179,738,112 |
| 1,000,000 | 1,048,576 | 499,999,500,000 | 549,755,289,600 | 1,099,510,579,200 |

这些是组合计数而非实测。按当前 `ProtocolIUcmpPartyMaterial::serialize` 的布局，单方**裸序列化 key** 长度为 `33+16(b+1)+8+8b=57+24b` bytes：b=50 时 1257 bytes，b=53 时 1329 bytes。仅用该源码布局作容量算术，n=100000 时每方每轮约 10,797,465,403,392 bytes；n=1000000 时约 730,624,779,878,400 bytes；尚未含包头、mask、shuffle、索引、对象/allocator 开销、复制及传输帧。它们**不是**生产测量或可分配承诺。现有 C dealer、party package 都有 `1,000,000` 边 cap，package 另有 64 MiB cap，因而两种大规模输入无法通过当前完整包；若仍走全池路线，需要流式生成/外存索引/分段交付及失败恢复的新合同，不能移除 cap 直接分配。

现有 framed transport 单帧 payload 上限为 1 MiB；所有 D 向量与超大离线包都需要规范 chunking/streaming、顺序/总长/哈希绑定、失败后不可重放的 ledger。当前 chunked helper 不等于能承载上述全池，因为 package 本身仍有 64 MiB 与百万边限制；若新协议分段，不得把多次读写重算为零成本。

所选固定 D 公开帧合同还须每轮每方交付 D 个 mask shares（若沿用当前 uint64 槽，逻辑载荷 `8rD` bytes/方）；在线首轮 C record 两层各 `24D` bytes/方的当前结构体载荷，之后每轮 masked word `8D` bytes/方，rank reveal 可在每个本地 share 先模 `2^log2(D)` 后以 `D·ceil(log2(D)/8)` bytes/方/轮编码，最终 inverse 另计。若为 inactive singleton 仍传固定 D 帧，长度为公开 D 的函数；若跳过它们，则长度/时序是公开控制状态的函数，也必须在泄露表登记。这些是**拟议字段宽度的解析计数**，不是已实现的 wire bytes；帧头、认证、重传、abort 和峰值内存均 `NOT_MEASURED`。

## 8. 最小新接口、验证门与最终判定

若阻塞项由独立安全复审闭合，建议新增而不改历史标签：`ProtocolIAav86Layout(n,D,K,r,b)`；`ProtocolIAav86DealerMaterial`（同一 π 的初始 C 式前向与逆路由、每轮节点 masks、边槽 manifest）；`ProtocolIAav86PartyPackage`（版本/session/t/handle pair/one-shot ID）；`ProtocolIAav86SparseCmpAgg`（只接公开 graph 边且 canonical 查表）；`ProtocolIAav86Control`（按 §1 rank-only 规则更新）；`ProtocolIAav86Pipeline`（首轮融合、后续迭代、原序 XOR 输出和分阶段 metrics）。预期只在 `VFSS/include/moe_topk/protocol_i_aav86_*.h`、`VFSS/src/moe_topk/protocol_i_aav86_*.cpp` 与相应独立测试/构建登记新增；旧 `protocol_i_parallel_shuffle*`、`protocol_i_pipeline*`、`VFSS-baseline/` 保持原样。实现顺序应为小 D 的 material/key binding 与持久 ledger → 相同 π shuffle+masked-list+inverse 的功能及单方模拟 → 稀疏 CA rank 控制 → full-sort 到 mask → conformance、oracle differential、独立进程 E2E → 按实际 revision/输入/命令/计数测量。此列是**获准后**的合同，不是本轮实施许可。

| 门 | E6 判定与最小缺口 |
|---|---|
| ALGORITHM/CA_CONTROL | **PASS（TEST_ONLY 算法控制）**：local ranks 唯一决定 pivot order/buckets/图；3050 图节点差分通过；padding 仅小例验证 |
| ALL_PAIRS_AVAILABILITY | **CONDITIONAL PASS（小 D 概念与 E1 隔离实验）**：槽域/索引/正确性已列；生产包缺 t/edge ID/持久 claim，百万边/64 MiB cap 与极大资源量阻大 D |
| FULL_POOL_KEY_VIEW | **NO-GO**：VFSS 未核实可用于共享节点 mask、相关阈值、全部 keys/选取性 eval、公开 rank 辅助信息的多实例单方模拟/安全定义；需底层正式证明或独立归约及源代码随机币审计 |
| ADAPTIVE_TRANSCRIPT_LEAKAGE | **NO-GO**：公开 rank→图/访问的确定性已证明；条件于实际单方 shuffle+全池 key+output 的联合视图模拟、错误/流量合同未闭合 |
| SAME_PI_SHUFFLE | **NO-GO**：现有 C 初始两轮具有同 π 的功能代数，但绑固定 clique；新 CA shuffle/key/逆材料的同 π 联合合同和模拟未实现 |
| ORIGINAL_ORDER_MASK | **NO-GO**：数学式与恰 K 证明已给；C π 专用逆路由材料/接口、原序隐藏及 XOR 输出 E2E 未实现 |
| CAUSAL_ROUNDS | **CONDITIONAL**：新首轮融合可画 `2r+1` 核心 DAG；当前可运行 AAV86 路径不存在，raw-score→mask 总数 `NOT_PROVEN` |
| PROTOCOL_I_AAV86_RUNTIME | **NO-GO**：本执行窗口仅产生待独立复审的设计/TEST_ONLY 控制证据，不声称独立评审通过 |

最小可执行下一步：（1）对 VFSS DCF keygen/eval/序列化及其依赖的正式定义，完成“共享节点 mask + 全池相关阈值 + 自适应选择 + rank reveal 辅助信息”的联合单方视图审查，缺定义即给出新证明或更换有正式性质的原语；（2）为 C 式前向 R1/R2、每轮 fresh mask、专用逆路由写接口级联合模拟与 same-π 不变量，连同持久 claim/abort/流量合同；（3）用 n=1、非二次幂、INT32 极值/全同分、n=5/r=2 做小 D 独立进程 conformance 和 mask E2E，核对每因果帧。以上未闭合前，不进入 Protocol I+AAV86 secure runtime。若独立复检将这些门判为通过，下一任务才可按上段顺序实现小规模路径；固定 M 仍延期，Protocol III 不在本轮。

本轮没有改写 E1–E5、生产代码、冻结基线、论文或参考树；没有全仓测试、benchmark、提交、推送、合并或 PR。E6 定向验证的命令与机器结果保存在新增 JSON；最终 `git diff --check` 与完整差异清单在工作报告中记录。
