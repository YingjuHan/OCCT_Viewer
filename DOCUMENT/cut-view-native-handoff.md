# Cut View Manager 原生开发接续说明

## 1. 接续范围与当前停点

用户已在 Windows 工作区发出继续开发指令。N02 已按文档范围闭环并提交；
N03.1 恢复控件/定义持久化、N03.2 张量、N03.3 双侧来源/独立选侧及
N03.4 实际 OR 并集/透明显示均已验收。N03.5 完整双配置闭环审计通过，
N03 已按 [原生支持契约](cut-view-n03-audit.md) 闭环。最新证据在第 23 节，
旧 milestone 记录保留为历史。N04 导入与普通 Viewer 已启动，见 [N04 队列](cut-view-n04-plan.md)；
N05–N09 尚未启动。最新 N04 证据在第 24 节。
后文的 Linux 验证记录保留为历史证据；
本次 Windows 配置与结果以本节及第 6 节的接续记录为准。

- 仓库：`D:/workspace/OpenCASCADE/OCCT_Viewer`；分支：`develop`；初始接续基线：`95fb291`。
- 目标：完成原生 Cut View Manager 的 N02-N09 及对应 TODO，实施、修复并通过 native/DRAW 验收。
- 排除：Web/Vue/TypeScript、浏览器测试、WASM SDK 与打包。排除项不是 done。
- 上轮增量 `d02dd2e`、N02.1/N02.2 `7c925f2`、N02.3 `f975b86` 已提交；
  N02.4 `7910a48`、N03.1 `2caeb2f`、N03.2 `17f421c` 已提交；
  N03.3 `f719eeb`、N03.4a `db6268b`、N03.4b `addbb03`、N03.4c `eba2a37` 已提交；
  曲面 cap/内部面修复 `3a1fd44`、N03 闭环文档 `6bacc14` 已提交；
  N01–N03 已按支持范围闭环，N04.1 诊断/导入发布基础已验收；N05–N09 尚未实施。
- 每个验收工作项单独提交；保留用户既有修改及 `AGENTS.md`。

接续时以本说明的最新证据为准。主交接、执行队列和设计中的当前 N03 状态为 verified；
历史 milestone 的 31/34 cases 记录保留，不代表当前 Windows 套件计数，也不能据此把未完成阶段改为 done。

相关规划与说明：

- [完整需求与 TODO](cut-view-development.md)
- [任务依赖和执行队列](cut-view-execution.md)
- [原生设计、模块职责和契约](cut-view-native-design.md)
- [VTK 单元支持矩阵及限制](vtk-cell-support.md)
- [构建、DRAW 测试说明](../test/README.md)
- [OCCT SDK 修复补丁](occt-cut-view-sdk.patch)

## 2. 不可退化的要求

1. 平面、球、圆柱截面是与原始体网格/面网格的几何求交。不得以场等值面替代几何截面。
2. 鼠标按住拖曳期间必须更新几何、原始 FE 插值、云图和显示版本；不能只更新切面辅助图形或等松开再计算。
3. 负侧/切面/正侧对应位掩码 `1/2/4`，八种组合包含 `0`。多个 cut 独立定义，不能意外变为串联 AND 裁剪。
4. 非原始 node 上的结果来自原始单元形函数。高阶权重可以为负或大于一，不能按线性插值习惯截断。
5. element-node 数据必须保留原始单元来源及不连续值；不能通过全局坐标焊接抹掉材料跳变。
6. 显示恢复与物理积分必须分离。颜色平滑不能改变原始应力、热流等物理结果。
7. 使用现有 `VIEWDRAW` 插件，保留一个 VIEWDRAW launch 配置；不新增 `CUTVIEW` 插件。
8. Tcl 脚本放在 `test/`，新增控件必须扩展实际交互测试与控件清单，不能只测试命令调用。
9. 保留 `AGENTS.md` 和用户既有修改；不要 reset、覆盖、删除或批量暂存不相关文件。
10. 小模型截图、几何耗时及 48 类型矩阵通过，不等于所有数据均无 bug，也不等于满足百万单元全帧指标。

结构探索使用安装的 `codebase-memory` skill。当前 MCP project 为
`D-workspace-OpenCASCADE-OCCT_Viewer` 与 `D-workspace-OpenCASCADE-OCCT_private`。
每个依赖文件检查 index coverage；C++ 模板/Handle 相关解析不完整或关系不可信时，
读取对应源码。恢复相关文件存在 partial/unusable/metadata_changed，已直接核对源码。

## 3. 当前 Windows 环境与构建位置

| 项目 | 路径或配置 |
| --- | --- |
| OCCT 源码 | `D:/workspace/OpenCASCADE/OCCT_private` |
| 当前 OCCT SDK | `D:/workspace/OpenCASCADE/OCCT_private/install/mingw` |
| OCCT SDK/DRAW 版本 | 7.7 系列，MinGW Debug，Tk grid 与 drag reset 修复已重建安装 |
| 原生 VTK SDK | `D:/workspace/OpenCASCADE/3rdparty/mingw64/vtk-9.5.2-mingw64` |
| VTK CMake package | 上述 SDK 下 `lib/cmake/vtk-9.5` |
| VTK 源码/构建 | `D:/workspace/VTK/vtk-9.5.2` / `build-9.5.2-mingw64` |
| VTK 官方数据 | `D:/workspace/VTK/test-data-9.5.2`，六个 SHA512 校验文件 |
| 本项目当前构建 | `build/mingw`，MinGW Debug，VTK ON，DRAW/visual ON，Web OFF |
| 无 VTK 验证 | `build/mingw-no-vtk`，MinGW Debug，VTK OFF / visual OFF；8/8 各三次通过 |
| 产物 | `build/mingw/bind/`、`build/mingw/libd/` |

使用自定义 OCCT SDK。Windows 应用现有补丁中的 `AIS_ViewController.cxx` 和
`Draw_GraphicCommands.cxx`；Linux X11 callback 改动不适用于 Windows。
核对实际安装库，不能只检查源码。VTK 的 MinGW 导出补丁和可复现配置见
[vtk-9.5.2-mingw.md](vtk-9.5.2-mingw.md)。已有 VTK 7.1.1 checkout 未改动，
外部 VTK fixtures 未修改。

## 4. 已有架构与实现入口

