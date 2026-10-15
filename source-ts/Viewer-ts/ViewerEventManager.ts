import type {IVec2, ObjectId, Quantity} from './CommonTools.js';
import {ObjectPart} from './InteractiveObject.js';
import type {InteractiveObject} from './InteractiveObject.js';
import type {InteractiveCaeMesh} from './InteractiveCaeMesh.js';
import type {InteractiveColorScale} from './InteractiveColorScale.js';
import type {Subview} from './Subview.js';
import {ViewerEventTarget} from './ViewerEventTarget.js';

import type {SCwebviewer} from './SCwebviewer.js';

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
  /** Dragged object. */
  public object: InteractiveObject | null;

  /** Object's part. */
  public part: ObjectPart;

  /** Dragging action within `AIS_DragAction` C++ enum. */
  public action: number;

  /** Dragging action, TRUE if dragging is completed and FALSE otherwise. */
  public isStopped: boolean;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.object = null;
    this.part = new ObjectPart();
    this.action = -1;
    this.isStopped = false;
  }
};

/**
 * {@link InteractiveObject} mouse click event {@link UserEvents.onObjectMouseClick}.
 */
export class ObjectMouseClickEvent extends Event {
  /** Clicked object. */
  public object: InteractiveObject | null;

  /** Object part id. */
  public part: ObjectPart;

  /** Mouse position. */
  public point: IVec2;

  /** Pressed mouse button, as in MouseEvent.button */
  public button: number;

  /** Keyboard flags */
  public ctrlKey: boolean;
  public shiftKey: boolean;
  public altKey: boolean;
  public metaKey: boolean;

  /** Flag indicating a double-click event. */
  public isDoubleClick: boolean;

  private _myIsDefaultPrevented: boolean;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.reset();
  }

  /** Returns TRUE - default handling could be canceled. */
  public get cancelable(): boolean { return true; }

  /** Returns TRUE if preventDefault() was called. */
  public get defaultPrevented(): boolean { return this._myIsDefaultPrevented; }

  /** Reset event. */
  public reset(): void {
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
  public preventDefault(): void {
    super.preventDefault();
    this._myIsDefaultPrevented = true;
  }
};

/**
 * {@link Subview} fullscreen mode change event {@link UserEvents.onSubviewFullscreen}.
 */
export class SubviewFullscreenEvent extends Event {
  /** New fullscreen state. */
  public isFullscreen: boolean;

  /** Altered Subview. */
  public view: Subview | null;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.isFullscreen = false;
    this.view = null;
  }
};

/**
 * {@link Subview} click event {@link UserEvents.onSubviewClick}.
 */
export class SubviewClickEvent extends Event {
  /** Altered Subview. */
  public view: Subview | null;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.view = null;
  }
};

/**
 * Units change event {@link UserEvents.onUnitsChanged} for {@link UnitManager}.
 */
export class UnitsChangedEvent extends Event {
  /** Altered quantity. */
  public quantity: Quantity | null;

  /** Altered section. */
  public section: number;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.quantity = null;
    this.section = -1;
  }
};

/**
 * Change status of {@link InteractiveColorScale} object,
 * event {@link UserEvents.onColorMapperChanged}.
 */
export enum ColorScaleChangeStatus {
  Update = 0, Create = 1
};

/**
 * {@link InteractiveCaeMesh} color mapper create/change event.
 * Recieved also by linked {@link InteractiveColorScale} object.
 */
export class ColorMapperChangedEvent extends Event {
  /** Mesh object. */
  public mesh: InteractiveCaeMesh | null;
  /** @deprecated Id of the corresponding mesh object. */
  public meshId: ObjectId;
  /** Mesh property id. */
  public propertyId: string;
  /** Mesh property component index. */
  public componentIndex: number;
  /** Color scale object. */
  public colorScale: InteractiveColorScale | null;
  /** Change status. */
  public changeStatus: ColorScaleChangeStatus;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.mesh = null;
    this.meshId = '';
    this.propertyId = '';
    this.componentIndex = -1;
    this.colorScale = null;
    this.changeStatus = ColorScaleChangeStatus.Create;
  }
};

/**
 * {@link InteractiveCaeMesh} animation step change event,
 * {@link UserEvents.onMeshAnimationChanged}.
 */
export class MeshAnimationChangedEvent extends Event {
  /** Mesh object. */
  public mesh: InteractiveCaeMesh | null;
  /** Mesh active time step. */
  public activeTimeStep: number;
  /** Mesh active time step changed flag. */
  public isChanged: boolean;
  /** Mesh animation status. */
  public isRunning: boolean;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.mesh = null;
    this.activeTimeStep = -1;
    this.isChanged = false;
    this.isRunning = false;
  }
};

/**
 * {@link InteractiveCaeMesh} mesh properties request event,
 * {@link UserEvents.onMeshPropertiesRequested}.
 */
export class MeshPropertiesRequestedEvent extends Event {
  /** Mesh object. */
  public mesh: InteractiveCaeMesh | null;
  /** Mesh properties time step. */
  public timeStep: number;
  /** Properties that are required for time step but not loaded. */
  public propsToLoad: Array<string>;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.mesh = null;
    this.timeStep = -1;
    this.propsToLoad = [];
  }
};

