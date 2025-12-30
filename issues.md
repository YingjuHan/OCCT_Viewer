# OCC_VIEWER Enhancement Issues（基于 Codex 落地）

本文件把前期分析结论与 `DOCUMENT/TODO/codex-plan.md` 的模板落到“可直接创建的 Issue 列表”。每条 Issue 都包含：背景/范围/验收/样例/失败策略/建议改动点/可复制的 Codex 提示词。

## 约定

- **优先级**
  - **P0**：对标一流 Viewer 的硬门槛（没有就无法进入主赛道）
  - **P1**：强竞争力（显著拉开与普通 Viewer 差距）
  - **P2**：产品化与生态（规模化落地/商业质量）
- **样例数据优先使用仓库内可复现资源**
  - Shape：`install/wasm32-fallback/models/brep/*`、`install/wasm32-fallback/models/gltf/*`、`install/wasm32-fallback/models/stl/*`
  - CAE：`install/wasm32-fallback/models/gmsh/*`
  - 若需 STEP/IGES/装配/PMI：先在 Issue 中声明“样例文件来源与放置路径”（建议新增到 `samples/models/`，避免依赖 `install/` 产物目录）。

---

## P0（必须补齐）

### P0-001：定义 STEP/IGES 转换产物规范（server-side first）

- **背景/用户故事**：作为 Web 用户，我需要直接打开 STEP/IGES，但浏览器/Wasm 直读可能受体积/内存/依赖限制；先通过 server-side 转换形成可用链路。
- **范围**：仅定义“输入 STEP/IGES → 输出 glTF/glb + 元数据”的文件结构、字段、坐标系/单位/材质映射、分块策略；不实现转换器。
- **验收标准**
  - 在 `DOCUMENT/TODO/` 下新增规范文档，包含：目录结构、JSON schema（或字段表）、示例文件片段、版本号策略、兼容策略。
  - 明确：装配树/属性/可见性/视图状态/PMI 的表达方式（即使 PMI 暂不实现也要预留字段）。
- **样例数据**：无；文档中引用现有 glb/brep 示例做对照即可。
- **失败策略**：不适用。
- **建议改动点**：新增文档 `DOCUMENT/TODO/format-step-iges-conversion-spec.md`。
- **Codex 提示词**
  - “在 `DOCUMENT/TODO` 增加 STEP/IGES→glb+metadata 产物规范文档，要求包含目录结构、字段表、版本号、兼容策略、分块策略，并给出最小示例 JSON。”

### P0-002：建立 SceneGraph（装配树）领域模型与序列化（C++/TS 对齐）

- **背景/用户故事**：作为一流 CAX Viewer，必须有“产品结构树/部件层级/属性”来支撑搜索、隔离、批量操作与后续 PMI/Saved Views。
- **范围**
  - C++：定义 SceneGraph 节点结构（id/name/parent/children/bbox/props/tags/visibility）。
  - TS：暴露类型与获取接口（例如 `interactiveContext.sceneGraph` 或 `getSceneGraph()`）。
  - 暂不强制与 STEP/IGES/XDE 绑定；先以现有 `InteractiveShape` 作为数据源生成树（例如按 Explode 或按 ObjectId 前缀生成）。
- **验收标准**
  - TS 侧能拿到稳定结构：节点 id 唯一；children 可遍历；节点 bbox 可用于 fit。
  - 对当前场景中的 `InteractiveShape` 至少能生成 1 层树（root -> objects）。
- **样例数据**
  - `install/wasm32-fallback/models/brep/as1-oc-214.bbrep`
  - `install/wasm32-fallback/models/gltf/as1-oc-214.glb`
- **失败策略**
  - 场景为空：返回空树而非抛异常。
  - bbox 计算失败：用 invalid bbox 表示，UI 侧禁用 fit。
