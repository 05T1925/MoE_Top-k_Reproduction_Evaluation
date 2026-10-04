# M6A-P2-III-E2：隐藏 handle AAV86 + ring DPF 构造门

日期 2026-10-04。基点为 E1 事后报告 `67739472c4409a62a676823c9e786fe4aa40bb57`，E2 勘误源码检查点 `ae100e7c7df9d11437aadcd5be40d5b5d5bdcf73`，分支 `codex/m6a-p2-iii-e2`。**候选身份仅为项目组合，非 Agarwal 作者精确 Protocol III+AAV86。项目所有者已明确允许第 3 节的扩展泄露，继续审查稀疏图方案。当前 `SECURE_RUNTIME_GATE = NO-GO`：native ring DPF 的联合辅助信息目标隐私缺可审阅依据，完整组合帧/材料尚未实现和审计。**功能 TEST_ONLY 可继续；不能把这个门写成“padding 缺陷”，E1 的该事实错误已另行勘误。

## 0. 证据及冻结接口

论文定义：AAV86 的 pivot 递归比较图；Agarwal 会议版 §5 的 shuffle-based CA compiler（Theorem 5.1 是 `2r+1`，并未给本组合的 `2r` 安全定理）。本地行为：`protocol_i_aav86_small_party` 早期及末轮均公开 local rank、最后公开完整 shuffled 全序再由 P0 构造 carrier；E7 是该 I 路线的**条件性**全池 hybrid。已签收项目接口：`protocol_iii_raw_score_mask_party` 四轮，从双方各自的 signed Q20.12 `Z_(2^32)` 加法份额到原顺序 XOR mask；score adapter 两轮，固定原槽 `logical_n` GRank 一轮，ring DPF 一轮。M5 两轮 odd-prime field Fselect/Fsort 是另一种一般 payload 功能，不参与本位 mask 构造。下列 D-handle 图、最后 rank 份额及逆 π 均为待验证项目扩展。

E2 直接核查 main/E1 的 score adapter 相同 blob：padding 输入份额 P0=`0x80000000`、P1=`0`，真实 dummy key 是 `INT32_MIN` 且下标较大。现有正式 GRank 只比较 `n` 个真实槽；要跑隐藏 D-handle 图，必须**消费现有 score adapter 输出的 D 个 key 份额**，而不是把现有 GRank 原槽输出重命名。n=3/K=3 的 `[-1,-1,-1]` 与全 `INT32_MIN` 已通过真实 adapter + 正式入口定向测试；E1 的 score-0 反例只为反事实错误接线。

## 1. 固定角色、阶段与最小接口

`n≥2`、`D=max(2,next_pow2(n))`、`1≤K≤n`、`1≤r≤5` 公开。严格 priority key 为 `(UINT32_MAX-(raw^0x80000000))<<log2(D) | original_index`，小者优先。T 可信、不合谋、无需擦除，仅看公开 shape、session/fingerprint/material ID/版本、K/r/位宽；在线输入份额分发之前发完材料、收到离线 ACK 后退出，没有在线 FD、查询或补料。P0/P1 只收自己的 score 份额和一次性包；测试控制器独占清分数/输入 seed/oracle。线上假设私有、有序、完整、半诚实；未主张恶意方或恒时侧信道。

