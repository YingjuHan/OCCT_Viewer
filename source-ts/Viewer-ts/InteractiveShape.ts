import type {IManager, ColorString, IVisMaterial, IVec3, IAx2,
             ObjectId, IShapeInfo, ICylinderParams, IConeParams, ITrsf} from './CommonTools.js';
import {Vec3, ShapeInfo, Trsf, ShapeEnum} from './CommonTools.js';
import type {IDetectedShapeElem, ISelectionShapeElems} from './InteractiveObject.js';
import {InteractiveObject} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * Material presets defined by `Graphic3d_NameOfMaterial` enumeration.
 */
export const MaterialPreset = {
  Brass: 'Brass',
  Bronze: 'Bronze',
  Copper: 'Copper',
  Gold: 'Gold',
  Pewter: 'Pewter',
  Plastered: 'Plastered',
  Plastified: 'Plastified',
  Silver: 'Silver',
  Steel: 'Steel',
  Stone: 'Stone',
  ShinyPlastified: 'Shiny_plastified',
  Satin: 'Satined',
  Metalized: 'Metalized',
  Ionized: 'Ionized',
  Chrome: 'Chrome',
  Aluminum: 'Aluminium',
  Obsidian: 'Obsidian',
  Neon: 'Neon',
  Jade: 'Jade',
  Charcoal: 'Charcoal',
  Water: 'Water',
  Glass: 'Glass',
  Diamond: 'Diamond',
  Transparent: 'Transparent',
  Default: 'Default',
};

/**
 * ShapeObjectPrsMode represents presentation mode of {@link InteractiveShape}, from `InteractiveShapePrsMode`.
 */
export enum ShapeObjectPrsMode {
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
};

/**
 * ShapeObjectSelMode represents selection mode of {@link InteractiveShape}, from `InteractiveShapeSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export enum ShapeObjectSelMode {
  /** Selection of whole shape object. */
  Global = 0x0000,
  /** Selection of shape vertices. */
  Vertex = 0x0001,
  /** Selection of shape edges. */
  Edge = 0x0002,
  /** Selection of shape wires. */
  Wire = 0x0004,
  /** Selection of shape faces. */
  Face = 0x0008,
  /** Selection of shape shells. */
  Shell = 0x0010,
  /** Selection of shape solids. */
  Solid = 0x0020,
  /** Selection of shape compsolids. */
  CompSolid = 0x0040,
  /** Selection of groups. */
  Group = 0x0080
};

export interface IShapeMassProps {
  area: number;
  volume: number | null;
  centroid: IVec3;
  centroidSource: 'surface' | 'volume';
  volumeValid: boolean;
};

export class ShapeMassProps {
  public static parseJson(theJson: string): IShapeMassProps | null {
    if (!theJson) {
      return null;
    }

    let aParsed: any;
    try {
      aParsed = JSON.parse(theJson);
    } catch {
      return null;
    }

    if (!aParsed || !aParsed.centroid) {
      return null;
    }

    const aCentroid = aParsed.centroid;
    if (typeof aCentroid.x !== 'number' || typeof aCentroid.y !== 'number' || typeof aCentroid.z !== 'number') {
      return null;
    }

    const volumeValid = aParsed.volumeValid === true || typeof aParsed.volume === 'number';
    const volume = volumeValid && typeof aParsed.volume === 'number' ? aParsed.volume : null;
    const centroidSource = aParsed.centroidSource === 'volume' ? 'volume' : 'surface';

    return {
      area: typeof aParsed.area === 'number' ? aParsed.area : Number(aParsed.area ?? 0),
      volume,
      centroid: {x: aCentroid.x, y: aCentroid.y, z: aCentroid.z},
      centroidSource,
      volumeValid,
    };
  }
};

/**
 * @internal Class to define detected/selected shape elements.
 */
export class SelectionShapeElems implements ISelectionShapeElems {
  groups: string = '';
  compSolids: string = '';
  solids: string = '';
  shells: string = '';
  faces: string = '';
  wires: string = '';
  edges: string = '';
  verts: string = '';

  /** Convert selection mode to shape enum. */
  public static selModeToShapeEnum(theType: ShapeObjectSelMode): ShapeEnum {
    switch (theType) {
      case ShapeObjectSelMode.Global: return ShapeEnum.SHAPE;
      case ShapeObjectSelMode.Group: return ShapeEnum.SHAPE;
      case ShapeObjectSelMode.CompSolid: return ShapeEnum.COMPSOLID;
      case ShapeObjectSelMode.Solid: return ShapeEnum.SOLID;
      case ShapeObjectSelMode.Shell: return ShapeEnum.SHELL;
      case ShapeObjectSelMode.Face: return ShapeEnum.FACE;
      case ShapeObjectSelMode.Wire: return ShapeEnum.WIRE;
      case ShapeObjectSelMode.Edge: return ShapeEnum.EDGE;
      case ShapeObjectSelMode.Vertex: return ShapeEnum.VERTEX;
      default: return ShapeEnum.SHAPE;
    }
  }

  /** Convert shape enum to selection mode. */
  public static shapeEnumToSelMode(theType: ShapeEnum): ShapeObjectSelMode {
    switch (theType) {
      case ShapeEnum.SHAPE: return ShapeObjectSelMode.Global;
      case ShapeEnum.COMPSOLID: return ShapeObjectSelMode.CompSolid;
      case ShapeEnum.SOLID: return ShapeObjectSelMode.Solid;
      case ShapeEnum.SHELL: return ShapeObjectSelMode.Shell;
      case ShapeEnum.FACE: return ShapeObjectSelMode.Face;
      case ShapeEnum.WIRE: return ShapeObjectSelMode.Wire;
      case ShapeEnum.EDGE: return ShapeObjectSelMode.Edge;
      case ShapeEnum.VERTEX: return ShapeObjectSelMode.Vertex;
      default: return ShapeObjectSelMode.Global;
    }
  }

  /** Get indices of specified type. */
  public static getIndices(theElems: ISelectionShapeElems, theType: ShapeObjectSelMode): number[] {
    return SelectionShapeElems.indexArrayFromString(SelectionShapeElems.getIndicesString(theElems, theType));
  }

  /** Set indices of specified type. */
  public static setIndices(theElems: ISelectionShapeElems, theType: ShapeObjectSelMode, theIds: number[]): void {
    SelectionShapeElems.setIndicesString(theElems, theType, SelectionShapeElems.indexArrayToString(theIds));
  }

  /** Get indices of specified type. */
  public static getIndicesString(theElems: ISelectionShapeElems, theType: ShapeObjectSelMode): string {
    switch (theType) {
      case ShapeObjectSelMode.Group: return theElems.groups;
      case ShapeObjectSelMode.CompSolid: return theElems.compSolids;
      case ShapeObjectSelMode.Solid: return theElems.solids;
      case ShapeObjectSelMode.Shell: return theElems.shells;
      case ShapeObjectSelMode.Face: return theElems.faces;
      case ShapeObjectSelMode.Wire: return theElems.wires;
      case ShapeObjectSelMode.Edge: return theElems.edges;
      case ShapeObjectSelMode.Vertex: return theElems.verts;
      default: return '';
    }
  }

  /** Set indices of specified type. */
  public static setIndicesString(theElems: ISelectionShapeElems, theType: ShapeObjectSelMode, theIds: string): void {
    switch (theType) {
      case ShapeObjectSelMode.Group: theElems.groups = theIds; break;
      case ShapeObjectSelMode.CompSolid: theElems.compSolids = theIds; break;
      case ShapeObjectSelMode.Solid: theElems.solids = theIds; break;
      case ShapeObjectSelMode.Shell: theElems.shells = theIds; break;
      case ShapeObjectSelMode.Face: theElems.faces = theIds; break;
      case ShapeObjectSelMode.Wire: theElems.wires = theIds; break;
      case ShapeObjectSelMode.Edge: theElems.edges = theIds; break;
      case ShapeObjectSelMode.Vertex: theElems.verts = theIds; break;
      default: break;
    }
  }

