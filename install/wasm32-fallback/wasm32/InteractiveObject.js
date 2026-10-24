import { ColorTools, Vec3, ViewCorner, BndBox3, Trsf } from './CommonTools.js';
import { Subview } from './Subview.js';
import { ViewerEventTarget } from './ViewerEventTarget.js';
import { SceneGraph } from './SceneGraph.js';
/**
 * Font aspect defined by `Font_FontAspect` enumeration.
 */
export var FontAspect;
(function (FontAspect) {
    FontAspect[FontAspect["Regular"] = 0] = "Regular";
    FontAspect[FontAspect["Bold"] = 1] = "Bold";
    FontAspect[FontAspect["Italic"] = 2] = "Italic";
    FontAspect[FontAspect["BoldItalic"] = 3] = "BoldItalic";
})(FontAspect || (FontAspect = {}));
;
/**
 * Presentation Z-Layer defined by `Graphic3d_ZLayerId` enumeration.
 */
export var ZLayerId;
(function (ZLayerId) {
    /** identifier for invalid ZLayer */
    ZLayerId[ZLayerId["UNKNOWN"] = -1] = "UNKNOWN";
    /** underlay for 2D presentations (On-Screen-Display) */
    ZLayerId[ZLayerId["BotOSD"] = -5] = "BotOSD";
    /** default Z-layer for main presentations */
    ZLayerId[ZLayerId["Default"] = 0] = "Default";
    /** overlay for 3D presentations which inherits Depth from previous ZLayer */
    ZLayerId[ZLayerId["Top"] = -2] = "Top";
    /** overlay for 3D presentations with independent Depth */
    ZLayerId[ZLayerId["Topmost"] = -3] = "Topmost";
    /** overlay for 2D presentations (On-Screen-Display) */
    ZLayerId[ZLayerId["TopOSD"] = -4] = "TopOSD";
})(ZLayerId || (ZLayerId = {}));
;
/**
 * This enumeration defines line types.
 * It reflects values of corresponding C++ `Aspect_TypeOfLine` enumeration defined by OCCT.
 */
export var TypeOfLine;
(function (TypeOfLine) {
    /** Hidden, `Aspect_TOL_EMPTY`. */
    TypeOfLine[TypeOfLine["EMPTY"] = -1] = "EMPTY";
    /** Continuous, `Aspect_TOL_SOLID`. */
    TypeOfLine[TypeOfLine["SOLID"] = 0] = "SOLID";
    /** Dashed, `Aspect_TOL_DASH`. */
    TypeOfLine[TypeOfLine["DASH"] = 1] = "DASH";
    /** Dotted, `Aspect_TOL_DOT`. */
    TypeOfLine[TypeOfLine["DOT"] = 2] = "DOT";
    /** Mixed, `Aspect_TOL_DOTDASH`. */
    TypeOfLine[TypeOfLine["DOTDASH"] = 3] = "DOTDASH";
})(TypeOfLine || (TypeOfLine = {}));
;
/**
 * This enumeration defines marker types.
 * It reflects values of corresponding C++ `Aspect_TypeOfMarker` enumeration defined by OCCT.
 */
