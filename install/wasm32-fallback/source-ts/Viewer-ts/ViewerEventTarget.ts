// Created by: Kirill Gavrilov
// Copyright (c) 2022-2024 Huawei Technologies Co., Ltd.

/**
 * Extends EventTarget with map of registered callbacks.
 */
export class ViewerEventTarget extends EventTarget {
  private _myListenerMap: Map<string, EventListenerOrEventListenerObject[]> | undefined;

  /** Main constructor. */
  constructor() {
    super();
  }

  /** @internal Return overall number of listeners. */
  public get nbEventListeners(): number {
    let aCounter: number = 0;
    this._myListenerMap?.forEach((theValue, theKey) => { ++aCounter; });
    return aCounter;
  }

  /** @internal Return event listener types. */
  public get eventListenerTypes(): string[] {
    return this._myListenerMap ? Array.from(this._myListenerMap.keys()) : [];
  }

  /** Return all event listeners. */
  public get eventListeners(): Map<string, EventListenerOrEventListenerObject[]> | undefined {
    return this._myListenerMap;
  }

  // overriders for EventTarget interface

  /** Add event listener, but avoid inclusion of the same listener twice. */
  public addEventListener(theType: string,
                          theListener: EventListenerOrEventListenerObject | null,
                          theOptions?: boolean | AddEventListenerOptions): void {
    if (theType === '') { throw new Error('Invalid event type'); }
    super.addEventListener(theType, theListener, theOptions);

    if (!this._myListenerMap) {
      this._myListenerMap = new Map<string, EventListenerOrEventListenerObject[]>();
    }
    let aListeners: EventListenerOrEventListenerObject[] | undefined = this._myListenerMap.get(theType);
    if (!aListeners) { aListeners = []; }
    aListeners = aListeners.filter((theElem, theIndex, theArray) => { return (theElem !== theListener); });
    if (theListener) {
      aListeners.push(theListener);
    }
    this._myListenerMap.set(theType, aListeners);
  }

  /** Remove event listener. */
  public removeEventListener(theType: string,
                             theListener: EventListenerOrEventListenerObject | null,
                             theOptions?: EventListenerOptions | boolean): void {
    if (theType === '') { throw new Error('Invalid event type'); }
    super.removeEventListener(theType, theListener, theOptions);

    let aListeners: EventListenerOrEventListenerObject[] | undefined = this.getEventListeners(theType);
    if (!aListeners) { return; }
    aListeners = aListeners.filter((theElem, theIndex, theArray) => { return (theElem !== theListener); });
    if (aListeners.length === 0) {
      this._myListenerMap?.delete(theType);
    } else {
      this._myListenerMap?.set(theType, aListeners);
    }
  }

  // extra methods

  /** Remove all event listeners. */
  public removeAllEventListeners(theType: string = '',
                                 theOptions?: EventListenerOptions | boolean): void {
    if (theType === '') {
      this._myListenerMap?.forEach((theValue, theKey) => {
        theValue.forEach((theListener: EventListenerOrEventListenerObject) => {
          super.removeEventListener(theKey, theListener, theOptions);
        });
      });
      this._myListenerMap?.clear();
      return;
    }

    const aListeners: EventListenerOrEventListenerObject[] | undefined = this.getEventListeners(theType);
    if (!aListeners) { return; }
    aListeners.forEach((theListener: EventListenerOrEventListenerObject) => {
      super.removeEventListener(theType, theListener);
    });
    this._myListenerMap?.delete(theType);
  }

  /** Return event listeners for specific type. */
  public getEventListeners(theType: string): EventListenerOrEventListenerObject[] | undefined {
    return this._myListenerMap?.get(theType) ?? undefined;
  }

  /** Replace event listeners for specific type. */
  public setEventListeners(theType: string,
                           theListeners: EventListenerOrEventListenerObject[] | undefined,
                           theOptions?: boolean | AddEventListenerOptions): void {
    if (theType === '') { throw new Error('Invalid event type'); }

    this.removeAllEventListeners(theType);
    if (!theListeners || theListeners.length === 0) {
      return;
    }

    theListeners?.forEach((theListener: EventListenerOrEventListenerObject) => {
      super.addEventListener(theType, theListener, theOptions);
    });

    if (!this._myListenerMap) {
      this._myListenerMap = new Map<string, EventListenerOrEventListenerObject[]>();
    }
    this._myListenerMap.set(theType, theListeners);
  }

  /** Remove all event listeners for specific type and adds specified one. */
  public setEventListener(theType: string,
                          theListener: EventListenerOrEventListenerObject | null,
                          theOptions?: boolean | AddEventListenerOptions): void {
    if (theType === '') { throw new Error('Invalid event type'); }

    this.removeAllEventListeners(theType);
    this.addEventListener(theType, theListener, theOptions);
  }
};