  /** Parse index list from the string. */
  public static indexArrayFromString(theIndices: string): number[] {
    let anElemIdsArray: number[] = [];
    const aSelIdsArr = theIndices.length !== 0 ? theIndices.split('\n') : [];
    for (const aSelIdIter of aSelIdsArr) {
      anElemIdsArray.push(parseInt(aSelIdIter, 10));
    }
    return anElemIdsArray;
  }

  /** Format index list into a string. */
  public static indexArrayToString(theIndices: number[]): string {
      let anElemIds = '';
      for (const anIdIter of theIndices) {
        anElemIds += anElemIds.length !== 0 ? '\n' : '';
        anElemIds += anIdIter.toString(10);
      }
      return anElemIds;
  }

  /** Get indices of specified type. */
  public getIndices(theType: ShapeObjectSelMode): number[] {
    return SelectionShapeElems.indexArrayFromString(SelectionShapeElems.getIndicesString(this, theType));
  }

  /** Set indices of specified type. */
  public setIndices(theType: ShapeObjectSelMode, theIds: number[]): void {
    SelectionShapeElems.setIndicesString(this, theType, SelectionShapeElems.indexArrayToString(theIds));
  }
};

/**
 * Auxiliary class for accessing InteractiveShape subshapes.
 */
export class SubshapeManager {
  private myShapePrs: InteractiveShape;
  private myShapeType: ShapeObjectSelMode;

  /** Constructor. */
  constructor(thePrs: InteractiveShape, theType: ShapeObjectSelMode) {
    this.myShapePrs = thePrs;
    this.myShapeType = theType;
  }

  /** Iteratable interface. */
  *[Symbol.iterator](): IterableIterator<IShapeInfo> {
    const aLen = this.length;
    for (let i = 0; i < aLen; ++i) {
      yield this.getShapeInfo(Number(i));
    }
  }

  /** Return shape type. */
  public get shapeType(): ShapeObjectSelMode { return this.myShapeType; }

  /** Return the number of available subshapes. */
  public get length(): number { return this.defined.length; }

  /** Return the list with indices of available subshapes. */
  public get defined(): number[] { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false); }

  /** Return the list with indices of free subshapes. */
  public get free(): number[] { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, true); }

  /** Return the list with indices of disabled subshapes. */
  public get disabled(): number[] { return this.myShapePrs.getDisabledSubshapes(this.myShapeType, false); }

  /** Return the list with indices of enabled subshapes. */
  public get enabled(): number[] { return this.myShapePrs.getDisabledSubshapes(this.myShapeType, true); }

  /** Return the list with indices of selected subshapes */
  public get selected(): number[] { return this.myShapePrs.getSelectedSubshapes(this.myShapeType); }

  /** Return index list of colored subshapes. */
  public get colored(): number[] { return this.myShapePrs.getColoredSubshapes(this.myShapeType); }

  /** Return the last detected subshape index, or `-1` if unspecified. */
  public get lastDetected(): number {
    const aShape: IDetectedShapeElem | undefined = this.myShapePrs.getDetectedShapeElem();
    return aShape && aShape.entityType === this.myShapeType ? aShape.entityId : -1;
  }

  /** Return subshape info. */
  public get shapeInfo(): IShapeInfo[] {
    return this.myShapePrs.getSubshapesInfo(this.myShapeType, this.defined, true);
  }

  /** Return index list of visible subshapes. */
  public get visible(): number[] { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false, ShapeObjectSelMode.Global, -1, 1); }

  /** Return index list of hidden subshapes. */
  public get hidden(): number[] { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false, ShapeObjectSelMode.Global, -1, 0); }

  /** Set the list with indices of disabled subshapes. */
  public set disabled(theList: number[]) { this.myShapePrs.setDisabledSubshapes(this.myShapeType, false, theList); }

  /** Set the list with indices of enabled subshapes. */
  public set enabled(theList: number[]) { this.myShapePrs.setDisabledSubshapes(this.myShapeType, true, theList); }

  /** Set the list with indices of selected subshapes of this type but keeps selection of subshapes of other types. */
  public set selected(theList: number[]) {
    let aSelElemIds: ISelectionShapeElems = this.myShapePrs.getSelectedElems();
    SelectionShapeElems.setIndices(aSelElemIds, this.myShapeType, theList);
    this.myShapePrs.setSelectedShapeElems(aSelElemIds);
  }

  /** Set the list with indices of selected subshapes and resets selection of all other subshape types. */
  public set selectedOnly(theList: number[]) {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(this.myShapeType, theList);
    this.myShapePrs.setSelectedShapeElems(aSelElemIds);
  }

  /** Set index list of visible subshapes (makes hidden unspecified subshapes). */
  public set visible(theList: number[]) { this.setSubshapesHidden(theList, false, true); }

  /** Set index list of hidden subshapes (makes visible unspecified subshapes). */
  public set hidden(theList: number[]) { this.setSubshapesHidden(theList, true, true); }

  /** Return indexes of subshapes of specified subshape. */
  public getSubshapesIndexes(theIndex: number, theSubType: ShapeObjectSelMode): number[] {
    return this.myShapePrs.getSubshapesIndexes(theSubType, false, this.myShapeType, theIndex);
  }

  /** Return the subshape info. */
  public getShapeInfo(theIndex: number, theToExplore: boolean = true): IShapeInfo {
    return this.myShapePrs.getSubshapeInfo(this.myShapeType, theIndex, theToExplore);
  }

  /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
  public getConnectedSubshapes(theIndexList: number[]): number[] {
    return this.myShapePrs.getConnectedSubshapes(this.myShapeType, theIndexList);
  }

  /** Return subshape color or empty string */
  public getSubshapeColor(theIndex: number): ColorString {
    return this.myShapePrs.getSubshapeColor(this.myShapeType, theIndex);
  }

  /** Set or unset subhsape color. */
  public setSubshapeColor(theIndex: number, theColor: ColorString): void {
    this.myShapePrs.setSubshapeColor(this.myShapeType, theIndex.toString(10), theColor);
  }

  /** Set or unset subhsapes color; applied to all subhsapes when null is passed instead of index list */
  public setSubshapesColor(theIndices: number[] | null, theColor: ColorString): void {
    this.myShapePrs.setSubshapeColor(this.myShapeType,
                                     theIndices != null ? this.myShapePrs.indexArrayToString(theIndices) : '*',
                                     theColor);
  }

  /** Unset subshapes colors */
  public clearSubshapeColors(): void { this.myShapePrs.setSubshapeColor(this.myShapeType, '*', ''); }

  /** Return subshape hidden flag. */
  public isSubshapeHidden(theIndex: number): boolean {
    return this.myShapePrs.isSubshapeHidden(this.myShapeType, theIndex);
  }

  /** Set or unset subhsape hidden flag. */
  public setSubshapeHidden(theIndex: number, theIsHidden: boolean): void {
    this.myShapePrs.setSubshapeHidden(this.myShapeType, theIndex.toString(10), theIsHidden, false);
  }

  /** Set or unset subhsapes hidden flag; applied to all subshapes when null is passed instead of index list */
  public setSubshapesHidden(theIndices: number[] | null, theIsHidden: boolean, theToInvertOthers: boolean = false): void {
    this.myShapePrs.setSubshapeHidden(this.myShapeType,
                                      theIndices != null ? this.myShapePrs.indexArrayToString(theIndices) : '*',
                                      theIsHidden, theToInvertOthers);
  }

  /** Unset subshapes hidden flag. */
  public clearSubshapeHidden(): void { this.myShapePrs.setSubshapeHidden(this.myShapeType, '*', false, false); }

  /** Make a copy of existing object with new id and specified subshape. */
  public cloneSubshape(theIndex: number, theNewObjId: ObjectId = ''): InteractiveShape | null {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(this.myShapeType, [theIndex]);
    return this.myShapePrs.cloneSubshapes(aSelElemIds, theNewObjId);
  }

  /** Make a copy of existing object with new id and specified subshapes. */
  public cloneSubshapes(theIndices: number[], theNewObjId: ObjectId = ''): InteractiveShape | null {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(this.myShapeType, theIndices);
    return this.myShapePrs.cloneSubshapes(aSelElemIds, theNewObjId);
  }

  /** Dump shape into a file. */
  public saveSubshape(theIndex: number, theFile: string = ''): boolean {
    let anIds: number[] = [theIndex];
    return this.saveSubshapes(anIds, theFile);
  };

  /** Dump shapes into a file. */
  public saveSubshapes(theIndices: number[], theFile: string = ''): boolean {
    let aFilePath = theFile;
    if (aFilePath === '' && this.myShapePrs.id !== '') {
      let aSuff = '';
      switch (this.myShapeType) {
        case ShapeObjectSelMode.Group: aSuff = 'g'; break;
        case ShapeObjectSelMode.CompSolid: aSuff = 'cs'; break;
        case ShapeObjectSelMode.Solid: aSuff = 'so'; break;
        case ShapeObjectSelMode.Shell: aSuff = 'sh'; break;
        case ShapeObjectSelMode.Face: aSuff = 'f'; break;
        case ShapeObjectSelMode.Wire: aSuff = 'w'; break;
        case ShapeObjectSelMode.Edge: aSuff = 'e'; break;
        case ShapeObjectSelMode.Vertex: aSuff = 'v'; break;
        default: break;
      }

      let anIdSuff = theIndices.length === 1 ? theIndices[0].toString() : '';
      aFilePath = `${this.myShapePrs.id}_${aSuff}${anIdSuff}.brep`;
    }

    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(this.myShapeType, theIndices);
    return this.myShapePrs.saveSubshapes(aFilePath, aSelElemIds);
  };
};

