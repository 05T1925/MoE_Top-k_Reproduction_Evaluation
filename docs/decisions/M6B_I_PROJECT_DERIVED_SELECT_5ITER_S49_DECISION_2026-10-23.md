# M6B-I PROJECT_DERIVED_SELECT_5ITER S49 决策（2026-10-23）

## 决定

路线保持 `Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`。输入仍为双方 Q20.12 signed score shares；功能仍为 score 降序、原索引升序稳定同分，并输出原序、恰有 K 个 1 的 Top-K XOR mask shares。S48 已确认的 escrow 正常合同与角色不变。

S48 的 `e05b01d5c7e3899297b76a5e0fca212d38d24229` 是本轮唯一代码/证据基线。S47 FTv3 19 项增量在指定 S47 commit 下逐一匹配 Git blob；S48 九项规范快照摘要重算匹配。未更改运行时代码。

在独立同分布（IID）理想 32-bit 随机字、单次发材的非随机辅助步骤成功、且按 FTv3 固定 retry cap=7 发行时，成功条件下的语义材料投影与精确均匀 Fisher–Yates 理想材料分布完全相同。整次 cap abort 的概率满足 `δ_reject ≤ 1023·2^-154 < 2^-144`。该结论由非作者只读审查者复核；范围不包括真实 OpenSSL 流、完整签名 package transcript 与协议完整安全性。

不把 S42 的 H4 条件 party-view 证明直接迁移到 FTv3。S42 只覆盖 ideal `F_T/v1`、`N≤256` 和旧 schedule。FTv3 更换签名版本元组，并新增批处理 schedule/frame/phase 可见状态；尚无证明表明 v1 的 `Kpre` sampler 在 FTv3 package 和该消息 schedule 下生成相同的条件转录。对 `N=257..1000`，另有 S42 定理范围不覆盖该输入域的明确缺口。二者都是证明未完成，不是攻击或不可能性结论。

真实 `RAND_bytes` 联合流桥仍 OPEN：代码可复算单次 N=1000 profile 的 `Qmax=88,786,404` 字节、`Cmax=7,247` 次调用、单次请求至多 1 MiB；目标部署 provider、熵/seed source、配置、生命周期和并发模型没有证据。`Adv_DRBG_T(Q,C;E)` 保持符号，不用 S48 构建主机 OpenSSL 版本替代部署配置。TLS 1.3/mTLS 和源码身份校验是实现事实，部署信道优势仍 OPEN。

## 门禁

- `IDEAL_IID_FTV3_SEMANTIC_SUCCESS_LAW=CONDITIONAL_PASS_REVIEWED`：仅理想 IID 下语义材料条件律及 cap 失败界。
- `S42_H4_TO_FTV3_N_LE_256=OPEN_WITH_EXACT_GAP`；`S42_H4_TO_FTV3_N_257_1000=OPEN_SCOPE_LIMIT`。
- `DRBG_DEPLOYMENT_PROFILE=OPEN/NOT_AVAILABLE`；`JOINT_RNG_STREAM_BRIDGE=OPEN`；`PRIVATE_CHANNEL_BRIDGE=OPEN`。
- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`。
- S48 N≤1000 用例功能证据为 `INHERITED_PASS_WITH_SCOPE_NOT_RERUN`，不是全域正确性或安全结论。
- 本轮 `BUILD/TEST=NOT_RUN`；正式 LAN/WAN、RTT、AAV86、性能 `NOT_RUN/NOT_MEASURED`；自然业务 I/O 故障继承 `NOT_REPRODUCED`。
- 不创建 `secure` alias，不准入 `N>1000`，不推送、不合并。

## 后续最小工作

先为 FTv3 明确定义与签名包/上下文、burn/abort 语义一致的理想 `F_T/v3`，并给出从固定本方条件到 FTv3 批处理 transcript 的 `Kpre_3` sampler；逐项证明对外可见的 batch/phase/frame 字段由既有公开参数与 `L_S29` 决定，否则暂停并处理泄露合同。随后补齐脱敏目标部署 RNG/TLS profile，再证明全序、适应性请求接口下的 `Adv_DRBG_T(Q,C;E)`。不修改 runtime 或放宽泄露以配合证明。

负责人确认仍需单独记录：合法输入下的低概率 `ISSUANCE_ABORT` 是否可接受。S48 已确认的托管、正常交付和人工故障注入边界不重新询问。