# S52 决策：API-v2 独立功能接收与有限超时安全范围

日期：2026-10-25
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL
被测基线：S51 commit 1cc1e5202b3a603b55aaae985704933d70db0794

## 决定

1. 接收对象严格固定为 S51 commit 1cc1e5202b3a603b55aaae985704933d70db0794；S51 manifest 的源码与交付 Git blob 匹配。构建和功能验证在新 worktree 完成，执行者未参与 S51 runtime 实现。S52 没有改动 runtime 或测试源码。
2. S51 API-v2/FTv3/receipt-v2 的本轮独立定向功能接收为 PASS_WITH_SCOPE：default-OFF 隔离探针、显式 opt-in Release 目标、相关 conformance、正常和恢复路径、以及 N=1000 的 K={1,500,1000} 加 K={8,999} 诊断用例通过。它不是全输入正确性证明，也不是安全结论。N=1000 中双方 party 进程按协议并发；各 K 案例依次执行以约束内存。
3. S50 Kpre3 与 S51 Kpost3 可以在外生 SuccessFence 下作有界组合：理想 IID FTv3 且 cap_success；静态半诚实单方腐化；诚实对端按自身输入、输出、package 与凭据因果提交；A 使用理想独立 token RNG；环境保证每个所需合法应用消息在相应 deadline 前处理。结论限于 N≤256 的理想应用消息/API 成功视图，不包含真实耗时、timeout 选择、可用性或任意故障前缀。沿用 S51 已有非作者审阅范围；本轮没有新的独立密码学重推。
4. 实际有限超时下事后筛选的成功视图保持 OPEN。真实源码有每帧/握手 deadline、托管方等待第二份提交的 deadline 和无 deadline 的阻塞 TCP connect；没有部署 scheduler/network 延迟模型，也未证明成功事件条件独立于 Vpre。首个缺失步骤为
   Law(Vpre_b | Q_b, cap_success, S_actual=1) ≈ Law(Vpre_b | Q_b, cap_success)。
   这表示证明缺口，不表示攻击或不可能性。本轮未把时间、超时、消息活动形状或服务状态并入 L_S29。
5. 低概率发材中止接受继续为 OPEN_CONFIRMATION；自然业务 I/O 故障为 NOT_REPRODUCED。正式 LAN/WAN、RTT、AAV86 和性能矩阵未运行/未测量。真实 Dealer/DRBG、A token RNG、签名、TLS/mTLS、recipient encryption 与完整 GMW/Select/shuffle/output/escrow 组合继续独立开放；不创建 secure alias。

## 变更范围与交付

只更新 S52 decision、report、gates、cases、source/build/evidence manifest 和 docs/IMPLEMENTATION_PLAN.md。没有 runtime 修复。提交仅在隔离分支本地创建，不推送、不合并。S51/S50 历史记录及 VFSS-baseline/ 保持不变。

详细命令、二进制哈希、案例和证据边界见同日期 S52 reproduction report、gates、cases 与 source/build/evidence manifest。
