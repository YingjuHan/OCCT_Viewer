// Created by: Kirill Gavrilov
// Copyright (c) 2023 Huawei Technologies Co., Ltd.

import type {IManager, ObjectId, IVec3, IBndBox3} from './CommonTools.js';
import {Vec3, BndBox3} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';
import type {Subview} from './Subview.js';

import type {SCwebviewer} from './SCwebviewer.js';

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
  constructor(theViewer: SCwebviewer | null, theView: Subview | null, theObjId: ObjectId = '') {
    super(theObjId);
    if (theViewer != null) {
      this._myViewer = theViewer;
      if (this._myObjId === '') {
        this._myObjId = ':::InteractiveClipping';
      }
      this.clipMgr?.createClipping(this._myObjId, theView == null ? -2 : theView.id);
      this.objMgr?.addInteractiveObject(this);
    }
  }

  /** Return clipping flag; FALSE by default. */
  public get isClippingOn(): boolean { return this.clipMgr?.isClippingOn(this._myObjId) ?? false; }

  /** Return capping flag; FALSE by default. */
  public get isCappingOn(): boolean { return this.clipMgr?.isCappingOn(this._myObjId) ?? false; }

  /** Return TRUE for using object's color for capping. */
  public get toUseObjectMaterial(): boolean { return this.clipMgr?.toUseObjectMaterial(this._myObjId) ?? false; }

  /** Return shaded presentation flag; TRUE by default. */
  public get isShaded(): boolean { return this.clipMgr?.isShaded(this._myObjId) ?? false; }

  /** Return clipping box flipping flag; FALSE by default. */
  public get toFlipClipping(): boolean { return this.clipMgr?.toFlipClipping(this._myObjId) ?? false; }

  /** Return clipping box configuration - main corner point (origin). */
  public get clippingCorner(): IVec3 { return this.clipMgr?.getClippingCorner(this._myObjId, false) ?? Vec3.zero(); }

  /** Return clipping box configuration - main corner point (origin), relative to scene box. */
  public get relativeClippingCorner(): IVec3 { return this.clipMgr?.getClippingCorner(this._myObjId, true) ?? Vec3.zero(); }

  /**
   * Return clipping box configuration - box extent.
   * - zero extent disables clipping within specific dimension;
   * - finite extent defines finite clipping range from corner point (origin),
   *   e.g. a pair of clipping planes with opposite directions clipping box interior;
   * - infinite extent defines half-space clipping, e.g. a single clipping plane;
   * - negative extent specifies clipping side relative to corner point.
   */
  public get clippingExtent(): IVec3 { return this.clipMgr?.getClippingExtent(this._myObjId, false) ?? Vec3.zero(); }

  /** Return clipping box configuration - box extent, relative to scene box extent. */
  public get relativeClippingExtent(): IVec3 { return this.clipMgr?.getClippingExtent(this._myObjId, true) ?? Vec3.zero(); }

  /**
   * Return clipping box configuration - number of clipping planes per dimension (0, -1, 1, or -2, 2),
   * where negative number means negative extent
   * (e.g. -1 means clipping in opposite direction,
   * -2 means building a box in opposite direction from main corner point).
   */
  public get nbPlanesXYZ(): IVec3 { return this.clipMgr?.getNbPlanesXYZ(this._myObjId) ?? Vec3.zero(); }

  /** Return clipping box configuration. */
  public get clippingBox(): IBndBox3 { return this.clipMgr?.getClippingBox(this._myObjId, false) ?? BndBox3.voidBox(); }

  /** Return clipping box configuration, relative to scene box. */
  public get relativeClippingBox(): IBndBox3 { return this.clipMgr?.getClippingBox(this._myObjId, true) ?? BndBox3.voidBox(); }

  /** @internal Shortcut to clipping manager. */
  protected get clipMgr(): InteractiveClippingManager | null { return this._myViewer?.interactiveClippingManager ?? null; }

  /** Set clipping flag. */
  public set isClippingOn(theToCap: boolean) { this.clipMgr?.setClippingOn(this._myObjId, theToCap); }

  /** Set capping flag. */
  public set isCappingOn(theToCap: boolean) { this.clipMgr?.setCappingOn(this._myObjId, theToCap); }

  /** Set flag for using object's color for capping. */
  public set toUseObjectMaterial(theToUse: boolean) { this.clipMgr?.setUseObjectMaterial(this._myObjId, theToUse); }

  /** Set shaded presentation flag. */
  public set isShaded(theIsShaded: boolean) { this.clipMgr?.setShaded(this._myObjId, theIsShaded); }

  /** Set clipping box flipping flag. */
  public set toFlipClipping(theToFlip: boolean) { this.clipMgr?.setFlipClipping(this._myObjId, theToFlip); }

  /** Set clipping box configuration - main corner point (origin). */
  public set clippingCorner(thePnt: IVec3) { this.clipMgr?.setClippingCorner(this._myObjId, thePnt, false); }

  /** Set clipping box configuration - main corner point (origin), relative to scene box. */
  public set relativeClippingCorner(thePnt: IVec3) { this.clipMgr?.setClippingCorner(this._myObjId, thePnt, true); }

  /** Set clipping box configuration - box extent. */
  public set clippingExtent(theExt: IVec3) { this.clipMgr?.setClippingExtent(this._myObjId, theExt, false); }

  /** Set clipping box configuration - box extent, relative to scene box extent. */
  public set relativeClippingExtent(theExt: IVec3) { this.clipMgr?.setClippingExtent(this._myObjId, theExt, true); }

  /** Set clipping box configuration - number of clipping planes per dimension. */
  public set nbPlanesXYZ(theVec: IVec3) { this.clipMgr?.setNbPlanesXYZ(this._myObjId, theVec); }

  /** Set clipping box configuration. */
  public set clippingBox(theBox: IBndBox3) { this.clipMgr?.setClippingBox(this._myObjId, theBox, false); }

  /** Set clipping box configuration, relative to scene box. */
  public set relativeClippingBox(theBox: IBndBox3) { this.clipMgr?.setClippingBox(this._myObjId, theBox, true); }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveClipping | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveClipping(null, null);
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr?.addInteractiveObject(aCopy);
    return aCopy;
  }
};

