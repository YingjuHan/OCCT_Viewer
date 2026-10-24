# Cut View Manager：平面坐标系与绝对运动

## 1. 状态与坐标约定

本项目的平面求交仍使用单位法向方程 `n·x+d=0`。A02 增加 `MeshCutPlanePlacement`，保存定义原点 `O`、定义法向 `N`、平面内单位轴 `A2`；右手第三轴是 `A3=N×A2`。原点不必是平面到世界原点的垂足，旋转时必须使用用户指定的原点。

方向输入可以不是单位向量，但必须非零、有限且彼此垂直；Define 会归一化，平行或不垂直的输入会被拒绝。持久化状态校验单位长度、正交性、轴号及数值有限性。长度使用模型单位，角度使用度。

运动保存 `Offset`、`Angle`、`RotationAxis`：

- `absrotate` 根据原始局部 2/3 轴，通过 Rodrigues 公式直接计算当前法向；不会在上一次结果上累加。旋转清除 Offset，围绕定义原点进行。
- `translate` 设置绝对 Offset，沿当前法向移动；保留已经设置的旋转角度和旋转轴。
- 设旋转后的法向为 `n`，求交方程为 `d=-n·O-Offset`，当前手柄原点为 `O+n·Offset`。
- 切换 Translate/Rotate 面板只切换控件，不改变几何。实际输入或拖动才提交运动。

此处依照 [Abaqus 位置调整规则](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpposition.htm)实现绝对旋转、忽略已有平移以及非激活项的输入行为。没有在 Abaqus 可执行环境做双软件对照，不能据此宣称全部功能已经等价。

## 2. Manager 操作

选中平面后，点击 **Definition...**：

1. **Key-in** 输入 Origin、Normal (axis 1)、In-plane axis 2。
2. **From CSYS** 选择 Viewer Tools 中定义的 datum 和法向轴 1/2/3；`global` 始终可选。切面轴按循环顺序排列：选择 CSYS 2 为法向时，局部 2/3 轴分别是 CSYS 3/1。
3. Apply 建立新的定义坐标系并重置角度与位移。非法输入保留现场。对话框固定绑定打开时的 cut；模型被替换后需重新打开。

From CSYS 在 Apply 时取 datum 的快照。改变 datum 后需要再次 Apply；这里没有自动建立三节点材料跟随绑定。既有时间轴的显式 `follow` 刚性坐标系绑定仍可用，并保留完整 Placement；真正随有限元非均匀变形的材料截面属于 A03。

**Position** 现在是相对于定义原点的 Offset，不是世界方程的 `-d`。选 Rotate 后同一个输入框显示 **Angle**，轴选择是局部 2/3。旧 JSON 没有历史原点，迁移后当前位置成为 Offset=0；这是信息缺失时的确定性约定。

Sensitivity 可选 1/10/100。1 使用模型投影范围，10/100 以当前位置为中心收窄到原始范围的 1/10、1/100；改变灵敏度不移动切面。平移与旋转滑块在 cut 未启用时禁用，数值输入仍可编辑，且不会自动启用 cut。拖动两种滑块时按 Escape 恢复完整定义快照。

鼠标手柄使用当前 Placement 的原点、法向和面内轴。操纵器对完整定义坐标系施加刚体变换，相当于重定位定义坐标系；取消恢复拖动前完整状态。平面辅助图形的法向拖动保留面内方向。旧 `move A B C D` 对平行移动保留 frame；显式更改法向时投影原 pivot 到新平面、运输面内轴并重定义 frame。需要精确指定基准时请用 Definition/placement。

## 3. DRAW 接口与兼容

```tcl
# 已在 DRAW 中加载演示模型后执行。
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_demo.tcl
scvcutview cut_view_demo placement x 0.4 0.3 0.2 1 0 0 0 0 1
scvcutview cut_view_demo translate x 0.05
scvcutview cut_view_demo absrotate x 2 30
# 上一行清除 0.05 平移；再沿旋转后的法向平移。
scvcutview cut_view_demo translate x 0.08
scvcutview cut_view_demo list
scvcutview cut_view_demo motionrange x

scvcutview cut_view_demo datum fixture 0.2 0.3 0.4 0 1 0 -1 0 0 0 0 1
scvcutview cut_view_demo datums
scvcutview cut_view_demo fromcsys x fixture 2
```

