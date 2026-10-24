# Cut View Manager：形变网格的实时剖切、云图与线框

## 支持范围与坐标约定

先应用节点位移，再拖动切面。`cloud`、`wireframe`、`cloud_edges` 三种模式均使用形变后的几何；切面填充、截面单元边线和源体外皮使用相同的平面。节点标量、单元常量云图以及不绑定结果字段的网格显示均可使用。显示线框的边界和背面规则见 [显示模式指南](cut-view-manager-display-modes.md)。

形变不直接改写原网格的节点坐标，遵循本项目已有的归一化倍率：

```text
X = x + effectiveScale * displacement(x)
effectiveScale = normalizedScale * max(original AABB dimensions) / max(|displacement|)
```

零位移或零尺寸时沿用 `ComputeVectorTrsfRatio` 的回退比例。输入必须是匹配节点数量的三分量节点向量；单元向量、标量或不匹配的时间步不能充当位移。`deform` 不绑定云图，Result 字段仍独立选择。

云图值来自有限元结果插值，并不重新用当前空间坐标计算。例如原始 `temperature=z`，形变后在原始节点上仍保留该温度；交点温度为 `sum(weight_i * temperature_i)`，通常不等于交点当前的 Z 坐标。体外皮 UV 与截面 UV 都遵循这个约定。

## Windows 交互运行

本轮使用独立 Release 构建 `build/mingw-deformation`，避免覆盖正在运行的 DRAW 所加载的 DLL。使用匹配 OCCT 的 PowerShell 环境启动：

```powershell
$env:PATH = 'D:\workspace\OpenCASCADE\OCCT_Viewer\build\mingw-deformation\bin;D:\workspace\OpenCASCADE\OCCT_private\install\mingw\win64\gcc\bin;C:\mingw64\bin;D:\workspace\OpenCASCADE\3rdparty\mingw64\zlib-1.3.1-mingw-64\bin;' + $env:PATH
$env:CSF_OCCTResourcePath = 'D:/workspace/OpenCASCADE/OCCT_private/install/mingw/src'
$env:CSF_ShadersDirectory = "$env:CSF_OCCTResourcePath/Shaders"
$env:DRAWHOME = "$env:CSF_OCCTResourcePath/DrawResources"
$env:DRAWDEFAULT = "$env:DRAWHOME/DrawDefault"
$env:CSF_DrawPluginDefaults = $env:DRAWHOME
$env:OCCTV_DRAW_RUNTIME = 'D:/workspace/OpenCASCADE/OCCT_Viewer/build/mingw-deformation/bin/viewdraw-runtime.tcl'
& 'D:\workspace\OpenCASCADE\OCCT_private\install\mingw\win64\gcc\bin\DRAWEXE.exe' -i -f 'D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_deformation_demo.tcl'
```

演示默认使用仓库内的可解析剪切六面体。Manager 的 Display 切换三种模式，Position 滑条和视口手柄可继续拖动。DRAW 内可直接执行：

```tcl
scvcutview cut_modes deform displacement 0.1
scvcutview cut_modes deform
scvcutview cut_modes displaymode cloud_edges
scvcutview cut_modes visibility cut 2
scvcutview cut_modes visibility cut 3
scvcutview cut_modes renderinfo
scvcutview cut_modes deform none 0
```

无参数 `deform` 查询 `field / step / scale / effectiveScale`，不会重新求交。`deform none 0` 恢复原始几何。切面默认固定在空间中，位移变化不会自动把平面附着到材料坐标系；材料随动和协调时间步使用既有 Time / material-frame 流程。

## Linux 直接运行、调试与测试

先按 [Linux 环境指南](cut-view-manager-linux-run-test.md) 设置 `CUT_REPO`、`CUT_DRAWEXE`、`CUT_OCCT_LIB`、`CUT_BASE_LD_LIBRARY_PATH` 和 DRAW 资源，并分别构建 Release / Debug。本节命令已按工程配置核对，尚未在 Linux 实机执行。

