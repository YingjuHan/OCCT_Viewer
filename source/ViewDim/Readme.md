## 模块概览

`ViewDim` 聚焦于 Viewer 中的尺寸标注（长度、角度、半径）。它利用 OCCT 的 AIS 体系扩展出交互式标注对象，支持拖拽、实时更新、与 CAE/几何控件的协同显示。

## 关键组件

- `InteractiveDimensionOwner`：继承自 `ViewBase::InteractiveOwner`，负责标注对象的拾取、优先级与回调转发。
- `InteractiveLengthDimension` / `InteractiveAngleDimension` / `InteractiveRadiusDimension`：分别实现三种常见量测显示，封装了：
  - 构建文字和引线的 `Prs3d_DimensionAspect`;
  - 处理鼠标拖动以更新测量点；
  - 将测量值转换成 `ViewBase::UnitManager` 当前单位。

## 使用场景

- 在 `Viewer` 模块的 `InteractiveDimensionManager` 中批量创建/销毁标注。
- 与 `ViewShape` / `ViewCae` 结合，在 CAD 或仿真模型上交互式标注关键尺寸。

## 开发提示

- 若需新增尺寸类型（例如弧长），可参考现有三个类的结构：继承 `AIS_InteractiveObject + ViewBase::IInteractiveObject`，同时实现自定义 `Compute()` 和 `HandleMouseClick()`。
- 所有尺寸文本都会自动响应 DPI 缩放，确保在高分屏上仍保持清晰。
