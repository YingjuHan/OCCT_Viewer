[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Subview

# Class: Subview

Subview definition - a region in canvas for displaying 3D viewer content.

Application may subscribe to the following object's events:
- [UserEvents.onSubviewFullscreen](../variables/UserEvents.md#onsubviewfullscreen);
- [UserEvents.onSubviewClick](../variables/UserEvents.md#onsubviewclick).

## Extends

- `EventTarget`

## Constructors

### Constructor

> **new Subview**(`theViewId`): `Subview`

Empty constructor.

#### Parameters

##### theViewId

`number` = `-2`

#### Returns

`Subview`

#### Overrides

`EventTarget.constructor`

## Accessors

### camera

#### Get Signature

> **get** **camera**(): [`Camera`](Camera.md)

Return camera controller.

##### Returns

[`Camera`](Camera.md)

***

### hasInputFocus

#### Get Signature

> **get** **hasInputFocus**(): `boolean`

Return TRUE if view receives input focus.

##### Returns

`boolean`

***

### id

#### Get Signature

> **get** **id**(): `number`

Get view id as [ViewId](../type-aliases/ViewId.md); -2 is invalid id, -1 refers to a view receiving input focus.

##### Returns

`number`

***

### isActive

#### Get Signature

> **get** **isActive**(): `boolean`

Return TRUE if view is active (displayed on the screen).

##### Returns

`boolean`

#### Set Signature

> **set** **isActive**(`theActive`): `void`

Set view active state.

##### Parameters

###### theActive

`boolean`

##### Returns

`void`

***

### isDefined

#### Get Signature

> **get** **isDefined**(): `boolean`

Return TRUE if view with specified Id exists.

##### Returns

`boolean`

***

### isSubviewClickable

#### Get Signature

> **get** **isSubviewClickable**(): `boolean`

Return TRUE if subview is clickable.

##### Returns

`boolean`

#### Set Signature

> **set** **isSubviewClickable**(`theToEnable`): `void`

Set if subview should be clickable.

##### Parameters

###### theToEnable

`boolean`

##### Returns

`void`

***

### isSubviewComposer

#### Get Signature

> **get** **isSubviewComposer**(): `boolean`

Return TRUE if view serves as a composer to subviews.

##### Returns

`boolean`

#### Set Signature

> **set** **isSubviewComposer**(`theComposer`): `void`

Set view composer state.

##### Parameters

###### theComposer

`boolean`

##### Returns

`void`

***

### isSubviewFullscreen

#### Get Signature

> **get** **isSubviewFullscreen**(): `boolean`

Return TRUE if subview should fill entire canvas (replaces all subviews).

##### Returns

`boolean`

#### Set Signature

> **set** **isSubviewFullscreen**(`theToEnable`): `void`

Set if subview should fill entire canvas (replace all subviews) or not.

##### Parameters

###### theToEnable

`boolean`

##### Returns

`void`

***

### subviewCorner

#### Get Signature

> **get** **subviewCorner**(): [`ViewCorner`](../enumerations/ViewCorner.md)

Return subview corner.

##### Returns

[`ViewCorner`](../enumerations/ViewCorner.md)

#### Set Signature

> **set** **subviewCorner**(`theCorner`): `void`

Set subview corner.

##### Parameters

###### theCorner

[`ViewCorner`](../enumerations/ViewCorner.md)

##### Returns

`void`

***

### subviewMargins

#### Get Signature

> **get** **subviewMargins**(): [`IVec2`](../interfaces/IVec2.md)

Return subview margins in pixels.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **subviewMargins**(`theMargins`): `void`

Set subview margins in pixels.

##### Parameters

###### theMargins

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### subviewOffset

#### Get Signature

> **get** **subviewOffset**(): [`IVec2`](../interfaces/IVec2.md)

Return subview offset from the corner as a fraction to canvas dimensions.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **subviewOffset**(`theOffset`): `void`

Set subview offset.

##### Parameters

###### theOffset

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### subviewSize

#### Get Signature

> **get** **subviewSize**(): [`IVec2`](../interfaces/IVec2.md)

Return subview size as a fraction to canvas dimensions.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **subviewSize**(`theSize`): `void`

Set subview size.

##### Parameters

###### theSize

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

## Methods

### addEventListener()

> **addEventListener**(`type`, `callback`, `options?`): `void`

The **`addEventListener()`** method of the EventTarget interface sets up a function that will be called whenever the specified event is delivered to the target.

[MDN Reference](https://developer.mozilla.org/docs/Web/API/EventTarget/addEventListener)

#### Parameters

##### type

`string`

##### callback

`EventListenerOrEventListenerObject`

##### options?

`boolean` | `AddEventListenerOptions`

#### Returns

`void`

#### Inherited from

`EventTarget.addEventListener`

***

### create()

> **create**(`theViewer`, `theSize`): `void`

Create new subview.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theSize

[`IVec2`](../interfaces/IVec2.md)

the subview size

#### Returns

`void`

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

`EventTarget.dispatchEvent`

***

### remove()

> **remove**(): `void`

Remove subview.

#### Returns

`void`

***

### removeEventListener()

> **removeEventListener**(`type`, `callback`, `options?`): `void`

The **`removeEventListener()`** method of the EventTarget interface removes an event listener previously registered with EventTarget.addEventListener() from the target.

[MDN Reference](https://developer.mozilla.org/docs/Web/API/EventTarget/removeEventListener)

#### Parameters

##### type

`string`

##### callback

`EventListenerOrEventListenerObject`

##### options?

`boolean` | `EventListenerOptions`

#### Returns

`void`

#### Inherited from

`EventTarget.removeEventListener`

***

### setInputFocus()

> **setInputFocus**(): `void`

Set this view to receive input focus.

#### Returns

`void`

***

### wrap()

> **wrap**(`theViewer`, `theId`): `boolean`

Wrap existing view.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theId

`number` = `-2`

the id to wrap

#### Returns

`boolean`

FALSE if view with specified Id doesn't exist in the Viewer
