# M6B-I-PROJECT-SELECT-5ITER-S44 决策

日期：2026-10-22
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL

## 决定

1. 固定 S43 本地提交 a1b02a8133955786e0bb6b047f55b746af66acbf（tree 2b58e2a02ec9668cfbd9e6ba7419f76142f7f828，父级 S42 72a23fc067b5bfbf10edf110eeff45ec1f0ed6bd）。实际被审 Dealer/runtime 仍固定到 S39 commit 8028c0fece69fe0c648b1e0f1e12ad7fca723f20；S39 GMW source/header 的 Git blob SHA-256 与 S43 manifest 记录相符。S43 manifest 及其 7 项 S43 deliverable raw hashes 均核验。S43 继承的 S42 rows 中两个治理文档哈希异常单列记录；不得将其解释成 S39 Dealer 源码冲突。S41 provenance erratum 原样保留。
2. 未参与 S43 推导的审查者 /root/s43_final_text_review 只读复核 S43 意见并入后的最终证明文本，再复核 S44 RNG 条件与重试上限草案。复核不改文件、不跑 build/test，范围限于随机带条件、q/拒绝界与 bounded-rejection 功能语义。
3. 在固定 tuple、固定 T 签名 key、部署策略 E 下，对初始/reseed 熵输入 S 求平均（S 不含演化 DRBG state），并使用理想 IID byte tape 时，沿用 S43 材料联合 law 的 PASS_WITH_SCOPE。所需播种条件包含 `Law(S | P,sk_T,E)=Law(S | E)` 或可证明的替代条件；不得把 realized seed/熵输入固定在条件中。该结论不覆盖 OpenSSL provider、熵源、并发、fork/restart/reseed 或真实随机优势。
4. 首个真实概率断点仍是对隐藏 S 求平均后的整段 OpenSSL RAND_bytes transcript 联合不可区分条件：`Law(T_real | P,sk_T,E,Suc_real) ≈_c Law(T_U | P,sk_T,E,Suc_IID)`，其中 transcript 含返回字节、调用边界和状态，理想成功事件概率为 1。成功条件化须明确覆盖 RAND 错误的选择效应。需要覆盖所有有关调用、适应性长度、确定性后处理及并发观察的 `Adv_DRBG_T(Q,C;E)`。没有真实部署信息或数值优势；RNG_DRBG_BRIDGE OPEN。
5. N≤256 时 next_uniform 拒绝采样期望尝试有限，但源码 for(;;) 无上限，故实现为 expected-polynomial-time sampling，不是可由源码证明的 strict worst-case PPT。项目未找到接受 expected-PPT 的总规约；S38 已使用 strict-PPT λ 次上限先例。裁定当前版本在该证明时间门保持 OPEN，不通过。
6. 最小版本化候选为 F_T/v2 + bounded-rejection-v1：每个 Fisher–Yates 位置最多 r(λ)=λ 次；失败统一返回 ISSUANCE_ABORT 且无 package/context/lease。令 π_mat 仅保留解析后的语义材料并丢弃版本、签名和编码元数据；π_mat 上成功条件 law 与 F_T/v1 语义材料 law 精确一致，含 abort 时 `TV(π_mat(F_T/v2),π_mat(F_T/v1))=δ_reject(m,r)=1−∏(1−ρ_i^r)≤255·2^(−24r)`。这个 δ 不约束完整 package/context/signature transcript，因为签名版本字段会不同。bounded sampler 到匹配版本 F_T/v2 完整 transcript 的界是计算优势 `Adv_DRBG_T(Qmax,Cmax;E)`；仅在 π_mat 投影后若对照 F_T/v1 才再加 δ_reject。N=256 时 Qmax=5,289,856+1,020r bytes。此变更需版本化理想功能、package/signature 参数和失败合同；本轮未修改 runtime 或 F_T/v1。
7. RAND_bytes 错误路径在材料生成阶段抛异常，durable_create 尚未开始；没有证明敏感 buffer 清零、失败重试幂等或后续落盘失败恢复。它们不是自然 I/O 故障复现，也不纳入 RNG 成功路径定理。
8. 本轮不重跑 S35 的 195 案、S40 的 211 案；不 build/test、不执行正式 LAN/WAN/RTT。自然业务 I/O fault 继续 NOT_REPRODUCED；N>256 NOT_ADMITTED；v3 default OFF；secure alias 未创建；性能未测量。不得将此项写作完整安全 PASS、BB90/BMW16 复现或 BB90+DCF 结果。

## 门禁状态

- S43_SOURCE_RECEIPT = PASS_WITH_SCOPE（含明确的 S42 文档 metadata caveat）。
- DRBG_ASSUMPTION_AND_DEPLOYMENT_MODEL = OPEN。
- JOINT_RNG_STREAM_BRIDGE = OPEN。
- REJECTION_RUNTIME_BOUND = OPEN_STRICT_PPT_NOT_ESTABLISHED。
- FT_VERSION_COMPATIBILITY = OPEN_DESIGN_ONLY。
- ONE_SHOT_SCOPE = CONDITIONAL。
- PRIVATE_CHANNEL_BRIDGE 与 PARTY_VIEW_COMPOSITION = OPEN。
- SECURE_ALIAS_READY / PERFORMANCE_READY = NOT_READY。

详见 S44 report、gates 与 source/build/evidence manifest。仅本地提交，不推送、不合并。
