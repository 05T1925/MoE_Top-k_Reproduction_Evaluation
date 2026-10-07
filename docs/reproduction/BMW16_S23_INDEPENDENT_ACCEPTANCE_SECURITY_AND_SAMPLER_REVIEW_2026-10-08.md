# BMW16 S23：S22 独立接收、整池视图与抽样界复核

日期：2026-10-08
方案：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
被测源码 revision：`9210c11a7a877f8fe3687e0509c1722440cf63fe`（S22）
本轮变更：仅文档；没有修改 VFSS、Select、DCF、shuffle、材料或 party runtime 源码。

## 1. 接收范围和基线

本次从 S22 精确提交新建隔离 worktree `codex/m6b-i-bmw16-s23`。初始 HEAD 与 branch 为 `9210c11a7a877f8fe3687e0509c1722440cf63fe` / `codex/m6b-i-bmw16-s23`，初始 worktree clean。`main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`，与 S22 的 merge-base 也是该 revision；因此 S22 是尚未合入 main 的实验分支。本轮未 cherry-pick 其他实验提交。桌面主工作区原有未提交差异被保留，没有触碰 S22 原 worktree、E20/E21、PR #28、`VFSS-baseline/`、论文和本地参考树。

读取并核对：`AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、S16/S21/S22 决策与报告、Protocol I DCF/uCMP/shuffle、BMW16 party/stream store，以及 `protocol-reproduction` skill。项目正式 M6A→M6B/V3→V4 验收顺序没有改变；本轮只是授权范围内的提前 `EXPERIMENTAL` 复核。正式 LAN/WAN/九指标性能矩阵和六方案排名仍为 `NOT_RUN`。

### 被测源码身份

下表为 S22 tree 中本轮复核所用的 SHA-256；源码没有在本轮修改。

| 文件 | SHA-256 |
|---|---|
| `VFSS/ext/FSS/dcf.cpp` | `0EEB1926380726EFC5BE84B4B49E0ACB8C9636D0D6181469EDF505C1921B5DAC` |
| `VFSS/ext/FSS/include/FSS/dcf.h` | `80904E0518BD75772D97984F2D48EEAE7CBD805FD119156AB7B9097082AB3F47` |
| `VFSS/ext/FSS/include/FSS/keypack.h` | `26597698719B62A6ADECD54194E4A36DB39DB78FEB7F3F0AE30AF13252E991BB` |
| `VFSS/src/moe_topk/protocol_i_ucmp.cpp` | `B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23` |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `A5F61E04F0BE8A6722A579F918BCE38C94069FD231B235916F1D4AB0A61301CE` |
| `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` | `E2FCA7497426ACCBFB4B2DE44044C2721C64C8EEA32686F1A3A1E9C62E058F1D` |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `BA14008D28964E49965C75E2675E8726193C50991247BDA668F3398A01144BDE` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | `665C8072BBC3A3FC19CB500720F6AD6214C8942B581998A24F4C8C05F993EF27` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `84BEE8DFF57F8D7B647CF5F07CFE5EED32CEF97A7DB29004CC1726F5FFA98CCA` |

S22 历史证据不是本轮重跑：其外部 `SHA256SUMS.txt` 索引 SHA-256 为 `55EDA5EB29EEC79800F2216EF0CAF047A21C0D9E1420C1F8B9DC2BB07499BE41`（历史索引身份保留；该 digest 不用于替代本轮结果）。S22 历史报告记录的 party-node 二进制为 `b41673548359aea4e5ee4072e4aa4eb87cc85ae577452356eb2aed0e0c5c553d`，39 条 common-tape matrix 为 `27df4b2299b7052a92d686f643ff4137b6b944761e51db4e770085d7507c39e6`，CTest 日志为 `e41328c4f963b177d78e3e2b7cfbac5f69859d9824007bc87b36108f32cf7d89`，n=1000/K=80 历史 E2E 日志为 `a41dae081c35e9047b1efa3f8934fbd6f089a66e3692faf416bca0931afabda8`。这些属于 S22 报告，不计入 S23 实测。

## 2. 论文定义、源码行为和项目决定

### BGI15 到 VFSS DCF 的代码映射

BGI15《Function Secret Sharing》Definition 2（PDF p.7）给出完整单方 key 的 function-privacy 游戏；比较函数族与 Algorithm 5/6 位于 PDF pp.17–19，Theorem 6 位于 PDF pp.19–20。论文定义的比较对象为 `f^<_{a,g}(x)=g`（`x<a`）否则 0，输出为加法组中的函数份额。原文证明细节有部分留给 full version。本报告将其作为可审查的目标定义，不从功能通过反推论文等价。来源：[BGI15 原文](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf)。

仓库对应位置：`dcf.cpp:152–284` 为 `keyGenDCF`，`dcf.cpp:294–335` 为 `evalDCF`，`dcf.cpp:87–120` 为 tree traversal；uCMP 包装在 `protocol_i_ucmp.cpp:5–15`。`ProtocolIUcmpMaterial` 以 `alpha=(ml-mr) mod 2^b` 调用 `keyGenDCF(b,64,alpha,1)`。每个 slot 是一次 DCF KeyGen 产生的一对 party keys；同一方对该 key 在 `x=(zl-zr) mod 2^b` 和 `y=x-2^(b-1) mod 2^b` 上 Eval 两次，然后按 `Z/(2^64)` 规则组合。不是每个 uCMP slot 存两份独立 DCF function key。

完整 serialized party key 包含 header、`Bin/Bout/groupSize`、数组长度、`k[Bin+1]`、`g[groupSize]`、`v[Bin·groupSize]`，不只是 root seed。对当前 `Bout=64, groupSize=1` 序列化 ABI，`deserialize()` 约束 `k=b+1,g=1,v=b`，长度精确为 `9+24+16(b+1)+8+8b = 57+24b` bytes（b=34..53 时 873..1329 bytes）。函数方向、两个 Eval 点和 P0/P1 符号有源码与 conformance 证据；这些只支持功能对应。

实际 Gen 从 `FSSConfig::prngs[omp_get_thread_num()]` 取两个 128-bit root blocks；每层对两条 seed 路径调用 `ecbEncFourBlocks` 并生成 correction/state 数组；Eval 每层调用 `ecbEncTwoBlocks`。本轮没有建立 `dcf.cpp` 所用 root/control 编码、每层输出位、correction words、`g/v` 和 party 符号到 BGI15 Algorithm 5/6 的逐变量等式。因此不能将 Theorem 6 的函数隐私结论直接归给现有二进制。

精确缺口命名为 `DCFCodeEquivalentToBGI15GenEval(b, Bout=64, groupSize=1)`：需要给出序列化 key 每字段到 BGI 定义完整 party key 的映射，说明 AES expansion 与论文 PRG 假设的关系及 `cryptoTools::PRNG` 的调用/随机性替换优势，再说明 Theorem 6 的域、范围、输出组对 `b=34..53` 的适用性。我们没有发现功能反例；“没有反例”不填补该安全引理。

### 全池公式与随机带

令 `C=binom(n,2)`、`p=padded_n`。本轮检查 `experimental_bmw16_select_party.cpp:816–946`、`experimental_bmw16_stream_store.cpp:230,402,509`：

- DCF KeyGen pair 调用数为 `N_DCF=2p+9C`：raw carry/sign 的 `2p`，Select 两 task×四层及 membership 的 `9C`。
- 流式 Select/membership sidecar 只有 `9C` 个 pair-key slots，`expected_slot_count()` 对应此数；base `2p+2` 的计数把 raw 与 forward/inverse shuffle stage material 计入离线材料总槽，不能称为 DCF KeyGen 或 Select sidecar slots。
- BMW16 shuffle 配置 `include_cmpagg_material=false`；不能把普通 Protocol I 的 CmpAgg 边材料并入本方案 Select 侧车数量。
- 两 task 的真实句柄一致；不同 task/round 使用分开的 slot/key ID，同 slot 的两次 Eval 是同一 key 的设计行为；不得因复用 Eval 结果或反向 share 转换把逻辑比较调用删账。

`seed_fss_once()` 在 `protocol_i_parallel_shuffle.cpp:114–120` 从 OS `getrandom` 取随机种子初始化 256 个 PRNG stream，每个 stream seed 为 16 B。当前 T/offline KeyGen 串行使用调用线程 stream；每个 DCF KeyGen 消费 2 个 128-bit root blocks，即 32 B。因此 source-derived 为 `Q_root=2N_DCF` 个 block / `32N_DCF` bytes。Gen 每个 KeyGen、每层两个 seed 路径各做一次 4-block expansion：`2bN_DCF` 次 `ecbEncFourBlocks`、`8bN_DCF` 个 AES block 运算。每次 DCF Eval 为 `b` 次 2-block expansion；每个 uCMP Eval 对同一个 key 调两次 DCF，故 `Q_cmp` 次逻辑比较对应每方 `4bQ_cmp` 个 AES block 运算。这里的公式是源码布局推导，未启用低层 counter 的运行级 AES/PRG 实测字段一律 `NOT_MEASURED`。不报具体安全位数，优势保留为符号项 `Adv_PRG(Q_root)+Adv_AES(Q_AES)+N_DCF·ε_DCF`。

## 3. 候选泄露和完整单方视图

用户在本轮任务中转述的项目所有者/学姐答复批准以下作为候选公开泄露 `L`：公开 `n,K,session`；`public_z`/masked operands；匿名置换句柄上的比较端点/方向、边图、公开比较结果、pivot 位置、样本与 `U/V/W`；两个 Select task 的共同真实匿名 handle 与 selected anonymous handle；实际 key/slot ID 与顺序、比较次数、消息相位/长度；双方统一算法中止状态。许可只确定项目可接受的泄露集合，不证明其视图分布可模拟。原 score、original index 到匿名 handle 的映射、原序 rank、selected 明文键和完整明文 mask 不得公开。

P0/P1 的真实 view 还包括本方 raw share；Protocol I shuffle 的本地 `sigma/tau/a/e/r_share`、每轮帧与 `public_z`；本方完整的 raw、shuffle、Select、membership 已用/未用 key；本地所有 DCF Eval share；收到的 peer share；key slot 的读取/未读取与 durable-claim 状态；两 Select 共享的真实匿名 handle 关联；前后向 shuffle 的同一置换关系；各自最终原序 XOR mask share；正常 abort、工程/材料错误和断连状态及流量。`local_reason` 保持进程本地诊断，不等于两方共同确认的错误字段。

允许公开字段的来源依赖秘密输入和随机币，不能被写成“输入无关”。理想功能应明确允许泄露 `L` 和每个腐化方的自身输入/output share。两方输出 share 互异也属于各自理想输出；不能要求模拟器从明文 mask 重构对方 share。

## 4. 整池多 key 归约审查

**H0→H1（T 的随机带）：**假设 T 的独立 OS entropy 与本轮 S22 的 `getrandom` 成功。可将 PRNG 扩展流替换为相同长度的独立均匀带，代价是实际 PRG/AES distinguishing advantage；之后每个 key slot 的 root blocks 可视为独立。单线程顺序调用不会自动产生跨 key 随机带独立性，只有先做这一整流替换或证明具体多输出 PRG 性质才可把它写成独立 tape。`Q_root` 见上；随机生成线程/每方在线 PRG 运行总计数没有本轮实测，不杜撰 `Q` 位安全强度。

**H1→H2（相关整池）：**在已假设完整单 key function-privacy 的前提下，N-slot hybrid 并非因为 threshold 相关就自动失败。对固定两个世界，要求 corrupt 输入/output share 相同且实际泄露 `L` 相同；把 `L` 的匿名 endpoints、每个 slot 的公开 masked operands、打开 bit、后续自适应边/访问次序、selected handle、流量形状固定为 challenge-independent state。对第 j 个完整 party key，挑战候选为两世界的比较函数阈值；所有其余已用/未用 keys 作为同一 hybrid 混合分布的辅助 state。只允许候选函数对在所有该 key 已公开查询上给出同一 L bit；否则 L 已区分这两候选，不能作此步。

`evalDCF(x)` 与 `evalDCF(y)`、uCMP 本地组合、材料序列化和未来由旧 transcript 决定的边访问都是完整 challenge key 的确定性后处理。对于相同公开比较 bit `b`，party 的输出份额为 `s_c` 时线上收到的 peer 份额必须为 `b-s_c mod 2^64`；模拟器能精确计算该消息，而不必虚称 peer share 均匀。它只证明在同一固定 `L` 的比较类中输出消息可模拟；还要证明真实协议 view 可对 `L` 条件化、L 不由 KeyGen tape 决定、所有相关阈值/输入状态能用同一分布作 hybrid coupling。对两个 DCF Eval 点同一 key、两任务共享句柄、membership、未用 key和每层自适应选择均需纳入这个形式化游戏。

若单 key优势 `ε_DCF(b,64)` 与源随机流替换优势可分别成立，候选总界形如 `N_DCF·ε_DCF + Adv_PRG(Q_root)+Adv_AES(Q_AES)`（再加明确的 shuffle/其他原语项）。所有项维持符号量；当前代码尚不能引用 `ε_DCF`，故该式不是当前实现的安全界。BGI15 Definition 2/Theorem 6 本身不自动覆盖本仓库构造，S23 没有把“同一 L”事后作为 state 来循环假定，也没有把这个证明骨架评为通过。

## 5. Shuffle、输出和中止组合

实际 Protocol I dealer 源码 `protocol_i_parallel_shuffle.cpp:318–379` 给出 `tau0=pi∘sigma1^-1`、`tau1=pi∘sigma0^-1`、`e0=-tau0(a1)-h`、`e1=-tau1(a0)+h`。两轮 party 状态机对应相加后的 `public_z=pi(x0+x1)+r0+r1`。BMW16 party 在 `experimental_bmw16_select_party.cpp:1016–1115` 使用公开 `public_z` 与 shared anonymous handles；两个 task 复用真实 handle，membership 后走预发 inverse shuffle；成功路径只交付 `mask_records.shuffled_share[i].word0 & 1` 的本方份额，并在最终状态协商后报告成功。

代数/功能等式以及 roundtrip tests 不构成 joint-view simulator。当前精确未证引理为 `ProtocolIForwardInverseViewSimulator_L`：给定单方 raw input share、本方 ideal output share 和 `L`，共同模拟其 forward/inverse permutation factors、shuffle frames、`public_z`、同置换两 task handle、key-pool读取视图、membership local shares、selected handle、abort 与原序输出 share；证明逆路由 share 条件分布没有暴露原序映射，且与同一隐藏置换的 forward 辅助状态联合一致。没有找到满足“同一腐化方输入、同一 ideal output share、同一 L、不同诚实方输入”的可区分事件，但有限枚举/未发现反例不是证明。

状态合同区分成功、正常算法概率 abort、工程/不变量错误、材料错误和通信错误。只有成功完成最终 agreement 的双方可记 `PEER_AGREED`；无 ACK/peer close 时只记存活方 `LOCAL_ONLY`，且不得发布部分 mask。正常算法 abort 是协议状态，不代表完全不泄露阶段；比较轨迹可能暴露其来源。`local_reason`不作为对端状态字段。此合同的功能行为在 S22/S23 定向 E2E 有证据，不等于模拟证明。

## 6. 抽样概率口径

按冻结 S16 参数与取整，在理想独立均匀无放回抽样下，对每个 Partition：

- bracket abort 项：`s<M` 时 `2·e^-31`，否则 0；
- `U` 超 cap：当 `M>4096` 且未全量时至多 `M·e^(-8 M^(1/4))`，否则 0；
- `W` 超 cap：`M>64` 且 cap 未全量时至多 `M·e^-32`，否则 0；
- 两个 Select task 用 union bound，`P_abort ≤ 2(p_bracket+p_U+p_W)`，不要求任务独立。

`M=2·max(K,n-K+1)` 为可达偶数，`n=1` 是单独 shortcut；在 n≤256、任意 1≤K≤n 上，M≤512。独立脚本枚举全部 256 个可达偶数，最大上界 `1.3105804606611235e-11`（M=512）。在 M≤2,000,000 的**公式检查域**上最大值 `5.065679989546104e-8`（端点 M=2,000,000）；这不是当前 API 的支持规模，也不是运行证据或 BMW16 Theorem 8。

实现从两 party OS `getrandom` 各取 32-byte coin，经已认证连接交换并 XOR 得公开 seed；SHA-256 的输入由 seed/task/domain/counter 确定，`bounded()` 对有偏范围 rejection，再作 partial Fisher–Yates。公开 seed 不是秘密 PRF key。按随机预言机模型，若编码无歧义、输入域无重复、无预查询，哈希输出可作为独立随机 word；rejection 使范围采样精确均匀，不放回 shuffle 精确对应理想样本。应额外记 `Q_H/2^256` 的预查询项和跨 session seed 碰撞项（至多 `binom(q_session,2)/2^256` 的量级）；具体 SHA-256 到 RO 的优势 `Adv_SHA256→RO(Q_H)` 本项目未量化。因此 ideal abort 数值是 **ROM-conditional**，而非无条件的具体 SHA-256 实现失败概率。S23 没有用 0 次自然 abort 推导 ≤10^-6，也没有修改 sampler。`abort_bound_recheck.py` 对论文之外公式的枚举只能核验整数取整数值，不证明 S16 概率不等式本身。

## 7. 本轮独立构建、测试和原始证据

环境：WSL Ubuntu 24.04；GCC 13.3；CMake 3.28.3；OpenSSL 3.0.13；Release；`BUILD_TESTING=ON`、`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`、`MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON`。工作区构建目录和原始逐例日志均在仓库外：`C:\Users\28641\.codex\artifacts\BMW16_S23_20261008`。S4 冻结文件位于 `C:\Users\28641\.codex\artifacts\BMW16_S18_20261007\frozen_s4_source\select4r.py`，SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。

执行顺序与范围：

1. 从 S22 tree 新建 Release 测试构建并定向构建 BMW16 party test/node/E2E 与 Protocol I/III 相关目标。产物不是借来的 S22 binaries。Fresh hashes：party test `DECF0C4724B201C35E6C19A5525A887D5DBFC175C7FA09A9F62426250E5416A9`；party node `4AEDD0E1A6643499DF21328E99B669D4DBE4F1FA2F1F4BA132C323699292F416`；E2E harness `892080D4AE048D7A2B1276E97A483681EA6836EFB67F706815D9472C9AA34DF2`。
2. Conformance 与 Protocol I/III 相关定向 CTest 8/8 PASS：BMW16 party、M3 grank/DPF routing、M2 uCMP/transport/raw score/shuffle conformance 和 shuffle process E2E。日志 `s23_targeted_ctest.log`（1,345 bytes，SHA-256 `3973054BE8A8F18DC39BAC197975AF509DC47DA554D8681DE6C772EAC57517BA`）。另一个短摘要 `targeted_ctest.log` 是过渡日志，不作为主证据（SHA-256 `EE254E40F213AB7CD69357D68ADCAC41754FB6D99961051AA58599834CB20BC5`）。全仓注册 42 项；因未构建全部目标，Full CTest `NOT_RUN`，不得称全仓回归通过。
3. 使用 S4 Python 源与 C++ `--trace-case-tape` 的共同外部 tape 逐轨迹 differential：n=2..8 每个 K，另有 n=64 的 repeated-score、strict-order、INT32 extremes/repeats 配置，共 39/39 PASS。冻结 S4 hash 与上述相同。matrix JSON 28,437 bytes，SHA-256 `2E302A27969BD28FD2B667AC77E5A8FA3450A1EADC1AA17ED5CADA8D9F1EEC94`；文件中保留每例 input/tape/trace/diff 摘要及 seed。此处是 common-random-tape 差分，不把不同 PRNG 的相同 seed 当作同一随机轨迹。
4. 独立 T/P0/P1 E2E 使用 fresh material。非 TLS `--extended` 有 15 条 node 记录：n=1 shortcut 与 n=2/3/5/8/64/128/256 边界/中间配置 SUCCESS oracle check，另外 missing/truncated/tampered 材料拒绝；所有 SUCCESS 均由隔离测试控制器核对长度、二值、weight K 与冻结 oracle，失败无 mask。日志 20,789 bytes，SHA-256 `F59EACF0541881B94CFF67799B13FEB222A361CD799DBA2DA91D45027E109663`。
5. 不同 UID 的 TLS 独立进程 E2E：初次由普通 WSL 用户启动时，harness 在建协议前因需要 root 配置独立 UID 而退出 1；该环境前置失败单独保留，不能统计成协议失败。以 WSL root harness 重跑 `--tls` 后退出 0：包含 n=2/3/5/8/64/128/256 success、离线投递负例、算法 abort、工程错误、TLS fail-closed/identity、断连和最终状态分歧测试。日志 19,245 bytes，SHA-256 `14B9978C27DFDD9A0BEF5EC38EA33E314679A6C1E152AADF9B387733A727B96C`；初次失败日志 271 bytes，SHA-256 `3005BD72353B024ADE5E996881551A69A2ADE6691B8AAE529AC03F2EEF5B111C`。这是同主机 WSL 不同 UID + TCP/mTLS 的功能证据，不是远程部署证明。
6. S16 abort-bound checker 复跑；外部结果 JSON 1,840 bytes，SHA-256 `066A7E64FD96716922630AF36A3D9DEB8AB8F9199CAB59C365BF3E634D69705C`。JSON 明示 n≤1e6 行为公式检查而非实现范围，并将理想 sampler 数学 bound 与具体 SHA256/PRF 假设分开。

可复跑命令（WSL 路径以本机 worktree、外部 artifact 根替换；不包含任何凭据/材料）：

```powershell
wsl.exe -d Ubuntu-24.04 -- cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s23/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON
wsl.exe -d Ubuntu-24.04 -- cmake --build /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build --target moe_topk_bmw16_experimental_party_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_m3_grank_test moe_topk_m3_dpf_routing_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build --output-on-failure -R "moe_topk_bmw16_experimental_party_test|moe_topk_m3_grank_test|moe_topk_m3_dpf_routing_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_transport_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_parallel_shuffle_process_e2e_test"
python experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --cpp-test /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build/moe_topk_bmw16_experimental_party_test --s4 C:/Users/28641/.codex/artifacts/BMW16_S18_20261007/frozen_s4_source/select4r.py --out C:/Users/28641/.codex/artifacts/BMW16_S23_20261008/common_tape --wsl-distro Ubuntu-24.04
wsl.exe -d Ubuntu-24.04 -u root -- /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build/moe_topk_bmw16_experimental_party_node_e2e_test /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build/moe_topk_bmw16_experimental_party_node --extended
wsl.exe -d Ubuntu-24.04 -u root -- /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build/moe_topk_bmw16_experimental_party_node_e2e_test /mnt/c/Users/28641/.codex/artifacts/BMW16_S23_20261008/build/moe_topk_bmw16_experimental_party_node --tls
python experiments/TEST_ONLY_BMW16_S16/abort_bound_recheck.py
```

上述命令列为本轮复跑所用工具、目标与参数的可复跑形式；通过测试只证明当前 S22 被测源码的这些有限功能用例，不证明安全定理、渐近性能或失败概率。

本轮没有修改代码，因此未重跑 n=1000。S22 历史报告里 n=1000 单次功能结果仅作为历史证据，不计为 S23 E2E。未测阶段时间、正式通信、PRG/AES block 实际计数、远端部署和 LAN/WAN 九指标均为 `NOT_MEASURED/NOT_RUN`。所有逐例输出/输入 tape/trace、二进制与构建目录保留在仓库外上述 artifact 根；Git 只保留此报告与决策，不包含 shares、key、seed sidecar、证书、原始大日志或构建物。

## 8. S23 门禁

| 门禁 | 结论 | 可核验证据 / 缺口 |
|---|---|---|
| `DCF_IMPL_CORRESPONDENCE` | **NO-GO / UNPROVEN** | 函数接口、组和完整序列化 key 已对照；`DCFCodeEquivalentToBGI15GenEval` 与 PRG 优势映射未证明。 |
| `ADAPTIVE_FULL_POOL_VIEW` | **CONDITIONAL / NOT CLOSED** | 对固定同一 `L` 的逐槽 `N_DCF·ε_DCF` hybrid 有具体候选构造，相关阈值可放在 challenge-independent state；但尚无适用于真实 VFSS keygen 的 epsilon，且真实 `L` 条件分解/整池联合实验需独立形式化复核。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | **NO-GO / UNPROVEN** | 缺 `ProtocolIForwardInverseViewSimulator_L`，无反例也不构成通过。 |
| `SAMPLER_ABORT_BOUND` | **CONDITIONAL / ROM** | S16 理想无放回公式与当前整数规则复算通过；公开 seed 的 SHA-256 sampler 只在 RO 抽样假设下继承理想分布，具体 SHA-256→RO 优势未量化。 |
| `FUNCTIONAL_REGRESSION` | **PASS / BOUNDED** | 当前源码 fresh build；39/39 common tape、定向 CTest 8/8、不同 UID TLS 与 non-TLS n≤256 E2E 成功/故障路径通过。有限运行不是安全/概率证明。 |
| `SECURE_ALIAS_READY` | **NO-GO** | DCF→BGI 桥接和 shuffle/output 联合 view 未通过独立审查；继续无 secure alias。 |

## 9. 修改文件和后续最小工作

本地 S23 分支修改：

- `docs/decisions/BMW16_S23_INDEPENDENT_DCF_POOL_SHUFFLE_AND_ABORT_REVIEW_2026-10-08.md`：S23 安全、源码、概率决定；纠正 S22 对 Nε hybrid 的状态描述。
- `docs/reproduction/BMW16_S23_INDEPENDENT_ACCEPTANCE_SECURITY_AND_SAMPLER_REVIEW_2026-10-08.md`：本轮独立构建、哈希、复跑、公式、结果及门禁。
- `PROJECT.md`：候选表/当前摘要同步到 S23，不改变正式 M6A→M6B 验收顺序。
- `docs/IMPLEMENTATION_PLAN.md`：增补 S23 dated 实验注记，正式 V4 状态保留。

下一项最小可执行任务是对照 `dcf.cpp:152–335` 逐行给出 `DCFCodeEquivalentToBGI15GenEval`（包括完整 key 序列化形状、threshold/correction word/party sign、AES tree PRG 和两次 Eval 的方程），然后由未参与构造者复核。之后独立写出 `ProtocolIForwardInverseViewSimulator_L`。在两项过门之前不创建 secure alias，也不把 S23 本地功能数据放入正式六方案排名。
