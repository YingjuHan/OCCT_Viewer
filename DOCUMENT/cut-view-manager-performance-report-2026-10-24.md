# Cut View Manager 性能优化与最终回归（2026-10-24）

## 范围与默认决策

本报告对应 [T01–T10 任务清单](cut-view-manager-performance-todo.md)。优化限定为平面实时切割，不改变其他 cut 方式，不引入外部 VTK。主体常驻 GPU，由 shader clipping 处理平面；CPU 计算截面、原 FE 来源和场值，截面仍逐帧提交。完整帧包括 CPU 和实际绘制等待，不能称为全部计算都在 GPU 上。

| 工作 | 最终选择 |
| --- | --- |
| 计时 | 拆分共享点、输出追加、fallback，并记录五类互斥回退原因 |
| 输出 | 分块计算、前缀和、预分配、独立并行填充；统一共享边节点编号 |
| 固定变形 | 复用与源 BVH 同 epoch 的只读节点快照，同时失效并计入 source 预算 |
| FE trace | 直接读取已有原 FE 来源；内部截面并行填充，界面多 owner 保留通用语义 |
| 快速单元 | tetra、仿射 hex、平移型 prism、平面凸底 pyramid；不满足条件时保守回退 |
| 单元拓扑实验 | `OCCTV_CUT_CELL_TOPOLOGY_CACHE=1` 可启用，默认关闭；命中率未转化为稳定整帧收益 |
| 显示准备 | 复用 positions/UV/面边暂存，按独立范围并行生成；保留共同局部坐标和配对发布 |
| BVH | 保留 Morton；大数据单平面按既有子树并行查询，按原遍历顺序合并 |

`OCCTV_CUT_PARALLEL_QUERY=0` 可对照串行候选查询。337.5 万 hex 的独立重放中，Morton 构建约 200 ms，查询串行/并行约 2.50/1.21 ms；SAH16 构建约 801 ms、并行查询约 0.78 ms。建树成本、完整帧成本和候选正确性分别测量，未将微基准倍数当作 FPS 提升。三模型 2376 次候选比较均一致，见 [T09 数据](test-results/cut-view-performance-2026-10-24/t09/queries-summary.json)。

## 被测环境与版本

- Windows Release、MinGW GCC 14.2、定制 OCCT SDK；直接运行 DRAW，不挂 GDB。
- i7-1165G7，4 核 8 线程；同一台电脑上的 NVIDIA MX450 与 Intel Iris Xe 顺序执行。
- 模型 `build/large-vtk-deformed/hex_150.vtu`：3,375,000 hex、3,442,951 节点、303,366,033 字节；两卡使用同一文件。
- 模型 SHA-256：`83180755c60eac9064e2f2c47441b2e39ed9626e79ec2723ab36abb02da9c551`。
- 产品算法基于 `49df48c`；其后 T10 补充计时外进程内存采样、基准帮助及测试归档。运行清单如实记录未提交的测试辅助修改，最终文件与 T10 提交对应。
- 每卡保存模型、DRAWEXE 及六个关键 DLL 的 SHA-256，核对两批相同；真实 OpenGL renderer 由运行前后 probe、功能 smoke 和各性能场景分别验证。GPU 偏好退出时恢复。

| 项目 | 本轮配置 |
| --- | --- |
| 系统 / 内存 | Windows 11 专业工作站版 10.0.26200；系统可见 51,245,015,040 字节 |
| MX450 OpenGL | 4.6.0 NVIDIA 573.91 |
| IrisXe OpenGL | 4.6.0 - Build 32.0.101.7088 |
| 视口 / 电源 | 900×650；Windows 平衡计划，未修改电源计划 |

## 最终功能与性能结果

| GPU | 全量回归 | 耗时 s | 337.5 万单元门禁 |
| --- | --- | --- | --- |
| MX450 | 85/85 | 750.16 | 8/8 |
| IrisXe | 85/85 | 747.30 | 8/8 |

