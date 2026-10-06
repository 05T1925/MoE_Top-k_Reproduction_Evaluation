# BMW16 S13：独立接收 S12、修正执行/证据合同与容量预检报告

- 日期：2026-10-06
- 标识：`M6B-I-BMW16-S13`
- 接收基线：S12 `646f71c09d7ff382a115cdcee6b330e5ff4bc225`
- 起始 `main/origin/main`：`c3926c68fd14f270faa8b55234311071947fa080`
- worktree / branch：`C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s13\MoE_Top-k_Reproduction_Evaluation` / `codex/m6b-i-bmw16-s13`
- 标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 判定：**S12 功能接收；异常状态与证据存储合同已改进；联合安全门 NO-GO；n=128 未运行；未开始 V4 正式矩阵。**

## 1. 基线、隔离与来源复核

本分支从 S12 指定提交建立，未在桌面主工作区继续改动。主工作区已有的 M6A 差异保留。S12→S11→main 的历史链保持；S4 来源固定为 `04ed6f8a352ab2277434344094471dff82f83e4b`，S4 Python source SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。已登记的比较边、dummy、两 task 句柄、key pool、membership、inverse 和输出契约与现有 C++ party API、bundle manifest 和 S12 报告相符。S12 生成/trace 使用的 seed 与独立进程 OS CSPRNG 并非同一概念；39 条差分实际共享 tape 字节，而非只比较 PRNG seed。

S12/S13 方案来源关系：

| 条目 | 身份/证据 |
| --- | --- |
| BMW16 论文 | 原文算法/定理；不为本项目 C++ 组合提供 Theorem 8 保证 |
| S4 `select4r.py` | 固定的项目推导算法；原始源码和哈希保留 |
| S12 C++ party API、runner、bundle | 项目组合的已运行 EXPERIMENTAL 行为 |
| E20 `8986a40175f5f8eef56d765fef0cfa863e205985`、E21 `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4` | 工程接口来源检查，不迁移其安全接收结论 |
| PR #28 draft head `57e239ee820a4208ac606663d877803f0b692206` | 未改动 |

保护检查：`VFSS-baseline/`、`Papers/`、本地参考树、E20/E21 既有入口、PR #28 均不在本任务差异里；没有提交密钥、生成 bundle、party share、mask、构建物或本地参考工程。实验开关继续默认 OFF。

## 2. S12 独立接收复跑（历史与本轮分栏）

**S12 历史报告数据：** 52 SUCCESS、1 注入 algorithm abort；39 条 common-tape S4/C++ trace PASS；原报告的 n=64/K=8 是单次 loopback sample。S12 记录仍留在 S12 commit，不覆盖其数字或失败边界。

**S13 在干净构建目录对 S12 固定源码的独立复跑：**

- 构建：WSL Ubuntu 24.04、GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13、Release，开实验 target；fresh build 路径 `/tmp/bmw16-s13-rebuild`。
- 相关 CTest：`moe_topk_bmw16_experimental_party_test`、`moe_topk_m2_ucmp_conformance_test`、`moe_topk_m2_score_input_conformance_test`，3/3 PASS。
- 直接跑 S12 3proc executable：53 行配置，52 SUCCESS、1 注入概率 abort，oracle 输出逐位相同；0 自然 abort 样本。
- common tape runner：39 案例 PASS，其中 `n=2..8` 的所有 K 为 35 例，额外 `n=64` 为 K=1/8/32/63 四例。重跑 matrix SHA-256 `beece165644e58a192e10273457e8394c058091484e32696bd7f6ce05668ec12`。
- 独立 S12 E2E 日志 SHA-256 `12D3B62B4D79E006473E46A0ED8D83B8632A82EA91A9B0FB7794187394195D5C`，存放在外部 archive 的 `independent_s12_e2e.txt`；不把测试 harness 的重构权限算入任一 party view。

S12 中固定 source hash manifest 是工作树字节清单。跨 Windows checkout 因 CRLF/LF 造成 18 项差异；关闭 checkout 转换并 reset 后仍有 9 个文本文件字节哈希不同。S12 原 worktree 清单通过，S12 Git commit/tree identity 未变。本报告把它登记为**哈希合同不具可移植性**，不误报为 S12 源文件被修改。原目录字节已完整外存归档且对归档 SHA-256 manifest 重验通过。

## 3. 异常状态、退出码及启动器负例