```bash
# Release：直接交互运行。
export CUT_BUILD="$CUT_REPO/build/linux-release"
export LD_LIBRARY_PATH="$CUT_BUILD/lib:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/lib/viewdraw-runtime.tcl"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation_demo.tcl"

# Release 自动回归；图像用例要求 OCCTV_TEST_DRAW_VISUAL=ON 和可用的 DISPLAY。
(cd "$CUT_BUILD" && ctest --output-on-failure -R '^(cut_view|cut_cache|draw_cut_view_deformation)$')

# Debug：使用对应 Debug 插件，在 GDB 中输入 run。
export CUT_BUILD="$CUT_REPO/build/linux-debug"
export LD_LIBRARY_PATH="$CUT_BUILD/libd:$CUT_OCCT_LIB${CUT_BASE_LD_LIBRARY_PATH:+:$CUT_BASE_LD_LIBRARY_PATH}"
export OCCTV_DRAW_RUNTIME="$CUT_BUILD/libd/viewdraw-runtime.tcl"
gdb -q -ex 'set breakpoint pending on' \
  -ex 'break ViewCaeGeom::MeshCutViewManager::SetDeformation' \
  --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation_demo.tcl"

# 调试回归用例。
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/deformation-debug-images"
gdb -q --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation.tcl"
```

以生成的 runtime 实际位置为准：文件随 `ViewDraw` 库输出，Linux Debug 当前位于 `libd/`（Windows 为 `bind/`）；不要混用 Release 和 Debug 插件。Linux 加载实际由 `LD_LIBRARY_PATH` 及 DRAW 插件资源配置决定。

## 百万单元验收和拖动

使用自研读取器和处理算法，不链接外部 VTK。生成器仅用 Python 标准库：

```bash
python3 "$CUT_REPO/test/generate-large-vtk.py" --grids 150 --formats vtu \
  --deformation --output "$CUT_REPO/build/large-vtk-deformed"

# 使用前述 Release 环境；专用大模型验收不加入默认 CTest。
export CUTVIEW_BENCHMARK_FILE="$CUT_REPO/build/large-vtk-deformed/hex_150.vtu"
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/deformation-large"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_deformation_large.tcl"

# 真实鼠标拖动，支持 cloud / wireframe / cloud_edges。
export CUTVIEW_BENCHMARK_DEFORMATION=0.1
export CUTVIEW_BENCHMARK_MODE=cloud_edges
export CUTVIEW_BENCHMARK_MASK=3
export CUTVIEW_BENCHMARK_FRAMES=24
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_drag_benchmark.tcl"
```

Windows 的 DRAW 提示符中打开同一个大模型：

```tcl
set ::cut_deformation_file D:/workspace/OpenCASCADE/OCCT_Viewer/build/large-vtk-deformed/hex_150.vtu
set ::cut_deformation_scale 0.1
source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_deformation_demo.tcl
```

仅移动平面时，`body_builds` 应不变且 `submitted_body_bytes=0`，更新的是截面面片、边线、UV 和平面参数。**改变位移场或形变倍率本身需要重新准备几何、BVH 和源体 GPU 图元**，不属于零主体上传的暖拖动；不能据此承诺百万单元逐帧形变动画的帧率。多切面、非平面、透明及向量显示仍沿用原 CPU 回退条件。

## 实现与缓存修复

