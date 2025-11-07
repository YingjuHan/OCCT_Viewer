[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / UserEvents

# Variable: UserEvents

> `const` **UserEvents**: `object`

Represents dictionary with viewer events.

## Type Declaration

### onColorMapperChanged

> **onColorMapperChanged**: `string` = `'onColorMapperChanged'`

Called when color map is changed for [InteractiveColorScale](../classes/InteractiveColorScale.md) object,
recieved also by linked [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) object.
Event definition: [ColorMapperChangedEvent](../classes/ColorMapperChangedEvent.md).

### onDetectedObjectChanged

> **onDetectedObjectChanged**: `string` = `'onDetectedObjectChanged'`

On detection change.

### onMeshAnimationChanged

> **onMeshAnimationChanged**: `string` = `'onMeshAnimationChanged'`

Called when animation step is changed for [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) object.
Event definition: [MeshAnimationChangedEvent](../classes/MeshAnimationChangedEvent.md).

### onMeshGenGeomRequested

> **onMeshGenGeomRequested**: `string` = `'onMeshGenGeomRequested'`

Called when geometry is requested to load for [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) object.
Event definition: [MeshGenGeomRequestedEvent](../classes/MeshGenGeomRequestedEvent.md).

### onMeshPropertiesRequested

> **onMeshPropertiesRequested**: `string` = `'onMeshPropertiesRequested'`

Called when properties are requested to load for [InteractiveCaeMesh](../classes/InteractiveCaeMesh.md) object.
Event definition: [MeshPropertiesRequestedEvent](../classes/MeshPropertiesRequestedEvent.md).

### onObjectDragged

> **onObjectDragged**: `string` = `'onObjectDragged'`

Called when [InteractiveObject](../classes/InteractiveObject.md) is dragged.
Event definition: [ObjectDragEvent](../classes/ObjectDragEvent.md).

### onObjectMouseClick

> **onObjectMouseClick**: `string` = `'onObjectMouseClick'`

Called when mouse button is clicked over [InteractiveObject](../classes/InteractiveObject.md).
Event definition: [ObjectMouseClickEvent](../classes/ObjectMouseClickEvent.md).

### onSelectedObjectsChanged

> **onSelectedObjectsChanged**: `string` = `'onSelectedObjectsChanged'`

On selection change.

### onSubviewClick

> **onSubviewClick**: `string` = `'onSubviewClick'`

Called when [Subview](../classes/Subview.md) is clicked.
Event definition: [SubviewClickEvent](../classes/SubviewClickEvent.md).

### onSubviewFullscreen

> **onSubviewFullscreen**: `string` = `'onSubviewFullscreen'`

Called when [Subview](../classes/Subview.md) fullscreen mode is changed.
Event definition: [SubviewFullscreenEvent](../classes/SubviewFullscreenEvent.md).

### onUnitsChanged

> **onUnitsChanged**: `string` = `'onUnitsChanged'`

Called when display/system unit is changed for [UnitManager](../interfaces/UnitManager.md).
Event definition: [UnitsChangedEvent](../classes/UnitsChangedEvent.md).