S12 的异常处理根据异常字符串子串猜 abort 类别，并在 party 报告 abort 后返回 0。S13 移除该行为：party API 只捕获有类型的 transport/material 失败；其他异常传播。S12 raw-score adapter 原来把 transport 转换成通用 `runtime_error`，导致上层不能可靠区分通信失败；S13 将 transport 类型原样传播，并保留 adapter 的阶段计时。

| 运行结果 | process exit | `scope` | 启动器要求 |
| --- | ---: | --- | --- |
| SUCCESS | 0 | NONE | 两个结构化报告、两份 share；父级 TEST_ONLY 仅此时比较 oracle |
| 概率算法 abort | 10 | PEER_AGREED | 双方 status/reason 一致，双方无 share |
| 材料错误 | 20 | LOCAL_ONLY | 启动器记录为本地状态；不宣称 peer ACK，双方均无 share |
| communication | 30 | LOCAL_ONLY | 对端失联只记录存活方本地 abort；双方无 share |
| 意外异常/不变量错误 | 70 | PROCESS_ERROR / ALGORITHM_INVARIANT_ERROR | 作为进程错误，不能被归入正常 abort 或成功 |

S13 本轮三进程执行共 56 配置：

| 类型 | 数量 | 检查 |
| --- | ---: | --- |
| SUCCESS | 52 | 长度 n、bit 取值、weight K、原序 oracle 逐位相同 |
| TEST_ONLY 注入算法概率 abort | 1 | 双方状态一致，mask 文件均不存在 |
| 双 party bundle 密文篡改 | 1 | `ABORT_MATERIAL` / exit 20，无 mask |
| 故意使 party API 输入非法 | 1 | 异常传播为 `ERROR_UNEXPECTED` / exit 70，无 mask；没有“catch 后 return 0” |
| 单端 socket 关闭 | 1 | 两方观察到本地 transport 错误 / exit 30；scope 保留 `LOCAL_ONLY`，不冒称协议状态协商成功 |
| 自然算法概率 abort | 0 | 本轮无观察；不从零样本失败推出成功概率 |

启动器还检查 result file 存在、exit 与结构化状态一致、双方状态/scope 相同、成功才有两份 mask、非成功没有任何一份 mask。附属 AEAD 负例仍覆盖错 key、错 party、错 session、截断 tag、篡改密文/manifest、错宽度、重复 bundle/slot claim 和 claim 跨进程退出持久化；直接 transport 负例覆盖 peer closed/silent。peer closed E2E 只说明本方通信 abort 被正确分类，不构造双边 ACK。

## 4. 最终源码层 common-tape 差分

在全部源码修正后重新运行：

```powershell
python experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py `
  --wsl-distro Ubuntu-24.04 `
  --cpp-test /tmp/bmw16-s13-rebuild/moe_topk_bmw16_experimental_party_test `
  --s4 C:/Users/28641/.codex/worktrees/bmw16-s4/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py `
  --out C:/Users/28641/.codex/artifacts/BMW16_S12_verified_646f71c_20261006/s13/common_tape_all_ranks `
  --first-seed 50000
```

39/39 通过，最终 matrix SHA-256 `a094d0e72bee05c3d45d1399c19e8a2255f399e789a15b10f7eeeda29795b0fd`。比对两实现的共用随机 tape、逐 task 的抽样/pivot/U/V/W/R1–R4 边及 bit/abort/selected handle；不比较语言内 PRNG seed。该结果是有限 C++/S4 trace 差分，不是论文概率证明。

## 5. 完整三进程 E2E 和 n=64 计量样本

实际直接命令：

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc `
  'cmake --build /tmp/bmw16-s13-rebuild --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_score_input_conformance_test -j2 && ctest --test-dir /tmp/bmw16-s13-rebuild -R "moe_topk_(bmw16_experimental|m2_ucmp|m2_score_input)" --output-on-failure && /tmp/bmw16-s13-rebuild/moe_topk_bmw16_experimental_3proc_test > /mnt/c/Users/28641/.codex/artifacts/BMW16_S12_verified_646f71c_20261006/s13/s13_3proc_e2e.txt 2>&1'
