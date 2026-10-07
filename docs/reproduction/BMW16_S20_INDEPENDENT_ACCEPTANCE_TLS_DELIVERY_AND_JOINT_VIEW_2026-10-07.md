# S20 独立接收与 TLS 离线交付验证（2026-10-07）

## 结论摘要

方案身份保持 **Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**。本报告不称其为 BB90 原算法复现；不继承 BMW16 Theorem 8 的概率或比较常数；Select 的四层比较依赖也不等于完整协议四轮。

本轮为 S19 的本地受保护 claim-root 实验交付新增双向 TLS 1.3 私有认证发材接口。T 只按公开配置生成并交付分方 sidecar/shell；接收 party 验证元数据与内容后才原子发布并 ACK。测试启动合同要求 T 退出码为 0 且两个 receiver 均成功后，才创建在线输入并启动 party。测试覆盖了发布失败后留下的 P0 orphan，证明 ready marker 或一方 ACK 单独都不能充当启动许可。

这项工作取得了**本机、不同 UID、loopback mTLS 的交付与功能证据**，没有部署跨主机凭据注册/轮换服务，也没有形成 online party FD 的认证建立服务。联合单方视图中第一个未闭合步骤仍是 H1→H2：完整相关多 key 池在公开比较结果驱动的自适应查询下的输出感知模拟。安全状态仍为 EXPERIMENTAL。

## 基线、来源与身份

| 项 | 身份/结果 |
|---|---|
| 被接收提交 | S19 `617cd3cbbb7b5513c9f606ef5ceb7a73e7a96ef4` |
| S20 起始分支/HEAD | `codex/m6b-i-bmw16-s20`, `617cd3cbbb7b5513c9f606ef5ceb7a73e7a96ef4` |
| 起始差异 | 干净；桌面主工作区未更改 |
| 与 `origin/main` merge-base | `c3926c68fd14f270faa8b55234311071947fa080` |
| 被测源码 revision | 上述 S19 基线加本 S20 工作树差异；最终代码提交记录于报告提交。不得把报告提交称作构建时源码 revision。 |
| 环境 | WSL Ubuntu 24.04，GCC 13.3，CMake 3.28.3，OpenSSL 3；不同 UID 为 22011/22012/22013；loopback TCP |
| S20 构建开关 | Release、`BUILD_TESTING=ON`、实验 Select ON、FSS PRG counters OFF、EMP OT OFF、TEST_ONLY failpoints ON |
| 历史/本轮证据区分 | S19 报告值列为“历史基线”；下文 S20 TLS 测试及新日志列为“本轮复跑”。 |

论文和本地参考实现没有改动或提交。S19、S4 程序来源及旧实验材料仍按既有 S4–S19 决策记录追溯。

## S19 复跑与调用链接收

从 S19 精确提交独立配置并构建 party、T 发材、实验测试及 transport conformance 目标，没有使用 S19 构建物。历史 S19 证据仅用于对照；本轮独立重跑日志在外部证据索引中登记 SHA-256。

调用链复核结果：T 按规范槽序逐槽生成比较材料、写分方 sidecar 与紧凑 shell；party 先在固定受保护 claim-root 持久领取 bundle，再认证读取其活跃槽，沿用既有 VFSS party API 执行 raw-score adapter、shuffle、两个 Select task、membership 和 inverse。在线 party 不调用全池内存生成器，不在线补发材料，不运行明文 oracle。明文重构仅出现在 TEST_ONLY 三进程验证控制器。

n=1000 S19 历史原始证据（不是本轮 S20 数据）：每方 sidecar 4,967,527,664 B、shell 2,013,901 B；总槽数/方 4,497,550，其中 Select/membership 4,495,500；输入 n=1000/K=80 通过原序 mask oracle。其离线 100.664 s、T RSS 22,840 KiB、party sidecar 验证约 82.6 s、party RSS 约 39.5 MiB，均仅标为 S19 历史实测，不混入 S20。

## 私有认证发材合同

新 `t-tls` / `recv-tls` 通过 OpenSSL TLS 1.3 双向证书认证传输。客户端验证 T 的受信 CA 和 DNS 身份，T 验证接收方证书及 party 身份。64 KiB 有序分块传输 shell 与 sidecar；清单认证 session、参数 fingerprint、party、n/K、comparison width、stream ID、slot 数、claim-root 的设备号/inode、文件长度与摘要、双方 shell manifest。接收方写入 claim-root 下的临时文件，逐块核对序号、完整长度和 SHA-256，验证 shell/sidecar 结构后，收到 COMMIT 才 fdatasync、no-clobber 发布、设定只读权限、fsync 目录，最后回 ACK。外层 TLS/manifest 绑定不证明 FSS key 与阈值的数学关系；该关系仍由现有 KeyGen 及其安全分析承担。

