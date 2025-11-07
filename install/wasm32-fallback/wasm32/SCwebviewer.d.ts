import { Camera } from './Camera.js';
import type { IVec2, CameraOrientation, ColorString, IPromise, UnitManager } from './CommonTools.js';
import type { InteractiveContext, SelectionManager } from './InteractiveObject.js';
import { InteractiveScaleRuler } from './InteractiveScaleRuler.js';
import { InteractiveViewCube } from './InteractiveViewCube.js';
import { ViewerEventManager } from './ViewerEventManager.js';
import type { ViewManager } from './ViewManager.js';
import { Subview } from './Subview.js';
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
declare global {
    interface Window {
        Atomics: any;
        Worker: any;
        SharedArrayBuffer: any;
    }
}
/**
 * This enumeration defines gravity level of messages.
 * It reflects values of corresponding C++ `Message_Gravity` enumeration defined by OCCT,
 * and used to redirect message coming from C++ algorithms via {@link SCwebviewer.printMessage} callback.
 * See also {@link SCwebviewer.traceLevel} defining which messages should be delivered.
 */
export declare enum MessageGravity {
    /** Low-level details on algorithm execution (usually for debug purposes). */
    Trace = 0,
    /** Informative message. */
    Info = 1,
    /** Warning message. */
    Warning = 2,
    /** Non-critical error. */
    Alarm = 3,
    /** Fatal error. */
    Fail = 4
}
/**
 * This enumeration defines rendering transparency method.
 * It reflects values of corresponding C++ `Graphic3d_RenderTransparentMethod` enumeration defined by OCCT.
 */
export declare enum RenderTransparentMethod {
    /** Basic blend transparency with non-commuting blend operator without sorting. */
    Unordered = 0,
    /** Weighted Blended Order-Independent Transparency. */
    BlendOit = 1,
    /** Depth Peeling. */
    DepthPeelingOit = 2
}
/**
 * Represents Rendering Parameters manager (based on `Graphic3d_RenderingParams` from C++ OCCT).
 * See also {@link SCwebviewer.renderingParams} property.
 */
export interface RenderingParams {
    /** Background RGB color defined as {@link ColorString}. */
    backgroundColor: ColorString;
    /** IBL (Image Based Lighting) from background cubemap enabled state. */
    isImageBasedLighting: boolean;
    /** Enable PBR or fallback to legacy Phong shading model. */
    isPbrEnabled: boolean;
    /** Rendering resolution ratio; 1.0 means native resolution (100% scale). */
    renderResolutionScale: number;
    /** Number of samples (integer) for MSAA buffer, 0 means no MSAA anti-aliasing. */
    nbMsaaSamples: number;
    /** To show stats on/off. */
    toShowStats: boolean;
    /** To cast shadows on/off. */
    isShadowEnabled: boolean;
    /** Shadow map resolution in pixels (integer number). */
    shadowMapResolution: number;
    /** Shadow map bias. */
    shadowMapBias: number;
    /** Transparency method. */
    transparencyMethod: RenderTransparentMethod;
    /** Number of depth peeling layers. */
    nbOitDepthPeelingLayers: number;
    /**
     * Set cubemap background. File will be loaded asynchronously.
     * @param {string} theImagePath image path to load
     * @param {boolean} theLightmapOnly flag to not display cubemap in background
     */
    setBackgroundCubemap: (theImagePath: string, theLightmapOnly: boolean) => void;
}
/**
 * Represents Viewer global object.
 * Note that there is no reason to use class directly at application level
 * as {@link SCwebviewer} re-exports main Viewer properties.
 */
