# S32：统一计量合同与研究诊断决策（2026-10-10）

## 决策

将 `BMW16_RUN_RECORD_SCHEMA_2.json` 及 `tests/bench/bmw16_run_record.py` 作为
Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED 路线的逐次研究诊断记录与审计器。
主指标口径遵循 `PROJECT.md` 与 `BENCHMARK_VALIDATION_PLAN.md`；S31 schema v1 的
`online_comm_per_party_bits` 对象表示和 TLS 线字节主计数口径不再用于新记录。S32 不改变
S31 已接收的 `conditional_secure_v2` 安全范围、材料 ABI、在线消息或协议输出，也不启动 V4。

此处的 `offline_material_total_bits` 采用“实际交给在线方持有的序列化预处理 payload”统一主口径，
并另存 sidecar/package 字节、TLS 应用交付字节与元数据。`online_comm_total_bits` 采用在线
`ProtocolIFramedChannel` 实际发送的应用帧字节总和；TLS record/handshake、TCP/IP 和重传另列且
当前为未测。`online_comm_per_party_bits` 是标量 `total/2`。仅发送计入 total，接收只核对守恒。

`comparison_edges_total` 按实际执行的无序安全比较边计一次：raw-score adapter 的 uCMP 调用
（含 padding 位，另分真实/padding）、每个 Select task/round 的唯一真实材料边、membership 安全边。
Select 逻辑请求、dummy 本地请求、重复请求、材料槽和 DCF Eval 都作为辅助计数，不混为边。
同一无序边在不同 task/round 再次安全求值时再次计数；同轮两方计算同一边只计一次。

`online_prg_calls_total` 表示协议在线路径实际执行的长度倍增 AES-DCF 扩展次数。S32 代码审计确认
在线 raw adapter、shuffle、Select 控制及 inverse 没有另一个长度倍增 PRG 调用点；Select 的随机抽样
SHA-256 counter words、DCF Eval 次数和 AES block 数分别单列。DCF Eval counter 位于实际 AES
扩展调用点并与两方 uCMP Eval 数量核对。TLS 内部随机数/密码套件工作不计为协议 PRG。离线 cryptoTools
根随机流调用次数与 T RSS 当前为 `NOT_MEASURED`，不填零。

诊断 `offline_time_ms` 从独立 receiver/T 启动前开始，到 T、双方 receiver 成功退出、双方材料
pair-ready 且 TEST_ONLY raw-share 文件创建完毕；包含测试 harness 的 raw-share fixture 生成。
诊断 `online_time_ms` 从上述 ready 屏障到两方 party 退出并发布 share，包含子进程启动、测试编排及
mTLS setup。二者不是纯 party API 时间；JSON auxiliary 同时保留可测的 party API critical path。
`total_time_ms` 来自同一 outer wall interval，并由 auditor 检查 `total=offline+online`（2 μs 容差）。

在线因果轮数由实际阶段 counter 和消息依赖 DAG 交叉核对：raw adapter 2、forward shuffle 2、共同
抽样币 1、Select R1–R4 4、inverse 2、final agreement 1，共 12 个应用阶段。TLS 握手并行 setup、
测试进程管道和 socket frame 数不是协议轮数。

## 取舍与可比性

- 主 schema 只存每次尝试；成功、算法 abort 和工程失败均保留。只有 SUCCESS 可有 mask 正确性字段，
  非成功必须 no-mask。缺测为 JSON `null` 并与 `NOT_MEASURED` provenance 一一对应。
- `raw_record_sha256` 是同目录外部证据 manifest 文件原始字节的 SHA-256，不是 JSON record 自身的哈希。
- 原始 shares、密钥、证书、sidecar、mask 与大日志留在仓库外；仓库提交只含工具/schema、摘要和报告。
- S32 只给 BMW16-derived 路线增加同定义采集器。Protocol I 全对全及 Protocol I+AAV86 尚无
  同 revision/同边界 schema-v2 记录；旧 E15/E17/E20 不能拼入。跨路线性能排序仍 NOT_READY。
- 正式 V4 仍等待 M6A/V3 门及服务器端 1 次预热+5 次 LAN/WAN 运行；S32 数字只作
  `EXPERIMENTAL_DIAGNOSTIC`，不得进入正式表。

## 冻结配置

实现提交：`f2db098578b1cc096298ee39d990ff668d8dd6f7`。实验构建为 Ubuntu 24.04 WSL2、
GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13、Release、OMP_NUM_THREADS=1；BMW16 adapter、
conditional v2 与 DCF counters ON，v1/failpoints OFF。默认 OFF 配置另行独立构建验证。

不改变：PROJECT_DERIVED 身份、S31 条件安全假设与 n≤1000 范围、许可泄露函数、协议阶段、V3→V4
顺序。loopback E2E 不是正式网络实验；本次不启动正式 LAN/WAN 矩阵。
