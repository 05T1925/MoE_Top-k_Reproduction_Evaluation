# BMW16 S19 决策：发布重试合同与单方视图裁决

日期：2026-10-07\
身份：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`\
基线：S18 `9e25411027a01bcf47b9c316e3c463c2f7c72127`；被测工作树从该提交建立，分支 `codex/m6b-i-bmw16-s19`。本地 `origin/main` 合并基点为 `c3926c68fd14f270faa8b55234311071947fa080`。

## 决策摘要

S19 保留 S18 的逐槽离线 KeyGen、密文 sidecar 随机读和原 VFSS Select party API。修复 shell 的覆盖式 `rename`、T 异常清理无条件删除路径、双包无共同完成状态三个确定问题；使用 no-clobber 发布、按设备号/inode 回滚本次产物、并在最后发布 root-owned paired-ready marker。派对进程只有在调用方确认 T 以 exit 0 成功退出后才可启动；marker 自身不是可绕过该启动合同的在线协调服务。

实测成功支持到 n=1000/K=80，并与冻结稳定顺序 oracle 逐位比较。它仍是 EXPERIMENTAL 研究候选：本地材料发布合同不能替代生产认证传输；整池相关 DCF 视图和实际 shuffle/inverse 组合没有可审查的完整模拟器。没有找到能据此断言不可能的具体攻击，也没有取得可升级 secure 的证明。

## 来源和工作区

- S18 的源码/历史证据只作接收对象；S19 本轮从 S18 精确 Git tree 构建，旧报告和本轮运行分开记账。
- S18 核心源在开工时的 SHA-256：
  - `experimental_bmw16_stream_store.cpp`: `fcfa6edfd8c15ac18fc29d9313130a40aff5fadb82311a7741336267a163a1b3`
  - `experimental_bmw16_material_bundle.cpp`: `0b064bd0ed6c38855d980d4a24edd85a2e9c360adef6aab0929be88016c05520`
  - `experimental_bmw16_select_party.cpp`: `9ba17231af92f1aae92a2e10ba695823553026fda36a49c09df853dedbe8df3a`
- 主桌面工作区有既存未提交差异；未改动、未复制进此工作树。`VFSS-baseline/`、论文、参考树、E20/E21 和 PR #28 不在差异范围。
- 逐文件的 S19 源码、二进制及外部结果哈希登记在 [S19 外部证据索引](../reproduction/evidence/BMW16_S19_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。

## 实际调用链与发布合同

1. T 只接收公开配置，在 score 输入产生前生成 raw/shuffle 包及 `9·C(n,2)` 个 Select/membership uCMP 槽/方。当前 stream writer 对每个新生成 key 立即写入 party-specific AES-GCM sidecar；不先调用旧全池 `material_generate()` 后再伪装流式写出。
2. sidecar 和 shell 临时文件使用 `O_EXCL`。完成后以硬链接 no-clobber 发布；不会覆盖已存在路径。写入器保存自己创建的 temp/final `(st_dev, st_ino)`，失败时只删除仍与该身份匹配的文件。shell helper 返回发布身份，T catch 只回滚本次拥有的 shell；此前已存在的文件原样保留。
3. 每个 sidecar 完成槽数、密文文件长度、manifest digest、文件同步和目录同步后才可进入 shell 发布。两份 sidecar 与两份 shell 无跨文件系统原子 rename；因此另有 132-byte `BMW16RDY` marker，绑定 session、fingerprint、n/K、公共 stream id 和双方 shell manifest hash。T 完成双方 owner/mode、目录同步后才将 marker 设为 root-owned 0444 并同步目录。
4. 单独的启动合同规定：launcher 等待 T 退出，只有 exit 0 才创建 raw-score shares 并启动 P0/P1；非零/崩溃一律不得启动 party。没有文件轮询、也不以 marker 文件“出现”作为启动事件。party 在打开自己的 shell 后、持久 bundle claim 与网络/Eval 之前核验 marker 的 owner/mode/长度/配置/stream id/本方 manifest。测试覆盖 marker 缺失、截断、篡改时两方 exit 20 且无 mask。
5. 若 T 在 marker 发布、chmod 0444 和目录同步之后但 exit 0 之前崩溃，完整 marker 可能留在磁盘；对应 failpoint 验证 launcher 以 T 非零退出禁止启动双方，遗留文件应隔离，不能仅因 marker 形状而复用。自动恢复未实现。成功 exit 后 marker 是配对完成的只读见证，但不承担远程凭据或身份认证。
6. 本地包仍以 AES-GCM/AAD 保护内容与格式字段。AAD/AEAD 说明密文及被认证的外层 metadata 未被篡改，不证明密钥份额内部确实编码 `alpha=(mask_left-mask_right) mod 2^b`；后者依赖可信 T 调用正确 KeyGen。

### 故障状态

| 事件 | 处理与输出合同 |
|---|---|
| 既存 sidecar/shell/marker 路径 | no-clobber 拒绝；保留既存 inode/内容；普通 T 失败 exit 70，不生成可领取 pair marker |
| 同 session 同路径普通重试 | 仅在处理失败已回滚本次所有权产物、冲突由运维者确认/移走后可重试；新成功 bundle 使用 fresh KeyGen |
| 第二方 sidecar/shell、chown、文件/目录 fsync 失败 | T exit 70；只回滚本次 inode；不宣告 READY |
| T 在任一发布点崩溃 | exit 71；中间文件为 orphan；除最后 durable-marker 崩溃情形外 marker 缺失。durable-marker 崩溃保留待隔离文件，但 launcher 不启动 parties |
| party 过早启动/marker 错误 | 本地 `ABORT_MATERIAL` exit 20；无 mask。双方在测试夹具均可观察 exit 20；外部无 ACK 时只称 LOCAL_ONLY |
| slot 错端点/重复读取/坏 tag | material failure；claim 已 burn，须 fresh bundle；不重试当前 bundle |

本机一次性合同仍只覆盖固定、受保护且不可回滚的 party claim root。跨空 store 复制、快照回滚和管理员级对抗没有抗重放声明。

## 实际安全游戏和视图

候选理想功能输入是 P0/P1 的 signed Q20.12 加法份额及 K；成功输出仅给各自原序 XOR mask share，且 shares 异或后必须等于稳定 Top-K mask；随机算法失败返回一致 abort、无 mask。T 可信且在线静默。待项目所有者裁决的候选泄露函数包含公开配置、`public_z`、匿名图/规范端点、opened comparison bits、pivot/U/V/W、两 task 共用匿名真实 handle、selected anonymous handle、材料访问轨迹/流量和 abort 类别。该列表不是已批准泄露。

腐化 P0 或 P1 的实际 view 还包括本方 raw share，前后向 shuffle 的本地 `sigma/tau/a/e/r_share` 和消息，本方 shell 与九个密文 DCF/uCMP 池（包括未访问槽），共享节点 mask 导致的相关阈值差，自己的两次 `evalDCF` Eval share，公开 operand/比较位、后续自适应边，membership、inverse 状态与本方 mask share。T 的 view 仅离线公开配置、KeyGen 随机性及两方打包/发布状态；它不接触 score/share/rank。

### Hybrid 尝试

- **H0→H1：随机源替换。** DCF KeyGen 从 `FSSConfig::prngs[omp_get_thread_num()]` 取得初始 block，随后由 DCF 内 AES 树扩展。一个条件化前提是每个高熵、不回滚的线程 PRG stream 满足对精确输出长度 `Q_tid` 的多输出伪随机性，混合损失可写为 `Σ_tid Adv_PRG(Q_tid)`。当前全路径没有 `Q_tid` 计数（关闭的 DCF Eval 专用计数也不是它），因而这是未量化条件，不是已证引理。S4 sampler 则每方通过 OS `getrandom` 产生 32-byte contribution，在线交换并 XOR；两个 task/domain/counter 输入 SHA-256 counter sampler，bounded mapping 用 rejection sampling，sample 用 partial Fisher–Yates。理想无放回概率界与 SHA-256/OS CSPRNG 的计算假设分开。
- **H1→H2：整池 DCF。** 对 `N=9·C(n,2)` 个槽，阈值 `alpha_ij=(r_i-r_j) mod 2^b` 在同一节点 mask 下相关；party 视图还含未使用 key。在线 `eval_strict_lt` 对 `x=(z_l-z_r) mod 2^b` 与 `y=x-2^(b-1)` 执行两次 `evalDCF`；交换的局部算术份额合成并公开严格比较 bit，后续边依据该 bit 自适应生成。一个可用归约必须接收本方 shuffle 辅助输入，替换 correlated alpha 的完整未用/已用 key 集，且模拟给定先前公开 bits 的 Eval-share/后续查询联合分布。现有仓库公开 `keyGenDCF/evalDCF` 的 API/实现，没有声明或证明此 auxiliary-input、相关多 key、output-conditioned adaptive FSS 游戏。知道自己的 Eval share 和打开 bit 后，对方 share 是公开 bit的代数后处理，但这不提供本方 Eval share 与 correlated key-pool 的模拟分布。**这是第一个未闭合 hybrid**；不是已找到可区分器，也不是不可能性证明。
- **H2→H3：shuffle/inverse。** 即使 H2 成立，仍需把真实 Protocol I 的本地 shuffle 状态、交换消息、`public_z`、两 task 共用句柄、abort 和同置换 inverse 后的输出 share 与自适应 DCF transcript 联合模拟。当前无此引理。

有限匿名图枚举或当前功能 E2E 不能补足上述分布证明。本轮没有建立“两世界相同腐化方输入 share 与理想输出、不同诚实输入导致超出候选 leakage 的区分事件”；因此不宣称攻击、也不宣称安全。**下一项唯一关键安全任务**是为精确 VFSS DCF KeyGen/Eval 形式化并外部审查 H1→H2 的 correlated-threshold、完整 key pool、opened-output-conditioned adaptive-view 安全游戏；若标准定理不能满足其辅助输入，再提交新的控制访问构造及泄露/轮数/材料成本，不默认启用。

## 成功正确性与失败概率

S16 的 soundness 是条件式：每个算法 `SUCCESS` 分支要求 bracket、rank、窗口、集合大小/边界 guard 满足不变量；两哨兵交集唯一确定稳定第 K 项；membership `R_i ≥ R_selected` 与原 index 稳定键给恰 K 位；inverse 使用前向同一置换。边界/不变量错误不得改记为算法概率 abort。S19 E2E 的全部 SUCCESS 均通过 TEST_ONLY oracle，有限测试只验证这些输入。

理想独立均匀无放回 sampler 下，沿用 S16 的固定整数规则和联合界（两个 task 用 union bound，不依赖 task 独立）：`P_abort(n,K) ≤ 2(p_b+p_U+p_W)`，`M=2·max(K,n−K+1)`。S16 枚举 n≤256 的可达 M 给上界最大 `1.3105804606611235e−11`；M≤2,000,000 的公式审计上界最大 `5.065679989546104e−8`。这是理想 sampler 数学界；真实 sampler 的 OS 合币及 SHA-256 counter 需要另加计算安全论证/优势，不能把运行中的零自然 abort 当作 ≤10⁻⁶ 证明。本轮矩阵没有观察到自然算法 abort；注入/材料故障和自然失败需分开。

## 门禁

| 门 | S19 结论 |
|---|---|
| `FUNCTIONAL` | `PASS_FOR_REBUILT_TESTED_DOMAIN`：S4 common-tape 39 + n256 2；三进程成功样本均 oracle 一致；当前 E2E/CTest 范围详见验证报告。 |
| `PUBLICATION/RETRY` | `PASS_LOCAL_NO_CLOBBER_WITH_EXPLICIT_T_EXIT_GATE`：已有路径不覆盖/误删，非零 T exit 不触发 party；durable-marker crash orphan 需隔离。 |
| `ONE_SHOT_SCOPE` | `PASS_FIXED_PROTECTED_NONROLLBACK_CLAIM_ROOT_ONLY`；跨 store/snapshot 回滚不在保证内。 |
| `JOINT_PARTY_VIEW` | `NO-GO`：首缺口 H1→H2 correlated full-pool adaptive DCF simulator；H2→H3 shuffle/inverse 组合随后仍缺。 |
| `DELIVERY` | `CONDITIONAL_LOCAL_UID_AND_AES_GCM`；非生产 party 凭据注册/远程私有认证交付。 |
| `PERFORMANCE_READY` | `NOT_READY`；未执行正式 V4 LAN/WAN 五次矩阵，n=1000 功能样本的阶段数据仅为实验诊断。 |

本决策不建立 secure alias、不修改旧 Protocol I/AAV86 入口，不改变 M6A→M6B 正式验收顺序。
