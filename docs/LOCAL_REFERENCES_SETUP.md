# 本地论文与参考仓库配置

远端仓库有意不包含论文 PDF 和大型参考工程。每位参与协议阅读、迁移或对比实验的
队友，都需要在克隆仓库后自行把获准使用的资料放到仓库根目录下的固定位置。本文件
只描述目录和校验方式，不授予任何资料的再分发权。

## 1. 目标目录结构

假设仓库克隆到 `<repo>`，最终应形成：

```text
<repo>/
  VFSS/                    远端追踪的活动实现
  VFSS-baseline/           远端追踪的冻结基线
  Papers/                  本地论文与团队测试要求，不追踪
  Agarwal_TopK/            本地 Agarwal 实现参考，不追踪
  CipherGPT/               本地 CipherGPT 原生参考，不追踪
  ADSMPC/                  本地旧 Protocol III 原型，不追踪
```

目录名和层级必须保持一致，因为项目文档和后续差分脚本使用这些相对路径。不要把
它们放进 `VFSS/`，也不要用 `git add -f` 绕过忽略规则。

## 2. 如何取得资料

- 优先从项目负责人维护的私有归档、学校内部共享位置或资料原作者/官方出版页面
  获取；
- 只取得自己有权访问和使用的版本；
- 本仓库目前不提供公开下载链接，也没有确认这些资料可整体公开再分发；
- 如果取得的版本与本文件列出的文件名或 SHA-256 不同，不要直接替换为“差不多”
  的版本，应记录来源、版本和差异并通知团队。

完整本地参考体量约为：`Papers/` 15MB、`ADSMPC/` 45MB、`Agarwal_TopK/` 2.4GB、
`CipherGPT/` 119MB。空间不足时可以只准备当前分工必需的子集，见第 6 节。

## 3. Papers 目录

当前团队使用以下 9 个文件：

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

从仓库根目录校验：

```bash
shasum -a 256 -c docs/PAPERS.sha256
```

全部显示 `OK` 才表示与当前团队副本逐字节一致。Agarwal 当前只有 15 页 CCS 2024
会议版；没有 full version，不能把其他二手材料标记成全文。

## 4. 大型参考目录的最低结构检查

大型目录目前来自本地历史工作包，尚未形成可公开复现的 source-only revision。
因此暂时不为整个目录发布哈希；先检查当前工作所需入口是否存在：

```bash
test -f Agarwal_TopK/README_RUN.md
test -f Agarwal_TopK/protocol1/README.md
test -f Agarwal_TopK/protocol1/runtime/README.md
test -f Agarwal_TopK/protocol1_ca/docs/ADAPTIVE_PREPROCESSING_DECISION.md

test -f CipherGPT/src/globals.cpp
test -f CipherGPT/src/shuffle.cpp
test -f CipherGPT/test/Top_K_paper_test.cpp

test -f ADSMPC/src/protocol3.cpp
test -f ADSMPC/src/RankingPhase.h
test -f ADSMPC/src/routing_dpf.h
```

命令失败说明本地包不完整或目录层级不对。不要在项目代码中增加 fallback 路径；
先修正本地资料布局。

## 5. 确认不会上传

从仓库根目录执行：

```bash
git check-ignore -v Papers Agarwal_TopK CipherGPT ADSMPC
git status --short
git add -n .
```

第一条应显示四个目录分别命中根 `.gitignore`；后两条不得列出其中任何文件。每次
准备提交前都重复此检查。论文阅读笔记若需要进入远端，应在 `docs/` 中重新撰写不含
受限原文的总结，不能直接强制加入 `Papers/`。

## 6. 两人近期分工所需子集

### Protocol I / VFSS 负责人

必须准备：

- `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf`；
- `Papers/协议1shuffle.pdf`；
- `Papers/测试指标.md`；
- `Papers/Agarwal与CipherGPT实验对比.pdf`；
- `Agarwal_TopK/`，重点阅读 `protocol1/` 和 `protocol1_ca/`；
- 远端已有的 `VFSS/` 与 `VFSS-baseline/`。

### CipherGPT 原生负责人

必须准备：

- `Papers/CipherGPT.pdf`；
- `Papers/测试指标.md`；
- `Papers/Agarwal与CipherGPT实验对比.pdf`；
- `CipherGPT/` 原生工作包。

### 当前可选

- `FSS基础.pdf` 和 Boneh 等论文用于补基础；
- `CryptoMoE.pdf` 在 M6 前不是近期实现依赖；
- `协议2shuffle.pdf` 只用于未来 Ruffle/恶意安全路线；
- `ADSMPC/` 和 `Agarwal_TopK/protocol3_ca/` 在 Protocol III 暂缓期间只作备用。

## 7. 使用边界

- 参考工程只读；新实现只进入 `VFSS/`，CipherGPT 原生负责人在独立本地分支修改
  `CipherGPT/` 时，必须先按团队决定建立可审查的 source-only 路径，不能提交整个
  119MB 工作包；
- 不复用旧工程生成的 DCF/DPF 密钥、掩码、临时通信文件或二进制 ABI；
- 不把 B0、mock shuffle、明文 Dealer 或文件轮询当作目标实现；
- 本地资料缺失时主动报告，不根据文件名猜测算法细节；
- 后续若发布 source-only 快照，必须在 `REFERENCE_MANIFEST.md` 中记录来源、许可、
  revision/SHA-256、包含路径和排除项。

## 8. 新队友完成标准

新队友开始任务前应能确认：

1. 已阅读 `README.md`、`PROJECT.md`、`AGENTS.md` 和自己的里程碑；
2. 所需本地资料位于固定根目录，论文哈希通过或差异已记录；
3. 四个本地目录均被 Git 忽略；
4. 能复现 M1 四项测试；
5. 明确自己的代码所有权、输出契约和不得修改的目录。
