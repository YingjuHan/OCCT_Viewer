import { Camera } from './Camera.js';
import { UnitManagerExtensions } from './CommonTools.js';
import { InteractiveContextExtensions, SelectionManagerExtensions } from './InteractiveObject.js';
import { InteractiveShapeManagerExtensions } from './InteractiveShape.js';
import { InteractiveScaleRuler } from './InteractiveScaleRuler.js';
import { InteractiveViewCube } from './InteractiveViewCube.js';
import { ViewerEventManager } from './ViewerEventManager.js';
import { Subview, ViewManagerExtensions } from './Subview.js';
import { ViewerBuildConfig } from './ViewerBuildConfig.js';
export * from './Camera.js';
export * from './CommonTools.js';
export * from './InteractiveObject.js';
export * from './InteractiveColorScale.js';
export * from './InteractiveShape.js';
export * from './InteractiveCaeMesh.js';
export * from './InteractiveClipping.js';
export * from './InteractiveScaleRuler.js';
export * from './InteractiveTextLabel.js';
export * from './InteractiveDimension.js';
export * from './InteractiveViewCube.js';
export * from './InteractiveSphericalAxes.js';
export * from './ViewerEventManager.js';
export * from './ViewerEventTarget.js';
export * from './ViewManager.js';
export * from './Subview.js';
/**
 * This enumeration defines gravity level of messages.
 * It reflects values of corresponding C++ `Message_Gravity` enumeration defined by OCCT,
 * and used to redirect message coming from C++ algorithms via {@link SCwebviewer.printMessage} callback.
 * See also {@link SCwebviewer.traceLevel} defining which messages should be delivered.
 */
export var MessageGravity;
(function (MessageGravity) {
    /** Low-level details on algorithm execution (usually for debug purposes). */
    MessageGravity[MessageGravity["Trace"] = 0] = "Trace";
    /** Informative message. */
    MessageGravity[MessageGravity["Info"] = 1] = "Info";
    /** Warning message. */
    MessageGravity[MessageGravity["Warning"] = 2] = "Warning";
    /** Non-critical error. */
    MessageGravity[MessageGravity["Alarm"] = 3] = "Alarm";
    /** Fatal error. */
    MessageGravity[MessageGravity["Fail"] = 4] = "Fail";
})(MessageGravity || (MessageGravity = {}));
;
/**
 * This enumeration defines rendering transparency method.
 * It reflects values of corresponding C++ `Graphic3d_RenderTransparentMethod` enumeration defined by OCCT.
 */
export var RenderTransparentMethod;
(function (RenderTransparentMethod) {
    /** Basic blend transparency with non-commuting blend operator without sorting. */
    RenderTransparentMethod[RenderTransparentMethod["Unordered"] = 0] = "Unordered";
    /** Weighted Blended Order-Independent Transparency. */
    RenderTransparentMethod[RenderTransparentMethod["BlendOit"] = 1] = "BlendOit";
    /** Depth Peeling. */
    RenderTransparentMethod[RenderTransparentMethod["DepthPeelingOit"] = 2] = "DepthPeelingOit";
})(RenderTransparentMethod || (RenderTransparentMethod = {}));
;
;
/** @internal Managers enumeration. */
var ManagerType;
(function (ManagerType) {
    ManagerType[ManagerType["UNIT_MANAGER"] = 0] = "UNIT_MANAGER";
    ManagerType[ManagerType["VIEW_MANAGER"] = 1] = "VIEW_MANAGER";
    ManagerType[ManagerType["RENDERING_PARAMS"] = 2] = "RENDERING_PARAMS";
    ManagerType[ManagerType["INTERACTIVE_CONTEXT"] = 3] = "INTERACTIVE_CONTEXT";
    ManagerType[ManagerType["SELECTION_MANAGER"] = 4] = "SELECTION_MANAGER";
    ManagerType[ManagerType["ASPECTS_MANAGER"] = 5] = "ASPECTS_MANAGER";
    ManagerType[ManagerType["CAEMESH_MANAGER"] = 6] = "CAEMESH_MANAGER";
    ManagerType[ManagerType["SHAPE_MANAGER"] = 7] = "SHAPE_MANAGER";
    ManagerType[ManagerType["COLORSCALE_MANAGER"] = 8] = "COLORSCALE_MANAGER";
    ManagerType[ManagerType["CLIPPING_MANAGER"] = 9] = "CLIPPING_MANAGER";
    ManagerType[ManagerType["SCALERULER_MANAGER"] = 10] = "SCALERULER_MANAGER";
    ManagerType[ManagerType["TEXTLABEL_MANAGER"] = 11] = "TEXTLABEL_MANAGER";
    ManagerType[ManagerType["DIMENSION_MANAGER"] = 12] = "DIMENSION_MANAGER";
    ManagerType[ManagerType["VIEWCUBE_MANAGER"] = 13] = "VIEWCUBE_MANAGER";
    ManagerType[ManagerType["AXES_MANAGER"] = 14] = "AXES_MANAGER";
    ManagerType[ManagerType["NB_MANAGERS"] = 15] = "NB_MANAGERS";
})(ManagerType || (ManagerType = {}));
;
;
/**
 * Base class for {@link SCwebviewer} Module object.
 * For convenience, this class re-exports interface to the main Viewer.
 *
 * Hello World usage sample:
 * ```typescript
 *   // Somewhere in HTML - add 'canvas' element
 *   // with id 'ViewerCanvas' holding 3D viewer content.
 *   // <canvas id="ViewerCanvas" oncontextmenu="event.preventDefault()" tabindex=-1
 *   //         style="border:0 none;background-color:#000; user-select: none;"
 *   //         width="409" height="409">
 *   // </canvas>
 *
 *   // library imports
 *   import * as SCwebviewer from '/wasm32/SCwebviewer.js';
 *
 *   // create viewer object and specify prefix for loading extra files
 *   var scViewer = new SCwebviewer.SCwebviewer('/');
 *   // setup log verbose level BEFORE initialization
 *   scViewer.traceLevel = SCwebviewer.MessageGravity.Trace;
 *   // load WASM, initialize WebGL context and 3D Viewer
 *   scViewer.init().then (() => {
 *     // adjust rendering parameters
 *     scViewer.renderingParams.backgroundColor = "black";
 *     scViewer.viewCube.isVisible = true;
 *     scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.FRONT);
 *     // load some models
 *     let aShape = new SCwebviewer.InteractiveShape();
 *       aShape.loadUrlAsync(scViewer, "/models/shape.stl").then(() => {
 *       aShape.isVisible = true; // display AFTER loading completed
 *       scViewer.camera.fitAllAuto(true);
 *       scViewer.requestViewUpdate(); // should be added to redraw Viewer contents
 *     })
 *   })
 * ```
 */
