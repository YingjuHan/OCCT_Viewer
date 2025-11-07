## SCRWCae模块总览

SCRWCae（Read/Write CAE）模块是专门用于处理CAE（计算机辅助工程）网格数据的模块，主要包括网格数据的读取、写入、压缩等功能。该模块围绕网格数据结构和相关属性进行设计，提供了完整的网格数据处理能力。

## 各文件详细分析

### 1. MeshAdaptor.h / MeshAdaptor.cpp
网格适配器类，用于适配不同的网格数据格式：

- 定义了网格适配器的接口和实现
- 提供了对网格数据的统一访问接口
- 可能用于连接不同格式的网格数据与应用程序内部表示
- 模板化设计直接绑定具体 `MeshElemType` 的容器，暴露 `ElementFaces` / `NbFaces` / `FaceNodes` / `TrianglesIterator` 等方法，避免重复解析底层 packed storage。
- 附带 `PointElementChecker`、`ElementCenter`、`ComputeVectorTrsfRatio` 等算法，可拆解面片并判断点是否落在单元内部，常用于拾取、重采样与属性插值。

### 2. MeshAdaptorsIterator.h
网格适配器迭代器：

- 提供对网格适配器集合的迭代访问能力
- 允许遍历多个网格适配器实例
- 通过 `Switch` / `For` 两种模式把算法与具体元素类型解耦，模板参数在编译期展开，省去了手写 `if/else`。
- 预置全元素、2D、3D 三套类型组合，结合 `MeshAdaptor` 能一键遍历表面单元或体单元，方便批量计算。

### 3. MeshBinIOBase.h
网格二进制IO基础类：

- 定义网格二进制读写的基类接口
- 为网格数据的二进制序列化提供基础框架
- 集中管理 `MeshDataImportExportOptions`、压缩配置以及 `FileOffsets` 结构，reader/writer 共用一份状态。
- 内建 `SharedMapOfAsciiString` 过滤表，可按属性 ID + 时间步范围筛选要处理的数据，实现增量加载。

### 4. MeshBinReader.h / MeshBinReader.cpp
网格二进制读取器：

- 实现从二进制格式读取网格数据的功能
- 解析二进制网格文件并构建内部网格数据结构
- 读取流程先解析 JSON 元数据（资产信息、缓冲区、块树、属性描述），再按字段定位二进制缓冲，确保 `MeshData` 与属性一次填充完成。
- 同时支持无压缩与 Draco 压缩缓冲，`UpdatedProps()` 可返回此次读取被刷新过的属性集合，便于上层只更新必要的可视化。

### 5. MeshBinWriter.h / MeshBinWriter.cpp
网格二进制写入器：

- 实现将网格数据写入二进制格式的功能
- 将内部网格数据结构序列化为二进制文件
- 导出时严格区分 Header / JSON / BIN 三段，JSON 存放几何、块、属性元数据，BIN 段存放结构化的 nodes、elements、properties 缓冲。
- 支持按需强制单精度、写入 Draco 量化设置，并通过 `Message_ProgressRange` 汇报进度，方便 UI 展示长耗时任务。

### 6. MeshCompressionHelper.h
网格压缩助手基础类：

- 定义网格数据压缩的辅助接口
- 为不同的压缩算法提供统一的接口
- `MeshCompressionTypeConvertor` 提供枚举与字符串的双向映射，方便在 JSON 中序列化压缩方式。
- `MeshDracoCompressionInfo` 记录节点/属性量化位数和分片数，`NbParts(size)` 可根据数据尺寸自动拆分块，便于流式并行处理。

### 7. MeshData.h / MeshData.cpp
网格数据核心类：

