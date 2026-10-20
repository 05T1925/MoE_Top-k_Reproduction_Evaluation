# M6B-I-PROJECT-SELECT-5ITER-S15 独立接收、opt-in 修复与联合视图审查报告

日期：2026-10-15  
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 1. 结论摘要

S14 的 68 项来源清单逐文件哈希可复核，但 S14 原报告关于默认 OFF 的结论不成立：feature-OFF 静态库仍包含 `DCF_BCG21_FIG1_AES128_V1` 符号，独立接收者可编写并链接一个直接调用该 API 的客户端。该接收者未参与 S14 DCF 实现或证明撰写，因此本报告将 S14 接收裁决记为 **CONDITIONAL**：来源身份和 feature-ON 功能回放通过；默认 opt-in 边界未通过。

本轮在隔离 worktree 对该问题作了最小修复：BCG DCF 实现源文件和公开编译定义只在 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON` 时进入库；BCG 公共头/API 在 feature-OFF 构建中编译失败；旧 `M2UC/v1` 路径仍可用。新的 OFF/ON 构建、符号扫描、头文件探针和旧路径回归支持 `DEFAULT_OFF_BOUNDARY=PASS`。这项修复后的 S15 源码尚未由非作者接收。

当前实际 BCG DCF 的单 key 和隔离 UCMP invocation 可在明确的 AES-PRP、联合 RNG 伪随机性和静态一次性材料假设下给出**条件模拟**。将全部 shuffle 局部数组、逆路由状态、membership、输出 share 与 PREPARE/COMMIT/ABORT/crash 一并替换为仅由批准泄露和理想输出驱动的完整 P0/P1 simulator，仍缺 shuffle/output 分布引理。因此 `SHUFFLE_OUTPUT_COMPOSITION=OPEN`、`SECURE_ALIAS_READY=NO_GO`；没有发现攻击不构成安全通过。

功能证据绑定到本报告的 S15 源码修复快照和本地构建身份：6 个注册 CTest 目标通过；分段材料库和 TLS 两个直接目标通过；13 个 TEST_ONLY supervisor oracle 成功用例、6 个统一 abort/no-mask 用例通过；N=257 仍在预检阶段拒绝。未运行正式 LAN/WAN 矩阵。

## 2. 固定接收对象与独立性

- S14 原 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s14\MoE_Top-k_Reproduction_Evaluation`
- S15 修复 worktree：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s15\MoE_Top-k_Reproduction_Evaluation`
- S15 分支：`codex/m6b-project-select5-s15`
- 基础 HEAD：`c3926c68fd14f270faa8b55234311071947fa080`；S14 与 S15 均无任务提交，且该 HEAD 与当时 `main/origin/main` 相同。S14/S15 文件快照未合入 main。
- S14 source manifest：68 个选定文件；manifest 文件 SHA-256 为 `033c3799bff9b957f829a85c50d73e4d56c9529a52c02e94993dcac009674c1d`。S14 中 68/68 个逐文件哈希在干净接收 worktree 中匹配。
- S14 接收执行者未参与 S14 DCF 实现或证明撰写；此前参与 S1/S2 研究及 S5/S6/S7 接收。该接收者的运行不冒充 S15 修复版本的独立接收。
- S14 原 manifest 声称 canonical digest `a8bd42361ee0c6c717e624abdf351bd7f1c6a26284df09b68b06bbfd7cfeb422`。按其声明的 UTF-8 路径字节序排序、UTF-8 无 BOM、LF 结尾和小写 SHA-256 重新计算为 `3d41295e9cd0f3adf4a0e0f0f652493996d9cd77bb4a4062eaf557c675aa79bf`。两者不符；逐文件哈希仍一致。S14 文件中的 `classification` 字段实际为 `delta`，旧报告引用的 `s14_modified/s14_added` 集合字段不存在。没有改写 S14 manifest/report；S15 manifest 记录此更正。
- 复核、修复和构建只在上述 S15 隔离 worktree/evidence 路径进行。没有更改桌面主 checkout、`VFSS-baseline/`、论文、证书、密钥或参考工程。

## 3. S14 接收结果及默认边界缺陷

### 3.1 接收复跑

在干净接收 checkout 中 fresh Release 重建 S14 指定的 feature-ON 路径，覆盖 BCG DCF conformance、Select5 control、材料存储、uCMP、parallel shuffle、streamed store、TLS 和 T/P0/P1 E2E。TEST_ONLY supervisor 对成功 mask shares 作 XOR 重构，逐位对照冻结 C++ oracle，并检查原序、长度、0/1 值和 weight=K。S14 13 个 BCG 成功用例和 6 个 fault/no-mask 用例通过；N=128 monolithic case 301 是按源码目标单独补跑的。N=257 preflight 拒绝。

接收者的 fresh 构建为 Ubuntu 24.04 / GCC 13.3 / OpenSSL 3.0.13 / CMake 3.28.3；S14 原构建为 Ubuntu 20.04 / GCC 9.4 / OpenSSL 1.1.1f / CMake 3.31.6。S14 原报告称相对 S12 有 6/8 二进制 hash 不同并归因于嵌入绝对路径；本接收没有逐项证明这一归因。接收者 build 与作者 build 的编译器/依赖版本也不同，因此不声称二进制可互换；接收结论只绑定接收者自己的 fresh binaries 和 68 个逐项哈希文件。S14 首次 Bin=1, alpha=0, x=0 DCF conformance 失败及后续修复记录保留在原 S14 evidence。

### 3.2 实际缺陷与修复

S14 的 CMake 虽门控 Select5 runtime/test 注册，但 BCG DCF 源文件仍在公共静态库无条件编译；公共 BCG 头文件和 uCMP BCG ABI 也没有 feature-OFF 编译保护。接收者构建了一个只调用 BCG `keygen/eval/serialize/deserialize` 的普通客户端，OFF 静态库链接并运行成功。因此“CTest 不注册 Select5 目标”不足以证明 API 被关闭，`DEFAULT_OFF_BOUNDARY` 在 S14 为 FAIL。

S15 最小修复：

1. 将 `dcf_bcg21_fig1_aes128_v1.cpp` 移入既有 Select5 experimental CMake 条件源清单。
2. 仅在 opt-in 条件下向库和调用方传播 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=1`。
3. 在 BCG DCF 公共头文件加入缺少 opt-in 宏时的编译错误；uCMP 头文件中的 BCG include、ABI 枚举值、key 成员和构造接口均按同一宏条件编译，源文件相应实现也被门控。
4. 原 `M2UC/v1` 序列化和计算分支保留；没有把旧材料解释为新 ABI。

