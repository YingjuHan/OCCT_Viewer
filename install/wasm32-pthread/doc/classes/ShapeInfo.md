[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ShapeInfo

# Class: ShapeInfo

Describes information about TopoDS_Shape.

## Implements

- [`IShapeInfo`](../interfaces/IShapeInfo.md)

## Constructors

### Constructor

> **new ShapeInfo**(): `ShapeInfo`

#### Returns

`ShapeInfo`

## Properties

### checked

> **checked**: `boolean` = `false`

Checked flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`checked`](../interfaces/IShapeInfo.md#checked)

***

### closed

> **closed**: `boolean` = `false`

Closed flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`closed`](../interfaces/IShapeInfo.md#closed)

***

### color

> **color**: `string` = `''`

Subshape color.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`color`](../interfaces/IShapeInfo.md#color)

***

### convex

> **convex**: `boolean` = `false`

Convex flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`convex`](../interfaces/IShapeInfo.md#convex)

***

### curveType

> **curveType**: [`ShapeCurveType`](../enumerations/ShapeCurveType.md) = `ShapeCurveType.OtherCurve`

Geometry curve type (for EDGE shape type).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`curveType`](../interfaces/IShapeInfo.md#curvetype)

***

### free

> **free**: `boolean` = `false`

Free flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`free`](../interfaces/IShapeInfo.md#free)

***

### hidden

> **hidden**: `boolean` = `false`

Subshape hidden flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`hidden`](../interfaces/IShapeInfo.md#hidden)

***

### index

> **index**: `number` = `-1`

Selection index within subshapes of the same type.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`index`](../interfaces/IShapeInfo.md#index)

***

### infinite

> **infinite**: `boolean` = `false`

Infinite flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`infinite`](../interfaces/IShapeInfo.md#infinite)

***

### location

> **location**: [`ITrsf`](../interfaces/ITrsf.md)

Shape location.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`location`](../interfaces/IShapeInfo.md#location)

***

### locked

> **locked**: `boolean` = `false`

Locked flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`locked`](../interfaces/IShapeInfo.md#locked)

***

### modified

> **modified**: `boolean` = `false`

Modified flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`modified`](../interfaces/IShapeInfo.md#modified)

***

### nbChildren

> **nbChildren**: `number` = `0`

Number of direct subshapes.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`nbChildren`](../interfaces/IShapeInfo.md#nbchildren)

***

### nbPolySegments

> **nbPolySegments**: `number` = `0`

Number of tessellation segments (for EDGE shapes).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`nbPolySegments`](../interfaces/IShapeInfo.md#nbpolysegments)

***

### nbTriangles

> **nbTriangles**: `number` = `0`

Number of triangles (for FACE shape type).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`nbTriangles`](../interfaces/IShapeInfo.md#nbtriangles)

***

### nbTriNodes

> **nbTriNodes**: `number` = `0`

Number of triangle nodes (for FACE shape type).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`nbTriNodes`](../interfaces/IShapeInfo.md#nbtrinodes)

***

### objectId

> **objectId**: `string` = `''`

Presentation object id.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`objectId`](../interfaces/IShapeInfo.md#objectid)

***

### orientable

> **orientable**: `boolean` = `false`

Orientable flag.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`orientable`](../interfaces/IShapeInfo.md#orientable)

***

### orientation

> **orientation**: [`ShapeOrientation`](../enumerations/ShapeOrientation.md) = `ShapeOrientation.EXTERNAL`

Shape orientation.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`orientation`](../interfaces/IShapeInfo.md#orientation)

***

### point

> **point**: [`IVec3`](../interfaces/IVec3.md)

Vertex position (for VERTEX shape type).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`point`](../interfaces/IShapeInfo.md#point)

***

### shapeType

> **shapeType**: [`ShapeEnum`](../enumerations/ShapeEnum.md) = `ShapeEnum.SHAPE`

Shape type.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`shapeType`](../interfaces/IShapeInfo.md#shapetype)

***

### surfaceType

> **surfaceType**: [`ShapeSurfaceType`](../enumerations/ShapeSurfaceType.md) = `ShapeSurfaceType.OtherSurface`

Geometry surface type (for FACE shape type).

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`surfaceType`](../interfaces/IShapeInfo.md#surfacetype)

## Accessors

### compSolids

#### Get Signature

> **get** **compSolids**(): `number`[]

Return indices of defined composite solid subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined composite solid subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`compSolids`](../interfaces/IShapeInfo.md#compsolids)

***

### edges

#### Get Signature

> **get** **edges**(): `number`[]

Return indices of defined edge subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined edge subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`edges`](../interfaces/IShapeInfo.md#edges)

***

### faces

#### Get Signature

> **get** **faces**(): `number`[]

Return indices of defined face subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined face subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`faces`](../interfaces/IShapeInfo.md#faces)

***

### shells

#### Get Signature

> **get** **shells**(): `number`[]

Return indices of defined shell subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined shell subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`shells`](../interfaces/IShapeInfo.md#shells)

***

### solids

#### Get Signature

> **get** **solids**(): `number`[]

Return indices of defined solid subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined solid subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`solids`](../interfaces/IShapeInfo.md#solids)

***

### vertices

#### Get Signature

> **get** **vertices**(): `number`[]

Return indices of defined vertex subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined vertex subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`vertices`](../interfaces/IShapeInfo.md#vertices)

***

### wires

#### Get Signature

> **get** **wires**(): `number`[]

Return indices of defined wire subshapes within parent presentation object.

##### Returns

`number`[]

Return indices of defined wire subshapes within parent presentation object.

#### Implementation of

[`IShapeInfo`](../interfaces/IShapeInfo.md).[`wires`](../interfaces/IShapeInfo.md#wires)

## Methods

### format()

> `static` **format**(`theInfo`, `theUMgr`): `string`

Format shape info.

#### Parameters

##### theInfo

[`IShapeInfo`](../interfaces/IShapeInfo.md)

##### theUMgr

[`UnitManager`](../interfaces/UnitManager.md) = `null`

#### Returns

`string`

***

### formatFlags()

> `static` **formatFlags**(`theInfo`): `string`

Format shape flags.

#### Parameters

##### theInfo

[`IShapeInfo`](../interfaces/IShapeInfo.md)

#### Returns

`string`

***

### getSubshapes()

> `protected` `static` **getSubshapes**(`theInfo`, `theType`): `number`[]

Return indices of defined subshapes within parent presentation object.

#### Parameters

##### theInfo

[`IShapeInfo`](../interfaces/IShapeInfo.md)

##### theType

[`ShapeEnum`](../enumerations/ShapeEnum.md)

#### Returns

`number`[]
