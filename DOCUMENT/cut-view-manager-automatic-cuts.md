# 自动切面与结果数据适配

## 输入契约和生命周期

自动切面必须来自显式元数据；不会根据 `density`、`PHILSM` 等字段名称猜测。
`NativeCutAnalysis::SyncAutomaticJson(provider, json)` 接收一个数据提供者的完整快照。
先在临时 manager 验证全部记录，再替换现场；同一 provider 已消失的记录被移除，其他 provider 和手工 cut 保留。
ID 全局唯一；字段缺失、元组不匹配、重复 ID、所有权冲突或多个激活裂纹均拒绝整批输入。
显示准备失败时恢复旧 definitions，已发布的渲染数组保持可用。

```tcl
scvcutview model automaticsync solver {{
  "version":1,
  "cuts":[{
    "id":"crack-1","kind":"crack",
    "field":{"name":"signed_distance","nodal":true,"component":0,"step":-1},
    "threshold":0,"min":0,"max":1,"sign":1,"visibility":3,"enabled":true
  }]
}}
# provider 撤销全部记录
scvcutview model automaticsync solver {{"version":1,"cuts":[]}}
```

Definitions JSON 13 保存 `origin`、`provider` 和优化阈值范围，普通删除、复制、重命名和等值变量重绑拒绝自动 cut。
数据适配器专用的 `SetAutomaticMetadata` / `RemoveAutomatic` 管理所有权，不是 Manager 的编辑入口。
Manager 禁用受保护控件。Show、适用的 portion、样式是可调的显示选项；优化阈值可在元数据范围内调整。
旧 definitions 没有来源时仍为手工 cut；旧完整 session 的显式 automatic 映射可恢复所有权。
非零裂纹阈值和使用 Above 的旧映射需适配器明确修正，不能静默改成另一个裂纹。

## 几何语义与边界

- XFEM：零等值面。Below 显示完整源体表面，On 显示零等值面，Above 不可用；一次最多激活一个裂纹 component。
  `sign` 保留输入法向约定，不再交换 Below/Above。物理积分始终取独立原始 FE On 截面。
- Optimization：Below / On / Above 分别为小于、等于、大于阈值；显式 `min/max` 验证全部源值及滑块。
- `abaqus` 多 cut 策略仍强制只显示 On；`project` 扩展允许多个独立分支。
- 时间帧重新解析字段、验证 schema；缺字段时拒绝新帧，保留旧帧。provider 生命周期撤销须由适配器显式同步，不能把缺字段当作空裂纹。

本项目没有 ODB 解析器。这里实现通用零等值面及其管理语义，不会从一个无限延伸的 signed-distance 场自动推断有限裂纹前沿、富集自由度或位移间断。
ODB/VisIt 适配器须输出已经表达有效裂纹域的场/网格，或另行实现第二 level-set / active-element 域裁剪；不能把普通连续 FE 插值宣称为真实裂纹开口重构。
边界条件不是本项目 cut-owned overlay，因此没有伪造 Abaqus 的边界条件隐藏功能。

官方语义：[总体与自动 cut](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cutunderstand.htm)、
[自动对象的编辑限制](https://docs.software.vt.edu/abaqusv2025/English/SIMACAECAERefMap/simacae-c-cuthlpcopy.htm)。

## Linux 运行和测试

按 [Linux 环境指南](cut-view-manager-linux-run-test.md) 设置 `CUT_REPO/CUT_BUILD/CUT_DRAWEXE` 与插件动态库路径。

```bash
export CUTVIEW_OUTPUT_DIR="$CUT_BUILD/automatic-demo"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_automatic_test.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_automatic_test.tcl"
ctest --test-dir "$CUT_BUILD" -R '^(cut_physics|native_session|native_timeline|draw_cut_view_automatic)$' --output-on-failure
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_automatic_large.tcl"
```

目标 VisIt 完全不使用 OCCT；复用元数据 schema、所有权和事务边界，以目标渲染器的源表面/等值面分支实现相同行为。
空间加速和 FE 插值按迁移手册独立建设。数据适配器与渲染对象不能共用一个可变生命周期。
