# M6B-I PROJECT_DERIVED_SELECT_5ITER S55 决策

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **固定接收对象：`PASS_WITH_SCOPE`。**在隔离 worktree 从 S54 commit `4695d70a4bb9a7f2bbce5c6fc7e163cb3f93d978` 开始；tree 为 `8acd16e2ee891aed3be8f0021d8d1aaafd57ca1c`，父提交为 S53 `dde7137c8fd185d5725d88980688caacde16da5b`。`origin/main` 为 S54 manifest 记录的 `c3926c68fd14f270faa8b55234311071947fa080`，是固定提交的祖先。S54 manifest、四个交付文件的 Git blob 与六个已固定 runtime blob 核验一致；本轮扩展核查的随机调用源也直接绑定 S54 Git blob。
- **FTv3 源码级随机账本：`PASS_WITH_SCOPE`。**固定 FTv3 使用全局 `RAND_bytes` 包装器，单次请求最多 1 MiB，错误状态立即变为发材异常；先请求 `G` 个 triple 字节，再请求 `ceil(m²/8)` 个置换 share 字节，之后每个 Fisher–Yates 尝试单独请求 4 字节、每位置最多 7 次。重新计算：N≤256 安全证明域的最大单次发材为 `Qmax=5,296,996` 字节、`Cmax=1,792` 个 RAND API 调用；N≤1000 已测功能域的单次发材为 `Qmax=88,786,404` 字节、`Cmax=7,247` 个调用。两者都不是进程生命周期或部署全局上界。
- **理想 IID FTv3 及 S54 条件界：`PASS_WITH_SCOPE` / `CONDITIONAL`。**S54 的 IID 语义材料律和状态感知适应性随机流替换形式可在原有范围沿用；它要求优势覆盖请求边界/长度、返回字节、成功与错误状态及发行输出，且对真实隐藏初始播种和 reseed 熵按实际分布平均。若 `ε_T=Adv_DRBG_T(Q_total_API,C_total_API;E_T,P)`，对 `S_RNG` 的理想成功概率 `p_I≥1−δ`，则在该联合游戏中 `p_R≥p_I−ε_T`，条件视图优势至多 `2ε_T/(p_I−ε_T)≤2ε_T/(1−δ−ε_T)`，要求 `ε_T<1−δ`。`Q_total_API,C_total_API` 是 API 层单位；provider 内部计数须按 `E_T` 的 DRBG 模型另行映射，不能直接与其相加。这只是明确 DRBG 假设下的条件定理。
- **目标 T 环境实例化：`OPEN / NOT_AVAILABLE`。**固定源码库没有目标 Dealer 服务包、容器/服务清单、实际 libcrypto/provider、OSSL_LIB_CTX 配置、熵源/reseed、进程生命周期或部署方声明。S48 的 OpenSSL 3.0.13 是历史 WSL 构建主机信息，S45 的历史构建信息同样不是目标部署证据；本机默认值不代替 `E_T`。因此本轮不能给出数值安全位数或实现级 DRBG PASS。
- **部署聚合 Q/C：`OPEN`。**上述数值仅为单次 FTv3 invocation。API 层 `Q_total_API,C_total_API` 还取决于 T 进程内并发/重复发材和共享 RNG 的其他 API 调用；provider 内部 DRBG generate/reseed/entropy 事件应另记为 `Q_internal,C_internal`，不能直接与 API 字节/调用数相加。仓内可见其他 `RAND_bytes` 调用点，但没有目标 T 的可执行文件、调用图或进程配置，无法确定哪些共用同一随机上下文或给出总上界。
- **成功条件化：`CONDITIONAL`（仅 S_RNG），完整发行仍 `OPEN`。**`S_RNG` 指本次发行所有 `RAND_bytes` 调用成功且 bounded sampler 未达到 cap；真实错误状态不可静默条件掉。完整 `S_issue` 还要求签名、序列化和三个文件均成功发布。若 `Adv_DRBG` 联合实验显式包含 `S_issue` 及其可见结果，且有证据给出 `Pr_I[S_issue]≥p_min>ε_T`，才可应用 `2ε_T/(p_min−ε_T)` 型条件界；当前没有这样的 p_min，也没有把签名器和文件发布故障纳入目标发行游戏的证据。
- **一次性与发布边界：`PASS_WITH_SCOPE` / `OPEN`。**源码在材料采样前、签名 key 读取与参数校验后，以 `O_EXCL` 在 T 私有 0700 registry 持久创建绑定 session/invocation/profile 的 burn marker；它只约束同一未回滚 registry 路径和 FTv3 命名域。不同 registry、不同版本域、备份回滚和跨主机恢复不受该局部保证覆盖。package0、package1、context 依次创建，不构成跨文件原子事务；某次写入失败时当前文件及此前文件可能残留。`fsync` 源码调用不单独证明特定文件系统/设备/VM 的掉电耐久。
- **独立复核：`PASS_WITH_SCOPE`。**执行者参与 S45–S54 相关实现审查/推导，不能自称独立审查人。未参与 S45–S55 实现/推导的 reviewer 独立只读复核固定源码、请求/调用账本、部署证据缺口、状态感知条件界及 marker/发布边界，确认与证据一致，并建议显式区分 API 与 provider 内部 Q/C 口径；本轮已修订该记号。reviewer 未构建、未测试、未改文件。该复核不实例化目标部署 DRBG，也不属于功能接收。

## 保持开放的门

`DRBG_DEPLOYMENT_PROFILE`、实际 `JOINT_RNG_STREAM_BRIDGE`、聚合 query 上界、完整 `S_issue` 条件化、目标签名/provider、文件系统发布/回滚边界、A token RNG、TLS/mTLS、收件人加密/哈希绑定、完整 party-view、以及真实有限 timeout 成功视图继续 `OPEN`。低概率合法输入 `ISSUANCE_ABORT` 接受状态保持 `OPEN_CONFIRMATION`。N>256 安全扩域未准入；S48 的 N≤1000 功能案例只作为有限功能收据；正式 LAN/WAN、RTT、AAV86 和性能矩阵未运行/未测量。

本轮只更新文档，没有 runtime/test 修改；BUILD/TEST=`NOT_RUN`。未创建 `secure` alias，未推送、未合并；`VFSS-baseline/` 未修改。
