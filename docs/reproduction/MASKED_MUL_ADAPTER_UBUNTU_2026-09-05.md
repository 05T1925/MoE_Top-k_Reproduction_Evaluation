# 无重构乘法 adapter Ubuntu 验证记录（2026-09-05）

## 1. 范围与 revision

- 基线：`origin/main` 的合并提交 `fa380cc`；
- 分支：`design/protocol-iii-mul-adapter`；
- 实现身份：Protocol III 模块化 3 轮基线的前置乘法原语；
- 不在本次范围：M2 rank/runtime 接口、Protocol III 路由、独立进程 E2E、正式
  metrics 和论文精确 2 轮压缩。

## 2. 环境

- Ubuntu 24.04.4 LTS；
- Linux 7.0.0-30-generic x86_64；
- GCC/G++ 13.3.0；
- CMake 3.28.3；
- 4 个逻辑处理器，7.7 GiB 内存。

## 3. 命令

```bash
cmake -S VFSS -B VFSS/build-mul-adapter
cmake --build VFSS/build-mul-adapter \
  --target \
    moe_topk_m1_metrics_test \
    moe_topk_m1_dcf_conformance_test \
    moe_topk_m1_oracle_test \
    moe_topk_m1_cmpagg_test \
    moe_topk_dpf_conformance_test \
    moe_topk_masked_mul_adapter_test \
  -j2

./VFSS/build-mul-adapter/moe_topk_m1_metrics_test
./VFSS/build-mul-adapter/moe_topk_m1_dcf_conformance_test
./VFSS/build-mul-adapter/moe_topk_m1_oracle_test
./VFSS/build-mul-adapter/moe_topk_m1_cmpagg_test
./VFSS/build-mul-adapter/moe_topk_dpf_conformance_test
./VFSS/build-mul-adapter/moe_topk_masked_mul_adapter_test
ctest --test-dir VFSS/build-mul-adapter --output-on-failure
```

开发期间先从 `fa380cc` 按目标逐个回归；创建 PR 前发现 M1.1 已以 `078fcb2`
合并到 `main`，随后合入该基线并把 adapter 注册到现有 CTest 入口。没有改写 M1.1
的四个测试或 metrics 语义。

## 4. 覆盖

- 7 组显式边界/固定值和 32 组固定种子的随机 64-bit 环输入；
- 每组独立生成输入掩码、输出掩码和 `MultKey`；
- 两方材料均经过现有 `Peer`/`Dealer` 内存通道发送接收；
- 仅公开随机掩码输入 `L/R`，adapter 返回两方 additive product shares；
- 测试层求和验证 `share_0 + share_1 = x*y mod 2^64`；
- 覆盖未准备即求值、非法 party、重复准备和重复消费的硬错误；
- 回归运行四个 M1 测试和 DPF conformance 测试。

## 5. 结果与边界

```text
DPF local and Peer/Dealer transport conformance passed: 44 cases
Share-preserving multiplication adapter passed: 39 arithmetic cases and state checks
```

六个目标全部以退出码 0 完成。adapter 源码不调用 `reconstruct(...)`；乘积只在
测试可执行文件中通过两方 share 求和验证。

本结果证明当前 revision 下的 64-bit 环代数、一次性本地状态和现有材料序列化
路径符合设计。它不证明 socket 调度、跨进程材料唯一分配、M2 runtime 接入或完整
Protocol III 正确性；这些工作按设计暂停到 M2 rank/runtime 接口冻结后。
