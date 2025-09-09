import type {CameraOrientation, ICameraViewPoint, IBndBox3, ViewId} from './CommonTools.js';
import {BndBox3} from './CommonTools.js';
import type {ViewManager} from './ViewManager.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * Active left mouse button gesture (C++ `SCwebviewer::MouseGesture`).
 */
export enum MouseGesture { NONE = 0, ROTATE = 1, ROTATEVIEW = 2, PAN = 3, ZOOM = 4 };

/**
 * Stereoscopic output mode (C++ `SCwebviewer::StereoMode`).
 */
export enum StereoMode { MONO = 0, REDCYAN = 1, ROWINTERLACED = 2, ROWINTERLACEDINV = 3 };

/**
 * Represents Camera manager.
 * See also {@link SCwebviewer.camera} property.
 */
export class Camera {
  private _myViewId: ViewId;
  private _myViewMgr: ViewManager;

  /** Main constructor. */
  constructor(theViewer: SCwebviewer, theViewId: ViewId = -1) {
    if (!theViewer.viewManager) { throw new Error('Invalid viewer'); }

    this._myViewId = theViewId;
    this._myViewMgr = theViewer.viewManager;
  }

  /** Return AABB of all visible objects in the view. */
  public get sceneMinMaxVisible(): IBndBox3 { return this._myViewMgr.getSceneMinMax(this._myViewId, false); }

  /** Return AABB of selected objects in the view. */
  public get sceneMinMaxSelection(): IBndBox3 { return this._myViewMgr.getSceneMinMax(this._myViewId, true); }

  /** Orbit acceleration coefficient. */
  public get orbitAcceleration(): number { return this._myViewMgr.getOrbitAcceleration(); }

  /** Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit. */
  public get maxCamDistanceMultiplier(): number { return this._myViewMgr.getMaxCamDistanceMultiplier(); }

  /** Return current camera view point parameters. */
  public get viewPoint(): ICameraViewPoint { return this._myViewMgr.getViewPoint(this._myViewId); }

  /** Allow/disallow camera rotation. */
  public get toAllowRotation(): boolean { return this._myViewMgr.toAllowRotation(this._myViewId); }

  /** Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up. */
  public get toLockOrbitZUp(): boolean { return this._myViewMgr.toLockOrbitZUp(this._myViewId); }

  /** Allow/disallow camera panning. */
  public get toAllowPanning(): boolean { return this._myViewMgr.toAllowPanning(this._myViewId); }

  /** Allow/disallow camera zooming. */
  public get toAllowZooming(): boolean { return this._myViewMgr.toAllowZooming(this._myViewId); }

  /** Return left mouse button gesture; ROTATE (orbital) by default. */
  public get leftMouseGesture(): MouseGesture { return this._myViewMgr.getMouseGesture(this._myViewId, 0); }

  /** Return middle mouse button gesture; PAN by default. */
  public get middleMouseGesture(): MouseGesture { return this._myViewMgr.getMouseGesture(this._myViewId, 1); }

  /** Return right mouse button gesture; ROTATEVIEW by default. */
  public get rightMouseGesture(): MouseGesture { return this._myViewMgr.getMouseGesture(this._myViewId, 2); }

  /** Switch perspective/orthographic projection. */
  public get isPerspectiveProjection(): boolean { return this._myViewMgr.isPerspectiveProjection(this._myViewId); }

  /** Perspective projection FOVy in degrees. */
  public get perspectiveProjectionFOVy(): number { return this._myViewMgr.getPerspectiveProjectionFOVy(this._myViewId); }

  /** Stereoscopic mode; MONO by default. */
  public get stereoMode(): StereoMode { return this._myViewMgr.getStereoMode(this._myViewId); }

  /** Orbit acceleration coefficient. */
  public set orbitAcceleration(theAccel: number) { this._myViewMgr.setOrbitAcceleration(theAccel); }

