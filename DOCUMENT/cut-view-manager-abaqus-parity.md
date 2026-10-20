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

下表按 A01–A09 的最终实现更新。“本轮完成”指已交付并验证约定的工程范围；显式列出的适配、算法及平台边界仍有效，不等于已取得 Abaqus 的同输入对照证据。最终测试与性能记录见 [A10 验收记录](cut-view-manager-acceptance.md)。

| 编号 | 用户需求 / 官方依据 | 当前状态与证据入口 | 收敛条件 |
| --- | --- | --- | --- |
| R01 | 创建/编辑/复制/重命名/删除，默认 XYZ（S2/S6） | A01 完成；独立 Rename、复制定义与显示覆盖、删除清理 | 非激活项和非法操作保留现场；物理请求由独立配置/公共模板管理 |
| R02 | 选中项与激活项分离（S1/S3） | 已有；`selected` 与 `Enabled` 分离 | 非激活项选择、编辑、重命名不激活；关闭窗口不改显示 |
| R03 | Below/On/Above 的全部组合（S4） | 已有；mask 0..7，单平面 GPU 特殊路径 | 8 种 mask、空截面、正负法向、薄板断开截面、三显示模式均正确 |
| R04 | 多 cut 独立显示（S5） | A07 完成；single / abaqus / project 显式策略 | abaqus 只显示 On、隐藏自由体并禁用显示切片；project 的 retained-body OR 为扩展，不作串联裁剪 |
| R05 | Key-in 原点、法向、平面内第二轴（S2） | A02 已验收；完整 Placement、手柄、JSON 8 和旧版迁移 | `cut_placement`、`draw_cut_view_placement`、鼠标交互回归通过 |
| R06 | 从 CSYS 创建与编辑（S2） | A02 已提供 Manager 的 CSYS/法向轴入口；Apply 获取 datum 快照 | 改 datum 后重新 Apply；显式刚性 follow 独立存在。自动关联 datum 后续扩展，不能声称已实现 |
| R07 | 平移、精细灵敏度、位置输入（S3） | A02 已验收；1/10/100、非激活输入、禁用滑块 | 范围以当前位置为中心；切换精度与运动模式不编辑几何 |
| R08 | 关于原始局部 2/3 轴的绝对旋转（S3） | A02 已验收；保留定义轴/pivot，绝对角度，旋转清除平移 | 重复角度、切轴、Escape、跨帧与 session 回归通过 |
| R09 | 柱、球的中心/轴/半径（S2/S3） | 既有路径通过 A10 回归；自研通用单元内核 | 径向 Below=内部；保持 CPU 路径，支持范围与退化输入明确报告，不套用平面 GPU 快路 |
| R10 | Isosurface 绑定创建时的变量（S1/S2） | A05 完成；独立变量、派生量、恢复及 averaging 快照 | 改云图不改求交；显式重绑；高级不连续单元迹不支持 retained-body OR，独立 On 可用 |
| R11 | 三部分渲染方式、边线、颜色、透明度（S7） | A01/A07 完成；三页、动态 plot 继承、面/边色与线型、作用域 | 透明/混合/图案边按能力进入 CPU fallback；壳云图切线仍连续，见显示指南 |
| R12 | 云图/线框/云图+线框实时剖切 | 已有；`MeshCutGpuPresentation` | 无变形与变形数据、mask 2/3/6/7；拖动时主体提交字节为 0，空截面无残影 |
| R13 | 固定空间切面对变形网格剖切（S2 默认） | 已有；`SetDeformation` | 位移正/负/零、多倍率、字段插值与预算稳定；不把 FE 字段重新解释为空间坐标函数 |
| R14 | Follow model deformation，First/Last/Current 参考帧（S2） | A03 平面材料模式已验收；真实 FE 权重映射、当前曲面采样和局部法向，三节点刚性 frame 独立保留 | 平面范围闭环；保持非平面 cut 现有行为。采样误差/参数图支持边界见材料指南，不声称任意阶全域认证 |
| R15 | 时间/倍率动画中的 cut（S1） | A03/A05/A08/A09 完成约定帧绑定；字段、参考构形、自动来源和模板重绑 | 原子候选帧，失败保留现场；重网格无材料/显式集合对应关系时拒绝，不猜测映射 |
| R16 | Manager 每 cut 自由体开关（S4/S8） | A04 完成；每 cut 独立请求/enable、合力/矩/热流及覆盖清理 | Above/Below 法向和符号、关闭不改几何、A07 多 cut 限制及 A09 模板均有回归 |
| R17 | 积分域：整体/显示组/单元集（S8） | A04 完成；标签集合、显式 IDs、真实显示组快照 | 空域不等于全体；跨帧解析，重网格显式映射不完整则拒绝 |
| R18 | 求和点：截面形心/自定义（S8） | A04 完成；体面积/壳厚度/梁交点与自定义点 | 验证形心与力矩平移公式；退化/零测度明确处理 |
| R19 | 法/切向与 CSYS 分量向量（S8） | A04 完成；稳定 NT frame、输出 CSYS、分量/合向量 | 平行切向拒绝；数值计算与显示变换分离 |
| R20 | 实体/壳/复合/梁结果及热流（S1/S8） | A04 完成显式 FE 契约；原始 provenance、单位/分量/厚度/层/截面映射 | 仍需目标求解器适配；没有 ODB NFORC/SF/SM 自动读取，不把节点内力或截面结果冒充实体应力 |
| R21 | 切片范围/规则间距/旋转/路径（S9） | A06 完成空间平面集合；节点/等弧长路径与 parallel transport | 原子替换/删除、单源显示、集合快照；材料平面集合及光滑精确 Frenet frame 不在本轮范围 |
| R22 | 切片上的自由体分布与报告（S8/S9） | A06 完成；Manager 扫描/CSV、interval_units、路径求和点 | 临时视图逐片事务，失败不返回半份报告；不污染现场，预算受控 |
| R23 | 自动 XFEM/优化 cut（S1/S6） | A08 完成显式 provider 快照、保护、特殊 mask、替换/撤销与持久化 | 无 ODB 元数据发现；有限裂纹域/富集位移需额外适配，单一零等值面不能重构真实裂纹开口 |
| R24 | 状态持久化（S2/S6） | A09 完成；definitions 13 / session 4，读取旧 session 1/2/3 | 样式、plot、材料/iso/集合/来源/公共模板；源 SHA-256 校验，definitions Save 不保存完整 Viewer；不兼容 Abaqus XML/ODB |
| R25 | 多视口隔离 | A09 完成；共享只读源，独立 manager/BVH/时间轴；全部自有覆盖与 GPU affinity | 实际双视口图像、CAD/标注/箭头/session 验证；Manager 单窗口，临时状态按模型/视口暂存 |
| R26 | 性能、内存预算与取消（S1） | A10 测试与测量完成，性能目标仍未达到 | 337.5 万数据暖帧 body 上传/重建为 0，八组预算门禁通过；Iris Xe 全帧平移/旋转/变形约 14.91/8.05/6.68 FPS，均未满足 15 FPS+p95≤67 ms；4/16 GiB 限额不变 |

