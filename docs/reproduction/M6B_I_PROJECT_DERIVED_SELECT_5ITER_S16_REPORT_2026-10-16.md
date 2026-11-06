# M6B-I-PROJECT-SELECT-5ITER-S16 接收与联合视图复核报告

日期：2026-10-16  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`  
结论级别：S15 同会话技术复跑；功能复核完成；完整联合单方视图证明仍 OPEN。

## 1. 摘要与执行者关系

S16 对象是 S15 的默认关闭修复快照，不是 S14 的 68 项快照。S15 source manifest 所列 72 项文件哈希及 canonical digest 均在新隔离目录重新校验；S15 外部证据清单中 60 项文件的存在、字节数与 SHA-256 也重新核验。S15 仍未合入 `main`，source base 为 `c3926c68fd14f270faa8b55234311071947fa080`。S14 manifest 的历史 canonical 错误保留为 erratum：旧声明 `a8bd4236…`，S15 重新计算 `3d41295e…`；逐文件 68/68 哈希匹配，不以旧 canonical 值作为源码身份。Paper review 工具对本地 `Papers/FSS基础.pdf` 的 Docling 页对象检查失败，Marker 因内存错误退出；源文件 SHA-256 为 `992639124493D6923292666C0119C22446CE4C122BE0D4EF38A57A9840343818`，未修改。定理定位以 Microsoft Research 官方 PDF 和既有 page-grounded bridge 为准。

本报告作者参与过 S15 默认关闭修复和 S14/S15 DCF 证明草稿，不能充任用户要求的独立接收者或证明独立审查者。因此 `S15_INDEPENDENT_RECEIPT=NOT_INDEPENDENT`，S15 条件证明的独立确认也为 `NOT_RUN`。以下构建、测试、C++ 数据流和论文定理映射均是隔离的同会话复核证据，不能冒充异会话接收。

S16 使用新 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s16\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-project-select5-s16`，起始 `HEAD=c3926c68fd14f270faa8b55234311071947fa080`。桌面主 checkout 未修改。S16 未更改运行时代码；本轮工作树只新增/更新治理和证据文档。`VFSS-baseline/` 与 S14/S15 树逐文件 131 项一致，没有 S16 改动。

## 2. 源码身份与构建 provenance

S15 快照：

- 源清单：`docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_SOURCE_MANIFEST_2026-10-15.json`
- manifest SHA-256：`4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816`
- 72 项 source canonical SHA-256：`c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`
- S15 delta 相对 S14：CMake/API 边界改动 4 个既有源码文件；其余为 S14 继承文件与历史材料。S15 source manifest 的完整 72 项哈希原样登记在 S16 manifest 中；S16 文档更新单列，不改写 S15 清单。
- S15 另有 60 项外部 build/evidence artifact，S16 复算文件长度及哈希全部一致。S14 旧 canonical 错误只作为历史记录。

S16 构建环境：WSL Ubuntu 20.04.6、GCC/G++ 9.4.0、OpenSSL 1.1.1f；使用 `/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake` 与同目录 CTest 3.31.6。系统默认 CMake 是 3.16.3，不能与实际构建工具混记。ON/OFF 均为 Release、`BUILD_TESTING=ON`、`MOE_TOPK_ENABLE_EMP_OT=OFF`。

feature-ON 命令：

```sh
cmake -S <S16>/VFSS -B <evidence>/on/build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build <evidence>/on/build --target moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test moe_topk_project_derived_select5_conformance_test moe_topk_project_derived_select5_material_store_test moe_topk_project_derived_select5_process_e2e_test moe_topk_streamed_material_slot_store_test moe_topk_tls13_transport_conformance_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test -j2
ctest --test-dir <evidence>/on/build -R "moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test|moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_project_derived_select5_process_e2e_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test" --output-on-failure
```

