# Cut View Manager 性能优化任务清单

更新时间：2026-10-24。执行顺序：T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08 → T09 → T10。

每项完成后执行针对性验证、独立 Git 提交、刷新 codebase-memory 索引，再自动开始下一项。提交号在下一次清单更新中补录，避免自引用提交号。未经实测不记录性能提升；功能通过与性能达标分别记录。

## 当前状态

- 已有：GPU 平面 clipping、CPU slice、分块并行六面体求交、保守的平移拓扑缓存、请求取消检查及双 GPU 测试入口。
- 基线：337.5 万 hex；MX450 旋转 / 变形平移的 BVH 约 2.55 / 2.36 ms，merge 约 19.04 / 15.91 ms，trace 约 25.09 / 19.96 ms。merge 包含回退求交，需进一步拆分。
- 正在执行：T08。

## 任务

| ID | 优先级 | 状态 | 工作及验收 | 提交 |
| --- | --- | --- | --- | --- |
| T01 | P0 | 已完成 | 拆分输出追加、共享点处理和 fallback 计时；补充回退原因、拓扑缓存命中及逐帧 CSV；建立百万级基线。 | c0b00f2 |
| T02 | P0 | 已完成 | 连续数组输出：并行计数、前缀和、预分配、独立填充；保持共享边交点一致及有限元来源正确，减少串行追加。 | 252aae6 |
| T03 | P0 | 已完成 | 缓存固定变形构形，复用只读节点坐标与 BVH；网格、场、倍率或时间步变化时正确失效，缓存纳入预算。 | 2f3c5f8 |
| T04 | P0 | 已完成 | 复用求交携带的原始边、权重和单元信息，精简 trace/场值映射；保留共面界面双侧物理语义。 | 788b61e |
| T05 | P1 | 已完成 | 补齐四面体、棱柱、金字塔等线性单元并行路径；复杂单元保守回退，减少整批串行处理。 | 6fb5398 |
| T06 | P1 | 已完成（实验默认关闭） | 扩展固定法向平移增量更新；覆盖跨单元事件、反向拖动、大步跳转与共面退化。 | b4e1337 |
| T07 | P1 | 已完成 | 复用显示暂存数组、批量生成面和边；稳定拓扑复用索引，更新有效绘制数量，保持主体零上传。 | — |
| T08 | P1 | 进行中 | 百万级四面体、混合、畸变网格数据与性能覆盖；云图、线框、组合显示、变形均验证。 | — |
| T09 | P2 | 待开始 | 根据剩余瓶颈比较 BVH 布局、SIMD/并行遍历及建树策略；用实际平面轨迹决定是否启用。 | — |
| T10 | 收敛 | 待开始 | 完整功能回归、双卡长样本、图像和内存核验；更新报告，明确达标项及限制。 | — |

## 统一验收

- Release、直接运行 DRAW，不挂 GDB；以 337.5 万单元基线及新增百万级非规则模型为主要性能样本。
- MX450、Iris Xe 分别三轮，每场景每轮 60 帧；报告 FPS、p50、p95、CPU 阶段、上传字节及缓存情况。
- 性能目标：FPS ≥ 15 且 p95 ≤ 67 ms；保持现有 source/result 预算，不以增加预算掩盖退化。
- 验证共面、近顶点/边、正弦薄板、共享界面、原 FE 场值及变形；body 与 slice 配对发布，不能产生空洞或接缝。
- 只优化平面实时切割；非平面 cut 行为不改，不引入外部 VTK 依赖。

## 执行记录

- 2026-10-24：创建清单。codebase-memory 通过本地 CLI 访问；索引标记不完整或元数据变化的实现直接核对源码。

### T01：计时与基线

