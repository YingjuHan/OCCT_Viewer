import type { IManager, ColorString, IVec2, IVec3, ObjectId, IBndBox3, ITrsf } from './CommonTools.js';
import { ViewCorner } from './CommonTools.js';
import { Subview } from './Subview.js';
import { ViewerEventTarget } from './ViewerEventTarget.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Font aspect defined by `Font_FontAspect` enumeration.
 */
export declare enum FontAspect {
    Regular = 0,
    Bold = 1,
    Italic = 2,
    BoldItalic = 3
}
/**
 * Presentation Z-Layer defined by `Graphic3d_ZLayerId` enumeration.
 */
export declare enum ZLayerId {
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
    TopOSD = -4
}
/**
 * This enumeration defines line types.
 * It reflects values of corresponding C++ `Aspect_TypeOfLine` enumeration defined by OCCT.
 */
export declare enum TypeOfLine {
    /** Hidden, `Aspect_TOL_EMPTY`. */
    EMPTY = -1,
    /** Continuous, `Aspect_TOL_SOLID`. */
    SOLID = 0,
    /** Dashed, `Aspect_TOL_DASH`. */
    DASH = 1,
    /** Dotted, `Aspect_TOL_DOT`. */
    DOT = 2,
    /** Mixed, `Aspect_TOL_DOTDASH`. */
    DOTDASH = 3
}
/**
 * This enumeration defines marker types.
 * It reflects values of corresponding C++ `Aspect_TypeOfMarker` enumeration defined by OCCT.
 */
export declare enum TypeOfMarker {
    EMPTY = -1,
    POINT = 0,
    PLUS = 1,
    STAR = 2,
    X = 3,
    O = 4,
    O_POINT = 5,
    O_PLUS = 6,
    O_STAR = 7,
    O_X = 8,
    RING1 = 9,
    RING2 = 10,
    RING3 = 11,
    BALL = 12
}
/**
 * MeshEntityType represents type of {@link InteractiveCaeMesh} sub entity.
 */
export declare enum MeshEntityType {
    Unknown = -1,
    Node = 0,
    Element0d = 1,
    Triangle = 2,
    Triangle2nd = 3,
    Quadrangle = 4,
    Quadrangle2nd = 5,
    Polygon = 6,
    Tetrahedron = 7,
    Tetrahedron2nd = 8,
    Pyramid = 9,
    Pyramid2nd = 10,
    Prism = 11,
    Prism2nd = 12,
    Hexahedron = 13,
    Hexahedron2nd = 14,
    Polyhedron = 15,
    LabelGroup = 16
}
/**
 * MeshPropertyUsageType represents mesh property usage type, from `MeshPropertyUsageType`.
 */
export declare enum MeshPropertyUsageType {
    ColorMap = 0,
    Deformation = 1,
    IsoSurface = 2
}
/**
 * IMeshComponentValueInfo represents information about value of mesh property component,
 * from `MeshComponentValueInfo`.
 */
export interface IMeshComponentValueInfo {
    value: number;
    displayUnits: string;
    description: string;
}
/**
 * MeshComponentValueInfo represents information about value of mesh property component.
 */
export declare class MeshComponentValueInfo implements IMeshComponentValueInfo {
    value: number;
    displayUnits: string;
    description: string;
}
/**
 * IMeshPropertyValuesInfo represents information about values of mesh property,
 * from `MeshPropertyValuesInfo`.
 */
export interface IMeshPropertyValuesInfo {
    description: string;
    components: Array<IMeshComponentValueInfo>;
}
/**
 * MeshPropertyValuesInfo represents information about values of mesh property.
 */
export declare class MeshPropertyValuesInfo implements IMeshPropertyValuesInfo {
    description: string;
    components: Array<IMeshComponentValueInfo>;
}
/**
 * IMeshPropertyInfo represents mesh property component info,
 * from `MeshPropertyInfo`.
 */
export interface IMeshPropertyInfo {
    propertyId: string;
    componentIndex: number;
}
/**
 * IMeshPropertyInfo represents mesh property component info.
 */
