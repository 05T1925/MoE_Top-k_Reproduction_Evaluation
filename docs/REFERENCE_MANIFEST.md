# 本地参考资料清单

更新日期：2026-09-13。

## 1. 状态与用途

本清单定义远端仓库与本地参考资料的边界，记录资料用途、来源、版本和复用限制。

本清单不删除、移动或修改本地参考目录。Git 忽略规则用于防止论文、完整参考工程和生成物被普通 `git add .` 意外加入远端历史。

本次范围调整：

- CipherGPT 改为历史参考，不再作为当前实施基线或性能比较对象。
- 保留已有 CipherGPT 资料，不因取消任务而删除历史来源。
- 补充 BB90 原论文、采用版本和代码来源的登记要求。
- 区分已核实的文献信息、尚待取得的副本、待选择的算法版本和未确认的代码来源。

远端仓库主要追踪：

- `VFSS/`：唯一活动实现目录；
- `VFSS-baseline/`：冻结的源码级恢复与比较基线；
- `README.md`、`PROJECT.md`、`AGENTS.md`、`.gitignore`；
- `docs/`：来源、复检、决策、实施计划和验收规范；
- `.github/`、`.agents/skills/`：协作模板与仓库工作流；
- 经审查的项目源码、测试及必要脚本。

本地资料的放置、校验和按分工准备方式见 [本地参考配置](LOCAL_REFERENCES_SETUP.md)。当前任务范围以 [项目总纲](../PROJECT.md) 和 [新路线决策](decisions/ROADMAP_PRIORITY_2026-09-13.md) 为准。

| 本地路径 | 当前角色 | Git 状态 | 原因 |
| --- | --- | --- | --- |
| `Papers/` | 论文、学习笔记及历史测试要求 | 本地保留、忽略 | 版权与再分发状态未逐项确认 |
| `ADSMPC/` | Protocol III 与 CipherGPT-style 旧原型 | 本地保留、忽略 | 含构建树、实验输出，且不是目标运行时 |
| `Agarwal_TopK/` | Protocol I、CA、部分 Protocol III 算法参考 | 本地保留、忽略 | 历史工作包约 2.4GB，含归档、依赖、生成物和嵌套 Git 数据 |
| `CipherGPT/` | 原生 Top-K 及旧实验的历史参考 | 本地保留、忽略 | 本轮已取消实施；原包仍包含依赖、构建树和实验结果 |

上述大小是历史工作包记录，不作为当前文件完整性依据。

## 2. 既有本地目录的保留规则

```text
Papers/
ADSMPC/
Agarwal_TopK/
CipherGPT/
```

“被 Git 忽略”不等于“可以删除”，“取消实施”也不等于“删除历史资料”。

既有目录继续用于论文核对、代码追踪、失败模式分析及历史结果解释。新环境只需准备当前分工必需的子集，不要求为推进 M2、M5、M6A 或 M6B 下载完整 CipherGPT 工作包。

本次修订不要求：

- 删除 CipherGPT 论文或源码；
- 清理其历史分支；
- 修改旧实验记录中的真实来源；
- 强制迁移或重命名本地目录；
- 新建一个尚无来源的 `BB90/` 参考工程目录。

新增 BB90 资料时，先登记来源和副本信息，再按本地配置规则放置。

## 3. 主要参考入口与复用边界

| 对象 | 主要入口 | 用途与边界 |
| --- | --- | --- |
| Protocol I B0/B1 | `Agarwal_TopK/protocol1/`、`Agarwal_TopK/protocol1_ca/` | 全对全排名、shuffle、CA 与测试参考；不直接复用旧 FSS ABI |
| Protocol III 局部参考 | `Agarwal_TopK/protocol3_ca/` | DCF conformance 和明文 AAV86 图测试，不是完整 Protocol III |
| Protocol III 旧原型 | `ADSMPC/src/protocol3.cpp`、相关 `RankingPhase.h`、`routing_dpf.h` | 调用顺序与失败模式参考；文件轮询、明文 Dealer 和旧密钥格式不迁移 |
| AAV86 原型 | `Agarwal_TopK/protocol1_ca/`、`Agarwal_TopK/protocol3_ca/` | 算法及局部图执行参考；不自动证明自适应预处理和 Dealer 静默条件 |
| BB90 原论文 | 见第 4 节 | 顺序统计量选择算法来源；具体采用变体需另行固定 |
| BB90 参考代码 | 尚未确认，见第 5 节 | 不预设作者代码或可直接迁移的实现存在 |
| CipherGPT 原生历史参考 | `CipherGPT/src/globals.cpp`、`CipherGPT/test/Top_K_paper_test.cpp`、`CipherGPT/src/shuffle.cpp` | 历史原生 Top-K 行为及问题追踪；本轮不修复、不适配、不测性能 |
| CipherGPT-style 旧实验 | `ADSMPC/src/ciphergpt_topk_dcf_shuffle.cpp` | 历史 FSS/DCF 对照原型，不是原生 CipherGPT，也不是 BB90 |
| 历史测试要求 | `Papers/测试指标.md`、`Papers/Agarwal与CipherGPT实验对比.pdf` | 统一输出、矩阵和指标的历史来源；当前要求以更新后的项目文档为准 |