/**
 * @internal Represents Context for Interactive clipping object.
 * See also {@link SCwebviewer.interactiveClippingManager} property.
 */
export interface InteractiveClippingManager extends IManager {
// C++ methods

  /** @internal Create interactive clipping object. */
  createClipping: (theObjId: ObjectId, theViewId: number) => void;

  /** @internal Return clipping box configuration - corner point. */
  getClippingCorner: (theObjId: ObjectId, theIsRel: boolean) => IVec3;

  /** @internal Set clipping box configuration - corner point. */
  setClippingCorner: (theObjId: ObjectId, thePnt: IVec3, theIsRel: boolean) => void;

  /** @internal Return clipping box configuration - box extent. */
  getClippingExtent: (theObjId: ObjectId, theIsRel: boolean) => IVec3;

  /** @internal Set clipping box configuration - box extent. */
  setClippingExtent: (theObjId: ObjectId, theExt: IVec3, theIsRel: boolean) => void;

  /** @internal Return clipping box configuration. */
  getClippingBox: (theObjId: ObjectId, theIsRel: boolean) => IBndBox3;

  /** @internal Set clipping box configuration. */
  setClippingBox: (theObjId: ObjectId, theBox: IBndBox3, theIsRel: boolean) => void;

  /** @internal Return clipping box configuration - number of clipping planes per dimension (0, -1, 1, or -2, 2). */
  getNbPlanesXYZ: (theObjId: ObjectId) => IVec3;

  /** @internal Set clipping box configuration - number of clipping planes per dimension. */
  setNbPlanesXYZ: (theObjId: ObjectId, thePnt: IVec3) => void;

  /** @internal Return clipping box flipping flag. */
  toFlipClipping: (theObjId: ObjectId) => boolean;

  /** @internal Set clipping box flipping flag. */
  setFlipClipping: (theObjId: ObjectId, theToFlip: boolean) => void;

  /** @internal Return clipping flag. */
  isClippingOn: (theObjId: ObjectId) => boolean;

  /** @internal Set clipping flag. */
  setClippingOn: (theObjId: ObjectId, theToClip: boolean) => void;

  /** @internal Return capping flag. */
  isCappingOn: (theObjId: ObjectId) => boolean;

  /** @internal Set capping flag. */
  setCappingOn: (theObjId: ObjectId, theToCap: boolean) => void;

  /** @internal Return TRUE for using object's color for capping. */
  toUseObjectMaterial: (theObjId: ObjectId) => boolean;

  /** @internal Set flag for using object's color for capping. */
  setUseObjectMaterial: (theObjId: ObjectId, theToUse: boolean) => void;

  /** @internal Return shaded presentation flag. */
  isShaded: (theObjId: ObjectId) => boolean;

  /** @internal Set shaded presentation flag. */
  setShaded: (theObjId: ObjectId, theIsShaded: boolean) => void;
};
