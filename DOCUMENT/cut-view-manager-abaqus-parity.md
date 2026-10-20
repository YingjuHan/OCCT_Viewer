# Cut View Manager：Abaqus 对标需求与开发台账

## 1. 范围、证据与完成定义

以 **Abaqus/CAE 2025 Visualization 模块的 View Cut Manager** 官方帮助为功能基线，目标是能在本项目完成相同的后处理工作流。2026-10-20 开始本轮对标。尚未取得 Abaqus 可执行环境和相同结果文件，因此本文是官方文档与本项目源码的差距分析，不代表已完成双软件图像或数值比对，也不以截图相似度代替功能验收。

本项目继续使用 OCCT、自研 VTK/VTU 读取与有限元求交内核；不引入外部 VTK。目标 VisIt 的迁移仍遵循 [无 OCCT 迁移手册](cut-view-manager-visit-porting.md)，本轮不修改目标项目。

每个对标项须经过：状态模型与序列化 → C++/DRAW 接口 → Manager 控件 → 正确性回归 → 百万单元适用性检查 → 文档 → Git 提交。具有底层 API 不等于 Manager 工作流已经完成。性能结果使用 Release、不挂 GDB；图像和解析正确性使用小型确定性夹具，性能与预算使用 3,375,000 单元输入。

官方依据：

- [S1 总体语义](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cutunderstand.htm)：形状、场绑定、动画、缓存、变形及自动 cut。
- [S2 创建与编辑](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpcreate.htm)：Key-in/CSYS、局部轴、参考帧与随变形。
- [S3 位置调整](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpposition.htm)：平移、旋转、灵敏度、非平面参数。
- [S4 显示与自由体开关](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpdisplay.htm)：Show、三部分、自由体向量及正负方向。
- [S5 多 cut](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpmultiple.htm)：独立 cut 和多 cut 模式限制。
- [S6 复制、重命名、删除](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cuthlpcopy.htm)。
- [S7 部分显示选项](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpcustomize.htm)：Below/On/Above、继承 plot options、面与边、透明度。
- [S8 自由体选项](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpfreebody.htm)：积分域、求和点、分量坐标系、热流。
- [S9 切片](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpslicing.htm)：范围、旋转、路径、单 cut 显示限制。
- [S10 非 Visualization 的 cap 颜色](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpcustomizecae.htm)：单色/按主体颜色；不能与 Visualization 的云图选项混为一谈。

## 2. 当前能力与差距矩阵

“已有”仅表示源码和既有回归中存在该能力；“部分”表示存在入口但还有语义、交互或验收差距；“待开发”不承诺已支持。下表覆盖本轮工作包，仍需以运行验收逐项关闭。