- **建议改动点**
  - C++：`source/Viewer/Utils/*` + `source/Viewer/InteractiveContext.*`（或新增 `SceneGraph` 工具类）
  - TS：`source-ts/Viewer-ts/InteractiveObject.ts` 或新增 `SceneGraph.ts`
- **Codex 提示词**
  - “实现 SceneGraph API：C++ 侧生成场景树并导出到 Wasm；TS 侧补齐类型与封装；在 samples/html 加一个按钮把树打印到 console，并提供 fit-to-node 示例。”

### P0-003：Vue 示例：装配树 UI（搜索/定位/隔离/隐藏）

- **背景/用户故事**：作为评审/工程师，我需要在左侧树中搜索部件、点击定位、隔离或隐藏一组对象。
- **范围**
  - 在 `samples/vue-viewer` 落地“树 + 搜索 + 批量操作（hide/show/isolate）+ 点击定位（fit）”。
  - 使用 P0-002 的 SceneGraph API，不引入外部服务端。
- **验收标准**
  - 打开 vue 示例后可看到树；搜索能过滤；点击节点能高亮/选中并 fit。
  - isolate：只显示该节点子树对象；恢复：显示全部。
- **样例数据**：同 P0-002。
- **失败策略**：SceneGraph 不可用时给 UI 提示并隐藏树面板。
- **建议改动点**：`samples/vue-viewer/src/components/*`、`samples/vue-viewer/src/services/viewer.service.ts`（若存在）。
- **Codex 提示词**
  - “在 samples/vue-viewer 增加 SceneGraph 面板：树+搜索+hide/show/isolate+fit；通过现有 ViewerService 调用 TS API；提供最小可用交互与错误提示。”

### P0-004：批量可见性/隔离/透明操作 API（C++/TS）

- **背景/用户故事**：一流 Viewer 的基础能力：批量 hide/show/isolate/透明度控制，且必须可脚本化。
- **范围**：对 `InteractiveObject` 列表提供批量操作（按 ObjectId 列表、按 SceneGraph node id）。
- **验收标准**
  - TS：新增 `CommonTools`/`InteractiveContextExtensions` 中的批量接口（例如 `setVisibleMany(ids, bool)`）。
  - HTML sample：提供 3 个按钮验证 hide/show/isolate。
- **样例数据**：`install/wasm32-fallback/models/brep/fuse.bbrep`（可 explode 出多个对象）。
- **失败策略**：部分 id 不存在时跳过并 warning，不整体失败。
- **建议改动点**：`source/Viewer/InteractiveContext.*`、`source-ts/Viewer-ts/InteractiveObject.ts`。
- **Codex 提示词**
  - “增加批量可见性/隔离 API：C++ 侧实现并导出；TS 侧封装；samples/html 增加按钮演示，并保证不存在 id 时只 warning 不崩溃。”

### P0-005：测量 1：点/边/面捕捉与最短距离（Shape）

- **背景/用户故事**：工程师需要通过选择自动测量点-点、点-边、点-面、边-边最短距离。
- **范围**
  - 新增测量工具（不只是 Dimension），输出数值 + 参考几何（线段/垂线）+ 可选标注对象。
  - 支持从 `SelectionManager` 获取当前 pickedPoint/检测到的 subshape 信息。
- **验收标准**
  - 在 `samples/html` 中：选择两个实体后点击“Measure distance”输出数值并显示辅助线。
  - 支持单位（跟随 `UnitManager`）。
- **样例数据**：`install/wasm32-fallback/models/brep/face.brep`、`fuse.bbrep`。
- **失败策略**：选择不足/类型不支持时给 UI 提示（不抛异常）。
- **建议改动点**
  - C++：可能在 `source/ViewDim` 新增 `InteractiveDistanceMeasure`，或在 `Viewer/InteractiveDimensionManager` 扩展。
  - TS：新增 `InteractiveMeasure.ts` 或扩展 `InteractiveDimension.ts`。
