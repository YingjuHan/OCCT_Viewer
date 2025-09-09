## 项目总体结构

该项目是一个基于OpenCASCADE技术的可视化框架，主要用于处理和展示CAE（计算机辅助工程）网格数据和几何形状。项目分为多个模块，每个模块负责特定的功能领域。

## 模块详细分析

### FileExchange 模块

#### FileExchange.cpp
实现文件交换功能，可能用于不同格式之间的文件转换和数据传输。

### LodCae 模块

#### Simplifier.cpp / Simplifier.h
实现网格简化算法，用于减少网格的复杂度，提高渲染性能。包含网格简化的核心逻辑和算法实现。

### RWBase 模块（基础读写工具）

#### ConditionPromise.h
实现基于条件变量的Promise模式，用于异步编程中的条件等待机制。

#### EmulatedFileSystem.cpp / EmulatedFileSystem.h
实现虚拟文件系统，可能用于在内存中模拟文件操作或处理特定格式的数据。

#### FileSystemMap.cpp / FileSystemMap.h
实现文件系统映射功能，用于将文件路径映射到实际的数据源。

#### IPromise.h
定义Promise接口，为异步操作提供统一的接口规范。

#### JsonHelper.h
提供JSON数据处理的辅助函数和工具，用于序列化和反序列化JSON数据。

#### TaskManager.cpp / TaskManager.h
实现任务管理器，负责线程池管理和异步任务调度。

#### ValueConverter.h
提供值转换功能，用于在不同数据类型之间进行转换。

### RWCae 模块（CAE网格读写）

#### MdbJsonExample.json
JSON格式的网格数据示例文件，展示数据结构格式。

#### MeshAdaptor.cpp / MeshAdaptor.h
实现网格适配器，用于适配不同格式的网格数据到统一接口。

#### MeshAdaptorsIterator.h
定义网格适配器迭代器，用于遍历网格适配器集合。

#### MeshBinIOBase.h
定义网格二进制IO的基础类，为二进制读写操作提供基础接口。

#### MeshBinReader.cpp / MeshBinReader.h
实现网格二进制数据读取器，用于从二进制文件中读取网格数据。

#### MeshBinWriter.cpp / MeshBinWriter.h
实现网格二进制数据写入器，用于将网格数据写入二进制文件。

#### MeshCompressionHelper.h
定义网格压缩辅助类，提供网格数据压缩功能的接口。

#### MeshData.cpp / MeshData.h
实现网格数据核心类，包含网格的主要数据结构和操作方法。

#### MeshDataBlock.cpp / MeshDataBlock.h
实现网格数据块，用于组织和管理网格数据的分块存储。

#### MeshDataRange.h
定义网格数据范围，用于指定网格数据的范围和边界。

#### MeshDataVector.h
定义网格数据向量，用于存储网格相关的向量数据。

#### MeshDracoCompressionHelper.cpp / MeshDracoCompressionHelper.h
实现基于Draco算法的网格压缩辅助类，提供高效的网格数据压缩和解压缩功能。

#### MeshElement.h
定义网格元素基础类，是所有网格元素的基类。

#### MeshElements.h
定义各种网格元素类型，如节点、边、面等。

#### MeshFieldMapping.cpp / MeshFieldMapping.h
实现网格场映射功能，用于将物理场数据映射到网格上。

#### MeshFieldMappingInfo.h
定义网格场映射信息，存储场映射的相关元数据。

#### MeshGroupProperty.cpp / MeshGroupProperty.h
实现网格组属性，用于管理网格中元素的分组信息。

#### MeshImportExport.h
定义网格导入导出接口，为不同格式的导入导出提供统一接口。

#### MeshInfo.cpp / MeshInfo.h
实现网格信息类，存储网格的基本信息和元数据。

#### MeshNVecProperty.cpp / MeshNVecProperty.h
实现N向量属性，用于存储和处理N维向量数据。

#### MeshNodeVecProperty.cpp / MeshNodeVecProperty.h
实现节点向量属性，用于存储和处理节点相关的向量数据。

#### MeshNodeVecPropertyData.h
定义节点向量属性数据结构。

#### MeshPropertyBase.cpp / MeshPropertyBase.h
实现网格属性基础类，是所有网格属性类的基类。

#### MeshPropertyData.h
定义网格属性数据结构。

#### MeshResultType.h
定义网格结果类型枚举，用于标识不同类型的分析结果。

#### MeshScalarProperty.cpp / MeshScalarProperty.h
实现标量属性，用于存储和处理标量数据。

#### MeshScalarPropertyBase.cpp / MeshScalarPropertyBase.h
实现标量属性基础类，为标量属性提供基础功能。

