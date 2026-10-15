import { ObjectPart } from './InteractiveObject.js';
import { ViewerEventTarget } from './ViewerEventTarget.js';
/**
 * Represents dictionary with viewer events.
 */
export const UserEvents = {
    /**
     * Called when {@link InteractiveObject} is dragged.
     * Event definition: {@link ObjectDragEvent}.
     */
    onObjectDragged: 'onObjectDragged',
    /**
     * Called when mouse button is clicked over {@link InteractiveObject}.
     * Event definition: {@link ObjectMouseClickEvent}.
     */
    onObjectMouseClick: 'onObjectMouseClick',
    /** On selection change. */
    onSelectedObjectsChanged: 'onSelectedObjectsChanged',
    /** On detection change. */
    onDetectedObjectChanged: 'onDetectedObjectChanged',
    /**
     * Called when {@link Subview} fullscreen mode is changed.
     * Event definition: {@link SubviewFullscreenEvent}.
     */
    onSubviewFullscreen: 'onSubviewFullscreen',
    /**
     * Called when {@link Subview} is clicked.
     * Event definition: {@link SubviewClickEvent}.
     */
    onSubviewClick: 'onSubviewClick',
    /**
     * Called when display/system unit is changed for {@link UnitManager}.
     * Event definition: {@link UnitsChangedEvent}.
     */
    onUnitsChanged: 'onUnitsChanged',
    /**
     * Called when color map is changed for {@link InteractiveColorScale} object,
     * recieved also by linked {@link InteractiveCaeMesh} object.
     * Event definition: {@link ColorMapperChangedEvent}.
     */
    onColorMapperChanged: 'onColorMapperChanged',
    /**
     * Called when animation step is changed for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshAnimationChangedEvent}.
     */
    onMeshAnimationChanged: 'onMeshAnimationChanged',
    /**
     * Called when properties are requested to load for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshPropertiesRequestedEvent}.
     */
    onMeshPropertiesRequested: 'onMeshPropertiesRequested',
    /**
     * Called when geometry is requested to load for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshGenGeomRequestedEvent}.
     */
    onMeshGenGeomRequested: 'onMeshGenGeomRequested',
    /**
     * Called when Viewer emits a log/error message.
     * Event definition: {@link ViewerMessageEvent}.
     */
    onViewerMessage: 'onViewerMessage',
};
/**
 * {@link InteractiveObject} dragging event {@link UserEvents.onObjectDragged}.
 */
export class ObjectDragEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.object = null;
        this.part = new ObjectPart();
        this.action = -1;
        this.isStopped = false;
    }
}
;
/**
 * {@link InteractiveObject} mouse click event {@link UserEvents.onObjectMouseClick}.
 */
export class ObjectMouseClickEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.reset();
    }
    /** Returns TRUE - default handling could be canceled. */
    get cancelable() { return true; }
    /** Returns TRUE if preventDefault() was called. */
    get defaultPrevented() { return this._myIsDefaultPrevented; }
    /** Reset event. */
    reset() {
        this.object = null;
        this.part = new ObjectPart();
        this.point = { x: 0, y: 0 };
        this.button = 0;
        this.ctrlKey = false;
        this.shiftKey = false;
        this.altKey = false;
        this.metaKey = false;
        this.isDoubleClick = false;
        this._myIsDefaultPrevented = false;
    }
    /** Disable default processing of event. */
    preventDefault() {
        super.preventDefault();
        this._myIsDefaultPrevented = true;
    }
}
;
/**
 * {@link Subview} fullscreen mode change event {@link UserEvents.onSubviewFullscreen}.
 */
export class SubviewFullscreenEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.isFullscreen = false;
        this.view = null;
    }
}
;
/**
 * {@link Subview} click event {@link UserEvents.onSubviewClick}.
 */
export class SubviewClickEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.view = null;
    }
}
;
/**
 * Units change event {@link UserEvents.onUnitsChanged} for {@link UnitManager}.
 */
export class UnitsChangedEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.quantity = null;
        this.section = -1;
    }
}
;
/**
 * Change status of {@link InteractiveColorScale} object,
 * event {@link UserEvents.onColorMapperChanged}.
 */
export var ColorScaleChangeStatus;
(function (ColorScaleChangeStatus) {
    ColorScaleChangeStatus[ColorScaleChangeStatus["Update"] = 0] = "Update";
    ColorScaleChangeStatus[ColorScaleChangeStatus["Create"] = 1] = "Create";
})(ColorScaleChangeStatus || (ColorScaleChangeStatus = {}));
;
/**
 * {@link InteractiveCaeMesh} color mapper create/change event.
 * Recieved also by linked {@link InteractiveColorScale} object.
 */
export class ColorMapperChangedEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.mesh = null;
        this.meshId = '';
        this.propertyId = '';
        this.componentIndex = -1;
        this.colorScale = null;
        this.changeStatus = ColorScaleChangeStatus.Create;
    }
}
;
/**
 * {@link InteractiveCaeMesh} animation step change event,
 * {@link UserEvents.onMeshAnimationChanged}.
 */