`list` 中平面新增 `origin/normal/axis2/offset/angle/rotation_axis`。`range` 继续返回旧的世界法向投影范围；`motionrange` 返回相对于定义原点的可用 Offset。旧 `rotate id axisIndex degrees [pivot]` 仍按全局 X/Y/Z 增量旋转，不能与新 `absrotate` 混用语义。

本节记录 A02 当时的格式：Definitions JSON 升到 **version 8**，平面新增 `placement` 对象，同时保存派生平面方程。读取时校验二者一致，拒绝不完整或矛盾文件，失败不会替换现场。版本 1–7 使用原方程的最近原点和确定性垂直轴迁移，无法恢复未保存的历史切向姿态。Definitions、完整 Session、Duplicate 和时间帧切换均携带新状态；当时外层 Session 为 version 3。当前格式已为 definitions 13 / session 4，迁移规则见 [会话指南](cut-view-manager-session-scope.md)。

## 4. 代码入口与移植

| 入口 | 职责 |
| --- | --- |
| `source/ViewCaeGeom/MeshCutPlanePlacement.h`、`MeshCutFrame.cpp` | 纯向量平面状态、绝对运动、datum 坐标转换 |
| `MeshCutViewManager::{SetPlanePlacement,TranslatePlane,RotatePlane}` | 校验后沿原有缓存更新路径发布派生方程 |
| `MeshCutViewManager::{DumpDefinitionsJson,RestoreDefinitionsJson}` | version 8 与旧版原子迁移 |
| `source/ViewCae/InteractiveCaeCutPlane.cpp` | OCCT 定位辅助与完整 frame 的鼠标变换 |
| `source/Viewer/NativeCutTimeline.cpp` | 跨 step 携带 frame，刚性跟随时变换原点和各轴 |
| `test/cae/cut_view_placement.tcl`、`cut_view_manager.tcl` | 定义编辑器、绝对运动、滑块范围与取消 |

此包不改求交和空间加速算法。相同方程的 frame 编辑不触发几何重建；法向/位置变化仍通过原有 SetPlane 内部路径更新缓存。GPU clipping、CPU slice、源 BVH、云图与线框复用策略保持原有适用范围。无 OCCT 的迁移目标可用普通 double 向量/矩阵替换 `Graphic3d_Vec*` 和 `gp_*`，并在目标渲染器实现手柄；详见 [迁移手册](cut-view-manager-visit-porting.md)。

本轮还处理了安装 SDK 的拾取边界缺陷：`AIS_ClippingPlanes::ComputeSelection` 在包围盒截交多边形超过四个顶点时调用 `GetPlaneTriangles`；当前 SDK 的 `AddTriangleFanEdges` 从 `lower+1` 开始，额外生成一个退化三角形，写入数超过预留索引，抛出 `TOO many EDGE`。它会使手柄更新中断，表现为新方程已提交、画面仍停留旧 revision。项目派生类现在重写 `ComputeSelection`，沿用 OCCT 的包围盒截交多边形、拾取偏移和 `AIS_ClippingPlanesOwner`，显式生成恰好 `n-2` 个三角形；未修改外部 OCCT。该修复只针对定位辅助拾取，不能解释为有限元 slice 算法发生改变。

## 5. 构建与运行验收

Windows 使用独立 Release 构建，运行环境变量见 [形变指南](cut-view-manager-deformation.md)，不用 GDB 进行性能验收：

```powershell
cmake --build D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --target ViewDraw cut_placement_tests native_timeline_tests native_session_tests --parallel 6
ctest --test-dir D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation -R '^(cut_placement|native_timeline|native_session|draw_cut_view_placement)$' --output-on-failure
& 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe' -i -f 'D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_placement_large.tcl'
```

Linux 首先按 [Linux 运行与测试指南](cut-view-manager-linux-run-test.md) 配置 `CUT_REPO/CUT_OCCT_LIB/CUT_DRAWEXE` 等变量和资源目录。直接运行本项目的语义测试与大数据验收：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_placement_test.tcl"
export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk-deformed/hex_150.vtu"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_placement_large.tcl"
```

调试本项目的 Manager：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_demo.tcl"
# GDB 内：set breakpoint pending on，然后 run。
```

Linux 命令尚未在本轮 Windows 工作机执行。`cut_view_placement_large.tcl` 强制至少 100 万单元，测试变形倍率 0.1、三显示模式 × mask 2/3/7，以及旋转/平移期间主体上传为 0、源准备次数和缓存预算稳定。其 elapsed_ms 包含渲染与脚本查询，不能当成鼠标 FPS。
