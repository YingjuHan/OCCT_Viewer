[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / SelectionMeshEntity

# Class: SelectionMeshEntity

SelectionMeshEntity represents detected/selected info of [InteractiveCaeMesh](InteractiveCaeMesh.md) entity.

## Implements

- [`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)

## Constructors

### Constructor

> **new SelectionMeshEntity**(): `SelectionMeshEntity`

#### Returns

`SelectionMeshEntity`

## Properties

### entityId

> **entityId**: `number` = `-1`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`entityId`](../interfaces/ISelectionMeshEntity.md#entityid)

***

### entityType

> **entityType**: [`MeshEntityType`](../enumerations/MeshEntityType.md) = `MeshEntityType.Unknown`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`entityType`](../interfaces/ISelectionMeshEntity.md#entitytype)

***

### geomId

> **geomId**: `string` = `''`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`geomId`](../interfaces/ISelectionMeshEntity.md#geomid)

***

### hasRefEntity

> **hasRefEntity**: `boolean` = `false`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`hasRefEntity`](../interfaces/ISelectionMeshEntity.md#hasrefentity)

***

### isElemSurf

> **isElemSurf**: `boolean` = `false`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`isElemSurf`](../interfaces/ISelectionMeshEntity.md#iselemsurf)

***

### lineIndex

> **lineIndex**: `number` = `-1`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`lineIndex`](../interfaces/ISelectionMeshEntity.md#lineindex)

***

### pos

> **pos**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`pos`](../interfaces/ISelectionMeshEntity.md#pos)

***

### propsInfo

> **propsInfo**: `Map`\<`string`, [`IMeshPropertyValuesInfo`](../interfaces/IMeshPropertyValuesInfo.md)\>

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`propsInfo`](../interfaces/ISelectionMeshEntity.md#propsinfo)

***

### prsId

> **prsId**: `string` = `''`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`prsId`](../interfaces/ISelectionMeshEntity.md#prsid)

***

### prsInfo

> **prsInfo**: `string` = `''`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`prsInfo`](../interfaces/ISelectionMeshEntity.md#prsinfo)

***

### prsPropsInfo

> **prsPropsInfo**: `Map`\<`number`, [`IMeshPropertyInfo`](../interfaces/IMeshPropertyInfo.md)\>

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`prsPropsInfo`](../interfaces/ISelectionMeshEntity.md#prspropsinfo)

***

### refMeshData

> **refMeshData**: `ArrayBuffer` = `null`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`refMeshData`](../interfaces/ISelectionMeshEntity.md#refmeshdata)

***

### refMeshDataPath

> **refMeshDataPath**: `string`

#### Implementation of

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md).[`refMeshDataPath`](../interfaces/ISelectionMeshEntity.md#refmeshdatapath)
