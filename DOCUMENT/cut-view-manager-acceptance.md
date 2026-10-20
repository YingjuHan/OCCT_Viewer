# Cut View Manager：A01–A10 工程验收记录

## 1. 交付范围与版本

本记录对应 `develop` 的 A01–A10 开发，验收日期为 2026-10-21。
功能包 A01–A09 已分别提交，A10 包含最终回归修正、百万门禁、性能实测和文档归档。
当前完整会话为 session 4，definitions 为 13，旧 session 1/2/3 继续读取。
详细需求见 [对标台账](cut-view-manager-abaqus-parity.md)，迁移以 [无 OCCT 手册](cut-view-manager-visit-porting.md) 为准。

| 工作包 | 功能与提交 |
| --- | --- |
| A01 | 三部分显示选项、独立重命名、显示覆盖生命周期；`b85b89b` |
| A02 | 完整定义 frame、局部绝对运动、灵敏度及持久化；`5fe3eda` |
| A03 | 参考 FE 映射、材料曲面、当前误差采样/法向/回滚；`ae9edda`、`2f6ff90`、`314aa50` |
| A04 | 每 cut 自由体、域、求和点、分量 frame 和受控向量；`17e2f05` |
| A05 | 独立等值面、派生量/averaging 快照；`3a1d63f` |
| A06 | 有所有权的切片集合与事务物理扫描；`3c8c73c` |
| A07 | plot 动态继承、边/面选项、显式多 cut 策略；`4c66b5d` |
| A08 | 自动来源快照、保护、XFEM/优化语义；`7ccf403` |
| A09 | session 4、公共物理模板和全部自有显示的视口隔离；`c88b0f6` |

## 2. 环境与数据身份

- Windows、MinGW-w64 GCC 14.2.0，CMake Release；独立构建目录 `build/mingw-deformation`。
- 定制 OCCT 7.7.0 SDK，DRAW 路径为 `D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe`。
- Intel Core i7-1165G7，4 核/8 线程，系统可见内存 51,245,015,040 字节。
- 本机存在 MX450、Iris Xe 和虚拟显示适配器；实际使用的 renderer 以每场景的 `*-renderer.txt` 为准，不能按显卡清单推断。
- 所有 GUI 回归与百万门禁顺序执行；性能不挂 GDB，不同时运行其他测试。

大数据输入为 `build/large-vtk-deformed/hex_150.vtu`：3,375,000 六面体，3,442,951 节点，303,366,033 字节。
SHA-256：`83180755c60eac9064e2f2c47441b2e39ed9626e79ec2723ab36abb02da9c551`。
它由 `test/generate-large-vtk.py --grids 150 --formats vtk vtu --deformation` 生成，含 temperature 和 displacement。
场值取原 FE 插值，变形后不将 temperature 重新解释为空间坐标函数。
规则网格用于可复现性能门禁；它不能代表目标工具的所有不规则/多 domain 生产网格。

## 3. 构建与完整回归

最终功能构建记录为 `build/a09-scope-fix-build.log`；A10 另为基准命令增加可配置变形倍率，完整重建通过（`build/a10-build.log`），未修改产品 C++ ABI/求交算法。
开启 `OCCTV_BUILD_TESTS=ON`、`OCCTV_TEST_DRAW_VISUAL=ON`，运行全部 84 项测试。

首轮 `build/a10-all-tests.log` 为 **81/84**，两项旧断言失败、一项截图回归超时。
修正后 `build/a10-corrections-tests.log` 的 **5/5** 通过，包括三项原失败及追踪图像/基准 smoke；薄板耗时 **485.65 秒**，完成全部 384 张截图比较。
基准参数编译后再运行 `build/a10-benchmark-tests.log`，**1/1** 通过。
因此 84 个注册用例都有通过证据；没有宣称首轮无失败，也没有跳过超时用例。

本轮完整回归修正了以下兼容性问题：

1. `cut_view_trace.tcl` 仍硬编码 definitions 7，已更新为 13；原 int64 迹绑定、物理来源独立及失败原子性断言保留。
2. 旧 Physics 面板默认非零裂纹阈值，与 A08 的零等值面契约冲突。默认改为 0；用例先验证非零阈值拒绝，再验证 `sign=-1` 不将完整 Below 主体换成 Above，密度仍按独立阈值测试。
3. 文件全帧基准不再要求无关的 `CUTVIEW_BENCHMARK_GRID` 环境变量；文件模式会短路读取该变量。
   基准命令新增可选末尾 `deformationScale`（缺省仍为 0.005）及结果中的 `deformation_scale`，Tcl 通过 `CUTVIEW_BENCHMARK_DEFORMATION_SCALE` 配置。
4. 正弦薄板的 384 张截图超过原 420 秒时限，保持全部格式/法向/mask/backend 组合，将正确性回归时限改为 900 秒并输出逐平面进度；没有改变性能门槛。

