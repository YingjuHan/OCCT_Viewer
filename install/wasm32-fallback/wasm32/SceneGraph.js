export class SceneGraph {
    static parseJson(theJson) {
        var _a;
        if (!theJson) {
            return null;
        }
        let aParsed;
        try {
            aParsed = JSON.parse(theJson);
        }
        catch (_b) {
            return null;
        }
        if (!aParsed || !Array.isArray(aParsed.nodes) || typeof aParsed.rootNodeId !== 'string') {
            return null;
        }
        return {
            specVersion: (_a = aParsed.specVersion) !== null && _a !== void 0 ? _a : '',
            rootNodeId: aParsed.rootNodeId,
            nodes: aParsed.nodes.map((n) => {
                var _a, _b, _c;
                return ({
                    id: n.id,
                    name: (_a = n.name) !== null && _a !== void 0 ? _a : n.id,
                    type: (_b = n.type) !== null && _b !== void 0 ? _b : 'undefined',
                    parentId: (_c = n.parentId) !== null && _c !== void 0 ? _c : null,
                    children: Array.isArray(n.children) ? n.children : [],
                    bbox: SceneGraph.parseBBox(n.bbox),
                    propsRef: n.propsRef,
                });
            }),
        };
    }
    static search(theGraph, theQuery, theOptions) {
        var _a, _b, _c, _d;
        if (!theGraph || !theQuery) {
            return [];
        }
        const caseSensitive = (theOptions === null || theOptions === void 0 ? void 0 : theOptions.caseSensitive) === true;
        const useRegex = (theOptions === null || theOptions === void 0 ? void 0 : theOptions.useRegex) === true;
        const query = caseSensitive ? theQuery : theQuery.toLowerCase();
        let regex = null;
        if (useRegex) {
            try {
                regex = new RegExp(theQuery, caseSensitive ? '' : 'i');
            }
            catch (_e) {
                regex = null;
            }
        }
        const matches = [];
        for (const node of theGraph.nodes) {
            const id = (_a = node.id) !== null && _a !== void 0 ? _a : '';
            const name = (_c = (_b = node.name) !== null && _b !== void 0 ? _b : node.id) !== null && _c !== void 0 ? _c : '';
            const type = (_d = node.type) !== null && _d !== void 0 ? _d : '';
            const check = (value) => {
                if (!value) {
                    return false;
                }
                if (regex) {
                    return regex.test(value);
                }
                const val = caseSensitive ? value : value.toLowerCase();
                return val.includes(query);
            };
            if (check(id)) {
                matches.push({ id, name, type, matchField: 'id' });
            }
            else if (check(name)) {
                matches.push({ id, name, type, matchField: 'name' });
            }
            else if (check(type)) {
                matches.push({ id, name, type, matchField: 'type' });
            }
        }
        return matches;
    }
    static parseBBox(theBBox) {
        if (!theBBox || !theBBox.min || !theBBox.max) {
            return null;
        }
        const cornerMin = { x: theBBox.min[0], y: theBBox.min[1], z: theBBox.min[2] };
        const cornerMax = { x: theBBox.max[0], y: theBBox.max[1], z: theBBox.max[2] };
        return { cornerMin, cornerMax };
    }
}
//# sourceMappingURL=SceneGraph.js.map