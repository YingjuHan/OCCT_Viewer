
import type {IManager, IVec3, ObjectId} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';
import type {SCwebviewer} from './SCwebviewer.js';


/**
 * Parameters interface for an angular arrow of the coordinate system.
 */
export interface IAngularAxesParams {
  /** Angular axis ID, should be 'angular1' or 'angular2'. */
  axisId: string;
  /** Orientation of the angular arrow defined as rotation axis with a sign, e.g. +z or -x. */
  orientation: string;
  /** The attachment axis (zero axis) of the angular arrow. */
  zeroAxis: string;
  /** The text label of the angular arrow. */
  label: string;
};

/**
 * Parameters object for an angular arrow of the coordinate system.
 */
export class AngularAxesParams implements IAngularAxesParams {
  /** Angular axis ID is empty by default. */
  axisId: string = '';
  /** Orientation of the angular arrow defined as rotation axis with a sign, e.g. +z or -x. */
  orientation: string = '';
  /** The attachment axis (zero axis) of the angular arrow. */
  zeroAxis: string = '';
  /** The text label of the angular arrow. */
  label: string = '';
};

/**
 * Represents spherical coordinate axes interactive object.
 */
export class InteractiveSphericalAxes extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theObjId: ObjectId = '') {
      super(theObjId);
      if (theViewer != null) {
        this._myViewer = theViewer;
        if (this._myObjId === '') {
          this._myObjId = ':::InteractiveSphericalAxes';
        }
        this.axesMgr?.createSphericalAxes(this._myObjId);
        this.objMgr?.addInteractiveObject(this);
      }
    }

    /** Returns axes position. */
    public get position(): IVec3 { return this.axesMgr?.getAxesPosition(this._myObjId) ?? { x: 0.0, y: 0.0, z: 0.0 }; }

    /** Returns axes scale. */
    public get scale(): number { return this.axesMgr?.getAxesScale(this._myObjId) ?? 1.0; }

    /** Returns the orientation of longitude/latitude circles. */
    public get circlesOrientation(): string { return this.axesMgr?.getOrientation(this._myObjId) ?? 'oz'; }

    /** Returns the parameters of two angular axes. */
    public get angularAxesParams(): AngularAxesParams[] {
      let aParams: AngularAxesParams[] = new Array<AngularAxesParams>();
      if (this.axesMgr) {
        let anAxesIds: string[] = ['angular1', 'angular2'];
        for (const anAxisId of anAxesIds) {
          let anAxisParams = new AngularAxesParams();
          anAxisParams.axisId = anAxisId;
          anAxisParams.orientation = this.axesMgr!.getAngularArrowDir(this._myObjId, anAxisId);
          anAxisParams.zeroAxis = this.axesMgr!.getAngularArrowAxis(this._myObjId, anAxisId);
          anAxisParams.label = this.axesMgr!.getAngularArrowLabel(this._myObjId, anAxisId);
          aParams.push(anAxisParams);
        }
      }

      return aParams;
    }

    /** Sets axes position. */
    public set position(thePos: IVec3) { this.axesMgr?.setAxesPosition(this._myObjId, thePos); }

    /** Sets axes scale. */
    public set scale(theScale: number) { this.axesMgr?.setAxesScale(this._myObjId, theScale); }

    /** Sets the orientation of longitude/latitude circles. */
    public set circlesOrientation(theAxis: string) { this.axesMgr?.setOrientation(this._myObjId, theAxis); }

    /** Sets the parameters of two angular axes. */
    public set angularAxesParams(theAxisParams: AngularAxesParams[]) {
      if (this.axesMgr) {
        for (const anAxisParams of theAxisParams) {
          this.axesMgr!.setAngularArrowDir(this._myObjId, anAxisParams.axisId, anAxisParams.orientation);
          this.axesMgr!.setAngularArrowAxis(this._myObjId, anAxisParams.axisId, anAxisParams.zeroAxis);
          this.axesMgr!.setAngularArrowLabel(this._myObjId, anAxisParams.axisId, anAxisParams.label);
        }
      }
    }

    /** @internal Shortcut to C++ axes objects manager. */
    protected get axesMgr(): InteractiveAxesManager | null { return this._myViewer?.interactiveAxesManager ?? null; }

    /** Make a copy of existing object with new id. */
    public clone(theObjId: ObjectId = ''): InteractiveSphericalAxes | null {
      let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
      if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
      let aCopy = new InteractiveSphericalAxes(null);
      aCopy.wrap(this._myViewer, anObjId);
      this.objMgr?.addInteractiveObject(aCopy);
      return aCopy;
    }
}

/**
 * @internal Represents Context for Interactive axes objects.
 * Note that there is no reason to use class directly at application level
 * 
 * as {@link InteractiveSphericalAxes} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveAxesManager} property.
 */
export interface InteractiveAxesManager extends IManager {
    // C++ methods

    /** @internal Create interactive spherical axes presentation. */
    createSphericalAxes: (theObjId: ObjectId) => void;

    /** @internal Get position of specified axes object. */
    getAxesPosition: (theObjId: ObjectId) => IVec3;

    /** @internal Set position of specified axes object. */
    setAxesPosition: (theObjId: ObjectId, thePos: IVec3) => void;

    /** @internal Get scale of specified axes object. */
    getAxesScale: (theObjId: ObjectId) => number;

    /** @internal Set scale of specified axes object. */
    setAxesScale: (theObjId: ObjectId, theScale: number) => void;

    /** @internal Get the angular arrow direction. */
    getAngularArrowDir: (theObjId: ObjectId, theAxisId: string) => string;

    /** @internal Set the angular arrow direction. */
    setAngularArrowDir: (theObjId: ObjectId, theAxisId: string, theDir: string) => void;

    /** @internal Get the attachment axis (zero axis) of the angular arrow. */
    getAngularArrowAxis: (theObjId: ObjectId, theAxisId: string) => string;

    /** @internal Set the attachment axis (zero axis) of the angular arrow. */
    setAngularArrowAxis: (theObjId: ObjectId, theAxisId: string, theZeroAxis: string) => void;

    /** @internal Get the the angular arrow text label. */
    getAngularArrowLabel: (theObjId: ObjectId, theAxisId: string) => string;

    /** @internal Set the the angular arrow text label. */
    setAngularArrowLabel: (theObjId: ObjectId, theAxisId: string, theLabel: string) => void;

    /** @internal Get the orientation axis of the longitude/latitude circles. */
    getOrientation: (theObjId: ObjectId) => string;

    /** @internal Set the orientation axis of the longitude/latitude circles. */
    setOrientation: (theObjId: ObjectId, theAxis: string) => void;
};