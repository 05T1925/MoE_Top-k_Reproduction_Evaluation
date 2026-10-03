# M6A-P2-I-E7：E6 设计门复核与小 D 实现准入

日期：2026-10-03。复审对象为项目的可信离线 T、P0/P1 单方半诚实、每轮固定 D 全两两预发材料路线；固定 M 不在范围。本文由承接 E6 上下文的同一会话重新从代码与密码学定义推导，**不是人员独立签收**，后续仍需独立人员复核。本文先于任何 E7 secure runtime 改动保存。

## 1. 起点、来源和结论

隔离 worktree 分支 `codex/m6a-p2-i-allpairs-experiment`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`。起点 `git status --short --untracked-files=all` 仅有原先 E1–E6 的 19 个未跟踪文件：E3/E6 决策两份、E1/E2/E4/E5 报告四份、实验 CMake/README/两份 Python 参考与 E6 控制器/验证器/JSON、四份 CSV、E1 C++ 与计数器。具体起点清单以本报告所附工作记录和 E6 §0 为准；全部保留，本文新建文件不得覆盖。主工作区仍是 `feat/m6a-performance-evaluation@c3926c68`，有原有四个 tracked 文档修改及 P0/P1/P2 文档和 `siamjdiscrmath.pdf`，本轮不写。

已重读 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` M6A 门、`protocol-reproduction` skill、E3/E4/E5/E6、`protocol_i_ucmp.cpp`、`VFSS/ext/FSS/dcf.cpp`、C-INSTANTIATION shuffle、score adapter、permutation、party package 与 framed transport。论文定义是 AAV86 FOCS 1986 §3.1/Theorem 3.1 和 Agarwal CCS’24 §2.4/§5.2–5.4；E1–E6 是本地参考/项目审查；可信 T、稳定键、D padding、全池和原序 XOR mask 均属项目扩展。作者 full version/代码未核实到，`AUTHOR_EXACT=NOT_PROVEN`。

**设计门结论：`SMALL_D_RUNTIME_GATE = GO`，仅对明确标记的小 D 项目实例化、单方半诚实和下述密码学/通道假设成立。** 这不是密码学原语的源码证明、论文作者精确复现或大规模可行性证明。实施必须采用本报告第 5 节的 fresh 独立逆路由材料、第一轮同 R 绑定、持久一次性 claim 与显式小 D cap；任一不满足则实现门自动回退 NO-GO。此条件性 GO 是推导和接口准入，不是运行时验收；代码与测试完成后另判功能验收。

## 2. 底层假设与整池 key 视图的逐边归约

### 2.1 源码事实和精确假设

`ProtocolIUcmpMaterial(b,R_a,R_c)` 令 `alpha=(R_a-R_c) mod 2^b`，一次调用 `keyGenDCF(b,64,alpha,1)`，导出两份 party key。每方 `eval_strict_lt` 对**同一 key**在两个公开点 `x=y_a-y_c`、`z=x-2^(b-1)` 求值，再加公开 correction。单方序列化含 party、bits、DCF seed/correction arrays；没有 session/t/pair/material ID，且重载后内存 used flag 复位。`dcf.cpp:121–130` 从 `FSSConfig::prngs[omp_get_thread_num()]` 取初始 seed，后续用 AES PRG 派生校正值；C dealer 在 `protocol_i_parallel_shuffle.cpp:114–123` 用 `getrandom` 初始化 256 个线程 PRNG 一次。新 dealer 必须单线程顺序 keygen，安全初始化自己的 PRNG 状态，每 session 使用新 OS seed，不让多个并发调用竞用同一个 PRNG 状态。源码可证明调用关系与功能算式；AES/PRG 安全作为下述明确假设。

假设 **FSS-IND**：对任意 `b∈[34,53]`、任意两阈值 `alpha_0,alpha_1`、固定 payload=1，任一 party 的单份 DCF key 在独立新鲜 keygen 币下计算不可区分；对 key 的任意多次本地确定性 Eval 和序列化同样成立（由不可区分性的后处理）。假设 **PRG-IND**：VFSS 所用 AES 派生 PRG 在独立 OS seed 和顺序调用下实现 FSS-IND 所需伪随机性；不要求从 C++ 测试证明 AES。假设私有、有序、完整的 P0↔P1 与 T→party 通道，半诚实方执行协议，T 不合谋且离线交付后不接收在线帧；当前 framed header 只有字段/序号校验，没有密码学 MAC，因此不声称主动攻击或恶意网络安全。

### 2.2 相关阈值和辅助信息

