cmake_minimum_required (VERSION 3.7)
# 指定 CMake 最低版本要求为 3.7

set (OCCTViewer_PACKAGE "OCCTViewer")
# 设置项目包名变量
project (${OCCTViewer_PACKAGE})
# 创建项目，项目名为 OCCTViewer

# Path to CMake modules
# =====================
# CMake 模块路径
# =====================
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/adm/cmake" ${CMAKE_MODULE_PATH})
# 设置 CMake 模块路径为当前源目录下的 adm/cmake 目录

# Versioning
# ==========
# 版本控制
# ==========
set (APP_VERSION_MAJOR 1)
# 设置应用程序主版本号为 1
set (APP_VERSION_MINOR 0)
# 设置应用程序次版本号为 0
if (NOT OCCTV_VER)
  # 如果未定义 OCCTV_VER 变量
  set (OCCTViewer_VER_MAJOR -1)
  # 设置 OCCTViewer 主版本号为 -1
  set (OCCTViewer_VER_MINOR -1)
  # 设置 OCCTViewer 次版本号为 -1
  set (OCCTViewer_VER_PATCH -1)
  # 设置 OCCTViewer 修订版本号为 -1
else()
  # 如果定义了 OCCTV_VER 变量
  set(OCCTViewer_VER_MAJOR "$ENV{OCCTViewer_VER_MAJOR}")
  # 从环境变量获取主版本号
  set(OCCTViewer_VER_MINOR "$ENV{OCCTViewer_VER_MINOR}")
  # 从环境变量获取次版本号
  set(OCCTViewer_VER_PATCH "$ENV{OCCTViewer_VER_PATCH}")
  # 从环境变量获取修订版本号
endif()
set (OCCTViewer_GIT_HASH "")
# 初始化 Git 哈希值为空字符串

