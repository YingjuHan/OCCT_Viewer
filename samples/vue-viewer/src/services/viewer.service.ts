import { SCwebviewer,
    ObjectId, 
    InteractiveObject, 
    InteractiveViewCube,
    MessageGravity,
    CameraOrientation,
    InteractiveShape,
    ColorTools,
    InteractiveCaeMesh,
    ShapeObjectPrsMode,
    CaeMeshObjectPrsMode,
    InteractiveColorScale,
    ColorScaleParams,
    IBndBox3,
    BndBox3,
    ViewCorner, 
    UserEvents,
    ColorMapperChangedEvent,
    ColorScaleChangeStatus,
    CaeMeshPresentationType,
    CaeMeshPresentation,
    ICaeMeshPropInfo,
    CaeMeshObjectSelMode,
    Subview,
    UnitManager,
    ViewerBuildConfig} from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import { MeasurementStore } from '../../../../source-ts/Viewer-ts/MeasurementStore';
import { SavedViewStore } from '../../../../source-ts/Viewer-ts/SavedViewStore';
import { ProjectStore } from '../../../../source-ts/Viewer-ts/ProjectStore';
import type { ProjectData, ProjectSource } from '../../../../source-ts/Viewer-ts/ProjectStore';
import { SceneGraph } from '../../../../source-ts/Viewer-ts/SceneGraph';
import type { SceneSearchResult } from '../../../../source-ts/Viewer-ts/SceneGraph';
import { markRaw } from 'vue';
// import {} from 'crypto';

const PATH_TO_WASM = '/others/';

export interface LocalLoadResult {
    fileName: string;
    success: boolean;
    format?: string;
    kind?: 'mesh' | 'shape' | 'mesh-properties';
    message?: string;
    objectId?: ObjectId;
}

export interface MeshSummary {
    id: ObjectId;
    label: string;
}

export class ViewerService {
    private _viewer?: SCwebviewer;
    private _shape?: InteractiveShape;
    private _mesh?: InteractiveCaeMesh;
    private _lastLoadedMeshId?: ObjectId;
    private _lastLoadedShapeId?: ObjectId;
    public measurementStore: MeasurementStore | null = null;
    public savedViewStore: SavedViewStore | null = null;
    public projectStore: ProjectStore | null = null;
    private _projectSources: ProjectSource[] = [];
    private _pendingProject: ProjectData | null = null;
    private _pendingMissing: ProjectSource[] = [];
    public toExplodeShape = false;
    public shapeExplodeDepth = -1;
    public useMeshGenGeom = false;
    public useMeshTestCacheGenGeom = false;
    public subview1?: Subview;
    public subview2?: Subview;
    private _readyPromise: Promise<void>;
    private _resolveReady?: () => void;

    get viewer(): SCwebviewer | undefined {
        return this._viewer;
    }

    get scViewer(): SCwebviewer | undefined {
        return this._viewer;
    }

    get unitManager(): UnitManager | null {
        return this._viewer?.unitManager ?? null;
    }

    public whenReady(): Promise<void> {
        return this._readyPromise;
    }

    constructor() {
        this._readyPromise = new Promise((resolve) => {
            this._resolveReady = resolve;
        });
        window.addEventListener("resize", this.onResize.bind(this));
    }

    public async createViewer(): Promise<void> {
        this._viewer = markRaw(new SCwebviewer(PATH_TO_WASM));
        await this._viewer.init();
        this._viewer.traceLevel = MessageGravity.Trace;

        if (this._viewer.viewCube != null) {
            this._viewer.viewCube.isVisible = true;
        }
        this._viewer.camera?.setOrientation (CameraOrientation.FRONT);

        this._viewer.eventManager.addEventListener(UserEvents.onColorMapperChanged, this.updateColorScale.bind(this));
        this.measurementStore = new MeasurementStore(this._viewer);
        this.savedViewStore = new SavedViewStore(this._viewer);
        this.projectStore = new ProjectStore(this._viewer);
        this._resolveReady?.();
    }

    public async loadLocalFiles(files: File[] | FileList): Promise<LocalLoadResult[]> {
        const normalized = Array.isArray(files) ? files : Array.from(files);
        const results: LocalLoadResult[] = [];
        for (const file of normalized) {
            results.push(await this.loadLocalFile(file));
        }
        return results;
    }