固定 session、轮 t、端点域 D；T 独立采样每节点 `R[t,a]`，另采样本方均匀 share `R_p[t,a]`，补出对方 share。全池阈值 `alpha_e=R[t,a]-R[t,c]` 彼此相关；每边 keygen 从新的 PRG 状态取独立子串。按 `(t,a,c)` 字典序建立混合 H_j：前 j 条给腐化方的 key 改成阈值 0 的独立 party key，余下保留真实阈值。挑战边 e 的归约者先固定**真实**全部 R、输入、π、两方 shares 和公开 y，再生成所有其他边 keys 与材料；把 `(alpha_e,0)` 送给 FSS-IND 挑战，嵌入唯一挑战 key。其他边阈值与 `alpha_e` 相关无碍：归约者知道全 R，能在挑战币以外的独立 keygen 币下生成它们；该方 mask shares、公开 y、其他 keys、初始 shuffle 消息均作为同一、挑战位无关的辅助数据。挑战者知道 `alpha_e` 也不破坏选阈值隐私定义，因为它自选两阈值。至多 `r·C(D,2)` 次多项式 hybrid 损失；小 D 有固定上界。跨轮 fresh R 防止 `y_t-y_u` 消去 mask。

在线活跃图只由之前公开 local ranks 与公开 pivot 币确定。腐化方对活跃 key 的本地 Eval、查表次序、计数和消息构造是既有 key/公开 transcript 的确定性后处理；不要求 FSS 支持新的“选择性 keygen”功能。未活跃 key 仍在单方视图中，故 hybrid 必须覆盖**全池**，不能只对活跃边做证明。

rank reveal 不能被省略。令腐化方在挑战 key 下按真实本地程序算出 rank share 向量 `q_p`，公开允许泄露的 CA rank 为 `L`。在真实 H_0 中，正确性给出对方应发 share `q_{1-p}=L-q_p`（在 rank 打开环中）；模拟器**每一步都**发送这个值，故 H_0 与诚实运行一致。改动挑战 key 后，模拟器重算本方 q_p，并仍发送 `L-q_p`；整个公开 rank 仍为 L，后续图/访问完全一致。若发送其他独立随机 share，腐化方会把它与自己的 Eval 相加而检测到 rank 错误；本模拟器没有这个缺口。FSS-IND 后处理覆盖 `(key,Eval(key,y),L-Eval(key,y))`。模拟仅针对半诚实、允许 L 公开的理想功能；恶意方伪造 share/abort 不在结论内。

最后全部 key 已替换为与真实 R 无关的阈值 0 keys。本方 R share 均匀；给定它和自己的 shuffled record share，对方 fresh R share 使每轮整个 `y_t` 均匀，纵使同一节点 mask 用于多边也如此。公开 rank 后的 CA 图、bucket、访问顺序、规范消息长度由 L 和公共币确定。因而可从理想泄露采样本方全池 package、y 和 peer rank 消息。若原语仅承诺单点功能正确而**不**满足选阈值的单份 key 不可区分，则上述混合不成立：泄露 `alpha_{a,c}` 的 key 会使 `y_a-y_c-alpha_{a,c}=key_a-key_c` 泄露数值差；这是精确的最小反例。

## 3. 前向、逆向同一 π 与输出

采用仓库 `apply(pi,x)[a]=x[pi[a]]` 的约定。T 采样独立均匀 `π,σ_0,σ_1,a_0,a_1,h`，令 `τ_0=π∘σ_1^{-1}`、`τ_1=π∘σ_0^{-1}`，`e_0=-τ_0(a_1)-h`、`e_1=-τ_1(a_0)+h`。P0 的前向离线视图为 `(σ_0,τ_0,a_0,e_0,R_0[1,*],key_0[*])`；P1 相应换下标。R1 消息各发 `u_p=σ_p(x_p)+a_p`；收到 peer u 后形成 `s_0=τ_0(u_1)+e_0=π(x_1)-h`、`s_1=τ_1(u_0)+e_1=π(x_0)+h`。R2 各发 `s_p+R_p[1,*]`，公开 `y_1=s_0+s_1+R_0+R_1=π(x)+R[1,*]`。**生成第一轮边 key 的 R 必须就是这两个 R share 之和**；另抽或另开 y 都不满足合同。P0 所见 peer u_1 因 a_1 均匀，所见 s_0 因 h 均匀；P1 对称。R2 中对方 h 与未知 R share 的联合相关性由第 2 节 FSS-IND hybrid 消除后仍是一次性均匀掩码。

逆路由**重新**采样独立均匀 `γ_0,γ_1,a'_0,a'_1,h'`，目标置换严格固定为 `π^{-1}`，令 `δ_0=π^{-1}∘γ_1^{-1}`、`δ_1=π^{-1}∘γ_0^{-1}`，`e'_0=-δ_0(a'_1)-h'`、`e'_1=-δ_1(a'_0)+h'`。给 party p `(γ_p,δ_p,a'_p,e'_p)`，绝不直接给 π 或对方因子。carrier 由完整 CA 排序的公开 shuffled handle 顺序确定：`m[a]=1[rank_full(a)<K]`；P0 输入其加法 share m，P1 输入 0。双方互发 `γ_p(c_p)+a'_p` 一轮，即得 `v_0=π^{-1}(0)-h'=-h'`、`v_1=π^{-1}(m)+h'`，所以相加是原槽 membership。只输出前 n 个 share 的最低位：`(v_0[j]&1) XOR (v_1[j]&1)=m[π^{-1} 的对应槽]`，即原序 XOR mask；dummy 的 stable key 严格排在真实元素后，K≤n，恰 K 个真实位置为 1。任何重构只在 TEST_ONLY。

