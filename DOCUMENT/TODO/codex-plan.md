# 使用 Codex 落地 OCC_VIEWER 增强功能方案

本文面向“用 Codex 在本仓库内持续落地功能增强”的执行方案。内容包含：落地方法论、任务拆解（P0/P1/P2）、每类任务的 Codex 提示词模板、验收与质量门禁、以及与本项目代码结构的映射关系。

> 约定：本文只定义“怎么做、做到什么程度、怎么验收”。具体需求细节（行业、格式、平台约束）建议以 Issue 形式逐条固化，再让 Codex 逐条落地。

---

## 0. 总目标（Definition of Done）

对标国际一流 CAX Viewer，本项目的阶段性 DoD 建议分三层：

1) **能力层**：CAD 主流格式 + 装配结构 + PMI/MBD + 几何分析 + CAE 后处理（本项目已强）+ 导出/报告。
2) **性能层**：大模型可用（加载、交互帧率、内存峰值可控）+ LOD/分块/流式 + 统计与诊断。
3) **产品层**：可配置 UI/命令系统、可脚本化 API、可复现工程文件、协作审阅（可选）。

---

## 1. 仓库现实基线（落地前提）

结合仓库现状（以 `DOCUMENT/architecture.md`、`source-ts/Viewer-ts`、`samples/*` 为准）：

- Web 侧有完整的 Wasm Viewer + 手写 TypeScript API（`source-ts/Viewer-ts`）与样例（`samples/html`、`samples/vue-viewer`）。
- Shape 加载链路当前“实际可用”的格式集中在 **BREP / glTF(glb) / STL**（`source/RWShapeConv/ShapeLoader.cpp` + `samples/html/Panels/ImportPanel.js` 的 accept 列表）。
- CAE 后处理体系完整：标量/向量/流线、动画、ColorScale、切片/等值/裁剪/块等（`ViewCae*` + TS wrapper）。
- Viewer 链接的 OCCT lib 列表（例如 `source/Viewer/CMakeLists.txt`）当前未包含 `TKSTEP/TKIGES` 等，意味着 **STEP/IGES 并未在当前构建配置中“自然存在”**，后续需要明确“server-side 转换”还是“client-side Wasm 直读”路线，或两者并行。

---

## 2. 用 Codex 落地的工作流（强推荐）

### 2.1 一条 Issue 的最小结构（建议模板）

每个 Issue 请固定包含以下字段（这能显著提升 Codex 的命中率与交付质量）：

- **背景/用户故事**：谁在什么场景要解决什么问题
- **范围**：包含/不包含（避免无边界扩张）
- **验收标准（可执行）**：
  - 功能：具体 UI/接口行为
  - 性能：加载时间/帧率/内存峰值（给数字）
  - 兼容：桌面/wasm、浏览器/平台
- **样例数据**：仓库内模型路径或最小复现文件（若涉外部数据，先放到 `samples/models` 或私有存储并在 Issue 里说明获取方式）
- **失败策略**：缺依赖/不支持时应如何报错（日志码、UI 提示）

Codex 执行时就按“验收标准”反推改动范围，能最大化避免跑偏。

### 2.2 Codex 执行节奏（每条 Issue 的建议闭环）

1) **定位**：让 Codex 用 `rg` 找到相关模块/接口（C++ manager、Emscripten 导出、TS wrapper、sample UI）。
2) **计划**：让 Codex 用 `update_plan` 拆成 3–6 步（每步可验收）。
3) **实现顺序（建议固定）**：
   1. C++ 核心能力（算法/IO/数据结构）
   2. Wasm 导出（必要时）+ C++/JS 数据通道（JSON/Vector/Map，避免裸指针）
   3. TS wrapper（`source-ts/Viewer-ts`）补齐类型与 Promise 桥接
   4. 示例与文档（`samples/*`、`TS_API_GUIDE.md`、`DOCUMENT/*`）
4) **验证**：最小验证优先（能本地跑就跑；否则至少 build 通过 + 示例可用）。
5) **交付说明**：改动点、接口变化、升级注意事项、后续待办。

### 2.3 建议 Codex 常用命令（本仓库）

- 查找：`rg -n "<pattern>" source source-ts samples DOCUMENT`
- Wasm 构建（Windows）：`adm/scripts/wasm_build.bat -nopause`
- Web 示例（文档里提到的方式）：`adm/scripts/server.py --directory install/wasm32-fallback --open samples/html/index.html`
- CI 参考：`.github/workflows/ci-build.yml`（Linux + wasm32）

---

## 3. 路线图（P0/P1/P2）与 Codex 可执行任务拆解

### P0：决定能否对标“一流 Viewer”的硬门槛

#### Epic P0-A：CAD 主流格式导入（STEP/IGES）落地

