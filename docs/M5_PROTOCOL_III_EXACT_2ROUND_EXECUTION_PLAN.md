M5 Protocol III 精确 2 轮执行计划

1. 当前状态

截至本计划建立时：

M1 统一 score、stable tie 和 Top-K mask 语义已经冻结；

M2 Protocol I 与公共正确性底座已经合并；

M3 Protocol III 模块化 3 轮工程基线已经合并到 main；

M3 已完成：

GRank；

masked-rank DPF routing；

share-preserving secure combine；

Dealer、Party 0、Party 1 独立进程 E2E；

n=1...256、K=1/K=n、重复值、边界值和非二次幂测试；

Dealer 离线退出、三轮在线因果关系和 FD 生命周期验证；

M3 正在等待角色 A 进行协议语义和实现复检；

M4 CipherGPT 原生基线由角色 A 负责。

M5 的目标不是继续修改 M3，而是在保留 M3 作为稳定对照组的前提下，建立论文条件下的 Protocol III 精确 2 轮实现。

路线顺序保持：

M3 模块化 3 轮
→ M4 CipherGPT 原生基线
→ M5 Protocol III 精确 2 轮
→ M6 AAV86 / Direct Top-K

当前允许先完成 M5 的论文审计、域设计和执行计划。M5 runtime 的正式合并仍服从 M4 → M5 的主线顺序。

2. M5 目标

M5 最终应交付：

agarwal_protocol_iii_exact_2round

目标链路为：

Dealer offline preprocessing
        ↓
Dealer EXIT
        ↓
Party 0 / Party 1 receive secret input shares
        ↓
Online Round 1
        ↓
Online Round 2
        ↓
secret-shared Top-K result

M5 必须同时满足：

论文要求的代数域成立；

非零 payload 条件成立；

逆元操作具有明确定义；

在线消息依赖严格只有两轮；

Dealer 在 online 阶段完全静默；

secure runtime 不重构 score、rank、indicator、payload 或 selected index；

与 M1 oracle 和 M3 三轮基线逐项一致；

paper core 与项目统一 mask adapter 分开计量；

不遗漏输出适配成本；

通过独立进程 E2E、泄露审计和轮数审计。

在以上条件全部通过前，只能使用：

Protocol III 2-round candidate

不得使用：

agarwal_protocol_iii_exact_2round
Theorem 4.2 reproduction
paper-exact Protocol III

3. 不修改和必须复用的基线

M5 不得直接重写或覆盖：

protocol_iii_grank.*
protocol_iii_dpf_routing.*
protocol_iii_secure_combine.*
masked_mul_adapter.*
protocol_i_cmpagg.*
protocol_i_ucmp.*
protocol_i_transport.*
protocol_i_party_package.*

M3 保留为独立、可运行的三轮对照组。

M5 可以复用：

M1 score、stable tie 和 mask 语义；

M1 oracle；

M2 party/session/fingerprint 绑定；

M2 framed transport 和通信计数；

M2 GRank/CmpAgg 底座；

已验证的 DPF key generation、evaluation 和 transport；

Dealer/Party 独立进程测试框架；

MetricsRecord 和统一实验输入。

但任何只适用于 Z_(2^b) 的 M2/M3 接口，都必须经过 M5 域兼容性审计，不能因为代码可调用就视为数学上可复用。

4. M5.0：基线冻结与复检问题收集

4.1 工作内容

记录 M5 开始时的 main commit；

记录 M3 合并 PR、测试矩阵和 Ubuntu 环境；

收集角色 A 对 M3 的复检意见；

将 M3 中发现的问题分类为：

正确性问题；

协议语义问题；

工程健壮性问题；

仅影响 M5 的域或压缩问题；

M3 的修复通过独立分支和 PR 完成，不在 M5 中偷偷修复；

确认 M3 仍能作为 M5 的差分基线。

4.2 退出条件

M3 复检结论有书面记录；

阻塞 M5 的问题已经解决；

M3 全量回归通过；

M5 不依赖未提交的本地改动；

M5 的输入、输出和指标口径与 M3 一致。

5. M5.1：论文条件与两轮消息审计

本阶段只研究和写决策文档，不实现 runtime。

5.1 需要从论文固定的内容

Protocol III 的参与方和 Dealer 模型；

输入共享形式；

rank、DPF point 和 payload 所处的代数结构；

