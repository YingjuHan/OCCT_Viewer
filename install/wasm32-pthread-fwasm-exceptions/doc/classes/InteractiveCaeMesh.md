[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveCaeMesh

# Class: InteractiveCaeMesh

Interactive Mesh Object.

Application may subscribe to the following additional events:
- [UserEvents.onColorMapperChanged](../variables/UserEvents.md#oncolormapperchanged), when color map is changed.

## Extends

- [`InteractiveObject`](InteractiveObject.md)

## Constructors

### Constructor

> **new InteractiveCaeMesh**(`theObjId`): `InteractiveCaeMesh`

Empty constructor.

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveCaeMesh`

#### Overrides

[`InteractiveObject`](InteractiveObject.md).[`constructor`](InteractiveObject.md#constructor)

## Accessors

### activeTimeStep

#### Get Signature

> **get** **activeTimeStep**(): `number`

Return active time index.

##### Returns

`number`

#### Set Signature

> **set** **activeTimeStep**(`theTimeStep`): `void`

Set active time index.

##### Parameters

###### theTimeStep

`number`

##### Returns

`void`

***

### animation

#### Get Signature

> **get** **animation**(): [`CaeMeshAnimation`](CaeMeshAnimation.md)

Return animation object.

##### Returns

[`CaeMeshAnimation`](CaeMeshAnimation.md)

***

### aspects

#### Get Signature

> **get** **aspects**(): [`PresentationAspects`](PresentationAspects.md)

Return aspects manager.

##### Returns

[`PresentationAspects`](PresentationAspects.md)

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`aspects`](InteractiveObject.md#aspects)

***

### availableTimeSteps

#### Get Signature

> **get** **availableTimeSteps**(): `ICaeMeshTimeStep`[]

Return available time indices.

##### Returns

`ICaeMeshTimeStep`[]

#### Set Signature

> **set** **availableTimeSteps**(`theTimeSteps`): `void`

Set available time indices.

##### Parameters

###### theTimeSteps

`ICaeMeshTimeStep`[]

##### Returns

`void`

***

### availableVec3Props

#### Get Signature

> **get** **availableVec3Props**(): `ICaeMeshIdName`[]

Return available vector mesh properties.

##### Returns

`ICaeMeshIdName`[]

***

### axesNamingParams

#### Get Signature

> **get** **axesNamingParams**(): [`ICaeMesh2DAxesNamingInfo`](../interfaces/ICaeMesh2DAxesNamingInfo.md)

Get axes names and tick mark labels.

##### Returns

[`ICaeMesh2DAxesNamingInfo`](../interfaces/ICaeMesh2DAxesNamingInfo.md)

#### Set Signature

> **set** **axesNamingParams**(`theInfo`): `void`

Set axes names and tick mark labels.

##### Parameters

###### theInfo

[`ICaeMesh2DAxesNamingInfo`](../interfaces/ICaeMesh2DAxesNamingInfo.md)

##### Returns

`void`

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

### blocks

#### Get Signature

> **get** **blocks**(): [`CaeMeshBlock`](CaeMeshBlock.md)

Return mesh blocks.

##### Returns

[`CaeMeshBlock`](CaeMeshBlock.md)

***

### boundingBox

#### Get Signature

> **get** **boundingBox**(): [`IBndBox3`](../interfaces/IBndBox3.md)

Return mesh bounding box.

##### Returns

[`IBndBox3`](../interfaces/IBndBox3.md)

***

### colorScaleObjects

#### Get Signature

> **get** **colorScaleObjects**(): [`InteractiveColorScale`](InteractiveColorScale.md)[]

Return attached color scale objects.

##### Returns

[`InteractiveColorScale`](InteractiveColorScale.md)[]

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

### highlightColor

#### Get Signature

> **get** **highlightColor**(): `string`

Return entities selection color, "#CCCCCC33" by default.
Combined from selection shading color/transparency.
Kept for API compatibility.

##### Returns

`string`

#### Set Signature

> **set** **highlightColor**(`theColor`): `void`

Set entities selection color.
Affect selection shading color/transparency.
Kept for API compatibility.

##### Parameters

###### theColor

`string`

##### Returns

`void`

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

### info

#### Get Signature

> **get** **info**(): [`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md)

Return mesh info.

##### Returns

[`ICaeMeshInfo`](../interfaces/ICaeMeshInfo.md)

***

### infoString

#### Get Signature

> **get** **infoString**(): `string`

Return mesh info string.

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

[`InteractiveSphericalAxes`](InteractiveSphericalAxes.md).[`isSelected`](InteractiveSphericalAxes.md#isselected)

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

### presentations

#### Get Signature

> **get** **presentations**(): [`CaeMeshPresentation`](CaeMeshPresentation.md)[]

Return all mesh presentations.

##### Returns

[`CaeMeshPresentation`](CaeMeshPresentation.md)[]

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

### properties

#### Get Signature

> **get** **properties**(): [`ICaeMeshPropInfo`](../interfaces/ICaeMeshPropInfo.md)[]

Return mesh properties.

##### Returns

[`ICaeMeshPropInfo`](../interfaces/ICaeMeshPropInfo.md)[]

***

### selectionMode

#### Get Signature

> **get** **selectionMode**(): `number`

Return object's selection mode.

##### Returns

`number`

#### Set Signature

> **set** **selectionMode**(`theMode`): `void`

Set object's selection mode.

##### Parameters

###### theMode

`number`

##### Returns

`void`

***

### toCacheProperties

#### Set Signature

> **set** **toCacheProperties**(`theToCache`): `void`

Define if properties data is stored in MDB file and can be loaded/unloaded on demand.

##### Parameters

###### theToCache

`boolean`

##### Returns

`void`

***

### toHighlightEntities

#### Get Signature

> **get** **toHighlightEntities**(): `boolean`

Check if mesh entities should be detected/selected.

##### Returns

`boolean`

#### Set Signature

> **set** **toHighlightEntities**(`theToHighlight`): `void`

Define if mesh entities should be detected/selected.

##### Parameters

###### theToHighlight

`boolean`

##### Returns

`void`

***

### toShow2DGrid

#### Get Signature

> **get** **toShow2DGrid**(): `boolean`

Check whether the mesh draws a grid.

##### Returns

`boolean`

#### Set Signature

> **set** **toShow2DGrid**(`theToShow`): `void`

Set whether the mesh draws a grid.

##### Parameters

###### theToShow

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

### clone()

> **clone**(`theObjId`): `InteractiveCaeMesh`

Make a copy of existing object with new id

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveCaeMesh`

***

### createPresentation()

> **createPresentation**(`theType`, `thePrsId`): [`CaeMeshPresentation`](CaeMeshPresentation.md)

Create new mesh presentation of the specified type.

#### Parameters

##### theType

[`CaeMeshPresentationType`](../enumerations/CaeMeshPresentationType.md)

##### thePrsId

`string` = `''`

#### Returns

[`CaeMeshPresentation`](CaeMeshPresentation.md)

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

### getComponentDescription()

> **getComponentDescription**(`thePropertyId`, `theComponentIndex`): `string`

Return descrition for mesh property component.

#### Parameters

##### thePropertyId

`string`

##### theComponentIndex

`number`

#### Returns

`string`

***

### getComponentName()

> **getComponentName**(`thePropertyId`, `theComponentIndex`): `string`

Return name for mesh property component.

#### Parameters

##### thePropertyId

`string`

##### theComponentIndex

`number`

#### Returns

`string`

***

### getDetectedEntity()

> **getDetectedEntity**(): [`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)

Return detected sub entity.

#### Returns

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)

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

