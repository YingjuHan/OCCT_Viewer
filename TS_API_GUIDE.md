# TypeScript 层与 API 准则

本项目在 Web 侧完全放弃 EMSDK 提供的 TypeScript 生成器，所有声明与实现均由开发者手写，以确保接口形状稳定、规避指针暴露风险，并与不同版本的 EMSDK/Wasm 工具链保持兼容性。

## 设计目标
- **最小胶水层**：JS 只暴露 `createViewer` 这类高层 Promise 接口（`source/Viewer/ts/Viewer-wasm.d.ts.in`），不引入 EMSDK 自动生成的 `Module`/`ccall` 等绑定。
- **对象化交互**：`SCwebviewer` 及其配套 Manager/Extensions（位于 `source-ts/Viewer-ts`）负责封装所有 Viewer 能力，通过结构化对象、枚举和 `ObjectId` 字符串传递状态。
- **指针隔离**：任何与 C++ 交互的回调都必须以 JS 可序列化数据或数值句柄为边界（例如 `_cppWasmCallBack` 只接受 Promise 句柄 ID），禁止在 API 中暴露裸内存指针、`HEAP*` 缓冲区或 EMSDK Runtime 对象。

## API 规范
1. **手写声明**：`.d.ts` 文件由我们维护并跟随源码提交，新增接口时先补全类型/文档再实现，确保 IDE/Typedoc 行为可控。
2. **模块划分**：按功能拆分 `Interactive*`, `ViewerEvent*`, `CommonTools`, `Subview` 等 TS 源文件，并通过 `SCwebviewer.ts` 聚合导出，避免外部直接引用底层实现。
3. **Promise 桥接**：异步加载、选取等流程统一依赖 `IPromise` 绑定（`source/RWBase/IPromise.h`），JS 侧通过数字句柄唤起 `_cppWasmCallBack`，由 C++ 触发 `resolve/reject`。
4. **数据通道**：跨语言传输仅允许使用 JSON 兼容对象、`std::vector`、`std::map` 映射到 JS Array/Map（`Viewer::EmscriptenHelper`），如确需传递大型缓冲，必须通过 Viewer Manager 的高层接口封装。

## CMake 配置策略
TypeScript 构建完全由 CMake 驱动，既保持“手写接口”的思想，又能自动安装到 Web Viewer 发布目录。

### 顶层 `CMakeLists.txt`
- `OCCTV_BUILD_WEB_VIEWER`（默认 ON）决定是否编译/安装 TS 层，同时受 `EMSCRIPTEN` 开关控制（`CMakeLists.txt` 第 128 行附近）。
- `OCCTV_WASM_CONFIGS`、`OCCTV_USE_WASM_EXCEPTION/MEMORY64/PTHREADS` 控制 Wasm 目标 ABI 与编译特性，`ViewerBuildConfig.ts` 根据这些配置生成（`CMakeLists.txt` 第 169–175 行）。
- `OCCTV_WEBVIEWER_INSTALL_PREFIX` 指定 Web 产物安装根目录，CMake install 规则会把 Wasm、JS、TS 源与示例同步到该路径（`CMakeLists.txt` 第 178 行及 494–534 行）。

### `source-ts/Viewer-ts/CMakeLists.txt`
1. **TSC 目标**：`tsc` 使用 `tsconfig.json.in` 模板生成配置，再调用 `tsc(.cmd)` 编译所有手写 TS 源，产生 `.js/.d.ts/.map`（文件开头到第 68 行）。
2. **配置生成**：`ViewerBuildConfig.ts` 独立编译为 `tsc-cfg` 目标，以便 IDE/Typedoc 在无需 EMSDK 的情况下获得编译期常量（第 79–111 行）。
3. **文档与打包**：可选 `doc-ts` 目标通过 `typedoc` 输出 Markdown 文档；`package.json` 同样使用模板写入 build 目录并安装到 Web 前缀（第 112–151 行）。
4. **安装规则**：所有 `tsc`、`tsc-cfg` 产物以及原始 TS 源会被 `install()` 拷贝至 `${OCCTV_WEBVIEWER_INSTALL_PREFIX}` 下对应的 `wasm32/` 与 `source-ts/` 子目录，方便示例与外部项目复用。

## 扩展流程
1. **更新接口**：在对应 TS 文件补充类型/方法，并同步 `.d.ts`（若影响外部 API）。
2. **桥接实现**：在 C++ 层通过 `EmscriptenHelper`、`SelectionMeshEntity` 等工具绑定新对象，确保所有返回值均为结构化数据。
3. **配置验证**：运行 `cmake --build <build> --target tsc tsc-cfg` 或直接执行 `adm/scripts/wasm_build.bat`，确认 TSC 输出与安装路径正确。
4. **示例验证**：通过 `samples/html` 或 `samples/vue-viewer` 引用安装产物，验证新增 API 行为。

## 验证与发布
- 若开启 `OCCTViewer_BUILD_DOC_TYPESCRIPT`，`doc-ts` 目标会在 CI 中生成并校验 Typedoc 输出，保证手写声明的一致性。
- `install/wasm*/` 产物可直接配合 `adm/scripts/server.py --directory install/wasm32-fallback --open samples/html/index.html` 进行端到端测试；确保在不同 EMSDK 版本下仍能构建通过，即可交付给前端或第三方项目使用。
