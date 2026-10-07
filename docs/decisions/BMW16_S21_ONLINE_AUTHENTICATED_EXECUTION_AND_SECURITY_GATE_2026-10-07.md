# BMW16 S21 决策：在线认证执行与安全门

日期：2026-10-07
方案身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决定

1. 以 S20 提交 `0ec88896fcc72440818bd3110dac482eced80e2c` 为精确接收对象。保留其唯一 VFSS C++ Select、逐槽 KeyGen、sidecar、party API 与同置换 inverse；S21 只增加在线 TLS 数据流、可信本地启动门和凭据文件权限检查。S20 worktree 与主工作区未修改。
2. 新增 `party-tls` opt-in 路径。P0/P1 用 OpenSSL TLS 1.3 双向证书认证建立十二条 TCP channel；身份验证、TLS 完成后的应用绑定和所有协议帧都走同一 SSL stream。`ProtocolIFramedChannel` 对注册的 TLS stream 使用 `SSL_read_ex`/`SSL_write_ex`，不把握手后的裸 FD 交回协议。原有调用方注入 FD 路径保持不变。
3. 每条 channel 在 TLS 内绑定协议版本、session、fingerprint、n/K、比较位宽、方向、固定实现标签、channel ID 与名字。十二条 channel 对应 raw-score 两条、forward shuffle 两条、Select R1–R4 四条、inverse 两条、最终状态一条和抽样币一条。既有帧继续认证帧内 sender/receiver、phase/type、n/K、width、session/fingerprint 与递增序号。Select 两 task 仍按冻结 C++ 顺序装在对应轮次消息内，task/端点/轮次同时绑定各自材料槽。
4. TLS 1.3 被固定为唯一版本；session ticket、缓存和 early data/0-RTT 关闭。任一证书链或预期 peer identity 不匹配、channel binding 错误、超时、截断或关闭均返回 `ABORT_COMMUNICATION / LOCAL_ONLY`，party-node 退出 30 且不写 mask。未收到对端状态确认时不报告双方一致 abort。
5. 新增 `protocol_i_bmw16_start_after_offline_gate` 本地 supervisor primitive。它检查三个不同 PID 与 T/P0/P1 角色顺序、相同 `(session,fingerprint,n,K)`，等待三者退出；任何非零/信号退出都不调用输入创建回调。三个退出码全为零后，调用方还必须校验两侧已提交的 pair marker/包配对；测试实现检查两个 132-byte marker 完全相同且四个 shell/sidecar 文件均存在。只有配对校验成功后才调用创建输入 shares/启动 parties 的回调。marker 文件本身不授予启动权限。
6. 对跨主机 T/P0/P1，部署方必须通过受信任配置预置专属证书、私钥和 CA bundle；T 的私钥仅 T UID 可读，party 的私钥仅对应 party UID 可读。代码现在拒绝 owner 不匹配或 group/world 可读写的关键私钥/wrapping key/claim-root。S21 没有实现证书签发、注册、轮换/吊销服务，也没有跨主机 process-exit 收据控制面；本地 PID gate 和临时测试 CA 不冒充这些部署服务。
7. 正常配置默认关闭 TEST_ONLY failpoint。独立 `BUILD_TESTING=OFF` Release 构建也完成；其 CMakeCache 中测试开关为 OFF，二进制不包含 `MOE_BMW16_TEST_FAILPOINT` 字符串。实验入口仍须显式 opt-in，且无 secure alias。
8. 联合安全门保持 `NO-GO`。不因 TLS、分方进程或 oracle E2E 通过而升级标签，不继承 BMW16 Theorem 8 的概率/比较常数，不把四个 Select 消息层称为完整入口四轮。

## 安全视图及第一个未闭合步骤

候选理想功能以 P0/P1 的 signed Q20.12 加法份额为输入；成功时只输出各自原序 XOR mask share，重构结果按分数降序、`original_index` 升序且恰有 K 个 1；概率中止时双方不输出 mask。以下均是**待项目所有者裁决的泄露草案**，不是已批准泄露：

| 字段 | 实际所在代码/消息 | P0/P1 可见内容 | 当前判断 |
|---|---|---|---|
| n、K、session、fingerprint、width、label、party identity | TLS app binding、bundle shell、帧头 | 公共配置与证书身份 | 候选公开参数 |
| raw-score 转换和 `public_z`/masked operands | `protocol_i_bmw16_experimental_raw_score_mask_party` 的 raw 与 forward shuffle 帧；shuffle 输出的 `word0` | 两方收到相同的 masked 值 | 需进入泄露函数并证明模拟 |
| R1–R4 边、比较 bit、pivot、U/V/W、采样结果 | `Runtime::compare_round` 中冻结批次、本地 `eval_strict_lt` share 和打开后的 `local+peer` bit；每轮 channel payload | 两方由相同 opened bits 推进状态；测试 trace 仅在 TEST_ONLY tape 模式记录 | 自适应 transcript 未获批准 |
| 两个 task 的共同真实匿名 handle 与 selected handle | 同一 `public_z` 索引空间、两个 task 的候选集合交集与本地 `selected.second` | 不另发 selected-index 帧，但两方能从 transcript 计算同一匿名句柄 | 任务间关联需联合模拟 |
| 活跃材料槽和访问顺序 | `claim_slot`、stream-store `load_once`、每轮 message 长度 | 每方知道自己的 key 槽；双方知道边顺序/逻辑比较，文件 I/O 流量可观察 | 访问模式/长度泄露候选 |
| 概率 abort、材料/通信错误 | Select 状态、最终状态 channel、party-node 状态/退出码 | 可区分 PEER_AGREED 与 LOCAL_ONLY | abort 类别与部分失败需进功能定义 |
| mask shares | party-node 各自 `mask.share` | 各方仅见本方输出 share | 理想功能输出 |

