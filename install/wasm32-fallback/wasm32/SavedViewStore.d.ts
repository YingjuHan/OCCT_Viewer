import type { ObjectId, ICameraViewPoint, IVec3 } from './CommonTools.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
export declare class SavedViewStore {
    private _viewer;
    private _views;
    constructor(theViewer: SCwebviewer);
    get views(): SavedViewItem[];
    capture(theName?: string): SavedViewItem | null;
    rename(theId: string, theName: string): boolean;
    remove(theId: string): boolean;
    clear(): void;
    apply(theId: string): {
        missingIds: ObjectId[];
    };
    exportJson(): string;
    importJson(theJson: string): SavedViewItem[];
    private applyItem;
    private captureClipping;
    private applyClipping;
    private findClipping;
    private generateId;
}