export var TypeOfMarker;
(function (TypeOfMarker) {
    TypeOfMarker[TypeOfMarker["EMPTY"] = -1] = "EMPTY";
    TypeOfMarker[TypeOfMarker["POINT"] = 0] = "POINT";
    TypeOfMarker[TypeOfMarker["PLUS"] = 1] = "PLUS";
    TypeOfMarker[TypeOfMarker["STAR"] = 2] = "STAR";
    TypeOfMarker[TypeOfMarker["X"] = 3] = "X";
    TypeOfMarker[TypeOfMarker["O"] = 4] = "O";
    TypeOfMarker[TypeOfMarker["O_POINT"] = 5] = "O_POINT";
    TypeOfMarker[TypeOfMarker["O_PLUS"] = 6] = "O_PLUS";
    TypeOfMarker[TypeOfMarker["O_STAR"] = 7] = "O_STAR";
    TypeOfMarker[TypeOfMarker["O_X"] = 8] = "O_X";
    TypeOfMarker[TypeOfMarker["RING1"] = 9] = "RING1";
    TypeOfMarker[TypeOfMarker["RING2"] = 10] = "RING2";
    TypeOfMarker[TypeOfMarker["RING3"] = 11] = "RING3";
    TypeOfMarker[TypeOfMarker["BALL"] = 12] = "BALL";
})(TypeOfMarker || (TypeOfMarker = {}));
;
;
;
/**
 * MeshEntityType represents type of {@link InteractiveCaeMesh} sub entity.
 */
export var MeshEntityType;
(function (MeshEntityType) {
    MeshEntityType[MeshEntityType["Unknown"] = -1] = "Unknown";
    MeshEntityType[MeshEntityType["Node"] = 0] = "Node";
    MeshEntityType[MeshEntityType["Element0d"] = 1] = "Element0d";
    MeshEntityType[MeshEntityType["Triangle"] = 2] = "Triangle";
    MeshEntityType[MeshEntityType["Triangle2nd"] = 3] = "Triangle2nd";
    MeshEntityType[MeshEntityType["Quadrangle"] = 4] = "Quadrangle";
    MeshEntityType[MeshEntityType["Quadrangle2nd"] = 5] = "Quadrangle2nd";
    MeshEntityType[MeshEntityType["Polygon"] = 6] = "Polygon";
    MeshEntityType[MeshEntityType["Tetrahedron"] = 7] = "Tetrahedron";
    MeshEntityType[MeshEntityType["Tetrahedron2nd"] = 8] = "Tetrahedron2nd";
    MeshEntityType[MeshEntityType["Pyramid"] = 9] = "Pyramid";
    MeshEntityType[MeshEntityType["Pyramid2nd"] = 10] = "Pyramid2nd";
    MeshEntityType[MeshEntityType["Prism"] = 11] = "Prism";
    MeshEntityType[MeshEntityType["Prism2nd"] = 12] = "Prism2nd";
    MeshEntityType[MeshEntityType["Hexahedron"] = 13] = "Hexahedron";
    MeshEntityType[MeshEntityType["Hexahedron2nd"] = 14] = "Hexahedron2nd";
    MeshEntityType[MeshEntityType["Polyhedron"] = 15] = "Polyhedron";
    MeshEntityType[MeshEntityType["LabelGroup"] = 16] = "LabelGroup";
})(MeshEntityType || (MeshEntityType = {}));
;
/**
 * MeshPropertyUsageType represents mesh property usage type, from `MeshPropertyUsageType`.
 */
export var MeshPropertyUsageType;
(function (MeshPropertyUsageType) {
    MeshPropertyUsageType[MeshPropertyUsageType["ColorMap"] = 0] = "ColorMap";
    MeshPropertyUsageType[MeshPropertyUsageType["Deformation"] = 1] = "Deformation";
    MeshPropertyUsageType[MeshPropertyUsageType["IsoSurface"] = 2] = "IsoSurface";
})(MeshPropertyUsageType || (MeshPropertyUsageType = {}));
;
;
/**
 * MeshComponentValueInfo represents information about value of mesh property component.
 */
export class MeshComponentValueInfo {
    constructor() {
        this.value = Vec3.invalidDouble();
        this.displayUnits = '';
        this.description = '';
    }
}
;
;
/**
 * MeshPropertyValuesInfo represents information about values of mesh property.
 */
export class MeshPropertyValuesInfo {
    constructor() {
        this.description = '';
        this.components = new Array();
    }
}
;
;
/**
 * IMeshPropertyInfo represents mesh property component info.
 */
