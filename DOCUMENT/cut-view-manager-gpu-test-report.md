# Cut View Manager：MX450 / Iris Xe 功能与性能报告

后续性能优化及 85 项双卡全量复测见 [2026-10-24 报告](cut-view-manager-performance-report-2026-10-24.md)。本文保留 2026-10-21 的原始协议和结果，作为历史对照。

## 1. 测试结论与交付范围

测试日期：2026-10-21（测试机本地时间）。被测 C++ 版本为 `78fa2d5bf2370ac05c70ceee0195f4beb63f7ede`，Release，不挂 GDB。本次只增加测试驱动与报告，没有修改产品算法或被测 DLL。

<!-- RESULTS_BEGIN -->
MX450 本轮完整 CTest 为 **84/84 通过**，八组 337.5 万单元门禁为 **8/8 通过**。两卡分别完成三场景 × 三轮 × 60 帧实测，实际 renderer 均已核对。

| 场景 | MX450 FPS / p95 ms | Iris Xe FPS / p95 ms | 本轮 FPS 提升 |
| --- | --- | --- | --- |
| 平移 | 19.88 / 83.79 | 16.06 / 91.41 | 23.8% |
| 旋转并平移 | 10.33 / 126.83 | 10.37 / 119.95 | -0.4% |
| 变形后平移 | 9.57 / 130.87 | 9.00 / 137.48 | 6.3% |

**两卡的三个场景均未达到 FPS≥15 且 p95≤67 ms 的组合目标**。MX450 的改进不能解释为百万网格已经稳定流畅。全部长样本帧的主体构建次数保持 1，逐帧主体提交为 0；剩余耗时仍包括 CPU slice/trace、图元准备和实际渲染等待。
<!-- RESULTS_END -->

这里的“通过”分别表示功能断言通过和性能门槛通过。`CUT_BENCHMARK_PASS` 表示基准完成、数值/后端断言通过；文件基准允许 `target=0`，因此 PASS 本身不能证明性能达标。
旋转场景的 -0.4% 处于本轮波动范围内，不能称为 MX450 有性能收益；平移和变形后剖切的提升分别约 23.8% 和 6.3%。

完整需求与保留边界见 [A01–A10 验收记录](cut-view-manager-acceptance.md) 和 [Abaqus 对标台账](cut-view-manager-abaqus-parity.md)。本报告不把工程测试等同于 Abaqus 实机认证。

## 2. 硬件、软件与输入身份

| 项目 | 配置 |
| --- | --- |
| 系统 | Windows 11 专业工作站版，10.0.26200 |
| CPU / 内存 | i7-1165G7，4 核 8 线程；系统可见内存 51,245,015,040 字节 |
| 独显 | NVIDIA GeForce MX450，2 GiB；OpenGL 4.6.0 NVIDIA 573.91 |
| 核显 | Intel Iris Xe Graphics；OpenGL 4.6.0，Build 32.0.101.7088 |
| 构建 | MinGW GCC 14.2.0、Release、定制 OCCT 7.7.0 SDK；`build/mingw-deformation` |
| 视口 / 电源 | 900×650；Windows“平衡”计划，未强行切换电源计划 |
| 输入 | `build/large-vtk-deformed/hex_150.vtu`；3,375,000 个 hex、3,442,951 个节点；303,366,033 字节 |
| 字段 | 节点 `temperature=x+2y+3z` 与 `displacement`；读取后按原 FE 来源插值 |

输入 SHA-256：`83180755c60eac9064e2f2c47441b2e39ed9626e79ec2723ab36abb02da9c551`。

两卡使用同一 DRAWEXE、模型和 DLL；统计脚本逐项核对哈希：

- `libViewCaeTest.dll`：`5472ce2d45c44a3878a802730019d7df8883371e0a324877454c8e071378468c`。
- `libViewer.dll`：`4b0e6b7adae537a033550ea549a632c7ef79c093129ce0723722582f4372603e`。

