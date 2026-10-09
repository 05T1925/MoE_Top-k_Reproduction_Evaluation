# BMW16 S29：条件安全配置与版本化入口决策

日期：2026-10-09  
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**  
决策对象：S26 独立接收的 VFSS C++ 运行时及 S27/S28 安全复核  
本次边界：接受一个明确的**条件安全候选配置**供实现与审查；不作无条件、标准模型或生产部署安全声明。

## 1. 决策摘要与来源身份

S26 三个提交对象保持区分：最终功能源码 `2818bce20f30719eaccf0bc5df586cce4fe78c84`，
运行时修复 `ded433d636a3df929380bdce0ff22f4ce6121453`，异会话功能接收报告
`32f7f0ba3f80083ff282c9b5ebd0a827bd366898`。S27 为安全证明文档提交
`8bbf7119470bbc37f6950a3b425255ed993eaa8d`；S28 的独立安全复核提交为
`27bee47de4356a87af6a1f6ff06be6c7e0df2c9b`，没有改动 DCF 或 party runtime。
S26/S27/S28 均未合入 `main`。当时 `main = origin/main = merge-base =
 c3926c68fd14f270faa8b55234311071947fa080`。

本决策采纳下述窄化模型，只用于版本化的 `conditional_secure_v1` 候选入口：

- 方案仍是 **PROJECT_DERIVED**：不称 BB90 原算法、不称 BMW16 Algorithm 7 逐字实现，
  不继承 BMW16 Theorem 8 的概率、比较常数或四轮端到端结论。
- `conditional_secure_v1` 是有显式假设边界的**安全候选接口**，不是部署认证或对外无条件
  `secure` API。接口默认不构建，且只有在下表假设范围内才使用该安全标签。
- `Protocol I + BB90+DCF` 仍是路线计划中的原论文目标。BMW16-derived 候选占其 Protocol I
  选择路线的实验比较位置，但不是来源等价替换，也不能记作 BB90 完成。
- Select 的四层比较因果依赖只是完整入口的一部分。

S28 是未参与 S27 证明写作的独立接收者。它复核了实际完整序列化 `M2UC v1` party key 的
source-specific 理想扩展归纳、逐槽相关全池 hybrid、opened comparison bit 与自适应访问，及
当前 forward/inverse shuffle-output-share 条件模拟。因此 S29 对应安全门的记录是“条件性证明
已独立接收”，不是“尚未找人复核”。S28 没有发现完整 key 区分攻击，也没有宣称这些条件自动
适用于其他 DCF ABI 或安全模型。

## 2. 威胁模型和候选泄露函数 L

### 2.1 角色与腐化

| 角色 | 候选模型 | 可见输入/材料 |
|---|---|---|
| T | 可信、非合谋，只离线参与 | 公开 session、n、K、宽度等配置；生成全部材料并在在线输入产生前退出。T 可保留离线状态。 |
| P0、P1 | 至多腐化一个、半诚实 | 各自 raw-score 算术份额、私有 party bundle、mTLS 私钥/配置、本地执行状态、所有协议帧及本方输出 share。 |
| 通道 | 抽象证明采用理想私有认证通道 | 同一 session、party、channel/phase 的完整性、机密性和 peer 身份；真实本地 mTLS 只是受测工程实现。 |

不覆盖恶意在线方、两方合谋、T 与任一方合谋、T 泄露/在线补发、管理员读取两个
party root、快照/VM 回滚或生产 PKI 运维。网络/材料错误按工程失败合同处理，不纳入正常协议
安全证明。

### 2.2 允许公开的 L

S29 采用的泄露函数面向每个在线 party，包含下列公开或可由帧长度/执行时序直接观察的字段：

| 字段 | 实际阶段/源码路径 | 可见方 | L 中的含义 |
|---|---|---|---|
| session、fingerprint、n、K、comparison/index bits、peer 与 phase | TLS 建链和 `ProtocolIFramedChannel` 帧头 | P0/P1 | 会话和通道配置。 |
| carry/sign masked operands 与 raw adapter 长度 | score 输入阶段，`protocol_i_score_input.cpp` 的 phase 4、5 | P0/P1 | 适配所需公开掩码量及固定/可推导的传输尺寸。 |
| forward `public_z` / masked records | forward shuffle 两次消息及返回记录 | P0/P1 | 置换后的掩码优先键，不含明文稳定键。 |
| 匿名比较端点、打开的比较 bit、每层边数/帧长 | Select `compare_round` 与 R1–R4 phase 21–24 | P0/P1 | 每轮适应性图和打开比较 transcript。 |
| sample、pivot、prefix/suffix、U/V/W、reject/accept 的匿名位置 | Select 两 task 的阶段交换/公共状态 | P0/P1 | 只按 shuffle handle 标识，不给真实 index 对应关系。 |
| selected anonymous handle | 两个 Partition 交集以及 membership 使用 | P0/P1 | 置换后唯一候选句柄。 |
| key/slot ID、task/round、访问次序、比较数 | 预发材料 manifest 和在线活跃槽读取 | 本方；公开端点/轮次可由对端关联 | 固定用途的匿名材料标识与访问迹。 |
| membership 与 inverse masked values/public_z | membership 后的 inverse shuffle 两轮 | P0/P1 | 掩码共享成员位的置换后表示及其帧长。 |
| sampler 两方贡献、XOR 后 seed、算法 abort/final status | sampling phase 19、状态 phase 40 | P0/P1 | 公共随机币和双方统一的 SUCCESS/算法 ABORT 状态；公开 transcript 仍可能推测失败阶段。 |
| 输出份额 | inverse 完成后仅输出给对应 party | 各自仅见本方 | 各自原序 XOR bit-mask share，是理想功能输出的一部分。 |

