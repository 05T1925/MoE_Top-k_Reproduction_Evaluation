# BMW16 S31：服务器性能实验准备包

日期：2026-10-10  
方案标签：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**  
状态：**READY_FOR_FUTURE_V4**（非正式测量；V3 前置仍有效）

## 1. 适用范围

本包把 S30 conditional-v1/v2 入口及 S31 接收结果整理成后续服务器任务可复用的配置、数据、构建、身份与计量合同。v1 接收范围是 `1≤n≤256`，v2 是 `1≤n≤1000`；两者调用同一 Select/shuffle/DCF party runtime 和材料 ABI。该候选占原 BB90 目标项的 **PROJECT_DERIVED 替代路线**，不是 BB90 复现，不继承 BMW16 定理的失败界或比较常数。

本包不表示 V3 已完成、不启动 V4，也不把 WSL loopback E2E 当作 LAN/WAN 数据。正式矩阵仍为 1 次预热 + 5 次 fresh-material 正式重复，须待 V3 前置和实验候选性能资格另行满足后执行。

## 2. 冻结身份与构建

| 项 | S31 接收记录 |
|---|---|
| S29 最终接收基线 | `871b6683d15a34d117eb24353611d8ef48434aca` |
| S30 被测 runtime | `eac5151e96f3cc67d2bc664c0d300b51ef672e10` |
| S30 最终报告 | `d1e8d8dc0a4f594af6e7d6cdb4ce469a4d96797a` |
| S31 接收工作树起点 | `d1e8d8dc0a4f594af6e7d6cdb4ce469a4d96797a` |
| 条件编译 | Release、`BUILD_TESTING=ON`、实验适配器/v1/v2=`ON`、failpoints/DCF 计数器=`OFF` |
| 服务器正式构建 | 必须记录本包冻结的最终源码 commit、重建 binary SHA-256 和完整 CMake cache；S31 loopback binary 不冒充未来租赁主机 binary |

重建命令：

```bash
cmake -S VFSS -B build/bmw16-v2-release -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF \
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build build/bmw16-v2-release --target \
  moe_topk_bmw16_experimental_party_node \
  moe_topk_bmw16_experimental_party_node_e2e_test \
  moe_topk_bmw16_experimental_party_test \
  moe_topk_bmw16_conditional_secure_v1_conformance_test \
  moe_topk_bmw16_conditional_secure_v2_conformance_test
```

每次正式运行前必须用 `sha256sum` 记录 `party_node`、E2E/orchestrator、oracle 工具和相关静态库；保存 `git rev-parse HEAD` 与 `CMakeCache.txt` hash。不要复用旧 revision 的 binary 或材料。

## 3. 输入、oracle、会话和随机性

- 跨协议组共享相同 `n/K`、同一份 signed 32-bit 原始分数向量及同一 stable oracle 定义：分数降序、`original_index` 升序。冻结 `dataset_id`、生成器版本、输入种子、score 文件 SHA-256、oracle index/mask 文件 SHA-256。
- TEST_ONLY 控制器为每个 fresh run 生成新的加法 share split；记录 `share_split_seed`。同一 run 的 I 全对全、I+AAV86、BMW16-derived 组合使用相同底层 raw 输入，且记录每条路径的源码/bin revision。输入文件、shares、证书和材料放在仓库外受保护 run root。
- 本轮已核验的 S30/S31 n=1000 harness 使用 score/session 的 TEST_ONLY 标识 `0x2030100080`、`K=80`。该标识可复现测试输入，不是 party 算法随机币或 FSS KeyGen 随机种子。
- 正式 party 的算法随机性、材料 KeyGen 和 sampler coin 均来自运行时 CSPRNG；不得固定随机带。每次 repetition 使用唯一新 session、fresh T 材料和 fresh online OS entropy。保留材料 ID 与 coin commitment/hash（若日志合同允许），不保存私钥/KeyGen seed。共同随机 tape 只用于 TEST_ONLY differential。
- 记录 `preheat` 与 5 次 `formal` 每次尝试，包括 SUCCESS、自然算法 ABORT、工程/材料错误、通信错误、资源失败；失败 run 不从耗时分母删除。另报告成功条件下延迟和 all-attempt 资源/时间。

## 4. v2 准入与容量预算

