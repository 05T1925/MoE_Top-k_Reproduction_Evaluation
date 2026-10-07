# BMW16 S17：独立 party 节点、材料根绑定与流式侧车决策

日期：2026-10-07
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
起点：S16 报告提交 `f3f20cc7afb742ecfb43acd972ff8d8d37664ffd`
被复用的 S16 核心源码：`a95d7cec8105da0c418a465d96f314830a873630`

## 1. 决策摘要

S17 继续复用 S16 的 VFSS C++ raw-score→shuffle→两份四层 Select→uCMP/DCF membership→同置换 inverse→原序 mask 路径。新增默认关闭的独立 T / party-node 命令行接口、v3 bundle 的 claim-root 绑定、Select/membership 的随机访问密文侧车，以及流式槽读取 conformance。最终独立进程 E2E 现覆盖 n=1/2/5/8/64/128/256；n=256 的 C++/S4 common-tape differential 也复跑通过。

**功能门通过到已测域，密码学安全门没有通过。** 本地 T→party 文件落盘与分 UID 进程路径是 EXPERIMENTAL fixture；节点程序消费调用方注入的已连接 FD，不实现部署级私有认证网络。随机访问侧车仅流式读取已由现有 KeyGen 完整驻留内存的 Select/membership key pool；它不是 n=1000 流式 KeyGen，也不降低峰值 KeyGen RAM。生产 secure API、正式 V4 网络矩阵、六方案排名均不开放。

## 2. 来源、身份和改动边界

- 工作位于从 S16 文档提交新建的隔离分支 `codex/m6b-i-bmw16-s17`；基线 main / origin/main 为 S16 记录的 `c3926c68fd14f270faa8b55234311071947fa080`，工作起点为上述 S16 文档提交。
- S16 算法与协议实现的已声明源 revision 是 `a95d7cec…`；本分支是从 S16 报告提交继续，不把之后的报告提交冒充成被测源码版本。
- E20 的流式工程仅作接口思路参考；S17 未复制 E20 密钥布局或其 AAV86 接收结论。
- 未修改 `VFSS-baseline/`、论文、AAV86/E20/E21、桌面主工作区、本地参考树或 PR #28。随机材料、party bundle、构建物和原始逐次日志均在仓库外。
- 原论文 BMW16 与本项目采样/归约继续区分；不引用 BMW16 Theorem 8 给本组合的概率或比较常数，不称 BB90 复现。

## 3. T / P0 / P1 实验调用合同

`moe_topk_bmw16_experimental_party_node` 提供两个子命令：

1. `t` 仅接受公开 `n,K,session,fingerprint,claim roots,输出 bundle 路径,party 凭据路径及已配置 UID`。它调用现有 T 离线生成，封装 P0/P1 分方包，将各包交给相应 UID，完成后退出。在线输入在节点 E2E 中于 T 退出之后才创建。
2. `party` 只打开当前 UID 所有、模式受限的 raw-share、bundle、凭据和 claim-root 文件；验证 party/session/fingerprint/n/K/padding/比较位宽/材料清单与 v3 根身份；先对整个 bundle 写入并 `fdatasync` / 目录 `fsync` 的 burn marker，然后进入 C++ party API。进程内槽检查继续用于重复访问拒绝。
3. party CLI 消费 12 个 Protocol I 阶段 FD，并在进入 API 前通过额外 framed `READY` barrier（第 13 个 FD）同步配置。FD 由调用方提供，节点程序明确假设它们已属于私有认证传输；本地测试只用 Unix socketpair。Ready 相位是工程启动同步，不是论文 Select 比较层或部署网络验证。
4. 返回合同保留 `SUCCESS=0`、算法 abort `10`、材料 abort `20`、通信 abort `30`、意外错误 `70`。仅 SUCCESS 写出 party mask share。无 ACK 时只报告存活方 `LOCAL_ONLY` abort，不声称双方共同中止。测试控制器在 parties 退出后才重构 mask 并核冻结 oracle。

### Bundle 和一次性范围

bundle v3 把 claim-root `device,inode` 放入 authenticated header/AAD；v2 仅保留给无持久 claim 的内存 TEST_ONLY fixture。party 根目录必须预先属于本方、不能由另一 UID 读取；party bundle 以每方独立 AES-256-GCM wrapping key 加密，外层元数据与 slot manifest 绑定 session、party、n/K、padding、width、stage/task/round/端点及 slot id。GCM 证明密文字节/AAD 完整性；KeyGen 才定义 key 的内部数学关系。

S17 选择合同 A：**只支持一个固定、受保护、不可回滚的 claim root**。同一 bundle 复制至不同 inode/device 根被拒绝；同根 duplicate claim、槽重领、burn 后崩溃再启动被拒绝。代码不能阻止文件系统快照回滚、root 克隆或管理员绕过，因此 `ONE_SHOT_SCOPE` 仅在环境保证根不可回滚且 bundle 不迁移的条件下成立；不宣称跨 store anti-replay。