- **Codex 提示词**
  - “实现最短距离测量（Shape）：选择两要素后计算距离并在视图中显示辅助线/数值；提供 TS API 与 samples/html 按钮演示，失败给友好提示。”

### P0-006：测量 2：圆/圆弧直径与半径自动识别

- **背景/用户故事**：用户选中圆边/圆面后应自动给出半径/直径，不需要手工选 3 点。
- **范围**：基于 subshape 类型（Edge/Face）识别圆/圆柱/圆锥等；优先支持圆边与圆面。
- **验收标准**
  - 选中圆边 → 一键生成半径/直径标注（`InteractiveRadiusDimension` 可复用）。
  - 结果可加入“测量结果列表”（见 P0-008）。
- **样例数据**：如现有样例不含圆边，需补充一个最小 BREP/GLB（放到 `samples/models/`）。
- **失败策略**：识别失败提示“当前选择不支持自动半径/直径”。
- **建议改动点**：`source/ViewDim/*`、`source/Viewer/SelectionManager.*`、TS wrapper。
- **Codex 提示词**
  - “基于当前选择自动识别圆要素并生成半径/直径标注；优先复用现有 RadiusDimension；在 samples/html 添加按钮演示与失败提示。”

### P0-007：测量 3：面积/体积/质心/惯量（质量属性）

- **背景/用户故事**：工程评审需要快速查看部件质量属性（面积/体积/质心/惯量），并能复制/导出。
- **范围**：对选中的 `InteractiveShape` 提供质量属性计算 API；至少支持体积/面积/质心。
- **验收标准**
  - HTML/Vue 示例：选中对象后显示属性并可复制 JSON。
  - 输出单位跟随 `UnitManager`（长度/面积/体积单位规则明确）。
- **样例数据**：`install/wasm32-fallback/models/brep/as1-oc-214.bbrep`。
- **失败策略**：非封闭实体体积不可算时给 warning，仍可输出面积/质心（若可）。
- **建议改动点**：C++ 新增 `ShapeMassProps` 工具；TS 新增类型与调用。
- **Codex 提示词**
  - “为 InteractiveShape 增加质量属性 API（面积/体积/质心），导出到 TS，并在 samples/vue-viewer 中展示与复制 JSON；非封闭体积失败要降级处理。”

### P0-008：测量结果列表（可管理/可导出/可恢复）

- **背景/用户故事**：一流 Viewer 的测量不是一次性弹窗，而是可管理的结果列表（可删/可定位/可导出/可恢复）。
- **范围**
  - TS 侧提供 `MeasurementStore`（内存版）+ 导入导出 JSON。
  - UI：Vue 侧提供结果列表；点击条目高亮与定位；删除条目删除对应交互对象。
- **验收标准**
  - 生成 3 条测量结果后可导出 JSON；刷新页面导入 JSON 可恢复显示（在相同模型/场景下）。
- **样例数据**：同 P0-005~P0-007。
- **失败策略**：导入时找不到对象 id → 标记为失效项但不阻断导入。
- **建议改动点**：`samples/vue-viewer/src` + `source-ts/Viewer-ts`（必要 API）。
- **Codex 提示词**
  - “实现测量结果列表：可导出/导入 JSON、点击定位与高亮、删除同步移除交互对象；在 Vue 示例落地 UI。”

### P0-009：Saved Views（相机+可见性+剖切）保存/恢复

- **背景/用户故事**：评审需要保存多个视角（含可见性/剖切状态），一键切换。
- **范围**
  - 相机：位置/方向/投影/FOV（复用 `CameraViewPoint` 类概念）。
  - 可见性：对象 show/hide（批量）。
  - 剖切：`InteractiveClipping` 的关键参数。
- **验收标准**
  - Vue 示例：保存 3 个视图，列表切换；切换时相机与可见性/剖切同步变化。
