import { Vec3, Ax2 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
;
/**
 * Describes dimension parameters object.
 */
export class DimensionParams {
    constructor() {
        /** Plane position. */
        this.plane = Ax2.invalid();
        /** Real computed value. */
        this.realValue = 0.0;
        /** Custom text value overriding real value, when non-empty. */
        this.textValue = '';
        /** Special symbol (single character!) */
        this.specialSymbol = '';
        /** Special symbol position. */
        this.isSpecialSymbolAfter = false;
    }
}
;
/**
 * Represents interactive dimension object.
 */
export class InteractiveDimension extends InteractiveObject {
    /** Empty constructor. */
    constructor(theObjId = '') {
        super(theObjId);
    }
    /** Return dimension plane. */
    get plane() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionParams(this._myObjId).plane) !== null && _b !== void 0 ? _b : new Ax2(); }
    /** Return computed real value. */
    get realValue() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionParams(this._myObjId).realValue) !== null && _b !== void 0 ? _b : 0.0; }
    /** Return custom text value, or empty string. */
    get textValue() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionParams(this._myObjId).textValue) !== null && _b !== void 0 ? _b : ''; }
    /** Return special symbol. */
    get specialSymbol() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionParams(this._myObjId).specialSymbol) !== null && _b !== void 0 ? _b : ' '; }
    /** Return special symbol position. */
    get isSpecialSymbolAfter() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionParams(this._myObjId).isSpecialSymbolAfter) !== null && _b !== void 0 ? _b : false; }
    /** Return flag if custom text position is manually set or automatically computed. */
    get isCustomTextPosition() { var _a, _b; return !Vec3.isInfinite((_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionTextPosition(this._myObjId, true)) !== null && _b !== void 0 ? _b : Vec3.invalid()); }
    /** Return text position. */
    get textPosition() { var _a, _b; return (_b = (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.getDimensionTextPosition(this._myObjId, false)) !== null && _b !== void 0 ? _b : Vec3.invalid(); }
    /** @internal Shortcut to text label context. */
    get dimMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveDimensionManager) !== null && _b !== void 0 ? _b : null; }
    /** Set custom text value, or unset it by empty string. */
    set textValue(theText) {
        if (!this.dimMgr) {
            return;
        }
        let aParams = this.dimMgr.getDimensionParams(this._myObjId);
        aParams.textValue = theText;
        this.dimMgr.setDimensionParams(this._myObjId, aParams);
    }
    /** Set special symbol. */
    set specialSymbol(theSymb) {
        if (theSymb.length > 1) {
            throw Error('Invalid special symbol');
        }
        if (!this.dimMgr) {
            return;
        }
        let aParams = this.dimMgr.getDimensionParams(this._myObjId);
        aParams.specialSymbol = theSymb;
        this.dimMgr.setDimensionParams(this._myObjId, aParams);
    }
    /** Return special symbol position. */
    set isSpecialSymbolAfter(theIsAfter) {
        if (!this.dimMgr) {
            return;
        }
        let aParams = this.dimMgr.getDimensionParams(this._myObjId);
        aParams.isSpecialSymbolAfter = theIsAfter;
        this.dimMgr.setDimensionParams(this._myObjId, aParams);
    }
    /** Set flag if custom text position is manually set or automatically computed. */
    set isCustomTextPosition(theIsCustom) { this.textPosition = theIsCustom ? this.textPosition : null; }
    /** Set fixed text position. WARNING! Result might be adjusted. */
    set textPosition(thePnt) { var _a; (_a = this.dimMgr) === null || _a === void 0 ? void 0 : _a.setDimensionTextPosition(this._myObjId, thePnt ? thePnt : Vec3.invalid()); }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveDimension();
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
    /**
     * Build length dimension between two given points.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 first point
     * @param {IVec3} thePnt2 second point
     * @param {IAx2}  thePln  plane
     */
    buildLength(theViewer, thePnt1, thePnt2, thePln = Ax2.invalid()) {
        var _a, _b, _c, _d, _e;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('length')) !== null && _b !== void 0 ? _b : 'length';
        }
        const aRes = (_d = (_c = this.dimMgr) === null || _c === void 0 ? void 0 : _c.buildLengthDimension(this._myObjId, thePnt1, thePnt2, thePln)) !== null && _d !== void 0 ? _d : false;
        (_e = this.objMgr) === null || _e === void 0 ? void 0 : _e.addInteractiveObject(this);
        return aRes;
    }
    /**
     * Build radius dimension.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} theAx2    circle location
     * @param {IVec3} theRadius radius
     * @param {IAx2}  theAnchor optional anchor point on the circle
     */
    buildRadius(theViewer, theAx2, theRadius, theAnchor = Vec3.invalid()) {
        var _a, _b, _c, _d, _e;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('radius')) !== null && _b !== void 0 ? _b : 'radius';
        }
        const aRes = (_d = (_c = this.dimMgr) === null || _c === void 0 ? void 0 : _c.buildRadiusDimension(this._myObjId, theAx2, theRadius, theAnchor)) !== null && _d !== void 0 ? _d : false;
        (_e = this.objMgr) === null || _e === void 0 ? void 0 : _e.addInteractiveObject(this);
        return aRes;
    }
    /**
     * Build angle dimension.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 first point
     * @param {IVec3} thePnt2 second point (corner)
     * @param {IVec3} thePnt3 third point
     */
    buildAngle(theViewer, thePnt1, thePnt2, thePnt3) {
        var _a, _b, _c, _d, _e;
        this._myViewer = theViewer;
        if (this._myObjId === '') {
            this._myObjId = (_b = (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.generateId('angle')) !== null && _b !== void 0 ? _b : 'angle';
        }
        const aRes = (_d = (_c = this.dimMgr) === null || _c === void 0 ? void 0 : _c.buildAngleDimension(this._myObjId, thePnt1, thePnt2, thePnt3)) !== null && _d !== void 0 ? _d : false;
        (_e = this.objMgr) === null || _e === void 0 ? void 0 : _e.addInteractiveObject(this);
        return aRes;
    }
}
;
;
//# sourceMappingURL=InteractiveDimension.js.map