import type {ObjectId, IVec3, IAx2} from './CommonTools.js';
import {Ax2, Vec3} from './CommonTools.js';
import {InteractiveDimension} from './InteractiveDimension.js';
import type {InteractiveObject} from './InteractiveObject.js';
import type {SCwebviewer} from './SCwebviewer.js';

export type MeasurementKind = 'distance' | 'radius' | 'diameter';

export interface MeasurementTarget {
  objId: ObjectId;
  selMode: number;
  entityId: number;
}

export interface MeasurementItem {
  id: string;
  kind: MeasurementKind;
  label: string;
  value: number | null;
  valueText: string;
  dimensionId: ObjectId | null;
  objectIds: ObjectId[];
  inputs: {
    a?: MeasurementTarget;
    b?: MeasurementTarget;
    target?: MeasurementTarget;
  };
  valid: boolean;
  createdAt: string;
}

export interface MeasurementStoreJson {
  version: number;
  items: MeasurementItemJson[];
}

export interface MeasurementItemJson {
  id: string;
  kind: MeasurementKind;
  label?: string;
  value?: number;
  valueText?: string;
  createdAt?: string;
  inputs?: {
    a?: MeasurementTarget;
    b?: MeasurementTarget;
    target?: MeasurementTarget;
  };
}

type MeasureDistanceJson = {
  distance?: number;
  pnt1?: IVec3;
  pnt2?: IVec3;
};

type CircleParamsJson = {
  ax2?: { location?: IVec3; direction?: IVec3; xDirection?: IVec3 };
  radius?: number;
  anchor?: IVec3;
};

const STORE_VERSION = 1;

export class MeasurementStore {
  private _viewer: SCwebviewer;
  private _items: MeasurementItem[] = [];

  constructor(theViewer: SCwebviewer) {
    this._viewer = theViewer;
  }

  public get items(): MeasurementItem[] { return this._items; }