- **样例数据**：`install/wasm32-fallback/models/gltf/ball.glb` + 任意一个 BREP。
- **失败策略**：对象缺失时跳过并提示。
- **建议改动点**：`source/Viewer/ViewManager.*`、`source-ts/Viewer-ts/Camera.ts`、`InteractiveClipping.ts`、Vue UI。
- **Codex 提示词**
  - “实现 Saved Views：保存/恢复相机+可见性+剖切状态；导出/导入 JSON；在 vue 示例提供视图列表与切换按钮。”

### P0-010：场景工程文件（Project）保存/恢复（对象+样式+视图+测量）

- **背景/用户故事**：用户希望把当前 Viewer 状态保存成一个工程文件（可复现），方便分享与复盘。
- **范围**
  - 保存：已加载模型的来源（URL/本地文件名/缓存文件）、对象样式（颜色/材质/透明）、SceneGraph/SavedViews/Measurements。
  - 恢复：在相同资源可访问时重建场景。
- **验收标准**
  - `project.json` 可导出并在同一环境导入恢复。
  - 对“本地拖拽文件”给出限制与提示（例如仅保存为用户需重新选择文件）。
- **样例数据**：使用 install models + 一个本地拖拽文件场景。
- **失败策略**：资源不可访问时在 UI 中列出失败项与原因，支持手工重新绑定文件。
- **建议改动点**：TS 层（序列化）+ Vue UI（导入导出）。
- **Codex 提示词**
  - “实现 Viewer Project 导入导出：包含对象样式、Saved Views、Measurements；恢复时对缺失资源给可重绑定提示；在 Vue 示例增加导入/导出入口。”

### P0-011：框选/套索选择（Box/Lasso Selection）

- **背景/用户故事**：批量选择是装配/CAE 后处理的高频操作，必须有框选/套索。
- **范围**
  - 先实现 Box 选择（屏幕矩形），套索作为后续增强。
  - Shape 与 Mesh 都要支持（至少对象级选择；子元素级可分阶段）。
- **验收标准**
  - HTML 示例：按住快捷键拖拽框选，选中对象列表更新；支持添加/替换选择。
- **样例数据**：`fuse.bbrep` explode 多对象，或加载多个模型。
- **失败策略**：在不支持的浏览器/模式下禁用并提示。
- **建议改动点**：`source/Viewer/SelectionManager.*`、TS `SelectionManagerExtensions`、samples UI 事件。
- **Codex 提示词**
  - “实现 Box Selection：拖拽矩形框选对象，支持添加/替换选择；导出到 TS 并在 samples/html 演示快捷键+框选效果。”

### P0-012：STEP/IGES（Wasm 直读）构建开关与最小接入（可选但建议尽早评估）

- **背景/用户故事**：离线/私有部署场景可能不能依赖 server-side 转换，需要浏览器直接打开 STEP/IGES。
- **范围**
  - 增加 CMake 选项（例：`OCCTV_USE_STEP/OCCTV_USE_IGES`），在缺库时给出清晰错误。
  - 在 `RWShapeConv::ShapeLoader` 中扩展 `.step/.stp/.iges/.igs` 识别与打开（若 OCCT-Wasm SDK 具备对应 reader）。
  - samples/html 的 ImportPanel accept 放开对应扩展名（并对不支持构建给提示）。
- **验收标准**
  - 开关关闭时：UI 显示“不支持 STEP/IGES（构建未启用）”。
  - 开关开启且依赖齐全时：能成功加载一个最小 STEP（至少显示三角网格）。
- **样例数据**：需要补充 `samples/models/step/min.step` 与 `samples/models/iges/min.igs`。
- **失败策略**：解析失败返回详细错误并建议走 server-side 转换。
- **建议改动点**：`source/Viewer/CMakeLists.txt`、`source/RWShapeConv/ShapeLoader.*`、`samples/html/Panels/ImportPanel.js`。
- **Codex 提示词**
  - “增加 STEP/IGES 构建选项并在 ShapeLoader 中接入最小读取；samples/html 放开扩展名并在不支持时提示；给出如何验证的构建命令与样例文件路径。”

