import type {UnitManager, ColorString, IVec2, IVec3, IVec4, ObjectId, IBndBox3} from './CommonTools.js';
import {Vec2, Vec3, BndBox3, ColorTools} from './CommonTools.js';
import {InteractiveObject, SelectionMeshEntity} from './InteractiveObject.js';

import type {InteractiveColorScale} from './InteractiveColorScale.js';
import type {ISelectionMeshEntity, SCwebviewer} from './SCwebviewer.js';

import type {UserEvents} from './ViewerEventManager.js';
import type { InteractiveCaeMeshManager } from './InteractiveCaeMeshManager.js';

/**
 * CaeMeshObjectPrsMode represents presentation mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshPrsMode`.
 */
export enum CaeMeshObjectPrsMode {
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
};

/**
 * CaeMeshObjectElementType represents elements of {@link InteractiveCaeMesh} to display, from `InteractiveCaeMeshElementType`.
 */
export enum CaeMeshObjectElementType {
  Element0d = 0x1,
  Element1d = 0x2,
  Face = 0x4,
  Volume = 0x8,
  Node = 0x10,
  FreeNode = 0x20,
  Centroid = 0x40,
  FreeEdges = 0x80,
  SharpEdges = 0x100,
  AllElemental = 15, // Element0d|Element1d|Face|Volume
};

/**
 * CaeMeshObjectSelMode represents selection mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export enum CaeMeshObjectSelMode {
  /** Disabled selection. */
  None = 0x0000,
  /** Selection of mesh presentation. */
  Presentation = 0x0001,
  /** Selection of mesh elements. */
  Elements = 0x0002,
  /** Selection of mesh nodes. */
  Nodes = 0x0004
};

/**
 * CaeMeshVectorDistrType represents distribution type for vector mesh presentation, from `MeshVectorDistrType`.
 */
export enum CaeMeshVectorDistrType { Volume = 0, Surface = 1 };

/**
 * Represents type of mesh presentation, from `MeshPresentationType`.
 */
export enum CaeMeshPresentationType { Scalar = 0, Vector = 1, Streamlines = 2 };

/**
 * Represents type of mesh highlighting, from `MeshHighlightType`.
 */
export enum CaeMeshHighlightType { None = 0, Highlight = 1, Selection = 2 };

/**
 * ICaeMeshInfo represents mesh data info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshInfo {
  nbElems0d: number;
  nbElems1d: number;
  nbElems2d: number;
  nbTriangles: number;
  nbTriangles2nd : number;
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
};

/**
 * Represents mesh data info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshInfo implements ICaeMeshInfo {
  nbElems0d: number = 0;
  nbElems1d: number = 0;
  nbElems2d: number = 0;
  nbTriangles: number = 0;
  nbTriangles2nd: number = 0;
  nbQuads: number = 0;
  nbQuads2nd: number = 0;
  nbPolygons: number = 0;
  nbElems3d: number = 0;
  nbTetrahedrons: number = 0;
  nbTetrahedrons2nd: number = 0;
  nbPyramids: number = 0;
  nbPyramids2nd: number = 0;
  nbPrisms: number = 0;
  nbPrisms2nd: number = 0;
  nbHexahedrons: number = 0;
  nbHexahedrons2nd: number = 0;
  nbPolyhedrons: number = 0;
  nbNodes: number = 0;
  boundsMin: IVec3 = Vec3.invalid();
  boundsMax: IVec3 = Vec3.reverse(Vec3.invalid());

  /** Format mesh info string. */
  public static formatInfo(theInfo: ICaeMeshInfo, theUMgr: UnitManager | null = null): string {
    let aBndStr = 'N/A';
    if (!Vec3.isInfinite(theInfo.boundsMin)) {
      if (theUMgr) {
        aBndStr = `
  Min = (${theUMgr.length.format(theInfo.boundsMin)})
  Max = (${theUMgr.length.format(theInfo.boundsMax)})`;
      } else {
        aBndStr = `
  Min = (${theInfo.boundsMin.x}, ${theInfo.boundsMin.y}, ${theInfo.boundsMin.z})
  Max = (${theInfo.boundsMax.x}, ${theInfo.boundsMax.y}, ${theInfo.boundsMax.z})`;
      }
    }

    return `Nodes: ${theInfo.nbNodes}
0d elements: ${theInfo.nbElems0d}
1d elements: ${theInfo.nbElems1d}
2d elements: ${theInfo.nbElems2d}
  triangles: ${theInfo.nbTriangles}
  2nd order triangles: ${theInfo.nbTriangles2nd}
  quads: ${theInfo.nbQuads}
  2nd order quads: ${theInfo.nbQuads2nd}
  polygons: ${theInfo.nbPolygons}
3d elements: ${theInfo.nbElems3d}
  Tetrahedrons: ${theInfo.nbTetrahedrons}
  2nd order tetrahedrons: ${theInfo.nbTetrahedrons2nd}
  Pyramids: ${theInfo.nbPyramids}
  2nd order pyramids: ${theInfo.nbPyramids2nd}
  Prisms: ${theInfo.nbPrisms}
  2nd order prisms: ${theInfo.nbPrisms2nd}
  Hexahedrons: ${theInfo.nbHexahedrons}
  2nd order hexahedrons: ${theInfo.nbHexahedrons2nd}
  Polyhedrons: ${theInfo.nbPolyhedrons}
Bounds: ${aBndStr}`;
  }
};

/**
 * ICaeMeshPropCompInfo represents mesh data property component info of {@link InteractiveCaeMesh} entity.
 */
export interface ICaeMeshPropCompInfo {
  index: number;
  name: string;
  description: string;
  displayUnits: string;
  valueRange: IVec2;
};

/**
 * Represents mesh data property component info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshPropCompInfo implements ICaeMeshPropCompInfo {
  index: number = -1;
  name: string = '';
  description: string = '';
  displayUnits: string = '';
  valueRange: IVec2 = { x: 0.0, y: 0.0 };
};

/**
 * Represents type of mesh property, from `CaeMeshPropType`.
 */
export enum CaeMeshPropType { ScalarSet = 0, Vector = 1, Tensor = 2 };

/**
 * Represents type of mesh property elements, from `CaeMeshPropElemType`.
 */
export enum CaeMeshPropElemType { Scalar = 0, NodeVec = 1, Group = 2 };

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
};

/**
 * Represents mesh data property info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshPropInfo implements ICaeMeshPropInfo {
  propId: string = '';
  timeSteps: Array<number> = [];
  loadedTimeSteps: Array<number> = [];
  name: string = '';
  description: string = '';
  isElemental: boolean = false;
  type: CaeMeshPropType = CaeMeshPropType.ScalarSet;
  elemType: CaeMeshPropElemType = CaeMeshPropElemType.Scalar;
  comps: Array<ICaeMeshPropCompInfo> = [];
};

/**
 * @internal ICaeMeshAnnotationInfo represents mesh annotation info, from `CaeMeshAnnotationInfo`.
 */
