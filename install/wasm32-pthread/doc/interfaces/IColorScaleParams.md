[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / IColorScaleParams

# Interface: IColorScaleParams

Parameters interface for color scale object.

## Properties

### customColors

> **customColors**: `any`

Vector of colors (StringVector) used if isCustomColors = true.

***

### customLabels

> **customLabels**: `any`

Custom labels (StringVector) used if isCustomColors = true.

***

### defaultRange

> **defaultRange**: [`IVec2`](IVec2.md)

Default value range (actual range of property values), always in ascending order.

***

### displayRange

> **displayRange**: [`IVec2`](IVec2.md)

Display value range, always in ascending order.

***

### displayUnits

> **displayUnits**: `string`

Display units.

***

### intervalHighlightColor

> **intervalHighlightColor**: `string`

Color used for detected interval if isCustomColors = true.

***

### isCustomColors

> **isCustomColors**: `boolean`

Defines if used a continuous colors scale (from minColor to maxColor)
 or array of colors for each interval.

***

### isLogarithmic

> **isLogarithmic**: `boolean`

Controls whether the mode of colorscale is logarithmic.

***

### isReversed

> **isReversed**: `boolean`

Defines if display min/max values should be swapped in display.

***

### isSmooth

> **isSmooth**: `boolean`

Defines if colors are discrete or smooth.

***

### labelSize

> **labelSize**: `number`

Size of label text.

***

### maxColor

> **maxColor**: `string`

Max color used if isCustomColors = false.

***

### maxSize

> **maxSize**: `number`

Max height of color scale (as ratio from viewer height if <= 1.0 and in pixels otherwise).

***

### minColor

> **minColor**: `string`

Min color used if isCustomColors = false.

***

### nbIntervals

> **nbIntervals**: `number`

Number of intervals (colors).

***

### outOfRangeColor

> **outOfRangeColor**: `string`

Defines color for out-of-range values (including alpha).

***

### pos

> **pos**: [`IVec2`](IVec2.md)

Left-bottom position of color scale.

***

### positionSide

> **positionSide**: [`ViewCorner`](../enumerations/ViewCorner.md)

Controls the side of the screen to which the colorscale is attached.

***

### propertyComponent

> **propertyComponent**: `number`

Related property component.

***

### propertyId

> **propertyId**: `string`

Related property id.

***

### selectedRange

> **selectedRange**: [`IVec2`](IVec2.md)

Selected normalized range (within 0..1 range), always in ascending order.

***

### size

> **size**: [`IVec2`](IVec2.md)

Width, height of color scale.

***

### title

> **title**: `string`

Title text.

***

### toHighlightIntervals

> **toHighlightIntervals**: `boolean`

Flag to use special color for detected interval if isCustomColors = true.

***

### toSkipEmptyLabels

> **toSkipEmptyLabels**: `boolean`

Flag to skip empty custom labels in legend within non-smoothed presentation.