修复后：OFF 静态库的 `nm` 扫描没有 BCG `keygen/eval_share/serialize/deserialize` 符号；无宏客户端 include 在 BCG 公共头处失败；OFF target help 无 Select5 目标；OFF 下旧 uCMP 与 parallel-shuffle conformance 通过。ON 下定义由 CMake target 传播，新 DCF 客户端编译、链接并运行成功。最初的 `-lFSS/-lcryptoTools` 手工链接命令缺少依赖库路径，失败后改用 CMake 构建目标实际库路径完成探针；该失败保留在 evidence。

## 4. S15 构建与验证

### 4.1 环境和命令

环境：WSL Ubuntu 20.04.6 LTS、GCC/G++ 9.4.0、OpenSSL 1.1.1f。构建使用该环境中已安装的 CMake/CTest 3.31.6 路径；系统默认 `cmake` 是 3.16.3，不能与 configured tool 混称。

feature-ON：

```sh
cmake -S <S15-worktree>/VFSS -B <evidence>/on/build \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build <evidence>/on/build --target \
  moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test \
  moe_topk_project_derived_select5_conformance_test \
  moe_topk_project_derived_select5_material_store_test \
  moe_topk_project_derived_select5_process_e2e_test \
  moe_topk_streamed_material_slot_store_test \
  moe_topk_tls13_transport_conformance_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test -j2
/home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/ctest -R "moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test|moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_project_derived_select5_process_e2e_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test" --output-on-failure
```

