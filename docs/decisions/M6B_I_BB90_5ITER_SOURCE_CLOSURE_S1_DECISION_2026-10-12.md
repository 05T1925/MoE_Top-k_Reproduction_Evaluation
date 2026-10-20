# M6B-I-BB90-5ITER-SOURCE-CLOSURE-S1 决策记录

日期：2026-10-12
状态：**SOURCE_SPEC = NO-GO_FOR_IMPLEMENTATION；BB90_5ITER_TEST_ONLY_GATE = NO_GO**

## 决定

本次只确认来源身份、出版记录、MIT CCS ’24 自动存档附件内容及 BMW16 独立性。2024 论文印刷 p.3036 直接称有一个五迭代 BB90 中位数版本且比较数线性于 n；MIT ZIP 实际只包含 METS 与出版 PDF。BB90 1990 全文的出版方 PDF/ePDF 请求返回 HTTP 403。未找到可核验的五轮伪代码、采样/取整参数、比较图、更新/失败合同或概率证明。

因此不创建明文 Select，不运行 oracle differential，不声称算法规格或概率已闭合。实现准入为 NO-GO，直到完整逐轮来源和证明条件可追溯。

## 分开的状态

| 对象 | 状态 |
| --- | --- |
| S34 schema-v3 技术诊断 | docs/reproduction/BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md 位于非 main 提交 fbc4beed11f686f9e0b74ba89d23b7514e93ce24；不是异会话接收签收 |
| M6A V3 | OPEN |
| M6A V4 正式 LAN/WAN | NOT_ALLOWED / NOT_RUN |
| BMW16-derived Select + DCF | 独立 PROJECT_DERIVED / EXPERIMENTAL；条件安全结论不扩张 |
| BB90 1990 来源身份 | 已确认；正文算法未获全文核读 |
| 2024 五迭代 BB90 | 仅成本叙述已确认；算法版本关系及证明均缺失 |
| BB90 五迭代 TEST_ONLY | NO_GO |

用户 2026-10-12 明确将 BB90 来源追查和算法规格列为当前优先工作。本决定把来源工作作为 M6A 期间可并行的研究线；不改变 formal M6B secure/runtime 与 LAN/WAN 阶段仍须遵循 M6A V3 前置门的顺序。

## 下一步

开启 M6B-I-BB90-5ITER-SOURCE-CLOSURE-S2 前，先向学姐索取五迭代版本的准确来源、完整伪代码/公式、抽样和整数规则、成功概率模型/条件及与 BB90 四轮原文的关系。S2 收到材料后逐页核验。只有当实现者能唯一写出全部五轮、并复算正确性/概率/成本，才另行批准隔离 TEST_ONLY 规格；若目标只覆盖 median，TEST_ONLY 也必须明确限于 median。

BMW16 派生五层方案不能充当 BB90 替代品；现有 BMW16-derived 候选和它自己的证明、计量、实现身份保持独立。

详细证据和缺项见 [S1 来源闭合报告](../reproduction/M6B_I_BB90_5ITER_SOURCE_CLOSURE_S1_REPORT_2026-10-12.md)。