### P0-013：统一错误码与可诊断日志（Web 侧）

- **背景/用户故事**：一流产品必须“出错可定位”。当前有 `MessageGravity`，但缺少结构化错误码与上下文（文件名/参数/模块）。
- **范围**
  - 定义错误码（模块前缀 + 编号），在加载/转换/选择/测量等关键路径统一使用。
  - TS 层把错误显示为可复制文本（包含 errorCode、message、stack 可选）。
- **验收标准**
  - 至少覆盖：Shape 加载失败、Mesh 加载失败、缺依赖（MED/DRACO/STEP/IGES）、导出失败。
  - samples/vue-viewer：统一 toast/snackbar 展示错误摘要，可展开详情。
- **样例数据**：故意加载损坏文件（可用 0 字节文件模拟）。
- **失败策略**：不崩溃；错误上报后 viewer 可继续使用。
- **建议改动点**：C++ `Message` 输出点、TS `printMessage` 回调/事件桥接、Vue UI。
- **Codex 提示词**
  - “定义并落地结构化错误码：C++ 发出带 errorCode 的消息；TS 捕获并在 Vue 侧统一展示（摘要+详情可复制）；覆盖加载/依赖/导出关键路径。”

### P0-014：性能/内存统计与可复测基准（Web）

- **背景/用户故事**：大模型能力必须可量化。现在有 `toShowStats`、`dumpMemInfo`，但缺少统一基准输出与复测脚本。
- **范围**
  - 给出至少 2 个可复测指标：加载耗时（ms）、峰值内存（近似也可）、首帧时间。
  - 在 samples/html 增加“Benchmark”面板：一键运行并输出 JSON。
- **验收标准**
  - 同一模型重复运行，输出结果格式稳定。
  - 文档：`DOCUMENT/TODO/benchmark.md`（若新增）描述如何复测与解读。
- **样例数据**：`as1-oc-214.bbrep`、`buckling22.pos`。
- **失败策略**：不支持的统计项输出 null，不影响其它项。
- **建议改动点**：`source/Viewer/WasmViewer.*`（计时/统计导出）、TS、samples/html。
- **Codex 提示词**
  - “增加可复测基准：记录加载耗时/首帧/内存统计，导出 JSON；在 samples/html 增加 Benchmark 面板一键运行；在 DOCUMENT/TODO 写基准说明。”

### P0-015：Shape 与 Mesh 的“场景对象搜索”（按名称/属性）

- **背景/用户故事**：装配树只是入口，用户还需要搜索（名称、属性、正则、标签）并定位/批量操作。
- **范围**：实现 `search(query)` 返回 ObjectId 列表 + 命中字段；支持模糊匹配与过滤条件（最小实现即可）。
- **验收标准**
  - Vue 示例：搜索框输入关键词 → 列表显示命中对象 → 点击定位/隔离。
- **样例数据**：至少对现有 ObjectId 生效；后续可接入真实装配属性。
- **失败策略**：query 为空返回全部或空（需明确）。
- **建议改动点**：TS 层优先实现（基于 SceneGraph/objectsInside）；必要时再下沉 C++。
- **Codex 提示词**
  - “实现场景搜索：按名称/属性模糊匹配返回对象列表；在 Vue 示例加入搜索 UI，支持点击定位与批量隔离。”

### P0-016：剖切体验增强（交互式剖切 + 剖切预设）

- **背景/用户故事**：剖切是评审高频功能；需要更好的交互（拖拽、预设、恢复）。
- **范围**
  - 基于现有 `InteractiveClipping`：增加预设（XY/XZ/YZ、按 bbox 自动放置、按 SceneGraph node bbox 放置）。
  - 与 Saved Views/Project 互通（P0-009/P0-010）。
