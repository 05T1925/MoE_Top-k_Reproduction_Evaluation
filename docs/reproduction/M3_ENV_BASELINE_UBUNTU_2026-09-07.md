# M3 Protocol III Ubuntu 环境基线

日期：2026-09-10

代码基准：`main@bb0d0e84ce63b0560db822aa2fcc45b7d811571c`

## 1. 范围

本文记录以下两个 M3 实现的最终 Ubuntu 验证环境：

- `agarwal_protocol_iii_modular_3round`：输入为 `padded_n` 个 priority-key
  additive shares，在线路径为 GRank → DPF routing → secure combine，共 3 轮；
- `moe_topk_protocol_iii_raw_score_modular_5round`：输入为 `logical_n` 个 Q20.12
  raw-score additive shares，包含 carry、sign 两轮输入适配和三轮安全核心，共 5 轮。

本文不覆盖 Secure MoE dispatch、expert compute、完整 CryptoMoE pipeline，也不把
raw-score 五轮扩展表述为论文原生三轮实现。secure combine 属于 M3 三轮核心，不能
列为范围外能力。

## 2. 已记录环境

| 项目 | 值 |
| --- | --- |
| OS | Ubuntu 24.04.4 LTS |
| Kernel | Linux 7.0.0-31-generic x86_64 |
| Compiler | GCC 13.3.0 |
| CMake | 3.28.3 |
| Build type | Debug |
| `BUILD_TESTING` | `ON` |
| `MOE_TOPK_ENABLE_EMP_OT` | `OFF` |
| CPU | 4 logical processors |
| Memory | 7.7 GiB configured |
| Swap | 3.8 GiB |
| Network | local AF_UNIX socketpair |

## 3. 可复现命令

原验证的逐字 shell 历史未随报告保存。以下命令根据当前 CMake target 和测试注册
重建，可用于同配置复跑；不得把它们冒充原始 shell transcript。

```bash
m3_build_dir=/tmp/moe-m3-bb0d0e8
cmake -S VFSS -B "$m3_build_dir" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build "$m3_build_dir" --parallel
ctest --test-dir "$m3_build_dir" -N \
  -R 'moe_topk_(dpf_conformance|masked_mul_adapter|m3_)'
ctest --test-dir "$m3_build_dir" --output-on-failure \
  -R 'moe_topk_(dpf_conformance|masked_mul_adapter|m3_)'
```

## 4. 已提供的验证结果

队友提供的最终 Ubuntu 记录为：

```text
100% tests passed, 0 tests failed out of 11
Total Test time (real) = 193.59 sec
```

该记录覆盖 11 个 M3 CTest；DPF conformance 内含 44 组本地及 Peer/Dealer
transport 验证。结果与更完整的证据归属见
[M3 复检整改关闭记录](M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md)。