export class MeshAnimationChangedEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.mesh = null;
        this.activeTimeStep = -1;
        this.isChanged = false;
        this.isRunning = false;
    }
}
;
/**
 * {@link InteractiveCaeMesh} mesh properties request event,
 * {@link UserEvents.onMeshPropertiesRequested}.
 */
export class MeshPropertiesRequestedEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.mesh = null;
        this.timeStep = -1;
        this.propsToLoad = [];
    }
}
;
/**
 * {@link InteractiveCaeMesh} mesh geometry request event,
 * {@link UserEvents.onMeshGenGeomRequested}.
 */
export class MeshGenGeomRequestedEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.mesh = null;
        this.prsId = '';
        this.partId = -1;
        this.parameters = '';
    }
}
;
/**
 * Viewer message event, {@link UserEvents.onViewerMessage}.
 */
export class ViewerMessageEvent extends Event {
    /** Main constructor. */
    constructor(theType) {
        super(theType);
        this.message = '';
        this.rawMessage = '';
        this.gravity = 0;
        this.errorCode = null;
    }
}
;
/**
 * Event Manager is a global object accessible via {@link SCwebviewer.eventManager},
 * responsible for managing event handlers (subscribing and unsubscribing callbacks).
 */
export class ViewerEventManager extends ViewerEventTarget {
    /** Main constructor. */
    constructor(theViewer) {
        super();
        this._myViewer = theViewer;
    }
    // callbacks called from C++ level
    /** Emit object dragged event */
    onObjectDragged(thePartId, theSubPart, theAction) {
        if (!this._myViewer.interactiveContext) {
            return false;
        }
        const anObjId = this._myViewer.interactiveContext.getLastDraggedObject();
        const aDragObj = this._myViewer.interactiveContext.findInteractiveObject(anObjId);
        let anEvent = new ObjectDragEvent(UserEvents.onObjectDragged);
        anEvent.object = aDragObj;
        anEvent.part.id = thePartId;
        anEvent.part.subId = theSubPart;
        anEvent.action = theAction;
        anEvent.isStopped = theAction === 2; // AIS_DragAction_Stop
        if (aDragObj != null) {
            aDragObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
        return !anEvent.defaultPrevented;
    }
    /** Emit object click event */
    onObjectMouseClick(theObjId, thePart, theSubPart, theX, theY, theFlags) {
        if (!this._myViewer.interactiveContext) {
            return false;
        }
        const anObj = this._myViewer.interactiveContext.findInteractiveObject(theObjId);
        let anEvent = new ObjectMouseClickEvent(UserEvents.onObjectMouseClick);
        anEvent.reset();
        anEvent.object = anObj;
        anEvent.part.id = thePart;
        anEvent.part.subId = theSubPart;
        anEvent.point = { x: theX, y: theY };
        if (theFlags & (1 << 13)) {
            anEvent.button = 0x1;
        } // Aspect_VKeyMouse_LeftButton
        if (theFlags & (1 << 15)) {
            anEvent.button = 0x2;
        } // Aspect_VKeyMouse_RightButton
        if (theFlags & (1 << 14)) {
            anEvent.button = 0x4;
        } // Aspect_VKeyMouse_MiddleButton
        anEvent.ctrlKey = (theFlags & (1 << 9)) !== 0; // Aspect_VKeyFlags_CTRL
        anEvent.shiftKey = (theFlags & (1 << 8)) !== 0; // Aspect_VKeyFlags_SHIFT
        anEvent.altKey = (theFlags & (1 << 10)) !== 0; // Aspect_VKeyFlags_ALT
        anEvent.metaKey = (theFlags & (1 << 12)) !== 0; // Aspect_VKeyFlags_META
        anEvent.isDoubleClick = (theFlags & (1 << 20)) !== 0; // extension
        if (anObj != null) {
            anObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
        return !anEvent.defaultPrevented;
    }
    /** Emit objects selection change event */
    onSelectedObjectsChanged() {
        if (!this._myViewer.selectionManager) {
            return false;
        }
        const aSelList = this._myViewer.selectionManager.selectedObjects;
        let anEvent = new Event(UserEvents.onSelectedObjectsChanged);
        aSelList.forEach((theObj) => {
            theObj.dispatchEvent(anEvent);
        });
        return this.dispatchEvent(anEvent);
    }
    /** Emit object detection change event */
    onDetectedObjectChanged() {
        if (!this._myViewer.selectionManager) {
            return false;
        }
        const aDetObj = this._myViewer.selectionManager.lastDetectedObject;
        let anEvent = new Event(UserEvents.onDetectedObjectChanged);
        if (aDetObj != null) {
            aDetObj.dispatchEvent(anEvent);
        }
        ;
        return this.dispatchEvent(anEvent);
    }
    /** Emit subview fullscreen mode change event */
    onSubviewFullscreen(theViewId, theIsFullscreen) {
        if (!this._myViewer.viewManager) {
            return false;
        }
        const aSubview = this._myViewer.viewManager.findSubviewObject(theViewId);
        let anEvent = new SubviewFullscreenEvent(UserEvents.onSubviewFullscreen);
        anEvent.isFullscreen = theIsFullscreen;
        anEvent.view = aSubview;
        if (anEvent.view != null && aSubview != null) {
            aSubview.dispatchEvent(anEvent);
        }
        return this.dispatchEvent(anEvent);
    }
    /** Emit subview click event */
    onSubviewClick(theViewId) {
        if (!this._myViewer.viewManager) {
            return;
        }
        const aSubview = this._myViewer.viewManager.findSubviewObject(theViewId);
        let anEvent = new SubviewClickEvent(UserEvents.onSubviewClick);
        anEvent.view = aSubview;
        if (anEvent.view != null && aSubview != null) {
            aSubview.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
    }
    /** Emit units change event */
    onUnitsChanged(theQuantType, theSection) {
        if (!this._myViewer.unitManager) {
            return;
        }
        const aQuant = this._myViewer.unitManager.quantities[theQuantType];
        let anEvent = new UnitsChangedEvent(UserEvents.onUnitsChanged);
        anEvent.quantity = aQuant;
        anEvent.section = theSection;
        this.dispatchEvent(anEvent);
    }
    /** Emit mesh color mapper change event */
    onColorMapperChanged(theMeshId, thePropertyId, theComponentIndex, theColorScaleObjId) {
        if (!this._myViewer.interactiveContext) {
            return;
        }
        const anObj = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);
        const aColorScaleObj = this._myViewer.interactiveContext.findInteractiveObject(theColorScaleObjId);
        let anEvent = new ColorMapperChangedEvent(UserEvents.onColorMapperChanged);
        anEvent.mesh = anObj;
        anEvent.meshId = theMeshId;
        anEvent.propertyId = thePropertyId;
        anEvent.componentIndex = theComponentIndex;
        anEvent.colorScale = aColorScaleObj;
        anEvent.changeStatus = anEvent.colorScale == null && anEvent.propertyId !== '' ? ColorScaleChangeStatus.Create
            : ColorScaleChangeStatus.Update;
        if (anObj != null) {
            anObj.dispatchEvent(anEvent);
        }
        if (aColorScaleObj != null) {
            aColorScaleObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
    }
    /** Emit mesh animation step change event */
    onMeshAnimationChanged(theMeshId, theActiveTimeStep, theIsChanged, theIsRunning) {
        if (!this._myViewer.interactiveContext) {
            return;
        }
        const anObj = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);
        let anEvent = new MeshAnimationChangedEvent(UserEvents.onMeshAnimationChanged);
        anEvent.mesh = anObj;
        anEvent.activeTimeStep = theActiveTimeStep;
        anEvent.isChanged = theIsChanged;
        anEvent.isRunning = theIsRunning;
        if (anObj != null) {
            anObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
    }
    /** Emit mesh properties request event */
    onMeshPropertiesRequested(theMeshId, theTimeStep, thePropsToLoad) {
        if (!this._myViewer.interactiveContext) {
            return;
        }
        const anObj = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);
        let anEvent = new MeshPropertiesRequestedEvent(UserEvents.onMeshPropertiesRequested);
        anEvent.mesh = anObj;
        anEvent.timeStep = theTimeStep;
        anEvent.propsToLoad = thePropsToLoad === '' ? [] : thePropsToLoad.split('\n');
        if (anObj != null) {
            anObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
    }
    /** Emit mesh geometry request event */
    onMeshGenGeomRequested(theMeshId, thePrsId, thePartId, theParameters) {
        if (!this._myViewer.interactiveContext) {
            return;
        }
        const anObj = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);
        let anEvent = new MeshGenGeomRequestedEvent(UserEvents.onMeshGenGeomRequested);
        anEvent.mesh = anObj;
        anEvent.prsId = thePrsId;
        anEvent.partId = thePartId;
        anEvent.parameters = theParameters;
        if (anObj != null) {
            anObj.dispatchEvent(anEvent);
        }
        this.dispatchEvent(anEvent);
    }
    /** Emit viewer message event. */
    onViewerMessage(theMessage, theGravity, theErrorCode, theRawMessage) {
        let anEvent = new ViewerMessageEvent(UserEvents.onViewerMessage);
        anEvent.message = theMessage !== null && theMessage !== void 0 ? theMessage : '';
        anEvent.rawMessage = theRawMessage !== null && theRawMessage !== void 0 ? theRawMessage : '';
        anEvent.gravity = theGravity;
        anEvent.errorCode = theErrorCode;
        this.dispatchEvent(anEvent);
    }
}
;
//# sourceMappingURL=ViewerEventManager.js.map