export class SCwebviewer {
    /**
     * Main constructor.
     * @param {string} theBaseDataPath path to folder that contains
     *        'wasm32' + 'wasm32-pthread' folders (WASM files),
     *        'fonts' folder with default font files.
     */
    constructor(theBaseDataPath) {
        this._myImageMap = new Map();
        this._myImageCanvas = null;
        this._myToTraceImages = true;
        this._myTraceLevel = MessageGravity.Trace.valueOf();
        this._myEventManager = new ViewerEventManager(this);
        this._myWasmViewer = null;
        this._myUnitManager = null;
        this._myRenderParams = null;
        this._myViewManager = null;
        this._mySelectionManager = null;
        this._myInteractiveContext = null;
        this._myInteractiveAspectsManager = null;
        this._myInteractiveCaeMeshManager = null;
        this._myInteractiveShapeManager = null;
        this._myInteractiveColorScaleManager = null;
        this._myInteractiveClippingManager = null;
        this._myInteractiveScaleRulerManager = null;
        this._myInteractiveTextLabelManager = null;
        this._myInteractiveDimensionManager = null;
        this._myInteractiveViewCubeManager = null;
        this._myInteractiveAxesManager = null;
        this._myRootView = null;
        this._myFocusCamera = null;
        this._myFocusView = null;
        this._myViewCube = null;
        // prefix for *.data file location
        this._myBaseFolder = theBaseDataPath;
        this._myIsWasmLoaded = false;
        this._myToPreferPthread = true;
        this._myIsPthreadLoaded = false;
        // define WebGL canvas for WebAssembly viewer
        this._myCanvasId = 'ViewerCanvas';
        this.canvas = document.getElementById(this._myCanvasId);
        this.canvas.tabIndex = -1;
        this.canvas.onclick = (theEvent) => { this.canvas.focus(); };
        const aViewer = this;
        Object.defineProperty(this.canvas, 'scViewer', { get: function () { return aViewer; } });
        /* @code create WebGL context in advance with adjusted properties
        let aCtxProps = { alpha: false, depth: true, antialias: false, preserveDrawingBuffer: true };
        let aGlCtx =                   this.canvas.getContext('webgl2', aCtxProps );
        if (aGlCtx == null) { aGlCtx = this.canvas.getContext('webgl',  aCtxProps ); } @endcode */
        // tell Emscripten and Draw Harness to not use std::cin for commands input
        this.noExitRuntime = true;
        // bind WebAssembly callbacks to this context
        this.print = this.print.bind(this);
        this.printErr = this.printErr.bind(this);
        this.printMessage = this.printMessage.bind(this);
        this.locateFile = this.locateFile.bind(this);
        this.onRuntimeInitialized = this.onRuntimeInitialized.bind(this);
    }
    /** Check if WASM module has been loaded. */
    get isWasmLoaded() { return this._myIsWasmLoaded; }
    /** Return TRUE if pthread version should be loaded when supported and allowed by Browser. */
    get toPreferPthread() { return this._myToPreferPthread; }
    /** Check if pthread version of viewer is loaded. */
    get isPthreadLoaded() { return this._myIsPthreadLoaded; }
    /**
     * Get gravity level for logging messages.
     * @return {MessageGravity} lower message gravity to log
     */
    get traceLevel() { return this._myTraceLevel; }
    /** Flag to enable extra verbose logging. */
    get toTraceImages() { return this._myToTraceImages; }
    /**
     * Get subview manager.
     * @return {ViewManager} subview manager object
     */
    get viewManager() { return this._myViewManager; }
    /**
     * Get Camera manager.
     * @return {Camera} Camera manager object
     */
    get camera() { return this._myFocusCamera; }
    /**
     * Get root view object (either main view itself or composer of subviews).
     * @return {Subview} View object
     */
    get rootView() { return this._myRootView; }
    /**
     * Get view object receiving focus.
     * @return {Subview} View object
     */
    get focusView() { return this._myFocusView; }
    /**
     * Get Rendering Parameters manager.
     * @return {RenderingParams} Rendering Parameters manager object
     */
    get renderingParams() { return this._myRenderParams; }
    /**
     * Get unit conversion manager.
     * @return {UnitManager} unit conversion manager
     */
    get unitManager() { return this._myUnitManager; }
    /**
     * Get Object manager.
     * @return {InteractiveContext} Object manager
     */
    get interactiveContext() { return this._myInteractiveContext; }
    /**
     * Get selection manager.
     * @return {SelectionManager} selection manager
     */
    get selectionManager() { return this._mySelectionManager; }
    /**
     * Get event manager - an interface for subscribing and unsubscribing to various callbacks (like selection changes).
     * @return {ViewerEventManager} event manager
     */
    get eventManager() { return this._myEventManager; }
    /**
     * @internal Get aspects manager.
     * @return {InteractiveAspectsManager} aspects manager
     */
    get interactiveAspectsManager() { return this._myInteractiveAspectsManager; }
    /**
     * @internal Get CAE mesh manager.
     * @return {InteractiveCaeMeshManager} CAE mesh manager
     */
    get interactiveCaeMeshManager() { return this._myInteractiveCaeMeshManager; }
    /**
     * @internal Get shape manager.
     * @return {InteractiveShapeManager} shape manager
     */
    get interactiveShapeManager() { return this._myInteractiveShapeManager; }
    /**
     * @internal Get colorscale manager.
     * @return {InteractiveColorScaleManager} colorscale manager
     */
    get interactiveColorScaleManager() { return this._myInteractiveColorScaleManager; }
    /**
     * @internal Get clipping box manager.
     * @return {InteractiveClippingManager} clipping box manager
     */
    get interactiveClippingManager() { return this._myInteractiveClippingManager; }
    /**
     * @internal Get scale ruler manager.
     * @return {InteractiveScaleRulerManager} scale ruler manager
     */
    get interactiveScaleRulerManager() { return this._myInteractiveScaleRulerManager; }
    /**
     * @internal Get text label manager.
     * @return {InteractiveTextLabelManager} text label manager
     */
    get interactiveTextLabelManager() { return this._myInteractiveTextLabelManager; }
    /**
     * @internal Get dimension manager.
     * @return {InteractiveDimensionManager} dimensioon manager
     */
    get interactiveDimensionManager() { return this._myInteractiveDimensionManager; }
    /**
    * @internal Get viewcube manager.
    * @return {InteractiveViewCubeManager} viewcube manager
    */
    get interactiveViewCubeManager() { return this._myInteractiveViewCubeManager; }
    /**
     * @internal Get axes manager.
     * @return {InteractiveAxesManager} axes manager
     */
    get interactiveAxesManager() { return this._myInteractiveAxesManager; }
    /**
     * Get InteractiveScaleRuler object.
     * @return {InteractiveScaleRuler} InteractiveScaleRuler object
     */
    get scaleRuler() { return this._myScaleRuler; }
    /**
     * Get InteractiveViewCube object.
     * @return {InteractiveViewCube} InteractiveViewCube object
     */
    get viewCube() { return this._myViewCube; }
    /** @internal generate screenshot (short-cut for diagnostics). */
    get doScreenshot() { return this.makeScreenshot('viewer.png', -1, -1); }
    /**
     * Set gravity level for logging messages.
     * @param {MessageGravity} theGravity lower message gravity to log
     */
    set traceLevel(theGravity) {
        // Messages are filtered by both {@link SCwebviewer.printMessage} and `Message_Printer::send()`.
        // Propagating filter level to Message_Printer might improve performance by skipping extra function calls for too verbose messages.
        this._myTraceLevel = theGravity.valueOf();
        if (this._myIsWasmLoaded) {
            this._cppSetTraceLevel(theGravity.valueOf());
        }
    }
    /** Flag to enable extra verbose logging. */
    set toTraceImages(theToEnable) { this._myToTraceImages = theToEnable; }
    /**
     * Set if pthread version should be loaded when supported by Browser.
     * Should be set BEFORE SCwebviewer.init()!
     */
    set toPreferPthread(theToPrefer) { this._myToPreferPthread = theToPrefer; }
    /** Check browser support and return TRUE if WASM supported. */
    isWasmSupported() {
        try {
            if (typeof WebAssembly === 'object' && typeof WebAssembly.instantiate === 'function') {
                const aDummyModule = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00));
                if (aDummyModule instanceof WebAssembly.Module) {
                    return (new WebAssembly.Instance(aDummyModule) instanceof WebAssembly.Instance);
                }
            }
        }
        catch (theErr) {
            // skip errors
        }
        return false;
    }
    /** Check browser support and return TRUE if WASM64 supported. */
    isWasm64Supported() {
        try {
            if (typeof WebAssembly === 'object' && typeof WebAssembly.instantiate === 'function') {
                // this WAT might look wrong, but enough for checking WASM64 support
                // '(module (memory i64 1) )'
                const aDummyModule = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x05, 0x03, 0x01, 0x04, 0x01));
                if (aDummyModule instanceof WebAssembly.Module) {
                    return (new WebAssembly.Instance(aDummyModule) instanceof WebAssembly.Instance);
                }
            }
        }
        catch (theErr) {
            // skip errors
        }
        return false;
    }
    /** Check if multithreading is supported and allowed by Browser. */
    isPthreadAllowed() {
        return window.Worker !== undefined && window.Atomics !== undefined &&
            window.SharedArrayBuffer !== undefined;
    }
    /** Init {@link SCwebviewer} Module and load wasm file. */
    async init() {
        return new Promise(async (resolve, reject) => {
            try {
                await this._loadModule('Viewer-wasm.js');
                // load standard fonts
                await this._loadFonts();
                this._myWasmViewer = this._cppGetViewer();
                this._myUnitManager = this._myWasmViewer.getManager(ManagerType.UNIT_MANAGER);
                UnitManagerExtensions.init(this._myUnitManager);
                this._myRenderParams = this._myWasmViewer.getManager(ManagerType.RENDERING_PARAMS);
                this._myInteractiveAspectsManager = this._myWasmViewer.getManager(ManagerType.ASPECTS_MANAGER);
                this._myInteractiveCaeMeshManager = this._myWasmViewer.getManager(ManagerType.CAEMESH_MANAGER);
                this._myInteractiveShapeManager = this._myWasmViewer.getManager(ManagerType.SHAPE_MANAGER);
                InteractiveShapeManagerExtensions.init(this, this._myInteractiveShapeManager);
                this._myInteractiveColorScaleManager = this._myWasmViewer.getManager(ManagerType.COLORSCALE_MANAGER);
                this._myInteractiveClippingManager = this._myWasmViewer.getManager(ManagerType.CLIPPING_MANAGER);
                this._myInteractiveScaleRulerManager = this._myWasmViewer.getManager(ManagerType.SCALERULER_MANAGER);
                this._myInteractiveTextLabelManager = this._myWasmViewer.getManager(ManagerType.TEXTLABEL_MANAGER);
                this._myInteractiveDimensionManager = this._myWasmViewer.getManager(ManagerType.DIMENSION_MANAGER);
                this._myInteractiveViewCubeManager = this._myWasmViewer.getManager(ManagerType.VIEWCUBE_MANAGER);
                this._myInteractiveAxesManager = this._myWasmViewer.getManager(ManagerType.AXES_MANAGER);
                this._myInteractiveContext = this._myWasmViewer.getManager(ManagerType.INTERACTIVE_CONTEXT);
                InteractiveContextExtensions.init(this, this._myInteractiveContext);
                this._mySelectionManager = this._myWasmViewer.getManager(ManagerType.SELECTION_MANAGER);
                SelectionManagerExtensions.init(this, this._mySelectionManager);
                this._myViewManager = this._myWasmViewer.getManager(ManagerType.VIEW_MANAGER);
                ViewManagerExtensions.init(this, this._myViewManager);
                this._myFocusCamera = new Camera(this, -1);
                this._myRootView = new Subview();
                this._myRootView.wrap(this, 0);
                this._myFocusView = new Subview();
                this._myFocusView.wrap(this, -1);
                this._myScaleRuler = new InteractiveScaleRuler(this);
                this._myViewCube = new InteractiveViewCube(this);
                resolve();
            }
            catch (err) {
                reject(err);
            }
        });
    }
    /**
     * Register font (file in TTF format).
     * This should be done before displaying text labels using this font,
     * otherwise a fallback font will be used instead.
     * @param {string} theFontUrl url of font to download and register
     * @param {string} theFontName font name or empty string to keep name from file itself
     * @param {boolean} theToOverride override existing font family
     */
    async registerFontFromUrl(theFontUrl, theFontName = '', theToOverride = false) {
        const aPaths = theFontUrl.split('/');
        const aFileName = `/fonts/${aPaths.pop()}`;
        await this.uploadUrl(theFontUrl, aFileName, false);
        this._cppRegisterFont(aFileName, theFontName, theToOverride);
    }
    /**
     * Load and register SVG image to be accessible by viewer.
     * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
     */
    async registerImageFromUrl(theImageURl, theImageId = '', theIsScalable = undefined) {
        let aViewer = this;
        return new Promise((theResolve, theReject) => {
            let anSvgImg = new Image();
            anSvgImg.onload = function () {
                if (aViewer.registerImage(anSvgImg, theImageId !== '' ? theImageId : theImageURl, theIsScalable)) {
                    theResolve();
                }
                else {
                    theReject();
                }
            };
            anSvgImg.onerror = function () {
                theReject();
            };
            anSvgImg.src = theImageURl;
        });
    }
    /**
     * Register image to be accessible by viewer.
     * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
     */
    registerImage(theImage, theImageId, theIsScalable = undefined) {
        if (theImage == null) {
            if (this._myImageMap.delete(theImageId)) {
                if (theImageId.startsWith('/')) {
                    this.unlinkFile(theImageId);
                }
                return true;
            }
            return false;
        }
        if (theImage.width === 0 || theImage.height === 0) {
            throw new Error(`Unable to register image '${theImageId}' with zero dimensions`);
        }
        // this is a poor judjment - URL might have wrong or no file extension,
        // but Image class has no property returning this information.
        let isScalable = theIsScalable !== undefined ? theIsScalable : theImageId.endsWith('.svg');
        theImage.isScalable = isScalable;
        if (this._myToTraceImages) {
            const aNativeSize = theImage.naturalWidth !== theImage.width || theImage.naturalHeight !== theImage.height ?
                ` [native: ${theImage.naturalWidth}x${theImage.naturalHeight}]` : '';
            const aScalStr = isScalable ? ' [scalable]' : ' [non-scalable]';
            this.printMessage(`Registered image '${theImageId}' ${theImage.width}x${theImage.height}${aScalStr}${aNativeSize}`, MessageGravity.Trace.valueOf());
        }
        this._myImageMap.set(theImageId, theImage);
        // create a dummy file
        if (theImageId.startsWith('/')) {
            this.preloadFile(Uint8Array.of(0x0), theImageId, false);
        }
        return true;
    }
    /** Return TRUE if preloaded image is defined. */
    isImageRegistered(theImageId) { return this._myImageMap.has(theImageId); }
    /** Return TRUE if preloaded image is scalable (e.g. SVG). */
    isImageScalable(theImageId) {
        let anImg = this._myImageMap.get(theImageId);
        return anImg && anImg.isScalable;
    }
    /** Return dimensions of registered image. */
    getImageSize(theImageId, theIsNatural = false) {
        let anImg = this._myImageMap.get(theImageId);
        if (!anImg) {
            return { x: 0, y: 0 };
        }
        return theIsNatural ?
            { x: anImg.naturalWidth, y: anImg.naturalHeight } :
            { x: anImg.width, y: anImg.height };
    }
    /** Render registered image and return raw RGBA data buffer to be released by Module._free(). */
    renderImage(theImageId, theW, theH) {
        let anImg = this._myImageMap.get(theImageId);
        if (!anImg) {
            this.printMessage(`Error: Unable to render unregistered image '${theImageId}'`, MessageGravity.Fail.valueOf());
            // @c throw new Error("Unregistered image '" + theImageId + "'");
            return 0;
        }
        // create a canvas to draw pixels
        if (this._myImageCanvas == null) {
            this._myImageCanvas = document.createElement('canvas');
        }
        const aCtx = this._myImageCanvas.getContext('2d', { willReadFrequently: true });
        if (!aCtx) {
            throw new Error('Unable to create 2D canvas');
        }
        let aSize = { dx: theW, dy: theH };
        if (theW === 0 && theH === 0) {
            aSize = { dx: anImg.width, dy: anImg.height };
        }
        this._myImageCanvas.width = aSize.dx;
        this._myImageCanvas.height = aSize.dy;
        aCtx.drawImage(anImg, 0, 0, aSize.dx, aSize.dy);
        let anImgData = aCtx.getImageData(0, 0, aSize.dx, aSize.dy);
        let aRawMem = this._allocateArrayU8(new Uint8Array(anImgData.data));
        if (this._myToTraceImages) {
            this.printMessage(`Rendered image '${theImageId}' ${anImg.width}x${anImg.height} at ${aSize.dx}x${aSize.dy}`, MessageGravity.Trace.valueOf());
        }
        return aRawMem;
    }
    /**
     * Dump fonts information.
     * @param {string} theFontName font name to find or empty string to print all fonts
     * @param {boolean} theIsStrict flag to find exact font name
     * @param {boolean} theToPrintDetails flag to print details or only font name
     */
    dumpFontsInfo(theFontName = '', theIsStrict = false, theToPrintDetails = true) {
        this._cppDumpFontsInfo(theFontName, theIsStrict, theToPrintDetails);
    }
    /** Request View update (using `requestAnimationFrame()` or similar method). */
    requestViewUpdate() { var _a; (_a = this._myWasmViewer) === null || _a === void 0 ? void 0 : _a.requestViewUpdate(); }
    /** Resize rendering window. */
    resizeViewport() { var _a; (_a = this._myWasmViewer) === null || _a === void 0 ? void 0 : _a.resizeViewport(); }
    /**
     * Set cubemap background. File will be loaded asynchronously.
     * @param {string} theImagePath image path to load
     * @param {boolean} theLightmapOnly
     * @return {Promise} promise with image loading results
     */
    async setBackgroundCubemap(theImagePath, theLightmapOnly) {
        var _a;
        const aFilePath = '/emulated/cubemap.jpg';
        await this.uploadUrl(theImagePath, aFilePath, true);
        (_a = this._myRenderParams) === null || _a === void 0 ? void 0 : _a.setBackgroundCubemap(aFilePath, theLightmapOnly);
    }
    /** Print memory stats */
    dumpMemInfo() {
        var _a;
        (_a = this._myWasmViewer) === null || _a === void 0 ? void 0 : _a.dumpMemInfo();
    }
    /**
     * Get screenshot.
     * @param {theWidth}  theWidth  of screenshot (`-1` - take size of canvas)
     * @param {theHeight} theHeight of screenshot (`-1` - take size of canvas)
     * @return string in the format image/png or empty
     */
    getScreenshot(theWidth, theHeight) {
        var _a;
        const aWidth = theWidth === -1 ? this.canvas.width : theWidth;
        const aHeight = theHeight === -1 ? this.canvas.height : theHeight;
        const aByteSize = aWidth * aHeight * 4;
        const aDataBuffer = this._malloc(aByteSize);
        // make screenshot
        if (!((_a = this._myWasmViewer) === null || _a === void 0 ? void 0 : _a.makeScreenshot(aWidth, aHeight, aDataBuffer))) {
            this._free(aDataBuffer);
            return '';
        }
        const aBuff = this.HEAPU8.subarray(aDataBuffer, aDataBuffer + aByteSize);
        // create a canvas to draw pixels
        const aCanvas = document.createElement('canvas');
        const aCtx = aCanvas.getContext('2d');
        if (!aCtx) {
            return '';
        }
        aCanvas.width = aWidth;
        aCanvas.height = aHeight;
        // create imageData object
        let aData = aCtx.createImageData(aCanvas.width, aCanvas.height);
        // set our buffer as source
        aData.data.set(aBuff);
        aCtx.putImageData(aData, 0, 0);
        // free buffer
        this._free(aDataBuffer);
        // make magic to download the image
        const MIME_TYPE = 'image/png';
        const imgURL = aCanvas.toDataURL(MIME_TYPE);
        return imgURL;
    }
    /**
     * Make screenshot and download.
     * @param {theFilename}  theFilename file name to download
     * @param {theWidth}  theWidth  of screenshot (`-1` - take size of canvas)
     * @param {theHeight} theHeight of screenshot (`-1` - take size of canvas)
     * @return true on success
     */
    makeScreenshot(theFilename, theWidth, theHeight) {
        const imgURL = this.getScreenshot(theWidth, theHeight);
        const MIME_TYPE = 'image/png';
        if (imgURL === '') {
            return false;
        }
        const dlLink = document.createElement('a');
        dlLink.download = theFilename;
        dlLink.href = imgURL;
        dlLink.dataset.downloadurl = [MIME_TYPE, dlLink.download, dlLink.href].join(':');
        document.body.appendChild(dlLink);
        dlLink.click();
        document.body.removeChild(dlLink);
        return true;
    }
    /** Make a link to download file from virtual file system. */
    makeFileLink(theFileName, theMime = 'application/octet-stream') {
        const aData = this.FS.readFile(theFileName);
        const aBlob = new Blob([aData], { type: theMime });
        const anUrl = URL.createObjectURL(aBlob);
        const dlLink = document.createElement('a');
        dlLink.download = theFileName;
        dlLink.href = anUrl;
        dlLink.dataset.downloadurl = [theMime, dlLink.download, dlLink.href].join(':');
        document.body.appendChild(dlLink);
        dlLink.click();
        document.body.removeChild(dlLink);
        return true;
    }
    /**
     * Allocate memory block and initialize it's contents.
     * Use _free() to release returned memory object.
     * @internal
     */
    _allocateArrayU8(theArray) {
        const aDataBuffer = this._malloc(theArray.length);
        this.HEAPU8.set(theArray, aDataBuffer);
        return aDataBuffer;
    }
    /**
     * Take buffer and upload it to emulated file system.
     * @param {Uint8Array} theBuffer    buffer to load
     * @param {string}     theFilePath  file path on emulated file system (or empty string to take name from URL)
     * @param {boolean}    theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    preloadFile(theBuffer, theFilePath, theToPreload) {
        return new Promise((theResolve, theReject) => {
            let aDestDir = this.PATH.dirname(theFilePath);
            this.FS.mkdirTree(aDestDir);
            this.FS.writeFile(theFilePath, theBuffer);
            if (theToPreload) {
                // decode image
                this.FS.createPreloadedFile(!theFilePath.startsWith('/') ? this.FS.cwd() : '/', theFilePath, theBuffer, true, true, () => { theResolve(true); }, () => { theReject(new Error(`Preload '${theFilePath}' failed`)); }, true); // file is already created
            }
            else {
                theResolve(true);
            }
        });
    }
    /**
     * Upload specified ArrayBuffer to emulated file system.
     * To remove file from file system you should call {@link unlinkFile} if it is no longer needed
     * @param {string} theArray    array buffer to load
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     * @param {boolean} theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    uploadArrayBuffer(theArray, theFilePath, theToPreload) {
        let aPathSplit = theFilePath.split('/');
        let aFileName = theFilePath;
        if (aPathSplit.length > 1) {
            aFileName = aPathSplit[aPathSplit.length - 1];
        }
        let aDataArray = new Uint8Array(theArray);
        this.printMessage(`uploading file '${aFileName}' of size ${aDataArray.length} bytes to '${theFilePath}'...`, MessageGravity.Trace.valueOf());
        return this.preloadFile(aDataArray, theFilePath, theToPreload);
    }
    /**
     * Fetch remote file from specified URL and upload it to emulated file system.
     * To remove file from file system you should call {@link unlinkFile} if it is no longer needed
     * @param {string} theFileUrl  URL to load
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     * @param {boolean} theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    uploadUrl(theFileUrl, theFilePath, theToPreload) {
        let aPathSplit = theFileUrl.split('/');
        let aFileName = theFileUrl;
        if (aPathSplit.length > 1) {
            aFileName = aPathSplit[aPathSplit.length - 1];
        }
        let aFilePath = theFilePath;
        if (aFilePath === '') {
            aFilePath = aFileName;
        }
        const aCheckStatusFunc = function (theResponse) {
            if (!theResponse.ok) {
                throw new Error(`HTTP ${theResponse.status} - ${theResponse.statusText} (URL: '${theFileUrl}')`);
            }
            return theResponse;
        };
        return new Promise((theResolve, theReject) => {
            fetch(theFileUrl)
                .then((theResponse) => aCheckStatusFunc(theResponse) && theResponse.arrayBuffer())
                .then((theBuffer) => {
                let aDataArray = new Uint8Array(theBuffer);
                this.printMessage(`uploading file '${aFileName} ' of size ${aDataArray.length} bytes to '${aFilePath}'...`, MessageGravity.Trace.valueOf());
                this.preloadFile(aDataArray, aFilePath, theToPreload)
                    .then(aData => { theResolve(aData); })
                    .catch(anErr => { theReject(anErr); });
            }).catch((theErr) => {
                theReject(theErr);
            });
        });
    }
    /**
     * Unlinks the node at path.
     * This removes a name from the file system.
     * If that name was the last link to a file (and no processes have the file open) the file is deleted.
     * Usually you should call this function after {@link uploadUrl} to remove file which is no longer needed.
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     */
    unlinkFile(theFilePath) {
        const aRes = this.FS.analyzePath(theFilePath, true);
        const anImgRes = this._myImageMap.delete(theFilePath);
        if (aRes.exists) {
            this.FS.unlink(theFilePath);
            return true;
        }
        return anImgRes;
    }
    /**
     * Check if file exists in emulated file system.
     */
    checkFile(theFilePath) {
        return this.FS.analyzePath(theFilePath, true).exists;
    }
    // #region Module interface
    /**
     * C++ `std::cout` callback redirecting to Terminal.
     * @param {string} theText text to print
     */
    print(theText) { this.printMessage(theText, MessageGravity.Info); }
    /**
     * C++ `std::cerr` callback redirecting to Terminal.
     * @param {string} theText text to print
     */
    printErr(theText) { this.printMessage(theText, MessageGravity.Fail); }
    /**
     * C++ `Message::Send()` callback redirecting to Terminal.
     * @param {string} theText text to print
     * @param {number} theGravity message gravity within 0..4 range
     */
    printMessage(theText, theGravity) {
        if (theGravity.valueOf() < this._myTraceLevel) {
            return;
        }
        switch (theGravity) {
            case MessageGravity.Trace: // trace
                console.debug(theText);
                return;
            case MessageGravity.Info: // info
                console.info(theText);
                return;
            case MessageGravity.Warning: // warning
                console.warn(theText);
                return;
            case MessageGravity.Alarm: // alarm
            case MessageGravity.Fail: // fail
                console.error(theText);
                return;
            default:
                console.warn(theText);
                return;
        }
    }
    /**
     * Callback returning file path for loading WebAssembly components.
     * @param {string} thePath file path to locate
     * @param {string} thePrefix default file prefix
     * @return {string} full path to the resource
     */
    locateFile(thePath, thePrefix) {
        return this._myBasePrefix + thePath;
    }
    /** WebAssembly module callback on runtime initialization. */
    onRuntimeInitialized() {
        // @c console.log(" @@ onRuntimeInitialized()" + Object.getOwnPropertyNames (this));
    }
    // #endregion
    // #region C++ methods exported by Module (forward declarations)
    /**
     * @internal Memory allocation within C++ heap (forward declaration).
     * @param {number} theSize number of bytes to allocation
     * @return {number} offset within the heap
     */
    _malloc(theSize) { throw new Error('Method not implemented'); }
    /**
     * @internal Memory deallocation within C++ heap (forward declaration).
     * @param {number} theOffset offset within the heap
     */
    _free(theOffset) { throw new Error('Method not implemented'); }
    /**
     * Set gravity level for C++ OCCT messenger.
     * @param {number} theGravity lower message gravity to log
     */
    _cppSetTraceLevel(theGravity) { throw new Error('Method not implemented'); }
    /**
     * Register font.
     * @param {string} theFontPath the font path
     * @param {string} theFontName font name or empty string to keep name from file itself
     * @param {boolean} theToOverride override existing font family
     */
    _cppRegisterFont(theFontPath, theFontName, theToOverride) {
        throw new Error('Method not implemented');
    }
    /**
     * Dump fonts information.
     * @param {string} theFontName font name to find or empty string to print all fonts
     * @param {boolean} theIsStrict flag to find exact font name
     * @param {boolean} theToPrintDetails flag to print details or only font name
     */
    _cppDumpFontsInfo(theFontName, theIsStrict, theToPrintDetails) {
        throw new Error('Method not implemented');
    }
    /**
     * Call callback by its pointer.
     * @param {number} theCallback pointer to {@link IPromise} object
     */
    _cppWasmCallBack(theCallback) { throw new Error('Method not implemented'); }
    /**
     * Get global Viewer instance.
     * @return {WasmViewer} viewer instance
     */
    _cppGetViewer() { throw new Error('Method not implemented'); }
    // #endregion
    /** Import WASM module. */
    async _importModule(theModuleName, theWasmCfg) {
        this._myBasePrefix = `${this._myBaseFolder}${theWasmCfg}/`;
        this.mainScriptUrlOrBlob = `${this._myBasePrefix}${theModuleName}`;
        const isPThreaded = theWasmCfg.includes('pthread');
        if (!isPThreaded) {
            let ret = await import(/* webpackIgnore: true */ this._myBasePrefix + theModuleName);
            this._myIsPthreadLoaded = false;
            this._myIsWasmLoaded = true;
            return ret;
        }
        // Handle dynamic import of UMD module.
        // UMD module is required for building WASM with -pthread due to background Workers
        // initialization logic generated by Emscripten (but probably it is better on newer Emscripten).
        const aRet = await fetch(this._myBasePrefix + theModuleName);
        let aSrc = await aRet.text();
        if (!aRet.ok) {
            throw new Error(`Fail to fetch ${this.mainScriptUrlOrBlob} Response finished with ${aRet.status}`);
        }
        aSrc += '\nexport default createSCwebviewer';
        const aBlob = new Blob([aSrc], { type: 'text/javascript' });
        let AMod = await import(/* webpackIgnore: true */ URL.createObjectURL(aBlob));
        this._myIsPthreadLoaded = true;
        this._myIsWasmLoaded = true;
        return AMod;
    }
    /** Load WASM module. */
    async _loadModule(theModuleName) {
        // ["wasm64-pthread", "wasm64", "wasm32-pthread", "wasm32"]
        const aWasmCfgList = ViewerBuildConfig.wasmConfigs;
        let aLastErr = '';
        for (let aWasmCfgIter of aWasmCfgList) {
            const isPThreaded = aWasmCfgIter.includes('pthread');
            const isWasm64 = aWasmCfgIter.includes('wasm64');
            if (isPThreaded && !(this._myToPreferPthread && this.isPthreadAllowed())) {
                continue; // unsupported or ignored
            }
            if (isWasm64 && !this.isWasm64Supported()) {
                continue; // unsupported or ignored
            }
            let aBackProps = null;
            try {
                let aMod = await this._importModule(theModuleName, aWasmCfgIter);
                aBackProps = {};
                for (let aPropIter in this) {
                    if (aBackProps[aPropIter] === undefined) {
                        aBackProps[aPropIter] = this[aPropIter];
                    }
                }
                await aMod.default(this);
                return;
            }
            catch (err) {
                aLastErr = `Unable to load ${aWasmCfgIter} viewer, ${err})`;
                this.printMessage(aLastErr, MessageGravity.Fail);
                // remove partially initialized fields
                for (let aPropIter in (aBackProps != null ? this : {})) {
                    if (aBackProps[aPropIter] === undefined) {
                        delete this[aPropIter];
                    }
                }
            }
        }
        aLastErr = `No suitable WASM configuration (${aWasmCfgList.toString()}) found for this browser.\n${aLastErr}`;
        this.printMessage(aLastErr, MessageGravity.Fail);
        let aCtx = this.canvas.getContext('2d');
        if (aCtx) {
            aCtx.fillStyle = 'white';
            aCtx.font = 'bold 16px Arial';
            aCtx.textAlign = 'center';
            aCtx.textBaseline = 'middle';
            aCtx.fillText('Unable to load WASM viewer', (this.canvas.width / 2), (this.canvas.height / 2));
        }
    }
    /** Load standard fonts. */
    async _loadFonts() {
        const aFontsFolder = `${this._myBaseFolder}fonts/`;
        const aMonoFamily = ['UbuntuMono-R.ttf'];
        // skip italic and bold-italic to spare downloads - they could be synthesized
        // @c "OpenSans-Italic.ttf"
        const aSansFamily = ['OpenSans-Regular.ttf', 'OpenSans-Bold.ttf'];
        const aCjkFamily = ['HarmonyOS_Sans_SC_Regular.woff'];
        for (let aFontIter of aMonoFamily) {
            try {
                await this.registerFontFromUrl(aFontsFolder + aFontIter, 'monospace');
            }
            catch (err) {
                // skip errors
            }
        }
        for (let aFontIter of aSansFamily) {
            try {
                await this.registerFontFromUrl(aFontsFolder + aFontIter, 'sans-serif');
            }
            catch (err) {
                // skip errors
            }
        }
        // CJK fonts are large, it might be better loading them only when GUI settings are set to Chinese
        for (let aFontIter of aCjkFamily) {
            try {
                await this.registerFontFromUrl(aFontsFolder + aFontIter, 'cjk');
            }
            catch (err) {
                // skip errors
            }
        }
    }
}
;
//# sourceMappingURL=SCwebviewer.js.map