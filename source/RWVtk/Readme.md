## 模块概览

`RWVtk` 专注于 VTK 系列文件（Legacy、XML、MultiBlock、Series）的读取，将其转换为 `RWCae::MeshData` 或中间缓存，供 `ViewCae` 等模块直接消费。模块覆盖 PolyData、UnstructuredGrid 以及基于时间序列的数据集，是连接科研数据与可视化的桥梁。

## 关键组件

- `VtkBaseReader`：提供通用的文件打开、缓冲解析、错误处理逻辑。
- `VtkBaseXMLReader`：扩展了 XML 解析与多块（PVD、PVTU 等）遍历。
- `VtkPolyDataReader` / `VtkUnstructuredGridReader` / `VtkUnstructuredDataReader`：依数据类型将节点、单元、属性写入 `MeshData`。
- `VtkMultiBlockDataReader`：解析多块结构并构建 `MeshDataBlock` 树，保留层级关系。
- `VtkSeriesReader`：加载 `.series` 或多个时间步文件，提供时间轴信息。
- `VtkDataArray`：封装 VTK array 到内部标量/向量属性的映射，支持多种数值类型与字节序。

## 使用建议

- 读取大规模时可优先使用 `.series` + `VtkSeriesReader`，可在 `Viewer` 中按需 streaming。
- 若文件包含多个 block，可在 `ViewCae` 中通过 `MeshDataBlock` 直接构建 domain 树，实现按 block 显隐。
- `VtkBaseReader` 暴露了 `SetToSort()`，可结合 `MeshTools::SortMeshData()` 优化渲染性能。