在独立部署中，party 凭据需由已信任的配置流程预置并由独立 OS 身份持有；T 当前把 bundle 交到本地 root 管理的 party 目录。生产 T→party 网络认证/密钥注册未实现，不能把 root harness 里预置密钥当成部署方案。

## 4. 流式材料接口的准确范围

新侧车以 `BMW16STR` v1 固定头和 9 个 Select/membership pair-key 池的规范槽顺序写入随机访问文件。每个槽记录 AEAD tag，AAD 包含 bundle 配置/根身份及 `(slot id,stage,task,round,left,right)`；party open 检查精确文件长、当前 UID、文件模式、header 参数、根身份与槽数；`load_once` 通过 `pread` 随机读取、认证并拒绝同进程重复读取。data key 和其明文 wrapping buffer 采用 RAII 清零。

其界限：

- `protocol_i_bmw16_experimental_material_generate()` 仍构造全部 raw、shuffle、8 份 Select 和 membership key 后才返回；S17 sidecar 的输入就是此完整内存对象。
- sidecar 只外置 Select + membership uCMP keys；raw-score 与 forward/inverse shuffle 材料仍留在既有 bundle/内存接口。
- 单元路径核对所有 9·C(n,2) serialized pair keys 与内存来源逐字节相同，覆盖错误 root、AEAD tamper、truncation、重复读；同输入/同 TEST_ONLY 算法 tape 的独立完整运行分别 oracle 检查。因为两个完整运行必须 fresh 生成并且 shuffle replay guard 会拒绝复用同一 shuffle package，S17 不声称跨两次运行具有相同完整消息 trace；相同 source pool 的逐槽字节相等是存储差分证据。
- 因此这是一种 **随机访问存储 / lazy read 实验侧车**，尚非顺序流式 KeyGen。它不改变二次方槽数，不降低离线 KeyGen 峰值 RAM，不改变 n≤256 API 上限，也不让 n=1000 获准。

未来 n=1000 writer 需给 `keyGenDCF` / uCMP 加单槽生成输出接口：T 按规范 `(task,round,lo,hi)` 顺序每次生成两方 key、立即各自 AEAD seal、写两方不同文件和 manifest；完成全池、`fsync` 并校验总槽数/哈希后才发布完成标记和离线包。party 要在任何在线 Eval 前完成整包 durable claim，再仅凭 `(session,party,stage,task,round,canonical endpoints,width,slot id)` 做常数内存 `pread`；一个坏块统一触发材料 abort 且无 mask。该 writer 必须计量全部生成/包/磁盘字节，验证随机槽读序、根绑定与 restart/replay，并复查整池视图；它不能因 streaming 忽略未用 key 或被读槽相关性。

## 5. 组合安全审查：条件化 hybrid 与首个缺口

设 `Q=9·C(n,2)` 个真实端点 pair-key 槽（每个端点对在两个 task×四轮及 membership 共九份独立 `ProtocolIUcmpMaterial`）；另有 raw adapter `2·padded_n` 个输入槽及两份 shuffle 材料。对每方实际联合 view，包含：本方 raw input share；forward/inverse shuffle 的 `sigma/tau/a/e/r_share` 与消息；`public_z`；两个 task 共用的真实匿名 handle；全部已用与未用 party DCF/uCMP keys；节点 mask 差形成的相关阈值向量；本方 Eval shares；公开比较边/比较 bit、pivot/U/V/W/访问顺序、selected anonymous handle、abort、inverse transcript 与本方 mask share。

拟议 leakage function 暂列公开 `n,K,session,width/padding`、`public_z`、匿名 R1–R4 图及打开的比较位、pivot/U/V/W、材料访问类别/数量、selected anonymous handle、abort 类别和协议流量；理想输出为两方各自原序 XOR mask share。真实 scores、original index、rank、threshold 和完整 mask 不应额外公开。这个公开图/bit 清单是安全分析的候选 leakage，**尚未由项目隐私负责人批准**。

可检查的混合如下：

### H0 → H1：FSS PRNG 流

源码 `protocol_i_parallel_shuffle.cpp::seed_fss_once()` 为 256 个 `FSSConfig::prngs` 用 OS `getrandom` 种子。`keyGenDCF()` 在所选 OpenMP thread 上读取两个 PRNG blocks，再以 AES 节点扩展生成 correction material。若这些种子不可预测/不回滚，且每线程实际消耗块数可界定，则把每线程 PRNG stream 换成独立随机 tape 的区分优势可上界为 `Σ_tid Adv_PRG(Q_tid)`。S16/S17 的 online ordinary DCF expansion counter 不覆盖所有离线 KeyGen、shuffle 与系统库 PRG，故本报告没有可代入的全路径 `Q_tid` 实测值；此步是条件式，不是已关闭引理。

### H1 → H2：全池相关阈值 DCF