完整腐化方 view 还包括：本地输入 share、shuffle 的本地 `sigma/tau/a/e/r_share` 状态和交互、整个本方已用及未用 key pool（party 拥有 sidecar 解封 key）、共享节点 mask 导出的相关阈值、每次本地 Eval share、全部 TLS/frame transcript、claim/slot 轨迹、失败状态及输出 share。T 可信且在线静默、不与任一 party 合谋。

### Hybrid 裁决

- **H0→H1（随机源替换）**：FSS DCF seeds 由 `protocol_i_parallel_shuffle.cpp::seed_fss_once` 的 OS `getrandom` seed 到 `FSSConfig::prngs`；shuffle、stream nonce 与 sampler contribution 也使用 OS CSPRNG。在线采样币由双方独立 32-byte OS contribution 异或，随后按 task/domain/counter 送入 SHA-256 counter sampler。若构造正式证明，需明确 `osuCrypto::PRNG`/AES 扩展和 SHA-256 counter 的计算伪随机假设、各域/各调用数及熵来源；本轮没有把测试 seed/固定 tape带入 party 路径，也没有给出全路径 PRG 优势界。
- **H1→H2（首个失败步骤）**：本地 FSS 源码的 `keyGenDCF` 每次从 `FSSConfig::prngs[omp_get_thread_num()]` 取两块 seed，构造双方 `k0/k1`，并把同一调用生成的 `g0` 和 `v0` 指针放进两个 `DCFKeyPack`。`ProtocolIUcmpPartyMaterial` 对一条比较会在两方本地 Eval 份额；上层随后打开其和作为比较 bit。当前代码/仓库没有给出针对实际 key ABI 的游戏，证明一次性池中所有 used/unused `(k_i,g,v)` 在相关阈值向量辅助输入下可联合模拟，并在公开 bit/operand 条件下继续模拟本方 Eval share 和后续自适应查询。仅把单 key key-share 隐私逐个套用 hybrid 仍缺少“输出条件 + 相关辅助输入 + 已打开结果反馈”的归约接口；PRG keygen 独立性本身补不上这一步。没有发现具体可区分反例，也没有证明安全或不可能。精确缺项是 `AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy`：挑战者给定本方 shuffle 辅助信息、完整相关阈值/节点 mask 关系和一方全部 key pool；对手按先前 opened comparison bits 自适应给出公开 operands/slot；模拟器须联合生成 party Eval share、opened bit、访问轨迹及未用槽视图，并与真实完整 pool 计算不可区分。该引理须由实际 keyGenDCF/evalDCF 和序列化 ABI 的证明支持。
- **H2→H3**：还需把以上 simulator 与实际 Protocol I forward/inverse shuffle 的本地辅助状态、两 Select task 共用的匿名句柄、`public_z`、abort 协调和输出 mask share 组合起来；当前 shuffle roundtrip/E2E 仅是功能证据，不是单方模拟证明。

下一项安全工作是单独 formalize 并审查 `AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy`。可测试的第一批构造是对 n≤8 枚举多 key pool、相关 alpha 和适应性 query schedule，并比较“给定完整未用池与本地辅助信息”实验的 view；有限枚举只能找反例，不能代替一般定理。若不能由实际 FSS 构造导出该引理，应保持 EXPERIMENTAL，或另开非自适应秘密比较设计的新算法标签。

## 部署接口状态

在线配置由外部可信服务管理器提供：session/fingerprint/n/K、party ID、监听/对端地址与 base port、证书链、party 私钥、信任 bundle、预期 SAN identity、有限读写超时。P1 先绑定连续 12 个 listener 并通过本地受信 pipe 发出带 session/fingerprint/n/K/port base 的 readiness receipt；P0 逐条连接。该 pipe receipt 是本地 supervisor 的启动信号，不是远端身份凭据。跨主机 supervisor/receiver 退出状态必须由受信控制面验证，目前未实现。

离线仍使用 S20 的 T→P0/P1 mTLS 发材。T 使用公开配置与预置信任凭据逐槽发材后退出；接收方只持有自己的 AEAD sidecar/wrapping key 与 shell。失败包括预期断连、错误身份/绑定、超时、截断和帧损坏均无 mask；若无 ACK 只报 LOCAL_ONLY。固定受保护、不可回滚 claim-root 的 one-shot 范围不扩大。

## 门禁

| 门 | 状态 |
|---|---|
| `FUNCTIONAL_ENTRY` | 本机不同 UID、真实 TCP/mTLS、受限 n≤256 矩阵 PASS；n=1 为 shortcut |
| `OFFLINE_DELIVERY` | 本地 distinct-UID mTLS 条件 PASS；生产凭据注册/跨主机控制面 CONDITIONAL |
| `ONLINE_AUTHENTICATED_TRANSPORT` | 新增 TLS 1.3 完整数据流及错 SAN 负例 PASS；跨主机生产配置 CONDITIONAL |
| `T_EXIT_STARTUP_GATE` | 本地可信 PID supervisor、三进程 exit gate 与双侧 pair marker 校验 PASS；远程退出证明/启动服务 CONDITIONAL |
| `JOINT_PARTY_VIEW` | NO-GO；首个未闭合引理为 `AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy`，随后仍需 H2→H3 shuffle/output 模拟 |
| `SECURE_ALIAS` | NOT_CREATED；保持 EXPERIMENTAL |
| `PERFORMANCE_READY` | NOT_RUN；无正式 LAN/WAN/V4 资格 |
