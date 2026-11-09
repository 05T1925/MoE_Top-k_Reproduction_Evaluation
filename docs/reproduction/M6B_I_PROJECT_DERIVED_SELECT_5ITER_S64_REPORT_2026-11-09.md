# S64 远端独立接收与部署安全桥接报告

路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`。这是项目自有五迭代稳定 Select；论文 BB90 是研究背景，不能把项目实现、其局部功能证据或 S63 理想证明称作 BB90/BMW16 原协议复现或安全通过。

## 1. 来源与执行身份

| 对象 | 本轮核验结果 |
| --- | --- |
| 远端候选 | `refs/heads/m6b-i-project-select5-s63` → `29ac1c97ddf4fce3c9371e59d1fd259e381b3aeb`；tree `2ad3d23ca1e6c555c1f65efb4a1fe431e8bca869`；parent `c9c87799b0f27931249d5b82edce37a5f0f1814c` |
| S63 交付清单 | 5 个 delivery 文件的 Git blob/content SHA-256 重算匹配；规范 SHA-256 `0e81f9520a4bcc1eaea35e8120ea82254dce8c42866c4e8bd510dc9feb73b563` |
| 运行时与继承来源 | S63 manifest 中 7 个 runtime、40 个 S62 read-only evidence、5 个 S63 delivery 项均用各自记录的 commit 的 Git 原始字节逐项重算，结果 7/7、40/40、5/5 匹配；独立接收还核对构建与测试 harness 来源 |
| S48/S52 只读清单 | S48 source/build/evidence manifest 在 S63 树的 Git blob `7c2d4f3776c19d4a4497fc505c0163d97aca6bf9`、SHA-256 `62e7cbf05ef7946232e4e322a6fe8e0a3d3927d2fe600fdf98899efd9aefc9f8`；S52 对应 `24dc93fa1e58432b9a631dddd24402e6e5925c5c`、`547bf0a9db112779a3571da032d37859c2ef87c18a086d40e76a9268b4c90c23`。它们是历史证据，不是 S64 重跑结果 |
| 历史索引 | `codex/m6b-project-select5-historical-handoff@9f3a330f8dd7daac06adbc9b9479155defa036bd`；索引文件 SHA-256 `6bc3f46c94212ce31589640a9b125eac7404d5a819cc8d46f361dbe983e1e427`；导入清单 SHA-256 `c031be91313255d15c591a5b4507810b1b3a430bbb708c2dc1fe915f0b45b8db` |
| 历史关系 | S48 `e05b01d5…` 与 S52 `578ecadf…` 位于 S63 祖先链；交接归档日是归档日，不是原测试日；S17/S27/S28 早期归档不足以还原完整可运行快照 |
| 冻结基线 | S63 与历史交接分支在 `VFSS-baseline/` 的差异为空；S64 未修改该目录 |

S64 的审计与文档位于独立分支 `codex/m6b-i-project-select5-s64`。未参与 S63 推导的接收者在另一隔离工作树独立构建与测试；审计者对测试部署/证明义务作只读核查，不能把本人分析冒称非作者密码学签核。初次汇总哈希核查脚本曾对文件内容执行 `strip()`，错误地丢掉 `PROJECT.md` 尾随换行；改以 `git show` 原始字节重算后 5/5 匹配。这是核查工具的失败尝试，不是源码差异。

## 2. 既有合同与本轮证据类型

冻结功能是双方输入份额合成 signed Q20.12 score，按分数降序、同分按原始索引升序排序，输出原输入顺序、恰有 K 位为 1 的 XOR mask shares。T 是受信离线发材者；A 只保存双方分别加密的 share，两份密文齐备后共同逻辑提交，各方各取回本方 share；两 API 不要求物理同时返回。明文 share 在各自进程内短暂形成，不先写入本地文件。

| 证据层 | 本轮可用结论 | 禁止外推 |
| --- | --- | --- |
| 论文定义 | Protocol I 与 BB90 各自原文定义作为背景 | 本项目五迭代实现并非 BB90/BMW16 复现 |
| 本地参考/旧接收 | S48/S52 原配置的有限 oracle 与资源收据 | 不当作 S64 远端二进制新测试 |
| 项目派生与 S63 | 理想 FTv3 成功、理想 A token、外生 SuccessFence 下 N≤1000 单方静态半诚实应用/API 视图 `PASS_WITH_SCOPE` | 不含真实超时、部署加密/RNG/存储、安全别名 |
| S64 独立运行 | fresh Release、定向 conformance 与逐案多进程功能，详见 cases | 有限样例不证明全输入、密码学或正式性能 |

## 3. 远端 Release 接收

独立接收者从远端固定提交重新建立工作树，不从桌面脏工作区复制源码。default-OFF 与 opt-in 分别使用 fresh Release 目录；生产构建须排除 TEST_ONLY 随机源 hook。接收清单包括正常双进程执行、固定 oracle、receipt/fetch、失败路径与各方退出状态。完整命令、二进制哈希和逐案结果写在 [S64 逐案记录](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_CASES_2026-11-09.json)与[来源/构建/证据清单](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-11-09.json)，以它们为测试记录；S48/S52 历史案例只列继承背景。

独立接收发现原 TEST_ONLY E2E 的成功循环校验了重构 XOR mask 与 oracle，却未单独断言两方每个 share 字节为 bit。S64 仅在隔离测试驱动加入逐方 bit 检查及 N=128/256 的极值、负值、大量同分、边界 K 案例；生产 runtime 源码和 ABI 不变。补断言前的首批 8 案不得作为最终二进制结果，最终 cases 只引用补断言后重编的二进制。一次 `/proc/maps` 采集包装命令因短命子进程退出竞态返回 1，测试进程当时只完成部分案例；该诊断脚本失败单列，不写成协议读写故障。之后直接运行最终二进制的案例和退出码为准。

| fresh Release 入口 | 命令与结果 | SHA-256 |
| --- | --- | --- |
| default-OFF | `cmake -S <S64>/VFSS -B /tmp/s64-receiver-2404/build-off -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF`，构建 `sytorch`；5 个相关开关 OFF，实验 target/定义符号为 0；无宏包含 v3 头按第 4 行 `#error` 拒绝 | `libsytorch.a` `06b170dd5d6b3eba4246520747c9727c09cd2c72ce958a8c10e32bbfa7ff2f83` |
| 生产 opt-in | 显式 GMW-v3、receipt-v2、FTv3 为 ON，FTv2/独立 Select5 为 OFF；`BUILD_TESTING=OFF`；API-v2 符号存在，`test_only`/`with_test_random` 符号为 0 | `libsytorch.a` `b4968191b95b73517985827fbab1093fef6e55412fdea10c2908325f4176c3c0` |
| TEST_ONLY opt-in | 相同三项显式 ON，`BUILD_TESTING=ON`；runtime 和 FTv3 bounded conformance 各 exit 0 | 测试库 `fe9532c116fe34a4dd5c11591af0e9e2be1ecb7cd0b145afe5adb57c220af8d4`；最终 E2E `b2e7b0ae8fb61b2da07365c05e8cb28612dacf5a021e6ec757be70133da3d92a` |