payload 必须非零的具体原因；

哪一步需要乘法逆元；

零值是否属于合法输入；

在线两轮分别发送什么消息；

第二轮消息依赖第一轮的哪些公开值；

论文如何计算 online rounds；

offline preprocessing 是否依赖在线输入；

论文允许公开的 masked values；

输出是 selected payload、ranked payload 还是 Top-K mask；

Theorem 4.2 的全部前置条件；

论文 core 与项目 mask 输出之间的差异。

5.2 建立消息依赖图

必须写出类似以下结构的消息 DAG：

offline material
      |
      v
local pre-round computation
      |
      v
Round 1 messages
      |
      v
Round 1 opened masked values
      |
      v
Round 2 messages
      |
      v
local output shares

对每条消息记录：

sender；

receiver；

输入依赖；

是否依赖前一轮公开值；

是否由 Dealer 提供；

是否属于 online communication；

是否泄露额外信息；

实际序列化字节数。

不得通过并行线程、提前发送、socket buffering 或把消息移到 Dealer 阶段来虚假减少轮数。

5.3 交付物

新增：

docs/decisions/M5_PROTOCOL_III_PAPER_2ROUND_AUDIT.md

5.4 退出条件

只有明确写出论文两轮公式、域条件、公开值和消息依赖后，才能进入域实现。

6. M5.2：域、编码和非零 payload 设计门

6.1 核心限制

M3 当前部分计算位于：

Z_(2^b)

但 Z_(2^b) 在 b > 1 时不是 field，并非所有非零元素都有乘法逆元。

因此 M5 不允许：

将 ring word 直接称为 field element；

对任意非零 uint64_t 直接求逆；

默默跳过不可逆元素；

遇到零值时降级到明文；

用重采样掩盖输入语义错误；

混用 field addition 和 ring addition。

6.2 必须确定的设计

目标 field：

field 类型；

modulus 或不可约多项式；

field element 表示；

canonical encoding；

score、rank、DPF input 和 payload 如何映射到 field；

field element 如何序列化；

非零 payload 如何生成；

原始 payload 为零时如何编码；

encoded-zero 与真实零值如何区分；

逆元失败属于参数错误、材料错误还是协议错误；

field 与现有 DPF payload group 是否兼容；

如果需要新的 DPF payload adapter，其接口和证明义务是什么；

从 paper-core 输出转换为统一 Top-K mask 的成本和泄露；

field/ring 转换是否引入额外通信轮次；

每个转换是否保持 additive sharing。

6.3 候选方案评估表

至少比较：

候选

是否为 field

非零逆元

与现有 DPF 兼容

序列化成本

转换成本

结论

Z_(2^b)

否

不保证

当前兼容

低

低

不得直接用于论文精确实现

Prime field

是

保证

待验证

待测

待测

候选

Binary extension field

是

保证

待验证

待测

待测

候选

不得在审计前预先选定最终方案。

6.4 交付物

新增：

docs/decisions/M5_PROTOCOL_III_FIELD_AND_PAYLOAD.md

6.5 退出条件

域确实满足论文代数条件；

非零 payload 和零值语义完整；

逆元失败路径可测试；

field/ring 转换成本已经列出；

角色 A 完成设计评审；

没有依赖未证明的 C++ 整数溢出行为。

7. M5.3：两轮 API 与离线材料设计

本阶段先冻结接口，再编写实现。

7.1 建议新增文件

最终文件名应在设计评审后确认，建议：

VFSS/include/moe_topk/protocol_iii_exact_2round.h
VFSS/src/moe_topk/protocol_iii_exact_2round.cpp
VFSS/tests/moe_topk/protocol_iii_exact_2round_test.cpp

如果 field 或 DPF 需要独立适配器，再增加：

VFSS/include/moe_topk/protocol_iii_field_adapter.h
VFSS/src/moe_topk/protocol_iii_field_adapter.cpp
VFSS/tests/moe_topk/protocol_iii_field_adapter_test.cpp

7.2 API 必须包含

ProtocolIIIExact2RoundConfig
ProtocolIIIExact2RoundPartyMaterial
ProtocolIIIExact2RoundMetrics
ProtocolIIIExact2RoundOutput
protocol_iii_exact_2round_party(...)

配置至少绑定：

session；

fingerprint；

party；

logical_n；

padded_n；