### getPresentation()

> **getPresentation**(`thePrsId`): [`CaeMeshPresentation`](CaeMeshPresentation.md)

Get presentation by id.

#### Parameters

##### thePrsId

`string`

#### Returns

[`CaeMeshPresentation`](CaeMeshPresentation.md)

***

### getPropertyDescription()

> **getPropertyDescription**(`thePropertyId`): `string`

Return descrition for mesh property.

#### Parameters

##### thePropertyId

`string`

#### Returns

`string`

***

### getPropertyDisplayUnits()

> **getPropertyDisplayUnits**(`thePropertyId`, `theComponentIndex`): `string`

Return display units for specified mesh property.

#### Parameters

##### thePropertyId

`string`

##### theComponentIndex

`number`

#### Returns

`string`

***

### getPropertyIdByName()

> **getPropertyIdByName**(`thePropertyName`): `string`

Find property id by name.

#### Parameters

##### thePropertyName

`string`

#### Returns

`string`

***

### getPropertyName()

> **getPropertyName**(`thePropertyId`): `string`

Return name for mesh property.

#### Parameters

##### thePropertyId

`string`

#### Returns

`string`

***

### getPropertyValueRange()

> **getPropertyValueRange**(`thePropertyId`, `theComponentIndex`): [`IVec2`](../interfaces/IVec2.md)

Return value range for mesh property.

#### Parameters

##### thePropertyId

`string`

##### theComponentIndex

`number`

#### Returns

[`IVec2`](../interfaces/IVec2.md)

***

### getSelectedEntities()

