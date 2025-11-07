## 模块概览

`ViewCae` 是 CAE 结果的核心渲染层，基于 `ViewBase`/`Viewer` 的交互能力，提供标量、向量、流线、动画、注释等可视化。它直接消费 `RWCae::MeshData`，支持多属性、多域、多时间步。

## 目录梳理

- **基础工具**
  - `MeshAdaptorSensitive`：将 `RWCae::MeshAdaptor` 封装成可 pick 的 Selection 实体。
  - `MeshColorMapper` / `MeshColorScaleMapper`：根据属性范围生成颜色表、透明度与颜色条。
  - `MeshAnnotations`, `MeshPart`, `MeshSubEntity`：管理网格子部件、注释与拾取元信息。
- **交互对象**
  - `InteractiveCaeMesh`（及 Drawer/Owner/PrsMode/SelMode）：主网格展示类，支持切换显示模式、剖分、拾取。
  - `InteractiveColorScale` & Owner：二维颜色条控件，支持鼠标交互、单位同步。
  - `InteractiveSphericalAxes`：用于显示向量方向的球坐标辅助。
- **可视化派生类**
  - `MeshPresentation*` 系列：`Base`（共用逻辑）、`Scalar/Vector/Streamlines`（不同属性类型）、`MeshPresentations`（组合管理）。
  - `MeshPresentationTools` / `MeshPropertyData`：封装生成 Prs3d 几何和属性快照。
- **动态效果**
  - `CaeMeshAnimation`：驱动时间步切换和插值。
  - `CaeMeshStreamlinesBuilder`：生成流线几何。
  - `CaeMeshArrowsTools`：矢量箭头生成、缩放。

## 常见流程

1. `Viewer` 层创建 `InteractiveCaeMesh`，将 `RWCae::MeshData` 与所需属性传入。
2. 根据 UI 选择调用不同的 `MeshPresentation*` 构建展示（标量、向量、流线）。
3. `MeshColorMapper` 根据属性范围输出色标，并更新 `InteractiveColorScale`。
4. `MeshAdaptorSensitive` + `MeshSensPart` 提供拾取/量测能力，并通过 `MeshAnnotations` 生成标签。

## 与其它模块的关系

- `ViewBase`：提供通用 Viewer、Owner、2D 控件基础。
- `ViewCaeGeom`：负责几何/BVH/切片的重构，`ViewCae` 通过它获取缓存的 `MeshGeometry*`。
- `Viewer`：在管理层（Managers）中调用 `ViewCae` 的交互对象，映射到 UI 面板。
