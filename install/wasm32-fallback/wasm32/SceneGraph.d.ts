import type { IBndBox3, ObjectId } from './CommonTools.js';
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
export interface SceneSearchResult {
    id: ObjectId;
    name: string;
    type: string;
    matchField: 'id' | 'name' | 'type';
}
export declare class SceneGraph {
    static parseJson(theJson: string): ISceneGraph | null;
    static search(theGraph: ISceneGraph | null, theQuery: string, theOptions?: {
        caseSensitive?: boolean;
        useRegex?: boolean;
    }): SceneSearchResult[];
    private static parseBBox;
}
