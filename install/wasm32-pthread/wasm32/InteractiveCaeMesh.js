import { Vec2, Vec3, BndBox3, ColorTools } from './CommonTools.js';
import { InteractiveObject, SelectionMeshEntity } from './InteractiveObject.js';
/**
 * CaeMeshObjectPrsMode represents presentation mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshPrsMode`.
 */
export var CaeMeshObjectPrsMode;
(function (CaeMeshObjectPrsMode) {
    /** Mesh edges with no shading. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["MeshEdges"] = 0] = "MeshEdges";
    /** Shaded with no edges. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["FlatShaded"] = 1] = "FlatShaded";
    /** Shaded + mesh edges. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["ShadedMeshEdges"] = 2] = "ShadedMeshEdges";
    /** Shaded with shrunk mesh elements. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["ShadedMeshShrinked"] = 3] = "ShadedMeshShrinked";
    /** Shaded + free mesh edges. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["ShadedFreeEdges"] = 4] = "ShadedFreeEdges";
    /** Shaded + sharp mesh edges. */
    CaeMeshObjectPrsMode[CaeMeshObjectPrsMode["ShadedSharpEdges"] = 5] = "ShadedSharpEdges";
})(CaeMeshObjectPrsMode || (CaeMeshObjectPrsMode = {}));
;
/**
 * CaeMeshObjectElementType represents elements of {@link InteractiveCaeMesh} to display, from `InteractiveCaeMeshElementType`.
 */
export var CaeMeshObjectElementType;
(function (CaeMeshObjectElementType) {
    CaeMeshObjectElementType[CaeMeshObjectElementType["Element0d"] = 1] = "Element0d";
    CaeMeshObjectElementType[CaeMeshObjectElementType["Element1d"] = 2] = "Element1d";
    CaeMeshObjectElementType[CaeMeshObjectElementType["Face"] = 4] = "Face";
    CaeMeshObjectElementType[CaeMeshObjectElementType["Volume"] = 8] = "Volume";
    CaeMeshObjectElementType[CaeMeshObjectElementType["Node"] = 16] = "Node";
    CaeMeshObjectElementType[CaeMeshObjectElementType["FreeNode"] = 32] = "FreeNode";
    CaeMeshObjectElementType[CaeMeshObjectElementType["Centroid"] = 64] = "Centroid";
    CaeMeshObjectElementType[CaeMeshObjectElementType["FreeEdges"] = 128] = "FreeEdges";
    CaeMeshObjectElementType[CaeMeshObjectElementType["SharpEdges"] = 256] = "SharpEdges";
    CaeMeshObjectElementType[CaeMeshObjectElementType["AllElemental"] = 15] = "AllElemental";
})(CaeMeshObjectElementType || (CaeMeshObjectElementType = {}));
;
/**
 * CaeMeshObjectSelMode represents selection mode of {@link InteractiveCaeMesh}, from `InteractiveCaeMeshSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export var CaeMeshObjectSelMode;
(function (CaeMeshObjectSelMode) {
    /** Disabled selection. */
    CaeMeshObjectSelMode[CaeMeshObjectSelMode["None"] = 0] = "None";
    /** Selection of mesh presentation. */
    CaeMeshObjectSelMode[CaeMeshObjectSelMode["Presentation"] = 1] = "Presentation";
    /** Selection of mesh elements. */
    CaeMeshObjectSelMode[CaeMeshObjectSelMode["Elements"] = 2] = "Elements";
    /** Selection of mesh nodes. */
    CaeMeshObjectSelMode[CaeMeshObjectSelMode["Nodes"] = 4] = "Nodes";
})(CaeMeshObjectSelMode || (CaeMeshObjectSelMode = {}));
;
/**
 * CaeMeshVectorDistrType represents distribution type for vector mesh presentation, from `MeshVectorDistrType`.
 */
export var CaeMeshVectorDistrType;
(function (CaeMeshVectorDistrType) {
    CaeMeshVectorDistrType[CaeMeshVectorDistrType["Volume"] = 0] = "Volume";
    CaeMeshVectorDistrType[CaeMeshVectorDistrType["Surface"] = 1] = "Surface";
})(CaeMeshVectorDistrType || (CaeMeshVectorDistrType = {}));
;
/**
 * Represents type of mesh presentation, from `MeshPresentationType`.
 */
export var CaeMeshPresentationType;
(function (CaeMeshPresentationType) {
    CaeMeshPresentationType[CaeMeshPresentationType["Scalar"] = 0] = "Scalar";
    CaeMeshPresentationType[CaeMeshPresentationType["Vector"] = 1] = "Vector";
    CaeMeshPresentationType[CaeMeshPresentationType["Streamlines"] = 2] = "Streamlines";
})(CaeMeshPresentationType || (CaeMeshPresentationType = {}));
;
/**
 * Represents type of mesh highlighting, from `MeshHighlightType`.
 */
