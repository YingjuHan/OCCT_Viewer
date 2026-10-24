import type { IVec2, ObjectId, Quantity } from './CommonTools.js';
import { ObjectPart } from './InteractiveObject.js';
import type { InteractiveObject } from './InteractiveObject.js';
import type { InteractiveCaeMesh } from './InteractiveCaeMesh.js';
import type { InteractiveColorScale } from './InteractiveColorScale.js';
import type { Subview } from './Subview.js';
import { ViewerEventTarget } from './ViewerEventTarget.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Represents dictionary with viewer events.
 */
export declare const UserEvents: {
    /**
     * Called when {@link InteractiveObject} is dragged.
     * Event definition: {@link ObjectDragEvent}.
     */
    onObjectDragged: string;
    /**
     * Called when mouse button is clicked over {@link InteractiveObject}.
     * Event definition: {@link ObjectMouseClickEvent}.
     */
    onObjectMouseClick: string;
    /** On selection change. */
    onSelectedObjectsChanged: string;
    /** On detection change. */
    onDetectedObjectChanged: string;
    /**
     * Called when {@link Subview} fullscreen mode is changed.
     * Event definition: {@link SubviewFullscreenEvent}.
     */
    onSubviewFullscreen: string;
    /**
     * Called when {@link Subview} is clicked.
     * Event definition: {@link SubviewClickEvent}.
     */
    onSubviewClick: string;
    /**
     * Called when display/system unit is changed for {@link UnitManager}.
     * Event definition: {@link UnitsChangedEvent}.
     */
    onUnitsChanged: string;
    /**
     * Called when color map is changed for {@link InteractiveColorScale} object,
     * recieved also by linked {@link InteractiveCaeMesh} object.
     * Event definition: {@link ColorMapperChangedEvent}.
     */
    onColorMapperChanged: string;
    /**
     * Called when animation step is changed for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshAnimationChangedEvent}.
     */
    onMeshAnimationChanged: string;
    /**
     * Called when properties are requested to load for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshPropertiesRequestedEvent}.
     */
    onMeshPropertiesRequested: string;
    /**
     * Called when geometry is requested to load for {@link InteractiveCaeMesh} object.
     * Event definition: {@link MeshGenGeomRequestedEvent}.
     */
    onMeshGenGeomRequested: string;
};
/**
 * {@link InteractiveObject} dragging event {@link UserEvents.onObjectDragged}.
 */
