[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ColorTools

# Class: ColorTools

Auxiliary tool for color converting.

## Constructors

### Constructor

> **new ColorTools**(): `ColorTools`

#### Returns

`ColorTools`

## Methods

### convertColorStringToRgb()

> `static` **convertColorStringToRgb**(`theColorStr`): [`IRgb`](../interfaces/IRgb.md)

Converts ColorString to Rgb.

#### Parameters

##### theColorStr

`string`

string in #xxxxxxxx format

#### Returns

[`IRgb`](../interfaces/IRgb.md)

object with rgb values in [0, 255] range

***

### convertColorStringToRgba()

> `static` **convertColorStringToRgba**(`theColorStr`): [`IRgba`](../interfaces/IRgba.md)

Converts ColorString to Rgba.

#### Parameters

##### theColorStr

`string`

string in #xxxxxxxx format

#### Returns

[`IRgba`](../interfaces/IRgba.md)

object with rgba values in [0, 255] range

***

### convertRgbaToColorString()

> `static` **convertRgbaToColorString**(`theColor`): `string`

Converts Rgba to ColorString.

#### Parameters

##### theColor

[`IRgba`](../interfaces/IRgba.md)

object with rgb values in [0, 255] range

#### Returns

`string`

string in #xxxxxxxx format

***

### convertRgbToColorString()

> `static` **convertRgbToColorString**(`theColor`): `string`

Converts Rgb to ColorString.

#### Parameters

##### theColor

[`IRgb`](../interfaces/IRgb.md)

object with rgb values in [0, 255] range

#### Returns

`string`

string in #xxxxxx format