  public addDistance(theA: MeasurementTarget, theB: MeasurementTarget, theId?: string): MeasurementItem | null {
    const aMgr = this._viewer.interactiveShapeManager;
    if (!aMgr) { return null; }

    const aResStr = aMgr.measureDistanceJson(
      theA.objId, theA.selMode, theA.entityId,
      theB.objId, theB.selMode, theB.entityId
    );
    const aRes = this.parseDistanceJson(aResStr);
    if (!aRes || aRes.distance == null || !aRes.pnt1 || !aRes.pnt2) {
      return null;
    }

    const aDim = new InteractiveDimension();
    const isOk = aDim.buildLength(this._viewer, aRes.pnt1, aRes.pnt2);
    if (!isOk) { return null; }
    aDim.isVisible = true;
    const aValueText = this.formatLength(aRes.distance);
    aDim.textValue = aValueText;

    const aItem: MeasurementItem = {
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

  public addRadius(theTarget: MeasurementTarget, theId?: string): MeasurementItem | null {
    return this.addCircleMeasurement('radius', theTarget, theId);
  }

  public addDiameter(theTarget: MeasurementTarget, theId?: string): MeasurementItem | null {
    return this.addCircleMeasurement('diameter', theTarget, theId);
  }

  public remove(theId: string): boolean {
    const aIndex = this._items.findIndex((item) => item.id === theId);
    if (aIndex < 0) { return false; }
    const [aItem] = this._items.splice(aIndex, 1);
    this.removeDimension(aItem.dimensionId);
    return true;
  }

  public clear(): void {
    for (const item of this._items) {
      this.removeDimension(item.dimensionId);
    }
    this._items = [];
  }

  public exportJson(): string {
    const aPayload: MeasurementStoreJson = {
      version: STORE_VERSION,
      items: this._items.map((item) => ({
        id: item.id,
        kind: item.kind,
        label: item.label,
        value: item.value ?? undefined,
        valueText: item.valueText || undefined,
        createdAt: item.createdAt,
        inputs: item.inputs
      }))
    };
    return JSON.stringify(aPayload, null, 2);
  }

  public importJson(theJson: string): { items: MeasurementItem[]; invalidCount: number } {
    let aParsed: MeasurementStoreJson | null = null;
    try {
      aParsed = JSON.parse(theJson) as MeasurementStoreJson;
    } catch {
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

  public focus(theId: string): boolean {
    const aItem = this._items.find((item) => item.id === theId);
    if (!aItem) { return false; }
    return this.focusItem(aItem);
  }

  private addCircleMeasurement(theKind: 'radius' | 'diameter', theTarget: MeasurementTarget, theId?: string): MeasurementItem | null {
    const aMgr = this._viewer.interactiveShapeManager;
    if (!aMgr) { return null; }

    const aResStr = aMgr.getCircleParamsJson(theTarget.objId, theTarget.selMode, theTarget.entityId);
    const aRes = this.parseCircleJson(aResStr);
    if (!aRes || !aRes.ax2 || typeof aRes.radius !== 'number') {
      return null;
    }

    const aDim = new InteractiveDimension();
    const isOk = aDim.buildRadius(this._viewer, aRes.ax2, aRes.radius, aRes.anchor ?? Vec3.invalid());
    if (!isOk) { return null; }
    aDim.isVisible = true;

    const aValue = theKind === 'diameter' ? 2.0 * aRes.radius : aRes.radius;
    const aValueText = this.formatLength(aValue);
    aDim.textValue = aValueText;
    if (theKind === 'diameter') {
      aDim.specialSymbol = 'D';
      aDim.isSpecialSymbolAfter = false;
    }

    const aItem: MeasurementItem = {
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

  private restoreItem(theItem: MeasurementItemJson): MeasurementItem {
    if (!theItem || !theItem.kind) {
      return this.addInvalidItem('distance', theItem);
    }

    const anInputs = theItem.inputs ?? {};
    if (theItem.kind === 'distance' && anInputs.a && anInputs.b) {
      const aItem = this.addDistance(anInputs.a, anInputs.b, theItem.id);
      if (aItem) {
        aItem.label = theItem.label ?? aItem.label;
        return aItem;
      }
      return this.addInvalidItem('distance', theItem);
    }

    if ((theItem.kind === 'radius' || theItem.kind === 'diameter') && anInputs.target) {
      const aItem = theItem.kind === 'radius'
        ? this.addRadius(anInputs.target, theItem.id)
        : this.addDiameter(anInputs.target, theItem.id);
      if (aItem) {
        aItem.label = theItem.label ?? aItem.label;
        return aItem;
      }
      return this.addInvalidItem(theItem.kind, theItem);
    }

    return this.addInvalidItem(theItem.kind, theItem);
  }

  private addInvalidItem(theKind: MeasurementKind, theItem?: MeasurementItemJson): MeasurementItem {
    const aItem: MeasurementItem = {
      id: this.ensureId(theItem?.id),
      kind: theKind,
      label: theItem?.label ?? this.kindLabel(theKind),
      value: typeof theItem?.value === 'number' ? theItem?.value : null,
      valueText: theItem?.valueText ?? '',
      dimensionId: null,
      objectIds: [],
      inputs: theItem?.inputs ?? {},
      valid: false,
      createdAt: theItem?.createdAt ?? new Date().toISOString()
    };
    this._items.push(aItem);
    return aItem;
  }

  private focusItem(theItem: MeasurementItem): boolean {
    const aSelMgr = this._viewer.selectionManager;
    const aCtx = this._viewer.interactiveContext;
    if (!aSelMgr || !aCtx) { return false; }

    const aObjects: InteractiveObject[] = [];
    for (const id of theItem.objectIds) {
      const anObj = aCtx.findInteractiveObject(id);
      if (anObj) { aObjects.push(anObj); }
    }
    aSelMgr.selectedObjects = aObjects;

    if (theItem.dimensionId) {
      const aDimObj = aCtx.findInteractiveObject(theItem.dimensionId);
      if (aDimObj) {
        aDimObj.isSelected = true;
      }
    }

    this._viewer.camera?.fitAllAuto(true, true);
    this._viewer.requestViewUpdate();
    return aObjects.length > 0 || !!theItem.dimensionId;
  }

  private removeDimension(theDimId: ObjectId | null): void {
    if (!theDimId) { return; }
    const aCtx = this._viewer.interactiveContext;
    if (!aCtx) { return; }
    const aObj = aCtx.findInteractiveObject(theDimId);
    if (aObj) {
      aObj.remove();
      this._viewer.requestViewUpdate();
    }
  }

  private ensureId(theId?: string): string {
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

  private generateId(): string {
    return `ms-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
  }

  private kindLabel(theKind: MeasurementKind): string {
    switch (theKind) {
      case 'diameter': return 'Diameter';
      case 'radius': return 'Radius';
      default: return 'Distance';
    }
  }

  private formatLength(theValue: number): string {
    const aUnitMgr = this._viewer.unitManager;
    if (!aUnitMgr) { return `${theValue}`; }
    return aUnitMgr.length.format(theValue);
  }

  private parseDistanceJson(theJson: string): MeasureDistanceJson | null {
    if (!theJson) { return null; }
    try {
      const aParsed = JSON.parse(theJson) as MeasureDistanceJson;
      return aParsed;
    } catch {
      return null;
    }
  }

  private parseCircleJson(theJson: string): { ax2: IAx2; radius: number; anchor?: IVec3 } | null {
    if (!theJson) { return null; }
    let aParsed: CircleParamsJson | null = null;
    try {
      aParsed = JSON.parse(theJson) as CircleParamsJson;
    } catch {
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
    return { ax2: aAx2, radius: aParsed.radius, anchor: anAnchor ?? undefined };
  }

  private parseVec3(theObj: IVec3 | undefined): IVec3 | null {
    if (!theObj || typeof theObj.x !== 'number' || typeof theObj.y !== 'number' || typeof theObj.z !== 'number') {
      return null;
    }
    return { x: theObj.x, y: theObj.y, z: theObj.z };
  }
}
