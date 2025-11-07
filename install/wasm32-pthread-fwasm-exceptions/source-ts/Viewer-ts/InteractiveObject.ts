// Created by: Kirill Gavrilov
// Copyright (c) 2022 Huawei Technologies Co., Ltd.

import type {IManager, ColorString, IPromise, IVec2, IVec3, ViewId, ObjectId, IBndBox3, ITrsf} from './CommonTools.js';
import {ColorTools, Vec3, ViewCorner, BndBox3, Trsf} from './CommonTools.js';
import {Subview} from './Subview.js';
import {ViewerEventTarget} from './ViewerEventTarget.js';

import type {SCwebviewer, ShapeObjectSelMode} from './SCwebviewer.js';

import type {UserEvents} from './ViewerEventManager.js';

/**
 * Font aspect defined by `Font_FontAspect` enumeration.
 */
export enum FontAspect { Regular = 0, Bold = 1, Italic = 2, BoldItalic = 3, };

/**
 * Presentation Z-Layer defined by `Graphic3d_ZLayerId` enumeration.
 */
export enum ZLayerId {
  /** identifier for invalid ZLayer */
  UNKNOWN = -1,
  /** underlay for 2D presentations (On-Screen-Display) */
  BotOSD = -5,
  /** default Z-layer for main presentations */
  Default = 0,
  /** overlay for 3D presentations which inherits Depth from previous ZLayer */
  Top = -2,
  /** overlay for 3D presentations with independent Depth */
  Topmost = -3,
  /** overlay for 2D presentations (On-Screen-Display) */
  TopOSD = -4,
};

/**
 * This enumeration defines line types.
 * It reflects values of corresponding C++ `Aspect_TypeOfLine` enumeration defined by OCCT.
 */
export enum TypeOfLine {
  /** Hidden, `Aspect_TOL_EMPTY`. */
  EMPTY = -1,
  /** Continuous, `Aspect_TOL_SOLID`. */
  SOLID = 0,
  /** Dashed, `Aspect_TOL_DASH`. */
  DASH = 1,
  /** Dotted, `Aspect_TOL_DOT`. */
  DOT = 2,
  /** Mixed, `Aspect_TOL_DOTDASH`. */
  DOTDASH = 3,
};

/**
 * This enumeration defines marker types.
 * It reflects values of corresponding C++ `Aspect_TypeOfMarker` enumeration defined by OCCT.
 */
export enum TypeOfMarker {
  EMPTY = -1,
  POINT = 0,
  PLUS = 1, STAR = 2, X = 3, O = 4,
  O_POINT = 5, O_PLUS = 6, O_STAR = 7, O_X = 8,
  RING1 = 9, RING2 = 10, RING3 = 11, BALL = 12,
};

/**
 * @internal Structure to define detected shape element.
 */
export interface IDetectedShapeElem {
  entityId: number;
  entityType: ShapeObjectSelMode;
};

/**
 * @internal Structure to define detected/selected shape elements.
 */
export interface ISelectionShapeElems {
  groups: string;
  compSolids: string;
  solids: string;
  shells: string;
  faces: string;
  wires: string;
  edges: string;
  verts: string;
};

/**
 * MeshEntityType represents type of {@link InteractiveCaeMesh} sub entity.
 */
export enum MeshEntityType {
  Unknown = -1,
  Node = 0,
  Element0d = 1,
  Triangle = 2, Triangle2nd = 3, Quadrangle = 4, Quadrangle2nd = 5, Polygon = 6,
  Tetrahedron = 7, Tetrahedron2nd = 8, Pyramid = 9, Pyramid2nd = 10,
  Prism = 11, Prism2nd = 12, Hexahedron = 13, Hexahedron2nd = 14, Polyhedron = 15,
  LabelGroup = 16
};

/**
 * MeshPropertyUsageType represents mesh property usage type, from `MeshPropertyUsageType`.
 */
export enum MeshPropertyUsageType { ColorMap = 0, Deformation = 1, IsoSurface = 2 };

/**
 * IMeshComponentValueInfo represents information about value of mesh property component,
 * from `MeshComponentValueInfo`.
 */
export interface IMeshComponentValueInfo {
  value: number;
  displayUnits: string;
  description: string;
};

/**
 * MeshComponentValueInfo represents information about value of mesh property component.
 */
export class MeshComponentValueInfo implements IMeshComponentValueInfo {
  value: number = Vec3.invalidDouble();
  displayUnits: string = '';
  description: string = '';
};

/**
 * IMeshPropertyValuesInfo represents information about values of mesh property,
 * from `MeshPropertyValuesInfo`.
 */
export interface IMeshPropertyValuesInfo {
  description: string;
  components: Array<IMeshComponentValueInfo>;
};

/**
 * MeshPropertyValuesInfo represents information about values of mesh property.
 */
export class MeshPropertyValuesInfo implements IMeshPropertyValuesInfo {
  description: string = '';
  components: Array<IMeshComponentValueInfo> = new Array<IMeshComponentValueInfo>();
};

/**
 * IMeshPropertyInfo represents mesh property component info,
 * from `MeshPropertyInfo`.
 */
