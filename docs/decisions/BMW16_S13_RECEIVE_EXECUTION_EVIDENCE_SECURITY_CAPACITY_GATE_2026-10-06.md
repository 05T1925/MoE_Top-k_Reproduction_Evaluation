# BMW16 S13：S12 接收、执行/证据合同与安全/容量门

- 日期：2026-10-06
- 接收对象：S12 `646f71c09d7ff382a115cdcee6b330e5ff4bc225`
- S13 分支：`codex/m6b-i-bmw16-s13`，从 S12 提交隔离建立
- 身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
- 决定：**S12 功能与来源接收；执行合同和证据合同已修正；组合单方安全与生产发材门仍 NO-GO。**

## 1. 接收边界和来源

S13 在独立 worktree 中从精确 S12 提交开始，保留 S12 原提交及原始证据。`main` / `origin/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`；S12 是其后代。本任务没有修改 E20/E21、PR #28、`VFSS-baseline/`、论文或本地参考工程。S4 Select 源身份继续固定为提交 `04ed6f8a352ab2277434344094471dff82f83e4b` 的 `select4r.py`，SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。S4 的论文来源标签和 S4 项目推导不因 C++ 三进程运行而升级为 BMW16 Theorem 8 保证。

证据层次保持分开：

1. **论文定义：** BMW16 2016 年算法与定理只支持其原始模型/参数条件。
2. **S4/S12 本地实现行为：** S4-derived Select 的固定程序、trace 和有限测试；不反推论文结论。
3. **项目扩展：** 两份 Select、Protocol I 输入适配、shuffle、uCMP/DCF membership、逆路由和 XOR mask 组合。
4. **待验证设想：** 下文的完整 shuffle + 相关多 key FSS 自适应联合模拟。

S12 端到端函数沿用现有 `protocol_i_bmw16_experimental_raw_score_mask_party`，覆盖正式 raw-score adapter、真实 Protocol I shuffle、同一匿名真实句柄上的两份 Select、membership 和同一置换 inverse；只有 TEST_ONLY 父级 oracle 重构 XOR mask。S13 没有重写 Select，也没有把四层比较写成完整协议四轮。

## 2. 确定缺陷及修正合同

S12 party runner 曾将所有 `std::exception` 按异常字符串包含的 `frame/socket/transport` 子串分类为材料或通信 abort，写状态后统一 `return 0`。该行为会让启动器把程序不变量错误当成正常进程退出。party API 也曾把任意异常按字符串映射为 `ABORT_MATERIAL`。S13 作了窄类型修复：

| 情形 | 结构化 party 状态 | scope | 退出码 | mask |
| --- | --- | --- | ---: | --- |
| 成功 | `SUCCESS` | `NONE` | 0 | 两份均须存在 |
| 论文式/项目算法概率分支已交换相同状态 | `ABORT_ALGORITHM_PROBABILITY` | `PEER_AGREED` | 10 | 不存在 |
| 材料包/持久槽领取错误 | `ABORT_MATERIAL` | `LOCAL_ONLY` | 20 | 不存在 |
| 明确的 framed transport 错误 | `ABORT_COMMUNICATION` | `LOCAL_ONLY` | 30 | 不存在 |
| 意外异常或算法不变量错误 | runner `ERROR_UNEXPECTED` 或 `ABORT_ALGORITHM_INVALID` | `PROCESS_ERROR` 或 `PEER_AGREED` | 70 | 不存在 |

API 只捕获 `ProtocolITransportError` 与有类型的 `ProtocolIBmw16ExpectedFailure`；其他 `std::exception` 继续传播。raw-score adapter 不再吞掉传输异常并重抛成普通运行时错误。材料 manifest 缺槽、Select pool 覆盖/绑定错误和 membership pool 错误被显式映射为材料失败；这不把任意编程错误都当作预期 abort。

三进程启动器逐项检查两个退出码、两个原子结构化状态文件、双方状态/scope 一致、SUCCESS 时两份 mask 都存在并重构后逐位匹配冻结 oracle、非 SUCCESS 时双方 mask 都不存在。仅允许明确配置且报告 `LOCAL_ONLY` 的存活方本地中止；它不声称 peer 已 ACK。S13 增加双包 AEAD 篡改、API 输入导致的意外异常及单端 socket 关闭负例。算法注入中止与这些负例分开记录。

## 3. S12 原始证据的可移植合同

