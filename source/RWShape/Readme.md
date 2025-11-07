## 模块概览

`RWShape` 负责几何形状（TopoDS_Shape）与中立文件之间的读写。当前实现聚焦于 glTF/GLB 流程以及形状数据的轻量化缓存，便于在 Viewer 中做快速预览。

## 关键组件

- `GltfParser`：解析 glTF 结构，读取节点树、几何缓冲及材质，生成 `RWShape::ShapeData`。支持标准的 PBR 参数、法线/UV、实例重复等特性。
- `ShapeData`：封装了多种面片、折线、属性的集合，提供合并、过滤与序列化能力，是 `ViewShape` 的主要输入。
- `ShapeDataExplorer`：用于遍历 `ShapeData` 中的部件，生成 UI 列表或导出统计信息。

## 常见场景

- 将 glTF 模型导入 `ViewShape`，用于与 CAE 结果叠加显示。
- 作为形状缓存，供 `Viewer` 在 WebAssembly 或桌面端快速加载。

## 扩展建议

- 若需支持更多格式，可在 `GltfParser` 的模式上新增解析器，并把 `ShapeData` 作为统一抽象层。
- `ShapeData` 中每个实体都带材质/可见性 flag，可直接与交互 Owner（`InteractiveShapeOwner`）绑定以实现选择/隔离。
