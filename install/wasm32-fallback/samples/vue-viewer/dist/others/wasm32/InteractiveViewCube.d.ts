import type { ObjectId } from './CommonTools.js';
import { CameraOrientation } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Represents ViewCube property manager.
 * ViewCube is an auxiliary interactive object displayed at View corner and showing current View orientation.
 * See also {@link SCwebviewer.viewCube} property.
 */
export declare class InteractiveViewCube extends InteractiveObject {
    /** Empty constructor. */
    constructor(theViewer: SCwebviewer | null, theObjId?: ObjectId);
    /** Return view cube base size. */
    get baseSize(): number;
    /** Return view cube side label. */
    get boxSideLabelFront(): string;
    /** Return view cube side label. */
    get boxSideLabelBack(): string;
    /** Return view cube side label. */
    get boxSideLabelLeft(): string;
    /** Return view cube side label. */
    get boxSideLabelRight(): string;
    /** Return view cube side label. */
    get boxSideLabelTop(): string;
    /** Return view cube side label. */
    get boxSideLabelBottom(): string;
    /** Set view cube base size. */
    set baseSize(theSize: number);
    /** Set view cube side label. */
    set boxSideLabelFront(theLabel: string);
    /** Set view cube side label. */
    set boxSideLabelBack(theLabel: string);
    /** Set view cube side label. */
    set boxSideLabelLeft(theLabel: string);
    /** Set view cube side label. */
    set boxSideLabelRight(theLabel: string);
    /** Set view cube side label. */
    set boxSideLabelTop(theLabel: string);
    /** Set view cube side label. */
    set boxSideLabelBottom(theLabel: string);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveViewCube | null;
    /** Return view cube side label. */
    getBoxSideLabel(theSide: CameraOrientation): string;
    /** Set view cube side label. */
    setBoxSideLabel(theSide: CameraOrientation, theLabel: string): void;
}
