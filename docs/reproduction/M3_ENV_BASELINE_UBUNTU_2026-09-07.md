# M3 Protocol III Environment Baseline Ubuntu 2026-09-07


## Scope

Agarwal Protocol III modular 3-round baseline.


This document does NOT cover:

- Secure MoE dispatch
- Expert compute
- Secure combine
- Full CryptoMoE pipeline


## Repository

branch:

revision:


## Environment

OS:

GCC:

CMake:


## EMP

prefix:


## Build

Command:


## Validation

CTest command:


Result:


## Notes

M3 starts from validated M1/M2 baseline.

Protocol III core target:

agarwal_protocol_iii_modular_3round

online rounds:

3
## 复检整改后的最终状态（2026-09-10）

原环境基线建立后，M3 已完成复检整改。最终代码基准为：

```text
main
bb0d0e84ce63b0560db822aa2fcc45b7d811571c
最终环境：
Ubuntu 24.04.4 LTS
Linux 7.0.0-31-generic x86_64
GCC 13.3.0
CMake 3.28.3
Debug
BUILD_TESTING=ON
MOE_TOPK_ENABLE_EMP_OT=OFF
4 logical processors
7.7 GiB configured memory
3.8 GiB swap
当前存在两个明确区分的正式实现：
- agarwal_protocol_iii_modular_3round
  - 输入为 padded priority-key additive shares；
  - 3 个在线阶段。
- moe_topk_protocol_iii_raw_score_modular_5round
  - 输入为 logical Q20.12 raw-score additive shares；
  - 2 轮输入适配加 3 轮安全核心，共 5 轮。
M3 相关 11 个 CTest 在全新构建目录全部通过。DPF conformance
覆盖 44 组本地及 Peer/Dealer transport 验证。
详细关闭证据见：
[`M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`](M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md)
