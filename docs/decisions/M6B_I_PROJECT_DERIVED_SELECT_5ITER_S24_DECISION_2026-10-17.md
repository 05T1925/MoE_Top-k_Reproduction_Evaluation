# M6B-I-PROJECT-SELECT-5ITER-S24 决策

日期：2026-10-17
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
源码基线：S18 已接收的 S15 72 文件快照，canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。
S24 执行者：参与 S19–S23 推导；因此 `INDEPENDENT_REVIEW=NOT_SATISFIED`。

## 决定

1. 将 S15 运行时的 score conversion 输出定义为完整 priority-key 加法份额 `Q_b ∈ R_c^N`，其中 `c=33+h`、`h=index_bits_for(padded_n)`。在 carry/sign DCF correctness 下，`Q_0+Q_1=κ`，且 `κ_i=((0x7fffffff-score_i)<<h)+i mod 2^c`。该定义描述当前实现诱导的 share law，不假设单方份额均匀。
2. v1 每方份额低 `h` 位分别为 `i` 和 `0`。因此 `q_b=Q_b>>h mod 2^33` 在 v1 下可重构高位，因为低位相加不进位；这只是可推导映射，当前运行时后续消费完整 Q，不依赖独立 q。
3. 接受作为项目派生、版本化的 v2 实验设计：逐记录取 `rho_0←U(R_c)`、`rho_1=−rho_0`、`Q'_b=Q_b+rho_b mod 2^c`。完整键重构不变，signed-score 降序、original_index 升序和最终稳定 Top-K mask 不变。
4. 禁止将 `Q'_b>>h` 作为 q share。局部右移不与加法重构交换；当低位跨界时，两个局部商相加会少进位。v2 consumer 必须使用完整 `Q'_b`。
5. 以 M6B5/v4（N≤128 monolithic）与 M6S2/v3（N=256 streamed）保存全新的 refresh materials；v1 package/schema/路径保持兼容。刷新槽绑定 session、party、purpose、记录端点及已有 one-shot lease；没有在线 refresh message。
6. 在 ideal joint RNG tape 与 S22 范围内的 carry/sign pre-output sampler 假设下，刷新后完整本地 score-conversion view 对给定 `Q'_b` 的模拟为条件成立。实际 `epsilon_RNG_joint` 必须新增覆盖刷新抽样及完整 dealer/party 辅助状态，当前未量化；因此只记为 `CONDITIONAL/UNQUANTIFIED`，不记安全 PASS。
7. v2 功能路径已实现并完成定向正确性回归；feature-OFF 静态库无 v2 helper 符号，OFF build 不注册 v2 测试目标。仍保持 EXPERIMENTAL，不创建 `secure` alias。
8. 条件模拟可延伸到给定 `Q'_b` 后的确定性 `q'_b/key_records'`，并可在获准泄露包含相应 masked frame 与 masked aggregate `Y`、且 shuffle tape 符合独立理想模型时，覆盖 S23 固定键 forward-shuffle 的本地视图；这不证明完整材料池。首个后续断点是由样本选择的首次自适应 DCF 比较（同一 key 的两次 Eval、公开比较位及之后的自适应地址），其联合条件分布未闭合。inverse、membership、最终 output-share、crash/abort 组合仍 OPEN。V3 仍 OPEN、V4 未获准，正式 LAN/WAN 性能矩阵 NOT_RUN。

## 首个需保留的代数边界

取 `h=1,c=34,Q_0=Q_1=0,rho_0=1,rho_1=2^34−1`。则 `Q'_0+Q'_1=0 mod 2^34`，但 `floor(Q'_0/2)+floor(Q'_1/2)=2^33−1 mod 2^33`。这是“局部 q 解码失败”的代数反例，不是协议攻击。当前 S15 consumer 不读取 q，因此没有通过补丁掩盖该冲突。

## 下一项

由未参与 S19–S24 的密码学审查者，先审查 v2 的 F_scoreconv 单方输出条件模拟及随机带假设；若其通过，再独立审查两个材料 schema 的 adaptive pool 与 shuffle/output/crash 联合视图。正式性能仍遵守既有 V3→V4 前置，不在 S24 启动。

证据见 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S24_REPORT_2026-10-17.md`、配套 gates 与 source/build/evidence manifest。
