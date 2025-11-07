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
    BndBox3,
    ViewCorner, 
    UserEvents,
    ColorMapperChangedEvent,
    ColorScaleChangeStatus,
    CaeMeshPresentationType,
    ICaeMeshPropInfo} from '@hw/SCwebviewer/wasm32/SCwebviewer.js';
import { markRaw } from 'vue';
// import {} from 'crypto';

const PATH_TO_WASM = '/others/';

export class ViewerService {
    private _viewer?: SCwebviewer;

    get viewer(): SCwebviewer | undefined {
        return this._viewer;
    }

    constructor() {
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

        let aShape = new InteractiveShape(this.generateUUID());
        let isOk = await aShape.loadUrlAsync (this._viewer, theFileUrl);
        if (!isOk) {
            return "";
        }

        if (!this._viewer.unlinkFile("/" + aShape.id)) {
            throw new Error ("Cannot unlink a file " + "/" + aShape.id);
        }

        aShape.isVisible = true;
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

        aMesh.createPresentation(CaeMeshPresentationType.Scalar);
        aMesh.isVisible = true;
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

        aMesh.createPresentation(CaeMeshPresentationType.Scalar);
        aMesh.isVisible = true;
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

        if (toUpdate) {
            this._viewer.requestViewUpdate();
        }
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