export interface IMeshPropertyInfo {
  propertyId: string;
  componentIndex: number;
};

/**
 * IMeshPropertyInfo represents mesh property component info.
 */
export class MeshPropertyInfo implements IMeshPropertyInfo {
  propertyId: string = '';
  componentIndex: number = -1;
};

/**
 * ISelectionMeshEntity represents detected/selected info of {@link InteractiveCaeMesh} entity,
 * from `SelectionMeshEntity`.
 */
export interface ISelectionMeshEntity {
  entityId: number;
  entityType: MeshEntityType;
  isElemSurf: boolean;

  prsId: string;
  geomId: string;
  prsInfo: string;

  pos: IVec3;

  refMeshData: ArrayBuffer | null;
  refMeshDataPath: string;
  hasRefEntity: boolean;

  lineIndex: number;
  propsInfo: Map<string, IMeshPropertyValuesInfo>;
  prsPropsInfo: Map<number, IMeshPropertyInfo>;
};

/**
 * SelectionMeshEntity represents detected/selected info of {@link InteractiveCaeMesh} entity.
 */
export class SelectionMeshEntity implements ISelectionMeshEntity {
  entityId: number = -1;
  entityType: MeshEntityType = MeshEntityType.Unknown;
  isElemSurf: boolean = false;

  prsId: string = '';
  geomId: string = '';
  prsInfo: string = '';

  pos: IVec3 = Vec3.invalid();

  refMeshData: ArrayBuffer | null = null;
  refMeshDataPath: string;
  hasRefEntity: boolean = false;

  lineIndex: number = -1;
  propsInfo: Map<string, IMeshPropertyValuesInfo> = new Map<string, IMeshPropertyValuesInfo>();
  prsPropsInfo: Map<number, IMeshPropertyInfo> = new Map<number, IMeshPropertyInfo>();
};

/**
 * IDetectedObjectInfo represents info about detected object, from `DetectedObjectInfo`.
 */
export interface IDetectedObjectInfo {
  objectId: ObjectId;
  partId: number;
  subPartId: number;
  pickedPoint: IVec3;
  pickedNormal: IVec3;
};

/**
 * Structure to define object's part/subpart.
 */
export class ObjectPart {
  id: number = -1;
  subId: number = -1;
};

/**
 * Structure to define detected object + its subparts.
 */
export class DetectedObjectInfo {
  object: InteractiveObject | null = null;
  part: ObjectPart = new ObjectPart();
  pickedPoint: IVec3 = Vec3.invalid();
  pickedNormal: IVec3 = Vec3.invalid();
};

/**
 * Auxiliary class for accessing basic InteractiveObject aspects.
 */
export class PresentationAspects {
  /** Back-link to the object. */
  private myPrs: InteractiveObject;

  /** Constructor. */
  constructor(thePrs: InteractiveObject) {
    this.myPrs = thePrs;
  }

  /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
  public get transparency(): number { return this.myPrs.aspectsMgr?.getTransparency(this.myPrs.id) ?? 0.0; }

  /**
   * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
   * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
   */
  public get baseColor(): ColorString { return this.myPrs.aspectsMgr?.getBaseColor(this.myPrs.id) ?? ''; }

  /**
   * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
   * Note that if object has undefined color, but defined transparency - this property will return empty string!
   */
  public get baseColorRgba(): ColorString {
    let aColor = this.myPrs.aspectsMgr?.getBaseColor(this.myPrs.id) ?? '';
    if (aColor === '') { return ''; }
    let aTransp = this.myPrs.aspectsMgr?.getTransparency(this.myPrs.id) ?? 0.0;
    let anAlpha = Math.floor(255.0 * (1.0 - aTransp)).toString(16).toUpperCase();
    if (anAlpha.length === 1) { anAlpha = `0${anAlpha}`; }
    if (aColor.length === 9) { aColor = aColor.substring(0, 7); }
    return aColor + anAlpha;
  }

  /** Return object's font height. */
  public get textHeight(): number { return this.myPrs.aspectsMgr?.getTextHeight(this.myPrs.id) ?? 0.0; }

  /** Return object's font name. */
  public get textFont(): string { return this.myPrs.aspectsMgr?.getTextFont(this.myPrs.id) ?? ''; }

  /** Return object's font aspect. */
  public get textFontAspect(): FontAspect { return this.myPrs.aspectsMgr?.getTextFontAspect(this.myPrs.id) ?? FontAspect.Regular; }

  /** Return object's text alignment. */
  public get textJustification(): ViewCorner { return this.myPrs.aspectsMgr?.getTextJustification(this.myPrs.id) ?? ViewCorner.LEFT_LOWER; }

  /** Return object's line thickness. */
  public get lineWidth(): number { return this.myPrs.aspectsMgr?.getLineWidth(this.myPrs.id) ?? 1.0; }

  /** Return object's line type. */
  public get lineType(): TypeOfLine { return this.myPrs.aspectsMgr?.getLineType(this.myPrs.id) ?? TypeOfLine.SOLID; }

