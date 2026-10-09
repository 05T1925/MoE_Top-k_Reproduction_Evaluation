# BMW16 S30：条件安全接收、版本化规模和实验状态决策

日期：2026-10-10
方案：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**
决策边界：条件安全候选，不是 BB90 原算法复现、BMW16 Algorithm 7 逐字实现、无条件/恶意安全或生产部署 API。

## 1. 接收对象与身份

S29 的对象保持分开：S26 核心 runtime `2818bce20f30719eaccf0bc5df586cce4fe78c84`、S26 runtime 修复 `ded433d636a3df929380bdce0ff22f4ce6121453`、S26 异会话功能接收 `32f7f0ba3f80083ff282c9b5ebd0a827bd366898`、S27 条件证明文档 `8bbf7119470bbc37f6950a3b425255ed993eaa8d`、S28 独立复核 `27bee47de4356a87af6a1f6ff06be6c7e0df2c9b`、S29 被测 runtime `08e96d393da6f102bb047d230e8845da4509814d`、S29 最终文档/PR head `871b6683d15a34d117eb24353611d8ef48434aca`。

S30 从 S29 最终 head 在独立 worktree 开始；main、origin/main、merge-base 均为 `c3926c68fd14f270faa8b55234311071947fa080`。S30 没有合并或改写 S29 历史。S29 文件内出现的 `915fbbdd7e96bdd34f97f4d414b335f3b3951935` 是中间 ref 检查，不是最终 head。S30 重新读取 `origin/codex/m6b-i-bmw16-s26` 和 `refs/pull/29/head`，二者为 `871b6683d15a34d117eb24353611d8ef48434aca`；PR #29 目标是 main，状态 Open、非 Draft、未合并。GitHub 元数据连接器未登录，未能更新 PR 标题或正文；S30 本地分支也未推送。

## 2. 安全命题和许可泄露

### 2.1 假设

- T 可信、离线、在线静默，不与 P0/P1 任一方合谋，只根据公开配置发出新鲜材料。
- P0/P1 最多一方半诚实腐化。材料和在线信道在数学论证中为双方私有认证通道。
- 实际 M2UC v1 序列化 DCF 的证明依赖源码专属 G126 理想扩展归纳及受限 126-bit AES-key 子族的带辅助输入 PRG/PRP 假设；普通 AES-128 PRP 定理或 BGI15 Theorem 6 不直接替代该假设。
- cryptoTools AES-CTR 根流按源码实际消费的 `q_t` 个 block 进行 PRG 假设替换；根种子来自 OS CSPRNG。KeyGen 随机带独立，SHA-256 counter sampler 的理想界采用随机预言机模型（ROM）和至少一方诚实、不可预测的 OS 熵贡献。
- S28 已作为未参与 S27 写作的独立接收者复核 S27 对完整 M2UC v1 key、相关整池、opened comparison bits、自适应查询和 forward/inverse shuffle-output-share 的条件论证。S30 不把该条件性结论改写为无条件证明。

### 2.2 泄露函数 L

下列量由双方可见或可从协议长度/阶段直接取得，属于本项目批准的候选泄露函数：公开配置和阶段；raw adapter 的 carry/sign masked operand；forward/inverse `public_z`；匿名句柄端点和比较 bit；抽样/pivot/U/V/W/reject/accept 的匿名位置；两个 Select task 共用的匿名真实句柄及 selected anonymous handle；公开抽样币；key/slot ID、用途、task、round、访问顺序和次数；比较次数、消息相位/长度；统一算法状态；以及腐化方自己的最终输出 share。

L 不含原始 score、`original_index` 与匿名 handle 的映射、明文阈值、完整明文 mask、对方的输入/输出 share。材料/通信/OS 错误是工程错误合同，断连时仅承诺存活方 LOCAL_ONLY abort；它们不是正常协议概率 abort 的安全模拟结论。L 是规格，不等于把真实 transcript 预先交给模拟器。模拟器须从理想输入/输出与被许可泄露生成相应 masked 消息和后续公开 transcript。

