```markdown
# M5 Field Contract 实现前检查表

状态：**field-contract 分支检查表；不构成 M5 已实现证据**

基准分支：

```text
m5-protocol-iii-field-contract
基准 revision：
TO_BE_FILLED_AT_REVIEW
1. 分支范围
- 只新增 M5 决策和检查文档。
- 不修改 M1 冻结语义。
- 不修改 Protocol I 代码。
- 不修改 M3 三轮代码。
- 不修改 VFSS-baseline/。
- 不提交论文、密钥、日志或构建产物。
- 不声明 M5 runtime 已完成。
- 不启用最终 exact executable 标签。
2. 论文证据
- PDF SHA-256 与冻结值一致。
- Protocol III 身份映射到 Table 1 和 Theorem 4.2。
- 2+1 和单方静态半诚实模型已记录。
- CmpAgg ranking 和 DPF routing 已区分。
- modular 三轮与 compressed 两轮已区分。
- field 条件有明确出处。
- nonzero payload 条件有明确出处。
- inverse DPF payload s_i^-1 有明确出处。
- 会议版未指定的内容均标为项目决策。
- 未把本地参考代码行为反写为论文结论。
3. 两轮消息 DAG
- Dealer 在在线输入到达前退出。
- Round 1 同时包含 GRank 和 field multiplication 输入 opening。
- Round 1 结束后只有 rank shares 和 masked-product shares。
- Round 2 同时打开 masked ranks 和 multiplicatively masked payloads。
- Round 2 后只剩本地 DPF evaluation 和 aggregation。
- 没有隐藏的第三次在线发送。
- socket 数量和线程并行没有被误算为轮数。
- 每个公开值都有泄露说明。
- 每项材料都有生成、绑定和 one-shot 说明。
4. Field 决策
- field 固定为 GF(2^64)。
- 多项式固定为 x^64+x^4+x^3+x+1。
- reduction constant 固定为 0x1B。
- 实现前增加不可约性验证。
- addition/subtraction 固定为 XOR。
- multiplication 固定为 carry-less multiplication with reduction。
- inverse(0) 固定为硬错误。
- field wire encoding 固定为 8-byte big-endian。
- field 和 ring 使用不同 C++ 类型。
- field id/version/poly id 进入材料绑定。
5. Payload 决策
- 通用核心不限制 payload 为 1。
- payload 明文必须非零。
- s_i 从非零 field 元素采样。
- DPF beta 固定为 s_i^-1。
- encoded record 包含固定非零 tag。
- encoded record 包含冻结 priority key。
- priority key 保留 original index。
- 最大 record 位宽小于 64。
- ring-to-field 输入转换被列为独立 adapter。
- secure runtime 不通过重构检查 payload 非零。
6. DPF 兼容性
- 现有 evalDPF_Payload() 被记录为 ring-only。
- 不把 evalDPF_EQ() 输出直接当作 field payload share。
- 新 field DPF 复用现有 tree traversal。
- 不在 moe_topk 复制整份 DPF。
- field key 与 ring key 有显式类型或 group tag。
- leaf expansion 输出完整 64 位 field element。
- 新增 PRG/AES 成本进入指标。
- hit/non-hit、transport 和 ownership 测试已规划。
7. 输出身份
- 单 rank paper-core 输出定义为一个 selected record share。
- K-rank 扩展输出定义为 K 个 selected record shares。
- K-rank DPF Eval 数量按 n*K 记录。
- selected record 不在 secure runtime 中重构。
- mask adapter 被定义为独立阶段。
- mask adapter 成本计入端到端结果。
- 完成态 exact mask 标签仍被禁止。
8. 下一分支入口门
只有以下内容通过双方评审后，才能开始：
m5-protocol-iii-field-adapters
入口条件：
- 两份决策文档无未解决的论文身份冲突。
- field 和 payload 类型获得评审批准。
- 两轮消息 DAG 获得评审批准。
- M3 保留策略获得确认。
- M2 当前工作文件无重叠修改。
- git diff --check 无输出。
- PR 差异中没有 VFSS-baseline/。

这个分支不需要修改 `PROJECT.md`、`IMPLEMENTATION_PLAN.md` 或 `M3_ONWARD_TEAM_WORK_PLAN.md`：最新 `main` 已经包含 M5 路线和分支命名。这样 PR 差异最小，也不会与你搭档正在改的公共文档或 Protocol I 文件冲突。

- :codex-followup[审查三份文档]{prompt="审查这三份 M5 field-contract 文档的论文一致性、数学正确性和仓库命名，并指出合并前必须修改的问题。"}
- :codex-followup[准备有限域代码]{prompt="为下一分支 m5-protocol-iii-field-adapters 设计 C++ 头文件接口和完整 conformance 测试内容，但先不要修改文件。"}
- :codex-followup[设计 Field DPF]{prompt="基于当前 VFSS dpf.cpp 设计最小的 GF(2^64) output-group 扩展，列出需要新增或修改的文件及代码内容。"}