  /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
  public set transparency(theTransp: number) { this.myPrs.aspectsMgr?.setTransparency(this.myPrs.id, theTransp != null ? theTransp : 0.0); }

  /** Set object's global color or empty string to unset. */
  public set baseColor(theColor: ColorString) { this.myPrs.aspectsMgr?.setBaseColor(this.myPrs.id, theColor != null ? theColor : ''); }

  /** Set object's global color + transparency or empty string to unset both. */
  public set baseColorRgba(theColor: ColorString) {
    let aTransp = 0.0;
    if (theColor != null && theColor.length === 9) {
      let anRgba = ColorTools.convertColorStringToRgba(theColor);
      aTransp = 1.0 - (anRgba.a / 255.0);
    }
    this.myPrs.aspectsMgr?.setBaseColor(this.myPrs.id, theColor != null ? theColor : '');
    this.myPrs.aspectsMgr?.setTransparency(this.myPrs.id, aTransp);
  }

  /** Set object's font height. */
  public set textHeight(theHeight: number) { this.myPrs.aspectsMgr?.setTextHeight(this.myPrs.id, theHeight); }

  /** Set object's font name. */
  public set textFont(theFont: string) { this.myPrs.aspectsMgr?.setTextFont(this.myPrs.id, theFont); }

  /** Set object's font aspect. */
  public set textFontAspect(theAsp: FontAspect) { this.myPrs.aspectsMgr?.setTextFontAspect(this.myPrs.id, theAsp); }

  /** Set object's text alignment. */
  public set textJustification(theJust: ViewCorner) { this.myPrs.aspectsMgr?.setTextJustification(this.myPrs.id, theJust); }

  /** Set object's line width. */
  public set lineWidth(theWidth: number) { this.myPrs.aspectsMgr?.setLineWidth(this.myPrs.id, theWidth); }

  /** Set object's line type. */
  public set lineType(theType: TypeOfLine) { this.myPrs.aspectsMgr?.setLineType(this.myPrs.id, theType); }
};

/**
 * Interactive Object to be displayed in the Viewer.
 * This class stores a weak reference to the real object in memory identified by a string id {@link ObjectId}.
 * User might specify id explicitly at construction time or rely on an id generated from loaded file URL.
 * Beware to use unique ids - creating multiple objects with the same id will result in logical issues!
 *
 * Usual lifetime of the object:
 * - Create an empty instance of subclass (object is not bound to Viewer);
 * - Construct or load object from the given URL (object is registered in the Viewer);
 * - Show/hide/modify object in the Viewer (e.g. {@link InteractiveObject.isVisible});
 * - Remove object from the Viewer (object is unregistered from the Viewer and memory released) via {@link InteractiveObject.remove};
 *   alternatively, {@link InteractiveContext.removeAll} interface could be used to remove all registered objects,
 *   in which case {@link InteractiveObject} instances will keep dead references ({@link InteractiveObject.isAlive}).
 *
 * Application may subscribe to the following object's events:
 * - {@link UserEvents.onObjectDragged}, when object is dragged;
 * - {@link UserEvents.onObjectMouseClick}, when mouse is clicked over object;
 * - {@link UserEvents.onDetectedObjectChanged}, when object become detected (dynamically highlighted);
 * - {@link UserEvents.onSelectedObjectsChanged}, when object become selected;
 */
export class InteractiveObject extends ViewerEventTarget {
  /** @internal */
  protected _myObjId: ObjectId;
  /** @internal */
  protected _myObjOldId: ObjectId;
  /** @internal */
  protected _myViewer: SCwebviewer | null;
  /** @internal */
  protected _myAspects: PresentationAspects | null;

  /** Empty constructor. */
  constructor(theObjId: ObjectId = '') {
    super();
    this._myObjId = theObjId;
    this._myObjOldId = '';
    this._myViewer = null;
    this._myAspects = null;
  }

  /** @internal Shortcut to aspects manager. */
  public get aspectsMgr(): InteractiveAspectsManager | null { return this._myViewer?.interactiveAspectsManager ?? null; }

  /** Get object id as {@link ObjectId}. */
  public get id(): ObjectId { return this._myObjId; }

  /** Get object id before removal. */
  public get oldId(): ObjectId { return this._myObjOldId; }

  /** Return TRUE if object with specified Id exists. */
  public get isAlive(): boolean { return this.objMgr?.isAlive(this._myObjId) ?? false; }

  /** Return object type. */
  public get type(): string { return this.objMgr?.getType(this._myObjId) ?? ''; }

  /** Return TRUE if object with specified Id has deferred data loaded. */
  public get isLoaded(): boolean { return this.objMgr?.isLoaded(this._myObjId) ?? false; }

  /** Return object enabled state. */
  public get isEnabled(): boolean { return this.objMgr?.isEnabled(this._myObjId) ?? false; }

  /** Return object visibility state. */
  public get isVisible(): boolean { return this.objMgr?.isVisible(this._myObjId) ?? false; }

