# BMW16 S20 决策：私有认证离线发材、启动门与联合视图

日期：2026-10-07
方案身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决定

1. 继续复用 S19 的唯一 VFSS C++ party API、流式 KeyGen、随机访问 sidecar、BMW16-derived Select、membership 与同置换 inverse。S20 只增加离线 T→P0/P1 的相互认证 TLS 1.3 发材接口和测试；不新增 Select/完整协议副本。
2. T 使用预置的 dealer 证书/私钥与信任根，分别验证 `party0`、`party1` 的服务端证书及 DNS identity；P0/P1 验证 dealer 客户端证书、受信任签发链和 `dealer` identity。证书/私钥通过受信任的运维配置预置，不从 seed、raw input 或测试材料派生。在线密钥、party 凭据注册和跨主机部署仍要求环境管理流程，S20 测试 CA 不作为生产凭据方案。
3. T 只读公开 `(n,K,session,fingerprint,claim-root identity)` 和双方离线 wrapping key，逐槽生成两份独立密封 sidecar 与各自 shell；在其 T 私有 spool 中暂存。两条 TLS 1.3 mTLS 连接均使用有序 64 KiB 分块。固定 manifest 绑定 version、party、session、fingerprint、n/K、比较位宽、claim-root device/inode、stream ID、槽数、双方 shell manifest digest、shell/sidecar 长度及 SHA-256。端点/阶段/task/round/slot 的数学关系仍由 sidecar AEAD AAD 与既有 slot decoder 验证；TLS/外层摘要本身不证明 FSS key 的数学正确性。
4. P0/P1 仅接收各自包。接收端先写 0600 随机临时文件，核对分块 kind/sequence/length、摘要、shell manifest 与 sidecar header/length，再在收到 `COMMIT` 后执行 fdatasync、no-clobber hard-link、0400 权限和目录 fsync。所有步骤成功后才返回 `COMMITTED`。ready marker 仍是 S19 格式，用作 party bundle 参数一致性检查；它**不证明**T 进程已 exit 0，也不作为配对发材的启动门。
5. 唯一启动合同是外层可信 supervisor 同步等待 T、P0 receiver、P1 receiver 三个进程并检查都正常 exit 0；仅在此后创建 raw shares 和在线 parties。收到 marker、文件出现或一个 receiver 成功均不能启动协议。TLS API 不包含在线协议消息 transport：party 在线 API 仍要求调用方注入双方已建立且完成认证/session 绑定的 FD；S20 没有实现或接收该 online FD 服务。
6. 发布阶段不是跨两台主机的原子事务。若 P0 已 `COMMITTED` 后 P1 发布失败，T 非零退出且 P0 可能留有本地 orphan shell/sidecar/marker；监督器必须阻止所有在线启动，将该 session 两个 root 标为作废并换 fresh session/root。不得把该 orphan marker 解读为成功，不得覆盖或自动复用包。本地 one-shot 范围仍是固定受保护、不可回滚 claim-root；管理员/快照回滚、跨 store 重放未解决。
7. `MOE_BMW16_TEST_FAILPOINT` 相关代码仅在显式 `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON` 时编译；该选项默认 OFF，且 CMake 禁止与 `BUILD_TESTING=OFF` 同用。普通构建中的环境变量不能触发 failpoint。
8. 方案身份与安全状态保持 `PROJECT_DERIVED / EXPERIMENTAL`。本地互认证 TLS、三进程成功和 oracle 差分不证明整池 DCF 或 shuffle 的联合隐私；不创建 secure alias、不启动正式 LAN/WAN 五次矩阵、不进入六方案排名。

## 版本化发材消息顺序

```text
预置配置：T、P0、P1 各自获得独立身份材料及本方 claim-root
P0/P1 service bind+listen → T KeyGen/写双方密封 sidecar+shell 到 T 私有 spool
T ↔ P0 mTLS1.3: PREPARE(manifest), shell chunks, sidecar chunks, pair marker
P0: 校验、临时写入；返回 PREPARED
T ↔ P1 mTLS1.3: 同上；P1 返回 PREPARED
T→P0/P1: COMMIT；各自持久发布并返回 COMMITTED
T exit 0 + P0 receiver exit 0 + P1 receiver exit 0
supervisor 才创建 raw shares/启动在线 parties
```

