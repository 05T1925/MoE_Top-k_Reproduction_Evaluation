# M6B-I-PROJECT-SELECT-5ITER-S15 决策记录

日期：2026-10-15  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决定

1. 接收对象固定为 S14 manifest 列出的 68 个文件；逐项 SHA-256 检查通过。S14 尚未合入 main。S14 manifest 声称的 canonical digest 与按声明排序/编码方式重算的结果不同；该问题只在 S15 erratum 中记录，不改写 S14 原始文件。
2. S14 接收者未参与 S14 DCF 实现或证明撰写，独立复跑通过 S14 feature-ON DCF/Select5 功能矩阵；但其发现 feature-OFF 静态库仍能直接暴露并调用 BCG DCF API。S14 整体接收记为 `CONDITIONAL`，不能声称默认关闭门已通过。
3. S15 只修复构建/API opt-in 边界：将 BCG DCF 源、API 编译宏和 uCMP BCG 成员/构造门控绑定到显式 experimental CMake option。旧 `M2UC/v1` 行为保持。修复版本是新接收对象，须由未参与 S15 的接收者另行复核。
4. S15 fresh ON/OFF、CTest、直接目标、13 个成功 E2E、6 个 abort/no-mask 和 OFF API 可见性检查通过。仅宣称限定参数域内的功能/接口验证，不宣称密码学安全。
5. 接受针对真实 BCG DCF key 和单次 UCMP transcript 的条件模拟桥接：固定语义 key 的 Theorem 2 替换，优势 `δ_w ≤ w·ε_G + ε_RNG_joint`，`ε_G ≤ Adv_AES128^PRP(T,q=5)+10/2^128`。联合 `RAND_bytes/getrandom` 假设仍是外部占位假设，未实例化为数值优势，也不是统计独立。
6. N=128 key-only pool hybrid 为 `1,639,424·ε_G + ε_RNG_joint(T;M6B5/v3,N128)`；N=256 为 `6,719,104·ε_G + ε_RNG_joint(T;M6S2/v2,N256)`。仅覆盖 DCF/UCMP key/eval 投影、含未用 key 与批准 L 下的自适应地址；不覆盖 forward/inverse shuffle 或输出 share。
7. 完整 `SHUFFLE_OUTPUT_COMPOSITION` 与 `JOINT_PARTY_VIEW` 保持 OPEN。精确首缺项为将实际 party-local shuffle permutation/masked-record shares 与 membership/inverse/output/abort/restart 联合分布，模拟成仅输入腐化方 input share、理想 output share 和批准 L 的分布保持 hybrid。
8. 保持 `PROJECT_DERIVED/EXPERIMENTAL`、no-complete-mask 输出合同、N>256拒绝；不创建 `secure` alias、不称 BB90/BMW16、不替换六方案中的 BB90。
9. M6A V3 仍 OPEN；V4 与正式 LAN/WAN 为 NOT_ALLOWED/NOT_RUN，按既有 V3→V4 前置，不因本轮进展调整。

## 门禁

| 门 | 决策 |
| --- | --- |
| `S14_INDEPENDENT_RECEIPT` | CONDITIONAL（来源/ON功能通过；OFF API 暴露失败） |
| `DEFAULT_OFF_BOUNDARY`（S15修复版） | PASS（静态符号、头文件编译、target help、旧 M2 回归） |
| `N≤256_FUNCTIONAL_E2E`（S15修复版） | PASS（13 oracle、6 uniform abort/no-mask；资源边界拒绝） |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL |
| `UCMP_SINGLE_CALL_VIEW_SIMULATOR` | CONDITIONAL |
| `POOL_AND_ADAPTIVE_ACCESS` | CONDITIONAL（DCF/UCMP投影） |
| `SHUFFLE_OUTPUT_COMPOSITION` | OPEN |
| `JOINT_PARTY_VIEW` | OPEN |
| `SECURE_ALIAS_READY` | NO-GO |
| `N>256_ADMISSION` | NO-GO |
| `V3_V4_PERFORMANCE_STATUS` | V3 OPEN；V4/正式性能 NOT_ALLOWED/NOT_RUN |

完整证据、修复文件和后续复核入口见 [S15 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_REPORT_2026-10-15.md)、[source/build manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_SOURCE_MANIFEST_2026-10-15.json)。