T 先生成并准备两个 party 的包，再按顺序提交两方。当前两方文件发布不是跨主机分布式事务：P0 COMMIT 后若 P1 发布失败，P0 会留下可验证的 orphan 包。合同明确要求启动器同时观察 T exit 0 和两个 receiver exit 0，否则不得创建在线输入/启动 party；marker 存在或单方 ready 不足以授权启动。若对端 ACK 无法确认，只能报告本地/发材失败，不声称双方已达成共同中止。

当前一次性保证范围仍是 S19 固定受保护、不可回滚 claim-root。跨 store 复制、快照回滚、管理员权限不在保证范围内。

## 本轮 TLS 独立进程验证

测试命令形态：

```text
moe_topk_bmw16_experimental_party_node_e2e_test <party-node> --tls
moe_topk_bmw16_experimental_party_node_e2e_test <party-node> --tls-n1000
```

`--tls` 在三个不同 UID 下 fork/exec T、P0、P1，生成临时测试 CA 并通过真实 loopback TCP 完成 mTLS。测试控制器只在两方状态成功且 T exit 0 后提供测试输入，并在 TEST_ONLY 边界重构输出与冻结 oracle 比较。

| 范围 | 本轮结果 |
|---|---|
| 成功矩阵 | n=2/5/8/64/128/256，输入 seeds 为 `0x20201001/05/08`、`0x20206401`、`0x20212880`、`0x20225602`；K=1/5/4/8/128/2，覆盖重复、极值及非二次幂；成功均检查原序、二值、weight K 和逐位 oracle |
| n=1 | 最终 `--tls` 测试命令先复跑 n=1 singleton shortcut；它不生成/领取材料，报告不将其冒充一般 Select 路径 |
| 负例 | 13 项：错 T 凭据、party/session/stream 错配、乱序/截断/坏 tag、既存目标保护、接收 fsync/publish 错误、T 中断、P1 失败导致 P0 orphan、peer close 与认证后静默；失败路径不创建在线输入/不产生 mask |
| OS 身份访问 | 6 个成功配置均以 UID 22012/22013/22011 fork 后实际尝试交叉打开对方输入 share、party key、shell、sidecar；P0/P1 互不可读，T 不能读取在线输入，均由 EACCES/EPERM 判定 PASS。仅证明本机 UID/目录模式测试边界 |
| 传输/文件 | n=256 每方 sidecar 310,504,484 B、shell 504,781 B，双方加 framing 收发合计 622,142,670 B；这是本机测试观测值，不是网络性能结论 |
| S20 n=1000/K=80 | fresh bundle，T/接收/P0/P1 全部 exit=0；SUCCESS_ORACLE_CHECKED；每方 sidecar 4,967,527,664 B、shell 2,013,901 B；T 离线 522.479 s、双向传输约 415.454 s、party 全链约 522.457 s、RSS 约 39,860 KiB，全部是本机 loopback 的研究性实测，不能作为 LAN/WAN 数字 |
| claim-root 重启重领 | 6 个 TLS 配置均先成功，再以相同 party 身份和同一固定 claim-root 重启；P0/P1 均 exit=20、DURABLE_CLAIM_REJECTED、无 mask |

首次 n=1000 运行已交付材料，但 party 的 30 s 在线超时先于全链结束，存活方记录 LOCAL_ONLY communication abort，无 mask。将 n≥1000 E2E 的有界在线超时改为 600 s 后，用 fresh bundle 重跑成功；第一次失败没有从成功统计中删去。该合同不声称跨 store 防复制。

## 异常、启动与 failpoint

party exit 合同沿用结构化 SUCCESS=0、算法 abort=10、材料错误=20、通信错误=30、意外错误=70。传输测试发现断连会触发 SIGPIPE 的问题，改为忽略 SIGPIPE 并返回结构化传输错误；材料验证错误归材料错误。普通构建中的 `MOE_BMW16_TEST_FAILPOINT` 环境分支已移至显式 `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS` 编译选项；选项默认 OFF，且不允许在 `BUILD_TESTING=OFF` 时开启。

## 安全视图和可裁决缺口