export interface ICaeMeshAnnotationInfo {
  isVisible: boolean;
  text: SCwebviewer['StringVector'];
  icons: SCwebviewer['StringVector'];
  toShowText: boolean;
  toShowIcons: boolean;
  pos: IVec3;
};

/**
 * text + icon pair
 */
interface ICaeMeshTextIcon {
  text: string;
  icon: string;
};

/**
 * CaeMeshAnnotation represents mesh annotation info.
 */
export interface CaeMeshAnnotation {
  isVisible: boolean;
  data: Array<ICaeMeshTextIcon>;
  toShowText: boolean;
  toShowIcons: boolean;
  pos: IVec3;
};

/**
 * CaeMeshBlock represents mesh block node.
 * It contains name and array of children blocks so it forms a tree structure.
 */
export class CaeMeshBlock {
  name: string;
  children?: Array<CaeMeshBlock>;
};

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
};

/**
 * Represents streamlines info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshStreamlinesInfo implements ICaeMeshStreamlinesInfo {
  /** Property id. */
  property: string = '';
  /** Switch between regular (true) and random (false) distribution of the initial points in the seed plane. */
  isRegular: boolean = false;
  /** Total number of streamlines for random distribution of initial points. */
  nbStreamlines: number = 0;
  /** "U x V" streamlines layout for regular distribution of initial points. */
  uvStreamlines: IVec2 = Vec2.zero();
  /** Origin of plane used for streamlines initial points generation. */
  planeOrigin: IVec3 = Vec3.zero();
  /** Sized side vector of plane used for streamlines initial points generation. */
  planeSide: IVec3 = Vec3.dy();
  /** Sized up vector of plane used for streamlines initial points generation. */
  planeUp: IVec3 = Vec3.dz();
  /** Integration step coefficient; lower value leads to more accurate result but takes more time to calculate. */
  stepCoeff: number = 1.0;
  /** Streamlines max length coefficient. */
  lengthCoeff: number = 1.0;
  /** Thickness as a coefficient multiplied by bounding box size. */
  thickness: number = 0.0;
  /** Animation particles speed coefficient, in [0.0, 1.0] range. */
  animationSpeed: number = 0.25;
  /** Animation particles trail length coefficient, in [0.0, 1.0] range. */
  animationTrail: number = 0.5;
  /** Animation particles frequency, integer number in [1, 100] range. */
  animationFrequency: number = 10;
  /** Flag to enable/disable streamlines animation. */
  animationEnabled: boolean = false;
};

/**
 * id + index pair for iso properties.
 */
interface ICaeMeshIdIndex {
  id: string;
  index: number;
};

/**
 * id + name pair for properties.
 */
interface ICaeMeshIdName {
  id: string;
  name: string;
};

/**
 * Interface for names of 2D grid axes and tick mark labels
 * when mesh is displayed as part of 2D graph.
 */
export interface ICaeMesh2DAxesNamingInfo {
  nameX: string;
  nameY: string;
  ticksX: string[];
  ticksY: string[];
};

/**
 * Contains default values for ICaeMesh2DAxesNamingInfo.
 */
export class CaeMesh2DAxesNamingInfo implements ICaeMesh2DAxesNamingInfo {
  nameX: string = '';
  nameY: string = '';
  ticksX: string[] = [];
  ticksY: string[] = [];
};

/**
 * Auxiliary class for accessing InteractiveCaeMesh presentations.
 */
export class CaeMeshPresentation {
  /** @internal */
  private myMeshMgr: InteractiveCaeMeshManager | null;
  /** @internal */
  private myMeshObj: InteractiveCaeMesh;
  /** @internal */
  private myPrsId: string;

  /** @internal Constructor. */
  constructor(theMeshMgr: InteractiveCaeMeshManager | null, theObj: InteractiveCaeMesh, thePrsId: string) {
    this.myMeshMgr = theMeshMgr;
    this.myMeshObj = theObj;
    this.myPrsId = thePrsId;
  }

  /** Return presentation id. */
  public get id(): string { return this.myPrsId; }

  /** Return presentation type. */
  public get type(): CaeMeshPresentationType {
    return this.myMeshMgr?.getMeshPresentationType(this.myMeshObj.id, this.myPrsId) ?? CaeMeshPresentationType.Scalar;
  }

  /** Return mesh presentation mode. */
  public get presentationMode(): CaeMeshObjectPrsMode {
    return this.myMeshMgr?.getMeshPresentationMode(this.myMeshObj.id, this.myPrsId) ?? CaeMeshObjectPrsMode.FlatShaded;
  }

  /** Return active mesh property. */
  public get activeProperty(): string { return this.myMeshMgr?.getActiveMeshProperty(this.myMeshObj.id, this.myPrsId) ?? ''; }

  /** Return index of active mesh property component. */
  public get activeComponent(): number { return this.myMeshMgr?.getActiveMeshComponentIndex(this.myMeshObj.id, this.myPrsId) ?? -1; }