  /** Return view in which object is visible or NULL if it is not displayed or displayed in multiple views. */
  public get visibleInView(): Subview | null {
    if (!this._myViewer || !this._myViewer.viewManager) { return null; }
    let aViewId = this.objMgr?.getVisibleInView(this._myObjId) ?? -1;
    if (aViewId === -2 || aViewId === -1) { return null; }

    let aView = this._myViewer.viewManager.findSubviewObject(aViewId);
    if (aView != null) { return aView; }

    let aViewWrap = new Subview();
    aViewWrap.wrap(this._myViewer, aViewId);
    return aViewWrap;
  }

  /** Return TRUE if this object is listed within selected objects. */
  public get isSelected(): boolean {
    if (this._myObjId === '' || !this._myViewer || !this._myViewer.selectionManager) { return false; }
    let aSelObjs = this._myViewer.selectionManager.selectedObjects;
    for (const aSelIter of aSelObjs) {
      if (aSelIter.id === this._myObjId) { return true; }
    }
    return false;
  }

  /** Return if this presentation is defined in 2D or 3D space. */
  public get is2d(): boolean { return this.viewCorner !== ViewCorner.FREE3D; }

  /** Return view corner of 2D presentation; has no meaning for 3D objects. */
  public get viewCorner(): ViewCorner { return this.objMgr?.getViewCorner(this._myObjId) ?? ViewCorner.FREE3D; }

  /** Return view corner offset of 2D presentation; has no meaning for 3D objects. */
  public get viewCornerOffset(): IVec2 { return this.objMgr?.getViewCornerOffset(this._myObjId) ?? {x: 0, y: 0}; }

  /** Return local transformation. */
  public get localTransformation(): ITrsf { return this.objMgr?.getLocalTransformation(this._myObjId) ?? new Trsf(); }

  /** Return object AABB (axes-aligned bounding box). */
  public get minMax(): IBndBox3 { return this.objMgr?.getMinMax(this._myObjId) ?? BndBox3.voidBox(); }

  /** Return object's Z-Layer. */
  public get zlayer(): ZLayerId { return this.aspectsMgr?.getZLayer(this._myObjId) ?? ZLayerId.UNKNOWN; }

  /** Return a value that is added to the default displaying/selection object's priority, 0 by default. */
  public get priorityModifier(): number { return this.objMgr?.getPriorityModifier(this._myObjId) ?? 0; }

  /**
   * Infinite presentation flag, FALSE by default.
   * Used to distinguish auxiliary presentations (construction elements, manipulators) from main scene objects.
   * Such objects are not considered within FitAll operation.
   */
  public get isInfiniteState(): boolean { return this.objMgr?.isInfiniteState(this._myObjId) ?? false; }

  /** Return aspects manager. */
  public get aspects(): PresentationAspects {
    if (this._myAspects == null) { this._myAspects = new PresentationAspects(this); }
    return this._myAspects;
  }

  /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
  public get transparency(): number { return this.aspectsMgr?.getTransparency(this._myObjId) ?? 0.0; }

  /**
   * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
   * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
   */
  public get baseColor(): ColorString { return this.aspectsMgr?.getBaseColor(this._myObjId) ?? ''; }

  /**
   * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
   * Note that if object has undefined color, but defined transparency - this property will return empty string!
   */
  public get baseColorRgba(): ColorString { return this.aspects.baseColorRgba; }

  /** @internal Shortcut to interactive context. */
  protected get objMgr(): InteractiveContext | null { return this._myViewer?.interactiveContext ?? null; }

  /** @internal Shortcut tii selection manager. */
  protected get selectionMgr(): SelectionManager | null { return this._myViewer?.selectionManager ?? null; }

  /** @internal remove object from scene. */
  private get doRemove(): boolean { const aRes = this.remove(); this._myViewer?.requestViewUpdate(); return aRes; }

  /** @internal hide object in scene. */
  private get doHide(): boolean { this.isVisible = false; this._myViewer?.requestViewUpdate(); return this.isVisible; }

  /** @internal show object in scene. */
  private get doShow(): boolean { this.isVisible = true; this._myViewer?.requestViewUpdate(); return this.isVisible; }

  /** @internal show only this object in scene. */
  private get doShowOnly(): boolean {
    const is2dObj: boolean = this.is2d;
    for (let anObjIter of this._myViewer?.interactiveContext?.objectsInside ?? []) {
      if (anObjIter.is2d === is2dObj && anObjIter.id !== this._myObjId) {
        anObjIter.isVisible = false;
      }
    }
    this.isVisible = true;
    this._myViewer?.requestViewUpdate();
    return this.isVisible;
  }

  /** @internal select this object in scene. */
  private get doSelect(): boolean {
    this.isSelected = true;
    this._myViewer?.requestViewUpdate();
    return this.isSelected;
  }

  /** @internal select only this object in scene. */
  private get doSelectOnly(): boolean {
    if (!this._myViewer || !this._myViewer.selectionManager) { return false; }
    this._myViewer.selectionManager.oneSelectedObject = this;
    this._myViewer?.requestViewUpdate();
    return true;
  }

  /** Set object enabled state. */
  public set isEnabled(theToEnable: boolean) { this.objMgr?.setEnabled(this._myObjId, theToEnable); }

