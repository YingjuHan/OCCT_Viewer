# STEP/IGES → Web Viewer 产物规范（v0）

本规范定义将 CAD（STEP/IGES）转换为 OCC_VIEWER Web 侧可加载资产的**交付格式**，用于：
- server-side 转换（推荐先落地）
- client-side（Wasm）直读 STEP/IGES 的补充缓存产物（可选）

目标：让 Web 侧可以在不解析 STEP/IGES 的前提下，获得**几何 + 装配树 + 属性 + 视图状态 +（预留）PMI**。

> 版本：`specVersion = "0.1"`（向后兼容优先；破坏性变更需升主版本）。

---

## 1. 产物目录结构（建议）

一个模型包（Model Package）建议为一个文件夹或 zip/tar：

```
<packageRoot>/
  manifest.json
  geometry/
    root.glb
    chunks/
      <nodeId>.glb
      ...
  metadata/
    scenegraph.json
    properties.json
    views.json
    pmi.json           # 预留
  thumbnails/
    <viewId>.png       # 可选
  artifacts/
    conversion.log     # 可选
    warnings.json      # 可选
```

说明：
- **最低交付**：`manifest.json` + `geometry/root.glb` + `metadata/scenegraph.json`
- `geometry/chunks/` 用于按需加载（装配大模型的关键）；可先只有 `root.glb`。

---

## 2. manifest.json（必需）

```json
{
  "specVersion": "0.1",
  "packageId": "demo-assembly-001",
  "source": {
    "format": "STEP",
    "fileName": "demo.step",
    "sha256": "<optional>"
  },
  "units": {
    "lengthUnit": "mm"
  },
  "coordinateSystem": {
    "up": "Z",
    "front": "Y",
    "handedness": "RH"
  },
  "geometry": {
    "root": "geometry/root.glb",
    "chunkFolder": "geometry/chunks",
    "chunking": {
      "enabled": false,
      "strategy": "by-assembly-node",
      "maxTrianglesPerChunk": 500000
    }
  },
  "metadata": {
    "scenegraph": "metadata/scenegraph.json",
    "properties": "metadata/properties.json",
    "views": "metadata/views.json",
    "pmi": "metadata/pmi.json"
  },
  "capabilities": {
    "hasAssembly": true,
    "hasPMI": false,
    "hasSavedViews": true
  }
}
```

字段约定：
- `units.lengthUnit`：`mm|cm|m|inch|ft`（最少支持这些；可扩展）
- `geometry.chunking.enabled=false` 时，Web 侧只加载 `geometry.root`
- `capabilities` 允许前端 UI 按能力降级显示

---

## 3. metadata/scenegraph.json（必需）

### 3.1 数据结构

```json
{
  "specVersion": "0.1",
  "rootNodeId": "root",
  "nodes": [
    {
      "id": "root",
      "name": "Demo Assembly",
      "type": "Assembly",
      "parentId": null,
      "children": ["part-001", "part-002"],
      "bbox": {
        "min": [0, 0, 0],
        "max": [10, 20, 30]
      },
      "visibility": {
        "defaultVisible": true
      },
      "geometryRef": {
        "kind": "glb",
        "path": "geometry/chunks/root.glb",
        "nodeNames": ["root"] 
      },
      "propsRef": "props-root"
    }
  ]
}
```

### 3.2 关键字段

- `id`：稳定且唯一；建议使用转换器生成的 UUID 或“装配路径 hash”
- `type`：`Assembly|Part|Instance|Body|FaceSet|Mesh`（可扩展）
- `children`：子节点 id 列表（顺序可用于 UI）
- `bbox.min/max`：模型坐标系 AABB，单位同 `manifest.units.lengthUnit`
- `visibility.defaultVisible`：默认可见性（支持视图状态覆盖）
- `geometryRef`：
  - `kind="glb"`：glTF/glb 文件
  - `path`：相对 `packageRoot` 的路径
  - `nodeNames`：glb 内需要显示的 glTF node 名称（可选；用于一个 glb 包含多个节点时的子集显示）
- `propsRef`：指向 `properties.json` 中的一条记录

---

## 4. metadata/properties.json（推荐）

```json
{
  "specVersion": "0.1",
  "items": [
    {
      "id": "props-root",
      "displayName": "Demo Assembly",
      "props": {
        "PartNumber": "A-001",
        "Material": "Steel",
        "Mass": 1.23
      },
      "tags": ["Top"]
    }
  ]
}
```

约定：
- `props` 的值必须是 JSON 标量或数组/对象（避免二进制）
- 数值单位建议在 `props` 中显式标注或在 `manifest.units` 中统一定义

---

## 5. metadata/views.json（推荐：Saved Views）

```json
{
  "specVersion": "0.1",
  "views": [
    {
      "id": "view-iso",
      "name": "ISO",
      "camera": {
        "projection": "Perspective",
        "fovyDeg": 35,
        "eye": [100, 120, 80],
        "at": [0, 0, 0],
        "up": [0, 0, 1]
      },
      "overrides": {
        "visibility": [
          { "nodeId": "part-001", "visible": true },
          { "nodeId": "part-002", "visible": false }
        ],
        "clipping": []
      },
      "thumbnail": "thumbnails/view-iso.png"
    }
  ]
}
```

约定：
- `overrides.visibility` 是可选覆盖；未出现节点使用默认可见性
- `clipping` 预留与 `InteractiveClipping` 对齐的参数（后续补 schema）

---

## 6. metadata/pmi.json（预留）

建议先定义最小结构，后续逐步补齐：

```json
{
  "specVersion": "0.1",
  "items": [
    {
      "id": "pmi-001",
      "type": "Dimension",
      "text": "⌀10",
      "anchor": { "nodeId": "part-001", "pos": [1,2,3] },
      "viewIds": ["view-iso"]
    }
  ]
}
```

---

## 7. 分块/流式建议（chunking）

### 7.1 最小可行策略

- 策略：`by-assembly-node`
- 每个 `SceneGraph node` 对应一个 `geometry/chunks/<nodeId>.glb`
- Web 侧按树展开/视锥/用户点击加载对应 chunk

### 7.2 推荐阈值（初始值）

- `maxTrianglesPerChunk`: 200k ~ 1M（按设备分档）
- `maxNodesPerChunk`: 1k

---

## 8. 与 OCC_VIEWER 的对接建议（实现提示）

- SceneGraph 先在 TS 层消费 `scenegraph.json` 生成树 UI（P0-003）
- 后续如果实现 C++ SceneGraph API（P0-002），建议返回与本规范兼容的 JSON 结构（至少字段一致）
- ObjectId 映射策略：
  - 若一个 node 对应一个 `InteractiveShape`：ObjectId = nodeId
  - 若一个 node 对应多个 shape：ObjectId = `${nodeId}::<subId>`（需保持稳定）

