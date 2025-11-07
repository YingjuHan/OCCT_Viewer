[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / CaeMeshInfo

# Class: CaeMeshInfo

Represents mesh data info of [InteractiveCaeMesh](InteractiveCaeMesh.md) entity.

## Implements

- [`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md)

## Constructors

### Constructor

> **new CaeMeshInfo**(): `CaeMeshInfo`

#### Returns

`CaeMeshInfo`

## Properties

### boundsMax

> **boundsMax**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`boundsMax`](../interfaces/ICaeMeshInfo.md#boundsmax)

***

### boundsMin

> **boundsMin**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`boundsMin`](../interfaces/ICaeMeshInfo.md#boundsmin)

***

### nbElems0d

> **nbElems0d**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbElems0d`](../interfaces/ICaeMeshInfo.md#nbelems0d)

***

### nbElems1d

> **nbElems1d**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbElems1d`](../interfaces/ICaeMeshInfo.md#nbelems1d)

***

### nbElems2d

> **nbElems2d**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbElems2d`](../interfaces/ICaeMeshInfo.md#nbelems2d)

***

### nbElems3d

> **nbElems3d**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbElems3d`](../interfaces/ICaeMeshInfo.md#nbelems3d)

***

### nbHexahedrons

> **nbHexahedrons**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbHexahedrons`](../interfaces/ICaeMeshInfo.md#nbhexahedrons)

***

### nbHexahedrons2nd

> **nbHexahedrons2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbHexahedrons2nd`](../interfaces/ICaeMeshInfo.md#nbhexahedrons2nd)

***

### nbNodes

> **nbNodes**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbNodes`](../interfaces/ICaeMeshInfo.md#nbnodes)

***

### nbPolygons

> **nbPolygons**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPolygons`](../interfaces/ICaeMeshInfo.md#nbpolygons)

***

### nbPolyhedrons

> **nbPolyhedrons**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPolyhedrons`](../interfaces/ICaeMeshInfo.md#nbpolyhedrons)

***

### nbPrisms

> **nbPrisms**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPrisms`](../interfaces/ICaeMeshInfo.md#nbprisms)

***

### nbPrisms2nd

> **nbPrisms2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPrisms2nd`](../interfaces/ICaeMeshInfo.md#nbprisms2nd)

***

### nbPyramids

> **nbPyramids**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPyramids`](../interfaces/ICaeMeshInfo.md#nbpyramids)

***

### nbPyramids2nd

> **nbPyramids2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbPyramids2nd`](../interfaces/ICaeMeshInfo.md#nbpyramids2nd)

***

### nbQuads

> **nbQuads**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbQuads`](../interfaces/ICaeMeshInfo.md#nbquads)

***

### nbQuads2nd

> **nbQuads2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbQuads2nd`](../interfaces/ICaeMeshInfo.md#nbquads2nd)

***

### nbTetrahedrons

> **nbTetrahedrons**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbTetrahedrons`](../interfaces/ICaeMeshInfo.md#nbtetrahedrons)

***

### nbTetrahedrons2nd

> **nbTetrahedrons2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbTetrahedrons2nd`](../interfaces/ICaeMeshInfo.md#nbtetrahedrons2nd)

***

### nbTriangles

> **nbTriangles**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbTriangles`](../interfaces/ICaeMeshInfo.md#nbtriangles)

***

### nbTriangles2nd

> **nbTriangles2nd**: `number` = `0`

#### Implementation of

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md).[`nbTriangles2nd`](../interfaces/ICaeMeshInfo.md#nbtriangles2nd)

## Methods

### formatInfo()

> `static` **formatInfo**(`theInfo`, `theUMgr`): `string`

Format mesh info string.

#### Parameters

##### theInfo

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md)

##### theUMgr

[`UnitManager`](../interfaces/UnitManager.md) = `null`

#### Returns

`string`
