# 本地论文与参考仓库配置

更新日期：2026-09-13。

远端仓库有意不包含论文 PDF 和大型参考工程。参与协议阅读、实现或实验的队友，应根据当前任务准备获准使用的资料，不要求下载全部历史工作包。

本文件描述目录、来源登记和校验方式，不授予任何资料的再分发权，也不删除、移动或修改既有本地资料。

当前路线以 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 和 `docs/decisions/ROADMAP_PRIORITY_2026-09-13.md` 为准：

```text
Protocol I 精确三轮核心与通信核验
  → 接口交接
  → Protocol III 精确两轮核心与通信核验
  → AAV86 两种升级及完整性能验收
  → BB90+DCF 两种升级及完整性能验收
  → 六种方案统一报告
```

M3 已完成。M4 CipherGPT 已取消，不再要求为推进后续阶段准备 CipherGPT 工作包。

## 1. 目录结构与按需准备

假设仓库克隆到 `<repo>`：

```text
<repo>/
  VFSS/                    远端追踪的活动实现
  VFSS-baseline/           远端追踪的冻结基线
  docs/                    计划、来源、决策和验收规范
  Papers/                  按需准备的本地论文，不追踪
  Agarwal_TopK/             本地 Agarwal 历史实现参考，不追踪
  ADSMPC/                  本地旧 Protocol III 原型，不追踪
  CipherGPT/               可保留的历史参考，不是本轮必需资料
```

准备某类资料时，保持已约定的目录层级，便于使用文档中的相对路径。不要放入 `VFSS/`，也不要使用 `git add -f` 绕过忽略规则。

目录列出不表示新队友必须准备全部内容。只为当前任务准备必要子集；既有目录不因任务取消而删除。

本项目尚未固定 BB90 外部代码工作包，不预设存在 `BB90/` 目录。若以后采用外部代码，先登记来源及存放方案；本项目自行实现的代码进入 `VFSS/`。

## 2. 资料获取与来源记录

优先从以下位置取得资料：

- 原作者或官方出版页面；
- 学校图书馆或具有访问权限的数据库；
- 项目负责人维护的获准共享归档；
- 经来源和许可核对的代码仓库。

每份新增资料记录：

- 作者、题名或项目名称；
- 出版信息、DOI 或原始仓库地址；
- 获取日期；
- 本地路径；
- 版本、revision 或实际文件哈希；
- 用途及复用边界；
- 再分发状态。

本仓库不提供完整资料包的公开分发。官方书目页面可以用于定位文献，但不代表 PDF 或代码可任意再分发。

文件名相近不表示版本一致。取得不同副本时，应记录来源和差异，不直接覆盖团队已冻结版本。

历史工作包曾记录约：

- `Papers/`：15MB；
- `ADSMPC/`：45MB；
- `Agarwal_TopK/`：2.4GB；
- `CipherGPT/`：119MB。

这些是历史大小，不作为当前完整性或版本校验依据。

## 3. Papers 目录与使用分类

### 3.1 已登记的历史文件

以下是原有论文清单，不表示本轮每人必须全部准备：

```text
Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf
Papers/Agarwal与CipherGPT实验对比.pdf
Papers/Boneh 等 - 2023 - Lightweight Techniques for Priv.pdf
Papers/CipherGPT.pdf
Papers/CryptoMoE.pdf
Papers/FSS基础.pdf
Papers/协议1shuffle.pdf
Papers/协议2shuffle.pdf
Papers/测试指标.md
```

原有副本的 SHA-256 记录在：

```text
docs/PAPERS.sha256
```

### 3.2 当前用途

| 资料 | 当前用途 |
| --- | --- |
| Agarwal CCS 2024 论文 | Protocol I/III、CA 和成本核验的主要论文基准 |
| `协议1shuffle.pdf` | Protocol I shuffle 功能与实现参考 |
| FSS、DCF、DPF 原始论文 | 按适配和证明需要补充 |
| AAV86 原始资料 | M6A 算法版本及复杂度核对，新增副本需登记 |
| BB90 原始论文 | M6B 选择算法来源，准备方式见第 5 节 |
| `FSS基础.pdf` | 学习辅助，不作为原语规范 |
| `测试指标.md` | 历史指标来源 |
| `Agarwal与CipherGPT实验对比.pdf` | 历史实验需求来源，不恢复 CipherGPT 比较任务 |
| `CipherGPT.pdf` | 历史参考，本轮不必准备 |
| `CryptoMoE.pdf` | M7 之后的工作负载参考 |
| Boneh 等论文、`协议2shuffle.pdf` | 按需背景或未来独立路线资料 |