  /** Set object visibility state. */
  public set isVisible(theToShow: boolean) { this.objMgr?.setVisible(this._myObjId, theToShow); }

  /**
   * Set object to be visible only within specific view or all views (NULL) - view affinity.
   * Note that {@link isVisible} still has to be set independently.
   */
  public set visibleInView(theView: Subview | null) {
    this.objMgr?.setVisibleInView(this._myObjId, theView == null ? -2 : theView.id);
  }

  /** Put/remove object from selected objects. */
  public set isSelected(theToSelect: boolean) {
    if (this._myObjId === '' || !this._myViewer || !this._myViewer.selectionManager) { return; }
    let aSelObjs: InteractiveObject[] = this._myViewer.selectionManager.selectedObjects;
    let aNewSelObjs: InteractiveObject[] = [];
    for (const aSelIter of aSelObjs) {
      if (aSelIter.id === this._myObjId) {
        if (theToSelect) {
          return;
        } else {
          continue;
        }
      }
      aNewSelObjs.push(aSelIter);
    }
    if (theToSelect) {
      aNewSelObjs.push(this);
    }
    this._myViewer.selectionManager.selectedObjects = aNewSelObjs;
  }

  /** Set if this presentation is defined in 2D or 3D space. */
  public set is2d(theIs2d: boolean) {
    if (!theIs2d) { this.viewCorner = ViewCorner.FREE3D; }
    else if (!this.is2d) { this.viewCorner = ViewCorner.LEFT_UPPER; }
    else {}
  }

  /** Set view corner. */
  public set viewCorner(theCorner: ViewCorner) { this.objMgr?.setViewCorner(this._myObjId, theCorner); }

  /** Set view corner offset. */
  public set viewCornerOffset(theOffset: IVec2) { this.objMgr?.setViewCornerOffset(this._myObjId, theOffset); }

  /** Set local transformation. */
  public set localTransformation(theLoc: ITrsf) { this.objMgr?.setLocalTransformation(this._myObjId, theLoc); }

  /** Set object's Z-Layer. */
  public set zlayer(theLayer: ZLayerId) { this.aspectsMgr?.setZLayer(this._myObjId, theLayer != null ? theLayer : ZLayerId.Default); }

  /** Set a value that is added to the default displaying/selection object's priority. */
  public set priorityModifier(theModifier: number) { this.objMgr?.setPriorityModifier(this._myObjId, theModifier); }

  /** Set infinite presentation flag. */
  public set isInfiniteState(theState: boolean) { this.objMgr?.setInfiniteState(this._myObjId, theState); }

  /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
  public set transparency(theTransp: number) { this.aspectsMgr?.setTransparency(this._myObjId, theTransp != null ? theTransp : 0.0); }

  /** Set object's global color or empty string to unset. */
  public set baseColor(theColor: ColorString) { this.aspectsMgr?.setBaseColor(this._myObjId, theColor != null ? theColor : ''); }

  /** Set object's global color + transparency or empty string to unset both. */
  public set baseColorRgba(theColor: ColorString) { this.aspects.baseColorRgba = theColor; }

  /** Remove object and invalidates it's id. */
  public remove(): boolean {
    if (this._myObjId === '') { return false; }
    const aRes = this.objMgr?.remove(this._myObjId) ?? false;
    this._myObjOldId = this._myObjId;
    this._myObjId = '';
    return aRes;
  }

  /** @internal invalidate object id without removing from context. */
  public invalidate(): void {
    if (this._myObjId === '') { return; }
    this._myObjOldId = this._myObjId;
    this._myObjId = '';
  }

  /**
   * Wrap existing object.
   * @param {SCwebviewer} theViewer viewer module
   * @param {string} theId the id to wrap
   * @return {boolean} FALSE if object with specified Id doesn't exist in the Viewer
   */
  public wrap(theViewer: SCwebviewer, theId: string = ''): boolean {
    if (theId !== '') { this._myObjId = theId; }
    this._myViewer = theViewer;
    return this.objMgr?.isAlive(this._myObjId) ?? false;
  }

  /** Return file path from object id. */
  public getFilePathFromObjId(): string {
    return (this._myObjId.length > 0 && this._myObjId[0] === '/') ?
            this._myObjId :
            `/${this._myObjId}`;
  }

  /**
   * Load deferred data async.
   * @param theViewer module object
   * @return {Promise<boolean>} FALSE if model cannot be opened
   */
  protected loadDataAsync(theViewer: SCwebviewer): Promise<boolean> {
    return new Promise((theResolve, theReject) => {
      const aCallable = {
        _myObj: undefined,
        resolve: function (): void {
          theResolve(true);
          this._myObj.delete();
        },
        reject: function (theErr: string): void {
          theReject(new Error(theErr));
          this._myObj.delete();
        },
      };
      const aWasmPromise = theViewer.IPromise.implement(aCallable);
      aCallable._myObj = aWasmPromise as any;
      this.objMgr?.loadData(this._myObjId, aWasmPromise);
    });
  }
};

/**
 * Represents Context for Interactive Objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveObject} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveContext} property.
 */
