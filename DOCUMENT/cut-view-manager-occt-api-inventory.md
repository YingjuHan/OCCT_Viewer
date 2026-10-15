# Cut View Manager：OCCT 功能、调用与替代清单

## 1. 清单边界与使用方法

本附录说明源项目 Cut View Manager 直接使用的 OCCT 职责，以及迁移到旧 VisIt 时需要逐项替换的能力。阅读前先看 [总体架构](cut-view-manager-migration-guide.md)。

**本次目标明确完全不使用 OCCT。** 本附录记录的是源项目依赖，目标必须逐项替换；文中其他方案讨论只用于解释源实现和一般替代成本，不构成本次目标保留 OCCT 的授权。具体替换表和无 OCCT 验收见 [迁移手册](cut-view-manager-visit-porting.md)。

核对的本机 OCCT 安装版本为 7.7.0，源仓库为 `D:\workspace\OpenCASCADE\OCCT_private`，核对提交 `db3716c9f`。它是定制分支；版本字符串相同不保证标准发行版有完全相同的 `AIS_ClippingPlanes`、shader、mutable buffer 或交互接口。迁移前应对实际使用的头文件、导出符号及源码差异做功能探测。

下面有两种证据：

- **手工功能清单**：按调用目的解释构造器、成员方法、静态接口、生命周期与替代成本。调用位置链接到本项目源码。
- **自动直接依赖清单**：扫描指定范围中所有直接 `.hxx` include，列出明确限定类名的静态调用和全部直接引用文件。它不推断变量接收者、虚函数、模板内部或 OCCT 内部调用，因此不能当作“所有运行时函数穷举”。

算法采用 OCCT 时，其内部还会调用其他 OCCT 模块。要获得部署库闭包，应结合构建链接、导出符号和 `ldd / readelf`；仅看 include 无法证明只需要某几个 `.so`。

## 2. 哪些工作由 OCCT 完成，哪些由本项目完成

| 工作 | 本项目职责 | OCCT 职责 |
| --- | --- | --- |
| FE 语义 | 源 ID、单元拓扑、形函数、来源权重、trace、字段恢复 | 数学容器、线性解算等辅助 |
| 原生平面 slice | 候选筛选规则、原边求交、六面体快路、拓扑复用与归并 | BVH 基础类、线程池 |
| 通用曲面切割 | 源单元 chart、边界准备、容差、预算、FE 逆映射 | BRep 建造、几何曲面、布尔和离散化 |
| GPU 平面显示 | body / cap 分离、局部原点、shader 条件、版本成对发布 | 图形对象、shader 框架、缓冲区与绘制 |
| 交互 | cut 定义更新、事务、mask、字段与 UI 语义 | AIS 手柄、变换、显示对象和拾取 |
| 物理分析 | 应力 / 通量、坐标、单位、积分域、求积和报告 | 局部几何与部分几何面积 / 体积工具 |
| 会话 / 文件 | 自研 VTK 解析、JSON schema、源身份和 bundle | XML / 文件抽象、BRep 读写、图像等辅助 |

原生平面快路没有逐单元 BRep 布尔。OCCT 自动封盖也不承担带 FE 字段与来源的 CPU 切面。迁移评估应分别计算这两条路径的依赖成本。

## 3. 基础类型、内存和容器

| API / 类型 | 使用方式和意义 | 目标替换建议 |
| --- | --- | --- |
| `Standard_Transient`、`Handle`、RTTI 宏 | 引用计数与 OCCT 类型系统；mesh、geometry、AIS 等跨层共享 | 目标改为明确的 shared / unique ownership 与自有类型；勿混合两套删除规则 |
| `Standard_EXPORT`、`Standard_Macro`、`Standard_Type` | 导出、类型注册、平台宏 | 独立核心可换自己的导出宏，但 ABI 面必须稳定 |
| `Standard_Mutex::Sentry` 等 | 队列和共享数据的作用域锁 | C++ mutex 或已有平台锁；保留锁范围 |
| `NCollection_Vector / Array1 / DataMap / Map / IndexedMap / DoubleMap / List / Sequence` | packed 元素、节点 / 元素映射、缓存、去重、顺序存储 | STL 等价容器；单独核对迭代稳定性、索引起点与哈希 |
| `NCollection_IncAllocator / StdAllocator / Shared` | 短生命期批量分配、容器分配器和共享对象 | arena 或 monotonic allocator；保留生命周期，勿在帧内无界增长 |
| `TColStd_*`、`TColgp_*` | 实数、整数和点的一 / 二维数组，以及 packed integer map | 连续数组或 bitset；多数 OCCT API 的数组下标可从 1 开始，不能照搬成 0-based |
| `TCollection_AsciiString / ExtendedString` | 字段名、路径、标注和 OCCT 接口字符串 | 边界显式编码转换；不得跨不同 libstdc++ ABI 传 string |
| `Graphic3d_Buffer` | `MeshData` 等连续数据存储和格式化访问 | typed view / vector；明确 alignment、stride、所有权 |

证据：[MeshData](../source/RWCae/MeshData.h)、[MeshGeometryTools](../source/ViewCaeGeom/MeshGeometryTools.h)、[TaskManager](../source/RWBase/TaskManager.h)。转换 container 后，必须复核旧 handle 是否延长缓存 lease，以及 map value 是否持有对源网格的引用。

## 4. 几何数学、包围盒与解算

| API / 方法族 | 实际用途 | 替换时必须保持 |
| --- | --- | --- |
| `gp_Pnt / XYZ / Vec / Dir`，坐标读取、点积、叉积、距离 / 平移 | BRep 边界、平面和轴构建、法向、位置转换 | 方向归一化、非有限检查、长度单位 |
| `gp_Pln / Ax1 / Ax2 / Ax3 / Trsf` | 截面、局部坐标、球柱轴与交互变换 | 点与法向变换区别、左右手系、非均匀缩放规则 |
| `Graphic3d_Vec2 / Vec3 / Vec4` 及 float / double 变体 | FE 坐标、平面四系数、GPU 顶点和 UV | 计算精度 / 显示精度分开、normal 与 d 同时归一化 |
| `Bnd_Box::Add / IsVoid / CornerMin / CornerMax` 等 | 源 bounds、变形范围、工具范围、局部 origin | 空盒、退化盒、float 外扩，避免漏相交 |
| `math_Matrix / math_Vector / math_Gauss`，`IsDone / Solve` | FE 逆映射及采样值到 Bernstein 系数的线性系统 | pivot / 奇异判断与残差；线性解成功不等于 chart 逆映射成功 |

证据：[MeshCellKernel.cpp](../source/ViewCaeGeom/MeshCellKernel.cpp)、[MeshVtkAnalytic.cpp](../source/ViewCaeGeom/MeshVtkAnalytic.cpp)、[MeshCutFrame.cpp](../source/ViewCaeGeom/MeshCutFrame.cpp)。本项目的 shape functions、逆映射迭代和张量处理是自研逻辑，不能用一个通用矩阵库替代全部行为。

## 5. BVH 与并行

### 5.1 BVH API

`BVH_BoxSet<T,N,Data>` 保存对象与盒；`Add / SetSize / Build / Box / Element` 等连接本项目数据。`BVH_LinearBuilder` 提供建树；`BVH_Traverse` 的 `RejectNode / Accept / Stop / SetBVHSet / Select` 由项目覆写，实现平面和标量阈值的筛选规则。

`MeshHBvhBoxSet::Update` 将子集 bounds 放到上层，再按需构建下层。`MeshBvhData` 缓存原生空间及标量结构，`MeshVtkCutSource` 保存通用单元空间结构。BVH 依赖不要求创建 AIS 或 OpenGL context。

目标替代方案：实现连续内存 AABB tree，并重写遍历接口，不保留 OCCT BVH。目标已有 `avtIntervalTree` 可先用于 domain 筛选，再增加 cell tree。构建器、并行、refit 与保守误差的责任需要单独验收。

证据：[MeshHBvhBoxSet.h](../source/ViewCaeGeom/MeshHBvhBoxSet.h)、[MeshBvhData.cpp](../source/ViewCaeGeom/MeshBvhData.cpp)、[MeshGeometrySlice.cpp](../source/ViewCaeGeom/MeshGeometrySlice.cpp)。

### 5.2 线程与计时 API

| API | 本项目用途 | 迁移条件 |
| --- | --- | --- |
| `OSD_ThreadPool::DefaultPool`、`Launcher(...,4)`、`Perform` | 候选分块、投影排序、trace 和字段填充、显示顶点准备 | worker 独立暂存，避免共享惰性写；与 MPI / OpenMP 预算协调 |
| `OSD_Parallel::NbLogicalProcessors` | 任务环境的硬件并行信息 | 不直接等同最佳 worker 数 |
| `OSD_Thread`、条件事件与 mutex | `RWBase::TaskManager` 的任务队列 / 后台基础设施 | 并非 manager 已能任意后台调用 |
| `OSD_Timer::Start / ElapsedTime` | 文件、队列等计时 | 性能测试使用 monotonic wall time，并区分 CPU / GPU / idle |

证据：[TaskManager.cpp](../source/RWBase/TaskManager.cpp)、[MeshCutTrace.cpp](../source/ViewCaeGeom/MeshCutTrace.cpp)、[MeshCutGpuPresentation.cpp](../source/ViewCae/MeshCutGpuPresentation.cpp)。可用目标线程池替换，但要保持异常收集、取消和确定性归并语义。

## 6. BRep 建造与有效性

