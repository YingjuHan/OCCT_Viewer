# Cut View Manager 工程文档

本文档集以当前 OCCTViewer 源码和 `D:\workspace\visit` 定制分支为依据，面向将 Cut View Manager 迁移到 CentOS 7.9 CAE 后处理工具的工程人员。

| 文档 | 内容与阅读顺序 |
| --- | --- |
| [Abaqus 对标需求与开发台账](cut-view-manager-abaqus-parity.md) | 官方行为基线、26 项差距与验收条件、A01–A10 开发顺序、真正随变形截面的设计约束 |
| [平面坐标系与绝对运动](cut-view-manager-plane-motion.md) | Key-in/CSYS、原点和面内轴、局部绝对旋转、精细平移、JSON 8 兼容与 Windows/Linux 测试入口 |
| [参考构形与随材料变形截面](cut-view-manager-material-sections.md) | Follow model deformation、固定参考 step、有限元权重映射、两侧分类、缓存、JSON 9 与运行入口 |
| [Linux 直接运行、GDB 调试与测试](cut-view-manager-linux-run-test.md) | 本项目现有 Cut View Manager 的 Release / Debug 构建、DRAW 启动、CTest、337.5 万单元拖动与接缝测试命令 |
| [6×6 正弦薄板回归与交互复现](cut-view-manager-sine-plate-test.md) | 单层六面体、temperature=z、周期采样、分离截面、两侧体积与 GPU/CPU 比较，含 Windows / Linux 命令 |
| [云图、线框与实时平面切割](cut-view-manager-display-modes.md) | 可执行 Tcl、Manager 的 Display 切换、GPU 边线裁剪、截面周边线、Linux 直接/调试运行与 337.5 万单元拖动测试 |
| [形变网格的实时剖切、云图与线框](cut-view-manager-deformation.md) | 位移倍率、当前几何与有限元场插值、形变缓存修复、Windows / Linux 直接和调试命令、百万单元验收 |
| [架构、设计、算法与实现](cut-view-manager-migration-guide.md) | 先读：源网格、有限元来源、求交、BVH、缓存、GPU、物理计算及代码入口 |
| [OCCT 功能和依赖清单](cut-view-manager-occt-api-inventory.md) | 源项目的 OCCT 职责、调用位置与目标迁移时的替换责任；含可重新生成的直接依赖清单 |
| [无 OCCT 的 VisIt / CentOS 7.9 迁移实施手册](cut-view-manager-visit-porting.md) | 再读：目标完全不使用 OCCT；包含实际入口、接口草案、替换表、ABI、工作包和验收标准 |

“已实现”指 OCCTViewer 当前代码；“目标源码已确认”指只读检查 VisIt 得出的事实；“迁移建议”与示例接口尚未在目标工程实现。本文档不表示迁移已完成。

Linux 命令针对当前依赖 OCCT 的参考项目；目标 VisIt 迁移产物完全不使用 OCCT。这两个运行环境与依赖约束分别说明。

更新 OCCT 附录的自动清单：

```powershell
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py --check
```

新增 Cut 相关源文件、改变模块边界或 OCCT 调用后，同时更新手工说明和脚本扫描范围。自动清单只证明指定源文件的直接头文件依赖，不能代替算法与调用链审查。
