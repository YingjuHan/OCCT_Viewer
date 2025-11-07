import type { ColorString, IVisMaterial, IVec3, IAx2, ObjectId, IShapeInfo, ICylinderParams, IConeParams, ITrsf } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Material presets defined by `Graphic3d_NameOfMaterial` enumeration.
 */
export declare const MaterialPreset: {
    Brass: string;
    Bronze: string;
    Copper: string;
    Gold: string;
    Pewter: string;
    Plastered: string;
    Plastified: string;
    Silver: string;
    Steel: string;
    Stone: string;
    ShinyPlastified: string;
    Satin: string;
    Metalized: string;
    Ionized: string;
    Chrome: string;
    Aluminum: string;
    Obsidian: string;
    Neon: string;
    Jade: string;
    Charcoal: string;
    Water: string;
    Glass: string;
    Diamond: string;
    Transparent: string;
    Default: string;
};
/**
 * ShapeObjectPrsMode represents presentation mode of {@link InteractiveShape}, from `InteractiveShapePrsMode`.
 */
export declare enum ShapeObjectPrsMode {
    /** Hollow wireframe geometry (face edges). */
    Wireframe = 0,
    /** Shaded interior with smooth normals. */
    Shaded = 1,
    /** Shaded interior with smooth normals and face edges. */
    ShadedFaceBoundaries = 2,
    /** Shaded interior with flat normals. */
    ShadedFlat = 3,
    /** Shaded interior with triangulation edges. */
    ShadedTessellation = 4,
    /** Hidden line removal (HLR). */
    HiddenLine = 5
}
/**
 * ShapeObjectSelMode represents selection mode of {@link InteractiveShape}, from `InteractiveShapeSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export declare enum ShapeObjectSelMode {
    /** Selection of whole shape object. */
    Global = 0,
    /** Selection of shape vertices. */
    Vertex = 1,
    /** Selection of shape edges. */
    Edge = 2,
    /** Selection of shape wires. */
    Wire = 4,
    /** Selection of shape faces. */
    Face = 8,
    /** Selection of shape shells. */
    Shell = 16,
    /** Selection of shape solids. */
    Solid = 32,
    /** Selection of shape compsolids. */
    CompSolid = 64,
    /** Selection of groups. */
    Group = 128
}
/**
 * Auxiliary class for accessing InteractiveShape subshapes.
 */
export declare class SubshapeManager {
    private myShapePrs;
    private myShapeType;
    /** Constructor. */
    constructor(thePrs: InteractiveShape, theType: ShapeObjectSelMode);
    /** Iteratable interface. */
    [Symbol.iterator](): IterableIterator<IShapeInfo>;
    /** Return shape type. */
    get shapeType(): ShapeObjectSelMode;
    /** Return the number of available subshapes. */
    get length(): number;
    /** Return the list with indices of available subshapes. */
    get defined(): number[];
    /** Return the list with indices of free subshapes. */
    get free(): number[];
    /** Return the list with indices of disabled subshapes. */
    get disabled(): number[];
    /** Return the list with indices of enabled subshapes. */
    get enabled(): number[];
    /** Return the list with indices of selected subshapes */
    get selected(): number[];
    /** Return index list of colored subshapes. */
    get colored(): number[];
    /** Return the last detected subshape index, or `-1` if unspecified. */
    get lastDetected(): number;
    /** Return subshape info. */
    get shapeInfo(): IShapeInfo[];
    /** Return index list of visible subshapes. */
    get visible(): number[];
    /** Return index list of hidden subshapes. */
    get hidden(): number[];
    /** Set the list with indices of disabled subshapes. */
    set disabled(theList: number[]);
    /** Set the list with indices of enabled subshapes. */
    set enabled(theList: number[]);
    /** Set the list with indices of selected subshapes of this type but keeps selection of subshapes of other types. */
    set selected(theList: number[]);
    /** Set the list with indices of selected subshapes and resets selection of all other subshape types. */
    set selectedOnly(theList: number[]);
    /** Set index list of visible subshapes (makes hidden unspecified subshapes). */
    set visible(theList: number[]);
    /** Set index list of hidden subshapes (makes visible unspecified subshapes). */
    set hidden(theList: number[]);
    /** Return indexes of subshapes of specified subshape. */
    getSubshapesIndexes(theIndex: number, theSubType: ShapeObjectSelMode): number[];
    /** Return the subshape info. */
    getShapeInfo(theIndex: number, theToExplore?: boolean): IShapeInfo;
    /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
    getConnectedSubshapes(theIndexList: number[]): number[];
    /** Return subshape color or empty string */
    getSubshapeColor(theIndex: number): ColorString;
    /** Set or unset subhsape color. */
    setSubshapeColor(theIndex: number, theColor: ColorString): void;
    /** Set or unset subhsapes color; applied to all subhsapes when null is passed instead of index list */
    setSubshapesColor(theIndices: number[] | null, theColor: ColorString): void;
    /** Unset subshapes colors */
    clearSubshapeColors(): void;
    /** Return subshape hidden flag. */
    isSubshapeHidden(theIndex: number): boolean;
    /** Set or unset subhsape hidden flag. */
    setSubshapeHidden(theIndex: number, theIsHidden: boolean): void;
    /** Set or unset subhsapes hidden flag; applied to all subshapes when null is passed instead of index list */
    setSubshapesHidden(theIndices: number[] | null, theIsHidden: boolean, theToInvertOthers?: boolean): void;
    /** Unset subshapes hidden flag. */
    clearSubshapeHidden(): void;
    /** Make a copy of existing object with new id and specified subshape. */
    cloneSubshape(theIndex: number, theNewObjId?: ObjectId): InteractiveShape | null;
    /** Make a copy of existing object with new id and specified subshapes. */
    cloneSubshapes(theIndices: number[], theNewObjId?: ObjectId): InteractiveShape | null;
    /** Dump shape into a file. */
    saveSubshape(theIndex: number, theFile?: string): boolean;
    /** Dump shapes into a file. */
    saveSubshapes(theIndices: number[], theFile?: string): boolean;
}
/**
 * Proxy interface to access subshapes information
 * using array-alike interface (by index)
 * without instantiating a real array for all subshapes.
 */
