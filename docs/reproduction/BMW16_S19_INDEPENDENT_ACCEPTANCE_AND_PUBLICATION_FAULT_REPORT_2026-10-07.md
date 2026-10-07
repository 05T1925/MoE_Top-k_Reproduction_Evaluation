# BMW16 S19 独立接收、发布故障和联合视图复核报告

**方案/证据标签：** `Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`\
**被测基线：** S18 commit `9e25411027a01bcf47b9c316e3c463c2f7c72127`\
**本轮修改后的被测源码：** S19 worktree tree（hash 见[外部证据索引](evidence/BMW16_S19_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)）\
**报告日期：** 2026-10-07\
**原始结果目录：** `C:\Users\28641\.codex\artifacts\BMW16_S19_20261007`（仓库外；原始 E2E 日志不含 score shares、DCF keys、凭据或完整 mask）。

## 1. 接收基线与独立复核边界

在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s19\MoE_Top-k_Reproduction_Evaluation`、分支 `codex/m6b-i-bmw16-s19` 上从 S18 commit 开始；初始 worktree clean，初始 HEAD 等于 S18。`merge-base(HEAD, origin/main)` 为 `c3926c68fd14f270faa8b55234311071947fa080`。主桌面工作区未提交差异保留原样。`VFSS-baseline/`、Papers、本地参考树、E20/E21 与 PR #28 未改。

S18 开工源身份已冻结：stream writer SHA-256 `fcfa6edfd8c15ac18fc29d9313130a40aff5fadb82311a7741336267a163a1b3`，bundle helper `0b064bd0ed6c38855d980d4a24edd85a2e9c360adef6aab0929be88016c05520`，party select `9ba17231af92f1aae92a2e10ba695823553026fda36a49c09df853dedbe8df3a`。S18 的 n=1000 成功行仅作为历史报告；本报告分开记录本轮独立构建和实际执行。S15 报告里的历史 pilot 不在本轮重用。

为独立重建 S18 本身，本轮另建 detached clean worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s18-independent`，HEAD=`9e25411027a01bcf47b9c316e3c463c2f7c72127`，在 `/tmp/bmw16-s18-s19-receive-build` 以 Release 重配并构建 party test、party node、node E2E 和 transport target。首次 CTest 从非 root 用户执行时因 build dir 由 root 创建而无法写 CTest log；切至 root 后 transport 通过。S18 party CTest 第一次触发其原 fixture `pid+session` 命名与外部陈旧目录碰撞，末尾报 `BMW16 stream create output`；不改 S18 源码、也不删碰撞目录，换新测试进程重跑后 party 与 transport 均通过。该首次失败是保留的 S18 fixture 重入缺陷；S19 把 TEST_ONLY stream root 加单调时间 nonce 后，两次 S19 目标 party CTest 均通过。此精确接收流程与日志在外部证据索引中分开标识。

本轮源码审查确认 T 路径使用 S18 stream callback 为每槽生成并立刻密封；不是先调用全池内存 `material_generate()` 再写文件。party 先开本方紧凑 shell、核验完成 marker、持久 burn bundle claim、全文件 bounded hash/sidecar metadata validation，再按需随机读每个 uCMP/DCF 槽；完整 9 池明文 key 未还原到 party 内存。每方 Select+membership 是 `9·C(n,2)` 槽，另有 raw/shuffle shell/sidecar 状态；slot id/任务/轮次/规范端点保持既有 ABI。四个 Select 依赖层不等于 raw-score→mask 的完整在线轮数。

### 旧失败与本轮问题

- S18 曾报告 n=1000 传输超过单帧上限后以 bounded chunks 修复；这是历史纠错，S19 不改 Select/传输分块算法。
- 本轮第一次完整构建所有默认目标时，外部 `bitpack_test` 链接失败：`undefined reference to bitpack::mod(unsigned long, int)`。后续只构建本阶段所需 party、E2E、transport 和 unit 目标；报告不声称全仓 Release 构建或全量 CTest 通过。
- 本轮一次 party unit CTest 碰到旧 TEST_ONLY 临时目录名 `pid+session` 与先前被中止运行残留目录冲突，报 `BMW16 stream create output`。已将该 fixture 路径加单调时钟 nonce；重建后目标 party CTest 通过。旧临时目录留在仓库外，未递归清理。
- Windows Python 的 common-tape launcher 首次未能解析 WSL 子进程。Linux Python 重跑则因 WSL Python 缺 NumPy 在比较器处停止；这些不产生算法结论。最终使用 Windows Python + System32 `wsl.exe` wrapper 调用原 runner、原 C++ 测试及冻结 S4 比较器，完成 39 条与 2 条矩阵。失败诊断记录及原始路径列入外部索引。