feature-OFF 使用相同选项、将实验选项设为 `OFF`；构建 `sytorch`、`moe_topk_m2_ucmp_conformance_test` 与 `moe_topk_m2_parallel_shuffle_conformance_test`，运行两项旧回归。实际的 build directory 和日志在外部 evidence 路径 `C:\Users\28641\.codex\evidence\M6B_I_S16_REVIEW_20261015_01`。fresh S16 二进制与 S15 二进制哈希不同，不可互换：S16/S15 的静态库同为 2,606,006 B；逐字节归一化检查发现 8 个差异只来自 ELF 对象中嵌入的 S15/S16 绝对 source path。六个大小相同的 ON 测试二进制各有 24 个差异字节：4 处嵌入 source path 字符变化和 20-byte GNU build-id 随内容变化。S16 结论绑定 S16 自己的 binary hash，不借用 S15 binary 结果；S14→S15 旧报告记录的其他 binary 差异仍按其原有限定，不声称二进制互换。

## 3. Fresh 功能和构建边界复核

### ON 构建与 CTest

Release ON 目标全部构建。六个注册 CTest 结果：DCF conformance、Select5 control、Select5 material store、legacy M2 uCMP、legacy parallel shuffle 为 PASS；三进程 process E2E 在普通 UID 下按测试约定 `SKIPPED`（需要 trusted supervisor 建立 P0/P1 不同 OS UID）。这不是通过证据，也不是逻辑失败。随后以 root supervisor 直接运行 E2E modes，覆盖了对应 T/P0/P1 场景；root 只用于 supervisor，不作为 party 隔离证据。CTest 汇总显示 6 项中 5 PASS、1 SKIP、0 FAIL。streamed-store 和 TLS 目标未注册在 CTest，按 S15 约定直接运行，二者 PASS。

### ON T/P0/P1 E2E

- `--bcg21-smoke`：12 个样例全部 `ORACLE_PASS`，覆盖 `(n,K)=(1,1),(2,1),(2,2),(128,1/64/128),(256,1/2/8/128/255/256)`；N=128/256 使用 streamed schema。
- `--bcg21-monolithic-n128`：case 301，N=128、K=64，`M6B5/v3 + M2CM/v1 + M6P2/v1`，`ORACLE_PASS`。
- `--bcg21-faults`：6/6 `UNIFORM_ABORT_NO_MASK`，覆盖 tampered package、disconnect、错 party/K、TLS peer 身份错误、T 在 ready 前退出、非法 seed magic。
- 资源：N=256 monolithic 在材料生成前拒绝；N=257 streamed 预检拒绝。
- 额外 `--crash-step 1..6`：6/6 报 `NO_COMPLETE_MASK`。step 1–4 两方 output bytes 均为 0；step 5–6 P0 崩溃、P1 可留下 8-byte 局部 share、P0 为 0 bytes，仍没有完整 mask。
- 成功输出由 E2E TEST_ONLY supervisor 重构 shares，检查 signed Q20.12 降序、同分 original_index 升序、原始顺序、长度、位值和恰 K 位，并与冻结 C++ stable oracle 逐位一致。party 进程本身只产生其局部 share；明文重构仅发生在 TEST_ONLY supervisor。

`S16_E2E_CASES` JSON 保留 13 成功、6 abort 及 6 crash prefix 的机器可读记录；成功案例中的输入 seed 是测试 harness 的输入-share seed，不是公开 sample seed。sample seed 字节本身不写入逐案证据。

### OFF/ON API 检查

S15 将 `dcf_bcg21_fig1_aes128_v1.cpp`、编译宏和 BCG uCMP API 均置于 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL` 条件下；公共 BCG 头在无 opt-in 宏时 `#error`。S16 fresh OFF Release 的 `libsytorch.a` 为 2,122,218 B，BCG `nm` grep 结果为空；CMake target help 不注册 BCG/Select5 实验 targets；OFF header client 按约定编译失败；M2 uCMP、parallel shuffle 两项 legacy 测试 2/2 PASS。ON 下外部 probe 对 BCG keygen、eval、serialize、deserialize 编译、链接并运行成功。新 DCF API 已从默认 OFF 目标排除；这是构建边界，不是 secure 安全结论。

