[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / SelectionManager

# Interface: SelectionManager

Represents selection manager for Interactive Objects.
See also [SCwebviewer.selectionManager](../classes/SCwebviewer.md#selectionmanager) property.

## Extends

- [`IManager`](IManager.md)

## Properties

### lastDetectedInfo

> `readonly` **lastDetectedInfo**: [`DetectedObjectInfo`](../classes/DetectedObjectInfo.md)

Return the last detected object info

***

### lastDetectedObject

> `readonly` **lastDetectedObject**: [`InteractiveObject`](../classes/InteractiveObject.md)

Return the last detected object

***

### oneSelectedObject

> **oneSelectedObject**: [`InteractiveObject`](../classes/InteractiveObject.md)

Return/set a single selected object. Return null if multiple/none objects selected

***

### resetSelection()

> **resetSelection**: () => `void`

Unselect all previously selected objects

#### Returns

`void`

***

### selectedObjects

> **selectedObjects**: [`InteractiveObject`](../classes/InteractiveObject.md)[]

Return/set the array of selection objects

***

### toAllowMultiselection

> **toAllowMultiselection**: `boolean`

Return/set selection mode multiple-true, single-false