K；

score/rank/field 位宽；

field 标识；

protocol label；

timeout；

seed provenance。

7.3 离线材料要求

input-independent；

与 party、session、fingerprint 和尺寸绑定；

move-only；

fresh；

one-shot；

材料数量必须精确；

不足、重复或多余材料均硬失败；

通过正式 Peer/Dealer transport；

不直接序列化对象内存布局；

反序列化后明确所有权；

Dealer 在 online 前退出；

online 阶段不存在 Dealer socket。

7.4 输出分层

必须区分：

paper-core output

与：

project unified Top-K XOR mask output

分别记录：

correctness；

rounds；

online bytes；

offline bytes；

adapter bytes；

adapter time；

adapter 是否增加在线轮次。

如果统一 mask adapter 增加额外通信轮次，则不得把总执行错误标记为完整 2 轮。

8. M5.4：Primitive conformance

正式 runtime 前先完成最小原语验证。

8.1 Field 测试

覆盖：

zero；

one；

maximum canonical element；

addition/subtraction；

multiplication；

inverse；

x * inverse(x) = 1；

zero inverse 必须失败；

非 canonical encoding 必须失败；

serialize/deserialize round trip；

两方 additive share reconstruction；

随机向量差分。

8.2 Payload 测试

覆盖：

原始零 payload；

原始非零 payload；

encoded-zero；

随机非零 mask；

解码恢复；

重复值；

非法编码；

逆元失败；

材料复用失败。

8.3 DPF/field 兼容性测试

覆盖：

hit point 重构为目标 payload；

non-hit point 重构为零；

边界 point；

n=1,3,5,127,128,129,256；

key 经 Peer/Dealer 传输后求值不变；

key 所有权和对齐正确；

不将 XOR share 当作 arithmetic share；

不在 secure runtime 中重构 DPF output。

8.4 退出条件

所有 primitive conformance 测试通过后，才允许接入两轮 runtime。

9. M5.5：两轮 runtime 实现

9.1 实现原则

保留 M3 三轮 runtime，不进行原地修改；

M5 使用独立 protocol label；

只复用经过域兼容审计的 M2/M3 组件；

每轮使用独立 framed phase/type；

消息必须绑定 session、fingerprint、party、n、K 和 field；

secure runtime 不调用 test-only reconstruct；

不足材料、非法 field element 或消息错误立即失败；

不允许 fallback 到 M3、明文或本地 oracle。

9.2 在线轮次

具体公式以 M5.1 审计结果为准，但实现必须证明：

Round 1 output
       ↓
Round 2 input
       ↓
final local output shares

两轮之后只能进行本地运算。如果统一 mask adapter 需要额外通信，必须单独报告，不得藏在 Round 2 或 preprocessing 中。

9.3 代码审计

使用静态搜索确认 secure runtime 不包含：

reconstruct
true_rank
plaintext_rank
selected_indices
oracle
.bin synchronization
fixed sleep

10. M5.6：独立进程 E2E

建立：

Dealer
Party 0
Party 1

三个独立子进程。

严格顺序：

Dealer generate/send preprocessing
        ↓
Dealer EXIT
        ↓
parent release online input shares
        ↓
R1
        ↓
R2
        ↓
secret output shares

父进程不得保留能够掩盖 peer HUP 的在线 socket 副本。

10.1 正确性矩阵

至少覆盖：

n=1,K=1；

重复 score；

全相等；

INT32_MIN；

INT32_MAX；

K=1；

K=n；

非二次幂；

n=127,128,129；

n=256,K=2；

n=256,K=8；

固定 seed；

随机 differential；

与 M1 oracle 一致；

与 M3 输出逐项一致。

10.2 错误测试

覆盖：

wrong session；

wrong fingerprint；

wrong party；

field mismatch；

非 canonical field element；

zero inverse；

zero/nonzero payload 编码错误；

truncated package；

trailing bytes；

材料数量不足；

one-shot material reuse；

socket fragmentation；

timeout；

peer 提前退出；

Dealer 在线连接残留。

10.3 输出要求

测试层可以重构最终结果，但 secure runtime 不得重构。

最终输出必须：

长度等于 logical_n；

每个重构元素为 0 或 1；

恰好包含 K 个 1；

保持原输入顺序；

遵循 M1 stable tie；

与 oracle 完全一致。