保留的失败/操作尝试：非 root CTest E2E 返回 skip 77 后由 root supervisor 运行；第一次 shell 变量穿过 PowerShell/WSL 边界失败，没有生成输出，改用绝对路径；一次比较 S15/S16 二进制的 PowerShell 引号解析失败，之后以参数数组重跑成功。没有运行无关全仓测试，也没有覆盖失败日志。 两次最终校验脚本格式/JSON-root假设失误也追加至 attempts-and-failures.log；均发生在脚本解析/取值阶段，未运行仓库代码或改写原始测试证据。

## 4. DCF 与 UCMP 的条件证明复核

### 实际 ABI 映射

S16 直接审阅 `VFSS/src/moe_topk/dcf_bcg21_fig1_aes128_v1.cpp`、`protocol_i_ucmp.cpp` 及 S14 bridge。DCF ABI 是 `DCF_BCG21_FIG1_AES128_V1 / M2B1/v1`，旧 `M2UC/v1` 分支和材料不变。输入宽度在路径中为 34–53；UCMP 在 `Z/(2^w)` 上使用 `alpha=(mask_left-mask_right) mod 2^w`、`beta=1`，group 为 `Z/(2^64)`。根 seed 为 128 bit；Figure 1 的 `G` 按 5 个 AES-128 固定域分离 block 得到 514 个 MSB-first 位；终端 `Convert_G` 取首 64 个 seed 位并按大端形成 `Z/(2^64)` 元素。序列化为规范、可逆的 party-key encoding；公开 ABI/count header 和保留控制位是固定后处理，不声称 wire key 是均匀随机字节串。

单方 UCMP 实际对公开/局部合成的 `x=(z_left-z_right) mod 2^w` 与 `y=(x-2^(w-1)) mod 2^w` 各做一次本地 `eval_share`。随后 `ge=t2-t1`，party 1 按源码执行环绕修正；P0 发送 `1-ge`，P1 发送 `-ge`。双方交换 64-bit 本地结果 share，求和必须是 0 或 1 的公开 comparator bit。每个 DCF key 绑定一次调用、party、参数与 slot；α 不序列化给 party。

### 定理边界与符号

Boyle et al. EUROCRYPT 2021 Figure 1、Theorem 2 和 Appendix F.1 的适用结论是固定函数参数下一个 party 的 semantic DCF key privacy；作者公开 PDF：[Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)，Theorem 2 约 printed p.13，Appendix F.1 约 printed pp.42–45。精确构造映射与版本 ABI 在 S14 bridge 中记录。

令安全参数 `lambda=128`、比较输入宽 `w∈[34,53]`、输出群宽 `ell=64≤lambda`，单 key 的条件界写为：

`delta_w(A,t) <= w * epsilon_G(A,t) + epsilon_RNG_joint(A,t; X_slot)`，

`epsilon_G(A,t) <= Adv_AES128^PRP(A,t,q=5) + 10/2^128`。

这里 `epsilon_RNG_joint` 是外部计算假设，不是统计独立、无条件均匀或已量化安全位数：给定指定平台、请求顺序/长度、schema 与该 slot 的 party-visible mask-share 辅助视图，一个时间/资源为 `t` 的 PPT distinguisher 区分实际 OpenSSL `RAND_bytes` 与 Linux `getrandom` 的联合输出、和理想相互独立均匀位串的优势。S15/S14 单 slot/静态 pool 公式只能用于其明确上下文；若地址由逐轮公开 seed 自适应选择，则该上下文还必须包含在线 seed 的 OS CSPRNG 输出与完整先前泄露前缀。该项尚未由平台证书或数值实例化，pool 结论只能是条件式。

信任假设为离线 T 完全可信、输入无关地生成材料后退出且不与在线方合谋；P0/P1 至多一方半诚实腐化，在线应用通道 TLS 1.3 双向认证。主动腐化、T+party 合谋、恶意管理员回滚及生产证书运维不在范围内。没有把 `RAND_bytes` 与 `getrandom` 视为统计独立。

