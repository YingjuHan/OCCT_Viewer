import type { IVec3, ObjectId } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
}
/**
 * Parameters object for an angular arrow of the coordinate system.
 */
export declare class AngularAxesParams implements IAngularAxesParams {
    /** Angular axis ID is empty by default. */
    axisId: string;
    /** Orientation of the angular arrow defined as rotation axis with a sign, e.g. +z or -x. */
    orientation: string;
    /** The attachment axis (zero axis) of the angular arrow. */
    zeroAxis: string;
    /** The text label of the angular arrow. */
    label: string;
}
/**
 * Represents spherical coordinate axes interactive object.
 */
export declare class InteractiveSphericalAxes extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theObjId?: ObjectId);
    /** Returns axes position. */
    get position(): IVec3;
    /** Returns axes scale. */
    get scale(): number;
    /** Returns the orientation of longitude/latitude circles. */
    get circlesOrientation(): string;
    /** Returns the parameters of two angular axes. */
    get angularAxesParams(): AngularAxesParams[];
    /** Sets axes position. */
    set position(thePos: IVec3);
    /** Sets axes scale. */
    set scale(theScale: number);
    /** Sets the orientation of longitude/latitude circles. */
    set circlesOrientation(theAxis: string);
    /** Sets the parameters of two angular axes. */
    set angularAxesParams(theAxisParams: AngularAxesParams[]);
    /** Make a copy of existing object with new id. */
    clone(theObjId?: ObjectId): InteractiveSphericalAxes | null;
}