export interface InteractiveContext extends IManager {

// JS extensions

  /** Return the array of created objects */
  readonly objectsInside: InteractiveObject[];

  /** @internal Add interactive object */
  addInteractiveObject: (theObj: InteractiveObject) => void;

  /** @internal find existing interactive object */
  findInteractiveObject: (theObjId: ObjectId) => InteractiveObject | null;

  /** @internal Remove existing interactive object (from objects array only) */
  removeInteractiveObject: (theObjId: ObjectId) => boolean;

// C++ methods

  /** @internal Return TRUE if object with specified {@link ObjectId} exists. */
  isAlive: (theObjId: ObjectId) => boolean;

  /** @internal Return object type. */
  getType: (theObjId: ObjectId) => string;

  /** @internal Return TRUE if object with specified {@link ObjectId} has deferred geometry data loaded. */
  isLoaded: (theObjId: ObjectId) => boolean;

  /** @internal Return object {@link ObjectId} enabled state. */
  isEnabled: (theObjId: ObjectId) => boolean;

  /** @internal Set object {@link ObjectId} enabled state. */
  setEnabled: (theObjId: ObjectId, theToShow: boolean) => void;

  /** @internal Return object {@link ObjectId} visibility state. */
  isVisible: (theObjId: ObjectId) => boolean;

  /** @internal Set object {@link ObjectId} visibility state. */
  setVisible: (theObjId: ObjectId, theToShow: boolean) => void;

  /** @internal Return view id where object is set to be visible or -2 if visibility state is undefined or active in multiple views. */
  getVisibleInView: (theObjId: ObjectId) => ViewId;

  /** @internal Set object to be visible only within specific view or within all views (NULL). */
  setVisibleInView: (theObjId: ObjectId, theViewId: ViewId) => void;

  /** @internal Return view corner of 2D presentation; has no meaning for 3D objects. */
  getViewCorner: (theObjId: ObjectId) => ViewCorner;

  /** @internal Set view corner. */
  setViewCorner: (theObjId: ObjectId, theCorner: ViewCorner) => void;

  /** @internal Return view corner offset of 2D presentation; has no meaning for 3D objects. */
  getViewCornerOffset: (theObjId: ObjectId) => IVec2;

  /** @internal Set view corner offset. */
  setViewCornerOffset: (theObjId: ObjectId, theOffset: IVec2) => void;

  /** @internal Return local transformation. */
  getLocalTransformation: (theObjId: ObjectId) => ITrsf;

  /** @internal Set local transformation. */
  setLocalTransformation: (theObjId: ObjectId, theLoc: ITrsf) => void;

  /** @internal Return object AABB. */
  getMinMax: (theObjId: ObjectId) => IBndBox3;

  /** @internal Return a value that is added to the default displaying/selection object's priority. */
  getPriorityModifier: (theObjId: ObjectId) => number;

  /** @internal Set a value that is added to the default displaying/selection object's priority. */
  setPriorityModifier: (theObjId: ObjectId, theModifier: number) => void;

  /** @internal Return infinite state. */
  isInfiniteState: (theObjId: ObjectId) => boolean;

  /** @internal Set infinite state. */
  setInfiniteState: (theObjId: ObjectId, theState: boolean) => void;

  /** @internal Remove object {@link ObjectId}. */
  remove: (theObjId: ObjectId) => boolean;

  /**
   * Remove all named objects.
   * Warning! Using this method would make existing InteractiveObject instances
   * to have dead references (see also {@link InteractiveObject.isAlive}).
   */
  removeAll: () => boolean;

  /**
   * @internal Make a copy of existing object; the object will be created in erased state.
   * @param theObjIdOld id of existing object
   * @param theObjIdNew id of object to create or "" if id should be generated
   * @return id of created object or "" on failure
   */
  clone: (theObjIdOld: ObjectId, theObjIdNew: ObjectId) => ObjectId;

  /** @internal Load deferred data. */
  loadData: (theObjId: ObjectId, theCallback: IPromise) => boolean;

  /** @internal Generate unique id of not yet existing object. */
  generateId: (theBaseObjId: ObjectId) => ObjectId;

  /**
   * @internal Return ids of all named objects.
   * @param theIdPrefix optional id prefix that should present in returned named object
   */
  getObjectsInside: (theIdPrefix: string) => string;

  /**
   * Return id of last dragged object or empty string.
   */
  getLastDraggedObject: () => string;
};

/**
 * Auxiliary tool to initialize JS extensions to InteractiveContext C++ class.
 * @internal
 */