/**
 * Proxy interface to access subshapes information
 * using array-alike interface (by index)
 * without instantiating a real array for all subshapes.
 */
class SubshapeManagerProxy extends Array<IShapeInfo> {
  private _myProxy: SubshapeManager;

  /** Main constructor. */
  constructor(theProxy: SubshapeManager) {
    super();
    this._myProxy = theProxy;
    return new Proxy(this, {
      get: (theObj, theKey) => {
        if (typeof(theKey) === 'string' && (Number.isInteger(Number(theKey)))) {
          return theObj._myProxy.getShapeInfo(Number(theKey));
        }
        return theObj._myProxy[theKey];
      },
    });
  }
};

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
export class InteractiveShape extends InteractiveObject {
  private _myGroupsMgr: SubshapeManager | undefined;
  private _myCompSolidMgr: SubshapeManager | undefined;
  private _mySolidMgr: SubshapeManager | undefined;
  private _myShellMgr: SubshapeManager | undefined;
  private _myFaceMgr: SubshapeManager | undefined;
  private _myWireMgr: SubshapeManager | undefined;
  private _myEdgeMgr: SubshapeManager | undefined;
  private _myVertMgr: SubshapeManager | undefined;
  private _mySubshapeMgr: SubshapeManager[] | undefined;

  /** Empty constructor. */
  constructor(theObjId: ObjectId = '') {
    super(theObjId);
  }

  /** Return shape info string. */
  public get infoString(): string {
    const aTris: IShapeInfo = this.shapeInfo;
    const aLoc: string = !Trsf.isIdentity(aTris.location) ? Trsf.format(aTris.location, this._myViewer?.unitManager ?? null) : 'IDENTITY';

    let anInfo: string =
`Vertices:  ${this.vertices.length.toString()} (free: ${this.vertices.free.length.toString()})
Edges:     ${this.edges.length.toString()} (free: ${this.edges.free.length.toString()})
Wires:     ${this.wires.length.toString()}
Faces:     ${this.faces.length.toString()} (free: ${this.faces.free.length.toString()})
Shells:    ${this.shells.length.toString()}
Solids:    ${this.solids.length.toString()}
CompSolids:${this.compSolids.length.toString()}
Groups:    ${this.groups.length.toString()}
Triangles: ${aTris.nbTriangles.toString()}
Nodes:     ${aTris.nbTriNodes.toString()}
Location:  ${aLoc}
`;
    return anInfo;
  }

  /** Return initial root shape location WITHOUT considering local transformation of interactive object. */
  public get shapeLocation(): ITrsf {
    return this.getSubshapeInfo(ShapeObjectSelMode.Global, 0, false).location;
  }

  /** Return multiplication of initial root shape location and local transformation of interactive object. */
  public get shapeFinalLocation(): ITrsf {
    const aTrsfSh:  ITrsf = this.shapeLocation;
    const aTrsfPrs: ITrsf = this.localTransformation;
    const aTrsfRes: ITrsf = Trsf.multiply(aTrsfSh, aTrsfPrs);
    return aTrsfRes;
  }

  /** Return shape info. */
  public get shapeInfo(): IShapeInfo { return this.getSubshapeInfo(ShapeObjectSelMode.Global, 0, true); }

  /** Return object's presentation mode. */
  public get presentationMode(): ShapeObjectPrsMode { return this.shapeMgr?.getPresentationMode(this._myObjId) ?? ShapeObjectPrsMode.Shaded; }

  /** Return TRUE if object's material is set. */
  public get hasMaterial(): boolean {
    const aMat = this.material;
    return aMat.isDefined && aMat.baseColor !== '';
  }