## 3. S28 条件证明在 S29 最终入口的适用性

S29 的 `conditional_secure_v1` 只新增 default-off 的入口约束：要求已认证 TLS、`1 ≤ n ≤ 256`，拒绝 TEST_ONLY fault/tape 控制，再委托同一 S26 party runtime、材料 ABI、Select、shuffle、DCF/uCMP membership 和 inverse。S30 审查 v1 的实参解析、12 路 TLS channel 注册/取用、身份/session/fingerprint/n/K/phase 绑定及缺 TLS fail-closed；未发现第二套 Select、裸 FD 降级、明文重构、在线 T 或可继续执行的重复领取路径。

S28 条件证明核对的是：

1. 完整序列化 M2UC v1 party key（root、每层 correction、`v`、终端项及 party sign）的源码专属单 key 函数隐私归纳。S24 发现 common correction field 与 BGI15 字段的直接同名映射不成立；它不是完整序列化 key 的攻击。
2. `N_DCF = 2p + 9·C(n,2)` 次独立 KeyGen、每方槽数 `2p + 2 + 9·C(n,2)`，其中额外两个槽为 forward/inverse shuffle；同一 uCMP key 两次 Eval 是固定后处理，不计作两把 key。未用 key 也在完整视图中。条件优势项为
   `Adv_pool ≤ Adv_root((q_t)_t) + N_DCF·b·Adv_G126_aux + C(2·N_DCF·b,2)·2^-126`，`b≤53`；受限族优势保留符号，不编造安全位数。
3. 实际 forward/inverse 置换状态、两 task 共享匿名 handle、selected anonymous handle 和最终 output-share 的联合条件模拟；理想私有认证通道、诚实离线 T、单方半诚实腐化和完整 L 是前提。
4. 抽样理想失败界在理想无放回/ROM 与诚实 OS 熵贡献下成立。受测 `n≤256` 的最坏界为 `1.3105804606611235e-11`；标准模型 SHA-256 sampler 界没有由此证明。零次自然 abort 观察不是概率证明。

这些结论的独立安全复核人为 S28，复核对象为 S26 实际 runtime，不是 S30 新造的数学定理。S30 的版本 v2 仅将入口 admission cap 从 256 提至 1000，使用同一 runtime 和材料 ABI；对应较大 `N_DCF` 的优势项已按同一参数式重算，仍明确带上述条件。

## 4. 两个版本的入口合同

| 版本 | 符号/CLI | 允许规模 | 构建状态 | 实际职责 |
|---|---|---:|---|---|
| v1 | `protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party` / `party-conditional-secure-v1` | `1 ≤ n ≤ 256` | 默认 OFF；需实验适配器和 v1 选项均 ON | S29 已有条件入口；S30 复跑。 |
| v2 | `protocol_i_bmw16_conditional_secure_v2_raw_score_mask_party` / `party-conditional-secure-v2` | `1 ≤ n ≤ 1000` | 新增选项默认 OFF；需实验适配器与 v2 选项均 ON | 复用 v1 的同一个 party runtime、FSS/材料格式/Select；只增加经公式和 n=1000 E2E 验证的 admission scope。 |

两入口均强制 authenticated TLS，并拒绝 TEST_ONLY failpoint；不存在重试、明文排序、oracle、在线 Dealer、测试随机 tape 或裸 FD fallback。`n=1` 按双方本地 shortcut 处理，不发材料、不启动 T/网络。

SUCCESS 输出为原始输入顺序的 XOR mask shares；仅隔离测试控制器重构检查 oracle、长度、bit 值和 weight K。算法概率 abort 必须双方 agreement 且无 mask；工程、材料和通信失败使用独立状态。网络断连只有 LOCAL_ONLY 结论。S30 不更改原核心消息轮数或泄露。

## 5. n=1000 与大规模准入

