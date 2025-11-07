import type { ObjectId, IVec2 } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Represents Scale Ruler property manager.
 * Scale Ruler is an auxiliary interactive object displayed length measure units at view corner.
 * See also {@link SCwebviewer.scaleRuler} property.
 */
export declare class InteractiveScaleRuler extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theObjId?: ObjectId);
    /** Return scale ruler base size. */
    get baseSize(): IVec2;
    /** Set scale ruler base size. */
    set baseSize(theSize: IVec2);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveScaleRuler | null;
}