  /** Return object's material. */
  public get material(): IVisMaterial {
    return this.shapeMgr?.getMaterial(this._myObjId) ??
           { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
  }

  /** Return object's base color (overrides base implementation). */
  public get baseColor(): ColorString { return this.material.baseColor; }

  /** Return object's metallic factor within `[0.0, 1.0]` range. */
  public get materialMetallic(): number { return this.material.metallic; }

  /** Return object's roughness within `[0.0, 1.0]` range. */
  public get materialRoughness(): number { return this.material.roughness; }

  /** Return object's overcolor (forced color), which overrides base/subshape colors, or an empty string. */
  public get forcedColor(): ColorString { return this.shapeMgr?.getForcedColor(this._myObjId) ?? ''; }

  /** Return object's overtransparency (forced transparency), which overrides base/subshape transparency, or -1 if unset. */
  public get forcedTransparency(): number { return this.shapeMgr?.getForcedTransparency(this._myObjId) ?? -1.0; }

  /** Return index list of colored faces */
  public get coloredFaces(): number[] { return this.getColoredSubshapes(ShapeObjectSelMode.Face); }

  /** Return index list of colored edges */
  public get coloredEdges(): number[] { return this.getColoredSubshapes(ShapeObjectSelMode.Edge); }

  /** Return index list of colored vertices */
  public get coloredVertices(): number[] { return this.getColoredSubshapes(ShapeObjectSelMode.Vertex); }

  /** Return object's selection mode. */
  public get selectionMode(): ShapeObjectSelMode {
    return this._myViewer?.selectionManager?.getSelectionMode(this._myObjId) ?? ShapeObjectSelMode.Global;
  }

  /** Return the last detected subshape index, or `-1` if unspecified. */
  public get lastDetectedSubshape(): number {
    const aShape: IDetectedShapeElem | undefined = this._myViewer?.selectionManager?.getDetectedShapeElem(this._myObjId);
    return aShape && aShape.entityType === this.selectionMode ?
           aShape.entityId :
           -1;
  }

  /** Return the last detected subshape info. */
  public get lastDetectedSubshapeInfo(): IShapeInfo {
    const aShape: IDetectedShapeElem | undefined = this._myViewer?.selectionManager?.getDetectedShapeElem(this._myObjId);
    return aShape && aShape.entityId !== -1 ?
           this.getSubshapeInfo(aShape.entityType, aShape.entityId, true) ?? new ShapeInfo() :
           new ShapeInfo();
  }

  /** Return the last detected subshape dump. */
  public get lastDetectedSubshapeDump(): string {
    const anId: number = this.lastDetectedSubshape;
    return anId !== -1 ?
           this.shapeMgr?.getSubshapeDump(this._myObjId, this.selectionMode, anId) ?? '' :
           '';
  }

  /**
   * Return the list with indices of selected subshapes of active selection mode
   * or empty list of no local selection is active / multiple selection modes are active.
   */
  public get selectedSubshapes(): number[] {
    return this.selectionMode !== ShapeObjectSelMode.Global ?
           this.getSelectedSubshapes(this.selectionMode) :
           [];
  }

  /**
   * Return the list with selected subshapes info within active selection mode
   * or empty list of no local selection is active / multiple selection modes are active.
   */
  public get selectedSubshapesInfo(): IShapeInfo[] {
    if (this.selectionMode === ShapeObjectSelMode.Global || this.shapeMgr == null) {
      return [];
    }

    let anInfoArray = new Array<IShapeInfo>();
    for (let aTypeIter of [ShapeObjectSelMode.Group,
                           ShapeObjectSelMode.CompSolid, ShapeObjectSelMode.Solid, ShapeObjectSelMode.Shell, ShapeObjectSelMode.Face,
                           ShapeObjectSelMode.Wire, ShapeObjectSelMode.Edge, ShapeObjectSelMode.Vertex]) {
      const aSubShapes: number[] = this.getSelectedSubshapes(aTypeIter);
      const anInfos: IShapeInfo[] = this.getSubshapesInfo(aTypeIter, aSubShapes, true);
      anInfoArray = anInfoArray.concat(anInfos);
    }
    return anInfoArray;
  }

  /** Return the list with indices of available subshapes within active selection mode. */
  public get definedSubshapes(): number[] {
    return this.selectionMode !== ShapeObjectSelMode.Global ?
           this.getSubshapesIndexes(this.selectionMode, false) :
           [];
  }

  /** Return subshape managers. */
  public get subshapes(): SubshapeManager[] {
    if (!this._mySubshapeMgr) {
      this._mySubshapeMgr = [this.groups, this.compSolids, this.solids, this.shells, this.faces, this.wires, this.edges, this.vertices];
    }
    return this._mySubshapeMgr;
  }

  /** Return selection groups subshape manager. */
  public get groups(): SubshapeManager {
    if (!this._myGroupsMgr) { this._myGroupsMgr = new SubshapeManager(this, ShapeObjectSelMode.Group); }
    return this._myGroupsMgr;
  }

  /** Return composite solid subshape manager. */
  public get compSolids(): SubshapeManager {
    if (!this._myCompSolidMgr) { this._myCompSolidMgr = new SubshapeManager(this, ShapeObjectSelMode.CompSolid); }
    return this._myCompSolidMgr;
  }

  /** Return solid subshape manager. */
  public get solids(): SubshapeManager {
    if (!this._mySolidMgr) { this._mySolidMgr = new SubshapeManager(this, ShapeObjectSelMode.Solid); }
    return this._mySolidMgr;
  }

  /** Return shell subshape manager. */
  public get shells(): SubshapeManager {
    if (!this._myShellMgr) { this._myShellMgr = new SubshapeManager(this, ShapeObjectSelMode.Shell); }
    return this._myShellMgr;
  }

  /** Return face subshape manager. */
  public get faces(): SubshapeManager {
    if (!this._myFaceMgr) { this._myFaceMgr = new SubshapeManager(this, ShapeObjectSelMode.Face); }
    return this._myFaceMgr;
  }

  /** Return wire subshape manager. */
  public get wires(): SubshapeManager {
    if (!this._myWireMgr) { this._myWireMgr = new SubshapeManager(this, ShapeObjectSelMode.Wire); }
    return this._myWireMgr;
  }

  /** Return edge subshape manager. */
  public get edges(): SubshapeManager {
    if (!this._myEdgeMgr) { this._myEdgeMgr = new SubshapeManager(this, ShapeObjectSelMode.Edge); }
    return this._myEdgeMgr;
  }

  /** Return vertex subshape manager. */
  public get vertices(): SubshapeManager {
    if (!this._myVertMgr) { this._myVertMgr = new SubshapeManager(this, ShapeObjectSelMode.Vertex); }
    return this._myVertMgr;
  }

  /** Return composite solid subshape manager with interface for accessing to subshape by index. */
  public get expCompSolids(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.compSolids); }