S12 工作树的原始 `SHA256SUMS` 在生成它的 S12 worktree 中通过。把同一提交在另一 checkout 上按该清单重验时，文本文件的工作树换行转换使 18 项不匹配；执行一次性 `core.autocrlf=false` 规范重置后仍有 9 项不匹配。S12 Git commit/tree 未变，这证明旧清单混合了工作树字节哈希与固定源码身份，不能跨 checkout 当作源码损坏证据。

S13 已将原 S12 证据目录完整复制到外部归档，保留其原始字节，并在归档的 `BMW16_S12_ARCHIVE_SHA256SUMS.txt` 上逐文件验证成功。S13 新运行和 tape matrix 使用独立的外部文件 SHA-256 清单。S12 的历史报告/commit 未被改写；当前分支仅删除由 S12 提交带入的大型逐次生成 trace/log 副本，保留复跑脚本、matrix 摘要、校验清单和 `n=2,K=1` 小型确定性夹具。恢复目录、文件数、字节数、manifest 哈希及验证命令见 [证据归档索引](../reproduction/evidence/BMW16_S12_S13_ARCHIVE_INDEX_2026-10-06.md)。

## 4. 理想功能、拟允许泄露与完整视图

目标功能是：双方输入 signed Q20.12 分数的二加法份额和公开 `(n,K)`；成功时取得原输入顺序、恰 `K` 位为 1 且稳定顺序正确的 XOR mask shares；概率 abort、材料错误、通信错误分别报告，不输出任何 mask。成功功能与排序键为分数降序、original index 升序的冻结 oracle 一致。

S13 按当前真实代码审查的**拟允许泄露草案**如下；这是候选 ideal leakage，不代表仓库隐私政策已经批准：

| 字段 | 公开给双方的来源/时间 | 当前审查处理 |
| --- | --- | --- |
| `n,K,session,width,padding` | 建联/材料配置 | 明确公开配置 |
| shuffle 后 `public_z` masked list | forward shuffle 后 | 当前双方可见；不能将 masked operands 当作不泄露而跳过模拟 |
| 两 task 共用的真实匿名 handle 空间 | 两份 Select 运行期间 | 对应同一真实记录的匿名句柄关系进入泄露草案；不能以两次边际均匀为证明 |
| R1–R4 匿名边、打开比较 bit、pivot/U/V/W/活跃边 | 每轮比较交换后 | 全部进入拟泄露；后轮图是前轮公开 bit 与抽样币的自适应后处理 |
| selected anonymous handle | 可由最终公开比较 transcript 与固定算法推导 | 视为 transcript 的派生泄露，即使 API 未单独发送 |
| 算法 abort 类别 | final agreement 相位 | 只有确实交换相同状态的算法路径是 `PEER_AGREED` |
| 材料/通信 abort、流量、阶段长度、槽访问 | 对应发生时 | 可能是单方本地可见；不能声称双方一致或隐藏访问模式 |
| 最终各方输出 mask share | 输出时 | 理想功能的各方输出；不允许重构完整 mask 的 party 日志 |

腐化 P0/P1 的实际 view 还分别包含：自己的正式 raw-score share 与输入适配中间 share；自己的 forward/inverse shuffle `sigma/tau/a/e/r_share` 和对端消息；两 task 共用的匿名句柄 transcript；自己的完整解密 bundle（包含未用槽）及持久 claim 轨迹；实际本地 uCMP/DCF key shares、masked operands、自己发出和接收的比较 share、打开的 bits；selected handle、abort scope 和自己的输出 share。父级 TEST_ONLY root harness 能接触双方 fixture、bundle、密钥和 oracle，是测试权限，不是 P0/P1 模拟器。T 在可信非合谋模型中只按公开配置离线发材；S12/S13 root harness 的预置 AEAD 凭据不是生产凭据发行协议。

## 5. 安全引理状态和第一个未闭合 hybrid

**引理 A（shuffle permutation 关系，部分证明）。** 在 dealer 按均匀独立 `pi,sigma0,sigma1` 生成的代码中，`tau0=pi∘sigma1^{-1}`、`tau1=pi∘sigma0^{-1}`。固定 P0 的 `sigma0` 后，独立均匀 `sigma1` 使 `tau0` 均匀，故仅就 `(sigma0,tau0)` 的边缘，分布不依赖 `pi`；P1 对称。该等式是代数/信息论事实。它不模拟本地 `a/e/r_share`、CMP-aggregate party keys、在线 shuffle 消息、公开 `public_z` 与逆路由输出 shares 的**联合**分布，所以完整 party-view shuffle 引理仍 `NOT_PROVED`。

