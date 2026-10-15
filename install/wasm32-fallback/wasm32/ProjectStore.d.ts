import type { ObjectId } from './CommonTools.js';
import type { SCwebviewer } from './SCwebviewer.js';
import type { ISceneGraph } from './SceneGraph.js';
import type { MeasurementStoreJson } from './MeasurementStore.js';
import type { SavedViewStoreJson } from './SavedViewStore.js';
export type ProjectSourceType = 'url' | 'local' | 'buffer';
export type ProjectObjectKind = 'shape' | 'mesh';
export interface ProjectSource {
    id: ObjectId;
    kind: ProjectObjectKind;
    sourceType: ProjectSourceType;
    source: string;
}
export interface ProjectObjectStyle {
    id: ObjectId;
    visible: boolean;
    baseColor: string;
    transparency: number;
}
export interface ProjectData {
    version: string;
    createdAt: string;
    sources: ProjectSource[];
    styles: ProjectObjectStyle[];
    savedViews?: SavedViewStoreJson | null;
    measurements?: MeasurementStoreJson | null;
    sceneGraph?: ISceneGraph | null;
}
export interface ProjectExportInput {
    sources: ProjectSource[];
    savedViews?: SavedViewStoreJson | null;
    measurements?: MeasurementStoreJson | null;
}
export interface ProjectImportResult {
    project: ProjectData;
    missingSources: ProjectSource[];
}
export declare class ProjectStore {
    private _viewer;
    constructor(theViewer: SCwebviewer);
    exportJson(theInput: ProjectExportInput): string;
    importJson(theJson: string): ProjectImportResult | null;
    private collectStyles;
    private readSceneGraph;
}
