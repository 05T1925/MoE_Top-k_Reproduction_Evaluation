# M6B-I-BB90-5ITER-SOURCE-CLOSURE-S1 来源闭合报告

日期：2026-10-12（Asia/Shanghai）
工作类型：来源审计、数学/协议边界推导、实现准入裁决
报告身份：当前会话的独立来源复核；不是异会话接收签收

## 1. 裁决摘要

**SOURCE_SPEC = NO-GO_FOR_IMPLEMENTATION**
**BB90_5ITER_TEST_ONLY_GATE = NO_GO**
**TEST_ONLY 实现：未创建；oracle differential：未运行。**

本次闭合了两个事实：1990 年 BB90 的正式文献身份和官方摘要给出的高层界；2024 CCS 会议版确实在安全中位数成本段提到一个“五迭代 BB90 版本”，并称其比较数量对 n 线性。没有闭合的是该五迭代版本本身：未发现逐轮伪代码、参数、边图、状态转移、概率证明、常数或失败合同。因而不具备独立实现条件。

来源不足不是某个取整值缺失，而是迭代 1–5 的算法主体均缺失。不能把 BB90 原文四轮算法、BMW16 的四轮算法、AAV86 或已有项目采样算法拼接成五轮版本。也不能把 2024 年的线性比较叙述当作可执行规格或成功概率定理。

## 2. 工作区、历史状态与差异保护

### 2.1 本任务实际 checkout

| 项目 | 状态 |
| --- | --- |
| 隔离 worktree | C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation |
| worktree HEAD | c3926c68fd14f270faa8b55234311071947fa080 |
| worktree 分支 | detached HEAD；本次使用独立 managed worktree |
| main / origin/main | 均为 c3926c68fd14f270faa8b55234311071947fa080 |
| 开始时 worktree 自身差异 | 无；随后只将主 checkout 的四个 tracked 用户差异补丁应用到隔离 worktree |
| 主 checkout | C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation，feat/m6a-performance-evaluation，HEAD 与 main 相同 |
| 主 checkout staged diff | 空 |
| 主 checkout tracked diff | 4 个文件，7727 bytes，SHA-256 6a212d4a037b337a98d28aa889339357fbe3c2ef3d585e24fd635c003ce5937c |
| 主 checkout 用户 untracked | 两份 M6A decisions、一份 M6A inventory、siamjdiscrmath.pdf；均留在主 checkout，未移动或改写 |

主 checkout 的四个未提交 tracked 文件是 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/PAPERS.sha256 和 docs/REFERENCE_MANIFEST.md。隔离 worktree 保留了这份原始补丁，再在其上增加本任务文档改动；主 checkout 中的文件、用户未跟踪材料和另一 M6A worktree 均未被本任务写入。论文、ZIP、构建物和原始日志没有加入版本库。

仓库中此前提到的 BB90 五迭代来源规格报告，在本次检查的 main、docs/reproduction、docs/decisions 和可见分支路径中未找到可作为文件引用的副本。此报告是当前 dated source-closure 结果，不冒称是对先前异会话报告的接收签字。

### 2.2 S34 与各准入门分别登记

| 门 | 本次核实状态 | 边界 |
| --- | --- | --- |
| S34 技术诊断 | docs/reproduction/BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md 在分支 codex/m6b-i-bmw16-s33-metrics，tip/报告提交 fbc4beed11f686f9e0b74ba89d23b7514e93ce24；报告文件 SHA-256 12f846ab69ba946dc3b29604959731d83944294f4a93c75dcd34c14dc7d3e5c2；不在 main | 技术复核，不是异会话独立接收；报告记录 inventory 7 PASS / 15 FAIL，诊断结果不升级正式性能验收 |
| M6A / V3 | OPEN | S34 不关闭 V3 |
| M6A / V4 LAN/WAN | NOT_ALLOWED / NOT_RUN | 原顺序仍要求先完成 V3 |
| BMW16-derived 候选安全 | CONDITIONAL_ACCEPTED（其既有条件结论） | 只保留在 Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL 身份；本次未复核或扩张其证明 |
| BB90 五迭代来源门 | NO_GO | 不能实现或启动 BB90+DCF 依赖验证 |

