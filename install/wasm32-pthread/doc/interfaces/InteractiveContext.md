[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / InteractiveContext

# Interface: InteractiveContext

Represents Context for Interactive Objects.
Note that there is no reason to use class directly at application level
as [InteractiveObject](../classes/InteractiveObject.md) re-exports main properties and hides some implementation details.
See also [SCwebviewer.interactiveContext](../classes/SCwebviewer.md#interactivecontext) property.

## Extends

- [`IManager`](IManager.md)

## Properties

### getLastDraggedObject()

> **getLastDraggedObject**: () => `string`

Return id of last dragged object or empty string.

#### Returns

`string`

***

### objectsInside

> `readonly` **objectsInside**: [`InteractiveObject`](../classes/InteractiveObject.md)[]

Return the array of created objects

***

### removeAll()

> **removeAll**: () => `boolean`

Remove all named objects.
Warning! Using this method would make existing InteractiveObject instances
to have dead references (see also [InteractiveObject.isAlive](../classes/InteractiveObject.md#isalive)).

#### Returns

`boolean`