  /** To display 3d elements. */
  public get toDisplay3dElements(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Volume) !== 0;
  }

  /** To display 2d elements. */
  public get toDisplay2dElements(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Face) !== 0;
  }

  /** To display 0d elements. */
  public get toDisplay0dElements(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Element0d) !== 0;
  }

  /** To display element centroids. */
  public get toDisplayCentroids(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Centroid) !== 0;
  }

  /** To display nodes. */
  public get toDisplayNodes(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Node) !== 0;
  }

  /** Return true if free nodes should be used within element nodes in calculation/visualization. */
  public get toProcessFreeNodes(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.FreeNode) !== 0;
  }

  /** To display free edges. */
  public get toDisplayFreeEdges(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.FreeEdges) !== 0;
  }

  /** To display sharp edges. */
  public get toDisplaySharpEdges(): boolean {
    if (!this.myMeshMgr) { return false; }
    return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.SharpEdges) !== 0;
  }

  /** Return id of current active deformed mesh property. */
  public get activeDeformedProp(): string { return this.myMeshMgr?.getMeshDeformedProp(this.myMeshObj.id, this.myPrsId) ?? ''; }

  /** Return scale coefficient for mesh deformed presentation. */
  public get deformedScaleCoeff(): number { return this.myMeshMgr?.getMeshDeformedScaleCoeff(this.myMeshObj.id, this.myPrsId) ?? 0.0; }

  /**
   * Return true if it is needed to render/select 3d mesh only by its surface
   * and to not include internal parts, True by default.
   */
  public get isVolumeAsSurface(): boolean { return this.myMeshMgr?.isMeshVolumeAsSurface(this.myMeshObj.id, this.myPrsId) ?? true; }

  /** Return presentation visibility. */
  public get isVisible(): boolean { return this.myMeshMgr?.isMeshPrsVisible(this.myMeshObj.id, this.myPrsId) ?? true; }

  /** Return shading color, "#ffffff" by default. */
  public get shadingColor(): ColorString {
    return this.myMeshMgr?.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? '';
  }

  /** Return highlighting shading color, "#e5e5e5" by default. */
  public get shadingHighlightColor(): ColorString {
    return this.myMeshMgr?.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? '';
  }

  /** Return selection shading color, "#cccccc" by default. */
  public get shadingSelectionColor(): ColorString {
    return this.myMeshMgr?.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? '';
  }

  /** Return edges color, "#cccccc" by default. */
  public get edgesColor(): ColorString {
    return this.myMeshMgr?.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? '';
  }

  /** Return highlighting edges color, "#e5e5e5" by default. */
  public get edgesHighlightColor(): ColorString {
    return this.myMeshMgr?.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? '';
  }

  /** Return selection edges color, "#cccccc" by default. */
  public get edgesSelectionColor(): ColorString {
    return this.myMeshMgr?.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? '';
  }

  /** Return edges width, 1 by default. */
  public get edgesWidth(): number {
    return this.myMeshMgr?.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? 1.0;
  }

  /** Return highlighting edges width, 1 by default. */
  public get edgesHighlightWidth(): number {
    return this.myMeshMgr?.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? 1.0;
  }

  /** Return selection edges width, 1 by default. */
  public get edgesSelectionWidth(): number {
    return this.myMeshMgr?.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? 1.0;
  }

  /** Return points color, "#7f7f7f" by default. */
  public get pointsColor(): ColorString {
    return this.myMeshMgr?.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? '';
  }

  /** Return highlighting points color, "#e5e5e5" by default. */
  public get pointsHighlightColor(): ColorString {
    return this.myMeshMgr?.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? '';
  }

  /** Return selection points color, "#cccccc" by default. */
  public get pointsSelectionColor(): ColorString {
    return this.myMeshMgr?.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? '';
  }

  /** Return points size, 5 by default. */
  public get pointsSize(): number {
    return this.myMeshMgr?.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? 1.0;
  }

  /** Return highlighting points size, 5 by default. */
  public get pointsHighlightSize(): number {
    return this.myMeshMgr?.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? 1.0;
  }

  /** Return selection points size, 5 by default. */
  public get pointsSelectionSize(): number {
    return this.myMeshMgr?.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? 1.0;
  }

  /** Return shading transparency, 0.0 by default. */
  public get shadingTransparency(): number {
    return this.myMeshMgr?.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None) ?? 0.0;
  }

  /** Return highlighting shading transparency, 0.8 by default. */
  public get shadingHighlightTransparency(): number {
    return this.myMeshMgr?.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight) ?? 0.0;
  }

  /** Return shading transparency, 0.8 by default. */
  public get shadingSelectionTransparency(): number {
    return this.myMeshMgr?.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection) ?? 0.0;
  }

  /** Return id of current active vector mesh property. */
  public get activeVecProp(): string { return this.myMeshMgr?.getMeshVecProp(this.myMeshObj.id, this.myPrsId) ?? ''; }

  /** Return scale coefficient for mesh vector presentation. */
  public get vecPropScaleCoeff(): number { return this.myMeshMgr?.getMeshVecPropScaleCoeff(this.myMeshObj.id, this.myPrsId) ?? 0.0; }

  /** Return distribution type for mesh vector presentation. */
  public get vecPropDistrType(): CaeMeshVectorDistrType {
    return this.myMeshMgr?.getMeshVecPropDistrType(this.myMeshObj.id, this.myPrsId) ?? CaeMeshVectorDistrType.Volume;
  }

  /** Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0, 1.0 by default. */
  public get vecSamplingRatio(): number { return this.myMeshMgr?.getMeshVecSamplingRatio(this.myMeshObj.id, this.myPrsId) ?? 1.0; }

  /** Return equation of clip plane used to filter mesh elements (null if not set). */
  public get clipPlane(): IVec4 | null {
    if (this.myMeshMgr == null) { return null; }
    return this.myMeshMgr.hasClipPlane(this.myMeshObj.id, this.myPrsId) ? this.myMeshMgr.getClipPlane(this.myMeshObj.id, this.myPrsId) : null;
  }

  /** Check whether mesh clipping cuts or filters elements. */
  public get toCutOnClipPlane(): boolean { return this.myMeshMgr?.toCutOnClipPlane(this.myMeshObj.id, this.myPrsId) ?? false; }

  /** Return array of planes used to cut mesh. */
  public get slicePlanes(): Array<IVec4> {
    let aRes = new Array<IVec4>();
    if (this.myMeshMgr == null) {
      return aRes;
    }
    let aSlicePlanes = this.myMeshMgr.getSlicePlanes(this.myMeshObj.id, this.myPrsId);
    for (let i = 0; i < aSlicePlanes.size(); i++) {
      aRes.push(aSlicePlanes.get(i));
    }
    return aRes;
  }

  /** Return array of iso values. */
  public get isoProperty(): ICaeMeshIdIndex {
    let aRes: ICaeMeshIdIndex = {id: '', index: -1};
    if (this.myMeshMgr == null) {
      return aRes;
    }
    aRes.id = this.myMeshMgr.getIsoPropId(this.myMeshObj.id, this.myPrsId);
    aRes.index = this.myMeshMgr.getIsoPropComp(this.myMeshObj.id, this.myPrsId);
    return aRes;
  }

  /** Return array of iso values. */
  public get isoValues(): Array<number> {
    let aRes = new Array<number>();
    if (this.myMeshMgr == null) {
      return aRes;
    }
    let aValues = this.myMeshMgr.getIsoValues(this.myMeshObj.id, this.myPrsId);
    for (let i = 0; i < aValues.size(); i++) {
      aRes.push(aValues.get(i));
    }
    return aRes;
  }

  /** Return visibility of annotations for min/max values of the current active mesh property. */
  public get toShowMinMax(): boolean { return this.myMeshMgr?.toShowPrsMinMax(this.myMeshObj.id, this.myPrsId) ?? false; }

  /** Return visibility of annotations for the current active group mesh property. */
  public get toShowGroupsInfo(): boolean { return this.myMeshMgr?.toShowGroupsInfo(this.myMeshObj.id, this.myPrsId) ?? false; }

  /** Return streamlines presentation parameters. */
  public get streamlines(): ICaeMeshStreamlinesInfo {
    return this.myMeshMgr?.getMeshStreamlinesInfo(this.myMeshObj.id, this.myPrsId) ?? new CaeMeshStreamlinesInfo();
  }

  /** Return mesh selection mode. */
  public get selectionMode(): number {
    return this.myMeshMgr?.getMeshSelectionMode(this.myMeshObj.id, this.myPrsId) ?? CaeMeshObjectSelMode.None;
  }

  /** Set mesh presentation mode. */
  public set presentationMode(theMode: CaeMeshObjectPrsMode) { this.myMeshMgr?.setMeshPresentationMode(this.myMeshObj.id, this.myPrsId, theMode); }

  /** Set active mesh property. */
  public set activeProperty(thePropertyId: string) { this.myMeshMgr?.setActiveMeshProperty(this.myMeshObj.id, this.myPrsId, thePropertyId); }

  /** Set index of active mesh property component. */
  public set activeComponent(theComponentIndex: number) { this.myMeshMgr?.setActiveMeshComponentIndex(this.myMeshObj.id, this.myPrsId, theComponentIndex); }

  /** Set to display 3d elements. */
  public set toDisplay3dElements(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.Volume;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.Volume;
    }
    this.myMeshMgr?.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display 2d elements. */
  public set toDisplay2dElements(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.Face;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.Face;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display 0d elements. */
  public set toDisplay0dElements(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.Element0d;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.Element0d;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display element centroids. */
  public set toDisplayCentroids(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }
    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.Centroid;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.Centroid;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display nodes. */
  public set toDisplayNodes(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.Node;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.Node;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Define if free nodes should be used within element nodes in calculation/visualization. */
  public set toProcessFreeNodes(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.FreeNode;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.FreeNode;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display free edges. */
  public set toDisplayFreeEdges(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.FreeEdges;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.FreeEdges;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set to display sharp edges. */
  public set toDisplaySharpEdges(toDisplay: boolean) {
    if (!this.myMeshMgr) { return; }

    let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
    if (toDisplay) {
      anElementsTypes |= CaeMeshObjectElementType.SharpEdges;
    } else {
      anElementsTypes &= ~CaeMeshObjectElementType.SharpEdges;
    }
    this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
  }

  /** Set current active deformed mesh property. */
  public set activeDeformedProp(thePropertyId: string) { this.myMeshMgr?.setMeshDeformedProp(this.myMeshObj.id, this.myPrsId, thePropertyId); }

  /** Set scale coefficient for mesh deformed presentation. */
  public set deformedScaleCoeff(theValue: number) { this.myMeshMgr?.setMeshDeformedScaleCoeff(this.myMeshObj.id, this.myPrsId, theValue); }

  /**
   * Set if it is needed to render/select 3d mesh only by its surface
   * and to not include internal parts.
   */
  public set isVolumeAsSurface(theIsVolumeAsSurface: boolean) { this.myMeshMgr?.setMeshVolumeAsSurface(this.myMeshObj.id, this.myPrsId, theIsVolumeAsSurface); }

  /** Set presentation visibility. */
  public set isVisible(theIsVisible: boolean) { this.myMeshMgr?.setMeshPrsVisible(this.myMeshObj.id, this.myPrsId, theIsVisible); }

  /** Set shading color. */
  public set shadingColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
  }

  /** Set highlighting shading color. */
  public set shadingHighlightColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
  }

  /** Set selection shading color. */
  public set shadingSelectionColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
  }

  /** Set edges color. */
  public set edgesColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
  }

  /** Set highlighting edges color. */
  public set edgesHighlightColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
  }

  /** Set selection edges color. */
  public set edgesSelectionColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
  }

  /** Set points color. */
  public set pointsColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
  }

  /** Set highlighting points color. */
  public set pointsHighlightColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
  }

  /** Set selection points color. */
  public set pointsSelectionColor(theColor: ColorString) {
    this.myMeshMgr?.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
  }

  /** Set edges width. */
  public set edgesWidth(theWidth: number) {
    this.myMeshMgr?.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.None);
  }

  /** Set highlighting edges width. */
  public set edgesHighlightWidth(theWidth: number) {
    this.myMeshMgr?.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.Highlight);
  }

  /** Set selection edges width. */
  public set edgesSelectionWidth(theWidth: number) {
    this.myMeshMgr?.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.Selection);
  }

  /** Set points size. */
  public set pointsSize(theSize: number) {
    this.myMeshMgr?.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.None);
  }

  /** Set highlighting points size. */
  public set pointsHighlightSize(theSize: number) {
    this.myMeshMgr?.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.Highlight);
  }

  /** Set selection points size. */
  public set pointsSelectionSize(theSize: number) {
    this.myMeshMgr?.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.Selection);
  }

  /** Set shading transparency. */
  public set shadingTransparency(theValue: number) {
    this.myMeshMgr?.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.None);
  }

  /** Set highlighting shading transparency. */
  public set shadingHighlightTransparency(theValue: number) {
    this.myMeshMgr?.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.Highlight);
  }

  /** Set selection shading transparency. */
  public set shadingSelectionTransparency(theValue: number) {
    this.myMeshMgr?.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.Selection);
  }

  /** Set current active vector mesh property. */
  public set activeVecProp(thePropertyId: string) { this.myMeshMgr?.setMeshVecProp(this.myMeshObj.id, this.myPrsId, thePropertyId); }

  /** Set distribution type for mesh vector presentation. */
  public set vecPropDistrType(theType: CaeMeshVectorDistrType) { this.myMeshMgr?.setMeshVecPropDistrType(this.myMeshObj.id, this.myPrsId, theType); }

  /** Set scale coefficient for mesh vector presentation. */
  public set vecPropScaleCoeff(theValue: number) { this.myMeshMgr?.setMeshVecPropScaleCoeff(this.myMeshObj.id, this.myPrsId, theValue); }

  /** Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0. */
  public set vecSamplingRatio(theRatio: number) { this.myMeshMgr?.setMeshVecSamplingRatio(this.myMeshObj.id, this.myPrsId, theRatio); }

  /** Set/unset clip plane used to filter mesh elements. */
  public set clipPlane(theEq: IVec4 | null)
  {
    if (this.myMeshMgr != null) {
      if (theEq != null) {
        this.myMeshMgr.setClipPlane(this.myMeshObj.id, this.myPrsId, theEq);
      } else {
        this.myMeshMgr.unsetClipPlane(this.myMeshObj.id, this.myPrsId);
      }
    }
  }

  /** Define whether mesh clipping cuts or filters elements. */
  public set toCutOnClipPlane(theToCut: boolean) { this.myMeshMgr?.setCutOnClipPlane(this.myMeshObj.id, this.myPrsId, theToCut); }

  /** Set/unset array of planes used to cut mesh. */
  public set slicePlanes(theEqs: Array<IVec4>)
  {
    if (this.myMeshMgr != null) {
      let aSlicePlanes = this.myMeshMgr.getSlicePlanes(this.myMeshObj.id, this.myPrsId);
      aSlicePlanes.resize(0, {x:0, y:0, z:0, w:0});
      for (let anEq of theEqs) {
        aSlicePlanes.push_back(anEq);
      }
      this.myMeshMgr.setSlicePlanes(this.myMeshObj.id, this.myPrsId, aSlicePlanes);
    }
  }

  /** Set/unset array of iso values. */
  public set isoProperty(theProp: ICaeMeshIdIndex)
  {
    this.myMeshMgr?.setIsoProp(this.myMeshObj.id, this.myPrsId, theProp.id, theProp.index);
  }

  /** Set/unset array of iso values. */
  public set isoValues(theValues: Array<number>)
  {
    if (this.myMeshMgr != null) {
      let aValues = this.myMeshMgr.getIsoValues(this.myMeshObj.id, this.myPrsId);
      aValues.resize(0, 0);
      for (let aValue of theValues) {
        aValues.push_back(aValue);
      }
      this.myMeshMgr.setIsoValues(this.myMeshObj.id, this.myPrsId, aValues);
    }
  }

  /** Set visibility of annotations for min/max values of the current active mesh property. */
  public set toShowMinMax(theToShow: boolean) { this.myMeshMgr?.setShowPrsMinMax(this.myMeshObj.id, this.myPrsId, theToShow); }

  /** Set visibility of annotations for the current active group mesh property. */
  public set toShowGroupsInfo(theToShow: boolean) { this.myMeshMgr?.setShowGroupsInfo(this.myMeshObj.id, this.myPrsId, theToShow); }

  /** Set streamlines presentation parameters. */
  public set streamlines(theInfo: ICaeMeshStreamlinesInfo) { this.myMeshMgr?.setMeshStreamlinesInfo(this.myMeshObj.id, this.myPrsId, theInfo); }

  /** Set mesh selection mode. */
  public set selectionMode(theMode: number) {
    this.myMeshMgr?.setMeshSelectionMode(this.myMeshObj.id, this.myPrsId, theMode);
  }

  /** Return annotation for specified mesh group. */
  public getGroupInfo(theGroupId: number): CaeMeshAnnotation
  {
    let aRes : CaeMeshAnnotation = {isVisible: false, data: [], toShowText: false, toShowIcons: false, pos: Vec3.invalid()};
    if (this.myMeshMgr == null) {
      return aRes;
    }
    const anInfo = this.myMeshMgr.getGroupInfo(this.myMeshObj.id, this.myPrsId, theGroupId);

    aRes.isVisible = anInfo.isVisible;
    aRes.toShowText = anInfo.toShowText;
    aRes.toShowIcons = anInfo.toShowIcons;
    aRes.pos = anInfo.pos;

    for (let i = 0; i < anInfo.text.size(); i++) {
      aRes.data.push({text: anInfo.text.get(i), icon: anInfo.icons.get(i)});
    }

    return aRes;
  }
 
   /** Set annotation for specified mesh group. */
  public setGroupInfo(theGroupId: number, theInfo: CaeMeshAnnotation): void
  {
    if (this.myMeshMgr == null) {
      return;
    }

    let anInfo = this.myMeshMgr.getGroupInfo(this.myMeshObj.id, this.myPrsId, theGroupId);

    anInfo.isVisible = theInfo.isVisible;
    anInfo.toShowText = theInfo.toShowText;
    anInfo.toShowIcons = theInfo.toShowIcons;
    anInfo.pos = theInfo.pos;

    anInfo.text.resize(0, '');
    anInfo.icons.resize(0, '');

    for (let aData of theInfo.data) {
      anInfo.text.push_back(aData.text);
      anInfo.icons.push_back(aData.icon);
    }

    this.myMeshMgr.setGroupInfo(this.myMeshObj.id, this.myPrsId, theGroupId, anInfo);
  }

  /** Return visible mesh blocks. */
  public getVisibleBlocks(): CaeMeshBlock | null {
    if (this.myMeshMgr == null) {
      return null;
    }

    const aBlocks = this.myMeshMgr.getVisibleMeshBlocks(this.myMeshObj.id, this.myPrsId);

    try {
      return JSON.parse(aBlocks);
    } catch (theErr) {
      return null;
    }
  }

  /** Set mesh blocks visibility. */
  public setVisibleBlocks(theBlocks: CaeMeshBlock, theVisible: boolean): void {
    if (this.myMeshMgr == null) {
      return;
    }

    const aBlocks = JSON.stringify(theBlocks);
    this.myMeshMgr.setVisibleMeshBlocks(this.myMeshObj.id, this.myPrsId, aBlocks, theVisible);
  }

  /** Set custom geometry for the specified part. */
  public async setGenGeometry(theViewer: SCwebviewer, thePartId: number, theArray: ArrayBuffer): Promise<boolean>
  {
    if (this.myMeshMgr == null) {
      return false;
    }

    const aFilePath = `${this.myMeshObj.getFilePathFromObjId()}_${this.myPrsId}_${thePartId}`;
    try {
      await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    
    let aRes = this.myMeshMgr?.setMeshGenGeometry(this.myMeshObj.id, this.myPrsId, thePartId, aFilePath) ?? false;

    theViewer.unlinkFile(aFilePath);

    return aRes;
  }
};