#### MeshScalarPropertyData.h
定义标量属性数据结构。

#### MeshTools.cpp / MeshTools.h
实现网格工具类，提供各种网格处理和操作的工具函数。

#### MeshToolsSortingImpl.cpp
实现网格工具的排序功能，用于对网格元素进行排序。

### RWMed 模块（MED格式支持）

#### RWMedReader.cpp / RWMedReader.h
实现MED格式读取器，用于读取MED格式的网格和结果数据。

### RWMeshConv 模块（网格格式转换）

#### GmshLoader.cpp / GmshLoader.h
实现Gmsh格式加载器，用于读取Gmsh生成的网格文件。

#### GmshLoaderContext.cpp / GmshLoaderContext.h
实现Gmsh加载器上下文，用于管理Gmsh加载过程中的状态和配置。

#### MedConverter.cpp / MedConverter.h
实现MED格式转换器，用于在不同格式和MED格式之间进行转换。

#### MeshLoader.cpp / MeshLoader.h
实现通用网格加载器，为不同格式的网格加载提供统一接口。

### RWMeshExchanger 模块（网格交换）

#### MedFileUtils.cpp / MedFileUtils.h
实现MED文件工具函数，提供MED文件处理的辅助功能。

#### MeshExchanger.cpp / MeshExchanger.h
实现网格交换器，用于在不同网格格式之间进行数据交换。

#### MeshExchangerUtils.cpp / MeshExchangerUtils.h
实现网格交换工具函数，提供网格交换过程中的辅助功能。

#### Smds2MeshDataConvert.cpp / Smds2MeshDataConvert.h
实现SMDS到MeshData的转换功能，用于将SMDS格式数据转换为内部网格数据格式。

### RWShape 模块（形状读写）

#### GltfParser.cpp / GltfParser.h
实现GLTF格式解析器，用于解析GLTF格式的几何数据。

#### ShapeData.cpp / ShapeData.h
实现形状数据类，用于存储和处理几何形状数据。

#### ShapeDataExplorer.h
定义形状数据浏览器，用于浏览和查询形状数据。

### RWShapeConv 模块（形状转换）

#### ShapeLoader.cpp / ShapeLoader.h
实现形状加载器，用于加载不同格式的几何形状数据。

### RWVtk 模块（VTK格式支持）

#### VtkBaseReader.cpp / VtkBaseReader.h
实现VTK基础读取器，为VTK格式读取提供基础功能。

#### VtkBaseXMLReader.cpp / VtkBaseXMLReader.h
实现VTK XML格式基础读取器，用于读取XML格式的VTK文件。

#### VtkDataArray.cpp / VtkDataArray.h
实现VTK数据数组，用于存储VTK格式的数据数组。

#### VtkMultiBlockDataReader.cpp / VtkMultiBlockDataReader.h
实现VTK多块数据读取器，用于读取包含多个数据块的VTK文件。

#### VtkPolyDataReader.cpp / VtkPolyDataReader.h
实现VTK多边形数据读取器，用于读取多边形网格数据。

#### VtkSeriesReader.cpp / VtkSeriesReader.h
实现VTK序列读取器，用于读取时间序列的VTK数据。

#### VtkUnstructuredDataReader.cpp / VtkUnstructuredDataReader.h
实现VTK非结构化数据读取器，用于读取非结构化网格数据。

#### VtkUnstructuredGridReader.cpp / VtkUnstructuredGridReader.h
实现VTK非结构化网格读取器，专门用于读取非结构化网格。

### SimMesh 模块（简化网格）

#### SimMesh.cpp / SimMesh.h
实现简化网格核心类，提供网格简化的主要功能。

#### SimMesh_Attributes.cpp / SimMesh_Attributes.h
实现简化网格属性，用于管理简化网格的属性信息。

#### SimMesh_Color.h
定义简化网格颜色相关数据结构。

#### SimMesh_Edge.h
定义简化网格边数据结构。

#### SimMesh_IdList.h
定义简化网格ID列表，用于存储网格元素的ID集合。

#### SimMesh_Model.cpp / SimMesh_Model.h
实现简化网格模型，提供网格模型的管理功能。

#### SimMesh_Persistence.cpp / SimMesh_Persistence.h
实现简化网格持久化功能，用于网格数据的保存和加载。

#### SimMesh_Quadric.h
定义简化网格二次曲面相关数据结构。

#### SimMesh_Simplifier.cpp / SimMesh_Simplifier.h
实现简化网格简化器，提供网格简化算法的具体实现。

