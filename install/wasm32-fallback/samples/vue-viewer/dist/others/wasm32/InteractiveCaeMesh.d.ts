import type { UnitManager, ColorString, IVec2, IVec3, IVec4, ObjectId, IBndBox3 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { InteractiveColorScale } from './InteractiveColorScale.js';
import type { ISelectionMeshEntity, SCwebviewer } from './SCwebviewer.js';
/**
 * CaeMeshObjectPrsMode represents presentation mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshPrsMode`.
 */
export declare enum CaeMeshObjectPrsMode {
    /** Mesh edges with no shading. */
    MeshEdges = 0,
    /** Shaded with no edges. */
    FlatShaded = 1,
    /** Shaded + mesh edges. */
    ShadedMeshEdges = 2,
    /** Shaded with shrunk mesh elements. */
    ShadedMeshShrinked = 3,
    /** Shaded + free mesh edges. */
    ShadedFreeEdges = 4,
    /** Shaded + sharp mesh edges. */
    ShadedSharpEdges = 5
}
/**
 * CaeMeshObjectElementType represents elements of {@link InteractiveCaeMesh} to display, from `InteractiveCaeMeshElementType`.
 */
export declare enum CaeMeshObjectElementType {
    Element0d = 1,
    Element1d = 2,
    Face = 4,
    Volume = 8,
    Node = 16,
    FreeNode = 32,
    Centroid = 64,
    FreeEdges = 128,
    SharpEdges = 256,
    AllElemental = 15
}
/**
 * CaeMeshObjectSelMode represents selection mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export declare enum CaeMeshObjectSelMode {
    /** Disabled selection. */
    None = 0,
    /** Selection of mesh presentation. */
    Presentation = 1,
    /** Selection of mesh elements. */
    Elements = 2,
    /** Selection of mesh nodes. */
    Nodes = 4
}
/**
 * CaeMeshVectorDistrType represents distribution type for vector mesh presentation, from `MeshVectorDistrType`.
 */
export declare enum CaeMeshVectorDistrType {
    Volume = 0,
    Surface = 1
}
/**
 * Represents type of mesh presentation, from `MeshPresentationType`.
 */
export declare enum CaeMeshPresentationType {
    Scalar = 0,
    Vector = 1,
    Streamlines = 2
}
/**
 * Represents type of mesh highlighting, from `MeshHighlightType`.
 */
export declare enum CaeMeshHighlightType {
    None = 0,
    Highlight = 1,
    Selection = 2
}
/**
 * ICaeMeshInfo represents mesh data info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshInfo {
    nbElems0d: number;
    nbElems1d: number;
    nbElems2d: number;
    nbTriangles: number;
    nbTriangles2nd: number;
    nbQuads: number;
    nbQuads2nd: number;
    nbPolygons: number;
    nbElems3d: number;
    nbTetrahedrons: number;
    nbTetrahedrons2nd: number;
    nbPyramids: number;
    nbPyramids2nd: number;
    nbPrisms: number;
    nbPrisms2nd: number;
    nbHexahedrons: number;
    nbHexahedrons2nd: number;
    nbPolyhedrons: number;
    nbNodes: number;
    boundsMin: IVec3;
    boundsMax: IVec3;
}
/**
 * Represents mesh data info of {@link InteractiveCaeMesh} entity.
 */
export declare class CaeMeshInfo implements ICaeMeshInfo {
    nbElems0d: number;
    nbElems1d: number;
    nbElems2d: number;
    nbTriangles: number;
    nbTriangles2nd: number;
    nbQuads: number;
    nbQuads2nd: number;
    nbPolygons: number;
    nbElems3d: number;
    nbTetrahedrons: number;
    nbTetrahedrons2nd: number;
    nbPyramids: number;
    nbPyramids2nd: number;
    nbPrisms: number;
    nbPrisms2nd: number;
    nbHexahedrons: number;
    nbHexahedrons2nd: number;
    nbPolyhedrons: number;
    nbNodes: number;
    boundsMin: IVec3;
    boundsMax: IVec3;
    /** Format mesh info string. */
    static formatInfo(theInfo: ICaeMeshInfo, theUMgr?: UnitManager | null): string;
}
/**
 * ICaeMeshPropCompInfo represents mesh data property component info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshPropCompInfo {
    index: number;
    name: string;
    description: string;
    displayUnits: string;
    valueRange: IVec2;
}
/**
 * Represents mesh data property component info of {@link InteractiveCaeMesh} entity.
 */
