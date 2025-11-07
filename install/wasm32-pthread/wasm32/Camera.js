// Created by: Kirill Gavrilov
// Copyright (c) 2022 Huawei Technologies Co., Ltd.
import { BndBox3 } from './CommonTools.js';
/**
 * Active left mouse button gesture (C++ `SCwebviewer::MouseGesture`).
 */
export var MouseGesture;
(function (MouseGesture) {
    MouseGesture[MouseGesture["NONE"] = 0] = "NONE";
    MouseGesture[MouseGesture["ROTATE"] = 1] = "ROTATE";
    MouseGesture[MouseGesture["ROTATEVIEW"] = 2] = "ROTATEVIEW";
    MouseGesture[MouseGesture["PAN"] = 3] = "PAN";
    MouseGesture[MouseGesture["ZOOM"] = 4] = "ZOOM";
})(MouseGesture || (MouseGesture = {}));
;
/**
 * Stereoscopic output mode (C++ `SCwebviewer::StereoMode`).
 */
export var StereoMode;
(function (StereoMode) {
    StereoMode[StereoMode["MONO"] = 0] = "MONO";
    StereoMode[StereoMode["REDCYAN"] = 1] = "REDCYAN";
    StereoMode[StereoMode["ROWINTERLACED"] = 2] = "ROWINTERLACED";
    StereoMode[StereoMode["ROWINTERLACEDINV"] = 3] = "ROWINTERLACEDINV";
})(StereoMode || (StereoMode = {}));
;
/**
 * Represents Camera manager.
 * See also {@link SCwebviewer.camera} property.
 */
