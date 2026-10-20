# 完整会话、公共物理模板与多视口

## 会话版本

Native session 4 包含 definitions 13、当前 plot style、三部分默认样式和单 cut 覆盖、独立等值变量/恢复快照、材料参考构形、切片集合、自动来源、时间轴映射与公共物理模板。
源文件 bundle 和 SHA-256 身份验证继续沿用既有格式；不把旧源指纹改成新含义。
session 1/2/3 继续读取，缺失的新字段使用对应默认值；版本 4 的 `plotStyle` 和 `physicsDefault` 必须存在。
错误输入在临时 facade 完成验证前不替换现场。只保存 definitions 仍不包含样式、模板和 Viewer 状态。

```tcl
scvcutview model sessionsave /absolute/path/session.json
scvcutview model sessionload /absolute/path/session.json
```

session 保存相机状态，加载到当前绑定视口；进程内 View ID 不可跨运行持久化。
不会自动创建用户的所有窗口，多个视口分别保存自己的 session。

## 公共自由体模板

Free Body Options 提供 Capture common template 和 Use common template。
模板是配置快照，独立于每 cut 的请求、报告和开关；改变模板不覆盖已有请求。
新 cut 首次打开物理选项时，若有模板，则真实验证其截面、字段、单位和域后创建关闭的请求。
用户确认 Enable 后才显示报告和向量。缺字段、空域或不适用截面会报错，不能回退到全模型。

```tcl
# 从已配置的请求捕获；清除不会删除各 cut 的覆盖。
scvcutview model physicsdefault viewcut/p
scvcutview model physicsapplydefault q viewcut/q 0
scvcutview model physicsdefault
scvcutview model physicsdefault clear
```

模板不保存一个固定 cut ID；原 cut 删除后仍可应用。
加载时检查绑定 schema，应用时进行完整 FE 截面积分验证，即使请求默认关闭。
时间帧重绑定模板字段和复合层；重网格遇到显式单元/截面/法向映射时拒绝自动复用，须重新配置模板。

## 数据共享与显示隔离

每个 `NativeCutView` 绑定一个视口；共用一个 `MeshData` 不共用可变 cut/BVH/clip/缓存结果/选择。
DRAW 的 `scvcutview second share first` 只共享源数据，再独立创建 manager、时间轴和 session。
源字段应当视为只读；原地修改共享数据时，调用者必须通知每个消费者失效。

`displayOwned` 在 Display 前后清除其他视口的 affinity，再设置目标视口；
OCCT 首次注册对象会重置 mask，必须在首次 Display 后、任何 redraw 前重新限定。
主模型、图例、手柄、操纵器、标注、CAD 截线、自由体箭头使用同一路径。
GPU 常驻主体的 `Graphic3d_CStructure::ViewAffinity` 与所属 AIS presentation 共享；暖切面移动不会因此重建主体。
全局 clip 加到绑定的 `V3d_View`，不修改另一个窗口。
不指定 viewport 的低层 Attach 保持原有 context 全视口显示语义，应用需要隔离时必须传入目标视口。

Manager 仍是一个窗口，其 selected/motion/axis/sensitivity/gizmo 按模型实例和绑定视口暂存。
关闭窗口不改变激活状态；会话加载替换模型实例后重新解析可用定义。
保存/恢复 B 的 session 不更改 A 的 cut 定义、样式、相机或缓存。

## Linux 运行和回归

先配置 [Linux 运行环境](cut-view-manager-linux-run-test.md)，分别选择 Release 或 Debug 构建。

```bash
export CUTVIEW_OUTPUT_DIR="$CUT_BUILD/scope-demo"
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_scope_test.tcl"
gdb --args "$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_scope_test.tcl"
ctest --test-dir "$CUT_BUILD" -R '^(native_analysis|native_session|native_timeline|draw_cut_view_scope|draw_cut_view_free_body)$' --output-on-failure
"$CUT_DRAWEXE" -i -f "$CUT_REPO/test/cae/cut_view_scope_large.tcl"
```

VisIt 迁移完全不使用 OCCT。以目标窗口/renderer 的 layer ownership 实现 affinity，
GPU buffer 归属 renderer；共享只读数据、独立可变加速结构和事务发布的边界保持一致。
