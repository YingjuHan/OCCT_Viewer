[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ViewerEventManager

# Class: ViewerEventManager

Event Manager is a global object accessible via [SCwebviewer.eventManager](SCwebviewer.md#eventmanager),
responsible for managing event handlers (subscribing and unsubscribing callbacks).

## Extends

- [`ViewerEventTarget`](ViewerEventTarget.md)

## Constructors

### Constructor

> **new ViewerEventManager**(`theViewer`): `ViewerEventManager`

Main constructor.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

#### Returns

`ViewerEventManager`

#### Overrides

[`ViewerEventTarget`](ViewerEventTarget.md).[`constructor`](ViewerEventTarget.md#constructor)

## Accessors

### eventListeners

#### Get Signature

> **get** **eventListeners**(): `Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

Return all event listeners.

##### Returns

`Map`\<`string`, `EventListenerOrEventListenerObject`[]\>

#### Inherited from

[`ViewerEventTarget`](ViewerEventTarget.md).[`eventListeners`](ViewerEventTarget.md#eventlisteners)

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

### onColorMapperChanged()

> `protected` **onColorMapperChanged**(`theMeshId`, `thePropertyId`, `theComponentIndex`, `theColorScaleObjId`): `void`

Emit mesh color mapper change event

#### Parameters

##### theMeshId

`string`

##### thePropertyId

`string`

##### theComponentIndex

`number`

##### theColorScaleObjId

`string`

#### Returns

`void`

***

### onDetectedObjectChanged()

> `protected` **onDetectedObjectChanged**(): `boolean`

Emit object detection change event

#### Returns

`boolean`

***

### onMeshAnimationChanged()

> `protected` **onMeshAnimationChanged**(`theMeshId`, `theActiveTimeStep`, `theIsChanged`, `theIsRunning`): `void`

Emit mesh animation step change event

#### Parameters

##### theMeshId

`string`

##### theActiveTimeStep

`number`

##### theIsChanged

`boolean`

##### theIsRunning

`boolean`

#### Returns

`void`

***

### onMeshGenGeomRequested()

> `protected` **onMeshGenGeomRequested**(`theMeshId`, `thePrsId`, `thePartId`, `theParameters`): `void`

Emit mesh geometry request event

#### Parameters

##### theMeshId

`string`

##### thePrsId

`string`

##### thePartId

`number`

##### theParameters

`string`

#### Returns

`void`

***

### onMeshPropertiesRequested()

> `protected` **onMeshPropertiesRequested**(`theMeshId`, `theTimeStep`, `thePropsToLoad`): `void`

Emit mesh properties request event

#### Parameters

##### theMeshId

`string`

##### theTimeStep

`number`

##### thePropsToLoad

`string`

#### Returns

`void`

***

### onObjectDragged()

> `protected` **onObjectDragged**(`thePartId`, `theSubPart`, `theAction`): `boolean`

Emit object dragged event

#### Parameters

##### thePartId

`number`

##### theSubPart

`number`

##### theAction

`number`

#### Returns

`boolean`

***

### onObjectMouseClick()

> `protected` **onObjectMouseClick**(`theObjId`, `thePart`, `theSubPart`, `theX`, `theY`, `theFlags`): `boolean`

Emit object click event

#### Parameters

##### theObjId

`string`

##### thePart

`number`

##### theSubPart

`number`

##### theX

`number`

##### theY

`number`

##### theFlags

`number`

#### Returns

`boolean`

***

### onSelectedObjectsChanged()

> `protected` **onSelectedObjectsChanged**(): `boolean`

Emit objects selection change event

#### Returns

`boolean`

***

### onSubviewClick()

> `protected` **onSubviewClick**(`theViewId`): `void`

Emit subview click event

#### Parameters

##### theViewId

`number`

#### Returns

`void`

***

### onSubviewFullscreen()

> `protected` **onSubviewFullscreen**(`theViewId`, `theIsFullscreen`): `boolean`

Emit subview fullscreen mode change event

#### Parameters

##### theViewId

`number`

##### theIsFullscreen

`boolean`

#### Returns

`boolean`

***

### onUnitsChanged()

> `protected` **onUnitsChanged**(`theQuantType`, `theSection`): `void`

Emit units change event

#### Parameters

##### theQuantType

`number`

##### theSection

`number`

#### Returns

`void`

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