| GPU | 场景 | 帧数 | p50 ms | p95 ms | FPS | 组合门槛 |
| --- | --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 180 | 38.97 | 55.08 | 25.32 | 达标 |
| MX450 | 旋转并平移 | 180 | 71.68 | 86.86 | 13.83 | 未达标 |
| MX450 | 固定变形后平移 | 180 | 67.98 | 80.00 | 14.24 | 未达标 |
| IrisXe | 平移 | 180 | 59.02 | 74.36 | 16.42 | 未达标 |
| IrisXe | 旋转并平移 | 180 | 89.54 | 104.80 | 11.17 | 未达标 |
| IrisXe | 固定变形后平移 | 180 | 87.72 | 99.14 | 11.76 | 未达标 |

每轮 **FPS / p95 ms**（每格 60 帧）：

| GPU | 场景 | 第 1 轮 | 第 2 轮 | 第 3 轮 |
| --- | --- | --- | --- | --- |
| MX450 | 平移 | 26.81 / 51.97 | 25.71 / 53.77 | 23.66 / 61.63 |
| MX450 | 旋转并平移 | 14.24 / 78.71 | 14.25 / 78.66 | 13.08 / 89.22 |
| MX450 | 固定变形后平移 | 15.01 / 74.36 | 14.12 / 79.39 | 13.64 / 81.97 |
| IrisXe | 平移 | 16.42 / 74.15 | 16.46 / 74.28 | 16.39 / 75.70 |
| IrisXe | 旋转并平移 | 11.18 / 103.92 | 11.22 / 104.83 | 11.12 / 104.73 |
| IrisXe | 固定变形后平移 | 11.73 / 99.14 | 11.85 / 91.40 | 11.71 / 101.68 |

门槛为 **FPS ≥ 15 且 p95 ≤ 67 ms**，同时满足才达标。功能断言通过、基准运行完成和性能门槛达标是三件不同的事。任务完成不表示所有输入均达到交互帧率目标。

## 计时协议

两卡各三轮，每轮 crossing、rotating、deformed 三场景各 60 帧，共 1080 帧；每场景汇总 180 帧，另保留逐轮结果。每轮启动新 DRAW，在同一进程内依次创建三个场景；不是单次连续 180 帧拖动。

- 900×650 视口，云图，mask=3（Below + On），backend=auto；拓扑实验关闭，并行候选查询开启。
- 平移：固定法向跨单元移动；旋转：法向逐帧旋转并平移；变形：先准备归一化变形倍率 0.1，再平移切面。没有逐帧重建变形或播放时间步。
- `total_ms` 从 SetPlane 开始，包含 Native Refresh、`V3d_View::Redraw` 与 `glFinish`。数值检查、进程内存采样、首末截图在该帧计时外，但可能影响后续缓存或热状态。
- FPS=`1000*N/sum(total_ms)`；p50 为排序后上中位数；p95 为 nearest rank `ceil(0.95*N)`。三轮合并原始帧重新计算，不平均 FPS 或分位数。
- 每帧验证 GPU 平面后端、主体构建次数 1、主体提交字节 0，并抽查原 FE 温度。长样本按历史协议不启用逐节点全量检查；完整数值面积检查见百万非规则矩阵及原生解析用例。
- import 已包含在 setup 中，不重复相加；未清空 OS 文件缓存，不能称为冷启动测试。
- 每卡按功能回归 → 百万门禁 → 性能三轮顺序执行，不另设冷却阶段；不与编译、索引或其他性能程序并行。

这是同步 API 驱动的完整帧吞吐，含 GPU 完成等待；不是 GPU timer query 或鼠标输入到屏幕呈现延迟。实际鼠标路由、取消和配对发布由功能回归检查。

## CPU 阶段及剩余瓶颈

