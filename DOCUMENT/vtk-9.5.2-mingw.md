# VTK 9.5.2 MinGW64 SDK

Cut View 的原生 VTK 路径使用以下独立 SDK。已有 `D:/workspace/VTK/vtk-src`
是含本地修改的 VTK 7.1.1，本次未改动。

| 项目 | 当前配置 |
| --- | --- |
| 官方源码 | `https://gitlab.kitware.com/vtk/vtk.git`，tag `v9.5.2` |
| Commit | `7c0494a68bff379d32d6b1fbaa3d10d27a73af54` |
| 源码 | `D:/workspace/VTK/vtk-9.5.2` |
| 构建 | `D:/workspace/VTK/build-9.5.2-mingw64` |
| 安装 | `D:/workspace/OpenCASCADE/3rdparty/mingw64/vtk-9.5.2-mingw64` |
| 编译器 | `C:/mingw64/bin/g++.exe`，GCC 14.2.0，64-bit |
| 配置 | Ninja，Release，共享库；原生 C++，关闭 wrapping/tests/examples/docs/remote modules |
| 请求模块 | CommonCore、CommonDataModel、IOXML、IOLegacy；依赖自动启用 |

## DLL 模板导出补丁

[vtk-9.5.2-mingw.patch](vtk-9.5.2-mingw.patch) 修复 Debug/O0 消费者链接时缺失的
`vtkAOSDataArrayTemplate` value/typed-tuple 方法。需要同时为这些内联方法显式标记
`VTKCOMMONCORE_EXPORT`（仅 MinGW 模板实例化端），并在 13 个标量类型实例化源文件中保留 inline dllexport。
只添加 `-fkeep-inline-dllexport` 不能解决本次缺失符号。

在未应用补丁的相同 tag 上执行以下命令。先检查再应用；已应用的源码不要重复应用。

```powershell
git -C D:/workspace/VTK/vtk-9.5.2 apply --check D:/workspace/OpenCASCADE/OCCT_Viewer/DOCUMENT/vtk-9.5.2-mingw.patch
git -C D:/workspace/VTK/vtk-9.5.2 apply D:/workspace/OpenCASCADE/OCCT_Viewer/DOCUMENT/vtk-9.5.2-mingw.patch
cmake -S D:/workspace/VTK/vtk-9.5.2 -B D:/workspace/VTK/build-9.5.2-mingw64 -G Ninja `
  -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe `
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON `
  -DCMAKE_INSTALL_PREFIX=D:/workspace/OpenCASCADE/3rdparty/mingw64/vtk-9.5.2-mingw64 `
  -DVTK_BUILD_ALL_MODULES=OFF -DVTK_ENABLE_WRAPPING=OFF -DVTK_BUILD_TESTING=OFF `
  -DVTK_BUILD_EXAMPLES=OFF -DVTK_BUILD_DOCUMENTATION=OFF -DVTK_BUILD_SPHINX_DOCUMENTATION=OFF `
  -DVTK_ENABLE_REMOTE_MODULES=OFF `
  -DVTK_GROUP_ENABLE_StandAlone=DONT_WANT -DVTK_GROUP_ENABLE_Rendering=DONT_WANT `
  -DVTK_GROUP_ENABLE_Imaging=DONT_WANT -DVTK_GROUP_ENABLE_MPI=DONT_WANT `
  -DVTK_GROUP_ENABLE_Qt=DONT_WANT -DVTK_GROUP_ENABLE_Views=DONT_WANT `
  -DVTK_MODULE_ENABLE_VTK_CommonCore=YES -DVTK_MODULE_ENABLE_VTK_CommonDataModel=YES `
  -DVTK_MODULE_ENABLE_VTK_IOXML=YES -DVTK_MODULE_ENABLE_VTK_IOLegacy=YES
cmake --build D:/workspace/VTK/build-9.5.2-mingw64 --parallel 6
cmake --install D:/workspace/VTK/build-9.5.2-mingw64
```

构建日志为 SDK build 下的 `inline-exports-build.log`、`install.log`。
SDK 已完成编译/安装；import library 已核对 double/int 的 GetValue、SetValue 和
SetTypedTuple 导出，Debug/O0 消费者编译及本项目几何库链接已通过。
消费项目使用 `lib/cmake/vtk-9.5`，CTest 自动为 Windows 配置项目、OCCT、VTK、
MinGW runtime 和 zlib 的 DLL 搜索路径。

## 官方测试数据

`D:/workspace/VTK/test-data-9.5.2` 包含按该 tag 的 `Testing/Data/*.sha512`
校验的六个原始文件：`quadraticTetra01.vtu`、`onePolyhedron.vtu`、
`concavePolyhedron.vtu`、`Disc_BiQuadraticQuads_0_0.vtu`、`cube.vtu`、
`nonWatertightPolyhedron.vtu`。历史验收曾将最后一个作为拒绝样例；
去除外部 VTK 时已根据官方 `TestPolyhedron4.cxx` 纠正：该网格实际闭合，
现在要求求交成功，另用真正缺面的多面体验证拒绝路径。
当前项目无需此 SDK；见 [自研 VTK 处理审计](vtk-independent-audit.md)。

数据根据 SHA512 从 Kitware Data API 或 VTK ExternalData 下载，未修改。
历史 Linux 环境另外使用的 `nonplanar_quad.vtk` 和 `BoxIncorrectConnectivity.vtp`
未在当前官方 manifest 中找到，未伪造替代；当前 Windows 套件注册 32 项，
不能称为旧 34 项套件全部复现。