export declare class CaeMeshPropCompInfo implements ICaeMeshPropCompInfo {
    index: number;
    name: string;
    description: string;
    displayUnits: string;
    valueRange: IVec2;
}
/**
 * Represents type of mesh property, from `CaeMeshPropType`.
 */
export declare enum CaeMeshPropType {
    ScalarSet = 0,
    Vector = 1,
    Tensor = 2
}
/**
 * Represents type of mesh property elements, from `CaeMeshPropElemType`.
 */
export declare enum CaeMeshPropElemType {
    Scalar = 0,
    NodeVec = 1,
    Group = 2
}
/**
 * ICaeMeshPropInfo represents mesh data property info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshPropInfo {
    propId: string;
    timeSteps: Array<number>;
    loadedTimeSteps: Array<number>;
    name: string;
    description: string;
    isElemental: boolean;
    type: CaeMeshPropType;
    elemType: CaeMeshPropElemType;
    comps: Array<ICaeMeshPropCompInfo>;
}
/**
 * Represents mesh data property info of {@link InteractiveCaeMesh} entity.
 */
export declare class CaeMeshPropInfo implements ICaeMeshPropInfo {
    propId: string;
    timeSteps: Array<number>;
    loadedTimeSteps: Array<number>;
    name: string;
    description: string;
    isElemental: boolean;
    type: CaeMeshPropType;
    elemType: CaeMeshPropElemType;
    comps: Array<ICaeMeshPropCompInfo>;
}
/**
 * text + icon pair
 */
interface ICaeMeshTextIcon {
    text: string;
    icon: string;
}
/**
 * CaeMeshAnnotation represents mesh annotation info.
 */
export interface CaeMeshAnnotation {
    isVisible: boolean;
    data: Array<ICaeMeshTextIcon>;
    toShowText: boolean;
    toShowIcons: boolean;
    pos: IVec3;
}
/**
 * CaeMeshBlock represents mesh block node.
 * It contains name and array of children blocks so it forms a tree structure.
 */
export declare class CaeMeshBlock {
    name: string;
    children?: Array<CaeMeshBlock>;
}
/**
 * ICaeMeshStreamlinesInfo represents streamlines info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshStreamlinesInfo {
    /** Property id. */
    property: string;
    /** Switch between regular (true) and random (false) distribution of the initial points in the seed plane. */
    isRegular: boolean;
    /** Total number of streamlines for random distribution of initial points. */
    nbStreamlines: number;
    /** "U x V" streamlines layout for regular distribution of initial points. */
    uvStreamlines: IVec2;
    /** Origin of plane used for streamlines initial points generation. */
    planeOrigin: IVec3;
    /** Sized side vector of plane used for streamlines initial points generation. */
    planeSide: IVec3;
    /** Sized up vector of plane used for streamlines initial points generation. */
    planeUp: IVec3;
    /** Integration step coefficient; lower value leads to more accurate result but takes more time to calculate. */
    stepCoeff: number;
    /** Streamlines max length coefficient. */
    lengthCoeff: number;
    /** Thickness as a coefficient multiplied by bounding box size. */
    thickness: number;
    /** Animation particles speed coefficient, in [0.0, 1.0] range. */
    animationSpeed: number;
    /** Animation particles trail length coefficient, in [0.0, 1.0] range. */
    animationTrail: number;
    /** Animation particles frequency, integer number in [1, 100] range. */
    animationFrequency: number;
    /** Flag to enable/disable streamlines animation. */
    animationEnabled: boolean;
}
/**
 * Represents streamlines info of {@link InteractiveCaeMesh} entity.
 */
