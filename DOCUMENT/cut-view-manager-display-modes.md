# Cut View Manager：云图、线框与实时平面切割

## 1. 三种模式

| DRAW / Manager 的 Display 值 | C++ 模式 | 显示内容 |
| --- | --- | --- |
| `cloud` | `FlatShaded`（1） | 有限元标量云图；没有绑定字段时显示实体颜色 |
| `wireframe` | `MeshEdges`（0） | 深灰色网格边线，不绘制填充面 |
| `cloud_edges` | `ShadedMeshEdges`（2） | 云图叠加深灰色网格边线 |

Manager 的 Result 区新增 Display 下拉框，修改作用于所有 cut、所有 portion 和默认样式。只修改 mode，保留已有透明度、线宽、颜色及样式覆盖；在 DRAW 中切换后，Manager 会同步更新。`mixed` / `custom` 表示通过底层 `cutstyle` 设置了不同或其他样式，重新选择三种模式即可统一。

云图绑定与显示模式独立，切换线框不会取消 `temperature`。纯线框用统一边线颜色，不按场值染线，也不隐藏背面边线；标尺仍可显示已绑定字段。默认外表面线框不会绘制体网格内部所有单元的边，CPU slice 仍对完整体网格求交。

## 2. 可直接运行的 Tcl

使用本次构建的 `ViewDraw` 插件，以 Release DRAW 直接启动，在提示符执行：

```tcl
set ::env(OCCTV_DRAW_RUNTIME) D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-release/bin/viewdraw-runtime.tcl
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_display_modes_demo.tcl
```

若当前 DRAW 已经加载其他构建目录的旧插件，需要重启 DRAW，再设置上述 runtime；DLL 不能靠重新 `source` 安全热替换。

默认打开仓库自带的 6×6×1 正弦薄板，节点 `temperature=z`，初始为云图＋线框。要使用已有的 **337.5 万六面体**数据，在 `source` 前指定输入：

```tcl
set ::cut_modes_file D:/workspace/OpenCASCADE/OCCT_Viewer/build/large-vtk/hex_150.vtu
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_display_modes_demo.tcl
```

支持 `.vtk` / `.vtu`；大文件未生成时先按 [Linux 运行与测试指南](cut-view-manager-linux-run-test.md) 第 5 节生成。文件路径不是下载地址。示例会自动选择 `temperature` 或第一个节点字段，以 Y 中面建立 cut，打开 Manager 并启用切面手柄。

演示使用 1 像素线宽；可在 `source` 前设置 `::cut_modes_width`。大网格缩小时，粗线可能覆盖云图，应放大视图或减小线宽。已有模型可用 `scvcutview cut_modes cutstyle defaults 0 0 1 2` 设置负侧默认样式（依次为 portion、透明度、线宽、模式），对 `1/2` 两个 portion 同样设置；cut 自身若有样式覆盖，则将 `defaults` 换成 cut ID。

```tcl
scvcutview cut_modes displaymode cloud
scvcutview cut_modes displaymode wireframe
scvcutview cut_modes displaymode cloud_edges
scvcutview cut_modes displaymode
scvcutview cut_modes renderinfo

scvcutview cut_modes visibility cut 2
scvcutview cut_modes visibility cut 3
scvcutview cut_modes visibility cut 6
scvcutview cut_modes visibility cut 7
```

`2` 只显示截面，源体及其线框全部隐藏；`3` / `6` 保留单侧并补切面；`5` 显示完整源体，不裁剪也不生成独立切面；`7` 显示完整源体＋独立切面，源体不裁剪。云图填充会遮挡内部切面，纯线框可看见内部切面边线。选择 `2` 可以清楚查看切面网格。

## 3. 实时切割如何处理线框

单个启用平面、纯体网格、无向量图、不透明且三个 portion 使用同一种上述模式时，`renderinfo` 返回 `backend gpu_plane`：

