# M1 score、Top-K 与计量边界

状态：**已冻结**（2026-09-04）。这是 M1 的统一 score 语义，后续实现、测试数据、
输入适配和指标记录必须引用本文件及 `VFSS/include/moe_topk/score_semantics.h`。

## 团队统一语义（醒目入口）

- 存储：32-bit raw word，按二补码解释。
- 数值：signed fixed-point，scale 为 12；raw word 的数值是其二补码整数除以 `2^12`。
- 顺序：数值降序；数值相等时 original_index 升序。
- Top-K：选择此前 K 个位置并输出原始输入顺序的 bit-mask。
- CmpAgg rank：按上述 Top-K 顺序编号，rank 0 为最高优先级。它保留 Agarwal stable
  tie 的“较小 original_index 在前”关系；升降序差异在这个明文/协议边界显式处理。
- 随机向量：使用记录的种子，量化整数均匀采样于 `[-32*2^12, 32*2^12]`，两端均包含。
- Protocol I / VFSS DCF 边界：比较前将每个 raw score 变换为 `raw ^ 0x80000000`，再调用
  raw unsigned DCF；此单调映射必须属于协议计量路径，不能作为测试后处理。

## M1 固定测试向量

测试输入格式为 `std::vector<uint32_t>`，每个元素是上述 32-bit raw word，位置即
`original_index`。`topk_oracle_test.cpp` 固定随机 seed 为 `0x4d315f31`，
`cmpagg_test.cpp` 固定随机 seed 为 `0x434d5041`；两者都从已冻结闭区间均匀抽取量化
整数后编码为 raw word。固定手工向量覆盖重复、全相等、负数、非 2 次幂长度和
`INT32_MIN/INT32_MAX`。这些向量只用于 test 模式，不会进入未来 secure 接口。

## 证据

| 来源 | 存储类型 | 有符号/无符号解释 | 定点 scale | 比较方向 | 溢出与合法范围 | 不确定性 |
| --- | --- | --- | --- | --- | --- | --- |
| Agarwal CCS 2024，§3 与 Figure 2 | 输入为有序域 `[L]` 内的元素，分享群为 `Z_L` | 未规定 | 未规定 | rank 为更小元素数；最大值 rank 为 `n-1` | 论文要求值在 `[L]`，未给本项目的 32 位范围 | 未规定 signedness、scale、分布 |
| Agarwal CCS 2024，§3.1 | 有序输入 | 未规定 | 未规定 | stable rank 额外计入相等且原始下标较小的元素 | 同上 | 只定义稳定顺序，不规定编码 |
| `Agarwal_TopK/protocol1/runtime/include/protocol_row.h` 与 `src/runtime.cpp` | `uint64_t` score；以 `score * 2^index_bits + original_index` 形成 key | 无符号 | 无 | 升序 stable rank；取排序结果最后 K 个 | score 必须不超过声明 bit mask；参考 runtime 还要求 stable key 在其 64 位 guard range 内 | 这是本地参考行为，不是论文的 32 位团队决定 |
| CipherGPT 论文 §5、§8.2 | `Z_(2^l)` 中的共享值 | Top-K 算法未单独声明 signedness；模型表示来自量化浮点 | 论文评估 `L=12`，并保证值小于 `2^l-1`，`l=37` | 取最大的 K；算法使用 `x_i >= pivot` | 论文没有给 32 位测试域，且未定义稳定同分 | 不能从论文推出 32 位二补码 Top-K 的完整边界 |
| `CipherGPT/src/define.h`、`cpu_calculate_function.h`、`test/Top_K_paper_test.cpp` | `uint64_t` 容器中的 40-bit ring word | 二补码：MSB 为负，`get_signed_val` 解码 | `default_scale = 12` | 测试 oracle 对 `int64_t` 降序；协议分区使用 `>= pivot` | 测试生成 `[-32*2^12, 32*2^12)`；容器值掩码到 40 bit | 没有稳定同分规则；不是 32 位设置 |
| `VFSS/ext/FSS/include/FSS/group_element.h`、`dcf.cpp` | `GroupElement = uint64_t` | DCF input 是原始位串的无符号词典序 | 无 | `keyGenDCF`/`evalDCF` 重构为 `x < threshold` | `Bin=32` 时输入为 `[0, 2^32-1]` | 这是原语调用语义；不是最终 signed-score 比较语义 |

## 已冻结契约

- Top-K 方向：选择数值最大的 `K` 个 score。
- 同分规则：原始下标较小者优先。它与 Agarwal stable rank 一致：升序 rank 中较小
  下标先出现；因此在“score 降序、index 升序”的 Top-K 顺序中也优先。
- 合法 `K`：非空输入时 `1 <= K <= m`。`K=0`、`K>m` 和空输入均为显式错误；不会
  截断或返回部分 mask。
- 输出：原始输入顺序、长度 `m` 的 bit-mask；每项为 0/1，恰有 `K` 项为 1。
- `test` 模式可重构固定向量以验证；未来 `secure` 模式不得进入测试专用明文路径。
- M1 DCF conformance 只证明原始 VFSS API 的 raw unsigned `x < threshold`。它不读取
  旧 key、不复制 FSS；最终 score 比较必须先应用已冻结的 `raw ^ 0x80000000` 映射。

## 基线边界

Agarwal Protocol I 当前本地参考以无符号 stable key 排序；CipherGPT 当前本地参考以
二补码 fixed-point word 表示模型值。两者不能直接共用 32-bit 随机输入。

Protocol I 的边界应在生成 stable key 前把 raw 32-bit word 映射为
`raw ^ 0x80000000`；该映射保留 signed
数值顺序，再以原始下标完成 stable tie。CipherGPT 保留自己的 signed 解码与比较，
但必须在同分时比较绑定的原始下标。该转换属于协议被测路径，不能作为测试后处理。

此前未决的 signedness、scale 与随机分布已由团队确认，不再是 M1 的语义阻塞项。

## M1 复现命令

本机 Apple Clang 需要 Homebrew 的 `eigen@3` 和 `libomp`；不需要 Homebrew GCC。配置时
显式选择 Eigen 3，避免已链接的 Eigen 5 被 `find_package(Eigen3 3.3)` 误选：

```sh
cmake -S VFSS -B /tmp/moe_plan_vfss_m1_build -DCMAKE_BUILD_TYPE=Debug \
  -DEigen3_DIR="$(brew --prefix eigen@3)/share/eigen3/cmake"
cmake --build /tmp/moe_plan_vfss_m1_build --target \
  moe_topk_m1_oracle_test moe_topk_m1_cmpagg_test \
  moe_topk_m1_metrics_test moe_topk_m1_dcf_conformance_test -j2
/tmp/moe_plan_vfss_m1_build/moe_topk_m1_oracle_test
/tmp/moe_plan_vfss_m1_build/moe_topk_m1_cmpagg_test
/tmp/moe_plan_vfss_m1_build/moe_topk_m1_metrics_test
/tmp/moe_plan_vfss_m1_build/moe_topk_m1_dcf_conformance_test
```