| 顺序阶段 | T 在输入前产生和两方所持 | 公开消息及每方输出 | 现有复用 / 新增 |
| --- | --- | --- | --- |
| S1 score→D key | 每方 D 个 carry/sign uCMP 材料；dummy 由现有 adapter P0/P1 份额设置 | carry、sign 两个 framed exchange；各方获 D 个 key 加法份额，原下标已绑定 | 直接复用 `protocol_i_raw_score_input_party`，保持版本/shape/一次性约束。 |
| S2 同 π 前向布局 | T 均匀 π，分发 P0/P1 各自 `σ,τ,a,e` 与 fresh 首轮节点 mask share；不得把 π 给任一在线方 | 一次 share-shuffle exchange 后各方获 `π(key)` 的 D 个加法份额；不公开 handle↔原槽 | 复用 I+AAV86 的两遍线性 shuffle 代数/材料格式；新 III package 必须绑定与逆向同 π、与首轮 mask/key 一致。 |
| S3 每轮图 CA | 对每个 `t∈[1,r]`、`a<c<D` 独立 keygen `uCMP(ell,R[t,a],R[t,c])`，每方持 `rD` node-mask shares 与 `r·C(D,2)` key 槽；仅访问公开图 `E_t` | 固定 D-word `y_t=π(key)+R_t` masked-list exchange；本地 Eval 仅对 `E_t`，产生 local-rank shares。`t<r` 时再交换 D-word rank shares 并公开 `L_t`，决定 pivot/bucket/下一图；`t=r` **不得交换 rank shares** | 可复用 uCMP/DCF 单边 Eval 与 E7 图规则；必须新增 `III local-rank-share` 边界，禁止调用 I 的 final `flatten`/public carrier。 |
| S4 全局 rank ring | T 不需输入相关材；前轮公开 pivot rank/bucket 给每个子节点公开偏移 | P0 对已公开 pivot/singleton 或最后局部份额加公开 offset，P1 加 0；双方得到每个 D handle 在 `Z_(2^b)` 的全局 rank 份额，`b=log2 D`，不公开最终 rank | 最小 rank-offset adapter；不能复用固定原槽 `protocol_iii_grank_party`。 |
| S5 handle ring DPF | T 对每个 D handle 独立采样 `u_h∈Z_(2^b)`、两方加法 mask shares 和一对 `DPF(b,64,u_h,1)` keys；key/点均绑定该 handle | 双方交换 D-word `rank_share+u_share`，公开均匀 `m_h`；各自在 `m_h-j mod 2^b`、`0≤j<K` Eval，`Z_(2^64)` 累加取低位，得 handle XOR bit 份额 | 可复用 F1 ring DPF 算法/64 位输出与本地 parity；新 D-handle package/路由层。不能按原槽 n-key 向量错配。 |
| S6 同 π 逆路由 | T 另采独立 `γ,δ,a',e',h'`，目标恰为 S2 的 `π^{-1}`；新随机币，不重用 S2 掩码 | 每方把 handle bit 视为 `Z_(2^b)` 加法 share，一次 inverse share-shuffle exchange，原槽份额再取低位并截 `n`，输出各自 n 个 XOR bit | 可复用 I 逆置换线性代数，但输入不再是 I 的**公开** carrier；须审 DPF bit shares 与逆材料联合单方视图。 |

每个包/材料须绑定 `(version,session,fingerprint,material ID,party,n,D,K,r,ell,b,stage,t,canonical a<c 或 handle)`；T keygen 单线程独立随机币，fresh 每轮 R 与每条 keygen 币，D 个 DPF mask/key 一对一。应用包、fd/帧、材料领取需在首个输入相关消息前一次性 claim；崩溃、超时、短包、错误 peer/phase、重放都不可重试同一 ID。现有 F1 bundle 与 I+AAV86 bundle 是各自合同，**没有**这样一个组合包；这是最小新增接口之一，绝不通过文件轮询或在线 T 拼接。离线预留 `r·C(D,2)` 与实际 `e_A=Σ|E_t|`、逐轮 `v_t` 分列。固定 M 延期。

## 2. 功能代数及 TEST_ONLY 边界

E1 TEST_ONLY 明文模型对 n=2/5/8、K=1/中间/n、r=1..5、不同 pivot seed、全等/重复/极值、非二次幂验证 rank-offset 递归与原序恰 K；E2 已把其 score-0 分支明确标成反事实，默认正确 dummy 与现有 adapter 相同。公开早期 local rank 让根及子节点的 pivot 名次与桶大小可见。每个 pivot 的全局 rank 是公开 `offset+local_rank`；第 j 桶偏移由前一 pivot rank+1 给出，空桶不产生子节点。提前 singleton 的名次也公开。最终 depth=1 的节点取 `m-1` pivot，其比较图为完整 clique，故 final local-rank **份额**加公开 offset 为全局份额。唯一严格 key 保证 0..D−1 是置换，无 rank 真值回绕；dummy 同 `INT32_MIN` 时仍因原下标靠后。该归纳不允许末轮像 I 一样公开全序。