export class Camera {
    /** Main constructor. */
    constructor(theViewer, theViewId = -1) {
        if (!theViewer.viewManager) {
            throw new Error('Invalid viewer');
        }
        this._myViewId = theViewId;
        this._myViewMgr = theViewer.viewManager;
    }
    /** Return AABB of all visible objects in the view. */
    get sceneMinMaxVisible() { return this._myViewMgr.getSceneMinMax(this._myViewId, false); }
    /** Return AABB of selected objects in the view. */
    get sceneMinMaxSelection() { return this._myViewMgr.getSceneMinMax(this._myViewId, true); }
    /** Orbit acceleration coefficient. */
    get orbitAcceleration() { return this._myViewMgr.getOrbitAcceleration(); }
    /** Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit. */
    get maxCamDistanceMultiplier() { return this._myViewMgr.getMaxCamDistanceMultiplier(); }
    /** Return current camera view point parameters. */
    get viewPoint() { return this._myViewMgr.getViewPoint(this._myViewId); }
    /** Allow/disallow camera rotation. */
    get toAllowRotation() { return this._myViewMgr.toAllowRotation(this._myViewId); }
    /** Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up. */
    get toLockOrbitZUp() { return this._myViewMgr.toLockOrbitZUp(this._myViewId); }
    /** Allow/disallow camera panning. */
    get toAllowPanning() { return this._myViewMgr.toAllowPanning(this._myViewId); }
    /** Allow/disallow camera zooming. */
    get toAllowZooming() { return this._myViewMgr.toAllowZooming(this._myViewId); }
    /** Return left mouse button gesture; ROTATE (orbital) by default. */
    get leftMouseGesture() { return this._myViewMgr.getMouseGesture(this._myViewId, 0); }
    /** Return middle mouse button gesture; PAN by default. */
    get middleMouseGesture() { return this._myViewMgr.getMouseGesture(this._myViewId, 1); }
    /** Return right mouse button gesture; ROTATEVIEW by default. */
    get rightMouseGesture() { return this._myViewMgr.getMouseGesture(this._myViewId, 2); }
    /** Switch perspective/orthographic projection. */
    get isPerspectiveProjection() { return this._myViewMgr.isPerspectiveProjection(this._myViewId); }
    /** Perspective projection FOVy in degrees. */
    get perspectiveProjectionFOVy() { return this._myViewMgr.getPerspectiveProjectionFOVy(this._myViewId); }
    /** Stereoscopic mode; MONO by default. */
    get stereoMode() { return this._myViewMgr.getStereoMode(this._myViewId); }
    /** Orbit acceleration coefficient. */
    set orbitAcceleration(theAccel) { this._myViewMgr.setOrbitAcceleration(theAccel); }
    /** Set max camera distance multiplier. */
    set maxCamDistanceMultiplier(theMax) { this._myViewMgr.setMaxCamDistanceMultiplier(theMax); }
    /** Set camera view point parameters. */
    set viewPoint(theParams) { this._myViewMgr.setViewPoint(this._myViewId, theParams); }
    /** Allow/disallow camera rotation. */
    set toAllowRotation(theToAllow) { this._myViewMgr.setAllowRotation(this._myViewId, theToAllow); }
    /** Get/set if camera up orientation should be forced Z up. */
    set toLockOrbitZUp(theToLock) { this._myViewMgr.setLockOrbitZUp(this._myViewId, theToLock); }
    /** Allow/disallow camera panning. */
    set toAllowPanning(theToAllow) { this._myViewMgr.setAllowPanning(this._myViewId, theToAllow); }
    /** Allow/disallow camera zooming. */
    set toAllowZooming(theToAllow) { this._myViewMgr.setAllowZooming(this._myViewId, theToAllow); }
    /** Set left mouse button gesture. */
    set leftMouseGesture(theGest) { this._myViewMgr.setMouseGesture(this._myViewId, 0, theGest); }
    /** Set middle mouse button gesture. */
    set middleMouseGesture(theGest) { this._myViewMgr.setMouseGesture(this._myViewId, 1, theGest); }
    /** Set right mouse button gesture. */
    set rightMouseGesture(theGest) { this._myViewMgr.setMouseGesture(this._myViewId, 2, theGest); }
    /** Switch perspective/orthographic projection. */
    set isPerspectiveProjection(theIsPersp) { this._myViewMgr.setPerspectiveProjection(this._myViewId, theIsPersp); }
    /** Perspective projection FOVy in degrees. */
    set perspectiveProjectionFOVy(theFovy) { this._myViewMgr.setPerspectiveProjectionFOVy(this._myViewId, theFovy); }
    /** Stereoscopic mode. */
    set stereoMode(theMode) { this._myViewMgr.setStereoMode(this._myViewId, theMode); }
    /** Return AABB of all or selected objects in the view. */
    getSceneMinMax(theSelected) { return this._myViewMgr.getSceneMinMax(this._myViewId, theSelected); }
    /** Fit selected or fit all visible objects into view. */
    fitAllAuto(theIsAuto, theToAnimate = false) {
        // @c this._myViewMgr.fitAllAuto(this._myViewId, theIsAuto);
        const aBoxAll = this.sceneMinMaxVisible;
        if (!theIsAuto) {
            this.fitMinMax(aBoxAll, theToAnimate);
            return;
        }
        const aBoxSel = this.sceneMinMaxSelection;
        const aBoxFit = (BndBox3.isVoid(aBoxSel) || this.isFitMinMax(aBoxSel)) ? aBoxAll : aBoxSel;
        this.fitMinMax(aBoxFit, theToAnimate);
    }
    /** Return TRUE if box tightly fits into the view. */
    isFitMinMax(theMinMax) { return this._myViewMgr.isFitMinMax(this._myViewId, theMinMax); }
    /** Fit box into view. */
    fitMinMax(theMinMax, theToAnimate) {
        this._myViewMgr.fitMinMax(this._myViewId, theMinMax, theToAnimate);
    }
    /**
     * Set camera view orientation.
     * @param theOrientation viewer orientation
     */
    setOrientation(theOrientation, theToAnimate = false) {
        this._myViewMgr.setOrientation(this._myViewId, theOrientation, theToAnimate);
    }
}
;
//# sourceMappingURL=Camera.js.map