- 定义网格数据的主要结构和接口
- 包含网格的基本信息如节点、单元等
- 是整个网格数据体系的核心类
- 继承 `Poly_Triangulation`，额外维护节点颜色/法向/UV、`IndexedElement` 索引和 block 树，并缓存 AABB 以支撑快速包围盒查询。
- `AddElement` / `SurfaceElements` / `ChangeElements` 等接口将不同拓扑类型分区存储，还提供 `AddNodePositionChecked` 以容错 NaN 输入。
- 与 `MeshNVecProperties` 深度耦合，析构或 `Clear()` 时会同步清理属性依赖，保证 `Handle` 引用不会泄漏。

### 8. MeshDataBlock.h / MeshDataBlock.cpp
网格数据块：

- 表示网格数据的一个逻辑块
- 可能用于分块加载或处理大型网格数据
- 每个 block 记录 `MeshDataRange`，`FillAndSortRanges()` 能自动计算并按范围排序子块，方便 UI 构建树。
- 支持 `AppendChild` / `GetChild` / `Path` / `ToJson` / `FromJson` 等操作，可把块层级写入 MDB 元数据并恢复。

### 9. MeshDataRange.h
网格数据范围：

- 定义网格数据的范围概念
- 可能用于指定数据子集或区间
- 同时跟踪节点/单元的上下界，`NbNodes` / `NbElems` 用于快速统计子集大小。
- `AppendRange` 可把多个范围合并成包络区，通常配合 `MeshDataBlock` 计算父 block 的覆盖范围。

### 10. MeshDataVector.h
网格数据向量：

- 定义网格数据的向量表示
- 可能用于存储向量型的网格属性数据
- 在 `NCollection_Vector` 基础上增加 `IsContinuous` / `NbBlocks` / `Increment` 等探测接口，可判断数据是否位于连续内存，方便直接传给 OpenGL/Draco。
- 构造函数支持自定义增量与分配器，适配不同规模的网格缓存策略。

### 11. MeshDracoCompressionHelper.h / MeshDracoCompressionHelper.cpp
Draco网格压缩助手：

- 实现基于Draco库的网格数据压缩功能
- Draco是由Google开发的3D几何网格压缩库
- 提供高效的网格数据压缩和解压缩能力
- `.cpp` 中封装了与 Draco 库的量化、切片和错误处理逻辑，被 MeshBinReader/MeshBinWriter 直接复用。
- 负责在节点与属性之间切换不同压缩参数，保证解压后的拓扑索引仍能与 `MeshData` 对齐。

### 12. MeshElement.h
网格元素基础定义：

- 定义网格元素的基本概念和接口
- 网格元素是构成网格的基本单位
- 覆盖所有一次/二次单元（triangle、quad、tetra、prism、polyhedron 等）的节点数量、访问接口与面信息，是 `MeshAdaptor` 的工作基础。
- 通过轻量结构和 `operator[]` 暴露 packed node index，使算法能以统一方式处理不同单元。

### 13. MeshElements.h
网格元素集合：

- 定义网格元素的集合管理
- 提供对多种类型网格元素的统一管理
- 封装了每种元素类型的 `Nodes` / `NodesOffsets` / `Faces` / `FacesOffsets` packed array，确保拓扑数据在内存中紧凑存储。
- `NbElements`、`GlobalIndex`、`SurfaceElements` 等接口与 `MeshAdaptor` / `MeshData` 协作，加速 typed 元素的检索和修改。

### 14. MeshFieldMapping.h / MeshFieldMapping.cpp
网格场映射：

- 实现网格数据场与物理场之间的映射关系
- 用于处理仿真结果数据与网格几何的关联
- Reader/Writer 支持 0.0~1.4 多个版本的 JSON 映射文件，并为每个版本实现专门的解析/写入函数以兼容旧格式。
- 借助 `ValueConverter` 实现 `MeshMappingFieldType` / `MeshMappingCalcPoint` 与字符串的互转，还能输出需要映射到的结果类型列表。

### 15. MeshFieldMappingInfo.h
网格场映射信息：

- 存储场映射的相关元信息
- 描述场数据如何映射到网格上
- 记录 `FieldId`、掩码、单位、值类型、计算点以及 `NormalizeFlag`，是 `MeshFieldMappingReader` 返回的核心结构。
- `ComponentMasks` / `SourceUnits` 让同一场的不同分量可绑定到不同的源属性或单位。

