import type {IBndBox3, IVec3, ObjectId} from './CommonTools.js';

export interface ISceneGraphNode {
  id: ObjectId;
  name: string;
  type: string;
  parentId: ObjectId | null;
  children: ObjectId[];
  bbox: IBndBox3 | null;
  propsRef?: string;
}

export interface ISceneGraph {
  specVersion: string;
  rootNodeId: ObjectId;
  nodes: ISceneGraphNode[];
}

interface ISceneGraphJsonBBox {
  min: [number, number, number];
  max: [number, number, number];
}

interface ISceneGraphJsonNode {
  id: ObjectId;
  name: string;
  type: string;
  parentId: ObjectId | null;
  children: ObjectId[];
  bbox: ISceneGraphJsonBBox | null;
  propsRef?: string;
}

interface ISceneGraphJson {
  specVersion: string;
  rootNodeId: ObjectId;
  nodes: ISceneGraphJsonNode[];
}

export class SceneGraph {
  public static parseJson(theJson: string): ISceneGraph | null {
    if (!theJson) {
      return null;
    }

    let aParsed: ISceneGraphJson;
    try {
      aParsed = JSON.parse(theJson) as ISceneGraphJson;
    } catch {
      return null;
    }

    if (!aParsed || !Array.isArray(aParsed.nodes) || typeof aParsed.rootNodeId !== 'string') {
      return null;
    }

    return {
      specVersion: aParsed.specVersion ?? '',
      rootNodeId: aParsed.rootNodeId,
      nodes: aParsed.nodes.map((n) => ({
        id: n.id,
        name: n.name ?? n.id,
        type: n.type ?? 'undefined',
        parentId: n.parentId ?? null,
        children: Array.isArray(n.children) ? n.children : [],
        bbox: SceneGraph.parseBBox(n.bbox),
        propsRef: n.propsRef,
      })),
    };
  }

  private static parseBBox(theBBox: ISceneGraphJsonBBox | null | undefined): IBndBox3 | null {
    if (!theBBox || !theBBox.min || !theBBox.max) {
      return null;
    }
    const cornerMin: IVec3 = {x: theBBox.min[0], y: theBBox.min[1], z: theBBox.min[2]};
    const cornerMax: IVec3 = {x: theBBox.max[0], y: theBBox.max[1], z: theBBox.max[2]};
    return {cornerMin, cornerMax};
  }
}

