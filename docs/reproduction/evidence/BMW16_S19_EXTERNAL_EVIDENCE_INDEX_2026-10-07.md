# BMW16 S19 外部证据索引

日期：2026-10-07。原始数据目录在 Git 外：`C:\Users\28641\.codex\artifacts\BMW16_S19_20261007`。该目录保存 fixed TEST_ONLY 输入/tape、逐 case trace、E2E 日志和失败诊断；不得把目录里的 party-only包或任何密钥作为提交内容。

## 基线与被测身份

| 对象 | revision/路径 | SHA-256 |
|---|---|---|
| S18 接收源 | Git `9e25411027a01bcf47b9c316e3c463c2f7c72127`；clean detached rebuild tree | — |
| S18 stream writer | `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` at S18 | `fcfa6edfd8c15ac18fc29d9313130a40aff5fadb82311a7741336267a163a1b3` |
| S18 bundle helper | `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` at S18 | `0b064bd0ed6c38855d980d4a24edd85a2e9c360adef6aab0929be88016c05520` |
| S18 Select party | `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` at S18 | `9ba17231af92f1aae92a2e10ba695823553026fda36a49c09df853dedbe8df3a` |
| S4 frozen Python selector | S4 detached worktree `experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py` | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |

### S19 源码 tree 文件哈希

| 路径 | SHA-256 |
|---|---|
| `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h` | `64f664039604dce12dfa50041ed7bbac136deed3f9182d04d3dbf4bf803fd8118` |
| `VFSS/include/moe_topk/experimental_bmw16_stream_store.h` | `8d9da1ae1817345ad4250f964d6f3c0c26e0c5d5f03a7dca4fb2dbd221c86aee` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `740935f66bf69838ad9e555f9d324efe6960b4fb41dc4b49a00ebc46980f92f3` |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | `c34434f4a38f1ed77e6ed91714d233f04e9debb01a11cae4f1b7874dda3547f1` |
| `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` | `f7421af5b3b69df1598656995d00fc2cfa042f6383583d43ee536c3eaf783f5c` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `9d9b360175855809cd0a113b9608ca2b9e500cd13e9e8ed84abdcee02697f28c` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | `d9211eef9ce953d09228d0a3535a53d7b784a3ecc41b888c4bb58b3e25732850` |
| `VFSS/tests/moe_topk/protocol_i_transport_conformance_test.cpp` | `fe68c9de7cc4fa11b50dddd305775d07a97370c428a97ab5c47d52b228b29276` |

## 二进制身份

构建为 WSL2 Ubuntu 24.04 / GCC 13.3.0 / CMake Release，CMake options：`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`、`BUILD_TESTING=ON`、`MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF`、`MOE_TOPK_ENABLE_EMP_OT=OFF`。构建目录 `/tmp/bmw16-s19-build`；clean S18 rebuild 在 `/tmp/bmw16-s18-s19-receive-build`。

| 构建 | 二进制 | SHA-256 |
|---|---|---|
| S19 | `moe_topk_bmw16_experimental_party_node` | `4bb2c560e857c3500646d3b3a7dce1e361a1306b9e898fb204a0e86f1fb45cd6` |
| S19 | `moe_topk_bmw16_experimental_party_node_e2e_test` | `756d047ea0b480fefe11015150b3070e192bccb742a9d88ac50a0318be0ecba4` |
| S19 | `moe_topk_bmw16_experimental_party_test` | `62203bdd9bdf60d964154d0566a4dcba9e28ba6d7456040dcde4937b9302de4e` |
| S19 | `moe_topk_m2_transport_conformance_test` | `02dbc977ac904382cd362c761f8eca17ec98fa4c0fc5a4d05f54e106bce50a28` |
| S18 clean rebuild | `moe_topk_bmw16_experimental_party_node` | `01cca1f1a9c2b977b96d4976860118cc55380bfe18f1ab1b3f975c4c03f1468b` |
| S18 clean rebuild | `moe_topk_bmw16_experimental_party_node_e2e_test` | `cdaba0fa377e23de79cb45d87f8dd4c9940552335a306585131a9f4d93b753d3` |
| S18 clean rebuild | `moe_topk_bmw16_experimental_party_test` | `8b4a04eeee2af02e752659dab466e421f7f8372b386acdeb17a19b2d00d8f548` |
| S18 clean rebuild | `moe_topk_m2_transport_conformance_test` | `106768d8a186d0d4335583ec3b513514444da8c8b7d452ee2989cf8d4c30aff6` |

