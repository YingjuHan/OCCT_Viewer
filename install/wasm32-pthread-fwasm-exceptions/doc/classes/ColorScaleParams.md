[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ColorScaleParams

# Class: ColorScaleParams

Parameters object for color scale object.

## Implements

- [`IColorScaleParams`](../interfaces/IColorScaleParams.md)

## Constructors

### Constructor

> **new ColorScaleParams**(): `ColorScaleParams`

#### Returns

`ColorScaleParams`

## Properties

### customColors

> **customColors**: `any` = `null`

Vector of colors (StringVector) used if isCustomColors = true.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`customColors`](../interfaces/IColorScaleParams.md#customcolors)

***

### customLabels

> **customLabels**: `any` = `null`

Custom labels (StringVector) used if isCustomColors = true.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`customLabels`](../interfaces/IColorScaleParams.md#customlabels)

***

### defaultRange

> **defaultRange**: [`IVec2`](../interfaces/IVec2.md)

Default value range (actual range of property values).

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`defaultRange`](../interfaces/IColorScaleParams.md#defaultrange)

***

### displayRange

> **displayRange**: [`IVec2`](../interfaces/IVec2.md)

Display value range.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`displayRange`](../interfaces/IColorScaleParams.md#displayrange)

***

### displayUnits

> **displayUnits**: `string` = `''`

Display units.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`displayUnits`](../interfaces/IColorScaleParams.md#displayunits)

***

### intervalHighlightColor

> **intervalHighlightColor**: `string` = `''`

Color used for detected interval if isCustomColors = true.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`intervalHighlightColor`](../interfaces/IColorScaleParams.md#intervalhighlightcolor)

***

### isCustomColors

> **isCustomColors**: `boolean` = `false`

Defines if used a continuous colors scale.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`isCustomColors`](../interfaces/IColorScaleParams.md#iscustomcolors)

***

### isLogarithmic

> **isLogarithmic**: `boolean` = `false`

Controls whether the mode of colorscale is logarithmic.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`isLogarithmic`](../interfaces/IColorScaleParams.md#islogarithmic)

***

### isReversed

> **isReversed**: `boolean` = `false`

Defines if display min/max values should be swapped in display.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`isReversed`](../interfaces/IColorScaleParams.md#isreversed)

***

### isSmooth

> **isSmooth**: `boolean` = `false`

Defines if colors are discrete or smooth.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`isSmooth`](../interfaces/IColorScaleParams.md#issmooth)

***

### labelSize

> **labelSize**: `number` = `12`

Size of label text.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`labelSize`](../interfaces/IColorScaleParams.md#labelsize)

***

### maxColor

> **maxColor**: `string` = `''`

Max color used if isCustomColors = false.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`maxColor`](../interfaces/IColorScaleParams.md#maxcolor)

***

### maxSize

> **maxSize**: `number` = `1.0`

Max height of color scale (as ratio from viewer height if <= 1.0 and in pixels otherwise).

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`maxSize`](../interfaces/IColorScaleParams.md#maxsize)

***

### minColor

> **minColor**: `string` = `''`

Min color used if isCustomColors = false.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`minColor`](../interfaces/IColorScaleParams.md#mincolor)

***

### nbIntervals

> **nbIntervals**: `number` = `10`

Number of intervals (colors).

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`nbIntervals`](../interfaces/IColorScaleParams.md#nbintervals)

***

### outOfRangeColor

> **outOfRangeColor**: `string` = `''`

Defines color for out-of-range values (including alpha).

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`outOfRangeColor`](../interfaces/IColorScaleParams.md#outofrangecolor)

***

### pos

> **pos**: [`IVec2`](../interfaces/IVec2.md)

Left-bottom position of color scale.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`pos`](../interfaces/IColorScaleParams.md#pos)

***

### positionSide

> **positionSide**: [`ViewCorner`](../enumerations/ViewCorner.md) = `ViewCorner.RIGHT_CENTER`

Controls the side of the screen to which the colorscale is attached.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`positionSide`](../interfaces/IColorScaleParams.md#positionside)

***

### propertyComponent

> **propertyComponent**: `number` = `-1`

Related property component.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`propertyComponent`](../interfaces/IColorScaleParams.md#propertycomponent)

***

### propertyId

> **propertyId**: `string` = `''`

Related property id.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`propertyId`](../interfaces/IColorScaleParams.md#propertyid)

***

### selectedRange

> **selectedRange**: [`IVec2`](../interfaces/IVec2.md)

Selected normalized range (within 0..1 range), always in ascending order.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`selectedRange`](../interfaces/IColorScaleParams.md#selectedrange)

***

### size

> **size**: [`IVec2`](../interfaces/IVec2.md)

Width, height of color scale.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`size`](../interfaces/IColorScaleParams.md#size)

***

### title

> **title**: `string` = `''`

Title text.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`title`](../interfaces/IColorScaleParams.md#title)

***

### toHighlightIntervals

> **toHighlightIntervals**: `boolean` = `true`

Flag to use special color for detected interval if isCustomColors = true.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`toHighlightIntervals`](../interfaces/IColorScaleParams.md#tohighlightintervals)

***

### toSkipEmptyLabels

> **toSkipEmptyLabels**: `boolean` = `false`

Flag to skip empty custom labels in legend within non-smoothed presentation.

#### Implementation of

[`IColorScaleParams`](../interfaces/IColorScaleParams.md).[`toSkipEmptyLabels`](../interfaces/IColorScaleParams.md#toskipemptylabels)
