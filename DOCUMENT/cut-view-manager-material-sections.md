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
| `MeshCutMaterialSampling.cpp` | 当前构形边中点/重心误差、共边细分、稀疏单元 Jacobian 检查和定向法向 |
| `MeshGeometryTraceSection` 材料构造函数 | 复制原始 FE 追踪与界面双侧来源，独立映射当前场 |
| `NativeCutTimeline` | 将固定参考配置和缓存传给候选 step；候选成功后原子替换视图 |
| `InteractiveCaeCutView` | 材料模式明确走 CPU presentation，空间模式仍使用现有后端 |

当前倍率或数据 step 改变时重建映射，参考交点继续复用。参考 step、参考倍率、定义方程或几何容差改变时重建参考交点。源几何显式失效会清除参考缓存。拓扑比较包括节点数、元素顺序、连接表和通用单元类型/面/阶次/有理权重；没有材料对应表的重网格会被拒绝。相同连接但材料 ID 已重新编号的数据须由导入适配层拒绝或重排，不能仅凭数组大小推定对应。

云图从当前 step 的原始字段按参考权重插值；不把 temperature 等字段重新解释成当前空间坐标函数。显示追踪和物理追踪继续独立选择。在当前构形重做世界空间半空间布尔会破坏材料分类，因此材料分支不进入该布尔；多分支显示仍有加性扩展的边界，不能声称已经完成 Abaqus 多 cut 兼容。

Definitions JSON 为 version 9，读取 1–9；新增 `material` 对象保存参考元数据，缺失参考帧、字段或拓扑不符时拒绝恢复。大规模节点权重保留在运行缓存中，不写入每次滑块快照；完整 session 随源数据保存参考绑定，加载后重建缓存。

参考结果和映射结果使用相同的源/结果预算。原生平面仅求 On 时，参考预算由同一 BVH 平面测试选出的候选单元上界计算，避免为每个参考截面预留整个模型的替代体网格；通用/高阶路径仍沿用原有保守预算。预算是逻辑分配上界，不是进程峰值内存。

仅显示原生材料截面时，当前帧不创建 BVH 或整模型节点缓存，其映射缓冲按实际截面节点计费；参考构形的完整源缓存继续计费。旧/新帧原子替换时可以同时保留，不需要提高 4/16 GiB 限额。

已为 On 截面与 Below/Above 的生成边界补充当前构形采样：每个三角形检查三条边的中点和面重心，比较原始 FE 映射位置与当前线性三角形的位置；误差超过几何容差时细分。新增点在参考单元内重新求 FE 权重，不能平均端点权重来代替高阶插值。每个 portion 内的相邻三角形共享已标记的边，使用 1→2、1→3 或 1→4 模板，避免内部悬挂边。原生路径保留替代体单元，重建其显式外表面；通用路径细分完整生成边界。壳线检查中点并二分。当前为**采样误差控制**，不是整个任意阶曲面上的严格误差证明。

三个 portion 统一以参考模型包围盒对角长度作为相对尺度，`max(absolute, relative × scale)`；沿用 geometry 的面/线深度及 primitive 上限。达到深度或预算而尚未满足容差时显式失败，不发布粗糙替代结果。`scvcutview model materialinfo cut` 返回 `tolerance/residual/depth/nodes/primitives`；`residual` 是验收采样点的最大残差，不是全域误差上界。多项式单元可用更严格容差复核收敛。不同 portion 具有独立网格，不声称跨 portion 顶点完全一致；解析夹具在公共边界的 1/4、1/2、3/4 位置检查到另一侧三角形的距离不超过 `2 × tolerance + float storage allowance`，不能把容差内近似写成全域零缝隙保证。

逐帧只检查相交单元。节点间的仿射映射直接使用稀疏节点与基向量，跳过逆映射；非仿射单元在内部和细分采样位置检查参考/当前 FE Jacobian 的秩和相对取向。原始位移含 NaN/Inf、样本退化或相对参考反演时拒绝。没有对整个模型所有未相交单元作质量认证，也不能保证任意高阶单元的采样点之间不存在局部反演。缺少参数图的非仿射通用单元会明确拒绝 Jacobian 检查，不能静默降级。

材料实体截面的应力/热流积分使用当前三角形面积和定向法向；参考平面法向仅决定 Below→Above 取向。新增二次四面体解析测试验证 `z=0.3+k·x²` 的曲率、内部共边和均匀 z 向热流的投影面积积分 `0.245`。壳仍需要当前局部厚度/层与 frame 适配，梁需要参考交点物理适配；不能把它们当成实体面积积分。A04 继续连接 Manager 的自由体工作流。

Follow Apply 先在共享预算的临时 manager 中验证参考截面，再修改现场；无效参考不会改定义。倍率/时间切换仍由候选显示对象完成后发布。失败细分的临时预算立即释放，回归覆盖多次重试不增长。相交单元退化/相对反演会失败；平面在模型外形成的空交集合法且清空旧截面，不扫描未相交单元作全模型质量认证。回归覆盖非仿射六面体、二次四面体、弯曲二次壳线、梁交点、8 种 mask、参考 First/Last/Current、参考侧别和坏帧不发布。A03 的工程支持边界是本页明确列出的平面材料切面、参数图和采样容差；A04 负责完整物理交互，A07 负责多 cut 兼容。

本阶段 Windows Release 的 19 项回归通过；337.5 万六面体材料截面门禁 `CUT_MATERIAL_LARGE_PASS`。三显示模式各 4 次倍率变化，参考求交始终为 1 次，每次映射 22,801 个截面节点；source/result 逻辑占用稳定为 `1,461,030,784` / `381,761,856` 字节。每组四步连同显示更新约 1.03–1.10 秒；该值不是鼠标 FPS，未覆盖大模型材料两侧实体的性能。产物为 `build/a03-tests.log`、`build/cut-material-large/run.log`。

加入采样验证后再次通过同一百万单元门禁：source/result 稳定为 `1,461,030,784` / `445,418,560` 字节，每步仍只映射 22,801 个点，参考求交为 1 次。该次与其他 GUI 回归运行时间有重叠，四步约 2.02–2.46 秒，仅作为缓存/预算验收，不能用于 FPS 或版本间速度比较。

A03 最终门禁在 GUI 回归结束后单独执行，cloud/wireframe/cloud_edges 的四步批次分别约 `1571/1703/2601 ms`；预算及参考复用保持相同。仍为整组命令耗时，没有替代鼠标交互 FPS 测量。最终 19 项回归与扩展实体/壳/界面测试通过；普通空间平面继续使用 GPU clipping，材料模式使用此处明确的 CPU 路径。

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
