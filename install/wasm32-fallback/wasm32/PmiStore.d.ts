import type { ObjectId, IVec3 } from './CommonTools.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
export declare class PmiStore {
    private _viewer;
    private _items;
    constructor(theViewer: SCwebviewer);
    get items(): PmiItem[];
    clear(): void;
    importJson(theJson: string): {
        items: PmiItem[];
        invalidCount: number;
    };
    focus(theId: string): boolean;
    private buildItem;
    private buildLabel;
    private buildDimension;
    private parseAnchor;
    private parseAx2;
    private parseVec3;
    private parseVec3Array;
    private buildObjectId;
    private removeInteractiveObject;
    private generateId;
}
export {};
