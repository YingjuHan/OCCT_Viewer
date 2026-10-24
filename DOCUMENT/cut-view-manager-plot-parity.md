# Plot options 与多 cut 模式

## 外观状态

`MeshCutDisplayStyle` 分离面色来源、面 RGB、边 RGB、线型、宽度、渲染方式和透明度。
`UsePlotOptions` 从视口当前 `PlotStyle` 动态解析；三个 portion 的模型默认值和单 cut 覆盖仍分别保存。
单 cut 覆盖是项目扩展，Abaqus 的 common plot options 对应这里的 Model defaults / Current plot options。
显示云图变量继续独立保存；选择单色只改变渲染，不删除 FE 字段，也不改变物理积分或探测值。

在 Manager 的 View Cut Options 中选择作用域，勾选 Use current plot options，
或关闭继承并编辑本页。Color faces by result field 关闭时使用单色；边可单独指定颜色和线型。
线型 0/1/2/3 为实线/虚线/点线/点划线，线宽为像素。

```tcl
# 参数依次为透明度、宽度、mode、面 RGB、边 RGB、线型、云图面色、plot 继承。
scvcutview model cutstyle defaults 1 0 2 2 .8 .2 .1 0 0 1 0 0 0
scvcutview model cutstyle plot 0 0 2 2 .2 .7 .5 0 0 0 0 1 0
scvcutview model cutstyle defaults 1 0 2 2 1 1 1 0 0 0 0 1 1
```

实线平面在现有 GPU 路径上按 Below/Above/On 独立选取云图或单色，主体边色也按侧别解析。
透明度、混合渲染方式和有图案的边进入 CPU 路径，可用 `renderinfo` 查询原因。
CPU 网格边使用 OCCT 的 line stipple；壳切线的云图 shader 仍是连续色线，线型选项主要控制网格边。
不能据此宣称数百万单元的虚线 CPU fallback 也有与实线 GPU 一样的交互性能。

## 三种多 cut 策略

`scvcutview model multimode` 可查询/设置：

| 值 | 行为 |
| --- | --- |
| `single` | 只允许一个手工源 cut 激活；显示其保存的 portion mask；可显示切片集合 |
| `abaqus` | Allow Multiple Cuts 兼容模式：所有激活 cut 仅显示 On；隐藏自由体覆盖；禁用 portion/自由体编辑；切片不能同时显示 |
| `project` | 保留既有独立 cut retained-body OR 扩展；允许各 cut 的 portion 与自由体；不作串联裁剪 |

低层 API 和旧文件默认 `project`，避免升级时改变已有场景；Manager 顶部可明确选择 `abaqus`。
兼容模式只覆盖显示，不改保存的 mask、不删除物理请求，退出后恢复原设置。
隐藏的物理报告仍可用于离线扫描。兼容模式中的多个切面独立显示，不运行主体布尔联合。
不连续等值面 retained-body OR 仍明确拒绝；应使用独立单 cut 或兼容模式的 On 显示。
Definitions JSON 12 保存策略；完整 session 另保存 plot style、默认值和 cut 覆盖。

官方依据：[多 cut](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpmultiple.htm)、
[plot options](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-t-cuthlpcustomize.htm)。

## Linux 验证与无 OCCT 迁移

先按 [Linux 指南](cut-view-manager-linux-run-test.md) 切换 Release/Debug 动态库和插件环境。

```bash
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_plot_parity_test.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_plot_parity_test.tcl"
ctest --test-dir "$CUT_BUILD" -R '^(cut_display|cut_union|native_session|native_timeline|draw_cut_view_plot_parity|draw_cut_view_plot_options|draw_cut_view_controls)$' --output-on-failure
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_plot_options_large.tcl"
```

VisIt 目标完全不使用 OCCT。保留同样的样式解析优先级、显示策略和字段独立性，
在现有 OpenGL 渲染对象中实现每侧的颜色/字段选择、边图案和透明 pass。
不要把 source FE 缓存与 portion style 绑定在同一个失效键上。
