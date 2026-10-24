import type {ObjectId, IAx2, IVec3} from './CommonTools.js';
import {Ax2, Vec3} from './CommonTools.js';
import {InteractiveDimension} from './InteractiveDimension.js';
import {InteractiveTextLabel} from './InteractiveTextLabel.js';
import type {InteractiveObject} from './InteractiveObject.js';
import {TypeOfMarker} from './InteractiveObject.js';
import type {SCwebviewer} from './SCwebviewer.js';

export type PmiKind = 'label' | 'dimension';
export type PmiDimensionKind = 'length' | 'angle' | 'radius' | 'diameter';

export interface PmiAnchor {
  nodeId?: string;
  pos?: IVec3;
}

export interface PmiItem {
  id: string;
  type: string;
  text: string;
  anchor?: PmiAnchor;
  viewIds: string[];
  kind: PmiKind;
  objectId: ObjectId | null;
  valid: boolean;
}

export interface PmiStoreJson {
  specVersion?: string;
  items: PmiItemJson[];
}

export interface PmiItemJson {
  id?: string;
  type?: string;
  text?: string;
  anchor?: PmiAnchorJson;
  viewIds?: string[];
  dimension?: PmiDimensionJson;
}

interface PmiAnchorJson {
  nodeId?: string;
  pos?: [number, number, number];
}

interface PmiDimensionJson {
  kind?: PmiDimensionKind;
  pnt1?: PmiVec3Json;
  pnt2?: PmiVec3Json;
  pnt3?: PmiVec3Json;
  ax2?: PmiAx2Json;
  radius?: number;
  anchor?: PmiVec3Json;
}

interface PmiAx2Json {
  location?: PmiVec3Json;
  direction?: PmiVec3Json;
  xDirection?: PmiVec3Json;
}

interface PmiVec3Json {
  x?: number;
  y?: number;
  z?: number;
}

export class PmiStore {
  private _viewer: SCwebviewer;
  private _items: PmiItem[] = [];

  constructor(theViewer: SCwebviewer) {
    this._viewer = theViewer;
  }

  public get items(): PmiItem[] { return this._items; }

  public clear(): void {
    for (const item of this._items) {
      this.removeInteractiveObject(item.objectId);
    }
    this._items = [];
  }

  public importJson(theJson: string): { items: PmiItem[]; invalidCount: number } {
    let parsed: PmiStoreJson | null = null;
    try {
      parsed = JSON.parse(theJson) as PmiStoreJson;
    } catch {
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

  public focus(theId: string): boolean {
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

  private buildItem(theItem: PmiItemJson): PmiItem {
    const id = (theItem?.id && theItem.id.length > 0) ? theItem.id : this.generateId();
    const type = theItem?.type ?? 'Text';
    const text = theItem?.text ?? '';
    const anchor = this.parseAnchor(theItem?.anchor);
    const viewIds = Array.isArray(theItem?.viewIds) ? theItem?.viewIds ?? [] : [];

    let objectId: ObjectId | null = null;
    let kind: PmiKind = 'label';
    let valid = false;

    if (theItem?.dimension) {
      const dim = this.buildDimension(id, text, theItem.dimension);
      if (dim) {
        objectId = dim;
        kind = 'dimension';
        valid = true;
      }
    }

    if (!valid) {
      const labelId = this.buildLabel(id, text, anchor?.pos ?? null);
      if (labelId) {
        objectId = labelId;
        kind = 'label';
        valid = true;
      }
    }

    const item: PmiItem = {
      id,
      type,
      text,
      anchor: anchor ?? undefined,
      viewIds,
      kind,
      objectId,
      valid,
    };
    this._items.push(item);
    return item;
  }

  private buildLabel(theId: string, theText: string, thePos: IVec3 | null): ObjectId | null {
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

  private buildDimension(theId: string, theText: string, theDim: PmiDimensionJson): ObjectId | null {
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
    } else if (theDim.kind === 'angle') {
      const p1 = this.parseVec3(theDim.pnt1);
      const p2 = this.parseVec3(theDim.pnt2);
      const p3 = this.parseVec3(theDim.pnt3);
      if (p1 && p2 && p3) {
        isOk = dim.buildAngle(viewer, p1, p2, p3);
      }
    } else if (theDim.kind === 'radius' || theDim.kind === 'diameter') {
      const ax2 = this.parseAx2(theDim.ax2);
      const radius = typeof theDim.radius === 'number' ? theDim.radius : null;
      const anchor = this.parseVec3(theDim.anchor);
      if (ax2 && radius != null) {
        isOk = dim.buildRadius(viewer, ax2, radius, anchor ?? Vec3.invalid());
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

  private parseAnchor(theAnchor?: PmiAnchorJson): PmiAnchor | null {
    if (!theAnchor) {
      return null;
    }
    const pos = this.parseVec3Array(theAnchor.pos);
    const anchor: PmiAnchor = {
      nodeId: theAnchor.nodeId,
      pos: pos ?? undefined,
    };
    return anchor;
  }

  private parseAx2(theAx2?: PmiAx2Json): IAx2 | null {
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

  private parseVec3(theObj?: PmiVec3Json): IVec3 | null {
    if (!theObj || typeof theObj.x !== 'number' || typeof theObj.y !== 'number' || typeof theObj.z !== 'number') {
      return null;
    }
    return { x: theObj.x, y: theObj.y, z: theObj.z };
  }

  private parseVec3Array(theArr?: [number, number, number]): IVec3 | null {
    if (!Array.isArray(theArr) || theArr.length !== 3) {
      return null;
    }
    const [x, y, z] = theArr;
    if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') {
      return null;
    }
    return { x, y, z };
  }

  private buildObjectId(theId: string, theSuffix: string): ObjectId {
    return `${theId}::${theSuffix}`;
  }

  private removeInteractiveObject(theId: ObjectId | null): void {
    if (!theId) {
      return;
    }
    const ctx = this._viewer.interactiveContext;
    if (!ctx) {
      return;
    }
    const obj: InteractiveObject | null = ctx.findInteractiveObject(theId);
    if (obj) {
      obj.remove();
      this._viewer.requestViewUpdate();
    }
  }

  private generateId(): string {
    return `pmi-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
  }
}
