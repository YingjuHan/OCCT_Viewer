# 本项目 Cut View Manager：Linux 直接运行、GDB 调试与测试命令

## 1. 适用范围与前置条件

本文件操作的是 **`image-client / OCCTViewer` 当前已经实现的 Cut View Manager**，通过 `DRAWEXE` 加载本项目 `libViewDraw.so`。以下均为 Linux **Bash** 命令，模型使用 3,375,000 单元的 `hex_150.vtu`；不是 PowerShell，也不是尚未实现的 VisIt 迁移版启动命令。

当前参考实现依赖 OCCT。必须使用能提供本项目所需 `AIS_ClippingPlanes`、shader、mutable buffer 等接口的匹配 SDK；现有本机参考基线是定制 OCCT 7.7.0，不能保证任意发行版系统包可直接替代。**目标 VisIt 迁移版完全不使用 OCCT**，约束另见 [无 OCCT 迁移手册](cut-view-manager-visit-porting.md)。

需要可用的 C++17 编译器、CMake（以下使用 `-S/-B`，建议已配置的 3.16 或更新工具）、Ninja、Python 3、RapidJSON、zlib、OpenGL / X11 开发环境，以及带 DRAW、Tcl / Tk 和资源文件的 OCCT SDK。CentOS 7.9 上先启用项目实际使用的工具链；不要把 Windows MinGW DLL 或 `.exe` 拷到 Linux 运行。

**下面命令已按当前 CMake、DRAW 参数及测试脚本核对，本轮未在 Linux 机器上实跑。** 第一次执行时填写实际安装路径，保留启动日志与 `ldd` 输出。

### 1.1 每个新终端先设置路径和 DRAW 环境

`/opt/image-client` 和 `/opt/occt` 是路径示例，需要改成 Linux 实际位置。资源也可能在 SDK 的 `src/`，库也可能在 `lib64/`；以该 SDK 的 CMake 配置为准。

```bash
export CUT_REPO=/opt/image-client
export CUT_OCCT=/opt/occt
export CUT_OCCT_CMAKE="$CUT_OCCT/lib/cmake/opencascade"
export CUT_OCCT_LIB="$CUT_OCCT/lib"
export CUT_OCCT_RES="$CUT_OCCT/share/opencascade/resources"
export CUT_DRAWEXE="$CUT_OCCT/bin/DRAWEXE"
export CUT_MODEL="$CUT_REPO/build/large-vtk/hex_150.vtu"

# 保存额外运行库路径，随后切换 Release / Debug 时只装入对应构建目录。
export CUT_BASE_LD_LIBRARY_PATH="${LD_LIBRARY_PATH-}"
export CASROOT="$CUT_OCCT"
export CSF_OCCTResourcePath="$CUT_OCCT_RES"
export CSF_ShadersDirectory="$CUT_OCCT_RES/Shaders"
export DRAWHOME="$CUT_OCCT_RES/DrawResources"
export DRAWDEFAULT="$DRAWHOME/DrawDefault"
export CSF_DrawPluginDefaults="$DRAWHOME"
export CSF_VIEWDRAWDefaults="$CUT_REPO/source-draw/ViewDraw"

test -f "$CUT_REPO/CMakeLists.txt"
test -f "$CUT_OCCT_CMAKE/OpenCASCADEConfig.cmake"
test -x "$CUT_DRAWEXE"
test -f "$DRAWDEFAULT"
test -f "$CSF_VIEWDRAWDefaults/VIEWDRAW"
cd "$CUT_REPO"
```

这些检查应全部成功。若 SDK 必须先 `source /实际路径/env.sh`，在上面设置之前加载它；再核对这里的库、资源和 executable 是否属于同一 SDK。`DRAWDEFAULT` 指向文件，不能只写目录。

图形交互还要求当前 `DISPLAY` 可用，可在 Linux 桌面终端检查：

```bash
printf 'DISPLAY=%s\n' "${DISPLAY-}"
glxinfo -B
```

不要随意强设 `DISPLAY=:0`；必须有该 X display 的访问权限。`glxinfo -B` 用来记录真实 renderer / 驱动，软件渲染环境不用于判定生产 GPU 性能。