| 编号 | 用户需求 / 官方依据 | 当前状态与证据入口 | 收敛条件 |
| --- | --- | --- | --- |
| R01 | 创建/编辑/复制/重命名/删除，默认 XYZ（S2/S6） | 部分；`MeshCutViewManager`、Manager；重命名混在几何 Apply，DRAW 复制只复制定义 | 独立重命名；复制显示覆盖；删除后相同 ID 不继承旧样式；非法操作保留状态 |
| R02 | 选中项与激活项分离（S1/S3） | 已有；`selected` 与 `Enabled` 分离 | 非激活项选择、编辑、重命名不激活；关闭窗口不改显示 |
| R03 | Below/On/Above 的全部组合（S4） | 已有；mask 0..7，单平面 GPU 特殊路径 | 8 种 mask、空截面、正负法向、薄板断开截面、三显示模式均正确 |
| R04 | 多 cut 独立显示（S5） | 部分；加性分支与 OR presentation，不做串联裁剪 | 对照 S5 的多 cut 模式只显示切面/自由体限制；明确兼容模式与本项目扩展，不能直接认定 OR 已完全等价 |
| R05 | Key-in 原点、法向、平面内第二轴（S2） | A02 已验收；完整 Placement、手柄、JSON 8 和旧版迁移 | `cut_placement`、`draw_cut_view_placement`、鼠标交互回归通过 |
| R06 | 从 CSYS 创建与编辑（S2） | A02 已提供 Manager 的 CSYS/法向轴入口；Apply 获取 datum 快照 | 改 datum 后重新 Apply；显式刚性 follow 独立存在。自动关联 datum 后续扩展，不能声称已实现 |
| R07 | 平移、精细灵敏度、位置输入（S3） | A02 已验收；1/10/100、非激活输入、禁用滑块 | 范围以当前位置为中心；切换精度与运动模式不编辑几何 |
| R08 | 关于原始局部 2/3 轴的绝对旋转（S3） | A02 已验收；保留定义轴/pivot，绝对角度，旋转清除平移 | 重复角度、切轴、Escape、跨帧与 session 回归通过 |
| R09 | 柱、球的中心/轴/半径（S2/S3） | 已有入口；自研通用单元内核 | 保持既有非平面路径，径向 Below=内部；适用单元、容差、退化轴明确报告 |
| R10 | Isosurface 绑定创建时的变量（S1/S2） | 部分；独立 field/component/step/value 已持久化 | 改云图不改切面；显式 Edit 才重绑；等值范围、派生不变量及 averaging override 独立验收 |
| R11 | 三部分渲染方式、边线、颜色、透明度（S7） | 部分；`Style/EffectiveStyle` 与 Viewer Tools 简单入口 | Manager 三页选项；默认值与单 cut 扩展覆盖可查询；边色/线型、面色/云图来源、继承 plot options 后续补齐 |
| R12 | 云图/线框/云图+线框实时剖切 | 已有；`MeshCutGpuPresentation` | 无变形与变形数据、mask 2/3/6/7；拖动时主体提交字节为 0，空截面无残影 |
| R13 | 固定空间切面对变形网格剖切（S2 默认） | 已有；`SetDeformation` | 位移正/负/零、多倍率、字段插值与预算稳定；不把 FE 字段重新解释为空间坐标函数 |
| R14 | Follow model deformation，First/Last/Current 参考帧（S2） | A03 平面材料模式已验收；真实 FE 权重映射、当前曲面采样和局部法向，三节点刚性 frame 独立保留 | 平面范围闭环；保持非平面 cut 现有行为。采样误差/参数图支持边界见材料指南，不声称任意阶全域认证 |
| R15 | 时间/倍率动画中的 cut（S1） | 部分；`NativeCutTimeline` 与 Time 面板 | 数据帧/变形/场/结果/手柄同代发布；失败停播并保留可用帧；补倍率动画和参考帧语义 |
| R16 | Manager 每 cut 自由体开关（S4/S8） | 部分；Analysis 请求与 Physics 独立面板 | 按 cut 显示合力/矩/热流；开关不改 cut；删除/切换模型清理；Above/Below 法向与符号相反 |
| R17 | 积分域：整体/显示组/单元集（S8） | 部分；显式 element IDs/GroupName | 连接实际显示组/集合，空集区别于全体，随帧/重网格映射或明确拒绝 |
| R18 | 求和点：截面形心/自定义（S8） | 部分；固定 `MomentOrigin` 已有，缺自动形心策略 | 面积加权体截面、厚度加权壳线、梁交点；零测度可解释；平移求和点满足力矩变换 |
| R19 | 法/切向与 CSYS 分量向量（S8） | 部分；显式 output frame/向量覆盖已有 | 自动局部 frame、法切向/合向量显示和稳定切向；数值与显示分离 |
| R20 | 实体/壳/复合/梁结果及热流（S1/S8） | 部分；`MeshCutPhysics`、原始 FE provenance | 单位/应力 Voigt 顺序/厚度/层映射可见；NFORC、SF、SM 输入的适配与应力积分分别验证，不声称二者等价 |
| R21 | 切片范围/规则间距/旋转/路径（S9） | 部分；`MeshCutSlicing` 生成独立 cut 定义 | 显示切片与手工 cut 生命周期分离；路径节点/重采样、单激活限制、删除/覆盖/预算控制 |
| R22 | 切片上的自由体分布与报告（S8/S9） | 部分；Physics Slices API、CSV | Manager 统一入口，区间/路径/求和点、单位列、失败中止和不污染现场 |
| R23 | 自动 XFEM/优化 cut（S1/S6） | 部分；显式 signed-distance/density 映射，无 ODB 元数据发现 | 语义数据适配、受保护的定义、特殊 mask、自动创建/移除；缺字段不能推断为自动 cut |
| R24 | 状态持久化（S2/S6） | 部分；definitions JSON 与 version 3 session bundle 已有 | 明确 Save definitions 不保存样式；完整 session 保存样式/绑定；新增 frame/options 版本迁移；不宣称直接兼容 Abaqus XML/ODB |
| R25 | 多视口隔离 | 部分；Native facade 绑定一个视口，DRAW Manager 为单实例 | 每视口 cut 激活/样式/临时选择隔离，模型数据共享；保存恢复不影响另一视口 |
| R26 | 性能、内存预算与取消（S1） | 已有基础，变形网格仍有性能差距 | 百万级路径无每步全量上传/节点重算；取消不发布过期帧；记录 FPS/p95/分阶段时间/预算；不能以提高限额掩盖泄漏 |