export declare class CaeMeshStreamlinesInfo implements ICaeMeshStreamlinesInfo {
    /** Property id. */
    property: string;
    /** Switch between regular (true) and random (false) distribution of the initial points in the seed plane. */
    isRegular: boolean;
    /** Total number of streamlines for random distribution of initial points. */
    nbStreamlines: number;
    /** "U x V" streamlines layout for regular distribution of initial points. */
    uvStreamlines: IVec2;
    /** Origin of plane used for streamlines initial points generation. */
    planeOrigin: IVec3;
    /** Sized side vector of plane used for streamlines initial points generation. */
    planeSide: IVec3;
    /** Sized up vector of plane used for streamlines initial points generation. */
    planeUp: IVec3;
    /** Integration step coefficient; lower value leads to more accurate result but takes more time to calculate. */
    stepCoeff: number;
    /** Streamlines max length coefficient. */
    lengthCoeff: number;
    /** Thickness as a coefficient multiplied by bounding box size. */
    thickness: number;
    /** Animation particles speed coefficient, in [0.0, 1.0] range. */
    animationSpeed: number;
    /** Animation particles trail length coefficient, in [0.0, 1.0] range. */
    animationTrail: number;
    /** Animation particles frequency, integer number in [1, 100] range. */
    animationFrequency: number;
    /** Flag to enable/disable streamlines animation. */
    animationEnabled: boolean;
}
/**
 * id + index pair for iso properties.
 */
interface ICaeMeshIdIndex {
    id: string;
    index: number;
}
/**
 * id + name pair for properties.
 */
interface ICaeMeshIdName {
    id: string;
    name: string;
}
/**
 * Interface for names of 2D grid axes and tick mark labels
 * when mesh is displayed as part of 2D graph.
 */
export interface ICaeMesh2DAxesNamingInfo {
    nameX: string;
    nameY: string;
    ticksX: string[];
    ticksY: string[];
}
/**
 * Contains default values for ICaeMesh2DAxesNamingInfo.
 */
export declare class CaeMesh2DAxesNamingInfo implements ICaeMesh2DAxesNamingInfo {
    nameX: string;
    nameY: string;
    ticksX: string[];
    ticksY: string[];
}
/**
 * Auxiliary class for accessing InteractiveCaeMesh presentations.
 */