- **验收标准**
  - HTML/Vue：一键设置 XY 剖切、反向、开启 capping；支持“重置剖切”。
- **样例数据**：任意 BREP/GLB。
- **失败策略**：bbox 无效时禁用“按 bbox 放置”。
- **建议改动点**：TS `InteractiveClipping.ts` + samples UI。
- **Codex 提示词**
  - “在现有 InteractiveClipping 上增加剖切预设（XY/XZ/YZ、按 bbox 放置、重置），并在 samples/html 增加对应 UI 控件与演示。”

### P0-017：统一导出入口（截图 + Shape/Mesh 导出）与下载体验

- **背景/用户故事**：一流 Viewer 要能把当前结果导出（截图、模型、CAE mesh），并在浏览器端可下载。
- **范围**
  - 截图：复用 `makeScreenshot`，补齐 TS 侧返回 Blob/下载。
  - Shape 导出：已有 `.brep/.sbrep` 保存；补齐 UI 与默认命名。
  - Mesh 导出：已有 `saveModelMesh`（TS 暗示），补齐 UI 与提示。
- **验收标准**
  - samples/html：提供“下载截图 PNG”“导出当前 shape”“导出当前 mesh”按钮。
- **样例数据**：任意。
- **失败策略**：导出失败时弹错误码（与 P0-013 对齐）。
- **建议改动点**：TS `SCwebviewer.ts`/`InteractiveShape.ts`/`InteractiveCaeMesh.ts`、samples。
- **Codex 提示词**
  - “补齐统一导出体验：截图导出为 Blob 并下载；Shape/Mesh 导出提供 UI；失败走统一错误码；在 samples/html 增加按钮验证。”

### P0-018：完善 README/能力矩阵（面向用户的“支持列表”）

- **背景/用户故事**：用户需要明确知道“支持什么、不支持什么、怎么构建启用”。
- **范围**：补齐顶层 `ReadMe.md` 与 `DOCUMENT/architecture.md` 中的支持列表/构建开关/限制说明（格式、平台、依赖）。
- **验收标准**
  - 给出“Web 侧支持格式列表（默认/可选）”与“构建开关表”。
  - 与 samples 的 accept 列表一致。
- **样例数据**：不适用。
- **失败策略**：不适用。
- **建议改动点**：`ReadMe.md`、`DOCUMENT/*`、`TS_API_GUIDE.md`（若涉及）。
- **Codex 提示词**
  - “更新文档：整理 Web/桌面支持格式、构建开关、依赖与限制，并确保与 samples 的导入列表一致。”

---

## P1（强竞争力）

### P1-001：PMI/MBD（只读）展示：把 PMI 映射为可交互对象（文本/尺寸）

- **背景/用户故事**：AP242 PMI 是一流 CAD Viewer 标配；至少要能显示与筛选 PMI。
- **范围**：只读展示与列表；编辑后续再做。优先做：文本 + 尺寸（复用 `InteractiveTextLabel/InteractiveDimension`）。
- **验收标准**
  - 若导入来源包含 PMI（server-side 或 XDE），能显示并在列表中定位。
- **样例数据**：需提供含 PMI 的 STEP（AP242）样例，放到 `samples/models/step/pmi.step`。
- **失败策略**：不支持 PMI 时明确提示。
- **建议改动点**：取决于数据来源（server-side metadata 或 XDE）；TS/Viewer 侧需通用 PMI 对象表示。
- **Codex 提示词**
  - “实现 PMI 只读展示：定义 PMI 数据结构、导出到 TS、渲染为 TextLabel/Dimension，并在 Vue 侧提供 PMI 列表与点击定位。”

### P1-002：几何分析：干涉检查（AABB 粗筛 + 精确检测）

- **背景/用户故事**：装配评审需要快速发现干涉与最小间隙。
- **范围**
  - Phase1：AABB 粗筛（快速），输出可能干涉对。
  - Phase2：精确检测（基于 OCCT 算法）。
