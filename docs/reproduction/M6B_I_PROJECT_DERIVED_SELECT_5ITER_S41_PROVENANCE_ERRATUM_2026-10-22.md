# S41 来源汇总哈希勘误（S43，2026-10-22）

本勘误补充核查 S41 source/evidence manifest 中的 `OPEN_METADATA_DIVERGENCE`。它不改写 S41 报告、决策或 manifest，也不把不一致归咎为源码差异。

## 重算方法与结果

采用与 S40 manifest 已记录且能重现其 canonical 值的规则：相对路径按 UTF-8 ordinal 升序排列，每行写为 `path<TAB>lowercase_sha256<LF>`，对拼接后的 UTF-8 字节取 SHA-256。

| 输入 | 项数 | 重算值 | manifest 中记录值 | 结果 |
|---|---:|---|---|---|
| S41 manifest 的 `sha256_s40_original_worktree` 逐项值 | 85 | `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6` | S40 canonical `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6` | 一致 |
| S41 manifest 的 `sha256_s41_checkout` 逐项值 | 85 | `e0bb4d2507a2cf2536c5d28d53c5c2c6aa19f927359482c1e236c5b19b5f8a69` | `sha256_s41_checkout = bfa8ba9c051ca69c43a22130fa9d1cefc8885cbed10284ed0506762667ab726c` | 不一致 |

检查 S41 manifest 自身记录的 S40 manifest 文件 SHA-256：记录值 `66b15c5f1354cb1a52812b721e4c456b9a67678bca06ca0209babd2fcdaf029b` 与 S41 固定提交 `486026ba425a2d17ca15173d056db6eb309747d1` 中该文件 Git blob 的 SHA-256 相同。S41 的 85 项 `sha256_s41_checkout` 行值与固定提交内容在 83 项上可由 CRLF checkout 序列化复现；`PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md` 两项不匹配该提交的 LF/CRLF 版本，故逐项值代表 manifest 所记录的 checkout 清单，不能冒称为固定提交的 85 个原始 Git blob 哈希。

## 差异原因与来源裁定

可复核的原因是 **S41 manifest 汇总字段与其自身 85 个逐项值不一致**：同一条已校准规则能重现 S40 原始 canonical 值，却不能得到 S41 汇总字段 `bfa8…`，而能得到 `e0bb…`。S41 manifest 没有记录生成 `bfa8…` 所用的另一种序列化/输入，因此不能从现有证据判断其具体生成原因；本勘误不猜测为笔误，也不静默替换历史字段。

S43 采用的被审来源身份是 Git 固定提交链及 S42 已提交清单：S35 runtime commit `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`、S39 runtime commit `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`、S40 evidence commit `b04618117b11cffe0e899947726ab1c66807ffdd`、S41 文档提交 `486026ba425a2d17ca15173d056db6eb309747d1`、S42 文档固定提交 `72a23fc067b5bfbf10edf110eeff45ec1f0ed6bd`。S42 manifest 的 85 项工作树源文件哈希逐一重算为零差异，其 canonical SHA-256 为 `2ea2896cce7dd284f3d317a7462dde528c188cc15cb32af16c744dad4e5e6837`；S42 manifest 原始文件 SHA-256 为 `d6884503e9aff9997b8a0f5e59ec37b2e6a7ade50b1915d748b238ff051442ab`。

因此，S41 aggregate divergence 是历史 provenance 元数据缺陷；它没有改变 S35/S39 固定 runtime Git 对象，也不构成运行时代码差异。S43 以 S42 已提交快照和 S35/S39 runtime commits 为依据，不以当前主工作区或 main 替代。