1. **源体**：初次显示生成源外皮三角形和/或有限元边线，存入常驻 OpenGL 图元。片元着色器对三角形和线段使用同一局部平面方程做 discard。仅移动平面时，体网格和源体边线不重新上传。
2. **截面**：CPU 经 BVH 筛选体单元并求交，更新截面。填充面由多边形三角化；线框从每个截面多边形的周边生成，保留单元分界，**不画填充三角化产生的扇形对角线**。相邻单元可提交重合边，当前未额外焊接去重。
3. **叠加**：面和线分别使用 fill / line aspect。`Graphic3d_Aspects::SetPolygonOffsets(Aspect_POM_Fill,1,1)` 将填充面的深度稍向后偏，减少共面边线闪烁；不移动真实剖切几何。其深度偏移语义见 [OCCT Aspects 文档](https://dev.opencascade.org/doc/occt-7.5.0/refman/html/class_graphic3d___aspect_line3d.html)。
4. **同步**：截面三角形和线段分别保留 mutable buffer 容量。位置、法向、UV、有效顶点数、bounds 与平面一起发布；空截面必须将两类图元计数归零，防止残影。

切换显示模式可能首次创建相应的源体图元并上传；修改网格或变形也会重建。连续拖动的验收条件是 `body_builds` 保持不变且 `submitted_body_bytes=0`，截面上传量仍非零。这两个 submitted 计数是应用提交统计，**不是驱动实测 PCIe 流量**。

多 cut、非平面、透明度、向量以及其他显示模式仍走既有 CPU 路径。三个 portion 的 mode 不一致报告 `mixed_display_modes`；两侧线宽不同报告 `mixed_line_widths`。CPU 合并网格的实体线框会包含部分布尔/三角化边线，与 GPU 保留原有限元边线的画面可能不同。不要把这些拓扑边线差异误判为剖切缺面。

实现入口：[命令](../source-draw/ViewCaeTest/CutViewCommands.cpp)、[Manager](../test/cae/cut_view_manager.tcl)、[模式与源体缓存](../source/ViewCae/InteractiveCaeCutView.cpp)、[GPU 面/线与截面缓冲区](../source/ViewCae/MeshCutGpuPresentation.cpp)。对应 OCCT 能力为 `Graphic3d_ArrayOfTriangles / ArrayOfSegments`、`ShaderProgram / ShaderObject`、`AspectFillArea3d / AspectLine3d`、mutable attribute/index buffer、`Group` 与 `Structure`。迁移到 VisIt 时需以目标渲染器替换这些能力，目标仍完全不依赖 OCCT。

## 4. Linux 直接运行与调试运行

先按 [Linux 运行与测试指南](cut-view-manager-linux-run-test.md) 配置 `CUT_REPO`、`CUT_DRAWEXE`、OCCT / Tcl / Tk 资源与构建。以下默认打开薄板：

```bash
# Release：直接运行，不挂 GDB。
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_display_modes_demo.tcl"

# Debug：使用 Debug 插件与 GDB。
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_display_modes_demo.tcl"
```

GDB 中执行 `set breakpoint pending on`、`break ViewCae::MeshCutGpuPresentation::Update`、`run`。测帧率使用 Release 直接运行；调试器下的拖动时间不作为性能结论。Linux DRAW 中可先 `set ::cut_modes_file /absolute/path/hex_150.vtu`，再 `source $env(CUT_REPO)/test/cae/cut_view_display_modes_demo.tcl` 切换到大模型。

## 5. 回归与大数据拖动测试

启用 `OCCTV_BUILD_TESTS=ON`、`OCCTV_TEST_DRAW_VISUAL=ON`，并提供可用的 OpenGL 显示环境：

```bash
ctest --test-dir "$CUT_REPO/build/linux-release" \
  -R '^(cut_view|draw_cut_view_display_modes|draw_cut_view_gpu_plane|draw_cut_view_controls)$' \
  --output-on-failure

export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk/hex_150.vtu"
export CUTVIEW_BENCHMARK_FRAMES=60
export CUTVIEW_BENCHMARK_MASK=3
export CUTVIEW_BENCHMARK_MODE=wireframe  # 也可 cloud / cloud_edges
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/bench-wireframe-3375k"
# 此处恢复上节的 Release 环境。
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_drag_benchmark.tcl"
```

基准模拟实际鼠标拖动，验证每步同步一次、源体不重建/重传、释放与取消一致，输出 `drag.csv` 与首尾图像。分别用 `mask=2/3/7` 测试；增加线框会增加线段绘制和截面上传成本，不能保证与单纯云图帧率相同。

线框与叠加的基准默认使用 1 像素边线，可通过 `CUTVIEW_BENCHMARK_LINEWIDTH` 覆盖。测试继续检查叠加模式存在场值颜色，避免把被粗线完全盖黑的截面当作有效云图。

### 本次工程验证记录（2026-10-20）

Windows / MinGW Release，i7-1165G7，DRAW 1000×720，无 GDB，`hex_150.vtu` 共 3,375,000 个六面体、3,442,951 个节点；每组 24 个实际鼠标拖动步，边线为 1 px。FPS 包括事件处理、帧等待和 Manager 刷新；系统负载与 renderer 未固定，以下是本次样本，不是跨平台性能保证。

| 模式 | mask | FPS | p95（ms） | 拖动源体提交字节 |
| --- | --- | --- | --- | --- |
| cloud | 3 | 31.05 | 59.78 | 0 |
| wireframe | 2 | 32.81 | 56.38 | 0 |
| wireframe | 3 | 30.04 | 63.38 | 0 |
| wireframe | 7 | 29.82 | 60.48 | 0 |
| cloud_edges | 2 | 28.10 | 63.97 | 0 |
| cloud_edges | 3 | 21.86 | 78.46 | 0 |
| cloud_edges | 7 | 26.33 | 65.78 | 0 |

7 组均通过 `CUT_DRAG_BENCHMARK_PASS`，包含释放/取消和像素变化验证。原始日志、CSV 与图像在本地 `build/cut-modes-large/<模式>-<mask>/`；这些构建产物不纳入 Git。

6 项 CTest 通过：`cut_view`、`cut_display`、`draw_cut_view_gpu_plane`、`draw_cut_view_display_modes`、`draw_cut_view_controls`、`draw_cut_view_interaction`。另外直接运行演示脚本并触发三种 Manager Display 回调，验证通过。线框回归检查四边截面只有 4 条周边线、没有扇形对角线，模式切换与空截面没有遗留图元。Linux 命令已列出，当前环境未实际执行 Linux / CentOS 构建。
