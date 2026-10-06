# BMW16 S10 独立接收、正式输入接线与执行隔离验证报告

- 日期：2026-10-06
- 实现标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL_TEST_ONLY_BMW16_S10_ISOLATED_FORMAL_ADAPTER`
- 源码基线：S9 `a4402b1ef67d359a0d85997a86175497516ab20d`
- 结论：**真正运行了 TEST_ONLY Select+mask 组合。正式 raw-score adapter、两方分进程、T 离线和本地 UID 文件 ACL 功能验证通过；完整安全门 NO-GO，不存在 secure API。**

## 1. 结论和范围

S10 在 S9 原有三进程 `EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS` runner 上接入了 opt-in VFSS adapter。输入是两方持有的 signed Q20.12 32-bit raw-score 加法份额；每个 party 的 C++ worker 直接运行仓库正式 `protocol_i_raw_score_input_party`，再将正式 priority-key shares 转成 S4 Select 的反向稳定键。T 在 party 输入生成前离线运行并退出；P0/P1 用不同 Linux UID 运行原有 shuffle、uCMP/DCF、四层 Select、membership、逆路由。仅外部 TEST_ONLY runner 重构输出并调用仓库冻结 C++ oracle。

**功能验证证据**：S10 固定矩阵 11/11 `n≤8` 成功且 11/11 oracle PASS；n=64 非全量抽样样本成功且 oracle PASS；强制概率 abort 和双向材料 abort 均无 mask；正式 raw-score conformance CTest 1/1 通过。自然算法概率失败在本轮这些固定样本中观测为 0；这不是概率界，也不继承 BMW16 Theorem 8。

**安全结论**：不同 UID 和目录 ACL 修复了 S9 的同 UID 文件读取反例在本地实验环境中的执行隔离部分。文件交付仍由 root TEST_ONLY harness 本地复制，没有 T→P AEAD/来源认证；进程参数对异 UID 可见路径/PID；线上公开比较位、自适应图和 selected anonymous handle；correlated multi-key DCF/uCMP 全池与 shuffle 状态、访问历史和输出 share 的联合模拟仍缺。因此该结果不得标作 secure Protocol I、不得进六方案正式性能表、不得声称完整入口四轮。

V3 未完成时推进这项隔离开发，是用户在 S10 明确授权的候选研发范围；V3、V4、LAN/WAN 正式矩阵与 M7 六方案验收仍未完成。

## 2. 接收基线、身份与来源

| 对象 | 确认身份 |
|---|---|
| main / origin/main | `c3926c68fd14f270faa8b55234311071947fa080` |
| 本 worktree | `codex/m6b-i-bmw16-s10`，起始 HEAD 为 S9 `a4402b1ef67d359a0d85997a86175497516ab20d`；开始时干净；当前变更只在隔离 checkout |
| S4 Select 源 | S4 commit `04ed6f8a352ab2277434344094471dff82f83e4b`，原文件 SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`；沿用 S9/S8 已记录共享源副本，不改算法规则 |
| S9 source / binary | 冻结 S9 清单 `docs/reproduction/evidence/BMW16_S9_SHA256SUMS.txt` SHA-256 `f89c980ebab33bc56e4df822d797695cb2fea3f12963487badd45108940d43d2`，其中列出 Select、runner、raw carry/sign 移植、材料及 official conformance 的各文件身份；S9 backend SHA-256 `0989efa4b2458267f330237ef0027a87812356473b028de64c2c6a14ca3bc3cf` |
| E20 | `8986a40175f5f8eef56d765fef0cfa863e205985`，干净；仅阅读离线发材/存储/一次性消费思路，没有移植 AAV86 runtime 或继承其安全结论 |
| E21 | `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`，原有 docs/evidence/scripts 未提交差异保留，未修改 |
| PR #28 | Draft 分支 head 为 `57e239ee820a4208ac606663d877803f0b692206`；未改 PR |
| 桌面 main worktree | 保留原有未提交差异、paper 和证据；未在其中编辑 |
| 论文 | BMW16 PDF 44 页、SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；辅助 `Papers/017.pdf` 249 页、SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15`，仅交叉核对。两者均不在提交差异 |

主要算法来源、S4 推导、S8/S9 view 限制仍按各自历史报告区分，不将 S10 代码行为反写成论文步骤。Protocol I 组合和稳定 Top-K mask 是项目扩展，candidate 仍明确标 `PROJECT_DERIVED`。

### S9 独立复跑

在更改 S10 源码之前，以 S9 原 source、runner 和冻结 binary 再跑 S9 raw-score matrix 与 n64 输入：

| 运行 | 结果 | 文件身份 |
|---|---|---|
| S9 raw-score 11-case matrix | 11/11 SUCCESS、11/11 frozen C++ oracle PASS | `%TEMP%/BMW16_S10_S9_RAW_MATRIX.json`，315,234 bytes，SHA-256 `368d23fde67f1747faaa3789895dca5300df99df3b6d927fde62f94cebb17351` |
| S9 n64 | SUCCESS、oracle PASS | `%TEMP%/BMW16_S10_S9_N64.json`，1,261,673 bytes，SHA-256 `7e903d6e572f5e8a45f8694f7e02c3f6f7556714ab4f5fd1d16d1c1c34848e02` |

这组历史路径用的是 S9 实验 backend 的 carry/sign lift 移植，不是 S10 新增 official adapter 的证据。S10 摘要同时保存其 case/status 以及 raw 输出哈希，以区分复跑和后续新实现。

## 3. 改动的接口与键方向

新增默认关闭的 CMake 选项 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER`。开关关闭时不编译新源；新头文件也会拒绝未 opt-in include。唯一新增 VFSS 函数名含 `experimental`，它不是 Top-K secure API。