/**
 * {@link InteractiveCaeMesh} mesh geometry request event,
 * {@link UserEvents.onMeshGenGeomRequested}.
 */
export class MeshGenGeomRequestedEvent extends Event {
  /** Mesh object. */
  public mesh: InteractiveCaeMesh | null;
  /** Mesh presentation id. */
  public prsId: string;
  /** Mesh presentation part id. */
  public partId: number;
  /** Mesh presentation parameters as Json string. */
  public parameters: string;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.mesh = null;
    this.prsId = '';
    this.partId = -1;
    this.parameters = '';
  }
};

/**
 * Viewer message event, {@link UserEvents.onViewerMessage}.
 */
export class ViewerMessageEvent extends Event {
  /** Message text without error code prefix. */
  public message: string;
  /** Raw message as received from C++ side. */
  public rawMessage: string;
  /** Message gravity as number (see MessageGravity). */
  public gravity: number;
  /** Parsed error code if present. */
  public errorCode: string | null;

  /** Main constructor. */
  constructor(theType: string) {
    super(theType);
    this.message = '';
    this.rawMessage = '';
    this.gravity = 0;
    this.errorCode = null;
  }
};

/**
 * Event Manager is a global object accessible via {@link SCwebviewer.eventManager},
 * responsible for managing event handlers (subscribing and unsubscribing callbacks).
 */
export class ViewerEventManager extends ViewerEventTarget {
  private _myViewer: SCwebviewer;

  /** Main constructor. */
  constructor(theViewer: SCwebviewer) {
    super();
    this._myViewer = theViewer;
  }

  // callbacks called from C++ level

  /** Emit object dragged event */
  protected onObjectDragged(thePartId: number, theSubPart: number, theAction: number): boolean {
    if (!this._myViewer.interactiveContext) { return false; }

    const anObjId: ObjectId = this._myViewer.interactiveContext.getLastDraggedObject();
    const aDragObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(anObjId);

    let anEvent = new ObjectDragEvent(UserEvents.onObjectDragged)
    anEvent.object = aDragObj;
    anEvent.part.id = thePartId;
    anEvent.part.subId = theSubPart;
    anEvent.action = theAction;
    anEvent.isStopped = theAction === 2; // AIS_DragAction_Stop

    if (aDragObj != null) { aDragObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
    return !anEvent.defaultPrevented;
  }

  /** Emit object click event */
  protected onObjectMouseClick(theObjId: ObjectId, thePart: number, theSubPart: number,
                               theX: number, theY: number, theFlags: number): boolean {
    if (!this._myViewer.interactiveContext) { return false; }

    const anObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theObjId);

    let anEvent = new ObjectMouseClickEvent(UserEvents.onObjectMouseClick);
    anEvent.reset();
    anEvent.object = anObj;
    anEvent.part.id = thePart;
    anEvent.part.subId = theSubPart;
    anEvent.point = { x: theX, y: theY };
    if (theFlags & (1 << 13)) { anEvent.button = 0x1; } // Aspect_VKeyMouse_LeftButton
    if (theFlags & (1 << 15)) { anEvent.button = 0x2; } // Aspect_VKeyMouse_RightButton
    if (theFlags & (1 << 14)) { anEvent.button = 0x4; } // Aspect_VKeyMouse_MiddleButton
    anEvent.ctrlKey = (theFlags & (1 << 9)) !== 0; // Aspect_VKeyFlags_CTRL
    anEvent.shiftKey = (theFlags & (1 << 8)) !== 0; // Aspect_VKeyFlags_SHIFT
    anEvent.altKey = (theFlags & (1 << 10)) !== 0; // Aspect_VKeyFlags_ALT
    anEvent.metaKey = (theFlags & (1 << 12)) !== 0; // Aspect_VKeyFlags_META
    anEvent.isDoubleClick = (theFlags & (1 << 20)) !== 0; // extension

    if (anObj != null) { anObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
    return !anEvent.defaultPrevented;
  }

  /** Emit objects selection change event */
  protected onSelectedObjectsChanged(): boolean {
    if (!this._myViewer.selectionManager) { return false; }

    const aSelList: InteractiveObject[] = this._myViewer.selectionManager.selectedObjects;

    let anEvent = new Event(UserEvents.onSelectedObjectsChanged);
    aSelList.forEach((theObj: InteractiveObject) => {
      theObj.dispatchEvent(anEvent);
    });
    return this.dispatchEvent(anEvent);
  }

  /** Emit object detection change event */
  protected onDetectedObjectChanged(): boolean {
    if (!this._myViewer.selectionManager) { return false; }

    const aDetObj: InteractiveObject | null = this._myViewer.selectionManager.lastDetectedObject;

    let anEvent = new Event(UserEvents.onDetectedObjectChanged);
    if (aDetObj != null) {
      aDetObj.dispatchEvent(anEvent);
    };
    return this.dispatchEvent(anEvent);
  }

  /** Emit subview fullscreen mode change event */
  protected onSubviewFullscreen(theViewId: number, theIsFullscreen: boolean): boolean {
    if (!this._myViewer.viewManager) { return false; }

    const aSubview: Subview | null = this._myViewer.viewManager.findSubviewObject(theViewId);

    let anEvent = new SubviewFullscreenEvent(UserEvents.onSubviewFullscreen);
    anEvent.isFullscreen = theIsFullscreen;
    anEvent.view = aSubview;

    if (anEvent.view != null && aSubview != null) {
      aSubview.dispatchEvent(anEvent);
    }
    return this.dispatchEvent(anEvent);
  }

  /** Emit subview click event */
  protected onSubviewClick(theViewId: number): void {
    if (!this._myViewer.viewManager) { return; }

    const aSubview: Subview | null = this._myViewer.viewManager.findSubviewObject(theViewId);

    let anEvent = new SubviewClickEvent(UserEvents.onSubviewClick);
    anEvent.view = aSubview;

    if (anEvent.view != null && aSubview != null) {
      aSubview.dispatchEvent(anEvent);
    }
    this.dispatchEvent(anEvent);
  }

  /** Emit units change event */
  protected onUnitsChanged(theQuantType: number, theSection: number): void {
    if (!this._myViewer.unitManager) { return; }

    const aQuant: Quantity | null = this._myViewer.unitManager.quantities[theQuantType];

    let anEvent = new UnitsChangedEvent(UserEvents.onUnitsChanged);
    anEvent.quantity = aQuant;
    anEvent.section = theSection;
    this.dispatchEvent(anEvent);
  }

  /** Emit mesh color mapper change event */
  protected onColorMapperChanged(theMeshId: ObjectId, thePropertyId: string, theComponentIndex: number, theColorScaleObjId: ObjectId): void {
    if (!this._myViewer.interactiveContext) { return; }

    const anObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);
    const aColorScaleObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theColorScaleObjId);

