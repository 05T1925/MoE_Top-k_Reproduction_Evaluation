# BMW16 S26：S25 实验候选的后续技术复核决策

日期：2026-10-09
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
接收基线：S25 文档 HEAD `e8e06c90ebb32cd975afcb582bb25b2962bdad19`
被复核运行时提交：`112cd8a6d641ded1be5244b317ffb5c42f3b86d1`

## 本轮裁决边界

本轮在 S25 精确候选上检查来源、代码和证据，并修复一个可复现的输出发布错误。当前执行聊天此前承接过 S25 工作，因此本轮结果是**后续技术复核**，不满足任务要求的“未参与 S25 的异聊天接收者”身份。报告不自称独立签收；候选 Draft PR 需要另一接收者按最终源码 revision 复核。

S25 候选仍是默认关闭的工程实验入口，不是 BB90 原算法复现，也不是 BMW16 Algorithm 7 的逐字复现。BMW16 Theorem 8 的概率界和比较常数不转移到本项目。四个 Select 比较依赖层不是完整协议在线轮数。

## 修正项

`write_mask_exclusive` 先 `fdatasync` 暂存文件，再用 `link` 以 no-clobber 方式发布，最后 `unlink(temp)`。若最终链接成功而临时文件删除失败，旧实现直接抛错，party 退出码为 70，但最终 `mask.share` 仍存在；发布后也没有对父目录执行 `fsync`。

TEST_ONLY 注入 `mask_unlink_after_publish` 重现了“两方均 exit 70、P0/P1 最终 mask 文件均存在”的状态。修正后的 writer 对任何发布阶段错误回滚本次创建的最终路径和临时路径，发布成功前同步文件数据，发布和临时文件清理后同步父目录。existing destination 仍由硬链接 no-clobber 拒绝，不删除调用前已有文件。仅在父目录文件系统允许 unlink/fsync 的范围内，错误退出不留下本次输出；设备故障导致回滚 syscall 本身失败仍是本地工程故障，需由外层不接受非零退出码的启动合同拒绝该次结果。

新增的发布失败 failpoint 仅编译进 `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON` 构建；正常 Release 配置关闭该宏。未改变协议消息、算法、材料格式、允许泄露、比较方向、通信轮次或历史指标边界。

common-tape harness 另修复 Windows CRLF 工作树下误报 S4 哈希不匹配的问题：验证前将源字节的 CRLF 规范化为 LF 计算固定 Git blob SHA-256；加载与执行的 S4 源内容不作变换。

## 验证范围与门禁

最终代码改动、命令、哈希、运行次数和限制在 `docs/reproduction/BMW16_S26_POST_CANDIDATE_REVIEW_2026-10-09.md`。当前结论分开登记：

- 功能、输出发布和材料/传输合同仅按报告实际执行配置接受；
- 单 key DCF 隐私、整池自适应 view、shuffle/output 联合模拟仍为 `UNPROVEN`；
- `SECURE_ALIAS_READY=NO-GO`；
- `FORMAL_PERFORMANCE_READY=NO-GO / NOT_RUN`。

S26 是用户授权的实验候选后续复核，不改变 M6A→M6B→M7 的正式顺序，也不将 loopback 冒烟结果计入六方案性能表。
