import { CameraOrientation } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
/**
 * Represents ViewCube property manager.
 * ViewCube is an auxiliary interactive object displayed at View corner and showing current View orientation.
 * See also {@link SCwebviewer.viewCube} property.
 */
export class InteractiveViewCube extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::InteractiveViewCube';
            }
            (_a = this.viewCubeMgr) === null || _a === void 0 ? void 0 : _a.createViewCube(this._myObjId);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Return view cube base size. */
    get baseSize() { var _a, _b; return (_b = (_a = this.viewCubeMgr) === null || _a === void 0 ? void 0 : _a.getViewCubeBaseSize(this._myObjId)) !== null && _b !== void 0 ? _b : 0.0; }
    /** Return view cube side label. */
    get boxSideLabelFront() { return this.getBoxSideLabel(CameraOrientation.FRONT); }
    /** Return view cube side label. */
    get boxSideLabelBack() { return this.getBoxSideLabel(CameraOrientation.BACK); }
    /** Return view cube side label. */
    get boxSideLabelLeft() { return this.getBoxSideLabel(CameraOrientation.LEFT); }
    /** Return view cube side label. */
    get boxSideLabelRight() { return this.getBoxSideLabel(CameraOrientation.RIGHT); }
    /** Return view cube side label. */
    get boxSideLabelTop() { return this.getBoxSideLabel(CameraOrientation.TOP); }
    /** Return view cube side label. */
    get boxSideLabelBottom() { return this.getBoxSideLabel(CameraOrientation.BOTTOM); }
    /** @internal Shortcut to view cube context. */
    get viewCubeMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveViewCubeManager) !== null && _b !== void 0 ? _b : null; }
    /** Set view cube base size. */
    set baseSize(theSize) { var _a; (_a = this.viewCubeMgr) === null || _a === void 0 ? void 0 : _a.setViewCubeBaseSize(this._myObjId, theSize); }
    /** Set view cube side label. */
    set boxSideLabelFront(theLabel) { this.setBoxSideLabel(CameraOrientation.FRONT, theLabel); }
    /** Set view cube side label. */
    set boxSideLabelBack(theLabel) { this.setBoxSideLabel(CameraOrientation.BACK, theLabel); }
    /** Set view cube side label. */
    set boxSideLabelLeft(theLabel) { this.setBoxSideLabel(CameraOrientation.LEFT, theLabel); }
    /** Set view cube side label. */
    set boxSideLabelRight(theLabel) { this.setBoxSideLabel(CameraOrientation.RIGHT, theLabel); }
    /** Set view cube side label. */
    set boxSideLabelTop(theLabel) { this.setBoxSideLabel(CameraOrientation.TOP, theLabel); }
    /** Set view cube side label. */
    set boxSideLabelBottom(theLabel) { this.setBoxSideLabel(CameraOrientation.BOTTOM, theLabel); }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveViewCube(null);
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
    /** Return view cube side label. */
    getBoxSideLabel(theSide) { var _a, _b; return (_b = (_a = this.viewCubeMgr) === null || _a === void 0 ? void 0 : _a.getBoxSideLabel(this._myObjId, theSide)) !== null && _b !== void 0 ? _b : ''; }
    /** Set view cube side label. */
    setBoxSideLabel(theSide, theLabel) { var _a; (_a = this.viewCubeMgr) === null || _a === void 0 ? void 0 : _a.setBoxSideLabel(this._myObjId, theSide, theLabel); }
}
;
;
//# sourceMappingURL=InteractiveViewCube.js.map