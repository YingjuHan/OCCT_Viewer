[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / VisMaterial

# Class: VisMaterial

Object defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).

## Implements

- [`IVisMaterial`](../interfaces/IVisMaterial.md)

## Constructors

### Constructor

> **new VisMaterial**(): `VisMaterial`

#### Returns

`VisMaterial`

## Properties

### baseColor

> **baseColor**: `string` = `'WHITE'`

base RGBA color (includes transparency).

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`baseColor`](../interfaces/IVisMaterial.md#basecolor)

***

### emissiveFactor

> **emissiveFactor**: [`IVec3`](../interfaces/IVec3.md)

emissibe RGB color.

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`emissiveFactor`](../interfaces/IVisMaterial.md#emissivefactor)

***

### isDefined

> **isDefined**: `boolean` = `true`

flag indicating undefined material.

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`isDefined`](../interfaces/IVisMaterial.md#isdefined)

***

### metallic

> **metallic**: `number` = `0.0`

metallic factor within [0..1] range.

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`metallic`](../interfaces/IVisMaterial.md#metallic)

***

### refractionIndex

> **refractionIndex**: `number` = `1.5`

IOR.

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`refractionIndex`](../interfaces/IVisMaterial.md#refractionindex)

***

### roughness

> **roughness**: `number` = `1.0`

roughness factor within [0..1] range.

#### Implementation of

[`IVisMaterial`](../interfaces/IVisMaterial.md).[`roughness`](../interfaces/IVisMaterial.md#roughness)
