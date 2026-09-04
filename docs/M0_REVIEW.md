# M0 复检记录

复检日期：2026-09-04

## 1. 复检结论

M0 的“代码基线冻结、仓库边界、协议命名、统一输出和指标规范”已完成复检并在远端闭环。
M0 提交为 `354c40d`，当前 `main` 已推进到 M1 提交 `0d82dce`；冻结标签仍指向
`993696e`。本次没有把本地论文或大型参考工程加入 Git。

M1 的 oracle、CmpAgg、metrics 和 DCF conformance 测试已在本机 Apple Clang/CMake
环境构建并全部通过。参考资料仍需单独完成许可证确认和 source-only 清理后再决定分发方式。

## 2. 已通过项目

| 检查项 | 状态 | 证据 |
| --- | --- | --- |
| 主分支存在 | 通过 | 当前分支为 `main` |
| 基线提交存在 | 通过 | `993696e chore: establish VFSS project baseline` |
| 冻结标签存在 | 通过 | `vfss-baseline-2026-09-03` |
| 活动树与冻结树一致 | 通过 | `diff -qr VFSS VFSS-baseline` 返回 0 |
| 基线规模一致 | 通过 | 两侧各 131 个源文件，共 262 个 |
| Git 对象完整 | 通过 | `git fsck --full --no-reflogs` 无错误，仅报告 1 个 dangling tree |
| 符号链接 | 通过 | 已追踪文件中没有符号链接 |
| 明显凭据文件 | 通过 | 仓库两层内未发现 `.env`、PEM、key、credential/secret 文件 |
| 参考工程边界 | 通过 | `ADSMPC/`、`Agarwal_TopK/`、`CipherGPT/` 被忽略 |
| 论文边界 | 通过 | `Papers/` 已加入 `.gitignore` |
| 生成物边界 | 通过 | build、对象/库、bin/pkg/log、tmp、artifacts、output 被忽略 |
| 项目总纲 | 通过 | `PROJECT.md` 已中文化并同步论文、输出和指标边界 |
| 队友入口 | 通过 | 根目录 `README.md` 提供中文导航 |
| 详细计划 | 通过 | `docs/IMPLEMENTATION_PLAN.md` 定义里程碑与退出条件 |
| 来源清单 | 通过 | `docs/REFERENCE_MANIFEST.md` 已改为当前 Git 状态和中文说明 |
| M1 基础测试 | 通过 | oracle、CmpAgg、metrics、DCF conformance 四个目标均返回 0 |

## 3. 当前工作区预期变更

本轮应只包含以下项目治理变更：

- `PROJECT.markdown` 重命名为 `PROJECT.md`，并更新内容；
- 更新 `.gitignore`；
- 更新 `docs/REFERENCE_MANIFEST.md`；
- 新增 `README.md`；
- 新增 `docs/M0_REVIEW.md`；
- 新增 `docs/IMPLEMENTATION_PLAN.md`。

`Papers/` 及三个本地参考工程不应出现在待提交文件列表中。Git 在未暂存时会把
重命名显示为“删除旧文件 + 新增新文件”；暂存后应复核它是否被识别为 rename，
但识别形式不影响内容正确性。

## 4. 当前状态与仍需决定的事项

| 项目 | 状态 | 说明 |
| --- | --- | --- |
| GitHub 远端 | 已完成 | `origin` 为 `git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git` |
| 仓库可见性 | 已确认 | GitHub 页面显示为 Public |
| LICENSE | 未添加 | 当前没有项目级许可证，公开仓库默认不授予复用权限 |
| M0 文档提交 | 已完成 | `354c40d docs: finalize M0 repository and deployment baseline` |
| M1 提交 | 已完成 | `0d82dce feat: add M1 Top-K oracle and metrics conformance` |
| 远端推送 | 已完成 | `main` 与 `vfss-baseline-2026-09-03` 均已推送并核验 |
| 目标环境构建 | 部分完成 | 本机四个 M1 测试目标构建并通过；Linux/LAN/WAN 尚未测量 |
| 论文与参考工程 | 暂不提交 | 原目录含未确认再分发资料、构建物、日志、二进制、压缩包和嵌套 Git |

## 5. 推送前人工检查顺序（已执行）

```bash
git status --short
git diff --check
git add -n .
git diff --cached --stat
git diff --cached --check
```

确认预览中没有 `Papers/`、`ADSMPC/`、`Agarwal_TopK/`、`CipherGPT/`、构建目录和
密钥/日志后，再暂存并检查：

```bash
git add .
git status --short
git diff --cached --name-status
git diff --cached --check
```

本次使用 SSH 远端：

```bash
git remote set-url origin git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git
git push -u origin main
git push origin vfss-baseline-2026-09-03
```

如果远端已有提交，不要直接强推；先检查远端分支关系，再决定合并方式。

## 6. M0 退出条件

满足以下条件后，M0 才算在远端正式闭环：

1. 本轮待提交列表只包含第 3 节所列治理文档；
2. `.gitignore` 验证参考资料和生成物不会被加入；
3. `main` 与 `vfss-baseline-2026-09-03` 均已推送；
4. GitHub 页面能从 `README.md` 进入总纲、实施计划和来源清单；
5. 仓库可见性和许可证状态已经明确告知队友；
6. M1 开始后再次确认 `VFSS-baseline/` 仍未被修改；本次复检通过。