## 3. 开发顺序和可交付工作包

| 工作包 | 优先级 | 本轮状态 | 内容 / 依赖 |
| --- | --- | --- | --- |
| A01 Manager 显示与对象管理入口 | P0 | 本包完成 | R01/R11 的独立 Rename、Options 三页、样式覆盖查询/颜色输入、复制/删除样式生命周期；不代表 R11 全部收敛 |
| A02 完整坐标系和运动模型 | P0 | 本包完成，CSYS 为显式快照 | R05/R07/R08 验收；R06 创建编辑入口完成，动态 datum 关联保留为扩展边界；见平面运动指南 |
| A03 真正随变形截面 | P0 | 本包完成（平面材料模式） | First/Last/Current、FE 权重、参考侧别、缓存、JSON 9、三个 portion 曲面采样、当前法向、失败回滚和百万单元验收；参数图/采样边界见材料指南 |
| A04 Manager 自由体工作流 | P1 | 本包完成（显式 FE 适配契约） | 独立开关/面板、形心/自定义点、符号/NT/CSYS、标签集合/实际显示快照、材料梁参考权重；15 项回归及百万门禁通过；公共默认模板随 A09 一并持久化 |
| A05 Isosurface 完整绑定与 averaging | P1 | 待开发 | R10；派生量与单独平均配置、显式编辑重绑；沿用已有非平面路径 |
| A06 切片集合与报告 | P1 | 待开发 | R21/R22；临时集合、路径重采样、成组移除/保存、自由体扫描 |
| A07 全部 plot options 与多 cut 兼容 | P1 | 待开发 | R04/R11；当前 plot 继承、边色/线型、独立颜色来源、Abaqus 限制与扩展标志 |
| A08 自动 cut 与数据适配 | P2 | 待开发 | R23；不依赖 ODB 库也能使用显式元数据；ODB 导入另立适配边界 |
| A09 持久化与视口隔离 | P1 | 待开发 | R24/R25；增量版本迁移、完整 session、视口配置与共享源数据拆分 |
| A10 大规模性能及对照验收 | 持续 | 已有基线 | R26 和每包性能门禁；最终再做有 Abaqus 环境的同输入数值/画面对照 |

每包结束单独提交；矩阵按实际测试更新。总进度按通过验收的需求条目统计，当前不提供缺少分母与验收证据的百分比。

## 4. 关键设计约束

### 4.1 运动与参考构形

