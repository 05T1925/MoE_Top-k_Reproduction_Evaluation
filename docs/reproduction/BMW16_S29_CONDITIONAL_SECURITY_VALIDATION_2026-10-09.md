# BMW16 S29：条件安全入口验证报告

日期：2026-10-09
方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
报告源码基线：S29 功能/接口提交 `08e96d393da6f102bb047d230e8845da4509814d`
运行时被复用源码：S26 `2818bce20f30719eaccf0bc5df586cce4fe78c84`，运行时修复 `ded433d636a3df929380bdce0ff22f4ce6121453`

## 1. 结论摘要

S26 功能候选已经异会话接收；S28 是未参与 S27 证明撰写的独立接收者，复核了实际 M2UC v1 完整序列化 DCF party key、相关整池自适应 transcript 及 forward/inverse shuffle-output-share 条件模拟。S29 因此不重复抄写同一证明，也不把证明条件删掉：新增 `conditional_secure_v1` 版本化入口，明确限定其可使用的安全模型和假设，并将 TLS、规模上限和 TEST_ONLY 控制项约束写入代码。

最终 Release 源码未改变 Select、shuffle、DCF、材料或帧 ABI。新入口通过 TLS 强制配置后委托给唯一已接收的 S26/S28 实现；没有第二份 Select、明文 fallback、online Dealer 或测试随机带入口。此门是**条件性候选可用**，不是无条件安全、标准模型安全、BB90 复现、生产部署批准或正式性能验收。

## 2. 起点、来源和 Git 状态

工作分支为 `codex/m6b-i-bmw16-s29`，独立 worktree。创建 S29 修改前 HEAD 为 cherry-pick 后的 S28 文档提交 `be373f2d97fdb00ce98dbb351efc091d097b17f2`；S29 源码提交为 `08e96d393da6f102bb047d230e8845da4509814d`。原始证明对象身份保留为 S27 `8bbf7119470bbc37f6950a3b425255ed993eaa8d`、S28 独立复核 `27bee47de4356a87af6a1f6ff06be6c7e0df2c9b`；它们在 S29 本地历史中的 cherry-pick commit 分别为 `cecab4d34fba533c550bde05030b915485ce21c3` 和 `be373f2d97fdb00ce98dbb351efc091d097b17f2`。S28 没有改 DCF 或 party runtime。

核验时 `main = origin/main = merge-base = c3926c68fd14f270faa8b55234311071947fa080`。S26 源码分支远端 head 为 `2818bce...`。S26、S27、S28 的独立报告身份不与 S29 源码 revision 混用。桌面主工作区及其他 worktree 未修改；`VFSS-baseline/`、`Papers/`、参考工程、E20/E21 和 PR #28 未进入差异。

GitHub 页面最初核验到 PR #29 是 Open，源分支 `codex/m6b-i-bmw16-s26`、head `2818bce...`。S29 验证完成后，将该分支快进到 `096ee6aa790a5d7244c3ae5ec1a4d657354d129a`，故该 PR 现在包含 S26 独立功能接收、S27/S28 安全记录和 S29 条件入口。PR #29 仍是 Open 而非 Draft，且未合入 main。更新 PR 元数据时 GitHub connector 返回 `USER_NOT_LOGGED_IN`，所以不能改标题/正文或转换 Draft。可查看 [PR #29](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/29) 和 [compare link](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/compare/main...codex/m6b-i-bmw16-s26)。PR #28 是 AAV86 项，S29 未改变它。

## 3. S28 条件性证明接收与 S29 项目假设

