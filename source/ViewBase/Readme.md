## ViewBase 模块总览

`ViewBase` 提供了一组基于 Open CASCADE (OCCT) 的 Viewer 扩展，围绕 AIS 交互体系封装环境光照、离屏渲染、选择 Owner、交互式控件、可扩展纹理以及单位管理等通用能力，为上层应用快速构建一致的三维可视化体验。

## 核心 Viewer 组件

- **BaseViewer.[h/cpp]**：集中封装 Viewer 级别的环境初始化。`SetupCubemapIBL()` 会加载内置的 CubeMap 资源 (`cubemaps/CubeMapRoom.h`) 并配置背景；`SetupLightSources()` 则清空现有光源并创建定向光 + 环境光组合，默认启用阴影和片段着色。
- **BaseDefaultDrawer.[h/cpp]**：继承 `Prs3d_Drawer`，提供默认的文本高度、箭头长度与浮点精度，能根据 `UnitManager` 同步单位显示；`MergedMarker()` / `SplitMarker()` 可组合/拆分自定义标注符号，便于多层标记叠加。
- **BaseSelection.[h/cpp]**：对 `AIS_Selection` 的轻量封装，用于记录/恢复最近一次的选择策略 (`AIS_SelectionScheme`)，在不同选择模式之间切换时保持一致性。
- **BaseViewer/CMakeLists.txt**：将上述基础能力和交互控件编译为 `ViewBase` 静态/动态库，供其它模块链接。

## 交互框架与 Owner 体系

- **InteractiveOwner.[h/cpp]**：自定义 `SelectMgr_EntityOwner`，为每个交互对象分配全局唯一的字符串 ID，内建 `OwnerList`、`OwnerRange` 便于迭代，支持通过 `FindPrsOwnerById()` 在整个 AIS 上下文中定位 Owner，并实现多层子 Owner（例如控件子部件）高亮、不同 Z-Layer 的高亮展示以及拾取容差计算等。
- **IInteractiveObject.[h/cpp]**：配合 `InteractiveOwner` 使用的接口类，任何基于 AIS 的交互几何都可以继承它以获得统一的回调：查找对象 (`FindAisObjectById/Owner`)、刷新 Viewer (`InvalidateViewer`)、根据 DPI 变化重绘 (`OnPixelScaleChange`)、处理鼠标点击以及 2D Transform Persistence 偏移等。
- **Interactive*Owner.[h/cpp]**：针对特定控件（Clipping、ScaleRuler、TextLabel、ViewCube）实现的 Owner 派生类，负责细化子元素的选择优先级、命中区域及回调，配合对应的 `Interactive*` 对象处理选择反馈。

## 交互控件与可视化对象

- **InteractiveClipping.[h/cpp]**：扩展 `AIS_ClippingPlanes`，提供基于盒体的裁剪交互。支持相对/绝对坐标系切换、剪裁范围 (`Get/SetClippingExtent`)、是否翻转方向、是否启用剖切/封盖材质、剖面材质继承等；实现拖拽回调 `ProcessDragging()` 与重绘进度通知，以便在交互中实时刷新剖切。
- **InteractiveScaleRuler.[h/cpp]**：在视图上绘制刻度标尺的 2D 叠加控件，继承 `AIS_InteractiveObject + IInteractiveObject`，根据像素密度自动调整尺寸，支持 `InteractiveScaleRulerOwner` 处理点击事件（例如切换单位）。
- **InteractiveTextLabel.[h/cpp]**：多功能文本标签，支持包裹、多段连接线、可自定义的附着点样式（依赖 `BaseDefaultDrawer` 提供的标记合成功能），并通过 `ScalableTexture2D` 加载背景贴图。`calculateLabelParams()` 在渲染前计算文本与背景框尺寸，像素缩放改变时会触发刷新。
- **InteractiveViewCube.[h/cpp]**：基于 OCCT 自带的 `AIS_ViewCube` 扩展，重写 `Get2dObjectSize()` 以配合 Transform Persistence 抵消，借助 `InteractiveViewCubeOwner` 响应点击/拖拽从而改变相机视角。
- **InteractiveTextLabelOwner/InteractiveScaleRulerOwner/InteractiveViewCubeOwner/InteractiveClippingOwner**：上述控件的 Owner 实现，封装拾取节点 ID、双击/单击回调分发、Hover 高亮规则等。
- **InteractiveClippingOwner.cpp / InteractiveTextLabelOwner.cpp 等**：提供在 `InteractiveOwner` 基础上的附加逻辑，例如根据命中子区域切换不同的剪裁手柄或拖动文本锚点。
- **StatsActivator.[h/cpp]**：一个不可选取的 `AIS_InteractiveObject`，负责在 Viewer 中绘制点击区域和附加信息，配合外部逻辑切换 GPU/帧率等统计 HUD；`UpdateExtraInfo()` 会刷新描述文本。

