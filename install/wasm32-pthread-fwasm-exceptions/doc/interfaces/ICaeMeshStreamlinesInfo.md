[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ICaeMeshStreamlinesInfo

# Interface: ICaeMeshStreamlinesInfo

ICaeMeshStreamlinesInfo represents streamlines info of [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) entity.

## Properties

### animationEnabled

> **animationEnabled**: `boolean`

Flag to enable/disable streamlines animation.

***

### animationFrequency

> **animationFrequency**: `number`

Animation particles frequency, integer number in [1, 100] range.

***

### animationSpeed

> **animationSpeed**: `number`

Animation particles speed coefficient, in [0.0, 1.0] range.

***

### animationTrail

> **animationTrail**: `number`

Animation particles trail length coefficient, in [0.0, 1.0] range.

***

### isRegular

> **isRegular**: `boolean`

Switch between regular (true) and random (false) distribution of the initial points in the seed plane.

***

### lengthCoeff

> **lengthCoeff**: `number`

Streamlines max length coefficient.

***

### nbStreamlines

> **nbStreamlines**: `number`

Total number of streamlines for random distribution of initial points.

***

### planeOrigin

> **planeOrigin**: [`IVec3`](IVec3.md)

Origin of plane used for streamlines initial points generation.

***

### planeSide

> **planeSide**: [`IVec3`](IVec3.md)

Sized side vector of plane used for streamlines initial points generation.

***

### planeUp

> **planeUp**: [`IVec3`](IVec3.md)

Sized up vector of plane used for streamlines initial points generation.

***

### property

> **property**: `string`

Property id.

***

### stepCoeff

> **stepCoeff**: `number`

Integration step coefficient; lower value leads to more accurate result but takes more time to calculate.

***

### thickness

> **thickness**: `number`

Thickness as a coefficient multiplied by bounding box size.

***

### uvStreamlines

> **uvStreamlines**: [`IVec2`](IVec2.md)

"U x V" streamlines layout for regular distribution of initial points.
