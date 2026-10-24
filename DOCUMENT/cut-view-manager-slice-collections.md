# 切片集合、路径采样与自由体扫描

## 状态与生命周期

`MeshCutSlicing` 生成空间平面快照，`MeshCutViewManager::ReplaceSlices` 逐片预验证后
原子替换命名集合。集合成员保存源 cut、坐标、区间和单位，Definitions JSON 11 及完整 session
均保存这些数据。替换不会不断追加旧切片；删除集合只清理成员；删除源 cut 连带清理其集合。
成员不能单独删除，复制成员则变为独立手工 cut。

新集合默认不显示，便于隐藏扫描。`EnableSlices` 要求只有源 cut 激活；显示时以成员代替源 cut，
源定义和原有 mask 保留，关闭集合即恢复。路径/范围是创建时的快照，移动源 cut 不重新定位切片。
当前集合仅支持空间平面；材料随动平面需先改回 Spatial，避免伪装成正确的材料路径映射。

## 使用

Manager 的 Time / Slicing 入口可生成集合、开关显示、移除、扫描及导出 CSV。
范围和规则距离使用模型长度单位，旋转使用弧度；path 为节点采样，path_regular 为等弧长采样。
路径由折线构成，采用 parallel transport 稳定平面内轴；重复点、退化方向、平行初始 up 会报错。
折线转角处采用相邻采样点的弦方向，不宣称是光滑曲线的精确 Frenet frame。

```tcl
scvcutview model slicing plane range -10 10 5
scvcutview model slicelist
scvcutview model sliceenable plane_range 1
scvcutview model slicing plane path_regular 5 0 0 1 0 0 0 1 0 0 1 2 0
# requestId 来自该源 cut 的 Free Body 配置；也可传完整显式 JSON。
set csv [scvcutview model slicescan plane_range viewcut/plane 1]
scvcutview model sliceremove plane_range
```

扫描在独立临时 `NativeCutView` 中逐片执行，复用实体/壳/梁/热流、集合、单位、输出 frame 和法向契约。
sumOnPath=1 用路径位置作为求和点，=0 使用请求自己的 centroid/user 策略。
默认 CSV 每切片一行合量，显式 Distributed 请求才导出采样贡献。
全部成功后才返回 CSV；失败不发布半份报告，也不改变现场的显示、物理请求或定义。
临时管理器共享预算账本，源数据共享，但几何缓存按视图独立；并非无限内存扫描。

## Linux 运行、调试与迁移

先按 [Linux 指南](cut-view-manager-linux-run-test.md) 配置 Release/Debug 环境。

```bash
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_slices_test.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_slices_test.tcl"
ctest --test-dir "$CUT_BUILD" -R '^(cut_slicing|native_analysis|native_session|native_timeline|draw_cut_view_slices|draw_cut_view_time)$' --output-on-failure
# 性能单独跑，Release、不挂 GDB。
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_slices_large.tcl"
```

目标 VisIt 完全不使用 OCCT：移植集合状态、弧长采样、transport 和逐片事务，
用自有数学类型和 BVH/FE 内核替换源项目类型；复用目标已有渲染线程发布机制。

## 验收记录

Windows Release 构建通过，native 的 slicing/analysis/session/timeline 和两个 DRAW 窗口回归通过。
337.5 万六面体创建、显示三切片并逐片扫描、移除集合的批次耗时 8,699.519 ms；
逻辑 source/result 为 1,460,965,248 / 921,862,144 字节，限额仍为 4/16 GiB。
大数据用 displacement 的显式重复组件映射测试积分吞吐，属于合成性能输入，不是物理应力。
记录：`build/a06-tests.log`、`build/a06-ui-tests.log`、`build/cut-slices-large/run.log`。