  /** Return solid subshape manager with interface for accessing to subshape by index. */
  public get expSolids(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.solids); }

  /** Return shell subshape manager with interface for accessing to subshape by index. */
  public get expShells(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.shells); }

  /** Return face subshape manager with interface for accessing to subshape by index. */
  public get expFaces(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.faces); }

  /** Return wire subshape manager with interface for accessing to subshape by index. */
  public get expWires(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.wires); }

  /** Return edge subshape manager with interface for accessing to subshape by index. */
  public get expEdges(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.edges); }

  /** Return vertex subshape manager with interface for accessing to subshape by index. */
  public get expVertices(): SubshapeManagerProxy { return new SubshapeManagerProxy(this.vertices); }

  /** Shortcut to shape context. */
  private get shapeMgr(): InteractiveShapeManager | null { return this._myViewer?.interactiveShapeManager ?? null; }

  /** @internal Dump shape into a file. */
  private get doDump(): string {
    let aFilePath = `${this._myObjId}.brep`;
    return this.save(aFilePath) ? aFilePath : '';
  }

  /**
   * Set initial root shape location WITHOUT changing local transformation of interactive object.
   * This method will recompute presentation of the shape (use localTransformation for interactive manipulations instead!).
   */
  public set shapeLocation(theTrsf: ITrsf) {
    this.shapeMgr?.setShapeLocation(this._myObjId, theTrsf);
  }

  /** Set object's presentation mode. */
  public set presentationMode(theMode: ShapeObjectPrsMode) { this.shapeMgr?.setPresentationMode(this._myObjId, theMode); }

  /** Set or unsets object's overcolor. */
  public set forcedColor(theColor: ColorString) { this.shapeMgr?.setForcedColor(this._myObjId, theColor); }

  /** Set or unsets object's overtransparency. */
  public set forcedTransparency(theTransp: number) { this.shapeMgr?.setForcedTransparency(this._myObjId, theTransp); }

  /** Set object's material. */
  public set material(theMat: IVisMaterial) { this.shapeMgr?.setMaterial(this._myObjId, theMat); }

  /** Set object's base color (overrides base implementation). */
  public set baseColor(theColor: ColorString) {
    let aMat = this.material;
    aMat.isDefined = true;
    aMat.baseColor = theColor;
    this.material = aMat;
  }

  /** Set object's metallic factor. */
  public set materialMetallic(theFactor: number) {
    let aMat = this.material;
    aMat.isDefined = true;
    aMat.metallic = theFactor;
    this.material = aMat;
  }

  /** Set object's roughness. */
  public set materialRoughness(theFactor: number) {
    let aMat = this.material;
    aMat.isDefined = true;
    aMat.roughness = theFactor;
    this.material = aMat;
  }

  /** Set object's selection mode. */
  public set selectionMode(theMode: ShapeObjectSelMode) { this._myViewer?.selectionManager?.setSelectionMode(this._myObjId, theMode); }

  /**
   * Set the list with indices of selected subhsapes of active selection mode
   * and resets previous selecction.
   */
  public set selectedSubshapes(theSubshapes: number[]) {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(this.selectionMode, theSubshapes);
    this.setSelectedShapeElems(aSelElemIds);
  }

  /**
   * Probe file header for known shape formats.
   * @param {SCwebviewer} theViewer viewer module
   * @param {ArrayBuffer} theArray buffer to open
   * @param {string} theFilePath the file path to map
   * @return {string} detected format (if supported) or an empty string
   */
  public static probeFileFormat(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): string {
    let aHeader: Uint8Array = new Uint8Array(theArray);
    aHeader = aHeader.subarray(0, Math.min(aHeader.length, 4096));
    const aDataBuffer = theViewer._allocateArrayU8(aHeader);
    return theViewer.interactiveShapeManager?.probeFileFormat(theFilePath, aDataBuffer, aHeader.length, true) ?? '';
  }

  /** Get material preset {@link MaterialPreset}. */
  public static getMaterialPreset(theViewer: SCwebviewer, thePreset: string): IVisMaterial {
    return theViewer.interactiveShapeManager?.getMaterialPreset(thePreset) ??
           { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
  }

  /** Get default shape material. */
  public static getDefaultMaterial(theViewer: SCwebviewer): IVisMaterial {
    return theViewer.interactiveShapeManager?.getDefaultMaterial() ??
           { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
  }

  /** Set default shape material. */
  public static setDefaultMaterial(theViewer: SCwebviewer, theMat: IVisMaterial): void {
    return theViewer.interactiveShapeManager?.setDefaultMaterial(theMat);
  }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveShape | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveShape();
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr.addInteractiveObject(aCopy);
    return aCopy;
  }

  /**
   * Create single vertex shape.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec3} thePnt point definition
   */
  public buildPoint(theViewer: SCwebviewer, thePnt: IVec3): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('point') ?? 'point'; }
    this.shapeMgr?.buildPoint(this._myObjId, thePnt);
    this.objMgr?.addInteractiveObject(this);
  }

  /**
   * Create line segment edge shape.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec3} thePnt1 point one
   * @param {IVec3} thePnt2 point two
   */
  public buildLineSegment(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('segment') ?? 'segment'; }
    this.shapeMgr?.buildLineSegment(this._myObjId, thePnt1, thePnt2);
    this.objMgr?.addInteractiveObject(this);
  }

  /**
   * Create box shape from parameters (based on `BRepPreviewAPI_MakeBox` from C++ OCCT).
   * @param {SCwebviewer} theViewer viewer module
   * @param {IAx2}  theAx2  box lower corner (shape origin)
   * @param {IVec3} theDims box dimensions
   */
  public buildBox(theViewer: SCwebviewer,
                  theAx2: IAx2, theDims: IVec3): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('box') ?? 'box'; }
    this.shapeMgr?.buildBox(this._myObjId, theAx2, theDims);
    this.objMgr?.addInteractiveObject(this);
  }

  /**
   * Create sphere shape from parameters (based on `BRepPrimAPI_MakeSphere` from C++ OCCT).
   * @param {SCwebviewer} theViewer viewer module
   * @param {IAx2}   theAx2    sphere center (shape origin)
   * @param {number} theRadius sphere radius
   * @param {number} theDevAngle mesh deviation angle parameter
   */
  public buildSphere(theViewer: SCwebviewer,
                     theAx2: IAx2, theRadius: number,
                     theDevAngle: number = 20.0 * Math.PI / 180.0): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('sphere') ?? 'sphere'; }
    this.shapeMgr?.buildSphere(this._myObjId, theAx2, theRadius, theDevAngle);
    this.objMgr?.addInteractiveObject(this);
  }

  /**
   * Create cylinder shape from parameters (based on `BRepPrimAPI_MakeCylinder` from C++ OCCT).
   * @param {SCwebviewer} theViewer viewer module
   * @param {ICylinderParams} theParams cylinder parameters
   * @param {number} theDevAngle mesh deviation angle parameter
   */
  public buildCylinder(theViewer: SCwebviewer, theParams: ICylinderParams,
                       theDevAngle: number = 20.0 * Math.PI / 180.0): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('cylinder') ?? 'cylinder'; }
    this.shapeMgr?.buildCylinder(this._myObjId, theParams, theDevAngle);
    this.objMgr?.addInteractiveObject(this);
  }

  /**
   * Create cone shape from parameters (based on `BRepPrimAPI_MakeCone` from C++ OCCT).
   * @param {SCwebviewer} theViewer viewer module
   * @param {IConeParams} theParams cone parameters
   * @param {number} theDevAngle mesh deviation angle parameter
   */
  public buildCone(theViewer: SCwebviewer, theParams: IConeParams,
                   theDevAngle: number = 20.0 * Math.PI / 180.0): void {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('cone') ?? 'cone'; }
    this.shapeMgr?.buildCone(this._myObjId, theParams, theDevAngle);
    this.objMgr?.addInteractiveObject(this);
  }

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
  public async openUrlAsync(theViewer: SCwebviewer, theFileUrl: string): Promise<boolean> {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId(theFileUrl) ?? theFileUrl; }
    const aFilePath = this.getFilePathFromObjId();
    try {
      await theViewer.uploadUrl(theFileUrl, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    return this.openShapeFromFile(aFilePath);
  }

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
  public async openBufferAsync(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): Promise<boolean> {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId(theFilePath) ?? theFilePath; }
    const aFilePath = this.getFilePathFromObjId();
    try {
      await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    return this.openShapeFromFile(aFilePath);
  }

  /**
   * Load deferred shape data from file previously opened
   * by {@link InteractiveShape.openUrlAsync} or by opening new specified URL.
   * This method doesn't display the object in the Viewer - {@link InteractiveObject.isVisible}=true
   * should be called after loading data completion.
   * @param {SCwebviewer} theViewer viewer module
   * @param {string} theFileUrl the file URL address to read model from or empty string to load already opened model
   * @return {Promise<boolean>} FALSE if object was not found
   */
  public async loadUrlAsync(theViewer: SCwebviewer, theFileUrl: string = ''): Promise<boolean> {
    if (theFileUrl !== '') {
      try {
        await this.openUrlAsync(theViewer, theFileUrl);
      } catch (theErr) {
        return Promise.reject(theErr);
      }
    }
    return this.loadDataAsync(theViewer);
  }

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
  public async loadBufferAsync(theViewer: SCwebviewer,
                               theArray: ArrayBuffer | null = null,
                               theFileName: string = ''): Promise<boolean> {
    if (theArray !== null) {
      try {
        await this.openBufferAsync(theViewer, theArray, theFileName);
      } catch (theErr) {
        return Promise.reject(theErr);
      }
    }
    return this.loadDataAsync(theViewer);
  }

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
  public explodeShape(theViewer: SCwebviewer, theExplodeDepth: number = -1,
                      theIsAutoNaming: boolean = true, theToGenId: boolean = true,
                      theToPropagateId: boolean = true,
                      theNewShapeIds: string[] = []): InteractiveShape[] {
    this._myViewer = theViewer;
    if (this._myObjId === '' || !this.shapeMgr || !this.objMgr) { return []; }

    const aNewShapeIds = theNewShapeIds.join('\n');
    let anObjArray: InteractiveShape[] = [];
    const anExplodedShapes = this.shapeMgr.explodeShape(this._myObjId, theExplodeDepth, theIsAutoNaming, theToGenId, theToPropagateId, aNewShapeIds);
    if (anExplodedShapes.length === 0) { return []; }
    this.objMgr.removeInteractiveObject(this._myObjId);
    const anExplodedShapesArr = anExplodedShapes.length !== 0 ? anExplodedShapes.split('\n') : [];
    for (const aShapeIter of anExplodedShapesArr) {
      let aShape = new InteractiveShape(aShapeIter);
      aShape.wrap(this._myViewer, aShapeIter);
      this.objMgr.addInteractiveObject(aShape);
      anObjArray.push(aShape);
    }
    this._myObjOldId = this._myObjId;
    this._myObjId = '';
    return anObjArray;
  }

  /** Dump shape into a file. */
  public save(theFile: string = ''): boolean {
    let aFilePath = theFile;
    if (aFilePath === '' && this._myObjId !== '') {
      aFilePath = `${this._myObjId}.brep`;
    }
    let aSelElemIds: ISelectionShapeElems = new SelectionShapeElems();
    return (this.shapeMgr?.saveModelShape(this._myObjId, aSelElemIds, aFilePath) ?? false) &&
           (this._myViewer?.makeFileLink(aFilePath) ?? false);
  };

  /** Set material preset {@link MaterialPreset} to object. */
  public setMaterialPreset(thePreset: string): void {
    if (!this._myViewer) { return; }
    const aMat = InteractiveShape.getMaterialPreset(this._myViewer, thePreset);
    this.material = aMat;
  }

  /** Return position of vertex subshape of specified index */
  public getSubshapeVertex(theVertId: number): IVec3 { return this.shapeMgr?.getSubshapeVertex(this._myObjId, theVertId) ?? Vec3.invalid(); }

  /** Return face color or empty string */
  public getFaceColor(theIndex: number): ColorString {
    return this.shapeMgr?.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, theIndex) ?? '';
  }

  /** Set or unset face color. */
  public setFaceColor(theIndex: number, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, theIndex.toString(10), theColor);
  }

  /** Set or unset faces color; applied to all faces when null is passed instead of index list */
  public setFacesColor(theIndices: number[] | null, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId,
                                    ShapeObjectSelMode.Face,
                                    theIndices != null ? this.indexArrayToString(theIndices) : '*',
                                    theColor);
  }

  /** Unset faces' colors */
  public clearFaceColors(): void { this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, '*', ''); }

  /** Return edge color or empty string */
  public getEdgeColor(theIndex: number): ColorString {
    return this.shapeMgr?.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, theIndex) ?? '';
  }

  /** Set or unset edge color */
  public setEdgeColor(theIndex: number, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, theIndex.toString(10), theColor);
  }

  /** Set or unset edges color; applied to all edges when null is passed instead of index list */
  public setEdgesColor(theIndices: number[] | null, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId,
                                    ShapeObjectSelMode.Edge,
                                    theIndices != null ? this.indexArrayToString(theIndices) : '*',
                                    theColor);
  }

  /** Unset edges' colors */
  public clearEdgeColors(): void { this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, '*', ''); }

  /** Return vertex color or empty string */
  public getVertexColor(theIndex: number): ColorString {
    return this.shapeMgr?.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, theIndex) ?? '';
  }

  /** Set or unset vertex color */
  public setVertexColor(theIndex: number, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, theIndex.toString(10), theColor);
  }

  /** Set or unset vertices color; applied to all vertices when null is passed instead of index list */
  public setVerticesColor(theIndices: number[] | null, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId,
                                    ShapeObjectSelMode.Vertex,
                                    theIndices != null ? this.indexArrayToString(theIndices) : '*',
                                    theColor);
  }

  /** Unset vertex' colors */
  public clearVertexColors(): void { this.shapeMgr?.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, '*', ''); }

  /**
   * Remove all selection groups.
   * Tip: do NOT activate group selection mode while managing groups list!
   */
  public clearGroups(): void { this.shapeMgr?.clearSelectionGroups(this._myObjId); }

  /**
   * Add selection group pointint to the list of subshapes of specified type.
   * Tip: do NOT activate group selection mode while managing groups list!
   */
  public addGroup(theElemType: ShapeObjectSelMode, theIndexList: number[]): number {
    let anElems = this.indexArrayToString(theIndexList);
    return this.shapeMgr?.addSelectionGroup(this._myObjId, theElemType, anElems) ?? -1;
  }

  /**
   * Return the list with indices of available subshapes.
   * @param theElemType subshape type to explore
   * @param theIsFree if TRUE, only free subshapes will be considered
   * @param theParentType type of a parent shape or Global to explore all roots
   * @param theParentIndex index of a parent shape or -1 to explore all roots
   * @param theVisibility return only visible (1), hidden (0) or subshape with any visibility attribute
   * @return index list of subshapes
   */
  public getSubshapesIndexes(theElemType: ShapeObjectSelMode, theIsFree: boolean = false,
                             theParentType: ShapeObjectSelMode = ShapeObjectSelMode.Global, theParentIndex: number = -1,
                             theVisibility: number = -1): number[] {
    let aSelElems = this._myViewer?.selectionManager?.getSubshapesIndexes(this._myObjId, theElemType, theIsFree,
                                                                          theParentType, theParentIndex, theVisibility) ?? '';
    return SelectionShapeElems.indexArrayFromString(aSelElems);
  }

  /** Clear selected faces/edges/vertices. */
  public clearSelectedElems(): void {
    let aSelElemIds: ISelectionShapeElems = new SelectionShapeElems();
    this.setSelectedShapeElems(aSelElemIds);
  }

  /** Set the list with indices of selected faces/edges/vertices. */
  public setSelectedElems(theFaces: number[], theEdges: number[], theVertices: number[]): void {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(ShapeObjectSelMode.Face, theFaces);
    aSelElemIds.setIndices(ShapeObjectSelMode.Edge, theEdges);
    aSelElemIds.setIndices(ShapeObjectSelMode.Vertex, theVertices);
    this.setSelectedShapeElems(aSelElemIds);
  }

  /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
  public getConnectedSubshapes(theElemType: ShapeObjectSelMode, theIndexList: number[]): number[] {
    let anElems = this.indexArrayToString(theIndexList);
    const aRes: string = this.shapeMgr?.getConnectedSubshapes(this._myObjId, theElemType, anElems) ?? '';
    return this.indexArrayFromString(aRes);
  }

  /** Parse index list from the string. */
  public indexArrayFromString(theIndices: string): number[] { return SelectionShapeElems.indexArrayFromString(theIndices); }

  /** Format index list into a string. */
  public indexArrayToString(theIndices: number[]): string { return SelectionShapeElems.indexArrayToString(theIndices); }

  /** @internal Return subshape dump or empty string */
  public getSubshapeDump(theElemType: ShapeObjectSelMode, theIndex: number): string {
    return this.shapeMgr?.getSubshapeDump(this._myObjId, theElemType, theIndex) ?? '';
  }

  private getRootShapeInfo(theToExplore: boolean): IShapeInfo {
    const aMgr: any = this.shapeMgr;
    if (aMgr && typeof aMgr.getShapeInfo === 'function') {
      const aMgrInfo = aMgr.getShapeInfo(this._myObjId, theToExplore);
      const aRes = Object.assign(new ShapeInfo(), aMgrInfo);
      aRes.objectId = this._myObjId;
      return aRes;
    }

    const aInfo = new ShapeInfo();
    aInfo.objectId = this._myObjId;

    const aLoc = this.getAnySubshapeLocation();
    if (aLoc) {
      aInfo.location = aLoc;
    }

    if (theToExplore && this.shapeMgr) {
      const aFaces = this.getSubshapesIndexes(ShapeObjectSelMode.Face, false);
      if (aFaces.length > 0) {
        const aFaceInfos = this.getSubshapesInfo(ShapeObjectSelMode.Face, aFaces, false);
        for (const aFaceInfo of aFaceInfos) {
          aInfo.nbTriangles += aFaceInfo.nbTriangles;
          aInfo.nbTriNodes += aFaceInfo.nbTriNodes;
        }
      }

      const anEdges = this.getSubshapesIndexes(ShapeObjectSelMode.Edge, false);
      if (anEdges.length > 0) {
        const anEdgeInfos = this.getSubshapesInfo(ShapeObjectSelMode.Edge, anEdges, false);
        for (const anEdgeInfo of anEdgeInfos) {
          aInfo.nbPolySegments += anEdgeInfo.nbPolySegments;
        }
      }
    }

    return aInfo;
  }

  private getAnySubshapeLocation(): ITrsf | null {
    if (!this.shapeMgr) {
      return null;
    }

    const aTypes: ShapeObjectSelMode[] = [
      ShapeObjectSelMode.Face,
      ShapeObjectSelMode.Edge,
      ShapeObjectSelMode.Wire,
      ShapeObjectSelMode.Vertex,
      ShapeObjectSelMode.Shell,
      ShapeObjectSelMode.Solid,
      ShapeObjectSelMode.CompSolid,
      ShapeObjectSelMode.Group,
    ];

    for (const aType of aTypes) {
      const aIds = this.getSubshapesIndexes(aType, false);
      if (aIds.length === 0) {
        continue;
      }

      const aInfo = this.shapeMgr.getSubshapeInfo(this._myObjId, aType, aIds[0], false);
      if (aInfo && aInfo.location) {
        return aInfo.location;
      }
    }

    return null;
  }

  /** @internal Return subshape info */
  public getSubshapeInfo(theElemType: ShapeObjectSelMode, theIndex: number, theToExplore: boolean = true): IShapeInfo {
    if (theElemType === ShapeObjectSelMode.Global) {
      return this.getRootShapeInfo(theToExplore);
    }
    let anInfo = this.shapeMgr?.getSubshapeInfo(this._myObjId, theElemType, theIndex, theToExplore) ?? new ShapeInfo();
    let aRes = Object.assign(new ShapeInfo(), anInfo);
    aRes.objectId = this._myObjId; // avoid duplicating string - rely on JS sharing
    return aRes;
  }

  /** @internal Return subshape info */
  public getSubshapesInfo(theElemType: ShapeObjectSelMode, theIndices: number[], theToExplore: boolean = true): IShapeInfo[] {
    let aRes = new Array<IShapeInfo>();
    if (this.shapeMgr == null) {
      return aRes;
    }

    let anInfoVec = this.shapeMgr.getSubshapesInfo(this._myObjId, theElemType, this.indexArrayToString(theIndices), theToExplore);
    for (let i = 0; i < anInfoVec.size(); i++) {
      let aVal = anInfoVec.get(i);
      aVal.objectId = this._myObjId; // avoid duplicating string - rely on JS sharing
      aRes.push(aVal);
    }
    return aRes;
  }

  /** @internal Return subshape color or empty string */
  public getSubshapeColor(theElemType: ShapeObjectSelMode, theIndex: number): ColorString {
    return this.shapeMgr?.getSubshapeColor(this._myObjId, theElemType, theIndex) ?? '';
  }

  /** @internal Set or unset subshape color */
  public setSubshapeColor(theElemType: ShapeObjectSelMode, theIndices: string, theColor: ColorString): void {
    this.shapeMgr?.setSubshapeColor(this._myObjId, theElemType, theIndices, theColor);
  }

  /** @internal Return subshape hidden flag */
  public isSubshapeHidden(theElemType: ShapeObjectSelMode, theIndex: number): boolean {
    return this.shapeMgr?.isSubshapeHidden(this._myObjId, theElemType, theIndex) ?? false;
  }

  /** @internal Set or unset subshape hidden flag */
  public setSubshapeHidden(theElemType: ShapeObjectSelMode, theIndices: string, theIsHidden: boolean, theToInvertOthers: boolean): void {
    this.shapeMgr?.setSubshapeHidden(this._myObjId, theElemType, theIndices, theIsHidden, theToInvertOthers);
  }

  /** @internal Return index list of colored subshapes */
  public getColoredSubshapes(theElemType: ShapeObjectSelMode): number[] {
    if (!this.shapeMgr) { return []; }
    let aSubshapes = this.getSubshapesIndexes(theElemType, false);
    let aColored: number[] = [];
    for (let anIndexIter of aSubshapes) {
      let aColor = this.shapeMgr.getSubshapeColor(this._myObjId, theElemType, anIndexIter);
      if (aColor !== '') { aColored.push(anIndexIter); }
    }
    return aColored;
  }

  /** @internal Return the last detected subshape index, or `-1` if unspecified. */
  public getDetectedShapeElem(): IDetectedShapeElem | undefined {
    return this._myViewer?.selectionManager?.getDetectedShapeElem(this._myObjId);
  }

  /** @internal Set the list with indices of selected subhsapes. */
  public setSelectedShapeElems(theElems: ISelectionShapeElems): void {
    this._myViewer?.selectionManager?.setSelectedShapeElems(this._myObjId, theElems);
  }

  /** @internal Return the list with indices of selected subshapes. */
  public getSelectedSubshapes(theElemType: ShapeObjectSelMode): number[] {
    let aSelElems = this._myViewer?.selectionManager?.getSelectedShapeElems(this._myObjId, theElemType) ?? '';
    return SelectionShapeElems.indexArrayFromString(aSelElems);
  }

  /** @internal Return the list with indices of disabled subshapes. */
  public getDisabledSubshapes(theElemType: ShapeObjectSelMode, theToReverse: boolean): number[] {
    let aDisElems = this._myViewer?.selectionManager?.getDisabledShapeElems(this._myObjId, theElemType, theToReverse) ?? '';
    return SelectionShapeElems.indexArrayFromString(aDisElems);
  }

  /** @internal Set the list with indices of disabled subshapes. */
  public setDisabledSubshapes(theElemType: ShapeObjectSelMode, theToReverse: boolean, theIndexList: number[]): void {
    let anElems = this.indexArrayToString(theIndexList);
    this._myViewer?.selectionManager?.setDisabledShapeElems(this._myObjId, theElemType, theToReverse, anElems);
  }

  /** @internal Get the list with indices of selected faces/edges/vertices. */
  public getSelectedElems(): ISelectionShapeElems {
    let aSelElemIds: SelectionShapeElems = new SelectionShapeElems();
    aSelElemIds.setIndices(ShapeObjectSelMode.Group, this.getSelectedSubshapes(ShapeObjectSelMode.Group));
    aSelElemIds.setIndices(ShapeObjectSelMode.CompSolid, this.getSelectedSubshapes(ShapeObjectSelMode.CompSolid));
    aSelElemIds.setIndices(ShapeObjectSelMode.Solid, this.getSelectedSubshapes(ShapeObjectSelMode.Solid));
    aSelElemIds.setIndices(ShapeObjectSelMode.Shell, this.getSelectedSubshapes(ShapeObjectSelMode.Shell));
    aSelElemIds.setIndices(ShapeObjectSelMode.Face, this.getSelectedSubshapes(ShapeObjectSelMode.Face));
    aSelElemIds.setIndices(ShapeObjectSelMode.Wire, this.getSelectedSubshapes(ShapeObjectSelMode.Wire));
    aSelElemIds.setIndices(ShapeObjectSelMode.Edge, this.getSelectedSubshapes(ShapeObjectSelMode.Edge));
    aSelElemIds.setIndices(ShapeObjectSelMode.Vertex, this.getSelectedSubshapes(ShapeObjectSelMode.Vertex));
    return aSelElemIds;
  }

  /** @internal Dump subshape into a file. */
  public saveSubshapes(theFile: string, theElems: ISelectionShapeElems): boolean {
    return (this.shapeMgr?.saveModelShape(this._myObjId, theElems, theFile) ?? false) &&
           (this._myViewer?.makeFileLink(theFile) ?? false);
  }

  /** @internal Make a copy of existing object with new id and specified subshapes */
  public cloneSubshapes(theElems: ISelectionShapeElems, theNewObjId: ObjectId = ''): InteractiveShape | null {
    let anObjId: ObjectId = this.shapeMgr ? this.shapeMgr.cloneSubshapes(this._myObjId, theElems, theNewObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveShape();
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr.addInteractiveObject(aCopy);
    return aCopy;
  }

  /** Open shape from file path. */
  private openShapeFromFile(theFilePath: string): Promise<boolean> {
    const isDone = this.shapeMgr?.openModelShape(this._myObjId, theFilePath) ?? false;
    if (!isDone) { return Promise.reject(new Error('Open failed')); }
    this.objMgr?.addInteractiveObject(this);
    return Promise.resolve(true);
  }
};

