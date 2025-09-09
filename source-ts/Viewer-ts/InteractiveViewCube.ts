import type {IManager, ObjectId} from './CommonTools.js';
import {CameraOrientation} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * Represents ViewCube property manager.
 * ViewCube is an auxiliary interactive object displayed at View corner and showing current View orientation.
 * See also {@link SCwebviewer.viewCube} property.
 */
export class InteractiveViewCube extends InteractiveObject {
  /** Empty constructor. */
  constructor(theViewer: SCwebviewer | null, theObjId: ObjectId = '') {
    super(theObjId);
    if (theViewer != null) {
      this._myViewer = theViewer;
      if (this._myObjId === '') {
        this._myObjId = ':::InteractiveViewCube';
      }
      this.viewCubeMgr?.createViewCube(this._myObjId);
      this.objMgr?.addInteractiveObject(this);
    }
  }

  /** Return view cube base size. */
  public get baseSize(): number { return this.viewCubeMgr?.getViewCubeBaseSize(this._myObjId) ?? 0.0; }

  /** Return view cube side label. */
  public get boxSideLabelFront(): string { return this.getBoxSideLabel(CameraOrientation.FRONT); }

  /** Return view cube side label. */
  public get boxSideLabelBack(): string { return this.getBoxSideLabel(CameraOrientation.BACK); }

  /** Return view cube side label. */
  public get boxSideLabelLeft(): string { return this.getBoxSideLabel(CameraOrientation.LEFT); }

  /** Return view cube side label. */
  public get boxSideLabelRight(): string { return this.getBoxSideLabel(CameraOrientation.RIGHT); }

  /** Return view cube side label. */
  public get boxSideLabelTop(): string { return this.getBoxSideLabel(CameraOrientation.TOP); }

  /** Return view cube side label. */
  public get boxSideLabelBottom(): string { return this.getBoxSideLabel(CameraOrientation.BOTTOM); }

  /** @internal Shortcut to view cube context. */
  protected get viewCubeMgr(): InteractiveViewCubeManager | null { return this._myViewer?.interactiveViewCubeManager ?? null; }

  /** Set view cube base size. */
  public set baseSize(theSize: number) { this.viewCubeMgr?.setViewCubeBaseSize(this._myObjId, theSize); }

  /** Set view cube side label. */
  public set boxSideLabelFront(theLabel: string) { this.setBoxSideLabel(CameraOrientation.FRONT, theLabel); }

  /** Set view cube side label. */
  public set boxSideLabelBack(theLabel: string) { this.setBoxSideLabel(CameraOrientation.BACK, theLabel); }

  /** Set view cube side label. */
  public set boxSideLabelLeft(theLabel: string) { this.setBoxSideLabel(CameraOrientation.LEFT, theLabel); }

  /** Set view cube side label. */
  public set boxSideLabelRight(theLabel: string) { this.setBoxSideLabel(CameraOrientation.RIGHT, theLabel); }

  /** Set view cube side label. */
  public set boxSideLabelTop(theLabel: string) { this.setBoxSideLabel(CameraOrientation.TOP, theLabel); }

  /** Set view cube side label. */
  public set boxSideLabelBottom(theLabel: string) { this.setBoxSideLabel(CameraOrientation.BOTTOM, theLabel); }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveViewCube | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveViewCube(null);
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr?.addInteractiveObject(aCopy);
    return aCopy;
  }

  /** Return view cube side label. */
  public getBoxSideLabel(theSide: CameraOrientation): string { return this.viewCubeMgr?.getBoxSideLabel(this._myObjId, theSide) ?? ''; }

  /** Set view cube side label. */
  public setBoxSideLabel(theSide: CameraOrientation, theLabel: string): void { this.viewCubeMgr?.setBoxSideLabel(this._myObjId, theSide, theLabel); }
};

/**
 * @internal Represents Context for Interactive view cube.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveViewCube} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveViewCubeManager} property.
 */
export interface InteractiveViewCubeManager extends IManager {
// C++ methods

  /** @internal Create interactive view cube object. */
  createViewCube: (theObjId: ObjectId) => void;

  /** @internal Return view cube base size. */
  getViewCubeBaseSize: (theObjId: ObjectId) => number;

  /** @internal Set view cube base size. */
  setViewCubeBaseSize: (theObjId: ObjectId, theSize: number) => void;

  /** @internal Get view cube side label within V3d_TypeOfOrientation enumeration subset. */
  getBoxSideLabel: (theObjId: ObjectId, theSize: number) => string;

  /** @internal Set view cube side label within V3d_TypeOfOrientation enumeration subset. */
  setBoxSideLabel: (theObjId: ObjectId, theSize: number, theLabel: string) => void;
};