    let anEvent = new ColorMapperChangedEvent(UserEvents.onColorMapperChanged);
    anEvent.mesh = anObj as InteractiveCaeMesh;
    anEvent.meshId = theMeshId;
    anEvent.propertyId = thePropertyId;
    anEvent.componentIndex = theComponentIndex;
    anEvent.colorScale = aColorScaleObj as InteractiveColorScale;
    anEvent.changeStatus = anEvent.colorScale == null && anEvent.propertyId !== '' ? ColorScaleChangeStatus.Create
                                                                                   : ColorScaleChangeStatus.Update;

    if (anObj != null) { anObj.dispatchEvent(anEvent); }
    if (aColorScaleObj != null) { aColorScaleObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
  }

  /** Emit mesh animation step change event */
  protected onMeshAnimationChanged(theMeshId: ObjectId, theActiveTimeStep: number, theIsChanged: boolean, theIsRunning: boolean): void {
    if (!this._myViewer.interactiveContext) { return; }

    const anObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);

    let anEvent = new MeshAnimationChangedEvent(UserEvents.onMeshAnimationChanged);
    anEvent.mesh = anObj as InteractiveCaeMesh;
    anEvent.activeTimeStep = theActiveTimeStep;
    anEvent.isChanged = theIsChanged;
    anEvent.isRunning = theIsRunning;

    if (anObj != null) { anObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
  }

  /** Emit mesh properties request event */
  protected onMeshPropertiesRequested(theMeshId: ObjectId, theTimeStep: number, thePropsToLoad: string): void {
    if (!this._myViewer.interactiveContext) { return; }

    const anObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);

    let anEvent = new MeshPropertiesRequestedEvent(UserEvents.onMeshPropertiesRequested);
    anEvent.mesh = anObj as InteractiveCaeMesh;
    anEvent.timeStep = theTimeStep;
    anEvent.propsToLoad = thePropsToLoad === '' ? [] : thePropsToLoad.split('\n');

    if (anObj != null) { anObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
  }

  /** Emit mesh geometry request event */
  protected onMeshGenGeomRequested(theMeshId: ObjectId, thePrsId: string, thePartId: number, theParameters: string): void {
    if (!this._myViewer.interactiveContext) { return; }

    const anObj: InteractiveObject | null = this._myViewer.interactiveContext.findInteractiveObject(theMeshId);

    let anEvent = new MeshGenGeomRequestedEvent(UserEvents.onMeshGenGeomRequested);
    anEvent.mesh = anObj as InteractiveCaeMesh;
    anEvent.prsId = thePrsId;
    anEvent.partId = thePartId;
    anEvent.parameters = theParameters;

    if (anObj != null) { anObj.dispatchEvent(anEvent); }
    this.dispatchEvent(anEvent);
  }

  /** Emit viewer message event. */
  public onViewerMessage(theMessage: string, theGravity: number, theErrorCode: string | null, theRawMessage: string): void {
    let anEvent = new ViewerMessageEvent(UserEvents.onViewerMessage);
    anEvent.message = theMessage ?? '';
    anEvent.rawMessage = theRawMessage ?? '';
    anEvent.gravity = theGravity;
    anEvent.errorCode = theErrorCode;
    this.dispatchEvent(anEvent);
  }
};