一般路径 `n≥2`：`p=next_power_of_two(n)`、`C=n(n−1)/2`、`index_bits=ceil(log2 n)`、`comparison_bits=33+index_bits`、
`N_DCF=2p+9C`（KeyGen 次数）、每方槽数 `2p+2+9C`（另含 forward/inverse shuffle 两槽）。每个 uCMP 比较 key 在双方各执行两次 DCF Eval；Eval 次数不是槽数。n=1 是 shortcut。

| n | K | p | comparison bits | N_DCF | slots/party | sidecar bytes/party（S30/S31 layout） |
|---:|---:|---:|---:|---:|---:|---:|
| 128 | 2/8/64/128 | 128 | 40 | 73,408 | 73,410 | 75,566,180 |
| 256 | 2/8/128/256 | 256 | 41 | 294,272 | 294,274 | 310,504,484 |
| 1,000 | 80 | 1,024 | 43 | 4,497,548 | 4,497,550 | 4,967,527,664 |
| 10,000 | 16,384 | 47 | 449,987,768 | 449,987,770 | 540,395,955,164 |
| 100,000 | 131,072 | 50 | 44,999,812,144 | 44,999,812,146 | 57,284,427,150,164 |
| 1,000,000 | 1,048,576 | 53 | 4,499,997,597,152 | 4,499,997,597,154 | 6,052,493,947,500,164 |

容量公式来自 checked integer/格式推导；只有 S30/S31 明确注明的 `n=1000` E2E 字节是当前运行观察。表中 `n≥10,000` 不运行、不生成材料，不能外推时间或内存。每个 n=1000 fresh repetition约需 4.97 GB/party sidecar，双向 T 发材有效载荷约 9.94 GB；六次重复若保留全部两方包，光 payload 约 59.61 GB。推荐顺序执行并每次确认 parties 完成/销毁后清理材料，建议至少 30 GB 可用 scratch；若保留六份材料/原始文件，建议至少 80 GB。此为容量规划，不是主机资源实测或普适最小配置。

服务器准入前，对实际文件系统运行 checked preflight：容纳 `2×sidecar_bytes + 临时文件/输出/日志`，磁盘至少预留 20% 空间；测 RSS 与 T KeyGen 峰值。CPU 建议至少 8 vCPU、RAM 至少 16 GiB（S30 建议值，需在租机后实测）；禁用并发大配置以避免两个全池任务叠加。网络至少允许每次约 10 GB 离线材料 aggregate transfer，并需实测 RTT、可用带宽、丢包/重传和配额；不预填在线通信量。

## 5. 证书与启动合同

1. 由受信任运维配置步骤分别预置 T、P0、P1 的正式证书/私钥、CA/peer 身份映射、party bundle root、session namespace 和权限；凭据不可由公开 seed 或输入派生。TEST_ONLY CA 不得部署。
2. T 只接受公开 `session,n,K,fingerprint,width` 配置，生成新鲜材料并通过认证 TLS 分发给双方。T 与 P0/P1 必须不同 UID/容器/主机；T 完成且 exit 0 后才允许创建输入 shares 和启动 online party。
3. 双方 receiver 均提交完整 package/manifest/hash 后，orchestrator 验证成对 session、fingerprint、party identity、n/K、材料版本和 commit 状态，建立双方在线 mTLS 会话。任何单方 commit/ACK 不充分；部分提交则废弃 session，下一次必须 fresh session/material。
4. 在线 party 通过认证 TLS stream registry 取用已绑定的各 channel；缺 stream/身份/阶段绑定不得退回裸 FD。正常 party 路径不接受固定测试 tape、TEST_ONLY fault 或明文 oracle。
5. party 的输出状态分开记录：SUCCESS 输出本方 XOR mask share；双方确认的协议概率失败输出共同 ABORT 且不发布 mask；本地材料/通信/OS 错误不得冒称 peer-agreed；无 ACK 仅 LOCAL_ONLY。

该合同延续固定、受保护、不可回滚的 party claim-root 试验范围，不声称抵抗管理员或 VM snapshot rollback，也不等于完成跨主机生产 PKI 注册/轮换。

## 6. 九项正式指标及 provenance

