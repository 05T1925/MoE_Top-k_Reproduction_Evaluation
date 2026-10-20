# M6B-I PROJECT_DERIVED_SELECT_5ITER S4 决策记录

日期：2026-10-12  
身份：`PROJECT_DERIVED_SELECT_5ITER`  
组合：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF`  
状态：**`EXPERIMENTAL_FUNCTIONAL_CANDIDATE=PASS_BOUNDED`; `JOINT_VIEW_SECURITY=OPEN`; `FORMAL_PERFORMANCE_READY=NO`**

## 决策

1. 按本轮确认的路线泄露合同开放并实现以下字段：打乱坐标的比较 endpoint 和 pivot、key slot 地址、比较结果 bit、比较数和通信长度、双方统一 ABORT，以及本路线已批准的 masked operands。由此双方可公开、确定性计算 sample order、pivot rank、active candidate、residual rank 和下一访问图，不新增 secret rank mux，也不发送 score、阈值 key 或原序 index 映射。
2. 接受 `N≤128`、输入无关可信离线 T、五轮 `PROJECT_DERIVED_SELECT_5ITER`、strict-`<` DCF、membership 和 inverse routing 的有界实验候选。N=128 实际 party package 每方 `60,008,580B`，小于 64 MiB cap。
3. T 在发材时会知道所生成的随机合成置换 `Pi` 与 mask；这是因为现有 C-Instantiation dealer API 接收/生成完整 `Pi`。S4 将 T 视为受信任离线预处理方，保证 T 不接触输入和在线 rank/threshold/output。P0/P1 消息不发送 `Pi`，但本记录不声称对 T 隐藏 `Pi`，也不把 S31/S32 shuffle 证明转移给 T 视图。若项目合同要求 mapping 对 T 也隐藏，本实现不能满足，应先替换 shuffle preprocessing。
4. 同进程一次性 claim、每个 uCMP key 的 `used_`、迭代/端点槽描述和重边拒绝进入实现；跨进程重启的持久 anti-replay ledger 尚未实现。每次 session/material ID 目前要求由调用方/材料池持久保证唯一。
5. 通过明文 DCF membership、控制器 conformance、冻结 oracle 和 T/P0/P1 独立进程功能 E2E 后，允许保留为显式 opt-in `EXPERIMENTAL` API。禁止创建 `secure` alias，禁止接入默认排序 pipeline，禁止宣称安全证明、论文三轮或正式性能已通过。
6. S1 BB90 来源闭合 NO-GO、S2 明文 Stage A、S3 当时的 `L_S3` NO-GO、S34 接收状态、M6A V3/V4 门均保持原历史。S4 是在新泄露合同下推进 project-derived 新路线，不是改写 BB90 或 BMW16。

## 当前状态门

| Gate | 决定 |
|---|---|
| `S3_RECEIPT` | `PASS_WITH_SCOPE`：复核报告与源工具哈希；S3 artifact 未合入 main，非异会话接收 |
| `LEAKAGE_L_MAPPING` | P0/P1 字段 `PASS_WITH_TRUSTED_T_SCOPE`；T 知道随机 Pi 的边界需纳入明确威胁模型 |
| `MASKED_OPERAND_REVIEW` | 实际代码公式与一轮一密材料已核对；joint-view proof 开放 |
| `PUBLIC_CONTROL_DERIVATION` | `PASS`：由批准 endpoint/bit/seed 可确定性恢复控制态 |
| `MATERIAL_ABI` | `PASS_N128_SIZE`；cross-restart re-claim `CONDITIONAL` |
| `DCF_MEMBERSHIP` | `PASS_BOUNDED`：conformance + frozen stable oracle E2E |
| `MESSAGE_DAG` | `PASS_FUNCTIONAL`：完整 online exchanges 计量；TLS 未集成 |
| `FUNCTIONAL_E2E` | `PASS_BOUNDED`：T/P0/P1 独立进程，oracle、tamper-header、reclaim、disconnect/no-mask |
| `JOINT_VIEW_SECURITY` | `OPEN`：自适应图、masked z、unused pool、T view、shuffle/inverse、ABORT/output 组合未证明 |
| `FORMAL_PERFORMANCE_READY` | `NO`：未运行 LAN/WAN，V3/V4 前置状态不变 |

详细数据流、byte layout、测试种子、计量与源码 hash 见 [S4 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S4_REPORT_2026-10-12.md)。

## 后续准入条件

继续安全/网络阶段前，必须：

1. 冻结 adversary 与 T trust boundary，明确 mapping 是否仅对 P0/P1 隐藏。
2. 对 `L_S4`、masked operands、比较结果、adaptive access/unused slots、T setup、forward/inverse shuffle、abort 和 mask output 构造组合 simulator 或正式论证。
3. 为 material package 实现可审计的 durable single-use lease/anti-replay；同 session 重启不能重领 key。
4. 将 T package 与 P0/P1 在线传输接入认证 transport，并将 authenticated field binding 写入 protocol frame/version。
5. 之后先跑新的 independent security review 和 failure/E2E hardening；正式 LAN/WAN 仍必须等待 V3/V4 门。

