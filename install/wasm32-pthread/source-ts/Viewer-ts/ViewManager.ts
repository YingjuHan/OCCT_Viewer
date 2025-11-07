// Created by: Kirill Gavrilov
// Copyright (c) 2022 Huawei Technologies Co., Ltd.

import type {IManager, CameraOrientation, ICameraViewPoint, ColorString, IVec2, IBndBox3, ViewCorner, ViewId, ObjectId} from './CommonTools.js';

import type {Subview} from './Subview.js';

/**
 * Represents manager for subviews.
 * See also {@link SCwebviewer.viewManager} property.
 */
export interface ViewManager extends IManager {
// JS extensions

  /** Return the array of created subviews */
  readonly subviews: Subview[];

  /** @internal Add subview object */
  addSubviewObject: (theSubview: Subview) => void;

  /** @internal find existing Subview object */
  findSubviewObject: (theViewId: ViewId) => Subview | null;

// C++ implementation

  /** Orbit acceleration coefficient. */
  getOrbitAcceleration: () => number;

  /** Orbit acceleration coefficient. */
  setOrbitAcceleration: (theAccel: number) => void;

  /** Max camera distance multiplier; when it equals 0 or negative then disable the max zoom limit. */
  getMaxCamDistanceMultiplier: () => number;

  /** Set max camera distance multiplier. */
  setMaxCamDistanceMultiplier: (theMax: number) => void;

  /** Get subview border color (color filling in margins between all subviews). */
  getSubviewBorderColor: () => ColorString;

  /** Set subview border color. */
  setSubviewBorderColor: (theColor: ColorString) => void;

  /**
   * Return fullscreen mode, when canvas (with all Subviews) occupies entire screen.
   * See also {@link ViewManager.isSubviewFullscreen}.
   */
  isFullscreen: () => boolean;

  /**
   * Set fullscreen mode;
   * WARNING! This should be called in combination with user gesture (mouse click / button click) in browser!
   */
  setFullscreen: (theToEnable: boolean) => void;

//

  /** Return bounding box of all visible or selected objects in the view. */
  getSceneMinMax: (theViewId: ViewId, theSelection: boolean) => IBndBox3;

  /** Return TRUE if obx tightly fits into the view. */
  isFitMinMax: (theViewId: ViewId, theMinMax: IBndBox3) => boolean;

  /** Fit box into the view. */
  fitMinMax: (theViewId: ViewId, theMinMax: IBndBox3, theToAnimate: boolean) => void;

  /** Fit selected or fit all visible objects into view. */
  fitAllAuto: (theViewId: ViewId, theIsAuto: boolean) => void;

  /**
   * Set camera view orientation.
   * @param theOrientation viewer orientation
   */
  setOrientation: (theViewId: ViewId, theOrientation: CameraOrientation, theToAnimate: boolean) => void;

  /** Return current camera view point parameters. */
  getViewPoint: (theViewId: ViewId) => ICameraViewPoint;

  /** Set camera view point parameters. */
  setViewPoint: (theViewId: ViewId, theParams: ICameraViewPoint) => void;

  /** Allow/disallow camera rotation. */
  toAllowRotation: (theViewId: ViewId) => boolean;

  /** Allow/disallow camera rotation. */
  setAllowRotation: (theViewId: ViewId, theToAllow: boolean) => void;

  /** Get/set if camera up orientation within `AIS_NavigationMode_Orbit` rotation mode should be forced Z up. */
  toLockOrbitZUp: (theViewId: ViewId) => boolean;

  /** Set if camera up orientation should be forced Z up. */
  setLockOrbitZUp: (theViewId: ViewId, theToLock: boolean) => void;

  /** Allow/disallow camera panning. */
  toAllowPanning: (theViewId: ViewId) => boolean;

  /** Allow/disallow camera panning. */
  setAllowPanning: (theViewId: ViewId, theToAllow: boolean) => void;

  /** Allow/disallow camera zooming. */
  toAllowZooming: (theViewId: ViewId) => boolean;

  /** Allow/disallow camera zooming. */
  setAllowZooming: (theViewId: ViewId, theToAllow: boolean) => void;

