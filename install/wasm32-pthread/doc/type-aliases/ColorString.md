[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / ColorString

# Type Alias: ColorString

> **ColorString** = `string`

Represents RGB(A) color in HEX format `'#RRGGBB'` and `'#RRGGBBAA'` passed as string value.
Color is transparently converted from/to `Quantity_ColorRGBA`.
In addition, setters accept X11 color names like `'red'`, `'cyan'`, or `'gray50'` (see `Quantity_NameOfColor`).
