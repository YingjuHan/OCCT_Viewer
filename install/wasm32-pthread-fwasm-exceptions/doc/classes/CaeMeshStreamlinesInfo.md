[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / CaeMeshStreamlinesInfo

# Class: CaeMeshStreamlinesInfo

Represents streamlines info of [InteractiveCaeMesh](InteractiveCaeMesh.md) entity.

## Implements

- [`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md)

## Constructors

### Constructor

> **new CaeMeshStreamlinesInfo**(): `CaeMeshStreamlinesInfo`

#### Returns

`CaeMeshStreamlinesInfo`

## Properties

### animationEnabled

> **animationEnabled**: `boolean` = `false`

Flag to enable/disable streamlines animation.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`animationEnabled`](../interfaces/ICaeMeshStreamlinesInfo.md#animationenabled)

***

### animationFrequency

> **animationFrequency**: `number` = `10`

Animation particles frequency, integer number in [1, 100] range.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`animationFrequency`](../interfaces/ICaeMeshStreamlinesInfo.md#animationfrequency)

***

### animationSpeed

> **animationSpeed**: `number` = `0.25`

Animation particles speed coefficient, in [0.0, 1.0] range.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`animationSpeed`](../interfaces/ICaeMeshStreamlinesInfo.md#animationspeed)

***

### animationTrail

> **animationTrail**: `number` = `0.5`

Animation particles trail length coefficient, in [0.0, 1.0] range.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`animationTrail`](../interfaces/ICaeMeshStreamlinesInfo.md#animationtrail)

***

### isRegular

> **isRegular**: `boolean` = `false`

Switch between regular (true) and random (false) distribution of the initial points in the seed plane.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`isRegular`](../interfaces/ICaeMeshStreamlinesInfo.md#isregular)

***

### lengthCoeff

> **lengthCoeff**: `number` = `1.0`

Streamlines max length coefficient.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`lengthCoeff`](../interfaces/ICaeMeshStreamlinesInfo.md#lengthcoeff)

***

### nbStreamlines

> **nbStreamlines**: `number` = `0`

Total number of streamlines for random distribution of initial points.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`nbStreamlines`](../interfaces/ICaeMeshStreamlinesInfo.md#nbstreamlines)

***

### planeOrigin

> **planeOrigin**: [`IVec3`](../interfaces/IVec3.md)

Origin of plane used for streamlines initial points generation.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`planeOrigin`](../interfaces/ICaeMeshStreamlinesInfo.md#planeorigin)

***

### planeSide

> **planeSide**: [`IVec3`](../interfaces/IVec3.md)

Sized side vector of plane used for streamlines initial points generation.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`planeSide`](../interfaces/ICaeMeshStreamlinesInfo.md#planeside)

***

### planeUp

> **planeUp**: [`IVec3`](../interfaces/IVec3.md)

Sized up vector of plane used for streamlines initial points generation.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`planeUp`](../interfaces/ICaeMeshStreamlinesInfo.md#planeup)

***

### property

> **property**: `string` = `''`

Property id.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`property`](../interfaces/ICaeMeshStreamlinesInfo.md#property)

***

### stepCoeff

> **stepCoeff**: `number` = `1.0`

Integration step coefficient; lower value leads to more accurate result but takes more time to calculate.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`stepCoeff`](../interfaces/ICaeMeshStreamlinesInfo.md#stepcoeff)

***

### thickness

> **thickness**: `number` = `0.0`

Thickness as a coefficient multiplied by bounding box size.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`thickness`](../interfaces/ICaeMeshStreamlinesInfo.md#thickness)

***

### uvStreamlines

> **uvStreamlines**: [`IVec2`](../interfaces/IVec2.md)

"U x V" streamlines layout for regular distribution of initial points.

#### Implementation of

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md).[`uvStreamlines`](../interfaces/ICaeMeshStreamlinesInfo.md#uvstreamlines)
