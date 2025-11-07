## 模块总览

`SimMesh` 是项目的核心网格简化库，围绕 **SimMesh_Model → SimMesh_Simplifier → RWCae::MeshData** 这条链路实现。模块可将 `TopoDS_Shape` 或 `RWCae::MeshData` 转成内部的轻量模型，在保持属性（法向、颜色、纹理坐标、分组）与拓扑一致性的前提下降低面数。

- 架构图参考：`SimMesh_architecture.drawio`
- UML 关系图参考：`SimMesh_uml.drawio`

## 主要组件

| 组件 | 说明 |
| --- | --- |
| `SimMesh` | 对外入口，提供 `ModelFromShape()`、`ShapeFromModel()`、`ExtractTriangulation()` 等静态方法，方便在 CAD 与简化模型之间转换。 |
| `SimMesh_Model` | 持有顶点、三角形、边及每个顶点的属性列表。提供 `AddVertex/triangle/edge`、`GetTriangleNormal()`、`SplitVertices()`、`CompactVertices()` 等操作。 |
| `SimMesh_Simplifier` | Quadric Error Metrics 基础上的收缩算法，实现优先队列、边界约束、反转检测、聚合模式等。可配置目标三角形比例和最大误差，并返回统计信息。 |
| `SimMesh_Persistence` | 按版本化格式 (`CurrentVersion=1`) 读写 `.smesh`，便于缓存中间结果或调试。 |
| 几何/算法工具 | `SimMesh_Vertex/Triangle/Edge`、`SimMesh_Attributes`、`SimMesh_VertexDataList`、`SimMesh_VertexPair*`、`SimMesh_Quadric` 等组成底层数据结构与误差计算器。 |

## 工作流程

1. **建模**：调用 `SimMesh::ModelFromShape()` 或由 `LodCae::Simplifier` 按 block 拆分，生成 `SimMesh_Model`。  
2. **配置参数**：通过 `SimMesh_Simplifier::Initialize(model)`、`SetAggregationMode()`、`SetTargetRatio()`、`SetAllowedError()` 等函数约束简化结果。  
3. **执行收缩**：`Perform()` 期间生成顶点对（真实边 + 虚拟边），计算 Quadric/边界误差、检查反转、合并属性，直到达到目标或误差上限。  
4. **回写结果**：可通过 `SimMesh::ShapeFromModel()` 重建形状，或在 `LodCae`/`ViewCae` 中把 `SimMesh_Model` 转换为 `RWCae::MeshData` 继续渲染。

## 集成建议

- 简化完成后调用 `SimMesh_Model::CompactVertices()` 和 `SplitVertices()`，避免多余孤立顶点及属性断裂。
- `SimMesh_Simplifier` 可报告 `GetMaxError()`、`GetMaxDeviation()` 等指标，用于 UI 的质量提示。
- 如果需要持久化中间状态（便于断点或测试），可利用 `SimMesh_Persistence::Save/Load` 直接序列化模型。

## 与其它模块的关系

- `LodCae`：在 `Simplifier` 中批量创建 `SimMesh_Model`，并调用 `SimMesh_Simplifier` 生成 LOD。
- `ViewCae` / `Viewer`：可使用 `SimMesh::ExtractTriangulation()` 把结果作为 `Poly_Triangulation` 直接送入 OCCT Viewer。
