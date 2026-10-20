# Cut View Manager：参考构形与随材料变形截面

## 1. 两种不同的切面语义

Spatial 是默认模式：当前节点穿过固定的空间平面，使用原有 GPU clipping + CPU slice。
Material 是 Follow model deformation：先在一个固定参考构形中求交，将截面节点的原始单元归属和有限元形函数权重保存下来。随后只按当前节点坐标映射这些参考交点：

```text
x(t, ξ) = Σ Nᵢ(ξ) [Xᵢ(t) + effectiveScale(t) uᵢ(t)]
```

`effectiveScale` 沿用本项目由模型尺寸归一化的倍率约定。参考构形本身也可以带位移，启用 Follow 时保存参考 step、位移字段名称和当时的 normalized scale。当前倍率变化不会修改参考倍率。

这与三节点 datum 的刚性 `follow cut datum` 不同。非均匀变形下，参考平面映射后可以成为折面或曲面的离散近似。材料模式禁止使用单个世界平面进行 GPU discard；`renderinfo` 会报告 `material_reference_cut`。普通空间平面和已有非平面 cut 的行为保持原有语义。

官方参考帧含义见 [Abaqus 创建切面帮助](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpcreate.htm)。这里没有 Abaqus 可执行环境，验收依据是解析夹具和原始有限元插值关系。

## 2. Manager 和 DRAW

选中平面，打开 **Definition...**，勾选 **Follow model deformation**，选择 First / Last / Current，再点击 **Apply follow**。该按钮单独提交跟随配置，不重置平面坐标系。普通 Apply 仍编辑参考构形中的定义原点和轴。

First/Last/Current 在 Apply 时立即解析成固定 step ID。切换时间步不会使 Current 随着播放移动。无时间序列时三者都解析为 `-1`。已建立刚性 datum 跟随的 cut 不能同时启用材料跟随。当前材料入口仅开放给平面。

```tcl
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_deformation_demo.tcl
# 先定义参考状态；例如以未变形几何建立参考截面。
scvcutview cut_modes deform none 0
scvcutview cut_modes material cut 1 Current
scvcutview cut_modes deform displacement 0.3
scvcutview cut_modes materialstats cut
scvcutview cut_modes list
# 返回默认空间平面。
scvcutview cut_modes material cut 0 Current
```

`list` 包含 `material/reference_step/reference_scale/reference_displacement`。`materialstats` 的 `referenceBuilds` 按实际首次求交的 portion 计数；第一次显示 Below 或 Above 会增加计数。`mappingBuilds/mappedNodes` 统计当前构形的映射量，不能把它们当作整模型求交次数。

## 3. 实现与缓存边界

| 代码 | 责任 |
| --- | --- |
| `MeshCutViewManager` | 捕获参考配置、校验对应关系、JSON、当前配置失效 |
| `MeshCutMaterialCache` | 共享参考 manager、按 portion 延迟保存参考结果 |
| `MeshGeometryMaterial` | 保留参考的两侧分类，映射生成节点和原始 FE 来源 |
| `MeshGeometryTraceSection` 材料构造函数 | 复制原始 FE 追踪与界面双侧来源，独立映射当前场 |
| `NativeCutTimeline` | 将固定参考配置和缓存传给候选 step；候选成功后原子替换视图 |
| `InteractiveCaeCutView` | 材料模式明确走 CPU presentation，空间模式仍使用现有后端 |

当前倍率或数据 step 改变时重建映射，参考交点继续复用。参考 step、参考倍率、定义方程或几何容差改变时重建参考交点。源几何显式失效会清除参考缓存。拓扑比较包括节点数、元素顺序、连接表和通用单元类型/面/阶次/有理权重；没有材料对应表的重网格会被拒绝。相同连接但材料 ID 已重新编号的数据须由导入适配层拒绝或重排，不能仅凭数组大小推定对应。

云图从当前 step 的原始字段按参考权重插值；不把 temperature 等字段重新解释成当前空间坐标函数。显示追踪和物理追踪继续独立选择。在当前构形重做世界空间半空间布尔会破坏材料分类，因此材料分支不进入该布尔；多分支显示仍有加性扩展的边界，不能声称已经完成 Abaqus 多 cut 兼容。

Definitions JSON 为 version 9，读取 1–9；新增 `material` 对象保存参考元数据，缺失参考帧、字段或拓扑不符时拒绝恢复。大规模节点权重保留在运行缓存中，不写入每次滑块快照；完整 session 随源数据保存参考绑定，加载后重建缓存。

参考结果和映射结果使用相同的源/结果预算。原生平面仅求 On 时，参考预算由同一 BVH 平面测试选出的候选单元上界计算，避免为每个参考截面预留整个模型的替代体网格；通用/高阶路径仍沿用原有保守预算。预算是逻辑分配上界，不是进程峰值内存。

仅显示原生材料截面时，当前帧不创建 BVH 或整模型节点缓存，其映射缓冲按实际截面节点计费；参考构形的完整源缓存继续计费。旧/新帧原子替换时可以同时保留，不需要提高 4/16 GiB 限额。

本阶段先完成参考交点与 FE 权重映射。现有离散截面的节点可以正确随高阶 FE 权重移动，但这不等于当前曲面误差已经受控：后续还须按当前构形补充细分与误差验收。材料自由体积分目前要求显式当前局部法向；旧的固定平面简化积分拒绝材料结果，防止对弯曲截面静默使用旧法向。曲面细分、反演/退化检测、局部法向和更多参考/当前配置联验仍属于 A03/A04 的后续工作，不将当前阶段写成全部完成。

本阶段 Windows Release 的 19 项回归通过；337.5 万六面体材料截面门禁 `CUT_MATERIAL_LARGE_PASS`。三显示模式各 4 次倍率变化，参考求交始终为 1 次，每次映射 22,801 个截面节点；source/result 逻辑占用稳定为 `1,461,030,784` / `381,761,856` 字节。每组四步连同显示更新约 1.03–1.10 秒；该值不是鼠标 FPS，未覆盖大模型材料两侧实体的性能。产物为 `build/a03-tests.log`、`build/cut-material-large/run.log`。

## 4. 构建与测试入口

Windows 先配置 [运行环境](cut-view-manager-deformation.md)，然后：

```powershell
cmake --build D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --parallel 6
ctest --test-dir D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation -R '^(cut_material|native_timeline|native_session|draw_cut_view_material)$' --output-on-failure
& 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe' -i -f 'D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_material_large.tcl'
```

Linux 按 [Linux 指南](cut-view-manager-linux-run-test.md) 配置资源变量后，直接运行本项目：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/material-test"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_material_test.tcl"
export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk-deformed/hex_150.vtu"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_material_large.tcl"
```

Linux 调试本项目：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_material_test.tcl"
# GDB 内：set breakpoint pending on，然后 run。
```

Linux 命令尚需目标平台执行。迁移到 VisIt 的产物不使用 OCCT：参考交点、权重映射和预算协议可以直接移植，向量、渲染对象和 BVH 实现按 [无 OCCT 迁移手册](cut-view-manager-visit-porting.md) 替换。
