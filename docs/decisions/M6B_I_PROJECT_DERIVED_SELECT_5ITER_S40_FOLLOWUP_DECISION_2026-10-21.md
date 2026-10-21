# M6B-I-PROJECT-SELECT-5ITER-S40-FOLLOWUP 决策记录

日期：2026-10-21
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

## 决策

1. 以 S39 文档头 b3b04dd835b355193b83d689c698a688267605bc 和其父级 runtime commit 8028c0fece69fe0c648b1e0f1e12ad7fca723f20 为固定对象。S39 85 项 manifest 的登记 canonical SHA-256 为 09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9；清单重算一致。S40 中 runtime 三文件 Git blobs 与 8028 一致。
2. S40 增加 TEST_ONLY case/result/log harness，没有修改 runtime。receipt/v2、N=128/256 的 211 个独立进程案例在 final Release binary 上运行，207 个 oracle PASS，4 个 no-share abort 记录为 NOT_APPLICABLE_NO_SHARE。旧 195 matrix 是 v2 重新运行，不是沿用 S35 v1 结果。
3. fresh Release default-OFF 构建通过，CMake target/symbol boundary 无 v3；receipt/v2 ON 而 GMW v3 OFF 被配置依赖拒绝。显式 v3+receipt/v2 ON 构建、v2 conformance、v1 compatibility smoke 通过。v3 与 receipt/v2 默认仍 OFF。
4. 无注入 N=128/K=64 trace 中双方 exit=0、双方返回 committed share、oracle XOR 通过，A 为一笔 COMMITTED transaction、两份密文。phase 88 最后一对业务帧均 read/write OK；代理随后多读下一帧时报 unexpected EOF。定性为 POST_FINAL_PROXY_READ_EOF_OBSERVED；业务阶段自然 I/O fault 为 NOT_REPRODUCED。测试 readiness TCP connect-close 导致的 A TLS handshake EOF 也保留并归因为 harness readiness probe，不作为提交故障。
5. receipt/v2 token 的理想独立 A RNG 局部条件均匀结论沿用 S39 CONDITIONAL_PASS_REVIEWED，不在 S40 扩张为安全证明。精确成功路径模拟断点仍是第二个 authenticated peer deposit：A 必须先接收诚实对端密文才由 PENDING 进入 COMMITTED。Sim_b 无 peer material、credential、A context、T key 或 J_b 权限，现有 F_T/v1 没有可执行 honest-peer endpoint route。缺失联合 law 见报告第 7 节。结论是 OPEN_WITH_EXACT_GAP（模型/接口缺项），不是攻击或不可能性。
6. 推荐下一步先版本化 F_A-ATOMIC/v2 honest-peer endpoint transition，令诚实端点用自身理想 output/package 经 A 提交；Sim_b 只接收本方接口消息，不获得 peer secret。若需要具体运行时 relay，再单独定义角色、信任和 L_A；不扩张 L_S29、不伪造 peer-deposit oracle。
7. ideal F_T/v1 到真实 Dealer/triple/permutation/signature/DRBG 桥未到达。N>256 NOT_ADMITTED；A-only、任意 crash-prefix privacy、物理断电耐久性不是本轮定理。正式 LAN/WAN/RTT 未运行/未测量，不创建 secure alias，不推送/合并，不改 VFSS-baseline。

## 门禁摘要

| Gate | 状态 |
|---|---|
| S39 HEAD/runtime commit 与 S39 manifest identity | PASS |
| S40 runtime files unchanged | PASS |
| TEST_ONLY final source/build identity | PASS |
| fresh Release default OFF，target/symbol absent | PASS |
| v2 requires v3 opt-in | PASS |
| explicit v3 + receipt/v2 build | PASS |
| v1 compatibility smoke / v2 conformance | PASS |
| v2 195-case matrix re-run + 16 N128/N256 cases | PASS_WITH_SCOPE |
| normal no-injection protocol phase reads/writes | PASS_WITH_SCOPE |
| natural business-phase I/O fault | NOT_REPRODUCED |
| post-final-frame proxy EOF | OBSERVED_AFTER_FINAL_FRAME |
| honest peer deposit route / party success simulator | OPEN_WITH_EXACT_GAP |
| F_T/v1 to real offline Dealer bridge | NOT_REACHED |
| security conclusion | OPEN_WITH_EXACT_GAP |
| N>256 | NOT_ADMITTED |
| formal LAN/WAN / RTT | NOT_RUN / NOT_MEASURED |
| secure alias / v3 default | NOT_CREATED / OFF |

详见 S40 report、gates、cases 和 source/build/evidence manifest。