L **不包含**原始 score、original_index、原 index↔匿名 handle 对应、真实 rank、明文
selected stable key、完整明文 mask、对方的本地输出 share或 party 私有材料。任何日志或
诊断扩展不得暗中扩大 L。测试 trace 只有 TEST_ONLY harness 可收集；不是生产协议的隐藏额外帧。

L 不是“把真实执行得到的 transcript 当成模拟器输入”。理想功能从双方提交的 shares 恢复
功能输入后，内部生成自己的随机置换/掩码/公共币，运行被授权泄露函数 L，并把合法泄露提供给
模拟器。模拟器不得先得到真实 `public_z`、真实图或真实 abort 再声称这些量可模拟。

## 3. 已独立接收的条件性 DCF / 全池 / shuffle 论证

### 3.1 实际 M2UC v1 单 key

源码对象是 `VFSS/ext/FSS/dcf.cpp`、`dcf.h`、`keypack.h` 与
`VFSS/src/moe_topk/protocol_i_ucmp.cpp`。当前 DCF 的实际功能是
`β·[x < α] mod 2^64`，Bin 为 34..53，Bout=64、groupSize=1、MSB-first；uCMP 在一个
party key 上按固定变换执行两次 Eval，以重构严格比较的加法 share。这两次 Eval 是同一 key 的
确定性后处理，不是两把独立 key。

S24 确认 common seed correction 不能逐字段等同 BGI15 独立 target correction；这只否定了
那个直接映射，不是完整 party-key 攻击。S27/S28 改用对完整序列化 key 的源码专属分析：把
126-bit AES seed 的四个固定扩展输入抽象成 G126 输出（两个 child seeds/control 与 value 坐标），
在理想扩展器下，给定被腐化方 root、扩展状态和 serialized prefix，隐藏 losing seed/value
分别遮蔽 correction words、v 与终端 g；keep seed 的相关性按条件均匀性进入下一层归纳。结果是
整个腐化方 key 串（root、每层 CW、v 向量、终端 g）分布不依赖 α、β。S28 对含完整 root
block 中未用第二低位的 toy enumeration 作了修正并复核，枚举仅是代数一致性检查，不是 PRP 证明。

从理想 G126 到实际代码的条件为：

1. 对 `encode(S || 00)` 这一 126-bit 受限 key 家族，在每个节点扩展用到的四个固定 AES 输入上，
   假设相应的 PRG/PRP 安全及带所需辅助输入的归约；一般“均匀 128-bit AES PRP 安全”不自动
   推出该受限子族假设。
2. 受限 4 点 PRP→独立随机函数切换的显式统计项可取至多 `6/2^128` 每个独立 expansion；
   AES 区分优势保持符号，不编造安全位数。
3. cryptoTools AES-CTR 根随机流按实际消费块数 `q_t` 的根流假设；底层 OS CSPRNG 提供所需
   不可预测、相互独立的种子。

BGI15 Theorem 6 不直接覆盖此压缩 serialization ABI。本证明条件是 S28 审查的 source-specific
G126 bridge，不是“字段看起来相同”。

### 3.2 全池、多次 Eval 与自适应 transcript

令 `p=padded_n`，`C=binom(n,2)`。实际 DCF KeyGen 调用数与每方总材料槽分开：

\[
N_{DCF}=2p+9C,\qquad S_{party}=2p+2+9C.
\]

`2p` 是 raw carry/sign keygen；`8C` 是两个 Select task × 四层各自的真实边池；`C` 是独立
membership keygen。每个 DCF/uCMP slot 被两次 DCF Eval 调用；slot 数不乘 2 记。每方额外两个
槽是 forward/inverse shuffle；不计入 `N_DCF`。n=1 shortcut 不生成该池。

