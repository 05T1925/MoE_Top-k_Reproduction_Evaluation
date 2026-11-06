# M6B-I-PROJECT-SELECT-5ITER-S19 决策记录

日期：2026-10-16  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决策

1. S19 以 S18 接收的 S15 运行时快照为对象。重新校验 72/72 文件 SHA-256 及 canonical SHA-256：`c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`。本轮没有构建或重跑 S18 功能矩阵，不把继承证据改称 S19 新运行。
2. 对 M2B1/v1 的条件 UCMP 视图引理，S19 给出辅助输入提升：BCG+21 FSS key-only 论证对每个固定 `alpha,beta,party` 与非均匀区分器成立；在整段随机源按理想独立均匀 tape 建模时，固定与 key 根独立的本方非 key 辅助状态、各方实际发送/接收的 masked operand-share frames、其合并值 `z`、两次 Eval 点和 correctness 固定的 opened bit，逐点应用定理并取期望。同一 key 的两个 Eval 是同一个模拟 key 的确定性后处理，不被当成独立 key。模拟器用随机结构化 key 计算本地 share，再以 `c-u_b` 产生接收 share。结论依赖 AES-128 PRP 和未量化的 `epsilon_RNG_joint`，标为 `CONDITIONAL/UNQUANTIFIED`，不是无条件安全。
3. 对 N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` 及 N=256 `M6S2/v2`，分别给出整池 hybrid；所有已用、未用和被拒绝 key 均纳入。地址由获准公开的 seed/comparison transcript 决定，正确性使 opened bits 不依赖 DCF 根随机带，故访问自适应性是该公开前缀的后处理。优势系数按所有 DCF 输入宽度求和，分别为 `1,639,424` 与 `6,719,104` 个 PRG 输出替换项。
4. 对 M2CM/v1 两阶段 shuffle，分别为 P0/P1 给出条件模拟。源代码中的本地 `sigma/tau` 对单方边缘是与共同 `Pi/Pi^-1` 独立的均匀置换；`a/e/r` 与本地 shuffle share 可从获准 masked frames 和本方输入/输出 share 按条件分布生成。逆路由输出 share 由理想功能给模拟器；随机 pad 使其与目标 mask 独立。完整条件界为 `(Σw_j)ε_G + epsilon_RNG_joint + epsilon_channel`；理想私有认证通道模型下 `epsilon_channel=0`，实际 TLS 实例项未量化。该组合仍依赖同一个未量化随机源假设，以及 leakage L 确实包含代码发送的 masked Y/Q/operand-share 帧、逐坐标合并值、固定 phase/length 和获准 comparison transcript。
5. 联合单方视图在明确列出的 AES/RNG、可信离线 T、私有认证通道、单方腐化和泄露合同下裁为 `CONDITIONAL`。没有找到可复现攻击或 runtime/合同缺陷；没有因此创建 `secure` alias。S19 证明尚未由另一位非作者密码学审查者复核，墙钟侧信道也未测，故路线保持 `EXPERIMENTAL`。
6. 输出合同仍仅为测试范围内的 `no-complete-mask`。后期崩溃可留下本地方 XOR share；不声称原子发布或 all-or-nothing。正式性能仍不准入：M6A V3 OPEN，V4 未获准。

## 源码桥接要点

- `M2B1/v1` 使用 BCG+21 Figure 1 的 DCF：根控制位固定为 party id，两个根 seed 由 `RAND_bytes` 产生；每层 `G` 输出按 MSB-first 解析；`Convert_G` 对 `Z/(2^64)` 取 128-bit seed 的高 64 bit；输入域 `w=34..53`，输出群为 `Z/(2^64)`，uCMP 的 `beta=1`。
- 本地 `G` 把五个不同 AES-128 输入块的密文串接并截取 514 bit；故在 AES-128 PRP 假设下，单次 G 的 PRG 优势记为 `epsilon_G <= Adv_AES128^PRP(T',q=5)+10/2^128`。按 Appendix F.1 的逐层 hybrid，单 key 宽度 `w` 的 key-only 差异至多 `w*epsilon_G`；power-of-two 64-bit 群的 `Convert_G` 在理想均匀 seed 下无额外转换误差。序列化只在这些随机字段外加固定头、计数和 control padding。
- S19 不把 `epsilon_RNG_joint` 实例化成统计独立、无条件均匀或具体安全位数。它必须联合覆盖离线 `getrandom` 掩码/置换/掩码份额、OpenSSL `RAND_bytes` DCF roots、在线逐活动迭代 sample seed、拒绝采样调用顺序及所有获准可见辅助状态。

## 最小后续工作

由未撰写 S19 模拟论证的人逐项复核本决策及配套报告的定理量词、混合优势求和、P0/P1 shuffle 条件采样和失败前缀处理；另行给 `epsilon_RNG_joint` 指定可审计的平台/对手实验或保留为未量化条件。复核完成前继续保持 `EXPERIMENTAL`。无需重做 S12/S14 功能实现或 S18 已完成的接收矩阵。

详细推导、门禁、源码哈希与未运行项见 [S19 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S19_REPORT_2026-10-16.md)、[机器可读门禁](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S19_GATES_2026-10-16.json) 和 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S19_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-16.json)。
