[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ViewManager

# Interface: ViewManager

Represents manager for subviews.
See also [SCwebviewer.viewManager](../classes/SCwebviewer.md#viewmanager) property.

## Extends

- [`IManager`](IManager.md)

## Properties

### createSubview()

> **createSubview**: (`theSize`) => `number`

Set new subview size and return it's id.

#### Parameters

##### theSize

[`IVec2`](IVec2.md)

#### Returns

`number`

***

### fitAllAuto()

> **fitAllAuto**: (`theViewId`, `theIsAuto`) => `void`

Fit selected or fit all visible objects into view.

#### Parameters

##### theViewId

`number`

##### theIsAuto

`boolean`

#### Returns

`void`

***

### fitMinMax()

> **fitMinMax**: (`theViewId`, `theMinMax`, `theToAnimate`) => `void`

Fit box into the view.

#### Parameters

##### theViewId

`number`

##### theMinMax

[`IBndBox3`](IBndBox3.md)

##### theToAnimate

`boolean`

#### Returns

`void`

***

### getInputFocus()

> **getInputFocus**: (`theViewId`) => `number`

Return id of view that receives input focus.

#### Parameters

##### theViewId

`number`

#### Returns

`number`

***

### getMaxCamDistanceMultiplier()

> **getMaxCamDistanceMultiplier**: () => `number`

Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit.

#### Returns

`number`

***

### getMouseGesture()

> **getMouseGesture**: (`theViewId`, `theButton`) => `number`

Return mouse button gesture.

#### Parameters

##### theViewId

`number`

##### theButton

`number`

#### Returns

`number`

***

### getOrbitAcceleration()

> **getOrbitAcceleration**: () => `number`

Orbit acceleration coefficient.

#### Returns

`number`

***

### getPerspectiveProjectionFOVy()

> **getPerspectiveProjectionFOVy**: (`theViewId`) => `number`

Perspective projection FOVy in degrees.

#### Parameters

##### theViewId

`number`

#### Returns

`number`

***

### getSceneMinMax()

> **getSceneMinMax**: (`theViewId`, `theSelection`) => [`IBndBox3`](IBndBox3.md)

Return bounding box of all visible or selected objects in the view.

#### Parameters

##### theViewId

`number`

##### theSelection

`boolean`

#### Returns

[`IBndBox3`](IBndBox3.md)

***

### getStereoMode()

> **getStereoMode**: (`theViewId`) => `number`

Stereoscopic mode.

#### Parameters

##### theViewId

`number`

#### Returns

`number`

***

### getSubviewBorderColor()

> **getSubviewBorderColor**: () => `string`

Get subview border color (color filling in margins between all subviews).

#### Returns

`string`

***

### getSubviewCorner()

> **getSubviewCorner**: (`theViewId`) => [`ViewCorner`](../enumerations/ViewCorner.md)

Return subview corner.

#### Parameters

##### theViewId

`number`

#### Returns

[`ViewCorner`](../enumerations/ViewCorner.md)

***

### getSubviewMargins()

> **getSubviewMargins**: (`theViewId`) => [`IVec2`](IVec2.md)

Return subview margins defined in pixels.

#### Parameters

##### theViewId

`number`

#### Returns

[`IVec2`](IVec2.md)

***

### getSubviewOffset()

> **getSubviewOffset**: (`theViewId`) => [`IVec2`](IVec2.md)

Return subview offset from corner, defined as a fraction of main canvas.

#### Parameters

##### theViewId

`number`

#### Returns

[`IVec2`](IVec2.md)

***

### getSubviewSize()

> **getSubviewSize**: (`theViewId`) => [`IVec2`](IVec2.md)

Return subview size, defined as a fraction of main canvas.

#### Parameters

##### theViewId

`number`

#### Returns

[`IVec2`](IVec2.md)

***

### getViewPoint()

> **getViewPoint**: (`theViewId`) => [`ICameraViewPoint`](ICameraViewPoint.md)

Return current camera view point parameters.

#### Parameters

##### theViewId

`number`

#### Returns

[`ICameraViewPoint`](ICameraViewPoint.md)

***

### hasInputFocus()

> **hasInputFocus**: (`theViewId`) => `boolean`

Return TRUE if view with specified [ViewId](../type-aliases/ViewId.md) receives input focus.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### invalidate2dObjects()

> **invalidate2dObjects**: (`theObjId`) => `void`

Mark all 2d objects to update their position/size on the next draw call.

#### Parameters

##### theObjId

`string`

#### Returns

`void`

***

### isActive()

> **isActive**: (`theViewId`) => `boolean`

Return TRUE if view with specified [ViewId](../type-aliases/ViewId.md) is active (displayed on the screen).

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### isDefined()

> **isDefined**: (`theViewId`) => `boolean`

Return TRUE if view with specified [ViewId](../type-aliases/ViewId.md) exists.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### isFitMinMax()

> **isFitMinMax**: (`theViewId`, `theMinMax`) => `boolean`

Return TRUE if obx tightly fits into the view.

#### Parameters

##### theViewId

`number`

##### theMinMax

[`IBndBox3`](IBndBox3.md)

#### Returns

`boolean`

***

### isFullscreen()

> **isFullscreen**: () => `boolean`

Return fullscreen mode, when canvas (with all Subviews) occupies entire screen.
See also [ViewManager.isSubviewFullscreen](#issubviewfullscreen).

#### Returns

`boolean`

***

### isPerspectiveProjection()

> **isPerspectiveProjection**: (`theViewId`) => `boolean`

Switch perspective/orthographic projection.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### isSubviewClickable()

> **isSubviewClickable**: (`theViewId`) => `boolean`

Return subview clickable property.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### isSubviewComposer()

> **isSubviewComposer**: (`theViewId`) => `boolean`

Return TRUE if view with specified [ViewId](../type-aliases/ViewId.md) serves as a composer to subviews.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### isSubviewFullscreen()

> **isSubviewFullscreen**: (`theViewId`) => `boolean`

Return subview fullscreen mode.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### removeAllSubviews()

> **removeAllSubviews**: () => `void`

Remove all subview.

#### Returns

`void`

***

### removeSubview()

> **removeSubview**: (`theViewId`) => `void`

Remove subview.

#### Parameters

##### theViewId

`number`

#### Returns

`void`

***

### setActive()

> **setActive**: (`theViewId`, `theActive`) => `void`

Set view [ViewId](../type-aliases/ViewId.md) active state.

#### Parameters

##### theViewId

`number`

##### theActive

`boolean`

#### Returns

`void`

***

### setAllowPanning()

> **setAllowPanning**: (`theViewId`, `theToAllow`) => `void`

Allow/disallow camera panning.

#### Parameters

##### theViewId

`number`

##### theToAllow

`boolean`

#### Returns

`void`

***

### setAllowRotation()

> **setAllowRotation**: (`theViewId`, `theToAllow`) => `void`

Allow/disallow camera rotation.

#### Parameters

##### theViewId

`number`

##### theToAllow

`boolean`

#### Returns

`void`

***

### setAllowZooming()

> **setAllowZooming**: (`theViewId`, `theToAllow`) => `void`

Allow/disallow camera zooming.

#### Parameters

##### theViewId

`number`

##### theToAllow

`boolean`

#### Returns

`void`

***

### setFullscreen()

> **setFullscreen**: (`theToEnable`) => `void`

Set fullscreen mode;
WARNING! This should be called in combination with user gesture (mouse click / button click) in browser!

#### Parameters

##### theToEnable

`boolean`

#### Returns

`void`

***

### setInputFocus()

> **setInputFocus**: (`theViewId`) => `void`

Set view with specified [ViewId](../type-aliases/ViewId.md) to receive input focus.

#### Parameters

##### theViewId

`number`

#### Returns

`void`

***

### setLockOrbitZUp()

> **setLockOrbitZUp**: (`theViewId`, `theToLock`) => `void`

Set if camera up orientation should be forced Z up.

#### Parameters

##### theViewId

`number`

##### theToLock

`boolean`

#### Returns

`void`

***

### setMaxCamDistanceMultiplier()

> **setMaxCamDistanceMultiplier**: (`theMax`) => `void`

Set max camera distance multiplier.

#### Parameters

##### theMax

`number`

#### Returns

`void`

***

### setMouseGesture()

> **setMouseGesture**: (`theViewId`, `theButton`, `theGest`) => `void`

Set mouse button gesture.

#### Parameters

##### theViewId

`number`

##### theButton

`number`

##### theGest

`number`

#### Returns

`void`

***

### setOrbitAcceleration()

> **setOrbitAcceleration**: (`theAccel`) => `void`

Orbit acceleration coefficient.

#### Parameters

##### theAccel

`number`

#### Returns

`void`

***

### setOrientation()

> **setOrientation**: (`theViewId`, `theOrientation`, `theToAnimate`) => `void`

Set camera view orientation.

#### Parameters

##### theViewId

`number`

##### theOrientation

[`CameraOrientation`](../enumerations/CameraOrientation.md)

viewer orientation

##### theToAnimate

`boolean`

#### Returns

`void`

***

### setPerspectiveProjection()

> **setPerspectiveProjection**: (`theViewId`, `theIsPersp`) => `void`

Switch perspective/orthographic projection.

#### Parameters

##### theViewId

`number`

##### theIsPersp

`boolean`

#### Returns

`void`

***

### setPerspectiveProjectionFOVy()

> **setPerspectiveProjectionFOVy**: (`theViewId`, `theFovy`) => `void`

Perspective projection FOVy in degrees.

#### Parameters

##### theViewId

`number`

##### theFovy

`number`

#### Returns

`void`

***

### setStereoMode()

> **setStereoMode**: (`theViewId`, `theMode`) => `void`

Stereoscopic mode.

#### Parameters

##### theViewId

`number`

##### theMode

`number`

#### Returns

`void`

***

### setSubviewBorderColor()

> **setSubviewBorderColor**: (`theColor`) => `void`

Set subview border color.

#### Parameters

##### theColor

`string`

#### Returns

`void`

***

### setSubviewClickable()

> **setSubviewClickable**: (`theViewId`, `theToEnable`) => `void`

Set subview fullscreen mode.

#### Parameters

##### theViewId

`number`

##### theToEnable

`boolean`

#### Returns

`void`

***

### setSubviewComposer()

> **setSubviewComposer**: (`theViewId`, `theActive`) => `void`

Set if view with specified [ViewId](../type-aliases/ViewId.md) should serve as a composer to subviews.

#### Parameters

##### theViewId

`number`

##### theActive

`boolean`

#### Returns

`void`

***

### setSubviewCorner()

> **setSubviewCorner**: (`theViewId`, `theCorner`) => `void`

Set subview corner.

#### Parameters

##### theViewId

`number`

##### theCorner

[`ViewCorner`](../enumerations/ViewCorner.md)

#### Returns

`void`

***

### setSubviewFullscreen()

> **setSubviewFullscreen**: (`theViewId`, `theToEnable`) => `void`

Set subview fullscreen mode.

#### Parameters

##### theViewId

`number`

##### theToEnable

`boolean`

#### Returns

`void`

***

### setSubviewMargins()

> **setSubviewMargins**: (`theViewId`, `theMargins`) => `void`

Set subview margins .

#### Parameters

##### theViewId

`number`

##### theMargins

[`IVec2`](IVec2.md)

#### Returns

`void`

***

### setSubviewOffset()

> **setSubviewOffset**: (`theViewId`, `theOffset`) => `void`

Set subview offset from corner.

#### Parameters

##### theViewId

`number`

##### theOffset

[`IVec2`](IVec2.md)

#### Returns

`void`

***

### setSubviewSize()

> **setSubviewSize**: (`theViewId`, `theSize`) => `void`

Set subview size.

#### Parameters

##### theViewId

`number`

##### theSize

[`IVec2`](IVec2.md)

#### Returns

`void`

***

### setViewOffset()

> **setViewOffset**: (`theSide`, `theOffset`) => `void`

Set viewer offset for specified corner.

#### Parameters

##### theSide

[`ViewCorner`](../enumerations/ViewCorner.md)

##### theOffset

[`IVec2`](IVec2.md)

#### Returns

`void`

***

### setViewPoint()

> **setViewPoint**: (`theViewId`, `theParams`) => `void`

Set camera view point parameters.

#### Parameters

##### theViewId

`number`

##### theParams

[`ICameraViewPoint`](ICameraViewPoint.md)

#### Returns

`void`

***

### subviews

> `readonly` **subviews**: [`Subview`](../classes/Subview.md)[]

Return the array of created subviews

***

### toAllowPanning()

> **toAllowPanning**: (`theViewId`) => `boolean`

Allow/disallow camera panning.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### toAllowRotation()

> **toAllowRotation**: (`theViewId`) => `boolean`

Allow/disallow camera rotation.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### toAllowZooming()

> **toAllowZooming**: (`theViewId`) => `boolean`

Allow/disallow camera zooming.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### toLockOrbitZUp()

> **toLockOrbitZUp**: (`theViewId`) => `boolean`

Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up.

#### Parameters

##### theViewId

`number`

#### Returns

`boolean`

***

### unsetViewOffsets()

> **unsetViewOffsets**: () => `void`

Reset all viewer offsets.

#### Returns

`void`
