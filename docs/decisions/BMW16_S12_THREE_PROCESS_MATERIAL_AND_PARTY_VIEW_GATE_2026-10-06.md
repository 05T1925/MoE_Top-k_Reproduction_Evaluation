# BMW16 S12：三进程实验组合、逐方材料与单方视图门

- 日期：2026-10-06
- 方案身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED`
- 实现标签：`BMW16_S12_CPP_TP0P1_EXPERIMENTAL_TEST_ONLY`
- 接收对象：S11 `43c3c657fb370e73588b161f552b0012b0716d6c`
- 工作分支：`codex/m6b-i-bmw16-s12`
- 决定：**三进程实验功能与本地逐方材料检查通过；组合安全门仍 NO-GO；不创建 secure alias，不启动 V4 正式矩阵。**

## 1. 接收和来源

| 对象 | 冻结身份/处理 |
|---|---|
| `main` / `origin/main` | `c3926c68fd14f270faa8b55234311071947fa080`；是 S11 的祖先 |
| S11 | `43c3c657fb370e73588b161f552b0012b0716d6c`；S12 从这个接收提交的隔离 worktree 起步 |
| S4 算法 | `04ed6f8a352ab2277434344094471dff82f83e4b`；冻结源 `select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| S11 实现 | `VFSS` opt-in C++ party API；S12 原地扩展材料序列化、测试 tape 和进程 runner，不再另写一套 Select |
| E20 / E21 | `8986a40175f5f8eef56d765fef0cfa863e205985` / `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4`；只检查接口和来源，没有把其接收结论移植为本方案安全证明 |
| PR #28 | draft head `57e239ee820a4208ac606663d877803f0b692206`；未修改 |
| 桌面主工作区 | 预存文档、证据及本地 PDF 差异保留；S12 仅在 `.codex/worktrees/m6b-i-bmw16-s12` 工作 |
| BMW16 PDF | 沿用 S11 登记：44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9B79BACEC1259CA9CA23`；仅作 S4 推导来源，不继承 Theorem 8 |

S11 的报告和哈希清单是历史证据；S12 的构建、共同 tape、分方进程矩阵和输出哈希是本轮新证据。S11 原先的 8 个成功、1 个注入中止以及线程级结果不替代本轮结果。本轮修正 S11 计量中的预发槽表达：每方总槽数为 `2*padded_n + 2 + 11*C(n,2)`；n=64 为 22,306 个。两份 shuffle CmpAgg 池各 2,016 个，合计 4,032 个，实际路径未调用，现如实计入预发和未用材料。

## 2. 复用的完整候选路径

S12 复用 `protocol_i_bmw16_experimental_raw_score_mask_party`，只补齐它的离线封装和独立进程调用：

```text
两方 raw signed Q20.12 加法份额
  → S10 正式 Protocol I raw-score party adapter
  → 同一隐藏置换的前向 shuffle
  → 两个共享真实匿名句柄的 S4-derived Select task，各自 R1–R4
  → 两哨兵交集得到匿名 selected record
  → 真实 stable-key uCMP/DCF membership: R_i ≥ R_selected
  → 同一置换的 inverse shuffle
  → 原序 XOR mask shares / 或一致算法 abort（无 mask）