declare class SubshapeManagerProxy extends Array<IShapeInfo> {
    private _myProxy;
    /** Main constructor. */
    constructor(theProxy: SubshapeManager);
}
/**
 * Interactive Shape Object.
 *
 * Usual loading workflow:
 * ```typescript
 *   var scViewer = new SCwebviewer.SCwebviewer('/');
 *   ...
 *   let aShape = new SCwebviewer.InteractiveShape();
 *   aShape.loadUrlAsync (scViewer, "/models/shape.stl").then(() => {
 *     aShape.isVisible = true;
 *     scViewer.camera.fitAllAuto (true);
 *     scViewer.requestViewUpdate();
 *   })
 *   .catch(anErr => {
 *     console.error ("Loading shape failed: " + anErr);
 *   });
 * ```
 *
 * Properties like {@link InteractiveShape.baseColor}, {@link InteractiveShape.transparency}, {@link InteractiveShape.materialMetallic},
 * {@link InteractiveShape.materialRoughness}, and {@link InteractiveShape.setMaterialPreset} could be used to specify object's material.
 * When object is loaded from file formats like glTF, it will use materials defined in the file, which could be assigned to sub-shapes.
 * In this case, material properties mentioned above will return empty values, and assigning them will override materials of all sub-shapes.
 */
