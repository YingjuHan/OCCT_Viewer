[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveObject

# Class: InteractiveObject

Interactive Object to be displayed in the Viewer.
This class stores a weak reference to the real object in memory identified by a string id [ObjectId](../type-aliases/ObjectId.md).
User might specify id explicitly at construction time or rely on an id generated from loaded file URL.
Beware to use unique ids - creating multiple objects with the same id will result in logical issues!

Usual lifetime of the object:
- Create an empty instance of subclass (object is not bound to Viewer);
- Construct or load object from the given URL (object is registered in the Viewer);
- Show/hide/modify object in the Viewer (e.g. [InteractiveObject.isVisible](#isvisible));
- Remove object from the Viewer (object is unregistered from the Viewer and memory released) via [InteractiveObject.remove](#remove);
  alternatively, [InteractiveContext.removeAll](../interfaces/InteractiveContext.md#removeall) interface could be used to remove all registered objects,
  in which case InteractiveObject instances will keep dead references ([InteractiveObject.isAlive](#isalive)).

Application may subscribe to the following object's events:
- [UserEvents.onObjectDragged](../variables/UserEvents.md#onobjectdragged), when object is dragged;
- [UserEvents.onObjectMouseClick](../variables/UserEvents.md#onobjectmouseclick), when mouse is clicked over object;
- [UserEvents.onDetectedObjectChanged](../variables/UserEvents.md#ondetectedobjectchanged), when object become detected (dynamically highlighted);
- [UserEvents.onSelectedObjectsChanged](../variables/UserEvents.md#onselectedobjectschanged), when object become selected;

## Extends

- [`ViewerEventTarget`](ViewerEventTarget.md)

## Extended by

- [`InteractiveColorScale`](InteractiveColorScale.md)
- [`InteractiveShape`](InteractiveShape.md)
- [`InteractiveCaeMesh`](InteractiveCaeMesh.md)
- [`InteractiveClipping`](InteractiveClipping.md)
- [`InteractiveScaleRuler`](InteractiveScaleRuler.md)
- [`InteractiveTextLabel`](InteractiveTextLabel.md)
- [`InteractiveDimension`](InteractiveDimension.md)
- [`InteractiveViewCube`](InteractiveViewCube.md)
- [`InteractiveSphericalAxes`](InteractiveSphericalAxes.md)

## Constructors

### Constructor

> **new InteractiveObject**(`theObjId`): `InteractiveObject`

Empty constructor.

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveObject`

#### Overrides

[`ViewerEventTarget`](ViewerEventTarget.md).[`constructor`](ViewerEventTarget.md#constructor)

## Accessors

### aspects

#### Get Signature

> **get** **aspects**(): [`PresentationAspects`](PresentationAspects.md)

Return aspects manager.

##### Returns

[`PresentationAspects`](PresentationAspects.md)

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

***

### eventListeners

#### Get Signature

> **get** **eventListeners**(): `Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

Return all event listeners.

##### Returns

`Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

#### Inherited from

[`ViewerEventTarget`](ViewerEventTarget.md).[`eventListeners`](ViewerEventTarget.md#eventlisteners)

***

### id

#### Get Signature

> **get** **id**(): `string`

Get object id as [ObjectId](../type-aliases/ObjectId.md).

##### Returns

`string`

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

***

### isAlive

#### Get Signature

> **get** **isAlive**(): `boolean`

Return TRUE if object with specified Id exists.

##### Returns

`boolean`

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

***

### isLoaded

#### Get Signature

> **get** **isLoaded**(): `boolean`

Return TRUE if object with specified Id has deferred data loaded.

##### Returns

`boolean`

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

***

### minMax

#### Get Signature

> **get** **minMax**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return object AABB (axes-aligned bounding box).

##### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

***

### oldId

#### Get Signature

> **get** **oldId**(): `string`

Get object id before removal.

##### Returns

`string`

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

***

### type

#### Get Signature

> **get** **type**(): `string`

Return object type.

##### Returns

`string`

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
Note that [isVisible](#isvisible) still has to be set independently.

##### Parameters

###### theView

[`Subview`](Subview.md)

##### Returns

`void`

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`addEventListener`](ViewerEventTarget.md#addeventlistener)

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`dispatchEvent`](ViewerEventTarget.md#dispatchevent)

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`getEventListeners`](ViewerEventTarget.md#geteventlisteners)

***

### getFilePathFromObjId()

> **getFilePathFromObjId**(): `string`

Return file path from object id.

#### Returns

`string`

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

***

### remove()

> **remove**(): `boolean`

Remove object and invalidates it's id.

#### Returns

`boolean`

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`removeAllEventListeners`](ViewerEventTarget.md#removealleventlisteners)

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`removeEventListener`](ViewerEventTarget.md#removeeventlistener)

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`setEventListener`](ViewerEventTarget.md#seteventlistener)

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

[`ViewerEventTarget`](ViewerEventTarget.md).[`setEventListeners`](ViewerEventTarget.md#seteventlisteners)

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
