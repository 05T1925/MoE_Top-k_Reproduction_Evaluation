# DPF conformance：Ubuntu 24.04

日期：2026-09-04

基线 revision：`efb47ca`

验证分支：`test/protocol-iii-dpf-conformance`

## 范围

本记录属于 M3 前置原语验证，不是 Protocol III 路由实现，也不改变 M1/M2 的
score、rank、mask、通信或 metrics 接口。

验证对象：

1. `keyGenDPF` 与 `evalDPF_Payload` 的加法 share 重构；
2. DPF key 经 `Peer::send_dpf_keypack` 与 `Dealer::recv_dpf_keypack` 传输前后，
   每一方在全输入域上的求值 share 保持一致。

第 2 项使用 `MemBuf` 后端，以隔离 socket 建连和调度影响，同时实际调用 VFSS
生产代码中的 `Peer`/Dealer DPF 序列化函数。它不证明独立进程 TCP E2E；该项仍按
Protocol III 设计文档第 8.3 节在 M3 集成阶段验证。

## 环境

- OS：Ubuntu 24.04.4 LTS；
- kernel：`7.0.0-30-generic`；
- compiler：GCC/G++ 13.3.0；
- CMake：3.28.3；
- CPU：4 个逻辑处理器；
- memory：7.7 GiB。

## 覆盖

- `n = 1, 3, 5, 127, 128, 129`；
- DPF 点为 0 和 `n-1`；
- 对每个 `n` 求值完整的最小二进制域，因此非 2 次幂时包含 padding 位置；
- payload 为 0、1 和一般非零 64-bit 值；
- 输出位宽为 1、8、9、16、17、32、33、64，覆盖 `send_ge` 的各序列化宽度分支及边界；
- 对传输后的 party 0 和 party 1 key 分别比较每个输入点的 share，而不只比较最终
  重构值；
- 共执行 44 组 key generation / full-domain evaluation / transport case。

## 命令与结果

```bash
cmake -S VFSS -B VFSS/build-dpf-conformance -DCMAKE_BUILD_TYPE=Release
cmake --build VFSS/build-dpf-conformance \
  --target moe_topk_dpf_conformance_test -j2
./VFSS/build-dpf-conformance/moe_topk_dpf_conformance_test
```

结果：

```text
DPF local and Peer/Dealer transport conformance passed: 44 cases
```

结论：在上述 revision、环境和覆盖范围内，本地 DPF payload 重构与内存通道
Peer/Dealer key 传输一致性通过。socket 独立进程 E2E、秘密共享乘法 adapter 和
完整 M3 协议均未在本记录中验证。