feature-OFF 使用同一 Release / BUILD_TESTING / EMP OT 选项，将 `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF`，构建 `sytorch` 和两个旧 M2 conformance executable。命令全文、编译器、配置缓存和日志哈希记在 S15 source/build manifest 及外部 evidence 目录。
S15 的实际 process E2E 命令为：wsl.exe -u root -d Ubuntu-20.04 -- <evidence>/on/build/moe_topk_project_derived_select5_process_e2e_test --bcg21-smoke；同一可执行文件以 --bcg21-monolithic-n128 和 --bcg21-faults 分别运行 N=128 单包成功行与六个 fault 行。streamed store 和 TLS 直接运行 <evidence>/on/build/moe_topk_streamed_material_slot_store_test 与 <evidence>/on/build/moe_topk_tls13_transport_conformance_test。
OFF 的精确构建、target-help、nm 和头文件编译探针命令及二进制 hash 在 source/build manifest。

### 4.2 测试矩阵

| 类别 | 用例 | S15 结果 |
| --- | --- | --- |
| CTest | BCG DCF conformance；Select5 control；Select5 material store；T/P0/P1 process E2E；旧 M2 uCMP；旧 parallel shuffle | 6/6 PASS，310.12 s |
| 直接目标 | streamed material slot store；TLS 1.3 mTLS transport | 两项 PASS |
| BCG E2E success | case 101–112：N=1、2、128、256；含 K 边界、中间 K、signed Q20.12/重复分数夹具 | 12/12 `ORACLE_PASS` |
| monolithic | case 301：N=128、K=64、M6B5/v3 + M2CM/v1 + M6P2/v1 | 1/1 `ORACLE_PASS` |
| fault/no-mask | 201 篡改材料；202 disconnect；203 错 K；204 错 TLS peer identity；205 T 在 ready 前退出；206 非法 seed magic | 6/6 `UNIFORM_ABORT_NO_MASK` |
| resource gates | N=256 monolithic；N=257 streamed | 均在材料生成前拒绝 |
| OFF visibility | BCG symbols、CMake target help、无宏公共 API client 编译 | 无符号、无 Select5 目标、API 编译失败，均符合 OFF |
| OFF legacy regression | M2 uCMP、M2 parallel shuffle | 2/2 PASS |
| ON API probe | 定义 opt-in 宏、链接 fresh ON archive，BCG keygen/eval/serialize/deserialize | PASS |

successful E2E 的 mask 只在 TEST_ONLY supervisor 控制器重构；对照冻结 C++ oracle 后检查稳定的 Q20.12 降序、original_index 升序 tie-break、原输入位置、mask 长度、二值性和恰 K 个 1。fault 样例没有向控制器发布完整 mask。样本 seed 不写入 E2E JSON。完整逐案 JSON 和原始只追加日志见 S15 case 文件及 manifest 中的证据哈希。

这不是全仓库 clean build 结论。S14 接收报告记录了无关的全构建 `bitpack_test` 未定义符号；本轮只构建/运行上述定向目标。

### 4.3 保留的失败尝试和口径限制

- WSL 下直接读取 Windows worktree 的 `.git` 文件导致路径指向被解释为非法 Linux gitdir；改用 Windows Git/PowerShell 固定 revision 和 status。源码不变。
- 第一次 CTest 从桌面 cwd 用系统 CTest 3.16 执行，报告无测试；随后明确使用 build 内 CTest 3.31.6、以该 build 为 cwd，6/6 通过。之后对未注册的 streamed/TLS 目标使用直接 executable，而不是把“未注册”记成失败。
- 非 root 启动 monolithic party-isolation E2E 返回 skip 77，提示需要 trusted supervisor 创建不同 P0/P1 UID；以 root supervisor 重新运行后 case 301 oracle 通过。root 自身没有作为 party 隔离证据。
- 对 streamed-store/TLS 直接可执行文件使用 CTest 过滤器时返回无匹配测试；它们未注册为 add_test，改为直接运行两个 executable 后通过。
- 手工 ON API 探针首次用 WSL shell 变量拼接路径时被 PowerShell/WSL 参数边界清空，尝试写入 /on/... 失败；使用绝对字面路径的完整 CMake link-order 重试后通过。
- 第一次外部 API link probe 使用短库名但缺 FSS/cryptoTools link search path而失败；改用构建图实际 archive 路径后 ON probe 通过。OFF probe 的预期结果是编译失败，退出码 1，错误定位到 opt-in header guard。
- PowerShell 包装 WSL 子命令时曾错误读取 shell 变量并将编译器状态误报为 0；以直接 `wsl.exe ... g++` 结果复核，OFF probe 确认返回 1。此操作错误没有修改源码。

