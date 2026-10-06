# BMW16 S12/S13 原始证据归档索引

本索引将历史原始 traces/logs 与仓库内复跑工具、小型确定性夹具分离。归档目录在仓库之外，不进入 Git：

```text
C:\Users\28641\.codex\artifacts\BMW16_S12_verified_646f71c_20261006
```

## S12 原始归档

- 来源 revision：`646f71c09d7ff382a115cdcee6b330e5ff4bc225`
- 内容：S12 `experiments/TEST_ONLY_BMW16_S12/evidence/` 全部原字节 + S12 E2E/matrix/SHA 清单和报告引用的 trace hash 文件。
- 文件数：239；总字节：8,964,054。
- manifest：`BMW16_S12_ARCHIVE_SHA256SUMS.txt`
- manifest SHA-256：`D2C54620D450F157D659CF865285E8407C14D80F4796815B295B510E641E0337`
- 验证：WSL 进入归档目录运行 `sha256sum -c BMW16_S12_ARCHIVE_SHA256SUMS.txt`，所有记录通过。

## S13 独立复跑归档

- 来源：S13 worktree 的最终实验源码，S12 基线构建目录 `/tmp/bmw16-s13-rebuild`。
- 子目录：`s13/`。
- 内容：最终 3proc E2E 状态/计数日志、39 条 common-tape 差分及 traces/matrix；没有 key、PEM、明文 share 或重构 mask 文件。
- 文件数：197；总字节：7,342,413。
- manifest：`s13/BMW16_S13_EVIDENCE_SHA256SUMS.txt`
- manifest SHA-256：`41F898A1D865E89B66427E8940B20FC31F618D218C92E41459E4E885DC171C75`
- E2E log SHA-256：`089461BB3D50DBBF68992EE0ADAA3E8BD8273328623CCFC86D80F8D48988FFBC`
- common-tape `matrix.json` SHA-256：`a094d0e72bee05c3d45d1399c19e8a2255f399e789a15b10f7eeeda29795b0fd`
- 验证：WSL 进入 `s13/` 子目录运行 `sha256sum -c BMW16_S13_EVIDENCE_SHA256SUMS.txt`，所有记录通过。

## 仓库保留项

- 完整 S12 逐次 trace/log 文件保留在 S12 Git 历史中，也保留在上述外部归档；S13 当前树移出这些生成物以避免把所有 run artifacts 固定为源文件。
- 保留 S12 common-tape runner、比较器、README、matrix 摘要/校验文本和 `n=2,K=1` 夹具，供最小复跑/校验。
- 原 S12 `SHA256SUMS` 中文本文件哈希受 Windows 工作树换行影响，不能在任意 checkout 将工作树字节误当 Git blob identity。S12 commit/tree 固定源码；S13 两份外部 manifest 针对归档中的实际文件字节。
- 最新 S13 binary/build 只存 `/tmp/bmw16-s13-rebuild`，hash 列于 S13 报告；不把二进制提交到仓库。
