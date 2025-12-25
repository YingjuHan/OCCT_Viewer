# 开发环境准备指南

本指南整理自 `adm/scripts/wasm_build.bat`, 顶层 `CMakeLists.txt` 以及 `source-ts/Viewer-ts/CMakeLists.txt` 中的约束，帮助新开发者一次性补齐 OCC_VIEWER 的所有构建依赖。

## 1. 基础环境
- **操作系统**：Windows 10/11（桌面 + Bat 脚本默认路径），其他平台可按 CMake 配置手动调整。
- **编译工具链**：CMake ≥ 3.7、MSVC/Clang/MinGW（需支持 C++17），Git（用于版本信息与 `execute_process` 获取提交哈希）。
- **硬件**：建议 16GB+ RAM、足够磁盘空间（`work/occ_viewer*` 下会生成多套 wasm/桌面产物）。
- **OpenGL 驱动**：桌面模式需要系统 OpenGL（非 Windows 会强制 `find_package(OpenGL REQUIRED)`）。

## 2. 第三方依赖一览
| 依赖 | 作用 | 配置方式 |
| --- | --- | --- |
| **OpenCASCADE** | Viewer 依赖的核心 OCCT SDK | 安装后把路径写入 `OpenCASCADE_DIR`/`OpenCASCADE_ROOT_DIR`（建议复制 `adm/scripts/wasm_custom.bat.sample` 为 `wasm_custom.bat` 并在其中配置）。 |
| **RapidJSON** | Mesh/属性解析 | 通过 `RAPIDJSON_DIR` 或 `3RDPARTY_DIR` 提供；找到后自动定义 `HAVE_RAPIDJSON`。 |
| **HDF5 + MEDFile** | 解析 MED 网格 | 仅当 `OCCTV_USE_MEDFILE=ON` 时必需；CMake 模块会根据 `HDF5_ROOT_DIR`、`MEDFile_ROOT_DIR` 搜索并定义 `HAVE_MEDFILE`。 |
| **Draco** | 网格压缩 | 受 `OCCTV_USE_DRACO` 控制，需提供 `draco_DIR` 或 `DRACO_DIR`；成功后定义 `OCCTV_HAVE_DRACO`。 |
| **FreeType** | 字体渲染（WASM） | EMSCRIPTEN 构建时自动 `include(freetype)`，需在 `FREETYPE_DIR` 中指向对应安装。 |
| **SMESH** | Mesh Exchanger 插件 | 当 `OCCTV_BUILD_MESH_EXCHANGER=ON` 时需要，通过 `SMESH_DIR`（见 `include(SMESH)`）。 |
| **TypeScript / TypeDoc** | Web Viewer 与文档 | EMSDK 自带 Node，可 `npm install -g typescript` 以提供 `tsc.cmd`；若启用 TS 文档需另外全局安装 `typedoc` 与 `typedoc-plugin-markdown`。 |

> 运行时 DLL：如依赖落在非标准路径，可在 `CMakeLists.txt` 的 `3RDPARTY_DLL_DIRS` cache 变量中填入分号分隔的目录列表，CMake 会自动加入 PATH。

## 3. EMSDK / Wasm 工具链
1. 安装 Emscripten SDK，并确认 `EMSDK_ROOT\emsdk_env.bat` 可执行（脚本中直接 `call "%EMSDK_ROOT%\emsdk_env.bat"`）。
2. `wasm_build.bat` 会把 `OCC_VIEWER_TOOLCHAIN` 设置为 `${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`，确保路径存在。
3. `OCC_VIEWER_WASM_CONFIGS` 默认编译 `wasm32-pthread` 与 `wasm32`，可按需改成 `wasm64*`，同时通过下列开关控制行为：
   - `OCC_VIEWER_USE_PTHREADS`：多线程（自动随 `*-pthread` 开启）。
   - `OCC_VIEWER_USE_WASM_MEMORY64`：Memory64 支持（`wasm64*`）。
   - `OCC_VIEWER_USE_WASM_EXCEPTION`：`-fwasm-exceptions`，单线程 `wasm32` 默认关闭防止兼容性问题。
4. 脚本会把 `CMAKE_C_FLAGS/CMAKE_CXX_FLAGS` 注入额外参数（如 Memory64、PThreads），不需要手动修改 `CMakeLists.txt`。

## 4. 构建脚本用法
脚本入口：`adm/scripts/wasm_build.bat [-nopause]`

