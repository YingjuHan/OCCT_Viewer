[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / UnitManager

# Interface: UnitManager

Represents tool for managing units.
See also [SCwebviewer.unitManager](../classes/SCwebviewer.md#unitmanager) property.

## Extends

- [`IManager`](IManager.md)

## Properties

### angle

> `readonly` **angle**: [`Quantity`](../classes/Quantity.md)

Return plane angle quantity

***

### area

> `readonly` **area**: [`Quantity`](../classes/Quantity.md)

Return area quantity

***

### convert()

> **convert**: (`theVal`, `theFromUnit`, `theToUnit`) => `number`

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

> **convertAnyFromSI**: (`theVal`, `theToUnit`) => `number`

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

> **convertAnyToSI**: (`theVal`, `theFromUnit`) => `number`

Convert value to SI unit from another unit.

#### Parameters

##### theVal

`number`

##### theFromUnit

`string`

#### Returns

`number`

***

### formatUnitName()

> **formatUnitName**: (`theUnit`) => `string`

Format unit name (e.g. replace '**2' with squared sign and similar).

#### Parameters

##### theUnit

`string`

#### Returns

`string`

***

### length

> `readonly` **length**: [`Quantity`](../classes/Quantity.md)

Return length quantity

***

### quantities

> `readonly` **quantities**: [`Quantity`](../classes/Quantity.md)[]

Return the array of quantites

***

### volume

> `readonly` **volume**: [`Quantity`](../classes/Quantity.md)

Return volume quantity