### 16. MeshGroupProperty.h / MeshGroupProperty.cpp
网格组属性：

- 定义网格组的概念及其属性
- 网格组通常用于将相似的网格元素归类
- 以 32 位 bitmask 记录实体所属的多个分组，并维护 `GroupInfo`（名字 + 元素数）数组，便于统计。
- 提供 `SetGroups` / `SetGroup` / `GetGroups` 等接口以及 `NewInstance` 工厂，经常用于为块或材料定义分类属性。

### 17. MeshImportExport.h
网格导入导出接口：

- 定义网格数据导入导出的通用接口
- 为不同格式的网格文件提供统一的访问入口
- 列举 MDB 格式每个主版本的变更点，`MESH_DATA_HEADER` / `MESH_DATA_VERSION_*` 常量由 Reader/Writer 校验。
- `MeshDataImportExportOptions`、`MeshDataBufferType`、`MeshDataJsonElement` 及其 Convertor 统一了 JSON/buffer 字段名，减少魔法字符串。

### 18. MeshInfo.h / MeshInfo.cpp
网格信息：

- 存储网格的元数据信息
- 包括网格统计信息、版本信息等
- 构造函数遍历 `MeshData`，统计各阶/各维度元素数量并计算 `boundsMin` / `boundsMax`，用于调试或 UI 显示。
- 细粒度字段（如 `nbTriangles2nd`、`nbHexahedrons2nd` 等）可评估数据规模与类型分布。

### 19. MeshNVecProperty.h / MeshNVecProperty.cpp
网格法向量属性：

- 处理网格法向量相关的属性数据
- 法向量在渲染和计算中都很重要
- 支持 ScalarSet/Vector/Tensor 属性类型，`Init()` 可按模板创建合适的标量组件，并自动为向量属性追加模长。
- 维护时间戳、时间步描述、显示单位以及依赖属性列表，可表达动画/模态分析等序列数据。
- `UpdateComponents` / `EnabledComponents` / `NormalizeVector` 等方法帮助 UI 决定可视化分量并做归一化。

### 20. MeshNodeVecProperty.h / MeshNodeVecProperty.cpp
网格节点向量属性：

- 处理网格节点上的向量属性数据
- 如位移、速度等向量场数据
- 针对“每个单元的每个节点”存储场景，内部以 `MeshNodeVecPropertyData` 维护多对 index->value 映射，支持按节点局部编号访问。
- 具备 `Init` / `ReserveValueMap` / `SubIndices` / `NbSubIndices` 等接口，还能在检测到所有节点值一致时降级为纯 nodal/elemental scalar 属性，减少存储。

### 21. MeshNodeVecPropertyData.h
网格节点向量属性数据：

- 定义节点向量属性的数据结构
- `NodeVecValueMap` 模板继承 `std::vector<std::pair<int,T>>`，提供 `Find` / `Bind` / `Key` / `Value` 等便利函数，适合存 4~8 个节点的局部值。
- 同时维护 float/double 两份容器，可依据 `MeshPropertyElemType` 切换，`SetRawValue` / `RawValue` 让二进制 IO 更直接。

### 22. MeshPropertyBase.h / MeshPropertyBase.cpp
网格属性基础类：

- 所有网格属性类的基类
- 定义网格属性的通用接口和基本功能
- 统一管理属性的 `Id` / `Name` / `Description` / `RefFilePath`、映射类型以及加载状态，`generateId()` 确保跨文件唯一。
- 提供 `SetLoaded` / `CanBeLoaded` / `Unload` 钩子，Reader 可以在延迟加载或释放缓存时复用这些接口。

### 23. MeshPropertyData.h
网格属性数据：

- 定义网格属性的数据结构
- 定义 `MeshPropertyElemType`、`MeshPropertyEntityInfo` 等共享概念，并封装 NaN 哨兵值，保证不同属性在缺失数据时行为一致。
- `setType()` 会同步元素大小，方便序列化时快速计算每个值占用的字节数。