| 文件或模块 | 职责与接续注意点 |
| --- | --- |
| `source/RWCae/MeshData.{h,cpp}` | 原始拓扑、GenericCell、原始节点/单元 ID、字段与高阶元数据 |
| `source/RWVtk/` | Legacy/XML/composite/series 读取，原生 VTK fallback、导入报告 |
| `source/RWMeshConv/MeshLoader.cpp` | 上层导入入口，后续结构化诊断和相对引用 staging |
| `source/ViewCaeGeom/MeshCutViewManager.{h,cpp}` | cut 定义、生命周期、源 epoch、几何和字段失效、独立 Evaluate |
| `MeshVtkCutSource.{h,cpp}` | 原始单元 evaluator、保守 BVH、lazy OCCT shape、共享面邻接 |
| `MeshVtkCell.{h,cpp}` | 原始 VTK 形函数、逆映射、曲面/曲线离散、来源权重 |
| `MeshVtkAnalytic.{h,cpp}` | 齐次 Bernstein 转换、OCCT Bezier 曲线/曲面与显式 fallback |
| `MeshGeometryVtkCut.{h,cpp}` | 一般单元 plane/sphere/cylinder 几何求交、三部分边界网格 |
| `MeshGeometryQuadric.{h,cpp}` | 原生简单单元球/圆柱路径 |
| `MeshCutRecovery.{h,cpp}` | 仅用于显示的 region/threshold/element-node 恢复 |
| `MeshGeometry.cpp`、`MeshGeometrySlice.cpp` | 几何分支字段映射、FE 权重及面网格截线属性 |
| `MeshCutIntegration.{h,cpp}` | 原始物理场截面力、力矩、热流；目前主要为平面体单元 |
| `source/ViewCae/InteractiveCaeCutView.*` | AIS 云图、三部分共享样式、点和截线绘制、显示版本同步 |
| `source/ViewCae/InteractiveCaeCutPlane.*` | 辅助平面/曲面、真实鼠标操作、操纵器和 abort |
| `source-draw/ViewCaeTest/CutViewCommands.cpp` | `scvcutview` DRAW 接口和有效输入验证 |
| `test/cae/cut_view_manager.tcl` | 原生 DRAW/Tk manager，后续扩展现有面板 |
| `source/Viewer/` | 后续 native Viewer API、CAD 同步、时间与 session 编排 |

保持职责分工，不把求交、物理映射或会话解析塞进 Tk 事件回调。类布局变更后，必须重建所有引用这些类的测试和 VIEWDRAW，避免旧二进制造成伪崩溃。

## 5. 已完成的基础能力

- R00：真实 held-drag 的几何/场/像素同步、abort、无效输入、DRAW/Tk 崩溃修复和既有 32/32 控件检查。
- G01：原生一阶场等值面，独立 field/component/step、below/on/above、threshold 编辑与定义迁移。它是独立功能，不代替几何切面。
- M01：默认未激活 XYZ 平面、选择不隐式激活、模型 open/replace/close、拖曳期间替换/关闭清理。
- N01：manager 共享 source epoch、保守 OCCT BVH、lazy source shape、可信内部面隐藏与确定性共享面来源。
- N02 数值基础：绝对/相对容差、细分与 primitive 限额、匹配曲面邻接、原始 FE 插值及收敛参考。
- N02 解析增量：支持的多项式/有理边界转换为 OCCT Bezier；numerical/preferred/required 模式；曲面面积/体积使用自适应 OCCT 积分。
- 可选 VTK 路径：当前 SDK 48 个 concrete cell 类型的规范数值/DRAW 矩阵、六个真实有效 fixture 和两个无效 fixture；支持高阶次数、Bezier 权重及不限于 8-bit 的连接长度。

解析模式覆盖的是可转换的边界 chart，并非所有任意阶几何均精确。转换目前对支持的 chart 有 degree-six 上限；数值 fallback 明确报告，required 模式拒绝不支持的转换。输出显示坐标仍为 float，不能用更小容差声称消除了存储精度限制。

## 6. 最新增量与验证证据

### 6.1 历史 N03 恢复基础（当前状态见第 20–22 节）

新增 `MeshCutRecoveryOptions`：

- `ThresholdPercent`：有效范围 `[0,100]`。
- `Regions`：空表示一个区域，否则每个原始 element 一个 signed 64-bit ID。
- `InvariantsBeforeAveraging`：明确平均前后顺序；当前重点验收向量模长，尚不能宣称实现全部张量不变量。
- `RecoverCutField()`：输出 elemental-per-node `MeshNodeVecProperty`，保留区域边界、被阈值拒绝的跳变和缺失值。
- 阈值的当前算法：逐分量比较共享节点同 region 的局部 spread 与该分量全局有限值范围乘百分比。它是本项目当前契约，不能未经核对就声称与 Abaqus 阈值完全等同。
- 默认 `100/no regions/after` 的 scalar-component 字段沿用旧 nodal projection；原始 NodeVec 输入以及自定义策略走恢复函数。

`MeshCutResult` 新增 `PhysicalFields[3]` 和 `DisplayRecovered`。显示场与原始物理场分开映射；积分优先使用 `PhysicalFields`，若 recovered result 缺失该映射则拒绝，不回退到平滑显示值。

线与点的渲染读取补充 NodeVec subnode 索引。`CalculateLineProperty()` 按截线来源单元及 `OriginNode` FE 权重映射 element-node 值。NodeVec FE 插值不再遇到权重接近一或超过一就提前结束，并跳过零权重以避免无关 NaN 污染。

DRAW 新接口：

```tcl
scvcutview model recovery
scvcutview model recovery 100 0 material
scvcutview model recovery 0 0 none
scvcutview model recovery 100 1 none
# region field 可带 step；无效/小数 ID、错误 tuple 数和歧义绑定拒绝
scvcutview model recovery 100 0 material 0
```

`eval` 对原有 nodal/constant 输出保持兼容，新增 `valueMapping`。element-node 的 `values` 为 `{element node x y z value}`；nodal 为 `{x y z value}`，constant elemental 仍为标量列表。不要把三类输出混读。

新增测试与 fixtures：

- `test/native/CutRecoveryTests.cpp`，target `cut_recovery_tests`，CTest `cut_recovery`。
- `test/cae/cut_view_recovery.tcl`，CTest `draw_cut_view_recovery` 与 visual variant。
- `test/native/fixtures/recovery_hex.vtu`、`recovery_surface.vtp`。
- 验证相邻单元 7/9 的默认平均和材料跳变、阈值 0/99/100、平均前后向量模长、缺失值/无效参数、几何复用、原始 NodeVec 输入、截线映射及 signed FE 权重。
- 平面 `x=.25` 的显示投影为 7.25，但单位面积原始热流仍为 7；力为 7，关于原点的相应力矩为 `+3.5/-3.5`，所有显示策略保持不变。
- Visual 用真实鼠标 held motion 检查同步、7/9 跳变、变化的像素与 abort 回滚。