输入 adapter 的稳定键映射为：设 `q=raw xor 0x80000000`、`b=max(1,ceil(log2 n))`，正式 Protocol I key 为

```text
P_i = (UINT32_MAX - q_i) * 2^b + original_index_i
```

S4 Select 消费反向顺序，adapter 使用公开上界 `C=(UINT32_MAX)*2^b+(n-1)`：

```text
S_i = C - P_i = q_i * 2^b + (n - 1 - original_index_i)
```

P0/P1 分别在 `Z/(2^(33+b))` 中做常数分享相减。所有真实 `S_i` 位于 `[0, 2^(32+b)-1]`，故落在更宽一位 ring 中；合法 signed 极值、同分 tie-break 和原始 `original_index` 在这段变换中保持严格全序。n=64 使用 `b=6`、39 位 comparator。正式转换函数负责 carry/sign lift；TEST_ONLY runner 只生成标准 `Z/(2^32)` shares，不手工生成 Z/(2^34) 的 Select 输入。

worker 的两阶段 score adapter 需要对同一 full-duplex socket 复制两个独立 fd：正式 `ProtocolIFramedChannel` 析构会关闭其接管的 fd。S10 给 carry/sign 各自 `dup()`，保留 worker 的长期 peer fd；此连接修正经 n5/n64 composed 路径验证。

S9 一般 `ProtocolIPartyPackage` serializer 强制要求 shuffle 全池材料；S10 worker 仍使用 S9 custom TEST_ONLY package，在 C++ 内构造只含正式 score-input 方法所需 carry/sign key 的狭窄 `ProtocolIPartyPackage` 内存视图，**不会把不完整对象序列化成一般正式 package**。这是 adapter 能调用正式函数的测试桥，不是 package ABI 接收。

## 4. 独立身份与双世界访问测试

实跑环境：Windows 11 + WSL Ubuntu 24.04，Linux Python 3.12.3；T=`moeaudit` UID 1000，P0=`nobody` UID 65534，P1=`www-data` UID 33。实验 `/tmp` 位于 WSL ext4。因为 `sudo -n` 当前需要密码，外层 TEST_ONLY runner 以 WSL root 编排，用 `runuser` 分别启动三个 UID；root 编排器是额外的测试信任点，不能冒充协议实体。

- T 的 keygen 进程只拿公开配置和 T 输出路径；T 退出后 runner 才生成 input shares。T 不能读取 controller-stage 或 party input 文件。
- party 目录各自 `0700`、由对应 party UID 拥有；result 文件通过 party `umask(077)` 创建。P0/P1 的对方 input、offline package、result 三类 `test -r` 均拒绝；无法 list 对方 party 目录；无法读 T package stage。
- `/proc/<peer>/cmdline` 在本系统可读。可见的是对方本人的脚本、本方输入/package 路径、PID/公开 seed 等参数；没有 score/share word 或 key bytes。对端自身路径/PID 可见是已记录泄露，当前程序也不能据此声称 process metadata 私有。
- 双世界夹具保持 P0 输入与理想 P0 输出 share 的 SHA-256 不变；P1 的输入和 package 内容变化。两世界 P0 都不能读取 P1 share/package、不能列目录、不能读 T package；T 不能读 P0 input。结果 SHA-256 和可读性逐世界见 [双世界证据](evidence/BMW16_S10_UID_TWO_WORLD_OS_ACCESS.json)。
- T→P 文件分发是在 root TEST_ONLY runner 下 `install` 本地复制，没有 AEAD、T 签名、网络端点认证或 key 端点关系认证。通过 UID 测试不能充当私有认证通道的证明。

## 5. S10 运行与成本证据

S10 正式适配小矩阵使用 frozen 向量、CSPRNG 采样币和新材料。对 n≥2，双方各运行独立 party Python 进程/native C++ worker；n=1 使用现存 singleton shortcut。所有成功 mask 仅由外部 TEST_ONLY oracle verifier 重构。

