# S20 外部原始证据索引（2026-10-07）

日志、材料、凭据、临时 CA、bundle、二进制和逐次测试数据均保存在仓库外：
`C:\Users\28641\.codex\artifacts\BMW16_S20_20261007\` 及 WSL `/tmp/bmw16-s20-*`。
仓库只登记摘要与 SHA-256，不提交 sidecar、密钥、证书、构建物或逐次大日志。

方案/构建标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`。
起始 S19 revision：`617cd3cbbb7b5513c9f606ef5ceb7a73e7a96ef4`。
工具链：WSL2 Ubuntu 24.04，GCC 13.3，CMake 3.28.3，OpenSSL 3.0.13，Release。

## 复跑命令

S19 独立重建（先于 S20 修改）：

```text
cmake -S VFSS -B /tmp/bmw16-s20-receive-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/bmw16-s20-receive-build --target moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_test -j 4
moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node --extended
moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node --n1000
```

S20 conformance/E2E：

```text
cmake --build /tmp/bmw16-s20-build --target moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_test moe_topk_m2_transport_conformance_test -j 4
ctest --test-dir /tmp/bmw16-s20-build -R "moe_topk_bmw16_experimental_party_test|moe_topk_m2_transport_conformance_test" --output-on-failure
moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node --tls
moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node --tls-n1000
```

`--tls` 在隔离 TEST_ONLY 控制器下 fork/exec 三个 Linux 身份并生成临时测试 CA；party 凭据不是从输入份额或算法 seed 派生。TLS failpoint 测试版用 `-DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON` 构建。另以 `BUILD_TESTING=OFF` 且未指定该选项配置、构建普通 party-node，检查其 CMakeCache 为 OFF，并由 `strings` 确认二进制无 `MOE_BMW16_TEST_FAILPOINT`/failpoint selector 字符串。

common-tape：

```text
py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/bmw16-s20-build/moe_topk_bmw16_experimental_party_test --s4 C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\select4r.py --out C:\Users\28641\.codex\artifacts\BMW16_S20_20261007\common-tape-final
```

common-tape 输入种子由脚本固定规则生成：`first_seed=50000`，每例使用 `seed = first_seed + 100*n + K + case_index*10000`，tape PRNG seed 为 `seed XOR 0x636F6D6D6F6E7461`。39/39 PASS，含 n=2..8 每个 K 和 n=64 的四种输入剖面；它是 TEST_ONLY 控制流差分，不是协议熵或概率证明。

固定非自适应图候选 C 的独立语义夹具：

```text
py -3 experiments/TEST_ONLY_BMW16_S20/fixed_graph_rank_fixture.py
```

该夹具不是安全协议，只检查 TEST_ONLY 的固定全对 rank/share 聚合语义。最终输出 `cases=5055 ... security=NOT_TESTED`。

## 历史 S19 独立复跑记录（不得记作 S20）

| 文件 | SHA-256 | 内容 |
|---|---|---|
| `s19_independent_extended.log` | `e4d86fd0126cb71890b5f6a9a182eaa10f2aea530495d417181f9967a1357c61` | S19 源构建的本地 party-node 扩展矩阵；含 n=1/2/5/8/64/128/256、K 边界及 ready/material 失败案例 |
| `s19_independent_n1000.log` | `a6e88d281b4fbf94328de514f61f1d4d60a063e1a83fbb1ec6352b79c4a63427` | S19 源构建 n=1000/K=80 本地 fresh-material E2E 历史复跑；每方 sidecar 4,967,527,664 B |

S19 构建身份：party-node `4791e635ee5adf40506a9c4e02446d72814ad97c561bc0fc511e1430a232df57`；E2E runner `756d047ea0b480fefe11015150b3070e192bccb742a9d88ac50a0318be0ecba4`。

## S20 本轮逐次证据

