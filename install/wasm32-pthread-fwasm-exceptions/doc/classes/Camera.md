[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Camera

# Class: Camera

Represents Camera manager.
See also [SCwebviewer.camera](SCwebviewer.md#camera) property.

## Constructors

### Constructor

> **new Camera**(`theViewer`, `theViewId`): `Camera`

Main constructor.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### theViewId

`number` = `-1`

#### Returns

`Camera`

## Accessors

### isPerspectiveProjection

#### Get Signature

> **get** **isPerspectiveProjection**(): `boolean`

Switch perspective/orthographic projection.

##### Returns

`boolean`

#### Set Signature

> **set** **isPerspectiveProjection**(`theIsPersp`): `void`

Switch perspective/orthographic projection.

##### Parameters

###### theIsPersp

`boolean`

##### Returns

`void`

***

### leftMouseGesture

#### Get Signature

> **get** **leftMouseGesture**(): [`MouseGesture`](../enumerations/MouseGesture.md)

Return left mouse button gesture; ROTATE (orbital) by default.

##### Returns

[`MouseGesture`](../enumerations/MouseGesture.md)

#### Set Signature

> **set** **leftMouseGesture**(`theGest`): `void`

Set left mouse button gesture.

##### Parameters

###### theGest

[`MouseGesture`](../enumerations/MouseGesture.md)

##### Returns

`void`

***

### maxCamDistanceMultiplier

#### Get Signature

> **get** **maxCamDistanceMultiplier**(): `number`

Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit.

##### Returns

`number`

#### Set Signature

> **set** **maxCamDistanceMultiplier**(`theMax`): `void`

Set max camera distance multiplier.

##### Parameters

###### theMax

`number`

##### Returns

`void`

***

### middleMouseGesture

#### Get Signature

> **get** **middleMouseGesture**(): [`MouseGesture`](../enumerations/MouseGesture.md)

Return middle mouse button gesture; PAN by default.

##### Returns

[`MouseGesture`](../enumerations/MouseGesture.md)

#### Set Signature

> **set** **middleMouseGesture**(`theGest`): `void`

Set middle mouse button gesture.

##### Parameters

###### theGest

[`MouseGesture`](../enumerations/MouseGesture.md)

##### Returns

`void`

***

### orbitAcceleration

#### Get Signature

> **get** **orbitAcceleration**(): `number`

Orbit acceleration coefficient.

##### Returns

`number`

#### Set Signature

> **set** **orbitAcceleration**(`theAccel`): `void`

Orbit acceleration coefficient.

##### Parameters

###### theAccel

`number`

##### Returns

`void`

***

### perspectiveProjectionFOVy

#### Get Signature

> **get** **perspectiveProjectionFOVy**(): `number`

Perspective projection FOVy in degrees.

##### Returns

`number`

#### Set Signature

> **set** **perspectiveProjectionFOVy**(`theFovy`): `void`

Perspective projection FOVy in degrees.

##### Parameters

###### theFovy

`number`

##### Returns

`void`

***

### rightMouseGesture

#### Get Signature

> **get** **rightMouseGesture**(): [`MouseGesture`](../enumerations/MouseGesture.md)

Return right mouse button gesture; ROTATEVIEW by default.

##### Returns

[`MouseGesture`](../enumerations/MouseGesture.md)

#### Set Signature

> **set** **rightMouseGesture**(`theGest`): `void`

Set right mouse button gesture.

##### Parameters

###### theGest

[`MouseGesture`](../enumerations/MouseGesture.md)

##### Returns

`void`

***

### sceneMinMaxSelection

#### Get Signature

> **get** **sceneMinMaxSelection**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return AABB of selected objects in the view.

##### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

***

### sceneMinMaxVisible

#### Get Signature

> **get** **sceneMinMaxVisible**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return AABB of all visible objects in the view.

##### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

***

### stereoMode

#### Get Signature

> **get** **stereoMode**(): [`StereoMode`](../enumerations/StereoMode.md)

Stereoscopic mode; MONO by default.

##### Returns

[`StereoMode`](../enumerations/StereoMode.md)

#### Set Signature

> **set** **stereoMode**(`theMode`): `void`

Stereoscopic mode.

##### Parameters

###### theMode

[`StereoMode`](../enumerations/StereoMode.md)

##### Returns

`void`

***

### toAllowPanning

#### Get Signature

> **get** **toAllowPanning**(): `boolean`

Allow/disallow camera panning.

##### Returns

`boolean`

#### Set Signature

> **set** **toAllowPanning**(`theToAllow`): `void`

Allow/disallow camera panning.

##### Parameters

###### theToAllow

`boolean`

##### Returns

`void`

***

### toAllowRotation

#### Get Signature

> **get** **toAllowRotation**(): `boolean`

Allow/disallow camera rotation.

##### Returns

`boolean`

#### Set Signature

> **set** **toAllowRotation**(`theToAllow`): `void`

Allow/disallow camera rotation.

##### Parameters

###### theToAllow

`boolean`

##### Returns

`void`

***

### toAllowZooming

#### Get Signature

> **get** **toAllowZooming**(): `boolean`

Allow/disallow camera zooming.

##### Returns

`boolean`

#### Set Signature

> **set** **toAllowZooming**(`theToAllow`): `void`

Allow/disallow camera zooming.

##### Parameters

###### theToAllow

`boolean`

##### Returns

`void`

***

### toLockOrbitZUp

#### Get Signature

> **get** **toLockOrbitZUp**(): `boolean`

Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up.

##### Returns

`boolean`

#### Set Signature

> **set** **toLockOrbitZUp**(`theToLock`): `void`

Get/set if camera up orientation should be forced Z up.

##### Parameters

###### theToLock

`boolean`

##### Returns

`void`

***

### viewPoint

#### Get Signature

> **get** **viewPoint**(): [`ICameraViewPoint`](../interfaces/ICameraViewPoint.md)

Return current camera view point parameters.

##### Returns

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md)

#### Set Signature

> **set** **viewPoint**(`theParams`): `void`

Set camera view point parameters.

##### Parameters

###### theParams

[`ICameraViewPoint`](../interfaces/ICameraViewPoint.md)

##### Returns

`void`

## Methods

### fitAllAuto()

> **fitAllAuto**(`theIsAuto`, `theToAnimate`): `void`

Fit selected or fit all visible objects into view.

#### Parameters

##### theIsAuto

`boolean`

##### theToAnimate

`boolean` = `false`

#### Returns

`void`

***

### fitMinMax()

> **fitMinMax**(`theMinMax`, `theToAnimate`): `void`

Fit box into view.

#### Parameters

##### theMinMax

[`IBndBox3`](../interfaces/IBndBox3.md)

##### theToAnimate

`boolean`

#### Returns

`void`

***

### getSceneMinMax()

> **getSceneMinMax**(`theSelected`): [`IBndBox3`](../interfaces/IBndBox3.md)

Return AABB of all or selected objects in the view.

#### Parameters

##### theSelected

`boolean`

#### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

***

### isFitMinMax()

> **isFitMinMax**(`theMinMax`): `boolean`

Return TRUE if box tightly fits into the view.

#### Parameters

##### theMinMax

[`IBndBox3`](../interfaces/IBndBox3.md)

#### Returns

`boolean`

***

### setOrientation()

> **setOrientation**(`theOrientation`, `theToAnimate`): `void`

Set camera view orientation.

#### Parameters

##### theOrientation

[`CameraOrientation`](../enumerations/CameraOrientation.md)

viewer orientation

##### theToAnimate

`boolean` = `false`

#### Returns

`void`
