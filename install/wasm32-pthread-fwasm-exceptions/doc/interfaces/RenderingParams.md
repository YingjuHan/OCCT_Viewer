[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / RenderingParams

# Interface: RenderingParams

Represents Rendering Parameters manager (based on `Graphic3d_RenderingParams` from C++ OCCT).
See also [SCwebviewer.renderingParams](../classes/SCwebviewer.md#renderingparams) property.

## Properties

### backgroundColor

> **backgroundColor**: `string`

Background RGB color defined as [ColorString](../type-aliases/ColorString.md).

***

### isImageBasedLighting

> **isImageBasedLighting**: `boolean`

IBL (Image Based Lighting) from background cubemap enabled state.

***

### isPbrEnabled

> **isPbrEnabled**: `boolean`

Enable PBR or fallback to legacy Phong shading model.

***

### isShadowEnabled

> **isShadowEnabled**: `boolean`

To cast shadows on/off.

***

### nbMsaaSamples

> **nbMsaaSamples**: `number`

Number of samples (integer) for MSAA buffer, 0 means no MSAA anti-aliasing.

***

### nbOitDepthPeelingLayers

> **nbOitDepthPeelingLayers**: `number`

Number of depth peeling layers.

***

### renderResolutionScale

> **renderResolutionScale**: `number`

Rendering resolution ratio; 1.0 means native resolution (100% scale).

***

### setBackgroundCubemap()

> **setBackgroundCubemap**: (`theImagePath`, `theLightmapOnly`) => `void`

Set cubemap background. File will be loaded asynchronously.

#### Parameters

##### theImagePath

`string`

image path to load

##### theLightmapOnly

`boolean`

flag to not display cubemap in background

#### Returns

`void`

***

### shadowMapBias

> **shadowMapBias**: `number`

Shadow map bias.

***

### shadowMapResolution

> **shadowMapResolution**: `number`

Shadow map resolution in pixels (integer number).

***

### toShowStats

> **toShowStats**: `boolean`

To show stats on/off.

***

### transparencyMethod

> **transparencyMethod**: [`RenderTransparentMethod`](../enumerations/RenderTransparentMethod.md)

Transparency method.
