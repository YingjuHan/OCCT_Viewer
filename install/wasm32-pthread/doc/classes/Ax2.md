[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Ax2

# Class: Ax2

Represents axes object (based on `gp_Ax2` from C++ OCCT).

## Implements

- [`IAx2`](../interfaces/IAx2.md)

## Constructors

### Constructor

> **new Ax2**(): `Ax2`

#### Returns

`Ax2`

## Properties

### direction

> **direction**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`IAx2`](../interfaces/IAx2.md).[`direction`](../interfaces/IAx2.md#direction)

***

### location

> **location**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`IAx2`](../interfaces/IAx2.md).[`location`](../interfaces/IAx2.md#location)

***

### xDirection

> **xDirection**: [`IVec3`](../interfaces/IVec3.md)

#### Implementation of

[`IAx2`](../interfaces/IAx2.md).[`xDirection`](../interfaces/IAx2.md#xdirection)

## Methods

### invalid()

> `static` **invalid**(): [`IAx2`](../interfaces/IAx2.md)

Return invalid system.

#### Returns

[`IAx2`](../interfaces/IAx2.md)

***

### xoy()

> `static` **xoy**(): [`IAx2`](../interfaces/IAx2.md)

Identifies a coordinate system where its origin is Origin,
and its "main Direction" and "X Direction" coordinates
Z = 1.0, X = Y =0.0 and X direction coordinates X = 1.0, Y = Z = 0.0

#### Returns

[`IAx2`](../interfaces/IAx2.md)

***

### yoz()

> `static` **yoz**(): [`IAx2`](../interfaces/IAx2.md)

Identifies a coordinate system where its origin is Origin,
and its "main Direction" and "X Direction" coordinates
X = 1.0, Z = Y =0.0 and X direction coordinates Y = 1.0, X = Z = 0.0
In 2D space

#### Returns

[`IAx2`](../interfaces/IAx2.md)

***

### zox()

> `static` **zox**(): [`IAx2`](../interfaces/IAx2.md)

Identifies a coordinate system where its origin is Origin,
and its "main Direction" and "X Direction" coordinates
Y = 1.0, X = Z =0.0 and X direction coordinates Z = 1.0, X = Y = 0.0

#### Returns

[`IAx2`](../interfaces/IAx2.md)