对每个 handle，`m_h=(rank_h+u_h) mod 2^b`，DPF 目标为 `u_h`，在 `m_h-j` 处得到重构 `[rank_h=j]`。逐方的 `Z_(2^64)→Z_2` 低位投影保加法，故 XOR 为 `[rank_h<K]`；也可先映到任何较窄 2 幂环而保持低位。odd-prime field 低位无此性质。前/逆置换同 π，线性逆变换后的低位等于原槽 mask，截 dummy 后恰 K。错误地直接返回 handle mask 的 n=2 π-swap 反例仍有效。以上明文/代数夹具不调用真实新 secure 帧；不能当成 conformance、冻结 C++ oracle differential 或独立进程 E2E。

## 3. 逐方泄露合同与项目所有者选择

P0/P1 的现有正式 F1 视图：自己的 raw-score shares、离线 GRank/DPF keys、原槽 masked comparison list、原槽**均匀 masked rank**、固定全两两图/固定长度流量及自己的原序 XOR 输出份额。E2 候选另含自己的前/逆 share-shuffle 因子、每轮 D-handle masked list `y_t`、早期 local rank `L_t`、pivot seed/身份、bucket membership/大小与公开 offset、后续活跃图、canonical edge 访问序列、每轮实际帧长/中止时点、D-handle masked final rank 及逆路由 transcript；**末轮明文 rank、原槽 selected index、原序明文 mask 不在许可内**。T 额外知道完整 π、所有 R、两份 keygen 随机币/分发包，但从不见在线输入、y/L/m/输出，且与任一在线方不合谋。显式消息、访问流量和 abort 都应写入理想泄露函数；不得暗称它们与 F1 masked rank 相同。

| 选项 | 泄露与可行性 | 成本与状态 |
| --- | --- | --- |
| A：项目扩展，明确接受 hidden-handle CA 控制公开 | 公开每轮 `y_t`、`L_{t<r}`、pivot、bucket、图、edge 访问/帧长及 abort；这些给观察者 shuffled-domain 部分名次、递归分区、实际工作量，π 仍应对任一单方隐藏。是否与最终原槽输出联立仍需第 4 节证明。 | 可使用 E7 全池工具和活跃边 Eval；离线 `r·C(D,2)`，在线约 `e_A` 次比较，候选 CA/DPF `2r`。**项目所有者已于本任务答复允许选项 A**；这不是 I 的许可转移。 |
| B：保持 F1 的公开范围，隐藏图控制 | 不公开 `L_t`/桶/活跃 edge 访问，可用 oblivious control、secure mux/ORAM 或每轮求值全池并固定流量；需给出密文图更新与 rank-offset 计算的新协议。 | 现有代码无接口/证明；简单全池 Eval 使在线比较升至 `r·C(D,2)`，失去 AAV 稀疏工作优势；ORAM/mux 的交互、材料与轮数均 `NOT_MEASURED/NOT_PROVEN`，不能先声称 `2r`。 |

**已记录的项目决定：** 允许在线双方看到每轮 hidden-handle masked list、前 `r−1` 轮 local rank、pivot、bucket、活跃图、edge 访问/帧长和 abort 时点；末轮 rank、原槽 selected index 与明文 mask 仍不公开。此许可只确定选项 A 的泄露边界，不构成安全证明或 secure runtime 准入；选项 B 留作成本参照。不得把 Protocol I 的许可或证明直接转给 III。

## 4. E7 全池 hybrid 的逐步迁移及首个未闭合点