### UCMP 条件模拟及先前推导的限缩

对一个已给定调用的腐化方，`z_left/z_right`、边/slot 地址、打开 bit `c` 均作为批准 leakage `L`；腐化方知道自己的本地 masked operand 与 mask share，另一端对应 frame 也属于允许公开的 masked values。固定 `(alpha,beta=1)` 的 key-only hybrid 把本地 key 替换为 theorem 的 semantic candidate key；Eval 是 `(key,z)` 的确定性后处理，故两次 local Eval share 可由候选 key 计算。模拟器将该 party 的实际本地 64-bit share 写入其发送帧，并用 ideal `c` 构造 peer share `c-r_b`；正确性保证两份之和等于 `c`。模拟器仅需腐化方该 invocation 的自有 operand shares、理想输出/公开比较 bit和 `L`，不取 α、阈值、对方输入或真实 transcript。该论证只覆盖一个已定义输入的 invocation，不覆盖如何从 raw input 与 shuffle state 得到 invocation 输入，也未由独立审查者确认。

mask 生成 `share_mask(full)` 先取 `share0` 均匀，再令 `share1=full-share0`。在理想独立 Dealer 随机带下，每方的本地 additive mask shares 与完整差值 α 独立；S14 bridge 对这个 key-only条件使用已有代数论证。真实 CSPRNG 情形仍通过 `epsilon_RNG_joint` 假设而非代码本身推出。S15 的单次调用推导若不附上“local shares、L、fixed α 下的根随机源联合条件”不得单独拿来证明输入端到端安全。

对所有预先生成 DCF slots 的静态 key-only 投影：

