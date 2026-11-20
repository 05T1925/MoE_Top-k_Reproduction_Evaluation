# BMW16 S12 common-tape 与进程测试（TEST_ONLY）

此目录保存 S12 的历史 common-tape runner 和原始测试 evidence。它不是 secure protocol package。C++ party API 与完整实现上下文保留在归档分支 `m6b-i-bmw16-s12`；当前 `main` 收录测试数据和报告，不能据此假设主分支源码可重建该候选。

`run_common_tape_matrix.py` 为有限测试生成 `BMW16_S12_TAPE_V1` 文件；S4 Python comparator 和 C++ TEST_ONLY trace 消费相同的 64 位 tape words。逐案 tape、trace、differential JSON 和矩阵清单位于 `evidence/`，哈希见 `docs/reproduction/evidence/BMW16_S12_TRACE_ARTIFACTS_SHA256SUMS.txt` 及综合归档清单。

固定 test tape 只用于可重复的差分比较，不是协议随机性、生产 key 或可信 party view 的证据。历史实现的适用范围为 `n≤64`；历史 party-view 安全门为 `NO-GO / NOT_PROVED`。查看根仓库的 [证据归档说明](../../docs/EXPERIMENTAL_EVIDENCE_ARCHIVE_2026-11-20.md) 和 [S12 报告](../../docs/reproduction/BMW16_S12_IMPLEMENTATION_AND_VALIDATION_2026-10-06.md)。