> **getSelectedEntities**(): [`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)[]

Return selected sub entities.

#### Returns

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)[]

***

### isPropertyElemental()

> **isPropertyElemental**(`thePropertyId`): `boolean`

Return true if mesh property is elemental.

#### Parameters

##### thePropertyId

`string`

#### Returns

`boolean`

***

### isPropertyGroup()

> **isPropertyGroup**(`thePropertyId`): `boolean`

Return true if mesh property is a group.

#### Parameters

##### thePropertyId

`string`

#### Returns

`boolean`

***

### isPropertySingleScalar()

> **isPropertySingleScalar**(`thePropertyId`): `boolean`

Return true if mesh property contains one scalar component.

#### Parameters

##### thePropertyId

`string`

#### Returns

`boolean`

***

### loadBufferAsync()

> **loadBufferAsync**(`theViewer`, `theArray`, `theFilePath`): `Promise`\<`boolean`\>

Open object from the given ArrayBuffer and register presentation(s) in the hidden (erased) state.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theArray

`ArrayBuffer`

buffer to open

##### theFilePath

`string`

the file path to map

#### Returns

`Promise`\<`boolean`\>

FALSE if model cannot be opened

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

### loadPropertiesAsync()

> **loadPropertiesAsync**(`theViewer`, `theArray`): `Promise`\<`boolean`\>

Load properties from the given ArrayBuffer and append to the current object.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theArray

`ArrayBuffer`

buffer to load

#### Returns

`Promise`\<`boolean`\>

FALSE if properties cannot be loaded

***

### loadUrlAsync()

> **loadUrlAsync**(`theViewer`, `theFileUrl`): `Promise`\<`boolean`\>

Open object from the given URL and register presentation(s) in the hidden (erased) state.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theFileUrl

`string`

the file URL address

#### Returns

`Promise`\<`boolean`\>

FALSE if model cannot be opened

***

### property()

> **property**(`thePropertyId`): [`ICaeMeshPropInfo`](../interfaces/ICaeMeshPropInfo.md)

Return mesh property.

#### Parameters

##### thePropertyId

`string`

#### Returns

[`ICaeMeshPropInfo`](../interfaces/ICaeMeshPropInfo.md)

***

### propertyComponent()

> **propertyComponent**(`thePropertyId`, `theCompIndex`): [`ICaeMeshPropCompInfo`](../interfaces/ICaeMeshPropCompInfo.md)

Return mesh property component.

#### Parameters

##### thePropertyId

`string`

##### theCompIndex

`number`

#### Returns

[`ICaeMeshPropCompInfo`](../interfaces/ICaeMeshPropCompInfo.md)

***

### remove()

> **remove**(): `boolean`

Remove object and invalidates it's id.

#### Returns

`boolean`

#### Overrides

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

### removePresentation()

> **removePresentation**(`thePrs`): `void`

Remove mesh presentation.

#### Parameters

##### thePrs

[`CaeMeshPresentation`](CaeMeshPresentation.md)

#### Returns

`void`

***

### removeProperties()

> **removeProperties**(`theWithMetadata`): `void`

Remove all properties.

#### Parameters

##### theWithMetadata

`boolean` = `true`

#### Returns

`void`

***

### save()

> **save**(`theFile`): `boolean`

Dump mesh into a file.

#### Parameters

##### theFile

`string` = `''`

#### Returns

`boolean`

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

### setSelectedEntity()

> **setSelectedEntity**(`theEntity`): `Promise`\<`boolean`\>

Select sub entity.

#### Parameters

##### theEntity

[`ISelectionMeshEntity`](../interfaces/ISelectionMeshEntity.md)

#### Returns

`Promise`\<`boolean`\>

***

### setUseGenGeometry()

> **setUseGenGeometry**(): `void`

Set to use custom geometry for mesh presentations.

#### Returns

`void`

***

### setUseTestCacheGenGeometry()

> **setUseTestCacheGenGeometry**(): `void`

Set to use test custom geometry for mesh presentations.

#### Returns

`void`

***

### usedGroups()

> **usedGroups**(`thePropertyId`): `Map`\<`number`, `string`\>

Return map of groups with non-zero number of entities.

#### Parameters

##### thePropertyId

`string`

#### Returns

`Map`\<`number`, `string`\>

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

***

### probeFileFormat()

> `static` **probeFileFormat**(`theViewer`, `theArray`, `theFilePath`): `string`

Probe file header for known mesh formats.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theArray

`ArrayBuffer`

buffer to open

##### theFilePath

`string`

the file path to map

#### Returns

`string`

detected format (if supported) or an empty string