覆盖包括自研 reader、单元/高阶 FE、求交/恢复/界面/物理、材料构形、session、timeline、Manager 控件、实际鼠标路由、GPU/CPU 对照、正弦薄板及双视口图像。
小型解析/图像夹具用于正确性；默认 smoke 基准不充当百万性能证据。
人工抽查水平薄板切面的三条分离带及等 z 单色结果；分离带间空白是几何断开，不能用凸包填满作为“消缝”。

## 4. 337.5 万单元门禁

在相同最终 DLL 下，分别以新 DRAW 进程顺序运行以下脚本。记录在 `build/a10-large/<case>/run.log`，不覆盖各包历史日志。
source/result 是共享账本的逻辑保留字节，不等于进程 RSS，也不是逐帧上传量。
源/结果限额一直为 4/16 GiB，没有通过提高限额取得通过。

| case | 工作与硬断言 | 结果与逻辑预算 |
| --- | --- | --- |
| placement | 三显示模式 × mask 2/3/7，绝对旋转/平移，暖主体上传/重建为 0 | PASS；source/result `1,460,965,248 / 597,683,072` 字节 |
| material | 参考求交复用、三模式、多倍率，只更新材料截面点 | PASS；referenceBuilds=1，每次 22,801 点；source/result `1,461,030,784 / 445,418,560` 字节 |
| free_body | 原 FE 积分、形心求和、每 cut 有限向量、开关清理 | PASS；45,000 样本/2 个覆盖；source/result `1,460,965,248 / 1,093,811,072` 字节 |
| iso_binding | 两个温度等值面阈值、云图改动不改变绑定 | PASS；两次均 44,999 面；source/result `1,460,965,248 / 737,329,152` 字节 |
| slices | 三片创建、显示、逐片扫描、移除，现场不污染 | PASS；source/result `1,460,965,248 / 921,862,144` 字节 |
| plot_options | 动态继承/单色、三模式 × mask 2/3/7，暖主体复用 | PASS；source/result `1,460,965,248 / 597,683,072` 字节 |
| automatic | 优化 provider 两阈值整批替换及撤销 | PASS；source/result `1,460,965,248 / 460,855,296` 字节 |
| scope | 共享源的双视口、A 变形/B 不变形、A 暖移动不改 B | PASS；每视口 source `1,460,965,248`；A/B result `597,683,072 / 561,011,072` 字节 |

free_body/slices 的大数据张量由 displacement 的显式组件映射合成，只验证吞吐与生命周期，不是物理应力验证。
物理正确性由小型解析 FE 夹具独立验证。
命令批次包含求值、报告或 UI 工作，它们的毫秒数不可换算成鼠标 FPS。

## 5. 完整渲染帧测量

`cut_view_benchmark.tcl` 的文件模式，337.5 万单元、mask=3、auto backend，各场景 20 帧；deformed 明确使用归一化倍率 0.1，另两个场景无变形。
倍率与 DRAW `deform` 一样经 `MeshCutViewManager::SetDeformation` 按模型尺寸/最大位移模长换算；不是直接将原始位移乘以 0.1，公式见 [变形指南](cut-view-manager-deformation.md)。
native Refresh、实际 `V3d_View::Redraw` 与 `glFinish` 都在逐帧计时内；导入/首次准备单列。
每帧检查仍为 GPU 平面路径、主体重建次数不增加、主体提交字节为 0，并验证原 FE 字段来源。
截面数值检查和首末截图在计时区间外；FPS 为 `1000*frames/sum(total_ms)`，不是空闲视口计数器。
这是同步 API 驱动的完整帧吞吐；实际鼠标路由由交互回归另测，不能把 20 帧短样本当作生产长时间拖动的延迟分布。

实际 renderer 为 **Intel Iris Xe Graphics**，OpenGL `4.6.0 - Build 32.0.101.7088`，不是本机另一张 MX450。

| 场景 | p50 ms | p95 ms | FPS | import ms | setup ms（含 import） | target |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| crossing | 59.900 | 106.318 | 14.911 | 3,206.236 | 10,253.868 | 0 |
| rotating | 122.284 | 137.278 | 8.055 | 3,668.031 | 10,096.603 | 0 |
| deformed，归一化倍率 0.1 | 151.968 | 185.005 | 6.684 | 4,472.574 | 13,135.642 | 0 |

三个场景均完成正确性/后端硬断言，但**均未达到性能组合目标**；本轮不能声明稳定 15/30 FPS。
导入包含 OS 文件缓存影响，未建立受控冷缓存；setup 包含 import，不可再次相加。
所有 60 帧的 `body_builds=1`、`submitted_body_bytes=0`，各场景逐帧提交的截面数据均值约 4.32–4.96 MB。

CSV 分阶段均值如下（ms）：

| 场景 | intersection | slice | trace | BVH | node prepare | cell cut | merge | FE | primitives | redraw+glFinish |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| crossing | 12.362 | 4.038 | 8.325 | 0.311 | 0.000 | 0.707 | 2.134 | 3.314 | 6.713 | 39.389 |
| rotating | 69.498 | 37.407 | 32.091 | 2.654 | 0.000 | 6.740 | 21.425 | 4.970 | 10.556 | 39.070 |
| deformed | 83.927 | 55.569 | 28.358 | 2.754 | 21.847 | 3.240 | 19.697 | 4.754 | 12.213 | 39.905 |

