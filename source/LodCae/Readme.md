## 模块概览

`LodCae` 封装了基于 SimMesh 的 CAE 网格简化（LOD）流程。模块输入一份 `RWCae::MeshData`，根据设定的误差阈值、目标三角形数量与元素来源（独立 2D 网格或 3D 外表面）生成新的低面数 MeshData，便于做预览、远景渲染或快速传输。

## 关键组件

- `Simplifier`（`Simplifier.h/.cpp`）  
  - `AddMesh()`：把原始 `RWCae::MeshData` 填入待简化队列。  
  - `SetSourceElemType()`：决定使用表面元素还是原始 2D 元素。  
  - `Perform(const SimplParameters&)`：执行简化，内部会：
    1. 遍历 block 树（`MeshDataBlock`）拆分域；
    2. 将域内网格转成 `SimMesh_Model`；
    3. 调用 `SimMesh_Simplifier`，按 `TargetRatio`/`AllowedError`/聚合模式 收缩顶点对；
    4. 把结果回写为新的 `MeshData`。
  - `GetResult()` 返回最终 MeshData，同时可通过 `GetMaxError()`、`NbInversions()` 观察简化质量。
- `SimplParameters`：收集停止条件、聚合模式、是否允许反转等算法参数。
- `ModelData` / `DomainData`：在多域（block）情况下缓存 `SimMesh_Model` 与最大误差，便于并行/增量处理。

## 使用建议

- 表面优先：`SourceElemType::PreferSurface` 会优先使用 `MeshData::SurfaceElements()`，可显著降低 3D 模型简化的输入规模。
- 深度控制：`TargetRatio=0.2` 配合 `AllowedError` 能保证最大偏差受控；若完全追求质量，可将 `AllowedError` 设为期望的绝对偏差并把 `TargetRatio` 置 0。
- 聚合模式：`Aggregation=true` 允许把不相连的片段合并，适合生成统一壳体；如果要求保持原拓扑区域，则保持默认 `false`。
- 完成后建议 `MeshData::SplitVertices()` 以恢复被合并的属性断裂面，避免着色/纹理出现跨面拉伸。
