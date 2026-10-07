# BMW16 S16 外部证据索引

日期：2026-10-07。对应报告：[S16 独立接收、概率复核、安全门与计量报告](../BMW16_S16_INDEPENDENT_ACCEPTANCE_PROBABILITY_SECURITY_AND_METRICS_2026-10-07.md)。

## 外部归档

S16 原始文件与失败尝试保存在仓库外：

```text
C:\Users\28641\.codex\artifacts\BMW16_S16_20261007\
```

递归 `SHA256SUMS.txt` 覆盖 94 个文件（包括被拒绝的计量 probe、输出分离的 logs、独立审计 JSON 和构建期文件）；manifest SHA-256：`3306798e646b27d570bdae3dbb5b593848d903efb779bd5129a67a6f6d18e228`。先验证 manifest 自身，再执行 `sha256sum -c SHA256SUMS.txt`。测试 harness 输出不包含输入份额、DCF keys、解封凭据或完整 mask；不把 bundle/key material 纳入归档或 Git。

S15 外部归档目录为 `C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\`。其原始 `SHA256SUMS.txt` hash 为 `be7089865be7068f2101fb47ad5abbab9527935b2967cd22c997c32cdfcde9ea`，216 项均验证成功；S14 64 项归档清单 hash=`8899b04952e8d6d287a7a74804f9bc1eaece02e52174c1254eb38cf3736360ef`。

## 基线和构建身份

| 对象 | 身份 |
|---|---|
| S16 隔离分支 | `codex/m6b-i-bmw16-s16` |
| S16 被测源码 revision | `a95d7cec8105da0c418a465d96f314830a873630` |
| S16 报告/决策提交 | 被测源码之后单独提交；最终 revision 在交付回复列出 |
| S15 声明源 revision | `2c6937116fbbc170e7a254f4449a3a6411b0cb70` |
| S15 报告 revision | `b8e9fc622403ece033466b15c479643bc6ecfb74` |
| 起始 main/origin/main | `c3926c68fd14f270faa8b55234311071947fa080` |
| S4 冻结 Select 源 | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| S16 party-test Git blob SHA-256 | `aa32b2e0794c995a1eb0cf55b6a4b4e06b6ee3da5c6f42488db78a06623f75cb` |
| 3-process test binary | `0800995d89fe26cd20e9c1d5c97c8dbbcfb7563f7bfcc9b0602dc711d507cfab` |
| Party-test binary, counters ON | `d6215e3385219ee56dd17dc36efe9e0ec91fed5dca7816b844e17cc9e4659d8b` |
| Party-test binary, counters OFF | `7145f03560900201ad86653f5877e5d8dc94c1a27d543e0ff26766e72abeb775` |

## 关键外部文件 SHA-256

| 外部文件（相对于 S16 artifact root） | SHA-256 | 证据说明 |
|---|---|---|
| `s15-archive-check.log` | `ee965dbc2906478933226143e55feab04c134d5218744644e6b9453dd854d5ec` | S15 216 项原始归档校验结果 |
| `s15-source-identity-audit.json` | `bd3e7a3e59251741a865cb1f8a656d28c7377006ba5f0917f9fdc66b39817515` | 12 个源路径中 7 个 C++ 哈希不匹配且无可达 Git revision 对应 |
| `s15-independent-e2e.stdout.log` | `2ed2ccafcbed6b1a767e884bbb8b24b92d55aa32ef600ff9762dde5ef8ac1202` | S15 历史 E2E stdout 的 hash/summary 独立复算输入 |
| `s15-independent-e2e.stderr.log` | `c67047f31e48d0de526a5d250106cd1a86dbbd3dce8bcbb0bd5f178612b278e0` | S15 历史 E2E stderr |
| `s15-independent-e2e-audit.json` | `925233654532c054abedb50a190ff527e07cdb4000f5efda68fdfbe0e09aa1cc` | S15 61 行复核审计，57 success + 4 分类失败 |
| `s15-pilot-audit.json` | `ab30c3ec7b72ed0824b79b33743b4360092754ec6e8c575825a2b5ccddb7cc27` | 24 个 pilot raw hash 全匹配，4 profiles 汇总可复算；source binding caveat 仍在 |
| `ctest-relevant.log` | `735b25d61c3a8f48620171ef7a6f0e1f7fffc88bb404fccf8f3aecff6e8ab45a` | S16 相关 CTest 6/6 |
| `s16-e2e.stdout.log` | `2bffe719384fd8d38202c30f7f875870209f203a25636536ecf8c8fc730eec33` | S16 新 build 三进程矩阵 stdout |
| `s16-e2e.stderr.log` | `f3f76b104f0bfee4353529c6c97534d68f57c0fc58bb527d0462f03bd8b8ee90` | 与 stdout 分开保存的 stderr |
| `s16-e2e-audit.json` | `bf9a5ab4ab4bf19963f91650d678e70de28dd70fe58400ea328f62885590c838` | 61 行状态、退出码、oracle、计数独立审计 PASS |
| `s16-online-prg-audit.json` | `379e5d342924c09245a76864040023ca3c39fc58084e971390fcf1b2ae0ac72b` | 61 行 DCF expansion/AES blocks 逐方公式复算；sampler words 独立 |
| `abort-bound-recheck.json` | `066a7e64fd96716922630af36a3d9deb8ab8f9199cab59c365bf3e634d69705c` | 1M 可达偶数 M 的理想 sampler bound 复算 |
| `capacity-preflight.json` | `af86090d5c1b1612298ffb9af8014b2dce65e2c420d5234ba16bbec170ff1048` | checked slots/package lower bounds，非容量实测 |
| `counter-overhead-accepted/counter-overhead.json` | `90ef2f8da2a628b983061247dd1c6c4443924e7fa7ead878c2be023c13df7ae9` | 相同固定图 ON/OFF 1+5 TEST_ONLY 对照；不是 benchmark |
| `counter-overhead-fixedentropy-final2/` | 各文件见外部 manifest | 最终接受 probe 的原始行记录；manifest 含逐文件哈希 |
| `n256-common-tape-final/` | matrix SHA-256=`4aff390b75553b1b43424c01423169b9f203442008d55c6bd8fc03e5b6619df2` | n=256，K=2/8，S4/C++ 逐层 tape differential |

同一外部目录还保留早期被拒绝的计时和固定随机性探针，不把它们纳入通过数据。失败/被拒绝原件不得删除或覆盖。

## PRG 与性能口径

`online_prg_calls_total` 在该证据中专指 P0/P1 的普通 DCF `evalDCF` 内 `ecbEncTwoBlocks` 扩展调用之和；每次调用含两个 AES blocks。sampler PRF words 单独列；KeyGen、AES-GCM、库级未挂钩调用不在内。全库 PRG/AES 完整指令数 `NOT_MEASURED`。S16 的固定 tape ON/OFF 中位数为 44,971 μs 与 35,059 μs，约 28.2723% 计数器额外开销；该 in-process socketpair 诊断不能填进 V4 性能表。

S15 4 配置 1+5 pilot 在 S16 独立复核其 raw hash 和内部 summary，但由于 source manifest 中 7 个 C++ 源码 hash 无法对应 S15 声明的 Git revision，报告里所有 S15 pilot 时延应视作**历史归档数据、source identity 未接收**；不能与其它路线比较。

## 重跑索引

精确命令、环境、状态分类、未测项和边界见[报告第 8 节](../BMW16_S16_INDEPENDENT_ACCEPTANCE_PROBABILITY_SECURITY_AND_METRICS_2026-10-07.md)。复核 S15 源身份的脚本位于 `experiments/TEST_ONLY_BMW16_S16/audit_s15_source_identity.py`；JSON 报告写在外部目录，不入 Git。