export declare class ObjectDragEvent extends Event {
    /** Dragged object. */
    object: InteractiveObject | null;
    /** Object's part. */
    part: ObjectPart;
    /** Dragging action within `AIS_DragAction` C++ enum. */
    action: number;
    /** Dragging action, TRUE if dragging is completed and FALSE otherwise. */
    isStopped: boolean;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * {@link InteractiveObject} mouse click event {@link UserEvents.onObjectMouseClick}.
 */
export declare class ObjectMouseClickEvent extends Event {
    /** Clicked object. */
    object: InteractiveObject | null;
    /** Object part id. */
    part: ObjectPart;
    /** Mouse position. */
    point: IVec2;
    /** Pressed mouse button, as in MouseEvent.button */
    button: number;
    /** Keyboard flags */
    ctrlKey: boolean;
    shiftKey: boolean;
    altKey: boolean;
    metaKey: boolean;
    /** Flag indicating a double-click event. */
    isDoubleClick: boolean;
    private _myIsDefaultPrevented;
    /** Main constructor. */
    constructor(theType: string);
    /** Returns TRUE - default handling could be canceled. */
    get cancelable(): boolean;
    /** Returns TRUE if preventDefault() was called. */
    get defaultPrevented(): boolean;
    /** Reset event. */
    reset(): void;
    /** Disable default processing of event. */
    preventDefault(): void;
}
/**
 * {@link Subview} fullscreen mode change event {@link UserEvents.onSubviewFullscreen}.
 */
export declare class SubviewFullscreenEvent extends Event {
    /** New fullscreen state. */
    isFullscreen: boolean;
    /** Altered Subview. */
    view: Subview | null;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * {@link Subview} click event {@link UserEvents.onSubviewClick}.
 */
export declare class SubviewClickEvent extends Event {
    /** Altered Subview. */
    view: Subview | null;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * Units change event {@link UserEvents.onUnitsChanged} for {@link UnitManager}.
 */
export declare class UnitsChangedEvent extends Event {
    /** Altered quantity. */
    quantity: Quantity | null;
    /** Altered section. */
    section: number;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * Change status of {@link InteractiveColorScale} object,
 * event {@link UserEvents.onColorMapperChanged}.
 */
export declare enum ColorScaleChangeStatus {
    Update = 0,
    Create = 1
}
/**
 * {@link InteractiveCaeMesh} color mapper create/change event.
 * Recieved also by linked {@link InteractiveColorScale} object.
 */
export declare class ColorMapperChangedEvent extends Event {
    /** Mesh object. */
    mesh: InteractiveCaeMesh | null;
    /** @deprecated Id of the corresponding mesh object. */
    meshId: ObjectId;
    /** Mesh property id. */
    propertyId: string;
    /** Mesh property component index. */
    componentIndex: number;
    /** Color scale object. */
    colorScale: InteractiveColorScale | null;
    /** Change status. */
    changeStatus: ColorScaleChangeStatus;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * {@link InteractiveCaeMesh} animation step change event,
 * {@link UserEvents.onMeshAnimationChanged}.
 */
export declare class MeshAnimationChangedEvent extends Event {
    /** Mesh object. */
    mesh: InteractiveCaeMesh | null;
    /** Mesh active time step. */
    activeTimeStep: number;
    /** Mesh active time step changed flag. */
    isChanged: boolean;
    /** Mesh animation status. */
    isRunning: boolean;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * {@link InteractiveCaeMesh} mesh properties request event,
 * {@link UserEvents.onMeshPropertiesRequested}.
 */
export declare class MeshPropertiesRequestedEvent extends Event {
    /** Mesh object. */
    mesh: InteractiveCaeMesh | null;
    /** Mesh properties time step. */
    timeStep: number;
    /** Properties that are required for time step but not loaded. */
    propsToLoad: Array<string>;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * {@link InteractiveCaeMesh} mesh geometry request event,
 * {@link UserEvents.onMeshGenGeomRequested}.
 */
export declare class MeshGenGeomRequestedEvent extends Event {
    /** Mesh object. */
    mesh: InteractiveCaeMesh | null;
    /** Mesh presentation id. */
    prsId: string;
    /** Mesh presentation part id. */
    partId: number;
    /** Mesh presentation parameters as Json string. */
    parameters: string;
    /** Main constructor. */
    constructor(theType: string);
}
/**
 * Event Manager is a global object accessible via {@link SCwebviewer.eventManager},
 * responsible for managing event handlers (subscribing and unsubscribing callbacks).
 */
export declare class ViewerEventManager extends ViewerEventTarget {
    private _myViewer;
    /** Main constructor. */
    constructor(theViewer: SCwebviewer);
    /** Emit object dragged event */
    protected onObjectDragged(thePartId: number, theSubPart: number, theAction: number): boolean;
    /** Emit object click event */
    protected onObjectMouseClick(theObjId: ObjectId, thePart: number, theSubPart: number, theX: number, theY: number, theFlags: number): boolean;
    /** Emit objects selection change event */
    protected onSelectedObjectsChanged(): boolean;
    /** Emit object detection change event */
    protected onDetectedObjectChanged(): boolean;
    /** Emit subview fullscreen mode change event */
    protected onSubviewFullscreen(theViewId: number, theIsFullscreen: boolean): boolean;
    /** Emit subview click event */
    protected onSubviewClick(theViewId: number): void;
    /** Emit units change event */
    protected onUnitsChanged(theQuantType: number, theSection: number): void;
    /** Emit mesh color mapper change event */
    protected onColorMapperChanged(theMeshId: ObjectId, thePropertyId: string, theComponentIndex: number, theColorScaleObjId: ObjectId): void;
    /** Emit mesh animation step change event */
    protected onMeshAnimationChanged(theMeshId: ObjectId, theActiveTimeStep: number, theIsChanged: boolean, theIsRunning: boolean): void;
    /** Emit mesh properties request event */
    protected onMeshPropertiesRequested(theMeshId: ObjectId, theTimeStep: number, thePropsToLoad: string): void;
    /** Emit mesh geometry request event */
    protected onMeshGenGeomRequested(theMeshId: ObjectId, thePrsId: string, thePartId: number, theParameters: string): void;
}
