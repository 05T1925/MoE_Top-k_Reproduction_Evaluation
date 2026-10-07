# BMW16 S21：独立接收、在线认证执行与安全门验证报告

日期：2026-10-07
方案：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 结论摘要

S21 从 S20 的精确提交独立建树，复用了唯一的 VFSS C++ Select、party API、逐槽材料和同置换逆路由。它补上了在线 mTLS 的**完整数据通道**：Protocol I 帧头和 payload 都经 OpenSSL TLS 1.3 的 `SSL_read_ex`/`SSL_write_ex`，不再在握手后把裸 FD 交给 framed transport。新增本地可信 supervisor gate，在 T、P0 receiver、P1 receiver 均正常退出且双侧材料配对校验通过以前，不调用输入 share 创建回调。

不同 UID 的真实 TCP 本机 E2E 对 n=2/3/5/8/64/128/256 使用 fresh materials；7 个非单例配置均逐位通过冻结 oracle，mask 长度、二值与 weight K 检查通过。n=1 使用既有 singleton shortcut。另有 1 次 TEST_ONLY 强制概率 abort、1 次错误在线 peer identity 和 1 次在线静默 peer timeout；三者均无 mask。每个成功配置的 durable replay 均被两方拒绝。S20 原提交也独立重建并复跑；其结果与 S21 结果分开记录。

这只接受功能和本机执行路径。跨主机生产凭据注册/轮换与退出状态控制面尚未实现；更关键的是，实际 keyGenDCF/evalDCF 的相关整池、自适应、输出条件单方视图没有可审查的一般性归约。因此仍为 `EXPERIMENTAL`，不创建 secure alias，不启动正式性能矩阵。

## 基线、隔离与来源身份