### 1.2 生成百万单元数据及 Linux 演示入口

已有 `hex_150.vtu` 可跳过生成。生成命令也会得到 legacy `.vtk`，方便对照读取路径：

```bash
python3 test/generate-large-vtk.py \
  --grids 150 --formats vtk vtu --deformation \
  --output "$CUT_REPO/build/large-vtk"
test -s "$CUT_MODEL"

mkdir -p "$CUT_REPO/build"
cat > "$CUT_REPO/build/cut-view-linux-demo.tcl" <<'TCL'
source [file join $::env(CUT_REPO) test cae viewdraw_runtime.tcl]
set ::vtk_cut_demo_file $::env(CUT_MODEL)
source [file join $::env(CUT_REPO) test cae cut_view_vtk_demo.tcl]
scvcutview cut_view_vtk_demo field point temperature 0
scvcutview cut_view_vtk_demo visibility x 3
scvcutview cut_view_vtk_demo renderbackend auto
scvcutview cut_view_vtk_demo handle x 1 0
puts [scvcutview cut_view_vtk_demo renderinfo]
vrenderparams -perfCounters fps -perfUpdateInterval 0.5
TCL
```

脚本生成到 `build/`，按环境变量取路径，不修改仓库测试脚本。它启动视口与 manager，显示负侧实体＋切面及平面手柄。mask=2 为仅切面，mask=7 为两侧实体＋slice。

## 2. 直接运行：Release、无 GDB

### 2.1 首次构建或源码更新后构建

```bash
cmake -S "$CUT_REPO" -B "$CUT_REPO/build/linux-release" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCASCADE_DIR="$CUT_OCCT_CMAKE" \
  -DRapidJSON_ROOT_DIR=/usr \
  -DOCCTV_BUILD_DRAW=ON \
  -DOCCTV_BUILD_TESTS=ON \
  -DOCCTV_BUILD_WEB_VIEWER=OFF \
  -DOCCTV_BUILD_MESH_EXCHANGER=OFF \
  -DOCCTV_USE_CONAN=OFF \
  -DOCCTV_USE_DRACO=OFF \
  -DOCCTV_USE_MEDFILE=OFF \
  -DOCCTV_INSTALL_DEPENDENCIES=OFF
cmake --build "$CUT_REPO/build/linux-release" --parallel 4
```

RapidJSON 不在 `/usr/include/rapidjson` 时，将 `RapidJSON_ROOT_DIR` 改为真实前缀。这里必须启用 `OCCTV_BUILD_DRAW=ON`；仓库 CI 中关闭 DRAW 的精简构建不能直接运行 manager 界面。内存紧张时降低并行构建数量。

### 2.2 直接启动

新终端先执行第 1 节；已有 Release 构建后可以直接执行这一组：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
test -f "$CUT_BUILD/lib/libViewDraw.so"
ldd "$CUT_BUILD/lib/libViewDraw.so"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/build/cut-view-linux-demo.tcl"
```

`ldd` 不应出现 `not found`，并且各库来源应正确。`-f` 放在最后，其后只跟 Tcl 文件路径。窗口中拖动切面即可；性能测试使用这一模式，不挂 GDB。

在 `Draw[n]>` 提示符中执行的是 Tcl，例如：

```tcl
scvcutview cut_view_vtk_demo visibility x 2
scvcutview cut_view_vtk_demo renderinfo
scvcutview cut_view_vtk_demo visibility x 7
```

确认 `renderinfo` 的实际 backend 为 `gpu_plane`；auto 可能因多 cut、透明度、向量或显示模式回退 CPU。只看切面时仍由 CPU slice 计算、GPU 绘制结果。

## 3. 调试运行：Debug 插件＋GDB

### 3.1 构建可调试的本项目

Debug 使用独立构建目录。先执行第 1 节，随后：

```bash
cmake -S "$CUT_REPO" -B "$CUT_REPO/build/linux-debug" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g3' \
  -DOpenCASCADE_DIR="$CUT_OCCT_CMAKE" \
  -DRapidJSON_ROOT_DIR=/usr \
  -DOCCTV_BUILD_DRAW=ON \
  -DOCCTV_BUILD_TESTS=ON \
  -DOCCTV_BUILD_WEB_VIEWER=OFF \
  -DOCCTV_BUILD_MESH_EXCHANGER=OFF \
  -DOCCTV_USE_CONAN=OFF \
  -DOCCTV_USE_DRACO=OFF \
  -DOCCTV_USE_MEDFILE=OFF \
  -DOCCTV_INSTALL_DEPENDENCIES=OFF