    public async loadLocalFile(file: File): Promise<LocalLoadResult> {
        if (this._viewer == null) {
            return { fileName: file.name, success: false, message: "Viewer is not ready" };
        }

        try {
            const buffer = await file.arrayBuffer();
            const meshFormat = InteractiveCaeMesh.probeFileFormat(this._viewer, buffer, file.name);
            const shapeFormat = InteractiveShape.probeFileFormat(this._viewer, buffer, file.name);

            if (meshFormat !== "") {
                if (meshFormat === "MDBPROP") {
                    const meshId = await this.applyMeshPropertiesBuffer(buffer);
                    return {
                        fileName: file.name,
                        success: true,
                        format: meshFormat,
                        kind: 'mesh-properties',
                        objectId: meshId,
                        message: "成功更新网格属性"
                    };
                }

                const meshId = await this.onMeshFileRead(file.name, buffer);
                return {
                    fileName: file.name,
                    success: true,
                    format: meshFormat,
                    kind: 'mesh',
                    objectId: meshId,
                    message: "网格模型加载成功"
                };
            }

            if (shapeFormat === "STEP" && !ViewerBuildConfig.useStep) {
                return {
                    fileName: file.name,
                    success: false,
                    format: shapeFormat,
                    kind: 'shape',
                    message: "STEP 构建未启用，建议使用服务端转换或开启 OCCTV_USE_STEP"
                };
            }

            if (shapeFormat === "IGES" && !ViewerBuildConfig.useIges) {
                return {
                    fileName: file.name,
                    success: false,
                    format: shapeFormat,
                    kind: 'shape',
                    message: "IGES 构建未启用，建议使用服务端转换或开启 OCCTV_USE_IGES"
                };
            }

            if (shapeFormat !== "") {
                const shapeId = await this.onShapeFileRead(file.name, buffer);
                return {
                    fileName: file.name,
                    success: true,
                    format: shapeFormat,
                    kind: 'shape',
                    objectId: shapeId,
                    message: "几何模型加载成功"
                };
            }

            return {
                fileName: file.name,
                success: false,
                message: "当前文件格式未被SCwebviewer识别"
            };
        } catch (error: any) {
            const message = error?.message ?? "本地文件加载失败";
            return { fileName: file.name, success: false, message };
        }
    }

