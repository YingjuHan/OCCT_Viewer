[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Trsf

# Class: Trsf

Describes gp_Trsf location (transformation) object.

## Implements

- [`ITrsf`](../interfaces/ITrsf.md)

## Constructors

### Constructor

> **new Trsf**(): `Trsf`

#### Returns

`Trsf`

## Properties

### rotation

> **rotation**: [`IVec4`](../interfaces/IVec4.md)

rotation quaternion.

#### Implementation of

[`ITrsf`](../interfaces/ITrsf.md).[`rotation`](../interfaces/ITrsf.md#rotation)

***

### scale

> **scale**: `number` = `1.0`

uniform scale factor.

#### Implementation of

[`ITrsf`](../interfaces/ITrsf.md).[`scale`](../interfaces/ITrsf.md#scale)

***

### translation

> **translation**: [`IVec3`](../interfaces/IVec3.md)

translation vector.

#### Implementation of

[`ITrsf`](../interfaces/ITrsf.md).[`translation`](../interfaces/ITrsf.md#translation)

## Methods

### format()

> `static` **format**(`theLoc`, `theUMgr`): `string`

Format location info.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### theUMgr

[`UnitManager`](../interfaces/UnitManager.md) = `null`

#### Returns

`string`

***

### getEulerAngles()

> `static` **getEulerAngles**(`theLoc`, `theSeq`): [`IVec3`](../interfaces/IVec3.md)

Return rotation part as 3 Euler angles.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### theSeq

[`EulerSequence`](../enumerations/EulerSequence.md) = `EulerSequence.EulerAngles`

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### getMat4()

> `static` **getMat4**(`theLoc`): [`IMat4`](../interfaces/IMat4.md)

Return 4x4 matrix.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

[`IMat4`](../interfaces/IMat4.md)

***

### invert()

> `static` **invert**(`theLoc`): [`ITrsf`](../interfaces/ITrsf.md)

Compute inverted transformation.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

[`ITrsf`](../interfaces/ITrsf.md)

***

### isIdentity()

> `static` **isIdentity**(`theLoc`): `boolean`

Return TRUE if location is empty (identity).

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

`boolean`

***

### isRotated()

> `static` **isRotated**(`theLoc`): `boolean`

Return TRUE if location has rotation quaternion.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

`boolean`

***

### isScaled()

> `static` **isScaled**(`theLoc`): `boolean`

Return TRUE if location has scale factor.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

`boolean`

***

### isTranslated()

> `static` **isTranslated**(`theLoc`): `boolean`

Return TRUE if location has translation vector.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

`boolean`

***

### multiply()

> `static` **multiply**(`theLoc1`, `theLoc2`): [`ITrsf`](../interfaces/ITrsf.md)

Multiply two transformations in specified order.

#### Parameters

##### theLoc1

[`ITrsf`](../interfaces/ITrsf.md)

##### theLoc2

[`ITrsf`](../interfaces/ITrsf.md)

#### Returns

[`ITrsf`](../interfaces/ITrsf.md)

***

### rotateAroundAxis()

> `static` **rotateAroundAxis**(`theDir`, `theAngle`, `theOrigin`): [`ITrsf`](../interfaces/ITrsf.md)

Setup rotation transformation around specified axis (origin will define translation part) and angle in radians.

#### Parameters

##### theDir

[`IVec3`](../interfaces/IVec3.md)

##### theAngle

`number`

##### theOrigin

[`IVec3`](../interfaces/IVec3.md) = `...`

#### Returns

[`ITrsf`](../interfaces/ITrsf.md)

***

### setEulerAngles()

> `static` **setEulerAngles**(`theLoc`, `theAngles`, `theSeq`): [`ITrsf`](../interfaces/ITrsf.md)

Setup rotation part from 3 Euler angles.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### theAngles

[`IVec3`](../interfaces/IVec3.md)

##### theSeq

[`EulerSequence`](../enumerations/EulerSequence.md) = `EulerSequence.EulerAngles`

#### Returns

[`ITrsf`](../interfaces/ITrsf.md)

***

### transformDir()

> `static` **transformDir**(`theLoc`, `theDir`): [`IVec3`](../interfaces/IVec3.md)

Return transformed 3D direction.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### theDir

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### transformPnt()

> `static` **transformPnt**(`theLoc`, `thePnt`): [`IVec3`](../interfaces/IVec3.md)

Return transformed 3D point.

#### Parameters

##### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### thePnt

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)