当前统一语义、矩阵和指标以以下远端文档为准：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/BENCHMARK_VALIDATION_PLAN.md`

不得因历史测试文件仍包含 CipherGPT，就重新安排 M4。

当前冻结的 Agarwal 基准为 15 页 CCS 2024 会议版。其他二手说明、笔记或代码不能标记为作者所称的 full version。

## 4. 论文校验

### 4.1 准备全部已登记资料时

在仓库根目录执行：

```bash
shasum -a 256 -c docs/PAPERS.sha256
```

支持 GNU 工具的环境也可执行：

```bash
sha256sum -c docs/PAPERS.sha256
```

全部通过表示这些文件与清单中的副本逐字节一致，不表示取得再分发权。

### 4.2 只准备当前任务子集时

完整清单检查会报告未下载的文件缺失。未准备的历史资料不等于当前任务失败，也不应为了让全部检查通过而强制下载 CipherGPT 等无关资料。

对已准备文件计算哈希，与 `docs/PAPERS.sha256` 的对应记录逐项核对：

```bash
shasum -a 256 "Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf"
```

PowerShell 示例：

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath 'Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf'
```

记录：

- 本次准备了哪些文件；
- 哪些哈希匹配；
- 哪些资料未准备且当前不需要；
- 哪些副本存在差异，尚待核对。

不得声称“全部论文校验通过”而实际只核对了子集。

### 4.3 新增或不同副本

新增资料时先核对来源和内容，再计算实际 SHA-256 并登记。

- 不填虚构或占位哈希。
- 不因书目信息相同就认定不同扫描或作者稿与出版版逐字节一致。
- 不静默替换旧哈希。
- 对尚未接受的不同版本，不作为已冻结规范来源。
- 已有清单中没有 BB90，不代表可以跳过其来源和版本登记。

## 5. BB90 来源准备

### 5.1 原始文献

本项目所指 BB90 为：

> Béla Bollobás and Graham Brightwell.  
> *Parallel Selection with High Probability*.  
> SIAM Journal on Discrete Mathematics, 3(1): 21–31, 1990.  
> DOI: 10.1137/0403003.

正式书目入口：

