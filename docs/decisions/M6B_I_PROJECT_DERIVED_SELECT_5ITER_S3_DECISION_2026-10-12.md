# M6B-I S3：PROJECT_DERIVED_SELECT_5ITER Protocol I 泄露与资源复审

日期：2026-10-12  
任务：`M6B-I-PROJECT-SELECT-5ITER-S3`  
固定算法标签：`PROJECT_DERIVED_SELECT_5ITER`  
组合标签：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF`

## 裁决

- S2 明文算法与成本结论：`PASS_WITH_SCOPE`。接收基于精确源码/工具哈希、run7 原始汇总及另一审查线程的静态复核；本轮没有重跑 40,490 例全量套件。S2 verifier 验证边 trace，不是第二个 Select 实现。
- S3 已确认公开字段：端点、打乱坐标 pivot、key slot ID、比较次数/通信长度和统一 ABORT 可组成明确的项目泄露函数 `L_S3`。端点图会额外确定性暴露活动候选位置集合（图非空时）和材料访问序列；它们是获批端点字段的派生量，不是新的消息字段。
- N=128、K=2/8：逐调用新鲜掩码的 round-aware 全池格式静态估算为 `43,326,576 B/party`；加现有 shuffle 保留向量为 `43,601,008 B/party`，均低于 64 MiB 名义包限。现有 M2PK 仍因只接受恰好 `C(128,2)=8,128` 个单层边槽、无迭代命名空间而拒绝五阶段 `40,640` 槽。尺寸可行不等于现有 ABI 可运行。
- 总接入门：`NO_GO_FOR_INTEGRATION`。尚无秘密 rank/sample-quantile/pivot mux 与 `(C,r)` 更新原语；strict-`<` membership 的代数方向已闭合，但 masked operands 与 τ 分享接口未闭合。不能公开比较结果、rank、阈值或原始位置绕过此缺口。
- 本轮没有修改 `VFSS/`、没有创建 opt-in runtime、没有生成 DCF 材料、没有运行 DCF conformance / party E2E / TLS。保留 EXPERIMENTAL 研究状态。

## 泄露函数和派生状态

获批公开字段及消息时机、观察方见 S3 报告 §3。尤其：端点以打乱坐标表示；pivot 位置是打乱位置；slot 地址限定到 session/算法/iteration/purpose/direction/endpoints 且每次调用独立消费。调用方按端点和 slot 复算同一 schedule，无需另发 active-C bitmap。

每个活动比较图的端点并集覆盖当前候选 `C`（唯一无边例外是 singleton）；因此 `C` 的打乱位置及规模可从已批准端点导出。purpose/substage slot IDs 带 `sample_clique` 标记时，sample positions 也由对应端点导出；两 pivot-star 的图、边数和下一次 slot 访问顺序同样可计算。rank 值与 residual `r` 不由这些字段直接给出；但已公开 pivot 身份和自适应图可能泄露关于目标 rank 的相关信息，须由新 L 的 simulator 覆盖，不能声称零泄露。

比较结果 share、raw-score carry/sign operands `z_carry,z_sign`、Select operands `z_left,z_right`、membership operands `z_tau,z_i`、阈值及原始 index 到 shuffle 坐标映射不在 `L_S3`。若复用现有 raw-score/uCMP API，当前数据流会让 P0/P1 先重构共同 masked operands；这属于额外公开字段，必须先确认扩展 L 或换成不公开该值的接口。不能把公开比较位当作已批准字段。

## 一次性材料与容量

拟议新 sidecar 用 injective slot address `(session, algorithm, iteration, purpose, direction, left, right)`；公开 use claim 另绑定 sample-clique/pivot-star/final 的 substage。物理 pool 可在同迭代共享 generic Select purpose，因为 sample-clique 边端点均属于 S，pivot-star 边一端属于 S、一端在 C\S，两个 sublayer 不会调用同一 pair；两 pivot 到同一非 sample 也形成不同 pair。每个逻辑比较调用一把 DCF key，同边跨迭代必用新槽；输入转换、Select 和 membership 采用不同 purpose。全池含 Select `5*C(128,2)=40,640` 槽、输入 `2N=256` 槽、membership `N=128` 槽，总计 `41,024` 个 DCF key/party，uCMP 两次 Eval 对应最多 `82,048 Eval/party`、两方 `164,096 Eval`。

逐调用格式按每个 Select 槽 `D(40)+24 B descriptor+16 B fresh operand-mask shares`、输入槽 `D(34)+40 B`、membership 槽 `D(40)+40 B`，其中 `D(b)=57+24b` 是现有 party-key serializer 长度。Select pool 为 `42,956,480 B`，raw-score 材料 `233,728 B`，membership `135,296 B`，现有包头/节点 mask 向量 `1,072 B`，合计 `43,326,576 B/party`。这些是明确提出的保守布局估算，不是已经序列化的协议包。

若把当前 shuffle 的 `274,432 B/party` 逻辑保留向量并入同一个 bundle，估算 `43,601,008 B`；`1 MiB` frame、16 B chunk envelope、48 B wire header 下分 42 帧，另加 `2,688 B` wrapper/header 后为 `43,603,696 B`。TLS record、OT wire、运行时索引/allocator、秘密控制/阈值选择材料均 `NOT_MEASURED`。当前包边数上限为 1,000,000，五阶段槽总量低于数量上限；失败原因是 M2PK 形状校验，而非 N=128 的已知字节预算。

## DCF membership 合同

Protocol I packed key 是 39-bit 唯一稳定键，比较时使用 40-bit ring（`N=128,index_bits=7`）：signed Q20.12 分数降序、`original_index` 升序。将第 K 小 key 作为秘密 `τ`，每个打乱位置计算

```text
M_i = 1 - [τ < key_i] = [key_i <= τ].
```

现有 ABI 是 `strict_lt(left,right)`，所以方向必须为 `(τ,key_i)` 后取补；相等时 DCF 严格比较返回 0，mask 为 1。唯一稳定复合键保证恰 K 个有效位置，padding dummy 排在全部有效输入后。若 DCF 输出为 `Z_(2^64)` additive shares，取每方 LSB 作为 XOR share 与加法模 2 的投影一致；但 runtime 没有 τ 分享、membership 批量接口或该路线的端到端 conformance，因此这只关闭代数映射，不关闭 `DCF_MEMBERSHIP` 门。

## 继续实施的精确前置项

1. 提供秘密 sample-order statistic/pivot mux 与基于 uCMP output shares 的 rank aggregation、`(C,r)` 选择/更新、τ share 输出 API；不开放 rank/比较位。
2. 对 masked DCF operands 给出公开字段决策：如继续使用当前 adapter/uCMP 的共同 `z` 输入，需精确批准 raw-score `(z_carry,z_sign)`、Select `(endpoint/purpose,z_left,z_right)` 和 membership `(z_tau,z_i)` 给 P0/P1、坐标、发送时机与复用范围；否则实现输入不公开的新 comparator。已批准端点/pivot/slot/count/length/ABORT 无需重复确认。
3. 冻结 versioned sidecar/M2PK replacement；实现 session/iteration/purpose/endpoints/direction 一次性绑定、slot 消费与 replay reject，并证明整池预发无在线 Dealer。
4. 补足 shuffle 与 input/output route 的 mTLS transport；当前检索到的活动 Protocol I FDs 使用自定义 frame/原始 fd，未找到 TLS 实现。
5. 交付覆盖新 `L_S3`、端点/slot 自适应、未使用池、masked operands、shuffle/inverse、mask shares、资源失败和统一 ABORT 的双方 view simulator/安全证明复核。
6. 前述接口评审通过后才可启用版本化 opt-in EXPERIMENTAL 路径，依次执行原语 conformance、冻结 oracle differential、T/P0/P1 独立进程 E2E；不建 `secure` alias、不做正式 LAN/WAN。

S34、S1 的 BB90 来源 NO-GO、M6A V3/V4 门、BMW16-derived 候选和其他协议结论均不由本决策修改。