export declare class MeshPropertyInfo implements IMeshPropertyInfo {
    propertyId: string;
    componentIndex: number;
}
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
}
/**
 * SelectionMeshEntity represents detected/selected info of {@link InteractiveCaeMesh} entity.
 */
export declare class SelectionMeshEntity implements ISelectionMeshEntity {
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
}
/**
 * IDetectedObjectInfo represents info about detected object, from `DetectedObjectInfo`.
 */
export interface IDetectedObjectInfo {
    objectId: ObjectId;
    partId: number;
    subPartId: number;
    pickedPoint: IVec3;
    pickedNormal: IVec3;
}
/**
 * Structure to define object's part/subpart.
 */
export declare class ObjectPart {
    id: number;
    subId: number;
}
/**
 * Structure to define detected object + its subparts.
 */
export declare class DetectedObjectInfo {
    object: InteractiveObject | null;
    part: ObjectPart;
    pickedPoint: IVec3;
    pickedNormal: IVec3;
}
/**
 * Auxiliary class for accessing basic InteractiveObject aspects.
 */
export declare class PresentationAspects {
    /** Back-link to the object. */
    private myPrs;
    /** Constructor. */
    constructor(thePrs: InteractiveObject);
    /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
    get transparency(): number;
    /**
     * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
     * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
     */
    get baseColor(): ColorString;
    /**
     * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
     * Note that if object has undefined color, but defined transparency - this property will return empty string!
     */
    get baseColorRgba(): ColorString;
    /** Return object's font height. */
    get textHeight(): number;
    /** Return object's font name. */
    get textFont(): string;
    /** Return object's font aspect. */
    get textFontAspect(): FontAspect;
    /** Return object's text alignment. */
    get textJustification(): ViewCorner;
    /** Return object's line thickness. */
    get lineWidth(): number;
    /** Return object's line type. */
    get lineType(): TypeOfLine;
    /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
    set transparency(theTransp: number);
    /** Set object's global color or empty string to unset. */
    set baseColor(theColor: ColorString);
    /** Set object's global color + transparency or empty string to unset both. */
    set baseColorRgba(theColor: ColorString);
    /** Set object's font height. */
    set textHeight(theHeight: number);
    /** Set object's font name. */
    set textFont(theFont: string);
    /** Set object's font aspect. */
    set textFontAspect(theAsp: FontAspect);
    /** Set object's text alignment. */
    set textJustification(theJust: ViewCorner);
    /** Set object's line width. */
    set lineWidth(theWidth: number);
    /** Set object's line type. */
    set lineType(theType: TypeOfLine);
}
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
export declare class InteractiveObject extends ViewerEventTarget {
    /** Empty constructor. */
    constructor(theObjId?: ObjectId);
    /** Get object id as {@link ObjectId}. */
    get id(): ObjectId;
    /** Get object id before removal. */
    get oldId(): ObjectId;
    /** Return TRUE if object with specified Id exists. */
    get isAlive(): boolean;
    /** Return object type. */
    get type(): string;
    /** Return TRUE if object with specified Id has deferred data loaded. */
    get isLoaded(): boolean;
    /** Return object enabled state. */
    get isEnabled(): boolean;
    /** Return object visibility state. */
    get isVisible(): boolean;
    /** Return view in which object is visible or NULL if it is not displayed or displayed in multiple views. */
    get visibleInView(): Subview | null;
    /** Return TRUE if this object is listed within selected objects. */
    get isSelected(): boolean;
    /** Return if this presentation is defined in 2D or 3D space. */
    get is2d(): boolean;
    /** Return view corner of 2D presentation; has no meaning for 3D objects. */
    get viewCorner(): ViewCorner;
    /** Return view corner offset of 2D presentation; has no meaning for 3D objects. */
    get viewCornerOffset(): IVec2;
    /** Return local transformation. */
    get localTransformation(): ITrsf;
    /** Return object AABB (axes-aligned bounding box). */
    get minMax(): IBndBox3;
    /** Return object's Z-Layer. */
    get zlayer(): ZLayerId;
    /** Return a value that is added to the default displaying/selection object's priority, 0 by default. */
    get priorityModifier(): number;
    /**
     * Infinite presentation flag, FALSE by default.
     * Used to distinguish auxiliary presentations (construction elements, manipulators) from main scene objects.
     * Such objects are not considered within FitAll operation.
     */
    get isInfiniteState(): boolean;
    /** Return aspects manager. */
    get aspects(): PresentationAspects;
    /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
    get transparency(): number;
    /**
     * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
     * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
     */
    get baseColor(): ColorString;
    /**
     * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
     * Note that if object has undefined color, but defined transparency - this property will return empty string!
     */
    get baseColorRgba(): ColorString;
    /** Set object enabled state. */
    set isEnabled(theToEnable: boolean);
    /** Set object visibility state. */
    set isVisible(theToShow: boolean);
    /**
     * Set object to be visible only within specific view or all views (NULL) - view affinity.
     * Note that {@link isVisible} still has to be set independently.
     */
    set visibleInView(theView: Subview | null);
    /** Put/remove object from selected objects. */
    set isSelected(theToSelect: boolean);
    /** Set if this presentation is defined in 2D or 3D space. */
    set is2d(theIs2d: boolean);
    /** Set view corner. */
    set viewCorner(theCorner: ViewCorner);
    /** Set view corner offset. */
    set viewCornerOffset(theOffset: IVec2);
    /** Set local transformation. */
    set localTransformation(theLoc: ITrsf);
    /** Set object's Z-Layer. */
    set zlayer(theLayer: ZLayerId);
    /** Set a value that is added to the default displaying/selection object's priority. */
    set priorityModifier(theModifier: number);
    /** Set infinite presentation flag. */
    set isInfiniteState(theState: boolean);
    /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
    set transparency(theTransp: number);
    /** Set object's global color or empty string to unset. */
    set baseColor(theColor: ColorString);
    /** Set object's global color + transparency or empty string to unset both. */
    set baseColorRgba(theColor: ColorString);
    /** Remove object and invalidates it's id. */
    remove(): boolean;
    /**
     * Wrap existing object.
     * @param {SCwebviewer} theViewer viewer module
     * @param {string} theId the id to wrap
     * @return {boolean} FALSE if object with specified Id doesn't exist in the Viewer
     */
    wrap(theViewer: SCwebviewer, theId?: string): boolean;
    /** Return file path from object id. */
    getFilePathFromObjId(): string;
    /**
     * Load deferred data async.
     * @param theViewer module object
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    protected loadDataAsync(theViewer: SCwebviewer): Promise<boolean>;
}
/**
 * Represents Context for Interactive Objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveObject} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveContext} property.
 */
export interface InteractiveContext extends IManager {
    /** Return the array of created objects */
    readonly objectsInside: InteractiveObject[];
    /**
     * Remove all named objects.
     * Warning! Using this method would make existing InteractiveObject instances
     * to have dead references (see also {@link InteractiveObject.isAlive}).
     */
    removeAll: () => boolean;
    /**
     * Return id of last dragged object or empty string.
     */
    getLastDraggedObject: () => string;
}
/**
 * Represents selection manager for Interactive Objects.
 * See also {@link SCwebviewer.selectionManager} property.
 */
export interface SelectionManager extends IManager {
    /** Return/set the array of selection objects */
    selectedObjects: InteractiveObject[];
    /** Return/set a single selected object. Return null if multiple/none objects selected */
    oneSelectedObject: InteractiveObject | null;
    /** Return the last detected object */
    readonly lastDetectedObject: InteractiveObject | null;
    /** Return the last detected object info */
    readonly lastDetectedInfo: DetectedObjectInfo;
    /** Return/set selection mode multiple-true, single-false */
    toAllowMultiselection: boolean;
    /** Unselect all previously selected objects*/
    resetSelection: () => void;
}