**推荐策略：双轨并行（先快后稳）**

1) **A1：Server-side 转换管线（优先）**
   - 目标：STEP/IGES → 分块 glTF/glb + 元数据（装配树/属性）→ Web 侧加载
   - 原因：Wasm 直读 STEP/IGES 会受 OCCT-Wasm SDK、体积、内存、线程限制影响；server-side 能更快进入可用状态。
   - Codex 任务拆解（仓库内可落地部分）：
     - 定义/固化“转换产物规范”：`DOCUMENT/TODO/format-step-iges-conversion-spec.md`（建议新增）
       - 产物：`*.glb` + `assembly.json` + `properties.json` + `views.json`（示例命名）
       - 坐标系/单位/颜色/材质映射规则
       - 分块策略（按部件/按层级/按 bbox/按面数阈值）
     - 在 Viewer 侧补齐“装配/元数据装载 API”（见 P0-B）。

2) **A2：Client-side（Wasm）直读 STEP/IGES（作为增强项）**
   - C++ 侧可能涉及：`STEPControl_Reader`、`IGESControl_Reader` 或 XDE（CAF）路线（取决于是否要装配/PMI）。
   - 构建侧涉及：在 `source/Viewer/CMakeLists.txt` 等处引入 `TKSTEP/TKIGES/TKXDESTEP` 等库（以实际 OCCT 版本为准）。
   - Codex 任务拆解：
     - 增加 CMake 选项（例）：`OCCTV_USE_STEP`, `OCCTV_USE_IGES`，并在缺库时给清晰错误
     - 在 `RWShapeConv::ShapeLoader` 中扩展 `ProbeFileFormat/OpenShape` 支持 `.step/.stp/.iges/.igs`
     - 在 `samples/html/Panels/ImportPanel.js` accept 列表中放开对应扩展名
     - 在 TS API 文档里补齐支持列表与限制说明

**验收标准（建议）**
- Web 示例中可直接拖拽/选择 `.step/.iges`（至少一种方式可用：server-side 或 wasm 直读）
- 能显示基本颜色/材质，装配层级至少保留到“部件级”
- 错误提示可用：不支持/缺依赖/文件损坏时返回明确 message（`MessageGravity` 分级）

#### Epic P0-B：装配树/对象语义与批量操作

现状：已有 ObjectId/InteractiveObject/SelectionManager/Explode 等基础，但缺少一流产品常见的“产品结构树 + 查询/过滤/批量操作”稳定 API。

Codex 任务拆解（仓库内落地）：

- B1：定义领域模型与序列化（C++/TS 对齐）
  - 新增/扩展：`Viewer::Utils` 中的装配节点结构（id、name、parent、children、bbox、tags、props）
  - TS：增加 `AssemblyNode`、`PropertyBag` 类型与查询接口
- B2：Viewer 层提供最小 API
  - `interactiveContext.getSceneGraph()`（示例名称）返回树
  - `interactiveContext.findByPath/findByProp/filter()` 等最小查询
  - 批量操作：hide/show/isolate/transparent、fit-to-node
- B3：Vue 示例落地 UI
  - 左侧树 + 搜索框 + 批量操作按钮（隔离/隐藏/定位）

验收标准（建议）：
- 能在 Vue 示例中展示树、定位到选中节点（相机 fit）、批量隐藏/隔离工作正常
- 大场景（>1k 节点）树操作不卡死（至少异步/虚拟列表策略预留）

#### Epic P0-C：测量（Measurement）工作流产品化

现状：已有 `InteractiveDimension`（长度/角度/半径），但缺少“从选择推断/捕捉 + 结果列表管理 + 导出”。

Codex 任务拆解：

- C1：补齐测量类型（按优先级）
  - 点坐标/距离（点-点、点-线、点-面、最短距离）
  - 直径/半径自动识别（选中圆边/圆面）
  - 面积/体积/质心（Shape 侧）
- C2：测量结果面板
  - 结果列表（可复制/导出 JSON/CSV）
  - 单条结果可高亮/定位/删除
- C3：捕捉与选择辅助
  - 捕捉点（端点/中点/圆心/交点）与阈值
  - 交互提示（鼠标悬停显示候选）

验收标准（建议）：
- 不依赖手工输入三维坐标，用户通过选择即可完成常见测量
- 可导出测量结果并可复现（再次打开可恢复标注对象）

---

### P1：强竞争力（拉开与普通 Viewer 的差距）

#### Epic P1-A：PMI/MBD（AP242）显示与视图状态（Saved Views）

Codex 任务拆解：
- A1：Saved Views
  - 相机位姿/投影/可见性集合/剖切状态的保存与恢复
