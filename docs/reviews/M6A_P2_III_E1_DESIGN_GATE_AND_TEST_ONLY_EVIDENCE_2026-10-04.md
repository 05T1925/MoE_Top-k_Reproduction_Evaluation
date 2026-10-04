# M6A-P2-III-E1 执行报告：隐藏 handle AAV86→ring DPF→原序 mask

**E2 勘误（2026-10-04）：** 本报告第 2 项把现有 score adapter 的 dummy 误判为零，关于正式 III 入口的 padding 缺陷推论已撤回。实际份额为 P0=`0x80000000`、P1=`0`；`n=3,K=3` 两个定向实调用例通过。原零值反例保留为反事实错误接线。见 [E2 勘误](../decisions/M6A_P2_III_E2_E1_PADDING_ERRATUM_2026-10-04.md)。

日期：2026-10-04。执行结论：**DESIGN_GATE = NO-GO，secure runtime 未写，正式性能未运行**。设计与 TEST_ONLY 夹具的已提交源码检查点为 `8d6fc748263fe988b427912b647235916424d547`；本报告在其后单独提交，不把事后文档提交当成被测源码 revision。本任务不是对自己的 III secure 候选进行异会话独立签收。

## 基点、范围及未动对象

开始时 worktree 为干净 detached HEAD `6a9ef8447cfd6b74c17f9a5645eedebcb8a6ea72`，`main=origin/main=merge-base(HEAD,main)=merge-base(HEAD,origin/main)=c3926c68fd14f270faa8b55234311071947fa080`。按指定 E15 接收点建立本地 `codex/m6a-p2-iii-e1` 分支。E15 正式性能**被测源码**是 `346a92326e81aa3ab573c162439968503e792354`；其**事后报告**为 `40a1ccb450447c00ce604a08b058cbe5c0255e88`；`6a9ef844` 是独立接收检查点。本次没有重复 Protocol I 的 120 次矩阵，没有改写其原始目录或主工作区既有差异。