/**
 * @internal Represents Context for Interactive shape objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveShape} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveShapeManager} property.
 */
export interface InteractiveShapeManager extends IManager {
// JS extensions

  /** @internal Dump all shapes into a single file. */
  saveScene: (theFile: string) => boolean;

// C++ methods

  /** @internal Create vertex shape from parameters. */
  buildPoint: (theObjId: ObjectId, thePnt: IVec3) => void;

  /** @internal Create line segment shape between two points. */
  buildLineSegment: (theObjId: ObjectId, thePnt1: IVec3, thePnt2: IVec3) => void;

  /** @internal Create box shape from parameters. */
  buildBox: (theObjId: ObjectId, theAx2: IAx2, theDims: IVec3) => void;

  /** @internal Create sphere shape from parameters. */
  buildSphere: (theObjId: ObjectId, theAx2: IAx2, theRadius: number, theDevAngle: number) => void;

  /** @internal Create cylinder shape from parameters. */
  buildCylinder: (theObjId: ObjectId, theParams: ICylinderParams, theDevAngle: number) => void;

  /** @internal Create cone shape from parameters. */
  buildCone: (theObjId: ObjectId, theParams: IConeParams, theDevAngle: number) => void;

  /** @internal Probe supported format from file header. */
  probeFileFormat: (theName: string, theBuffer: number, theDataLen: number, theToFree: boolean) => string;