## 3. 开发顺序和可交付工作包

| 工作包 | 优先级 | 本轮状态 | 内容 / 依赖 |
| --- | --- | --- | --- |
| A01 Manager 显示与对象管理入口 | P0 | 本包完成 | R01/R11 的独立 Rename、Options 三页、样式覆盖查询/颜色输入、复制/删除样式生命周期；不代表 R11 全部收敛 |
| A02 完整坐标系和运动模型 | P0 | 本包完成，CSYS 为显式快照 | R05/R07/R08 验收；R06 创建编辑入口完成，动态 datum 关联保留为扩展边界；见平面运动指南 |
| A03 真正随变形截面 | P0 | 本包完成（平面材料模式） | First/Last/Current、FE 权重、参考侧别、缓存、JSON 9、三个 portion 曲面采样、当前法向、失败回滚和百万单元验收；参数图/采样边界见材料指南 |
| A04 Manager 自由体工作流 | P1 | 本包完成（显式 FE 适配契约） | 独立开关/面板、形心/自定义点、符号/NT/CSYS、标签集合/实际显示快照、材料梁参考权重；15 项回归及百万门禁通过；公共默认模板随 A09 一并持久化 |
| A05 Isosurface 完整绑定与 averaging | P1 | 本包完成 | R10；独立派生量/平均快照、显式重绑、JSON10、时间帧及 Manager 回归；337.5 万单元实际求交通过，见等值面指南 |
| A06 切片集合与报告 | P1 | 本包完成（空间平面集合） | R21/R22；规则/范围/旋转/路径等弧长、成组替换与删除、JSON11/session/时间轴、逐片 FE 扫描及 CSV；337.5 万单元三切片门禁通过 |
| A07 全部 plot options 与多 cut 兼容 | P1 | 本包完成 | R04/R11；动态 plot 继承、独立面/边色与线型、single/abaqus/project 策略；图案边走 CPU，壳云图切线仍连续，见显示指南 |
| A08 自动 cut 与数据适配 | P2 | 本包完成（显式元数据） | R23；provider 快照、自动移除、定义保护、XFEM 特殊显示和优化阈值、跨帧/session；ODB 发现和有限裂纹域适配不在通用零等值面契约内 |
| A09 持久化与视口隔离 | P1 | 本包完成 | R24/R25；session 4/definitions 13、旧版迁移、公共物理模板、共享只读源与独立 BVH、全部自有 AIS 对象/GPU 结构 affinity、Manager 临时状态隔离 |
| A10 大规模性能及对照验收 | 持续 | 本轮测试/测量交付完成，性能与外部对照有保留项 | 84 个注册用例均有通过证据，八组 337.5 万门禁通过；全帧目标未达，Abaqus 同输入对照及 Linux 实跑仍待环境；详见验收记录 |

每包结束单独提交；矩阵按实际测试更新。总进度按通过验收的需求条目统计，当前不提供缺少分母与验收证据的百分比。

