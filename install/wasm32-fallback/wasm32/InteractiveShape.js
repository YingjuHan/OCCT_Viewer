import { Vec3, ShapeInfo, Trsf, ShapeEnum } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
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
export var ShapeObjectPrsMode;
(function (ShapeObjectPrsMode) {
    /** Hollow wireframe geometry (face edges). */
    ShapeObjectPrsMode[ShapeObjectPrsMode["Wireframe"] = 0] = "Wireframe";
    /** Shaded interior with smooth normals. */
    ShapeObjectPrsMode[ShapeObjectPrsMode["Shaded"] = 1] = "Shaded";
    /** Shaded interior with smooth normals and face edges. */
    ShapeObjectPrsMode[ShapeObjectPrsMode["ShadedFaceBoundaries"] = 2] = "ShadedFaceBoundaries";
    /** Shaded interior with flat normals. */
    ShapeObjectPrsMode[ShapeObjectPrsMode["ShadedFlat"] = 3] = "ShadedFlat";
    /** Shaded interior with triangulation edges. */
    ShapeObjectPrsMode[ShapeObjectPrsMode["ShadedTessellation"] = 4] = "ShadedTessellation";
    /** Hidden line removal (HLR). */
    ShapeObjectPrsMode[ShapeObjectPrsMode["HiddenLine"] = 5] = "HiddenLine";
})(ShapeObjectPrsMode || (ShapeObjectPrsMode = {}));
;
/**
 * ShapeObjectSelMode represents selection mode of {@link InteractiveShape}, from `InteractiveShapeSelMode`.
 * It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
 */
export var ShapeObjectSelMode;
(function (ShapeObjectSelMode) {
    /** Selection of whole shape object. */
    ShapeObjectSelMode[ShapeObjectSelMode["Global"] = 0] = "Global";
    /** Selection of shape vertices. */
    ShapeObjectSelMode[ShapeObjectSelMode["Vertex"] = 1] = "Vertex";
    /** Selection of shape edges. */
    ShapeObjectSelMode[ShapeObjectSelMode["Edge"] = 2] = "Edge";
    /** Selection of shape wires. */
    ShapeObjectSelMode[ShapeObjectSelMode["Wire"] = 4] = "Wire";
    /** Selection of shape faces. */
    ShapeObjectSelMode[ShapeObjectSelMode["Face"] = 8] = "Face";
    /** Selection of shape shells. */
    ShapeObjectSelMode[ShapeObjectSelMode["Shell"] = 16] = "Shell";
    /** Selection of shape solids. */
    ShapeObjectSelMode[ShapeObjectSelMode["Solid"] = 32] = "Solid";
    /** Selection of shape compsolids. */
    ShapeObjectSelMode[ShapeObjectSelMode["CompSolid"] = 64] = "CompSolid";
    /** Selection of groups. */
    ShapeObjectSelMode[ShapeObjectSelMode["Group"] = 128] = "Group";
})(ShapeObjectSelMode || (ShapeObjectSelMode = {}));
;
;
export class ShapeMassProps {
    static parseJson(theJson) {
        var _a;
        if (!theJson) {
            return null;
        }
        let aParsed;
        try {
            aParsed = JSON.parse(theJson);
        }
        catch (_b) {
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
            area: typeof aParsed.area === 'number' ? aParsed.area : Number((_a = aParsed.area) !== null && _a !== void 0 ? _a : 0),
            volume,
            centroid: { x: aCentroid.x, y: aCentroid.y, z: aCentroid.z },
            centroidSource,
            volumeValid,
        };
    }
}
;
/**
 * @internal Class to define detected/selected shape elements.
 */