## 2. S19 发布/重试修正

旧 `protocol_i_bmw16_bundle_write_atomic` 使用覆盖式 `rename(temp,path)`；T 失败 catch 无条件 `unlink(shell0,shell1,sidecar0,sidecar1)`，可能删掉调用前就存在的用户文件。S18 先完成各 sidecar 再发布两 shell，但没有表达“两个 package 整体 ready”的单一状态。

S19 修改如下：

1. shell 和 stream sidecar 都使用 `O_EXCL` 临时项、写入后 `fdatasync`、`fstat` 记录 inode，再以同文件系统 hard link 发布到最终名。最终名已存在时拒绝而不覆盖。
2. writer 记录自己创建的 temp/final `(device,inode)`。普通异常仅在最终路径仍指向此 inode 时回滚。T 记录 shell/marker 返回的发布身份，catch 不触碰任何没有所有权证明的目标。构造 writer 成功后若整个 pair 未提交，其析构清理本次拥有的 sidecar final。
3. 两份 sidecar 与两份 shell 经过长度/槽数/hash/owner/mode/目录持久化后，T 最后发布 132-byte `BMW16RDY` marker，绑定 `session,fingerprint,n,K,stream_id,manifest0,manifest1`。party 在任何 durable claim、网络交换、key Eval 前核对 root owner、0444 mode、exact length、配置、自己 shell 的 stream id 和本方 manifest。
4. 非零 T exit 是唯一启动门：launcher 必须等待 T 进程正常 exit 0 后才生成 raw shares、启动 P0/P1。无轮询、无“文件出现即启动”。若 marker durable 后 T 崩溃，marker 和 package 可能仍存在，但 runner 不启动 party；该 orphan 需隔离。marker 本身不能证明远程 T 进程 exit code，也不能替代 launcher 合同。
5. 错误进程分类保留：SUCCESS `0`、自然算法 abort `10`、材料错误 `20`、通信错误 `30`、意外异常 `70`；测试中模拟进程崩溃退出 `71`。本轮发材错误是 T exit 70/注入崩溃 71；未把它们改成算法 abort。

覆盖故障：预存 sidecar/shell/ready 路径；同 session 对相同路径重试；第二方 sidecar/shell 发布冲突；shell/sidecar chown、bundle/sidecar/ready 目录 fsync 失败；sidecar/shell/ready 各阶段 crash；marker 缺失、截断、篡改；普通失败回滚；durable marker 后 T crash 与 launcher 阻止启动；恢复后 fresh path 同 session 新 KeyGen 重试。扩展 transport CTest 包含帧上限以下/等于/以上 payload、跨块顺序、duplicate/gap/truncation/wrong phase 和现有 EOF/silent 行为。

## 3. 构建和实际验证

**环境：** Windows 11 主机；WSL2 Ubuntu 24.04 / Linux 6.6.87.2；GCC 13.3.0；CMake 3.28.3；Release；OpenSSL 系统版；DCF PRG counters OFF；EMP OT OFF；BUILD_TESTING ON。Build 目录 `/tmp/bmw16-s19-build`，位于仓库外。E2E 测试控制器以 root 仅为夹具创建两个独立 UID 及文件权限；T/P0/P1 是 fork/exec 的独立进程，双方 UID 分别 22012/22013。root harness 不作为 party view，也不证明生产凭据配置。

**构建目标与 CTest：**

```text
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/bmw16-s19-build --target moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_test moe_topk_m2_transport_conformance_test -j 4
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /tmp/bmw16-s19-build -R moe_topk_bmw16_experimental_party_test --output-on-failure
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /tmp/bmw16-s19-build -R moe_topk_m2_transport_conformance_test --output-on-failure
```

以上四目标 build 成功，两个定向 CTest 各 1/1 通过。未运行全量 CTest。

**冻结 S4/C++ common-tape：** S4 文件来自独立 S4 worktree，SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。同一 current `moe_topk_bmw16_experimental_party_test` 二进制和原 S12/S15 runner 完成：