GPU 选择由测试脚本临时设置 DRAWEXE 的 Windows 每应用偏好，退出时恢复原值及类型。测试前后 probe 和每个性能场景均核对实际 OpenGL renderer，不能仅凭机器有 MX450 或偏好设置成功判定使用了独显。两卡测试顺序执行，不并行争用 GPU。

## 3. 功能回归与百万单元门禁

<!-- FUNCTIONAL_BEGIN -->
| 显卡 / 数据批次 | 功能结果 | 337.5 万单元门禁 |
| --- | --- | --- |
| MX450，本轮同一批次 | 84/84；总计 714.61 秒 | 8/8，本轮重跑 |
| Iris Xe，A10 历史证据 | 首轮 81/84；修正后 5/5，随后基准 1/1；84 项均有通过证据 | 8/8，历史证据 |

MX450 薄板 384 图回归耗时 **354.08 秒**，没有跳过或放宽图像阈值。Iris Xe 本轮重新执行性能测试，未重新执行整套 84 项；历史拆分复测过程见 A10 记录，不写成一次性 84/84。MX450 全量中三场景 smoke 的 renderer 也确认为 MX450；原生/生成器测试本身不执行 GPU 渲染。

| 百万门禁 | MX450 结果 | source / result 逻辑保留字节 |
| --- | --- | --- |
| placement | PASS | 1460965248 / 597683072 |
| material | PASS | 1461030784 / 445418560 |
| free_body | PASS | 1460965248 / 1093811072 |
| iso_binding | PASS | 1460965248 / 737329152 |
| slices | PASS | 1460965248 / 921862144 |
| plot_options | PASS | 1460965248 / 597683072 |
| automatic | PASS | 1460965248 / 460855296 |
| scope | PASS | 1460965248 / 561011072; 1460965248 / 597683072 |
<!-- FUNCTIONAL_END -->

功能覆盖分为以下层次：

| 层次 | 主要验证 |
| --- | --- |
| 数据与几何 | 自研 VTK/VTU reader、线性/高阶 FE、平面/其他切面、切分/恢复、原节点/单元来源与场值 |
| 渲染 | GPU/CPU 图像对照、云图/线框/云图加线框、三部分显示、变形网格、单色/plot 继承与 fallback |
| Manager | 控件、实际鼠标路由、Placement、材料随动、自由体、等值绑定、切片、自动来源、session/timeline、双视口隔离 |
| 缺陷回归 | 单层 6×6 正弦薄板、厚度 0.05、temperature=z，VTK/VTU × 6 平面 × 2 法向 × 8 mask × 2 backend，共 384 张图像比较 |
| 生命周期 | 定义/字段/显示更新、缓存预算、清除与恢复、失败原子性；百万门禁检查暖主体复用 |

功能夹具使用小型解析模型和图像以定位正确性。**本报告的性能数字全部来自 337.5 万单元模型**，没有用默认 smoke 小网格替代。

八组百万单元门禁检查 placement、material、free_body、iso_binding、slices、plot_options、automatic、scope。云图/线框/组合的多 mask 与移动复用由 placement/plot_options 等门禁验证；它们的整批耗时不是实时 FPS。

预算限额始终为 source 4 GiB、result 16 GiB，没有提高限额取得通过。常规 source 逻辑保留为 1,460,965,248 字节；material 为 1,461,030,784 字节。result 因 case 不同约 0.41–1.02 GiB。具体 state 行已归档到 `summary.json`。这些是共享账本的逻辑保留量，**不是进程 RSS 或显存占用**；MX450 的 2 GiB 显存也不是上述 CPU 预算。

free_body/slices 百万夹具使用 displacement 显式组件合成张量，验证吞吐、生命周期和来源；物理精度由解析 FE 夹具验证，不能把合成张量当作真实应力场验证。

额外进行跨卡图像核验：三轮 × 三场景 × 首末帧，共 **18 对 900×650 截图**，每对的前景掩码差异均为 **0 像素**，RGB 最大通道误差均为 **1/255**，超过预设 2/255 阈值的像素均为 0。前景定义为任意 RGB 通道小于 250。结果和图像哈希见 [图像比较记录](test-results/cut-view-gpu-2026-10-21/image-comparison.json)。这只验证这些采样帧，不等于全帧图像或任意场景等价证明。

