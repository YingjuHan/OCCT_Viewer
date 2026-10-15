export class ProjectStore {
    constructor(theViewer) {
        this._viewer = theViewer;
    }
    exportJson(theInput) {
        var _a, _b, _c, _d, _e;
        const objects = (_c = (_b = (_a = this._viewer) === null || _a === void 0 ? void 0 : _a.interactiveContext) === null || _b === void 0 ? void 0 : _b.objectsInside) !== null && _c !== void 0 ? _c : [];
        const project = {
            version: '1.0',
            createdAt: new Date().toISOString(),
            sources: Array.isArray(theInput.sources) ? theInput.sources : [],
            styles: this.collectStyles(objects),
            savedViews: (_d = theInput.savedViews) !== null && _d !== void 0 ? _d : null,
            measurements: (_e = theInput.measurements) !== null && _e !== void 0 ? _e : null,
            sceneGraph: this.readSceneGraph(),
        };
        return JSON.stringify(project, null, 2);
    }
    importJson(theJson) {
        var _a, _b, _c, _d, _e;
        if (!theJson) {
            return null;
        }
        let aParsed = null;
        try {
            aParsed = JSON.parse(theJson);
        }
        catch (_f) {
            return null;
        }
        if (aParsed == null || !Array.isArray(aParsed.sources)) {
            return null;
        }
        const project = {
            version: (_a = aParsed.version) !== null && _a !== void 0 ? _a : '1.0',
            createdAt: (_b = aParsed.createdAt) !== null && _b !== void 0 ? _b : new Date().toISOString(),
            sources: Array.isArray(aParsed.sources) ? aParsed.sources : [],
            styles: Array.isArray(aParsed.styles) ? aParsed.styles : [],
            savedViews: (_c = aParsed.savedViews) !== null && _c !== void 0 ? _c : null,
            measurements: (_d = aParsed.measurements) !== null && _d !== void 0 ? _d : null,
            sceneGraph: (_e = aParsed.sceneGraph) !== null && _e !== void 0 ? _e : null,
        };
        const missingSources = project.sources.filter((src) => src.sourceType !== 'url');
        return { project, missingSources };
    }
    collectStyles(theObjects) {
        return theObjects.map((obj) => {
            var _a, _b;
            return ({
                id: obj.id,
                visible: obj.isVisible,
                baseColor: (_a = obj.baseColor) !== null && _a !== void 0 ? _a : '',
                transparency: (_b = obj.transparency) !== null && _b !== void 0 ? _b : 0.0,
            });
        });
    }
    readSceneGraph() {
        var _a;
        const ctx = (_a = this._viewer) === null || _a === void 0 ? void 0 : _a.interactiveContext;
        if (!ctx || typeof ctx.getSceneGraph !== 'function') {
            return null;
        }
        return ctx.getSceneGraph();
    }
}
//# sourceMappingURL=ProjectStore.js.map