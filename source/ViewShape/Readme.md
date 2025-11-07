## 模块概览

`ViewShape` 负责 CAD 形状在 Viewer 中的可视化与交互。它围绕 OCCT `AIS_InteractiveObject` 扩展了一套 `InteractiveShape` 家族，支持多显示模式（线框/着色/选择集）、多选策略以及细粒度敏感实体。

## 关键组件

- `InteractiveShape`：主交互对象，封装：
  - `PrsMode`/`SelMode` 枚举，定义不同显示/选择组合；
  - 与 `ViewBase::IInteractiveObject` 的集成（DPI、自适应 HUD）；
  - 与 `ViewShape::Sensitive*` 系列协同，实现精确拾取。
- `InteractiveShapeDrawer`：集中控制 `Prs3d_Drawer` 设置（线宽、透明度、材质），并和 `Viewer::InteractiveAspectsManager` 协作。
- `InteractiveShapeOwner`：为实体、面、边、顶点定义独立 Owner，处理命中反馈与高亮。
- `Sensitive*` 系列：`SensitivePolyFace`, `SensitivePolyEdge`, `SensitivePolyEdgeOnTri`, `SensitiveVertex`, `SensitiveCompound` 等，将多边形化数据拆分成可拾取原子单元。
- `ShapeSelElems.h`：定义不同选择要素（面、边、顶点、复合体），供 SelMode 切换。

## 常见流程

1. `RWShapeConv::ShapeLoader` 或 `RWShape::GltfParser` 生成 `ShapeData`。
2. `ViewShape::InteractiveShape` 根据 `ShapeData` 构建几何及 `Sensitive*`。
3. `Viewer::InteractiveShapeManager` 统一管理多个 Shape，对接拾取与 UI 指令（隔离、隐藏、着色等）。

## 扩展建议

- 若需要自定义拾取行为，可在 `SensitiveCompound` 基础上实现新的敏感实体，并在 `InteractiveShapeOwner` 中注册。
- 可利用 `InteractiveShapeDrawer` 的 hooks 把 `ViewBase::UnitManager` 的单位显示、材质主题等注入进来，实现一致的 UI。