1. `MeshCutViewManager::SetDeformation` 校验位移并递增源几何 epoch；`InvalidateGeometry` 清理旧求交与空间索引。源链的 `MeshGeometryDeformed` 修改位置，源 bounds、候选单元 BVH、CPU slice 都使用当前坐标。
2. `InteractiveCaeCutView::Synchronize` 根据源 epoch 重建体外皮演示。`MeshCutGpuPresentation` 对外皮面和线使用同一 GPU 平面方程；截面面和周边线共用有限元来源权重恢复的位置，防止重复施加位移。一次形变发布后，平面移动继续复用主体图元。
3. 之前形变使旧几何失效，但渲染器的 `myPresentations / myPickResults / myBodyPresentation` 等仍持有旧几何及预算 lease。337.5 万单元已保留约 17.01 GB 逻辑结果预算，下一形变又申请约 16.65 GB，超过默认 16 GiB。这里是保守预算，不能视为实际 RAM。
4. GPU 平面路径的修复是在同步新源 epoch 前，释放渲染器内部已失效的 CPU 几何引用；另有显示预算计费的常驻 GPU 数组独立保留上一帧。CPU 回退保持既有保留策略。没有提高预算上限，也没有免除外部仍持有结果的计费。外部结果仍被持有时，预算不足会继续拒绝；释放它后可重试同步。
5. DRAW 的 `deform` 现在可查询当前状态，位移查找限定节点三维向量并拒绝歧义；变更前先取消活动平面拖动，避免旧拖动回调访问已失效的几何链。
6. 连续形变还暴露出独立来源追踪链的释放缺口：`MeshCutTraceSource::Data` 仅释放根 handle，而形变 modifier 与根之间是相互持有的父子链，源预算会逐 epoch 累积。现在由该拓扑对象在销毁时拆开其专属 source 链，也覆盖构造失败的清理；源预算必须回到一份稳定保留量。

`deform` 是当前网格的直接修改接口；协调时间步的 `timeddeform / step` 则预先构建完整候选视图后再切换，需要同时容纳旧、新两份有效结果。上述清理只针对已经失效的内部几何，不取消时间步事务的额外预算要求。原位更新位移数值后，应先更新场分量/范围，再显式重新绑定形变；原位修改节点坐标后调用 `InvalidateGeometry`。

## 回归覆盖

- Native：六面体非均匀剪切，正/负/零倍率；验证当前截面坐标、原始有限元场插值及 GPU UV；三模式、mask 2/3/6/7；隐藏主体时形变后恢复；紧预算反复变形；外部持有结果仍然计费。
- DRAW：Manager 模式切换、实际云图/线框像素、CPU/GPU 轮廓比较、节点探针、单元场及无场显示、取消形变和非法位移输入。
- 大模型：3,375,000 六面体 / 3,442,951 节点，三模式和多种可见性组合；用生成器仿射映射的解析逆验证切面温度，检查预算稳定和暖拖动零主体提交。

本轮 Windows Release 验证：17 项相关 CTest 全部通过，专用大模型验收输出 `CUT_DEFORMATION_LARGE_PASS`。在正、负、零倍率循环后，source/result 保留量稳定为 `1,460,965,248 / 17,010,275,392` 字节，默认上限仍为 `4 / 16 GiB`。这些值为逻辑预算；图像和日志位于本机构建目录，不随 Git 提交。Linux 命令尚未实跑。

本机 Release、无 GDB、位移倍率 0.1、mask=3、每种模式 24 个真实鼠标移动帧的样本：

| 模式 | 事件处理吞吐 FPS | p95 每帧 ms |
| --- | ---: | ---: |
| cloud | 8.97 | 150.15 |
| wireframe | 9.85 | 129.94 |
| cloud_edges | 8.21 | 157.81 |

三组均通过 `paired_frames=1 / body_uploads=0 / release_abort=1`。此处 FPS 按连续拖动的事件处理耗时计算，不是空闲视口 FPS，也不是帧率承诺。`cloud` 样本平均每帧事件耗时约 110 ms，其中求交及来源追踪约 78 ms；节点准备约 20 ms、合并约 17 ms、trace interior 准备约 13 ms（子阶段包含在求交总时长内，不能重复相加），显示数组准备约 9 ms。**已验证支持形变网格的交互剖切，但该大模型样本尚未达到 30 FPS**；进一步提速应针对形变节点快照与来源追踪缓存。原始记录位于 `build/deformation-large/<mode>-3/drag.csv` 和 `run.log`。

迁移工程需要复刻“当前位置求交 + 原始有限元权重插值 + 几何版本失效”的一致性；目标 VisIt 仍完全不使用 OCCT。当前参考工程所用 OCCT 节点向量、图元、shader、structure 和选择功能见 [依赖清单](cut-view-manager-occt-api-inventory.md)。