| 项目 | 身份 |
|---|---|
| S20 起始提交 | `0ec88896fcc72440818bd3110dac482eced80e2c` |
| S21 分支 | `codex/m6b-i-bmw16-s21` |
| S21 worktree | `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s21\MoE_Top-k_Reproduction_Evaluation` |
| 本轮起始状态 | S20 提交、干净；新增改动只在该 worktree |
| S20 独立 worktree | `codex/m6b-i-bmw16-s20`，HEAD 为 S20 提交且干净 |
| 桌面主 worktree | `feat/m6a-performance-evaluation`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`；存在用户未提交差异，全部保留，未复制进 S21 |
| `origin/main` 基线 | `c3926c68fd14f270faa8b55234311071947fa080`；S21 按任务指定从 S20 精确提交派生，没有把不同 main tree 的代码合入 |
| 工具链 | WSL2 Ubuntu 24.04；GCC 13.3；CMake 3.28.3；OpenSSL 3.0.13；Release |
| 受保护对象 | 桌面主工作区、S20 worktree、`VFSS-baseline/`、论文/参考树、PR #28 均未纳入改动 |

S20 源码身份在修改前从未改动的 S20 worktree 计算：

| S20 文件 | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `07e62585ab9e78ac74368aeb3d215b6516d3da36cc44d1e2c4ab44b680b102e2` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `2dec177b8935395f1c0a50499105bf0eb3c15ef19849828890fb75eb854735f7` |
| `VFSS/src/moe_topk/experimental_bmw16_material_delivery.cpp` | `0a2f1617c1211becfff6e4152042b56c575aac465cd57ceb1d953edec34023f9` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `8214555133420584788f713bbb4013cb3b1ff4eecc11ff246e26d28ed37db223` |
| S20 决策 | `122ea8682c19c63ad3ffb39f100ebf1775a8770fdd0100b47012e399f1efbc25` |
| S20 验证报告 | `d138a93e52ea042c3cda12fcf7ed56c8b8069c81de4416295a40bef95cc1e89c` |

S20 历史报告中的 n=1000 首次超时与 fresh retry 成功仍是两次运行。本窗口没有重跑 n=1000，也没有把历史 retry 作为 S21 证据。

## S20 独立接收

在独立、未修改的 S20 worktree 配置 Release/TEST_ONLY build，重新构建 party node、E2E runner 和 party test。S20 common-tape 脚本对 39 个 Python S4/C++ 控制轨迹逐层比较，39/39 通过。直接调用 S20 `--tls` runner，独立 UID fresh-material 三进程成功配置为 n=1 shortcut、n=2/3/5/8/64/128/256（runner 在 S20 tree 中含 n=3）；每个成功运行逐位核对冻结 oracle。S20 的在线数据仍走调用方 FD，这是它的历史能力边界，不被算成 S21 的 TLS online transport。

S20 代表 TLS E2E 原始日志 SHA-256：`3665c54d30b54ff2751d8b4fb1fef1d0b59a6f0b147cd39b36258507f38138c3`。S20 共同 tape `matrix.json`：`e99602f19e5425a38849524b5ac54d7fae944327a7cfe117e235f9912f45308e`。S20 party node SHA-256：`e5730ff9e7719e40904f6a998cd37d4aad5d4edde23f4b8d20f1d78b73e12981`；E2E runner：`8652fa0516da7ba3e36067d152bf82ce47840d945dae335196e120a91ec6da06`。

这次接收是对 S20 原提交的构建与复跑，不覆盖或改写 S20 历史报告。精确复跑命令见下方。

## S21 实现合同

### 在线认证加密传输

新增 `ProtocolIBmw16OnlineTlsChannels`，固定创建 12 条相互认证 TLS 1.3 TCP channel：raw-score 两条、forward shuffle 两条、Select R1–R4 四条、inverse 两条、final agreement 一条、sampling-coin 一条。OpenSSL 校验证书链并校验对端 SAN identity；客户端和服务端都验证对端。禁用 session cache、ticket 与 early data/0-RTT。

TLS app binding 对每条 channel 比较协议版本、session、fingerprint、n、K、比较位宽、规范方向、channel 序号/名称和固定实现标签，再交换绑定 session 的 ACK。现有 `ProtocolIFramedChannel` 构造时消费注册的 authenticated byte stream；TLS 路径先 `verify_all_registered()`，所以 party API 的实际帧头和 payload 都调用 OpenSSL stream。TLS 的绑定拒绝会在 party API 前失败；已进入协议后的 frame 继续验证 sender/receiver、phase/type、n/K/width、session/fingerprint 和序号。边端点、task/slot 和 round 由 Select 控制与材料 slot decoder 绑定，不把 app channel 名称误称为 FSS key 数学关系认证。

connect、listener accept、TLS 握手、binding 与在线 TLS read/write 都有有限超时；TLS 连接采用非阻塞 I/O 和绝对 poll deadline，避免对端逐字节慢发不断重置 socket idle timeout。截断、peer close、认证失败和超时映射为 `ABORT_COMMUNICATION / LOCAL_ONLY`、exit 30，无 mask。没收到另一方确认时不报告双方一致 abort。旧的 FD transport 保留给既有协议和回归；它不被 `party-tls` 路径使用。

### T/P0/P1 启动门与凭据

新增 `protocol_i_bmw16_start_after_offline_gate`：校验 T、P0 receiver、P1 receiver 的 PID 不同、角色顺序与 `(session,fingerprint,n,K)` 一致；等待三者各自退出；只有三个 exit code 都为 0，且调用方的双 package/marker 配对校验回调返回真，才设置 gate-open 并调用创建输入 shares/启动 parties 的回调。测试 E2E 将 share 创建放在唯一 post-gate callback 中。T 在 raw shares 创建前已退出。单边 PREPARE/COMMIT、P0 orphan marker 或文件存在都不能打开 gate；测试要求这些运行不产生 raw shares 或 mask。

这一接口是可调用的本机受信任 supervisor primitive，不是跨主机服务管理器。生产部署仍需外部可信 provisioning：独立发放 T/P0/P1 证书与 CA trust、按 SAN `dealer`/`party0`/`party1` 配置预期身份、限制私钥/AEAD wrapping key/claim-root 只归本方 UID 读取并提供证书轮换/吊销流程。代码检查关键敏感文件 owner 和 group/world 权限；本地 TEST_ONLY CA、root harness 与 PID gate 不等同于生产注册服务或远端退出证明。在线 P1 listener readiness pipe 只证明本机十二个 listener 已绑定且绑定参数一致，不充当远端认证。

概率 abort 的 E2E failpoint 只在 `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON` 的测试构建中存在。S21 另构建 `BUILD_TESTING=OFF`、failpoint OFF 的默认 Release 并检查普通 party-node 不含 `MOE_BMW16_TEST_FAILPOINT` 字符串。所有正常路径仍从 OS CSPRNG 获得协议随机性；共同 tape 与固定测试 seed 只进入 TEST_ONLY oracle harness。

## 本轮验证

### 构建与回归

S20 原始 tree 与 S21 tree 分别独立配置/构建。S21 定向 CTest 8/8 通过：BMW16 party、DPF、Protocol III 三进程、Protocol I uCMP、transport、score input、parallel shuffle conformance 与 parallel shuffle process E2E。无全量 CTest 声明。

S21 common tape 使用仓库 `experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py`：n≤8 的 39 个 S4 Python/C++ trace 逐层匹配；最终源码对应的 `s21_common_tape_final/matrix.json` SHA-256 为 `3fe4277ee51090785cebb94f86748343c1e66257674b7b6bfeff19623bd79660`。该结果只核算法控制轨迹，不证明安全或概率界。

S21 `--tls` 本机真实 TCP、不同 Linux UID、TEST_ONLY 临时证书 fresh-material E2E：

| 配置 | 结果 |
|---|---|
| n=1,K=1 | singleton shortcut，oracle 检查 PASS；不生成全链材料 |
| n=2,K=1 | SUCCESS，原序 mask 与冻结 oracle 逐位一致，weight=K |
| n=3,K=2 | SUCCESS，极值输入，原序 mask 与 oracle 一致，weight=K |
| n=5,K=5 | SUCCESS，重复/边界输入，oracle 一致，weight=K |
| n=8,K=4 | SUCCESS，非二次幂，oracle 一致，weight=K |
| n=64,K=8 | SUCCESS，seeded 输入，oracle 一致，weight=K |
| n=128,K=128 | SUCCESS，K=n，oracle 一致，weight=K |
| n=256,K=2 | SUCCESS，K 边界，oracle 一致，weight=K |

每个成功配置的 T、P0 receive、P1 receive、P0 online、P1 online 均 exit 0；各自同 root 持久 replay 时两方都 exit 20、无 mask。uid 隔离检查覆盖双方输入、材料、解封 key 和 T 对 input shares 不可读。另有：

- TEST_ONLY 强制算法概率 abort：n=3,K=2，P0/P1 exit 10，`PEER_AGREED`，没有 mask；这是注入状态合同检查，不是自然失败概率样本。
- 在线证书 peer SAN 错误：P0/P1 exit 30，身份拒绝，无 mask；abort 只报告本地/可观察状态，不伪称远端一致确认。
- mTLS 与 channel binding 已完成后，P1 暂不读取协议帧：P0 按 1.5 s framing deadline exit 30，P1 观察到断连后 exit 30；无 mask，状态标记 `LOCAL_ONLY_OR_PEER_OBSERVED`。静默只由 TEST_ONLY failpoint 注入，不用来同步正常启动。
- 13 个离线负例覆盖不受信任 T、错误 party/session/stream、既存目标、接收端 fsync/publish/silent、乱序/截断/篡改 chunk，以及 T 在 PREPARE 和部分 COMMIT 后崩溃。所有 case 的 gate 阻止 raw shares/online 启动；部分 COMMIT 可留下 orphan 文件，但要求换 session/material，不能复用。
- 没有观察到自然抽样 abort；本轮 abort 测试是显式注入，不能据此估计算法失败概率。
- 相关 Protocol I/III 与 DCF 8 项定向 CTest 全通过。并行执行 CTest 时曾发生多个进程争用同一 `LastTest.log` 的记录写错误；改为单个串行过滤 CTest 命令后 8/8 通过。这是测试日志竞争，不是用例失败。

E2E 日志中不同 party 的 stdout/stderr 可能按行交错；上表依据唯一带 `tls_e2e`、`tls_algorithm_abort`、`tls_online_auth_abort`、`tls_delivery_abort` 标签的最终汇总行和启动器退出码。party timing 诊断不用于本报告性能结论。

未在 S21 运行 n=1000、LAN/WAN、正式 1+5 或完整九指标矩阵；对应性能字段为 `NOT_MEASURED`。S20 历史 n=1000 首次超时/fresh retry 是单独的 S20 证据，不能移植到 S21。

### 复跑命令

以下命令在 WSL Ubuntu 24.04 执行；`/mnt/c/.../build-s20` 是 S21 worktree 的测试构建目录名（沿用外部 artifact 名称），源码来自 S21 worktree：

```text
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s21/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s20 -G Ninja -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON -DBUILD_TESTING=ON -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s20 --target moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_test moe_topk_m2_transport_conformance_test moe_topk_dpf_conformance_test moe_topk_m3_protocol_iii_three_process_e2e_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test -j 4
ctest --test-dir /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s20 -R "moe_topk_bmw16_experimental_party_test|moe_topk_dpf_conformance_test|moe_topk_m3_protocol_iii_three_process_e2e_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_transport_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_parallel_shuffle_process_e2e_test" --output-on-failure
moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node --tls
py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s20/moe_topk_bmw16_experimental_party_test --s4 C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\select4r.py --out C:\Users\28641\.codex\artifacts\BMW16_S21_20261007\s21_common_tape_final
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s21/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s21-default -G Ninja -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DBUILD_TESTING=OFF -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s21-default --target moe_topk_bmw16_experimental_party_node -j 4
strings /mnt/c/Users/28641/.codex/artifacts/BMW16_S21_20261007/build-s21-default/moe_topk_bmw16_experimental_party_node | grep MOE_BMW16_TEST_FAILPOINT
```

The last grep has no match (expected) in the ordinary build. The S20 exact-source build and independent `--tls` runner were separately invoked from the S20 worktree, not from this S21 source directory.

### 构建与原始证据 SHA-256

所有 artifacts 位于仓库外：`C:\Users\28641\.codex\artifacts\BMW16_S21_20261007\`。证书、私钥、输入份额、sidecar 和 mask 原件不提交；日志只用于本机审计，且测试控制器承担明文 oracle 重构。

| 外部文件 | SHA-256 |
|---|---|
| S20 exact TLS E2E log | `3665c54d30b54ff2751d8b4fb1fef1d0b59a6f0b147cd39b36258507f38138c3` |
| S20 exact common-tape `matrix.json` | `e99602f19e5425a38849524b5ac54d7fae944327a7cfe117e235f9912f45308e` |
| S21 final TLS E2E log | `1108777f662d24fb6f56bdd3fbb18e242a953303f7dc1669b77480c57f2a7b3a` |
| S21 targeted CTest log | `72b446591c5c542db060a5c684e6a239032ebdccb1c04b954612d17c845b3ea0` |
| S21 common-tape `matrix.json` | `3fe4277ee51090785cebb94f86748343c1e66257674b7b6bfeff19623bd79660` |
| S21 TEST_ONLY party node | `6f84bfc1d1019166b92594870486e99a7dd0ff0f554a86586579421df7c36720` |
| S21 TEST_ONLY E2E runner | `024da46637adfa52375ce101a0c4ab8e6b6f0b14e57b4f2a7da656c0e0668163` |
| S21 default `BUILD_TESTING=OFF` party node | `1260bdaece64994d9ccb05decad2bbbaa1db3745f078997a45d7af314de73b43` |

S21 primary source hashes before this report was added:

| File | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `9cf940a31acee01c5733046f0422f69d20b57976be4f7feb5c7bfbb9dc94f48b` |
| `VFSS/include/moe_topk/protocol_i_transport.h` | `62a1b42644339fcf44562ffd2b808efd8e27e8647b2a91fbbd583d2e2fb76284` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `1ab36116de0a1953d1acf1c4df34e2b991445db43534d8f07ee8dd1470b3bdc2` |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | `649c4f2156445051e006fec1b546895f7d49e442caeaecd076d9dbb159f46841` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `8bd335d4b0d94428f0a837a1c8fdad7653e2b97a442bf02617c8b5da0335aa3f` |
| `VFSS/include/moe_topk/experimental_bmw16_online_tls.h` | `7a3657ee9f7db7a0b8ed8b8b16ae1450129a422f909dd6edb6e7c80a9d89184c` |
| `VFSS/include/moe_topk/experimental_bmw16_startup_gate.h` | `d7566a81b544143aa987e7f57edc897f96ef33593e1430c8cf767d3ca3d622eb` |
| `VFSS/src/moe_topk/experimental_bmw16_online_tls.cpp` | `996575e5182449908d3bb72ced852e069afbf61b9f39b1418863fc8d435d6f1b` |
| `VFSS/src/moe_topk/experimental_bmw16_startup_gate.cpp` | `eed39909b52c187d659d560e18894598cca10603b84f408c56a42fb6495f885c` |

No binary, key, certificate, sidecar or raw large log is staged.

## 联合安全裁决

### 候选理想功能与泄露草案

理想功能输入为 P0/P1 各自 signed Q20.12 score additive share；成功仅给两方原始输入顺序的 XOR mask shares，其重构值稳定地按 score 降序、`original_index` 升序且 weight 恰为 K。概率算法 abort 双方无 mask。T 可信、离线且在线静默，不与任一方合谋。没有项目所有者批准下列额外泄露，因此它们均只是泄露草案：

| 字段 | 产生位置 | 当前可见性/裁决 |
|---|---|---|
| n、K、session、fingerprint、width、party/certificate identity | bundle shell、TLS app binding、frame headers | 必要配置字段；候选公开 |
| `public_z`/masked operands | 正式 raw-score adapter 与 forward-shuffle 帧 | 两方协议可见；须在泄露函数中明示 |
| 比较边、已打开 comparison bits、pivot/U/V/W/抽样状态 | 每个 Select round 的 framed payload 与公共状态推进 | 自适应 transcript 可见；尚未获准 |
| 两个 Select task 的共享匿名句柄、selected anonymous handle | 同一个 shuffle handle 空间与两 task 交集 | 不另发原始 index；两方可能从 transcript 推出 handle，仍需模拟证明 |
| slot/材料访问轨迹、流量长度、成功/abort 类别 | `load_once`/claim 和 frame traffic/状态 | 可由 party 本地及网络元数据观察；未获批准 |
| 本地 raw share、完整本地 DCF/uCMP key pool（含未使用槽）、Eval share、shuffle local state | party 进程内存、party sidecar 解封和 API 输出 | 腐化方实际完整视图的一部分，不能从泄露表省略 |
| 输出 share | 各自 `mask.share` | 理想功能的本方输出 |

### Hybrid 状态

- `H0→H1`：代码随机数来自 OS CSPRNG；FSS seed 通过 `protocol_i_parallel_shuffle.cpp::seed_fss_once` 导入现有 PRNG，joint sampler contributions 分别从系统熵获得后异或，再按 task/domain/counter 输入 SHA-256 counter sampler。若做计算安全证明，仍须给出 AES/`osuCrypto::PRNG` 与 SHA-256 counter 的具体伪随机假设、域分离及总调用/优势账本。测试 tape 不进入 party 路径。
- `H1→H2`：这是首个未闭合步骤。实际 `keyGenDCF/evalDCF` 的 party key 视图包含全池已用/未用 key、共享节点 mask 导致的相关阈值、shuffle 辅助状态；在线 party 又会打开两方 Eval shares 之和，且后续 slot/边由已打开 bit 自适应决定。仓库没有提供下面这种可引用的安全定理，也没有对实际序列化 ABI 完成此引理：

  `AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy`: 给出腐化方全部相关多 key pool（包括未用槽）、本地 shuffle 辅助输入和其可见参数；查询可依据此前打开的比较 bit 自适应选择合法 `(operand,slot)`；模拟器应联合模拟该方 Eval share、公开比较 bit、材料访问 transcript 与未用池视图，使其与真实执行不可区分。

  单 key DCF key-share 隐私不能自动推出“完整相关池条件分布 + 输出打开反馈 + 自适应查询”的模拟。仅以独立 KeyGen PRG seeds 做逐 key hybrid 会在第一次将本方 `Eval share` 与已打开输出 bit 关联后缺少条件模拟步骤。当前没有找到可区分攻击，因此也不写成不可能；准确结论是这条 reduction 未闭合。
- `H2→H3`：即使 H1→H2 成立，还要把实际 Protocol I 前向/逆向 shuffle 的 `sigma/tau/a/e/r_share` 本地状态和消息、共享匿名 handle、masked list、abort 与本地输出 mask share 放进同一个 simulator。roundtrip 和 E2E 只能证明功能，不证明这个联合视图。

下一步唯一的主要理论阻塞是给上述实际 ABI 的 `AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy` 完成通用 reduction，或改用满足该游戏的原语/泄露设计并重新审查；然后才处理 H2→H3。当前没有证据支持 `SECURE_ALIAS`。

## 分项门禁

| 门 | 结论 | 范围 |
|---|---|---|
| `FUNCTIONAL_ENTRY` | PASS | 本机不同 UID、fresh material、真实 TCP mTLS；n=1 shortcut 与 n=2/3/5/8/64/128/256 bounded E2E |
| `OFFLINE_DELIVERY` | CONDITIONAL | S20 TLS delivery 在本轮精确源码复跑；不同 UID 本地测试通过。生产凭据发放、跨主机服务与运维流程未部署 |
| `ONLINE_AUTHENTICATED_TRANSPORT` | PASS 本机 / CONDITIONAL 部署 | 所有协议 bytes 经 TLS stream，错误 peer identity 有运行负例；尚无跨主机部署验证 |
| `T_EXIT_STARTUP_GATE` | PASS 本地合同 / CONDITIONAL 生产 | T/P0/P1 exit 与双 package 配对后再建 shares；生产跨主机 supervisor 及退出状态证明缺失 |
| `JOINT_PARTY_VIEW` | NO-GO | 首缺实际 keyGenDCF/evalDCF 的自适应相关多 key 输出条件引理；H2→H3 shuffle/output simulator 随后 |
| `SECURE_ALIAS` | NOT_CREATED | 继续 `PROJECT_DERIVED / EXPERIMENTAL` |
| `PERFORMANCE_READY` | NOT_RUN | 无 n=1000、本地以外网络、正式 V4、LAN/WAN 1+5 或六方案性能资格 |

## 最终文件、差异和复核

本轮 `git diff --check` 通过；最终差异只包含下列 13 个 S21 文件。`VFSS-baseline/`、`Papers/`、密钥/证书、sidecar/bundle、构建物、原始大日志和本地参考工程均未进入 Git。S20 worktree 与桌面主工作区保持原样。提交只在本地 S21 分支完成，不推送、不合并、不修改 PR #28。

| 文件 | 修改内容 |
|---|---|
| `PROJECT.md` | 当前候选状态和 S21 记录 |
| `docs/IMPLEMENTATION_PLAN.md` | S21 提前实验状态及正式验收边界 |
| `docs/decisions/BMW16_S21_ONLINE_AUTHENTICATED_EXECUTION_AND_SECURITY_GATE_2026-10-07.md` | 在线 TLS、启动 gate 与联合视图决策 |
| `docs/reproduction/BMW16_S21_INDEPENDENT_ACCEPTANCE_ONLINE_TLS_AND_SECURITY_GATE_2026-10-07.md` | 本独立接收、验证、哈希与门禁报告 |
| `VFSS/CMakeLists.txt` | 仅在 opt-in adapter 中编入 online TLS 与 startup gate |
| `VFSS/include/moe_topk/protocol_i_transport.h` | authenticated byte stream 接口及 frame 使用状态 |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | TLS stream 注册、领取及 framed byte 路由 |
| `VFSS/include/moe_topk/experimental_bmw16_online_tls.h` | 实验在线 mTLS channel 接口 |
| `VFSS/src/moe_topk/experimental_bmw16_online_tls.cpp` | TLS 1.3 mTLS、binding、绝对 I/O deadline 与通道生命周期 |
| `VFSS/include/moe_topk/experimental_bmw16_startup_gate.h` | 本机可信启动 gate 接口 |
| `VFSS/src/moe_topk/experimental_bmw16_startup_gate.cpp` | T/P0/P1 退出与双 package 校验后的 gate 实现 |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `party-tls` 接线、owner/mode 检查和 TEST_ONLY abort 注入 |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | 不同 UID/TCP 在线 TLS E2E、n=3、概率 abort 和静默 peer deadline 覆盖 |

被测 S21 party-node / runner 对应上述源文件构建出的外部哈希。仓库本地提交 revision 用于冻结本报告与源码组合；没有更新正式安全或性能验收状态。