| 配置 | 数量 | SUCCESS | oracle PASS | 自然随机失败 | 观察限制 |
|---|---:|---:|---:|---:|---|
| Frozen boundary/tie matrix（n=1,2,3,5,8） | 11 | 11 | 11 | 0 | 边界包括 INT32_MIN/MAX、重复、全相等、K 两端、中间 K、非二次幂 |
| Fixed n64 sample，K=2 | 1 | 1 | 1 | 0 | 真实四层 Select 的非全量抽样分支；最终记录 case id `S10-n64-seed20261006-final` |
| Forced algorithm abort | 1 fixture | 双方一致 abort | NOT_RUN | 注入，不是自然失败 | P0/P1 code=2、无 mask |

n64 当前 release binary 本次 trace 的实际计数：

| 指标 | 实测/观察 |
|---|---:|
| comparator 宽度 | 39 bits |
| R1/R2/R3/R4 逻辑比较调用 | 8010 / 500 / 10549 / 6，总计 19065 |
| R1/R2/R3/R4 真—真边 | 1901 / 126 / 2574 / 6 |
| R1/R2/R3/R4 本地 dummy 相关逻辑比较 | 6109 / 374 / 7975 / 0 |
| Select 消耗的规范 uCMP 槽/party | 4607 |
| membership 消耗槽/party | 63 = n−1 个非自比较 |
| raw-score adapter 槽/party | 128 = 2×padded_n；双边 DCF Eval 合计 512 |
| Select+membership 双边 DCF Eval | 18680 = 4×(4607+63) |
| 含 raw-score 输入转换的双边 DCF Eval | 19192 |
| 离线全池序列化槽/party | 22304 = 16128 Select + 2016 membership + 4032 未使用 shuffle CmpAgg + 128 raw-score |
| serialized package bytes/party | 22,723,604 |
| n64 逻辑在线消息相位 | 16：S9 基线 13 + raw-score 两阶段 2 + joint coin 1 |
| P0 peer payload | 发送 732,033 bytes；接收 732,028 bytes |
| PRG 调用 / T 离线时间 / 正式 online time / 网络时间 | `NOT_MEASURED` |

单次 loopback 的 elapsed 字段留在原始 TEST_ONLY JSON 内，但不纳入本报告的性能结论、V4 表或 LAN/WAN 结果。逻辑边、唯一 uCMP 槽、DCF Eval 和离线全池槽分开计量；没有借用 S9 的成本数值填本轮计数。

S10 forced abort 实测为 `ABORT_ALGORITHM_PROBABILITY`，两 party exit codes `[2,2]`、`no_mask=true`、oracle `NOT_RUN_ABORT`。n2 材料夹具实测错 party、错轮、错端点、截断、重复领取、重复槽均拒绝；领取标志写入后 kill 进程，再次领取被拒绝；双 party 材料 abort 同状态 `ABORT_MATERIAL` 且均无 mask。peer silent 只验证存活方本地 `ABORT_COMMUNICATION`，对端缺席，不能算共同 ACK。没有 active adversary、恶意随机币偏置或远端完整错误协议接收。

完整、脱敏的逐例结果索引是 [S10 evidence summary](evidence/BMW16_S10_RUN_SUMMARY.json)。完整 run JSON 留在 WSL `/tmp` 和 Windows `%TEMP%`，没有复制进仓库；summary 不含 raw shares、key bytes、party mask shares、opened full masks 或完整 stdout/stderr。

## 6. 精确泄露与下一阻塞项

公开匿名图的置换等变论证和 n≤5 有限枚举只覆盖 public transcript；不能覆盖下列单方联合视图：本地 raw shares、forward/inverse shuffle 局部状态与双方消息、同一真实 handle 在两个 Select 实例的对应、由共同 shuffle node-mask vector 产生的完整本方 DCF/uCMP key pool、已访问/未访问槽、自适应 edge/query、公开 masked values/比较位/selected handle、abort 和最终本地 output share。

故安全门仍要求一个明确的多 key hybrid/simulator：在当前 shuffle 实现及本候选公开字段合同下，对一个 semi-honest P0 或 P1 联合模拟上述完整 view，并覆盖由同一 node-mask 导出的相关阈值、跨 Select task/round/membership 的独立 keygen、按公开自适应图选取的访问槽和 inverse-mask shares。该引理缺失是密码学 party-view 阻塞；它不同于本地 UID ACL 已通过，也不同于算法在测试样本上正确。

