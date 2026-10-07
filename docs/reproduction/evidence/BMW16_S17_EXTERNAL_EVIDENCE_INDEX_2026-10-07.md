# BMW16 S17 外部证据索引

证据目录：`C:\Users\28641\.codex\artifacts\BMW16_S17_20261007`。运行材料不进入 Git。所有路径、输入、种子与重建命令另见 S17 验证报告。

| 证据 | 内容 | SHA-256 / 身份 |
|---|---|---|
| `S17_START_SOURCE_SHA256.txt` | 起点与关键源初始哈希 | 本地索引；列在归档目录 |
| `party_test_s17_final.log` | 最终 C++ party test，11 行，含 n=64 和侧车 | `DB50345CDF26BE0F9BEB546329B31A48BF90FD59A758A6A2690962951C4E5492` |
| `threeproc_s17_rerun.log` | 现有 3proc runner，61 个顶层 case | `4A7C6FD2D2E98347E556EED6B0592327E47B943887612FA0134CBB2F033F587D` |
| `party_node_e2e_s17.log` | T 后输入、独立 UID P0/P1 party-node E2E，n=1/2/5/8 | `72428F50BA21E3B4E0D3016356B9718DC03E24D93F47C77B95C3DCE11D322452` |
| `party_node_e2e_extended_s17.log` | 上述基础 case 加 n=64/128/256 的独立进程 E2E；七例 oracle 检查 | `4E2CC3BFD6B1C48EB10E23CF3B778EE3DD1F76EBE126ED8CE0826A9950E7E6DB` |
| `capacity_preflight_s17.json` | n=64…10⁶ checked 容量公式 | `AF86090D5C1B1612298FFB9AF8014B2DCE65E2C420D5234BA16BBEC170FF1048` |
| `s4_cpp_n256_s17_final2/matrix.json` | S4/C++ n=256 K2/8 common tape | `d0c0675f1be2c185c4b15c3693f38d925d1826eb2b2205d06fde368289d3b539` |
| `select4r_frozen_s4.py` | S4 frozen source | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| `protocol_i_regressions_s17.log` | M2 score/uCMP/shuffle/inverse targets | `D1332FB0729855EA9C76D05E3D40CE84A7F38DCA543FD08329DD1C0060A758E9`；五个目标均 exit 0 |
| `m3_regressions_s17.log` | Protocol III six regression targets including raw-score independent-process entry | `AEF9D0A8C5F724048AEE53261A5503F1A26DAFEAFE168A629F193EEE02E95B61`；六个 CTest targets passed |

本轮重建二进制：

| Binary | SHA-256 |
|---|---|
| `moe_topk_bmw16_experimental_party_test` | `89fe1031ac7db62677c51533e037f7ea70f285ce06ab5ae08d4dea65493fc511` |
| `moe_topk_bmw16_experimental_party_node` | `9b2506c719f2508c9663f9713432edf81a446422c7fd6335fd2ae8744c01b2d0` |
| `moe_topk_bmw16_experimental_party_node_e2e_test` | `074fa6a42058c3502bccdcd392a5431400a1411fbd5bddb0ff83070c98987970` |
| `moe_topk_bmw16_experimental_3proc_test` | `cdf2340f5e326b13aff669e52abcdf6022767d889e394f3d3877832f82d94188` |

party test、party-node E2E、3proc runner 和 protocol regression 均在本轮最终构建/源码状态重跑。最终源码与文档逐文件 SHA-256 见外部 `S17_FINAL_SOURCE_AND_BINARY_HASHES.txt`；该清单不进入 Git。

证据分级：

- `PASS`: 命令本轮退出码 0，原始标准输出/错误留在上表路径，或 C++ 测试自行核验输出。
- `PASS_WITH_SCOPE`: 测试在 TEST_ONLY、loopback 或本地 root fixture 中通过，不表示部署安全或正式性能。
- `NOT_RUN`: n=1000 streaming KeyGen、V4 LAN/WAN、远程 credential/transport、生产 secure entry。
- `NOT_MEASURED`: 跨方案可比性、全库 PRG/AES、服务器级 IO/RSS、LAN/WAN。