- **验收标准**
  - Vue 示例：选择两个对象→运行检测→输出结果列表→点击高亮/定位。
- **样例数据**：需一个含装配的样例（建议 step 转换后生成多个子件）。
- **失败策略**：计算耗时长时可取消（至少 UI 可显示“处理中”）。
- **建议改动点**：C++ 新增分析模块或放入 Viewer Manager；TS 封装与 UI。
- **Codex 提示词**
  - “实现干涉检查：先 AABB 粗筛再精确检测；导出结果到 TS；Vue 示例展示结果列表、点击高亮定位，并支持取消/超时提示。”

### P1-003：截面分析增强：剖切填充/剖面线/多剖面

- **背景/用户故事**：一流 Viewer 的截面分析不仅是裁剪盒，还要有剖切填充与剖面线。
- **范围**：多平面剖切、剖面线显示、填充样式（至少一套）。
- **验收标准**：对 Shape 生效；UI 可切换剖面线与填充；与 Saved Views 兼容。
- **样例数据**：BREP/GLB。
- **失败策略**：性能不足时提示并允许降级（只裁剪不填充）。
- **建议改动点**：ViewShape/Viewer 渲染与剪裁相关；TS/UI。
- **Codex 提示词**
  - “在现有剖切基础上增加剖面线与剖切填充，多剖面支持；TS 暴露参数；samples/vue-viewer 增加控制面板与 Saved Views 兼容。”

### P1-004：CAE Probe：点值查询（插值/最近单元）与结果面板

- **背景/用户故事**：CAE 后处理一流体验必须有 Probe：点击模型返回当前字段值（含单位/分量/时间步）。
- **范围**：对标量/向量至少支持一种插值策略；结果面板可固定/导出。
- **验收标准**：在 `buckling22.pos` 上能点击返回值；时间步切换后值更新。
- **样例数据**：`install/wasm32-fallback/models/gmsh/buckling22.pos`。
- **失败策略**：无属性/未加载属性时提示并给“去加载属性”的入口。
- **建议改动点**：`ViewCae` picking + TS event + Vue UI。
- **Codex 提示词**
  - “实现 CAE Probe：点击网格返回属性值（含单位/分量/时间步）；提供 TS API/事件；在 Vue 示例增加 Probe 面板并支持固定/导出。”

### P1-005：CAE Plot：沿线/随时间曲线（XY Plot）

- **背景/用户故事**：工程师需要沿线或随时间查看数值曲线。
- **范围**：最小实现：随时间曲线（选点后输出该点随时间变化）；沿线作为二期。
- **验收标准**：能导出 CSV；UI 支持选择属性/分量。
- **样例数据**：需要含多时间步的 CAE 数据（若仓库无则补充样例）。
- **失败策略**：时间步不足提示。
- **建议改动点**：Vue 绘图库选择需谨慎（尽量不新增大依赖）；可先用简单 SVG。
- **Codex 提示词**
  - “实现最小 CAE 时间曲线 Plot：选点后生成随时间变化曲线（SVG/Canvas），可导出 CSV；避免引入大型依赖；给出样例数据要求。”

### P1-006：大模型：Shape 分块/LOD（借鉴 LodCae 思路）

- **背景/用户故事**：装配/大模型在 Web 端必须靠分块/LOD 才能可用。
- **范围**：定义 Shape 的分块阈值（面数/三角数/bbox），支持按需加载与卸载。
- **验收标准**：提供“按需加载开关”；在基准模型上能降低首帧时间与峰值内存（见 P0-014）。
- **样例数据**：需要大模型（若无则合成或引入外部样例）。
- **失败策略**：无法分块时回退到全量加载并提示。
- **建议改动点**：`RWShapeConv`/`ViewShape`/`Viewer::TaskManager`。
- **Codex 提示词**
  - “为 Shape 引入分块/LOD：定义阈值与分块策略，支持按需加载/卸载；输出基准对比数据；保证不支持时可回退全量加载。”

