## FileExchange 模块概览

`FileExchange`（命令名 `med2md`）是 OCC Viewer 中的网格数据转换工具，用于把多种 CAE 网格输入（MED、Gmsh、由 `RWMeshConv` 支持的其它格式）统一写入 MDB（Mesh DataBase）格式。核心依赖：

- `RWMeshConv::MedConverter`：解析 / 写入调度器，提供节点排序、字段映射、临时属性缓存等能力。
- `RWCae::MeshData`：承载中间网格与场数据，保证结果可直接被 Viewer 与后处理模块消费。
- `Message` / `Message_PrinterOStream`：跨平台日志系统，可选择彩色 / 非彩色输出以适配不同终端。

## 支持的格式与流程

| 输入格式 | 解析方式 | 典型扩展名 | 说明 |
| --- | --- | --- | --- |
| MED | `-med` / 自动识别 | `.med` | 官方第一优先，支持 `-propsTempFolder` 写入属性缓存 |
| Gmsh | 自动识别 | `.msh` | 依赖 `RWMeshConv` 的通用读取器 |
| 其它（OBJ、STL…） | 自动识别 | 见 `RWMeshConv` 支持列表 | 只要 `RWMeshConv` 能读，就能写成 MDB |
| MDB | `-mdb` | `.mdb` | 目标格式；FileExchange 仅写 MDB，不读 MDB |

流程：加载输入 → 可选字段映射 → 可选节点 / 单元排序 → 写入 MDB → 将未处理字段写入 `-missing` 日志。

## 构建与运行

```powershell
cmake --build build --target FileExchange
.\build\bin\med2md.exe -input case.med -output case.mdb
```

### 常用命令示例

```powershell
# 显式 MED → MDB，并收集缺失字段
med2md -med pump.med -mdb pump.mdb -mapping fields.json -missing missing.txt

# 自动识别输入格式，排序节点/单元并压缩节点向量场
med2md -input mesh.msh -output mesh.mdb -sort -nodeVecAsScalar

# 指定属性临时目录，避免大 MED 文件占满系统盘
med2md -med huge.med -mdb huge.mdb -propsTempFolder D:\tmp\props
```

## 参数说明

| 参数 | 作用 | 备注 |
| --- | --- | --- |
| `-input/--input/-i <file>` | 自动识别输入格式 | 也可用位置参数（第一个无前缀参数） |
| `-med/--med <file>` | 明确声明 MED 输入 | 同时设置 `InputFormat = MED` |
| `-output/--output/-o <file>` | 指定输出 MDB | 也可用位置参数（第二个无前缀参数） |
| `-mdb/--mdb <file>` | 与 `-output` 等价，但固定 `OutputFormat = MDB` |
| `-mapping/--mapping <file>` | 字段映射 JSON | 未覆盖字段会写入 `-missing` |
| `-missing/--missing/-artifact <file>` | 记录未处理字段 | 可供后续补齐映射 |
| `-sort` | 对节点 / 单元及其字段排序 | 便于可重复构建 |
| `-nodeVecAsScalar` | 将节点向量场拆成若干标量 | 减少 MDB 体积，适合只关心模值 |
| `-propsTempFolder/--propsTempFolder/--props-temp-folder <dir>` | 属性缓存目录 | 当前仅对 MED 生效 |
| `-noColorizeLog/-noColorLog` | 禁用彩色日志 | 适用于不支持 ANSI 的终端 |
| `-help/--help/-h` | 打印帮助并退出 | 返回码 0 |

> 注意：同一类路径参数不可重复出现，重复声明即视为错误。

## 输出衔接

- 生成的 MDB 被 `RWCae`、`ViewCae` 等模块直接读取；字段命名完全由 `-mapping` 与输入文件决定。
- `-missing` 文件可作为补充映射的待办列表，也可被自动化脚本解析。
- `-propsTempFolder` 中的中间文件可在成功结束后安全删除。

## 调试与最佳实践

1. 推荐在 CI 中始终追加 `-sort`，确保相同输入在不同机器上输出一致，便于 diff。
2. 对于超大 MED 文件，使用 `-propsTempFolder` 指向高速本地盘可以显著缩短 I/O。
3. 如果终端不支持 ANSI 颜色，请加 `-noColorizeLog` 防止日志乱码。
4. 任何未知参数都会立即报错；使用 `med2md -h` 快速查看最新语法。
