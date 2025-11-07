[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / BndBox3

# Class: BndBox3

Represents 3D axes-aligned bounding box object defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).

## Implements

- [`IBndBox3`](../interfaces/IBndBox3.md)

## Constructors

### Constructor

> **new BndBox3**(): `BndBox3`

#### Returns

`BndBox3`

## Properties

### cornerMax

> **cornerMax**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`IBndBox3`](../interfaces/IBndBox3.md).[`cornerMax`](../interfaces/IBndBox3.md#cornermax)

***

### cornerMin

> **cornerMin**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`IBndBox3`](../interfaces/IBndBox3.md).[`cornerMin`](../interfaces/IBndBox3.md#cornermin)

## Methods

### diagonal()

> `static` **diagonal**(`theBox`): [`IVec3`](../interfaces/IVec3.md)

Return box diagonal

#### Parameters

##### theBox

[`IBndBox3`](../interfaces/IBndBox3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### extent()

> `static` **extent**(`theBox`): `number`

Return box extent (diagonal modulus).

#### Parameters

##### theBox

[`IBndBox3`](../interfaces/IBndBox3.md)

#### Returns

`number`

***

### isVoid()

> `static` **isVoid**(`theBox`): `boolean`

Return TRUE if box is invalid (void).

#### Parameters

##### theBox

[`IBndBox3`](../interfaces/IBndBox3.md)

#### Returns

`boolean`

***

### voidBox()

> `static` **voidBox**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return void box.

#### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)