候选假设：E7 的 `FSS-IND`（任一独立单份 DCF key 的选阈值计算不可区分，允许序列化与本地多次 Eval 后处理）、独立 OS seed/AES `PRG-IND`、正确 uCMP、私有有序完整通道、可信不合谋 T、半诚实 P0/P1；另须 `DPF-IND-64`：对 `b≤log2 D`、输出加法群 `Z_(2^64)`、固定 payload 1，任一单份原生 DPF key 在独立新鲜 keygen 币下对任意两目标计算不可区分，即使给定挑战币独立的公开 Eval 点向量、自己 rank-mask share、早期 L、其他 keys、前/逆材料。要求含相同构建的实际 key serialization；仅 DPF conformance 不证明它。模拟器获公开 shape/session、允许的早期 `L_t`/图/流量和腐化方的输入/输出份额，不获原始分数、完整 π、最终 rank 或原序明文 mask。

1. `H0` 真执行；score adapter 的隐私沿既有 F1 条件，T 不见输入。前/逆材料的单方 permutation 因子联合计数可借 E7：固定任一方 `σ_p,τ_p,γ_p,δ_p`，对每个候选 π，另一方两个独立均匀因子各有唯一解；DPF handle keys 和 R 若独立采样，不主动标出原槽。**须在新包里重新核对**同 π 和独立随机币，不能只引用 I 测试。
2. 对 `(t,a,c)` 全池按字典序替换腐化方 DCF key 为阈值 0 的 key。归约固定**真实**全节点 R、y、输入、π、其他边 key，挑战边阈值 `R[t,a]-R[t,c]`；其他相关阈值作为挑战位无关辅助数据。未使用 key 也在包中，故全部 `r·C(D,2)` 都必须替换。每轮新 R 防止跨轮两个 y 消去 mask。此步依赖 `FSS-IND/PRG-IND`，不是单 key conformance。
3. `t<r` 的公开 local rank 不能随替换 key 漂移。若腐化方重算本地 share `q_p`，模拟 peer 发送 `L_t-q_p`，使公开 L、pivot、bucket、图及下一轮访问固定；这是 E7 的关键补丁，前提是选项 A 允许 `L_t`。图是此前 L 与公开 pivot 币的确定性后处理。若选项 B，不可使用此补丁，因为 L 不是模拟器可见量。
4. `t=r` **没有公开 L_r**，不能照搬第 3 步。令真实但均匀掩码后的最终打开值为 `m_h=rank_h+u_h`。挑战 key 改变腐化方 final rank share `q_p[h]` 时，模拟 peer 在 DPF 帧发送 `m_h-q_p[h]-u_p[h]`，保持 m 与后续 Eval 输入向量不变。这里在相邻 hybrid 的归约中可固定真实 rank、u、m，且它们不依赖挑战 keygen 币；但最终理想模拟器并不能得到 rank，须在替换完成后证明 `(m_h,u_p[h])` 可按联合分布采样。单凭 E7 的 `L-q_p` 论证不能跳过此步。
5. 对每个 handle 尝试把腐化方 `DPF(u_h,1)` key 换为 `DPF(0,1)` key；公开 Eval 点是 `m_h-j`，其与目标 u 因真实 rank 相关，但与该 DPF keygen **随机币**独立。若 `DPF-IND-64` 在上述辅助信息下成立，则腐化方所有 `K` 次 Eval 是确定性后处理。**这是迁移 E7 后第一个当前证据无法支持的转换：仓库只有 F1 DPF 功能/进程测试，没有适用的 native 64-bit DPF 单份 key 目标隐私与该联合辅助信息的可审阅假设/证明。** 不能把 E7 的 DCF `FSS-IND` 自动当作 DPF 结果。
6. 逆路由 peer 第一消息必须与另一方未知的 handle DPF bit share 联合模拟。E7 现有代数实际给出 `e_p=-τ_p(a_{1-p})±h'`，其中 fresh `h'` 与 `a_{1-p}` 均匀独立于 DPF keys、masked rank 和 carrier shares。固定腐化方已有的 `e_p,τ_p` 及任何 peer carrier `c_{1-p}` 后，`a_{1-p}` 仍均匀，故 `F_{1-p}=γ_{1-p}(c_{1-p})+a_{1-p}` 均匀。腐化方环输出 `z_p=τ_p(F_{1-p})+e_p` 也均匀；给定理想功能指定的**低位 XOR 输出 share**，模拟器可独立抽取每槽其余 `b-1` 位并反解 `F_{1-p}=τ_p^{-1}(z_p-e_p)`。此为针对 III secret carrier 的条件代数推导，而非援引 I 的 P1 carrier=0 特例；E2 增加了 `Z4` 两 handle 条件穷举 TEST_ONLY 夹具。该转换**在 fresh 独立 h'/a'、真实环帧按该代数实现、无 T 合谋的条件下闭合**；组合包及真实帧仍须 conformance/联合视图审计，不能把代数夹具写成 secure 证明。
7. 若前述两步补齐，替换后的本方 R/u shares 均匀、y/m 均匀；隐藏均匀 π 将严格全局 rank 标签均匀分配到 D handles，早期 L/图可由允许的 shuffled-domain 泄露模拟。T 持完整 π 与材料但不见在线值；T 与 P0/P1 合谋不在模型内。材料 ID 的持久一次性领取、重放/截断/peer abort 是工程 fail-closed 义务，不由此半诚实 hybrid 自动给出。

