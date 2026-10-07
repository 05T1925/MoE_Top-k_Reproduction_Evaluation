# BMW16 S18 external evidence index

Created 2026-10-07. Raw tapes, traces, and run logs remain outside Git under `C:\Users\28641\.codex\artifacts\BMW16_S18_20261007`; this file records identities only. Hash algorithm: SHA-256. No keys, party bundles, or generated sidecars are stored in that artifact root.

## Source tree and executable identity

| Repository path | SHA-256 of final S18 working-tree file |
|---|---|
| `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h` | `6387de505500aaf8b99d92a7941b68f9a05f9fe6dad096fe51493157b0658778` |
| `VFSS/include/moe_topk/experimental_bmw16_select_party.h` | `3d0050a9a730c54a4252811dae30cde8963e12e24a4450496ab309ea8af00eac` |
| `VFSS/include/moe_topk/experimental_bmw16_stream_store.h` | `66990db49ff1f01cd5d37dafdefc32fa7124006f85045595dd1d58283c29101a` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `ccb0820fb9b96375bd8e7c04bebc6abe00944b0fb537b75119f30af82700a67e` |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | `0b064bd0ed6c38855d980d4a24edd85a2e9c360adef6aab0929be88016c05520` |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `9ba17231af92f1aae92a2e10ba695823553026fda36a49c09df853dedbe8df3a` |
| `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` | `fcfa6edfd8c15ac18fc29d9313130a40aff5fadb82311a7741336267a163a1b3` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `6de1d35aa0e22c6ec614e105258741b52dee9d1c09424259788e6b8d1c161aa7` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | `5170fc7ad9db91204002542e4a78ea99c0886a7efc5a79d0f7bf88aed735c80e` |
| `experiments/TEST_ONLY_BMW16_S18/capacity_preflight.py` | `366ce42d4caa114c256b462285ed75e65ae8f808cf6d7f10a56cc0a01c9e60b9` |
| `docs/decisions/BMW16_S18_STREAMED_KEYGEN_PARTY_READ_AND_SECURITY_REVIEW_2026-10-07.md` | `0b404aefdf387808da764eefbbfc497ba668c8cec0f4c9f01c278dc290861cc6` |
| `docs/reproduction/BMW16_S18_STREAMED_KEYGEN_E2E_AND_SECURITY_REVIEW_2026-10-07.md` | `9f4df72bef405a4f03c354ea2140de5f489e7d1f1ee949220db57145ecdcf8c4` |
| `docs/IMPLEMENTATION_PLAN.md` | `b9741038506fd6698710530c505a9a5fbf669e561b50a5c4603e27d477dcaeb5` |

| External executable | bytes | SHA-256 |
|---|---:|---|
| `/tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node` | 648744 | `0b10be6c2b33a14db323f219f932e266e34797b4bdfa3b26357b8e07d95d1b7a` |
| `/tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_test` | 697648 | `8d9cea4051db4f3d1870eab278a1c71fb6f00bbbb34767c35fa374218781196e` |
| `/tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node_e2e_test` | 73624 | `cdaba0fa377e23de79cb45d87f8dd4c9940552335a306585131a9f4d93b753d3` |
| `/tmp/bmw16-s18-build/moe_topk_bmw16_experimental_3proc_test` | 715016 | `98f9fc195125b38a56c8f58337fbfd882c6eaea04d90e9e64bb8953bb4adc975` |

## External raw evidence files

