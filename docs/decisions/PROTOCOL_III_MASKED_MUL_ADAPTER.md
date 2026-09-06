# Protocol III 无重构乘法 adapter

状态：**M3 前置原语，等待评审；停在 M2 rank/runtime 接口之前**。

本文定义 Protocol III 模块化 3 轮基线 R3 使用的 64-bit 环乘法薄适配器。它是
VFSS `MultGen`/`MultEval` 的项目级工程绑定，不改变底层乘法原语，也不声称完成
Protocol III、独立进程 E2E 或论文精确 2 轮压缩。

## 1. 安全边界

- Dealer 只在输入未知的离线阶段生成材料；在线阶段不参与；
- 每个乘法实例使用独立的左输入掩码 `a`、右输入掩码 `b` 和输出掩码 `c`；
- 在线只公开 `L=x+a` 和 `R=y+b`，不公开 `x`、`y` 或 `x*y`；
- 每方调用 `MultEval` 后减去自己的 `[c]`，只得到加法 product share；
- adapter 不调用、包装或暴露 `reconstruct(...)`；正确性重构仅存在于测试可执行文件；
- 材料带有本地一次性状态：生成 masked open share 后不能再次生成，求值后不能
  再次求值。复制或跨进程重复分配的防护由未来 runtime 的 material allocator 负责。

## 2. 代数契约

在 `Z_(2^64)` 中，Dealer 调用：

```text
(key_0, key_1) = MultGen(a, b, c)
([c]_0, [c]_1) = splitShare(c, 64)
```

在线方 `p` 持有 `[x]_p`、`[y]_p`、`key_p` 和 `[c]_p`：

```text
[L]_p = [x]_p + key_p.a
[R]_p = [y]_p + key_p.b
L = [L]_0 + [L]_1
R = [R]_0 + [R]_1
[z]_p = MultEval(p, key_p, L, R) - [c]_p
```

底层公式保证 `[z]_0+[z]_1=x*y mod 2^64`。`L/R` 是允许公开的独立随机掩码
值；`[z]_p` 留在 share 形式中。

## 3. 代码接口与传输

`VFSS/include/moe_topk/masked_mul_adapter.h` 提供四类最小接口：

1. `generate_masked_mul_material`：Dealer 生成两方材料；
2. `prepare_masked_mul_open_share`：在线方生成待交换的 `[L]_p/[R]_p`；
3. `evaluate_masked_mul_share`：根据已公开的 `L/R` 返回 product share；
4. `send_masked_mul_material` / `receive_masked_mul_material`：复用 VFSS
   `Peer::send_mult_key`、`Dealer::recv_mult_key` 和 64-bit `send_ge/recv_ge`。

adapter 显式把 `MultKey.Bin/Bout` 初始化并校验为 64，避免原生 `MultGen` 未写入
这两个传输字段时携带不确定元数据。party 编号只接受 adapter 自己的 0/1 枚举，
不直接依赖 VFSS 全局的 Dealer/Server/Client 数值。

## 4. Conformance 测试

`moe_topk_masked_mul_adapter_test` 在内存 `Peer/Dealer` 通道上验证：

- 0、1、`UINT64_MAX`、溢出边界、固定一般值和 32 组确定性随机输入；
- 材料传输前后的 64-bit 环乘法语义；
- 输出保持为两方 additive shares，仅测试层求和并与明文环乘积比较；
- 传输字节计数与实际游标一致；
- 求值前未准备、非法 party、重复准备和重复消费均显式失败。

该测试隔离 socket 建连与调度，只证明生产序列化函数和 adapter 代数行为。三独立
进程 socket E2E 仍属于 M3 集成测试。

## 5. 暂停点：等待 M2 交接

本前置工作不新增 Protocol III runtime，也不接入 rank。以下工作明确暂停，直到
M2 冻结并合并 `docs/TEAM_WORK_PLAN.md` 第 4 节要求的接口：

- 从 M2 runtime 接收 priority-rank additive shares；
- 复用 M2 的 party/连接生命周期和 masked-value exchange primitive；
- 批量分配 `(i,k)` 乘法材料并接入一次性 allocator；
- 把 Peer counters 接入统一 `MetricsRecord`；
- 将 R2 DPF indicator shares 与 R3 单位 payload shares 送入本 adapter；
- 生成原顺序 XOR bit-mask，并进行独立进程与 oracle differential 测试。

接入时只能增加薄绑定，不得复制第二套 rank、party、通信或 metrics 语义。
