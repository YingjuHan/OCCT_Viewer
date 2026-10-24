# GitHub Actions 构建与测试

## 本次失败原因

对应提交 `5a71a60` 的 push / pull_request 各运行两个 job，因此界面出现四个失败检查。

- `build-wasm` 在初始化阶段失败，GitHub 注释为 `Unable to resolve action emscripten-core/emsdk@v3, unable to find version v3`；没有执行编译。
- `build-linux` 在安装依赖时以 100 退出，Configure / Build 均未执行。原配置引用 `libopencascade-dev`；项目还需要定制 OCCT 接口，系统包不能替代匹配源码。
- `No conflicts with base branch` 表示分支可合并，CI 成功还要求依赖、编译和测试都通过。

原始记录：[PR run](https://github.com/YingjuHan/OCCT_Viewer/actions/runs/40085823692)、[push run](https://github.com/YingjuHan/OCCT_Viewer/actions/runs/40085819475)。本地通过公开 API 读取了 job 步骤与失败注释；完整旧日志下载需要额外认证，未据此声称读取了完整日志。

## 一次性配置：读取私有 OCCT 仓库

OCCT 来源：`https://github.com/YingjuHan/OCCT_private.git`，默认分支 `ViewCaeGeom`。本地源码为 `D:\workspace\OpenCASCADE\OCCT_private`，本次对照提交为 `db3716c9f9189391d2b5f6b469bce3963ae99afa`。

1. 在 GitHub 账号的 **Settings → Developer settings → Personal access tokens → Fine-grained tokens** 创建读取凭据。
2. Repository access 只选择 `YingjuHan/OCCT_private`，Repository permissions 设置 **Contents: Read-only**。
3. 打开 **OCCT_Viewer → Settings → Secrets and variables → Actions → New repository secret**。
4. 名称填 **`OCCT_SOURCE_TOKEN`**，值填该凭据。不要把凭据写入 YAML、命令输出或 Git。

主仓库的默认 `GITHUB_TOKEN` 只能按其授权访问仓库；读取另外一个私有仓库需要上述独立凭据。来自 fork 的 PR 默认拿不到仓库 secrets，会在访问检查步骤明确失败。当前 develop 分支的 push 和同仓库 PR 可以使用仓库 secret。

可选：在同一页面的 **Variables** 新增 `OCCT_REF`，填完整提交 SHA，以便两种构建固定使用同一版本。未设置时读取 `ViewCaeGeom` 分支；每次 job 的 `versions.txt` 都记录实际 OCCT SHA。

## 工作流执行内容

入口：[ci-build.yml](../.github/workflows/ci-build.yml)。Ubuntu 24.04，各 job 默认两个编译进程，120 分钟上限。

| Job | 执行内容 | 成功条件 |
| --- | --- | --- |
| build-linux | 拉取定制 OCCT；构建共享 SDK；编译本项目和原生用例；串行执行 CTest | 全部命令成功，存在测试且无失败，生成 JUnit |
| build-wasm | Emscripten 3.1.58；TypeScript 5.4.5；构建 FreeType、zlib port、静态 OCCT SDK；编译并安装 wasm32 | 构建安装成功，JS/Wasm 存在，Node `WebAssembly.validate` 验证通过并记录 SHA-256 |

Linux 配置启用 `OCCTV_BUILD_TESTS=ON` 和 `OCCTV_BUILD_NATIVE_VIEWER=ON`，关闭 DRAW 插件、MedFile、Draco、Web 前端和 mesh exchanger。`OCCTV_BUILD_NATIVE_VIEWER` 允许独立构建 Viewer 库，纳入 native cut/timeline/analysis/session 的原生测试，不要求构建 DRAW 插件或创建显示窗口。SDK 保留 `Viewer` 需要的 TKViewerTest/TKDraw 依赖。

Wasm 使用 [官方 setup-emsdk action](https://github.com/emscripten-core/setup-emsdk)，与 OCCT、FreeType 使用同一编译器及单线程/32 位/JS exception 配置。FreeType 和自研 VTK reader 共用相同 Emscripten sysroot 的 zlib port，显式设置 `FT_REQUIRE_ZLIB=ON`。RapidJSON 头文件复制到隔离目录，避免宿主 `/usr/include` 侵入交叉编译器的标准头搜索路径。

两个 job 都保存阶段日志和实际版本；Linux 还上传 `ctest.xml`、`LastTest.log` 等。Artifacts 中的 `linux-test-results-*` / `wasm-build-logs-*` 保留 14 天。流水线遇到失败会保留失败状态，不用跳过测试取得绿色结果。

本配置的 Wasm 验证范围是编译与安装，未运行浏览器交互或 WebGL 图像用例。GitHub 托管 runner 的结果不作为 MX450 / Iris Xe 性能数据；这些硬件测试见 [双卡报告](cut-view-manager-performance-report-2026-10-24.md)。

## 如何触发新检查

推送包含工作流修复的新提交：

```powershell
Set-Location D:\workspace\OpenCASCADE\OCCT_Viewer
git push origin develop
```

现有 PR 会自动出现新提交对应的检查。在 **Actions → ci-build → 最新 run** 查看两个 job；有失败时展开首个失败步骤，并下载对应日志 artifact。

工作流进入默认分支后，也可以在 **Actions → ci-build → Run workflow** 选择分支手动运行。重新运行旧 run 会使用旧的 SHA 和 ref；修改 YAML 后应推送新提交触发新 run，见 [GitHub 重跑说明](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/re-run-workflows-and-jobs)。

## 在本机 Linux / WSL 复现

Linux 依赖安装与 YAML 的 Install dependencies 步骤相同。共用脚本位于 `adm/ci/`，以 Bash 调用；`.gitattributes` 固定脚本为 LF，避免 Windows checkout 的 CRLF 影响 Linux。

下面命令在 WSL Ubuntu 内执行，使用本地已提交的 OCCT 快照。先安装工作流中列出的 Linux 依赖：

```bash
mkdir -p "$HOME/occtviewer-ci/occt-source"
git -C /mnt/d/workspace/OpenCASCADE/OCCT_private archive HEAD \
  | tar -xf - -C "$HOME/occtviewer-ci/occt-source"
export CI_OCCT_REVISION=$(git -C /mnt/d/workspace/OpenCASCADE/OCCT_private rev-parse HEAD)
cd /mnt/d/workspace/OpenCASCADE/OCCT_Viewer
CI_BUILD_JOBS=4 bash adm/ci/build-linux.sh \
  "$HOME/occtviewer-ci/occt-source" "$HOME/occtviewer-ci/linux"
```

Wasm 复现需要先激活 Emscripten 3.1.58，并在 Linux 环境安装 TypeScript 5.4.5：

```bash
source /path/to/emsdk/emsdk_env.sh
npm install -g typescript@5.4.5
cd /mnt/d/workspace/OpenCASCADE/OCCT_Viewer
CI_BUILD_JOBS=2 bash adm/ci/build-wasm.sh \
  "$HOME/occtviewer-ci/occt-source" "$HOME/occtviewer-ci/wasm"
```

共用脚本记录每个阶段的退出状态，`tee` 不会掩盖编译或 CTest 失败。工作目录分为 `occt-build`、`occt-install`、`viewer` 和 `logs`；修改后可在同一目录增量重试。

## 本轮验证记录

| 任务 | 状态 | 验收 |
| --- | --- | --- |
| CI01 工作流与依赖来源 | 已完成，d5b5578 | 修复无效 action；拉取指定定制 OCCT；添加共用构建脚本、真实 CTest、JUnit 和日志 artifact；actionlint/Bash 检查通过 |
| CI02 Linux 编译与测试 | 已完成，7e768b3 | Ubuntu 24.04 / GCC；共享 SDK、本项目和独立 native Viewer 构建通过，33/33 原生 CTest 通过 |
| CI03 Wasm 编译与安装 | 已完成，ed5aa82 | Emscripten 3.1.58；FreeType、zlib、静态 SDK和本项目编译安装通过；最终模块格式验证通过 |

每项完成后独立提交、刷新 codebase-memory 索引，再推进下一项；提交号在后续记录中补录。

- 已通过 actionlint 1.7.12 和 Bash 语法检查。
- Wasm 校验器额外使用损坏模块做负向核验，按预期返回失败；日志 artifact 保留构建和校验的失败状态。
- 已确认远端 `ViewCaeGeom` 与本机 OCCT 提交一致。
- Linux 实编译修正了后续依赖问题：显式构建 `TKOpenGl`；移除 Gmsh reader 未使用的 `Interface_Static.hxx` 引用。新增独立 native Viewer 配置，避免 DRAW 关闭时漏测 native API。
- Linux 共享 SDK、本项目及独立 native Viewer 构建通过，33/33 原生 CTest 通过，测试耗时 59.15 秒。包含 native cut/timeline/analysis/session，关闭窗口型 DRAW 测试；与 Windows 的 85 项集合不同。[JUnit](test-results/github-actions-2026-10-24/linux/ctest.xml)、[测试日志](test-results/github-actions-2026-10-24/linux/ctest.log)、[版本/源码指纹](test-results/github-actions-2026-10-24/linux/manifest.json) 已归档。归档为修正后的最终增量构建轮次，CI 首次运行需要从源码全量编译 SDK。
- Wasm 依赖、静态 SDK、本项目 C++ / TypeScript 编译和最终安装通过；Node 模块格式验证通过，Wasm 大小 17,816,162 字节，SHA-256 为 `ddd105e70a58da350bb440d6c585512e2cbff4a0af8017cf1d0917b002409340`。本地后半段改为四进程增量编译，云端默认仍为二进程。日志与源码指纹见 [Wasm 验证证据](test-results/github-actions-2026-10-24/wasm/manifest.json)。
- Wasm 实编译补充 `<string>` 显式包含，解决 libc++ 不提供 GCC 间接包含的情况；显式设置 RapidJSON/FreeType 的交叉编译路径。FreeType 原使用内置 gzip 与独立 zlib 混合时，最终模块在 `inflate` 调用处发生栈签名校验失败；统一到 Emscripten zlib port 后构建和格式验证通过。本次没有修改第三方源码，也没有关闭错误检查。
- `<string>` 修正后，Linux 增量构建和 `cut_cache` 针对性复测通过（完整 33 项是在这项纯头文件修正前执行）；原生预算逻辑不改。

云端最终通过状态需要在配置 `OCCT_SOURCE_TOKEN` 并推送修复后核对，不能用本地构建结果代替 GitHub 的检查状态。