另用每场景 2 帧独立诊断核对 `vcamera` / `vviewparams`，两卡三个场景的相机参数及矩阵输出一致，见 [相机比较记录](test-results/cut-view-gpu-2026-10-21/camera-comparison.json)。诊断帧不混入性能样本。大变形截图预览曾疑似存在上边界差异，像素核验已排除，未据此记录为产品缺陷。

## 4. 性能协议

每张卡运行 3 轮，每轮 crossing、rotating、deformed 各 60 帧，每卡共 540 帧。每轮启动新 DRAW 进程，在进程内依次导入三个场景；两卡使用相同模型、相同路径和显示参数。汇总每个场景的 180 帧，同时保留各轮结果；这不是单次连续 180 帧拖动。

- `mask=3`，显示 Below + On；backend=`auto`。空间平面由 GPU clipping 保留主体，CPU slice 生成切面，字段从原 FE 插值。
- crossing：固定法向，平面跨单元平移；rotating：法向逐帧旋转并平移；deformed：固定归一化变形倍率 0.1 后平移切面。deformed **没有逐帧重建变形或播放时间步**。
- 路径中的平面为 `(cos(a), sin(a), 0, -75*(cos(a)+sin(a))-0.125-3.4*i/N)`，rotating 的 `a=0.005+0.3*i/N`，其他为 0。`N=60`，`i=0..59`。
- 变形倍率经 Manager 按模型尺寸和最大位移模长转换，不是直接把原 displacement 乘 0.1；见 [变形指南](cut-view-manager-deformation.md)。
- `total_ms` 包括 SetPlane、native Refresh、实际 `V3d_View::Redraw` 与 `glFinish`；数值检查和首末截图在该帧计时外。后续帧可能受这些检查留下的缓存状态影响。
- setup 包含 import 和首次准备，不与 import 重复相加。未清理 OS 文件缓存，导入结果不能称冷启动时间。
- FPS=`1000*N/sum(total_ms)`；p50 取排序后上中位数；p95 取 nearest rank `ceil(0.95*N)`。合并三轮原始帧重新计算，不简单平均 FPS 或分位数。
- 每帧验证平面 GPU 路径、`body_builds=1`、`submitted_body_bytes=0`，并抽查截面节点原 FE 温度值。截面三角形/颜色数据仍会逐帧提交。
- 性能门槛为 **FPS≥15 且 p95≤67 ms**，两者同时满足才 target=1；不是 30 FPS 承诺。

这是同步 API 驱动的完整帧吞吐，含 GPU 完成等待。它不是 GPU timer query 计时，也不是完整鼠标输入到屏幕呈现延迟；实际鼠标路由由功能回归覆盖。

## 5. 相同长样本性能对比

<!-- PERFORMANCE_BEGIN -->
| GPU | 场景 | 汇总帧数 | p50 ms | p95 ms | FPS | target |
| --- | --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 180 | 47.265 | 83.786 | 19.880 | 0 |
| MX450 | 旋转并平移 | 180 | 91.927 | 126.835 | 10.327 | 0 |
| MX450 | 变形后平移 | 180 | 98.129 | 130.868 | 9.568 | 0 |
| Iris Xe | 平移 | 180 | 59.128 | 91.415 | 16.062 | 0 |
| Iris Xe | 旋转并平移 | 180 | 90.709 | 119.946 | 10.367 | 0 |
| Iris Xe | 变形后平移 | 180 | 105.577 | 137.477 | 8.998 | 0 |

各轮 **FPS / p95 ms**（每格 60 帧）：

