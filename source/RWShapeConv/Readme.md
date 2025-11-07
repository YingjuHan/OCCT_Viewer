## 模块概览

`RWShapeConv` 提供 `TopoDS_Shape` 与 `RWShape::ShapeData` 之间的互转能力，侧重在工程模型（STEP/BREP）的加载与简化后再输出 glTF/自定义缓存。它与 `RWShape`、`ViewShape` 一起构成形状可视化链路。

## 关键组件

- `ShapeLoader`  
  - `LoadShape()`：调用 OCCT reader（STEP/IGES/BREP 等）读取 CAD 模型，并可根据参数生成离散化网格。  
  - `LoadShapeData()`：把 `TopoDS_Shape` 转换为 `ShapeData`，包含三角面、显示属性与分组信息。  
  - `DumpShapeData()`：序列化 `ShapeData` 以便缓存重用。

## 流程示例

1. 用户在 Viewer 中选择 STEP 文件 → `ShapeLoader` 读取成 `TopoDS_Shape`。
2. 调用 `LoadShapeData()` 生成三角网格/材质等可视数据。
3. 结果传给 `ViewShape::InteractiveShape` 进行交互展示，或经 `RWShape` 导出 glTF。

## 扩展建议

- 在加载大型装配时，可利用 `ShapeLoader` 现有的分块输出，把每个子 Shape 转成单独的 `ShapeData`，再在 Viewer 侧按需加载。
- 若需要支持减面，可在 `ShapeLoader` 侧集成 `LodCae` 或嵌入式网格简化步骤。