当前编码规则按 `p=next_power_of_two(n)`，`b=33+index_bits`，`C=n(n−1)/2`。全池解析式：

| n | p | b | `N_DCF=2p+9C` | 槽/party `2p+2+9C` | sidecar bytes/party |
|---:|---:|---:|---:|---:|---:|
| 256 | 256 | 41 | 294,272 | 294,274 | 310,504,484 |
| 1,000 | 1,024 | 43 | 4,497,548 | 4,497,550 | 4,967,527,664 |
| 10,000 | 16,384 | 47 | 449,987,768 | 449,987,770 | 540,395,955,164 |
| 100,000 | 131,072 | 50 | 44,999,812,144 | 44,999,812,146 | 57,284,427,150,164 |
| 1,000,000 | 1,048,576 | 53 | 4,499,997,597,152 | 4,499,997,597,154 | 6,052,493,947,500,164 |

`N_DCF` 是 KeyGen 数，槽数另含两个 shuffle slot；每个 uCMP slot 两次 DCF Eval。上述均为 checked 解析容量，不是性能实测。S29 v1 的 n≤256 合同不变。S30 新建默认关闭的 `conditional_secure_v2`（n≤1000），逐层 conformance、v1 共用算法 differential 之后，fresh-material 三进程 mTLS E2E 对 `(1000,80)` 实跑成功。S30 两次单机 fresh-material 功能运行的 T 生成时间分别为 612.129 s 和 546.766 s，mTLS 发材分别为 451.672 s 和 389.401 s；最终复跑的两方 TLS sent bytes 为 9,941,055,454。这些是两次单机 loopback 功能/资源记录，不是正式性能指标、LAN/WAN 结果或可比时间。

n=10,000 及更大容量超出本地可接受的全池预算，保留 PRECHECK_REJECTED/RESOURCE_INFEASIBLE；不生成材料。后续服务器试运行建议至少 8 vCPU、16 GiB RAM、每个 fresh-material 任务 30 GB 可用临时盘，T 与两 party 有独立 UID/私有存储和 claim roots；网络需支持约 10 GB 双方材料投递且由后续测试实测带宽/RTT。以上是租赁规格建议，不是本轮验证的最小必要资源或网络性能结论。

## 6. S30 分项决定

| 门 | S30 判定 | 边界 |
|---|---|---|
| S29 v1 独立入口接收 | PASS（n≤256） | 源码审查、default-off、conformance/common-tape、47 个独立 session E2E；见 S30 验证报告。 |
| DCF single-key privacy | CONDITIONAL / S28 独立接收 | 仅在 G126 与受限 key 子族 PRG/PRP、AES-CTR、OS CSPRNG 假设下；非 BGI15 直接适用。 |
| adaptive full-pool view | CONDITIONAL / S28 独立接收 | 包含相关阈值、未用 keys、自适应查询、同 key 两次 Eval 和打开 bit；优势保留符号。 |
| shuffle/output composition | CONDITIONAL / S28 独立接收 | 可信 T、理想私有认证信道和完整 L。 |
| sampler abort bound | CONDITIONAL / S28 独立接收 | 理想无放回 + ROM + 至少一方诚实 OS 熵；标准模型不主张。 |
| conditional security design | CONDITIONALLY_ACCEPTED | n≤256 v1：独立接收的代码门与条件安全设计门已关闭；v2 n≤1000 采用同假设且 S30 功能 E2E 通过，但新增 admission wrapper 本身应由下一接收者复核。 |
| production deployment | NOT_ACCEPTED | 生产证书注册/轮换、远程机器管理、管理员/快照回滚不在模型。 |
| formal performance | NOT_RUN | 不启动 LAN/WAN 1+5、V4、六方案排名；九指标保持 NOT_MEASURED。 |

下一步只需异会话复核 S30 的 v2 admission wrapper / CLI 和本次 n=1000 功能记录，再安排服务器上的正式性能矩阵；不得为 n≤256 v1 重复打开 S28 已接收的同一条件证明。
