import { Vec3, BndBox3 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
/**
 * Represents interactive clipping box object.
 * Clipping planes will be assigned to Subview object passed within constructor.
 * Note that:
 * - Hiding this interative object (isVisible) doesn't deactivates clipping;
 * - Object in disabled state (isEnabled) deactivates dragging capabilities, not clipping;
 * - Remove object from (remove) or disable clipping (isClippingOn) to deactivate clipping.
 *
 * Basic definition of clipping box configuration consists of:
 * - clippingCorner, Origin point, or box corner point, defined in 3D space or as a ratio to scene box.
 * - clippingExtent, box extent within each dimension, where:
 *   * zero extent disables clipping within specific dimension;
 *   * finite extent defines finite clipping range from corner point (origin),
 *     e.g. a pair of clipping planes with opposite directions clipping box interior;
 *   * infinite extent defines half-space clipping, e.g. a single clipping plane;
 *   * negative extent specifies clipping side relative to corner point.
 *
 * By default, clipping box clips interior of the box (within dimensions with finite extent),
 * which could be switched by toFlipClipping property to clip box exterior instead.
 *
 * For convenience, corner point (origin) and extent could be defined as a ratio to scene box;
 * but this is a transient interface (application should track scene box changes on its own).
 *
 * Application may use clippingExtent to define which clipping planes should be enabled, or alternativelly
 * - use nbPlanesXYZ interface that defines number of active clipping planes within each dimension.
 */
export class InteractiveClipping extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer, theView, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::InteractiveClipping';
            }
            (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.createClipping(this._myObjId, theView == null ? -2 : theView.id);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Return clipping flag; FALSE by default. */
    get isClippingOn() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.isClippingOn(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return capping flag; FALSE by default. */
    get isCappingOn() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.isCappingOn(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return TRUE for using object's color for capping. */
    get toUseObjectMaterial() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.toUseObjectMaterial(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return shaded presentation flag; TRUE by default. */
    get isShaded() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.isShaded(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return clipping box flipping flag; FALSE by default. */
    get toFlipClipping() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.toFlipClipping(this._myObjId)) !== null && _b !== void 0 ? _b : false; }
    /** Return clipping box configuration - main corner point (origin). */
    get clippingCorner() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingCorner(this._myObjId, false)) !== null && _b !== void 0 ? _b : Vec3.zero(); }
    /** Return clipping box configuration - main corner point (origin), relative to scene box. */
    get relativeClippingCorner() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingCorner(this._myObjId, true)) !== null && _b !== void 0 ? _b : Vec3.zero(); }
    /**
     * Return clipping box configuration - box extent.
     * - zero extent disables clipping within specific dimension;
     * - finite extent defines finite clipping range from corner point (origin),
     *   e.g. a pair of clipping planes with opposite directions clipping box interior;
     * - infinite extent defines half-space clipping, e.g. a single clipping plane;
     * - negative extent specifies clipping side relative to corner point.
     */
    get clippingExtent() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingExtent(this._myObjId, false)) !== null && _b !== void 0 ? _b : Vec3.zero(); }
    /** Return clipping box configuration - box extent, relative to scene box extent. */
    get relativeClippingExtent() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingExtent(this._myObjId, true)) !== null && _b !== void 0 ? _b : Vec3.zero(); }
    /**
     * Return clipping box configuration - number of clipping planes per dimension (0, -1, 1, or -2, 2),
     * where negative number means negative extent
     * (e.g. -1 means clipping in opposite direction,
     * -2 means building a box in opposite direction from main corner point).
     */
    get nbPlanesXYZ() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getNbPlanesXYZ(this._myObjId)) !== null && _b !== void 0 ? _b : Vec3.zero(); }
    /** Return clipping box configuration. */
    get clippingBox() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingBox(this._myObjId, false)) !== null && _b !== void 0 ? _b : BndBox3.voidBox(); }
    /** Return clipping box configuration, relative to scene box. */
    get relativeClippingBox() { var _a, _b; return (_b = (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.getClippingBox(this._myObjId, true)) !== null && _b !== void 0 ? _b : BndBox3.voidBox(); }
    /** @internal Shortcut to clipping manager. */
    get clipMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveClippingManager) !== null && _b !== void 0 ? _b : null; }
    /** Set clipping flag. */
    set isClippingOn(theToCap) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingOn(this._myObjId, theToCap); }
    /** Set capping flag. */
    set isCappingOn(theToCap) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setCappingOn(this._myObjId, theToCap); }
    /** Set flag for using object's color for capping. */
    set toUseObjectMaterial(theToUse) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setUseObjectMaterial(this._myObjId, theToUse); }
    /** Set shaded presentation flag. */
    set isShaded(theIsShaded) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setShaded(this._myObjId, theIsShaded); }
    /** Set clipping box flipping flag. */
    set toFlipClipping(theToFlip) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setFlipClipping(this._myObjId, theToFlip); }
    /** Set clipping box configuration - main corner point (origin). */
    set clippingCorner(thePnt) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingCorner(this._myObjId, thePnt, false); }
    /** Set clipping box configuration - main corner point (origin), relative to scene box. */
    set relativeClippingCorner(thePnt) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingCorner(this._myObjId, thePnt, true); }
    /** Set clipping box configuration - box extent. */
    set clippingExtent(theExt) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingExtent(this._myObjId, theExt, false); }
    /** Set clipping box configuration - box extent, relative to scene box extent. */
    set relativeClippingExtent(theExt) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingExtent(this._myObjId, theExt, true); }
    /** Set clipping box configuration - number of clipping planes per dimension. */
    set nbPlanesXYZ(theVec) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setNbPlanesXYZ(this._myObjId, theVec); }
    /** Set clipping box configuration. */
    set clippingBox(theBox) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingBox(this._myObjId, theBox, false); }
    /** Set clipping box configuration, relative to scene box. */
    set relativeClippingBox(theBox) { var _a; (_a = this.clipMgr) === null || _a === void 0 ? void 0 : _a.setClippingBox(this._myObjId, theBox, true); }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveClipping(null, null);
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
}
;
;
//# sourceMappingURL=InteractiveClipping.js.map