| GPU | 场景 | SetPlane | source | intersection | FE | primitives | redraw+finish | total |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 5.14 | 0.00 | 7.88 | 4.10 | 5.59 | 16.78 | 39.49 |
| MX450 | 旋转并平移 | 0.05 | 0.00 | 39.55 | 4.27 | 6.58 | 21.84 | 72.29 |
| MX450 | 固定变形后平移 | 2.26 | 0.00 | 36.63 | 4.10 | 5.80 | 21.44 | 70.25 |
| IrisXe | 平移 | 4.44 | 0.00 | 7.14 | 3.53 | 5.46 | 40.34 | 60.89 |
| IrisXe | 旋转并平移 | 0.05 | 0.00 | 37.71 | 4.31 | 6.62 | 40.80 | 89.50 |
| IrisXe | 固定变形后平移 | 2.35 | 0.00 | 33.97 | 4.12 | 5.59 | 38.97 | 85.01 |

内部阶段均值（ms）：

| GPU | 场景 | BVH | 节点准备 | cell cut | merge | trace |
| --- | --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 0.077 | 0.000 | 0.299 | 0.572 | 6.614 |
| MX450 | 旋转并平移 | 1.635 | 0.000 | 7.029 | 14.006 | 11.486 |
| MX450 | 固定变形后平移 | 1.616 | 0.000 | 6.289 | 13.017 | 10.606 |
| IrisXe | 平移 | 0.078 | 0.000 | 0.316 | 0.558 | 5.852 |
| IrisXe | 旋转并平移 | 1.432 | 0.000 | 6.709 | 12.706 | 11.738 |
| IrisXe | 固定变形后平移 | 1.483 | 0.000 | 5.811 | 11.373 | 10.488 |

merge 子项均值（ms）：

| GPU | 场景 | shared point | append | fallback |
| --- | --- | --- | --- | --- |
| MX450 | 平移 | 0.312 | 0.250 | 0.000 |
| MX450 | 旋转并平移 | 7.510 | 5.501 | 0.660 |
| MX450 | 固定变形后平移 | 6.689 | 5.398 | 0.585 |
| IrisXe | 平移 | 0.297 | 0.251 | 0.000 |
| IrisXe | 旋转并平移 | 6.841 | 4.982 | 0.617 |
| IrisXe | 固定变形后平移 | 5.906 | 4.703 | 0.508 |

在最终规则 hex 长样本中，旋转/变形的 BVH 均值约 1–2 ms，显著小于完整帧。输出合并、trace 和绘制等待仍是主要成本；只替换 BVH 建树器不足以使全部场景达标。

阶段有包含关系：`intersection` 包含 `slice` 和 `trace`；`slice` 中含 BVH、节点准备、并行求交及 merge；`shared_point/append/fallback` 是 merge 子项，不能重复相加。`primitives` 是准备总时长扣除 source/intersection/FE 后的剩余项，包含图元准备和相关管理开销；`redraw+finish` 含驱动提交与 GPU 等待，不等于纯 GPU 执行时间。

阶段均值包含平移整体拓扑缓存命中帧，尤其 crossing 的低 BVH 均值不能当作一次全新空间查询的延迟。固定变形的 node_prepare 接近零表示复用了已准备坐标，首次变形准备仍计入 setup。

畸变 hex 不满足仿射快速路径时仍进入通用轮廓算法。T08 的双卡、四文件、三显示模式矩阵共 72 场景，仅 24 个短样本场景同时满足性能目标；不能由规则 hex 的长样本推广到任意生产网格。详见 [百万级非规则网格测试](cut-view-manager-nonregular-tests.md)。

## 内存与上传

1080 帧的主体构建次数均为 1，主体提交均为 0 字节；截面仍更新并提交。以下为三个独立进程轮次中的最大值，单位 GiB，截面提交为每帧均值 MiB：