若任一阶段失败，接收端只报告自己的 `ABORT_MATERIAL` 或 `ABORT_COMMUNICATION / LOCAL_ONLY`；没有收到对端 ACK 时不声称双方达成 abort。部分 commit 的 orphan 按第 6 条作废。

## 联合视图决策

理想功能候选只向两方输出 Top-K mask shares，成功重构为 score 降序、同分按 original_index 升序的稳定 Top-K；允许一致显式 abort、无 mask。下表仍是**候选泄露草案，未获项目所有者批准**：

| 字段 | 当前所在消息/状态 | 本地 party 可见性 | 评估 |
|---|---|---|---|
| `n,K,session,width,stream_id` | mTLS manifest、bundle shell | 两方可见 | 配置泄露候选 |
| `public_z`/masked operands | raw/shuffle 与 uCMP 输入交换 | 两方可见 | 需在理想泄露函数中逐项说明 |
| 匿名边、比较 bit、pivot、U/V/W、访问次序 | Select R1–R4 的在线消息和本地状态 | 两方可见 | 输入顺序匿名不等于联合 view 可模拟 |
| 两 task 共用真实匿名 handle | 两 Select 状态/同一 shuffle record identity | 两方可见 | 任务间相关性须进同一个 simulator |
| selected anonymous handle、自然 abort 类别 | 两实例交集及状态协调 | 两方可见 | 不能当成已授权输出泄露 |
| sidecar slot/byte access 与流量 | AEAD 读取、mTLS chunks、通信计数 | 本方和网络端点 | 访问模式及长度泄露候选 |
| 最终 mask shares | 原序 inverse 输出 | 各方仅本地 share | 理想功能输出 |

本轮没有构造出相同腐化方输入/理想输出却可区分的二世界攻击；这不是安全证明。H1→H2 的缺口仍是：给定本地 shuffle 辅助信息、整池 used/unused key、共用 mask 产生的相关阈值，以及先前公开 bit 后的自适应查询，如何联合模拟 `evalDCF` 输出 share、打开 bit 与查询轨迹。普通单 key DCF key privacy 未直接给出所需的辅助输入、输出条件自适应多 key 定理。H2→H3 还缺实际 forward/inverse shuffle state、`public_z`、selected handle、abort 和输出 share 的组合模拟。

## 下一步可执行裁决任务（C）

S21 不以“再解释单 key 定理”作为目标，先实现隔离 TEST_ONLY 的**非自适应固定完整比较图**最小原型：所有真实匿名 handle 对在固定顺序比较；比较输出始终保持 shares，不打开 bit；用共享聚合构造秘密 rank，再作秘密 `rank<K` membership 和同置换 inverse。每条比较使用不复用的独立材料 domain/slot，固定顺序预发。先在 n≤8 common tape 与 oracle 上证明每次 SUCCESS 的 exact mask，再统计材料量/消息相位。随后提出可审查的“条件于一方全部本地辅助输入和所有未用 key 的多实例 DCF key privacy”安全游戏；若该游戏仍不由仓库原语定理支持，则该路线也不得升级。此候选将是不同算法标签，不能冒称 BMW16-derived Select，也不能替代本轮现有 EXPERIMENTAL 路径。

## 门禁

| 门 | 判定 |
|---|---|
| `FUNCTIONAL_ENTRY` | 见 S20 验证报告；限新建 Release binary 实测的配置 |
| `PRIVATE_AUTHENTICATED_OFFLINE_DELIVERY` | `PASS_FOR_LOCAL_DISTINCT_UID_MTLS_TEST / CONDITIONAL_FOR_DEPLOYMENT` |
| `T_EXIT_STARTUP_GATE` | `PASS_IN_TEST_SUPERVISOR_CONTRACT`; marker 单独无效，部分 commit orphan 作废 |
| `JOINT_PARTY_VIEW` | `NO-GO`；H1→H2 和 H2→H3 未闭合，候选 leakage 未获批准 |
| `ONLINE_AUTHENTICATED_TRANSPORT` | `NOT_IMPLEMENTED_BY_S20`; caller-injected FD 前提未验证 |
| `PERFORMANCE_READY` | `NOT_RUN`; 无正式 LAN/WAN 或六方案数据 |
