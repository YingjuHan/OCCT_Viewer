[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Vec3

# Class: Vec3

Represents 3D vector object (based on `Graphic3d_Vec3d` from C++ OCCT).
Provides also math operations between 3D vectors.

## Implements

- [`IVec3`](../interfaces/IVec3.md)

## Constructors

### Constructor

> **new Vec3**(): `Vec3`

#### Returns

`Vec3`

## Properties

### x

> **x**: `number` = `0.0`

#### Implementation of

[`IVec3`](../interfaces/IVec3.md).[`x`](../interfaces/IVec3.md#x)

***

### y

> **y**: `number` = `0.0`

#### Implementation of

[`IVec3`](../interfaces/IVec3.md).[`y`](../interfaces/IVec3.md#y)

***

### z

> **z**: `number` = `0.0`

#### Implementation of

[`IVec3`](../interfaces/IVec3.md).[`z`](../interfaces/IVec3.md#z)

## Methods

### add()

> `static` **add**(`theA`, `theB`): [`IVec3`](../interfaces/IVec3.md)

Compute per-component addition between two vectors A+B.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### angle()

> `static` **angle**(`theA`, `theB`): `number`

Compute angle between two normalized vectors

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

[`IVec3`](../interfaces/IVec3.md)

#### Returns

`number`

***

### coord()

> `static` **coord**(`theV`, `theI`): `number`

Return coordinate with specified index within [0,2] range.

#### Parameters

##### theV

[`IVec3`](../interfaces/IVec3.md)

##### theI

`number`

#### Returns

`number`

***

### cross()

> `static` **cross**(`theA`, `theB`): [`IVec3`](../interfaces/IVec3.md)

Compute cross product between two vectors.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### divide()

> `static` **divide**(`theA`, `theB`): [`IVec3`](../interfaces/IVec3.md)

Compute per-component division between two vectors A/B.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### dot()

> `static` **dot**(`theA`, `theB`): `number`

Compute dot product between two vectors.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

[`IVec3`](../interfaces/IVec3.md)

#### Returns

`number`

***

### dx()

> `static` **dx**(): [`IVec3`](../interfaces/IVec3.md)

Return DX vector.

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### dy()

> `static` **dy**(): [`IVec3`](../interfaces/IVec3.md)

Return DY vector.

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### dz()

> `static` **dz**(): [`IVec3`](../interfaces/IVec3.md)

Return DZ vector.

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### invalid()

> `static` **invalid**(): [`IVec3`](../interfaces/IVec3.md)

Return invalid vector (`RealLast()` values).

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### invalidDouble()

> `static` **invalidDouble**(): `number`

Return invalid (`RealLast()`) value.

#### Returns

`number`

***

### isInfinite()

> `static` **isInfinite**(`theA`): `boolean`

Return TRUE if point has infinite range (same as `Precision::IsInfinite()` check in OCCT).

#### Parameters

##### theA

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

`boolean`

***

### modulus()

> `static` **modulus**(`theV`): `number`

Return vector modulus.

#### Parameters

##### theV

[`IVec3`](../interfaces/IVec3.md)

#### Returns

`number`

***

### multiply()

> `static` **multiply**(`theA`, `theB`): [`IVec3`](../interfaces/IVec3.md)

Compute per-component multiplication between two vectors A*B.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### normalize()

> `static` **normalize**(`theV`): [`IVec3`](../interfaces/IVec3.md)

Return normalized vector.

#### Parameters

##### theV

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### reverse()

> `static` **reverse**(`theV`): [`IVec3`](../interfaces/IVec3.md)

Return opposite vector.

#### Parameters

##### theV

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### rotateVector()

> `static` **rotateVector**(`theVec`, `theAxis`, `theAngle`): [`IVec3`](../interfaces/IVec3.md)

Rotate normalized vector around another vector by angle.

#### Parameters

##### theVec

[`IVec3`](../interfaces/IVec3.md)

vector to rotate

##### theAxis

[`IVec3`](../interfaces/IVec3.md)

vector to rotate around

##### theAngle

`number`

angle in radians

#### Returns

[`IVec3`](../interfaces/IVec3.md)

rotated vector

***

### subtract()

> `static` **subtract**(`theA`, `theB`): [`IVec3`](../interfaces/IVec3.md)

Compute per-component subtraction between two vectors A-B.

#### Parameters

##### theA

[`IVec3`](../interfaces/IVec3.md)

##### theB

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### zero()

> `static` **zero**(): [`IVec3`](../interfaces/IVec3.md)

Return zero vector.

#### Returns

[`IVec3`](../interfaces/IVec3.md)
