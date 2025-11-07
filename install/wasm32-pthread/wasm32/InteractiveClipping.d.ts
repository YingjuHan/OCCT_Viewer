import type { ObjectId, IVec3, IBndBox3 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { Subview } from './Subview.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
export declare class InteractiveClipping extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theView: Subview | null, theObjId?: ObjectId);
    /** Return clipping flag; FALSE by default. */
    get isClippingOn(): boolean;
    /** Return capping flag; FALSE by default. */
    get isCappingOn(): boolean;
    /** Return TRUE for using object's color for capping. */
    get toUseObjectMaterial(): boolean;
    /** Return shaded presentation flag; TRUE by default. */
    get isShaded(): boolean;
    /** Return clipping box flipping flag; FALSE by default. */
    get toFlipClipping(): boolean;
    /** Return clipping box configuration - main corner point (origin). */
    get clippingCorner(): IVec3;
    /** Return clipping box configuration - main corner point (origin), relative to scene box. */
    get relativeClippingCorner(): IVec3;
    /**
     * Return clipping box configuration - box extent.
     * - zero extent disables clipping within specific dimension;
     * - finite extent defines finite clipping range from corner point (origin),
     *   e.g. a pair of clipping planes with opposite directions clipping box interior;
     * - infinite extent defines half-space clipping, e.g. a single clipping plane;
     * - negative extent specifies clipping side relative to corner point.
     */
    get clippingExtent(): IVec3;
    /** Return clipping box configuration - box extent, relative to scene box extent. */
    get relativeClippingExtent(): IVec3;
    /**
     * Return clipping box configuration - number of clipping planes per dimension (0, -1, 1, or -2, 2),
     * where negative number means negative extent
     * (e.g. -1 means clipping in opposite direction,
     * -2 means building a box in opposite direction from main corner point).
     */
    get nbPlanesXYZ(): IVec3;
    /** Return clipping box configuration. */
    get clippingBox(): IBndBox3;
    /** Return clipping box configuration, relative to scene box. */
    get relativeClippingBox(): IBndBox3;
    /** Set clipping flag. */
    set isClippingOn(theToCap: boolean);
    /** Set capping flag. */
    set isCappingOn(theToCap: boolean);
    /** Set flag for using object's color for capping. */
    set toUseObjectMaterial(theToUse: boolean);
    /** Set shaded presentation flag. */
    set isShaded(theIsShaded: boolean);
    /** Set clipping box flipping flag. */
    set toFlipClipping(theToFlip: boolean);
    /** Set clipping box configuration - main corner point (origin). */
    set clippingCorner(thePnt: IVec3);
    /** Set clipping box configuration - main corner point (origin), relative to scene box. */
    set relativeClippingCorner(thePnt: IVec3);
    /** Set clipping box configuration - box extent. */
    set clippingExtent(theExt: IVec3);
    /** Set clipping box configuration - box extent, relative to scene box extent. */
    set relativeClippingExtent(theExt: IVec3);
    /** Set clipping box configuration - number of clipping planes per dimension. */
    set nbPlanesXYZ(theVec: IVec3);
    /** Set clipping box configuration. */
    set clippingBox(theBox: IBndBox3);
    /** Set clipping box configuration, relative to scene box. */
    set relativeClippingBox(theBox: IBndBox3);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveClipping | null;
}