export class InteractiveContextExtensions {
  public static init(theViewer: SCwebviewer, theCtx: InteractiveContext): void {
    let aPrsObjArray: InteractiveObject[] = [];

    Object.defineProperty(theCtx, 'objectsInside', {
      get: function (): InteractiveObject[] { return aPrsObjArray; },
    });

    theCtx.addInteractiveObject = function (theObj: InteractiveObject): void {
      let aDuplicates: InteractiveObject[] = [];
      aPrsObjArray = aPrsObjArray.filter((theElem, theIndex, theArray) => {
        if (theElem.id === theObj.id && theElem !== theObj) { aDuplicates.push(theElem); }
        return (theElem.id !== theObj.id);
      });
      aDuplicates.forEach((theElem: InteractiveObject) => {
        console.warn(`Removing duplicated interactive object '${theElem.id}'`);
        // @c theElem.remove(); // will remove new object
        theElem.invalidate();
      });
      aPrsObjArray.push(theObj);
    };

    theCtx.findInteractiveObject = function (theObjId: ObjectId): InteractiveObject | null {
      let anObj = aPrsObjArray.find((theElem, theIndex, theArray) => { return (theElem.id === theObjId); });
      return anObj ? anObj : null;
    };

    theCtx.removeInteractiveObject = function (theObjId: ObjectId): boolean {
      if (theCtx.findInteractiveObject(theObjId) == null) { return false; }
      aPrsObjArray = aPrsObjArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theObjId); });
      return true;
    };

    let aCppRemoveObject = theCtx.remove.bind(theCtx);
    theCtx.remove = function (theObjId: ObjectId): boolean {
      theCtx.removeInteractiveObject(theObjId);
      return aCppRemoveObject(theObjId);
    };

    let aCppRemoveAllObjects = theCtx.removeAll.bind(theCtx);
    theCtx.removeAll = function (): boolean {
      let anObjsToRemove: InteractiveObject[] = aPrsObjArray.slice();
      anObjsToRemove.reverse();
      aPrsObjArray = [];
      anObjsToRemove.forEach((theObj: InteractiveObject) => {
        theObj.remove(); // invalidate objects
      });
      return aCppRemoveAllObjects() || anObjsToRemove.length !== 0;
    };
  }
};

/**
 * @internal Represents manager for presentation aspects of Interactive Objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveObject} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveAspectsManager} property.
 */
export interface InteractiveAspectsManager extends IManager {
// C++ methods

  /** @internal Return object's {@link ObjectId} Z-layer. */
  getZLayer: (theObjId: ObjectId) => number;

  /** @internal Set object's {@link ObjectId} Z-layer. */
  setZLayer: (theObjId: ObjectId, theLayer: number) => void;

  /** @internal Return object's {@link ObjectId} transparency level within [0.0, 1.0] range. */
  getTransparency: (theObjId: ObjectId) => number;

  /** @internal Set object's {@link ObjectId} transparency level within [0.0, 1.0] range. */
  setTransparency: (theObjId: ObjectId, theTransp: number) => void;

  /** @internal Return object's {@link ObjectId} color or empty string if unspecified. */
  getBaseColor: (theObjId: ObjectId) => ColorString;

  /** @internal Set object's {@link ObjectId} color or empty string to unset. */
  setBaseColor: (theObjId: ObjectId, theColor: ColorString) => void;

  /** @internal Return object's font height. */
  getTextHeight: (theObjId: ObjectId) => number;

  /** @internal Set object's font height. */
  setTextHeight: (theObjId: ObjectId, theHeight: number) => void;

  /** @internal Return object's font name. */
  getTextFont: (theObjId: ObjectId) => string;

  /** @internal Set object's font name. */
  setTextFont: (theObjId: ObjectId, theFont: string) => void;

  /** @internal Return object's font aspect. */
  getTextFontAspect: (theObjId: ObjectId) => number;

  /** @internal Set object's font aspect. */
  setTextFontAspect: (theObjId: ObjectId, theAspect: number) => void;

  /** @internal Return object's text alignment. */
  getTextJustification: (theObjId: ObjectId) => number;

  /** @internal Set object's text alignment. */
  setTextJustification: (theObjId: ObjectId, theHeight: number) => void;

  /** @internal Return object's {@link ObjectId} line width. */
  getLineWidth: (theObjId: ObjectId) => number;

  /** @internal Set object's {@link ObjectId} line width. */
  setLineWidth: (theObjId: ObjectId, theWidth: number) => void;

  /** @internal Return object's {@link ObjectId} line type. */
  getLineType: (theObjId: ObjectId) => number;

  /** @internal Set object's {@link ObjectId} line type. */
  setLineType: (theObjId: ObjectId, theType: number) => void;
};

/**
 * Represents selection manager for Interactive Objects.
 * See also {@link SCwebviewer.selectionManager} property.
 */
export interface SelectionManager extends IManager {
// JS extensions

  /** Return/set the array of selection objects */
  selectedObjects: InteractiveObject[];

  /** Return/set a single selected object. Return null if multiple/none objects selected */
  oneSelectedObject: InteractiveObject | null;

  /** Return the last detected object */
  readonly lastDetectedObject: InteractiveObject | null;

  /** Return the last detected object info */
  readonly lastDetectedInfo: DetectedObjectInfo;

// C++ methods

  /** Return/set selection mode multiple-true, single-false */
  toAllowMultiselection: boolean;

  /** @internal Return list of selected objects' ids */
  getSelectedObjects: () => string;

  /** @internal Set list of selected object */
  setSelectedObjects: (theObjIds: string, theToLevelUp: boolean) => void;