**引理 B（相关整池 uCMP/DCF，未证明）。** `ProtocolIUcmpMaterial` 在 `protocol_i_ucmp.cpp` 以 `alpha=(mask_left-mask_right) mod 2^b` 调用 `keyGenDCF(b,64,alpha,1)`；每次 party key 都单次消费。对每个真实无序句柄对，forward shuffle CmpAgg、两个 task 的 8 个 Select pool、membership 共 10 个 key-generation pool 复用来自 forward shuffle 的同一节点 mask 差 `alpha_ij`；inverse shuffle CmpAgg 使用其自己的节点 mask。S12 全池总槽为 `2p+2+11*C(n,2)`，其中 2 个 shuffle CmpAgg pool 按当前逻辑未 Eval。DCF KeyGen 通过 `FSSConfig::prngs[omp_get_thread_num()].get<array<block,2>>()` 取随机块；Protocol I shuffle 在 T 侧以 OS entropy 初始化全局 PRNG 后，各 KeyGen 顺序消费 AES PRNG 输出。这里是**一个带安全 PRG 种子的顺序流**，不是代码为每个 slot 单独做了 domain-separated KeyGen。

若要作归约，至少需要从该精确 DCF 实现证明：给定腐化方本地 shuffle/输入辅助状态，整个（含未访问槽）party key pool 对相关 `alpha_ij` 的多 key 模拟；再覆盖对公开 masked operands 的一次性/自适应 Eval 输出。可尝试逐-key hybrid，损失为实际相关 key 数乘单 key 优势，再加整个 AES PRNG 流的 PRG 优势；但当前仓库没有给出适用到这些辅助输入和被打开 Eval transcript 的定理、模拟算法或优势界。不能由单 key FSS 论述、独立调用次数、OS seed 或每个 key 的 AEAD 包封装直接推出它。**第一个未闭合步骤**是 H0 中腐化方的本地 shuffle material + 全部相关 DCF party keys（包括未用 key）替换为只依赖 leakage 和其 ideal output share 的联合模拟分布；当前没有可调用的 simulator 或已核实假设支持这一步。

**引理 C（自适应控制，条件成立）。** 固定允许泄露完整 `public_z`、当轮边、已打开 bit 和公开随机 tape 后，算法后续图、pivot、U/V/W、selected handle 和 slot ID 是这些公开值的确定/随机后处理；后轮 edge list 在前轮 Eval 和 bit 打开后才冻结。故控制本身不产生超出该强泄露草案的信息。但它不证明单方 key Eval share 或完整 pool 在自适应输入上的可模拟性；仍依赖引理 B。

**引理 D（组合，条件成立）。** 若 A 的完整 local-view 模拟、B 的 pool/Eval 模拟成立，并将上述 transcript/abort 视为明确理想泄露，则可按顺序组合两份共享句柄的 Select、membership 与 inverse shuffle，并模拟最终 party output share。当前 A/B 未闭合，且异常本地中止可能未与 peer 达成一致；因此联合组合 `NO-GO`。S13 未发现“匿名边际一定泄露原 index”的两世界反例，也不以此宣称安全或不可能。

**最小下一修正任务。** 针对 S12 pin 下的 `protocol_i_parallel_shuffle.cpp`、`protocol_i_ucmp.cpp` 与 `ext/FSS/dcf.cpp`，给出可审查的“相关节点 mask、多 key 全池、辅助 shuffle state、adaptive public-operand Eval transcript”单方联合模拟器与优势界；证明不了则换用给出该性质的适配器。修正须显式给每 slot 使用 session/stage/task/round/pair 域分离的 PRG 子流，并加入完整 bundle 多 key 视图的固定对照/审计测试。完成后再单独补齐 shuffle 消息和 output-share simulator。只加域分离不自动关闭证明门。

## 6. 材料合同与容量判定

S12 bundle v1 的 AES-256-GCM nonce 随包随机，header/session/party/n/K/padding/index/comparison width/manifest digest/nonce/length 参与 AAD，清单含 stage/task/round/规范端点/slot ID；一次性 bundle/slot claim 使用 `O_EXCL` marker、`fdatasync`、目录 `fsync`，重启后重复 claim 被拒。该合同分别证明包字节完整性和测试目录下的一次性 claim 行为，不证明 DCF key 与端点函数的数学关系。S12/S13 T 的 AES 解封密钥仍由 root TEST_ONLY harness 预置，并以本地目录/chown 交付；生产凭据建立、跨机器私有认证发材、轮换和密钥灾备均未实现。

