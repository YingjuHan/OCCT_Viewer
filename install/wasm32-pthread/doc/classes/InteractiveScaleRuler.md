[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveScaleRuler

# Class: InteractiveScaleRuler

Represents Scale Ruler property manager.
Scale Ruler is an auxiliary interactive object displayed length measure units at view corner.
See also [SCwebviewer.scaleRuler](SCwebviewer.md#scaleruler) property.

## Extends

- [`InteractiveObject`](InteractiveObject.md)

## Constructors

### Constructor

> **new InteractiveScaleRuler**(`theViewer`, `theObjId`): `InteractiveScaleRuler`

Empty constructor.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### theObjId

`string` = `''`

#### Returns

`InteractiveScaleRuler`

#### Overrides

[`InteractiveObject`](InteractiveObject.md).[`constructor`](InteractiveObject.md#constructor)

## Accessors

### aspects

#### Get Signature

> **get** **aspects**(): [`PresentationAspects`](PresentationAspects.md)

Return aspects manager.

##### Returns

[`PresentationAspects`](PresentationAspects.md)

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`aspects`](InteractiveObject.md#aspects)

***

### baseColor

#### Get Signature

> **get** **baseColor**(): `string`

Return object's global RGB color defined as [ColorString](../type-aliases/ColorString.md) or empty string if unspecified.
Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!

##### Returns

`string`

#### Set Signature

> **set** **baseColor**(`theColor`): `void`

Set object's global color or empty string to unset.

##### Parameters

###### theColor

`string`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`baseColor`](InteractiveObject.md#basecolor)

***

### baseColorRgba

#### Get Signature

> **get** **baseColorRgba**(): `string`

Return object's global RGBA color defined as [ColorString](../type-aliases/ColorString.md) or empty string if unspecified.
Note that if object has undefined color, but defined transparency - this property will return empty string!

##### Returns

`string`

#### Set Signature

> **set** **baseColorRgba**(`theColor`): `void`

Set object's global color + transparency or empty string to unset both.

##### Parameters

###### theColor

`string`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`baseColorRgba`](InteractiveObject.md#basecolorrgba)

***

### baseSize

#### Get Signature

> **get** **baseSize**(): [`IVec2`](../interfaces/IVec2.md)

Return scale ruler base size.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **baseSize**(`theSize`): `void`

Set scale ruler base size.

##### Parameters

###### theSize

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### eventListeners

#### Get Signature

> **get** **eventListeners**(): `Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

Return all event listeners.

##### Returns

`Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`eventListeners`](InteractiveObject.md#eventlisteners)

***

### id

#### Get Signature

> **get** **id**(): `string`

Get object id as [ObjectId](../type-aliases/ObjectId.md).

##### Returns

`string`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`id`](InteractiveObject.md#id)

***

### is2d

#### Get Signature

> **get** **is2d**(): `boolean`

Return if this presentation is defined in 2D or 3D space.

##### Returns

`boolean`

#### Set Signature

> **set** **is2d**(`theIs2d`): `void`

Set if this presentation is defined in 2D or 3D space.

##### Parameters

###### theIs2d

`boolean`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`is2d`](InteractiveObject.md#is2d)

***

### isAlive

#### Get Signature

> **get** **isAlive**(): `boolean`

Return TRUE if object with specified Id exists.

##### Returns

`boolean`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isAlive`](InteractiveObject.md#isalive)

***

### isEnabled

#### Get Signature

> **get** **isEnabled**(): `boolean`

Return object enabled state.

##### Returns

`boolean`

#### Set Signature

> **set** **isEnabled**(`theToEnable`): `void`

Set object enabled state.

##### Parameters

###### theToEnable

`boolean`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isEnabled`](InteractiveObject.md#isenabled)

***

### isInfiniteState

#### Get Signature

> **get** **isInfiniteState**(): `boolean`

Infinite presentation flag, FALSE by default.
Used to distinguish auxiliary presentations (construction elements, manipulators) from main scene objects.
Such objects are not considered within FitAll operation.

##### Returns

`boolean`

#### Set Signature

> **set** **isInfiniteState**(`theState`): `void`

Set infinite presentation flag.

##### Parameters

###### theState

`boolean`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isInfiniteState`](InteractiveObject.md#isinfinitestate)

***

### isLoaded

#### Get Signature

> **get** **isLoaded**(): `boolean`

Return TRUE if object with specified Id has deferred data loaded.

##### Returns

`boolean`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isLoaded`](InteractiveObject.md#isloaded)

***

### isSelected

#### Get Signature

> **get** **isSelected**(): `boolean`

Return TRUE if this object is listed within selected objects.

##### Returns

`boolean`

#### Set Signature

> **set** **isSelected**(`theToSelect`): `void`

Put/remove object from selected objects.

##### Parameters

###### theToSelect

`boolean`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isSelected`](InteractiveObject.md#isselected)

***

### isVisible

#### Get Signature

> **get** **isVisible**(): `boolean`

Return object visibility state.

##### Returns

`boolean`

#### Set Signature

> **set** **isVisible**(`theToShow`): `void`

Set object visibility state.

##### Parameters

###### theToShow

`boolean`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`isVisible`](InteractiveObject.md#isvisible)

***

### localTransformation

#### Get Signature

> **get** **localTransformation**(): [`ITrsf`](../interfaces/ITrsf.md)

Return local transformation.

##### Returns

[`ITrsf`](../interfaces/ITrsf.md)

#### Set Signature

> **set** **localTransformation**(`theLoc`): `void`

Set local transformation.

##### Parameters

###### theLoc

[`ITrsf`](../interfaces/ITrsf.md)

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`localTransformation`](InteractiveObject.md#localtransformation)

***

### minMax

#### Get Signature

> **get** **minMax**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return object AABB (axes-aligned bounding box).

##### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`minMax`](InteractiveObject.md#minmax)

***

### oldId

#### Get Signature

> **get** **oldId**(): `string`

Get object id before removal.

##### Returns

`string`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`oldId`](InteractiveObject.md#oldid)

***

### priorityModifier

#### Get Signature

> **get** **priorityModifier**(): `number`

Return a value that is added to the default displaying/selection object's priority, 0 by default.

##### Returns

`number`

#### Set Signature

> **set** **priorityModifier**(`theModifier`): `void`

Set a value that is added to the default displaying/selection object's priority.

##### Parameters

###### theModifier

`number`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`priorityModifier`](InteractiveObject.md#prioritymodifier)

***

### transparency

#### Get Signature

> **get** **transparency**(): `number`

Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified.

##### Returns

`number`

#### Set Signature

> **set** **transparency**(`theTransp`): `void`

Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset.

##### Parameters

###### theTransp

`number`

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`transparency`](InteractiveObject.md#transparency)

***

### type

#### Get Signature

> **get** **type**(): `string`

Return object type.

##### Returns

`string`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`type`](InteractiveObject.md#type)

***

### viewCorner

#### Get Signature

> **get** **viewCorner**(): [`ViewCorner`](../enumerations/ViewCorner.md)

Return view corner of 2D presentation; has no meaning for 3D objects.

##### Returns

[`ViewCorner`](../enumerations/ViewCorner.md)

#### Set Signature

> **set** **viewCorner**(`theCorner`): `void`

Set view corner.

##### Parameters

###### theCorner

[`ViewCorner`](../enumerations/ViewCorner.md)

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`viewCorner`](InteractiveObject.md#viewcorner)

***

### viewCornerOffset

#### Get Signature

> **get** **viewCornerOffset**(): [`IVec2`](../interfaces/IVec2.md)

Return view corner offset of 2D presentation; has no meaning for 3D objects.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **viewCornerOffset**(`theOffset`): `void`

Set view corner offset.

##### Parameters

###### theOffset

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`viewCornerOffset`](InteractiveObject.md#viewcorneroffset)

***

### visibleInView

#### Get Signature

> **get** **visibleInView**(): [`Subview`](Subview.md)

Return view in which object is visible or NULL if it is not displayed or displayed in multiple views.

##### Returns

[`Subview`](Subview.md)

#### Set Signature

> **set** **visibleInView**(`theView`): `void`

Set object to be visible only within specific view or all views (NULL) - view affinity.
Note that [isVisible](InteractiveObject.md#isvisible) still has to be set independently.

##### Parameters

###### theView

[`Subview`](Subview.md)

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`visibleInView`](InteractiveObject.md#visibleinview)

***

### zlayer

#### Get Signature

> **get** **zlayer**(): [`ZLayerId`](../enumerations/ZLayerId.md)

Return object's Z-Layer.

##### Returns

[`ZLayerId`](../enumerations/ZLayerId.md)

#### Set Signature

> **set** **zlayer**(`theLayer`): `void`

Set object's Z-Layer.

##### Parameters

###### theLayer

[`ZLayerId`](../enumerations/ZLayerId.md)

##### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`zlayer`](InteractiveObject.md#zlayer)

## Methods

### addEventListener()

> **addEventListener**(`theType`, `theListener`, `theOptions?`): `void`

Add event listener, but avoid inclusion of the same listener twice.

#### Parameters

##### theType

`string`

##### theListener

`EventListenerOrEventListenerObject`

##### theOptions?

`boolean` | `AddEventListenerOptions`

#### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`addEventListener`](InteractiveObject.md#addeventlistener)

***

### clone()

> **clone**(`theObjId`): `InteractiveScaleRuler`

Make a copy of existing object with new id

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveScaleRuler`

***

### dispatchEvent()

> **dispatchEvent**(`event`): `boolean`

The **`dispatchEvent()`** method of the EventTarget sends an Event to the object, (synchronously) invoking the affected event listeners in the appropriate order.

[MDN Reference](https://developer.mozilla.org/docs/Web/API/EventTarget/dispatchEvent)

#### Parameters

##### event

`Event`

#### Returns

`boolean`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`dispatchEvent`](InteractiveObject.md#dispatchevent)

***

### getEventListeners()

> **getEventListeners**(`theType`): `EventListenerOrEventListenerObject`[]

Return event listeners for specific type.

#### Parameters

##### theType

`string`

#### Returns

`EventListenerOrEventListenerObject`[]

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`getEventListeners`](InteractiveObject.md#geteventlisteners)

***

### getFilePathFromObjId()

> **getFilePathFromObjId**(): `string`

Return file path from object id.

#### Returns

`string`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`getFilePathFromObjId`](InteractiveObject.md#getfilepathfromobjid)

***

### loadDataAsync()

> `protected` **loadDataAsync**(`theViewer`): `Promise`\<`boolean`\>

Load deferred data async.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

module object

#### Returns

`Promise`\<`boolean`\>

FALSE if model cannot be opened

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`loadDataAsync`](InteractiveObject.md#loaddataasync)

***

### remove()

> **remove**(): `boolean`

Remove object and invalidates it's id.

#### Returns

`boolean`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`remove`](InteractiveObject.md#remove)

***

### removeAllEventListeners()

> **removeAllEventListeners**(`theType`, `theOptions?`): `void`

Remove all event listeners.

#### Parameters

##### theType

`string` = `''`

##### theOptions?

`boolean` | `EventListenerOptions`

#### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`removeAllEventListeners`](InteractiveObject.md#removealleventlisteners)

***

### removeEventListener()

> **removeEventListener**(`theType`, `theListener`, `theOptions?`): `void`

Remove event listener.

#### Parameters

##### theType

`string`

##### theListener

`EventListenerOrEventListenerObject`

##### theOptions?

`boolean` | `EventListenerOptions`

#### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`removeEventListener`](InteractiveObject.md#removeeventlistener)

***

### setEventListener()

> **setEventListener**(`theType`, `theListener`, `theOptions?`): `void`

Remove all event listeners for specific type and adds specified one.

#### Parameters

##### theType

`string`

##### theListener

`EventListenerOrEventListenerObject`

##### theOptions?

`boolean` | `AddEventListenerOptions`

#### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`setEventListener`](InteractiveObject.md#seteventlistener)

***

### setEventListeners()

> **setEventListeners**(`theType`, `theListeners`, `theOptions?`): `void`

Replace event listeners for specific type.

#### Parameters

##### theType

`string`

##### theListeners

`EventListenerOrEventListenerObject`[]

##### theOptions?

`boolean` | `AddEventListenerOptions`

#### Returns

`void`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`setEventListeners`](InteractiveObject.md#seteventlisteners)

***

### wrap()

> **wrap**(`theViewer`, `theId`): `boolean`

Wrap existing object.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theId

`string` = `''`

the id to wrap

#### Returns

`boolean`

FALSE if object with specified Id doesn't exist in the Viewer

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`wrap`](InteractiveObject.md#wrap)