### P1-007：Web Worker 协同（无 pthread 环境的任务分发）

- **背景/用户故事**：部分环境不能开 Wasm pthread，需要用 WebWorker 做 IO/解析/预处理，主线程只渲染。
- **范围**：先把“文件读取/解压/基础解析”迁移到 Worker；C++ 侧仍在主线程执行渲染相关。
- **验收标准**：在禁用 pthread 的 wasm32 构建中可用；UI 不明显卡顿。
- **样例数据**：`as1-oc-214.glb`。
- **失败策略**：Worker 不可用时回退主线程并提示。
- **建议改动点**：`samples/*` 前端侧 + TS wrapper（worker 封装）。
- **Codex 提示词**
  - “在 wasm32（无 pthread）模式下引入 WebWorker：把文件读取/预处理移到 worker，主线程调用现有 uploadArrayBuffer；提供 fallback 与性能对比。”

### P1-008：统一“命令系统”（可配置快捷键/命令面板）

- **背景/用户故事**：一流工具需要可配置的命令系统（类似命令面板/快捷键），便于扩展与脚本化。
- **范围**：TS/Vue 实现命令注册、快捷键绑定、命令面板 UI（最小可用）。
- **验收标准**：至少把 fitAll、toggleViewCube、isolateSelection、toggleClipping 等做成命令。
- **样例数据**：不依赖特定模型。
- **失败策略**：快捷键冲突时提示并允许覆盖。
- **建议改动点**：`samples/vue-viewer/src`。
- **Codex 提示词**
  - “在 Vue 示例实现最小命令系统：命令注册+快捷键绑定+命令面板 UI；接入现有 ViewerService 的常用动作并支持冲突提示。”

---

## P2（产品化与生态）

### P2-001：Markup（批注）系统：箭头/框/文字 + 截图钉住视图

- **背景/用户故事**：评审协作需要批注与截图，形成可追踪的审阅记录。
- **范围**：本地 JSON 版（无服务端）；对象绑定 Saved View；可导出/导入。
- **验收标准**：能创建/编辑/删除批注；导出后可恢复。
- **样例数据**：任意。
- **失败策略**：对象缺失时批注降级为屏幕坐标批注。
- **建议改动点**：TS + Vue UI；可复用 `InteractiveTextLabel`。
- **Codex 提示词**
  - “实现本地 Markup：箭头/框/文字/截图，导出/导入 JSON，可绑定 Saved View；对象缺失时降级为屏幕坐标批注。”

### P2-002：导出报告（HTML/PDF）：视图 + 测量 + 关键截图

- **背景/用户故事**：工程交付需要一键生成报告。
- **范围**：先做 HTML 报告（包含图片与表格），PDF 作为可选增强。
- **验收标准**：从 Project 导出报告，包含：模型信息、Saved Views 截图、测量列表。
- **样例数据**：任意。
- **失败策略**：截图失败时仍生成报告但标记缺失。
- **建议改动点**：前端侧生成 HTML；截图复用 P0-017。
- **Codex 提示词**
  - “实现 HTML 报告导出：包含 Saved Views 截图与测量表格；复用截图导出能力；失败时报告仍生成并标记缺失项。”

### P2-003：权限/水印/审计（企业版预留）

- **背景/用户故事**：企业场景需要水印与基本审计信息（谁打开、何时导出）。
- **范围**：先做“水印覆盖层 + 导出时写入水印 + 本地审计 JSON”。
- **验收标准**：可配置水印文本；截图/报告带水印；审计记录写入本地文件。
- **失败策略**：无。
- **建议改动点**：samples/vue-viewer。
- **Codex 提示词**
  - “在 Vue 示例加入水印与本地审计：水印覆盖在 viewer 上并影响截图/报告；审计记录导出为 JSON（打开/导出/分享动作）。”

