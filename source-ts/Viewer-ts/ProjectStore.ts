import type {ObjectId} from './CommonTools.js';
import type {SCwebviewer} from './SCwebviewer.js';
import type {InteractiveObject} from './InteractiveObject.js';
import type {ISceneGraph} from './SceneGraph.js';
import type {MeasurementStoreJson} from './MeasurementStore.js';
import type {SavedViewStoreJson} from './SavedViewStore.js';

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

export class ProjectStore {
  private _viewer: SCwebviewer | null;

  constructor(theViewer: SCwebviewer) {
    this._viewer = theViewer;
  }

  public exportJson(theInput: ProjectExportInput): string {
    const objects = this._viewer?.interactiveContext?.objectsInside ?? [];
    const project: ProjectData = {
      version: '1.0',
      createdAt: new Date().toISOString(),
      sources: Array.isArray(theInput.sources) ? theInput.sources : [],
      styles: this.collectStyles(objects),
      savedViews: theInput.savedViews ?? null,
      measurements: theInput.measurements ?? null,
      sceneGraph: this.readSceneGraph(),
    };
    return JSON.stringify(project, null, 2);
  }

  public importJson(theJson: string): ProjectImportResult | null {
    if (!theJson) {
      return null;
    }

    let aParsed: ProjectData | null = null;
    try {
      aParsed = JSON.parse(theJson) as ProjectData;
    } catch {
      return null;
    }

    if (aParsed == null || !Array.isArray(aParsed.sources)) {
      return null;
    }

    const project: ProjectData = {
      version: aParsed.version ?? '1.0',
      createdAt: aParsed.createdAt ?? new Date().toISOString(),
      sources: Array.isArray(aParsed.sources) ? aParsed.sources : [],
      styles: Array.isArray(aParsed.styles) ? aParsed.styles : [],
      savedViews: aParsed.savedViews ?? null,
      measurements: aParsed.measurements ?? null,
      sceneGraph: aParsed.sceneGraph ?? null,
    };

    const missingSources = project.sources.filter((src) => src.sourceType !== 'url');
    return { project, missingSources };
  }

  private collectStyles(theObjects: InteractiveObject[]): ProjectObjectStyle[] {
    return theObjects.map((obj) => ({
      id: obj.id,
      visible: obj.isVisible,
      baseColor: obj.baseColor ?? '',
      transparency: obj.transparency ?? 0.0,
    }));
  }

  private readSceneGraph(): ISceneGraph | null {
    const ctx: any = this._viewer?.interactiveContext as any;
    if (!ctx || typeof ctx.getSceneGraph !== 'function') {
      return null;
    }
    return ctx.getSceneGraph();
  }
}