| 安全部分 | S28 独立复核结论 | S29 使用的明确条件 |
|---|---|---|
| 完整 M2UC v1 单 key 分布 | CONDITIONAL | source-specific G126 理想扩展归纳；实际需要 126-bit 受限 AES key 子族 PRG/PRP 假设、cryptoTools AES-CTR 根流假设及 OS CSPRNG。BGI15 Theorem 6 不直接覆盖此 ABI；S24 否定的 common-correction 字段直接映射仍不成立，但不是完整 key 攻击。 |
| 相关 DCF 全池及自适应打开 transcript | CONDITIONAL | 独立 KeyGen 随机性；G126 auxiliary-input 安全范围容纳其它相关阈值/key、已生成 prefix 和此前允许公开 transcript；完整腐化方 key pool 包括未用槽；挑战 key 的本地 Eval share 是 key/operand 后处理，公开 bit 后 peer share 由 `bit − local_share mod 2^64` 生成；后续边由已模拟的泄露 transcript 决定。 |
| 实际 forward/inverse shuffle 与 output-share | CONDITIONAL | T 可信、离线静默且不与在线方合谋；单方半诚实腐化；shuffle 本地随机置换因子/mask 的独立性；允许泄露函数 L 完整；模拟器取得理想功能给出的本方 output share。 |
| sampler 抽样失败界 | CONDITIONAL | 理想均匀无放回分析加 SHA-256 counter sampler 的随机预言机模型（ROM），并至少有一个诚实方先于选定贡献的 OS 随机币。SHA-256 counter 在标准模型下没有由 S28 证明。 |

角色为可信且在线静默的 T 与 P0/P1 中至多一个半诚实腐化，T 不与在线方合谋。抽象安全证明使用理想私有认证通道；本机 mTLS 和分 UID 仅是工程功能证据，不代表生产 PKI/远程部署接收。排除恶意方、双方合谋、T 合谋、管理员/快照回滚和系统级故障。TLS/材料/OS 工程错误由工程中止合同处理，不纳入正常协议的密码学优势界。

候选允许泄露 L 包含：公开配置和会话阶段；raw 输入转换所需的 masked carry/sign operands；forward/inverse `public_z`/masked values；匿名端点与打开比较 bit；pivot、匿名 U/V/W/reject/accept 位置；两份 Select 共享句柄的公开关联；匿名 selected handle；公开 slot/key ID、用途、访问次序/次数、比较数、frame 长度与通信相位；sampler 双方贡献与 XOR seed；统一算法 ABORT（实际比较轨迹仍可能让双方推测失败阶段）；各方仅取得本方输出 share。L 不含 raw score、original_index、原序号到匿名 handle 映射、明文 rank/阈值、完整明文 mask 或对方 output share。测试控制器收集的 trace 不属于在线方 view。

S28 的优势表达仍是符号界，不给虚构的安全位数：

```text
Adv_pool <= Adv_root((q_t)_t)
          + N_DCF * b * Adv_G126_aux
          + binom(2 * N_DCF * b, 2) * 2^-126
N_DCF = 2p + 9*C(n,2)
S_party = 2p + 2 + 9*C(n,2)
```

`N_DCF` 只数 DCF KeyGen，不包括每方的两个 shuffle slot；`S_party` 是全部材料槽。每个 uCMP slot 内的两次 Eval 使用同一 DCF key，不是两次 KeyGen。对 n≤256 的 S28 布局，T 串行根流的 `q_0=2N_DCF` blocks、其它线程 q 为 0 是源码调用布局推导，非硬件 PRG 计数。n≤256 的理想无放回抽样最坏 abort 上界为 `1.3105804606611235e-11`，只在上述理想/ROM/诚实熵条件下适用；零次自然 abort 仅是运行观察。超出 n≤256 不由此入口支持。

## 4. S29 接口与代码变化

`MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1` 默认 `OFF`，必须与既有 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON` 一起显式启用。头文件暴露：

```text
protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(...)
```

函数拒绝 `require_authenticated_transport=false`、`n=0`、`n>256` 和 TEST_ONLY fault controls；没有 TEST_ONLY random tape 参数。party-node 子命令 `party-conditional-secure-v1` 只在 opt-in 构建中注册，并经现有 12 路在线 mTLS TLS stream 设施创建认证通道；它不是裸 FD 的另一个入口。运行时拒绝未注册/已消费 TLS stream 的行为由已有 `ProtocolIFramedChannel` 的 fail-closed 约束执行。包/材料 ABI、会话、消息相位和原 Select 实现不变。

成功时底层返回原序 XOR mask share；仅隔离 TEST_ONLY harness 在 P0/P1 exit 之后组合 shares 并核对冻结 Top-K oracle。自然算法抽样失败仍走协议级双方 ABORT 且不创建 mask；材料/通信/不变量故障不归类为概率 abort。S29 没有改变 raw 输入、keygen、shuffle、DCF、membership 或 inverse 算法，也没有引入安全 alias 到默认接口。

## 5. 最终源码 revision 的复跑

环境：WSL2 Ubuntu 24.04，x86_64；GCC 13.3.0，CMake 3.28.3。测试证书由隔离测试 harness 临时生成并清理；进程使用独立本地 UIDs。配置中 Release、`BUILD_TESTING=ON`、实验 Select opt-in、conditional-v1 opt-in、TEST_ONLY failpoints OFF、DCF PRG counters OFF。

构建命令（源码 revision `08e96d393da6f102bb047d230e8845da4509814d`）：

```bash
cmake -S VFSS -B /tmp/moe_bmw16_s29_v1_release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF \
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s29_v1_release --target \
  moe_topk_bmw16_experimental_party_node \
  moe_topk_bmw16_experimental_party_node_e2e_test \
  moe_topk_bmw16_experimental_party_test \
  moe_topk_bmw16_conditional_secure_v1_conformance_test \
  moe_topk_m1_dcf_conformance_test moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test \
  moe_topk_m2_score_input_conformance_test moe_topk_m2_transport_conformance_test \
  moe_topk_m3_protocol_iii_three_process_e2e_test --parallel 2
