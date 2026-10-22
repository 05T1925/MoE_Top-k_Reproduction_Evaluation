# M6B-I-PROJECT-SELECT-5ITER-S43 决策

日期：2026-10-22
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定

1. 固定 S42 证据为本地提交 `72a23fc067b5bfbf10edf110eeff45ec1f0ed6bd`（tree `1d72a0942adff4a39253a73f4a12e2989559064e`），父级为 S41 文档提交 `486026ba425a2d17ca15173d056db6eb309747d1`。S42 的 85 项源快照重算无差异，canonical SHA-256 为 `2ea2896cce7dd284f3d317a7462dde528c188cc15cb32af16c744dad4e5e6837`。S41 汇总 divergence 另见 S43 erratum，历史记录未修改。
2. S35/S39/S40 的来源关系被区分：S35 运行时为 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`；S39 运行时为 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`；S40 在该 runtime 上只增改 TEST_ONLY E2E harness；S42 只做治理/证明文档。被审 Dealer 实现采用 S39 固定对象，未以当前工作树的相似代码替代。
3. Under an ideal IID uniform byte tape, the S39 Dealer joint law of all Beaver triple shares, Fisher-Yates permutation and one-hot matrix XOR shares equals the S35 `F_T/v1` material law. Package/context digest/signature bytes couple exactly only when the public `receipt_format_version` parameter matches. Within the reviewed Dealer GMW source/header, S35-to-S39 changes add receipt-v2 support/validation; sampling code and encoding routine/layout are unchanged, although v2 sets the signed context state field from 1 to 2. A strict comparison against the S35 v1 context therefore has this explicit deterministic field difference; `F_T/v1` must receive the matching public state parameter to claim byte-level equality. The overall VFSS diff has seven tracked paths, not only these two Dealer files.
4. `F_T_TO_DEALER_JOINT_DISTRIBUTION` 仅为 `CONDITIONAL`：先要求理想接口将公开 `receipt_format_version` 固定为与真实签名 context 相同的值；在此参数匹配后，概率桥的首个断点是把 OpenSSL `RAND_bytes` 联合输出替换为理想独立字节带。源码检查 RAND 失败返回，但没有本项目可量化的 `Adv_DRBG_T(q)`、部署 entropy/provider/state/reseed 证据或符合所需优势界的离线随机源证明。未将这些缺口称为攻击或已证明不可能。
5. Ed25519 package/context purpose-domain、format-version、规范序列化、字段、材料摘要和双包摘要映射在固定验证密钥、理想签名接口及 receipt-format 参数匹配时为 `PASS_WITH_SCOPE`。签名/摘要是对耦合材料的后处理，不用“哈希看起来随机”作隐藏论证。恶意发行者、恶意方篡改、SHA-256 抗碰撞及 Ed25519 抗伪造不在单次诚实发材分布等式中获证明。
6. one-shot lease 的本地成功路径可逐字段映射：相同 package hash/session/invocation/party 在同一完整且不回滚的 lease DB 中唯一；成功插入在 READY 前提交，之后材料变动会失败但保持 burn。对 DB 回滚/删除、复制到新 lease DB、同 tuple 再发一套新 package、跨路径/跨目录并发发材、发材中途崩溃后的原子恢复没有全局合同或本轮故障证据，`ONE_SHOT_CONSUMPTION` 保持 `CONDITIONAL`，不宣称掉电耐久。
7. `next_uniform` 的拒绝采样返回值无偏，但循环无最大次数，因此严格 worst-case PPT 需要明确允许 expected-PPT，或在新版本采样接口中限制重试并把 `δ_reject` 计入；本轮不改 runtime。
8. 本轮执行者参与 S35–S42 工作，属于作者。非作者 `/root/s39_crypto_review` 未参与本轮证明撰写，已只读复核 ideal/real 联合律、材料独立性、拒绝采样、签名/tuple 映射及 lease/restart 边界；理想 IID tape 下单次成功发材联合律为 `PASS_WITH_SCOPE`，真实 DRBG bridge 仍 `OPEN/CONDITIONAL`。复核者未改文件、未构建、未运行测试；其意见要求的措辞澄清已并入报告，但未要求复核编辑后的报告。
9. 无可复现运行时缺陷；本轮不改 runtime、不构建、不测试、不运行 211/195 案矩阵或正式 LAN/WAN。自然业务 I/O 故障继续 `NOT_REPRODUCED`；任意 crash-prefix 隐私未证明；N>256 `NOT_ADMITTED`；正式性能/RTT `NOT_RUN / NOT_MEASURED`；secure alias 未创建；v3 默认 OFF；不修改 `VFSS-baseline/`，不推送、不合并。

## 门禁裁定

- S42 source receipt：`PASS`。
- ideal tape 下的材料联合采样为 `PASS_WITH_SCOPE`；package/context 的逐字节绑定以 receipt-format 公共参数匹配为前提。
- 真实 `RAND_bytes` 到理想均匀带：`OPEN`，符号优势 `Adv_DRBG_T(q)` 未量化，是整体 `F_T_TO_DEALER_JOINT_DISTRIBUTION` 的首个断点。
- 签名/材料/上下文绑定在同一 receipt-format 参数下为 `PASS_WITH_SCOPE`；签名真实性的对抗性密码学门仍独立。
- one-shot lease：完整 DB、单 package 的成功/重复拒绝逻辑 `PASS_WITH_SCOPE`；跨重启/回滚/重复签发语义 `OPEN`。
- private-channel、party-view 完整组合、secure alias、正式性能：仍 `OPEN / NOT_READY`。

详细推导、源码位置、逐步 hybrid 和风险边界见 S43 报告、gates 与 source/build/evidence manifest。
