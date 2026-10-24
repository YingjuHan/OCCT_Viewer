# Cut View Manager 百万级非规则网格测试

本轮为 Windows Release 直接运行 DRAW，不经过 GDB。数据生成、读取、切面计算均不依赖外部 VTK。源代码见 `test/generate-cut-grid.py`、`test/run-cut-grid-matrix.ps1` 和 `source-draw/ViewCaeTest/CutBenchmarkCommands.cpp`。

## 数据

| 文件 | 单元 | 节点 | 结构 |
| --- | ---: | ---: | --- |
| tetra_60.vtk / tetra_60.vtu | 1,296,000 | 226,981 | 每个宏六面体分为六个 tetra，共享节点 |
| mixed_70.vtu | 1,019,200 | 470,611 | 沿 X 分层交替 hex / prism / pyramid；宏单元侧面相容 |
| hex_100_warped.vtu | 1,000,000 | 1,030,301 | 内部节点作三维正弦扰动，幅度 0.15；外边界保持立方体 |

全部文件含点标量 `temperature=x+2y+3z`、点向量 `displacement=(0.02y,0.01x,0.005z)` 以及单元标量 `pressure=7+0.25*cellId`。扰动坐标量化为实际存储的 Float32 后再生成温度，避免参考值自身的舍入误差。生成器逐层写出二进制 VTK / raw-appended VTU，使用 Python 标准库；大文件仅保留在 build。

```powershell
Set-Location D:\workspace\OpenCASCADE\OCCT_Viewer
python D:\workspace\OpenCASCADE\OCCT_Viewer\test\generate-cut-grid.py --grid 60 --topology tetra --formats vtk vtu --output D:\workspace\OpenCASCADE\OCCT_Viewer\build\cut-nonregular
python D:\workspace\OpenCASCADE\OCCT_Viewer\test\generate-cut-grid.py --grid 70 --topology mixed --output D:\workspace\OpenCASCADE\OCCT_Viewer\build\cut-nonregular
python D:\workspace\OpenCASCADE\OCCT_Viewer\test\generate-cut-grid.py --grid 100 --topology hex --warp 0.15 --output D:\workspace\OpenCASCADE\OCCT_Viewer\build\cut-nonregular
& D:\workspace\OpenCASCADE\OCCT_Viewer\test\run-cut-grid-matrix.ps1 -OutputDirectory D:\workspace\OpenCASCADE\OCCT_Viewer\build\cut-grid-reproduce
```

输出目录必须不存在。脚本每组启动新 DRAW，设置并恢复 GPU 偏好，核对真实 OpenGL renderer；可以用 `-Gpus MX450` 单独运行。Linux 的构建、直接运行及 GDB 命令见 [Linux 指南](cut-view-manager-linux-run-test.md)，本轮没有 Linux 执行数据。

## 协议与正确性

- 两卡 × 四个文件 × 三种显示模式，共 24 组；每组平移、旋转、固定变形后平移各 12 帧，共 72 场景、864 帧。
- 使用现有 900×650 视口、mask=3、auto backend；每帧确认主体构建计数为 1、主体上传为 0，并抽查原 FE 温度。
- `-FullValidation` 在每个场景首末帧计时外检查全部截面节点的原 FE 权重、温度和切平面残差，并用整体立方体/仿射变形边界的独立求交面积核对截面总面积；没有用单元切面算法本身当参考。
- 权重和容差 1e-9，温度绝对误差 1e-6，平面残差 2e-5（覆盖原生 1e-5 分类容差），面积误差不超过 `max(1,referenceArea)*2e-7`；144 次全量节点/面积核验通过，归档最大绝对面积误差约 0.0002963。随后强化非有限值断言，对 Iris Xe / 扰动 hex / 组合显示再完整重跑三场景六次全量核验并通过，见 [补测记录](test-results/cut-view-performance-2026-10-24/t08/finite-recheck/run.log)。
- 数值验证和截图不计入 total_ms，但会影响后续缓存和热状态。12 帧只作为功能/性能诊断样本，不能替代双卡三轮 60 帧长样本，不能把功能通过解释为性能达标。
- 独立 Native 用例还覆盖 129.6 万 tetra、102.4 万 prism、105.3696 万 pyramid 的解析面积及原 FE 温度；见 [任务清单](cut-view-manager-performance-todo.md)。

## 诊断结果

每格为 **FPS / p95 ms**，均来自单轮 12 帧。24/72 个场景在短样本中同时满足 15 FPS 与 p95≤67 ms；其余场景保留失败结果，没有提高预算或删去慢帧。