| GPU | 场景 | 第 1 轮 | 第 2 轮 | 第 3 轮 |
| --- | --- | --- | --- | --- |
| MX450 | 平移 | 22.664 / 79.550 | 20.761 / 81.316 | 17.060 / 96.183 |
| MX450 | 旋转并平移 | 11.499 / 101.740 | 10.961 / 109.512 | 8.905 / 135.681 |
| MX450 | 变形后平移 | 9.841 / 125.133 | 8.785 / 146.424 | 10.193 / 121.115 |
| Iris Xe | 平移 | 16.158 / 89.704 | 15.775 / 91.415 | 16.262 / 88.471 |
| Iris Xe | 旋转并平移 | 10.656 / 106.525 | 10.259 / 119.692 | 10.197 / 121.089 |
| Iris Xe | 变形后平移 | 9.158 / 123.002 | 9.055 / 136.438 | 8.788 / 138.030 |

导入与首次准备的三轮算术均值：

| GPU | 场景 | import ms | setup ms（含 import） |
| --- | --- | --- | --- |
| MX450 | 平移 | 4394.997 | 11921.055 |
| MX450 | 旋转并平移 | 4332.739 | 11678.844 |
| MX450 | 变形后平移 | 4457.912 | 13329.346 |
| Iris Xe | 平移 | 3011.647 | 8737.865 |
| Iris Xe | 旋转并平移 | 3606.020 | 9536.955 |
| Iris Xe | 变形后平移 | 3548.371 | 11041.073 |
<!-- PERFORMANCE_END -->

三轮顺序测试没有随机交叉卡次，也没有锁定温度、频率或清空文件缓存。逐轮波动和提升比例只描述本机这次执行；不能直接推广到任意 MX450 功耗档、任意 Iris Xe 内存配置或生产模型。
两卡共用同一 CPU，但 CPU 阶段的耗时差异仍可能包含频率、热状态、内存竞争与驱动影响，不能解释为 GPU 在执行 CPU 求交。

## 6. 阶段耗时与瓶颈

<!-- STAGES_BEGIN -->
以下为 180 帧算术均值（ms，含缓存命中帧）：

| GPU | 场景 | SetPlane | intersection | slice | trace | FE | primitives | redraw+finish | total |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 5.383 | 10.621 | 1.751 | 8.870 | 4.101 | 9.697 | 20.500 | 50.303 |
| MX450 | 旋转并平移 | 0.053 | 59.849 | 34.760 | 25.089 | 4.884 | 10.460 | 21.586 | 96.833 |
| MX450 | 变形后平移 | 2.738 | 65.875 | 45.912 | 19.963 | 4.238 | 10.837 | 20.828 | 104.518 |
| Iris Xe | 平移 | 4.471 | 8.074 | 1.604 | 6.469 | 3.210 | 7.118 | 39.383 | 62.257 |
| Iris Xe | 旋转并平移 | 0.041 | 46.726 | 26.780 | 19.946 | 4.085 | 7.685 | 37.923 | 96.463 |
| Iris Xe | 变形后平移 | 2.185 | 55.284 | 38.638 | 16.646 | 3.755 | 8.805 | 41.109 | 111.139 |

CPU slice 内部阶段均值（ms）：

| GPU | 场景 | BVH | node prepare | cell cut | merge |
| --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 0.130 | 0.000 | 0.307 | 0.921 |
| MX450 | 旋转并平移 | 2.547 | 0.000 | 6.982 | 19.042 |
| MX450 | 变形后平移 | 2.355 | 18.033 | 2.983 | 15.905 |
| Iris Xe | 平移 | 0.109 | 0.000 | 0.324 | 0.832 |
| Iris Xe | 旋转并平移 | 2.092 | 0.000 | 5.392 | 14.589 |
| Iris Xe | 变形后平移 | 2.060 | 15.628 | 2.244 | 13.198 |

逐帧截面提交均值（十进制 MB）：MX450/平移 4.320 MB；MX450/旋转并平移 4.988 MB；MX450/变形后平移 4.696 MB；Iris Xe/平移 4.320 MB；Iris Xe/旋转并平移 4.988 MB；Iris Xe/变形后平移 4.696 MB。两卡全部 1,080 个长样本帧的主体提交均为 **0 字节**，主体构建计数均为 **1**。
<!-- STAGES_END -->

阶段列有包含关系：intersection 包含 slice/trace，slice 又包含 BVH、节点准备、单元求交和 merge 等。不要把所有阶段直接相加；完整帧以 total 为准。`primitives` 是 Refresh 扣除 source/intersection/FE 后的剩余时间，不是单独 GPU 上传计时。