增加 `DefinitionFrame`（原点、法向、平面内轴）、`Motion`（绝对位移、角度、旋转轴）、`CutConfiguration`（Spatial/Material）和 `ReferenceFrame`（First/Last/Current 实际解析后的 step ID）。用户输入与求交使用的世界坐标平面分离。保留旧 JSON 的平面方程读取，通过确定性轴构造迁移，不能伪造丢失的历史旋转姿态。

Spatial 模式按当前节点位置与空间平面求交，保持现有 GPU clipping + CPU slice 优化。Material 模式在参考构形求交并保留单元 ID、自然坐标和形函数权重，然后计算 `x(t,ξ)=Σ N_i(ξ)[X_i+s·u_i(t)]`。不同点的位移不同，截面可弯曲；材料两侧也须按参考参数分类，因此该模式不能直接套用一个世界平面 GPU discard。应建立专用参考符号场/拓扑路径与后端能力判断，避免修改现有非平面切割行为。

改切面定义、参考 step 或拓扑时失效参考交点；仅切时间/倍率时复用参考交点并更新位置和场。重网格必须有显式材料对应映射，缺失时拒绝跟随并说明原因。反演/退化单元不默默转成空间切面。

### 4.2 显示选项与状态边界

官方 S7 的三部分 plot options 是视口级设置；本项目现有单 cut 样式覆盖是扩展。UI 必须明确“Model defaults”与“Selected cut override”，不能把后者宣称为 Abaqus 原生行为。完整的“Use current plot options”还需要单独的原始 plot style 与 portion override 状态，A01 暂不伪装实现它。

显示修改不能更改原始 FE 字段、物理 provenance、cut 几何与启用状态。字段存在时面色仍取云图；A01 颜色仅控制无标量场时的面色。边色/线型、独立切面单色/云图选择在 A07 实现。混合模式/透明度可能进入现有 CPU fallback，应可用 `renderinfo` 查明，不能承诺所有 plot options 都走 GPU。

复制要区分定义、显示配置、物理请求、时间绑定和临时手柄；A01 仅完成定义和显示样式，后续工作包处理其余引用。删除后不得留下同 ID 的显示覆盖。普通 definitions Save/Load 和完整 session Save/Load 的范围必须明确。

### 4.3 物理与自动 cut

统一请求中显式保存积分域、输入分量/单位、配置、法向、求和点策略、输出 CSYS。先计算原始 FE 物理量，再做显示转换；不拿平滑后的云图积分。法向翻转和 Above/Below 切换必须成对验收。壳/梁/层合与节点内力输入不能自动套用实体应力公式。

自动 cut 的来源、可编辑性、特殊显示 mask 与普通用户 cut 分开保存。只有实际提供裂纹 signed-distance 或密度语义时才创建，不能按字段名字猜测。缺少 Abaqus 结果与目标数据适配时，不能声称 XFEM/优化工作流完全对齐。

## 5. 运行与测试入口

通用 Linux Release / GDB、资源变量及依赖配置见 [Linux 运行与测试指南](cut-view-manager-linux-run-test.md)。本轮推荐继续使用独立 `build/mingw-deformation`，避免覆盖用户 DRAW 正在加载的 DLL。

```powershell
cmake --build D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --target ViewDraw --parallel 6
ctest --test-dir D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation -R '^(cut_view|cut_display|native_session|draw_cut_view_controls|draw_cut_view_tools|draw_cut_view_plot_options|draw_cut_view_gpu_plane|draw_cut_view_display_modes|draw_cut_view_deformation)$' --output-on-failure
```

Linux 配置上述指南中的变量后，直接运行与调试运行分别为：

```bash
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation_demo.tcl"

export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/bin/viewdraw-runtime.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation_demo.tcl"
# GDB 内：set breakpoint pending on，然后 run。
```

新增回归使用 `OCCTV_TEST_DRAW_VISUAL=ON` 并需要可用 OpenGL 显示环境。Linux 命令按现有构建配置编写，当前工作机是 Windows，不能视为已在 Linux 执行通过。

