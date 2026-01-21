import { Ax2, Vec3 } from './CommonTools.js';
import { InteractiveDimension } from './InteractiveDimension.js';
import { InteractiveTextLabel } from './InteractiveTextLabel.js';
import { TypeOfMarker } from './InteractiveObject.js';
export class PmiStore {
    constructor(theViewer) {
        this._items = [];
        this._viewer = theViewer;
    }
    get items() { return this._items; }
    clear() {
        for (const item of this._items) {
            this.removeInteractiveObject(item.objectId);
        }
        this._items = [];
    }
    importJson(theJson) {
        let parsed = null;
        try {
            parsed = JSON.parse(theJson);
        }
        catch (_a) {
            return { items: this._items, invalidCount: 0 };
        }
        if (!parsed || !Array.isArray(parsed.items)) {
            return { items: this._items, invalidCount: 0 };
        }
        this.clear();
        let invalidCount = 0;
        for (const item of parsed.items) {
            const newItem = this.buildItem(item);
            if (!newItem.valid) {
                invalidCount += 1;
            }
        }
        return { items: this._items, invalidCount };
    }
    focus(theId) {
        const item = this._items.find((entry) => entry.id === theId);
        if (!item || !item.objectId) {
            return false;
        }
        const ctx = this._viewer.interactiveContext;
        const cam = this._viewer.camera;
        const sel = this._viewer.selectionManager;
        if (!ctx || !cam || !sel) {
            return false;
        }
        const obj = ctx.findInteractiveObject(item.objectId);
        if (!obj) {
            return false;
        }
        sel.resetSelection();
        obj.isSelected = true;
        cam.fitAllAuto(true, true);
        this._viewer.requestViewUpdate();
        return true;
    }
    buildItem(theItem) {
        var _a, _b, _c, _d;
        const id = ((theItem === null || theItem === void 0 ? void 0 : theItem.id) && theItem.id.length > 0) ? theItem.id : this.generateId();
        const type = (_a = theItem === null || theItem === void 0 ? void 0 : theItem.type) !== null && _a !== void 0 ? _a : 'Text';
        const text = (_b = theItem === null || theItem === void 0 ? void 0 : theItem.text) !== null && _b !== void 0 ? _b : '';
        const anchor = this.parseAnchor(theItem === null || theItem === void 0 ? void 0 : theItem.anchor);
        const viewIds = Array.isArray(theItem === null || theItem === void 0 ? void 0 : theItem.viewIds) ? (_c = theItem === null || theItem === void 0 ? void 0 : theItem.viewIds) !== null && _c !== void 0 ? _c : [] : [];
        let objectId = null;
        let kind = 'label';
        let valid = false;
        if (theItem === null || theItem === void 0 ? void 0 : theItem.dimension) {
            const dim = this.buildDimension(id, text, theItem.dimension);
            if (dim) {
                objectId = dim;
                kind = 'dimension';
                valid = true;
            }
        }
        if (!valid) {
            const labelId = this.buildLabel(id, text, (_d = anchor === null || anchor === void 0 ? void 0 : anchor.pos) !== null && _d !== void 0 ? _d : null);
            if (labelId) {
                objectId = labelId;
                kind = 'label';
                valid = true;
            }
        }
        const item = {
            id,
            type,
            text,
            anchor: anchor !== null && anchor !== void 0 ? anchor : undefined,
            viewIds,
            kind,
            objectId,
            valid,
        };
        this._items.push(item);
        return item;
    }
    buildLabel(theId, theText, thePos) {
        if (!thePos) {
            return null;
        }
        const objId = this.buildObjectId(theId, 'pmi-label');
        const label = new InteractiveTextLabel(this._viewer, objId);
        label.text = theText || theId;
        label.position = thePos;
        label.attachPoint = thePos;
        label.attachMarker = TypeOfMarker.O_PLUS;
        label.attachMarkerScale = 1.0;
        label.pointPadding = 12;
        label.rectanglePadding = 4;
        label.toDrawRectangle = true;
        label.isVisible = true;
        return label.id;
    }
    buildDimension(theId, theText, theDim) {
        if (!theDim || !theDim.kind) {
            return null;
        }
        const viewer = this._viewer;
        const objId = this.buildObjectId(theId, 'pmi-dim');
        const dim = new InteractiveDimension(objId);
        let isOk = false;
        if (theDim.kind === 'length') {
            const p1 = this.parseVec3(theDim.pnt1);
            const p2 = this.parseVec3(theDim.pnt2);
            if (p1 && p2) {
                isOk = dim.buildLength(viewer, p1, p2, Ax2.invalid());
            }
        }
        else if (theDim.kind === 'angle') {
            const p1 = this.parseVec3(theDim.pnt1);
            const p2 = this.parseVec3(theDim.pnt2);
            const p3 = this.parseVec3(theDim.pnt3);
            if (p1 && p2 && p3) {
                isOk = dim.buildAngle(viewer, p1, p2, p3);
            }
        }
        else if (theDim.kind === 'radius' || theDim.kind === 'diameter') {
            const ax2 = this.parseAx2(theDim.ax2);
            const radius = typeof theDim.radius === 'number' ? theDim.radius : null;
            const anchor = this.parseVec3(theDim.anchor);
            if (ax2 && radius != null) {
                isOk = dim.buildRadius(viewer, ax2, radius, anchor !== null && anchor !== void 0 ? anchor : Vec3.invalid());
                if (theDim.kind === 'diameter') {
                    dim.specialSymbol = 'D';
                    dim.isSpecialSymbolAfter = false;
                }
            }
        }
        if (!isOk) {
            return null;
        }
        if (theText) {
            dim.textValue = theText;
        }
        dim.isVisible = true;
        return dim.id;
    }
    parseAnchor(theAnchor) {
        if (!theAnchor) {
            return null;
        }
        const pos = this.parseVec3Array(theAnchor.pos);
        const anchor = {
            nodeId: theAnchor.nodeId,
            pos: pos !== null && pos !== void 0 ? pos : undefined,
        };
        return anchor;
    }
    parseAx2(theAx2) {
        if (!theAx2) {
            return null;
        }
        const loc = this.parseVec3(theAx2.location);
        const dir = this.parseVec3(theAx2.direction);
        const xDir = this.parseVec3(theAx2.xDirection);
        if (!loc || !dir || !xDir) {
            return null;
        }
        const ax2 = new Ax2();
        ax2.location = loc;
        ax2.direction = dir;
        ax2.xDirection = xDir;
        return ax2;
    }
    parseVec3(theObj) {
        if (!theObj || typeof theObj.x !== 'number' || typeof theObj.y !== 'number' || typeof theObj.z !== 'number') {
            return null;
        }
        return { x: theObj.x, y: theObj.y, z: theObj.z };
    }
    parseVec3Array(theArr) {
        if (!Array.isArray(theArr) || theArr.length !== 3) {
            return null;
        }
        const [x, y, z] = theArr;
        if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') {
            return null;
        }
        return { x, y, z };
    }
    buildObjectId(theId, theSuffix) {
        return `${theId}::${theSuffix}`;
    }
    removeInteractiveObject(theId) {
        if (!theId) {
            return;
        }
        const ctx = this._viewer.interactiveContext;
        if (!ctx) {
            return;
        }
        const obj = ctx.findInteractiveObject(theId);
        if (obj) {
            obj.remove();
            this._viewer.requestViewUpdate();
        }
    }
    generateId() {
        return `pmi-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
    }
}
//# sourceMappingURL=PmiStore.js.map