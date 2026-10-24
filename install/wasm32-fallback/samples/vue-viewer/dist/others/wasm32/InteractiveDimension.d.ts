import type { ObjectId, IVec3, IAx2 } from './CommonTools.js';
import { Ax2 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
}
/**
 * Describes dimension parameters object.
 */
export declare class DimensionParams implements IDimensionParams {
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
}
/**
 * Represents interactive dimension object.
 */
export declare class InteractiveDimension extends InteractiveObject {
    /** Empty constructor. */
    constructor(theObjId?: ObjectId);
    /** Return dimension plane. */
    get plane(): Ax2;
    /** Return computed real value. */
    get realValue(): number;
    /** Return custom text value, or empty string. */
    get textValue(): string;
    /** Return special symbol. */
    get specialSymbol(): string;
    /** Return special symbol position. */
    get isSpecialSymbolAfter(): boolean;
    /** Return flag if custom text position is manually set or automatically computed. */
    get isCustomTextPosition(): boolean;
    /** Return text position. */
    get textPosition(): IVec3;
    /** Set custom text value, or unset it by empty string. */
    set textValue(theText: string);
    /** Set special symbol. */
    set specialSymbol(theSymb: string);
    /** Return special symbol position. */
    set isSpecialSymbolAfter(theIsAfter: boolean);
    /** Set flag if custom text position is manually set or automatically computed. */
    set isCustomTextPosition(theIsCustom: boolean);
    /** Set fixed text position. WARNING! Result might be adjusted. */
    set textPosition(thePnt: IVec3 | null);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveDimension | null;
    /**
     * Build length dimension between two given points.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 first point
     * @param {IVec3} thePnt2 second point
     * @param {IAx2}  thePln  plane
     */
    buildLength(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3, thePln?: IAx2): boolean;
    /**
     * Build radius dimension.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} theAx2    circle location
     * @param {IVec3} theRadius radius
     * @param {IAx2}  theAnchor optional anchor point on the circle
     */
    buildRadius(theViewer: SCwebviewer, theAx2: IAx2, theRadius: number, theAnchor?: IVec3): boolean;
    /**
     * Build angle dimension.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec3} thePnt1 first point
     * @param {IVec3} thePnt2 second point (corner)
     * @param {IVec3} thePnt3 third point
     */
    buildAngle(theViewer: SCwebviewer, thePnt1: IVec3, thePnt2: IVec3, thePnt3: IVec3): boolean;
}