- Release 构建通过；`cut_view`、`cut_sine_plate`、`cut_cache`、`cut_trace_binding`、`cut_interface` 五项回归通过。
- 新增 `shared_point_ms`、`append_ms`、`fallback_ms`（均为 `merge_ms` 子项）、五类互斥回退原因及 `topology_hits`；同步导出 renderinfo、API 基准和鼠标基准 CSV。
- 337.5 万 hex / MX450 独立单轮，每场景 60 帧：平移 23.02 FPS / p95 72.22 ms；旋转 11.81 FPS / p95 100.19 ms；固定变形平移 10.50 FPS / p95 119.55 ms。三场景均未同时满足 FPS/p95 门槛；本轮建立基线，不声称优化收益。
- [原始 CSV](test-results/cut-view-performance-2026-10-24/t01/frames/) 与 [统计](test-results/cut-view-performance-2026-10-24/t01/summary.json) 已归档。先前与 CPU 回归重叠的一轮仅保留在 build，不纳入基线。
- 复核命令：`python test/analyze-cut-profile.py build/perf-t01-mx450-isolated --output build/perf-t01-mx450-isolated/profile.json`。脚本拒绝回退计数不一致、merge 子阶段重叠、主体重建或重传。

### T02：批量截面输出

- 快速截面按块预计算节点及三角形/四边形/多边形的独立输出范围，先完成容器分配，再并行写入；共享边统一编号，回退单元复用相同边键。暂存量仍受 4096 单元块限制。
- `cut_slicing` 升级到 100 万 hex，验证多块、共面、快慢路径混合、无重复共享节点、解析截面积及全部节点原 FE 值；与 `cut_view`、`cut_trace_binding`、`cut_interface` 四项均通过。
- MX450 / 337.5 万 hex / 单轮 60 帧：平移 30.78 FPS / p95 59.13 ms；旋转 13.06 / 85.17 ms；固定变形平移 11.56 / 95.03 ms。仅平移本轮达标；最终双卡三轮验证见 T10，不由单轮结果推断稳定收益。
- 旋转 merge 本轮均值 14.94 ms，shared point 9.69 ms，append 4.51 ms，fallback 0.52 ms。下一项优先消除固定变形的重复节点准备。
- [原始 CSV](test-results/cut-view-performance-2026-10-24/t02/frames/) 与 [统计](test-results/cut-view-performance-2026-10-24/t02/summary.json)。

### T03：固定变形坐标快照

- 修改器的节点坐标与源 BVH 同时准备、只读复用、同时失效；工作线程读取快照，不再逐帧在调用线程执行位移采样。快照按最多每个 cut 三条 root 加公共 source 保守计入 source 预算。
- 百万级节点快照复用、倍率变化失效、旧快照保活及零变形不分配均验证；cut_slicing、cut_view、cut_material、cut_cache、cut_interface、DRAW deformation 六项通过。原 source 预算断言调整为与含快照的新估算一致，继续拒绝旧 epoch 保留。
- MX450 / 337.5 万 hex / 单轮 60 帧：平移 32.82 FPS / p95 58.63 ms；旋转 12.94 / 84.48 ms；固定变形平移 13.50 / 84.59 ms。变形 node_prepare 均值从上一阶段约 16 ms 降到近零，坐标读取移入并行 cell_cut；完整帧仍未达标。
- [原始 CSV](test-results/cut-view-performance-2026-10-24/t03/frames/) 与 [统计](test-results/cut-view-performance-2026-10-24/t03/summary.json)。

### T04：trace 来源和内部单元处理

- 可直接使用原始 FE 边权重的截面不再建立中间来源副本；复用每个截面单元的连接暂存空间。
- 已验证内部截面采用无共享修改的并行 trace 生成，直接计算原单元内部点和 FE 支撑，跳过内部单元 hash 缓存准备。共面/多 owner 仍走原有双侧拓扑逻辑。
- 来源映射保持独立所有权，复用现有记录容量；工作线程只修改已分配的独立值，不改变哈希表结构。避免构造会分配内存的默认节点临时值。
- cut_slicing、cut_view、cut_trace_binding、cut_interface、cut_physics、cut_material、cut_union 七项通过；百万级用例增加解析单元内部点及 FE 支撑检查。
- MX450 单轮 60 帧 / crossing：30.73 FPS，p95 51.75 ms，trace 4.88 ms，target=True。
- MX450 单轮 60 帧 / deformed：14.77 FPS，p95 72.71 ms，trace 9.38 ms，target=False。
- MX450 单轮 60 帧 / rotating：14.32 FPS，p95 76.94 ms，trace 10.51 ms，target=False。
- [原始 CSV](test-results/cut-view-performance-2026-10-24/t04/frames/) 与 [统计](test-results/cut-view-performance-2026-10-24/t04/summary.json)。

