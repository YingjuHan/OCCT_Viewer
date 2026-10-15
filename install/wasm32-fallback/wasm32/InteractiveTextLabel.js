import { Vec3 } from './CommonTools.js';
import { InteractiveObject, TypeOfMarker } from './InteractiveObject.js';
;
/**
 * Represents Text Label interactive object.
 */
export class InteractiveTextLabel extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::InteractiveTextLabel';
            }
            (_a = this.textLabelMgr) === null || _a === void 0 ? void 0 : _a.createTextLabel(this._myObjId);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Text to be displayed. */
    get text() { return this.params.text; }
    /** Icon to be displayed. */
    get icon() { return this.params.icon; }
    /** 3D point to display label at. */
    get position() { return this.params.position; }
    /** Return TRUE if 3D attachment point is defined. */
    get hasAttachPoint() { return !Vec3.isInfinite(this.params.attachPoint); }
    /** 3D point to attach label. */
    get attachPoint() { return this.params.attachPoint; }
    /** 2d offset from 3D point to text label. */
    get pointPadding() { return this.params.pointPadding; }
    /** 2d padding for rectangle around text label. */
    get rectanglePadding() { return this.params.rectanglePadding; }
    /** Flag to draw rectangle around text. */
    get toDrawRectangle() { return this.params.toDrawRectangle; }
    /** Flag to draw 3D attach point as a marker. */
    get attachMarker() { return this.params.attachMarker; }
    /** 3D attach point marker scale. */
    get attachMarkerScale() { return this.params.attachMarkerScale; }
    /** @internal Return base parameters. */
    get params() {
        if (!this.textLabelMgr) {
            return { text: '', icon: '',
                position: { x: 0, y: 0, z: 0 }, attachPoint: Vec3.invalid(),
                pointPadding: 0, rectanglePadding: 0, toDrawRectangle: false,
                attachMarker: TypeOfMarker.EMPTY, attachMarkerScale: 1 };
        }
        return this.textLabelMgr.getLabelParams(this._myObjId);
    }
    /** @internal Shortcut to text label context. */
    get textLabelMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveTextLabelManager) !== null && _b !== void 0 ? _b : null; }
    /** Text to be displayed. */
    set text(theText) {
        let aParams = this.params;
        aParams.text = theText;
        this.params = aParams;
    }
    /** Icon to be displayed. */
    set icon(theIcon) {
        let aParams = this.params;
        aParams.icon = theIcon;
        this.params = aParams;
    }
    /** 3D point to display label at. */
    set position(thePnt) {
        let aParams = this.params;
        aParams.position = thePnt;
        this.params = aParams;
    }
    /** Set if 3D attachment point is defined or not. */
    set hasAttachPoint(theToDefine) {
        let aParams = this.params;
        if (Vec3.isInfinite(this.params.attachPoint) === theToDefine) {
            return;
        }
        aParams.attachPoint = theToDefine ? aParams.position : Vec3.invalid();
        this.params = aParams;
    }
    /** 3D point to attach label. */
    set attachPoint(thePnt) {
        let aParams = this.params;
        aParams.attachPoint = thePnt;
        this.params = aParams;
    }
    /** 2d offset from 3D point to text label. */
    set pointPadding(thePad) {
        let aParams = this.params;
        aParams.pointPadding = thePad;
        this.params = aParams;
    }
    /** 2d padding for rectangle around text label. */
    set rectanglePadding(thePad) {
        let aParams = this.params;
        aParams.rectanglePadding = thePad;
        this.params = aParams;
    }
    /** Flag to draw rectangle around text. */
    set toDrawRectangle(theToDraw) {
        let aParams = this.params;
        aParams.toDrawRectangle = theToDraw;
        this.params = aParams;
    }
    /** Flag to draw 3D attach point as a marker. */
    set attachMarker(theMarker) {
        let aParams = this.params;
        aParams.attachMarker = theMarker;
        this.params = aParams;
    }
    /** 3D attach point marker scale. */
    set attachMarkerScale(theScale) {
        let aParams = this.params;
        aParams.attachMarkerScale = theScale;
        this.params = aParams;
    }
    /** @internal Set base parameters. */
    set params(theParams) { var _a; (_a = this.textLabelMgr) === null || _a === void 0 ? void 0 : _a.setLabelParams(this._myObjId, theParams); }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        var _a;
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveTextLabel(null);
        aCopy.wrap(this._myViewer, anObjId);
        (_a = this.objMgr) === null || _a === void 0 ? void 0 : _a.addInteractiveObject(aCopy);
        return aCopy;
    }
}
;
;
//# sourceMappingURL=InteractiveTextLabel.js.map