| n | p | C | `N_DCF` | per-party slots `S_party` | S28 源布局 `q0=2N_DCF` |
|---:|---:|---:|---:|---:|---:|
| 2 | 2 | 1 | 13 | 15 | 26 |
| 3 | 4 | 3 | 35 | 37 | 70 |
| 5 | 8 | 10 | 106 | 108 | 212 |
| 8 | 8 | 28 | 268 | 270 | 536 |
| 64 | 64 | 2,016 | 18,272 | 18,274 | 36,544 |
| 128 | 128 | 8,128 | 73,408 | 73,410 | 146,816 |
| 256 | 256 | 32,640 | 294,272 | 294,274 | 588,544 |

T 端当前 KeyGen 串行，所以布局分析为 `q_0=2N_DCF` AES-CTR PRNG blocks，其他线程
`q_t=0`。这是源码控制流推导，不是硬件计数器实测。KeyGen seed 独立；阈值向量因共享 node-mask
而相关；相关阈值不表示 keygen seed 或完整 party key 相同。没有发现跨 task、round、edge 或
membership 槽复用；轮内反向逻辑请求用 canonical unordered endpoint 映射同一比较调用规则。

S28 对完整相关池 hybrid 的条件界写为：

\[
Adv_{pool}\le Adv_{root}((q_t)_t)
 + N_{DCF}\,b\,Adv^{aux}_{G126}
 + {2N_{DCF}b\choose2}2^{-126}.
\]

其中 `b=Bin≤53`，G126 假设必须允许固定参数向量、其它槽、已生成的 party-key 前缀及公开历史
transcript 作为辅助输入；碰撞项保守地覆盖至多 `2N_DCF b` 个 126-bit 状态。所有未使用 keys
也在在线之前生成并包含在 corrupt party view；槽 ID 和后续请求只按公开 L 与先前打开 bit 决定。
在逐槽替换中，归约固定其余相关 α/β 并本地生成其他独立 KeyGen；挑战槽的本方 Eval share
是 key 与公开 operand 的确定性函数。打开 bit 后 peer share 为 `bit - local_share mod 2^64`，
因此 simulator 从理想 L 取得该已打开 bit即可产生联合两份 Eval 记录，而不是只证明两份和正确。
自适应下一查询由模拟出的已有 transcript 决定；unused slots 仍包含在多 key hybrid 视图。

上述界是条件优势式，不给一个数值安全位数。它假设独立 KeyGen 根流替换成立，且安全游戏允许
自适应的 operand/槽使用；若项目改用未审查 DCF ABI、改变根随机流、slot 参数绑定或 key reuse，
该独立接收不自动迁移。

### 3.3 shuffle/output share 组合

S28 对实际代码重建：`τ0=π∘σ1⁻¹`、`τ1=π∘σ0⁻¹`；
`e0=−τ0(a1)−h`、`e1=−τ1(a0)+h`；消息
`m_b=σ_b(x_b)+a_b`、`q_b=τ_b(m_peer)+e_b+r_b`，合并
`z=π(x0+x1)+r0+r1`。forward 与 inverse 使用新鲜随机置换因子/掩码，inverse 输出的 bit
share 是 `word0 & 1`。

条件模拟给定 corrupt party 理想 output share `y_b` 与 L：先采样 peer 的 inverse round-1
frame；采样本方中间 `w_b` 使 LSB=`y_b`、其他可见位均匀；设 `e_b=w_b−τ_b(m_peer)`；再采
fresh `r_b` 并构造 `q_b=w_b+r_b`，peer frame 以公开总量作差。该模拟器不获 `π` 或
original-index↔handle 映射。S28 逐步复核该 output-share 条件联合分布并标为条件性通过；前提是
T 可信不合谋、局部随机置换/mask 正确独立、L 完整、输出 shares 按理想功能给定。

两 Select task 共用的匿名真实句柄属于 L 的公开关联；不能从“单个 task 图形边际匿名”推出
跨 task 隐私。S28 的模拟将两个 task 的关联 transcript 放在同一 L/view 中。

## 4. sampler、正确性与中止

在线两方各用 OS CSPRNG 取 32-byte contribution，经 phase 19 交换后 XOR；由
`SHA256(seed || task || domain || counter)` 和 rejection sampling 驱动无放回采样。共同 seed
和采样结果属于 L。算法自然中止与材料/通信/不变量错误分别处理；只有 phase 40 的两方状态
相同才是 `PEER_AGREED`。断连没有 ACK 时只能记 `LOCAL_ONLY`，不发布 mask。

S16/S28 的理想均匀无放回采样 union bound 在 S29 **接受的 ROM 配置**下使用，并要求至少一个
诚实方的 OS random contribution 在其选定前不可被对方控制。对当前 API 已受测范围 `n≤256`，
最坏报告界为 `δ_ideal≤1.3105804606611235×10⁻¹¹`。这不是 SHA-256 标准模型保证，不是
0 次自然 abort 的经验外推；标准模型 sampler 仍 UNPROVEN。更大的公式域 n≤10⁶ 的历史界
`5.065679989546104×10⁻⁸` 不扩张当前 API 的支持声明。

