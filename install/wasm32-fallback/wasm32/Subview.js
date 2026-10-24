import { ViewCorner } from './CommonTools.js';
import { Camera } from './Camera.js';
/**
 * Subview definition - a region in canvas for displaying 3D viewer content.
 *
 * Application may subscribe to the following object's events:
 * - {@link UserEvents.onSubviewFullscreen};
 * - {@link UserEvents.onSubviewClick}.
 */
export class Subview extends EventTarget {
    /** Empty constructor. */
    constructor(theViewId = -2) {
        super();
        this._myViewId = theViewId;
        this._myViewMgr = null;
        this._myCamera = null;
    }
    /** Get view id as {@link ViewId}; -2 is invalid id, -1 refers to a view receiving input focus. */
    get id() { return this._myViewId; }
    /** Return TRUE if view with specified Id exists. */
    get isDefined() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isDefined(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Return TRUE if view is active (displayed on the screen). */
    get isActive() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isActive(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Return camera controller. */
    get camera() { return this._myCamera; }
    /** Return TRUE if view receives input focus. */
    get hasInputFocus() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.hasInputFocus(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Return TRUE if view serves as a composer to subviews. */
    get isSubviewComposer() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isSubviewComposer(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Return subview size as a fraction to canvas dimensions. */
    get subviewSize() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.getSubviewSize(this._myViewId)) !== null && _b !== void 0 ? _b : { x: 0, y: 0 }; }
    /** Return subview offset from the corner as a fraction to canvas dimensions. */
    get subviewOffset() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.getSubviewOffset(this._myViewId)) !== null && _b !== void 0 ? _b : { x: 0, y: 0 }; }
    /** Return subview margins in pixels. */
    get subviewMargins() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.getSubviewMargins(this._myViewId)) !== null && _b !== void 0 ? _b : { x: 0, y: 0 }; }
    /** Return subview corner. */
    get subviewCorner() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.getSubviewCorner(this._myViewId)) !== null && _b !== void 0 ? _b : ViewCorner.FREE3D; }
    /** Return TRUE if subview should fill entire canvas (replaces all subviews). */
    get isSubviewFullscreen() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isSubviewFullscreen(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Return TRUE if subview is clickable. */
    get isSubviewClickable() { var _a, _b; return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isSubviewClickable(this._myViewId)) !== null && _b !== void 0 ? _b : false; }
    /** Set view active state. */
    set isActive(theActive) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setActive(this._myViewId, theActive); }
    /** Set view composer state. */
    set isSubviewComposer(theComposer) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewComposer(this._myViewId, theComposer); }
    /** Set subview size. */
    set subviewSize(theSize) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewSize(this._myViewId, theSize); }
    /** Set subview offset. */
    set subviewOffset(theOffset) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewOffset(this._myViewId, theOffset); }
    /** Set subview margins in pixels. */
    set subviewMargins(theMargins) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewMargins(this._myViewId, theMargins); }
    /** Set subview corner. */
    set subviewCorner(theCorner) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewCorner(this._myViewId, theCorner); }
    /** Set if subview should fill entire canvas (replace all subviews) or not. */
    set isSubviewFullscreen(theToEnable) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewFullscreen(this._myViewId, theToEnable); }
    /** Set if subview should be clickable. */
    set isSubviewClickable(theToEnable) { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setSubviewClickable(this._myViewId, theToEnable); }
    /**
     * Create new subview.
     * @param {SCwebviewer} theViewer viewer module
     * @param {IVec2} theSize the subview size
     */
    create(theViewer, theSize) {
        var _a, _b, _c;
        this._myViewMgr = theViewer.viewManager;
        this._myViewId = (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.createSubview(theSize)) !== null && _b !== void 0 ? _b : -2;
        (_c = this._myViewMgr) === null || _c === void 0 ? void 0 : _c.addSubviewObject(this);
        this._myCamera = new Camera(theViewer, this._myViewId);
    }
    /** Remove subview. */
    remove() {
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
    wrap(theViewer, theId = -2) {
        var _a, _b;
        if (theId !== -2) {
            this._myViewId = theId;
        }
        this._myViewMgr = theViewer.viewManager;
        this._myCamera = new Camera(theViewer, this._myViewId);
        return (_b = (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.isDefined(this._myViewId)) !== null && _b !== void 0 ? _b : false;
    }
    /** Set this view to receive input focus. */
    setInputFocus() { var _a; (_a = this._myViewMgr) === null || _a === void 0 ? void 0 : _a.setInputFocus(this._myViewId); }
}
;
/**
 * Auxiliary tool to initialize JS extensions to ViewManager C++ class.
 * @internal
 */
export class ViewManagerExtensions {
    static init(theViewer, theViewMgr) {
        let aSubviewArray = [];
        Object.defineProperty(theViewMgr, 'subviews', {
            get: function () { return aSubviewArray; },
        });
        theViewMgr.addSubviewObject = function (theSubview) {
            aSubviewArray = aSubviewArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theSubview.id); });
            aSubviewArray.push(theSubview);
        };
        theViewMgr.findSubviewObject = function (theViewId) {
            let anObj = aSubviewArray.find((theElem, theIndex, theArray) => { return (theElem.id === theViewId); });
            return anObj ? anObj : null;
        };
        let aCppRemoveSubview = theViewMgr.removeSubview.bind(theViewMgr);
        theViewMgr.removeSubview = function (theViewId) {
            aSubviewArray = aSubviewArray.filter((theElem, theIndex, theArray) => { return (theElem.id !== theViewId); });
            aCppRemoveSubview(theViewId);
        };
        let aCppRemoveAllSubviews = theViewMgr.removeAllSubviews.bind(theViewMgr);
        theViewMgr.removeAllSubviews = function () {
            let aSubviewToRemove = aSubviewArray.slice();
            aSubviewToRemove.reverse();
            aSubviewArray = [];
            aSubviewToRemove.forEach((theSubview) => {
                theSubview.remove(); // invalidate objects
            });
            aCppRemoveAllSubviews(); // should have no effect
        };
    }
}
;
//# sourceMappingURL=Subview.js.map