## 成功结果与哈希

| 外部文件 | 结果范围 | SHA-256 |
|---|---|---|
| `common_tape_s12_final/matrix.json` | S4/C++ common tape，n≤8 全 K，39 条 PASS | `cb4b5412778c9a955e5db009db08317cb10447d9a73b4eab9743b7abebbbba07` |
| `common_tape_n256_final/matrix.json` | n=256，K=2/8，2 条 PASS | `926e1f1b62f953867f5b512440ae82761c757362463390da93f9f8fe99839c34` |
| `node_e2e_extended_final2.log` | 12 SUCCESS oracle checked；3 invalid-ready bilateral material abort；24 publication faults；6 retry checks | `8d4f73fd3a3229d6f70fee62d87fe9ce8a5c826bf5184bd6ed82123d300c4c7d` |
| `node_e2e_n1000_final2.log` | 同基础夹具后 n=1000/K=80 fresh material，双 exit 0 / SUCCESS_ORACLE_CHECKED | `5430ab6285f7a0937a57b5c2d61c2108e28ed8a7c5fa63224a0a2849ef468d3d` |
| `ctest_s19_targeted.log` | S19 party + transport 各 1/1 PASS | `230129db8507bedd8ac98c4d8003c37b108fd9f92ca252f782a257934fe6b1ef` |
| `ctest_s18_replay_pass.log` | clean S18 source party + transport 各 1/1 PASS（root rerun） | `137dca004445e097a37dffcefe8b94669e5d7c5fda09bff74f1f55db67b72caa` |
| `preflight_n1000.json` | 当前磁盘/内存与 checked slot/sidecar preflight；不是实测包长 | `6581172ad376263030bacb06b3f7395892ab95bcb49b0b1493f21d2dcec500b4` |

## 失败和非最终调用

| 外部文件 | 含义 | SHA-256 |
|---|---|---|
| `ctest_s18_targeted.log` | root 下 S18 首次 party CTest 的旧 PID+session 测试目录冲突；不是算法失败 | `48627818bf578de4932f692a27c12b98a303f5c00162b29705bcc94cf5b5359e` |
| `common_tape_s12_linux_runner.log` | WSL Python comparator 缺 NumPy；C++ trace 已生成，未产生最终差分判定 | `5bfe995daa30967387d73189123fbd2b376af2491068d348fac1b9525b4e4780` |
| `failed_cmd.txt` | Windows runner 第一轮参数/WSL exe 解析错误的捕获命令诊断 | `bfbcb671a29e4b6b0254ec2373543df82a2432b7d22d10835476d53cb68786c2` |
| `nonfinal_invocations_and_failures.txt` | 汇总 full-build linker、权限、fixture collision 和 launcher 调用的终端失败 | `7c58b61ea228fd1f76f28b7c30d829ee8be33314ad700c46720df6dfcba41179` |

`run_matrix_with_system_wsl.py` 是只放在上述 artifacts 目录的 TEST_ONLY launcher shim，用来把原 runner 的 `wsl.exe` token 映射至 `C:\Windows\System32\wsl.exe`；SHA-256 `f8e9117663d0d44a5a7a66e89e438d938a074c12b68d2e2a3c8e07ad31495e1e`。它不改 C++ 算法或比较器。

## 复跑命令

在 S19 worktree 根目录：

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s19/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s19-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_EMP_OT=OFF
wsl.exe -d Ubuntu-24.04 -u root -- cmake --build /tmp/bmw16-s19-build --target moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_test moe_topk_m2_transport_conformance_test -j 4
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node --extended
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s19-build/moe_topk_bmw16_experimental_party_node --n1000
```

Common-tape 命令和每个 case 的 tape/trace/differential JSON 在同一 artifacts 子目录中，可按上方 S12/S15 runner 命令重跑。所有输出是 TEST_ONLY，不是 secure protocol 评估。
