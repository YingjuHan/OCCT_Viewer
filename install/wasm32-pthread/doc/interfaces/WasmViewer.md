[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / WasmViewer

# Interface: WasmViewer

Represents Viewer global object.
Note that there is no reason to use class directly at application level
as [SCwebviewer](../classes/SCwebviewer.md) re-exports main Viewer properties.

## Properties

### dumpMemInfo()

> **dumpMemInfo**: () => `void`

Dump memory usage stats

#### Returns

`void`

***

### makeScreenshot()

> **makeScreenshot**: (`theWidth`, `theHeight`, `theBuffer`) => `boolean`

Make screenshot.

#### Parameters

##### theWidth

`number`

width of image

##### theHeight

`number`

height of image

##### theBuffer

`number`

preallocated buffer

#### Returns

`boolean`

return FALSE on error

***

### requestViewUpdate()

> **requestViewUpdate**: () => `void`

Request View update (using requestAnimationFrame() or similar method).

#### Returns

`void`

***

### resizeViewport()

> **resizeViewport**: () => `void`

Resize viewport / window.

#### Returns

`void`