export class SelectionShapeElems {
    constructor() {
        this.groups = '';
        this.compSolids = '';
        this.solids = '';
        this.shells = '';
        this.faces = '';
        this.wires = '';
        this.edges = '';
        this.verts = '';
    }
    /** Convert selection mode to shape enum. */
    static selModeToShapeEnum(theType) {
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
    static shapeEnumToSelMode(theType) {
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
    static getIndices(theElems, theType) {
        return SelectionShapeElems.indexArrayFromString(SelectionShapeElems.getIndicesString(theElems, theType));
    }
    /** Set indices of specified type. */
    static setIndices(theElems, theType, theIds) {
        SelectionShapeElems.setIndicesString(theElems, theType, SelectionShapeElems.indexArrayToString(theIds));
    }
    /** Get indices of specified type. */
    static getIndicesString(theElems, theType) {
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
    static setIndicesString(theElems, theType, theIds) {
        switch (theType) {
            case ShapeObjectSelMode.Group:
                theElems.groups = theIds;
                break;
            case ShapeObjectSelMode.CompSolid:
                theElems.compSolids = theIds;
                break;
            case ShapeObjectSelMode.Solid:
                theElems.solids = theIds;
                break;
            case ShapeObjectSelMode.Shell:
                theElems.shells = theIds;
                break;
            case ShapeObjectSelMode.Face:
                theElems.faces = theIds;
                break;
            case ShapeObjectSelMode.Wire:
                theElems.wires = theIds;
                break;
            case ShapeObjectSelMode.Edge:
                theElems.edges = theIds;
                break;
            case ShapeObjectSelMode.Vertex:
                theElems.verts = theIds;
                break;
            default: break;
        }
    }
    /** Parse index list from the string. */
    static indexArrayFromString(theIndices) {
        let anElemIdsArray = [];
        const aSelIdsArr = theIndices.length !== 0 ? theIndices.split('\n') : [];
        for (const aSelIdIter of aSelIdsArr) {
            anElemIdsArray.push(parseInt(aSelIdIter, 10));
        }
        return anElemIdsArray;
    }
    /** Format index list into a string. */
    static indexArrayToString(theIndices) {
        let anElemIds = '';
        for (const anIdIter of theIndices) {
            anElemIds += anElemIds.length !== 0 ? '\n' : '';
            anElemIds += anIdIter.toString(10);
        }
        return anElemIds;
    }
    /** Get indices of specified type. */
    getIndices(theType) {
        return SelectionShapeElems.indexArrayFromString(SelectionShapeElems.getIndicesString(this, theType));
    }
    /** Set indices of specified type. */
    setIndices(theType, theIds) {
        SelectionShapeElems.setIndicesString(this, theType, SelectionShapeElems.indexArrayToString(theIds));
    }
}
;
/**
 * Auxiliary class for accessing InteractiveShape subshapes.
 */
export class SubshapeManager {
    /** Constructor. */
    constructor(thePrs, theType) {
        this.myShapePrs = thePrs;
        this.myShapeType = theType;
    }
    /** Iteratable interface. */
    *[Symbol.iterator]() {
        const aLen = this.length;
        for (let i = 0; i < aLen; ++i) {
            yield this.getShapeInfo(Number(i));
        }
    }
    /** Return shape type. */
    get shapeType() { return this.myShapeType; }
    /** Return the number of available subshapes. */
    get length() { return this.defined.length; }
    /** Return the list with indices of available subshapes. */
    get defined() { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false); }
    /** Return the list with indices of free subshapes. */
    get free() { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, true); }
    /** Return the list with indices of disabled subshapes. */
    get disabled() { return this.myShapePrs.getDisabledSubshapes(this.myShapeType, false); }
    /** Return the list with indices of enabled subshapes. */
    get enabled() { return this.myShapePrs.getDisabledSubshapes(this.myShapeType, true); }
    /** Return the list with indices of selected subshapes */
    get selected() { return this.myShapePrs.getSelectedSubshapes(this.myShapeType); }
    /** Return index list of colored subshapes. */
    get colored() { return this.myShapePrs.getColoredSubshapes(this.myShapeType); }
    /** Return the last detected subshape index, or `-1` if unspecified. */
    get lastDetected() {
        const aShape = this.myShapePrs.getDetectedShapeElem();
        return aShape && aShape.entityType === this.myShapeType ? aShape.entityId : -1;
    }
    /** Return subshape info. */
    get shapeInfo() {
        return this.myShapePrs.getSubshapesInfo(this.myShapeType, this.defined, true);
    }
    /** Return index list of visible subshapes. */
    get visible() { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false, ShapeObjectSelMode.Global, -1, 1); }
    /** Return index list of hidden subshapes. */
    get hidden() { return this.myShapePrs.getSubshapesIndexes(this.myShapeType, false, ShapeObjectSelMode.Global, -1, 0); }
    /** Set the list with indices of disabled subshapes. */
    set disabled(theList) { this.myShapePrs.setDisabledSubshapes(this.myShapeType, false, theList); }
    /** Set the list with indices of enabled subshapes. */
    set enabled(theList) { this.myShapePrs.setDisabledSubshapes(this.myShapeType, true, theList); }
    /** Set the list with indices of selected subshapes of this type but keeps selection of subshapes of other types. */
    set selected(theList) {
        let aSelElemIds = this.myShapePrs.getSelectedElems();
        SelectionShapeElems.setIndices(aSelElemIds, this.myShapeType, theList);
        this.myShapePrs.setSelectedShapeElems(aSelElemIds);
    }
    /** Set the list with indices of selected subshapes and resets selection of all other subshape types. */
    set selectedOnly(theList) {
        let aSelElemIds = new SelectionShapeElems();
        aSelElemIds.setIndices(this.myShapeType, theList);
        this.myShapePrs.setSelectedShapeElems(aSelElemIds);
    }
    /** Set index list of visible subshapes (makes hidden unspecified subshapes). */
    set visible(theList) { this.setSubshapesHidden(theList, false, true); }
    /** Set index list of hidden subshapes (makes visible unspecified subshapes). */
    set hidden(theList) { this.setSubshapesHidden(theList, true, true); }
    /** Return indexes of subshapes of specified subshape. */
    getSubshapesIndexes(theIndex, theSubType) {
        return this.myShapePrs.getSubshapesIndexes(theSubType, false, this.myShapeType, theIndex);
    }
    /** Return the subshape info. */
    getShapeInfo(theIndex, theToExplore = true) {
        return this.myShapePrs.getSubshapeInfo(this.myShapeType, theIndex, theToExplore);
    }
    /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
    getConnectedSubshapes(theIndexList) {
        return this.myShapePrs.getConnectedSubshapes(this.myShapeType, theIndexList);
    }
    /** Return subshape color or empty string */
    getSubshapeColor(theIndex) {
        return this.myShapePrs.getSubshapeColor(this.myShapeType, theIndex);
    }
    /** Set or unset subhsape color. */
    setSubshapeColor(theIndex, theColor) {
        this.myShapePrs.setSubshapeColor(this.myShapeType, theIndex.toString(10), theColor);
    }
    /** Set or unset subhsapes color; applied to all subhsapes when null is passed instead of index list */
    setSubshapesColor(theIndices, theColor) {
        this.myShapePrs.setSubshapeColor(this.myShapeType, theIndices != null ? this.myShapePrs.indexArrayToString(theIndices) : '*', theColor);
    }
    /** Unset subshapes colors */
    clearSubshapeColors() { this.myShapePrs.setSubshapeColor(this.myShapeType, '*', ''); }
    /** Return subshape hidden flag. */
    isSubshapeHidden(theIndex) {
        return this.myShapePrs.isSubshapeHidden(this.myShapeType, theIndex);
    }
    /** Set or unset subhsape hidden flag. */
    setSubshapeHidden(theIndex, theIsHidden) {
        this.myShapePrs.setSubshapeHidden(this.myShapeType, theIndex.toString(10), theIsHidden, false);
    }
    /** Set or unset subhsapes hidden flag; applied to all subshapes when null is passed instead of index list */
    setSubshapesHidden(theIndices, theIsHidden, theToInvertOthers = false) {
        this.myShapePrs.setSubshapeHidden(this.myShapeType, theIndices != null ? this.myShapePrs.indexArrayToString(theIndices) : '*', theIsHidden, theToInvertOthers);
    }
    /** Unset subshapes hidden flag. */
    clearSubshapeHidden() { this.myShapePrs.setSubshapeHidden(this.myShapeType, '*', false, false); }
    /** Make a copy of existing object with new id and specified subshape. */
    cloneSubshape(theIndex, theNewObjId = '') {
        let aSelElemIds = new SelectionShapeElems();
        aSelElemIds.setIndices(this.myShapeType, [theIndex]);
        return this.myShapePrs.cloneSubshapes(aSelElemIds, theNewObjId);
    }
    /** Make a copy of existing object with new id and specified subshapes. */
    cloneSubshapes(theIndices, theNewObjId = '') {
        let aSelElemIds = new SelectionShapeElems();
        aSelElemIds.setIndices(this.myShapeType, theIndices);
        return this.myShapePrs.cloneSubshapes(aSelElemIds, theNewObjId);
    }
    /** Dump shape into a file. */
    saveSubshape(theIndex, theFile = '') {
        let anIds = [theIndex];
        return this.saveSubshapes(anIds, theFile);
    }
    ;
    /** Dump shapes into a file. */
    saveSubshapes(theIndices, theFile = '') {
        let aFilePath = theFile;
        if (aFilePath === '' && this.myShapePrs.id !== '') {
            let aSuff = '';
            switch (this.myShapeType) {
                case ShapeObjectSelMode.Group:
                    aSuff = 'g';
                    break;
                case ShapeObjectSelMode.CompSolid:
                    aSuff = 'cs';
                    break;
                case ShapeObjectSelMode.Solid:
                    aSuff = 'so';
                    break;
                case ShapeObjectSelMode.Shell:
                    aSuff = 'sh';
                    break;
                case ShapeObjectSelMode.Face:
                    aSuff = 'f';
                    break;
                case ShapeObjectSelMode.Wire:
                    aSuff = 'w';
                    break;
                case ShapeObjectSelMode.Edge:
                    aSuff = 'e';
                    break;
                case ShapeObjectSelMode.Vertex:
                    aSuff = 'v';
                    break;
                default: break;
            }
            let anIdSuff = theIndices.length === 1 ? theIndices[0].toString() : '';
            aFilePath = `${this.myShapePrs.id}_${aSuff}${anIdSuff}.brep`;
        }
        let aSelElemIds = new SelectionShapeElems();
        aSelElemIds.setIndices(this.myShapeType, theIndices);
        return this.myShapePrs.saveSubshapes(aFilePath, aSelElemIds);
    }
    ;
}
;
/**
 * Proxy interface to access subshapes information
 * using array-alike interface (by index)
 * without instantiating a real array for all subshapes.
 */