  /** Return mouse button gesture. */
  getMouseGesture: (theViewId: ViewId, theButton: number) => number;

  /** Set mouse button gesture. */
  setMouseGesture: (theViewId: ViewId, theButton: number, theGest: number) => void;

  /** Switch perspective/orthographic projection. */
  isPerspectiveProjection: (theViewId: ViewId) => boolean;

  /** Switch perspective/orthographic projection. */
  setPerspectiveProjection: (theViewId: ViewId, theIsPersp: boolean) => void;

  /** Perspective projection FOVy in degrees. */
  getPerspectiveProjectionFOVy: (theViewId: ViewId) => number;

  /** Perspective projection FOVy in degrees. */
  setPerspectiveProjectionFOVy: (theViewId: ViewId, theFovy: number) => void;

  /** Stereoscopic mode. */
  getStereoMode: (theViewId: ViewId) => number;

  /** Stereoscopic mode. */
  setStereoMode: (theViewId: ViewId, theMode: number) => void;

//

  /** Return TRUE if view with specified {@link ViewId} exists. */
  isDefined: (theViewId: ViewId) => boolean;

  /** Return TRUE if view with specified {@link ViewId} is active (displayed on the screen). */
  isActive: (theViewId: ViewId) => boolean;

  /** Set view {@link ViewId} active state. */
  setActive: (theViewId: ViewId, theActive: boolean) => void;

  /** Return TRUE if view with specified {@link ViewId} serves as a composer to subviews. */
  isSubviewComposer: (theViewId: ViewId) => boolean;

  /** Set if view with specified {@link ViewId} should serve as a composer to subviews. */
  setSubviewComposer: (theViewId: ViewId, theActive: boolean) => void;

  /** Return id of view that receives input focus. */
  getInputFocus: (theViewId: ViewId) => ViewId;

  /** Return TRUE if view with specified {@link ViewId} receives input focus. */
  hasInputFocus: (theViewId: ViewId) => boolean;

  /** Set view with specified {@link ViewId} to receive input focus. */
  setInputFocus: (theViewId: ViewId) => void;

  /** Return subview size, defined as a fraction of main canvas. */
  getSubviewSize: (theViewId: ViewId) => IVec2;

  /** Set subview size. */
  setSubviewSize: (theViewId: ViewId, theSize: IVec2) => void;

  /** Return subview offset from corner, defined as a fraction of main canvas. */
  getSubviewOffset: (theViewId: ViewId) => IVec2;

  /** Set subview offset from corner. */
  setSubviewOffset: (theViewId: ViewId, theOffset: IVec2) => void;

  /** Return subview margins defined in pixels. */
  getSubviewMargins: (theViewId: ViewId) => IVec2;

  /** Set subview margins . */
  setSubviewMargins: (theViewId: ViewId, theMargins: IVec2) => void;

  /** Return subview corner. */
  getSubviewCorner: (theViewId: ViewId) => ViewCorner;

  /** Set subview corner. */
  setSubviewCorner: (theViewId: ViewId, theCorner: ViewCorner) => void;

  /** Return subview fullscreen mode. */
  isSubviewFullscreen: (theViewId: ViewId) => boolean;

  /** Set subview fullscreen mode. */
  setSubviewFullscreen: (theViewId: ViewId, theToEnable: boolean) => void;

  /** Return subview clickable property. */
  isSubviewClickable: (theViewId: ViewId) => boolean;

  /** Set subview fullscreen mode. */
  setSubviewClickable: (theViewId: ViewId, theToEnable: boolean) => void;

  /** Set new subview size and return it's id. */
  createSubview: (theSize: IVec2) => number;

  /** Remove subview. */
  removeSubview: (theViewId: ViewId) => void;

  /** Remove all subview. */
  removeAllSubviews: () => void;

//

  /** Set viewer offset for specified corner. */
  setViewOffset: (theSide: ViewCorner, theOffset: IVec2) => void;

  /** Reset all viewer offsets. */
  unsetViewOffsets: () => void;

  /** Mark all 2d objects to update their position/size on the next draw call. */
  invalidate2dObjects: (theObjId: ObjectId) => void;
};
