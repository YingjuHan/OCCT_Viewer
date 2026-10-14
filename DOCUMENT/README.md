# Cut View Manager 工程文档

本文档集以当前 OCCTViewer 源码和 `D:\workspace\visit` 定制分支为依据，面向将 Cut View Manager 迁移到 CentOS 7.9 CAE 后处理工具的工程人员。

| 文档 | 内容与阅读顺序 |
| --- | --- |
| [架构、设计、算法与实现](cut-view-manager-migration-guide.md) | 先读：源网格、有限元来源、求交、BVH、缓存、GPU、物理计算及代码入口 |
| [OCCT 功能和依赖清单](cut-view-manager-occt-api-inventory.md) | 按功能核对 OCCT 的职责、调用位置、迁移时的保留或替换方案；含可重新生成的直接依赖清单 |
| [定制 VisIt / CentOS 7.9 迁移实施手册](cut-view-manager-visit-porting.md) | 再读：目标分支的实际入口、接口草案、线程与进程边界、ABI、工作包和验收标准 |

“已实现”指 OCCTViewer 当前代码；“目标源码已确认”指只读检查 VisIt 得出的事实；“迁移建议”与示例接口尚未在目标工程实现。本文档不表示迁移已完成。

更新 OCCT 附录的自动清单：

```powershell
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py
python -B -X utf8 DOCUMENT/tools/build_cut_view_occt_inventory.py --check
```

新增 Cut 相关源文件、改变模块边界或 OCCT 调用后，同时更新手工说明和脚本扫描范围。自动清单只证明指定源文件的直接头文件依赖，不能代替算法与调用链审查。
