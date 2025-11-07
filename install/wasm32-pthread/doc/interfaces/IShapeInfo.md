[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / IShapeInfo

# Interface: IShapeInfo

Describes information about TopoDS_Shape.

## Properties

### checked

> **checked**: `boolean`

Checked flag.

***

### closed

> **closed**: `boolean`

Closed flag.

***

### color

> **color**: `string`

Subshape color.

***

### compSolids?

> `readonly` `optional` **compSolids**: `number`[]

Return indices of defined composite solid subshapes within parent presentation object.

***

### convex

> **convex**: `boolean`

Convex flag.

***

### curveType

> **curveType**: [`ShapeCurveType`](../enumerations/ShapeCurveType.md)

Geometry curve type (for EDGE shape type).

***

### edges?

> `readonly` `optional` **edges**: `number`[]

Return indices of defined edge subshapes within parent presentation object.

***

### faces?

> `readonly` `optional` **faces**: `number`[]

Return indices of defined face subshapes within parent presentation object.

***

### free

> **free**: `boolean`

Free flag.

***

### hidden

> **hidden**: `boolean`

Subshape hidden flag.

***

### index

> **index**: `number`

Selection index within subshapes of the same type.

***

### infinite

> **infinite**: `boolean`

Infinite flag.

***

### location

> **location**: [`ITrsf`](ITrsf.md)

Shape location.

***

### locked

> **locked**: `boolean`

Locked flag.

***

### modified

> **modified**: `boolean`

Modified flag.

***

### nbChildren

> **nbChildren**: `number`

Number of direct subshapes.

***

### nbPolySegments

> **nbPolySegments**: `number`

Number of tessellation segments (for EDGE shapes).

***

### nbTriangles

> **nbTriangles**: `number`

Number of triangles (for FACE shape type).

***

### nbTriNodes

> **nbTriNodes**: `number`

Number of triangle nodes (for FACE shape type).

***

### objectId

> **objectId**: `string`

Presentation object id.

***

### orientable

> **orientable**: `boolean`

Orientable flag.

***

### orientation

> **orientation**: [`ShapeOrientation`](../enumerations/ShapeOrientation.md)

Shape orientation.

***

### point

> **point**: [`IVec3`](IVec3.md)

Vertex position (for VERTEX shape type).

***

### shapeType

> **shapeType**: [`ShapeEnum`](../enumerations/ShapeEnum.md)

Shape type.

***

### shells?

> `readonly` `optional` **shells**: `number`[]

Return indices of defined shell subshapes within parent presentation object.

***

### solids?

> `readonly` `optional` **solids**: `number`[]

Return indices of defined solid subshapes within parent presentation object.

***

### surfaceType

> **surfaceType**: [`ShapeSurfaceType`](../enumerations/ShapeSurfaceType.md)

Geometry surface type (for FACE shape type).

***

### vertices?

> `readonly` `optional` **vertices**: `number`[]

Return indices of defined vertex subshapes within parent presentation object.

***

### wires?

> `readonly` `optional` **wires**: `number`[]

Return indices of defined wire subshapes within parent presentation object.