单方联合置换视图：固定 P0 的 `σ_0,τ_0,γ_0,δ_0`，对每个候选 π，方程 `compose(τ_0,σ_1)=π` 与 `compose(δ_0,γ_1)=inverse(π)` 各有唯一解 `σ_1,γ_1`；二者原本独立均匀，故 π 的条件分布仍均匀；P1 对称。两套 a/h 独立，前/逆在线 peer 第一消息各由未知均匀 a 掩盖；本方逆输出由 fresh h' 均匀掩盖。这个计数与一次性掩码证明覆盖**两套材料合看的视图**，并不借用另一条不相关 shuffle 的 inverse。实际实现仍须验证上述 compose 等式和相关材料序列化/领用不变量。

## 4. 公开泄露与因果轮数

每轮公开 `y_t`、shuffled handle 上的 local ranks `L_t`、公开 pivot 币、pivot、bucket、图、canonical key 查询顺序与活跃边数；协议参数/固定帧长度、错误/abort 时点亦可见。所有 active graph、bucket 和查表序列是 `L_{<t}`、公开币、D/r 的确定性函数；同一不可变 shuffled handle 贯穿各轮，只更新公共子问题列表，不作物理 record 重排，故不产生额外 handle↔原槽映射。若实现有秘密依赖异常、缓存/计时侧信道或向 T 发送在线日志，本归约不覆盖，必须 fail closed 并在验证中检查。协议只主张显式消息 transcript 的半诚实安全，不主张恒时实现。

对于任何唯一全序，隐藏均匀 π 将完整 stable rank 标签均匀分配到 D 个公开 handles；包含 dummy 时，最后 D−n 个 rank 对应随机 dummy handles，真实 rank 在其余 handles 上均匀。以此分布及公开 pivot 币即可模拟整棵 CA transcript，无需原分数。第 2 节 key/y 模拟、第 3 节前逆材料联合置换证明和 fresh inverse h' 输出 share 证明共同说明：条件于任一在线方自己的输入 shares、离线材料及最终输出 share，公开 CA 控制不额外暴露原槽映射。T 持有 π、R 和双份 key，但其进程只发送离线包并关闭通道；不收输入、y、rank、输出，事后与在线方合谋明确排除。

消息 DAG：离线 T→双方包和 ACK（不计在线轮）；score adapter carry→sign 两轮；前向 C 式 `u_p` 第一轮；首轮 masked-open `s_p+R_p[1]` 第二轮；每轮 active-edge 本地 Eval 后 rank reveal 一轮；t<r 时在上一轮 rank 决定公开子问题后再打开新 masked vector 一轮；末轮公开 full sort 后，本地生成 carrier；独立同 π 逆路由仅发一轮 `γ_p(c_p)+a'_p`。在所述新接口真实满足依赖和无额外同步时，CA 核心为 `1+2r=2r+1` 因果轮，raw-score→原序 XOR mask 为 `2+(2r+1)+1=2r+4`。**逆路由一轮**来自上述 C 式只需 share 输出、不需再次公开 masked list，不能把旧 Chase reverse 的两轮照搬。计量上将现有 fixed-clique 三轮/八轮路径与本项目新路径分开；在真实接口完成并 trace 验证前这些是设计 DAG，runtime 轮数记 `NOT_PROVEN`。

## 5. GO 的实施条件与界限

| 门 | 结论 |
|---|---|
| 算法/稳定键/D | 功能证明：`D=max(2,next_pow2(n))`、`b=33+log2 D`、INT32_MIN dummy 后置、CA rank 控制由 E5/E6 TEST_ONLY 对照。 |
| 全池 | 在 FSS-IND、PRG-IND、独立 keygen 币和 fresh 每轮 R 下，第 2 节逐边归约成立；源码本身不证明假设。 |
| 同 π 前向/逆向 | 第 3 节代数和单方联合视图计数成立；新材料必须完全独立且绑定同一 π。 |
| 自适应 transcript | 公开 rank、公共币的确定性后处理；模拟 peer rank share 必须取 `L-q_p`。 |
| 生命周期 | 新包须绑定 version/session/party/n/D/K/r/b/t/pair/material ID；持久独占 claim 在任何在线消息前完成。 |
| 容量 | 先限 `D≤8`、`1≤r≤5`，保留旧百万边/64 MiB cap；`n=10^5/10^6` 完全未验收。 |
| SMALL_D_RUNTIME_GATE | **GO（条件性项目实现门）**；若无法实现上述绑定/持久 claim/独立 inverse，立即回退 NO-GO，不降级为固定 M 或在线 T。 |

验证顺序：独立 API 的 package/代数 conformance → 冻结 oracle differential → T/P0/P1 独立进程 E2E → 相关旧 Protocol I/III 回归 → `git diff --check`。必须记录实际 revision、命令、输入/种子、因果 trace、每方发送字节、预留/活跃 key 和在线 Eval；未测时间/峰值内存写 `NOT_MEASURED`。此设计门没有运行上述新路径，不能称小规模功能验收已通过。
