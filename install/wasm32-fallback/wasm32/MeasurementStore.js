import { Ax2, Vec3 } from './CommonTools.js';
import { InteractiveDimension } from './InteractiveDimension.js';
const STORE_VERSION = 1;
export class MeasurementStore {
    constructor(theViewer) {
        this._items = [];
        this._viewer = theViewer;
    }
    get items() { return this._items; }
    addDistance(theA, theB, theId) {
        const aMgr = this._viewer.interactiveShapeManager;
        if (!aMgr) {
            return null;
        }
        const aResStr = aMgr.measureDistanceJson(theA.objId, theA.selMode, theA.entityId, theB.objId, theB.selMode, theB.entityId);
        const aRes = this.parseDistanceJson(aResStr);
        if (!aRes || aRes.distance == null || !aRes.pnt1 || !aRes.pnt2) {
            return null;
        }
        const aDim = new InteractiveDimension();
        const isOk = aDim.buildLength(this._viewer, aRes.pnt1, aRes.pnt2);
        if (!isOk) {
            return null;
        }
        aDim.isVisible = true;
        const aValueText = this.formatLength(aRes.distance);
        aDim.textValue = aValueText;
        const aItem = {
            id: this.ensureId(theId),
            kind: 'distance',
            label: 'Distance',
            value: aRes.distance,
            valueText: aValueText,
            dimensionId: aDim.id,
            objectIds: [theA.objId, theB.objId],
            inputs: { a: theA, b: theB },
            valid: true,
            createdAt: new Date().toISOString()
        };
        this._items.push(aItem);
        return aItem;
    }
    addRadius(theTarget, theId) {
        return this.addCircleMeasurement('radius', theTarget, theId);
    }
    addDiameter(theTarget, theId) {
        return this.addCircleMeasurement('diameter', theTarget, theId);
    }
    remove(theId) {
        const aIndex = this._items.findIndex((item) => item.id === theId);
        if (aIndex < 0) {
            return false;
        }
        const [aItem] = this._items.splice(aIndex, 1);
        this.removeDimension(aItem.dimensionId);
        return true;
    }
    clear() {
        for (const item of this._items) {
            this.removeDimension(item.dimensionId);
        }
        this._items = [];
    }
    exportJson() {
        const aPayload = {
            version: STORE_VERSION,
            items: this._items.map((item) => {
                var _a;
                return ({
                    id: item.id,
                    kind: item.kind,
                    label: item.label,
                    value: (_a = item.value) !== null && _a !== void 0 ? _a : undefined,
                    valueText: item.valueText || undefined,
                    createdAt: item.createdAt,
                    inputs: item.inputs
                });
            })
        };
        return JSON.stringify(aPayload, null, 2);
    }
    importJson(theJson) {
        let aParsed = null;
        try {
            aParsed = JSON.parse(theJson);
        }
        catch (_a) {
            return { items: this._items, invalidCount: 0 };
        }
        if (!aParsed || !Array.isArray(aParsed.items)) {
            return { items: this._items, invalidCount: 0 };
        }
        this.clear();
        let invalidCount = 0;
        for (const item of aParsed.items) {
            const aNew = this.restoreItem(item);
            if (!aNew.valid) {
                invalidCount += 1;
            }
        }
        return { items: this._items, invalidCount };
    }
    focus(theId) {
        const aItem = this._items.find((item) => item.id === theId);
        if (!aItem) {
            return false;
        }
        return this.focusItem(aItem);
    }
    addCircleMeasurement(theKind, theTarget, theId) {
        var _a;
        const aMgr = this._viewer.interactiveShapeManager;
        if (!aMgr) {
            return null;
        }
        const aResStr = aMgr.getCircleParamsJson(theTarget.objId, theTarget.selMode, theTarget.entityId);
        const aRes = this.parseCircleJson(aResStr);
        if (!aRes || !aRes.ax2 || typeof aRes.radius !== 'number') {
            return null;
        }
        const aDim = new InteractiveDimension();
        const isOk = aDim.buildRadius(this._viewer, aRes.ax2, aRes.radius, (_a = aRes.anchor) !== null && _a !== void 0 ? _a : Vec3.invalid());
        if (!isOk) {
            return null;
        }
        aDim.isVisible = true;
        const aValue = theKind === 'diameter' ? 2.0 * aRes.radius : aRes.radius;
        const aValueText = this.formatLength(aValue);
        aDim.textValue = aValueText;
        if (theKind === 'diameter') {
            aDim.specialSymbol = 'D';
            aDim.isSpecialSymbolAfter = false;
        }
        const aItem = {
            id: this.ensureId(theId),
            kind: theKind,
            label: theKind === 'diameter' ? 'Diameter' : 'Radius',
            value: aValue,
            valueText: aValueText,
            dimensionId: aDim.id,
            objectIds: [theTarget.objId],
            inputs: { target: theTarget },
            valid: true,
            createdAt: new Date().toISOString()
        };
        this._items.push(aItem);
        return aItem;
    }
    restoreItem(theItem) {
        var _a, _b, _c;
        if (!theItem || !theItem.kind) {
            return this.addInvalidItem('distance', theItem);
        }
        const anInputs = (_a = theItem.inputs) !== null && _a !== void 0 ? _a : {};
        if (theItem.kind === 'distance' && anInputs.a && anInputs.b) {
            const aItem = this.addDistance(anInputs.a, anInputs.b, theItem.id);
            if (aItem) {
                aItem.label = (_b = theItem.label) !== null && _b !== void 0 ? _b : aItem.label;
                return aItem;
            }
            return this.addInvalidItem('distance', theItem);
        }
        if ((theItem.kind === 'radius' || theItem.kind === 'diameter') && anInputs.target) {
            const aItem = theItem.kind === 'radius'
                ? this.addRadius(anInputs.target, theItem.id)
                : this.addDiameter(anInputs.target, theItem.id);
            if (aItem) {
                aItem.label = (_c = theItem.label) !== null && _c !== void 0 ? _c : aItem.label;
                return aItem;
            }
            return this.addInvalidItem(theItem.kind, theItem);
        }
        return this.addInvalidItem(theItem.kind, theItem);
    }
    addInvalidItem(theKind, theItem) {
        var _a, _b, _c, _d;
        const aItem = {
            id: this.ensureId(theItem === null || theItem === void 0 ? void 0 : theItem.id),
            kind: theKind,
            label: (_a = theItem === null || theItem === void 0 ? void 0 : theItem.label) !== null && _a !== void 0 ? _a : this.kindLabel(theKind),
            value: typeof (theItem === null || theItem === void 0 ? void 0 : theItem.value) === 'number' ? theItem === null || theItem === void 0 ? void 0 : theItem.value : null,
            valueText: (_b = theItem === null || theItem === void 0 ? void 0 : theItem.valueText) !== null && _b !== void 0 ? _b : '',
            dimensionId: null,
            objectIds: [],
            inputs: (_c = theItem === null || theItem === void 0 ? void 0 : theItem.inputs) !== null && _c !== void 0 ? _c : {},
            valid: false,
            createdAt: (_d = theItem === null || theItem === void 0 ? void 0 : theItem.createdAt) !== null && _d !== void 0 ? _d : new Date().toISOString()
        };
        this._items.push(aItem);
        return aItem;
    }
    focusItem(theItem) {
        var _a;
        const aSelMgr = this._viewer.selectionManager;
        const aCtx = this._viewer.interactiveContext;
        if (!aSelMgr || !aCtx) {
            return false;
        }
        const aObjects = [];
        for (const id of theItem.objectIds) {
            const anObj = aCtx.findInteractiveObject(id);
            if (anObj) {
                aObjects.push(anObj);
            }
        }
        aSelMgr.selectedObjects = aObjects;
        if (theItem.dimensionId) {
            const aDimObj = aCtx.findInteractiveObject(theItem.dimensionId);
            if (aDimObj) {
                aDimObj.isSelected = true;
            }
        }
        (_a = this._viewer.camera) === null || _a === void 0 ? void 0 : _a.fitAllAuto(true, true);
        this._viewer.requestViewUpdate();
        return aObjects.length > 0 || !!theItem.dimensionId;
    }
    removeDimension(theDimId) {
        if (!theDimId) {
            return;
        }
        const aCtx = this._viewer.interactiveContext;
        if (!aCtx) {
            return;
        }
        const aObj = aCtx.findInteractiveObject(theDimId);
        if (aObj) {
            aObj.remove();
            this._viewer.requestViewUpdate();
        }
    }
    ensureId(theId) {
        let aId = (theId && theId.length > 0) ? theId : this.generateId();
        if (!this._items.some((item) => item.id === aId)) {
            return aId;
        }
        let aCounter = 1;
        while (this._items.some((item) => item.id === `${aId}-${aCounter}`)) {
            aCounter += 1;
        }
        return `${aId}-${aCounter}`;
    }
    generateId() {
        return `ms-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
    }
    kindLabel(theKind) {
        switch (theKind) {
            case 'diameter': return 'Diameter';
            case 'radius': return 'Radius';
            default: return 'Distance';
        }
    }
    formatLength(theValue) {
        const aUnitMgr = this._viewer.unitManager;
        if (!aUnitMgr) {
            return `${theValue}`;
        }
        return aUnitMgr.length.format(theValue);
    }
    parseDistanceJson(theJson) {
        if (!theJson) {
            return null;
        }
        try {
            const aParsed = JSON.parse(theJson);
            return aParsed;
        }
        catch (_a) {
            return null;
        }
    }
    parseCircleJson(theJson) {
        if (!theJson) {
            return null;
        }
        let aParsed = null;
        try {
            aParsed = JSON.parse(theJson);
        }
        catch (_a) {
            return null;
        }
        if (!aParsed || !aParsed.ax2 || typeof aParsed.radius !== 'number') {
            return null;
        }
        const aLoc = this.parseVec3(aParsed.ax2.location);
        const aDir = this.parseVec3(aParsed.ax2.direction);
        const aXDir = this.parseVec3(aParsed.ax2.xDirection);
        if (!aLoc || !aDir || !aXDir) {
            return null;
        }
        const aAx2 = new Ax2();
        aAx2.location = aLoc;
        aAx2.direction = aDir;
        aAx2.xDirection = aXDir;
        const anAnchor = this.parseVec3(aParsed.anchor);
        return { ax2: aAx2, radius: aParsed.radius, anchor: anAnchor !== null && anAnchor !== void 0 ? anAnchor : undefined };
    }
    parseVec3(theObj) {
        if (!theObj || typeof theObj.x !== 'number' || typeof theObj.y !== 'number' || typeof theObj.z !== 'number') {
            return null;
        }
        return { x: theObj.x, y: theObj.y, z: theObj.z };
    }
}
//# sourceMappingURL=MeasurementStore.js.map