/**
 * timeStep + timestamp pair for CAE animation.
 */
interface ICaeMeshTimeStep {
  timeStep: number;
  timestamp: number;
};

/**
 * Auxiliary class for accessing InteractiveCaeMesh animation.
 */
export class CaeMeshAnimation {
  /** @internal */
  private myMeshMgr: InteractiveCaeMeshManager | null;
  /** @internal */
  private myMeshObj: InteractiveCaeMesh;

  /** @internal Constructor. */
  constructor(theMeshMgr: InteractiveCaeMeshManager | null, theObj: InteractiveCaeMesh) {
    this.myMeshMgr = theMeshMgr;
    this.myMeshObj = theObj;
  }

  /** Check if mesh properties animation contains valid input data. */
  public get isValid(): boolean { return this.myMeshMgr?.isMeshAnimationValid(this.myMeshObj.id) ?? false;}

  /** Check if mesh properties animation is running. */
  public get isActive(): boolean { return this.myMeshMgr?.isMeshAnimationActive(this.myMeshObj.id) ?? false;}

  /** Return duration of mesh properties animation, in seconds, 5.0 by default. */
  public get duration(): number { return this.myMeshMgr?.getMeshAnimationDuration(this.myMeshObj.id) ?? 0.0; }

  /** Check if animation should be restarted on the last update, true by default. */
  public get isLoopPlayback(): boolean { return this.myMeshMgr?.getMeshAnimationLoopPlayback(this.myMeshObj.id) ?? false; }

