[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / PresentationAspects

# Class: PresentationAspects

Auxiliary class for accessing basic InteractiveObject aspects.

## Constructors

### Constructor

> **new PresentationAspects**(`thePrs`): `PresentationAspects`

Constructor.

#### Parameters

##### thePrs

[`InteractiveObject`](InteractiveObject.md)

#### Returns

`PresentationAspects`

## Accessors

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

### lineType

#### Get Signature

> **get** **lineType**(): [`TypeOfLine`](../enumerations/TypeOfLine.md)

Return object's line type.

##### Returns

[`TypeOfLine`](../enumerations/TypeOfLine.md)

#### Set Signature

> **set** **lineType**(`theType`): `void`

Set object's line type.

##### Parameters

###### theType

[`TypeOfLine`](../enumerations/TypeOfLine.md)

##### Returns

`void`

***

### lineWidth

#### Get Signature

> **get** **lineWidth**(): `number`

Return object's line thickness.

##### Returns

`number`

#### Set Signature

> **set** **lineWidth**(`theWidth`): `void`

Set object's line width.

##### Parameters

###### theWidth

`number`

##### Returns

`void`

***

### textFont

#### Get Signature

> **get** **textFont**(): `string`

Return object's font name.

##### Returns

`string`

#### Set Signature

> **set** **textFont**(`theFont`): `void`

Set object's font name.

##### Parameters

###### theFont

`string`

##### Returns

`void`

***

### textFontAspect

#### Get Signature

> **get** **textFontAspect**(): [`FontAspect`](../enumerations/FontAspect.md)

Return object's font aspect.

##### Returns

[`FontAspect`](../enumerations/FontAspect.md)

#### Set Signature

> **set** **textFontAspect**(`theAsp`): `void`

Set object's font aspect.

##### Parameters

###### theAsp

[`FontAspect`](../enumerations/FontAspect.md)

##### Returns

`void`

***

### textHeight

#### Get Signature

> **get** **textHeight**(): `number`

Return object's font height.

##### Returns

`number`

#### Set Signature

> **set** **textHeight**(`theHeight`): `void`

Set object's font height.

##### Parameters

###### theHeight

`number`

##### Returns

`void`

***

### textJustification

#### Get Signature

> **get** **textJustification**(): [`ViewCorner`](../enumerations/ViewCorner.md)

Return object's text alignment.

##### Returns

[`ViewCorner`](../enumerations/ViewCorner.md)

#### Set Signature

> **set** **textJustification**(`theJust`): `void`

Set object's text alignment.

##### Parameters

###### theJust

[`ViewCorner`](../enumerations/ViewCorner.md)

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
