import type { ObjectId } from './CommonTools.js';
import type { SCwebviewer } from './SCwebviewer.js';
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
export declare class MeasurementStore {
    private _viewer;
    private _items;
    constructor(theViewer: SCwebviewer);
    get items(): MeasurementItem[];
    addDistance(theA: MeasurementTarget, theB: MeasurementTarget, theId?: string): MeasurementItem | null;
    addRadius(theTarget: MeasurementTarget, theId?: string): MeasurementItem | null;
    addDiameter(theTarget: MeasurementTarget, theId?: string): MeasurementItem | null;
    remove(theId: string): boolean;
    clear(): void;
    exportJson(): string;
    importJson(theJson: string): {
        items: MeasurementItem[];
        invalidCount: number;
    };
    focus(theId: string): boolean;
    private addCircleMeasurement;
    private restoreItem;
    private addInvalidItem;
    private focusItem;
    private removeDimension;
    private ensureId;
    private generateId;
    private kindLabel;
    private formatLength;
    private parseDistanceJson;
    private parseCircleJson;
    private parseVec3;
}
