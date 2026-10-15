import type {ObjectId, ICameraViewPoint, IVec3} from './CommonTools.js';
import {Vec3} from './CommonTools.js';
import {InteractiveClipping} from './InteractiveClipping.js';
import type {InteractiveObject} from './InteractiveObject.js';
import type {SCwebviewer} from './SCwebviewer.js';

export interface SavedViewClipping {
  isClippingOn: boolean;
  isCappingOn: boolean;
  toFlipClipping: boolean;
  toUseObjectMaterial: boolean;
  isShaded: boolean;
  relativeCorner: IVec3;
  relativeExtent: IVec3;
  nbPlanesXYZ: IVec3;
}

export interface SavedViewItem {
  id: string;
  name: string;
  camera: ICameraViewPoint;
  projection: {
    isPerspective: boolean;
    fovy: number;
  };
  hiddenObjectIds: ObjectId[];
  clipping: SavedViewClipping | null;
  createdAt: string;
}

export interface SavedViewStoreJson {
  version: number;
  views: SavedViewItemJson[];
}

export interface SavedViewItemJson {
  id: string;
  name?: string;
  camera?: ICameraViewPoint;
  projection?: {
    isPerspective?: boolean;
    fovy?: number;
  };
  hiddenObjectIds?: ObjectId[];
  clipping?: SavedViewClipping | null;
  createdAt?: string;
}

const STORE_VERSION = 1;

export class SavedViewStore {
  private _viewer: SCwebviewer;
  private _views: SavedViewItem[] = [];

  constructor(theViewer: SCwebviewer) {
    this._viewer = theViewer;
  }

  public get views(): SavedViewItem[] { return this._views; }

  public capture(theName: string = ''): SavedViewItem | null {
    const camera = this._viewer.camera;
    const ctx = this._viewer.interactiveContext;
    if (!camera || !ctx) { return null; }

    const hiddenIds: ObjectId[] = [];
    for (const obj of ctx.objectsInside) {
      if (!obj.isVisible) {
        hiddenIds.push(obj.id);
      }
    }

    const clipping = this.captureClipping();

    const item: SavedViewItem = {
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

  public rename(theId: string, theName: string): boolean {
    const item = this._views.find((v) => v.id === theId);
    if (!item) { return false; }
    item.name = theName;
    return true;
  }

  public remove(theId: string): boolean {
    const index = this._views.findIndex((v) => v.id === theId);
    if (index < 0) { return false; }
    this._views.splice(index, 1);
    return true;
  }

  public clear(): void {
    this._views = [];
  }

  public apply(theId: string): { missingIds: ObjectId[] } {
    const item = this._views.find((v) => v.id === theId);
    if (!item) { return { missingIds: [] }; }
    return this.applyItem(item);
  }

  public exportJson(): string {
    const payload: SavedViewStoreJson = {
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

  public importJson(theJson: string): SavedViewItem[] {
    let parsed: SavedViewStoreJson | null = null;
    try {
      parsed = JSON.parse(theJson) as SavedViewStoreJson;
    } catch {
      return this._views;
    }
    if (!parsed || !Array.isArray(parsed.views)) {
      return this._views;
    }

    this._views = parsed.views.map((view) => ({
      id: view.id || this.generateId(),
      name: view.name || 'View',
      camera: view.camera || this._viewer.camera?.viewPoint,
      projection: {
        isPerspective: view.projection?.isPerspective ?? true,
        fovy: view.projection?.fovy ?? 35
      },
      hiddenObjectIds: view.hiddenObjectIds ?? [],
      clipping: view.clipping ?? null,
      createdAt: view.createdAt ?? new Date().toISOString()
    })).filter((view): view is SavedViewItem => !!view.camera);

    return this._views;
  }

  private applyItem(theItem: SavedViewItem): { missingIds: ObjectId[] } {
    const camera = this._viewer.camera;
    const ctx = this._viewer.interactiveContext;
    if (camera) {
      camera.viewPoint = theItem.camera;
      camera.isPerspectiveProjection = theItem.projection.isPerspective;
      camera.perspectiveProjectionFOVy = theItem.projection.fovy;
    }

    const missingIds: ObjectId[] = [];
    if (ctx) {
      for (const obj of ctx.objectsInside) {
        obj.isVisible = true;
      }
      for (const id of theItem.hiddenObjectIds) {
        const obj = ctx.findInteractiveObject(id);
        if (obj) {
          obj.isVisible = false;
        } else {
          missingIds.push(id);
        }
      }
    }

    this.applyClipping(theItem.clipping);
    this._viewer.requestViewUpdate();
    return { missingIds };
  }

  private captureClipping(): SavedViewClipping | null {
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

  private applyClipping(theState: SavedViewClipping | null): void {
    const clip = this.findClipping() ?? (theState ? new InteractiveClipping(this._viewer, this._viewer.focusView) : null);
    if (!clip) { return; }
    if (!theState) {
      clip.isClippingOn = false;
      return;
    }
    clip.relativeClippingCorner = theState.relativeCorner ?? Vec3.zero();
    clip.relativeClippingExtent = theState.relativeExtent ?? Vec3.zero();
    clip.nbPlanesXYZ = theState.nbPlanesXYZ ?? Vec3.zero();
    clip.isClippingOn = theState.isClippingOn;
    clip.isCappingOn = theState.isCappingOn;
    clip.toFlipClipping = theState.toFlipClipping;
    clip.toUseObjectMaterial = theState.toUseObjectMaterial;
    clip.isShaded = theState.isShaded;
  }

  private findClipping(): InteractiveClipping | null {
    const ctx = this._viewer.interactiveContext;
    if (!ctx) { return null; }
    for (const obj of ctx.objectsInside) {
      if (obj instanceof InteractiveClipping) {
        return obj;
      }
    }
    return null;
  }

  private generateId(): string {
    return `view-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
  }
}
