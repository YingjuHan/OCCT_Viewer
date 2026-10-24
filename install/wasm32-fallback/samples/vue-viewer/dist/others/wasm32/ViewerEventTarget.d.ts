/**
 * Extends EventTarget with map of registered callbacks.
 */
export declare class ViewerEventTarget extends EventTarget {
    private _myListenerMap;
    /** Main constructor. */
    constructor();
    /** Return all event listeners. */
    get eventListeners(): Map<string, EventListenerOrEventListenerObject[]> | undefined;
    /** Add event listener, but avoid inclusion of the same listener twice. */
    addEventListener(theType: string, theListener: EventListenerOrEventListenerObject | null, theOptions?: boolean | AddEventListenerOptions): void;
    /** Remove event listener. */
    removeEventListener(theType: string, theListener: EventListenerOrEventListenerObject | null, theOptions?: EventListenerOptions | boolean): void;
    /** Remove all event listeners. */
    removeAllEventListeners(theType?: string, theOptions?: EventListenerOptions | boolean): void;
    /** Return event listeners for specific type. */
    getEventListeners(theType: string): EventListenerOrEventListenerObject[] | undefined;
    /** Replace event listeners for specific type. */
    setEventListeners(theType: string, theListeners: EventListenerOrEventListenerObject[] | undefined, theOptions?: boolean | AddEventListenerOptions): void;
    /** Remove all event listeners for specific type and adds specified one. */
    setEventListener(theType: string, theListener: EventListenerOrEventListenerObject | null, theOptions?: boolean | AddEventListenerOptions): void;
}