#### SimMesh_TargetType.h
定义简化网格目标类型枚举。

#### SimMesh_Triangle.h
定义简化网格三角形数据结构。

#### SimMesh_Types.h
定义简化网格相关类型别名和枚举。

#### SimMesh_Vertex.h
定义简化网格顶点数据结构。

#### SimMesh_VertexData.h
定义简化网格顶点数据结构。

#### SimMesh_VertexDataList.cpp / SimMesh_VertexDataList.h
实现简化网格顶点数据列表，用于管理顶点数据集合。

#### SimMesh_VertexPair.h
定义简化网格顶点对数据结构。

#### SimMesh_VertexPairContraction.h
定义简化网格顶点对收缩操作。

#### SimMesh_VertexPairHeap.h
定义简化网格顶点对堆数据结构，用于优化算法中的优先队列。

#### SimMesh_VertexPairList.h
定义简化网格顶点对列表数据结构。

### ViewBase 模块（基础视图）

#### BaseDefaultDrawer.cpp / BaseDefaultDrawer.h
实现基础默认绘制器，提供基本的绘制功能。

#### BaseSelection.cpp / BaseSelection.h
实现基础选择功能，处理用户选择操作。

#### BaseViewer.cpp / BaseViewer.h
实现基础查看器，提供基本的视图显示功能。

#### IInteractiveObject.cpp / IInteractiveObject.h
实现交互对象接口，定义交互对象的标准接口。

#### InteractiveClipping.cpp / InteractiveClipping.h
实现交互式裁剪功能，允许用户交互式地裁剪模型。

#### InteractiveClippingOwner.cpp / InteractiveClippingOwner.h
实现交互式裁剪所有者，管理裁剪操作的状态。

#### InteractiveOwner.cpp / InteractiveOwner.h
实现交互所有者基类，为交互对象提供基础管理功能。

#### InteractiveScaleRuler.cpp / InteractiveScaleRuler.h
实现交互式比例尺，用于显示模型的比例信息。

#### InteractiveScaleRulerOwner.cpp / InteractiveScaleRulerOwner.h
实现交互式比例尺所有者，管理比例尺的状态。

#### InteractiveTextLabel.cpp / InteractiveTextLabel.h
实现交互式文本标签，用于在视图中显示文本信息。

#### InteractiveTextLabelOwner.cpp / InteractiveTextLabelOwner.h
实现交互式文本标签所有者，管理文本标签的状态。

#### InteractiveViewCube.cpp / InteractiveViewCube.h
实现交互式视图立方体，提供视图方向的可视化控制。

#### InteractiveViewCubeOwner.cpp / InteractiveViewCubeOwner.h
实现交互式视图立方体所有者，管理视图立方体的状态。

#### OffscreenViewer.cpp / OffscreenViewer.h
实现离屏查看器，用于在不显示窗口的情况下进行渲染。

#### PrsTools.h
定义表示工具，提供可视化表示相关的工具函数。

#### ScalablePixMap.cpp / ScalablePixMap.h
实现可缩放像素图，用于处理可缩放的图像数据。

#### ScalableTexture2D.cpp / ScalableTexture2D.h
实现可缩放2D纹理，用于处理可缩放的纹理数据。

#### StatsActivator.cpp / StatsActivator.h
实现统计激活器，用于激活和管理性能统计功能。

#### UnitManager.cpp / UnitManager.h
实现单位管理器，用于管理模型中的单位系统。

### ViewCae 模块（CAE视图）

#### CaeMeshAnimation.cpp / CaeMeshAnimation.h
实现CAE网格动画，用于显示网格结果的时间序列动画。

#### CaeMeshArrowsTools.cpp / CaeMeshArrowsTools.h
实现CAE网格箭头工具，用于显示矢量场结果。

#### CaeMeshStreamlinesBuilder.cpp / CaeMeshStreamlinesBuilder.h
实现CAE网格流线构建器，用于构建和显示流线结果。

#### CaeMeshTools.cpp / CaeMeshTools.h
实现CAE网格工具，提供CAE网格处理的工具函数。

#### ColorScaleTools.cpp / ColorScaleTools.h
实现颜色标尺工具，用于管理和显示颜色标尺。

#### InteractiveCaeMesh.cpp / InteractiveCaeMesh.h
实现交互式CAE网格，提供CAE网格的交互功能。

#### InteractiveCaeMeshDrawer.cpp / InteractiveCaeMeshDrawer.h
实现交互式CAE网格绘制器，负责CAE网格的绘制操作。

#### InteractiveCaeMeshOwner.cpp / InteractiveCaeMeshOwner.h
实现交互式CAE网格所有者，管理CAE网格对象的状态。

