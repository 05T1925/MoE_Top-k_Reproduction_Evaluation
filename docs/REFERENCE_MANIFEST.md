# 本地参考资料清单

## 1. 状态与用途

本清单定义 M0 完成后的远端仓库边界。它不删除、移动或修改本地参考目录，只防止
这些目录被普通 `git add .` 意外加入远端历史。

远端仓库的追踪范围包括：

- `VFSS/`：唯一活动实现目录；
- `VFSS-baseline/`：冻结的源码级恢复与比较基线；
- `README.md`、`PROJECT.md`、`AGENTS.md`、`.gitignore`；
- `docs/` 下的来源、复检、决策和实施计划文档。

下列原始论文和参考工程保留在本地，等待后续明确来源、许可证和分发方式。
队友的具体放置、校验和按分工准备方式见 `docs/LOCAL_REFERENCES_SETUP.md`。

| 本地路径 | 角色 | 首版 Git 状态 | 原因 |
| --- | --- | --- | --- |
| `Papers/` | 论文、学习笔记和团队测试要求 | 本地保留、忽略 | 版权与再分发状态未逐项确认 |
| `ADSMPC/` | Protocol III 和 CipherGPT-style 旧原型 | 本地保留、忽略 | 含构建树、实验输出，且不是目标运行时 |
| `Agarwal_TopK/` | Protocol I、CA、Protocol III 算法参考 | 本地保留、忽略 | 约 2.4GB，含归档、依赖、静态库、生成结果和嵌套 Git 数据 |
| `CipherGPT/` | CipherGPT 原生两方基线 | 本地保留、忽略 | 含捆绑依赖、构建树和实验结果 |

## 2. 必须保留的本地目录

```text
Papers/
ADSMPC/
Agarwal_TopK/
CipherGPT/
```

“被 Git 忽略”不是“可以删除”。这些目录仍是论文核对、代码追踪、差分测试和
后续来源提取的第一参考。

## 3. 主要参考入口

| 对象 | 主要入口 | 用途与边界 |
| --- | --- | --- |
| Protocol I B0/B1 | `Agarwal_TopK/protocol1/`、`protocol1_ca/` | 全对全排名、参考 shuffle、CA 与测试；不直接复用其 FSS ABI |
| Protocol III 算法 | `Agarwal_TopK/protocol3_ca/` | DCF conformance 和明文 AAV86 图测试；不是完整实现 |
| Protocol III 旧原型 | `ADSMPC/src/protocol3.cpp`、`RankingPhase.h`、`routing_dpf.h` | 调用顺序参考；文件轮询、明文 Dealer 和旧密钥格式不迁移 |
| CipherGPT 原生 | `CipherGPT/src/globals.cpp`、`test/Top_K_paper_test.cpp`、`src/shuffle.cpp` | 原生 Top-K 基线；当前仍需修正终止、同分和统一 mask 输出 |
| CipherGPT-style 旧实验 | `ADSMPC/src/ciphergpt_topk_dcf_shuffle.cpp` | FSS/DCF 对照原型；不是原生 CipherGPT |
| 测试规范 | `Papers/测试指标.md`、`Papers/Agarwal与CipherGPT实验对比.pdf` | 团队统一输出、测试矩阵与指标来源；约束已同步到 `PROJECT.md` |

当前论文副本的逐文件 SHA-256 记录在 `docs/PAPERS.sha256`。哈希只用于确认团队
版本一致，不表示仓库对相应资料拥有再分发权。

## 4. 普通 Git 历史明确排除项

- 所有论文 PDF 和未确认可再分发的资料；
- `build/`、`CMakeFiles/`、对象文件、静态/动态库；
- 生成的 DCF/DPF 密钥、离线包、临时通信文件和日志；
- benchmark 输出、原始结果 CSV 和本地临时输入；
- `Agarwal_TopK/protocol1_ca/frozen_b1_stage4c/` 等 clean-room 运行产物；
- `Agarwal_TopK/frozen_inputs/` 下的压缩依赖或源码包；
- 参考包内的嵌套 `.git` 元数据。

## 5. 后续共享方式

队友确实需要完全相同的参考代码时，每个实现只选择一种经过审查的方式：

1. 在 `reference-snapshots/` 放置最小、只含源码的快照；
2. 使用不可变外部归档，并记录 SHA-256 与获取方式；
3. 仅在资料必要、许可允许且普通 Git 不适合时使用 Git LFS。

决策必须记录来源、许可证/再分发状态、revision 或归档哈希、包含/排除路径和构建
说明。不得为了方便直接强制加入整套本地参考目录。

## 6. VFSS 基线校验

基线提交为 `993696e`，标签为 `vfss-baseline-2026-09-03`。2026-09-04 复检时，
排除构建产物和 `.DS_Store` 后，`VFSS/` 与 `VFSS-baseline/` 的 131 个源文件仍
逐文件一致；`diff -qr VFSS VFSS-baseline` 返回 0。

初始归一化清单 SHA-1：

```text
972fcd23187ecb603b22e37a2fefe608b2cb830b
```

M1 已在 `VFSS/` 中加入统一语义、oracle、CmpAgg、metrics、测试及必要构建修正，
因此活动树现在与冻结树存在预期差异。2026-09-04 复检确认
`VFSS-baseline/` 相对冻结标签没有变化。后续不得同步修改冻结树来消除活动实现的
差异。
