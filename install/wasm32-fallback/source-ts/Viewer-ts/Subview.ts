import type {IVec2, ViewId} from './CommonTools.js';
import {ViewCorner} from './CommonTools.js';
import {Camera} from './Camera.js';
import type {ViewManager} from './ViewManager.js';

import type {SCwebviewer} from './SCwebviewer.js';

import type {UserEvents} from './ViewerEventManager.js';

/**
 * Subview definition - a region in canvas for displaying 3D viewer content.
 *
 * Application may subscribe to the following object's events:
 * - {@link UserEvents.onSubviewFullscreen};
 * - {@link UserEvents.onSubviewClick}.
 */
export class Subview extends EventTarget {
  private _myViewId: ViewId;
  private _myViewMgr: ViewManager | null;
  private _myCamera: Camera | null;

  /** Empty constructor. */
  constructor(theViewId: ViewId = -2) {
    super();
    this._myViewId = theViewId;
    this._myViewMgr = null;
    this._myCamera = null;
  }

  /** Get view id as {@link ViewId}; -2 is invalid id, -1 refers to a view receiving input focus. */
  public get id(): ViewId { return this._myViewId; }

  /** Return TRUE if view with specified Id exists. */
  public get isDefined(): boolean { return this._myViewMgr?.isDefined(this._myViewId) ?? false; }

  /** Return TRUE if view is active (displayed on the screen). */
  public get isActive(): boolean { return this._myViewMgr?.isActive(this._myViewId) ?? false; }

  /** Return camera controller. */
  public get camera(): Camera | null { return this._myCamera; }

  /** Return TRUE if view receives input focus. */
  public get hasInputFocus(): boolean { return this._myViewMgr?.hasInputFocus(this._myViewId) ?? false; }

  /** Return TRUE if view serves as a composer to subviews. */
  public get isSubviewComposer(): boolean { return this._myViewMgr?.isSubviewComposer(this._myViewId) ?? false; }

  /** Return subview size as a fraction to canvas dimensions. */
  public get subviewSize(): IVec2 { return this._myViewMgr?.getSubviewSize(this._myViewId) ?? {x: 0, y: 0}; }

  /** Return subview offset from the corner as a fraction to canvas dimensions. */
  public get subviewOffset(): IVec2 { return this._myViewMgr?.getSubviewOffset(this._myViewId) ?? {x: 0, y: 0}; }

  /** Return subview margins in pixels. */
  public get subviewMargins(): IVec2 { return this._myViewMgr?.getSubviewMargins(this._myViewId) ?? {x: 0, y: 0}; }

  /** Return subview corner. */
  public get subviewCorner(): ViewCorner { return this._myViewMgr?.getSubviewCorner(this._myViewId) ?? ViewCorner.FREE3D; }

  /** Return TRUE if subview should fill entire canvas (replaces all subviews). */
  public get isSubviewFullscreen(): boolean { return this._myViewMgr?.isSubviewFullscreen(this._myViewId) ?? false; }

  /** Return TRUE if subview is clickable. */
  public get isSubviewClickable(): boolean { return this._myViewMgr?.isSubviewClickable(this._myViewId) ?? false; }

  /** Set view active state. */
  public set isActive(theActive: boolean) { this._myViewMgr?.setActive(this._myViewId, theActive); }

  /** Set view composer state. */
  public set isSubviewComposer(theComposer: boolean) { this._myViewMgr?.setSubviewComposer(this._myViewId, theComposer); }

  /** Set subview size. */
  public set subviewSize(theSize: IVec2) { this._myViewMgr?.setSubviewSize(this._myViewId, theSize); }

  /** Set subview offset. */
  public set subviewOffset(theOffset: IVec2) { this._myViewMgr?.setSubviewOffset(this._myViewId, theOffset); }

  /** Set subview margins in pixels. */
  public set subviewMargins(theMargins: IVec2) { this._myViewMgr?.setSubviewMargins(this._myViewId, theMargins); }

  /** Set subview corner. */
  public set subviewCorner(theCorner: ViewCorner) { this._myViewMgr?.setSubviewCorner(this._myViewId, theCorner); }

  /** Set if subview should fill entire canvas (replace all subviews) or not. */
  public set isSubviewFullscreen(theToEnable: boolean) { this._myViewMgr?.setSubviewFullscreen(this._myViewId, theToEnable); }

  /** Set if subview should be clickable. */
  public set isSubviewClickable(theToEnable: boolean) { this._myViewMgr?.setSubviewClickable(this._myViewId, theToEnable); }

  /**
   * Create new subview.
   * @param {SCwebviewer} theViewer viewer module
   * @param {IVec2} theSize the subview size
   */
  public create(theViewer: SCwebviewer, theSize: IVec2): void {
    this._myViewMgr = theViewer.viewManager;
    this._myViewId = this._myViewMgr?.createSubview(theSize) ?? -2;
    this._myViewMgr?.addSubviewObject(this);
    this._myCamera = new Camera(theViewer, this._myViewId);
  }

  /** Remove subview. */
  public remove(): void {
    if (this._myViewMgr != null && this._myViewId !== -2) {
      this._myViewMgr.removeSubview(this._myViewId);
      this._myViewId = -2;
      this._myCamera = null;
    }
  }

  /**
   * Wrap existing view.
   * @param {SCwebviewer} theViewer viewer module
   * @param {ViewId} theId the id to wrap
   * @return {boolean} FALSE if view with specified Id doesn't exist in the Viewer
   */
  public wrap(theViewer: SCwebviewer, theId: ViewId = -2): boolean {
    if (theId !== -2) { this._myViewId = theId; }
    this._myViewMgr = theViewer.viewManager;
    this._myCamera = new Camera(theViewer, this._myViewId);
    return this._myViewMgr?.isDefined(this._myViewId) ?? false;
  }

  /** Set this view to receive input focus. */
  public setInputFocus(): void { this._myViewMgr?.setInputFocus(this._myViewId); }
};

/**
 * Auxiliary tool to initialize JS extensions to ViewManager C++ class.
 * @internal
 */
export class ViewManagerExtensions {
   public static init(theViewer: SCwebviewer, theViewMgr: ViewManager): void {
    let aSubviewArray: Subview[] = [];
    Object.defineProperty(theViewMgr, 'subviews', {
      get: function (): Subview[] { return aSubviewArray; },
    });

    theViewMgr.addSubviewObject = function (theSubview: Subview): void {
      aSubviewArray = aSubviewArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theSubview.id); });
      aSubviewArray.push(theSubview);
    };

    theViewMgr.findSubviewObject = function (theViewId: ViewId): Subview | null {
      let anObj = aSubviewArray.find((theElem, theIndex, theArray) => { return (theElem.id === theViewId); });
      return anObj ? anObj : null;
    };

    let aCppRemoveSubview = theViewMgr.removeSubview.bind(theViewMgr);
    theViewMgr.removeSubview = function (theViewId: ViewId): void {
      aSubviewArray = aSubviewArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theViewId); });
      aCppRemoveSubview(theViewId);
    };

    let aCppRemoveAllSubviews = theViewMgr.removeAllSubviews.bind(theViewMgr);
    theViewMgr.removeAllSubviews = function (): void {
      let aSubviewToRemove: Subview[] = aSubviewArray.slice();
      aSubviewToRemove.reverse();
      aSubviewArray = [];
      aSubviewToRemove.forEach((theSubview: Subview) => {
        theSubview.remove(); // invalidate objects
      });
      aCppRemoveAllSubviews(); // should have no effect
    };
  }
};