find_package (Git)
# 查找 Git 包
if (GIT_FOUND)
  # 如果找到 Git
  # extract commit hash using git
  # 使用 git 提取提交哈希值
  execute_process (COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE OCCTViewer_GIT_HASH
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # 执行 git 命令获取提交哈希值
  execute_process (COMMAND ${GIT_EXECUTABLE} show -s --format=%cd --date=format:%y
                   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE OCCTViewer_GIT_CMT_YEAR
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # 获取提交年份
  execute_process (COMMAND ${GIT_EXECUTABLE} show -s --format=%cd --date=format:%m
                   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE OCCTViewer_GIT_CMT_MONTH
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # 获取提交月份
  execute_process (COMMAND ${GIT_EXECUTABLE} show -s --format=%cd --date=format:%d
                   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE OCCTViewer_GIT_CMT_DAY
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # 获取提交日
  message ("git commit: ${OCCTViewer_GIT_HASH} date: ${OCCTViewer_GIT_CMT_YEAR}-${OCCTViewer_GIT_CMT_MONTH}-${OCCTViewer_GIT_CMT_DAY}")
  # 输出 git 提交信息
  if ("${OCCTViewer_VER_MAJOR}" STREQUAL "-1")
    # 如果主版本号为 -1
    math (EXPR OCCTViewer_VER_MAJOR ${OCCTViewer_GIT_CMT_YEAR})
    # 使用提交年份作为主版本号
    math (EXPR OCCTViewer_VER_MINOR ${OCCTViewer_GIT_CMT_MONTH})
    # 使用提交月份作为次版本号
    math (EXPR OCCTViewer_VER_PATCH ${OCCTViewer_GIT_CMT_DAY})
    # 使用提交日期作为修订版本号
  endif()
endif()

# set(OCCTV_LIBTYPE STATIC)
set(OCCTV_LIBTYPE SHARED)
# Multi-config or single config build
# ===================================
# 多配置或单配置构建
# ===================================
get_property (_generator_is_mc GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
# 获取生成器是否为多配置属性
if (_generator_is_mc AND NOT DEFINED CMAKE_BUILD_TYPE)
  # 如果是多配置生成器且未定义构建类型
  set (CMAKE_CONFIGURATION_TYPES Release Debug CACHE INTERNAL "" FORCE)
  # 设置配置类型为 Release 和 Debug
  set (OCCTV_SINGLE_GENERATOR OFF)
  # 设置单生成器标志为关闭
else()
  # 如果不是多配置生成器
  set (OCCTV_SINGLE_GENERATOR ON)
  # 设置单生成器标志为开启
  if (NOT CMAKE_BUILD_TYPE)
    # Set Release as a default build type
    # 将 Release 设置为默认构建类型
    set (CMAKE_BUILD_TYPE Release)
    # 设置默认构建类型为 Release
  endif()
  set (CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE INTERNAL "" FORCE)
  # 设置配置类型
endif()

# Build options
# =============
# 构建选项
# =============
# Option to treat compilation warnings as errors
# 将编译警告视为错误的选项
option (OCCTV_WARNINGS_AS_ERRORS "Fail on compiler warnings" OFF)
# 编译时是否将警告视为错误，关闭状态
# Build documentation option
# 构建文档选项
option (OCCTV_BUILD_DOXYGEN "Build Doxygen documentation" OFF)
# 是否构建 Doxygen 文档，关闭状态
include (CMakeDependentOption)
# 包含 CMake 依赖选项模块
# Build with/without draco option
# 使用/不使用 draco 库的选项
option (OCCTV_USE_DRACO "Use Draco library" ON)
# All VTK file processing is implemented in this repository. The historical
# OCCTV_USE_VTK cache setting is deliberately unused; no external SDK is found.

# 是否使用 Draco 库，开启状态

# Option to build typescript web viewer
# 构建 TypeScript Web 查看器选项
option (OCCTV_BUILD_WEB_VIEWER "Build ts web viewer" ON)
# 是否构建 TypeScript Web 查看器，开启状态

# Option to enable/disable STEP/IGES readers
# 启用/禁用 STEP/IGES 读取器
option (OCCTV_USE_STEP "Enable STEP reader support" OFF)
option (OCCTV_USE_IGES "Enable IGES reader support" OFF)

# Option to enable/disable MedFile option
# 启用/禁用 MedFile 选项
cmake_dependent_option (OCCTV_USE_MEDFILE "Use MEDFile library for MED parser" ON "NOT EMSCRIPTEN" OFF)
# 是否使用 MEDFile 库解析 MED 文件，在非 EMSCRIPTEN 环境下默认开启
# Option to enable/disable DRAW plugins
# 启用/禁用 DRAW 插件选项
cmake_dependent_option (OCCTV_BUILD_DRAW "Build DRAW Harness plugin" ON "NOT EMSCRIPTEN" OFF)
# 是否构建 DRAW Harness 插件，在非 EMSCRIPTEN 环境下默认开启
# Option to enable/disable mesh exchange plugin
# 启用/禁用网格交换插件选项
cmake_dependent_option (OCCTV_BUILD_MESH_EXCHANGER "Build mesh exchanger plugin" OFF "NOT EMSCRIPTEN;OCCTV_USE_MEDFILE" OFF)
# 是否构建网格交换插件，在非 EMSCRIPTEN 且使用 MEDFILE 的环境下默认关闭
# Option to enable/disable MxSim mesh data converter
# 启用/禁用 MxSim 网格数据转换器选项
cmake_dependent_option (OCCTV_BUILD_MXSIM_DRIVER "Build mesh MxSim driver" OFF "OCCTV_BUILD_MESH_EXCHANGER" OFF)
# 是否构建 MxSim 网格驱动程序，当构建网格交换插件时默认关闭
# Option to enable/disable OpenFOAM mesh data converter
# 启用/禁用 OpenFOAM 网格数据转换器选项
cmake_dependent_option (OCCTV_BUILD_OPENFOAM_DRIVER "Build mesh OpenFOAM driver" OFF "OCCTV_BUILD_MESH_EXCHANGER" OFF)
# 是否构建 OpenFOAM 网格驱动程序，当构建网格交换插件时默认关闭
# Use conan C++ Package Manager option
# 使用 conan C++ 包管理器选项
cmake_dependent_option (OCCTV_USE_CONAN "Use C++ conan package manager to obtain third-party dependencies" OFF
                        "NOT EMSCRIPTEN" OFF)
# 是否使用 conan 包管理器获取第三方依赖，在非 EMSCRIPTEN 环境下默认关闭
# Install third-party dependencies option
# 安装第三方依赖选项
cmake_dependent_option (OCCTV_INSTALL_DEPENDENCIES "Install 3rd-party prerequisites" ON "NOT EMSCRIPTEN" OFF)
# 是否安装第三方依赖，在非 EMSCRIPTEN 环境下默认开启
# Option to enable or disable generation of vscode workspace file
# 启用或禁用生成 VS Code 工作区文件选项
# Note: it works currently for single-configuration generators only # (requires CMAKE_BUILD_TYPE
# and CMAKE_*_OUTPUT_DIRECTORY vars)
# 注意：目前仅适用于单配置生成器（需要 CMAKE_BUILD_TYPE 和 CMAKE_*_OUTPUT_DIRECTORY 变量）
cmake_dependent_option (OCCTV_USE_VSCODE "Generate VS code workspace file" OFF "OCCTV_SINGLE_GENERATOR" OFF)
# 是否生成 VS Code 工作区文件，当使用单生成器时默认关闭
if (EMSCRIPTEN)
  # 如果是 EMSCRIPTEN 环境
  set (OCCTV_WASM_CONFIGS "wasm32-pthread wasm32" CACHE STRING "WebAssembly configuration list sorted by priority")
  # 设置 WebAssembly 配置列表，按优先级排序
  set (OCCTV_USE_WASM_EXCEPTION OFF CACHE BOOL "Use native WebAssembly exception handling instead of JavaScript-based")
  # 是否使用原生 WebAssembly 异常处理而不是基于 JavaScript 的处理，默认关闭
  set (OCCTV_USE_WASM_MEMORY64  OFF CACHE BOOL "Build 64-bit WebAssembly")
  # 是否构建 64 位 WebAssembly，默认关闭
  set (OCCTV_USE_PTHREADS       OFF CACHE BOOL "Use multithreading for WASM build")
  # WASM 构建是否使用多线程，默认关闭

  set (OCCTV_WEBVIEWER_INSTALL_PREFIX "" CACHE STRING "Paths to install folder for typescript web viewer")
  # 设置 TypeScript Web 查看器的安装文件夹路径
endif()

# Paths to 3rd party dynamic loaded libraries
# ============================================
# 第三方动态加载库路径
# ============================================
set (3RDPARTY_DLL_DIRS "" CACHE STRING "Paths to external DLLs separated by semicolon (FreeImage, FreeType, Tcl, etc.)")
# 设置第三方 DLL 路径，以分号分隔（FreeImage, FreeType, Tcl 等）
if (3RDPARTY_DLL_DIRS)
  # 如果定义了第三方 DLL 路径
  set (_dll_cmake_paths)
  # 初始化 DLL CMake 路径列表
  foreach (3RDPARTY_DLL_PATH ${3RDPARTY_DLL_DIRS})
    # 遍历每个第三方 DLL 路径
    file (TO_CMAKE_PATH ${3RDPARTY_DLL_PATH} 3RDPARTY_DLL_CMAKE_PATH)
    # 将路径转换为 CMake 路径格式
    list (APPEND _dll_cmake_paths ${3RDPARTY_DLL_CMAKE_PATH})
    # 将转换后的路径添加到列表中
  endforeach()
  set (3RDPARTY_DLL_DIRS "${_dll_cmake_paths}" CACHE STRING
       "Paths to external DLLs separated by semicolon (FreeImage, FreeType, etc.)" FORCE)
  # 强制设置第三方 DLL 路径列表
endif()

# Set platform-specific settings
# ==============================
# 设置平台特定配置
# ==============================
include (platform_setup)
# 包含平台设置脚本

# Include CMake module with common macros and functions
# =====================================================
# 包含带有通用宏和函数的 CMake 模块
# =====================================================
include (common_macros)
# 包含通用宏模块

# Generate "OCCTViewerConfig.h"
# ==============================
# 生成 "OCCTViewerConfig.h" 文件
# ==============================
set (OCCTViewer_GXX_DIR ${CMAKE_BINARY_DIR}/source/gxx)
# 设置 OCCTViewer GXX 目录路径
message ("Configuring ${OCCTViewer_PACKAGE} version: '${OCCTViewer_VER_MAJOR}.${OCCTViewer_VER_MINOR}.${OCCTViewer_VER_PATCH}'")
# 输出配置信息
configure_file (${CMAKE_SOURCE_DIR}/adm/cmake/OCCTViewerConfig.h.in
                ${OCCTViewer_GXX_DIR}/OCCTViewerConfig.h)
# 配置 OCCTViewerConfig.h 文件

# Build with conan
# ================
# 使用 conan 构建
# ================
if (OCCTV_USE_CONAN)
  # 如果使用 conan
  if (OCCTV_OCCT_CONAN_VERSION)
    # 如果定义了 OCCT conan 版本
    set (OCCT_PACKAGE_VERSION ${OCCTV_OCCT_CONAN_VERSION})
    # 设置 OCCT 包版本
  else()
    # 如果未定义 OCCT conan 版本
    execute_process (COMMAND ${GIT_EXECUTABLE} rev-parse HEAD:./occt.git
                     OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE _occt_sm_git_hash
                     RESULT_VARIABLE _git_cmd_return_code
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty)
    # 获取 occt.git 子模块的 Git 哈希值
    if (${_git_cmd_return_code} EQUAL 0)
      # 如果命令执行成功
      set (OCCT_PACKAGE_VERSION ${_occt_sm_git_hash})
      # 设置 OCCT 包版本
    else()
      message(FATAL_ERROR "Can't detect OpenCASCADE version from 3rdparty/occt.git submodule")
      # 如果命令执行失败，输出错误信息
    endif()
  endif()
  if (OCCTV_SMESH_CONAN_VERSION)
    # 如果定义了 SMESH conan 版本
    set (SMESH_PACKAGE_VERSION ${OCCTV_SMESH_CONAN_VERSION})
    # 设置 SMESH 包版本
  else()
    execute_process (COMMAND ${GIT_EXECUTABLE} rev-parse HEAD:./smesh.git
                     OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE _smesh_sm_git_hash
                     RESULT_VARIABLE _git_cmd_return_code
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty)
    # 获取 smesh.git 子模块的 Git 哈希值
    if (${_git_cmd_return_code} EQUAL 0)
      # 如果命令执行成功
      set (SMESH_PACKAGE_VERSION ${_smesh_sm_git_hash})
      # 设置 SMESH 包版本
    else()
      message(FATAL_ERROR "Can't detect SMESH version from 3rdparty/smesh.git submodule")
      # 如果命令执行失败，输出错误信息
    endif()
  endif()

  configure_file(adm/templates/conan/conanfile_linux_gcc.txt.in ${CMAKE_CURRENT_BINARY_DIR}/conanfile_linux_gcc.txt)
  # 配置 conanfile_linux_gcc.txt 文件
  simake_conan_install (CONANFILE ${CMAKE_CURRENT_BINARY_DIR}/conanfile_linux_gcc.txt)
  # 安装 conan 依赖
  set (OpenCASCADE_ROOT_DIR ${CONAN_OPENCASCADE_ROOT})
  # 设置 OpenCASCADE 根目录
endif()

# Find OpenCASCADE
# ================
# 查找 OpenCASCADE
# ================
include (opencascade)
# 包含 OpenCASCADE 查找脚本

# Find RapidJSON
# ==============
# 查找 RapidJSON
# ==============
include (rapidjson)
# 包含 RapidJSON 查找脚本
if (RapidJSON_FOUND)
  # 如果找到 RapidJSON
  add_definitions (-DHAVE_RAPIDJSON)
  # 添加 HAVE_RAPIDJSON 定义
  # Workaround to disable using std::iterator which is deprecated in c++17
  # 解决方法：禁用在 C++17 中已弃用的 std::iterator
  # Fixed in master version of RapidJSON
  # 在 RapidJSON 主版本中已修复
  add_definitions (-DRAPIDJSON_NOMEMBERITERATORCLASS)
  # 添加 RAPIDJSON_NOMEMBERITERATORCLASS 定义
  include_directories ("${RAPIDJSON_INCLUDE_DIRS}")
  # 包含 RapidJSON 头文件目录
endif()

# Find HDF5
# =========
# 查找 HDF5
# =========
if (OCCTV_USE_MEDFILE)
  # 如果使用 MEDFILE
  include (medfile)
  # 包含 MedFile 查找脚本
  if (HDF5_FOUND)
    # 如果找到 HDF5
    add_definitions (-DHAVE_MEDFILE)
    # 添加 HAVE_MEDFILE 定义
  endif()
endif()

# Find Draco
# ==========
# 查找 Draco
# ==========
if (OCCTV_USE_DRACO)
  # 如果使用 Draco
  include (draco)
  # 包含 Draco 查找脚本
  if (draco_FOUND)
    # 如果找到 Draco
    add_definitions (-DOCCTV_HAVE_DRACO)
    # 添加 OCCTV_HAVE_DRACO 定义
  endif()
endif()

if (EMSCRIPTEN)
  # 如果是 EMSCRIPTEN 环境
  # Find freetype
  # =============
  # 查找 freetype
  # =============
  include (freetype)
  # 包含 freetype 查找脚本
else()
  # 如果不是 EMSCRIPTEN 环境
  # Desktop dependencies:
  # 桌面依赖:
  # Prefer vendor OpenGL library under Linux
  # 在 Linux 下优先使用供应商提供的 OpenGL 库
  if (NOT WIN32)
    cmake_policy (SET CMP0072 NEW)
    # 设置 CMake 策略 CMP0072 为 NEW
  endif()
  # Find OpenGL library
  # ===================
  # 查找 OpenGL 库
  # ===================
  find_package (OpenGL REQUIRED)
  # 查找 OpenGL 包（必需）
endif()

# Mesh exchanger dependencies
# 网格交换器依赖
if (OCCTV_BUILD_MESH_EXCHANGER)
  # 如果构建网格交换器
  # SMESH
  # =====
  include(smesh)
  # 包含 SMESH 查找脚本
endif()

# solution search folders
# 解决方案搜索文件夹
include_directories (${PROJECT_SOURCE_DIR}/source)
# 包含源代码目录
include_directories (${PROJECT_SOURCE_DIR}/source/SimMesh)
# 包含 SimMesh 源代码目录
if (OCCTV_BUILD_DRAW)
  # 如果构建 DRAW
  include_directories (${PROJECT_SOURCE_DIR}/source-draw)
  # 包含 source-draw 目录
  include_directories (${PROJECT_SOURCE_DIR}/source-draw/SimMeshDraw)
  # 包含 SimMeshDraw 目录
endif()
include_directories (${PROJECT_SOURCE_DIR}/resources)
# 包含资源目录
include_directories (${PROJECT_SOURCE_DIR}/source)
# 再次包含源代码目录
include_directories (${OCCTViewer_GXX_DIR})
# 包含 OCCTViewer GXX 目录

# Add sub-projects the project
# ============================
# 添加子项目到项目中
# ============================
set_property (GLOBAL PROPERTY USE_FOLDERS ON)
# 设置全局属性 USE_FOLDERS 为 ON

# add_subdirectory (dox)
add_subdirectory (source/RWBase)
add_subdirectory (source/ViewBase)
# 添加 SCPrsBase 子目录

add_subdirectory (source/RWShape)
add_subdirectory (source/RWShapeConv)
add_subdirectory (source/ViewShape)
add_subdirectory (source/ViewDim)

add_subdirectory (source/RWCae)
add_subdirectory (source/RWMed)
add_subdirectory (source/RWVtk)
add_subdirectory (source/RWMeshConv)
if (OCCTV_BUILD_MESH_EXCHANGER)
  # 如果构建网格交换器
  add_subdirectory (source/RWMeshExchanger)
  # 添加 SCRWMeshExchanger 子目录
  if (OCCTV_BUILD_OPENFOAM_DRIVER)
    # 如果构建 OpenFOAM 驱动
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/RWMeshExchanger/DriverFOAM/CMakeLists.txt")
      add_subdirectory (source/RWMeshExchanger/DriverFOAM)
    else()
      message (WARNING "OCCTV_BUILD_OPENFOAM_DRIVER is ON but DriverFOAM sources are missing; skipping.")
    endif()
    # 添加 DriverFOAM 子目录
  endif()
endif()

add_subdirectory (source/ViewCae)
# 添加 ViewCaeGeom 子目录
add_subdirectory (source/ViewCaeGeom)

add_subdirectory (source/SimMesh)
# 添加 SimMesh 子目录
add_subdirectory (source/LodCae)
# 添加 SCLodCae 子目录

if ((EMSCRIPTEN AND OCCTV_BUILD_WEB_VIEWER) OR OCCTV_BUILD_DRAW)
  # 如果是 EMSCRIPTEN 环境且构建 Web 查看器，或者构建 DRAW
  add_subdirectory (source/Viewer)
  # 添加 SCwebviewer 子目录
endif()
if (OCCTV_BUILD_DRAW)
  # 如果构建 DRAW
  add_subdirectory (source-draw/DrawBase)
  # 添加 DrawBase 子目录
  add_subdirectory (source-draw/RWCaeTest)
  # 添加 SCRWCaeTest 子目录
  add_subdirectory (source-draw/RWShapeTest)
  # 添加 RWShapeTest 子目录
  add_subdirectory (source-draw/ViewBaseTest)
  # 添加 ViewBaseTest 子目录
  add_subdirectory (source-draw/ViewCaeTest)
  # 添加 ViewCaeTest 子目录
  add_subdirectory (source-draw/ViewShapeTest)
  # 添加 ViewShapeTest 子目录
  add_subdirectory (source-draw/ViewDraw)
  # 添加 ViewDraw 子目录
  add_subdirectory (source-draw/LodCaeTest)
  # 添加 LodCaeTest 子目录
  add_subdirectory (source-draw/SimMeshDraw)
  # 添加 SimMeshDraw 子目录
  # make DRAW plugin as default executable in VS solution
  # 将 DRAW 插件设置为 VS 解决方案中的默认可执行文件
  set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "ViewDraw")
  # 设置 VS 启动项目为 ViewDraw
endif()
if (NOT EMSCRIPTEN)
  # 如果不是 EMSCRIPTEN 环境
  add_subdirectory (source/FileExchange)
  # 添加 FileExchange 子目录
endif()
if (OCCTV_BUILD_WEB_VIEWER)
  # 如果构建 Web 查看器
  add_subdirectory (source-ts/Viewer-ts)
  # 添加 SCwebviewer-ts 子目录
endif()

# Workspace files for VSCode
# ==========================
# VSCode 的工作区文件
# ==========================
if (OCCTV_USE_VSCODE)
  # 如果使用 VSCode
  include (vscode)
  # 包含 VSCode 配置脚本
endif()

if (EMSCRIPTEN)
  # 如果是 EMSCRIPTEN 环境
  if (OCCTV_BUILD_WEB_VIEWER)  
    # 如果构建 Web 查看器
    install (FILES     "${CMAKE_CURRENT_SOURCE_DIR}/adm/scripts/server.py"
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/scripts")
    # 安装 server.py 脚本文件
    install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/samples"
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}"
            PATTERN ".angular" EXCLUDE
            PATTERN "node_modules" EXCLUDE)
    # 安装 samples 目录，排除 .angular 和 node_modules
    install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/images"
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}")
    # 安装 images 资源目录
    install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts"
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}")
    # 安装 fonts 资源目录

    set (_gltf models/gltf)
    # 设置 gltf 模型路径
    install (FILES
            resources/${_gltf}/as1-oc-214.glb
            resources/${_gltf}/ball.glb
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/${_gltf}")
    # 安装 gltf 模型文件
    set (_gmsh models/gmsh)
    # 设置 gmsh 模型路径
    install (FILES
            resources/${_gmsh}/buckling22.pos
            resources/${_gmsh}/LE10disp22.pos
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/${_gmsh}")
    # 安装 gmsh 模型文件
    set (_brep models/brep)
    # 设置 brep 模型路径
    install (FILES
            resources/${_brep}/as1-oc-214.bbrep
            resources/${_brep}/fuse.bbrep
            resources/${_brep}/face.brep
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/${_brep}")
    # 安装 brep 模型文件
    set (_stl models/stl)
    # 设置 stl 模型路径
    install (FILES
            resources/${_stl}/shape.stl
            DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/${_stl}")
    # 安装 stl 模型文件
  endif()
else()
  # 如果不是 EMSCRIPTEN 环境
  if (OCCTV_BUILD_DRAW)
    # 如果构建 DRAW
    install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/models"
            DESTINATION "${CMAKE_INSTALL_PREFIX}"
            PATTERN "med" EXCLUDE
            PATTERN "vtk" EXCLUDE)
    # 安装 models 资源目录，排除 med 和 vtk 文件
  endif()
endif()

# Generate and install CMake configuration files
# and runtime dependencies
# ==============================================
# 生成并安装 CMake 配置文件和运行时依赖
# ==============================================
include (CMakePackageConfigHelpers)
# 包含 CMake 包配置助手
configure_package_config_file (
  ${CMAKE_SOURCE_DIR}/adm/cmake/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${OCCTV_INSTALL_CMAKE_CONFIG}
  PATH_VARS OCCTV_INSTALL_CMAKE_CONFIG CMAKE_INSTALL_PREFIX)
# 配置包配置文件

write_basic_package_version_file (
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${OCCTViewer_VER_MAJOR}.${OCCTViewer_VER_MINOR}.${OCCTViewer_VER_PATCH}
  COMPATIBILITY AnyNewerVersion)
# 编写基本包版本文件

install (FILES
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${OCCTV_INSTALL_CMAKE_CONFIG})
# 安装配置文件
install (EXPORT ${OCCTViewer_PACKAGE}Targets
         FILE ${PROJECT_NAME}Targets.cmake
         DESTINATION ${OCCTV_INSTALL_CMAKE_CONFIG})

# 安装导出目标文件

if (OCCTV_INSTALL_DEPENDENCIES)
  # 如果安装依赖
  # DRAWEXE Resources
  # DRAWEXE 资源
  if (OCCTV_BUILD_DRAW)
    # 如果构建 DRAW
    install (DIRECTORY "${OpenCASCADE_RESOURCE_DIR}/DrawResources" DESTINATION "${CMAKE_INSTALL_PREFIX}/source")
    # 安装 DrawResources 目录
    # Do not copy tcl/tk libraries due a problem with native tcl/tk scripts
    # 不复制 tcl/tk 库，因为原生 tcl/tk 脚本存在问题
    set (_exclude_regexp "(tcl[0-9]+[.]|tk[0-9]+[.])")
    # 设置排除正则表达式
    set (_additional_dependencies DRAWEXE)
    # TKOpenGlesTest is only produced by an OCCT build with GLES2 enabled.
    if (OpenCASCADE_WITH_GLES2)
      list (APPEND _additional_dependencies TKOpenGlesTest)
    endif()
    # 设置额外依赖
  endif()
  if (OCCTV_USE_DRACO)
    # 如果使用 DRACO
    list(APPEND _additional_dependencies draco::draco)
    # 添加 draco 依赖到列表
  endif()
  # Dependencies
  # 依赖项
  if(WIN32)
    # 如果是 Windows 系统
    simake_install_dependencies (${CMAKE_SOURCE_DIR}
                                  DEPENDENCIES_TOOL file
                                  DESTINATION OCCTV_INSTALL_DLL
                                  EXECUTABLES_DESTINATION OCCTV_INSTALL_BIN
                                  ADDITIONAL_DEPENDENCIES ${_additional_dependencies}
                                  EXCLUDE_LIBRARIES_REGEXP ${_exclude_regexp})
  else()
    # 如果不是 Windows 系统
    simake_install_dependencies (${CMAKE_SOURCE_DIR}
                                  DESTINATION OCCTV_INSTALL_LIB
                                  EXECUTABLES_DESTINATION OCCTV_INSTALL_BIN
                                  ADDITIONAL_DEPENDENCIES ${_additional_dependencies}
                                  EXCLUDE_LIBRARIES_REGEXP ${_exclude_regexp})

  endif()
endif()

option (OCCTV_BUILD_TESTS "Build native VTK and cut-view tests" OFF)
if (OCCTV_BUILD_TESTS AND NOT EMSCRIPTEN)
  enable_testing()
  add_subdirectory (test/native)
endif()