大规模基线：3,375,000 六面体，变形倍率 0.1，mask=3；已有 Windows Release 测量 cloud/wireframe/cloud_edges 约 8.97/9.85/8.21 FPS，p95 150.15/129.94/157.81 ms，详见 [变形指南](cut-view-manager-deformation.md)。这只是当前基线，未达到稳定 30 FPS；A03/A10 应分别报告参考求交与逐帧更新成本。

## 6. A01 的使用方式与工程边界

打开既有演示后，点击 Manager 的 **View Cut Options...**。三个页签是 Below Cut / On Cut / Above Cut，对应 portion `0/1/2`。选择 **Model defaults** 编辑模型内各 cut 使用的默认样式；选择 **Selected cut override** 后勾选 Override，在打开对话框时选中的 cut 上设置覆盖。取消勾选再 Apply tab 恢复继承。切换 Manager 列表选中项不会把已打开的选项窗口悄悄指向别的 cut。

目前提供 Display（cloud / wireframe / cloud_edges）、透明度、边线宽度、无标量场时的面 RGB；Apply tab 只提交当前页，不丢弃其他页的待编辑值。默认样式和 cut 覆盖都已存在于原有 C++ 显示层；本轮新增独立 UI、颜色的 DRAW 输入及覆盖状态查询，没有更改求交算法或引入新依赖。

```tcl
# 在 DRAW 中运行；先加载一个模型和 Manager。
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_deformation_demo.tcl
scvcutoptions cut_modes
scvcutoptions cut_modes "Selected cut override"

# 查询返回 transparency / width / mode / color / override。
scvcutview cut_modes cutstyle cut 1
scvcutview cut_modes cutstyle defaults 1
# 旧的三个数值参数写法仍有效；可选的末尾三个数是 RGB。
scvcutview cut_modes cutstyle cut 1 0 1 2 0.2 0.4 0.8
scvcutview cut_modes clearstyle cut 1
```

Manager 的 **Rename** 是独立操作：即使 Position/Normal 中留有无效、尚未 Apply 的输入，重命名也只改名称。原有几何 Apply 的行为保留。DRAW `copy` 复制单 cut 的显示覆盖，随后两者独立；`remove` 即使在模型未显示时也立即清除该 ID 的覆盖。复制物理请求和时间绑定仍属于后续工作，不在此包内。

保存完整状态请使用已有 Session 面板或 `sessionsave/sessionload`；Manager 原有 Save/Load 只保存 cut definitions，不保存这些显示样式。透明度和混合 portion 模式仍按现有能力选择 CPU fallback；本轮不把这种配置标为 GPU 已加速。

新增文件：`test/cae/cut_view_plot_options.tcl`（UI）、`cut_view_plot_options_test.tcl`（语义与图像回归）、`cut_view_plot_options_large.tcl`（百万单元预算/源体复用验收）。后者不注册为默认 CTest；先生成 [变形指南](cut-view-manager-deformation.md) 中的 `hex_150.vtu` 并设置 Windows/Linux 运行环境后手动运行：

```powershell
# Windows：使用前文环境和 build/mingw-deformation 的插件。
& 'D:\workspace\OpenCASCADE\OCCT_private\install\mingw\win64\gcc\bin\DRAWEXE.exe' -i -f 'D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_plot_options_large.tcl'
```

```bash
# Linux：Release 环境，输入必须至少包含 1,000,000 个单元。
export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk-deformed/hex_150.vtu"
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/plot-options-large"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_plot_options_large.tcl"
```

## 7. 本轮开发记录