主体零提交说明没有逐帧重传整个模型，不代表每帧没有任何上传。`actual_redraw_ms` 包括提交、驱动、GPU 执行与同步等待；仅凭它不能判断具体 GPU kernel 耗时或断言驱动故障。

后续优化应继续分别测量旋转的 trace/merge、变形节点准备、显示图元生成以及 redraw 等待。BVH 只占总帧的一部分，单独继续加速 BVH 不足以保证整体交互达标。

本轮 MX450 的 redraw+finish 均值约 20.5–21.6 ms，Iris Xe 约 37.9–41.1 ms；独显在这个阶段的收益清晰。旋转总帧却分别为 96.83/96.46 ms：MX450 批次 CPU intersection 均值 59.85 ms，Iris Xe 为 46.73 ms，抵消了绘制阶段的优势。由于没有控制 CPU 热状态/频率，不能把 CPU 阶段差异归因为显卡本身。

按本轮耗时分布，下一轮应优先减少 trace/merge 的分配和合并工作、复用固定变形构形的节点准备，再分析图元生成与同步等待。MX450 变形场景 node prepare/merge 均值约 18.03/15.91 ms；BVH 约 2.36 ms，优先级应低于这些已测出的大项。

## 7. 历史 20 帧基线（单独保留）

<!-- BASELINE_BEGIN -->
| GPU | 场景 | p50 ms | p95 ms | FPS | target |
| --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 39.781 | 77.940 | 21.145 | 0 |
| MX450 | 旋转并平移 | 81.396 | 105.007 | 11.556 | 0 |
| MX450 | 变形后平移 | 95.777 | 108.231 | 10.172 | 0 |
| Iris Xe | 平移 | 59.900 | 106.318 | 14.911 | 0 |
| Iris Xe | 旋转并平移 | 122.284 | 137.278 | 8.055 | 0 |
| Iris Xe | 变形后平移 | 151.968 | 185.005 | 6.684 | 0 |
<!-- BASELINE_END -->

历史 Iris Xe 为 A10 数据 `build/a10-full-frame-large`；MX450 为本轮 `build/gpu-mx450-baseline20`。它们与长样本在帧数、平面采样密度及缓存命中上不同，不合并计算长样本提升比例，也不替换历史记录。

## 8. Windows 复现命令

先准备现有 Release 构建、OCCT 运行时及生成器输入。以下 PowerShell 每段可逐行执行；不经过 GDB，不与其他 GUI/性能任务并行。

```powershell
Set-Location D:\workspace\OpenCASCADE\OCCT_Viewer
python D:\workspace\OpenCASCADE\OCCT_Viewer\test\generate-large-vtk.py --grids 150 --formats vtk vtu --deformation --output D:\workspace\OpenCASCADE\OCCT_Viewer\build\large-vtk-deformed
$cutParams = @{
    BuildDirectory = 'D:\workspace\OpenCASCADE\OCCT_Viewer\build\mingw-deformation'
    DrawExe = 'D:\workspace\OpenCASCADE\OCCT_private\install\mingw\win64\gcc\bin\DRAWEXE.exe'
    OcctResources = 'D:\workspace\OpenCASCADE\OCCT_private\install\mingw\src'
    Model = 'D:\workspace\OpenCASCADE\OCCT_Viewer\build\large-vtk-deformed\hex_150.vtu'
    RuntimeDirectories = @('C:\mingw64\bin', 'D:\workspace\OpenCASCADE\3rdparty\mingw64\zlib-1.3.1-mingw-64\bin')
    Frames = 60
    Repetitions = 3
}
& D:\workspace\OpenCASCADE\OCCT_Viewer\test\run-cut-view-gpu-tests.ps1 @cutParams -Gpu MX450 -Mode All -OutputDirectory D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-mx450-reproduce
& D:\workspace\OpenCASCADE\OCCT_Viewer\test\run-cut-view-gpu-tests.ps1 @cutParams -Gpu IrisXe -Mode Performance -OutputDirectory D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-irisxe-reproduce
```