| GPU | 文件 | 显示 | 平移 | 旋转 | 固定变形平移 |
| --- | --- | --- | --- | --- | --- |
| MX450 | tetra_60.vtu | 云图 | 58.89 / 28.73 | 23.56 / 45.49 | 25.77 / 49.72 |
| MX450 | tetra_60.vtu | 线框 | 43.10 / 40.82 | 21.58 / 52.20 | 23.59 / 53.43 |
| MX450 | tetra_60.vtu | 云图+线框 | 29.02 / 47.70 | 18.57 / 60.19 | 19.61 / 65.88 |
| MX450 | tetra_60.vtk | 云图 | 39.99 / 39.38 | 23.77 / 46.92 | 25.97 / 49.98 |
| MX450 | tetra_60.vtk | 线框 | 35.33 / 43.82 | 21.67 / 49.96 | 23.75 / 52.35 |
| MX450 | tetra_60.vtk | 云图+线框 | 29.50 / 48.53 | 18.44 / 59.56 | 19.46 / 66.42 |
| MX450 | mixed_70.vtu | 云图 | 39.14 / 48.67 | 18.21 / 77.41 | 20.97 / 68.22 |
| MX450 | mixed_70.vtu | 线框 | 37.24 / 51.62 | 17.68 / 82.74 | 20.22 / 67.36 |
| MX450 | mixed_70.vtu | 云图+线框 | 42.22 / 46.93 | 14.88 / 99.92 | 16.51 / 84.49 |
| MX450 | hex_100_warped.vtu | 云图 | 20.70 / 95.01 | 14.12 / 76.75 | 13.03 / 142.92 |
| MX450 | hex_100_warped.vtu | 线框 | 21.95 / 90.22 | 13.13 / 88.85 | 12.86 / 141.08 |
| MX450 | hex_100_warped.vtu | 云图+线框 | 18.09 / 112.90 | 12.08 / 99.81 | 10.69 / 155.44 |
| IrisXe | tetra_60.vtu | 云图 | 18.78 / 75.67 | 13.81 / 78.65 | 14.68 / 76.50 |
| IrisXe | tetra_60.vtu | 线框 | 20.23 / 61.27 | 13.20 / 78.18 | 14.23 / 83.08 |
| IrisXe | tetra_60.vtu | 云图+线框 | 18.36 / 76.43 | 13.23 / 79.58 | 13.71 / 76.64 |
| IrisXe | tetra_60.vtk | 云图 | 19.85 / 61.26 | 14.00 / 76.66 | 13.99 / 86.06 |
| IrisXe | tetra_60.vtk | 线框 | 20.15 / 61.56 | 13.26 / 80.16 | 13.52 / 81.76 |
| IrisXe | tetra_60.vtk | 云图+线框 | 18.56 / 76.00 | 12.92 / 90.10 | 13.44 / 88.75 |
| IrisXe | mixed_70.vtu | 云图 | 17.75 / 78.15 | 10.02 / 142.69 | 11.73 / 100.97 |
| IrisXe | mixed_70.vtu | 线框 | 17.66 / 77.25 | 10.63 / 109.35 | 11.75 / 111.75 |
| IrisXe | mixed_70.vtu | 云图+线框 | 17.83 / 77.94 | 10.46 / 126.03 | 11.03 / 117.79 |
| IrisXe | hex_100_warped.vtu | 云图 | 12.58 / 141.45 | 8.76 / 154.73 | 8.36 / 205.10 |
| IrisXe | hex_100_warped.vtu | 线框 | 11.37 / 148.28 | 9.08 / 140.22 | 8.37 / 193.49 |
| IrisXe | hex_100_warped.vtu | 云图+线框 | 12.67 / 134.51 | 8.64 / 155.59 | 8.18 / 183.22 |

## 证据与限制

[逐场景统计](test-results/cut-view-performance-2026-10-24/t08/summary.json)、[执行矩阵](test-results/cut-view-performance-2026-10-24/t08/matrix.json)、[CSV/renderer/运行清单/文件哈希](test-results/cut-view-performance-2026-10-24/t08/evidence/) 与 [模型生成信息](test-results/cut-view-performance-2026-10-24/t08/models/) 均随代码归档。PNG 与完整运行日志保留在 `build/perf-t08-matrix`。统计复核：

```powershell
python D:\workspace\OpenCASCADE\OCCT_Viewer\DOCUMENT\tools\summarize_cut_grid_matrix.py D:\workspace\OpenCASCADE\OCCT_Viewer\build\perf-t08-matrix --output D:\workspace\OpenCASCADE\OCCT_Viewer\build\cut-grid-summary
```

扰动 hex 的面不再满足仿射快速路径条件，会进入通用轮廓求交；线框/组合显示也有更多绘制工作。这些数据表明瓶颈与单元几何和显示模式有关，不能只由单元数量或 Morton BVH 推断速度。本轮合成模型不代表任意生产网格，没有验证 MPI/ghost、动态时间步播放、任意高阶或凹多面体的百万级性能。