| GPU | 场景 | Private 采样最大 | RSS 采样最大 | 进程历史 RSS 峰值 | 截面提交 MiB/帧 |
| --- | --- | --- | --- | --- | --- |
| MX450 | 平移 | 1.142 | 0.952 | 1.436 | 4.120 |
| MX450 | 旋转并平移 | 1.128 | 0.963 | 1.511 | 4.757 |
| MX450 | 固定变形后平移 | 1.185 | 1.011 | 1.561 | 4.478 |
| IrisXe | 平移 | 0.947 | 0.948 | 1.440 | 4.120 |
| IrisXe | 旋转并平移 | 0.985 | 0.982 | 1.510 | 4.757 |
| IrisXe | 固定变形后平移 | 1.040 | 1.037 | 1.560 | 4.478 |

百万门禁的逻辑保留量（字节，列出各 case 已记录状态的所有唯一值；两卡逐项一致）：

| case | sourceReserved | resultReserved |
| --- | --- | --- |
| placement | 1626226896 | 597683072 |
| material | 1626292432 | 445418560 |
| free_body | 1626226896 | 1093811072 |
| iso_binding | 1626226896 | 737329152 |
| slices | 1460965248 | 921862144 |
| plot_options | 1626226896 | 597683072 |
| automatic | 1460965248 | 460855296 |
| scope | 1460965248, 1626226896 | 561011072, 597683072 |

source 预算仍为 4 GiB，result 仍为 16 GiB，没有提高限额。逻辑保留量表示缓存账本中的保守预留；Windows PrivateUsage、WorkingSetSize 和 PeakWorkingSetSize 是进程计数，二者不可互换，也都不是 GPU 显存。

`submitted_*_bytes` 是应用记录的提交量，不是硬件总线流量或驱动内部复制量；“主体零提交”不表示 GPU 没有继续读取主体的常驻缓冲。

逐帧进程计数在该帧结束后采样，表中的采样最大值不能覆盖帧内瞬时峰值。PeakWorkingSet 是当前 DRAW 进程生命周期的系统高水位，可能包含前面场景的导入和准备，不归因于单个切面。有限轮次的 first/last/maximum 不能单独证明无内存泄漏；生命周期回归和预算回收检查另有覆盖。非 Windows 基准当前返回零值占位，本轮统计脚本要求 Windows 有效采样。

## 正确性与图像证据

两卡三轮 × 三场景 × 首末帧共 **18 对 900×650 截图**：前景掩码差异均为 0，最大通道误差为 1/255，超过预设 2/255 阈值的像素均为 0。前景定义为任一 RGB 通道小于 250。图像哈希、比较指标和原始 PNG 路径见 [图像核验记录](test-results/cut-view-performance-2026-10-24/t10/image-comparison.json)。

T07 与 T04 的三场景首末六对截图逐像素完全一致。T08 还覆盖 129.6 万 tetra 的 VTK/VTU、101.92 万混合单元、100 万扰动 hex，以及云图、线框和组合显示；144 次全量截面来源/温度/平面残差/独立边界面积检查通过，强化有限值断言后最复杂组合再补测六次通过。T09 在百万级用例逐项验证串/并行节点顺序、坐标、FE 权重、连接和原始单元 ID 完全相同。

这些测试覆盖所列路径和采样帧，不是任意网格无空洞的形式证明。未在本轮执行 Linux/CentOS、WebAssembly C++ 重编译、MPI/ghost、大规模高阶/凹多面体或动态时间步性能测试。

Vue 示例执行 `npm ci --no-audit --no-fund` 和 `npm run build` 均返回 0，构建耗时 22.087 秒（Node 24.14.1 / npm 11.11.0）。日志保留了依赖引擎范围、旧浏览器数据提示和三个 webpack 资源体积警告。该构建使用已有 `install/wasm32-fallback` 包，只验证前端构建；本轮交互功能由 native DRAW 回归验证。见 [Vue 构建记录](test-results/cut-view-performance-2026-10-24/t10/vue-build.log)。

## 历史对照与解释边界

与 T01 的 MX450 定位基线对照（单轮与三轮协议不同，只报告观察值）：

