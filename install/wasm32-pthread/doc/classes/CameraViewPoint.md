[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / CameraViewPoint

# Class: CameraViewPoint

Represents camera View Point parameters object (see `Graphic3d_Camera` from C++ OCCT).

## Implements

- [`ICameraViewPoint`](../interfaces/ICameraViewPoint.md)

## Constructors

### Constructor

> **new CameraViewPoint**(): `CameraViewPoint`

#### Returns

`CameraViewPoint`

## Properties

### direction

> **direction**: [`IVec3`](../interfaces/IVec3.md)

Direction of look (normalized 3D vector).

#### Implementation of

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md).[`direction`](../interfaces/ICameraViewPoint.md#direction)

***

### distance

> **distance**: `number` = `0.0`

Distance between camera and point to look.

#### Implementation of

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md).[`distance`](../interfaces/ICameraViewPoint.md#distance)

***

### eye

> **eye**: [`IVec3`](../interfaces/IVec3.md)

Eye location (3d point).

#### Implementation of

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md).[`eye`](../interfaces/ICameraViewPoint.md#eye)

***

### scale

> **scale**: `number` = `1.0`

Viewport relative scale.

#### Implementation of

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md).[`scale`](../interfaces/ICameraViewPoint.md#scale)

***

### up

> **up**: [`IVec3`](../interfaces/IVec3.md)

Direction of up vector (normalized 3D vector).

#### Implementation of

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md).[`up`](../interfaces/ICameraViewPoint.md#up)