## 4. 关键设计约束

### 4.1 运动与参考构形

增加 `DefinitionFrame`（原点、法向、平面内轴）、`Motion`（绝对位移、角度、旋转轴）、`CutConfiguration`（Spatial/Material）和 `ReferenceFrame`（First/Last/Current 实际解析后的 step ID）。用户输入与求交使用的世界坐标平面分离。保留旧 JSON 的平面方程读取，通过确定性轴构造迁移，不能伪造丢失的历史旋转姿态。

Spatial 模式按当前节点位置与空间平面求交，保持现有 GPU clipping + CPU slice 优化。Material 模式在参考构形求交并保留单元 ID、自然坐标和形函数权重，然后计算 `x(t,ξ)=Σ N_i(ξ)[X_i+s·u_i(t)]`。不同点的位移不同，截面可弯曲；材料两侧也须按参考参数分类，因此该模式不能直接套用一个世界平面 GPU discard。应建立专用参考符号场/拓扑路径与后端能力判断，避免修改现有非平面切割行为。

改切面定义、参考 step 或拓扑时失效参考交点；仅切时间/倍率时复用参考交点并更新位置和场。重网格必须有显式材料对应映射，缺失时拒绝跟随并说明原因。反演/退化单元不默默转成空间切面。

### 4.2 显示选项与状态边界

官方 S7 的三部分 plot options 是视口级设置；本项目单 cut 样式覆盖是扩展。UI 明确区分“Model defaults”“Selected cut override”和“Current plot options”。A01 完成三页与作用域，A07 已加入独立 PlotStyle 和动态 Use current plot options。

显示修改不能更改原始 FE 字段、物理 provenance、cut 几何与启用状态。A07 已实现有字段时仍可独立选择单色/云图，以及边色/线型。混合模式、透明度和图案边可能进入 CPU fallback，可用 `renderinfo` 查明；壳云图切线仍为连续色线，不能承诺全部选项均 GPU 加速。

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

A10 新测量是同步 API 驱动的完整 redraw，使用 Iris Xe、20 帧/场景，平移/旋转/变形约 14.91/8.05/6.68 FPS；不是上述三显示模式的鼠标拖动同协议复测，不计算加速倍数。最新分阶段记录和全部保留项见 [验收记录](cut-view-manager-acceptance.md)。

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
- A07 已完成：Release 构建与 12 项相关 CTest 通过；修复 CPU 单色属性容器崩溃及图案边被重置问题。337.5 万单元三模式 × mask 2/3/7 门禁通过，plot 继承和单色切换后暖移动无主体重建/上传，source/result 保留 1,460,965,248 / 597,683,072 字节，限额仍为 4/16 GiB。日志 build/a07-final-tests.log、build/cut-plot-parity-large/run.log；索引已 full+persistence 刷新，OCCT inventory 检查通过。详见 [显示与多 cut 指南](cut-view-manager-plot-parity.md)。
- A08 已完成：8 项数值、session、时间轴与 Manager 回归通过。337.5 万单元两个优化阈值整批替换并撤销成功，批次 17,715.690 ms，source/result 保留 1,460,965,248 / 460,855,296 字节，4/16 GiB 限额不变；不是鼠标 FPS。修正了 nodal scalar 绑定预验证按完整裁剪结果预留预算的问题，验证只借用标量句柄。日志 build/a08-final-tests.log、build/cut-automatic-large/run.log；持久索引已刷新。详见 [自动切面契约与限制](cut-view-manager-automatic-cuts.md)。
- A09 已完成：10 项初步回归及扩大后的 7 项回归通过，覆盖 session 1/2/3 迁移、模板跨帧、双视口实际图像、CAD/标注/自由体覆盖和 session 恢复。修复 OCCT 首次 Display 注册时将 affinity 重置为所有视口可见的串显根因。337.5 万单元双视口门禁通过，A 四次暖移动批次 743.501 ms、主体上传为 0，B 定义/预算完全不变；每视口 source 1,460,965,248，A/B result 597,683,072 / 561,011,072 字节。日志 build/a09-scope-fix-tests.log、build/cut-scope-large/run.log；索引已持久刷新，OCCT inventory 已核对。详见 [会话与多视口指南](cut-view-manager-session-scope.md)。
- A10 本轮测试/测量交付完成：首轮 81/84，修正 definitions 版本断言、旧裂纹阈值测试和薄板 420 秒超时后，5/5 复测通过；薄板全部 384 张图耗时 485.65 秒。基准倍率参数编译后 smoke 再次通过；八组 337.5 万单元门禁全部通过，未提高 4/16 GiB 限额。完整 redraw 的三个场景均未达到组合性能目标，记录实际 FPS/p95 和合并/trace/节点/渲染开销，未冒充性能达标。迁移手册补齐 A06–A09，Linux 直接/GDB/全量与百万命令已核对但未在 Linux 实跑；Abaqus 同输入对照仍缺环境。证据见 [A10 验收记录](cut-view-manager-acceptance.md)。
