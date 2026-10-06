# BMW16 S14 原始证据索引

- 日期：2026-10-06
- 起始源码：S13 `0eab2e358bf34904fbca542629f8662db9dd62f3`
- 原始输出归档：`C:\Users\28641\.codex\artifacts\BMW16_S14_20261006`
- 归档 manifest：`SHA256SUMS.txt`
- manifest SHA-256：`8899b04952e8d6d287a7d74804f9bc1eaece02e52174c1254eb38cf3736360ef`
- 文件数：64（逐文件复算通过）

## 内容布局

| 子目录 | 内容 | 是否进 Git |
|---|---|---|
| `wsl/` | checked capacity JSON、资源预检、构建/CTest/final E2E 输出、n128 RSS、历史调试失败 | 否；仅外部归档 |
| `n128_common_tape/` | 128/K=2,8,80 的共同随机 tape、S4/C++ trace、逐项 differential JSON、matrix summary | 否；只在隔离 TEST_ONLY runner 中消费 |
| `pilot_final/` | 环境、64/8 与 128/80 warmup+五次结果、逐次 round/stage counter、完整 pilot JSON | 否；本机 loopback raw evidence |

仓库只提交复跑脚本、决策/总结报告与此索引。外部归档未包含构建 binary、party key、AEAD decryption key、真实输入 shares 或明文 mask；trace 中的数据是确定性 TEST_ONLY fixture 轨迹，按仓库生成材料/原始证据边界留在外部目录。

## 复核关键身份

- S4 Python Select：`04ed6f8a352ab2277434344094471dff82f83e4b`，SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。
- n128 matrix：`n128_common_tape/matrix.json`，SHA-256 `4948cee08b88a7208145cebc1eea1a1639046710980a714ac05afcb840e0558e`。
- 3proc final output：`wsl/final-review-3proc.log`，59 case / exit 0，SHA-256 `85b722044a2a23b9be34b5c43ea97e389ee3373d6f65610baae9be74c4aa94ba`；此前套件输出 `wsl/bmw16-s14-final-3proc.log` 也保留，SHA-256 `0a7b2e5a2fef48ced1c7bb892c108f67466ffed8d62d7bcd0348b47b6fe8d9c2`。n128 的 case seed 为 128002/128008/128080。
- CTest 6/6：`wsl/bmw16-s14-final-ctest2.log`，结果 hash `0505e6597e3fcee38dc9cb27026653c7fb13bcec6527d9be06e295d2ebd60baa`。
- build：`wsl/bmw16-s14-final-build.log`，SHA-256 `0c08aa87477193c4fa34bc3c67be29dad0448d0ff64e38d2f60d2b9b367652a4`。
- pilot：`pilot_final/pilot.json`，SHA-256 `efb2f35cb5db002354555335c0b47a66da29bd9c00e2e48e662eb4e0a1118158`。
- checked容量表：`wsl/bmw16-s14-capacity.json`，SHA-256 `a26203fe555eed7072e84847eff30f56dadb859da112896259b71d2ef9365495`。

> 如果 manifest 中的路径随外部归档移动，先按文件 basename 定位，之后从归档根运行哈希重验。报告中的二进制 SHA-256 是构建时外部路径 `/tmp/bmw16-s14-rebuild/moe_topk_bmw16_experimental_3proc_test` 的身份；binary 本身不在归档中。

## 运行与提交 identity

报告记录的命令和原始输入 seed 在 `BMW16_S14_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md`。最终代码 recheck 执行 CTest 6/6 和 3proc 59 行全矩阵，exit 0。源代码提交 revision 与文档报告提交 revision 分别在提交完成后的工作交付回复中记录；文档不尝试自引用其自身 commit hash。

## 本轮源文件字节 SHA-256

| 文件 | SHA-256 |
|---|---|
| `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h` | `3b358a1971158fc6ee07ba6faeada305de7ada10c3a1420342660dec449d0913` |
| `VFSS/include/moe_topk/experimental_bmw16_select_party.h` | `18948646fc6e84549371915fba608c8b55652dd64f4793683bbc9908a594c45f` |
| `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h` | `6fdfa6eaca917e7779d7b07e9ef9a0cc3d2af8bb56fea34aa2902a907d12bd53` |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | `d6512de4f5042282378d3144ed95915afa5e74f22de6b4fb7372723428e0e68c` |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `10a268c4de24c0dbf396b984e573153c19b696b985d4d7746958710df4dd5b7f` |
| `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp` | `67602f3a79dda4b8b5d8c41a4597a6149d376f641d92145a64540ab250a2813c` |
| `VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp` | `2f935abcda431ffc6724cfa880f2490294b4d503e2b8b03e12d45e21011032d5` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | `109e736bd493968a95d1aa9d6df5008d48a517de8618a4e6cbb7029597c6d118` |
| `experiments/TEST_ONLY_BMW16_S14/capacity_preflight.py` | `831779697ec915d4988774acb6c8c571b4531c671b5912762800d64de43abba4` |
| `experiments/TEST_ONLY_BMW16_S14/run_n128_common_tape.py` | `a108e1e8ace0336b4c6722b4511f4463b85fd0947576ab45b3869ff14e3bd0ee` |
| `experiments/TEST_ONLY_BMW16_S14/run_pilot.py` | `17f2fe4684761e5a4cbf6a14e421f085c5cd582479571da6c4b61d5ce1ef7c83` |