旧参考代码只能支持其实际验证过的行为，不能反向成为论文定义或当前安全性证明。

历史测试资料中的 CipherGPT 比较安排不再构成现行任务。保留其中的来源记录，不据此恢复 M4。

## 4. BB90 原论文与采用版本

### 4.1 已核实的原始文献

BB90 在本项目中指：

> Béla Bollobás and Graham Brightwell.  
> *Parallel Selection with High Probability*.  
> SIAM Journal on Discrete Mathematics, 3(1): 21–31, 1990.  
> DOI: 10.1137/0403003.

正式来源：

[SIAM 出版页面：Parallel Selection with High Probability](https://epubs.siam.org/doi/10.1137/0403003)

引用年份采用期刊出版年份 1990。出版网站另列的在线发布日期不改变 BB90 的文献年份。

### 4.2 当前登记状态

| 项目 | 状态 |
| --- | --- |
| 作者、题名、期刊、卷期、页码及 DOI | 已核实 |
| 文献基准 | 上述 1990 年正式期刊论文 |
| 本地 PDF 路径 | 待登记 |
| 本地 PDF SHA-256 | 待取得实际副本后计算 |
| 全文阅读与算法步骤核对 | 待完成 |
| 本项目具体采用的算法变体 | 待设计阶段固定 |
| 算法参数与适用 K 范围 | 待核对 |
| 参考代码仓库、revision 和许可证 | 尚未确认 |

“文献基准已确认”不等于“本地副本已经取得”，也不等于“算法实施版本已经选定”。

不得填写推测的哈希、下载地址、代码 revision 或许可证。

### 4.3 采用版本必须明确的内容

在 M6B 核心实现前，记录：

1. 采用论文中的哪一算法、定理或构造。
2. 对应节号、页码及必要参数。
3. 输入顺序、随机性和适用规模假设。
4. 目标顺序统计量与项目第 K 大稳定优先级的映射。
5. 正确性保证、概率成本界和失败事件的区别。
6. 是否采用 Agarwal 所讨论的变体或另行推导的版本。
7. 从比较模型到 CA 模型的转换步骤。
8. 与论文原算法相比的修改及其证据。
9. 重复值与 original index 稳定同分的处理。
10. 最终阈值、DCF 成员选择和共享 mask 的接口。

Agarwal 会议版提及 BB90 选择路线，不等于已完整给出本项目所需算法变体和全部实现细节。不能仅依据名称或摘要固定轮数与参数。

算法版本一旦确定，应记录在对应设计文档，并由实现及性能结果引用。更换变体时保留版本关系，不覆盖旧结果。

### 4.4 BB90 与项目组合扩展

以下内容应分别标注来源：

| 内容 | 证据身份 |
| --- | --- |
| BB90 原始选择算法 | 原论文定义，具体范围以全文核对为准 |
| Agarwal 对选择算法及 CA 的讨论 | Agarwal 论文中的相应结论 |
| 具体 CA 图实现 | 需映射到来源及独立验证 |
| 稳定第 K 大阈值表示 | 项目语义适配 |
| 阈值与 DCF 的安全衔接 | 项目组合设计 |
| Protocol I/III 两种 BB90+DCF 路线 | 项目组合扩展 |
| 原顺序秘密共享 Top-K mask | 项目统一输出适配 |

不能把完整 BB90+DCF Top-K 组合直接表述为 BB90 原论文已经提供的安全协议。

## 5. BB90 代码来源与登记规则

### 5.1 当前代码来源状态

当前清单尚未确认：

- BB90 作者发布的官方实现；
- 已核实可复用的第三方 BB90 仓库；
- 本地某个目录已经实现本项目采用的 BB90 版本。

这表示来源尚未确认，不表示公开代码一定不存在。

`Agarwal_TopK/` 或 `ADSMPC/` 中的 AAV86、Direct Top-K、pivot-pruning、QuickSelect 等实现不能仅因功能相近而登记为 BB90。

### 5.2 若采用外部代码

必须登记：

| 字段 | 要求 |
| --- | --- |
| 仓库或归档地址 | 可访问的原始来源 |
| 作者或维护者 | 明确主体，不推定为论文作者 |
| 来源类型 | 作者实现、第三方实现或其他 |
| revision | 固定 commit/tag；不能只写分支名 |
| 归档哈希 | 使用实际文件计算的 SHA-256 |
| 算法对应 | 论文算法、变体及差异 |
| 入口路径 | 实际采用的源码和测试文件 |
| 许可证 | 许可证文件及适用范围 |
| 复用方式 | 阅读参考、适配、复制片段或源码快照 |
| 本地修改 | 修改范围、原因及 patch/revision |
| 验证 | 明文 oracle、边界和随机性测试 |
| 排除内容 | 依赖、构建物、数据、密钥和嵌套 Git |

登记和审查完成前，不把整套代码强制加入远端历史。

### 5.3 若自行实现

若按论文独立实现，应登记为“本项目依据论文实现”，而非“迁移作者代码”。

必须记录：

- 采用文献和算法版本；
- 设计文档；
- 实际源码入口；
- 首个实现 commit；
- 后续修改 revision；
- 原算法、CA 转换和安全组合的对应关系；
- oracle 与测试证据；
- 是否使用过第三方代码片段及其来源。

新实现进入 `VFSS/`。具体文件路径在代码实际建立后登记，不预先写成已有入口。

### 5.4 代码行为不能替代来源证明

即使某个程序能输出第 K 大或正确 mask，也不能仅凭行为认定其实现 BB90。

需要核对算法步骤、参数、比较图、随机性和复杂度条件。不能把其他选择算法改名后用于 BB90 性能结论。

## 6. 论文副本与哈希管理

当前已登记论文副本的 SHA-256 位于：

`docs/PAPERS.sha256`

哈希用于确认具体字节副本一致，不表示拥有再分发权，也不自动证明资料是真实完整版本。

新增 BB90 副本时：

1. 记录正式来源和获取日期。
2. 核对作者、题名、卷期、页码及正文完整性。
3. 登记实际本地路径。
4. 对实际文件计算 SHA-256。
5. 将准确路径和哈希加入清单。
6. 双方按同一记录核对副本。
7. 在算法设计中注明采用的文献版本。

同一论文的出版版、作者稿或不同扫描副本可能有不同哈希。不能仅因书目信息相同就认定字节副本一致。

已有副本发生变化时保留变更说明，不静默替换旧哈希。未知值标为待登记，不使用占位哈希。

本次修改清单不表示已下载 BB90 PDF，也不表示已经更新 `docs/PAPERS.sha256`。

## 7. CipherGPT 历史参考边界

CipherGPT 原生代码和论文继续保留，用于解释旧计划、旧实验和已有参考行为。

当前不再安排：

- 原生 Top-K 终止与错误传播修复；
- 稳定同分或 original-index 适配；
- 原生秘密共享 mask 输出；
- CipherGPT-style VFSS adapter；
- CipherGPT 性能实验；
- CipherGPT 与六种方案的正式比较。

旧代码中曾记录的终止、同分、错误处理或测试重构问题，仍属于历史行为说明，不转化为当前待办任务。

保留资料不意味着将其作为已验收基线。取消任务也不意味着可以删除目录、改写旧报告或隐去原有代码来源。

## 8. 普通 Git 历史明确排除项

- 论文 PDF 和未确认可再分发的资料；
- `build/`、`CMakeFiles/`、对象文件、静态库和动态库；
- 生成的 DCF/DPF keys、离线包、秘密掩码；
- 临时通信文件和原始日志；
- benchmark 原始输出、结果 CSV 和本地临时输入；
- `Agarwal_TopK/protocol1_ca/frozen_b1_stage4c/` 等运行产物；
- `Agarwal_TopK/frozen_inputs/` 下的压缩依赖或源码包；
- 参考包内嵌套 `.git` 元数据；
- 未审查的 BB90 外部源码包及其捆绑依赖。

项目自己的实验脚本、来源说明和可审计汇总文档可以按正常评审追踪。它们不能包含秘密材料，也不能替代可定位的原始结果。

完整资料存放于本地或另行约定的位置，不通过强制添加目录绕过忽略规则。

## 9. 后续共享方式

队友确实需要相同参考代码时，为该份资料确定一种经过审查的主要分发方式：

1. 在 `reference-snapshots/` 放置最小、只含源码的快照；
2. 使用不可变外部归档，记录 SHA-256 和获取方式；
3. 仅在资料必要、许可允许且普通 Git 不适合时使用 Git LFS。

采用前记录：

- 来源及作者；
- 许可证与再分发状态；
- revision 或归档哈希；
- 包含和排除路径；
- 本地修改；
- 构建与验证说明。

以上是可选择的共享方式，不表示 `reference-snapshots/` 或相关 LFS 配置已经建立。

BB90 原论文、第三方代码和本项目实现分别审查来源。论文可阅读不等于其 PDF 可公开再分发；代码可下载也不等于可以无条件复制。

不得为了方便直接强制加入整套本地参考目录。

## 10. VFSS 基线校验

冻结基线提交：

```text
993696e
```

冻结标签：

```text
vfss-baseline-2026-09-03
```

2026-09-04 的历史复检记录：排除构建产物和 `.DS_Store` 后，`VFSS/` 与 `VFSS-baseline/` 的 131 个源文件逐文件一致，`diff -qr VFSS VFSS-baseline` 返回 0。

初始归一化清单 SHA-1：

```text
972fcd23187ecb603b22e37a2fefe608b2cb830b
```

此后 M1、M2、M3 已在 `VFSS/` 中形成活动实现，因此活动树与冻结树存在预期差异。

2026-09-04 复检确认 `VFSS-baseline/` 相对冻结标签未变化。后续不得同步修改冻结树来消除活动实现差异。

上述是历史校验记录，不表示本次修改清单重新执行了基线校验。

## 11. 后续登记任务

| 任务 | 负责方式 | 完成标志 |
| --- | --- | --- |
| CipherGPT 用途改为历史参考 | 文档负责人修改，另一方复核 | 不再作为现行实施或性能前置 |
| BB90 本地论文副本 | 获取方登记，另一方核对 | 路径、来源、版本和 SHA-256 完整 |
| BB90 算法变体 | M6B 设计双方共同确定 | 对应算法、参数和保证明确 |
| BB90 外部代码审查 | 实际采用方登记 | URL、revision、许可证及入口明确 |
| 项目自行实现来源 | 实现负责人登记 | 设计映射、源码入口和 commit 可追溯 |
| 资料配置与哈希同步 | 文档和资料负责人协调 | 本清单、配置说明和哈希记录一致 |

来源、版本或代码尚未确认时保持待登记状态。不得为了填满清单而虚构副本、仓库、许可证、revision 或完成结论。

## 12. M6B S20 DCF 安全复核来源与对象

- 被复核运行时：S18 锁定的 S15 默认关闭修复快照，72 项，S15 manifest SHA-256 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816，canonical source SHA-256 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。逐文件哈希清单见 S18 source/build/evidence manifest；S20 对 72 项重新核验，72/72 匹配。
- DCF 论文来源：Boyle et al., Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation, Microsoft Research 作者稿 PDF，Figure 1 第 13 页、Theorem 2 第 14 页、Appendix F.1 第 42–46 页。官方作者 PDF：[Microsoft Research PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。S20 未下载或添加论文本地副本。
- S19 条件推导：见 M6B_I_PROJECT_DERIVED_SELECT_5ITER_S19_REPORT_2026-10-16.md。其 epsilon_RNG_joint 仍为未量化的外部假设。
- S20 非作者密码学复核：审查者 `/root/s18_independent_receipt` 只参与过 S18 功能接收，未参与 S19 撰写；其只读复核支持单次 UCMP 与 key-only 池投影的 CONDITIONAL 结论，并指出 carry→sign continuation 是首个开放混合步骤。完整 adaptive pool 与 shuffle/output/crash 联合视图仍 OPEN；未运行构建/测试，未发现可复现攻击不构成安全通过。
- S20 决策、报告、机器门禁与 source/evidence manifest：见对应 M6B_I_PROJECT_DERIVED_SELECT_5ITER_S20 文件。S20 不修改 runtime，不改变 S18 功能接收范围，不创建 secure alias。
- M6A 性能状态不变：V3 OPEN，V4 未获准；S20 未运行正式 LAN/WAN 性能矩阵。

## 13. M6B S21 carry→sign 安全审查来源与对象

- 运行时对象：S18 锁定的 S15 默认关闭修复快照，72 文件；S15 manifest SHA-256 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816；S18 manifest SHA-256 5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78；canonical source SHA-256 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。S21 复核 72/72。
- 实际位宽：protocol_i_score_input.cpp 使用 kScoreBits=34，score carry/sign 的 ring 为 R34=Z/(2^34)。S21 reviewer 初稿中的 2^53 是错误表达，已更正；Select key 宽度 40/41 位不适用于 score conversion。
- 首个未闭合步骤：carry DCF 本地输出 share 不公开，低两位进入 lift 并形成 phase-5 sign operand。双方 phase-5 frames、sign key 和 masks 需要条件联合模拟；S20 的 opened-bit 后处理论证不覆盖该 continuation。未发现攻击，不构成安全 PASS。
- S21 审查者 /root/s12_receipt 未参与 S19/S20 安全推导或 S20 报告撰写，但曾只读审阅 S20 final bridge/report；本轮是延续复核，不是新的独立第二意见。其之前的 S12 工作仅为功能接收。本轮只读、不构建/测试。审查范围为 carry→sign 条件视图和报告公式，不签核完整协议证明。
- 两个池的 key-only 计数按 S15 slot 构造重算：N=128 41,024 slots / Σw=1,639,424；N=256 163,968 slots / Σw=6,719,104。此候选 bound 不覆盖 adaptive continuation、abort/crash slot ledger 或 shuffle/output。
- S21 决策、报告、gates 和 source/evidence manifest 位于 docs/decisions 与 docs/reproduction 的同名前缀文件。
- 论文依据仍限 BCG+21 Microsoft Research 作者稿 Theorem 2 / Appendix F.1 的固定参数单 key 桥接，不替代本地 Protocol I 联合视图证明。
- M6A V3 仍 OPEN，V4 未获准，正式 LAN/WAN 未运行。


## 14. M6B S22 R34 carry→sign continuation 复核对象与来源

- 被审运行时严格是 S18 manifest锁定的S15默认关闭修复快照：72项；S15 manifest SHA-256 `4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`；S18 receipt manifest SHA-256 `5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78`；canonical source SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。S22重新计算72/72无差异。S22 source/evidence manifest记录关键文件哈希与验证口径。
- 源码扩展：`protocol_i_score_input.cpp` 的实际运算环是R34；carry输出local share未公开，低两位形成lift并进入phase-5 sign frames。`protocol_i_project_derived_select5.cpp` 的full mask/份额在ideal joint tape下是uniform additive pair；DCF roots由`RAND_bytes`产生。RAND/getrandom联合假设仍未量化。
- DCF论文来源：Boyle et al., *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*，Microsoft Research作者稿，[官方PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。DCF Theorem 2见PDF p.13–14，Appendix F.1见pp.42–46。该定理支持固定函数参数下的单方key分布，不自行证明本路线完整协议 transcript。
- S22独立 reviewer `/root/independent_dcf_review` 明确未参与S19–S21，并仅对R34 carry→sign local pre-output transcript sampler给出CONDITIONAL范围复核；证据记于 `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S22_INDEPENDENT_REVIEW_2026-10-16.md`。该签核不覆盖ideal output-share条件化模拟。首个未闭合状态为 `q_b → key_records → forward shuffle`，详见S22 report/gates；不得将条件局部引理外推为自适应整链证明。
- S22不重算N=128 `M6B5/v3+M2CM/v1+M6P2/v1` 与N=256 `M6S2/v2` 完整池；既有key-only projection维持CONDITIONAL，adaptive full-pool和shuffle/output/crash维持OPEN。S22未构建/测试/跑正式性能，不改V3/V4状态。


## 15. M6B S23 F_scoreconv output-conditioned 审查对象与证据

- 运行时对象仍是 S18 锁定的 S15 default-OFF repair snapshot；S15 manifest SHA-256 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816，S18 receipt manifest SHA-256 5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78，72-file canonical source SHA-256 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。S23 重新核对 72/72；核心文件 hash 见 S23 source/evidence manifest。
- S23 审查 VFSS/src/moe_topk/protocol_i_score_input.cpp 的 R34 输入转换和 protocol_i_project_derived_select5.cpp:1890–1906 的 key record 入口。返回的 Q_b 是 priority-key 加法份额，不是均匀 share 或最终 mask share；完整 ideal share law 尚未写入 API。给定 Q_b 后 q_b 与 key_records 确定；在 carry/sign DCF correctness 下 Q_0+Q_1 等于稳定 priority key。
- 对 uniform-share 假设的 witness：真实输出满足低 h index bits 和由本方 x_b 决定的低32个 q bits；均匀 R_(2^(33+h)) share 满足该事件的概率为 2^-(h+32)，真实概率为1。此为拒绝错误 ideal law，不是对当前路线的攻击。
- S23 为 M2CM/v1 forward shuffle 子协议给出 P0/P1 固定 Q_b 的局部 sampler；结论只在理想独立均匀 dealer tape 下成立。实际 getrandom/FSS RNG 联合优势未量化，且该子引理不含 scoreconv carry/sign pre-output state。
- 完整 view 首个断点是从 S22 的 V_pre sampler 转成给定 Q_b 的 V_pre | (x_b,Q_b,L) 条件 sampler。没有条件混合引理/输出 share law；未发现攻击。执行者参与 S19–S22，S23 crypto independence 为 NOT_SATISFIED；S22 独立 review 不扩展为 S23 签核。
- S23 未重算 N=128 M6B5/v3 + M2CM/v1 + M6P2/v1 或 N=256 M6S2/v2 全池，不审 adaptive/inverse/output/crash，不构建或测试。S18 的 no-complete-mask 案例及可能遗留 P1 8-byte share 仍是历史、有限范围证据。
- S23 决策、报告、机器门禁和 source/evidence manifest：分别见 M6B_I_PROJECT_DERIVED_SELECT_5ITER_S23_DECISION_2026-10-16.md、M6B_I_PROJECT_DERIVED_SELECT_5ITER_S23_REPORT_2026-10-16.md、M6B_I_PROJECT_DERIVED_SELECT_5ITER_S23_GATES_2026-10-16.json 和同前缀 SOURCE_EVIDENCE_MANIFEST。
- 正式性能仍遵守 M6A V3→V4 顺序：V3 OPEN、V4 未获准、正式 LAN/WAN NOT_RUN。


## 16. M6B S24 F_scoreconv/v2 刷新设计、实现与证据

- S24 绑定 S18 已接收的 S15 基线：S15 源 manifest SHA-256 `4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`；S18 receipt manifest SHA-256 `5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78`；72 文件 canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。S24 外部 snapshot 复核为 72/72。
- `F_scoreconv/v1` 的输出是 R_(2^(33+h)) 的加法 priority-key shares；在 DCF correctness 下重构为 `((0x7fffffff-score)<<h)+original_index`。v1 具有低 h 位约束，未定义为均匀 share。
- S24 添加独立枚举 `FullKeyZeroSumRefreshV2`。刷新在完整键环中保持 share sum。局部右移语义不保持，故 v2 不提供可重构 q；consumer 使用完整 Q′。刷新材料使用 M6B5/v4 monolithic 或 M6S2/v3 streamed 版本，不兼容旧包，并绑定输出 ABI。
- `getrandom`/`RAND_bytes` 联合随机带优势、BCG+21 DCF 转换项与 TLS 信道优势仍须量化/明确适用条件。该刷新只条件支持 F_scoreconv 本地输出视图；不是整池或完整 Protocol I 安全证明。S24 执行者参与 S19–S23，独立密码学 review 为 `NOT_SATISFIED`。
- S24 决策、报告、门禁与 source/build/evidence manifest：`docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S24_DECISION_2026-10-17.md` 与同前缀 `docs/reproduction/` 文件。M6A V3 仍 OPEN、V4 未获准、正式矩阵 NOT_RUN。


## 17. M6B S43 F_T/v1 到真实 Dealer 联合材料分布审查来源

- 固定 runtime 来源链：S35 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`（85 项 canonical `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`）；S39 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`（S39 source manifest 重算 canonical `09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9`）；S40 `b04618117b11cffe0e899947726ab1c66807ffdd` 保留目标规模功能 evidence。S42 source manifest 固定 85 项工作树源哈希及 S42 文档快照，S43 只审 fixed Git 对象中的离线 issuer 与 lease 实现。
- 实际实现映射：`VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` 的 `random_bytes`、`next_uniform`、`score_to_mask_v3_dealer_issue_packages`、header/context/signature 与 `durable_create`；`VFSS/src/moe_topk/score_to_mask_v3_party.cpp` 的 package preflight/READY 前 lease；对应声明见 `VFSS/include/moe_topk/score_to_mask_v3_gmw.h`。S43 报告附有精确范围与联合概率推导。
- 外部标准/实现文档：OpenSSL 3.0 [RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/) 与 [EVP_RAND](https://docs.openssl.org/3.0/man7/EVP_RAND/) 说明 RAND API 的 CSPRNG/DRBG、seed-source 与自动 reseed 语义；RFC 8032 §5.1.6 说明 Ed25519 的消息依赖 nonce/signing 算法。它们用于限定实现假设，不替代当前部署下的 `Adv_DRBG_T(q)` 或完整协议证明。
- S41 manifest 的历史汇总字段 `bfa8ba9c…67ab726c` 与逐项值重算 `e0bb4d25…b5f8a69` 不一致；新增 [S41 来源勘误](reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S41_PROVENANCE_ERRATUM_2026-10-22.md)，不改 S41 原件。S43 作者参与 S35–S42；非作者 `/root/s39_crypto_review` 已复核理想 tape 下单次成功发材联合 law（`PASS_WITH_SCOPE`），真实 DRBG bridge 仍开放；范围见 manifest。
- S43 的 ideal-tape 复核范围为：材料联合 law 精确匹配；package/context/signature 在 `receipt_format_version` 参数匹配时逐值耦合。S39 v2 与 S35 v1 的签名状态字段值差异已在报告中单列。非作者复核未运行 build/test，且未重查其意见并入后的措辞编辑。真实 RNG bridge 与全协议 party-view 维持 `CONDITIONAL/OPEN`。
- S43 未构建/测试/执行 toy enumeration；不重跑 S35 195 案或 S40 211 案，不启动 LAN/WAN/RTT。理想 IID tape 下的上述范围性联合采样为 `PASS_WITH_SCOPE`，不改 VFSS-baseline、不推送/合并、不创建 `secure` alias。

## 18. M6B S44 Dealer RNG joint-stream bridge 与 rejection bound

- 对象固定为 S43 commit a1b02a8133955786e0bb6b047f55b746af66acbf；实际 S39 Dealer/runtime commit 8028c0fece69fe0c648b1e0f1e12ad7fca723f20。S43 source/evidence manifest SHA-256 9f8064b90a9c52a45d3c7d3488deca075497544de342c6f45a30fb1f0ceee320；七个核心 S39 runtime source Git blob SHA-256 逐项核对通过（7/7）。
- S39 Dealer RNG 源码定位：score_to_mask_v3_gmw.cpp:234–241 将 RAND_bytes 请求按最多 1 MiB 切块并在错误时抛出；:254–262 的 next_uniform 对 32-bit 字使用无界 rejection；:519–544 依次消费 gate_count 字节、ceil(m²/8) 字节和各 Fisher–Yates attempt 的 4 字节。N=256 时前两段共 5,289,856 字节，之后每次位置 attempt 另请求 4 字节；attempt 数无确定上限。
- ideal IID tape 单次成功发材联合 law 维持 PASS_WITH_SCOPE，经未参与 S43 推导的 /root/s43_final_text_review 复核。S44 将 E 限定为部署配置/生命周期策略；S 只含初始/reseed 熵输入、不含演化状态，所需条件包含 `Law(S | P,sk_T,E)=Law(S | E)` 或显式熵源替代定理。成功条件化必须处理 RAND 错误选择效应。真实部署依赖 provider/seed-source/entropy、生命周期和联合计算优势 Adv_DRBG_T(Q,C;E)，仍 OPEN。
- strict-PPT 当前未满足。候选 F_T/v2 bounded-rejection profile 令每个位置至多 r 次；只有投影到去除版本/签名/编码元数据后的语义材料空间，才有 `TV(π_mat(F_T/v2),π_mat(F_T/v1))≤δ_reject=1−∏(1−ρ_i^r)≤255·2^(−24r)`；完整跨版本签名 transcript 不承诺该距离。bounded DRBG 替换到匹配版本是计算优势。方案需要统一 pre-package ISSUANCE_ABORT、显式 version/signature binding 和独立接口审查。没有修改 runtime 或 F_T/v1。
- OpenSSL 官方资料用于说明 RAND API/provider/seed-source 行为边界，不代替具体部署证明：[RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/)、[RAND_get0_primary](https://docs.openssl.org/3.0/man3/RAND_get0_primary/)、[EVP_RAND](https://docs.openssl.org/3.0/man7/EVP_RAND/)、[EVP_RAND-SEED-SRC](https://docs.openssl.org/3.0/man7/EVP_RAND-SEED-SRC/)。
- S44 gates/报告/source-build-evidence manifest 记录 reviewer 身份、继承与未运行项。没有完整协议安全或性能结论；N>256 未准入，默认 v3 OFF，无 secure alias。

## 19. M6B S45 F_T/v2 有界拒绝采样版本化候选

- S45 固定 S44 提交 `aaa7f96095c350e8236246163d48c54a7fb8d9ba`（S43 父提交 `a1b02a8133955786e0bb6b047f55b746af66acbf`）及其 S39 Dealer/runtime 来源 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`。S35 85 项 canonical source identity 与 S39 85 项 canonical identity 作为历史来源记录；S44 嵌入的 S42治理文档 `OPEN_METADATA_DIVERGENCE` 及 S41 erratum 保留，不改写历史。
- 新实现位于 `VFSS/src/moe_topk/score_to_mask_v3_bounded_sampler.h`、`score_to_mask_v3_gmw.cpp` 与对应 package API/header；显式开关和 target 注册在 `VFSS/CMakeLists.txt`。TEST_ONLY 随机源只在 `BUILD_TESTING` 编译，conformance、进程 smoke 及 oracle 检查位于 S45 manifest/cases 指定文件。
- F_T/v2 package format 5 的签名 profile 字段独立于 receipt-format version；F_T/v1 format 4 和原 `next_uniform` 路径保持不变。有限尝试条件下每 Fisher–Yates residue 仍严格均匀，N≤256整次发材拒绝概率 `delta_reject≤255·2^-144`。该界仅是去版本/签名/编码元数据后的 `pi_mat` 统计界，不能套到完整签名 package/context 或跨版本 transcript。
- Release default-OFF、显式 v3/receipt-v2/FTv2 Release 与 BUILD_TESTING=OFF production 构建、配置依赖负例、FTv2 bounded/runtime conformance、receipt/v1 smoke、N128/256 FTv2独立进程 oracle E2E 及中止无交付 artifact 结果均见 S45 source/build/evidence manifest 与 case JSON。它们不是安全或性能证明。reviewer `/root/s45_non_author_review` 为只读、未运行构建/测试；执行者参与 S43/S44，非独立作者。
- `JOINT_RNG_STREAM_BRIDGE=OPEN`；provider/熵源/生命周期未从部署证据固定，`Adv_DRBG_T(Qmax,Cmax;E)` 未量化；`PARTY_VIEW_COMPOSITION=OPEN`。本轮不重复 S35/S40 全矩阵，不运行正式 LAN/WAN/RTT/性能，不准入 N>256，不创建 `secure` alias，v3 与 FTv2 均默认关闭。
- S45 决策、详细报告、gates、cases 和 source/build/evidence manifest 位于 `docs/decisions/` 与 `docs/reproduction/` 同名前缀文件；本地提交信息在交付记录中，不推送/合并。

## 20. M6B S46 F_T/v2 独立接收与 DRBG 联合流门

- 精确基线：S45 commit `6fd6bed0718be28d677892ca40125d0f52323f0e`；S46 独立 worktree 的起始 tree、父提交和状态见 S46 source/build/evidence manifest。S45 七项源码增量与被测 Git tree 已逐项核对；S46 仅增加 TEST_ONLY E2E target K 入口。
- 非作者 `/root/s45_non_author_review` fresh Release 验证默认关闭隔离、显式 FTv2 opt-in、sampler/runtime conformance、10 个 N=128/256 K 边界进程 E2E、发材中止和 v1 兼容 smoke；结论 `PASS_WITH_SCOPE`。S40 211-case 是旧配置历史证据，未迁移为 FTv2 结果；S35 195-case 与 S40 211-case 本轮未重跑。
- 单次 Dealer 随机请求：N=128 Qmax 1,294,376 bytes/Cmax 765；N=256 Qmax 5,295,976 bytes/Cmax 1,537；每请求不超过 1 MiB。有限请求界及 `delta_reject≤255·2^-144` 的适用投影详见报告。目标 OpenSSL provider、熵源与 lifecycle 未有部署证据，`DRBG_DEPLOYMENT_PROFILE` 和 `JOINT_RNG_STREAM_BRIDGE` 均 `OPEN`。
- S46 决策、报告、gates、逐案 JSON、source/build/evidence manifest 文件位于 `docs/decisions/` 与 `docs/reproduction/` 的 `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S46_*_2026-10-22` 同名前缀项；清单记录所有新增文件与相关源码哈希、构建哈希、错误 target 尝试及未运行项。仅本地提交，不推送/合并。

- 低概率 `ISSUANCE_ABORT` 的 runtime 语义及“无 package/context/party lease”已在定向测试中通过；项目文件未找到负责人单独接受合法输入下预处理偶发中止的明确记录，因此产品级接受状态保留 `OPEN_CONFIRMATION`。该项不改在线输出合同。
