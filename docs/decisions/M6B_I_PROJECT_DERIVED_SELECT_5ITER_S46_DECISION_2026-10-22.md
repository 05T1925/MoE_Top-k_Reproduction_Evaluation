# M6B-I-PROJECT-SELECT-5ITER S46 决策

日期：2026-10-22
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决定与结论

1. 被测对象固定为 S45 本地提交 `6fd6bed0718be28d677892ca40125d0f52323f0e`，tree `a77ba90ccce3ac18c7a735f6617e47ab8f0aaa2f`，父提交 `aaa7f96095c350e8236246163d48c54a7fb8d9ba`。S45 manifest 中七个本轮源增量均由该 Git tree 逐项核对；S46 只增加一个隔离的 TEST_ONLY 目标 K 进程案例入口，没有改 Dealer/runtime 逻辑。`VFSS-baseline/` 未改。
2. 非作者 `/root/s45_non_author_review` 未参与 S45 实现或证明撰写，在独立临时构建目录 fresh Release 构建并执行 S46 定向接收。默认关闭隔离、显式 FTv2 构建、sampler/runtime conformance、N=128/256 十个多进程目标 K 案例、发材中止案例和 F_T/v1 + receipt/v1 smoke 均按报告列出的范围通过。结论为 `F_TV2_INDEPENDENT_FUNCTIONAL_RECEIPT=PASS_WITH_SCOPE`，不扩展为完整矩阵或安全结论。
3. S45 的 Q/C 上界从固定源码重算并确认：N=128 为 1,294,376 请求字节、最多 765 次 `RAND_bytes`；N=256 为 5,295,976 字节、最多 1,537 次；单次请求不超过 1 MiB。上界是单次 F_T/v2 发材路径的源码级请求上界，假设每次 API 调用均返回；它不是墙钟上界，也不覆盖 Dealer 进程中其他调用或并发发材的聚合流。
4. 本机 WSL 测试构建使用 OpenSSL 3.0.13，CLI 观察到 active default provider；这只是本轮构建主机信息。目标部署的实际 libcrypto、provider/`OSSL_LIB_CTX`、配置、熵源、DRBG/reseed、并发/fork/重启和与其他调用的共享状态均未固定。故 `DRBG_DEPLOYMENT_PROFILE=OPEN`、`JOINT_RNG_STREAM_BRIDGE=OPEN`。保留符号项 `Adv_DRBG_T(Qmax,Cmax;E)`，不提供数值安全位数，不以 OpenSSL 文档或随机功能测试代替联合流证明。
5. `delta_reject≤255·2^-144` 继续只适用于去除 F_T/sampler 版本、签名及编码元数据后的语义材料投影 `pi_mat`，在独立 IID 32-bit 字假设下表示有限抽样的发材中止差异；它不约束完整签名 transcript，也不吸收进 DRBG 优势。`STRICT_PPT_BOUND` 仅对 bounded sampler 的循环与随机请求数量为 `PASS_WITH_SCOPE`；实际 `RAND_bytes`、文件系统或 OS 等待时间没有最坏界。
6. 达到 6 次拒绝后的 `ISSUANCE_ABORT` 定向复核通过：T 私有 burn marker 存在；party package、context、签名、party lease/READY 不交付。该语义属于离线发材版本，不改变在线两方的 mask 输出合同。项目文档没有单独记录负责人对“合法输入下低概率预处理发材中止”作产品级接受的句子；这是唯一潜在合同确认项，若后续作为正式发行契约仍需负责人确认。本地接收不擅自改变在线输出、角色或泄露。
7. 曾误启动普通 `moe_topk_score_to_mask_v3_process_e2e_test` 并在 73 条旧 S40 风格结果后 Ctrl-C（退出码 1）；日志和二进制 hash 单列于 cases/manifests。该片段不属于 S46 FTv2 结果，不构成完成或重跑 S35/S40 全矩阵。随后只调用带 TEST_ONLY hook 的 FTv2 target，10/10 S46 新案例完成。无遗留协议进程。

## 门禁状态

- `S45_SOURCE_RECEIPT = PASS`
- `F_TV2_INDEPENDENT_FUNCTIONAL_RECEIPT = PASS_WITH_SCOPE`
- `DEFAULT_OFF_ISOLATION = PASS_WITH_SCOPE`
- `BOUNDED_REJECTION_FAILURE_CONTRACT = PASS_WITH_SCOPE`（发材实现语义已测；产品级接受需按上文确认）
- `FINITE_Q_C_BOUND = PASS_WITH_SCOPE`
- `DRBG_DEPLOYMENT_PROFILE = OPEN`
- `JOINT_RNG_STREAM_BRIDGE = OPEN`
- `PRIVATE_CHANNEL_BRIDGE = OPEN`
- `PARTY_VIEW_COMPOSITION = OPEN`
- `SECURE_ALIAS_READY = NOT_READY`
- `TARGET_SCALE_READY = PASS_WITH_SCOPE`（仅 N=128/256 本轮指定 K 集；N>256 `NOT_ADMITTED`）
- `PERFORMANCE_READY = NOT_READY`（正式 LAN/WAN、RTT 与性能矩阵未运行/未测量）

本轮止于缺少目标部署 DRBG profile 的精确环境证据，不继续把条件 DRBG 假设提升成项目配置下的安全证明。下一项可执行任务可转向 N=1000 功能、内存与材料资源准入，但不得称为正式性能结果。

详细依据、构建哈希、逐案结果和待补环境信息见 [S46 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S46_REPORT_2026-10-22.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S46_GATES_2026-10-22.json)、[cases](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S46_CASES_2026-10-22.json) 和 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S46_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-22.json)。仅本地提交，不推送、不合并。
