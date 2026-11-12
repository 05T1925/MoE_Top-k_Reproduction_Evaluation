# S65 实验原型冻结与学姐代码审查交接报告

路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`。本阶段是代码审查交接，不是 BB90/BMW16 原算法复现、安全准入或正式性能比较。

## 1. 精确来源与工作区

| 项目 | 本轮核查 |
|---|---|
| 唯一运行时代码基线 | 远端 `m6b-i-project-select5-s64@8debdcfa8e03527decaea13b956e3e18cf941ccd`；tree `c3227a79a04d109c4abba2b96f08b8034af145b8`，父级 S63 `29ac1c97ddf4fce3c9371e59d1fd259e381b3aeb`。 |
| S64 来源清单 | 13 项关键 Git blob 的 SHA-256 逐项一致；按 path/TAB/hash/LF 复算规范 SHA-256 为 `73dc2e00269e910107d3a8ec90c5a4bdb3875511eb9adac6481cd3ba1529c2dd`。 |
| S65 差异 | 仅交接说明、决策、报告和 manifest；相对于 S64 的 `VFSS/`、`VFSS-baseline/` Git diff 为空。S64 原工作树及主工作树未清理或覆盖。 |
| Windows 行尾 | 本机 `core.autocrlf=true` 会使工作树物理文件与 Git blob 的字节哈希不同。来源核验读取远端 Git blob；源码包用 `git -c core.autocrlf=false archive`，解包后上述 13 项再验均一致。 |

仓库 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、S64 decision/report/gates/cases/source manifest 已核读。依据类别分别是：BB90 仅作研究参考；Top-K 语义、五迭代 Select 和托管流程属于项目派生规格；S64 的 28 案属于固定源码上的测试观察；真实部署安全仍是待证明假设。

## 2. 交接对象与代码路径

入口为 [一页中文说明](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S65_REVIEW_HANDOFF.md)，含最小构建命令、8 案运行命令、固定分数和预期 mask、角色表、调用链、源码导航、三项学姐审查问题以及可直接转发的短讯。对应代码：

| 环节 | 源码位置 | 审查要点 |
|---|---|---|
| 构建隔离 | `VFSS/CMakeLists.txt:27-52,115-145,358-398`；`VFSS/include/moe_topk/score_to_mask_v3_gmw.h:3-4` | 默认 OFF；GMW-v3、receipt/v2、FTv3 分别显式启用，头文件拒绝未 opt-in。 |
| T 发材 | `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp:840-1007,1126-1206,1470` | triples/置换、签名 package 与 metadata-only context、版本绑定、一次性 lease。 |
| 五次 Select | `VFSS/src/moe_topk/project_derived_select5.cpp:355-520` | `:369` 的 0..4 五次**算法迭代**，末次精确比较位于 `:382-407`。 |
| P0/P1 | `VFSS/src/moe_topk/score_to_mask_v3_party.cpp:522-915` | signed Q20.12 输入份额转换、比较/路由、原序 mask、刷新、加密提交、receipt/fetch。 |
| A 提交/恢复 | `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:249-254,318-437,839-903` | 第一份 PENDING、两份齐后 COMMITTED、固定 token、认证 fetch。 |
| 冻结 oracle 与独立进程 E2E | `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:1046-1550,2139-2173` | Dealer/A/P0/P1 子进程、逐方 bit 与 XOR oracle、重启取回。 |

计算方消息包括 READY、Beaver batch、采样 seed/ACK、活动比较结果和输出刷新；A 的 submit/receipt/fetch 是其后续服务消息。S64 `final-n1000.log` 的 SHA-256 为 `3ad62b5092a68811c4dc584164c494230ca5982a397836a5b6bc8395700cc2ee`，其中固定案例的 party exchange phase 为 N128/K64 **88**、N256/K128 **89**、N1000/K500 **106**。N1000 源码上界为 106；这不是五次算法迭代，也不是 RTT/正式性能。

## 3. 可核验源码包

本地归档由 S65 指南固定提交生成，只含 `VFSS/`（269 个受 Git 跟踪文件）、指南和 S64 cases/source manifest（共 272 个普通文件）。实际压缩包 SHA-256 和生成提交见 [交接 manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S65_HANDOFF_MANIFEST_2026-11-12.json)。归档解包后，S64 清单 13 项关键源码文件逐项 SHA-256 均匹配。归档路径检查未发现私钥、证书、材料包、SQLite DB、构建物、原始大日志或 `VFSS-baseline/`。包不加入仓库；远端 S65 分支与指南中的 `git archive` 命令供学姐直接获取或自行生成。

## 4. 继承证据与本轮新增验证

S64 的最终测试二进制 SHA-256 为 `b2e7b0ae8fb61b2da07365c05e8cb28612dacf5a021e6ec757be70133da3d92a`。其 default-OFF Release 不注册 v3/Select5 target/符号，未 opt-in 头文件拒绝使用；显式 ON 的生产库没有 TEST_ONLY 随机 hook；runtime 与 FTv3 bounded conformance 各 exit 0。最终二进制 28/28 定向独立进程案例达到各自预期：26 案得到 share 并与冻结 oracle 逐位一致，2 案人工注入的 no-share 状态符合预期。分组为目标规模 8、N128/256 边界 10、N1000 额外模式 3、API-v2 状态 7。原始日志留在 S64 测试临时目录，S65 不将日志加入仓库。详细逐案字段见 [S64 cases](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S64_CASES_2026-11-09.json)。

本轮不重跑 S64 的 28 案矩阵；由未参与 S63/S64 实现和 S65 指南撰写的接收者从远端 SSH 干净克隆 `11a2b1da734df8f3c09a7fbab4af471c865e3988`，使用 Ubuntu 24.04.4/WSL2、GCC 13.3.0、CMake 3.28.3、Ninja 1.11.1、OpenSSL CLI 3.0.13 新建 Release 构建。第一次配置诊断出缺少 `libsqlite3-dev`，包装器没有可靠捕获其退出码；以 root 安装 `libsqlite3-dev 3.45.1-1ubuntu2.9` 后同配置命令 exit 0。三目标构建 exit 0，runtime conformance 与 FTv3 bounded conformance 各 exit 0。`--ft-v3-n1000` 两次 exit 0，第二次 8/8 案 oracle PASS、双方 COMMITTED，Dealer/P0/P1 各 exit 0，A/重启 A 的 143 是驱动结束时 SIGTERM。第二次原始输出只留在 WSL `/tmp/select5-s65-e2e-receiver.log`，SHA-256 `1a37dda7e7d3c3c474f131e2dd853c8432fe00d569b6d3e079f603debbcfef07`；不入 Git。三项新二进制哈希和逐案 phase 见交接 manifest。

接收者随后只读核对远端最终指南提交 `68d7d09f51780beefd29c1cf170b366dfa22df44`：相对其已测提交，只变动指南文字与归档/克隆/祖先核验说明，`VFSS/`、`VFSS-baseline/` 和三项构建运行命令均未变化；`git merge-base --is-ancestor` 在最终提交 exit 0。该审查是独立功能接收，不是密码学证明、生产部署签核或完整 28 案再测。

## 5. 结论与门禁

**可交接的功能结论**：项目自有五迭代 Select 已在 S64 固定版本的所列 N≤1000 用例上生成 signed Q20.12、稳定同分、原序且恰 K 位的 XOR Top-K mask；双方分别取回本方份额。N1000 只限指定输入/K，有限测试不证明全输入正确性。

**证明与部署边界**：S63 在理想 FTv3 成功发材、理想输出刷新/A token、诚实对端因果执行、静态半诚实单方与外生 SuccessFence 下的 N≤1000 应用/API 视图属于继承的限定结论。真实 T/A 随机源、签名和多文件发布、SQLite/掉电、TLS/mTLS、收件人加密、实际有限 timeout、协议级中止和完整部署 party-view 保持 OPEN。FTv3 极低概率 `ISSUANCE_ABORT` 后的新 invocation 重试、发材失败双方通知/启动规则和 abort-aware 定理范围仍待负责人明确决定；测试控制器不是生产启动屏障。自然业务 I/O 故障沿用 S64 `NOT_REPRODUCED`，人工故障注入不等同于自然故障。

正式 LAN/WAN、RTT、AAV86 及跨方案性能：`NOT_RUN/NOT_MEASURED`；`secure` alias 未创建；不合并 main。建议学姐先审查交接页列出的三项具体问题，后续安全与性能分别立门。