  /** @internal Open shape model from specified file. */
  openModelShape: (theObjId: ObjectId, theFilePath: string) => boolean;

  /** @internal Dump shape into a file. */
  saveModelShape: (theObjId: ObjectId, theElems: ISelectionShapeElems, theFile: string) => boolean;

  /** @internal Create a new object from existing one with specified subhsapes. */
  cloneSubshapes: (theObjIdOld: ObjectId, theElems: ISelectionShapeElems, theObjIdNew: ObjectId) => ObjectId;

  /** @internal Return list of exploded objects' ids. */
  explodeShape: (theObjId: ObjectId,
                 theExplodeDepth: number,
                 theIsAutoNaming: boolean,
                 theToGenId: boolean,
                 theToPropagateId: boolean,
                 theResObjList: string) => string;

  /** @internal Set root shape location. */
  setShapeLocation: (theObjId: ObjectId, theTrsf: ITrsf) => void;

  /** @internal Get material preset {@link MaterialPreset}. */
  getMaterialPreset: (thePreset: string) => IVisMaterial;

  /** @internal Get default shape material. */
  getDefaultMaterial: () => IVisMaterial;

  /** @internal Set default shape material. */
  setDefaultMaterial: (theMat: IVisMaterial) => void;

  /** @internal Get object's material. */
  getMaterial: (theObjId: ObjectId) => IVisMaterial;