三个构建均为新 Release 目录。为在 Ubuntu 24.04 链接，接收者在 `/tmp/s64-receiver-2404/sqlite` 使用外部 SQLite 3.31.1 头文件及指向系统 `libsqlite3.so.0` 的链接，运行库是 3.45.1-1ubuntu2.5。此临时构建适配未修改仓内 runtime；完整配置命令见 cases JSON。

最终 TEST_ONLY 二进制的四条命令均 exit 0：`--ft-v3-n1000` 8/8、`--s64-ft-v3-n128-n256-boundary` 10/10、`--s48-ft-v3-n1000-extra` 3/3、`--s52-ft-v3-api-v2-targeted` 7/7。合计 28/28 达到预期，其中 26 案得到最终或恢复后的 share，逐方 bit、长度、XOR 冻结 oracle、原序、恰 K、receipt/重启取回均被检查；2 案是人工 timeout 或删除事务行后的预期 no-share/恢复拒绝。普通成功案例 Dealer/P0/P1 exit 0。A 的 exit 143 是测试结束后的主动 SIGTERM；注入案例 P0/P1 的 10/11 是预期 API 错误或恢复状态，不是自然业务崩溃。逐案状态、输入、版本、退出码与四个原始日志 SHA-256 见 [逐案 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_CASES_2026-11-09.json)；原始日志只留 `/tmp/s64-receiver-2404/logs`，不入库。