  /** Check if animation steps duration should take into account timestamps values, false by default. */
  public get uniformSteps(): boolean { return this.myMeshMgr?.getMeshAnimationUniformSteps(this.myMeshObj.id) ?? false; }

  /** Set duration of mesh properties animation, in seconds. */
  public set duration(theDur: number) { this.myMeshMgr?.setMeshAnimationDuration(this.myMeshObj.id, theDur); }

  /** Define if animation should be restarted on the last update. */
  public set isLoopPlayback(theIsLoop: boolean) { this.myMeshMgr?.setMeshAnimationLoopPlayback(this.myMeshObj.id, theIsLoop); }

  /** Define if animation steps duration should take into account timestamps values. */
  public set uniformSteps(theIsUniform: boolean) { this.myMeshMgr?.setMeshAnimationUniformSteps(this.myMeshObj.id, theIsUniform); }

  /** Start animation of mesh properties. */
  public start(theToRestart: boolean = false): void { this.myMeshMgr?.startMeshAnimation(this.myMeshObj.id, theToRestart); }

  /** Stop animation of mesh properties. */
  public stop(): void { this.myMeshMgr?.stopMeshAnimation(this.myMeshObj.id); }

  /**
   * Continue animation if it was paused after trying to move on the next time step
   * without not loaded related properties.
   * Should be called after all required properties have been loaded.
   */
  public continue(): void { this.myMeshMgr?.continueMeshAnimation(this.myMeshObj.id); }
};