export declare class InteractiveShape extends InteractiveObject {
    private _myGroupsMgr;
    private _myCompSolidMgr;
    private _mySolidMgr;
    private _myShellMgr;
    private _myFaceMgr;
    private _myWireMgr;
    private _myEdgeMgr;
    private _myVertMgr;
    private _mySubshapeMgr;
    /** Empty constructor. */
    constructor(theObjId?: ObjectId);
    /** Return shape info string. */
    get infoString(): string;
    /** Return initial root shape location WITHOUT considering local transformation of interactive object. */
    get shapeLocation(): ITrsf;
    /** Return multiplication of initial root shape location and local transformation of interactive object. */
    get shapeFinalLocation(): ITrsf;
    /** Return shape info. */
    get shapeInfo(): IShapeInfo;
    /** Return object's presentation mode. */
    get presentationMode(): ShapeObjectPrsMode;
    /** Return TRUE if object's material is set. */
    get hasMaterial(): boolean;
    /** Return object's material. */
    get material(): IVisMaterial;
    /** Return object's base color (overrides base implementation). */
    get baseColor(): ColorString;
    /** Return object's metallic factor within `[0.0, 1.0]` range. */
    get materialMetallic(): number;
    /** Return object's roughness within `[0.0, 1.0]` range. */
    get materialRoughness(): number;
    /** Return object's overcolor (forced color), which overrides base/subshape colors, or an empty string. */
    get forcedColor(): ColorString;
    /** Return object's overtransparency (forced transparency), which overrides base/subshape transparency, or -1 if unset. */
    get forcedTransparency(): number;
    /** Return index list of colored faces */
    get coloredFaces(): number[];
    /** Return index list of colored edges */
    get coloredEdges(): number[];
    /** Return index list of colored vertices */
    get coloredVertices(): number[];
    /** Return object's selection mode. */
    get selectionMode(): ShapeObjectSelMode;
    /** Return the last detected subshape index, or `-1` if unspecified. */
    get lastDetectedSubshape(): number;
    /** Return the last detected subshape info. */
    get lastDetectedSubshapeInfo(): IShapeInfo;
    /** Return the last detected subshape dump. */
    get lastDetectedSubshapeDump(): string;
    /**
     * Return the list with indices of selected subshapes of active selection mode
     * or empty list of no local selection is active / multiple selection modes are active.
     */
    get selectedSubshapes(): number[];
    /**
     * Return the list with selected subshapes info within active selection mode
     * or empty list of no local selection is active / multiple selection modes are active.
     */
    get selectedSubshapesInfo(): IShapeInfo[];
    /** Return the list with indices of available subshapes within active selection mode. */
    get definedSubshapes(): number[];
    /** Return subshape managers. */
    get subshapes(): SubshapeManager[];
    /** Return selection groups subshape manager. */
    get groups(): SubshapeManager;
    /** Return composite solid subshape manager. */
    get compSolids(): SubshapeManager;
    /** Return solid subshape manager. */
    get solids(): SubshapeManager;
    /** Return shell subshape manager. */
    get shells(): SubshapeManager;
    /** Return face subshape manager. */
    get faces(): SubshapeManager;
    /** Return wire subshape manager. */
    get wires(): SubshapeManager;
    /** Return edge subshape manager. */
    get edges(): SubshapeManager;
    /** Return vertex subshape manager. */
    get vertices(): SubshapeManager;
    /** Return composite solid subshape manager with interface for accessing to subshape by index. */
    get expCompSolids(): SubshapeManagerProxy;
    /** Return solid subshape manager with interface for accessing to subshape by index. */
    get expSolids(): SubshapeManagerProxy;
    /** Return shell subshape manager with interface for accessing to subshape by index. */
    get expShells(): SubshapeManagerProxy;
    /** Return face subshape manager with interface for accessing to subshape by index. */
    get expFaces(): SubshapeManagerProxy;
    /** Return wire subshape manager with interface for accessing to subshape by index. */
    get expWires(): SubshapeManagerProxy;
    /** Return edge subshape manager with interface for accessing to subshape by index. */
    get expEdges(): SubshapeManagerProxy;
    /** Return vertex subshape manager with interface for accessing to subshape by index. */
    get expVertices(): SubshapeManagerProxy;
    /** Shortcut to shape context. */
    private get shapeMgr();
    /**
     * Set initial root shape location WITHOUT changing local transformation of interactive object.
     * This method will recompute presentation of the shape (use localTransformation for interactive manipulations instead!).
     */
    set shapeLocation(theTrsf: ITrsf);
    /** Set object's presentation mode. */
    set presentationMode(theMode: ShapeObjectPrsMode);
    /** Set or unsets object's overcolor. */
    set forcedColor(theColor: ColorString);
    /** Set or unsets object's overtransparency. */
    set forcedTransparency(theTransp: number);
    /** Set object's material. */
    set material(theMat: IVisMaterial);
    /** Set object's base color (overrides base implementation). */
    set baseColor(theColor: ColorString);
    /** Set object's metallic factor. */
    set materialMetallic(theFactor: number);
    /** Set object's roughness. */
    set materialRoughness(theFactor: number);
    /** Set object's selection mode. */
    set selectionMode(theMode: ShapeObjectSelMode);
    /**
     * Set the list with indices of selected subhsapes of active selection mode
     * and resets previous selecction.
     */
    set selectedSubshapes(theSubshapes: number[]);
    /**
     * Probe file header for known shape formats.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to open
     * @param {string} theFilePath the file path to map
     * @return {string} detected format (if supported) or an empty string
     */
    static probeFileFormat(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): string;
    /** Get material preset {@link MaterialPreset}. */
    static getMaterialPreset(theViewer: SCwebviewer, thePreset: string): IVisMaterial;
    /** Get default shape material. */
    static getDefaultMaterial(theViewer: SCwebviewer): IVisMaterial;
    /** Set default shape material. */
    static setDefaultMaterial(theViewer: SCwebviewer, theMat: IVisMaterial): void;
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveShape | null;
    /**
     * Create single vertex shape.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt point definition
     */
    buildPoint(theViewer: SCwebviewer, thePnt: IVec3): void;
    /**
     * Create line segment edge shape.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 point one
     * @param {IVec3} thePnt2 point two
     */
    buildLineSegment(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3): void;
    /**
     * Create box shape from parameters (based on `BRepPreviewAPI_MakeBox` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IAx2}  theAx2  box lower corner (shape origin)
     * @param {IVec3} theDims box dimensions
     */
    buildBox(theViewer: SCwebviewer, theAx2: IAx2, theDims: IVec3): void;
    /**
     * Create sphere shape from parameters (based on `BRepPrimAPI_MakeSphere` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IAx2}   theAx2    sphere center (shape origin)
     * @param {number} theRadius sphere radius
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildSphere(theViewer: SCwebviewer, theAx2: IAx2, theRadius: number, theDevAngle?: number): void;
    /**
     * Create cylinder shape from parameters (based on `BRepPrimAPI_MakeCylinder` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {ICylinderParams} theParams cylinder parameters
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildCylinder(theViewer: SCwebviewer, theParams: ICylinderParams, theDevAngle?: number): void;
    /**
     * Create cone shape from parameters (based on `BRepPrimAPI_MakeCone` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IConeParams} theParams cone parameters
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildCone(theViewer: SCwebviewer, theParams: IConeParams, theDevAngle?: number): void;
    /**
     * Open object from the given URL and register presentation(s) in the hidden (erased) state.
     * If Interactive Object was created with empty id, it will be generated from the given URL.
     *
     * The file will be loaded asynchronously.
     * To remove file from file system you should call {@link SCwebviewer.unlinkFile} if it is no longer needed.
     *
     * Depending on the file format, this method might read
     * only a model structure without geometry - see {@link InteractiveShape.loadUrlAsync}.
     * Warning! Displaying an object with not yet loaded geometry will produce errors!
     *
     * @param {SCwebviewer} theViewer viewer module
     * @param {string} theFileUrl the file URL address
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    openUrlAsync(theViewer: SCwebviewer, theFileUrl: string): Promise<boolean>;
    /**
     * Open object from the given ArrayBuffer and register presentation(s) in the hidden (erased) state.
     * If Interactive Object was created with empty id, it will be generated from the given path.
     *
     * The file will be loaded asynchronously.
     * To remove file from file system you should
     * call {@link SCwebviewer.unlinkFile} if it is no longer needed.
     *
     * Depending on the file format, this method might read
     * only a model structure without geometry - see {@link InteractiveShape.loadUrlAsync}.
     * Warning! Displaying an object with not yet loaded geometry will produce errors!
     *
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to open
     * @param {string} theFilePath the file path to map
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    openBufferAsync(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): Promise<boolean>;
    /**
     * Load deferred shape data from file previously opened
     * by {@link InteractiveShape.openUrlAsync} or by opening new specified URL.
     * This method doesn't display the object in the Viewer - {@link InteractiveObject.isVisible}=true
     * should be called after loading data completion.
     * @param {SCwebviewer} theViewer viewer module
     * @param {string} theFileUrl the file URL address to read model from or empty string to load already opened model
     * @return {Promise<boolean>} FALSE if object was not found
     */
    loadUrlAsync(theViewer: SCwebviewer, theFileUrl?: string): Promise<boolean>;
    /**
     * Load deferred shape data from file previously opened
     * by {@link InteractiveShape.openBufferAsync} or by opening new specified buffer.
     * This method doesn't display the object
     * in the Viewer - {@link InteractiveObject.isVisible}=true should be called after loading data completion.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to read model from or null to load already opened model
     * @param {string} theFileName the file name to map
     * @return {Promise<boolean>} FALSE if object was not found
     */
    loadBufferAsync(theViewer: SCwebviewer, theArray?: ArrayBuffer | null, theFileName?: string): Promise<boolean>;
    /**
     * Explode current shape to sub-shapes.
     * Current object will be removed in scope of this method.
     * @param {SCwebviewer} theViewer viewer module
     * @param {number} theExplodeDepth depth of explode process (should be >= -1, default value is -1 - explode to maximal possible depth)
     * @param {boolean} theIsAutoNaming generate id by shape type ("compound", "solid", etc) depending on parents in case of attributes absence
     * @param {boolean} theToGenId generate unique ids or use initial ones in case of existing attributes
     * @param {boolean} theToPropagateId generate id depending on parents in case of existing attributes
     * @param {string[]} theNewShapeIds suggested ids for new objects or empty list to generate them
     * @return {InteractiveObject[]} exploded interactive objects
     */
    explodeShape(theViewer: SCwebviewer, theExplodeDepth?: number, theIsAutoNaming?: boolean, theToGenId?: boolean, theToPropagateId?: boolean, theNewShapeIds?: string[]): InteractiveShape[];
    /** Dump shape into a file. */
    save(theFile?: string): boolean;
    /** Set material preset {@link MaterialPreset} to object. */
    setMaterialPreset(thePreset: string): void;
    /** Return position of vertex subshape of specified index */
    getSubshapeVertex(theVertId: number): IVec3;
    /** Return face color or empty string */
    getFaceColor(theIndex: number): ColorString;
    /** Set or unset face color. */
    setFaceColor(theIndex: number, theColor: ColorString): void;
    /** Set or unset faces color; applied to all faces when null is passed instead of index list */
    setFacesColor(theIndices: number[] | null, theColor: ColorString): void;
    /** Unset faces' colors */
    clearFaceColors(): void;
    /** Return edge color or empty string */
    getEdgeColor(theIndex: number): ColorString;
    /** Set or unset edge color */
    setEdgeColor(theIndex: number, theColor: ColorString): void;
    /** Set or unset edges color; applied to all edges when null is passed instead of index list */
    setEdgesColor(theIndices: number[] | null, theColor: ColorString): void;
    /** Unset edges' colors */
    clearEdgeColors(): void;
    /** Return vertex color or empty string */
    getVertexColor(theIndex: number): ColorString;
    /** Set or unset vertex color */
    setVertexColor(theIndex: number, theColor: ColorString): void;
    /** Set or unset vertices color; applied to all vertices when null is passed instead of index list */
    setVerticesColor(theIndices: number[] | null, theColor: ColorString): void;
    /** Unset vertex' colors */
    clearVertexColors(): void;
    /**
     * Remove all selection groups.
     * Tip: do NOT activate group selection mode while managing groups list!
     */
    clearGroups(): void;
    /**
     * Add selection group pointint to the list of subshapes of specified type.
     * Tip: do NOT activate group selection mode while managing groups list!
     */
    addGroup(theElemType: ShapeObjectSelMode, theIndexList: number[]): number;
    /**
     * Return the list with indices of available subshapes.
     * @param theElemType subshape type to explore
     * @param theIsFree if TRUE, only free subshapes will be considered
     * @param theParentType type of a parent shape or Global to explore all roots
     * @param theParentIndex index of a parent shape or -1 to explore all roots
     * @param theVisibility return only visible (1), hidden (0) or subshape with any visibility attribute
     * @return index list of subshapes
     */
    getSubshapesIndexes(theElemType: ShapeObjectSelMode, theIsFree?: boolean, theParentType?: ShapeObjectSelMode, theParentIndex?: number, theVisibility?: number): number[];
    /** Clear selected faces/edges/vertices. */
    clearSelectedElems(): void;
    /** Set the list with indices of selected faces/edges/vertices. */
    setSelectedElems(theFaces: number[], theEdges: number[], theVertices: number[]): void;
    /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
    getConnectedSubshapes(theElemType: ShapeObjectSelMode, theIndexList: number[]): number[];
    /** Parse index list from the string. */
    indexArrayFromString(theIndices: string): number[];
    /** Format index list into a string. */
    indexArrayToString(theIndices: number[]): string;
    /** Open shape from file path. */
    private openShapeFromFile;
}
export {};
