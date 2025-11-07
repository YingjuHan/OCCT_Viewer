import type { IVec2, ViewId } from './CommonTools.js';
import { ViewCorner } from './CommonTools.js';
import { Camera } from './Camera.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Subview definition - a region in canvas for displaying 3D viewer content.
 *
 * Application may subscribe to the following object's events:
 * - {@link UserEvents.onSubviewFullscreen};
 * - {@link UserEvents.onSubviewClick}.
 */
export declare class Subview extends EventTarget {
    private _myViewId;
    private _myViewMgr;
    private _myCamera;
    /** Empty constructor. */
    constructor(theViewId?: ViewId);
    /** Get view id as {@link ViewId}; -2 is invalid id, -1 refers to a view receiving input focus. */
    get id(): ViewId;
    /** Return TRUE if view with specified Id exists. */
    get isDefined(): boolean;
    /** Return TRUE if view is active (displayed on the screen). */
    get isActive(): boolean;
    /** Return camera controller. */
    get camera(): Camera | null;
    /** Return TRUE if view receives input focus. */
    get hasInputFocus(): boolean;
    /** Return TRUE if view serves as a composer to subviews. */
    get isSubviewComposer(): boolean;
    /** Return subview size as a fraction to canvas dimensions. */
    get subviewSize(): IVec2;
    /** Return subview offset from the corner as a fraction to canvas dimensions. */
    get subviewOffset(): IVec2;
    /** Return subview margins in pixels. */
    get subviewMargins(): IVec2;
    /** Return subview corner. */
    get subviewCorner(): ViewCorner;
    /** Return TRUE if subview should fill entire canvas (replaces all subviews). */
    get isSubviewFullscreen(): boolean;
    /** Return TRUE if subview is clickable. */
    get isSubviewClickable(): boolean;
    /** Set view active state. */
    set isActive(theActive: boolean);
    /** Set view composer state. */
    set isSubviewComposer(theComposer: boolean);
    /** Set subview size. */
    set subviewSize(theSize: IVec2);
    /** Set subview offset. */
    set subviewOffset(theOffset: IVec2);
    /** Set subview margins in pixels. */
    set subviewMargins(theMargins: IVec2);
    /** Set subview corner. */
    set subviewCorner(theCorner: ViewCorner);
    /** Set if subview should fill entire canvas (replace all subviews) or not. */
    set isSubviewFullscreen(theToEnable: boolean);
    /** Set if subview should be clickable. */
    set isSubviewClickable(theToEnable: boolean);
    /**
     * Create new subview.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec2} theSize the subview size
     */
    create(theViewer: SCwebviewer, theSize: IVec2): void;
    /** Remove subview. */
    remove(): void;
    /**
     * Wrap existing view.
     * @param {SCwebviewer} theViewer viewer module
     * @param {ViewId} theId the id to wrap
     * @return {boolean} FALSE if view with specified Id doesn't exist in the Viewer
     */
    wrap(theViewer: SCwebviewer, theId?: ViewId): boolean;
    /** Set this view to receive input focus. */
    setInputFocus(): void;
}