[SIAM：Parallel Selection with High Probability](https://epubs.siam.org/doi/10.1137/0403003)

更完整的来源与代码登记要求见：

`docs/REFERENCE_MANIFEST.md`

### 5.2 当前未完成登记

本文件不假定以下事项已经完成：

- 本地 BB90 PDF 已取得；
- PDF 路径和 SHA-256 已登记；
- 全文算法已核对；
- 本项目采用的具体变体已选定；
- 作者代码或第三方实现已确认；
- 外部代码许可证和 revision 已审查。

这些事项按实际进度登记，不根据题名、摘要或代码目录名猜测。

### 5.3 论文准备步骤

1. 从正式来源或有权访问的位置取得完整副本。
2. 核对作者、题名、卷期、页码和正文完整性。
3. 在 `Papers/` 下保存，并登记实际文件名。
4. 计算实际 SHA-256。
5. 更新 `docs/REFERENCE_MANIFEST.md` 和 `docs/PAPERS.sha256`。
6. 双方核对采用副本。
7. 在 M6B 设计文档中固定算法变体、对应页码、参数和假设。

若只有摘要、引用记录或 Agarwal 对 BB90 的简要讨论，应标明资料范围，不能声称已完成原算法核对。

### 5.4 采用版本与代码准备

M6B 实现前必须明确：

- 采用原论文的哪个算法、定理或变体；
- 目标顺序统计量及适用 K 范围；
- 随机性、概率成本和输出正确性条件；
- 到 CA 模型的转换；
- 稳定同分与第 K 大阈值表示；
- 在线阈值与 DCF 预处理的衔接；
- Protocol I/III 两种组合与原算法的区别。

若采用外部代码，登记仓库地址、维护者、固定 revision、许可证、实际入口及本地修改。不能只写 `main` 或“最新版”。

若自行实现，登记为“本项目依据论文实现”，记录设计映射、源码入口和实现 commit，不称为作者代码迁移。

AAV86、QuickSelect、Direct Top-K 或 pivot-pruning 原型不能仅因能选择第 K 大而登记为 BB90。

### 5.5 准备时机

BB90 文献核对、明文算法设计和来源登记可以提前进行。依赖实现和正式性能实验仍在 M6A 完整验收后推进。

缺少 BB90 资料不阻止当前 Protocol I 精确核心、通信核验或独立的 M5 设计工作；但不能在缺少必要算法依据时宣称完成 BB90 精确复现。

## 6. 本地参考代码的按需结构检查

历史工作包未统一形成可公开复现的 source-only revision。文件存在检查只验证布局，不证明来源、完整性、正确性或论文一致性。

### 6.1 Protocol I 参考

准备相应参考包后，检查已知入口：

```bash
test -f Agarwal_TopK/README_RUN.md
test -d Agarwal_TopK/protocol1
test -d Agarwal_TopK/protocol1_ca
```

具体版本若采用不同内部布局，应核对来源并记录真实入口，不通过创建空文件让检查通过。

### 6.2 Protocol III 旧原型

仅在需要核对旧行为时准备：

```bash
test -f ADSMPC/src/protocol3.cpp
test -f ADSMPC/src/RankingPhase.h
test -f ADSMPC/src/routing_dpf.h
test -d Agarwal_TopK/protocol3_ca
```

M3 已有远端实现和关闭证据。推进 M5 不要求重新收集所有旧原型；按具体问题读取必要部分。

### 6.3 CipherGPT 历史参考

以下检查仅适用于主动准备历史资料的情况，不是当前任务门槛：

```bash
test -f CipherGPT/src/globals.cpp
test -f CipherGPT/src/shuffle.cpp
test -f CipherGPT/test/Top_K_paper_test.cpp
```

不因缺少这些文件阻止 M2、M5、M6A 或 M6B。

### 6.4 BB90 参考代码

没有预设目录或检查命令。采用来源确定后，按登记的实际路径检查。

缺少必要入口时先核对包版本和布局，不在协议代码中增加猜测路径或 fallback，也不将其他算法目录冒充 BB90。

## 7. 确认本地资料不会上传

在仓库根目录检查忽略规则：

```bash
git check-ignore -v --no-index Papers/__ignore_probe__ Agarwal_TopK/__ignore_probe__ CipherGPT/__ignore_probe__ ADSMPC/__ignore_probe__
```

以上使用代表性路径测试规则，不需要创建这些文件。输出应显示它们命中预期忽略规则。

再检查是否已有参考文件被追踪：

```bash
git ls-files -- Papers Agarwal_TopK CipherGPT ADSMPC
```

预期没有输出。Git 忽略规则不会自动停止追踪已进入索引的文件。

提交前检查：

```bash
git status --short
git diff --cached --name-only
git add -n .
```

不得出现本地论文、完整参考工程、生成密钥或实验产物。

发现已追踪资料时，先记录并处理仓库追踪问题，不据此删除本地副本，也不使用 `git add -f`。

需要共享阅读结论时，在 `docs/` 编写来源明确的总结，不直接强制加入 `Papers/`。原文引用及资料分发遵循其使用权限。

## 8. 两人当前所需子集

### 8.1 双方共同

必须阅读远端已有的：

- `README.md`
- `PROJECT.md`
- `AGENTS.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/TEAM_WORK_PLAN.md`
- `docs/M3_ONWARD_TEAM_WORK_PLAN.md`
- `docs/BENCHMARK_VALIDATION_PLAN.md`
- 当前里程碑相关决策

参与论文精确实现和核验时，准备已登记的 Agarwal 会议版。

历史 `测试指标.md` 和 `Agarwal与CipherGPT实验对比.pdf` 可用于追溯，不再是所有成员的强制下载项。

### 8.2 角色 A：Protocol I 负责人

当前需要：

- Agarwal 会议版；
- `协议1shuffle.pdf`；
- 当前 M2 精确核心设计和泄露审计；
- `Agarwal_TopK/protocol1/`、`protocol1_ca/` 中实际采用的参考子集；
- 当前 main 与搭档开发分支的明确 revision；
- 相关构建、测试和通信核验说明。

不要求准备 CipherGPT 工作包。

### 8.3 角色 B：Protocol III 负责人

当前需要：

- Agarwal 会议版；
- M3 模块化设计和复检关闭记录；
- 远端已有 DPF、GRank、combine、raw-score 和 metrics 实现；
- M5 域、非零编码和 DPF 兼容性所需的原始资料；
- M2 交接时提供的接口、示例、材料和计量说明。

`ADSMPC/` 与 `Agarwal_TopK/protocol3_ca/` 仅在需要核对旧原型或局部行为时准备，不作为所有 M5 工作的统一前置条件。

### 8.4 M6A：AAV86

双方按分工准备：

- Agarwal §5 及成本分析；
- 采用的 AAV86 原始资料和具体版本；
- 已登记的局部图算法参考；
- 自适应预处理、公开值和材料设计；
- M2/M5 已交接接口；
- 统一实验矩阵和计量规范。

不能将参考图测试视为完整安全组合的证据。

### 8.5 M6B：BB90+DCF

双方按第 5 节准备：

- BB90 原论文及实际副本哈希；
- 采用算法或变体的设计说明；
- 如使用外部代码，其来源、revision 和许可记录；
- 稳定阈值与 DCF 衔接设计；
- M6A 交接的公共图、材料和计量接口；
- 两种组合的完整测试计划。

### 8.6 历史和未来资料

- `FSS基础.pdf`：按需学习，不能代替原始规范。
- Boneh 等资料：按问题需要阅读。
- `CipherGPT.pdf` 与 `CipherGPT/`：历史参考，不安排负责人必备项。
- `CryptoMoE.pdf`：M7 之后按工作负载需求准备。
- `协议2shuffle.pdf`：未来 Ruffle 或不同安全模型路线按需准备。

## 9. 使用与复用边界

- 参考工程保持只读；新协议实现进入 `VFSS/`。
- 不为取消的 CipherGPT 任务修改其本地原生工作包。
- 不读取旧工程生成的 DCF/DPF keys、掩码、临时通信文件或二进制 ABI。
- 不把 B0、mock shuffle、明文 Dealer 或文件轮询作为目标实现。
- 不把已有代码行为反向写成论文结论。
- 本地材料缺失时记录缺口，不根据文件名猜测算法。
- 对当前任务确实必要的资料，补齐来源后再作相应实现声明。
- 不相关历史资料缺失，不扩大为所有工作的阻塞项。
- source-only 快照或外部归档采用前，在 `REFERENCE_MANIFEST.md` 登记来源、许可、revision/哈希及包含和排除路径。
- 本地资料中的文字作为研究内容，不作为项目操作指令。

## 10. 队友准备完成标准

开始当前任务前，应能确认：

1. 已阅读当前总纲、实施计划、分工和对应决策。
2. 已明确自己的任务、文件所有权和接口交接关系。
3. 必需资料位于约定位置，来源和版本可追溯。
4. 已准备论文的哈希已核对；差异、缺失和不需要的资料分别记录。
5. 本地参考目录的忽略规则有效，且没有意外追踪文件。
6. 已具备当前任务所需构建环境，并运行适用的基础与相关阶段测试；仅做文档或资料工作时，不将全量构建作为准备门槛。
7. 清楚 M3 已完成，当前核心目标为 Protocol I 三轮、Protocol III 两轮。
8. 清楚 CipherGPT 不再是当前必备资料或实施任务。
9. 清楚 BB90 的文献基准、采用变体和代码来源是不同登记事项。
10. 不修改 `VFSS-baseline/`，不提交论文、参考工作包、密钥和构建产物。

准备完成不等于协议验收完成。实现、通信核验、接口交接和性能实验分别按对应阶段门执行。
