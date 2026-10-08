# Protocol I + BMW16-derived Select + DCF：实验功能交接

日期：2026-10-08<br>
身份：`PROJECT_DERIVED / EXPERIMENTAL`（默认关闭；非 secure API）<br>
运行时源码身份：S22 `9210c11a7a877f8fe3687e0509c1722440cf63fe`；S23 是文档/复核提交，S24 同样未修改运行时代码。

## 接口边界

| 角色 | 当前接口 | 输入与合同 |
| --- | --- | --- |
| 离线 T | `protocol_i_bmw16_experimental_material_generate_streaming(config, slot_sink)`；`experimental_bmw16_stream_store` writer；`party-node t-tls` | 只接收公开 `session/fingerprint,n,K,padded_n,index_bits,comparison_bits` 等配置；逐槽 KeyGen 并把两方 slot 交给离线封装/交付。T 不收在线 shares、排名或输出；party 启动门等待 T 和双方 receiver exit 0 |
| P0/P1 | `protocol_i_bmw16_experimental_raw_score_mask_party(config, material, raw_score_share, score_fds, forward_shuffle_fds, select_round_fds, inverse_shuffle_fds, final_agreement_fd, sampling_coin_fd, test_only_tape)` | 每方输入 `uint32_t` signed Q20.12 raw-score 加法份额；调用正式 raw adapter、同置换 shuffle、两份并行 Select 的四层、DCF-backed membership、inverse shuffle。party 不重构 score/index/selected key/mask |
| 验证控制器 | `experimental_bmw16_party_node_e2e_test` 与隔离 oracle verifier | TEST_ONLY，可准备输入/随机带、启动三个进程并重构输出份额核对冻结 oracle；不是任一 party，也不属于协议安全 view |

接口定义：`VFSS/include/moe_topk/experimental_bmw16_select_party.h:53–203`。编译需显式启用 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`；默认 OFF，未启用时头文件拒绝 include。`MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS` 仅可在 `BUILD_TESTING=ON` 下启用。

## 数据、材料和错误合同

S22 根据用户转述的项目决定，将 public_z/masked operands、匿名比较端点与打开的比较 bit、pivot/U/V/W、两个 Select task 的共同匿名句柄关系、selected anonymous handle、用途/端点派生的 key ID 与实际顺序、比较次数、通信长度及双方统一的算法 abort 状态列为候选公开 transcript。该批准界定可讨论的泄露边界，不是对该 transcript 联合安全的证明；S24 未增加新的公开字段。

- 项目优先键语义是 signed Q20.12 分数降序、`original_index` 升序；index 随 record 在同一隐藏置换下绑定。返回值是每方原输入顺序 XOR bit-mask share。
- SUCCESS 的接受合同：长度 n、重构后恰 K 个 1 且逐位匹配冻结 oracle。正常算法概率 abort 是协议状态，不输出 mask；材料/通信错误及工程不变量错误分别分类。对端未确认的失败只能标 `LOCAL_ONLY`；不能假称双方达成一致。
- 每个 Select/membership pair slot 是独立 uCMP key pair；两次 DCF Eval 属于同一个比较。T 的流式 Select/membership sidecar 槽公式为 `9·C(n,2)`；完整 `offline_material_slots_per_party` 另含 raw 与 forward/inverse shuffle 材料。不能混用 KeyGen pair 调用数、sidecar slot 数与实际 Eval 数。
- Party 不重构原始 score、original_index 到匿名位置的映射、selected stable key 或完整明文 mask。上列打开的比较位按已批准的候选泄露合同由双方可见；不能描述为秘密比较位。
- 代码形状可表示 `n≤2^20`、`K∈[1,n]`，comparison width 为 `33+index_bits≤53`；可表示域不是资源认证或已运行域。旧内存生成器和当前独立接收资源矩阵更小。n=1 有 shortcut，不走 n≥2 的 Select/材料分支。

## 当前接收范围和证据身份

| 证据 | 被测身份 | 接收口径 |
| --- | --- | --- |
| S23 独立复建/回归 | S22 runtime tree `9210c11…`；S23 `7b78116…` 只增文档 | 39 条 common-tape、定向 CTest 8/8、不同 UID TLS/non-TLS bounded E2E；含 n≤256。报告与外部 hash 索引见 [S23 验证报告](BMW16_S23_INDEPENDENT_ACCEPTANCE_SECURITY_AND_SAMPLER_REVIEW_2026-10-08.md) |
| S22 历史材料 | 同一历史 runtime 源，但不同运行/证据窗口 | 有一次 n=1000/K=80 historical E2E；不等于 S23 或 S24 重跑，也不是正式性能记录 |
| S24 相关 C++ 回归 | `7b78116…` tree 中 runtime 未改；fresh build 证据见 S24 报告 | DCF/uCMP/party conformance；不重跑多 GB n=1000；S4 differential 与独立进程历史接收引用 S23 并明确保持历史身份 |

功能证据不推出 BGI15 key privacy、相关整池自适应隐私或 Protocol I shuffle/output 联合模拟。S24 的 DCF 逐式桥接裁决见 [S24 DCF source bridge report](BMW16_S24_DCF_SOURCE_BRIDGE_AND_FUNCTIONAL_HANDOFF_2026-10-08.md)。正式 LAN/WAN、V4 与六方案比较指标仍 `NOT_RUN / NOT_MEASURED`。
