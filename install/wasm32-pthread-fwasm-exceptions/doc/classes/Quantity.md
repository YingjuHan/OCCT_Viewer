[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / Quantity

# Class: Quantity

Defines quantity units convertion tool.

## Constructors

### Constructor

> **new Quantity**(`theMgr`, `theType`): `Quantity`

Main constructor.

#### Parameters

##### theMgr

[`UnitManager`](../interfaces/UnitManager.md)

##### theType

[`QuantityType`](../enumerations/QuantityType.md)

#### Returns

`Quantity`

## Accessors

### name

#### Get Signature

> **get** **name**(): `string`

Return Quantity name

##### Returns

`string`

***

### type

#### Get Signature

> **get** **type**(): [`QuantityType`](../enumerations/QuantityType.md)

Return Quantity type

##### Returns

[`QuantityType`](../enumerations/QuantityType.md)

***

### unitDisplay

#### Get Signature

> **get** **unitDisplay**(): `string`

Return quantity display units

##### Returns

`string`

#### Set Signature

> **set** **unitDisplay**(`theUnit`): `void`

Set quantity display units

##### Parameters

###### theUnit

`string`

##### Returns

`void`

***

### unitSI

#### Get Signature

> **get** **unitSI**(): `string`

Return quantity SI units

##### Returns

`string`

***

### unitSystem

#### Get Signature

> **get** **unitSystem**(): `string`

Return quantity system units

##### Returns

`string`

#### Set Signature

> **set** **unitSystem**(`theUnit`): `void`

Set quantity system units

##### Parameters

###### theUnit

`string`

##### Returns

`void`

## Methods

### convert()

> **convert**(`theVal`, `theFromUnit`, `theToUnit`): `number`

Convert value from one unit to another unit.

#### Parameters

##### theVal

`number`

##### theFromUnit

`string`

##### theToUnit

`string`

#### Returns

`number`

***

### convertAnyFromSI()

> **convertAnyFromSI**(`theVal`, `theToUnit`): `number`

Convert value from SI unit to another unit.

#### Parameters

##### theVal

`number`

##### theToUnit

`string`

#### Returns

`number`

***

### convertAnyToSI()

> **convertAnyToSI**(`theVal`, `theFromUnit`): `number`

Convert value to SI unit from another unit.

#### Parameters

##### theVal

`number`

##### theFromUnit

`string`

#### Returns

`number`

***

### format()

> **format**(`theSys`): `string`

Auxiliary tool converting real number from System to Display units and appending units.

#### Parameters

##### theSys

`number` | [`IVec3`](../interfaces/IVec3.md)

#### Returns

`string`

***

### formatReal()

> **formatReal**(`theSys`): `string`

Auxiliary tool converting real number from System to Display units and appending units.

#### Parameters

##### theSys

`number`

#### Returns

`string`

***

### formatVec3()

> **formatVec3**(`theSys`): `string`

Auxiliary tool converting real number from System to Display units and appending units.

#### Parameters

##### theSys

[`IVec3`](../interfaces/IVec3.md)

#### Returns

`string`

***

### systemFromDisplay()

> **systemFromDisplay**(`theDispVal`): `number`

Convert value from display units into system units.

#### Parameters

##### theDispVal

`number`

#### Returns

`number`

***

### systemFromDisplayVec3()

> **systemFromDisplayVec3**(`theDispVal`): [`IVec3`](../interfaces/IVec3.md)

Convert value from display units into system units.

#### Parameters

##### theDispVal

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### systemFromSI()

> **systemFromSI**(`theSIVal`): `number`

Convert value from SI units into system units.

#### Parameters

##### theSIVal

`number`

#### Returns

`number`

***

### systemFromSIVec3()

> **systemFromSIVec3**(`theSIVal`): [`IVec3`](../interfaces/IVec3.md)

Convert value from SI units into system units.

#### Parameters

##### theSIVal

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### systemToDisplay()

> **systemToDisplay**(`theSysVal`): `number`

Convert value from system units into display units.

#### Parameters

##### theSysVal

`number`

#### Returns

`number`

***

### systemToDisplayVec3()

> **systemToDisplayVec3**(`theSysVal`): [`IVec3`](../interfaces/IVec3.md)

Convert value from system units into display units.

#### Parameters

##### theSysVal

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)

***

### systemToSI()

> **systemToSI**(`theSysVal`): `number`

Convert value from system units into SI units.

#### Parameters

##### theSysVal

`number`

#### Returns

`number`

***

### systemToSIVec3()

> **systemToSIVec3**(`theSysVal`): [`IVec3`](../interfaces/IVec3.md)

Convert value from system units into SI units.

#### Parameters

##### theSysVal

[`IVec3`](../interfaces/IVec3.md)

#### Returns

[`IVec3`](../interfaces/IVec3.md)