### T05：线性单元并行覆盖

- 并行输入、边求交与批量输出统一覆盖 tetra、仿射 hex、平移型 prism、平面凸底 pyramid；检查体积退化、四边面共面性和凸性。其他几何、近节点及近端点仍进入原通用轮廓路径。
- 新增 `cut_linear_batch`：129.6 万 tetra、102.4 万 prism、105.3696 万 pyramid，逐节点验证平面残差、原 FE temperature，检查完整解析面积和回退原因；均命中并行批量路径。
- cut_linear_batch、cut_slicing、cut_view、cut_interface、vtk_cell_cut、cut_trace_binding 六项通过。原有 84 项加新用例为 85 项；T10 执行全量。
- [百万级原生参考输出](test-results/cut-view-performance-2026-10-24/t05/linear-reference.log) 中 elapsed_ms 包含生成、求交及数值检查，不是 GPU FPS；对应文件导入和绘制性能留给 T08。

### T06：单元拓扑增量实验

- 在同一源 BVH/变形 epoch 内缓存单元的顶点侧别与有序原始边；侧别不变只更新交点/权重，变化时重建该单元拓扑。支持平移和小角度旋转；共面、近端点及非凸/非仿射条件保守回退。
- 固定 65,536 槽位，碰撞只降低命中率；额外 result 预算保守计入 2 MiB。源 BVH 改变清空缓存，不引用旧几何的坐标。变形链的批量内存池判断改为检查源 root。
- 百万级 tetra/prism/pyramid 增加小角度旋转、反向及大步平移，逐节点 FE 与完整面积检查通过；六项针对性回归通过。
- 实验开关 `OCCTV_CUT_CELL_TOPOLOGY_CACHE=1`，默认关闭；`cut_linear_batch` 的 CTest 环境显式启用，其他回归验证默认路径。命中率不等于性能收益，单轮完整帧未证明稳定提升，因此不增加默认路径复杂度。
- 下列 [原始 CSV](test-results/cut-view-performance-2026-10-24/t06/frames/) 和 [统计](test-results/cut-view-performance-2026-10-24/t06/summary.json) 是**启用实验路径**的 MX450 单轮 60 帧测量；T07/T10 恢复默认关闭。
- crossing：31.27 FPS，p95 54.16 ms，cell hit 0.0%，target=True。
- deformed：14.68 FPS，p95 74.20 ms，cell hit 67.2%，target=False。
- rotating：14.30 FPS，p95 75.34 ms，cell hit 49.6%，target=False。

### T07：显示准备与来源暂存

- 显示节点、UV、三角形/边数组及范围描述复用容量；先确定各多边形独立范围，再并行生成面与轮廓边，避免逐图元 push_back 和重复边界归约。
- 原 FE 映射可直接读取时不再逐节点构造来源链；变形显示读取同 epoch 坐标快照，保留双精度重构及共同局部 GPU 坐标系。边来源暂存从多次小分配改为一次精确两权重分配。
- 源 FE/显示暂存仍独立于已发布 GPU 帧；失败检查在配对发布之前，主体不重传，截面绘制数量保持原更新协议。
- cut_view、cut_display、cut_slicing、cut_linear_batch、DRAW gpu_plane/display_modes/deformation 七项通过。与 T04 三场景首末帧共六对图像逐像素完全一致，记录见 [图像核验](test-results/cut-view-performance-2026-10-24/t07/image-comparison.json)。
- MX450 单轮 60 帧 / crossing：33.02 FPS，p95 45.74 ms，target=True。
- MX450 单轮 60 帧 / deformed：16.30 FPS，p95 66.79 ms，target=True。
- MX450 单轮 60 帧 / rotating：15.65 FPS，p95 68.91 ms，target=False。
- [原始 CSV](test-results/cut-view-performance-2026-10-24/t07/frames/) 与 [统计](test-results/cut-view-performance-2026-10-24/t07/summary.json)。旋转平均 FPS 已超过 15，但 p95 仍未达标；最终判断使用 T10 双卡三轮数据。