以上是可证伪的 hybrid 任务表，不是已完成的 III 安全证明。第一未闭合点是第 5 步的 `DPF-IND-64` 联合目标隐私。第 6 步在列出的独立均匀材料条件下有代数模拟，但新包与真实帧尚无实现/审计；项目所有者的扩展泄露选择不能填补这些证据缺口。

## 5. 消息 DAG、门项与后续验证

```text
offline T(shape/session only) → pre-send all keys, masks, forward/inverse π factors → T exit
R1 score carry → R2 score sign → D priority-key shares
R3 forward shuffle u_p → hidden π(key) shares
R4 open y_1 → Eval(E_1) → [r>1: R5 open L_1 → G_2]
[r>1: R6 open y_2 → ...] → R(2r+2) open y_r → final local rank shares + offsets
R(2r+3) open masked rank m → local DPF Eval/parity
R(2r+4) inverse π share message → original-order XOR bits
```

相对 score 后的 CA/DPF 候选为 `r` 次 masked-list open + `r-1` 次早期 local-rank open + 1 次 masked-rank open = **`2r`**；前向布局另 1、逆路由另 1、score adapter 另 2，完整候选为 `2r+4`。这里的 R 编号表示依赖层，不是已实现帧；尤其 R3 必须先于 R4，`L_t` 必须先于下轮图。**真实 III+AAV86 secure 帧数及 raw-score→原序 mask 轮数 = NOT_PROVEN**。现有四轮 III 全对全入口的 2+1+1 是独立已测功能基线，不与本候选混标。

门项：padding/严格键 `PASS`（真实适配器定向实调）；隐藏 D-handle rank 和 ring DPF/逆路由功能 `TEST_ONLY CONDITIONAL`；全池离线 shape `ANALYTIC ONLY`；项目扩展公开字段 `ACCEPTED`；完整联合单方视图 `NO-GO`（第 5 步和新包/帧审计）；真实组合包/帧/一次性/错误路径 `NOT_IMPLEMENTED`；实际因果轮 `NOT_PROVEN`；九指标及容量峰值 `NOT_MEASURED`。所以不进入 D≤8 secure runtime。若上述审查门全部满足，再按 conformance→冻结 oracle differential→独立 T/P0/P1 进程 E2E；n=2/5/8、K=1/中间/n、极值/全等/非二次幂、不同 pivot seed、错误材料/重放/peer abort/截断，逐轮 `e_t/v_t`、分方帧和材料 ID 原始记录。不得把本 TEST_ONLY 夹具作为第三层证据。
