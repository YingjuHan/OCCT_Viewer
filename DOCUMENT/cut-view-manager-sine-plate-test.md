# 正弦薄板剖切回归与交互复现

## 数据定义

此用例检查起伏薄体的截面缺片、保留体丢失、共享面重复和点场插值。数据位于
[`test/native/fixtures/sine_plate_6x6.vtu`](../test/native/fixtures/sine_plate_6x6.vtu)
和同目录的 [Legacy VTK](../test/native/fixtures/sine_plate_6x6.vtk)，可直接读取，无需安装外部 VTK。

- XY 范围均为 `[0,1]`，6×6×1，共 36 个八节点六面体、98 个共享节点。
- 顶面节点：`z_top = 0.25 + 0.15*sin(2*pi*x)`。
- 底面节点：`z_bottom = z_top - 0.05`。0.05 是 **Z 方向厚度**。
- 每个节点均携带标量 `PointData.temperature = z`，不是 CellData。
- 文件是 `UNSTRUCTURED_GRID` / `UnstructuredGrid`；节点编号和单元顺序经过重排，读取器必须使用 connectivity。

这是线性六面体网格，正弦公式作用于节点；单元内部按有限元插值连接，不能把离散薄板当成精确正弦曲面。
一周期只有六个 X 区间，采样顶面最大约 0.379904、最小约 0.120096，未采到连续正弦的极值 0.4 / 0.1。

## 在 DRAW 中直接打开

使用已配置运行环境的 DRAWEXE。在 Windows DRAW 提示符输入：

```tcl
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_sine_plate_demo.tcl
```

Linux 先完成 [Linux 运行文档](cut-view-manager-linux-run-test.md) 中的环境设置和 Release 构建，然后直接运行：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_sine_plate_demo.tcl"
```

调试时切换到该文档的 Debug 构建，使用匹配的 `libd`：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
gdb -q -ex 'set breakpoint pending on' \
  -ex 'break ViewCae::InteractiveCaeCutPlane::ProcessDragging' \
  --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_sine_plate_demo.tcl"
```

在 GDB 中执行 `run`，拖动切面触发断点，`bt` 查看调用栈，`continue` 继续。交互性能应使用 Release 直接运行测量。

示例启动 Cut View Manager，模型名 `sine_plate`，切面 ID `cut`，默认 `y=0.5`，显示负侧及截面，按 temperature 着色。
在 DRAW 中切换以下组合，随后继续拖动：

```tcl
# 完整正弦截面；Z 沿截面变化，因此 temperature 有渐变。
scvcutview sine_plate move cut 0 1 0 -0.5
scvcutview sine_plate visibility cut 2

# 水平面会切出分离的条带；该平面的 temperature 全为 0.225。
scvcutview sine_plate move cut 0 0 1 -0.225
scvcutview sine_plate visibility cut 3

# 另一侧 + 截面；两侧分别验证，不能少掉应保留的波峰/波谷。
scvcutview sine_plate visibility cut 6

# 两侧 + 截面：原薄板完整显示，体表不执行单侧 clipping。
scvcutview sine_plate visibility cut 7

# 在同一视角比较 CPU 与 GPU。
scvcutview sine_plate renderbackend cpu
scvcutview sine_plate renderbackend auto
scvcutview sine_plate renderinfo
```

| mask | 行为 |
| --- | --- |
| 0 | 隐藏所有部分 |
| 1 / 4 | 负侧 / 正侧，显示该侧封口 |
| 2 | 仅截面，两侧体均隐藏 |
| 3 / 6 | 负侧 + 截面 / 正侧 + 截面 |
| 5 | 两侧体，原体完整，隐藏独立截面 |
| 7 | 两侧体及独立截面 |

## 自动验证

Windows 使用现有 Release 配置：

```powershell
cmake -S . -B build/mingw-release -DOCCTV_BUILD_TESTS=ON -DOCCTV_TEST_DRAW_VISUAL=ON
cmake --build build/mingw-release --target cut_sine_plate_tests ViewDraw --parallel 6
ctest --test-dir build/mingw-release -R '^(cut_sine_plate|draw_cut_view_sine_plate)$' --output-on-failure
```

Linux 使用已有环境，图像测试需要可用的 X/OpenGL 显示环境：

```bash
cmake -S "$CUT_REPO" -B "$CUT_REPO/build/linux-release" \
  -DOCCTV_BUILD_TESTS=ON -DOCCTV_TEST_DRAW_VISUAL=ON
cmake --build "$CUT_REPO/build/linux-release" \
  --target cut_sine_plate_tests ViewDraw --parallel
(
  cd "$CUT_REPO/build/linux-release"
  ctest -R '^(cut_sine_plate|draw_cut_view_sine_plate)$' --output-on-failure
)
```

[`CutSinePlateTests.cpp`](../test/native/CutSinePlateTests.cpp) 的独立 XZ 二维几何参考检查逐单元截面面积、原始单元归属、节点 FE 权重、截面方程及 `temperature=z`。
同时检查 CPU 显示合并结果的两侧体积及总体积 0.05，防止“截面正确、保留体缺失”。共享面剖切只计一次。
测试覆盖 VTK/VTU 两种导入、不同周期和相位、水平/竖直/斜面、相反法向、截面显示组合和空截面。

[`cut_view_sine_plate.tcl`](../test/cae/cut_view_sine_plate.tcl) 比较两种格式、六组平面、相反法向和八种 mask 下的 CPU/GPU 轮廓，检查非空显示和点场值。
成功标记为 `CUT_SINE_PLATE_PASS`、`CUT_SINE_PLATE_DRAW_PASS`；截图保存到构建目录 `test/native/draw-sine-plate/`。
图像轮廓容差允许边缘像素差异，逐单元数值验证另行约束几何正确性。

本次在 Windows / MinGW Release 实测通过：800 组截面组合、三周期混叠用例、12 组基准保留体体积检查、192 组 CPU/GPU 图像对比（384 张截图），以及交互示例启动检查。
在这些组合中未复现异常缺片；此结论限于上述数据、平面和容差。Linux 命令已核对构建配置并通过 Bash 语法检查，本次未在 Linux 环境执行。

## 周期、断面与缺片的区别

水平剖切平面只在薄板穿过该高度的位置产生截面。例如 `z=0.225` 得到三条分离条带，中间留白符合几何。
不能把这些条带用一个大多边形连接，也不能用“非白轮廓内部不能有白色”验收非凸薄体。
判断是否缺片应以单元连接关系、实际相交范围、两侧体积和相同视角的 CPU/GPU 对比为准。

固定六个 X 区间时，增加正弦周期会改变节点采样，甚至产生混叠。例如三周期时节点落在 `sin(pi*i)=0`，离散网格几乎变成平板。
这属于源网格分辨率限制。要研究更高频率的连续波形，应增加 X 方向网格分辨率；不能通过给剖切结果补面来恢复未采样几何。

生成周期/相位变体，默认写入忽略的 `build/sine-plate/`，保留仓库基准数据：

```bash
python -B test/generate-sine-plate.py --cycles 2 --phase 0.31 --stem sine_2cycles
```

随后在 DRAW 中指定文件再打开示例：

```tcl
set ::sine_plate_file D:/workspace/OpenCASCADE/OCCT_Viewer/build/sine-plate/sine_2cycles.vtu
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_sine_plate_demo.tcl
```