| 场景 | T01 FPS / p95 ms | T10 FPS / p95 ms | 观察到的 FPS 变化 |
| --- | --- | --- | --- |
| 平移 | 23.02 / 72.22 | 25.32 / 55.08 | +10.0% |
| 旋转并平移 | 11.81 / 100.19 | 13.83 / 86.86 | +17.1% |
| 固定变形后平移 | 10.50 / 119.55 | 14.24 / 80.00 | +35.6% |

与 [2026-10-21 双卡三轮报告](cut-view-manager-gpu-test-report.md) 对照：

| GPU | 场景 | 历史 FPS / p95 ms | 本轮 FPS / p95 ms |
| --- | --- | --- | --- |
| MX450 | 平移 | 19.88 / 83.79 | 25.32 / 55.08 |
| MX450 | 旋转并平移 | 10.33 / 126.83 | 13.83 / 86.86 |
| MX450 | 固定变形后平移 | 9.57 / 130.87 | 14.24 / 80.00 |
| IrisXe | 平移 | 16.06 / 91.41 | 16.42 / 74.36 |
| IrisXe | 旋转并平移 | 10.37 / 119.95 | 11.17 / 104.80 |
| IrisXe | 固定变形后平移 | 9.00 / 137.48 | 11.76 / 99.14 |

T01 是 MX450 单轮 60 帧定位基线；T10 是双卡三轮。历史 2026-10-21 报告也有双卡三轮协议，但产品代码、日期和系统状态不同。测试没有锁定 CPU/GPU 频率、温度，没有随机交叉显卡顺序；不能把跨轮或跨日差异全部归于某一改动，或由两卡 CPU 阶段差异推断 GPU 在做 CPU 求交。

## 复现与归档

在仓库根目录的 PowerShell 中依次执行；两个 GPU runner 不可并行，输出目录必须不存在。

```powershell
Set-Location D:\workspace\OpenCASCADE\OCCT_Viewer
$env:PATH = 'C:/mingw64/bin;' + $env:PATH
cmake --build D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation --parallel 6
$cutParams = @{
    BuildDirectory = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation'
    DrawExe = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/win64/gcc/bin/DRAWEXE.exe'
    OcctResources = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/src'
    Model = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/large-vtk-deformed/hex_150.vtu'
    RuntimeDirectories = @('C:/mingw64/bin', 'D:/workspace/OpenCASCADE/3rdparty/mingw64/zlib-1.3.1-mingw-64/bin')
    Frames = 60
    Repetitions = 3
}
& D:/workspace/OpenCASCADE/OCCT_Viewer/test/run-cut-view-gpu-tests.ps1 @cutParams -Gpu MX450 -Mode All -OutputDirectory D:/workspace/OpenCASCADE/OCCT_Viewer/build/reproduce-t10-mx450
& D:/workspace/OpenCASCADE/OCCT_Viewer/test/run-cut-view-gpu-tests.ps1 @cutParams -Gpu IrisXe -Mode All -OutputDirectory D:/workspace/OpenCASCADE/OCCT_Viewer/build/reproduce-t10-irisxe
python -B D:/workspace/OpenCASCADE/OCCT_Viewer/DOCUMENT/tools/summarize_cut_performance.py --mx450 D:/workspace/OpenCASCADE/OCCT_Viewer/build/reproduce-t10-mx450 --irisxe D:/workspace/OpenCASCADE/OCCT_Viewer/build/reproduce-t10-irisxe --output D:/workspace/OpenCASCADE/OCCT_Viewer/build/reproduce-t10-summary
```

统计器检查 85 项回归、8 个百万门禁、1080 帧的计数/后端/来源标志、回退原因、merge 子阶段、主体零提交、实际 renderer、两卡文件哈希和资源采样。数据见 [T10 汇总及原始证据](test-results/cut-view-performance-2026-10-24/t10/summary.json)；PNG 和大模型保留在 build，截图 SHA-256 与像素比较另行归档。

Linux 的构建、调试和直接运行命令见 [Linux 指南](cut-view-manager-linux-run-test.md)。