  /** Set max camera distance multiplier. */
  public set maxCamDistanceMultiplier(theMax: number) { this._myViewMgr.setMaxCamDistanceMultiplier(theMax); }

  /** Set camera view point parameters. */
  public set viewPoint(theParams: ICameraViewPoint) { this._myViewMgr.setViewPoint(this._myViewId, theParams); }

  /** Allow/disallow camera rotation. */
  public set toAllowRotation(theToAllow: boolean) { this._myViewMgr.setAllowRotation(this._myViewId, theToAllow); }

  /** Get/set if camera up orientation should be forced Z up. */
  public set toLockOrbitZUp(theToLock: boolean) { this._myViewMgr.setLockOrbitZUp(this._myViewId, theToLock); }

  /** Allow/disallow camera panning. */
  public set toAllowPanning(theToAllow: boolean) { this._myViewMgr.setAllowPanning(this._myViewId, theToAllow); }

  /** Allow/disallow camera zooming. */
  public set toAllowZooming(theToAllow: boolean) { this._myViewMgr.setAllowZooming(this._myViewId, theToAllow); }

  /** Set left mouse button gesture. */
  public set leftMouseGesture(theGest: MouseGesture) { this._myViewMgr.setMouseGesture(this._myViewId, 0, theGest); }

  /** Set middle mouse button gesture. */
  public set middleMouseGesture(theGest: MouseGesture) { this._myViewMgr.setMouseGesture(this._myViewId, 1, theGest); }

  /** Set right mouse button gesture. */
  public set rightMouseGesture(theGest: MouseGesture) { this._myViewMgr.setMouseGesture(this._myViewId, 2, theGest); }

  /** Switch perspective/orthographic projection. */
  public set isPerspectiveProjection(theIsPersp: boolean) { this._myViewMgr.setPerspectiveProjection(this._myViewId, theIsPersp); }

  /** Perspective projection FOVy in degrees. */
  public set perspectiveProjectionFOVy(theFovy: number) { this._myViewMgr.setPerspectiveProjectionFOVy(this._myViewId, theFovy); }

  /** Stereoscopic mode. */
  public set stereoMode(theMode: StereoMode) { this._myViewMgr.setStereoMode(this._myViewId, theMode); }

  /** Return AABB of all or selected objects in the view. */
  public getSceneMinMax(theSelected: boolean): IBndBox3 { return this._myViewMgr.getSceneMinMax(this._myViewId, theSelected); }

  /** Fit selected or fit all visible objects into view. */
  public fitAllAuto(theIsAuto: boolean, theToAnimate: boolean = false): void {
    // @c this._myViewMgr.fitAllAuto(this._myViewId, theIsAuto);
    const aBoxAll: IBndBox3 = this.sceneMinMaxVisible;
    if (!theIsAuto) {
      this.fitMinMax(aBoxAll, theToAnimate);
      return;
    }

    const aBoxSel: IBndBox3 = this.sceneMinMaxSelection;
    const aBoxFit = (BndBox3.isVoid(aBoxSel) || this.isFitMinMax(aBoxSel)) ? aBoxAll : aBoxSel;
    this.fitMinMax(aBoxFit, theToAnimate);
  }

  /** Return TRUE if box tightly fits into the view. */
  public isFitMinMax(theMinMax: IBndBox3): boolean { return this._myViewMgr.isFitMinMax(this._myViewId, theMinMax); }

  /** Fit box into view. */
  public fitMinMax(theMinMax: IBndBox3, theToAnimate: boolean): void {
    this._myViewMgr.fitMinMax(this._myViewId, theMinMax, theToAnimate);
  }

  /**
   * Set camera view orientation.
   * @param theOrientation viewer orientation
   */
  public setOrientation(theOrientation: CameraOrientation, theToAnimate: boolean = false): void {
    this._myViewMgr.setOrientation(this._myViewId, theOrientation, theToAnimate);
  }
};