cmake --build "$CUT_REPO/build/linux-debug" --parallel 4
```

### 3.2 用 GDB 启动相同的百万单元演示

```bash
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
test -f "$CUT_BUILD/libd/libViewDraw.so"
ldd "$CUT_BUILD/libd/libViewDraw.so"
gdb -q \
  -ex 'set breakpoint pending on' \
  -ex 'set pagination off' \
  -ex 'break ViewCae::InteractiveCaeCutPlane::ProcessDragging' \
  --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/build/cut-view-linux-demo.tcl"
```

进入 GDB 后：

```gdb
run
bt
info sharedlibrary
continue
```

`run` 启动程序，窗口加载后拖动切面才命中拖动断点；`bt` 查看命中后的调用栈，`info sharedlibrary` 核对 Debug 库是否装入。插件延迟加载，所以需要 pending breakpoint。需要继续顺畅操作时可 `disable 1` 再 `continue`。

Debug / GDB 下首次读取和切面计算会更慢，不能把其 FPS 与 Release 基线混用。上面生成的是本项目调试符号；调 OCCT 内部还需要同一 SDK 对应符号和源码映射。未剥离的匹配 DRAW 宿主即可加载本项目 Debug 插件，不要求为只调本项目再混入另一套 SDK。

## 4. 自动测试：普通执行与 GDB 定位

### 4.1 Release CTest

先按 2.2 设置 Release 的 `CUT_BUILD / LD_LIBRARY_PATH`；在子 shell 中切换到构建目录，以兼容不支持 `ctest --test-dir` 的旧工具：

```bash
(
  cd "$CUT_BUILD"
  ctest -N -R '^(cut_.*|native_.*|quadric_cut|general_cut_cache|vtk_cell_cut)$'
  ctest --output-on-failure \
    -R '^(cut_.*|native_.*|quadric_cut|general_cut_cache|vtk_cell_cut)$'
)
```

先检查 `-N` 列表非空。这里覆盖 cut 的正确性、字段 / 物理、缓存和 native 门面；需要图形显示的用例另行执行。CTest 不包含下面的百万单元真实拖动命令，不应把小算例通过当作大模型性能通过。

### 4.2 Debug 测试与单个测试的 GDB

按 3.2 设置 Debug 环境后，可以同样执行 CTest。要定位单个 native 测试失败，直接调试其二进制：

```bash
gdb -q --args "$CUT_BUILD/bind/cut_view_tests"
```

GDB 中按需执行：

```gdb
catch throw
run
bt
continue
```

`catch throw` 适合异常定位，会在预期异常处也停住；只在需要时使用。调其他用例可换成 `cut_cache_tests`、`cut_physics_tests` 等实际 target 名称，不能把 CTest 的 `cut_view` 名称直接当 executable。

## 5. 337.5 万单元真实鼠标拖动测试

### 5.1 正式计时：Release 直接运行

先按 2.2 设置 Release 环境，确认模型存在，执行 mask=2/3/7，每项 60 帧：

```bash
export CUTVIEW_BENCHMARK_FILE="$CUT_MODEL"
export CUTVIEW_BENCHMARK_FRAMES=60
set -o pipefail

for mask in 2 3 7; do
  export CUTVIEW_BENCHMARK_MASK="$mask"
  export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/linux-results/drag-mask-$mask"
  mkdir -p "$CUTVIEW_OUTPUT_DIR"
  "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_drag_benchmark.tcl" \
    2>&1 | tee "$CUTVIEW_OUTPUT_DIR/draw.log" || break