| 主指标 | 正式边界/采集点 | S31 状态 |
|---|---|---|
| `offline_time_ms` | 从 T KeyGen 启动至双方完整材料 durable-commit/ready 屏障的真实 wall time；包含生成、封装、TLS 发材、receiver 验证与落盘；排除租机/编译/输入生成 | `NOT_MEASURED`（S31 E2E 日志中的 T generate 与 TLS delivery 分段只作功能证据） |
| `offline_material_total_bits` | 两个 party 收到的 sidecar+shell 精确文件字节总和×8；AEAD tag/manifest 按实际文件计；TLS record/握手开销另列 | 解析布局已知；正式九指标字段 `NOT_MEASURED` |
| `online_time_ms` | both-party material-ready barrier 后启动双方在线 runtime 到双方最终 agreement/输出原子发布结束的 orchestration wall time；同时保存每方 API 阶段耗时 | `NOT_MEASURED` |
| `online_comm_total_bits` | 在线时间窗内 P0+P1 实际 wire bytes（双向各计一次发送）；报告 TCP/TLS handshake/record 与 app-framed bytes 分项，主计数不得重复相加 | `NOT_MEASURED`；当前 party API 仅覆盖 framed bytes |
| `online_comm_per_party_bits` | 分方发送 wire bits：`P0_sent`、`P1_sent`；总数由两发送端相加，不把接收计数再相加 | `NOT_MEASURED` |
| `online_rounds` | 由实际依赖 DAG 计的完整入口因果通信层；同步采集实际 phase ID，TLS 握手是否计为 application round 单独登记 | 当前代码布局为 12 个应用消息相位推导，正式矩阵实测 `NOT_MEASURED`；Select 内部只有 4 层 |
| `online_prg_calls_total` | 对所有协议阶段的 PRG stream 调用统一计数；另记 AES block 数、DCF Eval、DCF expansion AES 调用和 SHA-256 sampler counter word，禁止互相替代 | `NOT_MEASURED`（现有 FSS counter 关闭且只覆盖局部扩展点） |
| `comparison_edges_total` | `raw-score uCMP queries + Σ四层两任务逻辑 Select 请求（重复也计请求）+ n−1 membership 星形请求`；另报 dummy、重复逻辑、唯一材料槽、实际 Eval | 计数点已有；服务器统一汇总字段 `NOT_MEASURED` |
| `total_time_ms` | 与前述 offline/online 序列相同的总 wall interval；记录总 wall 而非把阶段 median 相加 | `NOT_MEASURED` |

每行结果另含 `run_id, protocol_tag, source_commit, binary_sha256, build_flags, input_id/hash, oracle_hash, input_seed, share_split_seed, session, attempt_kind, natural_abort/engineering_status, host, CPU/RAM/disk, TLS/netem, raw_record_hash`。主数值使用 `measured / checked-layout / NOT_MEASURED` 标签。比较 DCF Eval 数、AES调用数、AES block 数、sampler SHA counter word 数、PRG stream 请求数分栏；`online_rounds` 不从 Select 的四层直接填 4。

## 7. 正式运行顺序与退出规则

1. 租机前保存镜像、依赖锁定、OS/compiler/CMake/OpenSSL 版本、CPU/RAM/disk/network 报告；验证 V3 完成门和本候选独立接收门。
2. 从冻结源码重新构建默认 OFF 与条件入口 ON 两种 Release；检查 disabled targets、failpoint OFF、conditional-v2 不接受 n=1001。
3. 先做数据/oracle 和原语 conformance，再冻结的输入差分，最后两方/T 跨进程 E2E；所有正式运行 fresh material/session。
4. 每个配置先 1 次预热，再 5 次正式尝试；正式数据不按 SUCCESS 筛选。报告 `all-attempt wall/cost`、成功条件延迟、自然算法 abort 率分母、工程失败率和每一失败状态。
5. LAN/WAN 采用同一主机镜像、输入/oracle、输出、材料生命周期、计时边界、指标口径；不同源码 commit 逐方案列明。报告 median/min/max，并保留每次原始记录。
6. n≥10,000 先重做 resource preflight，不因 n=1000 成功就自动准入；现有全池解析 sidecar 已达到约 540 GB/party 起，不在租机时盲目生成。

V3 未完成或九指标采集仍缺项时，运行最多只能标 `EXPERIMENTAL_DIAGNOSTIC`；不得标 `V4_ACCEPTED`，不进入六方案速度排名。