| Path relative to artifact root | bytes | SHA-256 |
|---|---:|---|
| `capacity_n1000.json` | 849 | `1618a49789159e76e37c72a01d789b94abe4ee3ec7a81dc21a9be5d55cee0702` |
| `capacity_n10000.json` | 845 | `4f9e9ba35c3a093e2c03ddaeb78018d3150ff8fbd76ea1e51215cbc8f922ede0` |
| `capacity_n100000.json` | 857 | `a6c6fd3c2cfbb096bbd1c2561bbacf4f83a92f00cdb52c3e9d01d54b85f6eec0` |
| `capacity_n1000000.json` | 869 | `6cd667876d5ae280f1816892e91061d6b93122ade88ac2a32dc0a83be6bb536f` |
| `final_party_test_build.log` | 532 | `1b61bed3fa764f806c6c8864276747c00f04111a6f6ee61efaa23baf3daa5a1d` |
| `final_relevant_regression_ctest.log` | 2110 | `eb973aa26204e310de57daaeeecc28ac9793341608e6fc52365d8f9251d93204` |
| `final_targeted_build.log` | 866 | `48bf53ccae17eb7451b0470de89e692f942491af333ca55e7def8f069d0d7f5f` |
| `frozen_s4_source/__pycache__/select4r.cpython-313.pyc` | 43390 | `a82193879995c8f91dd195cf78be5f5f332a9b2c5651c377adb8d8261f323d8b` |
| `frozen_s4_source/select4r.py` | 30223 | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| `full_ctest_registered_suite.log` | 67477 | `9488a79f21b8cf1028112e39117667b84418e871731fb2e8474942068f32519f` |
| `node_e2e_basic_final.log` | 3054 | `e46d63b97ab5fa1b071806c646da21a9bd6a1777fee4089c367329b664d5390e` |
| `node_e2e_basic_rerun.log` | 1239 | `845ac479b103a57bb2e7d4800c05a9941e74b5b91ad6e2e197100aeafbbf4d16` |
| `node_e2e_basic.log` | 2476 | `beb9460c70b2856b02277f3cd46779cd6e4f8814b87d37509d8c6825316c32aa` |
| `node_e2e_extended_chunkfix.log` | 9859 | `42c97ebc11c8189e398724b23595d25ab153b2d742868c82dca5954bb95d115c` |
| `node_e2e_extended_final.log` | 9852 | `cb68d3ed55739029c7b209624bc896a401d612c7c769ad336dbedc7376fd3c63` |
| `node_e2e_extended_releasecheck.log` | 9850 | `2e869b5c8fee047f6c08358833d2e6dd2faf6150460ffd6a44cd5f9c2f25d8bc` |
| `node_e2e_extended.log` | 10046 | `eee0f79fd5d25f20d25ddefb7631ceb5c856785ff8accd23dd6baa891e4653b7` |
| `node_e2e_n1000_chunkfix.log` | 4706 | `ef53f7315e428d6eb5becf2200e140b4cc2a56b6c5e1202e3a07014c120d5bbe` |
| `node_e2e_n1000.log` | 4485 | `e67ad322c9ee16cad695ce29d6ed74cc5db453fc42f68a69d7ae39ea82d9fbcc` |
| `party_test_concurrent_claim_final.log` | 8534 | `65b329cd72d1cb4e54c83694c0985100f4ddcb82bcc4e2149ad8e4c2e9a543ab` |
| `party_test_final.log` | 8306 | `fb5d62ec5985af9fcee343d8d23b6403a4fef0d011d4719c31656d49000db710` |
| `party_test_rerun.log` | 466 | `af2ae312eed1714f61685c2982d88e26385147b3a7918f28ea7a6f0af3c1e8d1` |
| `party_test_restart_claim.log` | 305 | `1be4205ba1b3ab533a008c7cf1fe3b88287f420f5311f90c3be459b1b910cdfd` |
| `party_test.log` | 697 | `0805c2f3b155a12fe7228be5a96a3e55bc1448e5c5fa675bb61de26776552c17` |
| `regression_ctest.log` | 1927 | `229f6d03175d83a4d9031d72f69bd00364d54bcc6705428fb1e5647f39ec76cb` |
| `s4_copy_identity_attempt.txt` | 667 | `3a76f9f7789546bb789516c1684254025b51a9f5175be71348533156f53201cd` |
| `s4_diff_n128_final/matrix.json` | 1629 | `a064c09ba5c26350c92beccef3c3364ac727ce2166217a23ebc310637836581f` |
| `s4_diff_n128_final/n128_k2/common_tape.json` | 84030 | `f6b1e8f7dbbbfb1083b1d0ddd234aedb232e6212e5988e0619c27b232a972fd4` |
| `s4_diff_n128_final/n128_k2/common_tape.txt` | 83587 | `ea89f4f85a82fb3efe9671dfb662ba611941b056e0407fc08ca5917fe1ed3d65` |
| `s4_diff_n128_final/n128_k2/cpp_run.txt` | 856 | `ade51e14cceb5d7f731a7e3291f3d96b6f5fea45c5f4037934a98c105b6a0050` |
| `s4_diff_n128_final/n128_k2/cpp_trace.txt` | 360540 | `7a2fe55b001890d1e0ba0464bec31ed2d1aa41afdefff317d2488d4562671c23` |
| `s4_diff_n128_final/n128_k2/differential.json` | 965 | `092742a78cba1e46b1a15a612bb1d37823e8e145aa57c6b9bfc5ac5e0e2316f1` |
| `s4_diff_n128_final/n128_k8/common_tape.json` | 83968 | `b57cc52c87552d0ce1b36fbf8c6070c66eb88d40d0f7355c1170292620680ec9` |
| `s4_diff_n128_final/n128_k8/common_tape.txt` | 83626 | `530066cbffd65ab5ff248a7f28fd46d800eab961f65201dd1dd5fa45f85bf6bb` |
| `s4_diff_n128_final/n128_k8/cpp_run.txt` | 855 | `32ade78529fc489fb872426e7a9205daecb8925f3a096ec24d5c9079c7bd9a44` |
| `s4_diff_n128_final/n128_k8/cpp_trace.txt` | 344808 | `7bc2be54b3f6a5861311fa604aedddb37f3f614e9d038e2d8b9b5e6813f8d84e` |
| `s4_diff_n128_final/n128_k8/differential.json` | 964 | `0089392e6d458eb752c6ccc69f4f32bab1b5859a37361b7160878c12d1904dc9` |
| `s4_diff_n128_final/n128_k80/common_tape.json` | 84118 | `751953ef08d7851ecebbfb228b8f632519e55b64c4d43129d30ee2134255cfc9` |
| `s4_diff_n128_final/n128_k80/common_tape.txt` | 83566 | `1a61ee759a4af76fdec048387ff9559b2efb97d861af73c1f8b31055156c67b0` |
| `s4_diff_n128_final/n128_k80/cpp_run.txt` | 854 | `73d6ea4c3f602f935dfc97729c3ee9f81677a64a9225487b196c0323cfc60da5` |
| `s4_diff_n128_final/n128_k80/cpp_trace.txt` | 193683 | `ced25e99f521937a6fa5e1a6e58dc9d013750e6147d39ba445233d17820e74d7` |
| `s4_diff_n128_final/n128_k80/differential.json` | 964 | `44bf0a3904b0e651097beb6c3ce7356e9936d8f223cf332f23c41621443c7c11` |
| `s4_diff_n128_frozen/matrix.json` | 1629 | `05184c50c31ca8444632c2f007214bad05418435c20bddb1f8d63fb7df621de0` |
| `s4_diff_n128_frozen/n128_k2/common_tape.json` | 84030 | `f6b1e8f7dbbbfb1083b1d0ddd234aedb232e6212e5988e0619c27b232a972fd4` |
| `s4_diff_n128_frozen/n128_k2/common_tape.txt` | 83587 | `ea89f4f85a82fb3efe9671dfb662ba611941b056e0407fc08ca5917fe1ed3d65` |
| `s4_diff_n128_frozen/n128_k2/cpp_run.txt` | 855 | `fe2d6b39852c92278709fd57480bc4faf36a3746fc4ea3a7f6c6daa5ceae9a6f` |
| `s4_diff_n128_frozen/n128_k2/cpp_trace.txt` | 357033 | `5054add0f82676a6fee2b0f219179fed6f2a337749aea19908c2f38d249a5dc5` |
| `s4_diff_n128_frozen/n128_k2/differential.json` | 964 | `833b2f0eb36817221bad923e9bc55310b25bd538ca225677fa729a9f3087a5a2` |
| `s4_diff_n128_frozen/n128_k8/common_tape.json` | 83968 | `b57cc52c87552d0ce1b36fbf8c6070c66eb88d40d0f7355c1170292620680ec9` |
| `s4_diff_n128_frozen/n128_k8/common_tape.txt` | 83626 | `530066cbffd65ab5ff248a7f28fd46d800eab961f65201dd1dd5fa45f85bf6bb` |
| `s4_diff_n128_frozen/n128_k8/cpp_run.txt` | 855 | `2027f026a2fba6b661bbdc57cc1c8443aa3047ce033d878f6ec64da7cff4a878` |
| `s4_diff_n128_frozen/n128_k8/cpp_trace.txt` | 340248 | `7f999c416a20ea778e8fb90ec0a96e3d4f71aff3bd263cc7f6cfb4ab782f0277` |
| `s4_diff_n128_frozen/n128_k8/differential.json` | 964 | `b1104aa7163dd5b597c834b85ae5bb9188c5cd6d12716cf384d9404dbde078a7` |
| `s4_diff_n128_frozen/n128_k80/common_tape.json` | 84118 | `751953ef08d7851ecebbfb228b8f632519e55b64c4d43129d30ee2134255cfc9` |
| `s4_diff_n128_frozen/n128_k80/common_tape.txt` | 83566 | `1a61ee759a4af76fdec048387ff9559b2efb97d861af73c1f8b31055156c67b0` |
| `s4_diff_n128_frozen/n128_k80/cpp_run.txt` | 854 | `503629635abac7252fc67f123cf9fee9abeebefcfcf68d1b6610505140b63948` |
| `s4_diff_n128_frozen/n128_k80/cpp_trace.txt` | 198976 | `7a92e6c305f73bbfdf58e2dc5fa8ca1449dd1cd2e79a3c40c6605ec594222b93` |
| `s4_diff_n128_frozen/n128_k80/differential.json` | 963 | `66a9e94280d7d1f3e2373519f4ffe4dbe235afca46f5549c3fe05cdd0876a0ac` |
| `s4_diff_n128/n128_k2/common_tape.json` | 84030 | `f6b1e8f7dbbbfb1083b1d0ddd234aedb232e6212e5988e0619c27b232a972fd4` |
| `s4_diff_n128/n128_k2/common_tape.txt` | 83587 | `ea89f4f85a82fb3efe9671dfb662ba611941b056e0407fc08ca5917fe1ed3d65` |
| `s4_diff_n128/n128_k2/cpp_run.txt` | 857 | `07d6fc28e21428c2f4385a3ba22f129fd756a38259e5db04f40f29c601b1175c` |
| `s4_diff_n128/n128_k2/cpp_trace.txt` | 374847 | `aa95fdb926e8cbcf3e6562a8491852cfc7088c6677aaf9cfdc78f616504e8f6c` |
| `s4_diff_n256_final/matrix.json` | 1175 | `a7ae70f31dcf3e67ca75f3183b95caed733cf7424545596028b67c46cdd6e43f` |
| `s4_diff_n256_final/n256_k2/common_tape.json` | 335066 | `b8d6d41539e544e3ca9667f1d7c0cd0f2bd1fc5bb9248b6f5fc5b791c2c93edf` |
| `s4_diff_n256_final/n256_k2/common_tape.txt` | 334115 | `14ad5fd95bfe7d9ed3a1e4f96b515a60b3f553993d8ae4520762d80a5ffe3cf3` |
| `s4_diff_n256_final/n256_k2/cpp_run.txt` | 863 | `955a4d1512ec0f46d760344baac3ccfcecd3e8022bdd1d380c6719546cb5c213` |
| `s4_diff_n256_final/n256_k2/cpp_trace.txt` | 822735 | `51a25bacec5f9728099ea34de0b6d7841c9b78bfa76259a116c512f38f1fe12a` |
| `s4_diff_n256_final/n256_k2/differential.json` | 969 | `e8dfd3ca2b7a24f2cbbd34e4343049069bb06564972141ffa2e4c13391a46594` |
| `s4_diff_n256_final/n256_k8/common_tape.json` | 335054 | `2ba10140d8411f5101fd53410a934a0be7d34e1529a6dd5b42afa84f3d0214ad` |
| `s4_diff_n256_final/n256_k8/common_tape.txt` | 334354 | `05ef358fb324a08cd6209f742eabd1b27ec961b1992ecf99fbcf8e21818d3d49` |
| `s4_diff_n256_final/n256_k8/cpp_run.txt` | 862 | `f059b180b1a47fd619964d7a7d8ad051def7956b9ce68e35c46b117564770a09` |
| `s4_diff_n256_final/n256_k8/cpp_trace.txt` | 783194 | `3a5a6b1c18352e82e97b5e0e3e717074ec2358da3b24b6f6a58cacae5ddd199b` |
| `s4_diff_n256_final/n256_k8/differential.json` | 966 | `cc68bdf5980729ddd11b5a40d7cce1daab85007863b7acb81578b70bedf26c93` |
| `s4_diff_n256_frozen/matrix.json` | 1175 | `e2c715444677ffe79c8caa7bb026a1e8f2596e17efcd30ba083532b79413187e` |
| `s4_diff_n256_frozen/n256_k2/common_tape.json` | 335066 | `b8d6d41539e544e3ca9667f1d7c0cd0f2bd1fc5bb9248b6f5fc5b791c2c93edf` |
| `s4_diff_n256_frozen/n256_k2/common_tape.txt` | 334115 | `14ad5fd95bfe7d9ed3a1e4f96b515a60b3f553993d8ae4520762d80a5ffe3cf3` |
| `s4_diff_n256_frozen/n256_k2/cpp_run.txt` | 861 | `af2fd97ff4a21b7149524099b554b40baf9b9b185224381b27962c7899467ac1` |
| `s4_diff_n256_frozen/n256_k2/cpp_trace.txt` | 774615 | `9a1244002c1b314554a9956efd9fec1ce11c764cbd824b753870f0da16add841` |
| `s4_diff_n256_frozen/n256_k2/differential.json` | 967 | `0be6180ec26864909cadd6a93eb8290b21d81e62490bb30a8fe0bccdf4297a80` |
| `s4_diff_n256_frozen/n256_k8/common_tape.json` | 335054 | `2ba10140d8411f5101fd53410a934a0be7d34e1529a6dd5b42afa84f3d0214ad` |
| `s4_diff_n256_frozen/n256_k8/common_tape.txt` | 334354 | `05ef358fb324a08cd6209f742eabd1b27ec961b1992ecf99fbcf8e21818d3d49` |
| `s4_diff_n256_frozen/n256_k8/cpp_run.txt` | 862 | `bacfa9b93fa4a68beec3f598eb194517a66c1fa617c8b6ba0a232cf0a76c3bcc` |
| `s4_diff_n256_frozen/n256_k8/cpp_trace.txt` | 713635 | `8cb8d51be9e64c8cfa66dd5e258b4cf2134a027bb23de9cdd4297f10636fb4b7` |
| `s4_diff_n256_frozen/n256_k8/differential.json` | 967 | `ecd8645c19c199f2d609fc32b555088035d13b67af14af3b54189450761c1a34` |
| `s4_diff_n256/n256_k2/common_tape.json` | 335066 | `b8d6d41539e544e3ca9667f1d7c0cd0f2bd1fc5bb9248b6f5fc5b791c2c93edf` |
| `s4_diff_n256/n256_k2/common_tape.txt` | 334115 | `14ad5fd95bfe7d9ed3a1e4f96b515a60b3f553993d8ae4520762d80a5ffe3cf3` |
| `s4_diff_n256/n256_k2/cpp_run.txt` | 862 | `8cd0d11d0bdb8825757ce7c83778436efff1d11602f51ace8bfebdfd8336e320` |
| `s4_diff_n256/n256_k2/cpp_trace.txt` | 810179 | `27f4912f8c00f69cc93b2e9bfc8f8cb0df515a1cc24642dfcd8a1c60c896015f` |
| `threeproc_rerun.log` | 199875 | `d7df46c844b2642859ccbdb6a037135c8bf0c60d5cd2fb455e7c840669d336f7` |
| `threeproc.log` | 208202 | `a5a74ce3228290fcceb3f9de2bb7147d22b563d9a54b30696c9359b4f3c3ba16` |

The frozen S4 source object hash and final differential matrix hashes are listed in the main S18 report. This index includes intermediate failed runs and superseded debug artifacts as well as final successful runs; they are intentionally retained so corrected failures remain auditable.
