# Cut View Manager 自由体：配置、积分与迁移

## 1. 工作流与状态

Manager 的 **Free Body** 列切换所选 cut 的全部物理请求；首次点击打开配置。
**Free Body Options...** 打开时锁定 cut，之后改变 Manager 选择不会重定向窗口。
窗口默认请求 ID 为 `viewcut/<cutId>`，新请求默认取截面形心；旧 JSON 保持自定义原点默认值。
`Enabled` 保留配置并停止积分、释放报告/向量；`Overlay` 只控制向量。
这些操作不激活 cut，不修改 Below/On/Above 或云图绑定。隐藏 cut 保留物理配置，删除 cut 清理请求。

```tcl
scvcutfreebody model cut
scvcutview model physicsstate cut
scvcutview model physicsenabled cut 0
scvcutview model physicsconfig viewcut/cut
scvcutview model physicsreport viewcut/cut
scvcutview model physicscsv viewcut/cut
```

原有 `scvcutphysics` 高级面板继续可用。完整 session 保存请求 JSON（版本 2，兼容省略版本及版本 1）；definitions Save 不保存物理请求。

## 2. 积分域与 FE 字段契约

整体模型使用 `group=all`、空 `elements`。显式单元集使用名称和源单元 ID；空命名集合拒绝。
`domainField` 可绑定当前帧的 **CellData 标量标签**，`domainValue` 为相等标签；勾选 `domainMask`
则要求原始组件确为 `MeshGroupProperty`，值表示 0..31 的组位。普通整数标签不能伪装成分组掩码。
选择发生在原始 FE 单元上，云图恢复/平均不影响积分；即使选中集合与 cut 无交集也不会回退全模型。
形心模式对空截面报错，自定义点模式可以报告零合力。

`Capture` 从同一个 `MeshData` 的 `InteractiveCaeMesh` presentation 实际 `AcceptElement`
可见性捕获源单元快照，命令为 `physicsdisplaygroup requestId sourceMeshObject presentationId`。
这需要现有原网格对象；独立 cut 演示未创建该对象时，使用单元字段绑定或显式集合。
捕获后源显示变化需重新 Capture；它不是自动跟随另一对象的可见性。隐藏/空 presentation 明确拒绝。
显式 ID/显示快照/section mapping 跨重网格要求 `SetForStep` 的显式对应；标签字段按新 step 解析。

应力的 9 项组件映射是完整矩阵的行优先索引，允许通过重复索引展开明确的 Voigt 顺序；不得按字段名称猜测。
输入 field frame 与 output frame 独立，分别给出 `origin X Y Z` 共 12 个数字。
五个单位因子依次为长度、应力、热流密度、力、力矩到 SI 的正数，报告为 m/N/Nm/m²/W。
壳须明确厚度、截面方向；复合壳须明确每层字段、分量、厚度、偏置和局部坐标系；梁须明确面积或已积分的六分量截面合力/矩。
原始节点内力 NFORC、壳 SF/SM 与实体应力不是相同数据，不能把它们直接塞入应力张量入口。
ODB 读取和这些量的来源约定由数据适配层负责，本项目不解析 ODB。

## 3. 数值实现

`MeshCutPhysics` 使用独立 physical trace 的原始 FE 权重；实体面使用三点三角形积分，
壳线使用两点 Gauss、厚度/层偏置，线性梁使用连线交点。空间模式使用当前几何求交；
材料模式使用参考交点权重映射到当前构形，实体面积和局部法向由当前采样三角面给出。
材料梁保存参考连线参数，再插值当前坐标与物理值；明确的梁 frame X 必须沿当前梁轴。
壳截面 frame/层方向与随时间更新须由调用方提供，不能假定任意有限转动的 director 等于原始固定轴。

实体形心按面积，壳按线长乘总厚度，梁无面积时按连线交点求中心。
切换求和点使用 `M_new=M_old+(old-new)×F`；完成平移后再做绝对贡献求和。
`normalSign=+1` 表示 Below→Above，`-1` 反转应力牵引与热流符号。
输出 resolution 为 0=显式 frame、1=平均法向/投影切向、2=datum；法向互相抵消或切向平行时拒绝并保留旧报告。
NT 的平均方向由面积加权，曲面局部牵引仍按各积分面的法向计算。

默认每个 cut 一组合力/矩，箭头起点是实际求和点；Components 绘制其分量。
Distributed 显式启用后每采样点生成向量，超过 1024 点拒绝；大模型使用合向量或独立切片扫描。
数值结果、AIS 新向量先准备成功，再替换旧状态；失败不留下部分新配置。

## 4. 验证与运行

```powershell
cmake --build D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --parallel 6
ctest --test-dir D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation -R '^(cut_physics|native_analysis|native_session|draw_cut_view_free_body|draw_cut_view_physics|draw_cut_view_controls)$' --output-on-failure
```

按 [Linux 运行与测试指南](cut-view-manager-linux-run-test.md) 配置资源与库路径后：

```bash
# Release 直接运行；大数据至少 100 万单元，不挂调试器。
export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk-deformed/hex_150.vtu"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_free_body_large.tcl"
# Debug：先切换到指南中的 linux-debug 库/插件环境。
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_free_body_test.tcl"
# 测试脚本需要 CUTVIEW_OUTPUT_DIR 指向可写目录。
```

大数据脚本从 displacement 显式构造**仅供性能测试的矩阵映射**，不声称它是物理应力。
检查真实切面采样数量、默认 O(cuts) 向量、禁用释放、预算；命令批次时间不等于鼠标 FPS。

Windows Release 验收：15 项相关 CTest 通过，记录 `build/a04-section-tests.log`。
3,375,000 个六面体、每步 45,000 个积分面片，默认 2 个合向量；四步批次 697.012 ms，
source/result 预算为 `1,460,965,248` / `1,093,811,072` 字节，4/16 GiB 限额不变。
记录 `build/cut-free-body-large/run.log`，它不是鼠标 FPS，也不是外部 Abaqus 数值比对。
`EvaluatePhysicalSection` 专供该工作流按 On 求值；原 `EvaluatePhysical` 仍遵守原 visibility mask，
避免影响已有 trace/interface 调用者。平面数值截面的预算按 BVH 候选数和组件数计算，
不为 On-only 的三分量字段申请整个被切体的字段结果。

## 5. 无 OCCT 的迁移边界

VisIt 端保留请求、域适配、FE 权重、积分、frame 与 SI 契约即可。
将 `Graphic3d_Vec3d` 换成自有向量，`MeshCutResult` 换成含源 owner/节点权重/当前坐标/三角形与线的结构。
`BRepBuilderAPI_MakeEdge`/`BRep_Builder`/`TopoDS_Compound` 仅构造本项目的 AIS 箭头，
在无 OCCT 项目中直接写入渲染器线段缓冲；`AIS_InteractiveContext::Display/Remove` 换成视口内向量实例的发布/移除。
捕获显示组时读取目标工具真实的单元可见性集合，避免把空集合当 all。
目标算法不依赖 OCCT、外部 VTK 或 OCCT BVH；空间候选查询可使用迁移指南中的自研 BVH。