输出目录必须不存在，以免覆盖旧证据。需要重跑 Iris Xe 全部功能时将其 Mode 改为 All；仅验证显卡时用 Probe。脚本自动设置依赖路径、保存环境、核对 renderer，并在 finally 恢复 GPU 偏好和环境。若进程被外部强制终止，finally 可能没有执行，可按 manifest 的 `preferenceBefore` 恢复 DRAWEXE 对应的原偏好。

相机诊断和一对图像比较（诊断另用 2 帧，不作为 FPS 基准）：

```powershell
$cutParams.Frames = 2
$cutParams.Repetitions = 1
& D:\workspace\OpenCASCADE\OCCT_Viewer\test\run-cut-view-gpu-tests.ps1 @cutParams -Gpu MX450 -Mode Camera -OutputDirectory D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-camera-mx450-reproduce
& D:\workspace\OpenCASCADE\OCCT_Viewer\test\run-cut-view-gpu-tests.ps1 @cutParams -Gpu IrisXe -Mode Camera -OutputDirectory D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-camera-irisxe-reproduce
powershell.exe -NoProfile -File D:\workspace\OpenCASCADE\OCCT_Viewer\test\compare-cut-view-gpu-images.ps1 -First D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-camera-mx450-reproduce\camera\round-1\deformed-1.png -Second D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-camera-irisxe-reproduce\camera\round-1\deformed-1.png
```

统计本次证据（已有历史 20 帧目录）：

```powershell
python D:\workspace\OpenCASCADE\OCCT_Viewer\DOCUMENT\tools\summarize_cut_view_gpu_tests.py --mx450 D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-mx450-validation --irisxe D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-irisxe-validation --mx450-baseline D:\workspace\OpenCASCADE\OCCT_Viewer\build\gpu-mx450-baseline20 --irisxe-baseline D:\workspace\OpenCASCADE\OCCT_Viewer\build\a10-full-frame-large --output D:\workspace\OpenCASCADE\OCCT_Viewer\DOCUMENT\test-results\cut-view-gpu-2026-10-21
```

Linux 直接运行、GDB 调试、构建和全部测试命令见 [Linux 操作指南](cut-view-manager-linux-run-test.md)。本次结果来自 Windows；尚无 Linux/CentOS 7.9 本轮执行数据。

## 9. 证据与适用边界

- 可随代码查看的 [统计与断言结果](test-results/cut-view-gpu-2026-10-21/summary.json)、[逐帧原始数值](test-results/cut-view-gpu-2026-10-21/frames.csv)；统计脚本使用 Python 标准库，失败时拒绝归档。
- MX450 全量：`build/gpu-mx450-validation`，含 manifest、renderer probe、JUnit、完整 CTest 日志、八门禁、三轮 CSV/首末图像。
- Iris Xe 长样本：`build/gpu-irisxe-validation`，含 manifest、renderer、三轮 CSV/首末图像。历史功能证据见 A10 验收记录，不冒称本次又完整重跑 84 项。
- 大模型、构建、完整日志和批量截图保留在本机 build，不纳入 Git；报告归档逐帧数值、各轮/汇总统计、实际 renderer、输入/DLL 哈希、JUnit 结果及门禁 state/日志哈希。
- 本轮长样本只测上述单平面、单实体规则 hex 模型和默认字段面显示；三种显示模式的功能通过不能替代三种模式各自的性能曲线。多 cut、透明度/混合模式 CPU fallback、非平面、材料曲面、动态时间步和多 domain/MPI 不在本次 FPS 测量范围内。
- 没有长时间真实鼠标延迟、受控进程/显存峰值、热稳态压测或不规则生产网格统计；没有实际 Abaqus 对照，也没有目标 VisIt/CentOS 移植执行证据。

目标 VisIt 迁移继续遵守 [完全不使用 OCCT 的工程手册](cut-view-manager-visit-porting.md)；本机 DRAW/OCCT 是本项目的测试载体，不是迁移目标的依赖要求。