```

验证顺序及结果：

1. **Conformance/定向回归**：在上述 Release build 上运行 CTest regex `moe_topk_(bmw16_experimental_party_test|bmw16_conditional_secure_v1_conformance_test|m1_dcf_conformance_test|m2_ucmp_conformance_test|m2_parallel_shuffle_conformance_test|m2_score_input_conformance_test|m2_transport_conformance_test|m3_protocol_iii_three_process_e2e_test)`：8/8 PASS。conditional API 专项验证 caller-owned FD 拒绝、n>256 拒绝、TEST_ONLY fault injection 拒绝。DCF/uCMP/shuffle/raw-score/transport 和 Protocol III 三进程回归通过。
2. **冻结 S4/C++ common-tape differential**：`py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s29_v1_release/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out C:\Users\28641\.codex\artifacts\BMW16_S29_20261009\common_tape_final`，39 cases PASS；matrix SHA-256 `ea80404b13c8344324bda38c9bcff86c4d1628785954c91e3350651112548460`。该 differential 验证共用随机 tape 的 Select 轨迹；不代替密码学证明。
3. **conditional-v1 三进程 E2E**：`wsl.exe -d Ubuntu-24.04 -u root -- /tmp/moe_bmw16_s29_v1_release/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s29_v1_release/moe_topk_bmw16_experimental_party_node --conditional-secure-v1`，fresh TLS/material，n=8、K=4，T/P0/P1 exit 0，oracle 核对 SUCCESS；不同 UID 的本地文件隔离 PASS；同 bundle 重领被双方拒绝、无 mask。此新入口本次直接 E2E 上限是 n=8。复用的唯一核心 runtime 由 S26 独立接收并经 S28 在 n≤256 的边界/中间 K 上复核；S29 没有把该历史规模写成本窗口新跑数据。

默认关闭检查：以所有 BMW16 选项默认 OFF 配置 CMake，目标列表没有 BMW16/conditional 目标，`node` 目标不存在；启用 conditional 选项但关闭实验 Select 的 CMake 配置被预期的依赖检查拒绝。Release party-node 的 CMake 参数为 `FAILPOINTS=OFF`，二进制 `strings` 检查未发现 `MOE_BMW16_TEST_FAILPOINT`。正式全仓 CTest 未运行（只构建并执行上列有针对性的目标）；n=1000/K=80 最终候选 E2E 未运行且由本 API 的 n≤256 contract 拒绝；正式 LAN/WAN 1+5 与九指标未测。

另有一次试探将默认 OFF 构建的不存在目标 `node` 作为 build target，CMake 因该目标未注册而返回 `No rule to make target 'node'`；这与预期隔离一致。Opt-in 目标正确名称是 `moe_topk_bmw16_experimental_party_node`。此尝试没有修改源码或测试状态。

## 6. 源码、二进制和原始证据身份

源码 commit：`08e96d393da6f102bb047d230e8845da4509814d`。最终 Release 二进制 SHA-256：

| 文件 | SHA-256 |
|---|---|
| `moe_topk_bmw16_experimental_party_node` | `957284f939041719e01daedaffe249664faeb3ad0bb37602a1792d7a4206b782` |
| `moe_topk_bmw16_experimental_party_node_e2e_test` | `72dc47e48dd473df5cea10dc170f0e39af1b20a8447ee5d682a64a664841d9ae` |
| `moe_topk_bmw16_experimental_party_test` | `a3362b06f71ecfc4dff986c7fd2583072b7ceb4d0231f752aa87cf81645002a6` |
| `moe_topk_bmw16_conditional_secure_v1_conformance_test` | `3d6fb910fe9d90de848a5933393c5a7c59b29d7050f8d0de294f2dca3c7519ca` |

源文件关键哈希：

| 文件 | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `241655f8b6f13c19ba4bc97bec2da26c63b6057f91d4605d72646b219b7d23a5` |
| `VFSS/include/moe_topk/protocol_i_bmw16_conditional_secure_v1.h` | `1eaef97febad9ad3e1c69ac7e1ffd72968a5e5eed6a8e9364314bdd8564188a7` |
| `VFSS/src/moe_topk/protocol_i_bmw16_conditional_secure_v1.cpp` | `38755e153264508664665aef8d35dc6ef252fc338a8f49f9c131ba0f6c275ab9` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `467afa40677e052811701104f6dc017d38c18467143c2407dfc5d5f1cc155830` |
| `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v1_conformance_test.cpp` | `65daf94710cf0ebb7ec3ace7c85cff53034e14877ed59d399fd6d083d689b92e` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `0b197db48d281bfa1a29fb021553f74bee3b03b993f037cee3b3b98cebdbe21c` |

日志和矩阵留在仓库外 `C:\Users\28641\.codex\artifacts\BMW16_S29_20261009\`，包括 `final_source_release_build.log`、`final_source_targeted_ctest.log`、`final_source_conditional_secure_e2e.log`、`common_tape_final/matrix.json`。以上日志哈希依次为：`b717ebc03a895fe0689b985ef57c4e200d4b647813db4be2455b3d93c22d7673`、`3a109ffe6a9cc55e5bc73c46a1f5ef2bcf17d9df67b1356aac916a5fe0c8d702`、`3c744a577c52be161cfa5d0bedb78a447257193c29ccd5ebefa0d4fd4f287e0b`、`ea80404b13c8344324bda38c9bcff86c4d1628785954c91e3350651112548460`。未把私钥、key、sidecar、bundle、share、证书或原始大日志加入 Git。

## 7. 九指标和门禁

正式九指标：逻辑比较调用、实际 DCF Eval、PRG/AES 调用、离线材料有效载荷/包字节、T 离线耗时、party 在线耗时、逐方通信、因果消息轮数、总时间/资源峰值及自然 abort 分母，均为 **NOT_MEASURED / NOT_RUN**。E2E harness 展示的单次 loopback TLS 字节和耗时仅用于功能诊断，不进入性能对比。正式 V4 1+5 LAN/WAN 和六方案排名未启动。

| 门 | S29 结论 | 依据/边界 |
|---|---|---|
| `FUNCTIONAL_CODE_ACCEPTANCE` | PASS（S29 新入口 n=8；复用核心独立接收到 n≤256） | S26 独立功能接收 `32f7f0b...`、S28 同 runtime 复核、S29 common-tape 39/39、最终源码 conditional mTLS E2E。 |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL / S28 已独立接收 | 只在 G126/restricted-AES、AES-CTR root stream 和 OS CSPRNG 假设下；不直接套 BGI15 Thm 6。 |
| `ADAPTIVE_FULL_POOL_VIEW` | CONDITIONAL / S28 已独立接收 | 使用完整 L、全池和未用槽、同 key 双 Eval、自适应后续访问 hybrid；优势界保留符号项。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | CONDITIONAL / S28 已独立接收 | 可信离线 T、单方半诚实、fresh shuffle masks、理想私有通道和给定 output share 的条件模拟。 |
| `SAMPLER_ABORT_GUARANTEE` | CONDITIONAL / S28 已独立接收 | n≤256 理想界仅在均匀无放回 + ROM + honest OS entropy contribution 条件下；标准模型未证。 |
| `CONDITIONAL_SECURE_V1_ENTRY` | PASS（仅显式条件 profile） | 默认 OFF，必须启用已有实验实现，强制 TLS，限 n≤256，复用已接收唯一 runtime。不是默认 `secure` alias 或生产认证。 |
| `SECURE_ALIAS_READY` | CONDITIONAL，不创建无条件/默认 secure alias | 条件候选可调用；正式项目采纳条件限于本报告第 3 节 profile，仍需最终异会话接收及适用假设认可。 |
| `FORMAL_PERFORMANCE_STATUS` | NOT_RUN | M6A→M6B→M7 正式顺序未变，未运行 V4 LAN/WAN 或统一九指标。 |

后续接收只需针对 S29 增加的版本化入口、本文的假设边界和最终源码 provenance 作异会话审查；不能将审查对象扩大为“重新发现一般 DCF 安全证明”，也不能把条件性结论升级为无条件 PASS。若项目要求标准模型 SHA-256 sampler、标准 AES-128 直接定理或生产凭据/跨主机部署保证，需另立门禁与对应原语/部署合同。

### PR 元数据交接

S29 已将当前分支快进推到 PR #29 源 ref；元数据连接器 `github_update_pull_request` 返回 `USER_NOT_LOGGED_IN`，故没有更新 PR 标题/正文或转换 Draft。当前准确远端 head 为 `096ee6aa790a5d7244c3ae5ec1a4d657354d129a`。可直接打开 [PR #29](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/29) 或 [main...candidate 比较页](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/compare/main...codex/m6b-i-bmw16-s26)。

建议标题：`S29: Conditional-security Protocol I + BMW16-derived Select candidate`

建议正文：

```markdown
PROJECT_DERIVED / EXPERIMENTAL Protocol I + BMW16-derived Select + DCF. This is not a BB90 implementation or a line-for-line BMW16 Algorithm 7 implementation, and it does not inherit BMW16 Theorem 8 guarantees.

S29 adds a default-off, versioned conditional_secure_v1 API that requires the existing opt-in implementation and authenticated TLS streams, rejects TEST_ONLY fault controls, caps n at 256, and delegates to the sole independently received S26/S28 runtime. No second Select, material ABI, plaintext fallback, or online Dealer is added.

Validation on source 08e96d393da6f102bb047d230e8845da4509814d: 8/8 targeted CTests passed; 39/39 common-tape cases passed; fresh-material T/P0/P1 mTLS E2E through the new entry passed at n=8,K=4. The reused core was independently received through n<=256 by S26/S28. Final-source n=1000 was not run and lies outside the new API scope.

Security conclusions remain conditional on the reviewed G126/restricted 126-bit AES key-family PRG/PRP assumption, cryptoTools AES-CTR root-stream assumption, OS CSPRNG, trusted offline non-colluding T, one semi-honest corrupt online party, the documented leakage L, and ROM sampling with at least one honest OS-random contribution. No standard-model SHA-256 sampler guarantee, production credentials/deployment acceptance, or unconditional/default secure alias is claimed. Formal V4 LAN/WAN and unified nine-metric performance remain NOT_RUN.
```