这些均值含缓存命中帧；intersection 包含 slice/trace，slice 内又包含 BVH、节点、单元求交和 merge 等阶段。
由本次数据可判断：继续只优化 BVH 难以解决总帧耗时，旋转/变形的合并与 trace、变形节点准备及渲染等待应分别剖析。
渲染等待需要进一步区分驱动/同步/实际 GPU 工作，当前不能归因为某个显卡驱动缺陷。
后续性能工作还须覆盖云图/线框/组合显示、长时实际鼠标拖动及不规则生产网格；本轮只完成上述可复现短样本。

输出为 `build/a10-full-frame-large/{crossing,rotating,deformed}.csv`、`*-import.txt`、`*-renderer.txt` 和首末截图。
CSV 的各阶段存在包含关系，不能把所有列直接相加；完整帧以 `total_ms` 为准。
本轮短样本用于复现，不构成长时稳定性或跨硬件性能承诺。
脚本目标为 FPS≥15 且 p95≤67 ms，两项都满足才 `target=1`；它也不是稳定 30 FPS 的承诺。

## 6. 复现命令

Linux 的依赖、环境、Release/Debug 构建、完整回归、百万脚本循环及 GDB 命令均在 [Linux 操作指南第 8 节](cut-view-manager-linux-run-test.md)。
本轮仅在 Windows 实跑；Linux 命令经过配置/脚本核对，尚无 Linux/CentOS 执行证据。

Windows PowerShell 先完整设置运行环境（每行单独执行）：

```powershell
$env:PATH = 'D:\workspace\OpenCASCADE\OCCT_Viewer\build\mingw-deformation\bin;D:\workspace\OpenCASCADE\OCCT_private\install\mingw\win64\gcc\bin;C:\mingw64\bin;D:\workspace\OpenCASCADE\3rdparty\mingw64\zlib-1.3.1-mingw-64\bin;' + $env:PATH
$env:CSF_OCCTResourcePath = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/src'
$env:CSF_ShadersDirectory = "$env:CSF_OCCTResourcePath/Shaders"
$env:DRAWHOME = "$env:CSF_OCCTResourcePath/DrawResources"
$env:DRAWDEFAULT = "$env:DRAWHOME/DrawDefault"
$env:CSF_DrawPluginDefaults = $env:DRAWHOME
$env:OCCTV_DRAW_RUNTIME = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation/bin/viewdraw-runtime.tcl'
ctest --test-dir D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --output-on-failure --parallel 1
$env:CUTVIEW_BENCHMARK_FILE = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/large-vtk-deformed/hex_150.vtu'
$env:CUTVIEW_BENCHMARK_FRAMES = '20'
$env:CUTVIEW_BENCHMARK_DEFORMATION_SCALE = '0.1'
$env:CUTVIEW_BENCHMARK_MASK = '3'
$env:CUTVIEW_BENCHMARK_SCENARIOS = 'crossing rotating deformed'
$env:CUTVIEW_OUTPUT_DIR = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/a10-full-frame-reproduce'
New-Item -ItemType Directory -Force -Path $env:CUTVIEW_OUTPUT_DIR | Out-Null
& 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe' -i -f 'D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_benchmark.tcl'
```

单独运行其他百万门禁时，设置新的 `CUTVIEW_OUTPUT_DIR`，替换最后一行的脚本为第 4 节对应的 `cut_view_<case>_large.tcl`。
GUI/性能脚本必须顺序执行；出现失败或缺 PASS 标记应先解决，不跳过。

## 7. 保留边界及外部验收

- 没有 Abaqus 可执行环境或相同 ODB 输入，本轮没有双软件数值/画面对照；官方文档对标与项目工程验收须分别理解。
- datum 为显式 Apply 快照；自动关联 datum 仍需后续扩展。材料随动范围为平面，误差/反演检查为采样算法，不是任意阶全域证明。
- FE 物理接口要求明确单位、分量、层/截面和 owner 映射；没有自动 ODB NFORC/SF/SM 适配。高级不连续等值面不支持 retained-body OR。
- 自动 XFEM 为显式 signed-distance 零面协议。有限裂纹前沿、有效裂纹域和富集位移间断需要求解器额外数据；本项目没有 ODB reader。
- 透明度、混合模式及图案边可进入 CPU fallback；壳云图切线仍连续。空间平面 GPU 快路不能套到弯曲材料切面或其他形状。
- 切片集合是空间平面/折线路径快照；多视口共享源数据须保持只读，原地修改由调用者通知全部消费者。
- VisIt/CentOS 7.9 目标尚未移植或实跑；目标完全不使用 OCCT，需要独立 BVH/FE/renderer 适配及实际多 domain/MPI、生产网格验收。

OCCT API inventory 以 `python DOCUMENT/tools/build_cut_view_occt_inventory.py --check` 校验；本轮检查通过。
代码图索引在提交前 full+persistence 刷新，保留 parser coverage 的已知不足，必要结论仍回读源文件。
缓存数据库、生成大网格、测试图像和本机运行日志均不提交。