- 2026-10-20：核验官方 S1..S10 与当前源码，建立 R01..R26 和 A01..A10；开始 A01。索引对 C++/Tcl 存在部分解析覆盖，本轮以相关源文件直接读取补全，不能把图查询未返回符号视为能力缺失。
- A01 已完成实现与验收：`ViewDraw` Release 构建通过；9 项相关 CTest 通过（`cut_view`、`cut_display`、`native_session`、`draw_cut_view_controls`、`draw_cut_view_tools`、`draw_cut_view_plot_options`、`draw_cut_view_gpu_plane`、`draw_cut_view_display_modes`、`draw_cut_view_deformation`）。补充完整 session 的 RGB/覆盖往返、未显示模型删除后的 ID 重用、输入格式修正后，新回归再次通过。新面板截图已人工检查；Windows 截图辅助进程曾导致测试超时，已将其移出自动回归，保留实际渲染的面色/云图图像比较。
- 百万单元：3,375,000 六面体、倍率 0.1，三模式 × mask 2/3/7 共 9 组完成 `CUT_PLOT_OPTIONS_LARGE_PASS`。打开 Options 不触发同步求交；Apply/恢复继承后 GPU 路径保留，后续 4 步切面移动均无主体重建/上传，源准备次数不增加。逻辑预算始终为 source `1,460,965,248`、result `17,010,275,392` 字节，限额仍为 4/16 GiB。它是预算与缓存回归，不是新的鼠标 FPS 基准。
- 本地验证产物：`build/cut-parity-large/run.log`、`plot-options-large.png`，以及 `build/mingw-deformation/test/native/draw-plot-options/` 的面色/云图图像；不纳入 Git。OCCT inventory 检查通过。下一工作包为 A02：完整定义 frame、局部绝对运动、滑块精度和持久化兼容；随后 A03 实现真正随材料变形的截面。
- A02 已完成：完整平面 Placement、局部 2/3 轴绝对旋转、Offset 平移、1/10/100 灵敏度、定义编辑器、JSON 8/旧版迁移、session 与时间轴保留。CSYS 在 Apply 时取快照；没有宣称自动关联 datum。详见 [平面运动指南](cut-view-manager-plane-motion.md)。修复私有 OCCT SDK 多边形拾取三角扇越界，项目派生类显式生成 n−2 个三角形，未修改外部 SDK。
- A02 最终 Release 构建通过，17 项 CTest 全通过（包含版本 7 迁移、非零运动跨帧、Manager 与实际鼠标交互）。337.5 万单元最终验收 `CUT_PLACEMENT_LARGE_PASS`：三显示模式 × mask 2/3/7，重复绝对旋转和平移，主体上传/重建均为 0，源准备次数不变；source/result 预算稳定为 `1,460,965,248` / `17,010,275,392` 字节。记录在 `build/a02-tests.log`、`build/cut-placement-large/run.log`；没有将命令批次耗时当成鼠标 FPS。下一包 A03。
- A03 第一阶段：完成 [参考构形与材料映射核心](cut-view-manager-material-sections.md)。19 项回归通过；337.5 万单元仅显示材料截面时，三模式 × 四次倍率变更均复用一次参考求交，每次仅映射 22,801 个截面节点，逻辑 source/result 稳定为 `1,461,030,784` / `381,761,856` 字节。真实大数据暴露并修复了参考/旧映射/新映射三份完整源预算重叠的问题，没有提高限额。A03 继续做当前曲面误差、退化/反演检测与扩展夹具；尚未启动 A04 的对外闭环。
- A03 最终验收：三个 portion 的当前 FE 采样、共边细分、局部法向、NaN/相对反演拒绝、失败重试释放预算、参考 Apply 原子验证已完成；二次实体/壳、非仿射 hex、梁交点、空截面与 8 个 mask 已覆盖。19 项交互回归及物理/界面/追踪回归通过。最终单独运行 337.5 万单元门禁再次通过，参考求交 1 次，每步 22,801 个节点，source/result 为 `1,461,030,784` / `445,418,560` 字节；三模式四步批次约 1.57/1.70/2.60 秒，不是鼠标 FPS。公共边界采样距离按两侧容差之和验收，没有宣称全域零误差或无反演证明。开始 A04：Manager 自由体、积分域、形心/自定义求和点和分量坐标系。
