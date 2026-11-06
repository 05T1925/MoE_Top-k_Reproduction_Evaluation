# M6B-I-PROJECT-SELECT-5ITER-S21 决策记录

日期：2026-10-16
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 决策

1. 审查对象是 S18 接收的 S15 默认关闭修复运行时快照，不以其他工作树当前文件代替：72 文件，S15 source manifest SHA-256 为 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816；S18 manifest SHA-256 为 5fffb54d36874fd3b35b30229e77a2de97f86ead78c294b911eb2b1ad59b9f78；source canonical SHA-256 为 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。S21 复算 72/72 文件哈希，无不符。
2. S21 非作者只读审查者为 /root/s12_receipt：未参与 S19/S20 证明推导或撰写，也未撰写 S20 报告；但曾以非作者身份只读审阅 S20 final bridge/report，因此 S21 是该审查者对同一问题的延续复核，不是一个新的独立第二意见。其较早参与 S12 功能接收不涉及 S19/S20 证明推导。根作者曾撰写 S20，作者自审不算独立签核。S21 独立于证明作者，但不声称新增第二位密码学审查者。审查只覆盖 carry→sign 断点及其表达，不构成完整协议安全签核。
3. reviewer 初稿中出现的 2^53 score ring 不匹配实际代码。S15 protocol_i_score_input.cpp 明确 kScoreBits=34，因此 carry/sign 输入、mask、operand frame 和 lift 都在 R34=Z/(2^34) 中；40/41 位仅用于 Select/membership keys。S21 以 R34 重写关系。位宽更正不改变证明裁决。
4. 实际 carry 输出 c_b 是本方 DCF share，未打开；carry_open 仅被声明而未使用。其低两位进入 lift_b=x_b−2^32(c_b mod 4)，并进入 phase 5 sign operand frame。已打开比较位可用 c−u_b 生成的结论仅适用于确实打开且由 correctness 决定的 bit，不能套用到 carry share。
5. 在理想独立 mask 随机带下，只条件于 carry state 的 peer sign-right frame 有边缘均匀性；这不足以模拟包含 phase-5 两个 operand frames、可恢复 M_L、本方右 mask share、sign key 和 carry continuation 的完整 party view。P0/P1 均缺 sign-key/carry-key 顺序 hybrid 或等价 continuation leakage sampler。首个未闭合点是 carry→sign 条件联合分布；这是证明缺口，未发现可复现攻击。
6. 因 carry→sign 前置引理未闭合，按任务规则不继续宣称后续自适应状态、整池完整 transcript 或 shuffle/output/crash 组合已通过。N=128 与 N=256 key-only pool 计数仅作为既有条件投影重新核算；adaptive full-pool view、M2CM/inverse/membership/output share/lease/crash 仍 OPEN/NO_GO。
7. 未发现可复现运行时缺陷，不改运行时代码，不复跑 S18 完整功能矩阵，不构建或运行测试。S18 指定样例的 no-complete-mask 仍是有限范围证据，不称为原子发布或 all-or-nothing。不得创建 secure alias。
8. 五个算法迭代、最多九个明文比较依赖层和完整 raw-score→mask 在线交换数分开登记；端到端消息交换数仍 NOT_CLOSED。M6A V3 仍 OPEN、V4 未获准，正式 LAN/WAN 矩阵 NOT_RUN。

## 下一步最小任务

版本化发起 S22：针对固定 S15 快照中的 34-bit score adapter，分别为 P0/P1 形式化 carry share、lift、sign 两个 operand frames、sign M2B1/v1 key 与本方 mask shares 的联合实验。给出只使用本方 input share、理想 output share 和获准泄露 L 的 continuation sampler，并逐步证明其在 sign-key-first/carry-key-second hybrid 下保持正确条件分布；若不能证明，明确设计新的 score-conversion ABI，使未打开 carry share 不再成为下一次公开 operand frame 的条件输入。S22 不实现、不增加泄露、不改变旧 ABI，除非新决策明确批准。该引理经非作者复核后才进入两种材料池及 shuffle/output/crash 组合。