```

S4 的冻结算法与 C++ trace 现已由外部 TEST_ONLY tape 对齐。每一轮两 task 的边批次先生成并冻结，再求值和打开本轮结果；后轮计划只在先前结果到达后产生。同轮反向请求通过规范无序 pair 缓存单次 key Eval，但仍保留全部逻辑比较调用计数。跨轮、跨 task 和 membership 使用不同预发 key，不共享 key。selected anonymous handle 不作为 API 返回值或日志字段，但双方能够从已公开的两份 partition 与 comparison transcript 推出它；安全视图审计按“已知于双方”处理，而不是把省略返回字段当作保密。

n=1 是明确的恒等分支：因合法输入必有 `K=1`，两方分别返回固定 `1/0` mask share，不执行 raw adapter、shuffle、Select 或材料领取。其余 n 范围限于本候选明示支持的 n≤64。

## 3. 随机币和 Select 等价核对

正式 EXPERIMENTAL party 路径不再使用 S11 的固定公开 Select seed。P0/P1 各用 OS `getrandom` 取 32 字节随机贡献，在独立的采样币消息相位互换并异或。异或结果经 task、抽样域、计数器域分离的 SHA-256 counter stream 产生候选 64 位字；拒绝映射消除模偏差，无放回抽样沿用 S4 的 pool-replacement 更新次序。假设是半诚实方按协议产生不可预测随机量以及 SHA-256 counter stream 的伪随机性；这是项目采样构造，不是 BMW16 定理保证。两方会看到合币结果和抽样/transcript。

确定性逐轨迹比较只允许走显式 `ProtocolIBmw16TestOnlyRandomTape` 参数。Tape 是外部文件 `BMW16_S12_TAPE_V1`，含每个 task 的 R1/R3 两域原始 64 位字；同一文件被 C++ 和冻结 S4 Python 读取。映射、无放回集合更新、pivot、U/V/W、四层边、结果、状态和 selected handle 均逐项比较。S12 对 n=2..8 的每个 K 均完成一例严格全序共同 tape 差分，共 35 例；另有 n=64 四种输入 profile、K=1/8/32/63 的 4 例共同 tape，另行保存 n=64 的额外 tape 与 zero/max/alternating 词流。总计 39 个外部 tape 案例全部 PASS。C++/Python 随机库 seed 相等不是比较标准；标准是实际共享 tape 字节相等。

共同 tape 与 party API 固定 OS 随机分支有意隔开。n≤64 的进程 E2E 使用 fresh OS 合币，不记固定算法 seed；其中命令行 `seed` 只用于 TEST_ONLY 输入份额切分、会话命名和 fixture 复跑。

## 4. party-only bundle 和领取合同

新增 bundle v1 由可信离线 T 只依据公开配置生成：session、fingerprint、party、n/K、padding、index bits、comparison bits、stage、task、round、规范端点和顺序化唯一 slot ID。每个接收方只收到自己的 whole-party 包。manifest 中的 slot 元组先 SHA-256，再将该 digest、会话参数和包头作为 AES-256-GCM AAD；GCM 保护 payload 和 key bytes 的来源完整性/保密。header 为 100 字节，tag 为 16 字节，故 n=64 每个 bundle 的认证 envelope 元数据是 116 字节。包写入使用临时文件、`fdatasync`、原子 rename 和目录 `fsync`。消费端先做持久 bundle claim，随后在每次 key Eval 前对 slot 写 `O_EXCL` marker 并同步目录；进程重启后 duplicate claim 被拒绝，失败不回退到在线 keygen。

这份实验代码准确提供的是 **AES-GCM 包封装 + 测试机私有目录交付**。S12 root TEST_ONLY harness 预置逐方密钥并把已加密包写到不同 UID 私有目录；真实生产 T 身份凭据配置、端到端私有认证网络/存储服务、证书/PSK 建立与轮换仍未实现。AES-GCM 认证“谁持有该预共享密钥生成的这些字节以及字节是否被改动”；端点和阈值间的数学关系由可信 T 的材料生成流程及密钥库保证，包头校验不能证明 FSS key 的内部函数正确。

共执行错接收者密钥、错 party、错 session、tag 截断、密文/slot manifest 篡改、错 comparison width、bundle 重领、slot 重领、进程退出后 marker 持久、peer 已关闭及 peer 静默的负例；均拒绝或由存活方本地超时中止。peer 不 ACK 时只报告本地方 `LOCAL_ONLY` abort，不声称双方达成中止共识。没有声称测试了硬断电时的文件系统持久性、真实证书发行或网络攻击者。

## 5. 独立身份和 T 时序

三进程 harness 为 T、P0、P1 分别 `fork/exec`。T 进程只收到公开 n/K/session/width 配置及本地测试凭据路径，创建 raw adapter、前向/逆向 shuffle、每个 task/round 的 Select、membership 的全部离线材料和逐方 AEAD bundles；T 进程退出并被父进程 `wait` 后，父进程才按 fixture 分割 raw score shares。P0/P1 分别以 WSL Linux UID 22012、22013 执行，打开各自 raw share、AEAD key 和 bundle。双方各自实际尝试读取对方上述三类文件，得到 `EACCES/EPERM`。

父级 TEST_ONLY harness 仍为 root，能生成并在最终 oracle 处重构 mask；它也创建预共享 AEAD 凭据、写包和 fixture。此权限不能计为任何 party view 的安全论据。P0/P1 的正常 party API 没有读取对方目录的路径；输出 share 只在父级测试控制器的 oracle 检验后清理。该方案尚非真实多主机私有网络协议。

## 6. 计数合同与实际样本

n≥2 成功执行的因果消息相位：raw adapter 2 + forward shuffle 2 + 两方合币 1 + Select R1/R2/R3/R4 各 1 + inverse shuffle 2 + 状态协商 1 = **12**。算法 abort 跳过 inverse shuffle，因而为 **10**。n=1 恒等路径为 0。Select 仍只是其中四个依赖层，不是完整 Protocol I 入口四轮。membership 的 uCMP/DCF Eval 本地执行，不额外交换比较结果。

n=64、K=8 的本轮 loopback 样本：

| 计量项 | 实际观察 |
|---|---:|
| 状态 | SUCCESS；冻结 C++ oracle mask 逐位相同，weight=8 |
| Select 逻辑请求 | 17,757（四轮分别 7,310 / 452 / 9,984 / 11） |
| Select unique key slots consumed | 5,557；dummy-related logical calls 12,200；repeats 0 |
| membership slots | 63；自比较本地为 1 |
| uCMP party Eval / DCF party Eval | 5,748 / 11,496（每个 uCMP 两次 DCF Eval） |
| party message phases | 12 |
| 各方 sent / received | 150,857 / 150,857 bytes；两方交叉恒等 |
| 每方 slots | 22,306 预发，5,750 消费；4,032 个 shuffle CmpAgg 未用 |
| 每方 AEAD bundle | 22,770,793 bytes（明文 22,770,677；manifest 446,128；ciphertext 22,770,693；header+tag 116） |
| T 本地离线生成/封装/写盘 | 355,042 μs，本单次本机观测 |
| party API 计时 | 约 10.34 s 单次 loopback 观测 |
| sampler PRF words | 328 SHA-256 stream words/方 |
| 其他 PRG、峰值 RSS、正式网络/五次重复统计 | `NOT_MEASURED` |

以上 n=64 数字来自一次特定 CSPRNG/shuffle run，是可核查冒烟数据，不是性能表或 LAN/WAN 测量。预发与未使用槽拆分：8 个 Select 全 pair pool 中未访问槽、membership 未访问槽、以及两个 shuffle CmpAgg pool；不把逻辑比较次数、FSS key slots 与 DCF Eval 相互替代。完整原始计数见 [三进程 evidence](../reproduction/evidence/BMW16_S12_TP0P1_E2E_2026-10-06.txt)。

共同比较账本的内部恒等式检查包括 `DCF Eval = 2 × uCMP party Eval`、发送量等于 peer 接收量、slot claim 等于成功/abort 路径预计领取数、预发槽符合 `2p+2+11*C(n,2)`。35 个小规模共同 tape 全部对齐，n=64 附加 tape 也逐层对齐。具体失败统计是 52 个进程 E2E SUCCESS、1 个**注入** probability abort、0 个自然 probability abort；所以没有可报告的抽样失败率，也没有自然 abort 的分方 E2E 样本。

## 7. 联合视图与安全裁决

S12 将允许泄露草案按最强实际 transcript 处理：公开 n/K/session 参数、公开 masked operands、匿名比较图和结果位、采样与 pivot、U/V/W、两个任务状态、推导可得的 selected anonymous handle、算法 abort 类别和通信错误类别。只给定理想随机置换下“匿名句柄对输入重标号等变”仍不足以模拟每个腐化方的完整视图。

对腐化 P0 或 P1，需要模拟本方 raw input share、正式 adapter 的两轮消息、本地前/逆 shuffle state 与消息、共用真实句柄在两个 Select task 中的相关性、全体相关阈值下产生的完整本方 uCMP/DCF key pool（包括未访问 key）、公开 masked operands、依赖打开位选取的自适应边与材料 claim、selected handle、abort，以及原序 mask share。当前缺少的**第一处未闭合 hybrid**是：将真实 Protocol I shuffle party state/transcript 与这些共享节点 mask 下的多 key、相关阈值、自适应开值 pool，一起替换为只由明示泄露与各方输出份额驱动的可计算模拟。当前接口和仓库材料未提供可引用的“相关阈值、整池、多 key、自适应打开比较位”联合安全定理或优势界。单 key DCF 安全、每次独立 KeyGen、n≤8/64 有限枚举、文件 ACL/AES-GCM 均不推出该命题；S12 也没有找到“匿名 transcript 必然泄露”的反例，因此结论是 `NOT_PROVED`，不是不可能性证明。

此外，生产级 party 身份凭据和认证的 T→party 网络/存储交付仍缺失。功能结果正确和独立 UID 文件访问通过不能把这两项提升为 secure。

## 8. 门禁与下一步

| 门 | 判定 | 理由 |
|---|---|---|
| VFSS 完整函数路径 | `PASS_FOR_TESTED_CASES / EXPERIMENTAL` | 复用 S11 C++ party API，三进程完整链，oracle 逐位一致；52/53 成功，其余为注入 abort |
| S4/C++ 逐层随机 tape | `PASS_FOR_TESTED_CASES` | 外部 tape，n=2..8 所有 K 共 35 例，n=64 补充 tape |
| 逐方 bundle 封装与耐久 one-shot | `PASS_FOR_LOCAL_TEST_CONTRACT` | AES-256-GCM/AAD、manifest、原子落盘、bundle+slot claims 和列明的负例 |
| T/P0/P1 进程与 UID 隔离 | `PASS_FOR_WSL_TEST_HARNESS` | T 先退出，raw shares 后创建；P0/P1 UID 不同且互读文件被拒 |
| 认证的生产 T→party 私有分发 | `NO-GO` | 只具备本机测试凭据和加密 bundle 写入；无独立认证凭据建立/轮换与正式传输端点 |
| PARTY_VIEW_SECURITY | `NO-GO / NOT_PROVED` | 实际 shuffle + 全相关自适应 DCF/uCMP pool hybrid 尚缺 |
| 自然抽样 abort 分方样本 | `NOT_OBSERVED` | 52 个随机成功加一个注入 abort；0 自然 abort，不由此声称失败概率 |
| V4 正式性能 | `NOT_STARTED` | 安全门、生产材料传输和独立计量接收均未通过 |

最小下一项不是再增写一般性安全说明，而是针对本实现给出或实现可引用的**相关输入多 key 自适应 FSS 模拟接口/定理**：覆盖两个任务共用真实句柄、共享 mask 诱导相关比较阈值、公开 masked operand、整池已用/未用 key 及 abort/output share；由独立审查确认后再讨论 secure 名称。并行的工程前置任务是生产 party 凭据/认证私有 bundle transport conformance。此前候选保持 `EXPERIMENTAL / TEST_ONLY`。