已核对 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/BENCHMARK_VALIDATION_PLAN.md`、protocol-reproduction skill、E15 接收报告及 E15 III 设计门、M5 G3/F1 与两轮 field 决策，以及 `VFSS` 的 score adapter、priority key、I+AAV86、III GRank、F1 ring DPF / raw-mask 代码。本审查区分论文定义、本地实现行为、项目扩展和待验证组合；Theorem 5.1 的 shuffle-based `2r+1` 不能移给本 III 候选。`AUTHOR_EXACT=NOT_PROVEN`。

## 判定和原始证据

1. **接口/代数。** F1 raw-score→原序 XOR mask 是正确的四轮工程基线；其 `Z_(2^64)` DPF 指示份额可逐方取低位，或先约减到较窄 2 幂环再取低位。M5 odd-prime field Fselect/Fsort 没有此同态，且不是本 bit-mask 入口的必需接口。候选不能直接调用固定原序 n 全两两 GRank；需要 D 个隐藏 handle 的图聚合、同 handle 的 rank-mask/DPF key，以及同一 π 的逆路由。
2. **已撤回的 padding 断言与仍有效的逆路由反例。** E1 错把 adapter 的 padding 份额读成零；现有实现其实已给 dummy `INT32_MIN` 和较大下标。`n=3,D=4,K=3,scores=[-1,-1,-1]` 只有在**反事实错误接线**自行使用 score `0` dummy 时才会只选中两个真实槽；不得指称正式入口失败。另 `n=2` 且 π 交换时，handle mask `[0,1]` 直接返回原槽仍错误，必须逆路由成 `[1,0]`。
3. **条件性 rank 归纳。** 前 `r-1` 层在隐藏 handle 域公开 pivot local rank、bucket membership；由父偏移、pivot 局部 rank、桶边界可归纳公开子偏移。最后活跃节点的 pivot 数是 `m-1`，图是完整 clique，故每 handle 得到局部 rank 加法份额；加公开偏移为全局 rank 份额。先前 singleton/pivot 可按已公开 rank 加分享常数。严格 score+original_index key 和 `INT32_MIN` dummy 处理同分与非二次幂。rank 真值小于 D，`ceil(log2 D)` 位环无真值回绕。该推导不是安全证明，也不允许在末轮公开 rank。
4. **安全/泄露阻塞。** F1 只打开均匀 masked rank；候选还公开早期 local rank、pivot、bucket、图、访问/流量。即使 π 隐藏，尚无证明这些量在任一 P0/P1 联合本地视图下可模拟；III 所有者对这些新增字段的许可为 `PENDING`。同一节点 mask 对全两两预发 DCF key 的关联、adaptive use、handle DPF/rank mask 与前/逆置换材料，亦没有一条覆盖全联合单方视图的 hybrid。I 的条件性论证及小规模测试不能替代此证明。T 仅看公开 shape/session、离线发材并在输入前退出，不能在线补料；可信、不合谋、无需擦除模型保持。
5. **消息/容量。** 候选依赖链为 score carry→sign→前向 shuffle→每轮 masked key open→早期 local rank open/下一轮图→末轮 rank shares→masked rank open/DPF→逆路由。CA/DPF 的 `2r` 只是该候选帧图的条件性计数；实际全入口因果轮 `NOT_PROVEN`，不能冒充已执行的 `2r+4`。D≤8,r≤5 的解析上界为每方 140 个预发 edge key 与 40 个节点 mask，实际 `e_t/v_t` 和材料字节/峰值尚未测；D128,r5 为每方 40,640 key 槽，仍受独立 64 MiB 包与 768 MiB RSS 预检约束。固定 M 延期。

完整接口表、角色与单方视图、代数、失败引理、泄露账本、DAG、容量和后续 n128 匹配方案见 [E1 设计门](../decisions/M6A_P2_III_E1_AAV86_RING_MASK_DESIGN_GATE_2026-10-04.md)。本门逐项为功能代数 `CONDITIONAL`、离线材料 `NO-GO`、单方视图/泄露 `NO-GO`、隐藏布局/逆映射 `CONDITIONAL`、消息 DAG `ACTUAL NOT_PROVEN`、容量 `SHAPE ONLY / ACTUAL NOT_MEASURED`，所以总门 `NO-GO`。

## 隔离测试及限制

命令：`python VFSS/tests/moe_topk/protocol_iii_aav86_test_only_gate.py`；结果 **4/4 unittest PASS**。其中明文差分子例 255 个，覆盖 n=2/5/8、K=1/中间/n、r=1..5、三个 pivot seed、全等/重复/signed 极值/非二次幂；每例对独立稳定排序 oracle 检查原序 mask、恰 K 和解析边槽上界。另三项断言分别固定零值 dummy 的 2/3 反例、handle 未逆路由反例、奇素数 field 低位不保加法的反例。夹具仅模拟 rank 加法份额及 64 位 DPF 输出份额的代数；其明文排序和 π 逆映射全部在 `TEST_ONLY`，没有真实 DCF/DPF key、网络、T/P0/P1 独立进程，也不是安全证据或冻结 C++ oracle differential。因为设计门 NO-GO，未进入 secure 实现的 conformance→冻结 oracle differential→三进程 E2E 阶梯；错误材料、重放、peer abort 和截断仍是后续 GO 实现的门项。

性能状态：本任务的离线时间/材料、在线时间、总时间、总/每方通信、实际因果轮、在线 PRG、实际边及 DPF Eval/DCF Eval/AES、峰值均 **NOT_MEASURED**。解析槽数只作容量 shape，不填充实测列，不引用 E15 的 Protocol I 数字作为 III 数字。后续若门关闭，按 `docs/BENCHMARK_VALIDATION_PLAN.md` 用同功能的 III 全两两完整 raw-score→mask 基线配对 n/K/输入/网络/重复，保留分方/阶段原始计数与失败种子，六方案汇总区分共同规模和容量上限。

## 修改文件与下一步

设计/TEST_ONLY 源码检查点 `8d6fc748263fe988b427912b647235916424d547` 修改：

| 文件 | 内容 |
| --- | --- |
| `docs/decisions/M6A_P2_III_E1_AAV86_RING_MASK_DESIGN_GATE_2026-10-04.md` | 先行设计门、失败引理、接口表、泄露、DAG、容量和性能方案。 |
| `VFSS/tests/moe_topk/protocol_iii_aav86_test_only_gate.py` | 隔离明文模型、最小反例和差分夹具。 |
| `docs/IMPLEMENTATION_PLAN.md` | 同步 E1 状态和阻塞，不改变 M6A 总验收。 |

本报告仅加入 `docs/reviews/`，将在独立的事后报告提交中记录。没有修改 `VFSS` secure runtime、冻结的 I/III 对外入口、`VFSS-baseline/`、论文、参考工程、E15 原始记录；无 key、log、PDF、构建物进入差异。检查命令与最终 Git 状态在本报告提交前后复核。

可执行下一研究步是：所有者明确 III 候选的 local-rank/bucket/图/流量许可，或决定隐藏图控制；随后独立写出共享节点 mask 全预发 key、adaptive use、DPF 和双向置换的单方 hybrid；再定义 D≤8 一次性绑定 package/帧并实施三层验证。未获得许可或证明前不实现 secure 候选，不报告 `2r` 已达成，也不开展正式 V3 性能。
