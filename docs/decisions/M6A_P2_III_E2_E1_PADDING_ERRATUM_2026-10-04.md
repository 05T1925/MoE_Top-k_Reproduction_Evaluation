# M6A-P2-III-E2：E1 padding 事实勘误

日期：2026-10-04。撤回 E1 所称“现有 score adapter 默认零值 dummy，因此正式 III 路径在 n=3/K=3 全负分数时只选两位”的源码事实与对正式入口的推论。保留 E1 的零值 dummy 明文夹具，身份改为**反事实错误接线**：若某个未来实现自行把 dummy 设为 score 0，夹具给出失败例；它没有调用现有 adapter，不能用于指控已签收的 Protocol III 入口。

直接核对 `main@c3926c68` 和 E1 检查点 `67739472` 的 `VFSS/src/moe_topk/protocol_i_score_input.cpp`，两处 blob 均为 `04d750044ba80b36df1b8a7a8280129d6e3a6882`。第 64–66 行先放真实加法份额，然后对每个 padding 槽写 `P0=0x80000000, P1=0`，重构为 signed `INT32_MIN`。`protocol_i_priority_key` 将原下标加在 score 优先级后；真实 `INT32_MIN` 与 dummy 同分时，真实下标小，故 dummy 仍居后。两处 `protocol_iii_grank.cpp` blob 均为 `39017015834db4830ad2ca35fd29fea4c13cc518`；`protocol_iii_grank_party` 只为 `logical_n` 个真实槽打开 masked keys 和比较，既有正式 `protocol_iii_raw_score_mask_party` 不把 padding 槽纳入 GRank 或 DPF 输出。

定向实调验证：在 `VFSS/tests/moe_topk/protocol_iii_raw_score_mask_test.cpp` 增加 n=3、K=3 的 `[-1,-1,-1]` 与 `[INT32_MIN,INT32_MIN,INT32_MIN]` 两例。每例先调用正式 raw-score→mask 入口，检查原序恰 K、冻结 oracle；再由现有 helper **直接调用真实** `protocol_i_raw_score_input_party`，逐槽重构并核对 `protocol_i_priority_key`（含 D=4 的 dummy），且与 M3 原序 mask 参考比对。WSL Ubuntu 24.04、CMake Debug/EMP OFF 的 `moe_topk_m5_fix_f1_raw_score_mask_test` 为 142/142 cases PASS，CTest 1/1 PASS。该验证是功能证据，不是 III+AAV86 的安全证明或性能测量。

E1 仍可引用其隐藏 handle rank 归纳、ring 低位同态和逆路由代数的**条件性**研究义务；但功能门不再因上述错误的 padding 断言阻塞。III+AAV86 的公开字段合同、全池联合单方视图、真实消息 DAG 与容量仍须在 E2 单独审查。勘误显式覆盖 E1 决策、E1 报告和实施计划中的同一错误，不改变 E15/E16/E17 原始数据或已接收的 Protocol III F1 入口。
