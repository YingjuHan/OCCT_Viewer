[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / CaeMeshPresentation

# Class: CaeMeshPresentation

Auxiliary class for accessing InteractiveCaeMesh presentations.

## Accessors

### activeComponent

#### Get Signature

> **get** **activeComponent**(): `number`

Return index of active mesh property component.

##### Returns

`number`

#### Set Signature

> **set** **activeComponent**(`theComponentIndex`): `void`

Set index of active mesh property component.

##### Parameters

###### theComponentIndex

`number`

##### Returns

`void`

***

### activeDeformedProp

#### Get Signature

> **get** **activeDeformedProp**(): `string`

Return id of current active deformed mesh property.

##### Returns

`string`

#### Set Signature

> **set** **activeDeformedProp**(`thePropertyId`): `void`

Set current active deformed mesh property.

##### Parameters

###### thePropertyId

`string`

##### Returns

`void`

***

### activeProperty

#### Get Signature

> **get** **activeProperty**(): `string`

Return active mesh property.

##### Returns

`string`

#### Set Signature

> **set** **activeProperty**(`thePropertyId`): `void`

Set active mesh property.

##### Parameters

###### thePropertyId

`string`

##### Returns

`void`

***

### activeVecProp

#### Get Signature

> **get** **activeVecProp**(): `string`

Return id of current active vector mesh property.

##### Returns

`string`

#### Set Signature

> **set** **activeVecProp**(`thePropertyId`): `void`

Set current active vector mesh property.

##### Parameters

###### thePropertyId

`string`

##### Returns

`void`

***

### clipPlane

#### Get Signature

> **get** **clipPlane**(): [`IVec4`](../interfaces/IVec4.md)

Return equation of clip plane used to filter mesh elements (null if not set).

##### Returns

[`IVec4`](../interfaces/IVec4.md)

#### Set Signature

> **set** **clipPlane**(`theEq`): `void`

Set/unset clip plane used to filter mesh elements.

##### Parameters

###### theEq

[`IVec4`](../interfaces/IVec4.md)

##### Returns

`void`

***

### deformedScaleCoeff

#### Get Signature

> **get** **deformedScaleCoeff**(): `number`

Return scale coefficient for mesh deformed presentation.

##### Returns

`number`

#### Set Signature

> **set** **deformedScaleCoeff**(`theValue`): `void`

Set scale coefficient for mesh deformed presentation.

##### Parameters

###### theValue

`number`

##### Returns

`void`

***

### edgesColor

#### Get Signature

> **get** **edgesColor**(): `string`

Return edges color, "#cccccc" by default.

##### Returns

`string`

#### Set Signature

> **set** **edgesColor**(`theColor`): `void`

Set edges color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### edgesHighlightColor

#### Get Signature

> **get** **edgesHighlightColor**(): `string`

Return highlighting edges color, "#e5e5e5" by default.

##### Returns

`string`

#### Set Signature

> **set** **edgesHighlightColor**(`theColor`): `void`

Set highlighting edges color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### edgesHighlightWidth

#### Get Signature

> **get** **edgesHighlightWidth**(): `number`

Return highlighting edges width, 1 by default.

##### Returns

`number`

#### Set Signature

> **set** **edgesHighlightWidth**(`theWidth`): `void`

Set highlighting edges width.

##### Parameters

###### theWidth

`number`

##### Returns

`void`

***

### edgesSelectionColor

#### Get Signature

> **get** **edgesSelectionColor**(): `string`

Return selection edges color, "#cccccc" by default.

##### Returns

`string`

#### Set Signature

> **set** **edgesSelectionColor**(`theColor`): `void`

Set selection edges color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### edgesSelectionWidth

#### Get Signature

> **get** **edgesSelectionWidth**(): `number`

Return selection edges width, 1 by default.

##### Returns

`number`

#### Set Signature

> **set** **edgesSelectionWidth**(`theWidth`): `void`

Set selection edges width.

##### Parameters

###### theWidth

`number`

##### Returns

`void`

***

### edgesWidth

#### Get Signature

> **get** **edgesWidth**(): `number`

Return edges width, 1 by default.

##### Returns

`number`

#### Set Signature

> **set** **edgesWidth**(`theWidth`): `void`

Set edges width.

##### Parameters

###### theWidth

`number`

##### Returns

`void`

***

### id

#### Get Signature

> **get** **id**(): `string`

Return presentation id.

##### Returns

`string`

***

### isoProperty

#### Get Signature

> **get** **isoProperty**(): `ICaeMeshIdIndex`

Return array of iso values.

##### Returns

`ICaeMeshIdIndex`

#### Set Signature

> **set** **isoProperty**(`theProp`): `void`

Set/unset array of iso values.

##### Parameters

###### theProp

`ICaeMeshIdIndex`

##### Returns

`void`

***

### isoValues

#### Get Signature

> **get** **isoValues**(): `number`[]

Return array of iso values.

##### Returns

`number`[]

#### Set Signature

> **set** **isoValues**(`theValues`): `void`

Set/unset array of iso values.

##### Parameters

###### theValues

`number`[]

##### Returns

`void`

***

### isVisible

#### Get Signature

> **get** **isVisible**(): `boolean`

Return presentation visibility.

##### Returns

`boolean`

#### Set Signature

> **set** **isVisible**(`theIsVisible`): `void`

Set presentation visibility.

##### Parameters

###### theIsVisible

`boolean`

##### Returns

`void`

***

### isVolumeAsSurface

#### Get Signature

> **get** **isVolumeAsSurface**(): `boolean`

Return true if it is needed to render/select 3d mesh only by its surface
and to not include internal parts, True by default.

##### Returns

`boolean`

#### Set Signature

> **set** **isVolumeAsSurface**(`theIsVolumeAsSurface`): `void`

Set if it is needed to render/select 3d mesh only by its surface
and to not include internal parts.

##### Parameters

###### theIsVolumeAsSurface

`boolean`

##### Returns

`void`

***

### pointsColor

#### Get Signature

> **get** **pointsColor**(): `string`

Return points color, "#7f7f7f" by default.

##### Returns

`string`

#### Set Signature

> **set** **pointsColor**(`theColor`): `void`

Set points color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### pointsHighlightColor

#### Get Signature

> **get** **pointsHighlightColor**(): `string`

Return highlighting points color, "#e5e5e5" by default.

##### Returns

`string`

#### Set Signature

> **set** **pointsHighlightColor**(`theColor`): `void`

Set highlighting points color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### pointsHighlightSize

#### Get Signature

> **get** **pointsHighlightSize**(): `number`

Return highlighting points size, 5 by default.

##### Returns

`number`

#### Set Signature

> **set** **pointsHighlightSize**(`theSize`): `void`

Set highlighting points size.

##### Parameters

###### theSize

`number`

##### Returns

`void`

***

### pointsSelectionColor

#### Get Signature

> **get** **pointsSelectionColor**(): `string`

Return selection points color, "#cccccc" by default.

##### Returns

`string`

#### Set Signature

> **set** **pointsSelectionColor**(`theColor`): `void`

Set selection points color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### pointsSelectionSize

#### Get Signature

> **get** **pointsSelectionSize**(): `number`

Return selection points size, 5 by default.

##### Returns

`number`

#### Set Signature

> **set** **pointsSelectionSize**(`theSize`): `void`

Set selection points size.

##### Parameters

###### theSize

`number`

##### Returns

`void`

***

### pointsSize

#### Get Signature

> **get** **pointsSize**(): `number`

Return points size, 5 by default.

##### Returns

`number`

#### Set Signature

> **set** **pointsSize**(`theSize`): `void`

Set points size.

##### Parameters

###### theSize

`number`

##### Returns

`void`

***

### presentationMode

#### Get Signature

> **get** **presentationMode**(): [`CaeMeshObjectPrsMode`](../enumerations/CaeMeshObjectPrsMode.md)

Return mesh presentation mode.

##### Returns

[`CaeMeshObjectPrsMode`](../enumerations/CaeMeshObjectPrsMode.md)

#### Set Signature

> **set** **presentationMode**(`theMode`): `void`

Set mesh presentation mode.

##### Parameters

###### theMode

[`CaeMeshObjectPrsMode`](../enumerations/CaeMeshObjectPrsMode.md)

##### Returns

`void`

***

### selectionMode

#### Get Signature

> **get** **selectionMode**(): `number`

Return mesh selection mode.

##### Returns

`number`

#### Set Signature

> **set** **selectionMode**(`theMode`): `void`

Set mesh selection mode.

##### Parameters

###### theMode

`number`

##### Returns

`void`

***

### shadingColor

#### Get Signature

> **get** **shadingColor**(): `string`

Return shading color, "#ffffff" by default.

##### Returns

`string`

#### Set Signature

> **set** **shadingColor**(`theColor`): `void`

Set shading color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### shadingHighlightColor

#### Get Signature

> **get** **shadingHighlightColor**(): `string`

Return highlighting shading color, "#e5e5e5" by default.

##### Returns

`string`

#### Set Signature

> **set** **shadingHighlightColor**(`theColor`): `void`

Set highlighting shading color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### shadingHighlightTransparency

#### Get Signature

> **get** **shadingHighlightTransparency**(): `number`

Return highlighting shading transparency, 0.8 by default.

##### Returns

`number`

#### Set Signature

> **set** **shadingHighlightTransparency**(`theValue`): `void`

Set highlighting shading transparency.

##### Parameters

###### theValue

`number`

##### Returns

`void`

***

### shadingSelectionColor

#### Get Signature

> **get** **shadingSelectionColor**(): `string`

Return selection shading color, "#cccccc" by default.

##### Returns

`string`

#### Set Signature

> **set** **shadingSelectionColor**(`theColor`): `void`

Set selection shading color.

##### Parameters

###### theColor

`string`

##### Returns

`void`

***

### shadingSelectionTransparency

#### Get Signature

> **get** **shadingSelectionTransparency**(): `number`

Return shading transparency, 0.8 by default.

##### Returns

`number`

#### Set Signature

> **set** **shadingSelectionTransparency**(`theValue`): `void`

Set selection shading transparency.

##### Parameters

###### theValue

`number`

##### Returns

`void`

***

### shadingTransparency

#### Get Signature

> **get** **shadingTransparency**(): `number`

Return shading transparency, 0.0 by default.

##### Returns

`number`

#### Set Signature

> **set** **shadingTransparency**(`theValue`): `void`

Set shading transparency.

##### Parameters

###### theValue

`number`

##### Returns

`void`

***

### slicePlanes

#### Get Signature

> **get** **slicePlanes**(): [`IVec4`](../interfaces/IVec4.md)[]

Return array of planes used to cut mesh.

##### Returns

[`IVec4`](../interfaces/IVec4.md)[]

#### Set Signature

> **set** **slicePlanes**(`theEqs`): `void`

Set/unset array of planes used to cut mesh.

##### Parameters

###### theEqs

[`IVec4`](../interfaces/IVec4.md)[]

##### Returns

`void`

***

### streamlines

#### Get Signature

> **get** **streamlines**(): [`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md)

Return streamlines presentation parameters.

##### Returns

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md)

#### Set Signature

> **set** **streamlines**(`theInfo`): `void`

Set streamlines presentation parameters.

##### Parameters

###### theInfo

[`ICaeMeshStreamlinesInfo`](../interfaces/ICaeMeshStreamlinesInfo.md)

##### Returns

`void`

***

### toCutOnClipPlane

#### Get Signature

> **get** **toCutOnClipPlane**(): `boolean`

Check whether mesh clipping cuts or filters elements.

##### Returns

`boolean`

#### Set Signature

> **set** **toCutOnClipPlane**(`theToCut`): `void`

Define whether mesh clipping cuts or filters elements.

##### Parameters

###### theToCut

`boolean`

##### Returns

`void`

***

### toDisplay0dElements

#### Get Signature

> **get** **toDisplay0dElements**(): `boolean`

To display 0d elements.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplay0dElements**(`toDisplay`): `void`

Set to display 0d elements.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplay2dElements

#### Get Signature

> **get** **toDisplay2dElements**(): `boolean`

To display 2d elements.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplay2dElements**(`toDisplay`): `void`

Set to display 2d elements.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplay3dElements

#### Get Signature

> **get** **toDisplay3dElements**(): `boolean`

To display 3d elements.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplay3dElements**(`toDisplay`): `void`

Set to display 3d elements.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplayCentroids

#### Get Signature

> **get** **toDisplayCentroids**(): `boolean`

To display element centroids.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplayCentroids**(`toDisplay`): `void`

Set to display element centroids.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplayFreeEdges

#### Get Signature

> **get** **toDisplayFreeEdges**(): `boolean`

To display free edges.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplayFreeEdges**(`toDisplay`): `void`

Set to display free edges.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplayNodes

#### Get Signature

> **get** **toDisplayNodes**(): `boolean`

To display nodes.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplayNodes**(`toDisplay`): `void`

Set to display nodes.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toDisplaySharpEdges

#### Get Signature

> **get** **toDisplaySharpEdges**(): `boolean`

To display sharp edges.

##### Returns

`boolean`

#### Set Signature

> **set** **toDisplaySharpEdges**(`toDisplay`): `void`

Set to display sharp edges.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toProcessFreeNodes

#### Get Signature

> **get** **toProcessFreeNodes**(): `boolean`

Return true if free nodes should be used within element nodes in calculation/visualization.

##### Returns

`boolean`

#### Set Signature

> **set** **toProcessFreeNodes**(`toDisplay`): `void`

Define if free nodes should be used within element nodes in calculation/visualization.

##### Parameters

###### toDisplay

`boolean`

##### Returns

`void`

***

### toShowGroupsInfo

#### Get Signature

> **get** **toShowGroupsInfo**(): `boolean`

Return visibility of annotations for the current active group mesh property.

##### Returns

`boolean`

#### Set Signature

> **set** **toShowGroupsInfo**(`theToShow`): `void`

Set visibility of annotations for the current active group mesh property.

##### Parameters

###### theToShow

`boolean`

##### Returns

`void`

***

### toShowMinMax

#### Get Signature

> **get** **toShowMinMax**(): `boolean`

Return visibility of annotations for min/max values of the current active mesh property.

##### Returns

`boolean`

#### Set Signature

> **set** **toShowMinMax**(`theToShow`): `void`

Set visibility of annotations for min/max values of the current active mesh property.

##### Parameters

###### theToShow

`boolean`

##### Returns

`void`

***

### type

#### Get Signature

> **get** **type**(): [`CaeMeshPresentationType`](../enumerations/CaeMeshPresentationType.md)

Return presentation type.

##### Returns

[`CaeMeshPresentationType`](../enumerations/CaeMeshPresentationType.md)

***

### vecPropDistrType

#### Get Signature

> **get** **vecPropDistrType**(): [`CaeMeshVectorDistrType`](../enumerations/CaeMeshVectorDistrType.md)

Return distribution type for mesh vector presentation.

##### Returns

[`CaeMeshVectorDistrType`](../enumerations/CaeMeshVectorDistrType.md)

#### Set Signature

> **set** **vecPropDistrType**(`theType`): `void`

Set distribution type for mesh vector presentation.

##### Parameters

###### theType

[`CaeMeshVectorDistrType`](../enumerations/CaeMeshVectorDistrType.md)

##### Returns

`void`

***

### vecPropScaleCoeff

#### Get Signature

> **get** **vecPropScaleCoeff**(): `number`

Return scale coefficient for mesh vector presentation.

##### Returns

`number`

#### Set Signature

> **set** **vecPropScaleCoeff**(`theValue`): `void`

Set scale coefficient for mesh vector presentation.

##### Parameters

###### theValue

`number`

##### Returns

`void`

***

### vecSamplingRatio

#### Get Signature

> **get** **vecSamplingRatio**(): `number`

Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0, 1.0 by default.

##### Returns

`number`

#### Set Signature

> **set** **vecSamplingRatio**(`theRatio`): `void`

Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0.

##### Parameters

###### theRatio

`number`

##### Returns

`void`

## Methods

### getGroupInfo()

> **getGroupInfo**(`theGroupId`): [`CaeMeshAnnotation`](../interfaces/CaeMeshAnnotation.md)

Return annotation for specified mesh group.

#### Parameters

##### theGroupId

`number`

#### Returns

[`CaeMeshAnnotation`](../interfaces/CaeMeshAnnotation.md)

***

### getVisibleBlocks()

> **getVisibleBlocks**(): [`CaeMeshBlock`](CaeMeshBlock.md)

Return visible mesh blocks.

#### Returns

[`CaeMeshBlock`](CaeMeshBlock.md)

***

### setGenGeometry()

> **setGenGeometry**(`theViewer`, `thePartId`, `theArray`): `Promise`\<`boolean`\>

Set custom geometry for the specified part.

#### Parameters

##### theViewer

[`SCwebviewer`](SCwebviewer.md)

##### thePartId

`number`

##### theArray

`ArrayBuffer`

#### Returns

`Promise`\<`boolean`\>

***

### setGroupInfo()

> **setGroupInfo**(`theGroupId`, `theInfo`): `void`

Set annotation for specified mesh group.

#### Parameters

##### theGroupId

`number`

##### theInfo

[`CaeMeshAnnotation`](../interfaces/CaeMeshAnnotation.md)

#### Returns

`void`

***

### setVisibleBlocks()

> **setVisibleBlocks**(`theBlocks`, `theVisible`): `void`

Set mesh blocks visibility.

#### Parameters

##### theBlocks

[`CaeMeshBlock`](CaeMeshBlock.md)

##### theVisible

`boolean`

#### Returns

`void`