VTK role 注意：三分量数组不会仅凭 component 数自动视为 vector。测试通过 `CellData Vectors="opposing"` 明确绑定向量角色，避免把 raw component 0 误认为模长。

该历史基线当时仍缺完整 tensor/trace、OR union 及恢复控件；这些门槛现已分别
通过 N03.1–N03.4。恢复设置的完整 session 持久化仍属于 N09。

### 6.2 N02 曲面家族参考扩展

`test/native/CutAnalyticTests.cpp` 最后补充并重建了以下独立参考：

- 曲楔体类型 `26/31/32/73/80`，及 non-unit rational Bezier wedge `80`。
- 非有理曲楔体体积参考：`1/2 + h/24`，其中 Bernstein control-point fixture 的等效 bow 系数与 Lagrange fixture 不同。
- Rational wedge 使用独立齐次 Bernstein moment 推导与 Simpson 积分，不用被测求交面积/体积作为期望值。
- 曲金字塔 `27/37`：二次横向 shear，截面面积 `1/4`，负/正体积 `7/24`、`1/24`；截面边界 shift 为 `0.1`，另检查原始二次字段。
- 新增楔体/金字塔和原有 tetra/hex/curve/polygon 用例一起运行，单次全部通过。

Windows 接续已收尾 fixture 的 double-to-float 显式转换。上述八个家族现在生成
独立 VTU fixture，由 `cut_view_analytic.tcl` 重新导入，检查解析模式、面积/体积、
原始 FE 场和 constant CellData。最终重复验收结果应使用下节的 Windows 记录，
不能把原 Linux 单次运行当作扩展后的三次全量证据。

### 6.3 Windows 恢复稳健性增量

- `MeshCutFieldTools.h` 共用字段验证、稳定均值、范围阈值和元数据复制。
- `Evaluate()` 与 recovery 验证所有 raw components，包含最后物理分量和 cached magnitude；
  null、错误 storage/mapping/tuple count、未加载组件拒绝。
- 默认投影和 element-node recovery 避免有限大数求和溢出；阈值比较支持相反符号极值
  及有限局部 spread/溢出全局 range。NaN trace 保留，infinity recovery 输入拒绝。
- `MeshNVecProperty::UpdateComponents()` 修复 vector magnitude offset 导致末分量未验证；
  scalar/nodevec 模长改用 `hypot`，验证 `1e200` 和 `1e-200`。
- field/time/component/magnitude 的 name、description、units、enabled 元数据保留。
- MinGW DLL 的 reader/base/slice 构造析构导出与 Windows `near` 宏冲突已修复；
  CTest 自动设置 DLL PATH、shader 和 DRAW plugin resources。
- 两个批处理 Tk 滑块测试改为处理完整事件队列，确保 Windows command 回调已执行；
  平面和等值面交互验证已通过。
- 无 VTK 配置：全部 8 项各连续三次通过，4.56 秒，日志
  `build/mingw/cut-native-build.log`、`cut-native-tests.log`。
- VTK-enabled：全部 32 项各连续三次通过，232.34 秒。包含扩展后的八类曲面
  native/DRAW 参考、48-cell 矩阵、真实 held-pointer/pixel、缓存、恢复和 32/32 控件。
  日志 `build/mingw/cut-vtk-build.log`、`cut-vtk-tests.log`；当前 cache 保持 VTK/visual ON。
  六个官方外部文件已校验；历史套件另外两个 fixture 缺失，不能称为旧 34 项完整复现。
- Windows Tk 控件单次完整通过，32/32，包括模型替换/关闭中的拖曳取消。
  测试使用真实 Tk 文件/确认对话框；Windows shell 对话框不在自动验收范围。
  Debug 功能测试 Windows timeout 为 300 秒，不是交互帧性能指标。
- `draw_cut_view_recovery_visual` 通过；已查看当前 `draw-recovery/held_3.png`
  与 `lines.png`，非空截面、材料两侧跳变及截线可见。
- 同时已查看 `draw-analytic/held_3.png`、`draw-vtk-cells/cell_12_held_2.png`，
  解析截面和 VTK 拖曳云图非空，场梯度可见。上述目录均位于 `build/mingw/test/native/`。

### 6.4 N02 独立参考与解析精度边界

- 执行顺序见 [N02 接续清单](cut-view-n02-plan.md)：先完成参考和精度门槛，再做高阶场等值面及闭环审计；随后恢复 N03 新功能。
- 新增 `cut_reference`、`draw_cut_view_references` 及 visual 变体，生成 24 组有效曲面家族/阶数参考和两组 unsupported fixture。
  曲线覆盖 21/35/68/75，曲面覆盖 22/23/28/30/34/69/70/76/77，新增体参考为 71/78；
  `cut_analytic` 增加混合阶 hex 33。几何 measure 和原始 FE scalar 使用独立多项式/Bernstein moments。
- 原生检查所有生成节点的非线性 FE 值、partition of unity、移动切面 source reuse 及曲面全部八个 masks；
  新增三角曲面、有理 quad 和有理 tetra 的 numerical measure 收敛检查。
- 验证 degree six、各向异性 2x3，以及 rational weights 整体乘 `1e-200`/`1e200` 后的相同几何/FE 参考。
- 修复 normalized positive weight `<=1e-12` 时 preferred 路径被端点采样异常中断的问题：
  转换前识别解析精度不支持，preferred 可显式 numerical fallback，required 原子拒绝。
  fallback 仍受细分/primitive 预算约束；极端权重 fixture 的粗网格只验证回退发布，不证明精度。
- DRAW 检查几何 plane、独立面积/体积、非线性 FE 值、CellData 和 line field；
  held-pointer 检查 release 前 revision/pixels、source reuse、abort 回滚。
- VTK-enabled **35/35** 每项各连续三次通过，105 次通过，635.41 秒；无 VTK **8/8** 每项各连续三次，24 次通过，7.75 秒。
  无 VTK 使用独立 `build/mingw-no-vtk`，当前 `build/mingw` 保持 VTK/visual ON。
- 最终 visual 用例只显示 negative+section，让实际云图裁剪边界随拖曳变化；测试调整后该项另连续三次通过，21.55 秒。
  已查看最终 `draw-references/before.png`、`held_3.png`。控件仍为 32/32，通过完整重复回归。
- 日志：`build/mingw/cut-n02-build.log`、`cut-n02-vtk-tests.log`、`cut-n02-reference-visual-tests.log`；
  `build/mingw-no-vtk/cut-n02-build.log`、`cut-n02-tests.log`。
  N02.1/N02.2 在已列参考/边界范围内 verified；N02.3 高阶场等值面和 N02.4 闭环审计仍未实施，N02 不能整体标 done。