### 24. MeshResultType.h
网格结果类型：

- 定义CAE分析结果的类型枚举
- 用于区分不同类型的结果数据（如应力、应变等）
- 保存分析结果的 `TypeId`、描述、输出格式以及附加 `Options`，常与 MeshFieldMapping 文件一起描述外部求解器输出。

### 25. MeshScalarProperty.h / MeshScalarProperty.cpp
网格标量属性：

- 处理网格上的标量属性数据
- 如温度、压力等标量场数据
- 基于 `MeshScalarPropertyData` 实现 `HasValue` / `Value` / `SetValue` 等方法，可按需初始化为 float/double，并支持 `AppendValue` 用于 streaming IO。
- 提供 `NewInstance` 工厂以配合 `MeshNVecProperty` 生成组件，`Unload()` 会释放内存并更新加载状态。

### 26. MeshScalarPropertyBase.h / MeshScalarPropertyBase.cpp
网格标量属性基础类：

- 标量属性类的基类
- 提供标量属性的通用功能
- 除纯虚读写接口外，还维护属性值范围 `Bnd_Range`、显示单位以及是否启用的标记，`SetExternalValue` 可在不加载数据的前提下扩展范围。
- `BaseType` 区分 Scalar / NodeVec / Group，方便遍历属性时进行类型分发。

### 27. MeshScalarPropertyData.h
网格标量属性数据：

- 定义标量属性的数据结构
- 维护 double/float 两份 `NCollection_Vector`，`init()` 会用 NaN 填充值，`HasValue` / `RawValue` / `SetRawValue` 直接作用于底层数组。
- 支持 `AppendValue` 在构建 MeshData 时逐条追加，`RawValue` 返回底层指针，适合与 MeshBinIO 直接写入流。

### 28. MeshTools.h / MeshTools.cpp
网格工具类：

- 提供各种网格处理的工具函数
- 包括网格操作、查询、验证等功能
- `FillPropertiesRange` / `restorePropertiesRange`、`MergeMeshData`、`SortMeshData`、`ComputeVectorTrsfRatio` 等封装了常见的跨模型操作。
- `LoadMeshProperty` / `DumpMeshProperty` / `ConvertMeshProperty` / `UpdateMeshPropertyUnits` 覆盖导入导出和单位处理流程。
- 内部类 `PointElementChecker`、`TrianglesIterator` 等进一步支撑拾取、面展开和插值计算。

### 29. MeshToolsSortingImpl.cpp
网格工具排序实现：

- 实现网格数据的排序算法
- 可能用于优化网格数据的存储或访问性能
- 具体实现 `SortMeshData`，采用基于 Hilbert 曲线的空间填充排序，同时同步重排节点、单元和属性，提升缓存友好性。
- 负责拆分批量任务并维护映射表，确保排序后仍能通过索引定位原始实体。

### 30. MdbJsonExample.json
MDB JSON示例文件：

- 提供MDB（Mesh Database）格式JSON表示的示例
- 用于演示网格数据的JSON格式结构
- 示例覆盖 asset、blocks、compression、properties、buffers 等全部关键信息，可作为调试或对接外部工具的对照模板。

## 模块特点总结

1. **完整的网格数据处理体系**：从基础数据结构到高级处理功能都有涵盖
2. **支持多种数据格式**：包括二进制格式以及可能的其他格式
3. **压缩功能**：集成Draco压缩算法，提高存储和传输效率
4. **属性管理系统**：完善的网格属性（标量、向量等）管理机制
5. **场数据映射**：支持仿真结果数据与网格几何的映射关系
6. **工具集**：丰富的网格处理工具函数
7. **扩展性设计**：通过适配器模式支持不同的网格数据源

这个模块是CAE应用的核心组件，为有限元分析结果的处理、存储和可视化提供了完整的技术支撑。
