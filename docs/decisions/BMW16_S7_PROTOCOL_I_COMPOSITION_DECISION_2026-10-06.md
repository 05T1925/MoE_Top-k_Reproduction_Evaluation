# BMW16 S7：Protocol I + BMW16-derived Select + DCF 决策

日期：2026-10-06
状态：`FUNCTIONAL_TEST_ONLY_GO / PARTY_VIEW_SECURITY_NO_GO`
标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED`

## 决策范围

用户明确将六方案中的 Protocol I 选择升级候选改为 **Protocol I + BMW16-derived Select + DCF**。此决定授权隔离研究和 TEST_ONLY 功能组合，不把方案称为 BB90，不继承 BMW16 Theorem 8 的概率或比较常数，也不声称完整安全入口只有四轮。S7 只有集中式 TEST_ONLY 组合；由于联合 party-view、离线覆盖与安全逆路由未闭合，不在 `VFSS/` 创建 secure 入口。

本文件承接 S4 算法及验证、S5 独立接收、S6 组合审计，保留三者历史结论。S4 代码从 `04ed6f8a352ab2277434344094471dff82f83e4b` 的对象库选择复制，原 SHA-256 为 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`；S5 为 `67c3f302a8564b68616f7d7247c8cee3415ace31`；S6 为 `c4f34e7e8f9fd954637573fe62b8d9b567362298`。本分支从 `main@c3926c68fd14f270faa8b55234311071947fa080` 建立。E20 流式材料实现只作接口参考，没有把它的功能验收转作本方案安全证明。

## 来源分层

| 主张/对象 | 身份 | 决策 |
|---|---|---|
| BMW16 noiseless comparison model、论文算法与定理 | `PAPER_DIRECT` | 仅在原文适用范围内引用；本项目 S4 样本括区不是其直接实现 |
| S4 四轮 sample-bracket Select 与有限规模处理 | `PROJECT_DERIVED` | 延续标签 `BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`；S5 接受仅限既定功能证据与边界 |
| 稳定 raw-score key、两个 Select 交集、DCF 成员及 mask 输出 | `PROJECT_DERIVED` | 需各自给出编码、绑定和正确性证明；不由选择论文直接保证 |
| `ProtocolIUcmpMaterial`、并行 shuffle、C++ frozen top-k oracle | `LOCAL_REFERENCE` | 作为被调用代码/验证 oracle；实际执行行为不自动证明协议安全 |
| TEST_ONLY 集中式 raw-score→mask 原型 | `S7_TEST_ONLY` | 功能与实际 Eval 可测，打开 shuffle map/比较份额，不能部署 |
| party-view 安全、输入无关离线自适应边材料、secure 逆路由 | `UNPROVEN` | 阻塞 secure 入口 |