### 6.5 历史 Linux 验证时间顺序

| 时间顺序 | 已证实的结果 | 不应推导出的结论 |
| --- | --- | --- |
| 前一基线 | 31/31 native/DRAW，各连续三次；VTK-disabled 6/6，各三次 | 不代表 N03-N09 完成 |
| 本次恢复增量后 | 34/34 native/DRAW，各连续三次，182.66 秒 | 此全量运行中的 analytic native 测试尚未包含最后加入的曲楔体/金字塔扩展 |
| 随后曲面家族扩展 | 重建 `cut_analytic_tests`，一次直接运行全部通过 | 尚未完成扩展后的重复全量验收 |
| VTK-disabled 停点 | ViewDraw、四个 native test targets 重建完成 | 新增 recovery 和扩展后的该构建测试尚未执行 |

截图：`build/test/native/draw-recovery/held_3.png` 已人工查看，材料跳变和非空几何可见；`lines.png` 由测试生成。其他历史图像见 `draw-analytic/`、`draw-cache/`、`draw-geometry/`、`draw-vtk-cells/`。

上表描述原 Linux 暂停时的验证顺序；Windows 接续结果在上节单独记录。

## 7. 当前接续与下一步

1. 已完成：Windows SDK 准备、MinGW 编译问题、恢复元数据/所有分量/极端数值验证，
   八类新曲面 DRAW 独立参考，以及 VTK-enabled/disabled 三次完整回归。
2. N02 已在 [闭环支持范围](cut-view-n02-audit.md) verified，N03.1–N03.4 已验收。
3. N03.5 已闭环：完整 VTK ON 48/48、OFF 14/14 每项三次；截图、支持边界和文档已核对。
   完整 session 持久化仍 N09；下一阶段为 N04，当前尚未启动。
4. 保留一般支持/精度限制；新增参考不能泛化为所有阶精确或百万单元全帧指标达标。

## 8. 历史 Linux 可复用命令