| API / 方法 | 用途 | 无 OCCT 时的替代工作 |
| --- | --- | --- |
| `BRepBuilderAPI_MakePolygon / MakeEdge / MakeWire / MakeFace`，`Add / Close / Edge / Wire / Face / Shape` | 从单元边界或曲面建立拓扑 | 建边、环方向、带孔面、曲线参数域和公差体系 |
| `BRepBuilderAPI_Sewing`，`Add / Perform / SewedShape` | 缝合离散或解析面形成边界 | 拓扑配对、容差一致性及非流形诊断 |
| `BRepBuilderAPI_MakeSolid`、`BRep_Builder::MakeShell / MakeCompound / Add` | 构造 solid / shell / 复合输出 | 区分边界网格和闭合实体，不能仅复制三角形 |
| `BRepCheck_Analyzer::IsValid` | 构造后检查 | 自交、开边、重复边、方向与退化检查 |
| `BRepLib::OrientClosedSolid` | 闭合体朝向 | 一致外法向、正体积及连通分量 |
| `BRepBuilderAPI_Copy(...).Shape()` | CAD / 会话保留源形体 | 深复制语义与共享几何是否可变 |
| `BRep_Builder::MakeFace / UpdateFace`、`BRepBndLib::Add` | mesh 与 OCCT face / bounds 适配 | 仅需要显示时可直接输出 polydata；不能因此省略通用布尔 |

证据：[MeshVtkCutSource.cpp](../source/ViewCaeGeom/MeshVtkCutSource.cpp)、[MeshGeometryQuadric.cpp](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshCutUnion.cpp](../source/ViewCaeGeom/MeshCutUnion.cpp)、[NativeCutView.cpp](../source/Viewer/NativeCutView.cpp)。

当前通用源准备缓存单元形体和面关系；不要在每次鼠标移动时对整个模型 Sewing / MakeSolid。并行迁移前还要核对共享底层 `TShape`、triangulation 和 OCCT 算法是否发生惰性写。

## 7. 解析曲面、投影与采样

`Geom_BezierSurface / Geom_BezierCurve` 承载可转换的 FE 参数图表；构造所需的控制点、权重由 `MeshVtkAnalytic` 自己计算，并进行权重和残差检查。转换中的线性系统由 `math_Gauss` 解算。OCCT 并不识别业务 FE 单元字段。

`GeomAPI_ProjectPointOnSurf` 的投影、最近点 / 距离 / 参数读取用于面几何的一致性等处理；几何投影结果不能替代原 FE chart 的逆映射。

`BRepAdaptor_Surface::GetType` 区分圆柱等面类型；`BRepAdaptor_Curve` 和 `GCPnts_QuasiUniformDeflection` 将边按挠度离散，使用 `IsDone / NbPoints / Value` 读取样本。不能用固定步长取样后仍宣称满足给定几何误差。

证据：[MeshVtkAnalytic.cpp](../source/ViewCaeGeom/MeshVtkAnalytic.cpp)、[MeshVtkCutSource.cpp](../source/ViewCaeGeom/MeshVtkCutSource.cpp)、[MeshGeometryVtkCut.cpp](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)。无 OCCT 方案需自建曲线曲面求值 / 投影与采样；首版只迁线性平面可暂不接入这些功能，并显式限制能力。

## 8. 切割工具、布尔、拓扑历史

| API | 实际调用意图 | 工程注意点 |
| --- | --- | --- |
| `BRepPrimAPI_MakeHalfSpace` | 平面的负 / 正半空间工具 | 半空间参考点决定方向，不能只看 normal 名称 |
| `BRepPrimAPI_MakeSphere` | 球体工具及球 guide | guide 与数值切割使用相同中心半径 |
| `BRepPrimAPI_MakeCylinder` | 覆盖模型范围的柱体工具及 guide | padding、长度和侧壁识别；避免把端面当切面 |
| `BRepAlgoAPI_Common` | 负侧相交或与切割边界求交 | 对不同维度区分体、面、线结果 |
| `BRepAlgoAPI_Cut` | 正侧差集 | 方向 / 容差 / 空结果处理 |
| `BRepAlgoAPI_Section` | CAD 或低维单元的交线 / 截面 | Section 不提供 FE 插值来源 |
| `BRepAlgoAPI_Fuse` | 显示区域 OR 的几何合并 | 合并结果不自动适用于物理积分 |
| `Build / IsDone / HasErrors / Shape` | 执行与错误检查 | 不发布失败或部分构造结果 |
| `Modified(face)` 与 shape maps | 将源内部面映射到布尔后结果并过滤 | 丢掉历史将出现内部面、重复面积或错误外皮 |