- S14 独立接收的初次过滤命令把 shell pipe 后的 Select5 未引用，bash 试图执行 Select5 并返回 127；失败后使用单独 CTest/target-help/nm 命令完成检查。此 shell 错误不等于功能测试失败。

## 5. L、UCMP invocation 与 DCF 模拟

### 5.1 模型和获准泄露

信任模型：离线 T 完全可信、生成输入无关材料后退出且不与任一在线方合谋；P0/P1 中至多一方半诚实腐化；在线双方通过 TLS 1.3 双向认证私有通道通信。TLS/证书运维的生产部署未测。T 的 package-generation 视图不属于 P0/P1 单方隐私目标；T+party 合谋、主动恶意方和管理员回滚均排除。

批准 L 仅含按当前轮时序可见的 shuffled-coordinate sample seed、端点、pivot 位置、slot/purpose/address、打开的比较 bit、masked operand、固定轨迹下的调用数/通信长度与双方一致 ABORT/no-mask。seed 仅在 forward shuffle 完成、该迭代激活时发送；后续 seed 尚未生成。原分数、原下标到 shuffled index 的映射、pivot 明文值、阈值、DCF α/key、掩码份额和完整 mask 不在 L。TLS 对外部观察者隐藏应用字段，仅长度、方向、时序等元数据可见。lease/ready/segment 及文件系统事件为 party 本地材料视图或 trusted supervisor 视图，不因存在于本地日志自动成为 peer L；仅实际 frame 中字段进入 peer transcript。

### 5.2 单次真实 uCMP call 的候选 simulator

单次 UCMP ideal call 的 simulator 输入仅为腐化方本次 invocation 的两项 operand shares、理想比较输出 c 和批准泄露 L。这里的 invocation inputs 是 shuffle 后 key shares，不是原始 raw-score shares；把前向 shuffle 的真实 party-local view 接到这些 shares 上，需要单独的 shuffle composition 引理。

1. 从 L 中的 masked-operand frames 和腐化方自己的 invocation shares 重建本地 operand frame及本地 mask-share view；peer operand frames 直接由获准 transcript 字段确定。未消费材料的掩码和 keys 由同一联合 RNG/key hybrid 生成。
2. 对本地 DCF party key，用 BCG+21 Theorem 2 的固定 alpha,beta 语义 key 分布进行伪随机替换。按 L 中公开的 z 计算 x=(z_left-z_right) mod 2^w、y=(x-2^(w-1)) mod 2^w，并以替代 key 做两次 Eval，得到腐化方实际视图中的 64-bit local result share r_b。
   P0 按实际实现返回 1−ge，P1 返回 −ge 并应用 party-1 wrap correction；模拟器按腐化方 b 运行对应分支。
3. 将 r_b 放入腐化方本地发送字段；由获准打开 bit c 计算 peer message share c-r_b mod 2^64。两条 result share 的和因此与理想比较输出严格相符。方向、slot、phase、endpoint 和地址按 L 输出。
4. 对未使用 slot 使用相同 key hybrid 生成其 party-key view。已使用 slot 的后续查询可依当前逐轮公开 seed、端点与先前打开 bit 自适应选择；地址轨迹本身在批准 L 内，hybrid 必须覆盖真实调用次序而不是静态独立查询。

此 simulator 不接收真实 transcript、对方输入 share、alpha 或阈值。结论仅限隔离的单次 UCMP invocation 及其 DCF/operand transcript；不模拟原始 score 转换、shuffle state、membership、逆路由和最终 mask share。
### 5.3 条件与优势

