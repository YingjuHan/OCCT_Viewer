[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / IVisMaterial

# Interface: IVisMaterial

Interface defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).

## Properties

### baseColor

> **baseColor**: `string`

base RGBA color (includes transparency); empty color indicates undefined material.

***

### emissiveFactor

> **emissiveFactor**: [`IVec3`](IVec3.md)

emissibe RGB color.

***

### isDefined

> **isDefined**: `boolean`

flag indicating undefined material.

***

### metallic

> **metallic**: `number`

metallic factor within [0..1] range.

***

### refractionIndex

> **refractionIndex**: `number`

IOR.

***

### roughness

> **roughness**: `number`

roughness factor within [0..1] range.
