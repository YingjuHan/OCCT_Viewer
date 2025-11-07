[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ISelectionMeshEntity

# Interface: ISelectionMeshEntity

ISelectionMeshEntity represents detected/selected info of [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) entity,
from `SelectionMeshEntity`.

## Properties

### entityId

> **entityId**: `number`

***

### entityType

> **entityType**: [`MeshEntityType`](../enumerations/MeshEntityType.md)

***

### geomId

> **geomId**: `string`

***

### hasRefEntity

> **hasRefEntity**: `boolean`

***

### isElemSurf

> **isElemSurf**: `boolean`

***

### lineIndex

> **lineIndex**: `number`

***

### pos

> **pos**: [`IVec3`](IVec3.md)

***

### propsInfo

> **propsInfo**: `Map`\<`string`, [`IMeshPropertyValuesInfo`](IMeshPropertyValuesInfo.md)\>

***

### prsId

> **prsId**: `string`

***

### prsInfo

> **prsInfo**: `string`

***

### prsPropsInfo

> **prsPropsInfo**: `Map`\<`number`, [`IMeshPropertyInfo`](IMeshPropertyInfo.md)\>

***

### refMeshData

> **refMeshData**: `ArrayBuffer`

***

### refMeshDataPath

> **refMeshDataPath**: `string`