- A2：PMI 展示（先只读）
  - 文本、尺寸、公差框、基准符号等映射为 `InteractiveTextLabel/InteractiveDimension` 类对象
- A3：筛选/搜索/列表
  - 按类型/层级/属性过滤 PMI

验收标准：
- 至少能“加载→显示→列表→点击定位→切换视图状态”

#### Epic P1-B：几何分析工具

优先级建议：
1) 截面分析增强（多剖面、剖切填充/剖面线）
2) 干涉检查/最小间隙（装配必备）
3) 厚度/曲率/拔模角（按行业选择）

验收标准：
- 结果可视化可交互（高亮/定位），并可导出报告/截图

#### Epic P1-C：大模型性能框架（Shape + Mesh 统一）

Codex 任务拆解：
- C1：资源与缓存预算（GPU/CPU 内存统计与上限）
- C2：LOD/分块策略落地到 Shape（参考 `LodCae` 思路）
- C3：视锥裁剪/遮挡裁剪（至少预留接口；Wasm/浏览器环境需谨慎）
- C4：Worker 化（Wasm pthreads 或 WebWorker 任务分发）

验收标准：
- 给出基准模型与指标：加载时间、首帧、交互帧率、峰值内存；在 CI 或本地脚本可复测

---

### P2：产品化与生态（规模化落地能力）

#### Epic P2-A：协作审阅（Markup/Comment）
- 2D/3D Markup：箭头/框/手绘/文字/截图钉住视图
- 评论线程（本地 JSON 版 → 可选对接服务端）
- 权限/水印/审计（企业版方向）

#### Epic P2-B：导出与报告
- 批量截图、视频录制（Web 侧可先做帧序列导出）
- 工程文件：场景 + 视图 + 标注 + 结果配置一键保存/恢复

---

## 4. Codex 提示词模板（可直接复制使用）

> 下面模板的目标是：让 Codex 一次只做一个闭环（代码+示例+验收），避免“泛泛而谈”。

### 4.1 C++ 新增能力（无 Wasm 导出）

```
在 OCC_VIEWER 中实现 <功能名>（仅 C++），要求：
1) 代码位置建议：<module/path>
2) 对外接口：<class/method>（若需新增头文件请说明）
3) 错误处理：使用 Message::SendFail/Warning/Info，并给出可定位的错误文本
4) 单元/回归：若本仓库已有相邻测试入口则补最小测试；否则给出最小可运行示例（Draw/CLI）
5) 不要改动无关代码；实现后给出如何验证的命令
```

### 4.2 “C++ → Wasm 导出 → TS wrapper → samples”整链路

```
把 <功能名> 暴露到 Web API，并在 samples/html 增加一个最小按钮演示：
1) C++：在 Viewer/<Manager>.{h,cpp} 增加方法 <method>，并在 WasmViewer 导出（Emscripten binding）
2) TS：在 source-ts/Viewer-ts/<file>.ts 增加类型声明与封装方法；Promise/回调遵循 TS_API_GUIDE.md 的约束（不暴露指针/HEAP）
3) Sample：在 samples/html/Panels/<Panel>.js 加 UI 控件，演示成功路径与失败提示
4) 验收：提供一组明确步骤（打开 sample → 点击 → 看到效果），并确保 cmake 构建通过
```

### 4.3 性能/基准任务（必须可复测）

```
为 <场景> 增加可复测的性能基准：
1) 指标：加载耗时、首帧时间、内存峰值（至少其中两项）
2) 数据：使用 samples/models 下的 <模型>，或生成一个可重复的合成数据
3) 输出：将结果写入日志（分级），并在 DOCUMENT/TODO 下补一页基准说明
4) 不引入外部在线依赖；能在 CI/Linux 或本地脚本复跑
```

---

## 5. 质量门禁（建议强制）

每个功能 PR（或每次 Codex 落地）至少满足：

- **Build**：本地或 CI 至少一个目标能编过（Linux 或 wasm32）
- **样例可用**：`samples/html` 或 `samples/vue-viewer` 至少有一个入口能演示
- **错误可诊断**：失败时有清晰错误信息（包含模块名/文件名/关键参数）
- **接口稳定**：TS API 变更必须同步 `.d.ts`/Typedoc（若启用）

---

## 6. 建议的下一步（把路线图变成可执行 Issue）

建议先创建 10–20 个 P0 Issue（每个 1–3 天工作量），按下面顺序：

1) STEP/IGES：先定义转换产物规范（server-side 路线），并把 Vue 树 UI 做出来
2) 装配树 API：最小可用（树+查询+批量操作）
3) 测量：最短距离 + 直径/半径自动识别 + 结果列表导出
4) Saved Views：相机 + 可见性 + 剖切状态的保存/恢复

完成上述后再进入 PMI/干涉/大模型性能框架（P1）。

