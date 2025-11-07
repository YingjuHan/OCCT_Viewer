## 模块概览

`ViewCaeGeom` 提供 CAE 网格的几何重建与空间索引服务。它将 `RWCae::MeshData` 转换为可重用的几何缓存（剖切、等值面、变形、BVH 等），从而让 `ViewCae` 在切换属性/视图时无需重新遍历原始数据。

## 关键组件

- `MeshGeometry*` 家族：每个派生类代表一种几何视图或处理方式。
  - `MeshGeometry`：基础接口，负责管理 `MeshGeometryBlock`、GPU 缓冲以及脏标记。
  - `MeshGeometryDeformed`、`MeshGeometryClip`、`MeshGeometrySlice`、`MeshGeometryIso` 等处理不同的可视化需求。
  - `MeshGeometryGen` / `MeshGeometryTools`：生成和复用几何块，提供多线程构建、属性拷贝、法线更新等工具。
- `MeshBvhData` / `MeshHBvhBoxSet`：用于构建层次包围盒，支撑拾取、快速裁剪与空间查询。
- `MeshGeometryBlock`：与 `RWCae::MeshDataBlock` 对应，存储每个域的几何缓冲与 AABB。

## 常用流程

1. `ViewCae` 请求某种几何（例如等值面）→ `MeshGeometryGen` 检查缓存是否存在。
2. 若无缓存，则根据 `MeshGeometryBlock` 遍历域，生成顶点/索引缓冲、属性、BVH，并存入 `MeshBvhData`。
3. `MeshGeometryClip/Slice` 可在已有缓冲基础上应用平面裁剪或切片，避免重复构建。

## 开发提示

- 新增几何处理方式时，可继承 `MeshGeometry` 并复用 `MeshGeometryTools` 的数据访问器。
- `MeshBvhData` 中的 BVH 可与 `ViewBase::InteractiveOwner` 结合，实现更高效的拾取过滤。