拟议理想功能是给两方输入算术份额，在成功时输出原序 XOR mask share；失败时无 mask。候选泄露尚未获批准，至少包括：

| 字段 | 在候选中的来源/作用 | 当前判断 |
|---|---|---|
| `public_z` / masked operands | shuffle 与 uCMP 比较路径上的公开/可观察消息 | 必须纳入模拟，不能只看边际均匀性 |
| 匿名比较图及访问顺序 | R1–R4 每层活跃端点及材料槽读取 | 由已公开比较 bit 自适应影响后续图；尚无联合模拟 |
| 比较 bit、pivot、U/V/W | 两方比较结果打开后驱动 Select 控制流 | 真实消息可见性应逐字段审查，不视为获准泄露 |
| 两 task 共用真实匿名 handle、selected handle | 同一 shuffle 记录跨两个实例的关联与结果选择 | 需要共同置换的组合模拟，未闭合 |
| abort 类别、槽/流量、输出 share | 状态协调及最终结果 | 需要包含失败和部分交付状态的理想功能 |

P0/P1 的完整 view 还包含本地 raw share、forward/inverse shuffle 局部状态和消息、九池已用及未用相关 DCF/uCMP key、Eval share、读取轨迹和本方输出 share。T 可信、离线且不与任一方合谋。

- H0→H1：替换 PRG 流为随机带，需要对应 PRG 安全性及实际调用域/随机用量界；本轮没有完成全路径 PRG 混合证明。
- **H1→H2（首个未闭合步骤）**：需要针对实际 `keyGenDCF`/`evalDCF` ABI 的多 key、相关阈值、完整未用池、输出感知并允许比较结果自适应查询的单方安全引理。普通单 key 隐私不能直接模拟双方 Eval share 合成后打开的 bit，也不能自动处理后续按 bit 选边/槽。当前没有找到可直接引用且覆盖这些辅助输入/输出的仓库定理；本轮没有证明可区分，也没有证明安全。
- H2→H3：还需组合真实 forward/inverse shuffle 局部状态、共同匿名句柄、输出 share 和 abort；roundtrip 测试仅证明功能，不是模拟证明。

可执行的下一步候选 C：建立新标签的非自适应固定全对比较图试验，不在比较结果公开后选择后续端点；将比较 shares 保持秘密并以秘密 rank 汇总后再做 membership。S20 已添加 TEST_ONLY 语义夹具 `experiments/TEST_ONLY_BMW16_S20/fixed_graph_rank_fixture.py`：5,055 个排列/K 用例（n≤6 穷举，另有 n=8 稳定同分与 signed 极值轮换）验证完整固定图的 rank/share 聚合可重构为恰 K、稳定 oracle mask；该夹具只验证功能代数，不模拟 FSS、shuffle 或隐私。候选 C 仍需单独实现协议、证明多 key 视图并计量新增材料、消息和轮数，不能悄悄替换当前算法。当前标签保持 EXPERIMENTAL，无 secure alias。

## 计量与正式实验资格

本地 TLS 矩阵只验证功能、交付和文件合同。完整在线因果轮数、全路径 PRG、正式峰值 RSS、阶段级跨主机时间、LAN/WAN 和失败成本矩阵均不据此填写。不得把单次 loopback 字节或耗时纳入正式六方案表。在线 party socket FD 的认证建立由调用方注入，S20 仅实现离线发材认证连接；因此在线认证连接仍是未实现接口，不应表述为完整跨主机部署。

`FUNCTIONAL_ENTRY`：本轮已跑配置 PASS；`PRIVATE_AUTHENTICATED_OFFLINE_DELIVERY`：不同 UID 本机 mTLS 条件下 PASS，生产信任根配置/跨主机部署 CONDITIONAL；`T_EXIT_STARTUP_GATE`：测试启动器合同 PASS；`JOINT_PARTY_VIEW`：NO-GO/未证明；`ONLINE_AUTHENTICATED_TRANSPORT`：NOT_IMPLEMENTED；`PERFORMANCE_READY`：NOT_RUN。

## 本轮构建、测试与证据

完整命令、外部日志、common-tape、源码及二进制 hash 统一登记于 [`BMW16_S20_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md`](evidence/BMW16_S20_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。本轮只构建并运行指定 Release 目标和两项相关 CTest；全仓 bitpack_test/全量 CTest 未构建，不能写成全仓回归通过。提交前执行 `git diff --check` 并核查禁止路径未进入差异。