class SubshapeManagerProxy extends Array {
    /** Main constructor. */
    constructor(theProxy) {
        super();
        this._myProxy = theProxy;
        return new Proxy(this, {
            get: (theObj, theKey) => {
                if (typeof (theKey) === 'string' && (Number.isInteger(Number(theKey)))) {
                    return theObj._myProxy.getShapeInfo(Number(theKey));
                }
                return theObj._myProxy[theKey];
            },
        });
    }
}
;
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
    /** Empty constructor. */
    constructor(theObjId = '') {
        super(theObjId);
    }
    /** Return shape info string. */
    get infoString() {
        var _a, _b;
        const aTris = this.shapeInfo;
        const aLoc = !Trsf.isIdentity(aTris.location) ? Trsf.format(aTris.location, (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.unitManager) !== null && _b !== void 0 ? _b : null) : 'IDENTITY';
        let anInfo = `Vertices:  ${this.vertices.length.toString()} (free: ${this.vertices.free.length.toString()})
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
    get shapeLocation() {
        return this.getSubshapeInfo(ShapeObjectSelMode.Global, 0, false).location;
    }
    /** Return multiplication of initial root shape location and local transformation of interactive object. */
    get shapeFinalLocation() {
        const aTrsfSh = this.shapeLocation;
        const aTrsfPrs = this.localTransformation;
        const aTrsfRes = Trsf.multiply(aTrsfSh, aTrsfPrs);
        return aTrsfRes;
    }
    /** Return shape info. */
    get shapeInfo() { return this.getSubshapeInfo(ShapeObjectSelMode.Global, 0, true); }
    /** Return object's presentation mode. */
    get presentationMode() { var _a, _b; return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getPresentationMode(this._myObjId)) !== null && _b !== void 0 ? _b : ShapeObjectPrsMode.Shaded; }
    /** Return TRUE if object's material is set. */
    get hasMaterial() {
        const aMat = this.material;
        return aMat.isDefined && aMat.baseColor !== '';
    }
    /** Return object's material. */
    get material() {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getMaterial(this._myObjId)) !== null && _b !== void 0 ? _b : { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
    }
    /** Return object's base color (overrides base implementation). */
    get baseColor() { return this.material.baseColor; }
    /** Return object's metallic factor within `[0.0, 1.0]` range. */
    get materialMetallic() { return this.material.metallic; }
    /** Return object's roughness within `[0.0, 1.0]` range. */
    get materialRoughness() { return this.material.roughness; }
    /** Return object's overcolor (forced color), which overrides base/subshape colors, or an empty string. */
    get forcedColor() { var _a, _b; return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getForcedColor(this._myObjId)) !== null && _b !== void 0 ? _b : ''; }
    /** Return object's overtransparency (forced transparency), which overrides base/subshape transparency, or -1 if unset. */
    get forcedTransparency() { var _a, _b; return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getForcedTransparency(this._myObjId)) !== null && _b !== void 0 ? _b : -1.0; }
    /** Return index list of colored faces */
    get coloredFaces() { return this.getColoredSubshapes(ShapeObjectSelMode.Face); }
    /** Return index list of colored edges */
    get coloredEdges() { return this.getColoredSubshapes(ShapeObjectSelMode.Edge); }
    /** Return index list of colored vertices */
    get coloredVertices() { return this.getColoredSubshapes(ShapeObjectSelMode.Vertex); }
    /** Return object's selection mode. */
    get selectionMode() {
        var _a, _b, _c;
        return (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getSelectionMode(this._myObjId)) !== null && _c !== void 0 ? _c : ShapeObjectSelMode.Global;
    }
    /** Return the last detected subshape index, or `-1` if unspecified. */
    get lastDetectedSubshape() {
        var _a, _b;
        const aShape = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getDetectedShapeElem(this._myObjId);
        return aShape && aShape.entityType === this.selectionMode ?
            aShape.entityId :
            -1;
    }
    /** Return the last detected subshape info. */
    get lastDetectedSubshapeInfo() {
        var _a, _b, _c;
        const aShape = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getDetectedShapeElem(this._myObjId);
        return aShape && aShape.entityId !== -1 ?
            (_c = this.getSubshapeInfo(aShape.entityType, aShape.entityId, true)) !== null && _c !== void 0 ? _c : new ShapeInfo() :
            new ShapeInfo();
    }
    /** Return the last detected subshape dump. */
    get lastDetectedSubshapeDump() {
        var _a, _b;
        const anId = this.lastDetectedSubshape;
        return anId !== -1 ?
            (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeDump(this._myObjId, this.selectionMode, anId)) !== null && _b !== void 0 ? _b : '' :
            '';
    }
    /**
     * Return the list with indices of selected subshapes of active selection mode
     * or empty list of no local selection is active / multiple selection modes are active.
     */
    get selectedSubshapes() {
        return this.selectionMode !== ShapeObjectSelMode.Global ?
            this.getSelectedSubshapes(this.selectionMode) :
            [];
    }
    /**
     * Return the list with selected subshapes info within active selection mode
     * or empty list of no local selection is active / multiple selection modes are active.
     */
    get selectedSubshapesInfo() {
        if (this.selectionMode === ShapeObjectSelMode.Global || this.shapeMgr == null) {
            return [];
        }
        let anInfoArray = new Array();
        for (let aTypeIter of [ShapeObjectSelMode.Group,
            ShapeObjectSelMode.CompSolid, ShapeObjectSelMode.Solid, ShapeObjectSelMode.Shell, ShapeObjectSelMode.Face,
            ShapeObjectSelMode.Wire, ShapeObjectSelMode.Edge, ShapeObjectSelMode.Vertex]) {
            const aSubShapes = this.getSelectedSubshapes(aTypeIter);
            const anInfos = this.getSubshapesInfo(aTypeIter, aSubShapes, true);
            anInfoArray = anInfoArray.concat(anInfos);
        }
        return anInfoArray;
    }
    /** Return the list with indices of available subshapes within active selection mode. */
    get definedSubshapes() {
        return this.selectionMode !== ShapeObjectSelMode.Global ?
            this.getSubshapesIndexes(this.selectionMode, false) :
            [];
    }
    /** Return subshape managers. */
    get subshapes() {
        if (!this._mySubshapeMgr) {
            this._mySubshapeMgr = [this.groups, this.compSolids, this.solids, this.shells, this.faces, this.wires, this.edges, this.vertices];
        }
        return this._mySubshapeMgr;
    }
    /** Return selection groups subshape manager. */
    get groups() {
        if (!this._myGroupsMgr) {
            this._myGroupsMgr = new SubshapeManager(this, ShapeObjectSelMode.Group);
        }
        return this._myGroupsMgr;
    }
    /** Return composite solid subshape manager. */
    get compSolids() {
        if (!this._myCompSolidMgr) {
            this._myCompSolidMgr = new SubshapeManager(this, ShapeObjectSelMode.CompSolid);
        }
        return this._myCompSolidMgr;
    }
    /** Return solid subshape manager. */
    get solids() {
        if (!this._mySolidMgr) {
            this._mySolidMgr = new SubshapeManager(this, ShapeObjectSelMode.Solid);
        }
        return this._mySolidMgr;
    }
    /** Return shell subshape manager. */
    get shells() {
        if (!this._myShellMgr) {
            this._myShellMgr = new SubshapeManager(this, ShapeObjectSelMode.Shell);
        }
        return this._myShellMgr;
    }
    /** Return face subshape manager. */
    get faces() {
        if (!this._myFaceMgr) {
            this._myFaceMgr = new SubshapeManager(this, ShapeObjectSelMode.Face);
        }
        return this._myFaceMgr;
    }
    /** Return wire subshape manager. */
    get wires() {
        if (!this._myWireMgr) {
            this._myWireMgr = new SubshapeManager(this, ShapeObjectSelMode.Wire);
        }
        return this._myWireMgr;
    }
    /** Return edge subshape manager. */
    get edges() {
        if (!this._myEdgeMgr) {
            this._myEdgeMgr = new SubshapeManager(this, ShapeObjectSelMode.Edge);
        }
        return this._myEdgeMgr;
    }
    /** Return vertex subshape manager. */
    get vertices() {
        if (!this._myVertMgr) {
            this._myVertMgr = new SubshapeManager(this, ShapeObjectSelMode.Vertex);
        }
        return this._myVertMgr;
    }
    /** Return composite solid subshape manager with interface for accessing to subshape by index. */
    get expCompSolids() { return new SubshapeManagerProxy(this.compSolids); }
    /** Return solid subshape manager with interface for accessing to subshape by index. */
    get expSolids() { return new SubshapeManagerProxy(this.solids); }
    /** Return shell subshape manager with interface for accessing to subshape by index. */
    get expShells() { return new SubshapeManagerProxy(this.shells); }
    /** Return face subshape manager with interface for accessing to subshape by index. */
    get expFaces() { return new SubshapeManagerProxy(this.faces); }
    /** Return wire subshape manager with interface for accessing to subshape by index. */
    get expWires() { return new SubshapeManagerProxy(this.wires); }
    /** Return edge subshape manager with interface for accessing to subshape by index. */
    get expEdges() { return new SubshapeManagerProxy(this.edges); }
    /** Return vertex subshape manager with interface for accessing to subshape by index. */
    get expVertices() { return new SubshapeManagerProxy(this.vertices); }
    /** Shortcut to shape context. */
    get shapeMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveShapeManager) !== null && _b !== void 0 ? _b : null; }
    /** @internal Dump shape into a file. */
    get doDump() {
        let aFilePath = `${this._myObjId}.brep`;
        return this.save(aFilePath) ? aFilePath : '';
    }
    /**
     * Set initial root shape location WITHOUT changing local transformation of interactive object.
     * This method will recompute presentation of the shape (use localTransformation for interactive manipulations instead!).
     */
    set shapeLocation(theTrsf) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setShapeLocation(this._myObjId, theTrsf);
    }
    /** Set object's presentation mode. */
    set presentationMode(theMode) { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setPresentationMode(this._myObjId, theMode); }
    /** Set or unsets object's overcolor. */
    set forcedColor(theColor) { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setForcedColor(this._myObjId, theColor); }
    /** Set or unsets object's overtransparency. */
    set forcedTransparency(theTransp) { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setForcedTransparency(this._myObjId, theTransp); }
    /** Set object's material. */
    set material(theMat) { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setMaterial(this._myObjId, theMat); }
    /** Set object's base color (overrides base implementation). */
    set baseColor(theColor) {
        let aMat = this.material;
        aMat.isDefined = true;
        aMat.baseColor = theColor;
        this.material = aMat;
    }
    /** Set object's metallic factor. */
    set materialMetallic(theFactor) {
        let aMat = this.material;
        aMat.isDefined = true;
        aMat.metallic = theFactor;
        this.material = aMat;
    }
    /** Set object's roughness. */
    set materialRoughness(theFactor) {
        let aMat = this.material;
        aMat.isDefined = true;
        aMat.roughness = theFactor;
        this.material = aMat;
    }
    /** Set object's selection mode. */
    set selectionMode(theMode) { var _a, _b; (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.setSelectionMode(this._myObjId, theMode); }
    /**
     * Set the list with indices of selected subhsapes of active selection mode
     * and resets previous selecction.
     */
    set selectedSubshapes(theSubshapes) {
        let aSelElemIds = new SelectionShapeElems();
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
    static probeFileFormat(theViewer, theArray, theFilePath) {
        var _a, _b;
        let aHeader = new Uint8Array(theArray);
        aHeader = aHeader.subarray(0, Math.min(aHeader.length, 4096));
        const aDataBuffer = theViewer._allocateArrayU8(aHeader);
        return (_b = (_a = theViewer.interactiveShapeManager) === null || _a === void 0 ? void 0 : _a.probeFileFormat(theFilePath, aDataBuffer, aHeader.length, true)) !== null && _b !== void 0 ? _b : '';
    }
    /** Get material preset {@link MaterialPreset}. */
    static getMaterialPreset(theViewer, thePreset) {
        var _a, _b;
        return (_b = (_a = theViewer.interactiveShapeManager) === null || _a === void 0 ? void 0 : _a.getMaterialPreset(thePreset)) !== null && _b !== void 0 ? _b : { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
    }
    /** Get default shape material. */
    static getDefaultMaterial(theViewer) {
        var _a, _b;
        return (_b = (_a = theViewer.interactiveShapeManager) === null || _a === void 0 ? void 0 : _a.getDefaultMaterial()) !== null && _b !== void 0 ? _b : { isDefined: false, baseColor: '', emissiveFactor: Vec3.zero(), metallic: 0, roughness: 0, refractionIndex: 0 };
    }
    /** Set default shape material. */
    static setDefaultMaterial(theViewer, theMat) {
        var _a;
        return (_a = theViewer.interactiveShapeManager) === null || _a === void 0 ? void 0 : _a.setDefaultMaterial(theMat);
    }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
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
    buildPoint(theViewer, thePnt) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('point')) !== null && _b !== void 0 ? _b : 'point';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildPoint(this._myObjId, thePnt);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
    }
    /**
     * Create line segment edge shape.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 point one
     * @param {IVec3} thePnt2 point two
     */
    buildLineSegment(theViewer, thePnt1, thePnt2) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('segment')) !== null && _b !== void 0 ? _b : 'segment';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildLineSegment(this._myObjId, thePnt1, thePnt2);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
    }
    /**
     * Create box shape from parameters (based on `BRepPreviewAPI_MakeBox` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IAx2}  theAx2  box lower corner (shape origin)
     * @param {IVec3} theDims box dimensions
     */
    buildBox(theViewer, theAx2, theDims) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('box')) !== null && _b !== void 0 ? _b : 'box';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildBox(this._myObjId, theAx2, theDims);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
    }
    /**
     * Create sphere shape from parameters (based on `BRepPrimAPI_MakeSphere` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IAx2}   theAx2    sphere center (shape origin)
     * @param {number} theRadius sphere radius
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildSphere(theViewer, theAx2, theRadius, theDevAngle = 20.0 * Math.PI / 180.0) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('sphere')) !== null && _b !== void 0 ? _b : 'sphere';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildSphere(this._myObjId, theAx2, theRadius, theDevAngle);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
    }
    /**
     * Create cylinder shape from parameters (based on `BRepPrimAPI_MakeCylinder` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {ICylinderParams} theParams cylinder parameters
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildCylinder(theViewer, theParams, theDevAngle = 20.0 * Math.PI / 180.0) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('cylinder')) !== null && _b !== void 0 ? _b : 'cylinder';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildCylinder(this._myObjId, theParams, theDevAngle);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
    }
    /**
     * Create cone shape from parameters (based on `BRepPrimAPI_MakeCone` from C++ OCCT).
     * @param {SCwebviewer} theViewer viewer module
     * @param {IConeParams} theParams cone parameters
     * @param {number} theDevAngle mesh deviation angle parameter
     */
    buildCone(theViewer, theParams, theDevAngle = 20.0 * Math.PI / 180.0) {
        var _a, _b, _c, _d;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('cone')) !== null && _b !== void 0 ? _b : 'cone';
        }
        (_c = this.shapeMgr) === null || _c === void 0 ? void 0 : _c.buildCone(this._myObjId, theParams, theDevAngle);
        (_d = this.objMgr) === null || _d === void 0 ? void 0 : _d.addInteractiveObject(this);
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
    async openUrlAsync(theViewer, theFileUrl) {
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
    async openBufferAsync(theViewer, theArray, theFilePath) {
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
    async loadUrlAsync(theViewer, theFileUrl = '') {
        if (theFileUrl !== '') {
            try {
                await this.openUrlAsync(theViewer, theFileUrl);
            }
            catch (theErr) {
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
    async loadBufferAsync(theViewer, theArray = null, theFileName = '') {
        if (theArray !== null) {
            try {
                await this.openBufferAsync(theViewer, theArray, theFileName);
            }
            catch (theErr) {
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
    explodeShape(theViewer, theExplodeDepth = -1, theIsAutoNaming = true, theToGenId = true, theToPropagateId = true, theNewShapeIds = []) {
        this._myViewer = theViewer;
        if (this._myObjId === '' || !this.shapeMgr || !this.objMgr) {
            return [];
        }
        const aNewShapeIds = theNewShapeIds.join('\n');
        let anObjArray = [];
        const anExplodedShapes = this.shapeMgr.explodeShape(this._myObjId, theExplodeDepth, theIsAutoNaming, theToGenId, theToPropagateId, aNewShapeIds);
        if (anExplodedShapes.length === 0) {
            return [];
        }
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
    save(theFile = '') {
        var _a, _b, _c, _d;
        let aFilePath = theFile;
        if (aFilePath === '' && this._myObjId !== '') {
            aFilePath = `${this._myObjId}.brep`;
        }
        let aSelElemIds = new SelectionShapeElems();
        return ((_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.saveModelShape(this._myObjId, aSelElemIds, aFilePath)) !== null && _b !== void 0 ? _b : false) &&
            ((_d = (_c = this._myViewer) === null || _c === void 0 ? void 0 : _c.makeFileLink(aFilePath)) !== null && _d !== void 0 ? _d : false);
    }
    ;
    /** Set material preset {@link MaterialPreset} to object. */
    setMaterialPreset(thePreset) {
        if (!this._myViewer) {
            return;
        }
        const aMat = InteractiveShape.getMaterialPreset(this._myViewer, thePreset);
        this.material = aMat;
    }
    /** Return position of vertex subshape of specified index */
    getSubshapeVertex(theVertId) { var _a, _b; return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeVertex(this._myObjId, theVertId)) !== null && _b !== void 0 ? _b : Vec3.invalid(); }
    /** Return face color or empty string */
    getFaceColor(theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, theIndex)) !== null && _b !== void 0 ? _b : '';
    }
    /** Set or unset face color. */
    setFaceColor(theIndex, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, theIndex.toString(10), theColor);
    }
    /** Set or unset faces color; applied to all faces when null is passed instead of index list */
    setFacesColor(theIndices, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, theIndices != null ? this.indexArrayToString(theIndices) : '*', theColor);
    }
    /** Unset faces' colors */
    clearFaceColors() { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Face, '*', ''); }
    /** Return edge color or empty string */
    getEdgeColor(theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, theIndex)) !== null && _b !== void 0 ? _b : '';
    }
    /** Set or unset edge color */
    setEdgeColor(theIndex, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, theIndex.toString(10), theColor);
    }
    /** Set or unset edges color; applied to all edges when null is passed instead of index list */
    setEdgesColor(theIndices, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, theIndices != null ? this.indexArrayToString(theIndices) : '*', theColor);
    }
    /** Unset edges' colors */
    clearEdgeColors() { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Edge, '*', ''); }
    /** Return vertex color or empty string */
    getVertexColor(theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, theIndex)) !== null && _b !== void 0 ? _b : '';
    }
    /** Set or unset vertex color */
    setVertexColor(theIndex, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, theIndex.toString(10), theColor);
    }
    /** Set or unset vertices color; applied to all vertices when null is passed instead of index list */
    setVerticesColor(theIndices, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, theIndices != null ? this.indexArrayToString(theIndices) : '*', theColor);
    }
    /** Unset vertex' colors */
    clearVertexColors() { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, ShapeObjectSelMode.Vertex, '*', ''); }
    /**
     * Remove all selection groups.
     * Tip: do NOT activate group selection mode while managing groups list!
     */
    clearGroups() { var _a; (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.clearSelectionGroups(this._myObjId); }
    /**
     * Add selection group pointint to the list of subshapes of specified type.
     * Tip: do NOT activate group selection mode while managing groups list!
     */
    addGroup(theElemType, theIndexList) {
        var _a, _b;
        let anElems = this.indexArrayToString(theIndexList);
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.addSelectionGroup(this._myObjId, theElemType, anElems)) !== null && _b !== void 0 ? _b : -1;
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
    getSubshapesIndexes(theElemType, theIsFree = false, theParentType = ShapeObjectSelMode.Global, theParentIndex = -1, theVisibility = -1) {
        var _a, _b, _c;
        let aSelElems = (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getSubshapesIndexes(this._myObjId, theElemType, theIsFree, theParentType, theParentIndex, theVisibility)) !== null && _c !== void 0 ? _c : '';
        return SelectionShapeElems.indexArrayFromString(aSelElems);
    }
    /** Clear selected faces/edges/vertices. */
    clearSelectedElems() {
        let aSelElemIds = new SelectionShapeElems();
        this.setSelectedShapeElems(aSelElemIds);
    }
    /** Set the list with indices of selected faces/edges/vertices. */
    setSelectedElems(theFaces, theEdges, theVertices) {
        let aSelElemIds = new SelectionShapeElems();
        aSelElemIds.setIndices(ShapeObjectSelMode.Face, theFaces);
        aSelElemIds.setIndices(ShapeObjectSelMode.Edge, theEdges);
        aSelElemIds.setIndices(ShapeObjectSelMode.Vertex, theVertices);
        this.setSelectedShapeElems(aSelElemIds);
    }
    /** Get subshapes connected to specified subshapes of the same type - for testing purposes. */
    getConnectedSubshapes(theElemType, theIndexList) {
        var _a, _b;
        let anElems = this.indexArrayToString(theIndexList);
        const aRes = (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getConnectedSubshapes(this._myObjId, theElemType, anElems)) !== null && _b !== void 0 ? _b : '';
        return this.indexArrayFromString(aRes);
    }
    /** Parse index list from the string. */
    indexArrayFromString(theIndices) { return SelectionShapeElems.indexArrayFromString(theIndices); }
    /** Format index list into a string. */
    indexArrayToString(theIndices) { return SelectionShapeElems.indexArrayToString(theIndices); }
    /** @internal Return subshape dump or empty string */
    getSubshapeDump(theElemType, theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeDump(this._myObjId, theElemType, theIndex)) !== null && _b !== void 0 ? _b : '';
    }
    getRootShapeInfo(theToExplore) {
        const aMgr = this.shapeMgr;
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
    getAnySubshapeLocation() {
        if (!this.shapeMgr) {
            return null;
        }
        const aTypes = [
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
    getSubshapeInfo(theElemType, theIndex, theToExplore = true) {
        var _a, _b;
        if (theElemType === ShapeObjectSelMode.Global) {
            return this.getRootShapeInfo(theToExplore);
        }
        let anInfo = (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeInfo(this._myObjId, theElemType, theIndex, theToExplore)) !== null && _b !== void 0 ? _b : new ShapeInfo();
        let aRes = Object.assign(new ShapeInfo(), anInfo);
        aRes.objectId = this._myObjId; // avoid duplicating string - rely on JS sharing
        return aRes;
    }
    /** @internal Return subshape info */
    getSubshapesInfo(theElemType, theIndices, theToExplore = true) {
        let aRes = new Array();
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
    getSubshapeColor(theElemType, theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.getSubshapeColor(this._myObjId, theElemType, theIndex)) !== null && _b !== void 0 ? _b : '';
    }
    /** @internal Set or unset subshape color */
    setSubshapeColor(theElemType, theIndices, theColor) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeColor(this._myObjId, theElemType, theIndices, theColor);
    }
    /** @internal Return subshape hidden flag */
    isSubshapeHidden(theElemType, theIndex) {
        var _a, _b;
        return (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.isSubshapeHidden(this._myObjId, theElemType, theIndex)) !== null && _b !== void 0 ? _b : false;
    }
    /** @internal Set or unset subshape hidden flag */
    setSubshapeHidden(theElemType, theIndices, theIsHidden, theToInvertOthers) {
        var _a;
        (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.setSubshapeHidden(this._myObjId, theElemType, theIndices, theIsHidden, theToInvertOthers);
    }
    /** @internal Return index list of colored subshapes */
    getColoredSubshapes(theElemType) {
        if (!this.shapeMgr) {
            return [];
        }
        let aSubshapes = this.getSubshapesIndexes(theElemType, false);
        let aColored = [];
        for (let anIndexIter of aSubshapes) {
            let aColor = this.shapeMgr.getSubshapeColor(this._myObjId, theElemType, anIndexIter);
            if (aColor !== '') {
                aColored.push(anIndexIter);
            }
        }
        return aColored;
    }
    /** @internal Return the last detected subshape index, or `-1` if unspecified. */
    getDetectedShapeElem() {
        var _a, _b;
        return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getDetectedShapeElem(this._myObjId);
    }
    /** @internal Set the list with indices of selected subhsapes. */
    setSelectedShapeElems(theElems) {
        var _a, _b;
        (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.setSelectedShapeElems(this._myObjId, theElems);
    }
    /** @internal Return the list with indices of selected subshapes. */
    getSelectedSubshapes(theElemType) {
        var _a, _b, _c;
        let aSelElems = (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getSelectedShapeElems(this._myObjId, theElemType)) !== null && _c !== void 0 ? _c : '';
        return SelectionShapeElems.indexArrayFromString(aSelElems);
    }
    /** @internal Return the list with indices of disabled subshapes. */
    getDisabledSubshapes(theElemType, theToReverse) {
        var _a, _b, _c;
        let aDisElems = (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.getDisabledShapeElems(this._myObjId, theElemType, theToReverse)) !== null && _c !== void 0 ? _c : '';
        return SelectionShapeElems.indexArrayFromString(aDisElems);
    }
    /** @internal Set the list with indices of disabled subshapes. */
    setDisabledSubshapes(theElemType, theToReverse, theIndexList) {
        var _a, _b;
        let anElems = this.indexArrayToString(theIndexList);
        (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) === null || _b === void 0 ? void 0 : _b.setDisabledShapeElems(this._myObjId, theElemType, theToReverse, anElems);
    }
    /** @internal Get the list with indices of selected faces/edges/vertices. */
    getSelectedElems() {
        let aSelElemIds = new SelectionShapeElems();
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
    saveSubshapes(theFile, theElems) {
        var _a, _b, _c, _d;
        return ((_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.saveModelShape(this._myObjId, theElems, theFile)) !== null && _b !== void 0 ? _b : false) &&
            ((_d = (_c = this._myViewer) === null || _c === void 0 ? void 0 : _c.makeFileLink(theFile)) !== null && _d !== void 0 ? _d : false);
    }
    /** @internal Make a copy of existing object with new id and specified subshapes */
    cloneSubshapes(theElems, theNewObjId = '') {
        let anObjId = this.shapeMgr ? this.shapeMgr.cloneSubshapes(this._myObjId, theElems, theNewObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveShape();
        aCopy.wrap(this._myViewer, anObjId);
        this.objMgr.addInteractiveObject(aCopy);
        return aCopy;
    }
    /** Open shape from file path. */
    openShapeFromFile(theFilePath) {
        var _a, _b, _c;
        const isDone = (_b = (_a = this.shapeMgr) === null || _a === void 0 ? void 0 : _a.openModelShape(this._myObjId, theFilePath)) !== null && _b !== void 0 ? _b : false;
        if (!isDone) {
            return Promise.reject(new Error('Open failed'));
        }
        (_c = this.objMgr) === null || _c === void 0 ? void 0 : _c.addInteractiveObject(this);
        return Promise.resolve(true);
    }
}
;
;
/**
 * Auxiliary tool to initialize JS extensions to InteractiveShapeManager C++ class.
 * @internal
 */
export class InteractiveShapeManagerExtensions {
    static init(theViewer, theMgr) {
        theMgr.saveScene = function (theFile = 'scene.brep') {
            let aSelElemIds = new SelectionShapeElems();
            return theMgr.saveModelShape('*', aSelElemIds, theFile) &&
                theViewer.makeFileLink(theFile);
        };
        // InteractiveShapeManager extensions
        Object.defineProperty(theMgr, 'doSceneDump', {
            get: function () { return this.saveScene('scene.sbrep') ? 'scene.sbrep' : ''; },
        });
        // ShapeInfo extensions
        Object.defineProperty(ShapeInfo, 'getSubshapes', {
            value: function (theInfo, theType) {
                var _a, _b;
                const aParType = SelectionShapeElems.shapeEnumToSelMode(theInfo.shapeType);
                const aSubType = SelectionShapeElems.shapeEnumToSelMode(theType);
                let aSelElems = (_b = (_a = theViewer === null || theViewer === void 0 ? void 0 : theViewer.selectionManager) === null || _a === void 0 ? void 0 : _a.getSubshapesIndexes(theInfo.objectId, aSubType, false, aParType, theInfo.index, -1)) !== null && _b !== void 0 ? _b : '';
                return SelectionShapeElems.indexArrayFromString(aSelElems);
            },
        });
    }
}
;
//# sourceMappingURL=InteractiveShape.js.map