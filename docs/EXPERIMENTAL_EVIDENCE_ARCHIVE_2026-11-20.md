# BMW16 TEST_ONLY 原始证据归档

归档日期：2026-11-20

本说明记录按项目负责人要求加入 `main` 的论文资料和 BMW16 S2/S12 原始 trace/tape。归档只增加资料、校验清单和说明文档；没有改动 `VFSS/`，没有重新运行历史测试，也没有升级任何阶段、安全或性能状态。

## 归档范围

| 路径 | 内容 | 文件数 |
| --- | --- | ---: |
| `Papers/` | 论文、研究笔记和历史测试要求 | 15 |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/` | 测试脚本/README 与历史生成结果；其中 `results/` 原始结果 10 个文件 | 18 |
| `experiments/TEST_ONLY_BMW16_S12/` | 共用 tape runner、README 与逐案 evidence；其中 `evidence/` 235 个文件 | 238 |
| `docs/decisions/`、`docs/reproduction/` | S2/S12 的决策、验证报告、原始运行记录和旧阶段哈希清单 | 配套说明 |

S2 与 S12 两个实验目录共 256 个文件、23,028,940 bytes。综合 SHA-256 清单包含 245 个 `results/` 与 `evidence/` 数据文件，以及 4 个关联的原始运行/矩阵/trace-audit 记录；见 [`RAW_TRACE_TAPE_ARCHIVE_SHA256SUMS.txt`](reproduction/evidence/RAW_TRACE_TAPE_ARCHIVE_SHA256SUMS.txt)。

关联实现/历史记录分支使用已整理的远端名称 `m6b-i-bmw16-s2` 与 `m6b-i-bmw16-s12`；原始数据现集中归档在 `main`。`.gitattributes` 为论文和原始 evidence 关闭换行转换，使哈希在不同操作系统上对应相同字节。S12 的 235 行局部清单 `BMW16_S12_TRACE_ARTIFACTS_SHA256SUMS.txt` 已按本次归档的实际字节重算；综合清单另含 4 个配套运行和审计记录。

`Papers/` 共 15 个文件、22,171,904 bytes，哈希清单为 [`PAPERS.sha256`](PAPERS.sha256)。其中 `siamjdiscrmath.pdf` 是书目汇编，不是单篇论文。`测试指标.md` 使用当前本地副本重新计算 SHA-256（`4a5945f4…a3701587`），与旧清单中的值不同；本次清单以实际归档字节为准。资料具体来源、版本和再分发状态见 [`REFERENCE_MANIFEST.md`](REFERENCE_MANIFEST.md)；入库本身不等于版权或许可核查。

## 实验身份和结论边界

### S2：四轮明文 Select 候选

S2 的实现标签是 `BMW16_DERIVED_SELECT_4R_TEST_ONLY`。历史记录把它定位为项目派生的明文 TEST_ONLY 候选；不是作者原样 Algorithm 5/7 复现，不是 BB90，也不是 secure Protocol I/Top-K 实现。具体设计和观察结果见 [S2 决策](decisions/BMW16_S2_DERIVED_SELECT_4R_DECISION_2026-10-05.md) 与 [S2 验证报告](reproduction/BMW16_S2_DERIVED_SELECT_4R_VALIDATION_2026-10-05.md)。报告中的数字是历史运行记录，本次没有复跑。

### S12：三进程候选与 common-tape differential

S12 的实现标签是 `BMW16_S12_CPP_TP0P1_EXPERIMENTAL_TEST_ONLY`。归档保留逐案 common tape、C++ trace、differential JSON、运行记录和对应 manifest。历史报告限定在测试过的有限规模（`n≤64`）；报告称 39 个 common-tape 案例逐层对齐，但不因此得出对所有 tape 的证明。

S12 的 party-view 联合安全门记录为 `NO-GO / NOT_PROVED`，生产级 T 凭据和经过认证的私有传输未交付。固定 tape、输入映射、root 测试 harness 及其 oracle 校验属于 TEST_ONLY；不得将其用于生产协议、秘密材料或随机性来源。详见 [S12 决策](decisions/BMW16_S12_THREE_PROCESS_MATERIAL_AND_PARTY_VIEW_GATE_2026-10-06.md) 和 [S12 实施/验证报告](reproduction/BMW16_S12_IMPLEMENTATION_AND_VALIDATION_2026-10-06.md)。报告中的 E2E、成本或计数是历史样本，不是正式 LAN/WAN 性能矩阵。

## 文件校验

在仓库根目录运行以下命令检查归档字节：

```powershell
Get-Content docs/reproduction/evidence/RAW_TRACE_TAPE_ARCHIVE_SHA256SUMS.txt | ForEach-Object {
  $parts = $_ -split '  ', 2
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $parts[1]).Hash.ToLowerInvariant()
  if ($actual -ne $parts[0]) { throw "SHA-256 mismatch: $($parts[1])" }
}
```

两份本地压缩包 `IDPF_IDCF_TopK_Experiment_Packag.rar` 与 `Protocol_I_AAV86_Core_Demo_2026-10-06.zip` 没有加入仓库；它们按用户要求在本机另行保留。桌面 `Moe` 目录不属于本次操作范围。