| 变量 | 默认 | 说明 |
| --- | --- | --- |
| `OCC_VIEWER_DO_CMAKE` | 1 | 是否运行 CMake 配置。 |
| `OCC_VIEWER_DO_MAKE` | 1 | 是否编译 `all` 目标（自动 `-j %NUMBER_OF_PROCESSORS%`）。 |
| `OCC_VIEWER_DO_INSTALL` | 1 | 是否执行 `cmake --build … --target install`。 |
| `OCC_VIEWER_DO_CLEAN` | 0 | 是否在同名 build/install 目录存在时清理。 |
| `OCC_VIEWER_BUILD_TYPE` | Release | 同步到生成目录后缀 `-Release`。 |
| `OCC_VIEWER_BUILD_WEB_VIEWER` | ON | 控制 TypeScript/资源安装。 |
| `OCC_VIEWER_BUILD_FOLDER_NAME` | `occ_viewer` | 输出目录前缀（位于 `${OCC_VIEWER_BUILD_ROOT}` 下）。 |

常用步骤：
1. 若需自定义路径/选项，复制 `adm/scripts/wasm_custom.bat.sample`（若存在）为 `wasm_custom.bat` 并覆盖变量。
2. 运行 `adm/scripts/wasm_build.bat`，脚本会自动：
   - 在 `work/occ_viewer-<abi>-make` 下生成/复用构建目录；
   - 把日志写入 `work/occ_viewer-<abi>-build.log` 并在失败时输出；
   - 根据 `OCC_VIEWER_WEB_DEST_DIR`（由 `OCCTV_WEBVIEWER_INSTALL_PREFIX` 派生）同步 `.gitattributes/.gitignore`、Wasm 包、脚本、示例数据等。

## 5. Web Viewer 与 TypeScript
- `OCCTV_BUILD_WEB_VIEWER=ON` 时，CMake 会在 EMSCRIPTEN 配置中添加 `source-ts/Viewer-ts`，调用 `tsc` 生成 JS/声明/Source Map 并安装到 `${OCCTV_WEBVIEWER_INSTALL_PREFIX}/wasm32`。
- `ViewerBuildConfig.ts` 属于生成文件（`tsc-cfg` 目标），不要直接修改；如需更新默认配置，请改 `source-ts/Viewer-ts/ViewerBuildConfig.ts` 模板。
- 如果需要产出 TypeScript API 文档，设置 `OCCTViewer_BUILD_DOC_TYPESCRIPT=ON` 并确保系统可执行 `typedoc`，否则 CMake 会 `SEND_ERROR`。
- `OCC_VIEWER_WEB_DEST_DIR` 下会出现以下内容：
  - `wasm32*/`：Wasm 模块、JS glue、TS 编译产物。
  - `source-ts/Viewer-ts`、`source-ts/SCwebviewer-ts`：原始 TS 与生成文件备份。
  - `scripts/`, `samples/html`, `samples/vue-viewer` 等资源，由顶层 `CMakeLists.txt` 的 install 规则拷贝。

## 6. Vue 示例环境
- 目录：`samples/vue-viewer`。
- 依赖：`vue@^3.2`, `vuetify@^3.6`, `@vue/cli-service@~5`, `typescript@^4.6.4` 等。运行：
  ```bash
  cd samples/vue-viewer
  npm install
  npm run serve
  ```
- `package.json` 中的 `@hyj/SCwebviewer` 指向 `../../install/wasm32-fallback`，因此需先完成 wasm install，再启动示例。
- 若想在其它项目中复用 SCwebviewer，可把对应 install 目录发布到私有 npm/registry，或修改依赖路径为实际部署位置。

## 7. 常见检查清单
1. **环境变量**：`EMSDK_ROOT`、`EMSDK`、`OpenCASCADE_DIR`、`FREETYPE_DIR`、`RAPIDJSON_DIR`、`DRACO_DIR` 等是否在脚本/缓存中设置。
2. **Node 工具**：`tsc --version`、`typedoc --version`（如启用文档）在 `emsdk_env` 后能否调用。
3. **权限**：`work/` 与 `install/` 目录需可写，防止 `cmake --build --target install` 失败。
4. **显卡/驱动**：桌面调试确保具备 OpenGL 4.x 驱动；Web 调试建议使用最新 Chrome/Firefox（WebGL2）。
5. **示例数据**：`samples/html`/`samples/vue-viewer` 依赖 `install/*` 目录的 `*.gltf/*.brep/*.mdb` 示例，install 过程中若自定义前缀，请同步更新示例配置。

准备就绪后，可在 `adm/scripts/wasm_build.bat -nopause` 路径下首次构建，随后按照需要启用桌面或 Web 前端。若需 CI 集成，可将上述变量写入 CMake Cache 或环境，并复用同一脚本实现无人值守构建。