```

n=64、K=8、输入 seed=1206 的本轮 OS-CSPRNG 单次 loopback 样本（不与 S12 的随机图计数硬拼）：

| 项目 | 本轮实测 |
| --- | ---: |
| 状态 / mask | SUCCESS；oracle 逐位匹配；weight=8 |
| 因果 phase | 12：raw 2 + forward shuffle 2 + coin 1 + Select 4 + inverse 2 + final state 1 |
| Select 逻辑比较 | 18,141；R1/R2/R3/R4 = 7,310 / 452 / 10,375 / 4 |
| Select unique slots / dummy / repeat | 5,742 / 12,399 / 0 |
| membership | 63 slots；自比较本地置 1 |
| uCMP / DCF Eval per party | 5,933 / 11,866 |
| bundle slots / consumed claims / unused claims | 22,306 / 5,935 / 16,371；unused shuffle CmpAgg 为 4,032 |
| 每方 AEAD bundle | 22,770,793 B；plaintext 22,770,677 B；manifest 446,128 B；ciphertext 22,770,693 B；头/tag 116 B |
| T 生成 / seal / write+chown delivery / total | 138,597 / 112,589 / 100,263 / 351,451 μs |
| party bundle read/decrypt/open | 58,087 / 56,733 μs |
| party online API 时间 | P0 8,953,595 μs；P1 8,953,409 μs |
| 持久 claim callback | P0 8,841,165 μs；P1 8,831,954 μs |
| slot manifest 查找 | P0 54,401 μs；P1 54,530 μs |
| FSS Eval计时 | raw adapter 0.271/0.259 ms；Select 合计约 24.8/24.5 ms；membership 0.280/0.243 ms |
| framed exchange 耗时 | P0 所测 raw+shuffle+coin+Select+inverse+status 合计约 2.06 ms；P1 约 11.17 ms（party 角色与等待不同） |
| sent/received | 每方 153,929 / 153,929 B；交叉计数相等 |
| sampler PRF words / FSS 库 PRG 调用 | 331 / `NOT_MEASURED` |
| 峰值 RSS（P0 / P1 / T） | 100,664 / 100,668 / 148,768 KiB |

逐槽 callback 每次调用下层持久 claim：`openat(O_EXCL)`、marker 写入、文件 `fdatasync` 和目录 `fsync`。计时包含这段整个 callback；不声称已做内核级 syscall profiler。n=64 的 callback 占 P0 API 时间约 98.7%，槽清单线性查找约 54 ms，uCMP Eval 约 25 ms，已计网络交换为毫秒级。因此 S12 单次约 10.34 秒路径的主要已定位开销是**逐 slot 持久同步合同**，不是 FSS Eval 或传输。本轮没有通过降低耐久性优化它；任何批量 claim 优化必须另证崩溃后的一次性消费仍成立。

计时字段有重叠：Select round inclusive time 包含 claim、Eval、交换；`durable_claim_io` 是其子项。不得将重叠值相加当端到端总时间。上述只是一个有界 loopback 探测，不是 LAN/WAN、不是五次中位数、不是正式 V4 指标。S13 原始运行文本只含状态和计数，不含 shares/key/明文 mask，留在外部归档；本报告的全部测量不能进入正式性能总表。

## 6. 证据保存、哈希和可复跑位置

原 S12 证据已从原 S12 worktree 字节逐文件复制并验证到：

```text
C:\Users\28641\.codex\artifacts\BMW16_S12_verified_646f71c_20261006
```

S12 原始 archive 清单 239 entries、8,964,054 B，manifest SHA-256 `D2C54620D450F157D659CF865285E8407C14D80F4796815B295B510E641E0337`；以 WSL `sha256sum -c BMW16_S12_ARCHIVE_SHA256SUMS.txt` 复核成功。S13 外部证据 197 files / 7,342,413 B，manifest `s13/BMW16_S13_EVIDENCE_SHA256SUMS.txt` SHA-256 `41F898A1D865E89B66427E8940B20FC31F618D218C92E41459E4E885DC171C75`；目录内无 `.key/.pem/.der` 文件。外部 E2E log SHA-256 `089461BB3D50DBBF68992EE0ADAA3E8BD8273328623CCFC86D80F8D48988FFBC`；matrix SHA-256 如上。

最后构建的三进程 binary SHA-256 `1d991e2827e44655bdc86a760c751f39d3d781c1d665dbe28b97c8fdbf72cc3c`；共同 tape party binary `746c09228511944cb5970df52619bc5bffa778ceeb4d68d400b45b47b0b7a5cb`。证据恢复位置、原始结果索引和验证命令见 [archive index](evidence/BMW16_S12_S13_ARCHIVE_INDEX_2026-10-06.md)。

### 复跑命令

```powershell
python experiments/TEST_ONLY_BMW16_S13/capacity_preflight.py
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc 'ctest --test-dir /tmp/bmw16-s13-rebuild -R "moe_topk_(bmw16_experimental|m2_ucmp|m2_score_input)" --output-on-failure'
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc '/tmp/bmw16-s13-rebuild/moe_topk_bmw16_experimental_3proc_test'
```

CTest 全量首次尝试会列出 42 个注册项目；当前 fresh build 只构建了本阶段 4 个 target，故全量调用中未构建的 39 项为 `Not Run`，不是断言失败。本任务相关正则集 3/3 PASS，三进程 runner 直接退出 0 且 56 cases 均满足其状态合同。

之后为传输类型与共享 shuffle 路径追加构建 `moe_topk_m2_transport_conformance_test`、`moe_topk_m2_parallel_shuffle_conformance_test`、`moe_topk_m2_parallel_shuffle_process_e2e_test`，最终相关 CTest 集为 6/6 PASS。没有把未构建的其他里程碑测试声称为通过。

## 7. 安全归约和门禁

P0/P1 完整视图已在决策中列出。纯随机置换等式证明局部 `(sigma_p,tau_p)` 的边缘不依赖 `pi`，但并不模拟 P0/P1 的 shuffle 辅助 state、在线消息、公开 masked list、完整 pool key share 和输出 share。代码中 forward shuffle、两 task 的 Select 与 membership 对每个真实 pair 共用 forward node-mask difference；整份 party bundle 对腐化方可见，包含未消费 key。底层 DCF key generation 依赖同一 OS-seeded `FSSConfig::prngs[tid]` AES PRNG 顺序流，源码没有 per-slot domain separator 或此联合辅助输入/adaptive Eval 的安全定理。即使分别假设单 key DCF 安全、AES-GCM、AES PRNG/OS entropy，也仍需明确组合 hybrid 对相关阈值、完整 pool、打开 operands 和自适应 transcript 的模拟算法与优势损失。

因此安全缺口是**一个可命名、可修复的证明义务**：从真实 shuffle + DCF 党内完整材料 view 到仅依赖明确 leakage/输出 shares 的模拟器，尤其是上述 H0→H1 pool 替换。不声称存在匿名图泄露反例，也不声称“不可能”。最小下一步应固定源版本、引入域分离 keygen 子流，构造多 key/辅助输入/自适应 Eval simulator 与优势界；若该性质不能由当前 DCF 构造导出，则换用明确满足该性质的原语，然后再补 shuffle 组合证明。生产身份凭据与 T→party 私有认证传输仍是另一条工程门。

| 门禁 | S13 结论 |
| --- | --- |
| S12 独立接收 | `ACCEPTED_WITH_EXPERIMENTAL_BOUNDARY` |
| 三进程异常退出/状态合同 | `PASS`（负例已验证） |
| 逐文件 archive/SHA-256 | `PASS`（不提交原始大 trace/log） |
| S4/C++ tape 等价 | `PASS`（39/39 有限样本） |
| Select/party function E2E | `PASS_FOR_TESTED_CASES`，52/52 成功分支正确；自然 abort 样本 0 |
| 概率失败界 | `NOT_PROVED`；单样本观察不作定理 |
| n=128 容量/运行 | `PREFLIGHT_ONLY / NOT_RUN`，硬 cap=64 |
| full one-party joint view | `NO-GO / NOT_PROVED` |
| production private authenticated T material delivery | `NOT_IMPLEMENTED` |
| V4 LAN/WAN five-run matrix / six方案完成 | `NOT_STARTED` |

## 8. 本轮修改文件

源码：

- `VFSS/include/moe_topk/experimental_bmw16_select_party.h`
- `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp`
- `VFSS/include/moe_topk/protocol_i_transport.h`
- `VFSS/src/moe_topk/protocol_i_transport.cpp`
- `VFSS/include/moe_topk/protocol_i_score_input.h`
- `VFSS/src/moe_topk/protocol_i_score_input.cpp`
- `VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp`

文档/脚本：

- `docs/decisions/BMW16_S13_RECEIVE_EXECUTION_EVIDENCE_SECURITY_CAPACITY_GATE_2026-10-06.md`
- `docs/reproduction/BMW16_S13_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md`
- `docs/reproduction/evidence/BMW16_S12_S13_ARCHIVE_INDEX_2026-10-06.md`
- `experiments/TEST_ONLY_BMW16_S13/capacity_preflight.py`
- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- S12 跟踪证据目录的清理删除详见 Git diff；S12 原始证据仍在原提交和外部归档中。

没有新增或改名 secure API。完整源 revision 和 staged diff 身份以此报告随附的本地 S13 提交为准；最终本地提交 hash 由交付回复提供。本提交不推送、不合并、不更新 PR #28。
