# M6B-I PROJECT_DERIVED_SELECT_5ITER S64 决策

日期：2026-11-09
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 固定对象

远端 `m6b-i-project-select5-s63@29ac1c97ddf4fce3c9371e59d1fd259e381b3aeb`，tree `2ad3d23ca1e6c555c1f65efb4a1fe431e8bca869`，父提交 `c9c87799b0f27931249d5b82edce37a5f0f1814c`。S63 的 7 个运行时源码 blob 和 5 项交付清单已按远端 Git 对象复核，交付规范摘要为 `0e81f9520a4bcc1eaea35e8120ea82254dce8c42866c4e8bd510dc9feb73b563`。历史交接索引用于定位来源，归档日期不作为原测试日期。独立功能接收在另一隔离工作树进行；S64 审计分支不修改运行时或 `VFSS-baseline/`。

## 裁决

1. **远端功能接收与安全声明分账。**非 S63 作者在 fresh Release 上完成 default-OFF 与显式 opt-in；runtime/FTv3 bounded conformance 各 exit 0。补逐方 bit 断言后的最终 TEST_ONLY 二进制完成 28/28 独立进程案例：26 个成功/恢复 share 案与冻结 oracle 一致，2 个故障注入 no-share 案符合预期。N=128/256/1000 的边界 K、极值、负值和同分均在实际逐案范围内。只对该提交、构建及这些固定用例记 `PASS_WITH_SCOPE`，不能替代全输入正确性或安全证明。生产 runtime 未改，新增检查仅在 TEST_ONLY 驱动。
2. **测试部署与目标部署分账。**本轮可复现的 Ubuntu 24.04/WSL2 环境提供非秘密本机包版本、库文件哈希与构建运行记录。OpenSSL CLI provider 列表、源码默认值或构建机版本均不能证明 T/A/P 进程的实际 DRBG、熵/重播种策略，也不能代表尚未提供的目标服务器。真实联合随机流桥和目标部署优势仍 `OPEN`。
3. **首个实现桥接断点。**Dealer 的 `RAND_bytes` 有序自适应请求/返回/错误流向理想 IID FTv3 带的替换需要目标部署参数化的 `Adv_DRBG_T(Q,C;E_T,P)`；现有目标配置与可靠具体优势缺失。即使将它列为假设，完整发布成功的条件化还需 `Pr_I[S_issue] ≥ p_min > ε_T` 或失败感知的无条件模拟，不能由功能测试成功率推断。
4. **合同仍待负责人决定。**FTv3 cap-abort 后新 invocation 重试、发材失败双方通知和启动规则、协议级中止是否纳入定理三项仍 `OPEN_CONFIRMATION`。`PAIRED_GENERIC_ABORT` 未被批准，TEST_ONLY 控制器不是生产启动屏障。正常成功、协议中止和实际有限超时分别过门。
5. **S63 理想模型结论维持原范围。**`N≤1000`、一方静态半诚实、理想 FTv3 成功发行/刷新/A token、诚实对端因果执行与外生 SuccessFence 下的应用/API 视图是继承的 `PASS_WITH_SCOPE`。真实部署、协议级中止与实际 wall-clock 成功筛选不据此通过。

## 禁止外推

本路线实现项目派生的 signed Q20.12 稳定 Top-K XOR mask 功能，非 BB90/BMW16 原算法复现。五个算法迭代、比较依赖层、exchange phase 与 RTT 是不同量。正式 LAN/WAN、RTT、AAV86 比较继续 `NOT_RUN/NOT_MEASURED`；无 `secure` alias，不推送、不合并。