    public async onShapeFileRead(fileName: string, buffer: ArrayBuffer): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        let filePath = "/models/" + fileName;
        const shape = new InteractiveShape();
        try {
            const isOpened = await shape.openBufferAsync(this._viewer, buffer, filePath);
            if (!isOpened) {
                throw new Error("无法打开几何文件");
            }

            this._shape = shape;
            const shapeId = shape.id;
            this._lastLoadedShapeId = shapeId;
            this.registerProjectSource({
                id: shapeId,
                kind: 'shape',
                sourceType: 'local',
                source: fileName
            });

            const isLoaded = await (shape as any).loadDataAsync(this._viewer);
            if (!isLoaded) {
                throw new Error("加载几何数据失败");
            }

            filePath = shape.getFilePathFromObjId();
            if (!this._viewer.unlinkFile(filePath)) {
                throw new Error("Cannot unlink a file '" + filePath + "'");
            }

            const explodedObjects = this.toExplodeShape
                ? shape.explodeShape(this._viewer, this.shapeExplodeDepth)
                : null;
            if (explodedObjects && explodedObjects.length > 0) {
                explodedObjects.forEach(obj => obj.isVisible = true);
                this.onLoad(explodedObjects[0]);
            } else {
                shape.isVisible = true;
                this.onLoad(shape);
            }

            return shapeId;
        } catch (error) {
            console.error("Loading shape failed: ", error);
            throw error;
        }
    }

    public async onMeshFileRead(fileName: string, buffer: ArrayBuffer): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const filePath = "/models/" + fileName;
        const mesh = new InteractiveCaeMesh();
        mesh.toCacheProperties = true;
        const isLoaded = await mesh.loadBufferAsync(this._viewer, buffer, filePath);
        if (!isLoaded) {
            throw new Error("加载网格失败");
        }

        this._mesh = mesh;
        this._lastLoadedMeshId = mesh.id;
        this.registerProjectSource({
            id: mesh.id,
            kind: 'mesh',
            sourceType: 'local',
            source: fileName
        });
        this.applyMeshGenerationOptions(mesh);
        mesh.isVisible = true;
        this.setMeshDefaultProps(mesh);
        this.onLoad(mesh);
        return mesh.id;
    }

    public setMeshDefaultProps(mesh: InteractiveCaeMesh): void {
        this.configureMeshAfterLoading(mesh);
    }

    public onLoad(object?: InteractiveObject): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.fitAllAuto(true);
        this._viewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
    }

    public getInteractiveObject(theObjectId: ObjectId): InteractiveObject | undefined {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjectId) {
                return obj;
            }
        }
    }

    public getSelectedShape(): InteractiveShape | null {
        const obj = this._viewer?.selectionManager?.oneSelectedObject;
        return obj instanceof InteractiveShape ? obj : null;
    }

    public getSelectedMesh(): InteractiveCaeMesh | null {
        const obj = this._viewer?.selectionManager?.oneSelectedObject;
        return obj instanceof InteractiveCaeMesh ? obj : null;
    }

    public getMassPropsJson(theObjectId: ObjectId): string {
        if (this._viewer == null || this._viewer.interactiveShapeManager == null) {
            return '';
        }
        return this._viewer.interactiveShapeManager.getMassPropsJson(theObjectId) ?? '';
    }

    public getSceneGraph(theIdPrefix: string = ''): any | null {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return null;
        }

        const ctx: any = this._viewer.interactiveContext as any;
        const fn = ctx?.getSceneGraph;
        if (typeof fn !== 'function') {
            return null;
        }
        return fn.call(ctx, theIdPrefix);
    }

    public searchScene(theQuery: string, theOptions?: { caseSensitive?: boolean; useRegex?: boolean }): SceneSearchResult[] {
        const graph = this.getSceneGraph();
        if (graph) {
            return SceneGraph.search(graph, theQuery, theOptions);
        }

        if (!theQuery || !this._viewer?.interactiveContext) {
            return [];
        }

        const query = (theOptions?.caseSensitive ? theQuery : theQuery.toLowerCase());
        const results: SceneSearchResult[] = [];
        for (const obj of this._viewer.interactiveContext.objectsInside) {
            const id = obj.id ?? '';
            const name = id;
            const value = theOptions?.caseSensitive ? id : id.toLowerCase();
            if (value.includes(query)) {
                results.push({ id, name, type: obj.constructor?.name ?? 'Object', matchField: 'id' });
            }
        }
        return results;
    }

    public selectObjectById(theObjectId: ObjectId): void {
        if (this._viewer == null || this._viewer.interactiveContext == null || this._viewer.selectionManager == null) {
            return;
        }

        const obj = this.getInteractiveObject(theObjectId);
        this._viewer.selectionManager.oneSelectedObject = obj ?? null;
        this._viewer.requestViewUpdate();
    }

    public setObjectVisible(theObjectId: ObjectId, theVisible: boolean): void {
        const obj = this.getInteractiveObject(theObjectId);
        if (obj == null || this._viewer == null) {
            return;
        }

        obj.isVisible = theVisible;
        this._viewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
    }

    public showAllObjects(): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (const obj of this._viewer.interactiveContext.objectsInside) {
            obj.isVisible = true;
        }

        this._viewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
    }

    public isolateObjectIds(theObjectIds: ObjectId[]): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        const visibleSet = new Set(theObjectIds);
        for (const obj of this._viewer.interactiveContext.objectsInside) {
            obj.isVisible = visibleSet.has(obj.id);
        }

        this._viewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
    }

    public toggleViewCube(): void {
        if (this._viewer != null && this._viewer.viewCube != null ) {
            this._viewer.viewCube.isVisible = !this._viewer.viewCube.isVisible;
            this._viewer.requestViewUpdate();
        }
    }

    public fitAll(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public fitToBBox(theBox: IBndBox3, toUpdate: boolean = true): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }
        if (theBox == null || BndBox3.isVoid(theBox)) {
            return;
        }

        this._viewer.camera.fitMinMax(theBox, true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setTopView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.TOP);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setBottomView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.BOTTOM);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setLeftView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.LEFT);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setRightView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.RIGHT);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setFrontView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.FRONT);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setBackView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.BACK);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setAxoView(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.camera == null) {
            return;
        }

        this._viewer.camera.setOrientation (CameraOrientation.AXORIGHT);
        this._viewer.camera.fitAllAuto (true);
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public getBackgroundColor(): { r: number, g: number, b: number} {
        if (this._viewer == null || this._viewer.renderingParams == null) {
            return {r: 0, g: 0, b: 0};
        }

        return ColorTools.convertColorStringToRgb(this._viewer.renderingParams.backgroundColor);
    }

    public setBackgroundColor(color: { r: number, g: number, b: number}, toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.renderingParams == null) {
            return;
        }

        let colorStr = ColorTools.convertRgbToColorString(color);
        this._viewer.renderingParams.backgroundColor = colorStr;
        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public requestViewUpdate(): void {
        if (this._viewer == null) {
            return;
        }

        this._viewer.requestViewUpdate();
    }

    public async loadShapeUrlAsync(theFileUrl: string): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const lowerUrl = theFileUrl.toLowerCase();
        if ((lowerUrl.endsWith('.step') || lowerUrl.endsWith('.stp')) && !ViewerBuildConfig.useStep) {
            throw new Error("STEP 构建未启用，无法直接加载该文件");
        }
        if ((lowerUrl.endsWith('.iges') || lowerUrl.endsWith('.igs')) && !ViewerBuildConfig.useIges) {
            throw new Error("IGES 构建未启用，无法直接加载该文件");
        }

        let aShape = new InteractiveShape(this.generateUUID());
        let isOk = await aShape.loadUrlAsync (this._viewer, theFileUrl);
        if (!isOk) {
            return "";
        }

        if (!this._viewer.unlinkFile("/" + aShape.id)) {
            throw new Error ("Cannot unlink a file " + "/" + aShape.id);
        }

        aShape.isVisible = true;
        this._shape = aShape;
        this._lastLoadedShapeId = aShape.id;
        this.registerProjectSource({
            id: aShape.id,
            kind: 'shape',
            sourceType: 'url',
            source: theFileUrl
        });
        return aShape.id;
    }

    public async loadShapeBufferAsync(theArray: ArrayBuffer): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        let aShape = new InteractiveShape (this.generateUUID());
        let isOk = await aShape.loadBufferAsync (this._viewer, theArray);
        if (!isOk) {
            return "";
        }

        if (!this._viewer.unlinkFile("/" + aShape.id)) {
            throw new Error ("Cannot unlink a file " + "/" + aShape.id);
        }

        aShape.isVisible = true;
        this._shape = aShape;
        this._lastLoadedShapeId = aShape.id;
        return aShape.id;
    }

    public async loadMeshUrlAsync(theFileUrl: string): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        let aMesh = new InteractiveCaeMesh(this.generateUUID());
        let isOk = await aMesh.loadUrlAsync (this._viewer, theFileUrl);
        if (!isOk) {
            return "";
        }

        this.applyMeshGenerationOptions(aMesh);
        this.configureMeshAfterLoading(aMesh);
        aMesh.isVisible = true;
        this._mesh = aMesh;
        this._lastLoadedMeshId = aMesh.id;
        this.registerProjectSource({
            id: aMesh.id,
            kind: 'mesh',
            sourceType: 'url',
            source: theFileUrl
        });
        return aMesh.id;
    }

    public async loadMeshBufferAsync(theArray: ArrayBuffer): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        let aMesh = new InteractiveCaeMesh (this.generateUUID());
        let isOk = await aMesh.loadBufferAsync (this._viewer, theArray, "");
        if (!isOk) {
            return "";
        }

        this.applyMeshGenerationOptions(aMesh);
        this.configureMeshAfterLoading(aMesh);
        aMesh.isVisible = true;
        this._mesh = aMesh;
        this._lastLoadedMeshId = aMesh.id;
        return aMesh.id;
    }

    public setShapesDisplayMode(theMode: ShapeObjectPrsMode, toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj instanceof InteractiveShape) {
                (obj as InteractiveShape).presentationMode = theMode;
            }
        }

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public setMeshesDisplayMode(theMode: CaeMeshObjectPrsMode, toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj instanceof InteractiveCaeMesh) {
                (obj as InteractiveCaeMesh).presentations[0].presentationMode = theMode;
            }
        }

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public getMeshProperties(theObjectId: ObjectId): Array<ICaeMeshPropInfo> {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return [];
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjectId && obj instanceof InteractiveCaeMesh) {
                return (obj as InteractiveCaeMesh).properties;
            }
        }

        return [];
    }

    public listMeshSummaries(): MeshSummary[] {
        const result: MeshSummary[] = [];
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return result;
        }
        for (const obj of this._viewer.interactiveContext.objectsInside) {
            if (obj instanceof InteractiveCaeMesh) {
                const label = (obj as any)?.metadata?.name ?? (obj as any)?.label ?? obj.id;
                result.push({ id: obj.id, label });
            }
        }
        return result;
    }

    public setActiveMeshProperty(theObjectId: ObjectId, thePropertyId: string, toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjectId && obj instanceof InteractiveCaeMesh) {
                (obj as InteractiveCaeMesh).presentations[0].activeProperty = thePropertyId;
                if (toUpdate) {
                    this._viewer.requestViewUpdate();
                }
                return;
            }
        }
    }

    private getMeshById(theObjectId: ObjectId): InteractiveCaeMesh | null {
        let aMesh: InteractiveCaeMesh | null = null;
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return aMesh;
        }
        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjectId && obj instanceof InteractiveCaeMesh) {
                aMesh = obj as InteractiveCaeMesh;
                break;
            }
        }
        return aMesh;
    }

    public getMeshElementsDisplayingStatus(theObjectId: ObjectId): {elements3d: boolean, elements2d: boolean, nodes: boolean} {
        let aRes = {elements3d: false, elements2d: false, nodes: false};
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }

        aRes.elements3d = aMesh.presentations[0].toDisplay3dElements;
        aRes.elements2d = aMesh.presentations[0].toDisplay2dElements;
        aRes.nodes = aMesh.presentations[0].toDisplayNodes;

        return aRes;
    }

    public setMeshElementsDisplayingStatus(theObjectId: ObjectId, status: {elements3d?: boolean, elements2d?: boolean, nodes?: boolean}) {
        if (this._viewer == null) {
            return;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return;
        }

        if (status.elements3d != undefined) {
            aMesh.presentations[0].toDisplay3dElements = status.elements3d;
        }
        if (status.elements2d != undefined) {
            aMesh.presentations[0].toDisplay2dElements = status.elements2d;
        }
        if (status.nodes != undefined) {
            aMesh.presentations[0].toDisplayNodes = status.nodes;
        }
    }

    public getMeshBoxSize(theObjectId: ObjectId): {x: number, y: number, z: number} {
        let aRes = {x: 0, y: 0, z: 0};
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }

        aRes = BndBox3.diagonal(aMesh.boundingBox);
        return aRes;
    }

    public getMeshPropertyValueRange(theObjectId: ObjectId, thePropertyId: string, theComponentIndex: number): {x: number, y: number} {
        let aRes = {x: 0, y: 0};
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }
        
        return aMesh.getPropertyValueRange(thePropertyId, theComponentIndex);
    }

    public getMeshPropertyName(theObjectId: ObjectId, thePropertyId: string): string {
        if (this._viewer == null) {
            return  "";
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return "";
        }
        
        return aMesh.getPropertyName(thePropertyId);
    }

    public getMeshAvailableVecProps(theObjectId: ObjectId): {id: string, name: string}[] {
        let aRes = new Array<{id: string, name: string}>();
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }
        
        return aMesh.availableVec3Props;
    }

    public getMeshActiveDefProp(theObjectId: ObjectId): string {
        let aRes = "";
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }
        
        return aMesh.presentations[0].activeDeformedProp;
    }

    public setMeshActiveDefProp(theObjectId: ObjectId, thePropertyId: string) {
        if (this._viewer == null) {
            return;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return;
        }
        
        aMesh.presentations[0].activeDeformedProp = thePropertyId;
    }

    public getMeshDefScaleCoeff(theObjectId: ObjectId): number {
        let aRes = 0;
        if (this._viewer == null) {
            return aRes;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return aRes;
        }
        
        return aMesh.presentations[0].deformedScaleCoeff;
    }

    public setMeshDefScaleCoeff(theObjectId: ObjectId, value: number) {
        if (this._viewer == null) {
            return;
        }

        let aMesh = this.getMeshById(theObjectId);

        if (aMesh == null) {
            console.error("Can't find a mesh " + theObjectId);
            return;
        }
        
        aMesh.presentations[0].deformedScaleCoeff = value;
    }

    private applyMeshGenerationOptions(mesh: InteractiveCaeMesh): void {
        if (this.useMeshGenGeom) {
            mesh.setUseGenGeometry();
        }
        if (this.useMeshTestCacheGenGeom) {
            mesh.setUseTestCacheGenGeometry();
        }
    }

    private configureMeshAfterLoading(mesh: InteractiveCaeMesh): void {
        let presentation: CaeMeshPresentation | null = mesh.presentations.length > 0 ? mesh.presentations[0] : null;
        if (presentation == null) {
            presentation = mesh.createPresentation(CaeMeshPresentationType.Scalar);
        }
        if (presentation == null) {
            return;
        }

        let properties = mesh.properties;
        if (properties.length > 0) {
            presentation.activeProperty = properties[0].propId;
            let timeSteps = mesh.availableTimeSteps;
            if (timeSteps.length > 0) {
                mesh.activeTimeStep = timeSteps[0].timeStep;
            }
        }

        presentation.presentationMode = CaeMeshObjectPrsMode.FlatShaded;
        mesh.selectionMode = CaeMeshObjectSelMode.Elements | CaeMeshObjectSelMode.Nodes;
    }

    private async applyMeshPropertiesBuffer(theArray: ArrayBuffer): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        let targetMesh = this.getLastLoadedMesh();
        if (targetMesh == null) {
            throw new Error("请先加载一个网格模型，再导入属性文件");
        }

        await targetMesh.loadPropertiesAsync(this._viewer, theArray);
        this.configureMeshAfterLoading(targetMesh);
        this._viewer.requestViewUpdate();
        this._lastLoadedMeshId = targetMesh.id;
        this._mesh = targetMesh;
        return targetMesh.id;
    }

    private getLastLoadedMesh(): InteractiveCaeMesh | null {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return null;
        }

        if (this._mesh) {
            return this._mesh;
        }

        if (this._lastLoadedMeshId) {
            const mesh = this.getMeshById(this._lastLoadedMeshId);
            if (mesh != null) {
                return mesh;
            }
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj instanceof InteractiveCaeMesh) {
                return obj as InteractiveCaeMesh;
            }
        }

        return null;
    }

    public updateColorScale(theEvent: Event): void {
        if (this._viewer == null) {
            return;
        }

        let anEvent = theEvent as ColorMapperChangedEvent;
        if (anEvent == null) {
            return;
        }

        if (anEvent.meshId === "" || anEvent.mesh == null) {
            return;
        }

        if (anEvent.changeStatus == ColorScaleChangeStatus.Create) {
            let aColorScale = new InteractiveColorScale(this._viewer, this.generateUUID());
            aColorScale.attach(anEvent.meshId, anEvent.propertyId, anEvent.componentIndex);

            aColorScale.viewCorner = ViewCorner.RIGHT_CENTER;
            aColorScale.isVisible = true;

            this.onResize();

            // Two ways to set color scale parameters

            // 1 - preferred if is needed to set a bunch of parameters at once
            let aParams = aColorScale.params;
            aParams.isSmooth = true;
            aParams.isCustomColors = true;
            aParams.nbIntervals = 3;
            aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 0, g: 0, b: 255}));
            aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 0, g: 255, b: 0}));
            aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 255, g: 0, b: 0}));
            aParams.labelSize = 12;
            aColorScale.params = aParams;

            // or 2
            // aColorScale.isSmooth = true;
            // aColorScale.nbIntervals = 3;
            // aColorScale.customColors = [ColorTools.convertRgbToColorString ({r: 0, g: 0, b: 255}),
            //                             ColorTools.convertRgbToColorString ({r: 0, g: 255, b: 0}),
            //                             ColorTools.convertRgbToColorString ({r: 255, g: 0, b: 0})];
            // aColorScale.labelSize = 12;

            this._viewer.viewManager?.invalidate2dObjects(aColorScale.id);
        }
        else {
            // Color scale property was updated
            if (anEvent.propertyId !== "" && anEvent.colorScale != null) {
                let aParams = anEvent.colorScale.params;
                if (anEvent.mesh.isPropertyGroup(anEvent.propertyId)) {
                    aParams.isSmooth = false;
                } else {
                    aParams.isSmooth = true;
                    aParams.isCustomColors = true;
                    aParams.nbIntervals = 3;
                    aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 0, g: 0, b: 255}));
                    aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 0, g: 255, b: 0}));
                    aParams.customColors.push_back(ColorTools.convertRgbToColorString ({r: 255, g: 0, b: 0}));
                }
                anEvent.colorScale.params = aParams;
            }
        }
    }

    public getColorScaleParams(theObjId: ObjectId): ColorScaleParams | undefined {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjId && obj instanceof InteractiveColorScale) {
                return (obj as InteractiveColorScale).params;
            }
        }

        return;
    }

    public setColorScaleParams(theObjId: ObjectId, theParams: ColorScaleParams, toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj.id == theObjId && obj instanceof InteractiveColorScale) {
                (obj as InteractiveColorScale).params = theParams;
            }
        }

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public showObjectsByType(typeOfObjects: any) {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return 0;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (obj instanceof typeOfObjects) {
                obj.isVisible = true;
            }
        }

        this._viewer.requestViewUpdate();
    }

    public hideAllObjects() {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return 0;
        }

        for (let obj of this._viewer.interactiveContext.objectsInside) {
            obj.isVisible = false;
        }

        this._viewer.requestViewUpdate();
    }

    public removeAll(toUpdate: boolean): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        this._viewer.interactiveContext.removeAll();

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public removeObjects(objects: InteractiveObject[]): void {
        if (this._viewer == null) {
            return;
        }
        for (let obj of objects) {
            obj.remove();
        }
    }

    public removeObjectsById(objectsIds: ObjectId[]): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }
        let objects = new Array<InteractiveObject>();
        for (let obj of this._viewer.interactiveContext.objectsInside) {
            if (objectsIds.indexOf(obj.id) != -1) {
                objects.push(obj);
            }
        }
        this.removeObjects(objects);
    }

    public removeObjectsByType(whiteList?: any[], blackList?: any[]): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        let objects = new Array<InteractiveObject>();
        for (let obj of this._viewer.interactiveContext.objectsInside) {
            let toRemove = true;
            if (whiteList != null) {
                toRemove = false;
                for (let objType of whiteList) {
                    if (obj instanceof objType) {
                        toRemove = true;
                        break;
                    }
                }
            }
            if (blackList != null) {
                for (let objType of blackList) {
                    if (obj instanceof objType) {
                        toRemove = false;
                        break;
                    }
                }
            }
            if (toRemove) {
                objects.push(obj);
            }
        }
        this.removeObjects(objects);
    }

    public clearScene(toUpdate: boolean): void {
        if (this._viewer == null) {
            return;
        }

        this.removeObjectsByType(undefined, [InteractiveViewCube]);
        this.measurementStore?.clear();
        this.savedViewStore?.clear();
        this._projectSources = [];
        this._pendingProject = null;
        this._pendingMissing = [];

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
    }

    public exportProject(): string {
        if (this._viewer == null || this.projectStore == null) {
            return "";
        }

        const safeParse = (text: string): any | null => {
            if (!text) {
                return null;
            }
            try {
                return JSON.parse(text);
            } catch {
                return null;
            }
        };

        const savedViews = this.savedViewStore ? safeParse(this.savedViewStore.exportJson()) : null;
        const measurements = this.measurementStore ? safeParse(this.measurementStore.exportJson()) : null;

        return this.projectStore.exportJson({
            sources: this._projectSources,
            savedViews,
            measurements
        });
    }

    public async importProject(theJson: string): Promise<{ project: ProjectData | null; missingSources: ProjectSource[]; message?: string }> {
        if (this.projectStore == null || this._viewer == null) {
            return { project: null, missingSources: [], message: "Viewer 未就绪" };
        }

        const parsed = this.projectStore.importJson(theJson);
        if (!parsed) {
            return { project: null, missingSources: [], message: "Project JSON 无法解析" };
        }

        this.clearScene(true);
        this._pendingProject = parsed.project;
        this._pendingMissing = parsed.missingSources.filter((src) => src.sourceType !== 'url');

        const urlSources = parsed.project.sources.filter((src) => src.sourceType === 'url');
        for (const src of urlSources) {
            await this.loadProjectSource(src);
        }

        if (this._pendingMissing.length === 0) {
            this.finalizeProjectImport();
        }

        return { project: parsed.project, missingSources: [...this._pendingMissing] };
    }

    public async rebindProjectSource(theSourceId: ObjectId, theFile: File): Promise<boolean> {
        if (this._viewer == null) {
            return false;
        }

        const idx = this._pendingMissing.findIndex((src) => src.id === theSourceId);
        if (idx < 0) {
            return false;
        }

        const src = this._pendingMissing[idx];
        const buffer = await theFile.arrayBuffer();
        if (src.kind === 'shape') {
            await this.loadShapeBufferWithId(src.id, theFile.name, buffer, src.sourceType);
        } else {
            await this.loadMeshBufferWithId(src.id, theFile.name, buffer, src.sourceType);
        }

        this._pendingMissing.splice(idx, 1);
        if (this._pendingMissing.length === 0) {
            this.finalizeProjectImport();
        }
        return true;
    }

    private async loadProjectSource(src: ProjectSource): Promise<ObjectId> {
        if (src.kind === 'shape') {
            return this.loadShapeUrlWithId(src.id, src.source);
        }
        return this.loadMeshUrlWithId(src.id, src.source);
    }

    private finalizeProjectImport(): void {
        if (this._pendingProject == null) {
            return;
        }

        this.applyProjectStyles(this._pendingProject.styles);

        if (this.savedViewStore && this._pendingProject.savedViews) {
            this.savedViewStore.importJson(JSON.stringify(this._pendingProject.savedViews));
        }
        if (this.measurementStore && this._pendingProject.measurements) {
            this.measurementStore.importJson(JSON.stringify(this._pendingProject.measurements));
        }

        this._pendingProject = null;
        this._pendingMissing = [];
        this._viewer?.requestViewUpdate();
    }

    private applyProjectStyles(theStyles: ProjectData['styles']): void {
        if (!Array.isArray(theStyles)) {
            return;
        }

        for (const style of theStyles) {
            const obj = this.getInteractiveObject(style.id);
            if (!obj) {
                continue;
            }
            if (typeof style.visible === 'boolean') {
                obj.isVisible = style.visible;
            }
            if (typeof style.baseColor === 'string') {
                obj.baseColor = style.baseColor;
            }
            if (typeof style.transparency === 'number') {
                obj.transparency = style.transparency;
            }
        }
    }

    private registerProjectSource(theSource: ProjectSource): void {
        const idx = this._projectSources.findIndex((src) => src.id === theSource.id);
        if (idx >= 0) {
            this._projectSources[idx] = theSource;
            return;
        }
        this._projectSources.push(theSource);
    }

    private async loadShapeUrlWithId(theObjId: ObjectId, theUrl: string): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const lowerUrl = theUrl.toLowerCase();
        if ((lowerUrl.endsWith('.step') || lowerUrl.endsWith('.stp')) && !ViewerBuildConfig.useStep) {
            throw new Error("STEP 构建未启用，无法直接加载该文件");
        }
        if ((lowerUrl.endsWith('.iges') || lowerUrl.endsWith('.igs')) && !ViewerBuildConfig.useIges) {
            throw new Error("IGES 构建未启用，无法直接加载该文件");
        }

        const shape = new InteractiveShape(theObjId);
        const isOk = await shape.loadUrlAsync(this._viewer, theUrl);
        if (!isOk) {
            return "";
        }

        const filePath = shape.getFilePathFromObjId();
        if (!this._viewer.unlinkFile(filePath)) {
            throw new Error("Cannot unlink a file '" + filePath + "'");
        }

        shape.isVisible = true;
        this._shape = shape;
        this._lastLoadedShapeId = shape.id;
        this.registerProjectSource({
            id: shape.id,
            kind: 'shape',
            sourceType: 'url',
            source: theUrl
        });
        return shape.id;
    }

    private async loadMeshUrlWithId(theObjId: ObjectId, theUrl: string): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const mesh = new InteractiveCaeMesh(theObjId);
        const isOk = await mesh.loadUrlAsync(this._viewer, theUrl);
        if (!isOk) {
            return "";
        }

        this.applyMeshGenerationOptions(mesh);
        this.configureMeshAfterLoading(mesh);
        mesh.isVisible = true;
        this._mesh = mesh;
        this._lastLoadedMeshId = mesh.id;
        this.registerProjectSource({
            id: mesh.id,
            kind: 'mesh',
            sourceType: 'url',
            source: theUrl
        });
        return mesh.id;
    }

    private async loadShapeBufferWithId(theObjId: ObjectId, theFileName: string, theBuffer: ArrayBuffer, theSourceType: ProjectSource['sourceType']): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const shape = new InteractiveShape(theObjId);
        const isOk = await shape.loadBufferAsync(this._viewer, theBuffer, theFileName);
        if (!isOk) {
            return "";
        }

        const filePath = shape.getFilePathFromObjId();
        if (!this._viewer.unlinkFile(filePath)) {
            throw new Error("Cannot unlink a file '" + filePath + "'");
        }

        shape.isVisible = true;
        this._shape = shape;
        this._lastLoadedShapeId = shape.id;
        this.registerProjectSource({
            id: shape.id,
            kind: 'shape',
            sourceType: theSourceType,
            source: theFileName
        });
        return shape.id;
    }

    private async loadMeshBufferWithId(theObjId: ObjectId, theFileName: string, theBuffer: ArrayBuffer, theSourceType: ProjectSource['sourceType']): Promise<ObjectId> {
        if (this._viewer == null) {
            return "";
        }

        const mesh = new InteractiveCaeMesh(theObjId);
        mesh.toCacheProperties = true;
        const isOk = await mesh.loadBufferAsync(this._viewer, theBuffer, theFileName);
        if (!isOk) {
            return "";
        }

        this.applyMeshGenerationOptions(mesh);
        this.configureMeshAfterLoading(mesh);
        mesh.isVisible = true;
        this._mesh = mesh;
        this._lastLoadedMeshId = mesh.id;
        this.registerProjectSource({
            id: mesh.id,
            kind: 'mesh',
            sourceType: theSourceType,
            source: theFileName
        });
        return mesh.id;
    }

    public onResize(): void {
        if (this._viewer == null || this._viewer.interactiveContext == null) {
            return;
        }

        let aCanvas = document.getElementById('ViewerCanvas') as HTMLCanvasElement;
        if (aCanvas != null) {
            aCanvas.style.width = `100%`;
            aCanvas.style.height = `100%`;
            let aDevicePixelRatio = window.devicePixelRatio || 1;
            aCanvas.width  = aCanvas.offsetWidth * aDevicePixelRatio;
            aCanvas.height = aCanvas.offsetHeight * aDevicePixelRatio;
        }
        this._viewer.resizeViewport();

        this._viewer.requestViewUpdate();
    }

    private generateUUID(): ObjectId {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
            const r = Math.random() * 16 | 0;
            const v = c === 'x' ? r : (r & 0x3 | 0x8); // y 位固定为 8-11 之间的值
            return v.toString(16);
        });
    }
}