## Viewer 与渲染管线辅助工具

- **OffscreenViewer.[h/cpp]**：封装了创建离屏 `V3d_View` 的完整流程，包含窗口/驱动创建、MSAA 配置以及默认字体分辨率设置，主要用于后台生成截图或自动化渲染。`DumpGlInfo()` 可以输出图形上下文信息以帮助诊断。
- **ScalablePixMap.[h/cpp]**：在 WebAssembly 或高 DPI 环境中按需加载高分辨率（或矢量 SVG）图像。记录原始路径、基础尺寸以及是否可缩放，`Load()` 可根据目标分辨率重新解析资源。
- **ScalableTexture2D.[h/cpp]**：包装 `ScalablePixMap` 生成 `Graphic3d_Texture2D`，支持外部设置分辨率缩放因子并据此重建纹理 ID，确保 UI 元素在不同像素密度下保持清晰。
- **PrsTools.h**：目前包含一个 `SegmentsHasher`，用于基于整数端点描述的线段去重/哈希，常用于交互式标注或轮廓显示中缓存几何。
- **UnitManager.[h/cpp]**：集中管理长度/时间/温度等多种单位。支持系统单位与显示单位切换、字符串格式化（例如把 `m**2` 变成 `m²`）、按单位推断量纲、数值换算（`Convert/ConvertAnyFromSI`）等，并实现全局单例 `Instance()`，供 `BaseDefaultDrawer`、标尺等组件同步单位显示。
- **BaseSelection/BaseDefaultDrawer/BaseViewer** 等文件中都引用了上述工具，形成一个连贯的渲染配置与 UI 控件生态。

## 其它文件

- **InteractiveClippingOwner.cpp、InteractiveScaleRulerOwner.cpp、InteractiveTextLabelOwner.cpp、InteractiveViewCubeOwner.cpp**：实现对应 Owner 的行为（事件转发、命中测试等）。
- **InteractiveScaleRulerOwner/InteractiveTextLabelOwner/InteractiveViewCubeOwner/InteractiveClippingOwner**：以类头/实现形式配套存在。
- **InteractiveClippingOwner.cpp** 还定义了 `InteractiveOwner::SelectionPart` 的具体子类，用以区分不同交互句柄。
- **StatsActivator.cpp / UnitManager.cpp / OffscreenViewer.cpp** 等实现文件位于同目录，含有与头文件匹配的逻辑。

## 使用建议

1. **初始化流程**：创建 Viewer 时先调用 `BaseViewer::SetupCubemapIBL()`、`BaseViewer::SetupLightSources()`，再配置 `BaseDefaultDrawer` 与 `UnitManager`，确保所有交互控件共享一致的光照与单位。
2. **自定义交互控件**：继承 `IInteractiveObject` 并搭配自定义 `InteractiveOwner`，即可共享查找/点击/像素缩放等通用逻辑。参考现有的 ViewCube / TextLabel 便于快速扩展。
3. **高 DPI 支持**：若控件需要贴图或 2D 样式，优先使用 `ScalablePixMap + ScalableTexture2D`，并在 `OnPixelScaleChange()` 中调整 `TransformPersistence` 偏移，保持视觉一致。
4. **离屏渲染**：需要批量截图或生成缩略图时，可通过 `OffscreenViewer` 初始化一个独立的 AIS 上下文与视图，复用同样的交互/材质设置。

ViewBase 模块通过这些通用组件，将 OCCT 的底层 Viewer 能力包装成可复用的“交互积木”，减少重复劳动，也让 UI 行为（选择、标注、裁剪、单位显示等）保持一致。结合其它模块（如 RWCae、ViewCae 等）即可快速构建功能完备的 CAE 后处理与可视化工具。