主要本地 PDF 身份：`Papers/1603.04941v1.pdf`，44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；`Papers/017.pdf`，249 页，SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15`，只作交叉核对。两份 PDF 未进入 Git。

## S6 复核裁决

### n=2 可见端点反例

如果比较端点可映射回原始记录 ID，S6 反例成立：令腐化 P0 的两项加法份额在两执行中均为 `(0,0)`；P1 的输入分别诱导 `0<1` 与 `1<0`。公开比较位两执行分别为 1 和 0。理想 Top-1 mask 分别为 `[0,1]` 和 `[1,0]`；P0 的理想 XOR 输出份额在两世界都在四个 2-bit 向量上均匀，因此该公开位即使条件于相同 P0 输入份额与输出份额仍可区分执行。

这不能直接推出“匿名句柄上的比较位泄露”。独立枚举器固定 n≤4 的严格隐藏总序、固定无向句柄边集，并遍历每个均匀随机句柄置换：对 n=2 的单边 transcript，两种真实顺序下均为 `{0:1,1:1}`；n=3 所有 8 个边子集、n=4 所有 64 个边子集的 transcript 分布对所有秘密顺序相同。故 S6 若把“原记录端点”与“未知均匀句柄”混为一谈，结论过宽。

枚举只覆盖固定图上公开比较 transcript 的边际分布，不覆盖自适应图、本地份额/置换、两实例共享真实句柄、相关材料、访问/未访问 key 槽、abort 或 mask 输出份额的联合分布。它既不是联合模拟器，也不是 PARTY_VIEW_SECURITY 的正面结论。实际 TEST_ONLY 代码会打开 shuffled identity 和比较位，故本原型直接泄露这些值。

### uCMP 方向与输入位宽

实际 `ProtocolIUcmpMaterial::eval_strict_lt` 重构值为 `c=[x<y]`，以 `Z/(2^64)` 分享。对严格互异键，反向方向可用本地变换 `(c0,c1) -> (1-c0 mod 2^64, -c1 mod 2^64)`；两方和为 `1-c`。对成员谓词 `x>=threshold`，该式在相等时也正确，因为 `1-[x<threshold]` 包含相等；它不能被误写为反向严格小于，后者在 x=y 时为 0。

S6 的 key 数值上界为 `<2^(33+b)`，其中 `b=max(1,ceil(log2 n))`。但当前 uCMP 的差值校正使用最高输入位 `h=2^(Bin-1)`，实测可用非负输入需严格小于 `h`；于是实际比较位宽需 `Bin=34+b`，而不是数值位宽 `33+b`。真实 DCF 反例：n=2/K=1，Bin=34，比较 0 与高哨兵 `8589934593`，数学上应为 1，当前 uCMP 实际重构为 0；Bin=35 时实测为 1。修正配置：n=128→41，256→42，1000→44；n=10^6→54，超出当前 34..53 API，故不支持。

TEST_ONLY 稳定真实编码为 `b=max(1,ceil(log2 n))`，`Q(s)=(uint32(s) XOR 0x80000000)`，`R(s,i)=(Q(s)<<b)+(n−1−i)`。R 递增等价于 score 递增、同分 original_index 递减；因此从高到低取序时优先级是 signed score 降序、original_index 升序，且 n 个真实 R 唯一。令 `Rmax=(2^32−1)2^b+(n−1)`，padding 为 L 个低 dummy、H 个高 dummy；测试 total-order code 为低 dummy `j`→j，低 sentinel→L，真实元素→L+1+R，高 sentinel→L+Rmax+2，高 dummy `j`→L+Rmax+3+j。dummy 通过类别位置与真实 R 的偏移严格分离，不占真实输出位。该整数算术只发生于明文 TEST_ONLY；安全份额编码仍未实现。

规范方向每条有向请求按实际 uCMP 参数生成 key；同一因果轮中同一无序边的反向逻辑调用可在 TEST_ONLY 复用一次结果并作上述 share complement，但逻辑调用仍分别计数。此优化仅限同轮同任务且端点顺序规范化；跨轮、跨 Select 实例、跨 membership 阶段不缓存。不同材料槽须绑定 session/party/endpoint/round/instance/stage/mask/slot，并一次性消费。此处是测试适配约定，不等于输入无关离线材料覆盖已经实现。

### D 与容量

`D=next_power_of_two(2M-n)` 是把两个实例的共享真实句柄及私有 dummy 合并后做全局填充的保守存储 envelope；不是每个 Select 活跃比较图的必要规模。S7 对两份 Select 每实例 M 域的有向最坏池使用 `8M(M−1)`，另将未知阈值的 membership 预发池 `n(n−1)` 独立相加。无向减半只有在 reverse-share transform、方向域和同轮访问合同均证明后才可用，不能从逻辑边成对出现自动推出。

以 n=1000/K=80：M=1842，H=2M−n=2684，D=4096；全局 D 合同池 `8D(D−1)+n(n−1)=135,183,960`；按活跃 M 池为 `8M(M−1)+n(n−1)=28,127,976` 有向槽。修正 Bin=44，S6 每槽解析序列化模型为 1153 bytes，对应活跃池每方 32,431,556,328 bytes 的理论下界模型；这不是生成/落盘/性能实测。现有材料包 1,000,000-edge cap 已小于此池。n=128/K=2 活跃池 530,352；n=256/K=2 为 2,142,000。n=10^6 不给容量字节，因为所需 Bin=54 不受当前 API 支持。

| 句柄合同 | 两哨兵交集 | 隐私/材料结论 |
|---|---|---|
| 全局 H 实句柄并 pad 至 D | 共享不透明真实 ID 时保留 | D 全池过宽；图隐私未证明；key ID 必须含实例/轮/阶段 |
| 两个不相交 M 局部命名空间 | 不可直接交集 | 需另做秘密跨实例真实记录 join；图隐私未证明 |
| 共享真实句柄 + 实例局部 dummy 地址 | 真实项可交集 | dummy 分域；公开实项交集可能暴露真实 cohort；仍需联合模拟 |

未选择这三者中的任何一种作为 secure 合同。`D=4096` 既非安全证明也非当前活动池最小值。

## TEST_ONLY 组合和成功语义

程序运行两个 Select 外层实例、两哨兵交集、实际仓库 shuffle state machines、uCMP/DCF、DCF membership、原序 XOR mask shares，并调用冻结 C++ `top_k_mask` 只作验证 oracle。测试控制器集中持有明文分数/份额、打开 shuffled→original 映射及比较结果，并在选出阈值后按明文方式 scatter；这些步骤是明确泄露，不可升级成 secure protocol。输入 n=1 的 singleton identity 仅为 TEST_ONLY 特例。

成功函数不变量为：稳定键 `R_i=(signed_score, original_index)` 的 total order 决定唯一 K-th；成员谓词必须是 `R_i >= R_selected`（按“优先级更高”方向具体编码），从而成功时恰 K 项。S7 集成夹具按真实 uCMP 校验该函数路径；没有实现 party-separated DCF 参数绑定、hidden-handle join 或 secure inverse route。失败不返回 mask；现有小矩阵未触发概率失败，不能视作中止概率界。

## 门禁

| 门 | S7 判定 |
|---|---|
| `FUNCTIONAL_COMPOSITION` | `PASS_TEST_ONLY_ONLY`：10/10 冻结样本与 C++ oracle 一致；不能推出安全性 |
| `PARTY_VIEW_SECURITY` | `NO_GO / UNPROVEN`：精确缺联合模拟/混合引理及输入相关材料访问、输出份额证明 |
| `SMALL_N_SECURE_RUNTIME` | `NOT_IMPLEMENTED`：安全门未过；VFSS 未改 |
| `FULL_PERFORMANCE` | `NOT_RUN`：无正式 LAN/WAN 数据 |

下一阶段若要申请 secure 入口，首先须提交可审查引理：对每个腐化方，其联合视图 `(local input shares, local shuffle state/messages, two-instance common handle relation, all material keys plus read/unread access trace, adaptive public transcript, abort class, output shares)` 在同一理想功能泄露下可模拟；同时证明预处理不依赖输入和未来图、或给出具体的安全在线材料获取协议，并闭合同置换秘密逆路由。只证明匿名固定图 transcript 边际均匀不满足该门。

## 复跑与记录

详细环境、运行命令、失败及 JSON 证据见 `docs/reproduction/BMW16_S7_COMPOSITION_AND_SECURITY_AUDIT_2026-10-06.md`。源代码位于 `experiments/TEST_ONLY_BMW16_S7_COMPOSITION/`；全部证据由该目录程序生成。正式 secure API、统计中止目标 ≤10^-6、安全 PRG 假设、分方离线字节与正式协议因果轮数均保持待定，不由这份决策默认通过。
