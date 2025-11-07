[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ICameraViewPoint

# Interface: ICameraViewPoint

Represents camera View Point parameters interface (see `Graphic3d_Camera` from C++ OCCT).
See also [Camera.viewPoint](../classes/Camera.md#viewpoint).

## Properties

### direction

> **direction**: [`IVec3`](IVec3.md)

Direction of look (normalized 3D vector).

***

### distance

> **distance**: `number`

Distance between camera and point to look.

***

### eye

> **eye**: [`IVec3`](IVec3.md)

Eye location (3d point).

***

### scale

> **scale**: `number`

Viewport relative scale.

***

### up

> **up**: [`IVec3`](IVec3.md)

Direction of up vector (normalized 3D vector).
