# BMW16 S25：Protocol I 功能候选冻结决策

日期：2026-10-08

实现身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**

本地候选分支：`codex/m6b-i-bmw16-s25`

起点：`origin/main` / merge-base `c3926c68fd14f270faa8b55234311071947fa080`
实验实现来源：S22 runtime tree `9210c11a7a877f8fe3687e0509c1722440cf63fe`；S24 `efafcd69f9d73b4386e8147df7fb5395afc78720` 是后续文档审计提交，未改 runtime。

## 决策

将 S22 中该候选所需的完整 `VFSS/` 差异及其 TEST_ONLY common-tape harness 整理到从最新 `main` 建立的独立候选。该候选覆盖 raw-score shares 到原序 Top-K XOR mask shares 的可调用三进程功能路径，CMake 默认关闭，所有在线接口仍显式命名为 EXPERIMENTAL。它不是 BB90 原算法复现，不是 BMW16 Algorithm 7 逐字实现，不继承 BMW16 Theorem 8 的概率或比较常数；Select 的四个比较依赖层也不是完整安全协议四轮。

S25 的范围是工程接收和有界功能证据。S24 对实际压缩 DCF key 与 BGI15 随机变量的直接映射给出的分布断点仍然成立：它否定直接逐字段套用 BGI15 定理，不是完整 VFSS key 的攻击，也没有关闭该 key 的函数隐私。整池自适应视图及 forward/inverse shuffle 与输出 share 的联合模拟仍未证明。S25 不创建 secure alias，也不宣布安全验收或正式性能资格。

## 代码来源与整合边界

- 对比 `origin/main@c3926c68` 与 S22 runtime tree，只把 S22 的 29 个 `VFSS/` 文件差异导入本候选：实验 party/material/TLS/startup API、raw-score adapter、DCF/uCMP 接线、shuffle/transport 接口与定向 C++ 测试；未导入 S1–S24 历史报告、逐次大日志或其他 M6A 工件。
- 保持唯一 C++ Select 实现 `protocol_i_bmw16_experimental_raw_score_mask_party`；未重写 Select、DCF 或旧 Protocol I/III 协议。
- 为可复核的 S4/C++ 轨迹差分保留 S12 common-tape 脚本，并纳入哈希冻结的 S4 Python 参考源码副本。脚本和参考源码在 TEST_ONLY 路径，不被 C++ party 编译或调用。所有逐次 tapes、trace、key 与 bundle 均在仓库外。
- S25 唯一运行时测试改动是增加 `--small-ranks` 全 K 独立进程矩阵，并在 `--extended` 增加 n=128 的 K=1 与 K=n。此改动只扩大 TEST_ONLY coverage，不改变协议、ABI、泄露、材料格式或轮数。

## 接口与输出合同

| 角色 | 接口职责 | 数据边界 |
| --- | --- | --- |
| T | 接收公开 `n,K,session,fingerprint` 和预置角色材料配置；离线生成 fresh raw、shuffle、Select/membership 材料并分方交付，成功退出后不接收在线输入 | 不接收 score、rank、K-th threshold 或协议结果；不得在线补发 |
| P0/P1 | 以 raw signed Q20.12 加法份额调用 `protocol_i_bmw16_experimental_raw_score_mask_party`；经 raw adapter、同置换 forward shuffle、两份并行四层 Select、DCF-backed membership、inverse shuffle、最终状态协商 | 成功仅发布本方原序 XOR bit-mask share；party 路径不重构 score、original index、selected key 或完整 mask |
| TEST_ONLY 控制器 | 创建独立进程/UID fixture、预置测试凭据、冻结输入和随机 tape；成功后核对 shares 重构与冻结 oracle | 控制器不属于 P0/P1 view；其重构数据不得进入 party 逻辑 |

成功合同为长度 `n` 的二值 XOR share，重构 mask 与稳定 oracle 逐位一致且 weight 为 `K`。正常抽样 abort 只有在 peer agreement 成功时返回协议级 ABORT 并且不产生 mask；材料/通信错误为本地结构化 abort，无 ACK 时不称双方已一致；工程不变量错误与未预期异常不归入概率 abort。候选 transcript 的已批准可见字段及实际消息映射沿用 S22/S24 handoff；“允许公开”不等于“已证明安全”。

## 默认关闭和启动范围

- `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER` 默认 `OFF`。关闭时 CMake 不注册 BMW16 party node/实验测试 targets，guarded headers 不可包含。
- `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS` 默认 `OFF`，且要求 `BUILD_TESTING=ON`；正常 Release party-node 不编译测试故障注入接口。
- opt-in 需要 OpenSSL 3；在线 mTLS 数据必须经注册的 TLS stream 收发，预期 TLS stream 缺失或不匹配时 fail closed，不回落到裸 FD。
- T 到 parties 的本地测试发材使用 TEST_ONLY 控制器/临时凭据。仓库不声称已提供生产凭据注册、跨主机服务管理或管理员/存储快照回滚防护。

## 证据和门禁

最终候选 revision、源码/二进制哈希、命令、运行矩阵和证据索引写入同日期 S25 接收报告。报告必须分别判定实验功能、默认关闭与基线隔离、材料和传输合同、源码到二进制 provenance、DCF 单 key 隐私、整池/shuffle view、安全 alias 和正式性能资格。S25 的功能测试不能关闭密码学或 V4 门。
