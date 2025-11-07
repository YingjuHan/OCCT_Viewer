[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveShape

# Class: InteractiveShape

Interactive Shape Object.

Usual loading workflow:
```typescript
  var scViewer = new SCwebviewer.SCwebviewer('/');
  ...
  let aShape = new SCwebviewer.InteractiveShape();
  aShape.loadUrlAsync (scViewer, "/models/shape.stl").then(() => {
    aShape.isVisible = true;
    scViewer.camera.fitAllAuto (true);
    scViewer.requestViewUpdate();
  })
  .catch(anErr => {
    console.error ("Loading shape failed: " + anErr);
  });
```

Properties like [InteractiveShape.baseColor](#basecolor), [InteractiveShape.transparency](#transparency), [InteractiveShape.materialMetallic](#materialmetallic),
[InteractiveShape.materialRoughness](#materialroughness), and [InteractiveShape.setMaterialPreset](#setmaterialpreset) could be used to specify object's material.
When object is loaded from file formats like glTF, it will use materials defined in the file, which could be assigned to sub-shapes.
In this case, material properties mentioned above will return empty values, and assigning them will override materials of all sub-shapes.

## Extends

- [`InteractiveObject`](InteractiveObject.md)

## Constructors

### Constructor

> **new InteractiveShape**(`theObjId`): `InteractiveShape`

Empty constructor.

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveShape`

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

Return object's base color (overrides base implementation).

##### Returns

`string`

#### Set Signature

> **set** **baseColor**(`theColor`): `void`

Set object's base color (overrides base implementation).

##### Parameters

###### theColor

`string`

##### Returns

`void`

#### Overrides

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

### coloredEdges

#### Get Signature

> **get** **coloredEdges**(): `number`[]

Return index list of colored edges

##### Returns

`number`[]

***

### coloredFaces

#### Get Signature

> **get** **coloredFaces**(): `number`[]

Return index list of colored faces

##### Returns

`number`[]

***

### coloredVertices

#### Get Signature

> **get** **coloredVertices**(): `number`[]

Return index list of colored vertices

##### Returns

`number`[]

***

### compSolids

#### Get Signature

> **get** **compSolids**(): [`SubshapeManager`](SubshapeManager.md)

Return composite solid subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

***

### definedSubshapes

#### Get Signature

> **get** **definedSubshapes**(): `number`[]

Return the list with indices of available subshapes within active selection mode.

##### Returns

`number`[]

***

### edges

#### Get Signature

> **get** **edges**(): [`SubshapeManager`](SubshapeManager.md)

Return edge subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

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

### expCompSolids

#### Get Signature

> **get** **expCompSolids**(): `SubshapeManagerProxy`

Return composite solid subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expEdges

#### Get Signature

> **get** **expEdges**(): `SubshapeManagerProxy`

Return edge subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expFaces

#### Get Signature

> **get** **expFaces**(): `SubshapeManagerProxy`

Return face subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expShells

#### Get Signature

> **get** **expShells**(): `SubshapeManagerProxy`

Return shell subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expSolids

#### Get Signature

> **get** **expSolids**(): `SubshapeManagerProxy`

Return solid subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expVertices

#### Get Signature

> **get** **expVertices**(): `SubshapeManagerProxy`

Return vertex subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### expWires

#### Get Signature

> **get** **expWires**(): `SubshapeManagerProxy`

Return wire subshape manager with interface for accessing to subshape by index.

##### Returns

`SubshapeManagerProxy`

***

### faces

#### Get Signature

> **get** **faces**(): [`SubshapeManager`](SubshapeManager.md)

Return face subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

***

### forcedColor

#### Get Signature

> **get** **forcedColor**(): `string`

Return object's overcolor (forced color), which overrides base/subshape colors, or an empty string.

##### Returns

`string`

#### Set Signature

> **set** **forcedColor**(`theColor`): `void`

Set or unsets object's overcolor.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### forcedTransparency

#### Get Signature

> **get** **forcedTransparency**(): `number`

Return object's overtransparency (forced transparency), which overrides base/subshape transparency, or -1 if unset.

##### Returns

`number`

#### Set Signature

> **set** **forcedTransparency**(`theTransp`): `void`

Set or unsets object's overtransparency.

##### Parameters

###### theTransp

`number`

##### Returns

`void`

***

### groups

#### Get Signature

> **get** **groups**(): [`SubshapeManager`](SubshapeManager.md)

Return selection groups subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

***

### hasMaterial

#### Get Signature

> **get** **hasMaterial**(): `boolean`

Return TRUE if object's material is set.

##### Returns

`boolean`

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

### infoString

#### Get Signature

> **get** **infoString**(): `string`

Return shape info string.

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

### lastDetectedSubshape

#### Get Signature

> **get** **lastDetectedSubshape**(): `number`

Return the last detected subshape index, or `-1` if unspecified.

##### Returns

`number`

***

### lastDetectedSubshapeDump

#### Get Signature

> **get** **lastDetectedSubshapeDump**(): `string`

Return the last detected subshape dump.

##### Returns

`string`

***

### lastDetectedSubshapeInfo

#### Get Signature

> **get** **lastDetectedSubshapeInfo**(): [`IShapeInfo`](../interfaces/IShapeInfo.md)

Return the last detected subshape info.

##### Returns

[`IShapeInfo`](../interfaces/IShapeInfo.md)

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

### material

#### Get Signature

> **get** **material**(): [`IVisMaterial`](../interfaces/IVisMaterial.md)

Return object's material.

##### Returns

[`IVisMaterial`](../interfaces/IVisMaterial.md)

#### Set Signature

> **set** **material**(`theMat`): `void`

Set object's material.

##### Parameters

###### theMat

[`IVisMaterial`](../interfaces/IVisMaterial.md)

##### Returns

`void`

***

### materialMetallic

#### Get Signature

> **get** **materialMetallic**(): `number`

Return object's metallic factor within `[0.0, 1.0]` range.

##### Returns

`number`

#### Set Signature

> **set** **materialMetallic**(`theFactor`): `void`

Set object's metallic factor.

##### Parameters

###### theFactor

`number`

##### Returns

`void`

***

### materialRoughness

#### Get Signature

> **get** **materialRoughness**(): `number`

Return object's roughness within `[0.0, 1.0]` range.

##### Returns

`number`

#### Set Signature

> **set** **materialRoughness**(`theFactor`): `void`

Set object's roughness.

##### Parameters

###### theFactor

`number`

##### Returns

`void`

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

### presentationMode

#### Get Signature

> **get** **presentationMode**(): [`ShapeObjectPrsMode`](../enumerations/ShapeObjectPrsMode.md)

Return object's presentation mode.

##### Returns

[`ShapeObjectPrsMode`](../enumerations/ShapeObjectPrsMode.md)

#### Set Signature

> **set** **presentationMode**(`theMode`): `void`

Set object's presentation mode.

##### Parameters

###### theMode

[`ShapeObjectPrsMode`](../enumerations/ShapeObjectPrsMode.md)

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

### selectedSubshapes

#### Get Signature

> **get** **selectedSubshapes**(): `number`[]

Return the list with indices of selected subshapes of active selection mode
or empty list of no local selection is active / multiple selection modes are active.

##### Returns

`number`[]

#### Set Signature

> **set** **selectedSubshapes**(`theSubshapes`): `void`

Set the list with indices of selected subhsapes of active selection mode
and resets previous selecction.

##### Parameters

###### theSubshapes

`number`[]

##### Returns

`void`

***

### selectedSubshapesInfo

#### Get Signature

> **get** **selectedSubshapesInfo**(): [`IShapeInfo`](../interfaces/IShapeInfo.md)[]

Return the list with selected subshapes info within active selection mode
or empty list of no local selection is active / multiple selection modes are active.

##### Returns

[`IShapeInfo`](../interfaces/IShapeInfo.md)[]

***

### selectionMode

#### Get Signature

> **get** **selectionMode**(): [`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

Return object's selection mode.

##### Returns

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

#### Set Signature

> **set** **selectionMode**(`theMode`): `void`

Set object's selection mode.

##### Parameters

###### theMode

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

##### Returns

`void`

***

### shapeFinalLocation

#### Get Signature

> **get** **shapeFinalLocation**(): [`ITrsf`](../interfaces/ITrsf.md)

Return multiplication of initial root shape location and local transformation of interactive object.

##### Returns

[`ITrsf`](../interfaces/ITrsf.md)

***

### shapeInfo

#### Get Signature

> **get** **shapeInfo**(): [`IShapeInfo`](../interfaces/IShapeInfo.md)

Return shape info.

##### Returns

[`IShapeInfo`](../interfaces/IShapeInfo.md)

***

### shapeLocation

#### Get Signature

> **get** **shapeLocation**(): [`ITrsf`](../interfaces/ITrsf.md)

Return initial root shape location WITHOUT considering local transformation of interactive object.

##### Returns

[`ITrsf`](../interfaces/ITrsf.md)

#### Set Signature

> **set** **shapeLocation**(`theTrsf`): `void`

Set initial root shape location WITHOUT changing local transformation of interactive object.
This method will recompute presentation of the shape (use localTransformation for interactive manipulations instead!).

##### Parameters

###### theTrsf

[`ITrsf`](../interfaces/ITrsf.md)

##### Returns

`void`

***

### shells

#### Get Signature

> **get** **shells**(): [`SubshapeManager`](SubshapeManager.md)

Return shell subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

***

### solids

#### Get Signature

> **get** **solids**(): [`SubshapeManager`](SubshapeManager.md)

Return solid subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

***

### subshapes

#### Get Signature

> **get** **subshapes**(): [`SubshapeManager`](SubshapeManager.md)[]

Return subshape managers.

##### Returns

[`SubshapeManager`](SubshapeManager.md)[]

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

### vertices

#### Get Signature

> **get** **vertices**(): [`SubshapeManager`](SubshapeManager.md)

Return vertex subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

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

### wires

#### Get Signature

> **get** **wires**(): [`SubshapeManager`](SubshapeManager.md)

Return wire subshape manager.

##### Returns

[`SubshapeManager`](SubshapeManager.md)

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

### addGroup()

> **addGroup**(`theElemType`, `theIndexList`): `number`

Add selection group pointint to the list of subshapes of specified type.
Tip: do NOT activate group selection mode while managing groups list!

#### Parameters

##### theElemType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

##### theIndexList

`number`[]

#### Returns

`number`

***

### buildBox()

> **buildBox**(`theViewer`, `theAx2`, `theDims`): `void`

Create box shape from parameters (based on `BRepPreviewAPI_MakeBox` from C++ OCCT).

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theAx2

[`IAx2`](../interfaces/IAx2.md)

box lower corner (shape origin)

##### theDims

[`IVec3`](../interfaces/IVec3.md)

box dimensions

#### Returns

`void`

***

### buildCone()

> **buildCone**(`theViewer`, `theParams`, `theDevAngle`): `void`

Create cone shape from parameters (based on `BRepPrimAPI_MakeCone` from C++ OCCT).

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theParams

[`IConeParams`](../interfaces/IConeParams.md)

cone parameters

##### theDevAngle

`number` = `...`

mesh deviation angle parameter

#### Returns

`void`

***

### buildCylinder()

> **buildCylinder**(`theViewer`, `theParams`, `theDevAngle`): `void`

Create cylinder shape from parameters (based on `BRepPrimAPI_MakeCylinder` from C++ OCCT).

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theParams

[`ICylinderParams`](../interfaces/ICylinderParams.md)

cylinder parameters

##### theDevAngle

`number` = `...`

mesh deviation angle parameter

#### Returns

`void`

***

### buildLineSegment()

> **buildLineSegment**(`theViewer`, `thePnt1`, `thePnt2`): `void`

Create line segment edge shape.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### thePnt1

[`IVec3`](../interfaces/IVec3.md)

point one

##### thePnt2

[`IVec3`](../interfaces/IVec3.md)

point two

#### Returns

`void`

***

### buildPoint()

> **buildPoint**(`theViewer`, `thePnt`): `void`

Create single vertex shape.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### thePnt

[`IVec3`](../interfaces/IVec3.md)

point definition

#### Returns

`void`

***

### buildSphere()

> **buildSphere**(`theViewer`, `theAx2`, `theRadius`, `theDevAngle`): `void`

Create sphere shape from parameters (based on `BRepPrimAPI_MakeSphere` from C++ OCCT).

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theAx2

[`IAx2`](../interfaces/IAx2.md)

sphere center (shape origin)

##### theRadius

`number`

sphere radius

##### theDevAngle

`number` = `...`

mesh deviation angle parameter

#### Returns

`void`

***

### clearEdgeColors()

> **clearEdgeColors**(): `void`

Unset edges' colors

#### Returns

`void`

***

### clearFaceColors()

> **clearFaceColors**(): `void`

Unset faces' colors

#### Returns

`void`

***

### clearGroups()

> **clearGroups**(): `void`

Remove all selection groups.
Tip: do NOT activate group selection mode while managing groups list!

#### Returns

`void`

***

### clearSelectedElems()

> **clearSelectedElems**(): `void`

Clear selected faces/edges/vertices.

#### Returns

`void`

***

### clearVertexColors()

> **clearVertexColors**(): `void`

Unset vertex' colors

#### Returns

`void`

***

### clone()

> **clone**(`theObjId`): `InteractiveShape`

Make a copy of existing object with new id

#### Parameters

##### theObjId

`string` = `''`

#### Returns

`InteractiveShape`

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

### explodeShape()

> **explodeShape**(`theViewer`, `theExplodeDepth`, `theIsAutoNaming`, `theToGenId`, `theToPropagateId`, `theNewShapeIds`): `InteractiveShape`[]

Explode current shape to sub-shapes.
Current object will be removed in scope of this method.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theExplodeDepth

`number` = `-1`

depth of explode process (should be >= -1, default value is -1 - explode to maximal possible depth)

##### theIsAutoNaming

`boolean` = `true`

generate id by shape type ("compound", "solid", etc) depending on parents in case of attributes absence

##### theToGenId

`boolean` = `true`

generate unique ids or use initial ones in case of existing attributes

##### theToPropagateId

`boolean` = `true`

generate id depending on parents in case of existing attributes

##### theNewShapeIds

`string`[] = `[]`

suggested ids for new objects or empty list to generate them

#### Returns

`InteractiveShape`[]

exploded interactive objects

***

### getConnectedSubshapes()

> **getConnectedSubshapes**(`theElemType`, `theIndexList`): `number`[]

Get subshapes connected to specified subshapes of the same type - for testing purposes.

#### Parameters

##### theElemType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

##### theIndexList

`number`[]

#### Returns

`number`[]

***

### getEdgeColor()

> **getEdgeColor**(`theIndex`): `string`

Return edge color or empty string

#### Parameters

##### theIndex

`number`

#### Returns

`string`

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

### getFaceColor()

> **getFaceColor**(`theIndex`): `string`

Return face color or empty string

#### Parameters

##### theIndex

`number`

#### Returns

`string`

***

### getFilePathFromObjId()

> **getFilePathFromObjId**(): `string`

Return file path from object id.

#### Returns

`string`

#### Inherited from

[`InteractiveObject`](InteractiveObject.md).[`getFilePathFromObjId`](InteractiveObject.md#getfilepathfromobjid)

***

### getSubshapesIndexes()

> **getSubshapesIndexes**(`theElemType`, `theIsFree`, `theParentType`, `theParentIndex`, `theVisibility`): `number`[]

Return the list with indices of available subshapes.

#### Parameters

##### theElemType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

subshape type to explore

##### theIsFree

`boolean` = `false`

if TRUE, only free subshapes will be considered

##### theParentType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md) = `ShapeObjectSelMode.Global`

type of a parent shape or Global to explore all roots

##### theParentIndex

`number` = `-1`

index of a parent shape or -1 to explore all roots

##### theVisibility

`number` = `-1`

return only visible (1), hidden (0) or subshape with any visibility attribute

#### Returns

`number`[]

index list of subshapes

***

### getSubshapeVertex()

> **getSubshapeVertex**(`theVertId`): [`IVec3`](../interfaces/IVec3.md)

Return position of vertex subshape of specified index

#### Parameters

##### theVertId

`number`

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### getVertexColor()

> **getVertexColor**(`theIndex`): `string`

Return vertex color or empty string

#### Parameters

##### theIndex

`number`

#### Returns

`string`

***

### indexArrayFromString()

> **indexArrayFromString**(`theIndices`): `number`[]

Parse index list from the string.

#### Parameters

##### theIndices

`string`

#### Returns

`number`[]

***

### indexArrayToString()

> **indexArrayToString**(`theIndices`): `string`

Format index list into a string.

#### Parameters

##### theIndices

`number`[]

#### Returns

`string`

***

### loadBufferAsync()

> **loadBufferAsync**(`theViewer`, `theArray`, `theFileName`): `Promise`\<`boolean`\>

Load deferred shape data from file previously opened
by [InteractiveShape.openBufferAsync](#openbufferasync) or by opening new specified buffer.
This method doesn't display the object
in the Viewer - [InteractiveObject.isVisible](InteractiveObject.md#isvisible)=true should be called after loading data completion.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theArray

`ArrayBuffer` = `null`

buffer to read model from or null to load already opened model

##### theFileName

`string` = `''`

the file name to map

#### Returns

`Promise`\<`boolean`\>

FALSE if object was not found

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

### loadUrlAsync()

> **loadUrlAsync**(`theViewer`, `theFileUrl`): `Promise`\<`boolean`\>

Load deferred shape data from file previously opened
by [InteractiveShape.openUrlAsync](#openurlasync) or by opening new specified URL.
This method doesn't display the object in the Viewer - [InteractiveObject.isVisible](InteractiveObject.md#isvisible)=true
should be called after loading data completion.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

viewer module

##### theFileUrl

`string` = `''`

the file URL address to read model from or empty string to load already opened model

#### Returns

`Promise`\<`boolean`\>

FALSE if object was not found

***

### openBufferAsync()

> **openBufferAsync**(`theViewer`, `theArray`, `theFilePath`): `Promise`\<`boolean`\>

Open object from the given ArrayBuffer and register presentation(s) in the hidden (erased) state.
If Interactive Object was created with empty id, it will be generated from the given path.

The file will be loaded asynchronously.
To remove file from file system you should
call [SCwebviewer.unlinkFile](SCwebviewer.md#unlinkfile) if it is no longer needed.

Depending on the file format, this method might read
only a model structure without geometry - see [InteractiveShape.loadUrlAsync](#loadurlasync).
Warning! Displaying an object with not yet loaded geometry will produce errors!

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

### openUrlAsync()

> **openUrlAsync**(`theViewer`, `theFileUrl`): `Promise`\<`boolean`\>

Open object from the given URL and register presentation(s) in the hidden (erased) state.
If Interactive Object was created with empty id, it will be generated from the given URL.

The file will be loaded asynchronously.
To remove file from file system you should call [SCwebviewer.unlinkFile](SCwebviewer.md#unlinkfile) if it is no longer needed.

Depending on the file format, this method might read
only a model structure without geometry - see [InteractiveShape.loadUrlAsync](#loadurlasync).
Warning! Displaying an object with not yet loaded geometry will produce errors!

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

### save()

> **save**(`theFile`): `boolean`

Dump shape into a file.

#### Parameters

##### theFile

`string` = `''`

#### Returns

`boolean`

***

### setEdgeColor()

> **setEdgeColor**(`theIndex`, `theColor`): `void`

Set or unset edge color

#### Parameters

##### theIndex

`number`

##### theColor

`string`

#### Returns

`void`

***

### setEdgesColor()

> **setEdgesColor**(`theIndices`, `theColor`): `void`

Set or unset edges color; applied to all edges when null is passed instead of index list

#### Parameters

##### theIndices

`number`[]

##### theColor

`string`

#### Returns

`void`

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

### setFaceColor()

> **setFaceColor**(`theIndex`, `theColor`): `void`

Set or unset face color.

#### Parameters

##### theIndex

`number`

##### theColor

`string`

#### Returns

`void`

***

### setFacesColor()

> **setFacesColor**(`theIndices`, `theColor`): `void`

Set or unset faces color; applied to all faces when null is passed instead of index list

#### Parameters

##### theIndices

`number`[]

##### theColor

`string`

#### Returns

`void`

***

### setMaterialPreset()

> **setMaterialPreset**(`thePreset`): `void`

Set material preset [MaterialPreset](../variables/MaterialPreset.md) to object.

#### Parameters

##### thePreset

`string`

#### Returns

`void`

***

### setSelectedElems()

> **setSelectedElems**(`theFaces`, `theEdges`, `theVertices`): `void`

Set the list with indices of selected faces/edges/vertices.

#### Parameters

##### theFaces

`number`[]

##### theEdges

`number`[]

##### theVertices

`number`[]

#### Returns

`void`

***

### setVertexColor()

> **setVertexColor**(`theIndex`, `theColor`): `void`

Set or unset vertex color

#### Parameters

##### theIndex

`number`

##### theColor

`string`

#### Returns

`void`

***

### setVerticesColor()

> **setVerticesColor**(`theIndices`, `theColor`): `void`

Set or unset vertices color; applied to all vertices when null is passed instead of index list

#### Parameters

##### theIndices

`number`[]

##### theColor

`string`

#### Returns

`void`

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

### getDefaultMaterial()

> `static` **getDefaultMaterial**(`theViewer`): [`IVisMaterial`](../interfaces/IVisMaterial.md)

Get default shape material.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

#### Returns

[`IVisMaterial`](../interfaces/IVisMaterial.md)

***

### getMaterialPreset()

> `static` **getMaterialPreset**(`theViewer`, `thePreset`): [`IVisMaterial`](../interfaces/IVisMaterial.md)

Get material preset [MaterialPreset](../variables/MaterialPreset.md).

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### thePreset

`string`

#### Returns

[`IVisMaterial`](../interfaces/IVisMaterial.md)

***

### probeFileFormat()

> `static` **probeFileFormat**(`theViewer`, `theArray`, `theFilePath`): `string`

Probe file header for known shape formats.

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

***

### setDefaultMaterial()

> `static` **setDefaultMaterial**(`theViewer`, `theMat`): `void`

Set default shape material.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### theMat

[`IVisMaterial`](../interfaces/IVisMaterial.md)

#### Returns

`void`
