## 模块概览

`Viewer` 组装了项目的完整交互式查看器：管理 OCCT `AIS_InteractiveContext`、UI 控件、渲染参数、WebAssembly 导出等。它位于 `ViewBase` 与具体业务模块（ViewCae/ViewShape/ViewDim 等）之间，提供统一的 Manager 层与工具函数。

## 目录指南

- **核心管理器**
  - `InteractiveContext`：封装 `AIS_InteractiveContext` 初始化、拾取/高亮策略、Z-Layer 管理。
  - `ViewManager` / `IManager`：抽象出 Manager 基类（生命周期、事件接口），`ViewManager` 负责视图与渲染器配置。
  - `SelectionManager`：处理选择策略、过滤器与多选逻辑。
- **互动对象管理**
  - `InteractiveCaeMeshManager`、`InteractiveShapeManager`、`InteractiveDimensionManager`、`InteractiveClippingManager` 等负责对应交互对象的创建、更新与销毁；它们调用 `ViewCae`、`ViewShape`、`ViewDim` 的类。
  - `InteractiveColorScaleManager`、`InteractiveScaleRulerManager`、`InteractiveTextLabelManager`、`InteractiveViewCubeManager` 等管理 HUD/控件。
  - `InteractiveAxesManager`：处理坐标轴与场景对齐。
- **渲染与参数**
  - `InteractiveAspectsManager`：统一设置 `Prs3d_Drawer`、材质、阴影、透明度等。
  - `RenderingParams`：封装视图、抗锯齿、阴影、后处理参数。
  - `UnitManager`：在 Viewer 层重新暴露单位设置，供菜单/命令面板使用（对接 `ViewBase::UnitManager`）。
- **平台特定**
  - `WasmViewer` / `WasmModulePrinter`：WebAssembly 导出入口，暴露 C 接口供 JS 调用。
  - `Utils/` & `ts/`：提供 TypeScript/Web 包装器、DOM 互操作、事件桥接。

## 工作流程

1. 启动时创建 `ViewManager` 和若干 `Interactive*Manager`，注册到 `IManager` 列表。
2. 某个 Manager（例如 `InteractiveCaeMeshManager`）接收 UI 命令，调用 `ViewCae::InteractiveCaeMesh` 构建/更新实体。
3. `SelectionManager` 监听鼠标事件，将拾取结果派发给对应 Owner/Manager。
4. `RenderingParams` 变化后广播至所有 Manager，以刷新阴影、抗锯齿、背景等设置。

## 扩展建议

- 若新增业务模块，可仿照已有 Manager 编写一个继承 `IManager` 的类，将其注入 `Viewer::InitializeManagers()`；这样就能自动接入选择、序列化与命令系统。
- WebAssembly 目标需要在 `WasmViewer` 中显式导出接口，并在 `ts/` 下编写对应的 TypeScript wrapper。