S17 不因 `alpha_ij = r_i-r_j` 在九个池重复相关就直接判不可能：若每次 `keyGenDCF` 的随机 tape 在 H1 后相互独立，相关阈值向量可整体固定，逐槽 hybrid 在逻辑上可以成立。若已有单 key 定理适用于**任意 auxiliary input 与 hidden threshold 相关、party 完整 key 和该点局部 Eval share、并允许自适应公开 query/bit transcript 的输出-aware game**，且候选向量保持同一允许 leakage，则顺序替换至多 `Q·ε_one-key`（加 PRG stream 替换优势）；raw/shuffle key 池还要单独计数。前序公开比较 bit 由两方 Eval share 合成，是规定 leakage 的后处理；模拟器仍必须联合模拟 corrupted party 的 key pool、Eval share、公开 bit 与后续自适应访问，而不能只证明序列化单个 key 的边际分布。

仓库 `keyGenDCF/evalDCF` 代码展示实际算法，不附有上述 auxiliary-input/adaptive/multi-key 定理或完整安全游戏。尤其 public masked operands、各方本地 shuffle helper 与 `alpha`/mask share 有共同来源；没有来源证明它们在单 key 隐私游戏里满足 required auxiliary-input independence。**第一个未闭合 hybrid 是 H1→H2**。欠缺的是针对当前 `ProtocolIUcmpPartyMaterial` 序列化格式、`eval_strict_lt` 两次 DCF Eval shares、相关阈值、整池未用 key及 adaptive leakage 的一方安全定义/定理及其归约；“每个 key 单独安全”措辞不足以替代它。

### H2 → H3：Protocol I shuffle / inverse

还需要一个模拟器把本地 `sigma/tau/a/e/r_share`、两阶段 shuffle 帧、公开 `public_z`、同一隐藏置换与 inverse output-share 联合起来，并能同 S4 transcript/selected handle 组合。已执行的 shuffle conformance/roundtrip 只证明功能合同，不证明该模拟器。AES-GCM、匿名图有限枚举、party 文件权限均不关闭该步。

所以 `CONDITIONAL_SECURITY=NO_GO`，但不是“已证明不可能”。最小可审查修正不是加随机 domain separation，而是拿到/给出一个以上述 auxiliary-input output-aware transcript 为条件的多 key DCF 定理及实际随机性来源；若定理不成立，再考虑将 opened bits/adaptive access 隐入固定访问协议，并重新评估泄露、消息层、材料数和成本。

## 6. 功能、成本与规模决策

S16 的成功 soundness 与 ideal uniform-without-replacement abort 界保持不变，不由 S17 测试取代：当前代码通过 SUCCESS guard 才做 membership `R_i≥R_selected` 与同置换 inverse；成功输出按实现约定逐位比较冻结 oracle。S17 新测试观察到的有限样本 abort 只有显式注入的 `TEST_ONLY_FORCED_ABORT_AFTER_SELECT`，不能作为自然失败率或理论概率界。

检查过的容量公式为：

```text
slots/party = 2·padded_n + 2 + 9·C(n,2)
key bytes   = 57 + 24·comparison_bits
lower bound = raw-key bytes + pair-key bytes + manifest + envelope overhead
```

由 checked S15 预检复跑：n=256 是 294,274 slots、312,136,740 B/party 下界；n=1000 是 4,497,550 slots、4,987,338,528 B/party（4.99 GB）下界，超过现 512 MiB bundle cap；n=10⁴/10⁵/10⁶ 的下界约 0.542 TB/57.5 TB/6.07 PB。均为公式下界，不是密文包实测；目前 writer、bundle/API 明确最多 n=256。n=1000 结果为 `PRECHECK_REJECTED / NOT_RUN`。

不运行正式 V4 LAN/WAN 或六方案比较。即使 sidecar 能降低常驻 RAM，离线生成仍是全池 O(n²)，且尚未做流式 T generator、安全视图审查与部署交付。

## 7. 门禁

| 门 | S17 判定 |
|---|---|
| `FUNCTIONAL` | `PASS_FOR_REBUILT_TESTED_DOMAIN`：party-node T/P0/P1 E2E n=1/2/5/8/64/128/256；n=256 common-tape K=2/8 differential；详见验证报告。 |
| `CONDITIONAL_SECURITY` | `NO_GO`：首缺口 H1→H2；shuffle/inverse 联合模拟待证。 |
| `PRIVATE_DELIVERY` | `EXPERIMENTAL_LOCAL_CREDENTIALS_AND_FILES_ONLY`；无远程认证传输/生产注册。 |
| `ONE_SHOT_SCOPE` | `CONDITIONAL_FIXED_PROTECTED_NONROLLBACKABLE_ROOT`；实现绑定 device/inode，环境防回滚未验证。 |
| `STREAM_STORE` | `PASS_AS_LAZY_SELECT_MEMBERSHIP_STORAGE_ONLY`；不是 streaming KeyGen，不进入 n=1000。 |
| `INTEGRATION` | `NO_SECURE_INTEGRATION`；Opt-in target 保持默认关闭。 |
| `PERFORMANCE_READY` | `NO_GO`；只保留功能/计数冒烟，不进入 V4 或六方案排名。 |
