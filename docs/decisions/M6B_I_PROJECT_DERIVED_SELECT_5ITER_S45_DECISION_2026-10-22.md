# M6B-I-PROJECT-SELECT-5ITER S45 决策

日期：2026-10-22
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL

## 决定

1. 从 S44 精确提交 `aaa7f96095c350e8236246163d48c54a7fb8d9ba`（父提交 `a1b02a8133955786e0bb6b047f55b746af66acbf`，tree `7bb41c0d57ed1158aa90764781ad523d83beab58`）的隔离 worktree 开始。S44 的 S39 Dealer/runtime 来源 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20` 未被主工作区替换。S43/S44 历史记录和 S41 provenance erratum 保持原样。
2. 新增默认关闭、显式构建开关 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL`，并提供独立的 `score_to_mask_v3_dealer_issue_packages_ft_v2_bounded` 入口。F_T/v1 仍用 package format 4 和旧无界 sampler；F_T/v2 使用 package format 5、`ft_version=2`、`sampler_version=1`、`lambda=128`、`retry_limit=6`。receipt-format 仍是 context 中的独立字段，可分别为 1 或 2；不将 F_T/v2 与 escrow receipt/v2 混用。
3. Fisher–Yates 的每个 32-bit 抽样最多尝试 6 次。所有尝试被拒绝时，在发材入口统一抛出 `ISSUANCE_ABORT`；此点在 party package/context/signature 持久化之前。实现先在由 T 拥有的 0700 registry 目录中原子登记并 fsync session/invocation burn marker；该内部 marker 会保留以阻止 F_T/v2 同根目录下的重发。此保证依赖部署固定该 registry 根目录；F_T/v1 不读取此目录，因而不是跨版本、跨目录或防回滚的全局一次性证明。尚未交付的随机材料缓冲区没有显式清零保证。
4. 对 `m≤256`，`rho_i=(2^32 mod i)/2^32 < 2^-24`，至多 `m−1` 个选择位置。取 `r=ceil((lambda+log2(m−1))/24)`，`m=1` 时令 `r=0` 且不取样；固定 `lambda=128` 时 `2≤m≤256` 均得 `r=6`。整次发材拒绝概率满足 `delta_reject≤(m−1)2^-144≤255·2^-144=(255/256)·2^-136<2^-136<2^-128`。成功条件下每个接受 residue 仍严格均匀；有限失败概率是统一发材中止，而非错误 mask。
5. `delta_reject` 只界定去掉 package/sampler version、签名与编码元数据后的语义材料投影 `pi_mat` 与永不失败理想语义发材之间的统计距离。它不界定完整签名 package/context/transcript，尤其不跨不同版本字段作全包 TV 声明。匹配版本的真实 RNG 到理想 IID tape 仍需计算假设 `Adv_DRBG_T(Qmax,Cmax;E)`；本轮未取得实际 provider/熵源/部署生命周期证据，`JOINT_RNG_STREAM_BRIDGE=OPEN`。
6. 最终 Release default-OFF、显式 F_T/v2 opt-in、bounded-sampler conformance、v3 runtime conformance、receipt/v1 兼容 smoke，以及 N=128/256 独立进程 smoke 通过。该结论只支持 N≤256 下本轮所测 F_T/v2 功能路径，不关闭完整 party-view 证明或安全门。N>256 不准入；不创建 `secure` alias；正式 LAN/WAN、RTT 和性能矩阵未运行/未测量。
7. 非作者 `/root/s45_non_author_review` 对重试界、版本/失败语义、失败副作用、源码映射和证据解释完成只读复核，结论为 `PASS_WITH_SCOPE_REVIEWED`；确认采样界、Qmax/Cmax 与源码映射无误。审查者未运行 build/test、未修改文件。首轮结构化输出格式、manifest 缺失和一项源哈希的证据问题已修正并复核。执行者参与 S43/S44 审查并撰写/实现本轮，故执行者自审不算独立安全审查。真实 DRBG 桥、完整协议视图组合仍 OPEN。

## 状态

- `F_T_BOUNDED_VERSION_FUNCTIONALITY = PASS_WITH_SCOPE`
- `STRICT_PPT_BOUND = PASS_WITH_SCOPE`（仅采样循环与 RAND 请求数量在有限响应随机带模型下有源码上界；不声称 OpenSSL/文件系统调用有墙钟上界）
- `MATERIAL_FAILURE_CONTRACT = PASS_WITH_SCOPE`（无 party package/context/signature/READY/party lease；内部 burn record 持久写入；不保证内存缓冲区擦除）
- `PACKAGE_VERSION_BINDING = PASS_WITH_SCOPE`
- `JOINT_RNG_STREAM_BRIDGE = OPEN`
- `PARTY_VIEW_COMPOSITION = OPEN`
- `SECURE_ALIAS_READY = NOT_READY`
- `PERFORMANCE_READY = NOT_READY`

详见 S45 报告、gates、逐案结果与 source/build/evidence manifest。仅本地提交，不推送、不合并。