N=1000 的 K 覆盖 `{1,8,500,999,1000}`，另加固定 seed `0x48c0ffee`、首尾交错和大量同分三案；N=128/256 各覆盖 `{1,8,N/2,N−1,N}`。固定分数含 `INT32_MIN/MAX`、负值、零与同分。API-v2 标准/非标准 callback 失败、提交结果未知、重启恢复与 no-share 拒绝由 7 案单列。这是有限样例差分，不是全输入正确性证明。

本轮在独立接收开始前观察 Ubuntu-24.04 WSL2 约 7.6 GiB RAM、6.7 GiB 可用、2 GiB swap 和 `/tmp` 约 915 GiB 可用。这些是资源预检，不是性能数字。最终 N=1000 目标案例没有资源中止；不从其成功推导更大规模的资源准入。

## 4. 测试部署的非秘密配置

独立测试环境是 Ubuntu 24.04.4 LTS / WSL2 x86_64，内核 `6.6.87.2-microsoft-standard-WSL2`。观察 shell 中 `openssl version -a` 给出 OpenSSL/libcrypto 3.0.13，Ubuntu 包 `3.0.13-0ubuntu3.15`；CLI 报 default provider 3.0.13 active，可列 CTR/HASH/HMAC DRBG；`OPENSSL_CONF` 与 `OPENSSL_MODULES` 在该观察 shell 未设置。`libcrypto.so.3` SHA-256 为 `6a66c3ba6b3749aacc9497973fff00f6ba61c703ba7015d0d7ab3fd9510974b6`，`libssl.so.3` 为 `8e49cebd373d90b55f3626da71dd97fe8e95c68dc4c064d8962b27907690736e`，`libsqlite3.so.0` 为 `eac351cf84d688d1f9235e6165bd57555b31a491cd2f65e0dca67daa69036370`。进程是否实际映射这些文件、OpenSSL 实际使用的 provider/DRBG、熵与 reseed 均以 [S64 环境清单](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_ENVIRONMENT_2026-11-09.json) 中的逐项状态为准。CLI 查询不是 T/A/P 进程内部 DRBG 证明。

接收者在 E2E 期间分别采到 Dealer、A、P0、P1 子进程的 `/proc/<pid>/maps`，确认都映射 `/usr/lib/x86_64-linux-gnu/libcrypto.so.3`；没有观察到各进程内实际 provider 选择、DRBG 类型、熵和 reseed。目标服务器/容器的 image/binary、实际 mapped libcrypto/provider、`OSSL_LIB_CTX`/配置、OS 熵/seed/reseed、fork/restart/并发/共享 RNG、签名器、TLS 密钥和 CA 管理、recipient key 生命周期、SQLite 版本/挂载与失败模型、issuer 文件发布/备份回滚和 deadline 均未由部署方提供。不会以本机 WSL 版本替代目标环境证据，也不收集私钥、seed、token、凭据或原始熵。所需脱敏字段与采集命令在环境 JSON 中逐项列出。

## 5. FTv3 到实际随机流的首个桥接断点

固定源码的 `score_to_mask_v3_gmw.cpp:280-286` 把随机字请求切为至多 1 MiB 的 `RAND_bytes` 调用；`:874-933` 生成 triples、置换并做至多 7 次/位置的 Fisher–Yates 有界拒绝；`:944-1007` 形成签名 package/context，顺序执行两个 package 和 context 的文件创建。burn marker 在发材前创建，`durable_create` 使用排他创建和同步。源代码给出处理顺序，不给出真实部署的随机输出计算安全或跨三文件的原子事务。

单次发行的源码上界（包含 FTv3 的语义材料请求，不包含同进程签名/TLS/其他调用者的内部 RNG）为

`Qmax(m)=G(m)+ceil(m²/8)+4·7·(m−1)` 字节，`Cmax(m)=ceil(G(m)/2²⁰)+1+7·(m−1)` 次 API 调用。`m=256` 为 `Q=5,296,996`、`C=1,792`；`N=1000→m=1024` 为 `Q=88,786,404`、`C=7,247`；单请求 ≤1 MiB。对全部自适应调用的聚合参数应为 `Q_T,total=Σ Q_invocation + Q_other_users`、`C_T,total=Σ C_invocation + C_other_users`，且要包括请求长度、成功/错误、返回字节和中止位置。没有进程寿命/共享 context 使用者上界，就不能将单调用数字充当部署总量。