export declare class CaeMeshPresentation {
    /** Return presentation id. */
    get id(): string;
    /** Return presentation type. */
    get type(): CaeMeshPresentationType;
    /** Return mesh presentation mode. */
    get presentationMode(): CaeMeshObjectPrsMode;
    /** Return active mesh property. */
    get activeProperty(): string;
    /** Return index of active mesh property component. */
    get activeComponent(): number;
    /** To display 3d elements. */
    get toDisplay3dElements(): boolean;
    /** To display 2d elements. */
    get toDisplay2dElements(): boolean;
    /** To display 0d elements. */
    get toDisplay0dElements(): boolean;
    /** To display element centroids. */
    get toDisplayCentroids(): boolean;
    /** To display nodes. */
    get toDisplayNodes(): boolean;
    /** Return true if free nodes should be used within element nodes in calculation/visualization. */
    get toProcessFreeNodes(): boolean;
    /** To display free edges. */
    get toDisplayFreeEdges(): boolean;
    /** To display sharp edges. */
    get toDisplaySharpEdges(): boolean;
    /** Return id of current active deformed mesh property. */
    get activeDeformedProp(): string;
    /** Return scale coefficient for mesh deformed presentation. */
    get deformedScaleCoeff(): number;
    /**
     * Return true if it is needed to render/select 3d mesh only by its surface
     * and to not include internal parts, True by default.
     */
    get isVolumeAsSurface(): boolean;
    /** Return presentation visibility. */
    get isVisible(): boolean;
    /** Return shading color, "#ffffff" by default. */
    get shadingColor(): ColorString;
    /** Return highlighting shading color, "#e5e5e5" by default. */
    get shadingHighlightColor(): ColorString;
    /** Return selection shading color, "#cccccc" by default. */
    get shadingSelectionColor(): ColorString;
    /** Return edges color, "#cccccc" by default. */
    get edgesColor(): ColorString;
    /** Return highlighting edges color, "#e5e5e5" by default. */
    get edgesHighlightColor(): ColorString;
    /** Return selection edges color, "#cccccc" by default. */
    get edgesSelectionColor(): ColorString;
    /** Return edges width, 1 by default. */
    get edgesWidth(): number;
    /** Return highlighting edges width, 1 by default. */
    get edgesHighlightWidth(): number;
    /** Return selection edges width, 1 by default. */
    get edgesSelectionWidth(): number;
    /** Return points color, "#7f7f7f" by default. */
    get pointsColor(): ColorString;
    /** Return highlighting points color, "#e5e5e5" by default. */
    get pointsHighlightColor(): ColorString;
    /** Return selection points color, "#cccccc" by default. */
    get pointsSelectionColor(): ColorString;
    /** Return points size, 5 by default. */
    get pointsSize(): number;
    /** Return highlighting points size, 5 by default. */
    get pointsHighlightSize(): number;
    /** Return selection points size, 5 by default. */
    get pointsSelectionSize(): number;
    /** Return shading transparency, 0.0 by default. */
    get shadingTransparency(): number;
    /** Return highlighting shading transparency, 0.8 by default. */
    get shadingHighlightTransparency(): number;
    /** Return shading transparency, 0.8 by default. */
    get shadingSelectionTransparency(): number;
    /** Return id of current active vector mesh property. */
    get activeVecProp(): string;
    /** Return scale coefficient for mesh vector presentation. */
    get vecPropScaleCoeff(): number;
    /** Return distribution type for mesh vector presentation. */
    get vecPropDistrType(): CaeMeshVectorDistrType;
    /** Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0, 1.0 by default. */
    get vecSamplingRatio(): number;
    /** Return equation of clip plane used to filter mesh elements (null if not set). */
    get clipPlane(): IVec4 | null;
    /** Check whether mesh clipping cuts or filters elements. */
    get toCutOnClipPlane(): boolean;
    /** Return array of planes used to cut mesh. */
    get slicePlanes(): Array<IVec4>;
    /** Return array of iso values. */
    get isoProperty(): ICaeMeshIdIndex;
    /** Return array of iso values. */
    get isoValues(): Array<number>;
    /** Return visibility of annotations for min/max values of the current active mesh property. */
    get toShowMinMax(): boolean;
    /** Return visibility of annotations for the current active group mesh property. */
    get toShowGroupsInfo(): boolean;
    /** Return streamlines presentation parameters. */
    get streamlines(): ICaeMeshStreamlinesInfo;
    /** Return mesh selection mode. */
    get selectionMode(): number;
    /** Set mesh presentation mode. */
    set presentationMode(theMode: CaeMeshObjectPrsMode);
    /** Set active mesh property. */
    set activeProperty(thePropertyId: string);
    /** Set index of active mesh property component. */
    set activeComponent(theComponentIndex: number);
    /** Set to display 3d elements. */
    set toDisplay3dElements(toDisplay: boolean);
    /** Set to display 2d elements. */
    set toDisplay2dElements(toDisplay: boolean);
    /** Set to display 0d elements. */
    set toDisplay0dElements(toDisplay: boolean);
    /** Set to display element centroids. */
    set toDisplayCentroids(toDisplay: boolean);
    /** Set to display nodes. */
    set toDisplayNodes(toDisplay: boolean);
    /** Define if free nodes should be used within element nodes in calculation/visualization. */
    set toProcessFreeNodes(toDisplay: boolean);
    /** Set to display free edges. */
    set toDisplayFreeEdges(toDisplay: boolean);
    /** Set to display sharp edges. */
    set toDisplaySharpEdges(toDisplay: boolean);
    /** Set current active deformed mesh property. */
    set activeDeformedProp(thePropertyId: string);
    /** Set scale coefficient for mesh deformed presentation. */
    set deformedScaleCoeff(theValue: number);
    /**
     * Set if it is needed to render/select 3d mesh only by its surface
     * and to not include internal parts.
     */
    set isVolumeAsSurface(theIsVolumeAsSurface: boolean);
    /** Set presentation visibility. */
    set isVisible(theIsVisible: boolean);
    /** Set shading color. */
    set shadingColor(theColor: ColorString);
    /** Set highlighting shading color. */
    set shadingHighlightColor(theColor: ColorString);
    /** Set selection shading color. */
    set shadingSelectionColor(theColor: ColorString);
    /** Set edges color. */
    set edgesColor(theColor: ColorString);
    /** Set highlighting edges color. */
    set edgesHighlightColor(theColor: ColorString);
    /** Set selection edges color. */
    set edgesSelectionColor(theColor: ColorString);
    /** Set points color. */
    set pointsColor(theColor: ColorString);
    /** Set highlighting points color. */
    set pointsHighlightColor(theColor: ColorString);
    /** Set selection points color. */
    set pointsSelectionColor(theColor: ColorString);
    /** Set edges width. */
    set edgesWidth(theWidth: number);
    /** Set highlighting edges width. */
    set edgesHighlightWidth(theWidth: number);
    /** Set selection edges width. */
    set edgesSelectionWidth(theWidth: number);
    /** Set points size. */
    set pointsSize(theSize: number);
    /** Set highlighting points size. */
    set pointsHighlightSize(theSize: number);
    /** Set selection points size. */
    set pointsSelectionSize(theSize: number);
    /** Set shading transparency. */
    set shadingTransparency(theValue: number);
    /** Set highlighting shading transparency. */
    set shadingHighlightTransparency(theValue: number);
    /** Set selection shading transparency. */
    set shadingSelectionTransparency(theValue: number);
    /** Set current active vector mesh property. */
    set activeVecProp(thePropertyId: string);
    /** Set distribution type for mesh vector presentation. */
    set vecPropDistrType(theType: CaeMeshVectorDistrType);
    /** Set scale coefficient for mesh vector presentation. */
    set vecPropScaleCoeff(theValue: number);
    /** Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0. */
    set vecSamplingRatio(theRatio: number);
    /** Set/unset clip plane used to filter mesh elements. */
    set clipPlane(theEq: IVec4 | null);
    /** Define whether mesh clipping cuts or filters elements. */
    set toCutOnClipPlane(theToCut: boolean);
    /** Set/unset array of planes used to cut mesh. */
    set slicePlanes(theEqs: Array<IVec4>);
    /** Set/unset array of iso values. */
    set isoProperty(theProp: ICaeMeshIdIndex);
    /** Set/unset array of iso values. */
    set isoValues(theValues: Array<number>);
    /** Set visibility of annotations for min/max values of the current active mesh property. */
    set toShowMinMax(theToShow: boolean);
    /** Set visibility of annotations for the current active group mesh property. */
    set toShowGroupsInfo(theToShow: boolean);
    /** Set streamlines presentation parameters. */
    set streamlines(theInfo: ICaeMeshStreamlinesInfo);
    /** Set mesh selection mode. */
    set selectionMode(theMode: number);
    /** Return annotation for specified mesh group. */
    getGroupInfo(theGroupId: number): CaeMeshAnnotation;
    /** Set annotation for specified mesh group. */
    setGroupInfo(theGroupId: number, theInfo: CaeMeshAnnotation): void;
    /** Return visible mesh blocks. */
    getVisibleBlocks(): CaeMeshBlock | null;
    /** Set mesh blocks visibility. */
    setVisibleBlocks(theBlocks: CaeMeshBlock, theVisible: boolean): void;
    /** Set custom geometry for the specified part. */
    setGenGeometry(theViewer: SCwebviewer, thePartId: number, theArray: ArrayBuffer): Promise<boolean>;
}
/**
 * timeStep + timestamp pair for CAE animation.
 */