#### InteractiveCaeMeshPrsMode.h
定义交互式CAE网格表示模式枚举。

#### InteractiveCaeMeshSelMode.h
定义交互式CAE网格选择模式枚举。

#### InteractiveColorScale.cpp / InteractiveColorScale.h
实现交互式颜色标尺，提供颜色标尺的交互功能。

#### InteractiveColorScaleOwner.cpp / InteractiveColorScaleOwner.h
实现交互式颜色标尺所有者，管理颜色标尺的状态。

#### InteractiveSphericalAxes.cpp / InteractiveSphericalAxes.h
实现交互式球面坐标轴，用于显示球面坐标系统。

#### MeshAdaptorSensitive.cpp / MeshAdaptorSensitive.h
实现网格适配器敏感体，用于处理网格对象的选择检测。

#### MeshAnnotations.cpp / MeshAnnotations.h
实现网格注释功能，用于在网格上添加注释信息。

#### MeshColorMapper.cpp / MeshColorMapper.h
实现网格颜色映射器，用于将数据值映射到颜色。

#### MeshColorScaleMapper.cpp / MeshColorScaleMapper.h
实现网格颜色标尺映射器，管理颜色标尺的映射关系。

#### MeshPart.cpp / MeshPart.h
实现网格部分，用于管理网格的分部分显示。

#### MeshPresentation.cpp / MeshPresentation.h
实现网格表示，提供网格数据的可视化表示。

#### MeshPresentationBase.cpp / MeshPresentationBase.h
实现网格表示基础类，为网格表示提供基础功能。

#### MeshPresentationScalar.cpp / MeshPresentationScalar.h
实现标量网格表示，用于显示标量场数据。

#### MeshPresentationStreamlines.cpp / MeshPresentationStreamlines.h
实现流线网格表示，用于显示流线结果。

#### MeshPresentationTools.cpp / MeshPresentationTools.h
实现网格表示工具，提供网格表示相关的工具函数。

#### MeshPresentationVector.cpp / MeshPresentationVector.h
实现矢量网格表示，用于显示矢量场数据。

#### MeshPresentations.cpp / MeshPresentations.h
实现网格表示集合，管理多个网格表示对象。

#### MeshPropertyData.cpp / MeshPropertyData.h
实现网格属性数据，用于存储网格属性相关数据。

#### MeshSensPart.cpp / MeshSensPart.h
实现网格敏感部分，用于处理网格部分的选择检测。

#### MeshSubEntity.cpp / MeshSubEntity.h
实现网格子实体，用于管理网格中的子实体对象。

### ViewCaeGeom 模块（CAE几何视图）

#### MeshBvhData.cpp / MeshBvhData.h
实现网格BVH数据，用于加速网格的空间查询。

#### MeshGeometry.cpp / MeshGeometry.h
实现网格几何，提供网格几何数据的管理和操作。

#### MeshGeometryBlock.cpp / MeshGeometryBlock.h
实现网格几何块，用于组织网格几何数据的分块存储。

#### MeshGeometryClip.cpp / MeshGeometryClip.h
实现网格几何裁剪，提供网格几何的裁剪功能。

#### MeshGeometryDeformed.cpp / MeshGeometryDeformed.h
实现变形网格几何，用于显示变形后的网格几何。

#### MeshGeometryGen.cpp / MeshGeometryGen.h
实现网格几何生成器，用于生成网格几何数据。

#### MeshGeometryIso.cpp / MeshGeometryIso.h
实现等值面网格几何，用于生成和显示等值面。

#### MeshGeometrySlice.cpp / MeshGeometrySlice.h
实现切片网格几何，用于生成和显示网格切片。

#### MeshGeometryTools.cpp / MeshGeometryTools.h
实现网格几何工具，提供网格几何处理的工具函数。

#### MeshHBvhBoxSet.h
定义网格层次BVH包围盒集合数据结构。

### ViewDim 模块（尺寸标注视图）

#### InteractiveAngleDimension.cpp / InteractiveAngleDimension.h
实现交互式角度尺寸，用于测量和显示角度尺寸。

#### InteractiveDimensionOwner.cpp / InteractiveDimensionOwner.h
实现交互式尺寸所有者，管理尺寸对象的状态。

#### InteractiveLengthDimension.cpp / InteractiveLengthDimension.h
实现交互式长度尺寸，用于测量和显示长度尺寸。

#### InteractiveRadiusDimension.cpp / InteractiveRadiusDimension.h
实现交互式半径尺寸，用于测量和显示半径尺寸。

### ViewShape 模块（形状视图）

