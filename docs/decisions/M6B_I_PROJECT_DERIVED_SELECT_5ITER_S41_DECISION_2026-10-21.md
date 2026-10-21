# M6B-I-PROJECT-SELECT-5ITER-S41 决策记录

日期：2026-10-21
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

## 决策

1. 固定接收 S40 本地提交 `b04618117b11cffe0e899947726ab1c66807ffdd`，父级为 S40 文档接收提交 `b3b04dd835b355193b83d689c698a688267605bc`；S40 runtime 基线仍为 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`。核对 S40 85 项 manifest canonical SHA-256 `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6`。S41 使用新隔离 worktree；未改主工作区或 `VFSS-baseline/`。
2. 为诚实对端的现有角色定义 proof-only `F_A-ATOMIC/v2` 的 authenticated endpoint transition。`P_(1-b)` endpoint 只能使用自己的输入、同一次 `F_TOPK` 调用给它的理想输出份额、`F_T/v1` 给它的 package、自己的 TLS 凭据和公开 recipient/service 设置；它按真实协议生成并发送 authenticated deposit。A 独自持有 context 与提交状态。`Sim_b` 只得到本方 API 实际返回字段，不能取得 peer package/share/credential、A context、T 私钥或 `J_b`。
3. 条件于相同完整 pre-submit boundary state，peer deposit → PENDING/COMMITTED → receipt/v2 response → committed fetch 的应用层 continuation kernel 可逐步对应源码状态机。独立接收者 `/root/s40_independent_receiver` 确认真实诚实 endpoint 的因果路由定义没有把 peer-deposit 成功事件交给 `Sim_b` 伪造；但整体联合模拟裁定为 `OPEN_WITH_EXACT_GAP`。原候选把含 `Vpre_b`、peer output/package、A context 的完整 `S_pre` 当作共同条件，未证明其在真实/理想执行中给定 `Q_b=(x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,C_b)` 后具有相同条件 law。固定 `S_pre` 的 kernel 相同不足以推出本方完整视图分布相同。不得推进 receipt/status/commit/fetch 联合模拟，也不得把 `F_T/v1`→Dealer bridge 写成唯一下一证明义务。
4. 新接口规范不创建运行时 API，不改变已确认的 P0/P1/A/T 角色、输出功能、`L_S29`、`L_A` 或 receipt/v2 字段。A 的 context/package-set 摘要、peer ciphertext/hash/length、DB 状态、到达顺序和时序继续只属于 A 的 `L_A`。P_b 的离散响应只含实际协议的 status、transaction、opaque token 及本方 fetch ciphertext；物理返回时刻不保证同时。本轮不对壁钟时延作隐藏性结论。
5. S39/S40 的理想独立 A 随机源下 32-byte receipt/v2 token 局部条件均匀引理沿用，不重复推导。理想 F_T/v1 到真实离线 Dealer/签名/RNG/TLS 桥尚未开始；必须先闭合并复核 pre-submit 条件联合 law，故它不是当前唯一下一断点。后续优势项继续保留符号，不填入安全位数。
6. 本轮只改审查/治理文档，不改 runtime，不构建、不测试、不重跑 S40 的 211 案。自然业务路径读写故障沿用 S40 `NOT_REPRODUCED`；本轮未执行故障注入。N>256 `NOT_ADMITTED`，LAN/WAN 与 RTT `NOT_RUN/NOT_MEASURED`，v3/receipt-v2 默认 OFF，不创建 `secure` alias，不推送或合并。

## 条件模拟续接的适用范围

当前可记录的局部结果仅为：对固定、完整且已给定的 `S_pre`，诚实 endpoint/service escrow continuation kernel 与源码状态机相同；独立接收者已审查并接受该因果 endpoint 路由规格。该结论没有证明 `Law_real(H_b|Q_b)=Law_ideal(H_b|Q_b)`，其中 `H_b` 包括 peer 输出/package、A context 与 A 预提交状态，`Q_b` 是本方可用视图条件；也没有提供不泄露 `H_b` 的联合 sampler。因此本轮联合模拟状态是 `OPEN_WITH_EXACT_GAP`，不是 `CONDITIONAL_PASS_REVIEWED`。

下一步先明确共同 F_T/F_TOPK session、invocation、N、K、route、circuit 与 gate-schedule 绑定，并构造或证明隐藏状态与本方前缀的条件联合 law；该候选需再经独立密码学复核。只有这一步闭合后，才继续讨论 ideal `F_T/v1` 到真实离线 Dealer 的 Beaver/permutation 联合分布、package/signature、RNG/DRBG、TLS、重启/reseed/并发观察、one-shot 消费及优势项。未闭合前，不把这些后续义务标为唯一断点。

详见 S41 报告、机器可读 gates 和 source/evidence manifest。
