import type {IManager, ObjectId, IVec2} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * Represents Scale Ruler property manager.
 * Scale Ruler is an auxiliary interactive object displayed length measure units at view corner.
 * See also {@link SCwebviewer.scaleRuler} property.
 */
export class InteractiveScaleRuler extends InteractiveObject {
  /** Empty constructor. */
  constructor(theViewer: SCwebviewer | null, theObjId: ObjectId = '') {
    super(theObjId);
    if (theViewer != null) {
      this._myViewer = theViewer;
      if (this._myObjId === '') {
        this._myObjId = ':::InteractiveScaleRuler';
      }
      this.scaleRulerMgr?.createScaleRuler(this._myObjId);
      this.objMgr?.addInteractiveObject(this);
    }
  }

  /** Return scale ruler base size. */
  public get baseSize(): IVec2 { return this.scaleRulerMgr?.getBaseSize(this._myObjId) ?? {x: 0.0, y: 0.0}; }

  /** @internal Shortcut to scale ruler context. */
  protected get scaleRulerMgr(): InteractiveScaleRulerManager | null { return this._myViewer?.interactiveScaleRulerManager ?? null; }

  /** Set scale ruler base size. */
  public set baseSize(theSize: IVec2) { this.scaleRulerMgr?.setBaseSize(this._myObjId, theSize); }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveScaleRuler | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveScaleRuler(null);
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr?.addInteractiveObject(aCopy);
    return aCopy;
  }
};

/**
 * @internal Represents Context for Interactive scale ruler.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveScaleRuler} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveScaleRulerManager} property.
 */
export interface InteractiveScaleRulerManager extends IManager {
// C++ methods

  /** @internal Create interactive scale ruler object. */
  createScaleRuler: (theObjId: ObjectId) => void;

  /** @internal Return scale ruler base size. */
  getBaseSize: (theObjId: ObjectId) => IVec2;

  /** @internal Set scale ruler base size. */
  setBaseSize: (theObjId: ObjectId, theSize: IVec2) => void;
};
