import type {IManager, ObjectId, IVec3, IAx2} from './CommonTools.js';
import {Vec3, Ax2} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * Describes dimension parameters interface.
 */
export interface IDimensionParams {
  /** Plane position. */
  plane: IAx2;
  /** Real computed value. */
  realValue: number;
  /** Custom text value overriding real value, when non-empty. */
  textValue: string;
  /** Special symbol (single character!) */
  specialSymbol: string;
  /** Special symbol position. */
  isSpecialSymbolAfter: boolean;
};

/**
 * Describes dimension parameters object.
 */
export class DimensionParams implements IDimensionParams {
  /** Plane position. */
  plane: IAx2 = Ax2.invalid();
  /** Real computed value. */
  realValue: number = 0.0;
  /** Custom text value overriding real value, when non-empty. */
  textValue: string = '';
  /** Special symbol (single character!) */
  specialSymbol: string = '';
  /** Special symbol position. */
  isSpecialSymbolAfter: boolean = false;
};

/**
 * Represents interactive dimension object.
 */
export class InteractiveDimension extends InteractiveObject {
  /** Empty constructor. */
  constructor(theObjId: ObjectId = '') {
    super(theObjId);
  }

  /** Return dimension plane. */
  get plane(): Ax2 { return this.dimMgr?.getDimensionParams(this._myObjId).plane ?? new Ax2(); }

  /** Return computed real value. */
  get realValue(): number { return this.dimMgr?.getDimensionParams(this._myObjId).realValue ?? 0.0; }

  /** Return custom text value, or empty string. */
  get textValue(): string { return this.dimMgr?.getDimensionParams(this._myObjId).textValue ?? ''; }

  /** Return special symbol. */
  get specialSymbol(): string { return this.dimMgr?.getDimensionParams(this._myObjId).specialSymbol ?? ' '; }

  /** Return special symbol position. */
  get isSpecialSymbolAfter(): boolean { return this.dimMgr?.getDimensionParams(this._myObjId).isSpecialSymbolAfter ?? false; }

  /** Return flag if custom text position is manually set or automatically computed. */
  get isCustomTextPosition(): boolean { return !Vec3.isInfinite(this.dimMgr?.getDimensionTextPosition(this._myObjId, true) ?? Vec3.invalid()); }

  /** Return text position. */
  get textPosition(): IVec3 { return this.dimMgr?.getDimensionTextPosition(this._myObjId, false) ?? Vec3.invalid(); }

  /** @internal Shortcut to text label context. */
  protected get dimMgr(): InteractiveDimensionManager | null { return this._myViewer?.interactiveDimensionManager ?? null; }

  /** Set custom text value, or unset it by empty string. */
  set textValue(theText: string) {
    if (!this.dimMgr) { return; }
    let aParams = this.dimMgr.getDimensionParams(this._myObjId);
    aParams.textValue = theText;
    this.dimMgr.setDimensionParams(this._myObjId, aParams);
  }

  /** Set special symbol. */
  set specialSymbol(theSymb: string) {
    if (theSymb.length > 1) { throw Error('Invalid special symbol'); }
    if (!this.dimMgr) { return; }
    let aParams = this.dimMgr.getDimensionParams(this._myObjId);
    aParams.specialSymbol = theSymb;
    this.dimMgr.setDimensionParams(this._myObjId, aParams);
  }

  /** Return special symbol position. */
  set isSpecialSymbolAfter(theIsAfter: boolean) {
    if (!this.dimMgr) { return; }
    let aParams = this.dimMgr.getDimensionParams(this._myObjId);
    aParams.isSpecialSymbolAfter = theIsAfter;
    this.dimMgr.setDimensionParams(this._myObjId, aParams);
  }

  /** Set flag if custom text position is manually set or automatically computed. */
  set isCustomTextPosition(theIsCustom: boolean) { this.textPosition = theIsCustom ? this.textPosition : null; }

  /** Set fixed text position. WARNING! Result might be adjusted. */
  set textPosition(thePnt: IVec3 | null) { this.dimMgr?.setDimensionTextPosition(this._myObjId, thePnt ? thePnt : Vec3.invalid()); }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveDimension | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveDimension();
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr?.addInteractiveObject(aCopy);
    return aCopy;
  }

  /**
   * Build length dimension between two given points.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec3} thePnt1 first point
   * @param {IVec3} thePnt2 second point
   * @param {IAx2}  thePln  plane
   */
  public buildLength(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3, thePln: IAx2 = Ax2.invalid()): boolean {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('length') ?? 'length'; }
    const aRes = this.dimMgr?.buildLengthDimension(this._myObjId, thePnt1, thePnt2, thePln) ?? false;
    this.objMgr?.addInteractiveObject(this);
    return aRes;
  }

  /**
   * Build radius dimension.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec3} theAx2    circle location
   * @param {IVec3} theRadius radius
   * @param {IAx2}  theAnchor optional anchor point on the circle
   */
  public buildRadius(theViewer: SCwebviewer, theAx2: IAx2, theRadius: number, theAnchor: IVec3 = Vec3.invalid()): boolean {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('radius') ?? 'radius'; }
    const aRes = this.dimMgr?.buildRadiusDimension(this._myObjId, theAx2, theRadius, theAnchor) ?? false;
    this.objMgr?.addInteractiveObject(this);
    return aRes;
  }

  /**
   * Build angle dimension.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec3} thePnt1 first point
   * @param {IVec3} thePnt2 second point (corner)
   * @param {IVec3} thePnt3 third point
   */
  public buildAngle(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3, thePnt3: IVec3): boolean {
    this._myViewer = theViewer;
    if (this._myObjId === '') { this._myObjId = this.objMgr?.generateId('angle') ?? 'angle'; }
    const aRes = this.dimMgr?.buildAngleDimension(this._myObjId, thePnt1, thePnt2, thePnt3) ?? false;
    this.objMgr?.addInteractiveObject(this);
    return aRes;
  }
};

/**
 * @internal Represents Context for Interactive dimension.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveDimension} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveDimensionManager} property.
 */
export interface InteractiveDimensionManager extends IManager {
// C++ methods

  /** @internal Create length interactive object. */
  createLengthDimension: (theObjId: ObjectId) => void;

  /** @internal Create radius interactive object. */
  createRadiusDimension: (theObjId: ObjectId) => void;

  /** @internal Build length dimension between two given points. */
  buildLengthDimension: (theObjId: ObjectId, thePnt1: IVec3, thePnt2: IVec3, thePln: IAx2) => boolean;

  /** @internal Build radius dimension. */
  buildRadiusDimension: (theObjId: ObjectId, theAx2: IAx2, theRadius: number, theAnchor: IVec3) => boolean;

  /** @internal Build angle dimension. */
  buildAngleDimension: (theObjId: ObjectId, thePnt1: IVec3, thePnt2: IVec3, thePnt3: IVec3) => boolean;

  /** @internal Return parameters. */
  getDimensionParams: (theObjId: ObjectId) => IDimensionParams;

  /** @internal Set parameters. */
  setDimensionParams: (theObjId: ObjectId, theParams: IDimensionParams) => void;

  /** @internal Return text position. */
  getDimensionTextPosition: (theObjId: ObjectId, theIsCustomOnly: boolean) => Vec3;

  /** @internal Set text position. */
  setDimensionTextPosition: (theObjId: ObjectId, thePnt: Vec3) => void;
};
