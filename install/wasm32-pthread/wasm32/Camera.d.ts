import type { CameraOrientation, ICameraViewPoint, IBndBox3, ViewId } from './CommonTools.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Active left mouse button gesture (C++ `SCwebviewer::MouseGesture`).
 */
export declare enum MouseGesture {
    NONE = 0,
    ROTATE = 1,
    ROTATEVIEW = 2,
    PAN = 3,
    ZOOM = 4
}
/**
 * Stereoscopic output mode (C++ `SCwebviewer::StereoMode`).
 */
export declare enum StereoMode {
    MONO = 0,
    REDCYAN = 1,
    ROWINTERLACED = 2,
    ROWINTERLACEDINV = 3
}
/**
 * Represents Camera manager.
 * See also {@link SCwebviewer.camera} property.
 */
export declare class Camera {
    private _myViewId;
    private _myViewMgr;
    /** Main constructor. */
    constructor(theViewer: SCwebviewer, theViewId?: ViewId);
    /** Return AABB of all visible objects in the view. */
    get sceneMinMaxVisible(): IBndBox3;
    /** Return AABB of selected objects in the view. */
    get sceneMinMaxSelection(): IBndBox3;
    /** Orbit acceleration coefficient. */
    get orbitAcceleration(): number;
    /** Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit. */
    get maxCamDistanceMultiplier(): number;
    /** Return current camera view point parameters. */
    get viewPoint(): ICameraViewPoint;
    /** Allow/disallow camera rotation. */
    get toAllowRotation(): boolean;
    /** Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up. */
    get toLockOrbitZUp(): boolean;
    /** Allow/disallow camera panning. */
    get toAllowPanning(): boolean;
    /** Allow/disallow camera zooming. */
    get toAllowZooming(): boolean;
    /** Return left mouse button gesture; ROTATE (orbital) by default. */
    get leftMouseGesture(): MouseGesture;
    /** Return middle mouse button gesture; PAN by default. */
    get middleMouseGesture(): MouseGesture;
    /** Return right mouse button gesture; ROTATEVIEW by default. */
    get rightMouseGesture(): MouseGesture;
    /** Switch perspective/orthographic projection. */
    get isPerspectiveProjection(): boolean;
    /** Perspective projection FOVy in degrees. */
    get perspectiveProjectionFOVy(): number;
    /** Stereoscopic mode; MONO by default. */
    get stereoMode(): StereoMode;
    /** Orbit acceleration coefficient. */
    set orbitAcceleration(theAccel: number);
    /** Set max camera distance multiplier. */
    set maxCamDistanceMultiplier(theMax: number);
    /** Set camera view point parameters. */
    set viewPoint(theParams: ICameraViewPoint);
    /** Allow/disallow camera rotation. */
    set toAllowRotation(theToAllow: boolean);
    /** Get/set if camera up orientation should be forced Z up. */
    set toLockOrbitZUp(theToLock: boolean);
    /** Allow/disallow camera panning. */
    set toAllowPanning(theToAllow: boolean);
    /** Allow/disallow camera zooming. */
    set toAllowZooming(theToAllow: boolean);
    /** Set left mouse button gesture. */
    set leftMouseGesture(theGest: MouseGesture);
    /** Set middle mouse button gesture. */
    set middleMouseGesture(theGest: MouseGesture);
    /** Set right mouse button gesture. */
    set rightMouseGesture(theGest: MouseGesture);
    /** Switch perspective/orthographic projection. */
    set isPerspectiveProjection(theIsPersp: boolean);
    /** Perspective projection FOVy in degrees. */
    set perspectiveProjectionFOVy(theFovy: number);
    /** Stereoscopic mode. */
    set stereoMode(theMode: StereoMode);
    /** Return AABB of all or selected objects in the view. */
    getSceneMinMax(theSelected: boolean): IBndBox3;
    /** Fit selected or fit all visible objects into view. */
    fitAllAuto(theIsAuto: boolean, theToAnimate?: boolean): void;
    /** Return TRUE if box tightly fits into the view. */
    isFitMinMax(theMinMax: IBndBox3): boolean;
    /** Fit box into view. */
    fitMinMax(theMinMax: IBndBox3, theToAnimate: boolean): void;
    /**
     * Set camera view orientation.
     * @param theOrientation viewer orientation
     */
    setOrientation(theOrientation: CameraOrientation, theToAnimate?: boolean): void;
}