在理想无放回/ROM 采样前提下，若 Select 报告 SUCCESS，其两哨兵交集唯一第 K 项、DCF membership
使用 `R_i ≥ R_selected` 的稳定键方向，且 inverse 置换未变，则输出严格 oracle Top-K mask，
长度 n、每位 XOR bit、恰 K 个 1。正常概率失败输出双方统一 ABORT 且无 mask。S28 功能接收
有界到 n≤256；此处不声明概率算法在所有随机带必定 SUCCESS。

## 5. 版本化 API 与运行限制

S29 新增的符号为：

```text
protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(...)
```

构建默认 `MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=OFF`；启用时还须显式打开既有
`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`。接口校验 `n≤256`、强制
`require_authenticated_transport=true`、拒绝 TEST_ONLY fault controls 和 TEST_ONLY random tape，
然后调用同一 `protocol_i_bmw16_experimental_raw_score_mask_party`。`party-conditional-secure-v1`
CLI 子命令只提供这一入口的独立进程运行方式，也固定使用 TLS；不存在 raw-FD 降级、第二套 Select、
oracle、明文 fallback 或在线 Dealer。

此接口“安全候选”标签只在本文件第 2 节模型和第 3–4 节假设下成立。底层原实验入口仍存在且
默认关闭；任何未携带条件假设的调用、非 mTLS channel、超出受测 n 或 failpoint/test tape 都
不是该安全配置。

## 6. 分项门禁

| 门 | S29 状态 | 依据/剩余边界 |
|---|---|---|
| `FUNCTIONAL_CODE_ACCEPTANCE` | S26/S28 已独立接收；S29 wrapper 待本报告验证 | underlying runtime 的 S28 bounded differential/E2E；S29 还须跑 wrapper mTLS E2E。 |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL，S28 独立接收 | G126 ideal induction + restricted-key AES/AES-CTR/OS 假设；非 BGI15 定理直接套用。 |
| `ADAPTIVE_FULL_POOL_VIEW` | CONDITIONAL，S28 独立接收 | 上述多 key hybrid、相关阈值、未用池、opened bit、adaptive query 和同 key 两 Eval。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | CONDITIONAL，S28 独立接收 | 同置换 forward/inverse 与给定 output-share 的 view simulator；可信 T、fresh masks、L 完整。 |
| `SAMPLER_ABORT_GUARANTEE` | CONDITIONAL，S28 独立接收 | 理想均匀无放回 + ROM + honest entropy contribution；标准模型无界。 |
| `CONDITIONAL_SECURE_V1_ENTRY` | PASS under this profile after S29 E2E | 默认 OFF、强制 TLS、限 n≤256、调用唯一已接收 runtime。 |
| `DEPLOYMENT` | NOT_ACCEPTED | 生产证书签发/轮换、跨主机运维/备份策略不属于此候选。 |
| `FORMAL_PERFORMANCE` | NOT_RUN | V3 前置未完成；无 V4 LAN/WAN 或九指标正式数据。 |

独立安全复核身份是 S28（报告提交 `27bee47...`），独立功能复核身份是 S26 receiver
（报告提交 `32f7f0b...`）。S29 负责将二者整合并验证新增薄 wrapper，不将本聊天的改动称为
异会话接收。若将来底层 DCF keypack、sampler、shuffle、消息泄露、材料 ABI 或安全假设变化，
须重新开启相应门。

## 7. PR 与性能状态

S26 接收者给 `DRAFT_PR_READY=PASS`；此状态本身不是 PR 存在或已合并的证据。S28 报告当时
记录尚未创建 PR；S29 后来核对发现 PR #29 已存在，随后将 S29 集成分支快进推到它的源分支。
当前远端源分支为 `codex/m6b-i-bmw16-s26`，S29 状态文档同步后最后核验 head 为 `915fbbdd7e96bdd34f97f4d414b335f3b3951935`，
目标仍为 `main`。PR #29 仍为 Open（非 Draft）；GitHub 元数据连接器返回
`USER_NOT_LOGGED_IN`，因此本任务没有修改 PR 标题/正文或 Draft 状态。远端 main 仍未包含该候选。
可查看 [PR #29](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/29) 和
[main 与候选分支比较](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/compare/main...codex/m6b-i-bmw16-s26)。
S29 不合并 main、不改 PR #28，也不把远端功能分支写成 main 状态。

正式 LAN/WAN 1+5、统一九指标和六方案排名保持 NOT_RUN；本地 loopback/mTLS 功能验证不计作
性能数据。