通用求交通过模板封装调用具体算法，词法静态列中出现“—”不表示没有使用这个类。证据：[MeshGeometryVtkCut.cpp](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[MeshCutUnion.cpp](../source/ViewCaeGeom/MeshCutUnion.cpp)。

## 9. 几何测量、离散化和结果读取

| API / 方法 | 当前用途 | 迁移不能遗漏的事项 |
| --- | --- | --- |
| `BRepGProp::SurfaceProperties / VolumeProperties`、`GProp_GProps::Mass` | OCCT 几何面积 / 体积 | 按维度及内部面过滤；物理单位由调用方管理 |
| `BRepMesh_IncrementalMesh` | 按 deflection 和角度阈值离散布尔结果 | 绝对 / 相对误差、重复 meshing、共享 shape 的线程安全 |
| `BRep_Tool::Triangulation(face,location)` | 从 face 读取网格 | 应用 location 到节点；`TopAbs_REVERSED` 调整绕序 |
| `Poly_Triangulation` | 节点和三角形、源 mesh 基类 | OCCT 的 1-based API 与本项目 0-based mesh index 区分 |
| `BRep_Tool::Pnt` | 读取孤立顶点结果 | 点维度输出也应保存来源 |
| `TopExp_Explorer::More / Next / Current`、`TopoDS::Face / Edge / Vertex / Shell` | 遍历结果按维度输出 | orientation、location、空形体与重复拓扑 |
| `TopExp::MapShapes`、`TopTools_MapOfShape / IndexedMapOfShape` | 去重、身份与 CAD 会话 | shape identity 不等于几何坐标相等 |
| `BRepMesh_Triangulator`、`Poly_MergeNodesTool` | 读取 / 表示层的多边形三角化与几何节点辅助 | 几何焊接不能无条件用于材料界面物理来源 |

证据：[MeshGeometryVtkCut.cpp](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[VtkPolyDataReader.cpp](../source/RWVtk/VtkPolyDataReader.cpp)、[CaeMeshTools.cpp](../source/ViewCae/CaeMeshTools.cpp)。

## 10. 图形管线和 GPU 缓冲区

### 10.1 图元与外观

`Graphic3d_ArrayOfTriangles / Segments / Points` 保存位置、法向、texel 和索引；`AddVertex / AddEdge` 等构造图元。`Graphic3d_Group::AddPrimitiveArray / SetGroupPrimitivesAspect / SetMinMaxValues` 连接图元、外观和 bounds。

`Graphic3d_AspectFillArea3d / AspectLine3d / AspectMarker3d`、`Graphic3d_MaterialAspect`、`Quantity_Color / ColorRGBA` 设置颜色、透明度、线宽和材质；cap 使用双面显示。`Prs3d_Drawer` 及对应 aspect 维护 AIS 表示属性。替代为目标 mapper / actor / property 时也要保留各 portion 的有效样式及继承优先级。

### 10.2 Shader、纹理和 clipping

`Graphic3d_ShaderObject::CreateFromSource` 和变量列表指定 shader 输入；`Graphic3d_ShaderProgram::AttachShader / PushVariableVec4 / PushVariableVec3 / PushVariableInt` 更新平面和模式等。代码使用 OCCT shader 接口及其内建变换 / 光照约定，shader 字符串不能不经适配直接粘到旧 VTK。

`Graphic3d_Texture2D / TextureParams / TextureSetBits`、`Image_SupportedFormats` 支持字段色带纹理；`AIS_ColorScale::FindColor` 等生成颜色。目标若保留已有 lookup table，应保证 body / cap 使用同一范围、离散 / 连续插值和无效值策略。

`Graphic3d_ClipPlane::SetEquation / SetOn / SetCapping` 在两个上下文中出现：

1. **GPU body 快路**：clip 对象关闭、自动 capping 关闭；实际裁剪由自定义 `CutMode` shader 实现。
2. **Native 全局 CAD / 视口裁剪**：按开关设置 clip，并通过 view 关联；这是独立功能。

`Graphic3d_SequenceOfHClipPlane` 是 clip 集合，不能由类型出现就推断当前 mesh 使用 OCCT 自动封盖。

### 10.3 Mutable buffer 与 structure 生命周期

`Graphic3d_ArrayFlags_AttribsMutable / IndexesMutable`、`Graphic3d_AttribBuffer::Invalidate`、`Graphic3d_MutableIndexBuffer::Invalidate`、buffer 的 `NbElements / ChangeData` 用于更新切面。实现保留容量、更新有效数量、memcpy 顶点，只在需要时失效 index；空截面仍需通知计数变化。

`Graphic3d_Structure::NewGroup / Clear / CalculateBoundBox / SetVisible` 和 presentation 的 `Connect / Disconnect` 保持 body 驻留、按掩码隐藏。源码指出连接实例会绕过子结构部分可见性 / clip 状态，因此 body 隐藏还执行 disconnect。不能只照搬一个 `SetVisible(false)`。

源数据和切面 bounds 更新后，还要维护 structure / layer 空间结构。已有 GPU upload 统计只能证明应用提交意图；需要在对应 OCCT OpenGL 实现验证 dirty range 是否真的局部上传。

证据：[MeshCutGpuPresentation.cpp](../source/ViewCae/MeshCutGpuPresentation.cpp)、[MeshPresentation.cpp](../source/ViewCae/MeshPresentation.cpp)、[MeshColorMapper.cpp](../source/ViewCae/MeshColorMapper.cpp)。完全保留目标 renderer 的路线应替换本节整层，而不把 Graphic3d handle 泄漏到计算接口。

## 11. AIS 交互、拾取与视口

| API / 行为 | 当前职责 | 目标对应责任 |
| --- | --- | --- |
| `AIS_InteractiveObject`、`Prs3d_Presentation` | cut 显示对象，计算表示和选择 | actor / drawable 生命周期 |
| `AIS_InteractiveContext::Display / Erase / Remove / Update / Redisplay / UpdateCurrentViewer` | 显示、隐藏、移除、更新和重绘 | 区分参数更新与整模重建；避免每 move 全量 Redisplay |
| `AIS_ClippingPlanes::ProcessDragging` | 平面手柄的定位和拖动基础 | 目标 `VisitPlaneTool` 或自有 gizmo；不是 GPU cut 本体 |
| `AIS_Manipulator::Attach / SetPosition / ProcessDragging` 等 | 平移 / 旋转交互变换 | Begin / Update / End / Abort 事务 |
| `AIS_Shape` | 球柱 guide 和 CAD section | 只作为辅助显示，计算状态仍来自 cut 定义 |
| `SelectMgr_EntityOwner`、`SelectMgr_Selection` | 源 / cut owner 与 selection 集合 | 返回稳定业务身份，不只返回显示三角形编号 |
| `Select3D_SensitivePoint / Segment / Triangle` | 点、线、面的 CPU 敏感体 | 原始来源、GPU 被裁掉一侧的命中过滤 |
| `AIS_TextLabel` | probe 与物理标注 | 文字、锚点、更新和模型关闭清理 |
| `AIS_ColorScale`、`Graphic3d_TransformPers` | 色标和屏幕空间显示 | 图例随字段变化独立更新 |
| `V3d_View`、`Graphic3d_CView / Camera` | fit、投影、相机、全局 clip、view 有效性 | 使用目标已有 renderer / camera；不能移植窗口裸指针 |

证据：[InteractiveCaeCutPlane.cpp](../source/ViewCae/InteractiveCaeCutPlane.cpp)、[InteractiveCaeCutView.cpp](../source/ViewCae/InteractiveCaeCutView.cpp)、[NativeCutView.cpp](../source/Viewer/NativeCutView.cpp)、[NativeCutSession.cpp](../source/Viewer/NativeCutSession.cpp)。

body 拾取缓存与动态 cap 选择按不同 epoch 更新；GPU 不可见部分需在 CPU pick 层排除。`Close / Detach` 要解除回调、手柄、显示和模型引用，不能将删除窗口后的异步结果发回旧 context。

## 12. 文件、XML、图像、日志和 DRAW

| API | 使用范围 | 是否必须迁移 |
| --- | --- | --- |
| `OSD_FileSystem::DefaultFileSystem` 及流访问 | mesh、VTK、session、benchmark 读取 | 目标已有数据加载时可替换；保留路径与错误语义 |
| `OSD_Path::FolderAndFileFromPath` | series / multiblock 相对引用 | 可用平台路径工具；不能把相对路径按进程 cwd 错解 |
| `LDOMParser` / DOM 节点与属性 | 自研 XML VTK reader | 目标直接提供内存 mesh 时可不迁移 |
| `BRepTools::Read / Write` | BRep 源、CAD 会话 | 需要 CAD 持久化才保留 |
| `Message::SendInfo / SendWarning / SendFail`、messenger、progress | 日志与读取诊断 | 接业务日志并保留模型 / cut / generation |
| `RWGltf_GltfJsonParser::FormatParseError` | JSON 错误格式辅助 | 不表示 cut 依赖 glTF 几何处理 |
| `Image_AlienPixMap` | DRAW 图像回归 | 生产计算核心可去掉 |
| `Draw_Interpretor::Add`、`Draw::ParseReal` | 注册命令与参数解析 | 用目标命令 / service 替换 |
| `ViewerTest::GetAISContext / CurrentView / Display / CurrentEventManager`、`ViewerTest_EventManager` | DRAW 显示与真实输入路由 benchmark | 测试宿主；不要成为目标内核依赖 |
| `DBRep::Get` | 从 DRAW 命名对象取得 CAD shape | 业务模型层适配 |

证据：[RWVtk](../source/RWVtk)、[MeshLoader.cpp](../source/RWMeshConv/MeshLoader.cpp)、[CutViewCommands.cpp](../source-draw/ViewCaeTest/CutViewCommands.cpp)、[CutBenchmarkCommands.cpp](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)。

## 13. 链接、提取范围和能力探测

### 13.1 当前链接事实

[ViewCaeGeom/CMakeLists.txt](../source/ViewCaeGeom/CMakeLists.txt) 公开链接 `RWBase RWCae TKernel TKMath TKService TKPrim TKBO TKMesh`。其他 OCCT 几何模块由这些目标及具体构建配置传递；显示层还使用 `TKV3d / TKOpenGl` 等。实际安装 SDK 的 CMake targets / 库依赖才决定完整闭包。

只提取 `ViewCaeGeom` 仍有 `Graphic3d_Vec*`、`Standard_*`、container、线程和 BRep 类型依赖，不能宣称“计算层天然没有 OCCT”。若需纯核心，先建立 adapter 并逐步消除类型泄漏。

若需要二进制审计，在 CentOS 构建机执行并归档：

```bash
nm -D -C --undefined-only /path/to/libCutCore.so > cutcore-imports.txt
readelf -d /path/to/libCutCore.so > cutcore-dynamic.txt
ldd /path/to/libCutCore.so > cutcore-libraries.txt
```

符号表还需结合 static link、inline / template、virtual dispatch 和运行 trace 分析；它同样不单独证明所有执行路径。用于交接的最终清单应将“直接源依赖”“实际链接闭包”“运行中调用”分别归档。

### 13.2 源项目参考验证与目标替换验证

需要在 Linux 运行源项目作参考时，为以下 OCCT 能力编译最小 probe，而不是仅比较 `OCC_VERSION_HEX`：

- 数学 / BVH：构造 box set，build，再 plane traverse，覆盖负侧、交叉、空盒。
- 并行：Launcher 分块填充数组，校验异常和结果归并。
- 几何：两个简单体做 Common / Cut；读 triangulation、location、orientation、面积体积。
- 高阶：一个已支持 chart 的 Bezier 转换与残差。
- 源项目显示：shader 编译、mutable buffer 范围更新、空图元计数、断连 / 重连、拾取、view 关闭清理。

这些是源项目 SDK 的建议探测项，不是本次已新增或运行的测试。目标完全不使用 OCCT，应对自研 BVH、线程池、求交、测量和 FE 内核分别建立等价测试，显示 probes 则测目标 mapper 与 OpenGL context；不能把 OCCT probe 编进目标测试。

### 13.3 参考资料

公共 API 可参考 [OCCT 7.7 Graphic3d_ClipPlane 头文件手册](https://dev.opencascade.org/doc/occt-7.7.0/refman/html/_graphic3d___clip_plane_8hxx.html)。定制分支的行为以本地头文件及实现为准。

线程池背景可参考 [OCCT 7.4 release notes](https://dev.opencascade.org/sites/default/files/pdf/release_notes_7.4.0.pdf)；该资料说明线程池及 launcher 的设计背景，不证明共享业务对象可并发写。

## 14. 自动清单维护

从仓库根目录运行：

```powershell
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py --check
```

扫描范围由脚本显式限定：全部 `ViewCaeGeom / RWCae / RWVtk`，`Viewer/NativeCut*`，相关 cut / presentation / property 显示辅助，以及任务、加载和 DRAW cut 入口。`RWCae / RWVtk` 中部分 I/O 与压缩辅助不在拖动热路径，但对“完整迁移这些模块”的依赖审查仍有价值。

新增外部 wrapper、间接头文件或移出本范围的实现后应扩充扫描范围。若要求二进制级所有 OCCT 调用，另外对最终构建产物做符号审计和运行时 trace，不能用下面列表替代。

<!-- generated-occt-inventory:start -->

## 扫描范围与完整直接头文件清单

扫描 185 个项目源文件，发现 146 种直接 `.hxx` 依赖。
这是指定范围的词法依赖清单，不是完整运行时调用图。头文件可能只提供类型，
不能据此断言该算法每帧执行。OCCT 内部传递调用、虚函数调度和模板展开未展开。
核心功能与成员方法的人工核对见前文；引用位置由下面的列表覆盖。

| 功能组 | OCCT 头文件 | 明确限定类名的静态调用 | 直接引用文件 |
| --- | --- | --- | --- |
| BRep / 布尔 / 离散化 | `BRepAdaptor_Curve.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepAdaptor_Surface.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepAlgoAPI_Common.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepAlgoAPI_Cut.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepAlgoAPI_Fuse.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp) |
| BRep / 布尔 / 离散化 | `BRepAlgoAPI_Section.hxx` | — | [MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[NativeCutView](../source/Viewer/NativeCutView.cpp) |
| BRep / 布尔 / 离散化 | `BRepBndLib.hxx` | `Add` | [MeshData](../source/RWCae/MeshData.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_Copy.hxx` | — | [NativeCutSession](../source/Viewer/NativeCutSession.cpp)、[NativeCutView](../source/Viewer/NativeCutView.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakeEdge.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp)、[NativeCutAnalysis](../source/Viewer/NativeCutAnalysis.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakeFace.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakePolygon.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakeSolid.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakeVertex.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_MakeWire.hxx` | — | [MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepBuilderAPI_Sewing.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepCheck_Analyzer.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepGProp.hxx` | `SurfaceProperties`, `VolumeProperties` | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepLib.hxx` | `OrientClosedSolid` | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| BRep / 布尔 / 离散化 | `BRepMesh_IncrementalMesh.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepMesh_Triangulator.hxx` | — | [VtkPolyDataReader](../source/RWVtk/VtkPolyDataReader.cpp) |
| BRep / 布尔 / 离散化 | `BRepPrimAPI_MakeCylinder.hxx` | — | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepPrimAPI_MakeHalfSpace.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepPrimAPI_MakeSphere.hxx` | — | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BRep / 布尔 / 离散化 | `BRepTools.hxx` | `Read`, `Write` | [MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| BRep / 布尔 / 离散化 | `BRep_Builder.hxx` | `MakeFace`, `UpdateFace` | [MeshData](../source/RWCae/MeshData.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp)、[NativeCutAnalysis](../source/Viewer/NativeCutAnalysis.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| BRep / 布尔 / 离散化 | `BRep_Tool.hxx` | `Pnt`, `Triangulation` | [MeshData](../source/RWCae/MeshData.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| BVH 空间与值域加速 | `BVH_BoxSet.hxx` | — | [MeshBvhData](../source/ViewCaeGeom/MeshBvhData.h)、[MeshHBvhBoxSet](../source/ViewCaeGeom/MeshHBvhBoxSet.h)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.h) |
| BVH 空间与值域加速 | `BVH_LinearBuilder.hxx` | — | [MeshBvhData](../source/ViewCaeGeom/MeshBvhData.cpp) |
| BVH 空间与值域加速 | `BVH_Traverse.hxx` | — | [MeshGeometryClip](../source/ViewCaeGeom/MeshGeometryClip.cpp)、[MeshGeometryIso](../source/ViewCaeGeom/MeshGeometryIso.cpp)、[MeshGeometrySlice](../source/ViewCaeGeom/MeshGeometrySlice.cpp)、[MeshHBvhBoxSet](../source/ViewCaeGeom/MeshHBvhBoxSet.h) |
| DRAW 测试宿主 | `DBRep.hxx` | `Get` | [CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| DRAW 测试宿主 | `Draw.hxx` | `ParseReal` | [CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| DRAW 测试宿主 | `Draw_Interpretor.hxx` | — | [CutBenchmarkCommands](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)、[CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.h) |
| DRAW 测试宿主 | `ViewerTest.hxx` | `CurrentEventManager`, `CurrentView`, `Display`, `GetAISContext` | [CutBenchmarkCommands](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)、[CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| DRAW 测试宿主 | `ViewerTest_EventManager.hxx` | — | [CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| GPU / 图形结构 | `Graphic3d_ArrayOfPoints.hxx` | — | [CaeMeshTools](../source/ViewCae/CaeMeshTools.h)、[InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| GPU / 图形结构 | `Graphic3d_ArrayOfPrimitives.hxx` | — | [MeshPart](../source/ViewCae/MeshPart.h) |
| GPU / 图形结构 | `Graphic3d_ArrayOfSegments.hxx` | — | [CaeMeshTools](../source/ViewCae/CaeMeshTools.h)、[InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| GPU / 图形结构 | `Graphic3d_ArrayOfTriangles.hxx` | `AddVertex` | [CaeMeshTools](../source/ViewCae/CaeMeshTools.h)、[MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.h) |
| GPU / 图形结构 | `Graphic3d_AspectFillArea3d.hxx` | — | [MeshPresentation](../source/ViewCae/MeshPresentation.h) |
| GPU / 图形结构 | `Graphic3d_AspectLine3d.hxx` | — | [MeshPresentation](../source/ViewCae/MeshPresentation.h) |
| GPU / 图形结构 | `Graphic3d_AspectMarker3d.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h)、[MeshPresentation](../source/ViewCae/MeshPresentation.h) |
| GPU / 图形结构 | `Graphic3d_AttribBuffer.hxx` | — | [MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.cpp) |
| GPU / 图形结构 | `Graphic3d_Buffer.hxx` | — | [MeshData](../source/RWCae/MeshData.cpp)、[MeshData](../source/RWCae/MeshData.h) |
| GPU / 图形结构 | `Graphic3d_CView.hxx` | — | [NativeCutView](../source/Viewer/NativeCutView.cpp) |
| GPU / 图形结构 | `Graphic3d_Camera.hxx` | — | [NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| GPU / 图形结构 | `Graphic3d_ClipPlane.hxx` | — | [MeshAdaptor](../source/RWCae/MeshAdaptor.h)、[MeshBvhData](../source/ViewCaeGeom/MeshBvhData.h)、[MeshGeometryBlock](../source/ViewCaeGeom/MeshGeometryBlock.h)、[MeshGeometryClip](../source/ViewCaeGeom/MeshGeometryClip.h) |
| GPU / 图形结构 | `Graphic3d_Group.hxx` | `AddPrimitiveArray` | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp)、[MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.h)、[MeshPartGeneralCells](../source/ViewCae/MeshPartGeneralCells.cpp) |
| GPU / 图形结构 | `Graphic3d_MutableIndexBuffer.hxx` | — | [MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.cpp) |
| GPU / 图形结构 | `Graphic3d_SequenceOfHClipPlane.hxx` | — | [MeshGeometrySlice](../source/ViewCaeGeom/MeshGeometrySlice.h) |
| GPU / 图形结构 | `Graphic3d_ShaderObject.hxx` | `CreateFromSource`, `ShaderVariable` | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp)、[MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.cpp) |
| GPU / 图形结构 | `Graphic3d_ShaderProgram.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp)、[MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.h) |
| GPU / 图形结构 | `Graphic3d_Texture2D.hxx` | — | [MeshColorMapper](../source/ViewCae/MeshColorMapper.h) |
| GPU / 图形结构 | `Graphic3d_TextureParams.hxx` | — | [MeshColorMapper](../source/ViewCae/MeshColorMapper.cpp) |
| GPU / 图形结构 | `Graphic3d_TextureSetBits.hxx` | — | [MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.cpp) |
| GPU / 图形结构 | `Graphic3d_TransformPers.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp) |
| GPU / 图形结构 | `Graphic3d_Vec2.hxx` | — | [MeshNVecProperty](../source/RWCae/MeshNVecProperty.cpp) |
| GPU / 图形结构 | `Graphic3d_Vec3.hxx` | `Cross` | [MeshInfo](../source/RWCae/MeshInfo.h)、[MeshCellKernel](../source/ViewCaeGeom/MeshCellKernel.cpp) |
| XML | `LDOMParser.hxx` | — | [VtkBaseXMLReader](../source/RWVtk/VtkBaseXMLReader.h)、[VtkMultiBlockDataReader](../source/RWVtk/VtkMultiBlockDataReader.h)、[VtkReferenceStager](../source/RWVtk/VtkReferenceStager.cpp)、[VtkUnstructuredDataReader](../source/RWVtk/VtkUnstructuredDataReader.h)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| XML | `LDOM_XmlWriter.hxx` | — | [VtkReferenceStager](../source/RWVtk/VtkReferenceStager.cpp) |
| 交互与对象生命周期 | `AIS_ClippingPlanes.hxx` | `ProcessDragging` | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.h) |
| 交互与对象生命周期 | `AIS_ColorScale.hxx` | `FindColor` | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| 交互与对象生命周期 | `AIS_DragAction.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| 交互与对象生命周期 | `AIS_InteractiveContext.hxx` | — | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.cpp)、[NativeCutAnalysis](../source/Viewer/NativeCutAnalysis.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp)、[NativeCutTimeline](../source/Viewer/NativeCutTimeline.cpp)、[NativeCutView](../source/Viewer/NativeCutView.cpp)、[CutBenchmarkCommands](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)、[CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| 交互与对象生命周期 | `AIS_InteractiveObject.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| 交互与对象生命周期 | `AIS_Manipulator.hxx` | `OptionsForAttach`, `ProcessDragging` | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.h) |
| 交互与对象生命周期 | `AIS_Shape.hxx` | — | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.h) |
| 交互与对象生命周期 | `AIS_TextLabel.hxx` | — | [NativeCutView](../source/Viewer/NativeCutView.h) |
| 其他 OCCT 辅助 | `BinTools.hxx` | `Read` | [MeshLoader](../source/RWMeshConv/MeshLoader.cpp) |
| 其他 OCCT 辅助 | `OpenGl_Group.hxx` | — | [MeshPart](../source/ViewCae/MeshPart.cpp) |
| 其他 OCCT 辅助 | `OpenGl_Structure.hxx` | — | [MeshPart](../source/ViewCae/MeshPart.cpp) |
| 其他 OCCT 辅助 | `Precision.hxx` | `Confusion` | [InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.cpp) |
| 其他 OCCT 辅助 | `Units.hxx` | `ToSI` | [MeshTools](../source/RWCae/MeshTools.cpp) |
| 其他 OCCT 辅助 | `gp.hxx` | `DY`, `DZ`, `Resolution` | [MeshNVecProperty](../source/RWCae/MeshNVecProperty.cpp) |
| 几何曲线曲面与投影 | `GCPnts_QuasiUniformDeflection.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| 几何曲线曲面与投影 | `GeomAPI_ProjectPointOnCurve.hxx` | — | [MeshVtkCell](../source/ViewCaeGeom/MeshVtkCell.cpp) |
| 几何曲线曲面与投影 | `GeomAPI_ProjectPointOnSurf.hxx` | — | [MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| 几何曲线曲面与投影 | `Geom_BezierCurve.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.h)、[MeshVtkCell](../source/ViewCaeGeom/MeshVtkCell.h) |
| 几何曲线曲面与投影 | `Geom_BezierSurface.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.h)、[MeshVtkCell](../source/ViewCaeGeom/MeshVtkCell.h) |
| 几何曲线曲面与投影 | `Geom_CylindricalSurface.hxx` | — | [NativeCutView](../source/Viewer/NativeCutView.cpp) |
| 几何曲线曲面与投影 | `Geom_SphericalSurface.hxx` | — | [NativeCutView](../source/Viewer/NativeCutView.cpp) |
| 几何测量 | `GProp_GProps.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| 包围盒与值域 | `Bnd_Box.hxx` | — | [MeshData](../source/RWCae/MeshData.h)、[MeshCutViewManager](../source/ViewCaeGeom/MeshCutViewManager.h)、[MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.h) |
| 包围盒与值域 | `Bnd_Range.hxx` | — | [MeshScalarPropertyBase](../source/RWCae/MeshScalarPropertyBase.h) |
| 包围盒与值域 | `Bnd_Tools.hxx` | — | [MeshBvhData](../source/ViewCaeGeom/MeshBvhData.h) |
| 基础几何数学 | `gp_Ax1.hxx` | — | [CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| 基础几何数学 | `gp_Pln.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshVtkCell](../source/ViewCaeGeom/MeshVtkCell.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.h)、[NativeCutView](../source/Viewer/NativeCutView.cpp) |
| 基础几何数学 | `gp_Pnt.hxx` | — | [MeshPresentationVector](../source/ViewCae/MeshPresentationVector.cpp) |
| 基础几何数学 | `math_Gauss.hxx` | — | [MeshCellKernel](../source/ViewCaeGeom/MeshCellKernel.cpp)、[MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.cpp)、[MeshVtkCell](../source/ViewCaeGeom/MeshVtkCell.cpp) |
| 基础运行时 | `Standard_ArrayStreamBuffer.hxx` | — | [MeshDataBlock](../source/RWCae/MeshDataBlock.cpp)、[VtkDataArray](../source/RWVtk/VtkDataArray.cpp) |
| 基础运行时 | `Standard_Atomic.hxx` | — | [MeshData](../source/RWCae/MeshData.cpp)、[MeshPropertyBase](../source/RWCae/MeshPropertyBase.cpp)、[MeshPresentationBase](../source/ViewCae/MeshPresentationBase.cpp)、[MeshPropertyData](../source/ViewCae/MeshPropertyData.cpp)、[MeshGeometry](../source/ViewCaeGeom/MeshGeometry.cpp) |
| 基础运行时 | `Standard_Condition.hxx` | — | [TaskManager](../source/RWBase/TaskManager.h) |
| 基础运行时 | `Standard_Macro.hxx` | — | [MeshDracoCompressionHelper](../source/RWCae/MeshDracoCompressionHelper.h)、[MeshCellKernel](../source/ViewCaeGeom/MeshCellKernel.h)、[MeshCutCache](../source/ViewCaeGeom/MeshCutCache.h)、[MeshCutTraceOptions](../source/ViewCaeGeom/MeshCutTraceOptions.h)、[NativeCutDigest](../source/Viewer/NativeCutDigest.h) |
| 基础运行时 | `Standard_Mutex.hxx` | — | [TaskManager](../source/RWBase/TaskManager.h) |
| 基础运行时 | `Standard_NotImplemented.hxx` | — | [MeshGroupProperty](../source/RWCae/MeshGroupProperty.cpp) |
| 基础运行时 | `Standard_WarningsDisable.hxx` | — | [MeshBinWriter](../source/RWCae/MeshBinWriter.h)、[MeshDataBlock](../source/RWCae/MeshDataBlock.cpp)、[MeshDracoCompressionHelper](../source/RWCae/MeshDracoCompressionHelper.cpp)、[MeshDracoCompressionHelper](../source/RWCae/MeshDracoCompressionHelper.h)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.h) |
| 基础运行时 | `Standard_WarningsRestore.hxx` | — | [MeshBinWriter](../source/RWCae/MeshBinWriter.h)、[MeshDataBlock](../source/RWCae/MeshDataBlock.cpp)、[MeshDracoCompressionHelper](../source/RWCae/MeshDracoCompressionHelper.cpp)、[MeshDracoCompressionHelper](../source/RWCae/MeshDracoCompressionHelper.h)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.h) |
| 容器与内存 | `NCollection_Array1.hxx` | — | [MeshTools](../source/RWCae/MeshTools.h)、[CaeMeshTools](../source/ViewCae/CaeMeshTools.h) |
| 容器与内存 | `NCollection_Buffer.hxx` | — | [VtkDataArray](../source/RWVtk/VtkDataArray.h) |
| 容器与内存 | `NCollection_DataMap.hxx` | — | [MeshData](../source/RWCae/MeshData.h)、[MeshImportExport](../source/RWCae/MeshImportExport.h) |
| 容器与内存 | `NCollection_DoubleMap.hxx` | — | [MeshCompressionHelper](../source/RWCae/MeshCompressionHelper.h)、[MeshImportExport](../source/RWCae/MeshImportExport.h)、[MeshGeometryIso](../source/ViewCaeGeom/MeshGeometryIso.cpp) |
| 容器与内存 | `NCollection_IncAllocator.hxx` | — | [MeshData](../source/RWCae/MeshData.cpp)、[CaeMeshTools](../source/ViewCae/CaeMeshTools.h)、[MeshCutTrace](../source/ViewCaeGeom/MeshCutTrace.cpp)、[MeshGeometryClip](../source/ViewCaeGeom/MeshGeometryClip.cpp)、[MeshGeometrySlice](../source/ViewCaeGeom/MeshGeometrySlice.cpp) |
| 容器与内存 | `NCollection_IndexedMap.hxx` | — | [MeshNVecProperty](../source/RWCae/MeshNVecProperty.h) |
| 容器与内存 | `NCollection_List.hxx` | — | [MeshDataBlock](../source/RWCae/MeshDataBlock.cpp) |
| 容器与内存 | `NCollection_Map.hxx` | — | [MeshBinReader](../source/RWCae/MeshBinReader.h)、[MeshBinWriter](../source/RWCae/MeshBinWriter.h)、[MeshData](../source/RWCae/MeshData.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.h)、[CaeMeshTools](../source/ViewCae/CaeMeshTools.h)、[MeshGeometryClip](../source/ViewCaeGeom/MeshGeometryClip.cpp)、[MeshGeometryIso](../source/ViewCaeGeom/MeshGeometryIso.cpp)、[MeshGeometrySlice](../source/ViewCaeGeom/MeshGeometrySlice.cpp)、[MeshGeometryTools](../source/ViewCaeGeom/MeshGeometryTools.h) |
| 容器与内存 | `NCollection_Sequence.hxx` | — | [MeshData](../source/RWCae/MeshData.h) |
| 容器与内存 | `NCollection_Shared.hxx` | — | [MeshBinIOBase](../source/RWCae/MeshBinIOBase.h) |
| 容器与内存 | `NCollection_StdAllocator.hxx` | — | [MeshTools](../source/RWCae/MeshTools.h) |
| 容器与内存 | `NCollection_Vector.hxx` | — | [MeshDataVector](../source/RWCae/MeshDataVector.h)、[MeshElement](../source/RWCae/MeshElement.h)、[MeshElements](../source/RWCae/MeshElements.h)、[MeshGroupProperty](../source/RWCae/MeshGroupProperty.h)、[MeshNodeVecPropertyData](../source/RWCae/MeshNodeVecPropertyData.h)、[MeshNVecProperty](../source/RWCae/MeshNVecProperty.h)、[MeshScalarProperty](../source/RWCae/MeshScalarProperty.h)、[MeshScalarPropertyData](../source/RWCae/MeshScalarPropertyData.h)、[VtkDataArray](../source/RWVtk/VtkDataArray.h) |
| 容器与内存 | `TColStd_Array1OfReal.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.cpp) |
| 容器与内存 | `TColStd_Array2OfReal.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.cpp) |
| 容器与内存 | `TColStd_DataMapOfIntegerInteger.hxx` | — | [CaeMeshTools](../source/ViewCae/CaeMeshTools.h) |
| 容器与内存 | `TColStd_ListOfInteger.hxx` | — | [NativeCutSession](../source/Viewer/NativeCutSession.cpp)、[NativeCutTimeline](../source/Viewer/NativeCutTimeline.cpp) |
| 容器与内存 | `TColStd_PackedMapOfInteger.hxx` | — | [MeshData](../source/RWCae/MeshData.h)、[MeshNodeVecProperty](../source/RWCae/MeshNodeVecProperty.h) |
| 容器与内存 | `TColStd_SequenceOfInteger.hxx` | — | [VtkPolyDataReader](../source/RWVtk/VtkPolyDataReader.cpp) |
| 容器与内存 | `TColgp_Array1OfPnt.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.cpp) |
| 容器与内存 | `TColgp_Array2OfPnt.hxx` | — | [MeshVtkAnalytic](../source/ViewCaeGeom/MeshVtkAnalytic.cpp) |
| 容器与内存 | `TCollection_AsciiString.hxx` | — | [MeshCompressionHelper](../source/RWCae/MeshCompressionHelper.h)、[MeshDataBlock](../source/RWCae/MeshDataBlock.h)、[MeshDataRange](../source/RWCae/MeshDataRange.h)、[MeshImportExport](../source/RWCae/MeshImportExport.h)、[MeshPropertyBase](../source/RWCae/MeshPropertyBase.h)、[MeshTools](../source/RWCae/MeshTools.h)、[VtkDataArray](../source/RWVtk/VtkDataArray.h)、[VtkReferenceStager](../source/RWVtk/VtkReferenceStager.h) |
| 拓扑数据与遍历 | `TopExp.hxx` | `MapShapes` | [NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| 拓扑数据与遍历 | `TopExp_Explorer.hxx` | — | [MeshData](../source/RWCae/MeshData.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| 拓扑数据与遍历 | `TopTools_IndexedMapOfShape.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp) |
| 拓扑数据与遍历 | `TopTools_MapOfShape.hxx` | — | [MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| 拓扑数据与遍历 | `TopoDS.hxx` | `Edge`, `Face`, `Shell`, `Vertex` | [MeshData](../source/RWCae/MeshData.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)、[MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.cpp) |
| 拓扑数据与遍历 | `TopoDS_Compound.hxx` | — | [NativeCutAnalysis](../source/Viewer/NativeCutAnalysis.cpp) |
| 拓扑数据与遍历 | `TopoDS_Face.hxx` | — | [MeshData](../source/RWCae/MeshData.cpp)、[MeshData](../source/RWCae/MeshData.h) |
| 拓扑数据与遍历 | `TopoDS_Shape.hxx` | — | [MeshVtkCutSource](../source/ViewCaeGeom/MeshVtkCutSource.h) |
| 拾取 | `Select3D_SensitiveEntity.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| 拾取 | `Select3D_SensitivePoint.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp) |
| 拾取 | `Select3D_SensitiveSegment.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp) |
| 拾取 | `Select3D_SensitiveTriangle.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp) |
| 拾取 | `SelectMgr_EntityOwner.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.h) |
| 拾取 | `SelectMgr_Selection.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp) |
| 日志 | `Message.hxx` | `SendFail`, `SendInfo`, `SendWarning` | [TaskManager](../source/RWBase/TaskManager.h)、[MeshBinReader](../source/RWCae/MeshBinReader.cpp)、[MeshBinWriter](../source/RWCae/MeshBinWriter.cpp)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.cpp)、[MeshGroupProperty](../source/RWCae/MeshGroupProperty.cpp)、[MeshTools](../source/RWCae/MeshTools.h)、[MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[VtkBaseReader](../source/RWVtk/VtkBaseReader.cpp)、[VtkBaseXMLReader](../source/RWVtk/VtkBaseXMLReader.cpp)、[VtkMultiBlockDataReader](../source/RWVtk/VtkMultiBlockDataReader.cpp)、[VtkPolyDataReader](../source/RWVtk/VtkPolyDataReader.cpp)、[VtkUnstructuredDataReader](../source/RWVtk/VtkUnstructuredDataReader.cpp)、[VtkUnstructuredGridReader](../source/RWVtk/VtkUnstructuredGridReader.cpp)、[CaeMeshTools](../source/ViewCae/CaeMeshTools.cpp)、[MeshPresentationTools](../source/ViewCae/MeshPresentationTools.cpp)、[MeshGeometryTools](../source/ViewCaeGeom/MeshGeometryTools.cpp) |
| 日志 | `Message_Messenger.hxx` | — | [MeshBinReader](../source/RWCae/MeshBinReader.cpp)、[MeshBinWriter](../source/RWCae/MeshBinWriter.cpp)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.cpp)、[VtkBaseReader](../source/RWVtk/VtkBaseReader.cpp)、[VtkBaseXMLReader](../source/RWVtk/VtkBaseXMLReader.cpp)、[VtkMultiBlockDataReader](../source/RWVtk/VtkMultiBlockDataReader.cpp)、[VtkPolyDataReader](../source/RWVtk/VtkPolyDataReader.cpp)、[VtkUnstructuredDataReader](../source/RWVtk/VtkUnstructuredDataReader.cpp)、[VtkUnstructuredGridReader](../source/RWVtk/VtkUnstructuredGridReader.cpp)、[MeshPresentationTools](../source/ViewCae/MeshPresentationTools.cpp)、[MeshGeometryTools](../source/ViewCaeGeom/MeshGeometryTools.cpp) |
| 日志 | `Message_ProgressRange.hxx` | — | [MeshBinWriter](../source/RWCae/MeshBinWriter.h) |
| 纹理与截图 | `Image_AlienPixMap.hxx` | — | [CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| 纹理与截图 | `Image_SupportedFormats.hxx` | — | [InteractiveCaeCutView](../source/ViewCae/InteractiveCaeCutView.cpp)、[MeshColorMapper](../source/ViewCae/MeshColorMapper.cpp)、[MeshColorScaleMapper](../source/ViewCae/MeshColorScaleMapper.cpp)、[MeshPartGeneralCells](../source/ViewCae/MeshPartGeneralCells.cpp) |
| 线程 / 文件系统 | `OSD_FileSystem.hxx` | `DefaultFileSystem` | [MeshBinReader](../source/RWCae/MeshBinReader.cpp)、[MeshBinWriter](../source/RWCae/MeshBinWriter.cpp)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.cpp)、[MeshLoader](../source/RWMeshConv/MeshLoader.h)、[VtkBaseReader](../source/RWVtk/VtkBaseReader.cpp)、[VtkBaseXMLReader](../source/RWVtk/VtkBaseXMLReader.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp)、[CutBenchmarkCommands](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp) |
| 线程 / 文件系统 | `OSD_Parallel.hxx` | `NbLogicalProcessors` | [TaskManager](../source/RWBase/TaskManager.cpp) |
| 线程 / 文件系统 | `OSD_Path.hxx` | `FolderAndFileFromPath` | [VtkMultiBlockDataReader](../source/RWVtk/VtkMultiBlockDataReader.cpp)、[VtkSeriesReader](../source/RWVtk/VtkSeriesReader.cpp) |
| 线程 / 文件系统 | `OSD_Thread.hxx` | — | [TaskManager](../source/RWBase/TaskManager.h) |
| 线程 / 文件系统 | `OSD_ThreadPool.hxx` | `DefaultPool` | [TaskManager](../source/RWBase/TaskManager.cpp)、[MeshCutGpuPresentation](../source/ViewCae/MeshCutGpuPresentation.cpp)、[MeshCutTrace](../source/ViewCaeGeom/MeshCutTrace.cpp)、[MeshCutViewManager](../source/ViewCaeGeom/MeshCutViewManager.cpp)、[MeshGeometrySlice](../source/ViewCaeGeom/MeshGeometrySlice.cpp) |
| 线程 / 文件系统 | `OSD_Timer.hxx` | — | [TaskManager](../source/RWBase/TaskManager.cpp)、[MeshBinReader](../source/RWCae/MeshBinReader.cpp) |
| 网格与离散数据 | `Poly_MergeNodesTool.hxx` | — | [CaeMeshTools](../source/ViewCae/CaeMeshTools.cpp) |
| 网格与离散数据 | `Poly_Triangulation.hxx` | — | [MeshData](../source/RWCae/MeshData.h)、[MeshCutUnion](../source/ViewCaeGeom/MeshCutUnion.cpp)、[MeshGeometryQuadric](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)、[MeshGeometryVtkCut](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp) |
| 视口与视图 | `V3d_View.hxx` | — | [ColorScaleTools](../source/ViewCae/ColorScaleTools.cpp)、[InteractiveCaeCutPlane](../source/ViewCae/InteractiveCaeCutPlane.cpp)、[NativeCutSession](../source/Viewer/NativeCutSession.cpp)、[NativeCutTimeline](../source/Viewer/NativeCutTimeline.cpp)、[NativeCutView](../source/Viewer/NativeCutView.cpp)、[CutBenchmarkCommands](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)、[CutViewCommands](../source-draw/ViewCaeTest/CutViewCommands.cpp) |
| 输入辅助 | `RWGltf_GltfJsonParser.hxx` | `FormatParseError` | [MeshBinReader](../source/RWCae/MeshBinReader.cpp)、[MeshFieldMapping](../source/RWCae/MeshFieldMapping.cpp)、[VtkSeriesReader](../source/RWVtk/VtkSeriesReader.cpp) |

### 纳入扫描的源文件

<details>
<summary>展开全部证据文件</summary>

- [source/RWBase/TaskManager.cpp](../source/RWBase/TaskManager.cpp)
- [source/RWBase/TaskManager.h](../source/RWBase/TaskManager.h)
- [source/RWCae/MeshAdaptor.cpp](../source/RWCae/MeshAdaptor.cpp)
- [source/RWCae/MeshAdaptor.h](../source/RWCae/MeshAdaptor.h)
- [source/RWCae/MeshAdaptorsIterator.h](../source/RWCae/MeshAdaptorsIterator.h)
- [source/RWCae/MeshBinIOBase.h](../source/RWCae/MeshBinIOBase.h)
- [source/RWCae/MeshBinReader.cpp](../source/RWCae/MeshBinReader.cpp)
- [source/RWCae/MeshBinReader.h](../source/RWCae/MeshBinReader.h)
- [source/RWCae/MeshBinWriter.cpp](../source/RWCae/MeshBinWriter.cpp)
- [source/RWCae/MeshBinWriter.h](../source/RWCae/MeshBinWriter.h)
- [source/RWCae/MeshCellTopology.h](../source/RWCae/MeshCellTopology.h)
- [source/RWCae/MeshCompressionHelper.h](../source/RWCae/MeshCompressionHelper.h)
- [source/RWCae/MeshData.cpp](../source/RWCae/MeshData.cpp)
- [source/RWCae/MeshData.h](../source/RWCae/MeshData.h)
- [source/RWCae/MeshDataBlock.cpp](../source/RWCae/MeshDataBlock.cpp)
- [source/RWCae/MeshDataBlock.h](../source/RWCae/MeshDataBlock.h)
- [source/RWCae/MeshDataRange.h](../source/RWCae/MeshDataRange.h)
- [source/RWCae/MeshDataVector.h](../source/RWCae/MeshDataVector.h)
- [source/RWCae/MeshDracoCompressionHelper.cpp](../source/RWCae/MeshDracoCompressionHelper.cpp)
- [source/RWCae/MeshDracoCompressionHelper.h](../source/RWCae/MeshDracoCompressionHelper.h)
- [source/RWCae/MeshElement.h](../source/RWCae/MeshElement.h)
- [source/RWCae/MeshElements.h](../source/RWCae/MeshElements.h)
- [source/RWCae/MeshFieldMapping.cpp](../source/RWCae/MeshFieldMapping.cpp)
- [source/RWCae/MeshFieldMapping.h](../source/RWCae/MeshFieldMapping.h)
- [source/RWCae/MeshFieldMappingInfo.h](../source/RWCae/MeshFieldMappingInfo.h)
- [source/RWCae/MeshGroupProperty.cpp](../source/RWCae/MeshGroupProperty.cpp)
- [source/RWCae/MeshGroupProperty.h](../source/RWCae/MeshGroupProperty.h)
- [source/RWCae/MeshImportExport.h](../source/RWCae/MeshImportExport.h)
- [source/RWCae/MeshInfo.cpp](../source/RWCae/MeshInfo.cpp)
- [source/RWCae/MeshInfo.h](../source/RWCae/MeshInfo.h)
- [source/RWCae/MeshNodeVecProperty.cpp](../source/RWCae/MeshNodeVecProperty.cpp)
- [source/RWCae/MeshNodeVecProperty.h](../source/RWCae/MeshNodeVecProperty.h)
- [source/RWCae/MeshNodeVecPropertyData.h](../source/RWCae/MeshNodeVecPropertyData.h)
- [source/RWCae/MeshNVecProperty.cpp](../source/RWCae/MeshNVecProperty.cpp)
- [source/RWCae/MeshNVecProperty.h](../source/RWCae/MeshNVecProperty.h)
- [source/RWCae/MeshPropertyBase.cpp](../source/RWCae/MeshPropertyBase.cpp)
- [source/RWCae/MeshPropertyBase.h](../source/RWCae/MeshPropertyBase.h)
- [source/RWCae/MeshPropertyData.h](../source/RWCae/MeshPropertyData.h)
- [source/RWCae/MeshResultType.h](../source/RWCae/MeshResultType.h)
- [source/RWCae/MeshScalarProperty.cpp](../source/RWCae/MeshScalarProperty.cpp)
- [source/RWCae/MeshScalarProperty.h](../source/RWCae/MeshScalarProperty.h)
- [source/RWCae/MeshScalarPropertyBase.cpp](../source/RWCae/MeshScalarPropertyBase.cpp)
- [source/RWCae/MeshScalarPropertyBase.h](../source/RWCae/MeshScalarPropertyBase.h)
- [source/RWCae/MeshScalarPropertyData.h](../source/RWCae/MeshScalarPropertyData.h)
- [source/RWCae/MeshTools.cpp](../source/RWCae/MeshTools.cpp)
- [source/RWCae/MeshTools.h](../source/RWCae/MeshTools.h)
- [source/RWCae/MeshToolsSortingImpl.cpp](../source/RWCae/MeshToolsSortingImpl.cpp)
- [source/RWMeshConv/MeshLoader.cpp](../source/RWMeshConv/MeshLoader.cpp)
- [source/RWMeshConv/MeshLoader.h](../source/RWMeshConv/MeshLoader.h)
- [source/RWVtk/VtkBaseReader.cpp](../source/RWVtk/VtkBaseReader.cpp)
- [source/RWVtk/VtkBaseReader.h](../source/RWVtk/VtkBaseReader.h)
- [source/RWVtk/VtkBaseXMLReader.cpp](../source/RWVtk/VtkBaseXMLReader.cpp)
- [source/RWVtk/VtkBaseXMLReader.h](../source/RWVtk/VtkBaseXMLReader.h)
- [source/RWVtk/VtkBinaryDecoder.h](../source/RWVtk/VtkBinaryDecoder.h)
- [source/RWVtk/VtkBoundedTextBuffer.h](../source/RWVtk/VtkBoundedTextBuffer.h)
- [source/RWVtk/VtkDataArray.cpp](../source/RWVtk/VtkDataArray.cpp)
- [source/RWVtk/VtkDataArray.h](../source/RWVtk/VtkDataArray.h)
- [source/RWVtk/VtkDatasetReader.cpp](../source/RWVtk/VtkDatasetReader.cpp)
- [source/RWVtk/VtkDatasetReader.h](../source/RWVtk/VtkDatasetReader.h)
- [source/RWVtk/VtkImportReport.h](../source/RWVtk/VtkImportReport.h)
- [source/RWVtk/VtkLegacyReader.cpp](../source/RWVtk/VtkLegacyReader.cpp)
- [source/RWVtk/VtkLegacyReader.h](../source/RWVtk/VtkLegacyReader.h)
- [source/RWVtk/VtkMultiBlockDataReader.cpp](../source/RWVtk/VtkMultiBlockDataReader.cpp)
- [source/RWVtk/VtkMultiBlockDataReader.h](../source/RWVtk/VtkMultiBlockDataReader.h)
- [source/RWVtk/VtkNativeReader.cpp](../source/RWVtk/VtkNativeReader.cpp)
- [source/RWVtk/VtkNativeReader.h](../source/RWVtk/VtkNativeReader.h)
- [source/RWVtk/VtkPolyDataReader.cpp](../source/RWVtk/VtkPolyDataReader.cpp)
- [source/RWVtk/VtkPolyDataReader.h](../source/RWVtk/VtkPolyDataReader.h)
- [source/RWVtk/VtkReferenceStager.cpp](../source/RWVtk/VtkReferenceStager.cpp)
- [source/RWVtk/VtkReferenceStager.h](../source/RWVtk/VtkReferenceStager.h)
- [source/RWVtk/VtkResourceLimits.h](../source/RWVtk/VtkResourceLimits.h)
- [source/RWVtk/VtkSeriesReader.cpp](../source/RWVtk/VtkSeriesReader.cpp)
- [source/RWVtk/VtkSeriesReader.h](../source/RWVtk/VtkSeriesReader.h)
- [source/RWVtk/VtkUnstructuredDataReader.cpp](../source/RWVtk/VtkUnstructuredDataReader.cpp)
- [source/RWVtk/VtkUnstructuredDataReader.h](../source/RWVtk/VtkUnstructuredDataReader.h)
- [source/RWVtk/VtkUnstructuredGridReader.cpp](../source/RWVtk/VtkUnstructuredGridReader.cpp)
- [source/RWVtk/VtkUnstructuredGridReader.h](../source/RWVtk/VtkUnstructuredGridReader.h)
- [source/RWVtk/VtkValidationReader.cpp](../source/RWVtk/VtkValidationReader.cpp)
- [source/RWVtk/VtkValidationReader.h](../source/RWVtk/VtkValidationReader.h)
- [source/ViewCae/CaeMeshTools.cpp](../source/ViewCae/CaeMeshTools.cpp)
- [source/ViewCae/CaeMeshTools.h](../source/ViewCae/CaeMeshTools.h)
- [source/ViewCae/ColorScaleTools.cpp](../source/ViewCae/ColorScaleTools.cpp)
- [source/ViewCae/ColorScaleTools.h](../source/ViewCae/ColorScaleTools.h)
- [source/ViewCae/InteractiveCaeCutPlane.cpp](../source/ViewCae/InteractiveCaeCutPlane.cpp)
- [source/ViewCae/InteractiveCaeCutPlane.h](../source/ViewCae/InteractiveCaeCutPlane.h)
- [source/ViewCae/InteractiveCaeCutView.cpp](../source/ViewCae/InteractiveCaeCutView.cpp)
- [source/ViewCae/InteractiveCaeCutView.h](../source/ViewCae/InteractiveCaeCutView.h)
- [source/ViewCae/MeshColorMapper.cpp](../source/ViewCae/MeshColorMapper.cpp)
- [source/ViewCae/MeshColorMapper.h](../source/ViewCae/MeshColorMapper.h)
- [source/ViewCae/MeshColorScaleMapper.cpp](../source/ViewCae/MeshColorScaleMapper.cpp)
- [source/ViewCae/MeshColorScaleMapper.h](../source/ViewCae/MeshColorScaleMapper.h)
- [source/ViewCae/MeshCutGpuPresentation.cpp](../source/ViewCae/MeshCutGpuPresentation.cpp)
- [source/ViewCae/MeshCutGpuPresentation.h](../source/ViewCae/MeshCutGpuPresentation.h)
- [source/ViewCae/MeshPart.cpp](../source/ViewCae/MeshPart.cpp)
- [source/ViewCae/MeshPart.h](../source/ViewCae/MeshPart.h)
- [source/ViewCae/MeshPartGeneralCells.cpp](../source/ViewCae/MeshPartGeneralCells.cpp)
- [source/ViewCae/MeshPartGeneralCells.h](../source/ViewCae/MeshPartGeneralCells.h)
- [source/ViewCae/MeshPresentation.cpp](../source/ViewCae/MeshPresentation.cpp)
- [source/ViewCae/MeshPresentation.h](../source/ViewCae/MeshPresentation.h)
- [source/ViewCae/MeshPresentationBase.cpp](../source/ViewCae/MeshPresentationBase.cpp)
- [source/ViewCae/MeshPresentationBase.h](../source/ViewCae/MeshPresentationBase.h)
- [source/ViewCae/MeshPresentations.cpp](../source/ViewCae/MeshPresentations.cpp)
- [source/ViewCae/MeshPresentations.h](../source/ViewCae/MeshPresentations.h)
- [source/ViewCae/MeshPresentationScalar.cpp](../source/ViewCae/MeshPresentationScalar.cpp)
- [source/ViewCae/MeshPresentationScalar.h](../source/ViewCae/MeshPresentationScalar.h)
- [source/ViewCae/MeshPresentationStreamlines.cpp](../source/ViewCae/MeshPresentationStreamlines.cpp)
- [source/ViewCae/MeshPresentationStreamlines.h](../source/ViewCae/MeshPresentationStreamlines.h)
- [source/ViewCae/MeshPresentationTools.cpp](../source/ViewCae/MeshPresentationTools.cpp)
- [source/ViewCae/MeshPresentationTools.h](../source/ViewCae/MeshPresentationTools.h)
- [source/ViewCae/MeshPresentationVector.cpp](../source/ViewCae/MeshPresentationVector.cpp)
- [source/ViewCae/MeshPresentationVector.h](../source/ViewCae/MeshPresentationVector.h)
- [source/ViewCae/MeshPropertyData.cpp](../source/ViewCae/MeshPropertyData.cpp)
- [source/ViewCae/MeshPropertyData.h](../source/ViewCae/MeshPropertyData.h)
- [source/ViewCaeGeom/MeshBvhData.cpp](../source/ViewCaeGeom/MeshBvhData.cpp)
- [source/ViewCaeGeom/MeshBvhData.h](../source/ViewCaeGeom/MeshBvhData.h)
- [source/ViewCaeGeom/MeshCellKernel.cpp](../source/ViewCaeGeom/MeshCellKernel.cpp)
- [source/ViewCaeGeom/MeshCellKernel.h](../source/ViewCaeGeom/MeshCellKernel.h)
- [source/ViewCaeGeom/MeshCellTables.h](../source/ViewCaeGeom/MeshCellTables.h)
- [source/ViewCaeGeom/MeshCutAutomatic.cpp](../source/ViewCaeGeom/MeshCutAutomatic.cpp)
- [source/ViewCaeGeom/MeshCutAutomatic.h](../source/ViewCaeGeom/MeshCutAutomatic.h)
- [source/ViewCaeGeom/MeshCutCache.cpp](../source/ViewCaeGeom/MeshCutCache.cpp)
- [source/ViewCaeGeom/MeshCutCache.h](../source/ViewCaeGeom/MeshCutCache.h)
- [source/ViewCaeGeom/MeshCutFieldTools.h](../source/ViewCaeGeom/MeshCutFieldTools.h)
- [source/ViewCaeGeom/MeshCutFrame.cpp](../source/ViewCaeGeom/MeshCutFrame.cpp)
- [source/ViewCaeGeom/MeshCutFrame.h](../source/ViewCaeGeom/MeshCutFrame.h)
- [source/ViewCaeGeom/MeshCutIntegration.cpp](../source/ViewCaeGeom/MeshCutIntegration.cpp)
- [source/ViewCaeGeom/MeshCutIntegration.h](../source/ViewCaeGeom/MeshCutIntegration.h)
- [source/ViewCaeGeom/MeshCutPhysics.cpp](../source/ViewCaeGeom/MeshCutPhysics.cpp)
- [source/ViewCaeGeom/MeshCutPhysics.h](../source/ViewCaeGeom/MeshCutPhysics.h)
- [source/ViewCaeGeom/MeshCutRecovery.cpp](../source/ViewCaeGeom/MeshCutRecovery.cpp)
- [source/ViewCaeGeom/MeshCutRecovery.h](../source/ViewCaeGeom/MeshCutRecovery.h)
- [source/ViewCaeGeom/MeshCutSlicing.cpp](../source/ViewCaeGeom/MeshCutSlicing.cpp)
- [source/ViewCaeGeom/MeshCutSlicing.h](../source/ViewCaeGeom/MeshCutSlicing.h)
- [source/ViewCaeGeom/MeshCutTensor.cpp](../source/ViewCaeGeom/MeshCutTensor.cpp)
- [source/ViewCaeGeom/MeshCutTensor.h](../source/ViewCaeGeom/MeshCutTensor.h)
- [source/ViewCaeGeom/MeshCutTrace.cpp](../source/ViewCaeGeom/MeshCutTrace.cpp)
- [source/ViewCaeGeom/MeshCutTrace.h](../source/ViewCaeGeom/MeshCutTrace.h)
- [source/ViewCaeGeom/MeshCutTraceOptions.h](../source/ViewCaeGeom/MeshCutTraceOptions.h)
- [source/ViewCaeGeom/MeshCutUnion.cpp](../source/ViewCaeGeom/MeshCutUnion.cpp)
- [source/ViewCaeGeom/MeshCutUnion.h](../source/ViewCaeGeom/MeshCutUnion.h)
- [source/ViewCaeGeom/MeshCutViewManager.cpp](../source/ViewCaeGeom/MeshCutViewManager.cpp)
- [source/ViewCaeGeom/MeshCutViewManager.h](../source/ViewCaeGeom/MeshCutViewManager.h)
- [source/ViewCaeGeom/MeshGeometry.cpp](../source/ViewCaeGeom/MeshGeometry.cpp)
- [source/ViewCaeGeom/MeshGeometry.h](../source/ViewCaeGeom/MeshGeometry.h)
- [source/ViewCaeGeom/MeshGeometryBlock.cpp](../source/ViewCaeGeom/MeshGeometryBlock.cpp)
- [source/ViewCaeGeom/MeshGeometryBlock.h](../source/ViewCaeGeom/MeshGeometryBlock.h)
- [source/ViewCaeGeom/MeshGeometryClip.cpp](../source/ViewCaeGeom/MeshGeometryClip.cpp)
- [source/ViewCaeGeom/MeshGeometryClip.h](../source/ViewCaeGeom/MeshGeometryClip.h)
- [source/ViewCaeGeom/MeshGeometryDeformed.cpp](../source/ViewCaeGeom/MeshGeometryDeformed.cpp)
- [source/ViewCaeGeom/MeshGeometryDeformed.h](../source/ViewCaeGeom/MeshGeometryDeformed.h)
- [source/ViewCaeGeom/MeshGeometryGen.cpp](../source/ViewCaeGeom/MeshGeometryGen.cpp)
- [source/ViewCaeGeom/MeshGeometryGen.h](../source/ViewCaeGeom/MeshGeometryGen.h)
- [source/ViewCaeGeom/MeshGeometryIso.cpp](../source/ViewCaeGeom/MeshGeometryIso.cpp)
- [source/ViewCaeGeom/MeshGeometryIso.h](../source/ViewCaeGeom/MeshGeometryIso.h)
- [source/ViewCaeGeom/MeshGeometryQuadric.cpp](../source/ViewCaeGeom/MeshGeometryQuadric.cpp)
- [source/ViewCaeGeom/MeshGeometryQuadric.h](../source/ViewCaeGeom/MeshGeometryQuadric.h)
- [source/ViewCaeGeom/MeshGeometrySlice.cpp](../source/ViewCaeGeom/MeshGeometrySlice.cpp)
- [source/ViewCaeGeom/MeshGeometrySlice.h](../source/ViewCaeGeom/MeshGeometrySlice.h)
- [source/ViewCaeGeom/MeshGeometryTools.cpp](../source/ViewCaeGeom/MeshGeometryTools.cpp)
- [source/ViewCaeGeom/MeshGeometryTools.h](../source/ViewCaeGeom/MeshGeometryTools.h)
- [source/ViewCaeGeom/MeshGeometryVtkCut.cpp](../source/ViewCaeGeom/MeshGeometryVtkCut.cpp)
- [source/ViewCaeGeom/MeshGeometryVtkCut.h](../source/ViewCaeGeom/MeshGeometryVtkCut.h)
- [source/ViewCaeGeom/MeshGeometryVtkIso.cpp](../source/ViewCaeGeom/MeshGeometryVtkIso.cpp)
- [source/ViewCaeGeom/MeshGeometryVtkIso.h](../source/ViewCaeGeom/MeshGeometryVtkIso.h)
- [source/ViewCaeGeom/MeshHBvhBoxSet.h](../source/ViewCaeGeom/MeshHBvhBoxSet.h)
- [source/ViewCaeGeom/MeshVtkAnalytic.cpp](../source/ViewCaeGeom/MeshVtkAnalytic.cpp)
- [source/ViewCaeGeom/MeshVtkAnalytic.h](../source/ViewCaeGeom/MeshVtkAnalytic.h)
- [source/ViewCaeGeom/MeshVtkCell.cpp](../source/ViewCaeGeom/MeshVtkCell.cpp)
- [source/ViewCaeGeom/MeshVtkCell.h](../source/ViewCaeGeom/MeshVtkCell.h)
- [source/ViewCaeGeom/MeshVtkCutSource.cpp](../source/ViewCaeGeom/MeshVtkCutSource.cpp)
- [source/ViewCaeGeom/MeshVtkCutSource.h](../source/ViewCaeGeom/MeshVtkCutSource.h)
- [source/Viewer/NativeCutAnalysis.cpp](../source/Viewer/NativeCutAnalysis.cpp)
- [source/Viewer/NativeCutAnalysis.h](../source/Viewer/NativeCutAnalysis.h)
- [source/Viewer/NativeCutAnalysisJson.cpp](../source/Viewer/NativeCutAnalysisJson.cpp)
- [source/Viewer/NativeCutDigest.cpp](../source/Viewer/NativeCutDigest.cpp)
- [source/Viewer/NativeCutDigest.h](../source/Viewer/NativeCutDigest.h)
- [source/Viewer/NativeCutSession.cpp](../source/Viewer/NativeCutSession.cpp)
- [source/Viewer/NativeCutSession.h](../source/Viewer/NativeCutSession.h)
- [source/Viewer/NativeCutTimeline.cpp](../source/Viewer/NativeCutTimeline.cpp)
- [source/Viewer/NativeCutTimeline.h](../source/Viewer/NativeCutTimeline.h)
- [source/Viewer/NativeCutView.cpp](../source/Viewer/NativeCutView.cpp)
- [source/Viewer/NativeCutView.h](../source/Viewer/NativeCutView.h)
- [source-draw/ViewCaeTest/CutBenchmarkCommands.cpp](../source-draw/ViewCaeTest/CutBenchmarkCommands.cpp)
- [source-draw/ViewCaeTest/CutViewCommands.cpp](../source-draw/ViewCaeTest/CutViewCommands.cpp)
- [source-draw/ViewCaeTest/CutViewCommands.h](../source-draw/ViewCaeTest/CutViewCommands.h)

</details>

<!-- generated-occt-inventory:end -->