  /** @internal Return last picked point or an infinite point if nothing was picked */
  getLastPickedPoint: (theObjId: string) => IVec3;

  /** @internal Return object's selection mode */
  getSelectionMode: (theObjId: string) => number;

  /** @internal Set selection mode for list of objects */
  setSelectionMode: (theObjIds: string, theMode: number) => void;

  /** @internal Return the last detected object */
  getLastDetectedObject: () => IDetectedObjectInfo;

  /** @internal Return the detected shape element for specified shape object. */
  getDetectedShapeElem: (theObjId: string) => IDetectedShapeElem;

  /** @internal Return list of selected shape element for specified shape object and subshape type ShapeObjectSelMode */
  getSelectedShapeElems: (theObjId: string, theElemType: number) => string;

  /** @internal Set list of selected shape element for specified shape object */
  setSelectedShapeElems: (theObjId: string, theElems: ISelectionShapeElems) => void;

  /** @internal Return list of disabled shape element for specified shape object and subshape type ShapeObjectSelMode */
  getDisabledShapeElems: (theObjId: string, theElemType: number, theToReverse: boolean) => string;

  /** @internal Set list of disabled shape element for specified shape object and subshape type ShapeObjectSelMode */
  setDisabledShapeElems: (theObjId: string, theElemType: number, theToReverse: boolean, theIndexList: string) => void;

  /** @internal Return indexes of subshapes of specified type. */
  getSubshapesIndexes: (theObjId: ObjectId, theElemType: number, theIsFree: boolean,
                        theParentType: number, theParentIndex: number, theVisibility: number) => string;

  /** @internal Return the detected sub entity for specified mesh object*/
  getDetectedMeshEntity: (theObjId: string) => ISelectionMeshEntity;

  /** @internal Return list of selected sub entities for specified mesh object*/
  getSelectedMeshEntities: (theObjId: string) => SCwebviewer['SelectionMeshEntityVector'];

  /** @internal Select sub entity of specified mesh object. */
  setSelectedMeshEntity: (theObjId: string, theEntity: ISelectionMeshEntity) => void;

  /** Unselect all previously selected objects*/
  resetSelection: () => void;
};

/**
 * Auxiliary tool to initialize JS extensions to SelectionManager C++ class.
 * @internal
 */
export class SelectionManagerExtensions {
  public static init(theViewer: SCwebviewer, theSelMgr: SelectionManager): void {
    Object.defineProperty(theSelMgr, 'selectedObjects', {
      get: function (): InteractiveObject[] {
        let anObjArray: InteractiveObject[] = [];
        if (!theViewer.interactiveContext) { return anObjArray; }

        const aSelObjIds = theSelMgr.getSelectedObjects();
        const aSelObjIdsArr = aSelObjIds.length !== 0 ? aSelObjIds.split('\n') : [];
        for (const aSelIdIter of aSelObjIdsArr) {
          const aPrsObj = theViewer.interactiveContext.findInteractiveObject(aSelIdIter) ?? null;
          // @c const aPrsObj = aPrsWrapper (aSelIdIter);
          if (aPrsObj != null) { anObjArray.push(aPrsObj); }
        }
        return anObjArray;
      },
      set: function(theObjs: InteractiveObject[]) {
        if (!theViewer.interactiveContext) { return; }

        let anObjIds: string[] = [];
        for (const anObj of theObjs) {
          if (anObj != null) {
            anObjIds.push(anObj.id);
          }
        }
        theSelMgr.setSelectedObjects(anObjIds.join('\n'), false);
      },
    });

    Object.defineProperty(theSelMgr, 'oneSelectedObject', {
      get: function (): InteractiveObject | null {
        const aSelObjIds = theSelMgr.getSelectedObjects();
        const aSelObjIdsArr = aSelObjIds.length !== 0 ? aSelObjIds.split('\n') : [];
        if (aSelObjIdsArr.length !== 1) { return null; }
        return theViewer.interactiveContext?.findInteractiveObject(aSelObjIdsArr[0]) ?? null;
      },
      set: function(theObj : InteractiveObject | null) {
        theSelMgr?.setSelectedObjects(theObj != null ? theObj.id : '', false);
      },
    });

    Object.defineProperty(theSelMgr, 'lastDetectedObject', {
      get: function (): InteractiveObject | null {
        const aDetObj = theSelMgr.getLastDetectedObject();
        return theViewer.interactiveContext?.findInteractiveObject(aDetObj.objectId) ?? null;
      },
    });

    Object.defineProperty(theSelMgr, 'lastDetectedInfo', {
      get: function (): DetectedObjectInfo {
        let aRes = new DetectedObjectInfo();
        const aDetObj = theSelMgr.getLastDetectedObject();
        if (theViewer.interactiveContext != null) {
          aRes.object = theViewer.interactiveContext.findInteractiveObject(aDetObj.objectId);
          aRes.part.id = aDetObj.partId;
          aRes.part.subId = aDetObj.subPartId;
          aRes.pickedPoint = aDetObj.pickedPoint;
          aRes.pickedNormal = aDetObj.pickedNormal;
        }
        return aRes;
      },
    });
  }
};
