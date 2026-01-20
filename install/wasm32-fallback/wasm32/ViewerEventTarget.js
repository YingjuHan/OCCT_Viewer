/**
 * Extends EventTarget with map of registered callbacks.
 */
export class ViewerEventTarget extends EventTarget {
    /** Main constructor. */
    constructor() {
        super();
    }
    /** @internal Return overall number of listeners. */
    get nbEventListeners() {
        var _a;
        let aCounter = 0;
        (_a = this._myListenerMap) === null || _a === void 0 ? void 0 : _a.forEach((theValue, theKey) => { ++aCounter; });
        return aCounter;
    }
    /** @internal Return event listener types. */
    get eventListenerTypes() {
        return this._myListenerMap ? Array.from(this._myListenerMap.keys()) : [];
    }
    /** Return all event listeners. */
    get eventListeners() {
        return this._myListenerMap;
    }
    // overriders for EventTarget interface
    /** Add event listener, but avoid inclusion of the same listener twice. */
    addEventListener(theType, theListener, theOptions) {
        if (theType === '') {
            throw new Error('Invalid event type');
        }
        super.addEventListener(theType, theListener, theOptions);
        if (!this._myListenerMap) {
            this._myListenerMap = new Map();
        }
        let aListeners = this._myListenerMap.get(theType);
        if (!aListeners) {
            aListeners = [];
        }
        aListeners = aListeners.filter((theElem, theIndex, theArray) => { return (theElem !== theListener); });
        if (theListener) {
            aListeners.push(theListener);
        }
        this._myListenerMap.set(theType, aListeners);
    }
    /** Remove event listener. */
    removeEventListener(theType, theListener, theOptions) {
        var _a, _b;
        if (theType === '') {
            throw new Error('Invalid event type');
        }
        super.removeEventListener(theType, theListener, theOptions);
        let aListeners = this.getEventListeners(theType);
        if (!aListeners) {
            return;
        }
        aListeners = aListeners.filter((theElem, theIndex, theArray) => { return (theElem !== theListener); });
        if (aListeners.length === 0) {
            (_a = this._myListenerMap) === null || _a === void 0 ? void 0 : _a.delete(theType);
        }
        else {
            (_b = this._myListenerMap) === null || _b === void 0 ? void 0 : _b.set(theType, aListeners);
        }
    }
    // extra methods
    /** Remove all event listeners. */
    removeAllEventListeners(theType = '', theOptions) {
        var _a, _b, _c;
        if (theType === '') {
            (_a = this._myListenerMap) === null || _a === void 0 ? void 0 : _a.forEach((theValue, theKey) => {
                theValue.forEach((theListener) => {
                    super.removeEventListener(theKey, theListener, theOptions);
                });
            });
            (_b = this._myListenerMap) === null || _b === void 0 ? void 0 : _b.clear();
            return;
        }
        const aListeners = this.getEventListeners(theType);
        if (!aListeners) {
            return;
        }
        aListeners.forEach((theListener) => {
            super.removeEventListener(theType, theListener);
        });
        (_c = this._myListenerMap) === null || _c === void 0 ? void 0 : _c.delete(theType);
    }
    /** Return event listeners for specific type. */
    getEventListeners(theType) {
        var _a, _b;
        return (_b = (_a = this._myListenerMap) === null || _a === void 0 ? void 0 : _a.get(theType)) !== null && _b !== void 0 ? _b : undefined;
    }
    /** Replace event listeners for specific type. */
    setEventListeners(theType, theListeners, theOptions) {
        if (theType === '') {
            throw new Error('Invalid event type');
        }
        this.removeAllEventListeners(theType);
        if (!theListeners || theListeners.length === 0) {
            return;
        }
        theListeners === null || theListeners === void 0 ? void 0 : theListeners.forEach((theListener) => {
            super.addEventListener(theType, theListener, theOptions);
        });
        if (!this._myListenerMap) {
            this._myListenerMap = new Map();
        }
        this._myListenerMap.set(theType, theListeners);
    }
    /** Remove all event listeners for specific type and adds specified one. */
    setEventListener(theType, theListener, theOptions) {
        if (theType === '') {
            throw new Error('Invalid event type');
        }
        this.removeAllEventListeners(theType);
        this.addEventListener(theType, theListener, theOptions);
    }
}
;
//# sourceMappingURL=ViewerEventTarget.js.map