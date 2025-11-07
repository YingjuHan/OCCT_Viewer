[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / SCwebviewer

# Class: SCwebviewer

Base class for SCwebviewer Module object.
For convenience, this class re-exports interface to the main Viewer.

Hello World usage sample:
```typescript
  // Somewhere in HTML - add 'canvas' element
  // with id 'ViewerCanvas' holding 3D viewer content.
  // <canvas id="ViewerCanvas" oncontextmenu="event.preventDefault()" tabindex=-1
  //         style="border:0 none;background-color:#000; user-select: none;"
  //         width="409" height="409">
  // </canvas>

  // library imports
  import * as SCwebviewer from '/wasm32/SCwebviewer.js';

  // create viewer object and specify prefix for loading extra files
  var scViewer = new SCwebviewer.SCwebviewer('/');
  // setup log verbose level BEFORE initialization
  scViewer.traceLevel = SCwebviewer.MessageGravity.Trace;
  // load WASM, initialize WebGL context and 3D Viewer
  scViewer.init().then (() => {
    // adjust rendering parameters
    scViewer.renderingParams.backgroundColor = "black";
    scViewer.viewCube.isVisible = true;
    scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.FRONT);
    // load some models
    let aShape = new SCwebviewer.InteractiveShape();
      aShape.loadUrlAsync(scViewer, "/models/shape.stl").then(() => {
      aShape.isVisible = true; // display AFTER loading completed
      scViewer.camera.fitAllAuto(true);
      scViewer.requestViewUpdate(); // should be added to redraw Viewer contents
    })
  })
```

## Constructors

### Constructor

> **new SCwebviewer**(`theBaseDataPath`): `SCwebviewer`

Main constructor.

#### Parameters

##### theBaseDataPath

`string`

path to folder that contains
       'wasm32' + 'wasm32-pthread' folders (WASM files),
       'fonts' folder with default font files.

#### Returns

`SCwebviewer`

## Properties

### CameraOrientation

> **CameraOrientation**: *typeof* [`CameraOrientation`](../enumerations/CameraOrientation.md)

Implementation of [CameraOrientation](../enumerations/CameraOrientation.md).

***

### canvas

> **canvas**: `HTMLCanvasElement`

'canvas' element for WebGL context (read by Emscripten Module).

***

### IPromise

> **IPromise**: [`IPromise`](../interfaces/IPromise.md)

Implementation of [IPromise](../interfaces/IPromise.md) type.

***

### noExitRuntime

> **noExitRuntime**: `boolean`

Flag defining `main()` return behavior (read by Emscripten Module).

## Accessors

### camera

#### Get Signature

> **get** **camera**(): [`Camera`](Camera.md)

Get Camera manager.

##### Returns

[`Camera`](Camera.md)

Camera manager object

***

### eventManager

#### Get Signature

> **get** **eventManager**(): [`ViewerEventManager`](ViewerEventManager.md)

Get event manager - an interface for subscribing and unsubscribing to various callbacks (like selection changes).

##### Returns

[`ViewerEventManager`](ViewerEventManager.md)

event manager

***

### focusView

#### Get Signature

> **get** **focusView**(): [`Subview`](Subview.md)

Get view object receiving focus.

##### Returns

[`Subview`](Subview.md)

View object

***

### interactiveContext

#### Get Signature

> **get** **interactiveContext**(): [`InteractiveContext`](../interfaces/InteractiveContext.md)

Get Object manager.

##### Returns

[`InteractiveContext`](../interfaces/InteractiveContext.md)

Object manager

***

### isPthreadLoaded

#### Get Signature

> **get** **isPthreadLoaded**(): `boolean`

Check if pthread version of viewer is loaded.

##### Returns

`boolean`

***

### isWasmLoaded

#### Get Signature

> **get** **isWasmLoaded**(): `boolean`

Check if WASM module has been loaded.

##### Returns

`boolean`

***

### renderingParams

#### Get Signature

> **get** **renderingParams**(): [`RenderingParams`](../interfaces/RenderingParams.md)

Get Rendering Parameters manager.

##### Returns

[`RenderingParams`](../interfaces/RenderingParams.md)

Rendering Parameters manager object

***

### rootView

#### Get Signature

> **get** **rootView**(): [`Subview`](Subview.md)

Get root view object (either main view itself or composer of subviews).

##### Returns

[`Subview`](Subview.md)

View object

***

### scaleRuler

#### Get Signature

> **get** **scaleRuler**(): [`InteractiveScaleRuler`](InteractiveScaleRuler.md)

Get InteractiveScaleRuler object.

##### Returns

[`InteractiveScaleRuler`](InteractiveScaleRuler.md)

InteractiveScaleRuler object

***

### selectionManager

#### Get Signature

> **get** **selectionManager**(): [`SelectionManager`](../interfaces/SelectionManager.md)

Get selection manager.

##### Returns

[`SelectionManager`](../interfaces/SelectionManager.md)

selection manager

***

### toPreferPthread

#### Get Signature

> **get** **toPreferPthread**(): `boolean`

Return TRUE if pthread version should be loaded when supported and allowed by Browser.

##### Returns

`boolean`

#### Set Signature

> **set** **toPreferPthread**(`theToPrefer`): `void`

Set if pthread version should be loaded when supported by Browser.
Should be set BEFORE SCwebviewer.init()!

##### Parameters

###### theToPrefer

`boolean`

##### Returns

`void`

***

### toTraceImages

#### Get Signature

> **get** **toTraceImages**(): `boolean`

Flag to enable extra verbose logging.

##### Returns

`boolean`

#### Set Signature

> **set** **toTraceImages**(`theToEnable`): `void`

Flag to enable extra verbose logging.

##### Parameters

###### theToEnable

`boolean`

##### Returns

`void`

***

### traceLevel

#### Get Signature

> **get** **traceLevel**(): [`MessageGravity`](../enumerations/MessageGravity.md)

Get gravity level for logging messages.

##### Returns

[`MessageGravity`](../enumerations/MessageGravity.md)

lower message gravity to log

#### Set Signature

> **set** **traceLevel**(`theGravity`): `void`

Set gravity level for logging messages.

##### Parameters

###### theGravity

[`MessageGravity`](../enumerations/MessageGravity.md)

lower message gravity to log

##### Returns

`void`

***

### unitManager

#### Get Signature

> **get** **unitManager**(): [`UnitManager`](../interfaces/UnitManager.md)

Get unit conversion manager.

##### Returns

[`UnitManager`](../interfaces/UnitManager.md)

unit conversion manager

***

### viewCube

#### Get Signature

> **get** **viewCube**(): [`InteractiveViewCube`](InteractiveViewCube.md)

Get InteractiveViewCube object.

##### Returns

[`InteractiveViewCube`](InteractiveViewCube.md)

InteractiveViewCube object

***

### viewManager

#### Get Signature

> **get** **viewManager**(): [`ViewManager`](../interfaces/ViewManager.md)

Get subview manager.

##### Returns

[`ViewManager`](../interfaces/ViewManager.md)

subview manager object

## Methods

### checkFile()

> **checkFile**(`theFilePath`): `boolean`

Check if file exists in emulated file system.

#### Parameters

##### theFilePath

`string`

#### Returns

`boolean`

***

### dumpFontsInfo()

> **dumpFontsInfo**(`theFontName`, `theIsStrict`, `theToPrintDetails`): `void`

Dump fonts information.

#### Parameters

##### theFontName

`string` = `''`

font name to find or empty string to print all fonts

##### theIsStrict

`boolean` = `false`

flag to find exact font name

##### theToPrintDetails

`boolean` = `true`

flag to print details or only font name

#### Returns

`void`

***

### dumpMemInfo()

> **dumpMemInfo**(): `void`

Print memory stats

#### Returns

`void`

***

### getImageSize()

> **getImageSize**(`theImageId`, `theIsNatural`): [`IVec2`](../interfaces/IVec2.md)

Return dimensions of registered image.

#### Parameters

##### theImageId

`string`

##### theIsNatural

`boolean` = `false`

#### Returns

[`IVec2`](../interfaces/IVec2.md)

***

### getScreenshot()

> **getScreenshot**(`theWidth`, `theHeight`): `string`

Get screenshot.

#### Parameters

##### theWidth

`number`

of screenshot (`-1` - take size of canvas)

##### theHeight

`number`

of screenshot (`-1` - take size of canvas)

#### Returns

`string`

string in the format image/png or empty

***

### init()

> **init**(): `Promise`\<`void`\>

Init SCwebviewer Module and load wasm file.

#### Returns

`Promise`\<`void`\>

***

### isImageRegistered()

> **isImageRegistered**(`theImageId`): `boolean`

Return TRUE if preloaded image is defined.

#### Parameters

##### theImageId

`string`

#### Returns

`boolean`

***

### isImageScalable()

> **isImageScalable**(`theImageId`): [`IVec2`](../interfaces/IVec2.md)

Return TRUE if preloaded image is scalable (e.g. SVG).

#### Parameters

##### theImageId

`string`

#### Returns

[`IVec2`](../interfaces/IVec2.md)

***

### isPthreadAllowed()

> **isPthreadAllowed**(): `boolean`

Check if multithreading is supported and allowed by Browser.

#### Returns

`boolean`

***

### isWasm64Supported()

> **isWasm64Supported**(): `boolean`

Check browser support and return TRUE if WASM64 supported.

#### Returns

`boolean`

***

### isWasmSupported()

> **isWasmSupported**(): `boolean`

Check browser support and return TRUE if WASM supported.

#### Returns

`boolean`

***

### locateFile()

> **locateFile**(`thePath`, `thePrefix`): `string`

Callback returning file path for loading WebAssembly components.

#### Parameters

##### thePath

`string`

file path to locate

##### thePrefix

`string`

default file prefix

#### Returns

`string`

full path to the resource

***

### makeFileLink()

> **makeFileLink**(`theFileName`, `theMime`): `boolean`

Make a link to download file from virtual file system.

#### Parameters

##### theFileName

`string`

##### theMime

`string` = `'application/octet-stream'`

#### Returns

`boolean`

***

### makeScreenshot()

> **makeScreenshot**(`theFilename`, `theWidth`, `theHeight`): `boolean`

Make screenshot and download.

#### Parameters

##### theFilename

`string`

file name to download

##### theWidth

`number`

of screenshot (`-1` - take size of canvas)

##### theHeight

`number`

of screenshot (`-1` - take size of canvas)

#### Returns

`boolean`

true on success

***

### onRuntimeInitialized()

> **onRuntimeInitialized**(): `void`

WebAssembly module callback on runtime initialization.

#### Returns

`void`

***

### preloadFile()

> **preloadFile**(`theBuffer`, `theFilePath`, `theToPreload`): `Promise`\<`boolean`\>

Take buffer and upload it to emulated file system.

#### Parameters

##### theBuffer

`Uint8Array`

buffer to load

##### theFilePath

`string`

file path on emulated file system (or empty string to take name from URL)

##### theToPreload

`boolean`

decode image file using Emscripten plugins

#### Returns

`Promise`\<`boolean`\>

evaluation result as promise returning TRUE or Error

***

### print()

> **print**(`theText`): `void`

C++ `std::cout` callback redirecting to Terminal.

#### Parameters

##### theText

`string`

text to print

#### Returns

`void`

***

### printErr()

> **printErr**(`theText`): `void`

C++ `std::cerr` callback redirecting to Terminal.

#### Parameters

##### theText

`string`

text to print

#### Returns

`void`

***

### printMessage()

> **printMessage**(`theText`, `theGravity`): `void`

C++ `Message::Send()` callback redirecting to Terminal.

#### Parameters

##### theText

`string`

text to print

##### theGravity

[`MessageGravity`](../enumerations/MessageGravity.md)

message gravity within 0..4 range

#### Returns

`void`

***

### registerFontFromUrl()

> **registerFontFromUrl**(`theFontUrl`, `theFontName`, `theToOverride`): `Promise`\<`void`\>

Register font (file in TTF format).
This should be done before displaying text labels using this font,
otherwise a fallback font will be used instead.

#### Parameters

##### theFontUrl

`string`

url of font to download and register

##### theFontName

`string` = `''`

font name or empty string to keep name from file itself

##### theToOverride

`boolean` = `false`

override existing font family

#### Returns

`Promise`\<`void`\>

***

### registerImage()

> **registerImage**(`theImage`, `theImageId`, `theIsScalable`): `boolean`

Register image to be accessible by viewer.
This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.

#### Parameters

##### theImage

`HTMLImageElement`

##### theImageId

`string`

##### theIsScalable

`boolean` = `undefined`

#### Returns

`boolean`

***

### registerImageFromUrl()

> **registerImageFromUrl**(`theImageURl`, `theImageId`, `theIsScalable`): `Promise`\<`void`\>

Load and register SVG image to be accessible by viewer.
This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.

#### Parameters

##### theImageURl

`string`

##### theImageId

`string` = `''`

##### theIsScalable

`boolean` = `undefined`

#### Returns

`Promise`\<`void`\>

***

### renderImage()

> **renderImage**(`theImageId`, `theW`, `theH`): `number`

Render registered image and return raw RGBA data buffer to be released by Module._free().

#### Parameters

##### theImageId

`string`

##### theW

`number`

##### theH

`number`

#### Returns

`number`

***

### requestViewUpdate()

> **requestViewUpdate**(): `void`

Request View update (using `requestAnimationFrame()` or similar method).

#### Returns

`void`

***

### resizeViewport()

> **resizeViewport**(): `void`

Resize rendering window.

#### Returns

`void`

***

### setBackgroundCubemap()

> **setBackgroundCubemap**(`theImagePath`, `theLightmapOnly`): `Promise`\<`void`\>

Set cubemap background. File will be loaded asynchronously.

#### Parameters

##### theImagePath

`string`

image path to load

##### theLightmapOnly

`boolean`

#### Returns

`Promise`\<`void`\>

promise with image loading results

***

### unlinkFile()

> **unlinkFile**(`theFilePath`): `boolean`

Unlinks the node at path.
This removes a name from the file system.
If that name was the last link to a file (and no processes have the file open) the file is deleted.
Usually you should call this function after [uploadUrl](#uploadurl) to remove file which is no longer needed.

#### Parameters

##### theFilePath

`string`

file path on emulated file system (or empty string to take name from URL)

#### Returns

`boolean`

***

### uploadArrayBuffer()

> **uploadArrayBuffer**(`theArray`, `theFilePath`, `theToPreload`): `Promise`\<`boolean`\>

Upload specified ArrayBuffer to emulated file system.
To remove file from file system you should call [unlinkFile](#unlinkfile) if it is no longer needed

#### Parameters

##### theArray

`ArrayBuffer`

array buffer to load

##### theFilePath

`string`

file path on emulated file system (or empty string to take name from URL)

##### theToPreload

`boolean`

decode image file using Emscripten plugins

#### Returns

`Promise`\<`boolean`\>

evaluation result as promise returning TRUE or Error

***

### uploadUrl()

> **uploadUrl**(`theFileUrl`, `theFilePath`, `theToPreload`): `Promise`\<`boolean`\>

Fetch remote file from specified URL and upload it to emulated file system.
To remove file from file system you should call [unlinkFile](#unlinkfile) if it is no longer needed

#### Parameters

##### theFileUrl

`string`

URL to load

##### theFilePath

`string`

file path on emulated file system (or empty string to take name from URL)

##### theToPreload

`boolean`

decode image file using Emscripten plugins

#### Returns

`Promise`\<`boolean`\>

evaluation result as promise returning TRUE or Error
