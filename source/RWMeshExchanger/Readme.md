## 模块概览

`RWMeshExchanger` 是面向多种 CAE/几何求解器的网格交换层。它将外部数据结构（SMDS、MED、专有块等）转换为项目内部的 `RWCae::MeshData`，并且负责属性、拓扑、块树与单位信息的双向映射。

## 关键组件

- `MeshExchanger`：统一入口。根据输入路径/流类型选择合适的读取器，并驱动 `RWCae::MeshImportExport` 生成 MDB 或内存模型。
- `MeshExchangerUtils`：封装了常见的三角/四边形复制、属性拷贝、ID 映射等工具，供 Reader & Writer 复用。
- `MedFileUtils`：包含基于 MED API 的低层封装，提供单元遍历、场数据读取、时间步管理等操作。
- `Smds2MeshDataConvert`：将 SMESH/SMDS 数据结构转换成 `MeshData`，包括边界组、分区与字段。

## 典型使用场景

- 在 GUI 或 CLI 中读取求解器生成的 MED/UNV/自定义网格，并转成 RWCae 结构供 `ViewCae` 或 `LodCae` 使用。
- 把 `RWCae::MeshData` 导出为其他格式以便回写到求解器或第三方工具。
- 作为 `ViewCae` 的“数据源插件”，实现在线加载/切换不同求解结果。

## 开发提示

- 所有 reader 写入 `MeshData` 时应同步维护 `MeshDataBlock`（分区树）与 `MeshNVecProperty`（字段），以便在 Viewer 中做分域拾取。
- 可以利用 `MeshExchanger` 暴露的回调获取读写进度并整合到 UI（例如 `Viewer` 模块的 `SelectionManager` 进度条）。