11. M5.7：轮数、泄露和指标审计

11.1 轮数审计

必须记录每条在线消息：

Round

Phase/type

Sender

Receiver

输入依赖

字节数













验收要求：

paper core 在线因果轮数为 2；

线程调度不用于伪造轮数；

Dealer 不参与 online；

preprocessing 不依赖在线公开状态；

mask adapter 的轮数单独列出；

total rounds 不低报。

11.2 泄露审计

允许公开值必须从论文和项目决策文档中逐项列出。

禁止公开：

score；

完整 rank；

DPF point；

field mask；

payload mask；

indicator；

selected index；

最终明文 mask；

非论文允许的中间值。

11.3 指标

至少记录：

protocol label；

git revision；

field；

n；

K；

seed；

party；

runtime/topology；

offline generation time；

offline material bytes；

R1 time/bytes；

R2 time/bytes；

paper-core total；

mask-adapter time/bytes/rounds；

complete total；

DCF/DPF/inverse/multiplication 调用数；

sent/received per party；

correctness；

failure reason。

不得用结构体大小、论文公式或旧日志代替真实传输计数。

12. M3 与 M5 对照

使用完全相同的：

score 输入；

K；

seed；

stable tie；

oracle；

机器环境；

编译类型；

网络配置；

重复次数；

输出格式。

生成对照表：

指标

M3 modular 3-round

M5 exact 2-round

correctness





paper conditions





online rounds

3

2

offline time





offline bytes





online time





online bytes





adapter time





adapter bytes





total time





total bytes





不能因为 M5 轮数减少就隐藏 field conversion、非零编码或统一 mask adapter 成本。

13. 角色分工

角色 B（你）：M5 主责

负责：

论文条件提取；

两轮消息 DAG；

field 和 payload 方案；

primitive conformance；

两轮 runtime；

独立进程 E2E；

M3/M5 differential；

Ubuntu 可复现记录；

指标和泄露审计。

角色 A（搭档）：评审

重点复检：

是否真的满足论文 field 条件；

非零 payload 和零值语义；

逆元失败处理；

两轮消息依赖；

Dealer 是否在线静默；

是否遗漏统一 mask adapter 成本；

是否错误复用 M2/M3 ring-only API；

是否存在重构或额外泄露；

是否有资格使用 exact/Theorem 标签。

14. 分支与提交顺序

当前只创建计划分支：

docs/m5-protocol-iii-2round-plan

计划通过后，建议依次建立：

m5-paper-2round-audit
m5-field-domain-gate
m5-protocol-iii-2round-primitives
m5-protocol-iii-2round-runtime
m5-protocol-iii-2round-e2e

推荐合并顺序：

M5 执行计划
→ 论文与消息审计
→ field/payload 决策
→ primitive conformance
→ runtime
→ independent-process E2E
→ leakage/round/metrics audit
→ M5 closeout

每个 PR 只承担一个可审查目标，不在同一 PR 中同时修改 M3、CipherGPT 或 M6。

15. 明确禁止

M5 不允许：

删除 M3 的 R3 后直接声称论文精确两轮；

把 Z_(2^b) 当作 field；

对不可逆元素静默求逆；

通过 Dealer 在线参与减少 party 轮数；

把 online-dependent material 记为 offline；

使用明文 rank 或 selected index；

调用 test-only reconstruction；

修改 M1 冻结语义；

覆盖 M3 protocol label；

把 mask adapter 成本排除在总结果之外；

修改 VFSS-baseline/；

提交论文、密钥、日志、构建目录或本地参考工程。

16. 最终退出条件

只有全部满足以下条件，M5 才算完成：

M3 复检阻塞项已经解决；

论文 Theorem 4.2 条件有逐项证据；

目标代数结构确实为 field；

非零 payload、零值和逆元语义完整；

primitive conformance 全部通过；

Dealer 在线静默；

两轮消息因果关系审计通过；

独立进程 E2E 通过；

大规模测试矩阵通过；

M1 oracle differential 通过；

M3/M5 differential 通过；

secure runtime 无中间值重构；

paper core 和 mask adapter 分开计量；

total cost 没有漏项；

角色 A 完成评审；

Ubuntu 可复现记录完成；

工作区和 PR 差异干净。

完成前使用：

Protocol III 2-round candidate

完成后才允许使用：

agarwal_protocol_iii_exact_2round