export class MeshPropertyInfo {
    constructor() {
        this.propertyId = '';
        this.componentIndex = -1;
    }
}
;
;
/**
 * SelectionMeshEntity represents detected/selected info of {@link InteractiveCaeMesh} entity.
 */
export class SelectionMeshEntity {
    constructor() {
        this.entityId = -1;
        this.entityType = MeshEntityType.Unknown;
        this.isElemSurf = false;
        this.prsId = '';
        this.geomId = '';
        this.prsInfo = '';
        this.pos = Vec3.invalid();
        this.refMeshData = null;
        this.hasRefEntity = false;
        this.lineIndex = -1;
        this.propsInfo = new Map();
        this.prsPropsInfo = new Map();
    }
}
;
;
/**
 * Structure to define object's part/subpart.
 */
export class ObjectPart {
    constructor() {
        this.id = -1;
        this.subId = -1;
    }
}
;
/**
 * Structure to define detected object + its subparts.
 */
export class DetectedObjectInfo {
    constructor() {
        this.object = null;
        this.part = new ObjectPart();
        this.pickedPoint = Vec3.invalid();
        this.pickedNormal = Vec3.invalid();
    }
}
;
/**
 * Auxiliary class for accessing basic InteractiveObject aspects.
 */
export class PresentationAspects {
    /** Constructor. */
    constructor(thePrs) {
        this.myPrs = thePrs;
    }
    /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
    get transparency() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTransparency(this.myPrs.id)) !== null && _b !== void 0 ? _b : 0.0; }
    /**
     * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
     * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
     */
    get baseColor() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getBaseColor(this.myPrs.id)) !== null && _b !== void 0 ? _b : ''; }
    /**
     * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
     * Note that if object has undefined color, but defined transparency - this property will return empty string!
     */
    get baseColorRgba() {
        var _a, _b, _c, _d;
        let aColor = (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getBaseColor(this.myPrs.id)) !== null && _b !== void 0 ? _b : '';
        if (aColor === '') {
            return '';
        }
        let aTransp = (_d = (_c = this.myPrs.aspectsMgr) === null || _c === void 0 ? void 0 : _c.getTransparency(this.myPrs.id)) !== null && _d !== void 0 ? _d : 0.0;
        let anAlpha = Math.floor(255.0 * (1.0 - aTransp)).toString(16).toUpperCase();
        if (anAlpha.length === 1) {
            anAlpha = `0${anAlpha}`;
        }
        if (aColor.length === 9) {
            aColor = aColor.substring(0, 7);
        }
        return aColor + anAlpha;
    }
    /** Return object's font height. */
    get textHeight() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTextHeight(this.myPrs.id)) !== null && _b !== void 0 ? _b : 0.0; }
    /** Return object's font name. */
    get textFont() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTextFont(this.myPrs.id)) !== null && _b !== void 0 ? _b : ''; }
    /** Return object's font aspect. */
    get textFontAspect() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTextFontAspect(this.myPrs.id)) !== null && _b !== void 0 ? _b : FontAspect.Regular; }
    /** Return object's text alignment. */
    get textJustification() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTextJustification(this.myPrs.id)) !== null && _b !== void 0 ? _b : ViewCorner.LEFT_LOWER; }
    /** Return object's line thickness. */
    get lineWidth() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getLineWidth(this.myPrs.id)) !== null && _b !== void 0 ? _b : 1.0; }
    /** Return object's line type. */
    get lineType() { var _a, _b; return (_b = (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getLineType(this.myPrs.id)) !== null && _b !== void 0 ? _b : TypeOfLine.SOLID; }
    /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
    set transparency(theTransp) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTransparency(this.myPrs.id, theTransp != null ? theTransp : 0.0); }
    /** Set object's global color or empty string to unset. */
    set baseColor(theColor) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setBaseColor(this.myPrs.id, theColor != null ? theColor : ''); }
    /** Set object's global color + transparency or empty string to unset both. */
    set baseColorRgba(theColor) {
        var _a, _b;
        let aTransp = 0.0;
        if (theColor != null && theColor.length === 9) {
            let anRgba = ColorTools.convertColorStringToRgba(theColor);
            aTransp = 1.0 - (anRgba.a / 255.0);
        }
        (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setBaseColor(this.myPrs.id, theColor != null ? theColor : '');
        (_b = this.myPrs.aspectsMgr) === null || _b === void 0 ? void 0 : _b.setTransparency(this.myPrs.id, aTransp);
    }
    /** Set object's font height. */
    set textHeight(theHeight) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTextHeight(this.myPrs.id, theHeight); }
    /** Set object's font name. */
    set textFont(theFont) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTextFont(this.myPrs.id, theFont); }
    /** Set object's font aspect. */
    set textFontAspect(theAsp) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTextFontAspect(this.myPrs.id, theAsp); }
    /** Set object's text alignment. */
    set textJustification(theJust) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTextJustification(this.myPrs.id, theJust); }
    /** Set object's line width. */
    set lineWidth(theWidth) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setLineWidth(this.myPrs.id, theWidth); }
    /** Set object's line type. */
    set lineType(theType) { var _a; (_a = this.myPrs.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setLineType(this.myPrs.id, theType); }
}
;
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
    /** Empty constructor. */
    constructor(theObjId = '') {
        super();
        this._myObjId = theObjId;
        this._myObjOldId = '';
        this._myViewer = null;
        this._myAspects = null;
    }
    /** @internal Shortcut to aspects manager. */
    get aspectsMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveAspectsManager) !== null && _b !== void 0 ? _b : null; }
    /** Get object id as {@link ObjectId}. */
    get id() { return this._myObjId; }
    /** Get object id before removal. */
    get oldId() { return this._myObjOldId; }
    /** Return TRUE if object with specified Id exists. */
    get isAlive() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isAlive(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return object type. */
    get type() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getType(this._myObjId)) !== null && _b !== void 0 ? _b : ''; }
    /** Return TRUE if object with specified Id has deferred data loaded. */
    get isLoaded() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isLoaded(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return object enabled state. */
    get isEnabled() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isEnabled(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return object visibility state. */
    get isVisible() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isVisible(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return view in which object is visible or NULL if it is not displayed or displayed in multiple views. */
    get visibleInView() {
        var _a, _b;
        if (!this._myViewer || !this._myViewer.viewManager) {
            return null;
        }
        let aViewId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getVisibleInView(this._myObjId)) !== null && _b !== void 0 ? _b : -1;
        if (aViewId === -2 || aViewId === -1) {
            return null;
        }
        let aView = this._myViewer.viewManager.findSubviewObject(aViewId);
        if (aView != null) {
            return aView;
        }
        let aViewWrap = new Subview();
        aViewWrap.wrap(this._myViewer, aViewId);
        return aViewWrap;
    }
    /** Return TRUE if this object is listed within selected objects. */
    get isSelected() {
        if (this._myObjId === '' || !this._myViewer || !this._myViewer.selectionManager) {
            return false;
        }
        let aSelObjs = this._myViewer.selectionManager.selectedObjects;
        for (const aSelIter of aSelObjs) {
            if (aSelIter.id === this._myObjId) {
                return true;
            }
        }
        return false;
    }
    /** Return if this presentation is defined in 2D or 3D space. */
    get is2d() { return this.viewCorner !== ViewCorner.FREE3D; }
    /** Return view corner of 2D presentation; has no meaning for 3D objects. */
    get viewCorner() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getViewCorner(this._myObjId)) !== null && _b !== void 0 ? _b : ViewCorner.FREE3D; }
    /** Return view corner offset of 2D presentation; has no meaning for 3D objects. */
    get viewCornerOffset() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getViewCornerOffset(this._myObjId)) !== null && _b !== void 0 ? _b : { x: 0, y: 0 }; }
    /** Return local transformation. */
    get localTransformation() {
        var _a;
        const aMgr = this.objMgr;
        const aGetter = aMgr === null || aMgr === void 0 ? void 0 : aMgr.getLocalTransformation;
        if (typeof aGetter === 'function') {
            return (_a = aGetter.call(aMgr, this._myObjId)) !== null && _a !== void 0 ? _a : new Trsf();
        }
        return new Trsf();
    }
    /** Return object AABB (axes-aligned bounding box). */
    get minMax() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getMinMax(this._myObjId)) !== null && _b !== void 0 ? _b : BndBox3.voidBox(); }
    /** Return object's Z-Layer. */
    get zlayer() { var _a, _b; return (_b = (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getZLayer(this._myObjId)) !== null && _b !== void 0 ? _b : ZLayerId.UNKNOWN; }
    /** Return a value that is added to the default displaying/selection object's priority, 0 by default. */
    get priorityModifier() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.getPriorityModifier(this._myObjId)) !== null && _b !== void 0 ? _b : 0; }
    /**
     * Infinite presentation flag, FALSE by default.
     * Used to distinguish auxiliary presentations (construction elements, manipulators) from main scene objects.
     * Such objects are not considered within FitAll operation.
     */
    get isInfiniteState() { var _a, _b; return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isInfiniteState(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return aspects manager. */
    get aspects() {
        if (this._myAspects == null) {
            this._myAspects = new PresentationAspects(this);
        }
        return this._myAspects;
    }
    /** Return object's transparency within `[0.0, 1.0]` range, or `0.0` if unspecified. */
    get transparency() { var _a, _b; return (_b = (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getTransparency(this._myObjId)) !== null && _b !== void 0 ? _b : 0.0; }
    /**
     * Return object's global RGB color defined as {@link ColorString} or empty string if unspecified.
     * Note that RGB color is formatted here as RGBA for consistency with transparency property ingored!
     */
    get baseColor() { var _a, _b; return (_b = (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.getBaseColor(this._myObjId)) !== null && _b !== void 0 ? _b : ''; }
    /**
     * Return object's global RGBA color defined as {@link ColorString} or empty string if unspecified.
     * Note that if object has undefined color, but defined transparency - this property will return empty string!
     */
    get baseColorRgba() { return this.aspects.baseColorRgba; }
    /** @internal Shortcut to interactive context. */
    get objMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveContext) !== null && _b !== void 0 ? _b : null; }
    /** @internal Shortcut tii selection manager. */
    get selectionMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.selectionManager) !== null && _b !== void 0 ? _b : null; }
    /** @internal remove object from scene. */
    get doRemove() { var _a; const aRes = this.remove(); (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate(); return aRes; }
    /** @internal hide object in scene. */
    get doHide() { var _a; this.isVisible = false; (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate(); return this.isVisible; }
    /** @internal show object in scene. */
    get doShow() { var _a; this.isVisible = true; (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate(); return this.isVisible; }
    /** @internal show only this object in scene. */
    get doShowOnly() {
        var _a, _b, _c, _d;
        const is2dObj = this.is2d;
        for (let anObjIter of (_c = (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveContext) === null || _b === void 0 ? void 0 : _b.objectsInside) !== null && _c !== void 0 ? _c : []) {
            if (anObjIter.is2d === is2dObj && anObjIter.id !== this._myObjId) {
                anObjIter.isVisible = false;
            }
        }
        this.isVisible = true;
        (_d = this._myViewer) === null || _d === void 0 ? void 0 : _d.requestViewUpdate();
        return this.isVisible;
    }
    /** @internal select this object in scene. */
    get doSelect() {
        var _a;
        this.isSelected = true;
        (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate();
        return this.isSelected;
    }
    /** @internal select only this object in scene. */
    get doSelectOnly() {
        var _a;
        if (!this._myViewer || !this._myViewer.selectionManager) {
            return false;
        }
        this._myViewer.selectionManager.oneSelectedObject = this;
        (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate();
        return true;
    }
    /** Set object enabled state. */
    set isEnabled(theToEnable) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setEnabled(this._myObjId, theToEnable); }
    /** Set object visibility state. */
    set isVisible(theToShow) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setVisible(this._myObjId, theToShow); }
    /**
     * Set object to be visible only within specific view or all views (NULL) - view affinity.
     * Note that {@link isVisible} still has to be set independently.
     */
    set visibleInView(theView) {
        var _a;
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setVisibleInView(this._myObjId, theView == null ? -2 : theView.id);
    }
    /** Put/remove object from selected objects. */
    set isSelected(theToSelect) {
        if (this._myObjId === '' || !this._myViewer || !this._myViewer.selectionManager) {
            return;
        }
        let aSelObjs = this._myViewer.selectionManager.selectedObjects;
        let aNewSelObjs = [];
        for (const aSelIter of aSelObjs) {
            if (aSelIter.id === this._myObjId) {
                if (theToSelect) {
                    return;
                }
                else {
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
    set is2d(theIs2d) {
        if (!theIs2d) {
            this.viewCorner = ViewCorner.FREE3D;
        }
        else if (!this.is2d) {
            this.viewCorner = ViewCorner.LEFT_UPPER;
        }
        else { }
    }
    /** Set view corner. */
    set viewCorner(theCorner) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setViewCorner(this._myObjId, theCorner); }
    /** Set view corner offset. */
    set viewCornerOffset(theOffset) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setViewCornerOffset(this._myObjId, theOffset); }
    /** Set local transformation. */
    set localTransformation(theLoc) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setLocalTransformation(this._myObjId, theLoc); }
    /** Set object's Z-Layer. */
    set zlayer(theLayer) { var _a; (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setZLayer(this._myObjId, theLayer != null ? theLayer : ZLayerId.Default); }
    /** Set a value that is added to the default displaying/selection object's priority. */
    set priorityModifier(theModifier) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setPriorityModifier(this._myObjId, theModifier); }
    /** Set infinite presentation flag. */
    set isInfiniteState(theState) { var _a; (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.setInfiniteState(this._myObjId, theState); }
    /** Set object's global transparency within `[0.0, 1.0]` range, or `0.0` to unset. */
    set transparency(theTransp) { var _a; (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setTransparency(this._myObjId, theTransp != null ? theTransp : 0.0); }
    /** Set object's global color or empty string to unset. */
    set baseColor(theColor) { var _a; (_a = this.aspectsMgr) === null || _a === void 0 ? void 0 : _a.setBaseColor(this._myObjId, theColor != null ? theColor : ''); }
    /** Set object's global color + transparency or empty string to unset both. */
    set baseColorRgba(theColor) { this.aspects.baseColorRgba = theColor; }
    /** Remove object and invalidates it's id. */
    remove() {
        var _a, _b;
        if (this._myObjId === '') {
            return false;
        }
        const aRes = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.remove(this._myObjId)) !== null && _b !== void 0 ? _b : false;
        this._myObjOldId = this._myObjId;
        this._myObjId = '';
        return aRes;
    }
    /** @internal invalidate object id without removing from context. */
    invalidate() {
        if (this._myObjId === '') {
            return;
        }
        this._myObjOldId = this._myObjId;
        this._myObjId = '';
    }
    /**
     * Wrap existing object.
     * @param {SCwebviewer} theViewer viewer module
     * @param {string} theId the id to wrap
     * @return {boolean} FALSE if object with specified Id doesn't exist in the Viewer
     */
    wrap(theViewer, theId = '') {
        var _a, _b;
        if (theId !== '') {
            this._myObjId = theId;
        }
        this._myViewer = theViewer;
        return (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.isAlive(this._myObjId)) !== null && _b !== void 0 ? _b : false;
    }
    /** Return file path from object id. */
    getFilePathFromObjId() {
        return (this._myObjId.length > 0 && this._myObjId[0] === '/') ?
            this._myObjId :
            `/${this._myObjId}`;
    }
    /**
     * Load deferred data async.
     * @param theViewer module object
     * @return {Promise<boolean>} FALSE if model cannot be opened
     */
    loadDataAsync(theViewer) {
        return new Promise((theResolve, theReject) => {
            var _a;
            const aCallable = {
                _myObj: undefined,
                resolve: function () {
                    theResolve(true);
                    this._myObj.delete();
                },
                reject: function (theErr) {
                    theReject(new Error(theErr));
                    this._myObj.delete();
                },
            };
            const aWasmPromise = theViewer.IPromise.implement(aCallable);
            aCallable._myObj = aWasmPromise;
            (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.loadData(this._myObjId, aWasmPromise);
        });
    }
}
;
;
/**
 * Auxiliary tool to initialize JS extensions to InteractiveContext C++ class.
 * @internal
 */
export class InteractiveContextExtensions {
    static init(theViewer, theCtx) {
        let aPrsObjArray = [];
        Object.defineProperty(theCtx, 'objectsInside', {
            get: function () { return aPrsObjArray; },
        });
        theCtx.addInteractiveObject = function (theObj) {
            let aDuplicates = [];
            aPrsObjArray = aPrsObjArray.filter((theElem, theIndex, theArray) => {
                if (theElem.id === theObj.id && theElem !== theObj) {
                    aDuplicates.push(theElem);
                }
                return (theElem.id !== theObj.id);
            });
            aDuplicates.forEach((theElem) => {
                console.warn(`Removing duplicated interactive object '${theElem.id}'`);
                // @c theElem.remove(); // will remove new object
                theElem.invalidate();
            });
            aPrsObjArray.push(theObj);
        };
        theCtx.findInteractiveObject = function (theObjId) {
            let anObj = aPrsObjArray.find((theElem, theIndex, theArray) => { return (theElem.id === theObjId); });
            return anObj ? anObj : null;
        };
        theCtx.removeInteractiveObject = function (theObjId) {
            if (theCtx.findInteractiveObject(theObjId) == null) {
                return false;
            }
            aPrsObjArray = aPrsObjArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theObjId); });
            return true;
        };
        let aCppRemoveObject = theCtx.remove.bind(theCtx);
        theCtx.remove = function (theObjId) {
            theCtx.removeInteractiveObject(theObjId);
            return aCppRemoveObject(theObjId);
        };
        let aCppRemoveAllObjects = theCtx.removeAll.bind(theCtx);
        theCtx.removeAll = function () {
            let anObjsToRemove = aPrsObjArray.slice();
            anObjsToRemove.reverse();
            aPrsObjArray = [];
            anObjsToRemove.forEach((theObj) => {
                theObj.remove(); // invalidate objects
            });
            return aCppRemoveAllObjects() || anObjsToRemove.length !== 0;
        };
        theCtx.getSceneGraph = function (theIdPrefix = '') {
            const aJson = theCtx.getSceneGraphJson(theIdPrefix);
            return SceneGraph.parseJson(aJson);
        };
        const filterObjectIds = function (theObjIds, theAction) {
            if (!Array.isArray(theObjIds) || theObjIds.length === 0) {
                return [];
            }
            const missing = [];
            const existing = [];
            for (const anId of theObjIds) {
                if (!anId) {
                    continue;
                }
                const anObj = theCtx.findInteractiveObject(anId);
                if (anObj == null) {
                    missing.push(anId);
                }
                else {
                    existing.push(anId);
                }
            }
            if (missing.length > 0) {
                console.warn(`InteractiveContext.${theAction}: skip missing objects: ${missing.join(', ')}`);
            }
            return existing;
        };
        theCtx.setVisibleManyObjects = function (theObjIds, theToShow) {
            const ids = filterObjectIds(theObjIds, 'setVisibleManyObjects');
            if (ids.length === 0) {
                return;
            }
            theCtx.setVisibleMany(ids.join('\n'), theToShow);
        };
        theCtx.setVisibleOnlyObjects = function (theObjIds) {
            const ids = filterObjectIds(theObjIds, 'setVisibleOnlyObjects');
            if (ids.length === 0) {
                return;
            }
            theCtx.setVisibleOnly(ids.join('\n'));
        };
        theCtx.setTransparencyManyObjects = function (theObjIds, theTransparency) {
            const aMgr = theViewer.interactiveAspectsManager;
            if (!aMgr) {
                return;
            }
            const ids = filterObjectIds(theObjIds, 'setTransparencyManyObjects');
            if (ids.length === 0) {
                return;
            }
            const aClamped = Math.min(1.0, Math.max(0.0, theTransparency));
            for (const anId of ids) {
                aMgr.setTransparency(anId, aClamped);
            }
        };
    }
}
;
;
;
/**
 * Auxiliary tool to initialize JS extensions to SelectionManager C++ class.
 * @internal
 */
export class SelectionManagerExtensions {
    static init(theViewer, theSelMgr) {
        Object.defineProperty(theSelMgr, 'selectedObjects', {
            get: function () {
                var _a;
                let anObjArray = [];
                if (!theViewer.interactiveContext) {
                    return anObjArray;
                }
                const aSelObjIds = theSelMgr.getSelectedObjects();
                const aSelObjIdsArr = aSelObjIds.length !== 0 ? aSelObjIds.split('\n') : [];
                for (const aSelIdIter of aSelObjIdsArr) {
                    const aPrsObj = (_a = theViewer.interactiveContext.findInteractiveObject(aSelIdIter)) !== null && _a !== void 0 ? _a : null;
                    // @c const aPrsObj = aPrsWrapper (aSelIdIter);
                    if (aPrsObj != null) {
                        anObjArray.push(aPrsObj);
                    }
                }
                return anObjArray;
            },
            set: function (theObjs) {
                if (!theViewer.interactiveContext) {
                    return;
                }
                let anObjIds = [];
                for (const anObj of theObjs) {
                    if (anObj != null) {
                        anObjIds.push(anObj.id);
                    }
                }
                theSelMgr.setSelectedObjects(anObjIds.join('\n'), false);
            },
        });
        Object.defineProperty(theSelMgr, 'oneSelectedObject', {
            get: function () {
                var _a, _b;
                const aSelObjIds = theSelMgr.getSelectedObjects();
                const aSelObjIdsArr = aSelObjIds.length !== 0 ? aSelObjIds.split('\n') : [];
                if (aSelObjIdsArr.length !== 1) {
                    return null;
                }
                return (_b = (_a = theViewer.interactiveContext) === null || _a === void 0 ? void 0 : _a.findInteractiveObject(aSelObjIdsArr[0])) !== null && _b !== void 0 ? _b : null;
            },
            set: function (theObj) {
                theSelMgr === null || theSelMgr === void 0 ? void 0 : theSelMgr.setSelectedObjects(theObj != null ? theObj.id : '', false);
            },
        });
        Object.defineProperty(theSelMgr, 'lastDetectedObject', {
            get: function () {
                var _a, _b;
                const aDetObj = theSelMgr.getLastDetectedObject();
                return (_b = (_a = theViewer.interactiveContext) === null || _a === void 0 ? void 0 : _a.findInteractiveObject(aDetObj.objectId)) !== null && _b !== void 0 ? _b : null;
            },
        });
        Object.defineProperty(theSelMgr, 'lastDetectedInfo', {
            get: function () {
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
}
;
//# sourceMappingURL=InteractiveObject.js.map