```text
python C:\Users\28641\.codex\artifacts\BMW16_S19_20261007\run_matrix_with_system_wsl.py experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --cpp-test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_test --s4 C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\select4r.py --out C:\Users\28641\.codex\artifacts\BMW16_S19_20261007\common_tape_s12_final --wsl-distro Ubuntu-24.04
python C:\Users\28641\.codex\artifacts\BMW16_S19_20261007\run_matrix_with_system_wsl.py experiments/TEST_ONLY_BMW16_S15/run_n256_common_tape.py --cpp-test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_test --s4 C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\select4r.py --out C:\Users\28641\.codex\artifacts\BMW16_S19_20261007\common_tape_n256_final --wsl-distro Ubuntu-24.04
```

结果分别 39/39（n≤8，各 K）和 2/2（n=256,K=2/8）通过逐轮共同随机 tape 比较；本轮矩阵 hash 见证据索引。

**独立进程 oracle E2E：**

```text
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc '/tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node --extended > /mnt/c/Users/28641/.codex/artifacts/BMW16_S19_20261007/node_e2e_extended_final2.log 2>&1'
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc '/tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node --n1000 > /mnt/c/Users/28641/.codex/artifacts/BMW16_S19_20261007/node_e2e_n1000_final2.log 2>&1'
```

`--extended` 当前源：12 个 SUCCESS_ORACLE_CHECKED（n=1/2/5/8、n=64 的 K=1/8/n、n=128 的 K=8/80、n=256 的 K=1/2/n），3 个 ready marker 负例双方 exit 20 且无 mask；24 个 publication failpoint/conflict/crash 例、6 个显式重试例。自然算法 abort 观测数 0；ready/error 与材料故障不计入自然失败。`--n1000` 复跑同一基础矩阵与故障夹具，再运行 n=1000/K=80 fresh-material T/P0/P1，双方 exit 0；TEST_ONLY 控制器检查原序 mask 长度 1000、二值、weight 80、逐位等于冻结 C++ oracle。明文 mask 只存在于 harness。

n=1000 本轮观测：T 报告 4,497,550 总 slot/party，其中 Select/membership sidecar 4,495,500 slot；每方 sidecar 恰 `4,967,527,664` bytes，shell `2,013,901` bytes。T phase `97,632,918 μs`，reported peak RSS `22,528 KiB`；P0/P1 sidecar validation 分别 `90,680,081/90,889,882 μs`，reported party peak RSS `40,316/40,380 KiB`。party 逻辑比较各 `319,461`、uCMP Eval 各 `117,859`、DCF Eval 各 `235,718`。这些是一次本机功能运行的记录，不是比较矩阵；全路径总时间、完整网络通信、全库 PRG calls、正式环境 peak RSS 均 `NOT_MEASURED`。party status 行的少量 stdout 字段因两个进程共享日志而交错；原始逐次日志保留，case summary 独立校验两方状态。

## 4. 抽样、SUCCESS 与失败概率

成功分支沿用 S16 条件 soundness：两个 Partition 均通过集合、rank、bracket/window guards 时，哨兵接受/拒绝交集唯一选中项目稳定键的第 K 项；membership `R_i ≥ R_selected` 与同分原始 index 使真实项恰 K 个；inverse 使用前向同一个隐藏置换，XOR share 重构等于冻结 oracle。bad invariant/material error/communication error 不得降格为允许的自然 abort。S19 E2E 验证该代码路径的有限输入，不是对全输入空间证明。

S16 实际参数的理想独立均匀无放回界保留原适用条件：每个 Select task 的 bracket、U-cap、W-cap 失败预算 `p_b,p_U,p_W`，两 task 用 union bound `P_abort≤2(p_b+p_U+p_W)`，`M=2·max(K,n−K+1)`。该源码的正式随机源通过两方 OS `getrandom` 贡献 XOR，再对 task/domain/counter 用 SHA-256 counter PRF 并 rejection-sampled Fisher–Yates；共同随机 tape 只在 TEST_ONLY。于是 S16 的数值是理想 sampler 的统计 bound；还需对实际 OS entropy、SHA-256 计算 PRF 和整个采样用量给计算假设/优势，不能把它们直接当统计概率，也不能用零自然 abort 实测代替。当前 E2E 无自然 abort样本，不报告一个实现级失败率。

## 5. 实际 party-view 安全裁决

