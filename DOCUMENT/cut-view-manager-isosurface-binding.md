# 等值面的独立字段、派生量与 averaging

## 1. 使用方式

Manager 创建 Isosurface 后，Cut Field/component/value/step 是切面自己的绑定。
**Isosurface Options...** 打开该 cut 的独立选项窗口，支持 components、Mises、pressure、三主值、Tresca、
张量顺序和 engineering shear。选项窗口锁定打开时的 cut；改变云图变量或显示 averaging 不会重绑它。
使用 Cut Field 的显式编辑会重新绑定，并捕获当时的结果选项；单纯调整 threshold 只移动等值面。

```tcl
scvcutview model isoadd iso point stress 0 3
# 查询当前绑定的数值处理选项。
scvcutview model isobinding iso
# measure=1 Mises, layout=0 xx yy zz xy xz yz, shear=0,
# override100=1, saved threshold=25, invariant-before-averaging=0。
scvcutview model isobinding iso 1 0 0 1 25 0
scvcutisobinding model iso
```

Measure 编号：0 components，1 Mises，2 pressure，3/4/5 最大/中间/最小主值，6 Tresca。
Layout 编号：0 `xx yy zz xy xz yz`，1 `xx yy zz xy yz xz`，2 row-major，3 column-major。
区域参数为空表示一个区域；非空须提供每个原始单元的区域 ID。

## 2. 数据与计算契约

`MeshCutDefinition::IsoRecovery` 保存创建/显式编辑时的结果处理快照，`IsoAveragingOverride`
决定是否用 100% 覆盖其中的 threshold；后续 `RecoveryOptions` 变化只影响云图。
Override 仍保留区域边界。关闭 override 后使用保存的阈值，不从不断变化的 UI 全局变量隐式读取。

节点张量先按显式约定求派生标量，再以 FE 形函数插值这个标量。它不是在每个内部积分点对
原始张量重新求不变量；两者对非线性不变量一般不同，应在数据适配时确认语义。
单元张量支持先派生再平均，以及先平均分量再派生；单元标量支持独立阈值和区域恢复。
恢复后的值是 element-node traces，求交和界面两侧分类均用 owner 单元的迹；不会再做一次全局平均。
这种处理只构造切面标量，物理积分仍使用原始 FE 字段/provenance。

默认 legacy 单元标量绑定继续采用全局等权节点投影，保证旧文件/旧脚本行为。
高级不连续单元迹使用自研 `MeshGeometryVtkIso` 的原始 FE 参数图求根；支持范围和数值采样限额
沿用现有内核，不依赖外部 VTK。常值体域与等值值完全相等时属于 plateau，无法唯一构造体内等值面，明确拒绝。

Definitions JSON 10 保存完整 isoBinding；1..9 迁移为原始组件、100% override。
完整 session 自动携带该定义；时间帧切换重新解析同名/同映射字段并保留快照，显式区域跨重网格
要求重新绑定；错误、非法分量、非对称张量或候选预算失败保留旧定义。

多 cut 的 retained-body union 目前要求连续 component 等值面。节点派生量可以使用；
带不连续 averaging/区域或单元张量派生量的 retained-body 联合明确拒绝并要求单独显示，
避免联合分类悄悄改用原始组件。它是可查询的工程边界，不能声称任意多场不连续 cut 已完全等价。

## 3. 验证与 Linux 命令

`cut_iso` 包含共享面两侧单元值 2/6：threshold=0 不产生虚假的中间面，100% override 恢复连续等值面；
另测云图配置独立、Mises 已知平面位置、JSON10、非法配置回滚。
`draw_cut_view_iso_binding` 验证 Manager 入口和完整 session，`draw_cut_view_controls` 覆盖真实控件。

按 [Linux 运行指南](cut-view-manager-linux-run-test.md) 设置相应 Release/Debug 环境后：

```bash
export CUTVIEW_OUTPUT_DIR="$CUT_REPO/build/linux-results/iso-binding"
mkdir -p "$CUTVIEW_OUTPUT_DIR"
# Release 直接运行。
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_iso_binding_test.tcl"
# 切换到 linux-debug 的 LD_LIBRARY_PATH 和插件路径后调试。
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_iso_binding_test.tcl"
ctest --test-dir "$CUT_BUILD" -R '^(cut_iso|cut_view|native_timeline|native_session|draw_cut_view_iso_binding)$' --output-on-failure
```

## 4. 无 OCCT 迁移

目标 VisIt 工程不使用 OCCT：将切面标量快照作为独立数据对象，张量求值、区域恢复与 FE shape functions
用自有向量/矩阵实现。局部求根、采样误差与空间索引均保留自研实现；不要把 `vtkDataSet` 管线作为新依赖。
目标显示组/多域的 owner ID 必须参与恢复区域与缓存键。文件版本迁移必须保留旧标量行为，
不能在升级时把旧等值面重新绑定到当前云图。

## 5. 百万单元验收

`cut_view_iso_binding_large.tcl` 使用 `build/large-vtk-deformed/hex_150.vtu`，
3,375,000 个六面体、3,442,951 个节点。temperature 为 `x+2y+3z`，
阈值 450.25 和 450.75 均实际生成 44,999 个面单元；改变云图和平均设置不改变绑定。
Windows Release 不挂 GDB，求交、切换云图和阈值的整个命令批次为 22,557.905 ms，
source/result 逻辑保留为 1,460,965,248 / 737,329,152 字节，4/16 GiB 限额不变。
记录见 `build/cut-iso-binding-large/run.log`。这是 CPU 等值面批次耗时，不是鼠标 FPS；
一般等值面仍需扫描源单元，不能把平面 GPU clipping 的交互性能套用到它。
仅显示切面时按线性单元节点标量包围区间估计候选预算，高阶/不连续迹不套用这个界限。

Linux 直接运行：`"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_iso_binding_large.tcl"`；
调试：切换 Debug 插件和动态库环境后，`gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_iso_binding_large.tcl"`。
