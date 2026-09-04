# M1.1 Ubuntu 24.04 验收记录

## 结论与边界

验收已通过。测试代码 revision 为
`a2efe5e3d2d22bb3c031fb24dc3246c37d442fad`，分支为
`codex/m1-1-ctest-metrics-provenance-integration`。在新的 Ubuntu 临时构建目录
`/tmp/moe_topk_m1_1_ubuntu.QNohzO` 中，`ctest -N` 恰发现 4 项，
`ctest --output-on-failure` 为 4/4 通过。

实现标签 `m1_1_test_and_metrics_foundation` 的证据等级是项目工程基础设施：本次只
整合 CTest 注册和 `MetricsRecord` provenance，不改变已冻结的 Q20.12 score、稳定
tie、原始顺序 bit-mask 输出、论文协议角色、消息、轮数或泄露。它不测量 LAN/WAN
性能；网络、时延、通信和性能字段保持 `NOT_MEASURED`。2026-09-04 的 DPF
conformance 仍只是 M3 前置原语验证，不是 M1 CTest 的第五项，也不是完整 M3 实现证据。

## 实际环境

| 项目 | 实际值 |
| --- | --- |
| OS | Ubuntu 24.04.4 LTS（Noble），WSL2，kernel `6.6.87.2-microsoft-standard-WSL2` |
| 架构 | `x86_64` |
| CPU | 13th Gen Intel(R) Core(TM) i9-13980HX；32 个在线逻辑 CPU |
| 内存 | 8122372096 bytes 总内存 |
| GCC / G++ | 13.3.0（Ubuntu `13.3.0-6ubuntu2~24.04.1`） |
| CMake | 3.28.3 |
| Git | 2.43.0 |
| Eigen | `libeigen3-dev` 3.4.0-4build0.1 |
| OpenMP | `libomp-dev` 18.0；CMake 发现 OpenMP C++ 4.5 |
| Build type | `Debug` |
| 编译 flags | `-Wno-write-strings -Wno-unused-result -maes -Wno-ignored-attributes -march=native -Wno-deprecated-declarations` |

依赖通过以下最小安装命令准备：

```sh
sudo apt update
sudo apt install -y build-essential cmake git libeigen3-dev libomp-dev
```

## 实际复现命令与结果

```sh
m11_build_dir="$(mktemp -d /tmp/moe_topk_m1_1_ubuntu.XXXXXX)"

cmake -S VFSS -B "$m11_build_dir" -DCMAKE_BUILD_TYPE=Debug

cmake --build "$m11_build_dir" --target \
  moe_topk_m1_oracle_test \
  moe_topk_m1_cmpagg_test \
  moe_topk_m1_metrics_test \
  moe_topk_m1_dcf_conformance_test -j2

ctest --test-dir "$m11_build_dir" -N
ctest --test-dir "$m11_build_dir" --output-on-failure
```

受调用端每次 30 秒生命周期限制影响，实际 `-j2` 构建在同一干净目录中以相同的目标
命令续跑；CMake 只重用已完成的同一目录对象文件，没有更改任何源码或测试预期。最终
结果为：

```text
Total Tests: 4
1/4 moe_topk_m1_metrics_test ........... Passed
2/4 moe_topk_m1_dcf_conformance_test ... Passed
3/4 moe_topk_m1_oracle_test ............ Passed
4/4 moe_topk_m1_cmpagg_test ............ Passed
100% tests passed, 0 tests failed out of 4
```

CMake 配置与最终构建/CTest 输出中没有编译器或 CMake 警告。WSL 启动时显示了宿主
localhost 代理不适用于 NAT 模式的提示；它不影响本地 configure、build 或本次 CTest，
也不能被解释为网络性能或 LAN/WAN 测量。

## 提交前复检要求

在提交与推送前重新执行：

```sh
git diff --check
git diff --quiet vfss-baseline-2026-09-03 -- VFSS-baseline
git check-ignore -v Papers Agarwal_TopK ADSMPC CipherGPT
git status --short
git diff --name-only
git diff
git diff --cached --check
```

预期只包含 M1.1 的 CTest、metrics provenance 与本验收记录；不得引入 M2/M3 路由、
shuffle、文件通信、在线 Dealer、`MockShuffle`、安全路径重构、参考工程、论文、密钥、
日志或冻结 baseline 变化。
