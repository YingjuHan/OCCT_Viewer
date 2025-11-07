## 模块概览

`RWMeshConv` 提供多种 CAE 网格格式（MED、Gmsh、自定义 ASCII/二进制）的解析与导出能力，是 `FileExchange`、`RWMeshExchanger` 等上层工具的核心依赖。模块将外部文件转成 `RWCae::MeshData`，并保留字段映射、颜色、分组及拓扑信息。

## 关键组件

- `MedConverter`  
  - 负责读取 MED 文件，拆分几何/属性缓冲并写入 MDB。  
  - 支持排序、分块、多线程、属性临时目录等选项。  
  - 在导出时会生成 `MeshFieldMapping` 缺失列表以及 `MissingFieldsPath` 报告。
- `GmshLoader` / `GmshLoaderContext`：解析 .msh（v2/v4）文件，构造 `MeshData`，包括节点坐标、单元、物理分组等。
- `MeshLoader`：对外的统一入口，根据文件扩展名或用户指定的 `RWMeshConv::FileFormat` 调用不同 loader。

## 典型流程

1. 调用 `MeshLoader::Perform(path, options)` 或 `MedConverter::SetInputPath/SetOutputPath`。
2. Loader 解析节点、元素、块数据并创建 `SimMesh_Model` 或 `RWCae::MeshData`。
3. 可选执行排序（Hilbert curve）以改善缓存局部性。
4. 输出 MDB 或直接返回 `Handle(RWCae::MeshData)` 供后续 Viewer / 分析使用。

## 扩展建议

- 新增格式时，可继承 `MeshLoader` 的抽象接口或仿照 `GmshLoader` 构造独立的 loader，再在 `MeshLoader.cpp` 注册。
- `MedConverter` 暴露了 `SetMappingPath()` 与 `SetMissingFieldsPath()`，适合与 `MeshFieldMapping` 或日志系统联动。