#### InteractiveShape.cpp / InteractiveShape.h
实现交互式形状，提供几何形状的交互功能。

#### InteractiveShapeDrawer.cpp / InteractiveShapeDrawer.h
实现交互式形状绘制器，负责几何形状的绘制操作。

#### InteractiveShapeOwner.cpp / InteractiveShapeOwner.h
实现交互式形状所有者，管理形状对象的状态。

#### InteractiveShapePrsMode.h
定义交互式形状表示模式枚举。

#### InteractiveShapeSelMode.h
定义交互式形状选择模式枚举。

#### SensitiveCompound.cpp / SensitiveCompound.h
实现敏感复合体，用于处理复合对象的选择检测。

#### SensitivePolyEdge.cpp / SensitivePolyEdge.h
实现敏感多边形边，用于处理多边形边的选择检测。

#### SensitivePolyEdgeOnTri.cpp / SensitivePolyEdgeOnTri.h
实现三角面上的敏感多边形边，用于处理三角面上边的选择检测。

#### SensitivePolyFace.cpp / SensitivePolyFace.h
实现敏感多边形面，用于处理多边形面的选择检测。

#### SensitiveShape.h
定义敏感形状基类，为敏感形状提供基础功能。

#### SensitiveVertex.cpp / SensitiveVertex.h
实现敏感顶点，用于处理顶点的选择检测。

#### ShapeSelElemType.h
定义形状选择元素类型枚举。

#### ShapeSelElems.h
定义形状选择元素集合。

### Viewer 模块（主查看器）

#### Utils/ (21个文件)
包含各种实用工具函数和类。

#### IManager.cpp / IManager.h
实现管理器接口，定义管理器的标准接口。

#### InteractiveAspectsManager.cpp / InteractiveAspectsManager.h
实现交互方面管理器，管理交互对象的视觉属性。

#### InteractiveAxesManager.cpp / InteractiveAxesManager.h
实现交互坐标轴管理器，管理坐标轴的显示和交互。

#### InteractiveCaeMeshManager.cpp / InteractiveCaeMeshManager.h
实现交互CAE网格管理器，管理CAE网格对象。

#### InteractiveClippingManager.cpp / InteractiveClippingManager.h
实现交互裁剪管理器，管理裁剪操作。

#### InteractiveColorScaleManager.cpp / InteractiveColorScaleManager.h
实现交互颜色标尺管理器，管理颜色标尺对象。

#### InteractiveContext.cpp / InteractiveContext.h
实现交互上下文，管理当前的交互状态和选择。

#### InteractiveDimensionManager.cpp / InteractiveDimensionManager.h
实现交互尺寸管理器，管理尺寸标注对象。

#### InteractiveScaleRulerManager.cpp / InteractiveScaleRulerManager.h
实现交互比例尺管理器，管理比例尺对象。

#### InteractiveShapeManager.cpp / InteractiveShapeManager.h
实现交互形状管理器，管理几何形状对象。

#### InteractiveTextLabelManager.cpp / InteractiveTextLabelManager.h
实现交互文本标签管理器，管理文本标签对象。

#### InteractiveViewCubeManager.cpp / InteractiveViewCubeManager.h
实现交互视图立方体管理器，管理视图立方体对象。

#### RenderingParams.cpp / RenderingParams.h
实现渲染参数，管理渲染相关的配置参数。

#### SelectionManager.cpp / SelectionManager.h
实现选择管理器，管理选择操作和选择集。

#### UnitManager.cpp / UnitManager.h
实现单位管理器，管理模型中的单位系统。

#### ViewManager.cpp / ViewManager.h
实现视图管理器，管理视图窗口和视图操作。

#### WasmModulePrinter.cpp / WasmModulePrinter.h
实现WASM模块打印机，用于在WebAssembly环境中输出信息。

#### WasmViewer.cpp / WasmViewer.h
实现WASM查看器，为WebAssembly环境提供查看器功能。

#### main.cpp
主程序入口点，初始化整个应用程序。

## 总结

该项目是一个功能完整的CAE网格和几何形状可视化框架，具有以下特点：

1. **模块化设计**：项目按照功能划分为多个模块，每个模块职责明确
2. **多格式支持**：支持多种网格和几何格式（MED、VTK、Gmsh、GLTF等）
3. **可视化功能丰富**：提供多种可视化方式（标量场、矢量场、流线等）
4. **交互性强**：支持丰富的交互操作（选择、裁剪、测量等）
5. **性能优化**：包含网格简化、BVH加速等性能优化技术
6. **扩展性好**：采用接口和抽象类设计，便于扩展新的格式和功能