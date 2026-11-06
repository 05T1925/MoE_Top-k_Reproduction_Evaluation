# S22 独立密码学复核记录

日期：2026-10-16
审查者：`/root/independent_dcf_review`
范围：S18 锁定的 S15 源码中的 R34 carry→sign **局部 pre-output continuation**。
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 独立性与对象

审查者明确声明：未参与 S19、S20、S21 的证明撰写、推导或决策。审查者自行核对 S15 72 文件，并重算 canonical SHA-256：

`c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`

该值与 S18 receipt manifest 一致。该独立性仅用于本记录标注的分析范围，不代表完整协议的独立安全接收。

## 复核结论

**CONDITIONAL PASS：P0 与 P1 的局部 R34 carry→sign pre-output transcript continuation。**

源码中，UCMP 每阶段设 `alpha=(mask_left-mask_right) mod 2^34`、`beta=1`，见 `protocol_i_ucmp.cpp:40,51`。carry 阶段 P0 的 operand base 为 `(0,x_0)`，P1 为 `(2^32-1-x_1,0)`，见 `protocol_i_score_input.cpp:95–98`；随后实际更新为

`lift_b = x_b - ((carry_b & 3) << 32) mod 2^34`

见 `protocol_i_score_input.cpp:109`。sign 阶段 P0 base 为 `(2^31-1,lift_0)`，P1 为 `(0,lift_1)`，见 `protocol_i_score_input.cpp:117–119`。priority-key shares 的实际计算位于 `protocol_i_score_input.cpp:129–141`。

对任意腐化方 `b`，在理想联合随机带下，stage-2 masks/party shares 和 DCF roots 是新生成的。`share_mask` 先采样一个 share，再由 full mask 减出另一 share；见 `protocol_i_project_derived_select5.cpp:910`。sign 阶段的 masks/key 在 online evaluation 前生成，见该文件 `:922` 与 `:1356`；DCF roots 由 `RAND_bytes` 生成，见 `dcf_bcg21_fig1_aes128_v1.cpp:253`。因此，在 joint `getrandom`/`RAND_bytes` idealization 下，给定 carry history 后 sign roots 仍为新鲜值；peer sign masked frames 可逐项按均匀 R34 值采样，无须读取 peer carry share。

固定 `alpha,beta,party` 后，按 BCG+21 的单方 key-only theorem 替换相应 party key；同一 key 的两次 Eval 及本地 share/`lift`/`q_b` 计算是该 key 和 operand 的确定性后处理，不能当作两个独立 key。sign-stage 单 key 替换项为

`34·epsilon_G + epsilon_RNG_joint(prefix)`，

carry 与 sign 两次替换的候选总项为

`68·epsilon_G + epsilon_RNG_joint(carry/sign prefix)`，

其中

`epsilon_G ≤ Adv_AES128^PRP(T′, q=5) + 10/2^128`。

`epsilon_RNG_joint` 必须覆盖跨阶段 `getrandom` mask/share 与 OpenSSL `RAND_bytes` roots 的联合输出及 party-visible 辅助状态，不能仅用单次调用边缘随机性替代。`epsilon_RNG_joint`、AES 对手运行时间 `T′` 和具体优势均未量化，故这里只是条件界，不是实现级安全位数。论文依据：Boyle et al., Figure 1、Theorem 2、Appendix F.1；[Microsoft Research 作者稿 PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。该定理只支撑固定函数参数下的单方 DCF key 替换，不自动证明 UCMP、pool 或 Protocol I 联合视图。

## 首个未闭合步骤

本次复核**没有**给出固定理想最终 output share 的模拟器。carry/sign 的局部 sampler会按替换 key 生成 `q_b`，但没有证明它与任意给定理想 share `Q_b` 的正确条件分布。源码中 `keys[i]` 于 `protocol_i_project_derived_select5.cpp:1901–1906` 被封装为 `key_records[i].word0` 并进入 `run_two_round_shuffle`；之后 shuffle 输出进入 `shuffled_keys`（`:1911–1913`），再用于 Select 比较（`:1921` 起）。缺少的首个组合引理是：由允许输入 `(x_b,Q_b,L)` 生成与真实视图同分布的 `q_b/key_records` 及 forward-shuffle 本地状态。

该点是**证明缺口**，不是攻击。本次没有构造输入区分器，也没有找到可重放攻击见证。

## 适用与排除范围

- 结论只涉及 S18 锁定的 S15 源码和 R34 carry→sign 局部 pre-output view。
- N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1`、N=256 `M6S2/v2` 的 pool 不在本结论范围内。
- 不包括 output-share-conditioned simulator、后续自适应访问、shuffle/inverse、membership、crash/abort 组合。
- 审查者提示一份历史 exact-leakage audit 仍写有 approval 未确认；本轮以用户 S22 请求中已明确批准的泄露字段作为当前 `L`。此处应作为文档同步差异记录，不应把批准字段等同安全证明。
- 未运行构建、测试、协议 trace、攻击实验或正式性能实验。


