import { InteractiveObject } from './InteractiveObject.js';
/**
 * Represents Scale Ruler property manager.
 * Scale Ruler is an auxiliary interactive object displayed length measure units at view corner.
 * See also {@link SCwebviewer.scaleRuler} property.
 */
export class InteractiveScaleRuler extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::InteractiveScaleRuler';
            }
            (_a = this.scaleRulerMgr) === null || _a === void 0 ? void 0 : _a.createScaleRuler(this._myObjId);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Return scale ruler base size. */
    get baseSize() { var _a, _b; return (_b = (_a = this.scaleRulerMgr) === null || _a === void 0 ? void 0 : _a.getBaseSize(this._myObjId)) !== null && _b !== void 0 ? _b : { x: 0.0, y: 0.0 }; }
    /** @internal Shortcut to scale ruler context. */
    get scaleRulerMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveScaleRulerManager) !== null && _b !== void 0 ? _b : null; }
    /** Set scale ruler base size. */
    set baseSize(theSize) { var _a; (_a = this.scaleRulerMgr) === null || _a === void 0 ? void 0 : _a.setBaseSize(this._myObjId, theSize); }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveScaleRuler(null);
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
}
;
;
//# sourceMappingURL=InteractiveScaleRuler.js.map