当前 Windows 命令见 [test/README.md](../test/README.md#windows-mingw-native-configuration)
及 [VTK SDK 配置](vtk-9.5.2-mingw.md)。以下保留原 Linux 环境命令，路径与 X11
环境不能直接用于 Windows。

### 8.1 保留已有配置并重建

新增 target 后先运行 CMake。直接 `cmake --build ... --target 新名称` 可能在旧 Makefile 尚未重新生成时报告 No rule to make target。

```bash
cd /home/hyj/Desktop/workspace/OCCT_Viewer
cmake -S . -B build
cmake --build build --target ViewDraw occtviewer_tests cut_view_tests \
  quadric_cut_tests vtk_cell_cut_tests general_cut_cache_tests \
  cut_geometry_tests cut_analytic_tests cut_recovery_tests --parallel 8
```

只有在确认 cache 缺失/配置不符时才显式补配置；不要为了这条任务重开 Web/WASM：

```bash
cmake -S . -B build -DOCCTV_BUILD_TESTS=ON -DOCCTV_BUILD_DRAW=ON \
  -DOCCTV_BUILD_WEB_VIEWER=OFF -DOCCTV_USE_VTK=ON -DOCCTV_TEST_DRAW_VISUAL=ON \
  -DVTK_DIR=/home/hyj/Desktop/workspace/OCCT_Viewer/build/deps/vtk/sdk/usr/lib/x86_64-linux-gnu/cmake/vtk-9.5 \
  -DOCCTV_VTK_TEST_DATA=/home/hyj/Desktop/workspace/vtk/build/ExternalData/Testing/Data
```

### 8.2 回归

下面的 XAUTHORITY 是本次机器的有效值，新登录后可能变化，先检查 `/run/user/1000/` 的隐藏授权文件及实际 DISPLAY。缺少显示环境时 visual 不得报告通过。

```bash
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/.mutter-Xwaylandauth.XGB5W3
export LIBGL_ALWAYS_SOFTWARE=1
ctest --test-dir build -R 'cut_analytic|draw_cut_view_analytic|cut_recovery|draw_cut_view_recovery' \
  --output-on-failure --repeat until-fail:3
ctest --test-dir build --output-on-failure --repeat until-fail:3 -j 4

cmake -S . -B build/cut-release
cmake --build build/cut-release --target ViewDraw occtviewer_tests cut_view_tests \
  quadric_cut_tests cut_recovery_tests --parallel 8
ctest --test-dir build/cut-release --output-on-failure --repeat until-fail:3 -j 3
```

无 VTK suite 新增 native/DRAW recovery 后预计由 6 项变为 8 项；以 `ctest -N` 当前列表为准，不拿旧计数当实际执行结果。

调试单个原生测试：

```bash
env LD_LIBRARY_PATH=/home/hyj/Desktop/workspace/OCCT_Viewer/build/libd:/home/hyj/Desktop/workspace/OCCT_Viewer/build/deps/vtk/sdk/usr/lib/x86_64-linux-gnu:/home/hyj/Desktop/workspace/OCCT_private/install/lib \
  stdbuf -oL -eL build/bind/cut_analytic_tests
```

### 8.3 DRAW 交互和演示

配置库搜索路径后运行自定义 SDK 的 DRAWEXE；插件 defaults 指向源码已有 VIEWDRAW 资源。脚本的 source 路径使用绝对路径，不要从 build 推导错误的 `build/native/fixtures`。

```bash
export LD_LIBRARY_PATH=/home/hyj/Desktop/workspace/OCCT_Viewer/build/libd:/home/hyj/Desktop/workspace/OCCT_Viewer/build/deps/vtk/sdk/usr/lib/x86_64-linux-gnu:/home/hyj/Desktop/workspace/OCCT_private/install/lib
export CSF_VIEWDRAWDefaults=/home/hyj/Desktop/workspace/OCCT_Viewer/source-draw/ViewDraw
/home/aaron/Desktop/workspace/OCCT_private/install/bin/DRAWEXE
```

在 DRAW 内：

```tcl
pload ALL
pload -VIEWDRAW
vinit CutView -width 800 -height 600
vsetbg -color WHITE
scvcutview model read /home/hyj/Desktop/workspace/OCCT_Viewer/test/native/fixtures/recovery_hex.vtu
scvcutview model add cut 0 1 0 -0.5
scvcutview model visibility cut 2
scvcutview model field cell pressure 0
scvcutview model cellmode cut projected
scvcutview model recovery 100 0 material
scvcutview model display
scvcutview model handle cut 1 0
scvcutview model fit
scvcutview model eval cut
```

`help scvcutview` 为当前完整命令入口。`test/cae/cut_view_vtk_demo.tcl`、`cut_view_quadric_demo.tcl` 为交互演示；自动回归脚本通常以 `exit` 结束，不能当常驻 demo source 后继续交互。

## 9. 后续阶段设计与执行顺序

### N03：恢复、物理界面 trace、多 cut 合并

- 先补恢复设置 manager 控件和测试，定义张量不变量/分量阈值、missing-value 行为，避免名字比实际能力更宽。
- 当前共享界面 section 使用最低原始 element index。增加显式 lowest/negative/positive/region traces，分别绑定显示和物理策略；物理 region 不能隐式复用显示 region，导致改颜色设置就改变积分。
- 在可信邻接记录中保留两侧来源，根据切面与来源内部点判断哪侧 trace；不依据遍历顺序或浮点焊接推测材料侧。
- 数值 `Evaluate(id)` 保持独立；另建 presentation union，按源单元与 OCCT 边界操作合并可见侧的 OR，再确定 section/cap 稳定所有权。
- 必测相同、重叠、相切、多方向 cuts、所有 masks、region 跳变、cap 来源和透明像素；不能只去掉完全相同三角形就宣称一般 OR union 完成。

### N04：VTK 导入与普通 Viewer

- 将点、线/polyline、一般单元能力接到普通 native viewer，不能仅 CutView 分支支持。
- 列清 Legacy/XML/composite/series 支持范围；fallback 成功不能留下“整体导入失败”的误导信息。
- composite/series 采用 staged import，保留每步几何与拓扑，验证时间、字段、deformation 和 tuple 对应；失败不替换 live model。
- 验证截断、endian、offset、数组长度、压缩块、多 appended offsets、坏连接及缺失引用。
- 配置最大数组/节点/单元/解压资源等限额并在分配前验证，测峰值内存；不能读入后才判断超限。
- 支持嵌套相对引用 staging、重名引用和缺文件诊断；结构化报告传到 MeshLoader/Viewer。

### N05：完整原生 Manager 和 Viewer API

- 实现 datum/local/global frames、reference axes，数字和鼠标编辑共用验证路径。
- 增加 cut/portion 样式覆盖、公共选项、透明度、边线、contour range/legend 与空/常量场状态。
- 实现结果节点/单元选择、probe 和来源值、vector/orientation plots、annotations。
- CAD section 和全局 clipping 独立且可同步；模型/viewport 生命周期正确清理。
- 对外 native Viewer API 覆盖生命周期、字段绑定、drag events 和错误契约；复用现有 AIS、mapper、manipulator、probe 设施。
- 每个新按钮、菜单、数字输入、slider/toggle 必须纳入实际 Tk widget inventory 与自动操作。

### N06：变形、时间与 slicing

- 材料跟随 frame 明确 reference/updated configuration，不仅移动 guide bounds。
- geometry/result/deformation step 原子切换，支持变化拓扑，拒绝缺字段而不是沿用旧场。
- 实现 animation pause/restart、regular/range/rotational/path slicing、路径 frame 和 interval results。
- 测切步后的 provenance、缓存失效、drag 取消、模型替换和拓扑变化。

### N07：物理结果与自动 cuts

- 实现 integration groups、summation、单位与坐标系的显式映射。
- shell/beam/composite 要有厚度、section semantics 和结果分量契约；不认识的字段不能伪造 physical resultant。
- 实现 resultant overlays、distributed/interval results 和可导出报告，方向与数值参考同时验证。
- crack-distance、optimization-density cuts 使用明确来源/符号/范围映射和特殊显示状态，不猜 field 名。
- 平面以外积分当前拒绝 local-normal 未定义情况，需实现并验证局部法向后才能开放。

### N08：性能、缓存与长会话

- 给 source/result caches 增加硬预算、估算与 eviction，验证重复拖动、时间切换后内存受控。
- 若引入后台作业，使用 immutable snapshot + generation；只有最新成功结果可在 Viewer 线程发布，关闭/替换必须取消。
- 对 source preparation、intersection、FE mapping、primitive generation、redraw 分别计时，区分 cold/warm。
- 百万体单元穿层、旋转、变形工况必须包括实际云图构建和 redraw：至少 15 FPS、p95 不超过 67 ms。
- 已有百万模型几何/字段穿层 p95 约 214 ms；不能用同拓扑缓存的约 1 ms 或加大测试 timeout 替代全帧优化。
- 长时间 open/close/edit/drag 检查资源增长、旧 handle、过时结果、abort 和 cancellation。

### N09：原生 session

- versioned JSON 保存 source reference/digest、definitions、field/deformation/step、frames、styles、legend、recovery/integration 和 camera。
- staged resolve/validate 后原子 commit；无效 session 保留 live model。
- 迁移测试覆盖现有 definition v1/v2/v3；现有 snapshot 只是定义，不是完整 session。
- 不序列化 OCCT shapes、裸指针或缓存项；相对引用使用 N04 的 staging 契约。

## 10. 完整原生 TODO 状态速查

下表保留各阶段状态；“待做”不表示前置模块毫无基础，“partial”也不能当完成。

| ID | 原生任务 | 暂停状态/下一步 |
| --- | --- | --- |
| R00 | held-drag、所有现有控件、崩溃修复 | verified；后续持续回归 |
| G01 | 一阶 field isosurface | verified；支持的高阶数值 FE 等值面另由 N02 验收，不代替 plane |
| G02 | sphere/cylinder、一般单元 | partial；全帧性能等仍待验收 |
| G03 | 高阶几何与 FE、参考精度 | verified at N02 bounded scope；族参考/精度回退/高阶 FE 等值面/闭环审计，不泛化为任意阶精确几何 |
| G04 | recovery/region/threshold/discontinuity | verified；完整 tensor/独立 trace/JSON v7/47 个控件及物理字段隔离已验收 |
| G05 | 多 cut OR 边界/cap、opacity | verified at N03.4 contract；实际 OR、来源/FE/透明云图；全帧性能仍 N08 |
| M01 | 模型生命周期、defaults、选择/激活分离 | verified |
| M02 | datum/reference axes、各 shape 编辑 | 待做，N05 |
| M03 | cut/portion styles 与 overrides | 仅共享样式基础，N05 |
| M04 | contour options、legend | 待做，N05 |
| M05 | result picking/probes | 待做，N05 |
| M06 | vector/orientation/annotations | 待做，N05 |
| M07 | CAD section/global clipping 同步 | 待做，N05 |
| D01 | deformation guides/material-following frame | partial；guide 已验收，material frame N06 |
| V01 | point/line/polyline 普通 Viewer | partial；CutView 路径已有，普通 Viewer N04 |
| V02 | Legacy 剩余范围及明确诊断 | 待做，N04 |
| V03 | 变化几何/拓扑 series 与时间校验 | 待做，N04 |
| V04 | 坏数据、binary/compression/offset 全面回归 | 已有基础，剩余 N04 |
| V05 | bounded decoding 与资源限额 | 待做，N04 |
| D02 | coordinated steps/animation | 待做，N06 |
| D03 | regular/range/rotation/path slicing | 待做，N06 |
| P01 | cache policy、硬预算和 eviction | partial；epoch/BVH/lazy 基础 verified，预算 N08 |
| I01 | groups/units/frames/summation | 已有体截面积分基础，剩余 N07 |
| I02 | shell/beam/composite mapping | 待做，N07 |
| I03 | overlays/distributed results/reports | 待做，N07 |
| A01 | mapped crack-distance automatic cuts | 待做，N07 |
| A02 | mapped optimization-density cuts | 待做，N07 |
| B01 | structured import reports 到 native caller | 剩余 N04 |
| B02 | relative-file staging | 待做，N04 |
| B03 | native Viewer Cut API | 待做，N05；TypeScript 排除 |
| S01 | native session migration/persistence | 待做，N09 |
| P02 | 百万单元全帧仪表与优化 | 待做，N08；当前未满足 |
| P03 | 长会话、memory/cancellation/lifecycle | 待做，N08 |
| B04/W01/W02 | Vue/WASM/browser | 排除，不标 done |

## 11. 每阶段完成门槛

1. 功能契约与 unsupported diagnostics 明确，不隐藏 fallback 或物理假设。
2. native 数值参考、来源与字段验证通过，包含无效输入和失败原子性。
3. DRAW Tcl 可独立复现，相关视觉/控件与真实 held-pointer 测试通过。
4. affected targets 全部重建；完整 VTK-enabled 与 VTK-disabled suite 按要求重复运行。
5. 视觉变化查看截图；性能变化记录全帧与内存测量，不能只报告非空画面。
6. 更新执行文档中的具体证据、限制、命令和状态，再推进下一阶段。

推荐接续指令：N03 已闭环，下一阶段按 N04-N09 顺序推进，先完成导入与普通
Viewer 的剩余范围。保留原有回归与用户修改，Web/WASM 始终排除。

## 12. 最新 N02.3 接续记录

N02.1/N02.2 已提交 `7c925f2`。新增原始 FE 参数场等值面、独立场容差和预算。
37 种参数单元、六类原生二阶单元、非单调双交叉/封闭圆球、独立有理函数、变形、
来源、失效、CellData 和失败显示原子性有 native 验收；DRAW 真实 Tk held-threshold
的 FE/版本/像素/Escape 回滚通过，初始与 held 图像已查看。最终定向 5/5 通过
（67.02 秒）；VTK OFF 全套 8/8 各三次通过（6.24 秒）。

[实现契约与限制](cut-view-isosurfaces.md)；日志位于 `build/mingw/cut-n02-iso-*`
与 `build/mingw-no-vtk/cut-n02-closure-tests.log`。N02.3 已验收；下一步 N02.4
双配置完整重复回归和范围审计，通过后推进 N03。前文旧停点保留为历史。

## 13. N02.4 已闭环，进入 N03

N02.3 提交 `f975b86`。完整 VTK ON 38/38 每项三次（114 次通过），610.23 秒；
OFF 8/8 每项三次（24 次通过），6.24 秒；控件 32/32，零失败。日志见
`build/mingw/cut-n02-closure-tests.log` 和 OFF 同名日志。

[范围审计](cut-view-n02-audit.md)明确采样、float、非匹配接口与 unsupported 限制；
N02 已在该范围 verified，后续物理侧迹和 OR union 属于 N03，百万单元全帧仍 N08。
现在按 [N03 队列](cut-view-n03-plan.md) 实施 N03.1 恢复控件及精确选项持久化。

## 14. N03.1 已验收：恢复控件和定义持久化

- 实际 Tk controls **36/36**，阈值 Return/Apply、非法草稿、checkbox、region
  菜单与 Save/Load 均通过；`draw-controls/manager_controls.png` 已查看，控件完整。
- JSON **v5** 持久化 threshold/before/regions，signed int64 极端 region IDs
  精确 round-trip；v1-v4 使用恢复默认值，非法字段/计数/溢出恢复原子拒绝。
- DRAW `recovery threshold before keep` 保留精确恢复的 IDs，getter 不转 double。
- Windows capture 使用 Tk root wrapper 的完整绘制；异步等待时继续分派 Tk
  事件，要求 fresh PNG、非空 client 内容和成功标记，避免旧截图假通过。
- VTK ON **38/38 每项三次**，OFF **8/8 每项三次**；日志分别为
  `build/mingw/cut-n03-recovery-tests.log` 和 OFF 同名日志。

后续按 N03.2 明确六/九分量布局、剪切约定和张量 measures，再做 N03.3
physical traces、N03.4 OR union/cap ownership、N03.5 闭环。N03 整体仍 partial；
完整 native session 是 N09，不能以 definitions v5 代替。

## 15. N03.2 张量派生量已验收

- `MeshCutTensor` 提供 Mises、pressure、降序三个 principal、Tresca；六分量
  两种 shear 排序和九分量 row/column-major，tensor/engineering shear 显式选择。
- 仅对明确标记的 Tensor 显示启用；scalar/vector 不改绑定。数值范围、
  NaN/Inf、对称性、单位与 element-node keys 契约见 [张量说明](cut-view-tensors.md)。
- before：原始 tuple 派生再恢复；after：恢复 tensor components 后在 source
  tuples 派生；最后均用原始 FE 权重映射 scalar。不是 cut 点处重算非线性张量。
- `EvaluatePhysical()` 与 DRAW freebody/heatflow 完全绕过显示恢复/派生，
  物理 shear/component map 不因 engineering 显示选项改变。
- JSON v6 保存选项、v1-v5 迁移默认；非法 enum/type 原子拒绝。
- Tk 三个 tensor 菜单实际交互，完整 **39/39**。已查看两张张量梯度图和
  `draw-controls/tensor_manager.png`、`manager_controls.png`，布局完整。
- 最终 ON **41/41 每项三次**，OFF **10/10 每项三次**；日志
  `build/mingw/cut-n03-tensor-tests.log`、OFF 同名文件。
  实际为 123 / 30 次成功，424.30 / 8.22 秒；Tk 布局统一使用 `pack`，
  完整回归覆盖 `-b` 延迟 Tk 初始化与 `-i` 交互入口。

N03 仍 partial；后续 N03.3 trace selection、N03.4 真正 OR union/cap ownership、
N03.5 闭环审计。不要把展示 scalar reconstruction 或 trace/cap 去重称为一般
非线性 FE invariant 或 OR union 已完成。

## 16. N03.3a 双侧来源基础

- `MeshVtkCutSource` 可信共享体单元面保存 reciprocal element/face IDs，
  `InterfaceTraces` 返回两侧各自原始 FE 权重和变形后的 interior points。
  返回顺序按原始 element ID 稳定；默认显示/积分仍使用最低 ID。
- `cut_interface` 用独立 affine/quadratic 参考验证 element-node 值跳变、
  曲面 analytic/numerical 路径和 deformation；断开、同侧重叠、三来源
  歧义界面拒绝，所有 masks 和原始 physical 积分保持原有契约。
- 定向测试连续三次通过（1.89 秒）。完整 ON **42/42**、OFF **10/10**，
  各项三次，实际 126 / 30 次成功，444.71 / 4.70 秒；日志为两配置的
  `cut-n03-interface-tests.log`。控件 inventory 仍为 **39/39**。
  验收仅闭环 N03.3a 来源基础，不能把 N03.3 标为完成。
- 范围见 [侧迹说明](cut-view-traces.md)。该 API 是 VTK prepared source
  的基础；native tetra 测试也走这一 API，VTK OFF fast Slice 尚未接入。
- 下一工作项 N03.3b：生成 section 图元在 owner filtering 前保留双侧来源，
  包括 native fast Slice。N03.3c 分开绑定 display/physical
  lowest/negative/positive/region 选项，physical regions 不复用显示 recovery
  regions；N03.3d 做 native controls 和持久化迁移；N03.3e 验收完整
  line/surface/volume、deformation、mask、跳变及积分矩阵。各项验收后单独提交。

N03.4 OR union/cap ownership 和 N03.5 闭环继续排队；N03 整体仍 partial。

## 17. N03.3b 图元来源绑定

`MeshGeometryTraceSection` 保存生成面/线/点的双侧原始 FE 来源，native fast
Slice 与 VTK general 路径均接入；可信来源对与权重指纹去重，不用坐标焊接。
默认 lowest ID、物理字段元数据、子范围查询和 fast 平移连接缓存保持兼容。
ON 完整 **43/43**（145.75 秒），OFF **11/11 各三次**（3.65 秒），ON
重点 **4/4 各三次**（2.43 秒）。日志为 `cut-n03-binding-tests.log` 和
ON `cut-n03-binding-focused.log`。已查看管理器与投影云图。
下项 N03.3c：display/physical 独立选侧和原始积分；N03 整体仍未闭环。
大量源码修改后主动执行 `index_repository(repo_path="D:/workspace/OpenCASCADE/OCCT_Viewer")`，
coverage partial/unusable 的文件继续直接核对源码；不把索引自动刷新当作完整解析保证。

## 18. N03.3c 独立选侧 API

每 cut 的 `DisplayTrace` / `PhysicalTrace` 分别保存策略、int64 region ID 和
assignments。`SetTraceOptions` 预验证隐藏 section，不可用/歧义策略原子拒绝。
`EvaluatePhysical` 使用原始场和物理策略，不修改展示侧迹；`PhysicalLineField`
提供独立线字段。Iso 选侧使用固定切割场的原始 FE 内部点值。
ON 重点 **11/11 各三次**（5.20 秒）、OFF 完整 **11/11 各三次**（5.73 秒），
日志 `cut-n03-selection-tests.log`。N03.3d 控件与 JSON migration 接着推进。

## 19. N03.3d 原生控件与迁移

DRAW `trace id display/physical` 与 Tk 独立策略、region ID、assignments、Apply
已接入，整数输入不经 double。JSON **v7** 保存两组 exact signed int64 配置，
独立缓存预验证后再替换；v1-v6 默认 lowest，无效输入/恢复保持当前定义。
ON 重点 **5/5 各三次**（53.30 秒）、OFF 完整 **12/12 各三次**（4.66 秒）。
实际控件 **47/47**；最终紧凑布局测试通过（16.08 秒），两张管理器和
negative/positive 云图已逐张查看。日志为 `cut-n03-trace-controls-focused.log`
与 ON `cut-n03-trace-controls-layout.log`。继续 N03.3e 完整矩阵验收。

## 20. N03.3e 侧迹闭环

变形后的 plane/iso 选侧、反向法向、全部 masks、独立原始应力合力/力矩、
线端点四种策略，以及 OFF 对 VTK-only 线拓扑的拒绝均补入矩阵。
完整 ON **45/45 各三次**，135 次成功（501.41 秒）；OFF **12/12 各三次**，
36 次成功（7.81 秒）。日志 `cut-n03-trace-matrix-tests.log`，控件 **47/47**。
完整族测试发现并修复空/多顶点 0D 单元的内部点 FE 反算问题，相关 DRAW
与 visual 依赖在最终重复套件通过。N03.3 在现有支持契约内已闭环。
N03.4 OR union/cap ownership 接着推进；N03 整体仍 partial，后续 N03.5 审计未完成。

## 21. N03.4a/b OR 基础与 FE 验收

N03.4a 已提交 `db6268b`：实际 OCCT retained-region OR、64 组双 cut masks、
可信邻接/断开来源、独立数值与 presentation 分离。ON 重点 7/7、OFF 完整
13/13 各三次（29.25 / 26.67 秒），日志 `cut-n03-union-foundation-tests.log`。
N03.4b 补齐斜向/相切/分离平面、球交叠与相切、圆柱、表面截线、变形、
material region 与 element-node 跳变、二次 FE、high-order iso/plane、线端点、
polyvertex/空单元。修复共享内部 On sheet、0D BRep 缺点及 OFF 邻侧微小
非共享权重引发 NaN。ON 重点 7/7、OFF 完整 13/13 各三次通过
（286.04 / 28.31 秒）；日志 `cut-n03-union-fields-tests.log`。
每次提交后刷新 MCP 索引；解析缺口继续直接核对源码。N03.4c 透明 DRAW
比较正在实施，N03.5 完整闭环审计尚未运行；N03 整体仍 partial。

## 22. N03.4c 透明 DRAW 验收

N03.4b 已提交 `addbb03`。新增 `scvcutview union` 的真实 OR volume/area 和
cut/portion/source/cap 来源诊断，portion 共享透明度查询/设置及 RGB 图片比较。
新增单位 hexa fixture；重复、交叉、mask7 数值与独立 cut 不变均通过。
变化字段使用 unlit + weighted OIT 消除光照/绘制顺序差异，重复/包含/full masks
要求 RGB byte mean <0.01；恒定 CellData 在普通透明混合下
仍比较一致，并要求透明度 0.55 改为 0 后 mean >5，验证门槛确实敏感。
ON 重点 3/3、OFF 重点 2/2 各三次（27.80 / 22.62 秒）；收紧后的 visual
另重复三次。日志 `cut-n03-union-visual-tests.log` 和 ON
`cut-n03-union-opacity-tests.log`；图片 `build/mingw/test/native/draw-union/`
已核对。N03.4 在现有支持范围内 verified，N03.5 完整双配置闭环审计接着运行。

## 23. N03.4d 修复与 N03.5 完整闭环

完整 ON 验收最初发现 VTK-cell/quadric visual 两项缺少 triangulation。
新增 warm sphere 八种 masks 解析参考复现了只显示 On 时的浮点极点三角形
BRep 退化，并补出 full mask 下 compsolid 共享内部球面的暴露面积错误。
`3a1fd44` 保留球面/圆柱的 OCCT cap 曲面，按同一原始单元内的实体共享
拓扑排除内部面，保持 one-percent-radius deflection。平面共面截面保留原
数值构造，断开拓扑的两个材料来源仍独立。重复/相交 On-only sphere 面积
及来源所有权均补入；OFF 明确使用已支持的 native tetra 曲面路径。

最终 ON **48/48 各三次**，144 次成功（538.76 秒）；OFF **14/14 各三次**，
42 次成功（47.00 秒）。已逐项检查日志中的次数，所有 case 都恰好通过三次。
日志 `build/mingw/cut-n03-closure-tests.log` 与对应 OFF 路径；初次失败记录
保存在 ON `cut-n03-closure-initial-tests.log`，没有将失败运行当作通过。
控件 **47/47**；最终 manager、tensor/trace、球面/圆柱、held-drag 和透明
OR 云图均已查看。完整支持范围及限制见 [N03 审计](cut-view-n03-audit.md)。

源码修复提交后已刷新 MCP 索引：34,418 nodes / 127,831 edges；部分解析
缺口继续直接读源码。N03.1–N03.5 均 verified，N03 已收敛。N04–N09 尚未
启动；完整 native sessions、per-cut styles/legend/probes、shell/beam 与非平面
物理映射、百万单元全帧/硬缓存预算仍是后续工作，Web/WASM 继续排除。

## 24. N04.1 导入报告与失败发布保留

统一 MeshLoader 和 CutView 的 VTK 导入入口；报告区分最终 outcome 和
portable/native-vtk 的每次尝试。成功 fallback 不再输出主 reader 的整体
失败诊断，原诊断仍保留在 attempts。映射请求不能通过 fallback 被悄悄丢弃。
Legacy 内部 XML reader、VTM/series 子 reader 继承消息策略并传递 warnings。
普通 native Viewer 的 `OpenMesh(..., options, &report)` 可返回结构化报告；
`scvcutview model importreport` 提供对应 Tcl dict，支持带空格的路径。

Viewer 和 `scvreadmesh` 在 reader 成功后才替换旧同名对象。原生测试验证
失败后旧 AIS 对象、mesh/fields 与显示状态保留；DRAW 验证非空普通 Viewer
四面体图像在缺文件、畸形文件、缺 composite 引用后均零像素变化。
VTK executive 失败不能发布其空输出；完全无实体输出明确拒绝，但合法
`vtkEmptyCell` 的零节点连接保留。首次完整回归发现该空单元误拒绝，
修复后重跑完整 ON 套件，未把失败及依赖未运行 case 算作通过。

最终 ON **51/51 各三次**：153 次成功，604.78 秒；OFF **16/16 各三次**：
48 次成功，50.79 秒。逐项核对所有 case 恰好三次，合计 201 次。
日志 `cut-n04-diagnostics-tests.log` 在两配置中；ON 首次失败日志
`cut-n04-diagnostics-first-full-tests.log` 保留。测试目标按 Viewer 是否存在
启用生命周期检查，精简 DRAW/Viewer-disabled 配置下 reader/report **1/1
各三次**（0.19 秒）另通过，日志 `build/cut-n04-headless-tests.log`。
条件化后的 Viewer 测试再验收 ON 3/3、OFF 2/2 各三次，日志
`cut-n04-diagnostics-viewer-tests.log`；构建日志为 `cut-n04-diagnostics-build.log`。
Tk inventory 仍 **47/47**。图片 `build/mingw/test/native/draw-import/`
已检查；原始、失败后及成功替换图片 SHA256 相同，且非空。

[N04.1 契约](cut-view-import.md) 与 [N04 task list](cut-view-n04-plan.md)
已同步。MCP 的 C++/Tcl partial 范围直接核对源码，提交后显式刷新索引。
下一项 N04.2：普通 Viewer 的点、line/polyline、一般 VTK 单元与原始字段
显示。N04 整体仍 active；变化拓扑 series、格式矩阵、资源限额及相对引用
staging 未完成，不能因本项诊断/发布基础通过就宣称 N04 闭环。

## 25. N04.2 — 普通 native Viewer 一般单元显示

普通 scalar presentation 接入 `MeshPartGeneralCells`，复用已验证的原始 VTK
单元求值与可信 internal-face 过滤，产生点、线和表面/体边界图元。
每个图元保留 original indexed element 与原始 FE vertex weights；CellData
不经节点投影替换。节点/单元结果切换、类型可见性和显示模式变化均更新图元。
独立点图元使用普通点颜色/尺寸，修复白背景不可见的问题。

定向 ON 3/3、OFF 2/2 每项三次通过（10.15 / 0.65 秒）；最终全量 ON
51/51（188.80 秒）、OFF 16/16（12.84 秒）各一次通过。日志为两配置的
`cut-n04-viewer-focused.log` / `cut-n04-viewer-full.log`。首次 ON 全量的
pure-point 白色显示失败保留在 `cut-n04-viewer-first-full.log`，修复后全套重跑。
图片 `draw-import/ordinary-*` 已检查：纯点、Legacy/VTU/VTP polyline、混合
点线面、二次曲线节点结果、一般 voxel；数值检查含 affine / quadratic FE
与原始 CellData。N03 47/47 Tk 与全部几何/物理/OR 回归保持通过。

契约见 [普通 Viewer](cut-view-viewer.md)。本项闭环后提交并刷新 MCP 索引。
N04 已完成 2/8 验收项，整体仍 active；下一项 N04.3 格式范围和诊断，随后
推进每步几何/拓扑 staging、安全矩阵、资源限额和相对引用闭环。
