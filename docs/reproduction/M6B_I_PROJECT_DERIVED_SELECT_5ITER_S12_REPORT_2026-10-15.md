# M6B-I PROJECT_DERIVED_SELECT_5ITER S12 独立安全复核、隔离与崩溃输出报告

日期：2026-10-15  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
审计对象：S10 接收的 S8 精确源码快照及 S11 DCF 证明桥接  
结论：功能候选完成本轮定向回归；S11 的 DCF 论文桥接未获独立确认；联合单方视图仍 OPEN。无 `secure` alias，不启动正式性能矩阵。

## 1. 接收对象与执行者边界

S12 在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-project-select5-s12\MoE_Top-k_Reproduction_Evaluation`、分支 `codex/m6b-project-select5-s12` 工作，HEAD/base 为 `c3926c68fd14f270faa8b55234311071947fa080`。S8 没有合入 main；S12 以 S11 manifest 中列出的 58 个 S8 文件及 SHA-256 重建对象。S8 manifest SHA-256 为 `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb`，S8 canonical source digest 为 `17f1827077bc3f9dbf6038688bbda466162c4fe53e6436d34abb09703725f957`。当前被审 58 项中，48 项仍与 S8 哈希一致；3 项是 S10/S11 对 `PROJECT.md`、实施计划和参考清单的历史文档 overlay；另 7 项是本轮 runtime/store/test 增量。该区分与逐文件 SHA-256 见 [S12 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json)。

S11 DCF 桥接由本轮另一个独立 review agent `/root/independent_dcf_review` 对实际 S8 `dcf.cpp` 与原论文重新检查；该 reviewer 未参与 S11 证明撰写，也没有承担本轮 runtime 改动。runtime 修改与测试由主实现者执行，**不称为 S12 独立功能接收**。S10 仍是 S8 seed 版本的独立功能接收；S12 接收结论只针对此次独立 DCF 定理映射复核，不把 S10 的功能接收范围扩展到 S12 新增代码。

S8 manifest 没有列出 `VFSS/ext/FSS/dcf.cpp`；该依赖仍是 base Git blob `b85fc93d854f5a3c9875501acb05145c5a9dff32`，当前工作树 SHA-256 为 `1e62cf39a5985e761f5bb7e18e20884835dfe6ac08a8bf55f69e34b1b1b3e0e8`。不得把此隐含依赖误报成 58 项 receipt 内容。

## 2. S11 DCF 桥接：独立复核结论

### 2.1 实际函数和参数映射

本地 `ProtocolIUcmpMaterial` 为 `alpha=(m_left-m_right) mod 2^b`、payload `beta=1` 生成 DCF；函数是 `1[x<alpha]`，输出在 `Z/(2^64)`。`eval_strict_lt` 将同一 party key 在 `x=(z_left-z_right) mod 2^b` 和 `x-2^(b-1)` 两点各 Eval 一次，再做 wrap correction。逻辑比较 bit 由两方 additive shares 打开。序列化包含 party、域宽、key/correction words、group 参数；没有序列化明文 `alpha` 字段。此处是源码行为与功能映射，不代表隐私定理已覆盖实际 key。

### 2.2 首个具体证明断点

检查的主来源为 Boyle et al., *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*, EUROCRYPT 2021 full version：Figure 1、Theorem 2（正文 pp. 12–13）和 Appendix F.1（pp. 42–45），[官方 PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。Theorem 2 中 `Convert_G` 对 `G=Z/(2^64)` 且 `k=64<=lambda` 取伪随机串前 64 位。

本地 `VFSS/ext/FSS/dcf.cpp` 的 `convert(Bout=64, groupSize=1, block, out)` 在 `bytesize=8` 时直接复制 block 的首 8 字节为一个 `uint64_t`。实际终端调用传入 `s & notThreeBlock`；`notThreeBlock=toBlock(~0,~3)` 清掉低 64-bit lane 的低两位。当前 x86_64 little-endian 构建中，这 8 个字节正是低 lane，所以输出总满足 `y & 3 == 0`。它仅有 `2^62` 个可能值。

可复核的分布见证针对“代码转换输入/输出应符合论文 Convert_G 的均匀组元素分布”这一步：谓词 `y mod 4=0` 在本地投影下概率为 1，在均匀 `Z/(2^64)` 下概率为 1/4，差为 3/4。这**不是**已证明的 DCF party-key 区分器；密钥序列化中没有单独的终端 `y` 字段。它足以说明 S11 的直接定理映射少了一个引理，不能照搬 paper theorem。

另一个尚待核验的点是代码以互补随机初始控制位代替 Figure 1 固定 `(0,1)`；即便修正/证明终端转换分布，此扩展也仍须由独立密码学审查确认。受限 AES key 子族 PRG、AES-CTR root stream、OS CSPRNG 和 TLS 假设也必须分别参数化。S12 没有将这些条件写成数值安全等级。

| 安全门 | S12 裁决 | 依据/缺项 |
|---|---|---|
| `DCF_SINGLE_KEY_PRIVACY` | OPEN / S11 bridge not confirmed | `Convert_G` 输入投影少两个随机位；需要对真实 terminal conversion 的定理覆盖证明，或新构造/新定理。不是已知 key attack。 |
| `CORRELATED_POOL_SIMULATION` | OPEN | 单 key 定理桥接尚未建立，不能把 S11 的多 key hybrid 结论升为 PASS。 |
| `ADAPTIVE_QUERY_SIMULATION` | OPEN | 后续地址是此前公开 seed、端点和比较 bit 的确定性函数，但 key pool 模拟仍依赖未闭合的单 key分布桥接。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | OPEN | party 隔离和输出合同已实现/测试；真实掩码记录、完整 approved-L hybrid 仍未被密码学审查闭合。 |
| `JOINT_PARTY_VIEW` | OPEN / NO_GO_FOR_SECURE_CLAIM | 没有完整 P0/P1 simulator。未发现攻击不等于证明通过。 |

S11 所述 M6B5/v2 中共享 `r` 导出的 `alpha_ij=R_i-R_j` 环关系仍只是 dealer 隐藏参数上的代数相关；本轮没有证明单方可从实际局部 key/share 中识别它，也没有凭“代数相关”本身判为攻击。它同样不能代替全池模拟证明。

### 2.3 P0/P1 的实际视图与未完成 hybrid

对 `P_b`（`b∈{0,1}`）的真实视图按本地状态和 peer transcript 分开写为：

`View_b = (input_share_b, package_b, used_and_unused_key_slots_b, local_mask_and_shuffle_state_b, lease/restart_state_b, TLS_peer_transcript_b, local_output_share_b or no-output state)`.

`package_b` 对 N=128 是该方 M6B5/v2 包，N=256 是该方 M6S2 manifest/segments；含本方 DCF/uCMP keys、mask shares、shuffle/input conversion material 和未消费条目。P_b 看不到另一方的 package/manifest/lease 文件。可信 T 在离线时见双方材料和用于生成 shuffle 的 `Pi`，发布后退出；T+P0/P1 合谋、恶意 T、主动腐化和恶意 root/admin 不在本轮模型。在线 transcript 包括本轮 shuffle 后 seed、当前端点/pivot 坐标、公开比较位、masked operands、逻辑 slot ID/方向、固定 PREPARE/COMMIT/ABORT 状态及帧长度。最终 party API 仅输出该方 XOR share。

对任一腐化方，候选 simulator 只能取得获批的这份 `L`、理想稳定 Top-K 输出及该方输入 share。其可先生成公开 phase/seed/端点/bit 记录，再尝试逐槽替换本方已用、未用和被拒绝执行的 DCF key view，并以对方的 ideal output 与本方输入 share 条件化输出 share、abort 和重启状态。但第一项 key-slot hybrid 就缺少单 key indistinguishability 引理：必须证明实际序列化 keyGen 分布可由 paper theorem 模拟，而 `Convert_G` 限制位掩码的 witness 正指出定理映射尚未建立。因此本报告**没有**宣称 P0 或 P1 simulator 成立，也没有把真实 transcript 当作 simulator 输入。即使单 key 引理补齐，两个 schema 仍需分别证明相关全池、opened bit 后自适应地址、unused keys/lease 和 shuffle-output 的逐 hybrid；S12 没有完成这些后续替换。

共享 `r` 的 alpha 关系目前属于“未见单方显式字段/未找到可复现单方区分”，而不是“已证明不可识别”。要使上述 hybrid 可继续，需要先给出适用于实际 party-key 与掩码联合分布的单 key 定理（含 terminal projection 与 randomized complementary control bits），再针对 M6B5/v2 和 M6S2 各提供相关池模拟及自适应组合证明。若选择新 DCF，应以独立 ABI/version 重新发材并重新跑功能与安全审查，不能复用旧材料 ABI。

## 3. 两种材料 schema、party store 与泄露字段

| 属性 | N=128 `M6B5/v2` monolithic | N=256 `M6S2` streamed |
|---|---|---|
| party 本地材料 | 每方一个 `M6B5/v2` 包；本轮 E2E 每方序列化 60,008,572 B | 每方独立 manifest 12,656,192 B、11 段；store 185,925,312 B/方 |
| DCF 槽 | 每方总计 57,280：Select 40,640、score 256、membership 128、另有未被此 shuffle path 使用的 M2CP 16,256 | 每方 166,528 个槽，其中 DCF 163,968、shuffle metadata 2,560；已用和未用 Select 槽均保持在各自 manifest 地址空间 |
| ready/lease | 新增 `M6P2/v1` 88-byte party marker，仅含本方 package digest；`.run` 是本方持久 one-shot session claim | M6S2 ready/manifest 仅绑定本方；每槽 `.lease` 绑定 session/material/party/purpose/iteration/substage/direction/endpoint/digest |
| 本轮隔离 | P0/P1 分目录，trusted supervisor 在 T 退出后改由不同 UID (1000/65534) 拥有；party 只能以自身 UID、私有目录读取 | 相同 OS 目录/UID 隔离，分段 reader 只打开自身 party manifest/segment |

S12 删除了可把两份 monolithic 包放在一个目录的 Select5 stage/publish API，改为强制接收两个不同 party directory。每个 party marker 只带自己的摘要；本地 claim 会核对 marker 内 party、session、fingerprint、N/K/width/material 与 package digest。共享目录路径、目录符号链接、跨 party traversal、对方目录中的所有敏感常规文件读取均由不同 UID 实际探测并拒绝。N=256 测试枚举对方 manifest/segment/ready/lease/trace 文件及目录并以 POSIX 权限拒绝。

### 3.1 实际 L 与各观察者

| 字段/状态 | 谁能看见 | 位置/时机/范围 |
|---|---|---|
| 双方输入 share、自己的 DCF party key/share、自己的 `sigma/tau` 与 shuffle mask state、自己的材料 lease | 对应本地 P0 或 P1；T 在离线生成阶段可见双方材料 | 不通过协议消息发送给对方；目录所有权/0700 与不同 UID 是本地 OS 边界。T 完全可信、无合谋，并在发布材料后退出。 |
| 前向/逆向 shuffle record arrays | 对端 party 能看 TLS 解密后的应用 payload；外部网络观察者仅见 TLS record length/timing | 两轮 shuffle 每方发送 24-byte record 元素数组（`word0..2`），由输入/output shares 与随机 mask/offset 组成。它们是实际的 masked record 值；S12 将其逐 phase 写进 L，而不是把它们模糊写成“不传输”。 |
| 本轮 sample seed | P0、P1 | P0 在前向 shuffle phase 21 完成后，仅对已激活的迭代取 `getrandom(32B)`，phase `40+i` 发送 seed；P1 校验并 ACK。未来迭代 seed 尚未生成。测试用 runtime event trace 验证 shuffle-complete 的调用事件先于每个 seed-send，迭代严格递增。trace 只记录事件、phase、iteration、payload 字节数，不记录 seed。 |
| 当前候选图的端点/pivot 坐标、比较位/调用数 | P0/P1；由同 seed 与前轮 bits 确定性重建 | 比较端点不是重复传一份 index 数组，而是按共同公开控制状态生成；端点在 L 中。后续 endpoint 与材料地址因此对双方可推导。 |
| 比较 masked operands `z_left/z_right` 与 opened bit | P0/P1 | 每个边两项 64-bit share 在 TLS phase pair 中交换；两方求和获得本轮 masked operand，DCF share 打开为 0/1 bit。该项在用户批准的 masked-value/comparison-bit L 内。 |
| 逻辑 DCF slot ID / direction | P0/P1；由已公开的当前 endpoint、iteration、purpose 确定性推出 | wire 上不另发对方文件路径；双方按同一图映射到对应逻辑槽。slot ID 属于已批准 L。文件名、manifest/lease 内部字段、本方 key 和本方 ready digest 只对该方与 trusted supervisor/T 可见。N128 `M6P2` 不再让 P0 从自身 marker 读取 P1 package digest；N256 segment/lease 地址同样只在本方 store 中。 |
| 输出协调 PREPARE/COMMIT/ABORT | 双方通过 mTLS 可见；调用方只收到本方 XOR mask share | 固定状态 opcode 与 phase 在 peer transcript 可见，绑定 session、party、N/K/width/material。PREPARE/COMMIT 是常量阶段通知，其出现顺序和字节长度由已公开的消息顺序/通信长度及成功或统一 ABORT 状态决定；没有附带 rank/share/阈值数据。S12 将这两个状态通知明列进候选 L，安全模拟仍 OPEN。成功时 runtime 仍只返回 local share。 |
| TLS 网络元数据 | TLS 对端可解密应用字段；外部网络观察者见 record 长度、方向和时序 | record 长度落在已批准通信长度；网络到达时间/时延的输入相关性本轮未测，也未做流量分析隐藏论证。当前 party 安全模型依赖私有认证通道，不声称对外部观察者隐藏流量模式。 |

Seed 时序证据来自进程内、紧邻 TLS send/receive 的 runtime event trace 及测试断言，不是被动网卡抓包。E2E 用 T/P0/P1 独立进程、TLS 1.3 socketpair；本轮没有测公网/局域网 wire capture。

## 4. 崩溃安全输出合同

每方本地状态由 O_EXCL + fsync 的 `.run`、`.prepared`、`.commit`、`.published` 递进。运行流程先完成 raw-score 转换、正向 shuffle、Select 和 membership、inverse route；之后写 `.prepared` 并交换 PREPARE；再写 `.commit` 并交换 COMMIT；再写 `.published`，之后函数才可返回本方 `n` 位 XOR share。出错时未成功调用方不输出 share；材料 `.run` 已创建就不能重启重领。

崩溃注入覆盖六个输出边界：run lease 后、prepare marker 后、PREPARE 双方交换后、commit marker 后、COMMIT 双方交换后、published marker 后。六例都没有完整 mask；步骤 5/6 中 P1 可能向 TEST_ONLY 父控制器交付一个本地 share（8 字节 length+share），P0 为崩溃方且无输出。该孤立 share 不足以重构 mask，允许成为 orphan。进程崩溃后 `.run`/阶段标记阻止用相同材料再发布。

保证为 **no complete mask under the tested crash prefixes and trusted recipient supervisor**，不是两个进程在任意崩溃模型中的原子同时发布，也不是“出现任意错误两方总能交换统一 ABORT”：已崩溃的一方无法响应。若调用方要求 all-or-nothing 完整输出，需要另有协调/提交服务或原子接收假设；S12 不声称具备该能力。测试的进程崩溃不是断电、内核故障、POSIX 文件系统回滚或管理员还原快照测试。root/admin 恶意、同 UID 部署、恶意 T、T 与 party 合谋和生产多主机 ACL 部署均排除。

## 5. 定向构建与回归

### 构建环境

- Ubuntu 20.04.6 WSL；GCC/G++ 9.4.0；OpenSSL 1.1.1f；CMake 3.31.6（用户级安装；系统自带 3.16.3 低于项目最低版本 3.17）。
- `Release`, `BUILD_TESTING=ON`, `MOE_TOPK_ENABLE_EMP_OT=OFF`, `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON`。
- 构建与日志位于 `.codex/evidence/M6B_I_S12_FINAL_20261015_153500/`，不在仓库中。最终源码 digest、每个可执行文件 SHA-256、配置与日志 hash 见 S12 manifest。

### 测试矩阵

| 目标 | 结果 |
|---|---|
| Select5 control / DCF membership conformance | PASS |
| M6B5 monolithic party store / ready binding / tamper / replay / crash / race | PASS |
| M6S2 streamed slot store / binding / durable one-shot / crash / race / integrity | PASS |
| TLS 1.3 mTLS framing / peer identity / binding conformance | PASS |
| DCF/uCMP conformance | PASS |
| two-round shuffle conformance | PASS |
| two-round shuffle process E2E | PASS |
| Select5 T/P0/P1 E2E | PASS；18 个 frozen-oracle 成功 + 6 个统一 abort/no-mask + 6 个 crash-prefix case；N=128、N=256；每个成功样例由隔离 TEST_ONLY 父控制器合并 share 并核对稳定 oracle。 |
| Feature-OFF configure + CTest registration | PASS；默认 option 为 OFF，CTests/目标列表没有 Select5 新目标。 |

Select5 E2E 成功输出检查原序、长度、bit 值、weight=K 和冻结 C++ stable K-th oracle；覆盖 N=1/7/128/256、K 边界与中间秩、重复 K=64/128 的不同随机抽样、signed Q20.12 边界样例。资源前置确认 N=256 monolithic 在生成前拒绝、N=257 streamed 拒绝。六个 no-mask fault case 覆盖 package tamper、disconnect、party K mismatch、mTLS wrong identity、T 未正常发布 ready、非法 sample-seed magic。party store 探针覆盖不同 UID、对方目录/各敏感文件拒读、路径穿越与符号链接。六个 crash case 单独作为 `CRASH_CASE_JSON`，不计入原 24 个功能/fault case。

本次随机 tape 的功能比较调用诊断为 N=128 的 7 个成功 case：min/median/max=`298/379/3661`；N=256 的 8 个成功 case：`650/1179.5/13922`。该计数取自逐案功能 JSON，仅描述这次样本，不构成期望成本、高概率界、时间性能或正式 benchmark。

新建的 CMake 3.31.6 fresh Release 在 code re-review 时一次性通过全部注册的 6 个 CTest；streamed-store 与 TLS 目标另直接运行通过。首次构建尝试因系统 CMake 3.16.3 不满足 3.17 最低版本而失败；第二次 build 命令的变量展开丢失 `-B` 参数；后续用明确路径成功。一个 `sudo -n` 测试执行因 WSL 不允许无密码 sudo 失败；改用 `wsl.exe -u root` 启动 trusted test supervisor。另一次误用系统 CTest 3.16 的 `--test-dir`，实际在 source cwd 列测试；后续明确调用用户级 CTest 3.31.6。上述失败均为工具/命令问题，原日志保留，没有覆盖或隐藏源码失败。

另有一次较早的 pre-fix E2E CTest 返回 1，日志仅留下 `faulted T/P0/P1 exit without mask` 的汇总断言，未记录具体 fault case 与各子进程状态，无法据此归因于 runtime 或测试 harness。该尝试未计为通过；后续 E2E 增加逐 fault 的 status/result JSON，将进程退出状态与“是否产生 mask”分开记录，最终 fresh Release 的 6 个 fault case 全部得到 `UNIFORM_ABORT_NO_MASK`。原始失败日志和 exit-code 文件保留在 BASETEST evidence 目录，manifest 记录哈希。

早期 ASAN 小样本复跑在 `FinalStatusTransaction` 复用同一 TLS channel 的析构路径上实际报出 SSL wrapper double-free/use-after-free。修复改为单 channel 加 phase 迁移后，保留的 `n=1` ASAN 日志通过；最终 Release 定向 E2E 及六个崩溃前缀也通过。另一项代码/合同缺陷是 monolithic `ready` 记录含双方 package digest，可能让持有该记录的一方获知对方包摘要；修复为 `M6P2/v1`、88-byte、recipient-bound 单方摘要，并通过 party-store 和 E2E 回归。初次 ASAN 失败与修复后日志的哈希列于 S12 manifest；历史失败文件没有覆盖。

E2E 计数会随已批准的 per-iteration public seed 改变，故每个配置结果是单次功能诊断；不将其写成 S2 期望比较成本证明、高概率界、时间/吞吐性能或正式 benchmark。24 个成功/统一 ABORT 案例和 6 个独立 crash-prefix case 从最终 CTest 原始日志提取至 [S12 case JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_E2E_CASES_2026-10-15.json)；不修改 S8/S10 历史 JSON。原始日志留在仓库外唯一 run-ID evidence 目录。

## 6. 安全门与范围裁决

| 门 | S12 裁决 |
|---|---|
| `S11_DCF_BRIDGE_INDEPENDENT_REVIEW` | `NOT_CONFIRMED`；具体第一个分布/定理映射断点见 §2.2。 |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` |
| `CORRELATED_POOL_SIMULATION` | `OPEN` |
| `ADAPTIVE_QUERY_SIMULATION` | `OPEN` |
| `LEAKAGE_L_MAPPING` | `CONDITIONAL_AUDIT_COMPLETE`：逐字段与观察者列出；密码学模拟未完成。用户已批准字段未重问。 |
| `PARTY_MATERIAL_ISOLATION_N128` | `PASS_IN_TESTED_POSIX_LOCAL_MODEL`；不同 UID/权限和 M6P2 本方摘要，非恶意 root/管理员保证。 |
| `PARTY_MATERIAL_ISOLATION_N256` | `PASS_IN_TESTED_POSIX_LOCAL_MODEL`；不同 UID、独立 manifest/segment/lease。 |
| `CRASH_SAFE_OUTPUT_CONTRACT` | `PASS_NO_COMPLETE_MASK_FOR_TESTED_PREFIXES`；`ALL_OR_NOTHING=NOT_CLAIMED`，允许一个孤立 local share。 |
| `FUNCTIONAL_E2E_N256` | `PASS_FOR_TESTED_K_MATRIX`（见逐案 JSON；不等于安全接收）。 |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` |
| `SECURE_ALIAS_READY` | `NO` |
| `N_GT_256_ADMISSION` | `NO_GO`；N=257 预检拒绝。 |
| `V3_V4_PERFORMANCE_STATUS` | V3 `OPEN`；V4 `NOT_ALLOWED/NOT_RUN`；正式 LAN/WAN `NOT_ALLOWED/NOT_RUN`。 |

后续最短可执行工作：先对 DCF 终端 `Convert_G` 输入分布与 BCG+21 Figure 1 / Theorem 2 给出可审查引理。候选选项只能是（a）保持 ABI 并证明对 126-bit/masked terminal state 的新分布同样安全，或（b）新版本转换/DCF ABI、重新生成材料并完整跑 conformance→oracle differential→T/P0/P1 E2E，再独立审查。不得把 `dcf.cpp` 临时改成均匀投影后继续使用旧 M6B5/v2 或 M6S2 key。单 key 分布桥接通过后，再对两个材料 schema 分别证明相关全池 hybrid、自适应 slot 轨迹和实际 24-byte shuffle arrays/output-share/abort/restart 联合模拟。只有这条模拟闭合并经异会话独立审查，才可评估 `secure` alias。

S12 未把 PROJECT_DERIVED 路线列为 BB90 对比项的替代，也未更改 BB90 来源门；M6A V3/V4 原前置不变。源文件修改与报告/计划更改清单见末尾的 [machine-readable gate summary](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_GATES_2026-10-15.json) 和 [source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json)。

## 7. S12 本轮修改文件

Runtime/store/test 改动（7 项）：

- `VFSS/CMakeLists.txt`
- `VFSS/include/moe_topk/protocol_i_project_derived_select5.h`
- `VFSS/include/moe_topk/protocol_i_transport.h`
- `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp`
- `VFSS/src/moe_topk/protocol_i_transport.cpp`
- `VFSS/tests/moe_topk/protocol_i_project_derived_select5_material_store_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp`

项目记录和本轮 evidence indexes：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/REFERENCE_MANIFEST.md`
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_DECISION_2026-10-15.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_REPORT_2026-10-15.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_E2E_CASES_2026-10-15.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_GATES_2026-10-15.json`

S1–S11 历史报告、原始 JSON 和 evidence 文件未被改写；S12 构建目录、二进制和日志均放在仓库外。工作树中另有继承的 S1–S11 未跟踪历史材料，它们不属于上述 S12 修改清单。