引用 Boyle et al., EUROCRYPT 2021, Figure 1、Theorem 2 和 Appendix F.1；定位及版本链接见[作者机构公开论文 PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)。新 `DCF_BCG21_FIG1_AES128_V1 / M2B1/v1` 按实际 party-key 字段、根 PRG、correction words、control bits、64-bit group 输出和 canonical serialization 对照该定理。确定性 serialization/parser/Eval 作为后处理，不改变 indistinguishability。对固定 `alpha,beta`，位宽为 `w` 的语义 key 优势写为：
`delta_w ≤ w·epsilon_G + epsilon_RNG_joint(T;schema,prefix)`，
其中 `epsilon_G ≤ Adv_AES128^PRP(T,q=5)+10/2^128`；对 `G=Z/(2^64)` 的 64-bit `Convert_G` 截取不存在 S12 旧 ABI 的低位支持集断点。

`epsilon_RNG_joint` 是明确外置而尚未数值实例化的假设：攻击者在给定 schema、可见辅助信息及 party mask-share view 时区分实际 OpenSSL `RAND_bytes` 与 Linux `getrandom` 的有序联合输出和相应相互独立均匀位串的优势。它不是统计独立性或无条件均匀保证。安全结论必须沿用运行平台 CSPRNG/熵源实现、AES-128 PRP、材料 slot 不重用且 session/purpose/party 参数绑定。若该联合 RNG 假设无法由部署环境接受，则结论退回 OPEN。

全池 key-only/static-address hybrid 的 union bound（只针对 DCF key/eval projection，不覆盖 shuffle/output）：

