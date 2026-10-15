import { Vec3 } from './CommonTools.js';
import { InteractiveClipping } from './InteractiveClipping.js';
const STORE_VERSION = 1;
export class SavedViewStore {
    constructor(theViewer) {
        this._views = [];
        this._viewer = theViewer;
    }
    get views() { return this._views; }
    capture(theName = '') {
        const camera = this._viewer.camera;
        const ctx = this._viewer.interactiveContext;
        if (!camera || !ctx) {
            return null;
        }
        const hiddenIds = [];
        for (const obj of ctx.objectsInside) {
            if (!obj.isVisible) {
                hiddenIds.push(obj.id);
            }
        }
        const clipping = this.captureClipping();
        const item = {
            id: this.generateId(),
            name: theName || `View ${this._views.length + 1}`,
            camera: camera.viewPoint,
            projection: {
                isPerspective: camera.isPerspectiveProjection,
                fovy: camera.perspectiveProjectionFOVy
            },
            hiddenObjectIds: hiddenIds,
            clipping,
            createdAt: new Date().toISOString()
        };
        this._views.push(item);
        return item;
    }
    rename(theId, theName) {
        const item = this._views.find((v) => v.id === theId);
        if (!item) {
            return false;
        }
        item.name = theName;
        return true;
    }
    remove(theId) {
        const index = this._views.findIndex((v) => v.id === theId);
        if (index < 0) {
            return false;
        }
        this._views.splice(index, 1);
        return true;
    }
    clear() {
        this._views = [];
    }
    apply(theId) {
        const item = this._views.find((v) => v.id === theId);
        if (!item) {
            return { missingIds: [] };
        }
        return this.applyItem(item);
    }
    exportJson() {
        const payload = {
            version: STORE_VERSION,
            views: this._views.map((view) => ({
                id: view.id,
                name: view.name,
                camera: view.camera,
                projection: view.projection,
                hiddenObjectIds: view.hiddenObjectIds,
                clipping: view.clipping,
                createdAt: view.createdAt
            }))
        };
        return JSON.stringify(payload, null, 2);
    }
    importJson(theJson) {
        let parsed = null;
        try {
            parsed = JSON.parse(theJson);
        }
        catch (_a) {
            return this._views;
        }
        if (!parsed || !Array.isArray(parsed.views)) {
            return this._views;
        }
        this._views = parsed.views.map((view) => {
            var _a, _b, _c, _d, _e, _f, _g, _h;
            return ({
                id: view.id || this.generateId(),
                name: view.name || 'View',
                camera: view.camera || ((_a = this._viewer.camera) === null || _a === void 0 ? void 0 : _a.viewPoint),
                projection: {
                    isPerspective: (_c = (_b = view.projection) === null || _b === void 0 ? void 0 : _b.isPerspective) !== null && _c !== void 0 ? _c : true,
                    fovy: (_e = (_d = view.projection) === null || _d === void 0 ? void 0 : _d.fovy) !== null && _e !== void 0 ? _e : 35
                },
                hiddenObjectIds: (_f = view.hiddenObjectIds) !== null && _f !== void 0 ? _f : [],
                clipping: (_g = view.clipping) !== null && _g !== void 0 ? _g : null,
                createdAt: (_h = view.createdAt) !== null && _h !== void 0 ? _h : new Date().toISOString()
            });
        }).filter((view) => !!view.camera);
        return this._views;
    }
    applyItem(theItem) {
        const camera = this._viewer.camera;
        const ctx = this._viewer.interactiveContext;
        if (camera) {
            camera.viewPoint = theItem.camera;
            camera.isPerspectiveProjection = theItem.projection.isPerspective;
            camera.perspectiveProjectionFOVy = theItem.projection.fovy;
        }
        const missingIds = [];
        if (ctx) {
            for (const obj of ctx.objectsInside) {
                obj.isVisible = true;
            }
            for (const id of theItem.hiddenObjectIds) {
                const obj = ctx.findInteractiveObject(id);
                if (obj) {
                    obj.isVisible = false;
                }
                else {
                    missingIds.push(id);
                }
            }
        }
        this.applyClipping(theItem.clipping);
        this._viewer.requestViewUpdate();
        return { missingIds };
    }
    captureClipping() {
        const clip = this.findClipping();
        if (!clip) {
            return null;
        }
        return {
            isClippingOn: clip.isClippingOn,
            isCappingOn: clip.isCappingOn,
            toFlipClipping: clip.toFlipClipping,
            toUseObjectMaterial: clip.toUseObjectMaterial,
            isShaded: clip.isShaded,
            relativeCorner: clip.relativeClippingCorner,
            relativeExtent: clip.relativeClippingExtent,
            nbPlanesXYZ: clip.nbPlanesXYZ
        };
    }
    applyClipping(theState) {
        var _a, _b, _c, _d;
        const clip = (_a = this.findClipping()) !== null && _a !== void 0 ? _a : (theState ? new InteractiveClipping(this._viewer, this._viewer.focusView) : null);
        if (!clip) {
            return;
        }
        if (!theState) {
            clip.isClippingOn = false;
            return;
        }
        clip.relativeClippingCorner = (_b = theState.relativeCorner) !== null && _b !== void 0 ? _b : Vec3.zero();
        clip.relativeClippingExtent = (_c = theState.relativeExtent) !== null && _c !== void 0 ? _c : Vec3.zero();
        clip.nbPlanesXYZ = (_d = theState.nbPlanesXYZ) !== null && _d !== void 0 ? _d : Vec3.zero();
        clip.isClippingOn = theState.isClippingOn;
        clip.isCappingOn = theState.isCappingOn;
        clip.toFlipClipping = theState.toFlipClipping;
        clip.toUseObjectMaterial = theState.toUseObjectMaterial;
        clip.isShaded = theState.isShaded;
    }
    findClipping() {
        const ctx = this._viewer.interactiveContext;
        if (!ctx) {
            return null;
        }
        for (const obj of ctx.objectsInside) {
            if (obj instanceof InteractiveClipping) {
                return obj;
            }
        }
        return null;
    }
    generateId() {
        return `view-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
    }
}
//# sourceMappingURL=SavedViewStore.js.map