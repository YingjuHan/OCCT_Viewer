import { InteractiveObject } from './InteractiveObject.js';
;
/**
 * Parameters object for an angular arrow of the coordinate system.
 */
export class AngularAxesParams {
    constructor() {
        /** Angular axis ID is empty by default. */
        this.axisId = '';
        /** Orientation of the angular arrow defined as rotation axis with a sign, e.g. +z or -x. */
        this.orientation = '';
        /** The attachment axis (zero axis) of the angular arrow. */
        this.zeroAxis = '';
        /** The text label of the angular arrow. */
        this.label = '';
    }
}
;
/**
 * Represents spherical coordinate axes interactive object.
 */
export class InteractiveSphericalAxes extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::InteractiveSphericalAxes';
            }
            (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.createSphericalAxes(this._myObjId);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Returns axes position. */
    get position() { var _a, _b; return (_b = (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.getAxesPosition(this._myObjId)) !== null && _b !== void 0 ? _b : { x: 0.0, y: 0.0, z: 0.0 }; }
    /** Returns axes scale. */
    get scale() { var _a, _b; return (_b = (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.getAxesScale(this._myObjId)) !== null && _b !== void 0 ? _b : 1.0; }
    /** Returns the orientation of longitude/latitude circles. */
    get circlesOrientation() { var _a, _b; return (_b = (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.getOrientation(this._myObjId)) !== null && _b !== void 0 ? _b : 'oz'; }
    /** Returns the parameters of two angular axes. */
    get angularAxesParams() {
        let aParams = new Array();
        if (this.axesMgr) {
            let anAxesIds = ['angular1', 'angular2'];
            for (const anAxisId of anAxesIds) {
                let anAxisParams = new AngularAxesParams();
                anAxisParams.axisId = anAxisId;
                anAxisParams.orientation = this.axesMgr.getAngularArrowDir(this._myObjId, anAxisId);
                anAxisParams.zeroAxis = this.axesMgr.getAngularArrowAxis(this._myObjId, anAxisId);
                anAxisParams.label = this.axesMgr.getAngularArrowLabel(this._myObjId, anAxisId);
                aParams.push(anAxisParams);
            }
        }
        return aParams;
    }
    /** Sets axes position. */
    set position(thePos) { var _a; (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.setAxesPosition(this._myObjId, thePos); }
    /** Sets axes scale. */
    set scale(theScale) { var _a; (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.setAxesScale(this._myObjId, theScale); }
    /** Sets the orientation of longitude/latitude circles. */
    set circlesOrientation(theAxis) { var _a; (_a = this.axesMgr) === null || _a === void 0 ? void 0 : _a.setOrientation(this._myObjId, theAxis); }
    /** Sets the parameters of two angular axes. */
    set angularAxesParams(theAxisParams) {
        if (this.axesMgr) {
            for (const anAxisParams of theAxisParams) {
                this.axesMgr.setAngularArrowDir(this._myObjId, anAxisParams.axisId, anAxisParams.orientation);
                this.axesMgr.setAngularArrowAxis(this._myObjId, anAxisParams.axisId, anAxisParams.zeroAxis);
                this.axesMgr.setAngularArrowLabel(this._myObjId, anAxisParams.axisId, anAxisParams.label);
            }
        }
    }
    /** @internal Shortcut to C++ axes objects manager. */
    get axesMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveAxesManager) !== null && _b !== void 0 ? _b : null; }
    /** Make a copy of existing object with new id. */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveSphericalAxes(null);
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
}
;
//# sourceMappingURL=InteractiveSphericalAxes.js.map