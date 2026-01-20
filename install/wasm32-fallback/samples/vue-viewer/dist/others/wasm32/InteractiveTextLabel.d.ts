import type { ObjectId, IVec3 } from './CommonTools.js';
import { InteractiveObject, TypeOfMarker } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Represents Text Label interactive object.
 */
export declare class InteractiveTextLabel extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theObjId?: ObjectId);
    /** Text to be displayed. */
    get text(): string;
    /** Icon to be displayed. */
    get icon(): string;
    /** 3D point to display label at. */
    get position(): IVec3;
    /** Return TRUE if 3D attachment point is defined. */
    get hasAttachPoint(): boolean;
    /** 3D point to attach label. */
    get attachPoint(): IVec3;
    /** 2d offset from 3D point to text label. */
    get pointPadding(): number;
    /** 2d padding for rectangle around text label. */
    get rectanglePadding(): number;
    /** Flag to draw rectangle around text. */
    get toDrawRectangle(): boolean;
    /** Flag to draw 3D attach point as a marker. */
    get attachMarker(): TypeOfMarker;
    /** 3D attach point marker scale. */
    get attachMarkerScale(): number;
    /** Text to be displayed. */
    set text(theText: string);
    /** Icon to be displayed. */
    set icon(theIcon: string);
    /** 3D point to display label at. */
    set position(thePnt: IVec3);
    /** Set if 3D attachment point is defined or not. */
    set hasAttachPoint(theToDefine: boolean);
    /** 3D point to attach label. */
    set attachPoint(thePnt: IVec3);
    /** 2d offset from 3D point to text label. */
    set pointPadding(thePad: number);
    /** 2d padding for rectangle around text label. */
    set rectanglePadding(thePad: number);
    /** Flag to draw rectangle around text. */
    set toDrawRectangle(theToDraw: boolean);
    /** Flag to draw 3D attach point as a marker. */
    set attachMarker(theMarker: TypeOfMarker);
    /** 3D attach point marker scale. */
    set attachMarkerScale(theScale: number);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveTextLabel | null;
}