| 文件 | SHA-256 | 内容/结果 |
|---|---|---|
| `common-tape-final/matrix.json` | `202dbfa16cc7d4c4f3ef077ea1bb8322a981621fd3b4eadf0b1b6154da21e467` | 最终 S20 party-test binary 的 39/39 S4/C++ common-tape trace 差分 |
| `tls_e2e_final.log` | `c702d2e232d06e4654a799f54f1554eb7da2fd4feb5612534580ca577fb0ec46` | 最终 TLS E2E runner：n=1 singleton shortcut、n=2/5/8/64/128/256 成功；6 例跨 UID 文件拒读 PASS、6 例同 root 重启重领拒绝；13 项 TLS 交付/静默负例无 mask |
| `tls_e2e_n1000.log` | `7717ea13ab68a871c20eb050189a403968c6f98c3c70863004a22df65146e98e` | n=1000 首次已完成 T 发材/接收，但 30 s party online timeout，存活方 LOCAL_ONLY 通信 abort；无成功 mask |
| `tls_e2e_n1000_retry.log` | `3e61bcdd3db6e13991a65fe74ea9e1d162ba121d8c32eb5e06167f0db9ced98` | 加长到有界 600 s party timeout 后，以 fresh bundle 重跑 n=1000/K=80；T、receivers 和双方 party 均 exit 0，oracle exact success |
| `targeted_ctest.log` | `217f9ccca9fd390f6eada02742f407eae094508dafcc5a1c593605b71aa8fb10` | 相关 `moe_topk_bmw16_experimental_party_test`、`moe_topk_m2_transport_conformance_test`：2/2 PASS；不是全量 CTest |
| `targeted_ctest_final.log` | `a720153729d13fc17c2ac9156c42d70d7a3a64384692342cdae324f580574270` | 最终改动后相关 CTest：2/2 PASS；不是全量 CTest |
| `fixed_graph_rank_fixture.log` | `c3e51323f96a2cc073f56d4e981c2b9355f1cf076d7ff3d67bb00a6b0670ee40` | TEST_ONLY 非自适应全对图 rank/share 语义夹具：5,055 cases PASS，security=NOT_TESTED |

注意：TLS runner 的并发 stdout 存在行级交错；以单次最终 `tls_e2e`/`tls_replay`/`tls_delivery_abort` 汇总状态和退出码为判据，不把混排的 party 诊断行当独立计时日志。n=1000 首次失败没有筛掉，单独保留。

## 二进制 SHA-256

| 二进制 | SHA-256 | 用途 |
|---|---|---|
| `/tmp/bmw16-s20-build/moe_topk_bmw16_experimental_party_node` | `e5730ff9e7719e40904f6a998cd37d4aad5d4edde23f4b8d20f1d78b73e12981` | 最终 S20 Release、TEST_ONLY failpoint ON 的 TLS/party 入口；小矩阵由此二进制运行 |
| `/tmp/bmw16-s20-build/moe_topk_bmw16_experimental_party_node_e2e_test` | `8652fa0516da7ba3e36067d152bf82ce47840d945dae335196e120a91ec6da06` | 最终 TLS/failpoint/replay/UID 测试启动器 |
| `/tmp/bmw16-s20-default-build/moe_topk_bmw16_experimental_party_node` | `7706f3f387744d4e67f9a7ca4cbd4144df9477c766e115b7372c0560d1797392` | `BUILD_TESTING=OFF`、failpoint OFF；`strings` 未发现 failpoint 或本地测试投递 selector |
| `/tmp/bmw16-s20-build/moe_topk_bmw16_experimental_party_node`（n=1000 尝试时） | `e9e5e89e00fd94087d5f9b0a8629b770440feecf29170693309438340c7d328d` | n=1000 功能尝试二进制；后续只对测试/本地命令分支和 silent failpoint 增加了隔离，不改变 TLS 交付/party 主路径 |

## 最终源码 SHA-256

| 文件 | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `07e62585ab9e78ac74368aeb3d215b6516d3da36cc44d1e2c4ab44b680b102e2` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `2dec177b8935395f1c0a50499105bf0eb3c15ef19849828890fb75eb854735f7` |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | `b46d0ffae57f40b8f4e40fce6c84cc243c9ebad09fb743c89d184c013de74298` |
| `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` | `60946dd32a778dcccc352c9116e366165ed859aea76d710bd07b3da4170bc601` |
| `VFSS/include/moe_topk/experimental_bmw16_material_delivery.h` | `1d7126bdf806a29a6845a2881e8b4d9daf786223e3c94fbe9f8d7c03f33ceeb6` |
| `VFSS/src/moe_topk/experimental_bmw16_material_delivery.cpp` | `0a2f1617c1211becfff6e4152042b56c575aac465cd57ceb1d953edec34023f9` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `8214555133420584788f713bbb4013cb3b1ff4eecc11ff246e26d28ed37db223` |
| `experiments/TEST_ONLY_BMW16_S20/fixed_graph_rank_fixture.py` | `a5ab0dfa9bc40a4eff711899b65cc8f0c085ab755594e3d462d7a19cd3df2a3c` |

`common-tape-final` JSON 保存每个输入/随机 tape、C++ trace 和 differential 子报告的各自 hash；外部 TLS log 不含私钥、bundle payload 或 score shares。
