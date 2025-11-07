[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / SubshapeManager

# Class: SubshapeManager

Auxiliary class for accessing InteractiveShape subshapes.

## Constructors

### Constructor

> **new SubshapeManager**(`thePrs`, `theType`): `SubshapeManager`

Constructor.

#### Parameters

##### thePrs

[`InteractiveShape`](InteractiveShape.md)

##### theType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

#### Returns

`SubshapeManager`

## Accessors

### colored

#### Get Signature

> **get** **colored**(): `number`[]

Return index list of colored subshapes.

##### Returns

`number`[]

***

### defined

#### Get Signature

> **get** **defined**(): `number`[]

Return the list with indices of available subshapes.

##### Returns

`number`[]

***

### disabled

#### Get Signature

> **get** **disabled**(): `number`[]

Return the list with indices of disabled subshapes.

##### Returns

`number`[]

#### Set Signature

> **set** **disabled**(`theList`): `void`

Set the list with indices of disabled subshapes.

##### Parameters

###### theList

`number`[]

##### Returns

`void`

***

### enabled

#### Get Signature

> **get** **enabled**(): `number`[]

Return the list with indices of enabled subshapes.

##### Returns

`number`[]

#### Set Signature

> **set** **enabled**(`theList`): `void`

Set the list with indices of enabled subshapes.

##### Parameters

###### theList

`number`[]

##### Returns

`void`

***

### free

#### Get Signature

> **get** **free**(): `number`[]

Return the list with indices of free subshapes.

##### Returns

`number`[]

***

### hidden

#### Get Signature

> **get** **hidden**(): `number`[]

Return index list of hidden subshapes.

##### Returns

`number`[]

#### Set Signature

> **set** **hidden**(`theList`): `void`

Set index list of hidden subshapes (makes visible unspecified subshapes).

##### Parameters

###### theList

`number`[]

##### Returns

`void`

***

### lastDetected

#### Get Signature

> **get** **lastDetected**(): `number`

Return the last detected subshape index, or `-1` if unspecified.

##### Returns

`number`

***

### length

#### Get Signature

> **get** **length**(): `number`

Return the number of available subshapes.

##### Returns

`number`

***

### selected

#### Get Signature

> **get** **selected**(): `number`[]

Return the list with indices of selected subshapes

##### Returns

`number`[]

#### Set Signature

> **set** **selected**(`theList`): `void`

Set the list with indices of selected subshapes of this type but keeps selection of subshapes of other types.

##### Parameters

###### theList

`number`[]

##### Returns

`void`

***

### selectedOnly

#### Set Signature

> **set** **selectedOnly**(`theList`): `void`

Set the list with indices of selected subshapes and resets selection of all other subshape types.

##### Parameters

###### theList

`number`[]

##### Returns

`void`

***

### shapeInfo

#### Get Signature

> **get** **shapeInfo**(): [`IShapeInfo`](../interfaces/IShapeInfo.md)[]

Return subshape info.

##### Returns

[`IShapeInfo`](../interfaces/IShapeInfo.md)[]

***

### shapeType

#### Get Signature

> **get** **shapeType**(): [`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

Return shape type.

##### Returns

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

***

### visible

#### Get Signature

> **get** **visible**(): `number`[]

Return index list of visible subshapes.

##### Returns

`number`[]

#### Set Signature

> **set** **visible**(`theList`): `void`

Set index list of visible subshapes (makes hidden unspecified subshapes).

##### Parameters

###### theList

`number`[]

##### Returns

`void`

## Methods

### \[iterator\]()

> **\[iterator\]**(): `IterableIterator`\<[`IShapeInfo`](../interfaces/IShapeInfo.md)\>

Iteratable interface.

#### Returns

`IterableIterator`\<[`IShapeInfo`](../interfaces/IShapeInfo.md)\>

***

### clearSubshapeColors()

> **clearSubshapeColors**(): `void`

Unset subshapes colors

#### Returns

`void`

***

### clearSubshapeHidden()

> **clearSubshapeHidden**(): `void`

Unset subshapes hidden flag.

#### Returns

`void`

***

### cloneSubshape()

> **cloneSubshape**(`theIndex`, `theNewObjId`): [`InteractiveShape`](InteractiveShape.md)

Make a copy of existing object with new id and specified subshape.

#### Parameters

##### theIndex

`number`

##### theNewObjId

`string` = `''`

#### Returns

[`InteractiveShape`](InteractiveShape.md)

***

### cloneSubshapes()

> **cloneSubshapes**(`theIndices`, `theNewObjId`): [`InteractiveShape`](InteractiveShape.md)

Make a copy of existing object with new id and specified subshapes.

#### Parameters

##### theIndices

`number`[]

##### theNewObjId

`string` = `''`

#### Returns

[`InteractiveShape`](InteractiveShape.md)

***

### getConnectedSubshapes()

> **getConnectedSubshapes**(`theIndexList`): `number`[]

Get subshapes connected to specified subshapes of the same type - for testing purposes.

#### Parameters

##### theIndexList

`number`[]

#### Returns

`number`[]

***

### getShapeInfo()

> **getShapeInfo**(`theIndex`, `theToExplore`): [`IShapeInfo`](../interfaces/IShapeInfo.md)

Return the subshape info.

#### Parameters

##### theIndex

`number`

##### theToExplore

`boolean` = `true`

#### Returns

[`IShapeInfo`](../interfaces/IShapeInfo.md)

***

### getSubshapeColor()

> **getSubshapeColor**(`theIndex`): `string`

Return subshape color or empty string

#### Parameters

##### theIndex

`number`

#### Returns

`string`

***

### getSubshapesIndexes()

> **getSubshapesIndexes**(`theIndex`, `theSubType`): `number`[]

Return indexes of subshapes of specified subshape.

#### Parameters

##### theIndex

`number`

##### theSubType

[`ShapeObjectSelMode`](../enumerations/ShapeObjectSelMode.md)

#### Returns

`number`[]

***

### isSubshapeHidden()

> **isSubshapeHidden**(`theIndex`): `boolean`

Return subshape hidden flag.

#### Parameters

##### theIndex

`number`

#### Returns

`boolean`

***

### saveSubshape()

> **saveSubshape**(`theIndex`, `theFile`): `boolean`

Dump shape into a file.

#### Parameters

##### theIndex

`number`

##### theFile

`string` = `''`

#### Returns

`boolean`

***

### saveSubshapes()

> **saveSubshapes**(`theIndices`, `theFile`): `boolean`

Dump shapes into a file.

#### Parameters

##### theIndices

`number`[]

##### theFile

`string` = `''`

#### Returns

`boolean`

***

### setSubshapeColor()

> **setSubshapeColor**(`theIndex`, `theColor`): `void`

Set or unset subhsape color.

#### Parameters

##### theIndex

`number`

##### theColor

`string`

#### Returns

`void`

***

### setSubshapeHidden()

> **setSubshapeHidden**(`theIndex`, `theIsHidden`): `void`

Set or unset subhsape hidden flag.

#### Parameters

##### theIndex

`number`

##### theIsHidden

`boolean`

#### Returns

`void`

***

### setSubshapesColor()

> **setSubshapesColor**(`theIndices`, `theColor`): `void`

Set or unset subhsapes color; applied to all subhsapes when null is passed instead of index list

#### Parameters

##### theIndices

`number`[]

##### theColor

`string`

#### Returns

`void`

***

### setSubshapesHidden()

> **setSubshapesHidden**(`theIndices`, `theIsHidden`, `theToInvertOthers`): `void`

Set or unset subhsapes hidden flag; applied to all subshapes when null is passed instead of index list

#### Parameters

##### theIndices

`number`[]

##### theIsHidden

`boolean`

##### theToInvertOthers

`boolean` = `false`

#### Returns

`void`