令 `R_T=((ℓ_i,status_i,bytes_i))_i` 是实际请求/结果有序流，`P` 是公开调用元组，`E_T` 是目标部署 provider/熵/生命周期配置，`S` 是按实际分布平均的隐藏初始与重播种熵。需要证明或明确假设：

`Law(D_P(R_T), S_issue, pkg_0, pkg_1, ctx_A, V_b | P,E_T) ≈ Law(D_P(U_IID), S_issue^I, pkg_0^I, pkg_1^I, ctx_A^I, V_b^I | P,E_T)`，

其中 `D_P` 是同一实际源码的有状态确定/随机后处理（签名、TLS、recipient encryption 的额外随机性须单独耦合），`S_issue` 包括 RAND、签名、序列化与三次发布都成功。若比较无条件失败视图，理想侧还须指定 `status_i` 的错误核；纯 IID 字带本身没有 RAND 失败状态，故不能直接将其当成此等式右侧的完整游戏。若只比较 RAND 均成功的流，第一步需要覆盖完整自适应流的 `ε_T=Adv_DRBG_T(Q_T,total,C_T,total;E_T,P)` 和成功选择效应。目标 provider/熵/生命周期与可引用的优势缺失，源代码和 CLI 版本信息均不能实例化它。此即本轮**首个未闭合实现步骤**。OpenSSL 的 [RAND_bytes 手册](https://docs.openssl.org/3.0/man3/RAND_bytes/) 和 [EVP_RAND 文档](https://docs.openssl.org/3.0/man7/EVP_RAND/)说明 API/配置接口，不能证明本项目目标部署的具体优势。

理想 IID 下 bounded sampler 的 cap-abort `δ≤1023·2^-154<2^-144` 仅是单次语义材料投影的界；不同版本/签名/编码的完整 transcript 不自动继承。若按实际完整发布成功条件化，还须证明 `Pr_I[S_issue]≥p_min>ε_T`，才可使用类似 `Adv_cond≤2ε_T/(p_min−ε_T)` 的条件界；历史 E2E 全成功不是 `p_min` 的证明。更稳妥的目标是给失败分支显式状态核并直接比较无条件 `(view,status)` 联合分布，但现有 S62 发行/启动合同未批准且实际通知接口不完整。

A 在 `score_to_mask_v3_escrow.cpp:249-254` 于最终提交尝试调用一次 `RAND_bytes(32)`；`:318-437` 把 receipt/v2 token 与 COMMITTED 更新置于最终 SQLite 事务，第一份密文可先在 PENDING 的另一次事务，回滚重试可生成不同 token。需独立 `Adv_DRBG_A(Q_A,total,C_A,total;E_A,P)`、事务成功/回滚/恢复状态核及条件事件下的联合 token 分布；A 和 T 是否共享 DRBG/context 不能默认为独立。party 在 `score_to_mask_v3_party.cpp:169,444-445,802` 使用 RAND 生成 seed、AES key/nonce 与 refresh；RSA-OAEP、TLS 也可能产生内部调用，不能由 T 的单次 `Qmax` 覆盖。

## 6. 签名、信道、存储与超时分门

| 组件 | 固定源码事实 | 部署证明仍缺 |
| --- | --- | --- |
| 签名/版本 | FTv3 package/context 的 profile/版本/摘要纳入签名消息；先读 Ed25519 私钥，再 burn marker，最后依次发布三文件 | 目标签名器、密钥保护、失败概率、文件系统原子性/持久性；三文件非单事务 |
| one-shot | party lease SQLite WAL/FULL，T 私有 marker 排他创建，未回滚本地 registry 下拒绝同 invocation 重发 | 跨目录/版本、备份回滚、并发 issuer、部分文件及断电全局结论 |
| 私有信道 | `protocol_i_transport.cpp:211-238` 固定 TLS 1.3/mTLS 配置，`:245-370` 执行证书和主机身份校验，`:484-528` 使用每帧 deadline | 实际 cipher、CA/证书生命周期、mapped libssl/provider、网络/record 视图的联合归约 |
| recipient envelope | `score_to_mask_v3_party.cpp:444-476` 使用 AES-256-GCM，RSA-OAEP-SHA256/MGF1-SHA256 包 key；escrow 取回端相应解密 | recipient 公钥/私钥生命周期、加密随机性/优势与 receipt/hash/context 的组合 |
| escrow 存储 | `score_to_mask_v3_escrow.cpp:228-241` 配 SQLite WAL/FULL/busy timeout；PENDING 与最终 COMMITTED 分属可能不同事务 | 真实挂载/介质、回滚/重启/掉电、服务身份与进程约束 |
| 真实超时 | TLS 握手/逐帧 deadline、escrow 第二份提交等待、connect 等多个可观察切点 | `Law(Vpre_b|Q_b,cap_success,S_actual)` 的正确条件 sampler 或目标调度律；外生 SuccessFence 不等于实际 wall-clock |

源码级语义和功能通过都不量化 `Adv_SIG`、`Adv_RSA_OAEP`、`Adv_AES_GCM`、`Adv_TLS_mTLS`、hash binding、SQLite 或真实 deadline 成功筛选项。不存在可据此汇总为“完整实现安全”的数值优势。

## 7. 中止合同与独立门禁

S62 三项负责人书面决定在本固定快照中未找到：① cap-abort 烧毁 invocation 后是否允许新 invocation 重试；②发材失败是否要求双方同一通知/均不启动，以及通知不可达时的 UNKNOWN/timeout；③隐私定理是否覆盖协议级中止。算法随机数值失败要求暂缓，与 FTv3 cap-abort 接受不是同一决定。`PAIRED_GENERIC_ABORT` 未确认属于既有 `L_S29`；生产 issuer/party API 无共同启动/通知屏障，TEST_ONLY 控制器不构成生产保证。这些为 `OPEN_CONFIRMATION`，不阻碍本轮成功路径有限功能接收，但阻止 abort-aware 安全声明。

独立接收者 `/root/s64_remote_receiver` 未参与 S63 运行时代码、证明或测试证据生成；在自己的远端固定源码工作树亲自 fresh 构建、运行并提供结构化逐案记录。其另行只读检查了 `RAND_bytes` 分块/错误路径、有界 FY、签名与三次顺序发布以及 S55 的条件优势式，同意“目标 `E_T`/整流优势与 `S_issue` 联合状态和 `p_min` 缺失”的 OPEN 裁决。该复核没有实例化目标部署优势，没有对完整 GMW/escrow 安全作密码学签核；审计者本人撰写本报告，不把自审称为独立复核。

正常成功理想应用视图沿用 S63 的 `PASS_WITH_SCOPE`。协议中止联合视图 `OPEN`；真实有限超时 `OPEN`；实际 T/A/P DRBG、签名/发布、SQLite、TLS/mTLS、recipient encryption 以及完整部署 party-view `OPEN`。人工 kill 注入属于工程故障；本轮若无自然业务 I/O 错误，应写 `NOT_REPRODUCED`，而非宣称不可能。正式 LAN/WAN、RTT、AAV86、性能 `NOT_RUN/NOT_MEASURED`。

## 8. 下一步最小证据

部署/SRE 提供目标服务器或容器的脱敏 image、服务拓扑、OS/挂载、fork/restart/并发/限时配置；构建发布提供最终 T/A/P binary 与 process-mapped libcrypto/libssl/provider 文件哈希和 OpenSSL config/property query；密码学维护者提供目标 DRBG 实例化/熵/reseed/共享调用边界及可引用的整流优势；签名/数据库维护者提供签名器、文件发布与数据库失败/恢复模型。采集脚本只能输出非秘密元数据，环境 JSON 给出可执行示例。项目负责人另行决定上述三项中止合同。每项到位后按最早未闭合混合重新审查，不用 WSL 观察代替目标配置。

本轮未改 runtime、测试、材料 ABI、默认开关或 `VFSS-baseline/`；未运行历史 195/211 案或正式性能。S64 新测试集合和任何失败尝试均由逐案 JSON 独立列明。