export var CaeMeshHighlightType;
(function (CaeMeshHighlightType) {
    CaeMeshHighlightType[CaeMeshHighlightType["None"] = 0] = "None";
    CaeMeshHighlightType[CaeMeshHighlightType["Highlight"] = 1] = "Highlight";
    CaeMeshHighlightType[CaeMeshHighlightType["Selection"] = 2] = "Selection";
})(CaeMeshHighlightType || (CaeMeshHighlightType = {}));
;
;
/**
 * Represents mesh data info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshInfo {
    constructor() {
        this.nbElems0d = 0;
        this.nbElems1d = 0;
        this.nbElems2d = 0;
        this.nbTriangles = 0;
        this.nbTriangles2nd = 0;
        this.nbQuads = 0;
        this.nbQuads2nd = 0;
        this.nbPolygons = 0;
        this.nbElems3d = 0;
        this.nbTetrahedrons = 0;
        this.nbTetrahedrons2nd = 0;
        this.nbPyramids = 0;
        this.nbPyramids2nd = 0;
        this.nbPrisms = 0;
        this.nbPrisms2nd = 0;
        this.nbHexahedrons = 0;
        this.nbHexahedrons2nd = 0;
        this.nbPolyhedrons = 0;
        this.nbNodes = 0;
        this.boundsMin = Vec3.invalid();
        this.boundsMax = Vec3.reverse(Vec3.invalid());
    }
    /** Format mesh info string. */
    static formatInfo(theInfo, theUMgr = null) {
        let aBndStr = 'N/A';
        if (!Vec3.isInfinite(theInfo.boundsMin)) {
            if (theUMgr) {
                aBndStr = `
  Min = (${theUMgr.length.format(theInfo.boundsMin)})
  Max = (${theUMgr.length.format(theInfo.boundsMax)})`;
            }
            else {
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
}
;
;
/**
 * Represents mesh data property component info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshPropCompInfo {
    constructor() {
        this.index = -1;
        this.name = '';
        this.description = '';
        this.displayUnits = '';
        this.valueRange = { x: 0.0, y: 0.0 };
    }
}
;
/**
 * Represents type of mesh property, from `CaeMeshPropType`.
 */
export var CaeMeshPropType;
(function (CaeMeshPropType) {
    CaeMeshPropType[CaeMeshPropType["ScalarSet"] = 0] = "ScalarSet";
    CaeMeshPropType[CaeMeshPropType["Vector"] = 1] = "Vector";
    CaeMeshPropType[CaeMeshPropType["Tensor"] = 2] = "Tensor";
})(CaeMeshPropType || (CaeMeshPropType = {}));
;
/**
 * Represents type of mesh property elements, from `CaeMeshPropElemType`.
 */
export var CaeMeshPropElemType;
(function (CaeMeshPropElemType) {
    CaeMeshPropElemType[CaeMeshPropElemType["Scalar"] = 0] = "Scalar";
    CaeMeshPropElemType[CaeMeshPropElemType["NodeVec"] = 1] = "NodeVec";
    CaeMeshPropElemType[CaeMeshPropElemType["Group"] = 2] = "Group";
})(CaeMeshPropElemType || (CaeMeshPropElemType = {}));
;
;
/**
 * Represents mesh data property info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshPropInfo {
    constructor() {
        this.propId = '';
        this.timeSteps = [];
        this.loadedTimeSteps = [];
        this.name = '';
        this.description = '';
        this.isElemental = false;
        this.type = CaeMeshPropType.ScalarSet;
        this.elemType = CaeMeshPropElemType.Scalar;
        this.comps = [];
    }
}
;
;
;
;
/**
 * CaeMeshBlock represents mesh block node.
 * It contains name and array of children blocks so it forms a tree structure.
 */
export class CaeMeshBlock {
}
;
;
/**
 * Represents streamlines info of {@link InteractiveCaeMesh} entity.
 */
export class CaeMeshStreamlinesInfo {
    constructor() {
        /** Property id. */
        this.property = '';
        /** Switch between regular (true) and random (false) distribution of the initial points in the seed plane. */
        this.isRegular = false;
        /** Total number of streamlines for random distribution of initial points. */
        this.nbStreamlines = 0;
        /** "U x V" streamlines layout for regular distribution of initial points. */
        this.uvStreamlines = Vec2.zero();
        /** Origin of plane used for streamlines initial points generation. */
        this.planeOrigin = Vec3.zero();
        /** Sized side vector of plane used for streamlines initial points generation. */
        this.planeSide = Vec3.dy();
        /** Sized up vector of plane used for streamlines initial points generation. */
        this.planeUp = Vec3.dz();
        /** Integration step coefficient; lower value leads to more accurate result but takes more time to calculate. */
        this.stepCoeff = 1.0;
        /** Streamlines max length coefficient. */
        this.lengthCoeff = 1.0;
        /** Thickness as a coefficient multiplied by bounding box size. */
        this.thickness = 0.0;
        /** Animation particles speed coefficient, in [0.0, 1.0] range. */
        this.animationSpeed = 0.25;
        /** Animation particles trail length coefficient, in [0.0, 1.0] range. */
        this.animationTrail = 0.5;
        /** Animation particles frequency, integer number in [1, 100] range. */
        this.animationFrequency = 10;
        /** Flag to enable/disable streamlines animation. */
        this.animationEnabled = false;
    }
}
;
;
;
;
/**
 * Contains default values for ICaeMesh2DAxesNamingInfo.
 */
export class CaeMesh2DAxesNamingInfo {
    constructor() {
        this.nameX = '';
        this.nameY = '';
        this.ticksX = [];
        this.ticksY = [];
    }
}
;
/**
 * Auxiliary class for accessing InteractiveCaeMesh presentations.
 */
export class CaeMeshPresentation {
    /** @internal Constructor. */
    constructor(theMeshMgr, theObj, thePrsId) {
        this.myMeshMgr = theMeshMgr;
        this.myMeshObj = theObj;
        this.myPrsId = thePrsId;
    }
    /** Return presentation id. */
    get id() { return this.myPrsId; }
    /** Return presentation type. */
    get type() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPresentationType(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : CaeMeshPresentationType.Scalar;
    }
    /** Return mesh presentation mode. */
    get presentationMode() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPresentationMode(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : CaeMeshObjectPrsMode.FlatShaded;
    }
    /** Return active mesh property. */
    get activeProperty() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getActiveMeshProperty(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : ''; }
    /** Return index of active mesh property component. */
    get activeComponent() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getActiveMeshComponentIndex(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : -1; }
    /** To display 3d elements. */
    get toDisplay3dElements() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Volume) !== 0;
    }
    /** To display 2d elements. */
    get toDisplay2dElements() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Face) !== 0;
    }
    /** To display 0d elements. */
    get toDisplay0dElements() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Element0d) !== 0;
    }
    /** To display element centroids. */
    get toDisplayCentroids() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Centroid) !== 0;
    }
    /** To display nodes. */
    get toDisplayNodes() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.Node) !== 0;
    }
    /** Return true if free nodes should be used within element nodes in calculation/visualization. */
    get toProcessFreeNodes() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.FreeNode) !== 0;
    }
    /** To display free edges. */
    get toDisplayFreeEdges() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.FreeEdges) !== 0;
    }
    /** To display sharp edges. */
    get toDisplaySharpEdges() {
        if (!this.myMeshMgr) {
            return false;
        }
        return (this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId) & CaeMeshObjectElementType.SharpEdges) !== 0;
    }
    /** Return id of current active deformed mesh property. */
    get activeDeformedProp() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshDeformedProp(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : ''; }
    /** Return scale coefficient for mesh deformed presentation. */
    get deformedScaleCoeff() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshDeformedScaleCoeff(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : 0.0; }
    /**
     * Return true if it is needed to render/select 3d mesh only by its surface
     * and to not include internal parts, True by default.
     */
    get isVolumeAsSurface() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.isMeshVolumeAsSurface(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : true; }
    /** Return presentation visibility. */
    get isVisible() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.isMeshPrsVisible(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : true; }
    /** Return shading color, "#ffffff" by default. */
    get shadingColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return highlighting shading color, "#e5e5e5" by default. */
    get shadingHighlightColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return selection shading color, "#cccccc" by default. */
    get shadingSelectionColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return edges color, "#cccccc" by default. */
    get edgesColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return highlighting edges color, "#e5e5e5" by default. */
    get edgesHighlightColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return selection edges color, "#cccccc" by default. */
    get edgesSelectionColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return edges width, 1 by default. */
    get edgesWidth() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return highlighting edges width, 1 by default. */
    get edgesHighlightWidth() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return selection edges width, 1 by default. */
    get edgesSelectionWidth() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return points color, "#7f7f7f" by default. */
    get pointsColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return highlighting points color, "#e5e5e5" by default. */
    get pointsHighlightColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return selection points color, "#cccccc" by default. */
    get pointsSelectionColor() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsColor(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return points size, 5 by default. */
    get pointsSize() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return highlighting points size, 5 by default. */
    get pointsHighlightSize() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return selection points size, 5 by default. */
    get pointsSelectionSize() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPointsSize(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : 1.0;
    }
    /** Return shading transparency, 0.0 by default. */
    get shadingTransparency() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.None)) !== null && _b !== void 0 ? _b : 0.0;
    }
    /** Return highlighting shading transparency, 0.8 by default. */
    get shadingHighlightTransparency() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Highlight)) !== null && _b !== void 0 ? _b : 0.0;
    }
    /** Return shading transparency, 0.8 by default. */
    get shadingSelectionTransparency() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, CaeMeshHighlightType.Selection)) !== null && _b !== void 0 ? _b : 0.0;
    }
    /** Return id of current active vector mesh property. */
    get activeVecProp() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshVecProp(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : ''; }
    /** Return scale coefficient for mesh vector presentation. */
    get vecPropScaleCoeff() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshVecPropScaleCoeff(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : 0.0; }
    /** Return distribution type for mesh vector presentation. */
    get vecPropDistrType() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshVecPropDistrType(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : CaeMeshVectorDistrType.Volume;
    }
    /** Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0, 1.0 by default. */
    get vecSamplingRatio() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshVecSamplingRatio(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : 1.0; }
    /** Return equation of clip plane used to filter mesh elements (null if not set). */
    get clipPlane() {
        if (this.myMeshMgr == null) {
            return null;
        }
        return this.myMeshMgr.hasClipPlane(this.myMeshObj.id, this.myPrsId) ? this.myMeshMgr.getClipPlane(this.myMeshObj.id, this.myPrsId) : null;
    }
    /** Check whether mesh clipping cuts or filters elements. */
    get toCutOnClipPlane() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.toCutOnClipPlane(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : false; }
    /** Return array of planes used to cut mesh. */
    get slicePlanes() {
        let aRes = new Array();
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
    get isoProperty() {
        let aRes = { id: '', index: -1 };
        if (this.myMeshMgr == null) {
            return aRes;
        }
        aRes.id = this.myMeshMgr.getIsoPropId(this.myMeshObj.id, this.myPrsId);
        aRes.index = this.myMeshMgr.getIsoPropComp(this.myMeshObj.id, this.myPrsId);
        return aRes;
    }
    /** Return array of iso values. */
    get isoValues() {
        let aRes = new Array();
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
    get toShowMinMax() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.toShowPrsMinMax(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : false; }
    /** Return visibility of annotations for the current active group mesh property. */
    get toShowGroupsInfo() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.toShowGroupsInfo(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : false; }
    /** Return streamlines presentation parameters. */
    get streamlines() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshStreamlinesInfo(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : new CaeMeshStreamlinesInfo();
    }
    /** Return mesh selection mode. */
    get selectionMode() {
        var _a, _b;
        return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshSelectionMode(this.myMeshObj.id, this.myPrsId)) !== null && _b !== void 0 ? _b : CaeMeshObjectSelMode.None;
    }
    /** Set mesh presentation mode. */
    set presentationMode(theMode) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPresentationMode(this.myMeshObj.id, this.myPrsId, theMode); }
    /** Set active mesh property. */
    set activeProperty(thePropertyId) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setActiveMeshProperty(this.myMeshObj.id, this.myPrsId, thePropertyId); }
    /** Set index of active mesh property component. */
    set activeComponent(theComponentIndex) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setActiveMeshComponentIndex(this.myMeshObj.id, this.myPrsId, theComponentIndex); }
    /** Set to display 3d elements. */
    set toDisplay3dElements(toDisplay) {
        var _a;
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.Volume;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.Volume;
        }
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display 2d elements. */
    set toDisplay2dElements(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.Face;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.Face;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display 0d elements. */
    set toDisplay0dElements(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.Element0d;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.Element0d;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display element centroids. */
    set toDisplayCentroids(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.Centroid;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.Centroid;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display nodes. */
    set toDisplayNodes(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.Node;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.Node;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Define if free nodes should be used within element nodes in calculation/visualization. */
    set toProcessFreeNodes(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.FreeNode;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.FreeNode;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display free edges. */
    set toDisplayFreeEdges(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.FreeEdges;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.FreeEdges;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set to display sharp edges. */
    set toDisplaySharpEdges(toDisplay) {
        if (!this.myMeshMgr) {
            return;
        }
        let anElementsTypes = this.myMeshMgr.getMeshElemTypes(this.myMeshObj.id, this.myPrsId);
        if (toDisplay) {
            anElementsTypes |= CaeMeshObjectElementType.SharpEdges;
        }
        else {
            anElementsTypes &= ~CaeMeshObjectElementType.SharpEdges;
        }
        this.myMeshMgr.setMeshElemTypes(this.myMeshObj.id, this.myPrsId, anElementsTypes);
    }
    /** Set current active deformed mesh property. */
    set activeDeformedProp(thePropertyId) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshDeformedProp(this.myMeshObj.id, this.myPrsId, thePropertyId); }
    /** Set scale coefficient for mesh deformed presentation. */
    set deformedScaleCoeff(theValue) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshDeformedScaleCoeff(this.myMeshObj.id, this.myPrsId, theValue); }
    /**
     * Set if it is needed to render/select 3d mesh only by its surface
     * and to not include internal parts.
     */
    set isVolumeAsSurface(theIsVolumeAsSurface) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshVolumeAsSurface(this.myMeshObj.id, this.myPrsId, theIsVolumeAsSurface); }
    /** Set presentation visibility. */
    set isVisible(theIsVisible) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPrsVisible(this.myMeshObj.id, this.myPrsId, theIsVisible); }
    /** Set shading color. */
    set shadingColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
    }
    /** Set highlighting shading color. */
    set shadingHighlightColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
    }
    /** Set selection shading color. */
    set shadingSelectionColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
    }
    /** Set edges color. */
    set edgesColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
    }
    /** Set highlighting edges color. */
    set edgesHighlightColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
    }
    /** Set selection edges color. */
    set edgesSelectionColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
    }
    /** Set points color. */
    set pointsColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.None);
    }
    /** Set highlighting points color. */
    set pointsHighlightColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Highlight);
    }
    /** Set selection points color. */
    set pointsSelectionColor(theColor) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsColor(this.myMeshObj.id, this.myPrsId, theColor, CaeMeshHighlightType.Selection);
    }
    /** Set edges width. */
    set edgesWidth(theWidth) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.None);
    }
    /** Set highlighting edges width. */
    set edgesHighlightWidth(theWidth) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.Highlight);
    }
    /** Set selection edges width. */
    set edgesSelectionWidth(theWidth) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshEdgesWidth(this.myMeshObj.id, this.myPrsId, theWidth, CaeMeshHighlightType.Selection);
    }
    /** Set points size. */
    set pointsSize(theSize) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.None);
    }
    /** Set highlighting points size. */
    set pointsHighlightSize(theSize) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.Highlight);
    }
    /** Set selection points size. */
    set pointsSelectionSize(theSize) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshPointsSize(this.myMeshObj.id, this.myPrsId, theSize, CaeMeshHighlightType.Selection);
    }
    /** Set shading transparency. */
    set shadingTransparency(theValue) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.None);
    }
    /** Set highlighting shading transparency. */
    set shadingHighlightTransparency(theValue) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.Highlight);
    }
    /** Set selection shading transparency. */
    set shadingSelectionTransparency(theValue) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshShadingTransparency(this.myMeshObj.id, this.myPrsId, theValue, CaeMeshHighlightType.Selection);
    }
    /** Set current active vector mesh property. */
    set activeVecProp(thePropertyId) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshVecProp(this.myMeshObj.id, this.myPrsId, thePropertyId); }
    /** Set distribution type for mesh vector presentation. */
    set vecPropDistrType(theType) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshVecPropDistrType(this.myMeshObj.id, this.myPrsId, theType); }
    /** Set scale coefficient for mesh vector presentation. */
    set vecPropScaleCoeff(theValue) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshVecPropScaleCoeff(this.myMeshObj.id, this.myPrsId, theValue); }
    /** Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0. */
    set vecSamplingRatio(theRatio) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshVecSamplingRatio(this.myMeshObj.id, this.myPrsId, theRatio); }
    /** Set/unset clip plane used to filter mesh elements. */
    set clipPlane(theEq) {
        if (this.myMeshMgr != null) {
            if (theEq != null) {
                this.myMeshMgr.setClipPlane(this.myMeshObj.id, this.myPrsId, theEq);
            }
            else {
                this.myMeshMgr.unsetClipPlane(this.myMeshObj.id, this.myPrsId);
            }
        }
    }
    /** Define whether mesh clipping cuts or filters elements. */
    set toCutOnClipPlane(theToCut) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setCutOnClipPlane(this.myMeshObj.id, this.myPrsId, theToCut); }
    /** Set/unset array of planes used to cut mesh. */
    set slicePlanes(theEqs) {
        if (this.myMeshMgr != null) {
            let aSlicePlanes = this.myMeshMgr.getSlicePlanes(this.myMeshObj.id, this.myPrsId);
            aSlicePlanes.resize(0, { x: 0, y: 0, z: 0, w: 0 });
            for (let anEq of theEqs) {
                aSlicePlanes.push_back(anEq);
            }
            this.myMeshMgr.setSlicePlanes(this.myMeshObj.id, this.myPrsId, aSlicePlanes);
        }
    }
    /** Set/unset array of iso values. */
    set isoProperty(theProp) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setIsoProp(this.myMeshObj.id, this.myPrsId, theProp.id, theProp.index);
    }
    /** Set/unset array of iso values. */
    set isoValues(theValues) {
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
    set toShowMinMax(theToShow) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setShowPrsMinMax(this.myMeshObj.id, this.myPrsId, theToShow); }
    /** Set visibility of annotations for the current active group mesh property. */
    set toShowGroupsInfo(theToShow) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setShowGroupsInfo(this.myMeshObj.id, this.myPrsId, theToShow); }
    /** Set streamlines presentation parameters. */
    set streamlines(theInfo) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshStreamlinesInfo(this.myMeshObj.id, this.myPrsId, theInfo); }
    /** Set mesh selection mode. */
    set selectionMode(theMode) {
        var _a;
        (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshSelectionMode(this.myMeshObj.id, this.myPrsId, theMode);
    }
    /** Return annotation for specified mesh group. */
    getGroupInfo(theGroupId) {
        let aRes = { isVisible: false, data: [], toShowText: false, toShowIcons: false, pos: Vec3.invalid() };
        if (this.myMeshMgr == null) {
            return aRes;
        }
        const anInfo = this.myMeshMgr.getGroupInfo(this.myMeshObj.id, this.myPrsId, theGroupId);
        aRes.isVisible = anInfo.isVisible;
        aRes.toShowText = anInfo.toShowText;
        aRes.toShowIcons = anInfo.toShowIcons;
        aRes.pos = anInfo.pos;
        for (let i = 0; i < anInfo.text.size(); i++) {
            aRes.data.push({ text: anInfo.text.get(i), icon: anInfo.icons.get(i) });
        }
        return aRes;
    }
    /** Set annotation for specified mesh group. */
    setGroupInfo(theGroupId, theInfo) {
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
    getVisibleBlocks() {
        if (this.myMeshMgr == null) {
            return null;
        }
        const aBlocks = this.myMeshMgr.getVisibleMeshBlocks(this.myMeshObj.id, this.myPrsId);
        try {
            return JSON.parse(aBlocks);
        }
        catch (theErr) {
            return null;
        }
    }
    /** Set mesh blocks visibility. */
    setVisibleBlocks(theBlocks, theVisible) {
        if (this.myMeshMgr == null) {
            return;
        }
        const aBlocks = JSON.stringify(theBlocks);
        this.myMeshMgr.setVisibleMeshBlocks(this.myMeshObj.id, this.myPrsId, aBlocks, theVisible);
    }
    /** Set custom geometry for the specified part. */
    async setGenGeometry(theViewer, thePartId, theArray) {
        var _a, _b;
        if (this.myMeshMgr == null) {
            return false;
        }
        const aFilePath = `${this.myMeshObj.getFilePathFromObjId()}_${this.myPrsId}_${thePartId}`;
        try {
            await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
        }
        catch (theErr) {
            return Promise.reject(theErr);
        }
        let aRes = (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshGenGeometry(this.myMeshObj.id, this.myPrsId, thePartId, aFilePath)) !== null && _b !== void 0 ? _b : false;
        theViewer.unlinkFile(aFilePath);
        return aRes;
    }
}
;
;
/**
 * Auxiliary class for accessing InteractiveCaeMesh animation.
 */
export class CaeMeshAnimation {
    /** @internal Constructor. */
    constructor(theMeshMgr, theObj) {
        this.myMeshMgr = theMeshMgr;
        this.myMeshObj = theObj;
    }
    /** Check if mesh properties animation contains valid input data. */
    get isValid() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.isMeshAnimationValid(this.myMeshObj.id)) !== null && _b !== void 0 ? _b : false; }
    /** Check if mesh properties animation is running. */
    get isActive() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.isMeshAnimationActive(this.myMeshObj.id)) !== null && _b !== void 0 ? _b : false; }
    /** Return duration of mesh properties animation, in seconds, 5.0 by default. */
    get duration() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshAnimationDuration(this.myMeshObj.id)) !== null && _b !== void 0 ? _b : 0.0; }
    /** Check if animation should be restarted on the last update, true by default. */
    get isLoopPlayback() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshAnimationLoopPlayback(this.myMeshObj.id)) !== null && _b !== void 0 ? _b : false; }
    /** Check if animation steps duration should take into account timestamps values, false by default. */
    get uniformSteps() { var _a, _b; return (_b = (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.getMeshAnimationUniformSteps(this.myMeshObj.id)) !== null && _b !== void 0 ? _b : false; }
    /** Set duration of mesh properties animation, in seconds. */
    set duration(theDur) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshAnimationDuration(this.myMeshObj.id, theDur); }
    /** Define if animation should be restarted on the last update. */
    set isLoopPlayback(theIsLoop) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshAnimationLoopPlayback(this.myMeshObj.id, theIsLoop); }
    /** Define if animation steps duration should take into account timestamps values. */
    set uniformSteps(theIsUniform) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.setMeshAnimationUniformSteps(this.myMeshObj.id, theIsUniform); }
    /** Start animation of mesh properties. */
    start(theToRestart = false) { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.startMeshAnimation(this.myMeshObj.id, theToRestart); }
    /** Stop animation of mesh properties. */
    stop() { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.stopMeshAnimation(this.myMeshObj.id); }
    /**
     * Continue animation if it was paused after trying to move on the next time step
     * without not loaded related properties.
     * Should be called after all required properties have been loaded.
     */
    continue() { var _a; (_a = this.myMeshMgr) === null || _a === void 0 ? void 0 : _a.continueMeshAnimation(this.myMeshObj.id); }
}
;
/**
 * Interactive Mesh Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export class InteractiveCaeMesh extends InteractiveObject {
    /** Empty constructor. */
    constructor(theObjId = '') {
        super(theObjId);
        this._loadedFiles = new Array();
        this._toCacheProperties = false;
    }
    /** Return all mesh presentations. */
    get presentations() {
        let aRes = new Array();
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
    get properties() {
        let aRes = new Array();
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
    get boundingBox() { var _a, _b; return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshBox(this._myObjId)) !== null && _b !== void 0 ? _b : BndBox3.voidBox(); }
    /** Return available vector mesh properties. */
    get availableVec3Props() {
        let aRes = new Array();
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
    get colorScaleObjects() {
        let aRes = new Array();
        if (!this._myViewer || !this.objMgr || !this._myViewer.interactiveColorScaleManager) {
            return aRes;
        }
        let aColorScaleIds = this._myViewer.interactiveColorScaleManager.getMeshAttachedColorScales(this._myObjId);
        for (let i = 0; i < aColorScaleIds.size(); i++) {
            let anObj = this.objMgr.findInteractiveObject(aColorScaleIds.get(i));
            if (anObj != null) {
                aRes.push(anObj);
            }
        }
        return aRes;
    }
    /** Return object's selection mode. */
    get selectionMode() {
        var _a, _b, _c;
        return (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getSelectionMode(this._myObjId)) !== null && _c !== void 0 ? _c : CaeMeshObjectSelMode.None;
    }
    /** Return mesh info. */
    get info() {
        var _a, _b;
        return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshInfo(this._myObjId)) !== null && _b !== void 0 ? _b : new CaeMeshInfo();
    }
    /** Return mesh info string. */
    get infoString() { var _a, _b; return CaeMeshInfo.formatInfo(this.info, (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.unitManager) !== null && _b !== void 0 ? _b : null); }
    /** Return mesh blocks. */
    get blocks() {
        if (this.meshMgr == null) {
            return null;
        }
        const aBlocks = this.meshMgr.getMeshBlocks(this._myObjId);
        try {
            return JSON.parse(aBlocks);
        }
        catch (theErr) {
            return null;
        }
    }
    /** Return active time index. */
    get activeTimeStep() {
        var _a, _b;
        return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshActiveTimeStep(this._myObjId)) !== null && _b !== void 0 ? _b : -1;
    }
    /** Return available time indices. */
    get availableTimeSteps() {
        let aRes = new Array();
        if (this._myViewer == null || this.meshMgr == null) {
            return aRes;
        }
        const aTimeSteps = this.meshMgr.getAvailableMeshTimeSteps(this._myObjId);
        for (let i = 0; i < aTimeSteps.size(); i++) {
            let aTimeStep = aTimeSteps.get(i);
            aRes.push({ timeStep: aTimeStep.x, timestamp: aTimeStep.y });
        }
        return aRes;
    }
    /** Return animation object. */
    get animation() { return new CaeMeshAnimation(this.meshMgr, this); }
    /** Check if mesh entities should be detected/selected. */
    get toHighlightEntities() { var _a, _b; return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.toHighlightEntities(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /**
     * Return entities selection color, "#CCCCCC33" by default.
     * Combined from selection shading color/transparency.
     * Kept for API compatibility.
     */
    get highlightColor() {
        if (!this.meshMgr) {
            return '';
        }
        const aColorStr = this.meshMgr.getMeshShadingColor(this._myObjId, "", CaeMeshHighlightType.Selection);
        let aColor = ColorTools.convertColorStringToRgba(aColorStr);
        aColor.a = (1.0 - this.meshMgr.getMeshShadingTransparency(this._myObjId, "", CaeMeshHighlightType.Selection)) * 255;
        return ColorTools.convertRgbaToColorString(aColor);
    }
    /** Check whether the mesh draws a grid. */
    get toShow2DGrid() { var _a, _b; return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.toShow2DGrid(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Get axes names and tick mark labels. */
    get axesNamingParams() { var _a, _b; return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getAxesNaming(this._myObjId)) !== null && _b !== void 0 ? _b : new CaeMesh2DAxesNamingInfo(); }
    /** Shortcut to interactive CAE mesh context. */
    get meshMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveCaeMeshManager) !== null && _b !== void 0 ? _b : null; }
    /** @internal Dump mesh into a file. */
    get doDump() {
        let aFilePath = `${this._myObjId}.mdb`;
        return this.save(aFilePath) ? aFilePath : '';
    }
    /** Set object's selection mode. */
    set selectionMode(theMode) {
        var _a, _b;
        (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.setSelectionMode(this._myObjId, theMode);
    }
    /** Set active time index. */
    set activeTimeStep(theTimeStep) {
        var _a;
        (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setMeshActiveTimeStep(this._myObjId, theTimeStep);
    }
    /** Set available time indices. */
    set availableTimeSteps(theTimeSteps) {
        if (this._myViewer == null || this.meshMgr == null) {
            return;
        }
        let aTimeSteps = new Array();
        for (let aTime of theTimeSteps) {
            aTimeSteps.push({ x: aTime.timeStep, y: aTime.timestamp });
        }
        this.meshMgr.setAvailableMeshTimeSteps(this._myObjId, aTimeSteps);
    }
    /** Define if properties data is stored in MDB file and can be loaded/unloaded on demand. */
    set toCacheProperties(theToCache) {
        if (this.meshMgr == null) {
            this._toCacheProperties = theToCache;
        }
    }
    /** Define if mesh entities should be detected/selected. */
    set toHighlightEntities(theToHighlight) { var _a; (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setHighlightEntities(this._myObjId, theToHighlight); }
    /**
     * Set entities selection color.
     * Affect selection shading color/transparency.
     * Kept for API compatibility.
     */
    set highlightColor(theColor) {
        if (!this.meshMgr) {
            return;
        }
        const aColor = ColorTools.convertColorStringToRgba(theColor);
        this.meshMgr.setMeshShadingTransparency(this._myObjId, "", 1.0 - aColor.a / 255.0, CaeMeshHighlightType.Selection);
        this.meshMgr.setMeshShadingColor(this._myObjId, "", theColor, CaeMeshHighlightType.Selection);
    }
    /** Set whether the mesh draws a grid. */
    set toShow2DGrid(theToShow) { var _a; (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setShow2DGrid(this._myObjId, theToShow); }
    /** Set axes names and tick mark labels. */
    set axesNamingParams(theInfo) {
        var _a;
        (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setAxesNaming(this._myObjId, theInfo);
    }
    /**
     * Probe file header for known mesh formats.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ArrayBuffer} theArray buffer to open
     * @param {string} theFilePath the file path to map
     * @return {string} detected format (if supported) or an empty string
     */
    static probeFileFormat(theViewer, theArray, theFilePath) {
        var _a, _b;
        let aHeader = new Uint8Array(theArray);
        aHeader = aHeader.subarray(0, Math.min(aHeader.length, 4096));
        const aDataBuffer = theViewer._allocateArrayU8(aHeader);
        return (_b = (_a = theViewer.interactiveCaeMeshManager) === null || _a === void 0 ? void 0 : _a.probeFileFormat(theFilePath, aDataBuffer, aHeader.length, true)) !== null && _b !== void 0 ? _b : '';
    }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveCaeMesh();
        aCopy.wrap(this._myViewer, anObjId);
        this.objMgr.addInteractiveObject(aCopy);
        return aCopy;
    }
    /** Create new mesh presentation of the specified type. */
    createPresentation(theType, thePrsId = '') {
        var _a, _b;
        let anId = (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.createPresentation(this._myObjId, theType, thePrsId)) !== null && _b !== void 0 ? _b : '';
        return anId !== '' ? new CaeMeshPresentation(this.meshMgr, this, anId) : null;
    }
    /** Remove mesh presentation. */
    removePresentation(thePrs) {
        if (this.meshMgr == null || thePrs == null) {
            return;
        }
        this.meshMgr.removePresentation(this._myObjId, thePrs.id);
    }
    /** Get presentation by id. */
    getPresentation(thePrsId) {
        return new CaeMeshPresentation(this.meshMgr, this, thePrsId);
    }
    /** Return mesh property. */
    property(thePropertyId) {
        var _a, _b;
        const aPropInfo = (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshProperty(this._myObjId, thePropertyId)) !== null && _b !== void 0 ? _b : new CaeMeshPropInfo();
        return aPropInfo.propId !== '' ? aPropInfo : null;
    }
    /** Return mesh property component. */
    propertyComponent(thePropertyId, theCompIndex) {
        var _a, _b;
        const aCompInfo = (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPropertyComponent(this._myObjId, thePropertyId, theCompIndex)) !== null && _b !== void 0 ? _b : new CaeMeshPropCompInfo();
        return aCompInfo.index !== -1 ? aCompInfo : null;
    }
    /** Return value range for mesh property. */
    getPropertyValueRange(thePropertyId, theComponentIndex) {
        var _a, _b;
        return (_b = (_a = this.propertyComponent(thePropertyId, theComponentIndex)) === null || _a === void 0 ? void 0 : _a.valueRange) !== null && _b !== void 0 ? _b : { x: 0, y: 0 };
    }
    /** Return name for mesh property. */
    getPropertyName(thePropertyId) {
        var _a, _b;
        return (_b = (_a = this.property(thePropertyId)) === null || _a === void 0 ? void 0 : _a.name) !== null && _b !== void 0 ? _b : '';
    }
    /** Return descrition for mesh property. */
    getPropertyDescription(thePropertyId) {
        var _a, _b;
        return (_b = (_a = this.property(thePropertyId)) === null || _a === void 0 ? void 0 : _a.description) !== null && _b !== void 0 ? _b : '';
    }
    /** Return name for mesh property component. */
    getComponentName(thePropertyId, theComponentIndex) {
        var _a, _b;
        return (_b = (_a = this.propertyComponent(thePropertyId, theComponentIndex)) === null || _a === void 0 ? void 0 : _a.name) !== null && _b !== void 0 ? _b : '';
    }
    /** Return descrition for mesh property component. */
    getComponentDescription(thePropertyId, theComponentIndex) {
        var _a, _b;
        return (_b = (_a = this.propertyComponent(thePropertyId, theComponentIndex)) === null || _a === void 0 ? void 0 : _a.description) !== null && _b !== void 0 ? _b : '';
    }
    /** Return display units for specified mesh property. */
    getPropertyDisplayUnits(thePropertyId, theComponentIndex) {
        var _a, _b;
        return (_b = (_a = this.propertyComponent(thePropertyId, theComponentIndex)) === null || _a === void 0 ? void 0 : _a.displayUnits) !== null && _b !== void 0 ? _b : '';
    }
    /** Return true if mesh property is elemental. */
    isPropertyElemental(thePropertyId) {
        var _a, _b;
        return (_b = (_a = this.property(thePropertyId)) === null || _a === void 0 ? void 0 : _a.isElemental) !== null && _b !== void 0 ? _b : false;
    }
    /** Return true if mesh property is a group. */
    isPropertyGroup(thePropertyId) {
        var _a;
        return ((_a = this.property(thePropertyId)) === null || _a === void 0 ? void 0 : _a.elemType) === CaeMeshPropElemType.Group;
    }
    /** Return true if mesh property contains one scalar component. */
    isPropertySingleScalar(thePropertyId) {
        const aPropInfo = this.property(thePropertyId);
        if (aPropInfo == null) {
            return false;
        }
        return aPropInfo.type === CaeMeshPropType.ScalarSet && aPropInfo.comps.length === 1;
    }
    /** Find property id by name. */
    getPropertyIdByName(thePropertyName) {
        var _a, _b;
        return (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.getMeshPropertyIdByName(this._myObjId, thePropertyName)) !== null && _b !== void 0 ? _b : '';
    }
    /** Return map of groups with non-zero number of entities. */
    usedGroups(thePropertyId) {
        let aRes = new Map();
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
    getDetectedEntity() {
        var _a, _b, _c;
        return (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getDetectedMeshEntity(this._myObjId)) !== null && _c !== void 0 ? _c : new SelectionMeshEntity();
    }
    /** Return selected sub entities. */
    getSelectedEntities() {
        var _a, _b, _c;
        let aRes = new Array();
        let aSelSubEntities = (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getSelectedMeshEntities(this._myObjId)) !== null && _c !== void 0 ? _c : null;
        if (aSelSubEntities == null) {
            return aRes;
        }
        for (let i = 0; i < aSelSubEntities.size(); i++) {
            let aSelSubEntity = aSelSubEntities.get(i);
            if (aSelSubEntity.entityId === -1) {
                continue;
            }
            aRes.push(aSelSubEntity);
        }
        return aRes;
    }
    /** Select sub entity. */
    async setSelectedEntity(theEntity) {
        var _a;
        if (this._myViewer == null) {
            return false;
        }
        if (theEntity.refMeshData != null) {
            theEntity.refMeshDataPath = `${this.getFilePathFromObjId()}_ent_${theEntity.prsId}${theEntity.geomId}${theEntity.entityId}`;
            try {
                await this._myViewer.uploadArrayBuffer(theEntity.refMeshData, theEntity.refMeshDataPath, false);
            }
            catch (theErr) {
                return Promise.reject(theErr);
            }
        }
        (_a = this._myViewer.selectionManager) === null || _a === void 0 ? void 0 : _a.setSelectedMeshEntity(this._myObjId, theEntity);
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
    async loadUrlAsync(theViewer, theFileUrl) {
        var _a, _b;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId(theFileUrl)) !== null && _b !== void 0 ? _b : theFileUrl;
        }
        const aFilePath = this.getFilePathFromObjId();
        try {
            await theViewer.uploadUrl(theFileUrl, aFilePath, false);
        }
        catch (theErr) {
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
    async loadBufferAsync(theViewer, theArray, theFilePath) {
        var _a, _b;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId(theFilePath)) !== null && _b !== void 0 ? _b : theFilePath;
        }
        const aFilePath = this.getFilePathFromObjId();
        try {
            await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
        }
        catch (theErr) {
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
    async loadPropertiesAsync(theViewer, theArray) {
        this._myViewer = theViewer;
        let aFilePath = this.getFilePathFromObjId();
        let i = 0;
        while (this._myViewer.checkFile(`${aFilePath}_prop_${i}`)) {
            i++;
        }
        aFilePath += `_prop_${i}`;
        try {
            await theViewer.uploadArrayBuffer(theArray, aFilePath, false);
        }
        catch (theErr) {
            return Promise.reject(theErr);
        }
        return this.loadProperties(aFilePath);
    }
    /** Dump mesh into a file. */
    save(theFile = '') {
        var _a, _b, _c, _d;
        let aFilePath = theFile;
        if (aFilePath === '' && this._myObjId !== '') {
            aFilePath = `${this._myObjId}.mdb`;
        }
        return ((_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.saveModelMesh(this._myObjId, aFilePath)) !== null && _b !== void 0 ? _b : false) &&
            ((_d = (_c = this._myViewer) === null || _c === void 0 ? void 0 : _c.makeFileLink(aFilePath)) !== null && _d !== void 0 ? _d : false);
    }
    ;
    /** Remove all properties. */
    removeProperties(theWithMetadata = true) {
        var _a;
        (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.removeMeshProperties(this._myObjId, theWithMetadata);
        if (theWithMetadata) {
            this.unlinkLoadedFiles();
        }
    }
    /** Remove object and invalidates it's id. */
    remove() {
        const aRes = super.remove();
        this.unlinkLoadedFiles();
        return aRes;
    }
    /** Set to use custom geometry for mesh presentations. */
    setUseGenGeometry() { var _a; (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setUseMeshGenGeometry(this._myObjId); }
    /** Set to use test custom geometry for mesh presentations. */
    setUseTestCacheGenGeometry() { var _a; (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.setUseMeshTestCacheGenGeometry(this._myObjId); }
    /** Unlink related files. */
    unlinkLoadedFiles() {
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
    openMeshFromFile(theFilePath) {
        var _a, _b, _c, _d, _e;
        const isDone = (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.openModelMesh(this._myObjId, theFilePath, this._toCacheProperties)) !== null && _b !== void 0 ? _b : false;
        if (!isDone) {
            (_c = this._myViewer) === null || _c === void 0 ? void 0 : _c.unlinkFile(theFilePath);
            return Promise.reject(new Error('Open failed'));
        }
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
        if (this._toCacheProperties) {
            this._loadedFiles.push(theFilePath);
        }
        else {
            (_e = this._myViewer) === null || _e === void 0 ? void 0 : _e.unlinkFile(theFilePath);
        }
        return Promise.resolve(true);
    }
    /** Load properties from the file and append to the current object. */
    loadProperties(theFilePath) {
        var _a, _b, _c, _d;
        const isDone = (_b = (_a = this.meshMgr) === null || _a === void 0 ? void 0 : _a.loadMeshProperties(this._myObjId, theFilePath)) !== null && _b !== void 0 ? _b : false;
        if (!isDone) {
            (_c = this._myViewer) === null || _c === void 0 ? void 0 : _c.unlinkFile(theFilePath);
            return Promise.reject(new Error('Load failed'));
        }
        if (this._toCacheProperties) {
            this._loadedFiles.push(theFilePath);
        }
        else {
            (_d = this._myViewer) === null || _d === void 0 ? void 0 : _d.unlinkFile(theFilePath);
        }
        return Promise.resolve(true);
    }
}
;
//# sourceMappingURL=InteractiveCaeMesh.js.map