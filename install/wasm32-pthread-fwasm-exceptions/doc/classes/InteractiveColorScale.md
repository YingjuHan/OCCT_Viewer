[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveColorScale

# Class: InteractiveColorScale

Interactive Color Scale Object.

Application may subscribe to the following additional events:
- [UserEvents.onColorMapperChanged](../variables/UserEvents.md#oncolormapperchanged), when color map is changed.

## Extends

- [`InteractiveObject`](InteractiveObject.md)

## Constructors

### Constructor

> **new InteractiveColorScale**(`theViewer`, `theObjId`): `InteractiveColorScale`

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### theObjId

`string` = `''`

#### Returns

`InteractiveColorScale`

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

### customColors

#### Get Signature

> **get** **customColors**(): `string`[]

Get scale object customColors parameter.

##### Returns

`string`[]

#### Set Signature

> **set** **customColors**(`theColors`): `void`

Set scale object customColors parameter
and automatically sets isCustomColors if list is empty or not.

##### Parameters

###### theColors

`string`[]

##### Returns

`void`

***

### customLabels

#### Get Signature

> **get** **customLabels**(): `string`[]

Get scale object customLabels parameter.

##### Returns

`string`[]

#### Set Signature

> **set** **customLabels**(`theLabels`): `void`

Set scale object customLabels parameter; have effect only with isCustomColors=true.

##### Parameters

###### theLabels

`string`[]

##### Returns

`void`

***

### defaultRange

#### Get Signature

> **get** **defaultRange**(): [`IVec2`](../interfaces/IVec2.md)

Get default value range (actual range of property values).

##### Returns

[`IVec2`](../interfaces/IVec2.md)

***

### displayRange

#### Get Signature

> **get** **displayRange**(): [`IVec2`](../interfaces/IVec2.md)

Get display value range.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **displayRange**(`theRange`): `void`

Set display value range.

##### Parameters

###### theRange

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### displayUnits

#### Get Signature

> **get** **displayUnits**(): `string`

Get scale display units.

##### Returns

`string`

#### Set Signature

> **set** **displayUnits**(`theUnits`): `void`

Set scale display units.

##### Parameters

###### theUnits

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

### intervalHighlightColor

#### Get Signature

> **get** **intervalHighlightColor**(): `string`

Get color used for detected interval if isCustomColors = true.

##### Returns

`string`

#### Set Signature

> **set** **intervalHighlightColor**(`theUnits`): `void`

Set color used for detected interval if isCustomColors = true.

##### Parameters

###### theUnits

`string`

##### Returns

`void`

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

### isCustomColors

#### Get Signature

> **get** **isCustomColors**(): `boolean`

Get scale object isCustomColors parameter.

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

### isLogarithmic

#### Get Signature

> **get** **isLogarithmic**(): `boolean`

Returns true if colorscale is logarithmic.

##### Returns

`boolean`

#### Set Signature

> **set** **isLogarithmic**(`theIsLogarithmic`): `void`

Sets whether the colorscale is logarithmic.

##### Parameters

###### theIsLogarithmic

`boolean`

##### Returns

`void`

***

### isReversed

#### Get Signature

> **get** **isReversed**(): `boolean`

Get scale object isReversed parameter.

##### Returns

`boolean`

#### Set Signature

> **set** **isReversed**(`theToReverse`): `void`

Set scale object isReversed parameter.

##### Parameters

###### theToReverse

`boolean`

##### Returns

`void`

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

[`InteractiveSphericalAxes`](InteractiveSphericalAxes.md).[`isSelected`](InteractiveSphericalAxes.md#isselected)

***

### isSmooth

#### Get Signature

> **get** **isSmooth**(): `boolean`

Get scale object isSmooth parameter.

##### Returns

`boolean`

#### Set Signature

> **set** **isSmooth**(`theIsSmooth`): `void`

Set scale object isSmooth parameter.

##### Parameters

###### theIsSmooth

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

#### Inherited from

[`InteractiveSphericalAxes`](InteractiveSphericalAxes.md).[`isVisible`](InteractiveSphericalAxes.md#isvisible)

***

### labelSize

#### Get Signature

> **get** **labelSize**(): `number`

Get scale object labelSize parameter.

##### Returns

`number`

#### Set Signature

> **set** **labelSize**(`theLabelSize`): `void`

Set scale object labelSize parameter.

##### Parameters

###### theLabelSize

`number`

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

### minMaxColors

#### Get Signature

> **get** **minMaxColors**(): `IColorScaleMinMaxColors`

Get scale object minColor/maxColor parameters.

##### Returns

`IColorScaleMinMaxColors`

#### Set Signature

> **set** **minMaxColors**(`colors`): `void`

Set scale object minColor/maxColor parameters.

##### Parameters

###### colors

`IColorScaleMinMaxColors`

##### Returns

`void`

***

### nbIntervals

#### Get Signature

> **get** **nbIntervals**(): `number`

Get scale object nbIntervals parameter.

##### Returns

`number`

#### Set Signature

> **set** **nbIntervals**(`theNbIntervals`): `void`

Set scale object nbIntervals parameter.

##### Parameters

###### theNbIntervals

`number`

##### Returns

`void`

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

### outOfRangeColor

#### Get Signature

> **get** **outOfRangeColor**(): `string`

Get color for out-of-range values (including alpha).

##### Returns

`string`

#### Set Signature

> **set** **outOfRangeColor**(`theUnits`): `void`

Set color for out-of-range values (including alpha).

##### Parameters

###### theUnits

`string`

##### Returns

`void`

***

### params

#### Get Signature

> **get** **params**(): [`IColorScaleParams`](../interfaces/IColorScaleParams.md)

Get scale object parameters.

##### Returns

[`IColorScaleParams`](../interfaces/IColorScaleParams.md)

#### Set Signature

> **set** **params**(`theParams`): `void`

Set scale object parameters.

##### Parameters

###### theParams

[`IColorScaleParams`](../interfaces/IColorScaleParams.md)

##### Returns

`void`

***

### pos

#### Get Signature

> **get** **pos**(): [`IVec2`](../interfaces/IVec2.md)

Get scale object pos parameter.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **pos**(`thePos`): `void`

Set scale object pos parameter.

##### Parameters

###### thePos

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### positionSide

#### Get Signature

> **get** **positionSide**(): [`ViewCorner`](../enumerations/ViewCorner.md)

Returns the side of the screen to which the colorscale is attached.

##### Returns

[`ViewCorner`](../enumerations/ViewCorner.md)

#### Set Signature

> **set** **positionSide**(`thePosSide`): `void`

Sets the side of the screen to which the colorscale is attached.

##### Parameters

###### thePosSide

[`ViewCorner`](../enumerations/ViewCorner.md)

##### Returns

`void`

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

### selectedRange

#### Get Signature

> **get** **selectedRange**(): [`IVec2`](../interfaces/IVec2.md)

Get selected range.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **selectedRange**(`theRange`): `void`

Set selected range.

##### Parameters

###### theRange

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### size

#### Get Signature

> **get** **size**(): [`IVec2`](../interfaces/IVec2.md)

Get scale object size parameter.

##### Returns

[`IVec2`](../interfaces/IVec2.md)

#### Set Signature

> **set** **size**(`theSize`): `void`

Set scale object size parameter.

##### Parameters

###### theSize

[`IVec2`](../interfaces/IVec2.md)

##### Returns

`void`

***

### title

#### Get Signature

> **get** **title**(): `string`

Get scale object title.

##### Returns

`string`

#### Set Signature

> **set** **title**(`theTitle`): `void`

Set scale object title.

##### Parameters

###### theTitle

`string`

##### Returns

`void`

***

### toHighlightIntervals

#### Get Signature

> **get** **toHighlightIntervals**(): `boolean`

Get flag to use special color for detected interval if isCustomColors = true.

##### Returns

`boolean`

#### Set Signature

> **set** **toHighlightIntervals**(`theToHighlight`): `void`

Set flag to use special color for detected interval if isCustomColors = true.

##### Parameters

###### theToHighlight

`boolean`

##### Returns

`void`

***

### toSkipEmptyLabels

#### Get Signature

> **get** **toSkipEmptyLabels**(): `boolean`

Get flag to skip empty custom labels in legend within non-smoothed presentation.

##### Returns

`boolean`

#### Set Signature

> **set** **toSkipEmptyLabels**(`theToSkip`): `void`

Set flag to skip empty custom labels in legend within non-smoothed presentation.

##### Parameters

###### theToSkip

`boolean`

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

[`InteractiveSphericalAxes`](InteractiveSphericalAxes.md).[`viewCorner`](InteractiveSphericalAxes.md#viewcorner)

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

### attach()

> **attach**(`theObjId`, `thePropertyId`, `theComponentIndex`): `void`

Attach this color scale object to mesh object for specified property.

#### Parameters

##### theObjId

`string` | [`InteractiveObject`](InteractiveObject.md)

##### thePropertyId

`string`

##### theComponentIndex

`number`

#### Returns

`void`

***

### clone()

> **clone**(`theObjId`): `InteractiveColorScale`

Make a copy of existing object with new id

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveColorScale`

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