export interface WasmViewer {
    /** Resize viewport / window. */
    resizeViewport: () => void;
    /** Dump memory usage stats */
    dumpMemInfo: () => void;
    /**
     * Make screenshot.
     * @param {number} theWidth width of image
     * @param {number} theHeight height of image
     * @param {number} theBuffer preallocated buffer
     * @return {bool} return FALSE on error
     */
    makeScreenshot: (theWidth: number, theHeight: number, theBuffer: number) => boolean;
    /** Request View update (using requestAnimationFrame() or similar method). */
    requestViewUpdate: () => void;
}
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
export declare class SCwebviewer {
    /** Flag defining `main()` return behavior (read by Emscripten Module). */
    noExitRuntime: boolean;
    /** 'canvas' element for WebGL context (read by Emscripten Module). */
    canvas: HTMLCanvasElement;
    /** Implementation of {@link IPromise} type. */
    IPromise: IPromise;
    /** Implementation of {@link CameraOrientation}. */
    CameraOrientation: typeof CameraOrientation;
    private _myBaseFolder;
    private _myBasePrefix;
    private _myCanvasId;
    private _myIsWasmLoaded;
    private _myToPreferPthread;
    private _myIsPthreadLoaded;
    private mainScriptUrlOrBlob;
    private _myTraceLevel;
    private _myEventManager;
    private _myWasmViewer;
    private _myUnitManager;
    private _myViewManager;
    private _myRenderParams;
    private _mySelectionManager;
    private _myInteractiveContext;
    private _myInteractiveAspectsManager;
    private _myInteractiveCaeMeshManager;
    private _myInteractiveShapeManager;
    private _myInteractiveColorScaleManager;
    private _myInteractiveClippingManager;
    private _myInteractiveScaleRulerManager;
    private _myInteractiveTextLabelManager;
    private _myInteractiveDimensionManager;
    private _myInteractiveViewCubeManager;
    private _myInteractiveAxesManager;
    private _myRootView;
    private _myFocusView;
    private _myFocusCamera;
    private _myScaleRuler;
    private _myViewCube;
    private _myImageMap;
    private _myImageCanvas;
    private _myToTraceImages;
    /** Memory heap (forward declaration, exported by Module). */
    private HEAPU8;
    /** Entry to emulated File System API (forward declaration, exported by Emscripten Module). */
    private FS;
    /** Emulated File System helper (forward declaration, exported by Emscripten Module). */
    private PATH;
    /**
     * Main constructor.
     * @param {string} theBaseDataPath path to folder that contains
     *        'wasm32' + 'wasm32-pthread' folders (WASM files),
     *        'fonts' folder with default font files.
     */
    constructor(theBaseDataPath: string);
    /** Check if WASM module has been loaded. */
    get isWasmLoaded(): boolean;
    /** Return TRUE if pthread version should be loaded when supported and allowed by Browser. */
    get toPreferPthread(): boolean;
    /** Check if pthread version of viewer is loaded. */
    get isPthreadLoaded(): boolean;
    /**
     * Get gravity level for logging messages.
     * @return {MessageGravity} lower message gravity to log
     */
    get traceLevel(): MessageGravity;
    /** Flag to enable extra verbose logging. */
    get toTraceImages(): boolean;
    /**
     * Get subview manager.
     * @return {ViewManager} subview manager object
     */
    get viewManager(): ViewManager | null;
    /**
     * Get Camera manager.
     * @return {Camera} Camera manager object
     */
    get camera(): Camera | null;
    /**
     * Get root view object (either main view itself or composer of subviews).
     * @return {Subview} View object
     */
    get rootView(): Subview | null;
    /**
     * Get view object receiving focus.
     * @return {Subview} View object
     */
    get focusView(): Subview | null;
    /**
     * Get Rendering Parameters manager.
     * @return {RenderingParams} Rendering Parameters manager object
     */
    get renderingParams(): RenderingParams | null;
    /**
     * Get unit conversion manager.
     * @return {UnitManager} unit conversion manager
     */
    get unitManager(): UnitManager | null;
    /**
     * Get Object manager.
     * @return {InteractiveContext} Object manager
     */
    get interactiveContext(): InteractiveContext | null;
    /**
     * Get selection manager.
     * @return {SelectionManager} selection manager
     */
    get selectionManager(): SelectionManager | null;
    /**
     * Get event manager - an interface for subscribing and unsubscribing to various callbacks (like selection changes).
     * @return {ViewerEventManager} event manager
     */
    get eventManager(): ViewerEventManager;
    /**
     * Get InteractiveScaleRuler object.
     * @return {InteractiveScaleRuler} InteractiveScaleRuler object
     */
    get scaleRuler(): InteractiveScaleRuler | null;
    /**
     * Get InteractiveViewCube object.
     * @return {InteractiveViewCube} InteractiveViewCube object
     */
    get viewCube(): InteractiveViewCube | null;
    /**
     * Set gravity level for logging messages.
     * @param {MessageGravity} theGravity lower message gravity to log
     */
    set traceLevel(theGravity: MessageGravity);
    /** Flag to enable extra verbose logging. */
    set toTraceImages(theToEnable: boolean);
    /**
     * Set if pthread version should be loaded when supported by Browser.
     * Should be set BEFORE SCwebviewer.init()!
     */
    set toPreferPthread(theToPrefer: boolean);
    /** Check browser support and return TRUE if WASM supported. */
    isWasmSupported(): boolean;
    /** Check browser support and return TRUE if WASM64 supported. */
    isWasm64Supported(): boolean;
    /** Check if multithreading is supported and allowed by Browser. */
    isPthreadAllowed(): boolean;
    /** Init {@link SCwebviewer} Module and load wasm file. */
    init(): Promise<void>;
    /**
     * Register font (file in TTF format).
     * This should be done before displaying text labels using this font,
     * otherwise a fallback font will be used instead.
     * @param {string} theFontUrl url of font to download and register
     * @param {string} theFontName font name or empty string to keep name from file itself
     * @param {boolean} theToOverride override existing font family
     */
    registerFontFromUrl(theFontUrl: string, theFontName?: string, theToOverride?: boolean): Promise<void>;
    /**
     * Load and register SVG image to be accessible by viewer.
     * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
     */
    registerImageFromUrl(theImageURl: string, theImageId?: string, theIsScalable?: boolean | undefined): Promise<void>;
    /**
     * Register image to be accessible by viewer.
     * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
     */
    registerImage(theImage: HTMLImageElement | null, theImageId: string, theIsScalable?: boolean | undefined): boolean;
    /** Return TRUE if preloaded image is defined. */
    isImageRegistered(theImageId: string): boolean;
    /** Return TRUE if preloaded image is scalable (e.g. SVG). */
    isImageScalable(theImageId: string): IVec2;
    /** Return dimensions of registered image. */
    getImageSize(theImageId: string, theIsNatural?: boolean): IVec2;
    /** Render registered image and return raw RGBA data buffer to be released by Module._free(). */
    renderImage(theImageId: string, theW: number, theH: number): number;
    /**
     * Dump fonts information.
     * @param {string} theFontName font name to find or empty string to print all fonts
     * @param {boolean} theIsStrict flag to find exact font name
     * @param {boolean} theToPrintDetails flag to print details or only font name
     */
    dumpFontsInfo(theFontName?: string, theIsStrict?: boolean, theToPrintDetails?: boolean): void;
    /** Request View update (using `requestAnimationFrame()` or similar method). */
    requestViewUpdate(): void;
    /** Resize rendering window. */
    resizeViewport(): void;
    /**
     * Set cubemap background. File will be loaded asynchronously.
     * @param {string} theImagePath image path to load
     * @param {boolean} theLightmapOnly
     * @return {Promise} promise with image loading results
     */
    setBackgroundCubemap(theImagePath: string, theLightmapOnly: boolean): Promise<void>;
    /** Print memory stats */
    dumpMemInfo(): void;
    /**
     * Get screenshot.
     * @param {theWidth}  theWidth  of screenshot (`-1` - take size of canvas)
     * @param {theHeight} theHeight of screenshot (`-1` - take size of canvas)
     * @return string in the format image/png or empty
     */
    getScreenshot(theWidth: number, theHeight: number): string;
    /**
     * Make screenshot and download.
     * @param {theFilename}  theFilename file name to download
     * @param {theWidth}  theWidth  of screenshot (`-1` - take size of canvas)
     * @param {theHeight} theHeight of screenshot (`-1` - take size of canvas)
     * @return true on success
     */
    makeScreenshot(theFilename: string, theWidth: number, theHeight: number): boolean;
    /** Make a link to download file from virtual file system. */
    makeFileLink(theFileName: string, theMime?: string): boolean;
    /**
     * Take buffer and upload it to emulated file system.
     * @param {Uint8Array} theBuffer    buffer to load
     * @param {string}     theFilePath  file path on emulated file system (or empty string to take name from URL)
     * @param {boolean}    theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    preloadFile(theBuffer: Uint8Array, theFilePath: string, theToPreload: boolean): Promise<boolean>;
    /**
     * Upload specified ArrayBuffer to emulated file system.
     * To remove file from file system you should call {@link unlinkFile} if it is no longer needed
     * @param {string} theArray    array buffer to load
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     * @param {boolean} theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    uploadArrayBuffer(theArray: ArrayBuffer, theFilePath: string, theToPreload: boolean): Promise<boolean>;
    /**
     * Fetch remote file from specified URL and upload it to emulated file system.
     * To remove file from file system you should call {@link unlinkFile} if it is no longer needed
     * @param {string} theFileUrl  URL to load
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     * @param {boolean} theToPreload decode image file using Emscripten plugins
     * @return {Promise} evaluation result as promise returning TRUE or Error
     */
    uploadUrl(theFileUrl: string, theFilePath: string, theToPreload: boolean): Promise<boolean>;
    /**
     * Unlinks the node at path.
     * This removes a name from the file system.
     * If that name was the last link to a file (and no processes have the file open) the file is deleted.
     * Usually you should call this function after {@link uploadUrl} to remove file which is no longer needed.
     * @param {string} theFilePath file path on emulated file system (or empty string to take name from URL)
     */
    unlinkFile(theFilePath: string): boolean;
    /**
     * Check if file exists in emulated file system.
     */
    checkFile(theFilePath: string): boolean;
    /**
     * C++ `std::cout` callback redirecting to Terminal.
     * @param {string} theText text to print
     */
    print(theText: string): void;
    /**
     * C++ `std::cerr` callback redirecting to Terminal.
     * @param {string} theText text to print
     */
    printErr(theText: string): void;
    /**
     * C++ `Message::Send()` callback redirecting to Terminal.
     * @param {string} theText text to print
     * @param {number} theGravity message gravity within 0..4 range
     */
    printMessage(theText: string, theGravity: MessageGravity): void;
    /**
     * Callback returning file path for loading WebAssembly components.
     * @param {string} thePath file path to locate
     * @param {string} thePrefix default file prefix
     * @return {string} full path to the resource
     */
    locateFile(thePath: string, thePrefix: string): string;
    /** WebAssembly module callback on runtime initialization. */
    onRuntimeInitialized(): void;
    /**
     * Set gravity level for C++ OCCT messenger.
     * @param {number} theGravity lower message gravity to log
     */
    private _cppSetTraceLevel;
    /**
     * Register font.
     * @param {string} theFontPath the font path
     * @param {string} theFontName font name or empty string to keep name from file itself
     * @param {boolean} theToOverride override existing font family
     */
    private _cppRegisterFont;
    /**
     * Dump fonts information.
     * @param {string} theFontName font name to find or empty string to print all fonts
     * @param {boolean} theIsStrict flag to find exact font name
     * @param {boolean} theToPrintDetails flag to print details or only font name
     */
    private _cppDumpFontsInfo;
    /**
     * Call callback by its pointer.
     * @param {number} theCallback pointer to {@link IPromise} object
     */
    private _cppWasmCallBack;
    /**
     * Get global Viewer instance.
     * @return {WasmViewer} viewer instance
     */
    private _cppGetViewer;
    /** Import WASM module. */
    private _importModule;
    /** Load WASM module. */
    private _loadModule;
    /** Load standard fonts. */
    private _loadFonts;
}