容量计算脚本使用显式 uint64 overflow checks，key size `57+24*b`，`b=33+log2(p)`，`p=next_power_of_two(max(n,2))`，slot 数 `2p+2+11*C(n,2)`，manifest `8+20*slots`。下表的 bytes 是单方最小包下界，不含两个 shuffle 的非 key 状态、其他 framing/容器开销；K 不改变全池尺寸，列举端点和中间 K 以防隐藏 K 假设。

| n | K 示例 | p / bits | slots/方 | bundle 下界/方 | 两方下界 | 执行状态 |
|---:| --- | ---: | ---: | ---: | ---: | --- |
| 64 | 1/32/64 | 64 / 39 | 22,306 | 22,578,756 B | 45,157,512 B | 当前 API 上限内 |
| 128 | 1/64/128 | 128 / 40 | 89,666 | 92,944,868 B | 185,889,736 B | 仅预检；代码拒绝 n>64 |
| 256 | 1/128/256 | 256 / 41 | 359,554 | 381,398,820 B | 762,797,640 B | 仅预检 |
| 1,000 | 1/500/1,000 | 1,024 / 43 | 5,496,550 | 6,095,229,528 B | 12,190,459,056 B | 仅预检 |
| 10,000 | 1/5,000/10,000 | 16,384 / 47 | 549,977,770 | 662,712,986,988 B | 1,325,425,973,976 B | 仅预检 |
| 100,000 | 1/50,000/100,000 | 131,072 / 50 | 54,999,712,146 | 70,234,531,744,756 B | 140,469,063,489,512 B | 仅预检 |
| 1,000,000 | 1/500,000/1,000,000 | 1,048,576 / 53 | 5,499,996,597,154 | 7,419,494,453,256,900 B | 14,838,988,906,513,800 B | 仅算术预检；53-bit cap 内 |

上述均为**解析预检，不是生成/落盘测量**。n=128 即使字节下界约 93 MB/方，也因 party API 明确 `n≤64`、没有更大规模的 conformance→oracle→进程 E2E 和预算实测而不运行。n=1e6 位宽仍在当前 53-bit uCMP 上限，但 O(n²) 池容量已是 PB 级，绝不能据此宣称可运行。

## 7. 计量和阶段顺序

S13 给实际 API 和三进程 runner 加入 raw adapter、forward shuffle、R1–R4、membership、inverse、状态协调、bundle open/claim、slot 查找/持久 claim 回调、party/T peak RSS 的独立计时；S4 Eval、Select channel exchange、raw adapter Eval/exchange 另列。`durable_claim_io` 是持久 claim callback 的总时间；代码路径每个 slot 执行 `O_EXCL` 创建、文件 `fdatasync` 和目录 `fsync`，计数可直接对应，计时不宣称内核 syscall 的独立 profiling。uCMP/DCF、逻辑边、key 槽、unused CmpAgg、字节和因果 phase 继续分别记数。库内 AES/PRG 总调用次数仍 `NOT_MEASURED`。

这只准备了与 `BMW16_COMPARISON_RUN_SCHEMA_V1` 相容的失败/阶段字段，不启动正式 LAN/WAN 五次矩阵。V3/V4 顺序不变：允许用户已授权的有界研发；V3/V4 正式性能验收及六方案报告均未完成。

## 8. 门禁

| 项目 | 结论 |
| --- | --- |
| S12 functional/source acceptance | ACCEPTED_WITH_EXPERIMENTAL_BOUNDARY |
| S12 evidence portability | 修正归档/校验合同；旧逐工作树哈希不能跨 checkout 直接比较 |
| S13 party exit/status/mask contract | PASS（本地测试 runner 与 API 类型边界） |
| S4 common-tape differential | PASS，39 例逐层差分复跑 |
| S13 three-process functional E2E | 52 SUCCESS / 1 注入算法中止 / 1 材料中止 / 1 未预期 API 异常 / 1 传输关闭；无自然概率 abort 观察 |
| complete party-view security | NO-GO / NOT_PROVED，首个缺口为引理 B 的 H0→H1 联合替换 |
| production T credentials/private delivery | NOT_IMPLEMENTED；本地 root 预置凭据不是生产传输 |
| n=128 / larger-scale | 未运行；当前硬上限 n=64 |
| V4 formal performance / six-scheme acceptance | NOT_STARTED |

完整复跑、指标、归档哈希、代码与原始记录见 [S13 验证报告](../reproduction/BMW16_S13_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md)。