| schema | DCF slots / 宽度 | `sum(w_s)` | key-only pool 项 |
| --- | --- | ---: | --- |
| N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` | 41,024：40,768×40 bit + 256×34 bit | 1,639,424 | `1,639,424*epsilon_G + epsilon_RNG_joint(A,t;X_N128)` |
| N=256 `M6S2/v2` | 163,968：163,456×41 bit + 512×34 bit | 6,719,104 | `6,719,104*epsilon_G + epsilon_RNG_joint(A,t;X_N256)` |

前者 44,985,596 B/party monolithic package；后者 192,644,416 B/party streamed store、12,656,192 B/party manifest、11 segments。上述是容量观测，不是性能数据。正常成功每个选择比较调用消耗一个 Select DCF slot（slot 内两次 Eval）；此外 score conversion 2N 个 DCF slots 和 membership N 个 DCF slots。N=128 case301 有 3,393 次 Select 比较，另有 384 个 score/membership DCF 调用，因此 3,777 个 DCF slots 被调用；41,024 总槽中 37,247 个 Select slots 未使用。N=256 case110 K=128 有 13,485 次 Select 比较，另有 768 个 score/membership DCF 调用，因此 14,253 个总槽被调用；163,968 总槽中 149,715 个 Select slots 未使用。该分项与逐案 JSON 的 `dcf_slot_count`/`unused_dcf_slots` 一致。未使用 slots 仍在各 party package/store 中；失败时已消费数没有逐 fault 计量，记 `NOT_MEASURED`。该 hybrid 仍未同时模拟 shuffle metadata、完整网络 transcript、输出和崩溃状态。

## 5. 泄露字段、消息时序与输出合同

### 获准且实际出现的 peer fields

| 字段 | 来源/观察方 | 状态 |
| --- | --- | --- |
| package binding、session/fingerprint、N/K/width/ABI、phase、slot/purpose/方向/端点 | P0/P1 私有 TLS 应用帧 | 绑定参数与已批准地址/计数；无原始下标映射 |
| 两轮 shuffle record frames `Y_b=σ_b(X_b)+a_b`、`Q_b=S_b+r_b` 与公开 masked aggregate | 对端接收；两方都能复算 public masked list | 属于已批准 masked values；不等于暴露明文分数或 Pi |
| 当轮公开 32-byte sample seed、shuffle 坐标样本/端点/pivot、opened comparison bit | P0 发送 seed，P1 ACK；当前 active iteration | 已批准字段，seed 不在 package；只在 forward shuffle 完成及本轮激活后发送 |
| UCMP 两个本地 masked operands 与每方 64-bit Eval result share | 双向交换；两方求和得到 bit | mask operand 与 comparator bit 属于批准 L；本地 Eval/key shares 不由此公开给 peer |
| membership masked operands；inverse shuffle masked records | 双方私有 TLS 收发 | masked values；membership output share 不直接打开 |
| PREPARE/COMMIT 常数标记或统一 ABORT/no-mask | P0/P1 | 发送仅表示固定协议阶段成功/失败，不携带阈值、rank 或 mask；消息长度及一致终态可见 |
| M2CM/M6P2/M6S2 slot manifest、lease、segments、`σ/τ/a/e/r`、key 与 mask shares | owning party local filesystem/process；T 在生成期看双方完整材料与 Pi | peer 不可直接读；不得把 T+party 或恶意管理员 view 纳入单方模型 |

静态源码与刚才 E2E 运行都核对 seed：forward shuffle phase 2 完成 trace 先于任何 seed frame；phase 为 `40+iteration`，P0 为当前 active iteration 生成 seed，P1 验 magic/version/iteration 并 ACK；`ProjectDerivedSelect5DisclosureState` 禁止 shuffle 前或越序启动。E2E harness 的实际 wire-event trace 检查通过。后续 seed 不在 package/manifest/binding/start metadata；激活前未生成。S16 E2E 日志不会写出 seed 的原始字节。

本轮没有发现超出获准 L 的新 peer-visible 明文字段。masked record frame 的 endpoint 和数值属于获批 masked values；TLS 外部观察者只可见网络元数据。本条是字段范围复核，不表示 L 已有 simulator。

### 真实 shuffle 的未闭合联合分布

M2CM/v1 metadata 生成的精确关系为：

`tau0 = Pi ∘ sigma1^{-1}`, `tau1 = Pi ∘ sigma0^{-1}`；

`e0 = -tau0(a1)-h`, `e1 = -tau1(a0)+h`；

`Y_b = sigma_b(X_b)+a_b`, `S_0=tau0(Y_1)+e0`, `S_1=tau1(Y_0)+e1`；

`Q_b=S_b+r_b`, `Z=Q_0+Q_1=Pi(X_0+X_1)+r_0+r_1`。

单方 local marginals（均匀置换、uniform pads）看似隐藏 Pi；但本次没有给出/复核一个有效概率命题，证明 `P_b` 所见 `(sigma_b,tau_b,a_b,e_b,r_b, Y_peer,Q_peer,S_b,Z)` 的**联合**分布可由其 raw input share、理想输出 share 和 L 生成，尤其 `e_b`、peer `Y` 与 peer `Q` 共用的 `a_peer/h` 相关项。由比较 bit 触发的后续 adaptive endpoints、seed transcript 和材料地址必须在同一 simulator 中一致。`key-only` FSS 混合不处理这些数组。

逆路由再次使用相同结构的 party-local permutation/mask state；membership secret share 写入 carrier，最终本地输出是 inverse-route local share 的 LSB。PREPARE/COMMIT 前任一 crash 都可以留 orphan share；crash-step 5/6 已观察 P1 8-byte 输出、P0 0 byte。既有合同只保证测试控制器不会拿到双方完整 mask；不证明 orphan share 在与该方全 view 联合时与理想 output share 可模拟，不声称原子发布。此处是首个端到端证明断点，不是观察到的攻击。

## 6. 门禁裁决

| 门 | 状态 | 证据/范围 |
| --- | --- | --- |
| `S15_INDEPENDENT_RECEIPT` | `NOT_INDEPENDENT` | 作者参与 S15 修复；72 项 hash 重验和 fresh replay 只算同会话技术复跑 |
| `DEFAULT_OFF_API_BOUNDARY` | `PASS`（技术复核） | fresh OFF 无 BCG symbol、API probe 编译拒绝、无 target；旧 M2 回归 2/2；ON API probe 运行成功 |
| `DCF_SINGLE_KEY_PRIVACY` | `CONDITIONAL` | BCG+21 Theorem 2 固定参数 key-only；AES PRP + 明确但未量化的 joint RNG 假设 |
| `UCMP_SINGLE_CALL_VIEW` | `CONDITIONAL` | 仅 invocation shares、L 中 masked operands/opened bit、单 key/两次 eval；不覆盖 raw-input-to-shuffle |
| `POOL_AND_ADAPTIVE_ACCESS` | `CONDITIONAL` | 两 schema 数量界只对 key/Eval projection；在线 seed 与 Dealer 全部随机带的 joint term须包含在假设上下文中 |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | 缺 M2CM/v1 forward/inverse arrays、消息 frames、membership/output share 与 L 的联合 P0/P1 simulator |
| `CRASH_ABORT_OUTPUT_VIEW` | `CONDITIONAL`（功能合同）/ `OPEN`（隐私模拟） | 6 crash-prefix 观测 `NO_COMPLETE_MASK`；步骤 5/6 存在 orphan share，未证明其联合模拟 |
| `N≤256_FUNCTIONAL_E2E` | `PASS`（同会话复跑） | 13 oracle + 6 abort + N128 monolithic + 6 crash steps；N256 streamed 边界 K 覆盖 |
| `N>256_ADMISSION` | `NO_GO` | N257 preflight reject；没有更大规模材料/功能证据 |
| `SECURE_ALIAS_READY` | `NO_GO` | full joint view simulator 未闭合；保持 EXPERIMENTAL |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | 严格沿用现有 V3→V4 前置；正式 LAN/WAN 未运行 |

最终功能口径：signed Q20.12、降序分数、original_index 升序稳定 tie、原序且恰 K 位 mask 在列明的 N≤256 测试范围通过冻结 oracle。它是项目派生功能验证，不是 BB90/BMW16 复现，也不证明 secure/conditional-security protocol。

## 7. 复跑命令、失败保留与证据

Evidence 根目录：`C:\Users\28641\.codex\evidence\M6B_I_S16_REVIEW_20261015_01`。主日志与哈希、所有 fresh binary、CMakeCache、ON/OFF probe、target help、symbol scan 在随附 S16 source/build manifest 中。测试矩阵 JSON：`M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_E2E_CASES_2026-10-16.json`。

从仓库对应 WSL worktree 重建的命令：

```powershell
wsl.exe -d Ubuntu-20.04 -- /home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-select5-s16/MoE_Top-k_Reproduction_Evaluation/VFSS -B /mnt/c/Users/28641/.codex/evidence/M6B_I_S16_REVIEW_20261015_01/on/build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
```

完整目标列表和 E2E 命令见 S16 manifest；需 root trusted supervisor 时使用 `wsl.exe -u root -d Ubuntu-20.04 -- <binary> --bcg21-smoke`、`--bcg21-monolithic-n128`、`--bcg21-faults`、`--crash-step 1..6`。没有复用/删除 S15 evidence，S16 的新日志独立存放。`git diff --check` 与冻结基线路径检查在 S16 tree 执行；正式性能没有启动。

## 8. 改动文件

S16 runtime 变更：无。

S16 本轮仅新增/更新以下文档：

- 新增 `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_DECISION_2026-10-16.md`
- 新增 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_REPORT_2026-10-16.md`
- 新增 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_E2E_CASES_2026-10-16.json`
- 新增 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_GATES_2026-10-16.json`
- 新增 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S16_SOURCE_MANIFEST_2026-10-16.json`
- 更新 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/REFERENCE_MANIFEST.md`

历史 S14/S15 报告、case JSON、manifest、失败证据与 `VFSS-baseline/` 均未修改。工作仍未提交，也未合入 main。