interface ICaeMeshTimeStep {
    timeStep: number;
    timestamp: number;
}
/**
 * Auxiliary class for accessing InteractiveCaeMesh animation.
 */
export declare class CaeMeshAnimation {
    /** Check if mesh properties animation contains valid input data. */
    get isValid(): boolean;
    /** Check if mesh properties animation is running. */
    get isActive(): boolean;
    /** Return duration of mesh properties animation, in seconds, 5.0 by default. */
    get duration(): number;
    /** Check if animation should be restarted on the last update, true by default. */
    get isLoopPlayback(): boolean;
    /** Check if animation steps duration should take into account timestamps values, false by default. */
    get uniformSteps(): boolean;
    /** Set duration of mesh properties animation, in seconds. */
    set duration(theDur: number);
    /** Define if animation should be restarted on the last update. */
    set isLoopPlayback(theIsLoop: boolean);
    /** Define if animation steps duration should take into account timestamps values. */
    set uniformSteps(theIsUniform: boolean);
    /** Start animation of mesh properties. */
    start(theToRestart?: boolean): void;
    /** Stop animation of mesh properties. */
    stop(): void;
    /**
     * Continue animation if it was paused after trying to move on the next time step
     * without not loaded related properties.
     * Should be called after all required properties have been loaded.
     */
    continue(): void;
}
/**
 * Interactive Mesh Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export declare class InteractiveCaeMesh extends InteractiveObject {
    private _loadedFiles;
    private _toCacheProperties;
    /** Empty constructor. */
    constructor(theObjId?: ObjectId);
    /** Return all mesh presentations. */
    get presentations(): Array<CaeMeshPresentation>;
    /** Return mesh properties. */
    get properties(): Array<ICaeMeshPropInfo>;
    /** Return mesh bounding box. */
    get boundingBox(): IBndBox3;
    /** Return available vector mesh properties. */
    get availableVec3Props(): ICaeMeshIdName[];
    /** Return attached color scale objects. */
    get colorScaleObjects(): Array<InteractiveColorScale>;
    /** Return object's selection mode. */
    get selectionMode(): number;
    /** Return mesh info. */
    get info(): ICaeMeshInfo;
    /** Return mesh info string. */
    get infoString(): string;
    /** Return mesh blocks. */
    get blocks(): CaeMeshBlock | null;
    /** Return active time index. */
    get activeTimeStep(): number;
    /** Return available time indices. */
    get availableTimeSteps(): ICaeMeshTimeStep[];
    /** Return animation object. */
    get animation(): CaeMeshAnimation;
    /** Check if mesh entities should be detected/selected. */
    get toHighlightEntities(): boolean;
    /**
     * Return entities selection color, "#CCCCCC33" by default.
     * Combined from selection shading color/transparency.
     * Kept for API compatibility.
     */
    get highlightColor(): ColorString;
    /** Check whether the mesh draws a grid. */
    get toShow2DGrid(): boolean;
    /** Get axes names and tick mark labels. */
    get axesNamingParams(): ICaeMesh2DAxesNamingInfo;
    /** Shortcut to interactive CAE mesh context. */
    private get meshMgr();
    /** Set object's selection mode. */
    set selectionMode(theMode: number);
    /** Set active time index. */
    set activeTimeStep(theTimeStep: number);
    /** Set available time indices. */
    set availableTimeSteps(theTimeSteps: ICaeMeshTimeStep[]);
    /** Define if properties data is stored in MDB file and can be loaded/unloaded on demand. */
    set toCacheProperties(theToCache: boolean);
    /** Define if mesh entities should be detected/selected. */
    set toHighlightEntities(theToHighlight: boolean);
    /**
     * Set entities selection color.
     * Affect selection shading color/transparency.
     * Kept for API compatibility.
     */
    set highlightColor(theColor: ColorString);
    /** Set whether the mesh draws a grid. */
    set toShow2DGrid(theToShow: boolean);
    /** Set axes names and tick mark labels. */
    set axesNamingParams(theInfo: ICaeMesh2DAxesNamingInfo);
    /**
     * Probe file header for known mesh formats.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to open
     * @param {string} theFilePath the file path to map
     * @return {string} detected format (if supported) or an empty string
     */
    static probeFileFormat(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): string;
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveCaeMesh | null;
    /** Create new mesh presentation of the specified type. */
    createPresentation(theType: CaeMeshPresentationType, thePrsId?: string): CaeMeshPresentation | null;
    /** Remove mesh presentation. */
    removePresentation(thePrs: CaeMeshPresentation): void;
    /** Get presentation by id. */
    getPresentation(thePrsId: string): CaeMeshPresentation;
    /** Return mesh property. */
    property(thePropertyId: string): ICaeMeshPropInfo | null;
    /** Return mesh property component. */
    propertyComponent(thePropertyId: string, theCompIndex: number): ICaeMeshPropCompInfo | null;
    /** Return value range for mesh property. */
    getPropertyValueRange(thePropertyId: string, theComponentIndex: number): IVec2;
    /** Return name for mesh property. */
    getPropertyName(thePropertyId: string): string;
    /** Return descrition for mesh property. */
    getPropertyDescription(thePropertyId: string): string;
    /** Return name for mesh property component. */
    getComponentName(thePropertyId: string, theComponentIndex: number): string;
    /** Return descrition for mesh property component. */
    getComponentDescription(thePropertyId: string, theComponentIndex: number): string;
    /** Return display units for specified mesh property. */
    getPropertyDisplayUnits(thePropertyId: string, theComponentIndex: number): string;
    /** Return true if mesh property is elemental. */
    isPropertyElemental(thePropertyId: string): boolean;
    /** Return true if mesh property is a group. */
    isPropertyGroup(thePropertyId: string): boolean;
    /** Return true if mesh property contains one scalar component. */
    isPropertySingleScalar(thePropertyId: string): boolean;
    /** Find property id by name. */
    getPropertyIdByName(thePropertyName: string): string;
    /** Return map of groups with non-zero number of entities. */
    usedGroups(thePropertyId: string): Map<number, string>;
    /** Return detected sub entity. */
    getDetectedEntity(): ISelectionMeshEntity;
    /** Return selected sub entities. */
    getSelectedEntities(): Array<ISelectionMeshEntity>;
    /** Select sub entity. */
    setSelectedEntity(theEntity: ISelectionMeshEntity): Promise<boolean>;
    /**
     * Open object from the given URL and register presentation(s) in the hidden (erased) state.
     * @param {SCwebviewer} theViewer viewer module
     * @param {string} theFileUrl the file URL address
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    loadUrlAsync(theViewer: SCwebviewer, theFileUrl: string): Promise<boolean>;
    /**
     * Open object from the given ArrayBuffer and register presentation(s) in the hidden (erased) state.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to open
     * @param {string} theFilePath the file path to map
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    loadBufferAsync(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): Promise<boolean>;
    /**
     * Load properties from the given ArrayBuffer and append to the current object.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to load
     * @return {Promise<boolean>} FALSE if properties cannot be loaded
     */
    loadPropertiesAsync(theViewer: SCwebviewer, theArray: ArrayBuffer): Promise<boolean>;
    /** Dump mesh into a file. */
    save(theFile?: string): boolean;
    /** Remove all properties. */
    removeProperties(theWithMetadata?: boolean): void;
    /** Remove object and invalidates it's id. */
    remove(): boolean;
    /** Set to use custom geometry for mesh presentations. */
    setUseGenGeometry(): void;
    /** Set to use test custom geometry for mesh presentations. */
    setUseTestCacheGenGeometry(): void;
    /** Unlink related files. */
    private unlinkLoadedFiles;
    /** Open mesh from file path. */
    private openMeshFromFile;
    /** Load properties from the file and append to the current object. */
    private loadProperties;
}
export {};