目标 functionality 的 SUCCESS 输出是各方本地 mask share，异或后精确等于稳定 Top-K；算法自然失败一致 abort、无 share。待裁决的 leakage 草案包含公共 n/K/session/宽度、`public_z`、匿名 graph/comparison bits、pivot/U/V/W、selected anonymous handle、access/traffic、abort 类别；不能把待议泄露写成已授权。腐化 P0/P1 的完整 view 包含本方 raw share、Protocol I shuffle/inverse 的局部 `sigma/tau/a/e/r_share` 与消息、九个 party-only key pools 的 used+unused 槽、相关阈值、打开 operand/bit、自适应边和 key access、membership 和本方最终 mask share。T 在线静默；本地 root harness 不计入 party view。

第一个未证明 hybrid 是 H1→H2：针对 `keyGenDCF/evalDCF` 的整个相关 key pool，party 对随机源替换后仍持有所有未使用 key；已开 comparison bit 确定下一轮查询，且本方 Eval share 与 opened bit 必须联合模拟。若把单 key key privacy 用作逐键 hybrid，归约仍须支持 correlated threshold vector/本方 shuffle auxiliary input、未替换其他 pool key、输出条件与 adaptive 后续 Eval。仓库 FSS API/代码未给该安全游戏或定理，因此不能写 `N×single-key advantage` 就宣称完成。H2→H3 还没有 Protocol I 的实际 shuffle state/messages/public_z/inverse output shares 的联合 simulator。S19 没发现超出拟泄露的具体二世界区分器；也没有证明模拟存在。候选 leakage 没获得 owner 批准，故安全门仍 NO-GO，而非发现攻击结论。

独立 source map、混合前提、未决的 keygen `Q_tid`、发材边界和 ONE_SHOT root 适用范围见[S19 决策](../decisions/BMW16_S19_PUBLICATION_RETRY_AND_PARTY_VIEW_DECISION_2026-10-07.md)。

## 6. 计量、限制与门禁

- `9·C(n,2)` 是每方 Select/membership sidecar 槽数量；logical comparison requests、唯一规范 key slots、uCMP party eval、DCF eval 不能互相替代。
- `DCF Eval = 2×uCMP party Eval` 在本次 n=1000 样本成立。Sampler PRF words 与 DCF AES calls 分字段；S19 没有完整跨全代码路径 PRG 调用计数。
- 全局单次 n=1000 sidecar 实际 byte count 与 shell byte count分别记录，不能将解析 lower bound写成实测。在线总时、每方完整收发、完整 PRG、生产峰值 RSS、V4 WAN/LAN 数据一律 `NOT_MEASURED`。
- 固定 protected local claim root 内的一次性领取适用；copy 到另一个空 store、管理员/snapshot rollback 防护未支持。
- T→party 仍是本机 root/UID drop fixture；生产 party 凭据建立、认证私有远程通道、跨主机 package identity provisioning 未实现。下一步最小接口是：预置 party-specific credential，加私有认证传输；AAD 绑定 session/party/n/K/stage/task/round/endpoint/width/slot；在固定不可回滚 claim root 中一次性 burn；启动协调器须等待 T exit 0。

| 门禁 | 判定 |
|---|---|
| `FUNCTIONAL` | `PASS_FOR_REBUILT_TESTED_DOMAIN` |
| `PUBLICATION/RETRY` | `PASS_LOCAL_NO_CLOBBER_WITH_EXPLICIT_T_EXIT_GATE` |
| `ONE_SHOT_SCOPE` | `PASS_FIXED_PROTECTED_NONROLLBACK_LOCAL_ROOT_ONLY` |
| `JOINT_PARTY_VIEW` | `NO-GO`：首缺 H1→H2 correlated-threshold full-pool adaptive multi-key DCF simulator；H2→H3 shuffle/inverse simulator 仍缺 |
| `DELIVERY` | `CONDITIONAL_LOCAL_UID/AES-GCM`，生产交付未完成 |
| `PERFORMANCE_READY` | `NO-GO/NOT_RUN`：不运行正式 LAN/WAN 矩阵、不排名 |

## 7. 复现与工件

完整构建命令、二进制 SHA-256、本轮源码 SHA-256、common-tape matrix SHA、E2E raw log SHA 和非通过调用索引见[外部证据索引](evidence/BMW16_S19_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。最终提交前运行 `git diff --check` 并确认禁止目录没有进入差异。