/**
 * Interactive Mesh Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export class InteractiveCaeMesh extends InteractiveObject {
  private _loadedFiles = new Array<string>();
  private _toCacheProperties = false;

  /** Empty constructor. */
  constructor(theObjId: ObjectId = '') {
    super(theObjId);
  }

  /** Return all mesh presentations. */
  public get presentations(): Array<CaeMeshPresentation> {
    let aRes = new Array<CaeMeshPresentation>();
    if (this._myViewer == null || this.meshMgr == null) {
      return aRes;
    }

    let aPresentations = this.meshMgr.getPresentations(this._myObjId);
    for (let i = 0; i < aPresentations.size(); i++) {
      aRes.push(new CaeMeshPresentation(this.meshMgr, this, aPresentations.get(i)));
    }
    return aRes;
  }

  /** Return mesh properties. */
  public get properties(): Array<ICaeMeshPropInfo> {
    let aRes = new Array<ICaeMeshPropInfo>();
    if (this._myViewer == null || this.meshMgr == null) {
      return aRes;
    }

    let aProps = this.meshMgr.getMeshProperties(this._myObjId);
    for (let i = 0; i < aProps.size(); i++) {
      aRes.push(aProps.get(i));
    }
    return aRes;
  }

  /** Return mesh bounding box. */
  public get boundingBox(): IBndBox3 { return this.meshMgr?.getMeshBox(this._myObjId) ?? BndBox3.voidBox(); }

  /** Return available vector mesh properties. */
  public get availableVec3Props(): ICaeMeshIdName[] {
    let aRes = new Array<ICaeMeshIdName>();
    if (this._myViewer == null || this.meshMgr == null) {
      return aRes;
    }

    let aProps = this.meshMgr.getAvailableMeshVec3Props(this._myObjId);
    for (let i = 0; i < aProps.size(); i++) {
      let anId = aProps.get(i);
      let aName = this.getPropertyName(anId);
      let aDesc = this.getPropertyDescription(anId);
      aRes.push({ id: anId, name: aDesc !== '' ? aDesc : aName });
    }
    return aRes;
  }

  /** Return attached color scale objects. */
  public get colorScaleObjects(): Array<InteractiveColorScale> {
    let aRes = new Array<InteractiveColorScale>();
    if (!this._myViewer || !this.objMgr || !this._myViewer.interactiveColorScaleManager) {
      return aRes;
    }

    let aColorScaleIds = this._myViewer.interactiveColorScaleManager.getMeshAttachedColorScales(this._myObjId);
    for (let i = 0; i < aColorScaleIds.size(); i++) {
      let anObj = this.objMgr.findInteractiveObject(aColorScaleIds.get(i)) as InteractiveColorScale;
      if (anObj != null) {
        aRes.push(anObj);
      }
    }
    return aRes;
  }

  /** Return object's selection mode. */
  public get selectionMode(): number {
    return this._myViewer?.selectionManager?.getSelectionMode(this._myObjId) ?? CaeMeshObjectSelMode.None;
  }

  /** Return mesh info. */
  public get info(): ICaeMeshInfo {
    return this.meshMgr?.getMeshInfo(this._myObjId) ?? new CaeMeshInfo();
  }

  /** Return mesh info string. */
  public get infoString(): string { return CaeMeshInfo.formatInfo(this.info, this._myViewer?.unitManager ?? null); }

  /** Return mesh blocks. */
  public get blocks(): CaeMeshBlock | null {
    if (this.meshMgr == null) {
      return null;
    }

    const aBlocks = this.meshMgr.getMeshBlocks(this._myObjId);

    try {
      return JSON.parse(aBlocks);
    } catch (theErr) {
      return null;
    }
  }

  /** Return active time index. */
  public get activeTimeStep(): number {
    return this.meshMgr?.getMeshActiveTimeStep(this._myObjId) ?? -1;
  }

  /** Return available time indices. */
  public get availableTimeSteps(): ICaeMeshTimeStep[] {
    let aRes = new Array<ICaeMeshTimeStep>();
    if (this._myViewer == null || this.meshMgr == null) {
      return aRes;
    }

    const aTimeSteps = this.meshMgr.getAvailableMeshTimeSteps(this._myObjId);
    for (let i = 0; i < aTimeSteps.size(); i++) {
      let aTimeStep = aTimeSteps.get(i) as IVec2;
      aRes.push({timeStep: aTimeStep.x, timestamp: aTimeStep.y});
    }
    return aRes;
  }

  /** Return animation object. */
  public get animation(): CaeMeshAnimation { return new CaeMeshAnimation(this.meshMgr, this); }

  /** Check if mesh entities should be detected/selected. */
  public get toHighlightEntities(): boolean { return this.meshMgr?.toHighlightEntities(this._myObjId) ?? false; }

  /**
   * Return entities selection color, "#CCCCCC33" by default.
   * Combined from selection shading color/transparency.
   * Kept for API compatibility.
   */
  public get highlightColor(): ColorString {
    if (!this.meshMgr) {
      return '';
    }
    const aColorStr = this.meshMgr.getMeshShadingColor(this._myObjId, "", CaeMeshHighlightType.Selection);
    let aColor = ColorTools.convertColorStringToRgba(aColorStr);
    aColor.a = (1.0 - this.meshMgr.getMeshShadingTransparency(this._myObjId, "", CaeMeshHighlightType.Selection)) * 255;
    return ColorTools.convertRgbaToColorString(aColor);
  }

  /** Check whether the mesh draws a grid. */
  public get toShow2DGrid(): boolean { return this.meshMgr?.toShow2DGrid(this._myObjId) ?? false; }

  /** Get axes names and tick mark labels. */
  public get axesNamingParams(): ICaeMesh2DAxesNamingInfo { return this.meshMgr?.getAxesNaming(this._myObjId) ?? new CaeMesh2DAxesNamingInfo(); }

  /** Shortcut to interactive CAE mesh context. */
  private get meshMgr(): InteractiveCaeMeshManager | null { return this._myViewer?.interactiveCaeMeshManager ?? null; }

  /** @internal Dump mesh into a file. */
  private get doDump(): string {
    let aFilePath = `${this._myObjId}.mdb`;
    return this.save(aFilePath) ? aFilePath : '';
  }

  /** Set object's selection mode. */
  public set selectionMode(theMode: number) {
    this._myViewer?.selectionManager?.setSelectionMode(this._myObjId, theMode);
  }

  /** Set active time index. */
  public set activeTimeStep(theTimeStep: number) {
    this.meshMgr?.setMeshActiveTimeStep(this._myObjId, theTimeStep);
  }

  /** Set available time indices. */
  public set availableTimeSteps(theTimeSteps: ICaeMeshTimeStep[]) {
    if (this._myViewer == null || this.meshMgr == null) {
      return;
    }

    let aTimeSteps = new Array<IVec2>();

    for (let aTime of theTimeSteps) {
      aTimeSteps.push({x: aTime.timeStep, y: aTime.timestamp});
    }
    this.meshMgr.setAvailableMeshTimeSteps(this._myObjId, aTimeSteps);
  }

  /** Define if properties data is stored in MDB file and can be loaded/unloaded on demand. */
  public set toCacheProperties(theToCache: boolean) {
    if (this.meshMgr == null) {
      this._toCacheProperties = theToCache;
    }
  }

  /** Define if mesh entities should be detected/selected. */
  public set toHighlightEntities(theToHighlight: boolean) { this.meshMgr?.setHighlightEntities(this._myObjId, theToHighlight); }

  /**
   * Set entities selection color.
   * Affect selection shading color/transparency.
   * Kept for API compatibility.
   */
  public set highlightColor(theColor: ColorString) {
    if (!this.meshMgr) {
      return;
    }
    const aColor = ColorTools.convertColorStringToRgba(theColor);
    this.meshMgr.setMeshShadingTransparency(this._myObjId, "", 1.0 - aColor.a / 255.0, CaeMeshHighlightType.Selection);
    this.meshMgr.setMeshShadingColor(this._myObjId, "", theColor, CaeMeshHighlightType.Selection);
  }

  /** Set whether the mesh draws a grid. */
  public set toShow2DGrid(theToShow: boolean) { this.meshMgr?.setShow2DGrid(this._myObjId, theToShow); }
  
  /** Set axes names and tick mark labels. */
  public set axesNamingParams(theInfo: ICaeMesh2DAxesNamingInfo) {
    this.meshMgr?.setAxesNaming(this._myObjId, theInfo);
  }

  /**
   * Probe file header for known mesh formats.
   * @param {SCwebviewer} theViewer viewer module
   * @param {ArrayBuffer} theArray buffer to open
   * @param {string} theFilePath the file path to map
   * @return {string} detected format (if supported) or an empty string
   */
  public static probeFileFormat(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): string {
    let aHeader: Uint8Array = new Uint8Array(theArray);
    aHeader = aHeader.subarray(0, Math.min(aHeader.length, 4096));
    const aDataBuffer = theViewer._allocateArrayU8(aHeader);
    return theViewer.interactiveCaeMeshManager?.probeFileFormat(theFilePath, aDataBuffer, aHeader.length, true) ?? '';
  }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveCaeMesh | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveCaeMesh();
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr.addInteractiveObject(aCopy);
    return aCopy;
  }

  /** Create new mesh presentation of the specified type. */
  public createPresentation(theType: CaeMeshPresentationType, thePrsId: string = ''): CaeMeshPresentation | null
  {
    let anId = this.meshMgr?.createPresentation(this._myObjId, theType, thePrsId) ?? '';
    return anId !== '' ? new CaeMeshPresentation(this.meshMgr, this, anId) : null;
  }

  /** Remove mesh presentation. */
  public removePresentation(thePrs: CaeMeshPresentation): void
  {
    if (this.meshMgr == null || thePrs == null) {
      return;
    }
    this.meshMgr.removePresentation(this._myObjId, thePrs.id);
  }

  /** Get presentation by id. */
  public getPresentation(thePrsId: string): CaeMeshPresentation
  {
    return new CaeMeshPresentation(this.meshMgr, this, thePrsId);
  }

  /** Return mesh property. */
  public property(thePropertyId: string): ICaeMeshPropInfo | null {
    const aPropInfo = this.meshMgr?.getMeshProperty(this._myObjId, thePropertyId) ?? new CaeMeshPropInfo();
    return aPropInfo.propId !== '' ? aPropInfo : null;
  }

  /** Return mesh property component. */
  public propertyComponent(thePropertyId: string, theCompIndex: number): ICaeMeshPropCompInfo | null {
    const aCompInfo = this.meshMgr?.getMeshPropertyComponent(this._myObjId, thePropertyId, theCompIndex) ?? new CaeMeshPropCompInfo();
    return aCompInfo.index !== -1 ? aCompInfo : null;
  }

  /** Return value range for mesh property. */
  public getPropertyValueRange(thePropertyId: string, theComponentIndex: number): IVec2 {
    return this.propertyComponent(thePropertyId, theComponentIndex)?.valueRange ?? {x: 0, y: 0};
  }

  /** Return name for mesh property. */
  public getPropertyName(thePropertyId: string): string {
    return this.property(thePropertyId)?.name ?? '';
  }

  /** Return descrition for mesh property. */
  public getPropertyDescription(thePropertyId: string): string {
    return this.property(thePropertyId)?.description ?? '';
  }

  /** Return name for mesh property component. */
  public getComponentName(thePropertyId: string, theComponentIndex: number): string {
    return this.propertyComponent(thePropertyId, theComponentIndex)?.name ?? '';
  }

  /** Return descrition for mesh property component. */
  public getComponentDescription(thePropertyId: string, theComponentIndex: number): string {
    return this.propertyComponent(thePropertyId, theComponentIndex)?.description ?? '';
  }

  /** Return display units for specified mesh property. */
  public getPropertyDisplayUnits(thePropertyId: string, theComponentIndex: number): string {
    return this.propertyComponent(thePropertyId, theComponentIndex)?.displayUnits ?? '';
  }

  /** Return true if mesh property is elemental. */
  public isPropertyElemental(thePropertyId: string): boolean {
    return this.property(thePropertyId)?.isElemental ?? false;
  }

  /** Return true if mesh property is a group. */
  public isPropertyGroup(thePropertyId: string): boolean {
    return this.property(thePropertyId)?.elemType === CaeMeshPropElemType.Group;
  }

  /** Return true if mesh property contains one scalar component. */
  public isPropertySingleScalar(thePropertyId: string): boolean {
    const aPropInfo = this.property(thePropertyId);
    if (aPropInfo == null) {
      return false;
    }
    return aPropInfo.type === CaeMeshPropType.ScalarSet && aPropInfo.comps.length === 1;
  }

  /** Find property id by name. */
  public getPropertyIdByName(thePropertyName: string): string {
    return this.meshMgr?.getMeshPropertyIdByName(this._myObjId, thePropertyName) ?? '';
  }

  /** Return map of groups with non-zero number of entities. */
  public usedGroups(thePropertyId: string): Map<number, string> {
    let aRes = new Map<number, string>();
    if (!this._myViewer || !this.meshMgr) {
      return aRes;
    }
    let aGroups = this.meshMgr.getMeshUsedGroups(this._myObjId, thePropertyId);
    let aGroupIds = aGroups.keys();
    for (let i = 0; i < aGroupIds.size(); i++) {
      let aGroupId = aGroupIds.get(i);
      aRes.set(aGroupId, aGroups.get(aGroupId));
    }
    return aRes;
  }

  /** Return detected sub entity. */
  public getDetectedEntity() : ISelectionMeshEntity {
    return this._myViewer?.selectionManager?.getDetectedMeshEntity(this._myObjId) ?? new SelectionMeshEntity();
  }

  /** Return selected sub entities. */
  public getSelectedEntities() : Array<ISelectionMeshEntity> {
    let aRes = new Array<ISelectionMeshEntity>();
    let aSelSubEntities = this._myViewer?.selectionManager?.getSelectedMeshEntities(this._myObjId) ?? null;
    if (aSelSubEntities == null) {
      return aRes;
    }
    for (let i = 0; i < aSelSubEntities.size(); i++) {
      let aSelSubEntity = aSelSubEntities.get(i) as ISelectionMeshEntity;
      if (aSelSubEntity.entityId === -1) {
        continue;
      }
      aRes.push(aSelSubEntity);
    }
    return aRes;
  }

  /** Select sub entity. */
  public async setSelectedEntity(theEntity: ISelectionMeshEntity) : Promise<boolean> {
    if (this._myViewer == null) {
      return false;
    }

    if (theEntity.refMeshData != null) {
      theEntity.refMeshDataPath = `${this.getFilePathFromObjId()}_ent_${theEntity.prsId}${theEntity.geomId}${theEntity.entityId}`;
      try {
        await this._myViewer.uploadArrayBuffer(theEntity.refMeshData, theEntity.refMeshDataPath, false);
      } catch (theErr) {
        return Promise.reject(theErr);
      }
    }
    this._myViewer.selectionManager?.setSelectedMeshEntity(this._myObjId, theEntity);

    if (theEntity.refMeshDataPath !== '') {
      this._myViewer.unlinkFile(theEntity.refMeshDataPath);
    }

    return true;
  }

  /**
   * Open object from the given URL and register presentation(s) in the hidden (erased) state.
   * @param {SCwebviewer} theViewer viewer module
   * @param {string} theFileUrl the file URL address
   * @return {Promise<boolean>} FALSE if model cannot be opened
   */
  public async loadUrlAsync(theViewer: SCwebviewer, theFileUrl: string): Promise<boolean> {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId(theFileUrl) ?? theFileUrl; }
    const aFilePath = this.getFilePathFromObjId();
    try {
      await theViewer.uploadUrl(theFileUrl, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    return this.openMeshFromFile(aFilePath);
  }

  /**
   * Open object from the given ArrayBuffer and register presentation(s) in the hidden (erased) state.
   * @param {SCwebviewer} theViewer viewer module
   * @param {ArrayBuffer} theArray buffer to open
   * @param {string} theFilePath the file path to map
   * @return {Promise<boolean>} FALSE if model cannot be opened
   */
  public async loadBufferAsync(theViewer: SCwebviewer, theArray: ArrayBuffer, theFilePath: string): Promise<boolean> {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId(theFilePath) ?? theFilePath; }
    const aFilePath = this.getFilePathFromObjId();
    try {
      await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    return this.openMeshFromFile(aFilePath);
  }

  /**
   * Load properties from the given ArrayBuffer and append to the current object.
   * @param {SCwebviewer} theViewer viewer module
   * @param {ArrayBuffer} theArray buffer to load
   * @return {Promise<boolean>} FALSE if properties cannot be loaded
   */
  public async loadPropertiesAsync(theViewer: SCwebviewer, theArray: ArrayBuffer): Promise<boolean> {
    this._myViewer = theViewer;
    let aFilePath = this.getFilePathFromObjId();
    let i = 0;
    while (this._myViewer.checkFile(`${aFilePath}_prop_${i}`)) {
      i++;
    }
    aFilePath += `_prop_${i}`;
    try {
      await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
    } catch (theErr) {
      return Promise.reject(theErr);
    }
    return this.loadProperties(aFilePath);
  }

  /** Dump mesh into a file. */
  public save(theFile: string = ''): boolean {
    let aFilePath = theFile;
    if (aFilePath === '' && this._myObjId !== '') {
      aFilePath = `${this._myObjId}.mdb`;
    }
    return (this.meshMgr?.saveModelMesh(this._myObjId, aFilePath) ?? false) &&
           (this._myViewer?.makeFileLink(aFilePath) ?? false);
  };

  /** Remove all properties. */
  public removeProperties(theWithMetadata: boolean = true): void
  {
    this.meshMgr?.removeMeshProperties(this._myObjId, theWithMetadata);
    if (theWithMetadata) {
      this.unlinkLoadedFiles();
    }
  }

  /** Remove object and invalidates it's id. */
  public remove(): boolean {
    const aRes = super.remove();
    this.unlinkLoadedFiles();
    return aRes;
  }

  /** Set to use custom geometry for mesh presentations. */
  public setUseGenGeometry(): void { this.meshMgr?.setUseMeshGenGeometry(this._myObjId); }

  /** Set to use test custom geometry for mesh presentations. */
  public setUseTestCacheGenGeometry(): void { this.meshMgr?.setUseMeshTestCacheGenGeometry(this._myObjId); }

  /** Unlink related files. */
  private unlinkLoadedFiles(): void {
    if (this._myViewer == null) {
      return;
    }
    for (let aFile of this._loadedFiles) {
      if (!this._myViewer.unlinkFile(aFile)) {
        throw new Error(`Cannot unlink a file '${aFile}'`);
      }
    }
    this._loadedFiles = [];
  }

  /** Open mesh from file path. */
  private openMeshFromFile(theFilePath: string): Promise<boolean> {
    const isDone = this.meshMgr?.openModelMesh(this._myObjId, theFilePath, this._toCacheProperties) ?? false;
    if (!isDone) {
      this._myViewer?.unlinkFile(theFilePath);
      return Promise.reject(new Error('Open failed'));
    }
    this.objMgr?.addInteractiveObject(this);
    if (this._toCacheProperties) {
      this._loadedFiles.push(theFilePath);
    } else {
      this._myViewer?.unlinkFile(theFilePath);
    }
    return Promise.resolve(true);
  }

  /** Load properties from the file and append to the current object. */
  private loadProperties(theFilePath: string): Promise<boolean> {
    const isDone = this.meshMgr?.loadMeshProperties(this._myObjId, theFilePath) ?? false;
    if (!isDone) {
      this._myViewer?.unlinkFile(theFilePath);
      return Promise.reject(new Error('Load failed'));
    }
    if (this._toCacheProperties) {
      this._loadedFiles.push(theFilePath);
    } else {
      this._myViewer?.unlinkFile(theFilePath);
    }
    return Promise.resolve(true);
  }
};