done
```

检查每个目录的 `draw.log` 都含 `CUT_DRAG_BENCHMARK_PASS` 和 `CUT_DRAG_BENCHMARK_RESULT`，同时保存 `drag.csv / drag-first.png / drag-last.png`。循环在失败时停止，后面的 mask 可能未执行；不能只看最后一个 shell 返回值。

该脚本通过真实 AIS 鼠标路由验证：一次 move 一次同步、body 不重新上传、body / cap 成对、松手位置不跳变、abort 恢复。它需要可用 X 显示与 Tk，不要改成 `-b`，否则无法验证 manager 的真实交互。

如需 legacy VTK 对照，将 `CUT_MODEL` 改成同目录的 `hex_150.vtk` 后重新执行，并使用不同输出目录保留证据。首次读取 / 建树和连续 warm drag 应分别报告。

### 5.2 调试同一拖动测试

按 3.2 设置 Debug 环境后执行，数据规模仍为 3,375,000 单元：

```bash
export CUTVIEW_BENCHMARK_FILE="$CUT_MODEL"
export CUTVIEW_BENCHMARK_FRAMES=60
export CUTVIEW_BENCHMARK_MASK=3
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/linux-results/drag-debug"
mkdir -p "$CUTVIEW_OUTPUT_DIR"
gdb -q -ex 'set breakpoint pending on' \
  --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_drag_benchmark.tcl"
```

用 `run` 启动，必要时中断查看 `thread apply all bt`。这个模式用于定位问题，生成的耗时包含调试开销，不并入正式性能表。

## 6. 接缝回归与无桌面环境

按 2.2 设置 Release 环境，可单独运行已有 GPU / CPU 掩码和接缝回归：

```bash
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/linux-results/gpu-plane"
mkdir -p "$CUTVIEW_OUTPUT_DIR"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_gpu_plane.tcl"
```

成功输出 `CUT_GPU_PLANE_PASS`。其中包含小型精确几何回归，用于检查大坐标、变形和接缝；它不作为百万单元性能测试。

没有桌面时，如果已安装 Xvfb，可仅为图像 / 交互功能回归创建虚拟显示：

```bash
xvfb-run -a -s '-screen 0 1280x1024x24' \
  "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_gpu_plane.tcl"
```

虚拟显示仍需可用 GLX / OpenGL；若 renderer 是软件实现，不比较生产 FPS。已有 CTest 图像用例可通过 `OCCTV_TEST_DRAW_VISUAL=ON` 注册后运行，但其中 smoke 数据不能替代第 5 节的大数据测试。

## 7. 常见启动问题

| 现象 | 检查命令 / 处理 |
| --- | --- |
| 找不到 `libViewDraw.so` 或其依赖 | `ldd "$CUT_BUILD/lib/libViewDraw.so"`；Debug 换 `libd`；必须在启动 DRAW 前设置 `LD_LIBRARY_PATH` |
| `pload -VIEWDRAW` 找不到资源 | 核对 `CSF_VIEWDRAWDefaults` 指向仓库 `source-draw/ViewDraw`，该目录有 `VIEWDRAW` |
| DRAW 提示 `No default file` | 核对 `DRAWDEFAULT` 是有效文件，`DRAWHOME` 是对应目录 |
| manager 不存在 / Tk 不可用 | 确认 SDK 构建了 Tcl / Tk，使用 `-i` 与有效 `DISPLAY` |
| 读到目录导致 permission denied | `test -f "$CUT_MODEL"`；`source` 后必须是 Tcl 文件，模型路径传给 read / demo 变量 |
| Debug 断点不命中 | `info sharedlibrary`；确认加载 `linux-debug/libd`，源码有 `-g`，并开启 pending breakpoint |
| GPU 路径未启用 | DRAW 内执行 `scvcutview cut_view_vtk_demo renderinfo` 查看 fallback reason |
| 拖动仅在 GDB 下很慢 | 用第 2 节 Release 直接运行对照；Debug 和性能计时分别记录 |

命令依据：[构建选项](../CMakeLists.txt)、[输出目录](../adm/cmake/platform_setup.cmake)、[测试注册](../test/native/CMakeLists.txt)、[插件资源](../source-draw/ViewDraw/VIEWDRAW)、[Linux 也会调用的插件加载脚本](../test/cae/viewdraw_runtime.tcl)、[大模型演示](../test/cae/cut_view_vtk_demo.tcl)、[鼠标拖动测试](../test/cae/cut_view_drag_benchmark.tcl)。