| schema | DCF slots / widths | DCF root bytes | `Σ w·epsilon_G` | 结论范围 |
| --- | --- | ---: | ---: | --- |
| N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` | 41,024；40,768 个 w=40，256 个 w=34 | 1,312,768 | 1,639,424·epsilon_G | 条件 key-pool 投影 |
| N=256 `M6S2/v2` | 163,968；163,456 个 w=41，512 个 w=34 | 5,246,976 | 6,719,104·epsilon_G | 条件 key-pool 投影 |

两行还分别加 `epsilon_RNG_joint(T;M6B5/v3,N128)` 与 `epsilon_RNG_joint(T;M6S2/v2,N256)`。N=128 每方已观察 package 44,985,596 B；N=256 每方 streamed store 192,644,416 B、manifest 12,656,192 B、11 segments。它们是 schema/容量数据，不是性能指标。

**独立审查状态：**未参与 S14 DCF 实现的只读 reviewer 复核了 Theorem 2 到 S14 key schema 的语义映射、单次 UCMP view 候选模拟和上述 key-only 优势式；其审查范围不包含 forward/inverse shuffle 与最终 output 的完整 party transcript。单 key 与单 invocation 的状态为 CONDITIONAL，不是 unconditional theorem assertion，也不升级为 secure protocol。

## 6. 自适应池与 shuffle/output 的边界

- 对 DCF/UCMP 子视图，独立槽根随机性、一次性地址绑定及对每个 slot 逐项 hybrid 可以条件覆盖未使用 key 和前缀比较 bit 驱动的自适应端点/slot 访问；地址序列本身在批准 L 中。该结论依赖两 schema 的联合 RNG 假设，对 N=128、N=256 分别累加上表位宽项。它不把 party-local shuffle state 或输出状态加入被模拟分布。
- 首个缺失的完整视图 hybrid 在 DCF 调用结束后、forward shuffle/record shares 的 party-local view 到理想输入/输出接口替换处：实际 party 持有的置换与逆置换状态、masked record arrays、输入分数 share 的 lane packing、shuffle mask/share 和后续逆路由 membership/output share 具有跨调用关联。BCG Theorem 2 只替换 DCF party key；它既不生成这些数组的联合分布，也不证明其与腐化方原始输入 share、公开 seed/比较 transcript 的联合分布可由 L 和理想输出复现。
- 对 P0 与 P1 的安全结论相同：未发现可复核攻击，但 shuffle/input/output/abort/crash 联合 simulator 尚未成立。需要一个针对实际 shuffle ABI 的独立 permutation/mask simulator 引理，再把 inverse-route、membership、输出 share、PREPARE/COMMIT 和失败/重启状态纳入同一 hybrid；模拟器仅可读取批准 L、腐化方自身输入 share、理想输出 share。
- M6P2/v1 与 M6S2/v2 分别保留各自的 used/unused slot、manifest/segment、lease/restart 账本。N=128 与 N=256 不能合并为一份安全对象。
- S12 测得的合同是 **no-complete-mask**，不是双方原子发布/all-or-nothing。崩溃后某一方可能残留单个 XOR share；完整联合视图证明必须显式模拟 orphan-share、PREPARE/COMMIT、ABORT 和重启状态。

因此：`DCF_SINGLE_KEY_PRIVACY=CONDITIONAL`；`UCMP_SINGLE_CALL_VIEW_SIMULATOR=CONDITIONAL`；`POOL_AND_ADAPTIVE_ACCESS=CONDITIONAL`（仅 DCF/UCMP 投影）；`SHUFFLE_OUTPUT_COMPOSITION=OPEN`；`JOINT_PARTY_VIEW=OPEN`。

## 7. 门禁

| 门 | 裁决 |
| --- | --- |
| S14 精确快照哈希接收 | PASS，68/68 文件；manifest canonical digest 需按本报告更正 |
| S14 独立接收（包含默认 OFF） | CONDITIONAL；ON 功能回放过，OFF API 暴露失败 |
| S15 修复功能回归 | PASS，6 CTest + 2 直接目标 + 13 oracle + 6 no-mask |
| `DEFAULT_OFF_BOUNDARY` | PASS，ON/OFF 链接与头文件边界验证 |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL，AES-PRP 和 joint RNG 假设 |
| `UCMP_SINGLE_CALL_VIEW_SIMULATOR` | CONDITIONAL，限真实 BCG uCMP call transcript |
| `POOL_AND_ADAPTIVE_ACCESS` | CONDITIONAL，仅 DCF/UCMP key/eval projection；完整 party state 未模拟 |
| `SHUFFLE_OUTPUT_COMPOSITION` | OPEN，缺实际 shuffle-to-ideal-view hybrid |
| `N≤256_FUNCTIONAL_E2E` | PASS，N=128/256指定案例、冻结 oracle；N257拒绝 |
| `N>256_ADMISSION` | NO-GO |
| `SECURE_ALIAS_READY` | NO-GO |
| `V3_V4_PERFORMANCE_STATUS` | V3 OPEN；V4 与正式 LAN/WAN NOT_ALLOWED/NOT_RUN |

## 8. 完整改动清单和复跑入口

相对 S14 精确文件快照的 S15 改动为四个运行时代码/构建文件：`VFSS/CMakeLists.txt`、`VFSS/include/moe_topk/dcf_bcg21_fig1_aes128_v1.h`、`VFSS/include/moe_topk/protocol_i_ucmp.h`、`VFSS/src/moe_topk/protocol_i_ucmp.cpp`。另更新 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/REFERENCE_MANIFEST.md`，并新建本报告、S15 decision、gates、case JSON 与 source/build manifest。S14 历史报告、source manifest、case JSON、失败 evidence 未改。

S15 外部证据目录：`C:\Users\28641\.codex\evidence\M6B_I_S15_REVIEW_20261015_01`（失败尝试清单位于该目录的 attempts-and-failures.log）；独立 S14 接收证据：`C:\Users\28641\.codex\evidence\M6B_I_S15_INDEPENDENT_RECEIPT_20261015_02`。复跑命令、所有构建与二进制 SHA-256、日志 hash、逐案记录见 [S15 source/build manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_SOURCE_MANIFEST_2026-10-15.json)、[门禁 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_GATES_2026-10-15.json) 与 [逐案 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S15_E2E_CASES_2026-10-15.json)。

本轮只做功能/构建/API 可见性和安全证明审查，不做正式性能。M6A V3 的既有未闭合前置与 V3→V4 顺序不变。