  /** @internal Set object's material. */
  setMaterial: (theObjId: ObjectId, theMat: IVisMaterial) => void;

  /** @internal Return object's presentation mode. */
  getPresentationMode: (theObjId: ObjectId) => ShapeObjectPrsMode;

  /** @internal Set object's presentation mode. */
  setPresentationMode: (theObjId: ObjectId, theMode: ShapeObjectPrsMode) => void;

  /** @internal Return object's overcolor (forced color), which overrides base/subshape colors, or an empty string. */
  getForcedColor: (theObjId: ObjectId) => ColorString;

  /** @internal Set or unsets object's overcolor (forced color). */
  setForcedColor: (theObjId: ObjectId, theColor: ColorString) => void;

  /** @internal Return object's overtransparency (forced transparency), which overrides base/subshape colors, or an empty string. */
  getForcedTransparency: (theObjId: ObjectId) => number;

  /** @internal Set or unsets object's overtransparency (forced transparency). */
  setForcedTransparency: (theObjId: ObjectId, theTransp: number) => void;

  /** @internal Return vertex position. */
  getSubshapeVertex: (theObjId: ObjectId, theVertId: number) => IVec3;

  /** @internal Get subshape color or empty string. */
  getSubshapeColor: (theObjId: ObjectId, theElemType: number, theIndex: number) => ColorString;

  /** @internal Set or unset subshape color. */
  setSubshapeColor: (theObjId: ObjectId, theElemType: number, theIndices: string, theColor: ColorString) => void;

  /** @internal Get subshape hidden flag. */
  isSubshapeHidden: (theObjId: ObjectId, theElemType: number, theIndex: number) => boolean;

  /** @internal Set or unset subshape hidden flag. */
  setSubshapeHidden: (theObjId: ObjectId, theElemType: number, theIndices: string, theIsHidden: boolean, theToInvertOthers: boolean) => void;

  /** @internal Get subshape info. */
  getSubshapeInfo: (theObjId: ObjectId, theElemType: number, theIndex: number, theToExplore: boolean) => IShapeInfo;

  /** @internal Get subshape info. */
  getSubshapesInfo: (theObjId: ObjectId, theElemType: number, theIndices: string, theToExplore: boolean) => SCwebviewer['ShapeInfoVector'];

  /** @internal Get subshapes connected to specified subshapes of the same type - for testing purposes. */
  getConnectedSubshapes: (theObjId: ObjectId, theElemType: ShapeObjectSelMode, theIndexList: string) => string;

  /** @internal Get subshape dump or empty string. */
  getSubshapeDump: (theObjId: ObjectId, theElemType: number, theIndex: number) => string;

  /** @internal Measure minimal distance between two shapes/subshapes and return JSON string. */
  measureDistanceJson: (theObjId1: ObjectId, theSelMode1: number, theEntityId1: number,
                        theObjId2: ObjectId, theSelMode2: number, theEntityId2: number) => string;

  /** @internal Return circle params for specified subshape (currently Edge only) as JSON string. */
  getCircleParamsJson: (theObjId: ObjectId, theSelMode: number, theEntityId: number) => string;

  /** @internal Return mass properties for specified shape as JSON string. */
  getMassPropsJson: (theObjId: ObjectId) => string;

  /** @internal Remove all selection groups. */
  clearSelectionGroups: (theObjId: ObjectId) => void;

  /** @internal Add selection group. */
  addSelectionGroup: (theObjId: ObjectId, theElemType: ShapeObjectSelMode, theIndices: string) => number;
};

/**
 * Auxiliary tool to initialize JS extensions to InteractiveShapeManager C++ class.
 * @internal
 */
 export class InteractiveShapeManagerExtensions {
  public static init(theViewer: SCwebviewer, theMgr: InteractiveShapeManager): void {
    theMgr.saveScene = function (theFile: string = 'scene.brep'): boolean {
      let aSelElemIds: ISelectionShapeElems = new SelectionShapeElems();
      return theMgr.saveModelShape('*', aSelElemIds, theFile) &&
             theViewer.makeFileLink(theFile);
    };

    // InteractiveShapeManager extensions
    Object.defineProperty(theMgr, 'doSceneDump', {
      get: function (): string { return this.saveScene('scene.sbrep') ? 'scene.sbrep' : ''; },
    });

    // ShapeInfo extensions
    Object.defineProperty(ShapeInfo, 'getSubshapes', {
      value: function(theInfo: IShapeInfo, theType: ShapeEnum): number[] {
        const aParType = SelectionShapeElems.shapeEnumToSelMode(theInfo.shapeType);
        const aSubType = SelectionShapeElems.shapeEnumToSelMode(theType);
        let aSelElems = theViewer?.selectionManager?.getSubshapesIndexes(theInfo.objectId, aSubType, false, aParType, theInfo.index, -1) ?? '';
        return SelectionShapeElems.indexArrayFromString(aSelElems);
      },
    });
  }
};
