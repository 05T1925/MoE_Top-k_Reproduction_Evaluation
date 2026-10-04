# M6A-P2-I-E20：Protocol I+AAV86 流式材料最小实现合同

日期：2026-10-04；此记录先于 E20 secure 代码修改。新标签拟为 `E20_STREAM_AEAD_V1`，旧 `E16_IN_MEMORY_ALL_PAIRS` 的入口、材料格式、n128/256 数据均保持独立。来源为 E19 `5caec909830daa881d42a4b0e9a7f3e097c7e88c`；本合同是**项目扩展设计**，不是论文定义或已运行结论。

## 角色、相位和边界

| 项 | E20 约束 |
|---|---|
| T | 可信、离线、与双方不合谋；只接收 session/fingerprint/n/K/r/material ID 和独立随机币，不接触在线输入、rank 或活跃图。先生成共享节点 mask、每轮每个 canonical 无序端点对的两方 uCMP key，逐条经各自私有完整 T→P 通道交付；全部 `r·C(D,2)` 条均完成后退出。 |
| P0/P1 | 各自收齐本方 score/permutation/node 材料及全池边 key；边 key 逐条 AEAD 加密写入各自受保护存储，在完成文件同步、完整条数/长度检查、ready 屏障前不得接收在线输入。P 的盘密钥由 OS 熵产生，只在该次 P 进程内存持有；不存入材料文件。 |
| 在线 | 首次输入依赖消息前，用已有持久 `O_EXCL` claim 原子领取整个 session/material ID/party 份额，领取后崩溃即消耗。按 `(t,min(a,c),max(a,c))` 计算 checked 固定偏移，仅解密/解码实际活跃边，不访问 T；不另发网络消息。读取及 AEAD 验证计入 secure 在线时钟；OS 可看到文件访问模式，活跃图本来已在当前项目泄露合同中公开给双方，额外 OS 观察须视作可信宿主条件。 |
| 输出 | signed Q20.12、score 降序/原下标升序稳定同分、原输入顺序 XOR Top-K bit-mask 和既有 `2r+4` 因果网络轮保持不变；secure 路径不重构比较位、rank、index 或 mask。 |

## 方案比较与选择

1. **T 逐边生成，经两条私有完整通道流式发给 P，P 各自封存。** T 只留两份 O(D) 的 score/permutation/node 基础材料与当前两份 key；包内存由 O(rD²) 降为 O(D)。P 的记录固定长度、随机定位；磁盘总量 O(rD²)，额外磁盘读只在在线活跃边发生。选择此方案，先用小 D 证明与旧路径材料代数及输出一致。
2. **T 直接生成两份磁盘文件后转交。** T 写文件时会短暂拥有双方密文和封装密钥，文件路径/所有权与私有交付的绑定更复杂；若只交明文文件则违背静态保护，若双方共享宿主目录则不能隔离读权限。本阶段不选。

## 记录、领取和失败关闭

每方独立的 0700 目录、0600 文件，以 `openat(O_NOFOLLOW|O_EXCL)` 建临时文件，完成全池写入、`fdatasync`、原子重命名及目录 `fsync` 后才发送 ready。文件首部包括格式版本、party、session、fingerprint、logical n、D、K、r、比较宽度、material ID、记录长、预期条数；每条记录重复 `(t,a,c,material ID+slot)`，从 canonical 序数与文件偏移对照。每条 DCF key 密文使用 OpenSSL AES-256-GCM、独立本方临时 256-bit 密钥及由唯一 slot 编码的 96-bit nonce；首部和记录标签作为 AAD，GCM tag 验证失败立即终止。这里的完整性来自 AEAD 与私有分发/未落盘的独立密钥，普通 checksum 只可用于原始证据文件，不称材料认证。两个 P 在同一测试主机同 UID 时，0700 目录本身不构成对另一同 UID 进程的 DAC 隔离；仍须使用独立 OS 身份/挂载空间或将实际隔离限于 AEAD 与可信宿主假设，报告必须如实区分。

文件截断、记录序号/端点/轮次/party/session 错误、同规格 blob 交换、tag 篡改、重复领取、T 失败、peer 关闭/静默及进程中断均 fail closed，不退回旧材料、不请求 T、不以空 key 替代。外层 AEAD/标签不能证明 uCMP key 与节点 mask 的代数关联；该关联仍依赖可信 T，并由 TEST_ONLY dealer 一致性检查补充。材料秘密离线输入无关，但算法随机性/同 key 重用仍必须一次性。
离线私有 socket 的读写分别设置 `SO_RCVTIMEO` / `SO_SNDTIMEO`，按配置的
`timeout_ms` 对静默端点或中途停发做有界关闭；超时只导致本次材料失败，
不发生重领或补发。

## 计量和放行

E15 离线从 T/P 启动前至 T 退出及双方 ready，包含全池 keygen、私有交付、P 加密写盘和同步；在线从双方收到输入后的 secure 入口至原序 mask，包含首次领取与活跃边文件读取/验证/解码。ready 时分别记录双方磁盘密文占用、内存材料有效载荷、临时文件与实际 RSS，不能把包长或准入预算代入实际峰值。每配置每次 fresh 全池；先 conformance→冻结 oracle differential→独立 T/P0/P1 E2E，之后才准同 revision、配对输入、LAN/WAN 1 预热+5 正式。基线 n1000 另设清楚的低内存交付标签并独立三层验证；无法同规模成功时不写配对胜负。任何安全/绑定/一次性关闭无法成立即 `NO-GO`，所有未跑指标 `NOT_MEASURED`。n≥10⁴ 仍为当前全两两资源受限。多 key 联合模拟未证明，`AUTHOR_EXACT=NOT_PROVEN`。
# 同规模全对全基线扩展（实现前补充）

为使 n=1000 的 EMP-ON Protocol I 基线拥有同样有界的材料驻留，新增独立
`E20_CLIQUE_SEALED_V1` 标签。T 使用同一个已审查的流式生成器、AES-GCM
分方存储和一次性领取机制，取 `r=1`，离线完整预发 `C(D,2)` 条 uCMP key。
基线只取其中与全对全 CmpAgg 相同的节点 mask、score carry/sign key 和逐边
uCMP key；独立的双方 shuffle 仍执行原有真实 EMP OT，不使用 T 所生成的
AAV86 permutation/shuffle 候选状态。T 为该存储生成的额外 O(D) 状态仍计入
基线 ready 材料和离线时间，不隐去成本。在线完整消费全对全边，序列化格式、
私有交付、OS 用户隔离、AEAD 校验、整份原子领取及故障关闭边界与上述 E20
存储合同一致；没有在线 T 或额外在线网络轮次。该基线新路径须单独通过
oracle 与独立进程验证，不能将既有 E15/E16 基线数值重新贴标签。