S34 报告在 main 之外的提交历史中；不把 main 上的文件缺失误报为已经合入。BMW16 候选与 BB90 算法来源门是不同对象，二者状态不得合并。

## 3. 来源版本、访问和文件清单

### 3.1 可验证来源表

| 来源 / 版本 | 页码、位置 | 正式来源与访问结果 | 本地副本、页数、SHA-256 | 与目标五迭代版本的关系 |
| --- | --- | --- | --- | --- |
| Bollobás–Brightwell，Parallel Selection with High Probability，SIAM J. Discrete Math. 3(1), 1990，DOI 10.1137/0403003 | 全文页码 21–31；此次只核对到出版方摘要，无算法号或正文页码可引 | [SIAM 正式记录](https://epubs.siam.org/doi/10.1137/0403003)。官方页面/摘要可读；PDF 与 ePDF 的直接出版方请求返回 HTTP 403。 | BB90 正文未取得，无正文 SHA-256、页数实测值或本地算法核读。主 checkout 的 siamjdiscrmath.pdf 是 535 页书目汇编，不是 BB90 正文，SHA-256 07e9a9dbf58a3adc2a10545d8818af350b1360ed30ea8e28da9e68ac8b0890c4。 | 唯一被 2024 论文 [12] 引用的原始文献。官方摘要仅称四轮、每轮 cn 比较、均匀随机顺序模型下 1-o(1) 成功；不能由摘要推出五迭代规格。 |
| Agarwal et al., Secure Sorting and Selection via Function Secret Sharing，CCS 2024 出版版 | 15 页 PDF；印刷 pp.3023–3037。目标叙述在印刷 p.3036（PDF p.14）；Valiant 迭代定义 §5.1 在印刷 p.3033（PDF p.11）；CA/定理 5.1 在印刷 pp.3034–3035（PDF pp.12–13）；引文 [12] 在印刷 p.3037（PDF p.15）。 | [MIT DSpace 正式记录](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)，记录标明 Final published version、CC BY、DOI 10.1145/3658644.3690359。 | 仓库忽略文件 Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf：15 页，1,249,281 bytes，SHA-256 18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c。 | 目标表述直接来源。它明确说五迭代 BB90 用于安全中位数成本对比，称线性比较；没有 BB90 逐轮定义。 |
| MIT 自动存档 ZIP | DSpace 页面列出 3658644.3690359.zip，1.13 MB；REST bitstream 标记 bundle SWORD、sequenceId 3 | 同一 MIT DSpace 正式记录。常规 /bitstreams/.../download 返回 HTTP 405；公开 DSpace REST 元数据和 /server/api/core/bitstreams/{id}/content 可访问。 | ZIP 1,179,822 bytes，SHA-256 59d0fbbd6e68fdb20e0215b94b89f69b4a76d410f3de2fdc23749e621801d040，MD5 b57534b5cd23bfbe8cce7ea493e38ca8；仅保存于系统 Temp。成员：mets.xml（5,945 bytes，SHA-256 02acc7d6cc252ee3fa827a9b235094d60777a47aa1d5dd020fe9276143de9dd8）；3658644.3690359.pdf（1,215,229 bytes，15 页，SHA-256 1e7a1d69ef7f18f5218defc9ab98e7ef6ee162077969f470c8ca00a0a573e7f2）。 | 实际检查完整成员表：只有 METS XML 与出版 PDF；没有 .tex、源稿、补充算法、代码、artifact 或其他附件。MIT ZIP 内 PDF 与仓库 PDF 字节哈希不同，未称二者逐字节相同。 |
| Braverman–Mao–Weinberg，Parallel Algorithms for Select and Partition with Noisy Comparisons，arXiv:1603.04941v1，2016 | PDF 44 页；Algorithm 5 在 PDF p.29（印刷 p.28）；Algorithm 7 在 PDF p.34（印刷 p.33）；Theorem 8 在 PDF p.11（印刷 p.10）。 | [arXiv v1 正式版本记录](https://arxiv.org/abs/1603.04941v1)。 | Papers/1603.04941v1.pdf：453,168 bytes，44 页，SHA-256 f46f83cba279f37e9e3aaa0d64b145fdfcfbc52c0c9c8b79bacec1259ca9ca23。 | 独立算法与分析参考，不是 BB90。Algorithm 7 是四轮 noiseless median Partition；不因同属随机比较选择就等同 BB90，也不向 BB90 转移概率、常数或安全结论。 |

### 3.2 MIT 附件检查方法

MIT 页面列出 PDF 和自动存档 ZIP。网页点击 ZIP 与普通下载 URL 返回 405；随后只使用该页面对应的公开 DSpace REST API 内容端点读取同一 bitstream。API 元数据确认文件名、大小、SWORD bundle 和 MD5；解包后检查成员名、字节数、magic、各成员哈希，并对唯一 PDF 检查页数。存档实际不含程序代码或算法补充。因此 ZIP 的存在本身没有被当成算法证据。

## 4. 五迭代表述的直接证据与关系裁决

### 4.1 2024 论文直接说了什么

印刷 p.3036（PDF p.14，Section 6 的 secure median selection 成本段）称：作者以“more efficient 5-iteration version of BB90 algorithm [12]”替换五迭代 AAV，并报告该 BB90 版本仅需线性于 n 的比较数；还报告在两个 (n, ℓ) 设置下相对 AAV 的性能改善。引文 [12] 在印刷 p.3037（PDF p.15），对应 Bollobás 与 Brightwell 1990。

这是论文直接报告的实验成本叙述，不是逐轮算法定义，也不是包含参数和概率条件的定理。Table 2/3 实际列出的五迭代 AAV 行不能当作 BB90 行。Section 5.3 明示作者只把“少数”算法转换到 Compare-Aggregate (CA) 模型，并把更完整的 survey 留给 full version；本次对 MIT 自动存档附件、作者页面、标题/变体/代码查询均未核到可验证的 full version 或 BB90 代码。

印刷 p.3033 的 §5.1（脚注 9）把 Valiant 算法中的算法比较轮称为 iterations，以避免与安全协议的 rounds 混淆。该定义说每次迭代给出一张图 H_i=(V_i,E_i)，E_i 为输入位置之间的无向比较边，后继图可依赖之前迭代结果。此术语说明使 2024 的“五迭代”应按算法迭代读；它没有给出 BB90 的五张图。

### 4.2 与 1990 原文关系

SIAM 官方摘要支持以下有限结论：原文选择 t-th highest；每种输入顺序等可能；存在四轮、每轮 cn 次比较、成功率 1-o(1)（n→∞）的算法；并有三轮 cn 比较不可能的下界。

因此，2024 所述的五迭代中位数版本在迭代数和目标范围上都不能直接认定为“1990 原算法照抄”。最小可靠表述是：**2024 作者称其为引用 BB90 的五迭代版本，并将它用于中位数安全选择成本分析；与原始四轮构造的具体变换关系未说明。**它可能是改写、参数化、CA 可用变体或其他派生版本；在取得定义前，这些都只是待验证设想，不能选定一种写成结论。

### 4.3 证据分类

| 分类 | 本任务可写入的内容 |
| --- | --- |
| 论文定义 | BB90 官方摘要的四轮/每轮 cn/随机顺序/渐近成功叙述；2024 的 Valiant 图、CA 定义、Theorem 5.1 通用编译条件；2024 对五迭代 BB90 中位数成本的文字报告。 |
| 本地参考行为 | BMW16 Algorithm 5/7 及测试/决策文档；VFSS 当前稳定 priority 编码、unsigned 小于阈值 DCF conformance、uCMP 接口。它们仅是本地行为。 |
| 项目推导 | Q20.12 signed score 到稳定 priority 的映射；若阈值选择正确则按 priority 做成员判断；满足 CA 编译假设时的条件轮数代入。 |
| 待验证设想 | 五轮各自的采样及图；对任意 K 的适配；有限 n 成功界；完整 Protocol I 消息轮数；BB90 图下 offline pool 的隐藏和条件模拟。 |

## 5. 逐轮算法规格缺口

当前来源只命名“五迭代 BB90 版本”。对 i=1,2,3,4,5，每一轮均没有找到以下可复核定义：

| 必需字段 | 迭代 1 | 迭代 2 | 迭代 3 | 迭代 4 | 迭代 5 |
| --- | --- | --- | --- | --- | --- |
| 进入轮次的状态（候选集、区间、rank、当前目标） | 未给 | 未给 | 未给 | 未给 | 未给 |
| 样本空间、抽样分布、样本量、是否有放回 | 未给 | 未给 | 未给 | 未给 | 未给 |
| 幂次/实数转整数的取整和小规模边界 | 未给 | 未给 | 未给 | 未给 | 未给 |
| pivot 集合产生及 pivot 次序统计规则 | 未给 | 未给 | 未给 | 未给 | 未给 |
| V_i、E_i 的节点/边生成公式 | 未给 | 未给 | 未给 | 未给 | 未给 |
| 本轮比较结果如何聚合/更新状态 | 未给 | 未给 | 未给 | 未给 | 未给 |
| 停止条件、失败条件、abort、重试及输出 | 未给 | 未给 | 未给 | 未给 | 未给 |
| 该轮单独及累计复杂度 | 未给 | 未给 | 未给 | 未给 | 未给 |

因此无法给出不是猜测的伪代码，也无法冻结任一轮边集。缺少的不是一个可由 oracle 补出来的输出规则：比较图和随机状态本身决定理论算法、概率和预处理材料身份。

## 6. 正确性、输入范围和随机保证

### 6.1 已知模型与未闭合保证

- BB90 官方摘要只揭示随机输入顺序（各顺序等可能）和渐近的 1-o(1) 成功表述。此次未取得正文，故没有可复算的失败项、n 下界、常数、定理条件或参数。
- 2024 论文只对五迭代 BB90 说中位数选择和线性比较数；该句没有定理编号、误差概率、输入排列分布、采样独立性、参数或有限 n 界。
- 不得将原 BB90 的渐近概率自动赋给 2024 的五迭代版本；也不得从“线性比较数”反推出其成功概率。
- 对五迭代版本的正确性和成功概率结论均为 **UNPROVEN / SOURCE_MISSING**。有限 n 可计算失败界、目标成功率、错误输出与 abort 的区分、重试合同均未知。

若日后完整原文只证明 median，本项目仍只能先实现 median-only TEST_ONLY；任意 K 是另一个需要来源或证明的扩展。原文摘要的任意 t 结论不会自动覆盖 2024 的 median 派生五迭代版本。

### 6.2 项目稳定顺序映射（仅项目推导）

本项目冻结语义为 Q20.12 signed score，并按 score 降序、original_index 升序稳定取 K 个。现有 Protocol I 本地编码把 raw signed 32-bit score 转成 unsigned 有序高位，再反转高位并拼接原始下标：

    ordered = raw_score XOR 0x80000000
    priority = ((UINT32_MAX - ordered) << index_bits) | original_index
    index_bits = max(1, ceil(log2(n)))

因此目标阈值应当对应 priority 升序的第 K 项；最终成员谓词是 priority_i ≤ tau，而不是只比较 raw score 是否大于等于阈值分数。此映射是项目语义和本地代码行为，不是 BB90 定理。还需证明所用 Select 允许输入该复合严格全序，以及随机打乱、original_index 与节点顺序的关系符合它的概率模型。

2024 §5.2 对相等项目给出 local stable rank 定义：节点集合有序，边端点依节点序规定 i<j。若项目通过 shuffle 更改槽位，槽位序可能不同于 original_index；必须将稳定复合键作为比较对象或证明 CA 稳定 tie 与项目稳定 tie 一致，不能默认两者相同。

## 7. 比较成本表与计数口径

2024 §5.1 的论文定义是 e_A(n,k)=Σ_i |E_i|、v_A(n,k)=Σ_i |V_i|。每个 E_i 是无向边集合；同一无序对若跨迭代再次出现，会再次计入总迭代边量。对随机算法，论文把边、节点总数视为随机变量，并使用其期望。具体 BB90 图未公开，故这些定义不能给出数值。

| 对象 | 每轮/总比较量 | 图节点与边 | 跨轮重复 | 有向调用 / 唯一材料槽 / DCF Eval |
| --- | --- | --- | --- | --- |
| BB90 1990 摘要 | 四轮，每轮 cn，故总量至多以 4cn 表示；c 的具体值在摘要不可得 | 正文图、节点数均未核读 | 未知 | 摘要不定义工程调用与材料槽 |
| 2024 五迭代 BB90 | 总量仅称 O(n)；分轮上界及线性常数未知 | V_i、E_i 和 e_A、v_A 未公开 | 未知；无法冻结边集 | 全部 NOT_MEASURED / 未定义 |
| Theorem 5.1 CA 编译 | 条件式在线计算为 2e_A·DCF.Eval（跨两个 online parties）；e_A 未知 | 通信另依赖 v_A | 按各轮 E_i 累计 | 这是论文通用编译计数，不是五迭代 BB90 实测或材料池证明 |
| 项目 DCF membership | 逻辑上每个输入位置要产生成员位；具体协议调用与材料数尚未制定 | 不计入 BB90 E_i | 不适用 | NOT_MEASURED；需与比较图调用分开报告 |

若以后实现，必须同时保留：逻辑比较调用数、每轮无序比较关系数、跨轮重复关系数、唯一材料槽数、两方 DCF Eval 总次数和材料字节。不得将 Theorem 5.1 的 2e_A 当作项目已测 DCF 调用，也不得把最终成员判断混入 BB90 比较数。

## 8. 迭代、编译轮次和完整路径映射

| 口径 | 当前值 | 证据与限制 |
| --- | --- | --- |
| 2024 报告的算法迭代数 | 5 | 2024 印刷 p.3036 的明确称呼；逐轮图缺失 |
| 明文并行比较依赖层 | 报告为 5 个 Valiant iterations；精确数据依赖未能审计 | §5.1 将 iteration 定义为一批图比较；BB90 的 H_i 未公开 |
| CA 编译核心轮 | **条件值 2·5+1=11** | 仅当目标算法确为满足 Theorem 5.1 前提的 CA 算法，并且其 k=5、边/节点复杂度及功能接口对应时成立。不能将该条件值写成已核实 BB90 协议轮数。 |
| Protocol I 论文核心 | 3 轮目标 | Protocol I 自身 Theorem 4.1 核心口径；不能与 BB90 CA 编译核心直接相加或自动替换。 |
| Protocol I + BB90 核心 | 未推导 | 缺 BB90 图、CA 转换和完整消息依赖。 |
| raw score 到原序 Top-K mask | **NOT_DERIVED / NOT_MEASURED** | 另含 score-to-key、shuffle、稳定阈值、DCF membership、inverse route、mask share 输出及适配。 |

消息因果草图（括号中标明证据性质）：

    raw Q20.12 score + original_index
      -> Protocol I score/key 表示转换（本地适配行为；carry/sign 分段）
      -> 初始秘密 shuffle（Protocol I 组件）
      -> [若且仅若 BB90 可表示为 CA]
         对 i=1..5：
           masked input reveal -> FSS/DCF 比较聚合 -> local-rank reveal
           -> 依据已揭示 rank 和秘密/公开状态形成 H_(i+1)
      -> 选出并保持秘密共享的稳定 priority 阈值 tau
      -> 对各原始/打乱位置做 DCF membership，得到成员位共享
      -> 按隐藏 shuffle 的逆路由相关性还原输入顺序
      -> 输出 XOR/目标域共享 Top-K mask

此图是所需接口的因果依赖草图，不声称五个 BB90 图或各消息已定义。Theorem 5.1 对其 CA 核心给出 masked reveal、local-rank/重排的通用路线和 2k+1 轮公式；论文本身也指出 2024 中算法迭代与安全协议轮不同。输入编码、初始化 shuffle、终止选择、最终 DCF membership、逆路由和共享输出必须在实现 DAG 上逐项计轮。不得把 3、11 相加或宣称完整路径为 11/14 轮。

## 9. Protocol I 接入与安全/预处理交接

### 9.1 候选阶段和角色

| 阶段 | 所需功能 | 可复用范围 |
| --- | --- | --- |
| 原始输入 | 两方持有 signed Q20.12 shares 与稳定原始下标语义 | 统一 oracle 和 score 输入合同可复用；现有编码不是 BB90 定理 |
| CA/Select | 依据比较结果迭代产生下一张图并输出稳定阈值 | 必须等来源和逐轮图闭合；不得使用 BMW16 图代替 |
| offline T | 为未知 online 输入和比较提供输入无关、单次绑定材料并保持在线静默 | 材料生成、edge/session/party 绑定、消费顺序和访问图泄露需为 BB90 图重审 |
| online P0/P1 | 用 shares、masked reveal 与各自 FSS key eval 完成比较和 rank 聚合 | 2024 定理可作为一般构造参照；现有 BMW16 whole-pool 安全证明不转移 |
| 阈值成员与输出 | DCF 生成成员位共享；逆路由回原序并输出 mask | 接口想法可复用；新阈值表示、方向、pool 绑定和完整证明未完成 |

### 9.2 DCF ABI 核对结果

本地 VFSS DCF conformance 路径构造的是无符号输入 x 与 alpha 的严格谓词 x<alpha，keyGenDCF(bin, 1, threshold, 1)，输出组位；测试验证 evalDCF 两方重构此严格小于。Protocol I priority 编码是 32+index_bits 位无符号复合键。Protocol I 的 uCMP 测试则是对两个被掩码值作严格比较，接口/材料形态不同。

所以可复用的只是“现有 DCF 有经过 conformance 的 unsigned strict-less 原语”这一局部事实。项目要求 priority_i≤tau；阈值在线产生，须单独处理 inclusive 边界、域最大值、稳定索引拼接、keyGen 输入表示、在线阈值与预处理时序。单 key DCF 的 source-specific 假设是否覆盖该调用 ABI、输出方向和组合视图也需要逐项对齐。当前 conformance 测试不证明离线材料可在未知 tau 下预发，更不证明自适应 BB90 图安全。

### 9.3 必须重新证明的条件

- H_i、自适应选择的样本/pivot、边地址和使用顺序是否向任一 online party 或 T 泄露；公开 rank 是否足以推知后续结构。
- T 生成的 pool 对所有允许输入、五层图和失败路径是否输入无关；材料槽如何隐藏绑定到未来的边；是否可重复、是否允许补取。
- 单方视图模拟需覆盖已用/未用材料槽、adaptive transcript、随机 abort 与 retry；BMW16 whole-pool/adaptive-query/shuffle-output 条件模拟不能直接转移。
- DCF 需按 ABI 确认域、组、阈值方向、严格/非严格边界、Q20.12 与 stable-index 表示，以及输入值是否为论文假设的随机 masked field values。
- 逆路由须保持 score、original_index、成员 bit 绑定；需证明 mask 回到原始输入顺序且输出恰好 K 位为 1。
- 协议端到端消息 DAG、材料字节、在线消息和泄露须独立审查。

## 10. BMW16 对照路线（独立登记）

BMW16 v1 与 BB90 是不同论文。其 Algorithm 5 是 noiseless 模型下的两轮近似中位数 Partition 算法；Algorithm 7 是使用 Algorithm 5、两个 dummy-padded runs、U/V/W 采样与末轮精确处理的四轮 O(n) 中位数 Partition。Theorem 8 的成功表述为 0<epsilon<1/18 时至少 1-exp(-Omega(n^epsilon))。该论文和其项目派生实现不能回答 2024 五迭代 BB90 缺失的采样图。

既有 BMW16_DERIVED_SELECT_4R_SOURCE_AND_COMPOSITION_DECISION_2026-10-05.md 对逐字实现论文伪代码记录了独立的有限 rank 阻塞：Algorithm 5 初始化 k1=n/2 可能大于 A1 可达 rank，Algorithm 7 的 n/2−|R(2)| 在声称成功的路径上可能为负。这是对可执行伪代码闭合的审计，不是对 Theorem 8 的反证。已经派生出的 BMW16 项目方案必须沿用自己的证明、测试和条件安全证据；不能因为它可被另行推导/实现而成为 BB90。

已有 BMW16-derived Select + DCF 应保持独立 PROJECT_DERIVED / EXPERIMENTAL 标签、证据目录、概率推导与安全结论。本次没有修改其代码、边计数或条件安全结论。若团队另行希望定义一个 BMW16 派生的五层方案，须建立新的独立规格/标签，写清与 BMW16 Algorithm 7 的关系及新的复杂度和概率证明；不能把现有四层版本机械加一层，也不能将其命名为 BB90。本报告不建议现在把 BB90 源闭合目标切换为未定义的五层算法。

## 11. 已尝试来源、复跑命令和结果

| 范围 | 命令/入口 | 结果 |
| --- | --- | --- |
| SIAM 官方出版全文 | Python urllib GET https://epubs.siam.org/doi/pdf/10.1137/0403003 与 https://epubs.siam.org/doi/epdf/10.1137/0403003 | 均 HTTP 403；只依据官方页面元数据和摘要，不绕过登录/付费控制 |
| MIT DSpace ZIP 普通下载 | 页面 ZIP 链接或 GET https://dspace.mit.edu/bitstreams/33f96cda-a328-4cbe-a31d-d6f68b7c0286/download | HTTP 405 |
| MIT DSpace REST | GET https://dspace.mit.edu/server/api/core/bitstreams/33f96cda-a328-4cbe-a31d-d6f68b7c0286 以及末尾加 /content | HTTP 200；读取并检查实际 ZIP 内容，见第 3.1 节 |
| ACM DOI 文章页 | 官方 DOI 页面经浏览工具打开 | 403；MIT 正式仓储 PDF 可读，作为会议版复核来源 |
| 作者/机构稿 | 查询作者正式个人/机构发布目录和题名、DOI、full version、BB90/五迭代关键词 | 未核实到可归属的 BB90 全文稿；Brightwell 机构主页核实其作者身份，未提供该文 PDF |
| 第三方开放版本线索 | DBLP 的 BB90 书目记录搜索结果标有 “unpaywalled version”；尝试打开 DBLP 记录与镜像页 | 线索目标 URL 未能从结果页解析；页面受 Anubis 拦截，标准 JSON/XML/HTML 请求又遇 TLS 证书验证失败。没有获得全文，未作为算法定义来源 |
| 代码、supplement、artifact、errata | 题名 + GitHub/code/artifact/supplement/errata 搜索；MIT SWORD ZIP 实际解包 | 未发现可验证 BB90 代码、算法补充、勘误或额外附件 |
| BMW16 | 本地 PDF，PyMuPDF 提取 PDF pp.29、34、11；另读既有 dated source decision | 可复核 BMW16 Algorithm 5/7 和 Theorem 8；其身份与概率界与 BB90 隔离 |

MIT REST ZIP 可用如下只读命令复跑并验证其成员（输出文件写入临时目录，不写入仓库）：

    python -c "import urllib.request,zipfile,io,hashlib; u='https://dspace.mit.edu/server/api/core/bitstreams/33f96cda-a328-4cbe-a31d-d6f68b7c0286/content'; b=urllib.request.urlopen(u).read(); print(len(b),hashlib.sha256(b).hexdigest()); z=zipfile.ZipFile(io.BytesIO(b)); print([(x.filename,x.file_size) for x in z.infolist()])"

SIAM PDF 的复跑只用于确认出版方访问状态：

    python -c "import urllib.request; u='https://epubs.siam.org/doi/pdf/10.1137/0403003'; print(urllib.request.urlopen(u).status)"

预期为 HTTPError 403。若审查者具有合法图书馆权限，应通过自己的正常授权访问获取正文，不用未授权镜像替代。

## 12. 准确缺项、来源目标与下一步

| 缺项 | 完成所需证据 | 当前状态 |
| --- | --- | --- |
| 原始 BB90 的正文算法和证明 | SIAM 合法全文、作者稿或作者/机构授权副本；记录版本和页码 | 出版记录已确认，全文未取得 |
| 五迭代 BB90 的唯一版本身份 | 2024 作者稿/补充材料中明确版本，或作者说明其对 BB90 的修改与来源 | 未核实 |
| 五轮全部状态、抽样、pivot、边图和更新 | 每轮完整伪代码/公式，含整数取整、边界、停止/失败分支 | 五轮均缺 |
| 正确性和概率定理 | 假设、随机变量、输入顺序分布、n 与参数范围、失败概率表达式/常数 | 未核实；有限 n 界无来源 |
| 目标范围 | 是仅 median 还是任意 t/K；目标排列、稳定同分语义 | 2024 句子只说 median；不能扩展 |
| 复杂度常数 | 每轮 |E_i|、总边/节点复杂度及重复边语义 | 仅有“linear in n”文字，常数未知 |
| CA 编译资格 | 该五迭代图如何只依赖可揭示 local-rank，及 Theorem 5.1 对应接口 | 2024 未列算法步骤；只能作条件推导 |
| Protocol I 组合 | 完整共享阈值、DCF inclusive membership、离线池隐藏/单次用、逆路由与 mask DAG | 项目扩展，尚未设计/证明 |

### 给学姐的最小确认问题

1. 您所说的“五迭代 BB90”具体依据哪一个版本？请给出题名/作者/年份、页码、算法号、讲义或仓储链接；若是 2024 作者的改写，请指出 full version、补充材料或代码 commit。
2. 能否提供五轮完整伪代码或明确的逐轮公式：样本集合/样本量/抽样分布、取整、pivot 规则、每轮边集、更新、结束及失败/abort/retry？
3. 该版本只处理 median，还是任意 t-th/项目 K？成功率是对均匀随机输入顺序、算法抽样，还是两者；定理条件和有限 n 失败界是什么？
4. 1990 论文摘要称四轮，2024 称五迭代。第五轮及其余变化是原文之外的参数化、CA 改写还是安全适配？具体是谁做的，证明在哪一稿？
5. “线性比较数”的每轮/总边数常数、节点数、跨轮边重复和失败策略分别是什么？若作者副本/代码不能公开，能否提供可供独立实现者复核的授权稿页或完整规格？

### 下一阶段入口

下一任务建议：**M6B-I-BB90-5ITER-SOURCE-CLOSURE-S2**，仅执行以下来源交付：

1. 收到可合法核读的 BB90 全文或带准确页码的作者稿；
2. 收到可定位到来源的五轮完整伪代码/参数/定理，或让作者明确确认 2024 仅是成本估计而无可提供算法细节；
3. 对原四轮与五迭代版本逐项对照，闭合任意 rank/median 范围、失败合同、线性常数、有限或渐近概率的适用条件；
4. 独立研究者据此可重写每轮并复算边数和概率，随后再冻结 TEST_ONLY 规格。

若作者无法提供上述材料，保留 SOURCE_SPEC=NO-GO_FOR_IMPLEMENTATION。不要改走启发式或用 BMW16 派生方案填补来源。

## 13. 后续明文 TEST_ONLY 的精确准入

只有以下条件全部满足，才新建隔离名称如 TEST_ONLY_BB90_5ITER_SELECT（若来源只支持 median，则名称与 API 必须明确 median-only）：

1. 每轮状态、抽样、取整、pivot、比较图、更新和终止/失败规则均由可定位来源或逐步证明唯一确定。
2. 正确性及概率定理的模型/参数与实现相同；给出有限规模目标，或明确算法仅对渐近随机模型作研究性验证且不可用于项目准入。
3. 任意 K、稳定 original_index 和 Q20.12 的项目映射若不在论文范围，单独给出扩展证明；不得偷偷纳入 median 算法。
4. 冻结每轮边集定义、随机种子合同和失败样本保存规则。
5. 按用户要求实现 TEST_ONLY，与冻结 stable K-th oracle differential，覆盖全排列、重复/边界值、K 边界及每轮边数审计。
6. 测试结果只表示代码符合已闭合的规格，不替代论文概率证明，也不构成安全协议实现。

当前任何一项均未因本报告而通过。无 secure runtime、DCF 集成或正式 LAN/WAN 性能测试授权。