独立的工程阻塞是受认证、完整、端点绑定的 T→P package 交付与持久一次性领取合同。目前 serializer 的外层 fingerprint/session/slot 校验没有认证 DCF key 内部阈值和端点数学关系。下一项可执行工作应先冻结 package ABI/AEAD/nonce 与端点绑定并增加 tamper/replay/crash 跨会话 conformance，同时对上述 joint view hybrid 给出可审查证明；在这两个阻塞关闭前不创建 secure API、不启动 V4。

## 7. 复跑命令与文件身份

WSL Ubuntu 24.04 中使用 `cmake 3.28.3`、`g++ 13.3.0`、Python 3.12.3；NumPy 2.5.3 wheel 临时解到 `/tmp/m6b_i_bmw16_s10_pydeps`，不进入 Git。

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS -B /tmp/m6b_i_bmw16_s10_build -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=OFF -DBUILD_TESTING=ON
wsl.exe -d Ubuntu-24.04 -u root -- cmake --build /tmp/m6b_i_bmw16_s10_build --target bmw16_s10_experimental_backend moe_topk_m2_score_input_conformance_test -j2
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/m6b_i_bmw16_s10_build/vfss --output-on-failure -R '^moe_topk_m2_score_input_conformance_test$'
wsl.exe -d Ubuntu-24.04 -u root -- env PYTHONPATH=/tmp/m6b_i_bmw16_s10_pydeps python3 /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend /tmp/m6b_i_bmw16_s10_build/bmw16_s10_experimental_backend --matrix --input-seed 20261006 --algorithm-seed 1000 --formal-raw-score-abi --joint-csprng-coins --isolated-t-user moeaudit --isolated-party-users nobody www-data --output /tmp/BMW16_S10_FORMAL_ISOLATED_MATRIX_RELEASE_FINAL.json
wsl.exe -d Ubuntu-24.04 -u root -- env PYTHONPATH=/tmp/m6b_i_bmw16_s10_pydeps python3 /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend /tmp/m6b_i_bmw16_s10_build/bmw16_s10_experimental_backend --scores-json /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/n64_seed20261006.json --k 2 --case-id S10-n64-seed20261006-final --input-seed 20261006 --algorithm-seed 1007 --formal-raw-score-abi --joint-csprng-coins --isolated-t-user moeaudit --isolated-party-users nobody www-data --output /tmp/BMW16_S10_FORMAL_ISOLATED_N64_RELEASE_FINAL.json
wsl.exe -d Ubuntu-24.04 -u root -- env PYTHONPATH=/tmp/m6b_i_bmw16_s10_pydeps python3 /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend /tmp/m6b_i_bmw16_s10_build/bmw16_s10_experimental_backend --scores-json /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/n64_seed20261006.json --k 2 --case-id S10-forced-abort-final --input-seed 20261006 --algorithm-seed 1007 --formal-raw-score-abi --joint-csprng-coins --isolated-t-user moeaudit --isolated-party-users nobody www-data --test-only-force-algorithm-abort --output /tmp/BMW16_S10_FORCED_ABORT_RELEASE_FINAL.json
wsl.exe -d Ubuntu-24.04 -u root -- python3 /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_s10_uid_two_worlds.py --output /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s10/MoE_Top-k_Reproduction_Evaluation/docs/reproduction/evidence/BMW16_S10_UID_TWO_WORLD_OS_ACCESS.json
```

最终二进制 SHA-256：`301b8a8b06b778fb4667e8b8e437faf419dc409879b04d6bbde94be289187ebc`（367,992 bytes）。最终 matrix、n64、强制 abort 原始记录的 SHA-256 分别为 `4ef2ee4a7731e59f07b43eefd6651d8286965692898e3f55d79dc286226e99c0`、`6b1683f5e92d09b1ce5ce92803b33ff2205f7b2fc0b9be4a0a6b30c9ff571b6d`、`a58f2926aa35c3b4c14dced51bb640b245cdd4a7282b13ca591d561946b5a610`。详细 S9/S10 raw result、摘要和工作树源码哈希见 [S10 SHA256 清单](evidence/BMW16_S10_SHA256SUMS.txt)。

## 8. 文件变更

- 生产树只增加默认关闭的 `VFSS/include/moe_topk/experimental_bmw16_select_adapter.h`、`VFSS/src/moe_topk/experimental_bmw16_select_adapter.cpp` 以及开关/源文件项；未增加 secure API，也未触碰旧 Protocol I/AAV86 runtime。
- 更新隔离实验 backend、party runner 和进程/材料访问测试，增加 S10 UID 双世界测试和脱敏 evidence summarizer。
- 新增本决策、独立验证报告、运行 summary、UID 双世界 summary 和 SHA256 manifest。
- 同步 `PROJECT.md`、实施计划与 benchmark validation plan：记载提前研发授权，不把 V3/V4 正式顺序改为完成。

未修改 `VFSS-baseline/`、论文、key/build/log 文件、原始参考树、E20/E21、main worktree 或 PR #28。
