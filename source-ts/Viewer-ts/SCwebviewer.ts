import {Camera} from './Camera.js';
import type {IManager, IVec2, CameraOrientation, ColorString, IPromise, UnitManager} from './CommonTools.js';
import {UnitManagerExtensions} from './CommonTools.js';
import {InteractiveContextExtensions, SelectionManagerExtensions} from './InteractiveObject.js';
import type {InteractiveAspectsManager, InteractiveContext, SelectionManager} from './InteractiveObject.js';
import type {InteractiveColorScaleManager} from './InteractiveColorScale.js';
import type {InteractiveShapeManager} from './InteractiveShape.js';
import {InteractiveShapeManagerExtensions} from './InteractiveShape.js';
import type {InteractiveCaeMeshManager} from './InteractiveCaeMeshManager.js';
import type {InteractiveClippingManager} from './InteractiveClipping.js';
import {InteractiveScaleRuler} from './InteractiveScaleRuler.js';
import type {InteractiveScaleRulerManager} from './InteractiveScaleRuler.js';
import type {InteractiveTextLabelManager} from './InteractiveTextLabel.js';
import type {InteractiveDimensionManager} from './InteractiveDimension.js';
import { InteractiveSphericalAxes } from './InteractiveSphericalAxes.js';
import type { InteractiveAxesManager } from './InteractiveSphericalAxes.js';
import {InteractiveViewCube} from './InteractiveViewCube.js';
import type {InteractiveViewCubeManager} from './InteractiveViewCube.js';
import {ViewerEventManager} from './ViewerEventManager.js';
import type {ViewManager} from './ViewManager.js';
import {Subview, ViewManagerExtensions} from './Subview.js';
import {ViewerBuildConfig} from './ViewerBuildConfig.js';

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
export * from './SceneGraph.js';
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
export enum MessageGravity {
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
};

/**
 * This enumeration defines rendering transparency method.
 * It reflects values of corresponding C++ `Graphic3d_RenderTransparentMethod` enumeration defined by OCCT.
 */
export enum RenderTransparentMethod {
  /** Basic blend transparency with non-commuting blend operator without sorting. */
  Unordered = 0,
  /** Weighted Blended Order-Independent Transparency. */
  BlendOit = 1,
  /** Depth Peeling. */
  DepthPeelingOit = 2
};

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
};

/** @internal Managers enumeration. */
enum ManagerType {
  UNIT_MANAGER = 0,
  VIEW_MANAGER = 1,
  RENDERING_PARAMS = 2,
  INTERACTIVE_CONTEXT = 3,
  SELECTION_MANAGER = 4,
  ASPECTS_MANAGER = 5,
  CAEMESH_MANAGER = 6,
  SHAPE_MANAGER = 7,
  COLORSCALE_MANAGER = 8,
  CLIPPING_MANAGER = 9,
  SCALERULER_MANAGER = 10,
  TEXTLABEL_MANAGER = 11,
  DIMENSION_MANAGER = 12,
  VIEWCUBE_MANAGER = 13,
  AXES_MANAGER = 14,
  NB_MANAGERS = 15
};

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

  /**
   * @internal Get manager object.
   * @return {IManager} manager object from {@link ManagerType} enumeration
   */
  getManager: (theType: number) => IManager;
};

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
  /** Flag defining `main()` return behavior (read by Emscripten Module). */
  public noExitRuntime: boolean;

  /** 'canvas' element for WebGL context (read by Emscripten Module). */
  public canvas: HTMLCanvasElement;

  /** @internal wrapper of c++ vector of {@link IVec3}. */
  public Vec3Vector: any;

  /** @internal wrapper of c++ vector of `float`. */
  public FloatVector: any;

  /** @internal wrapper of c++ vector of `double`. */
  public DoubleVector: any;

  /** @internal wrapper of c++ vector of {@link IMeshEntityBasicInfo}. */
  public MeshEntityBasicInfoVector: any;

  /** @internal wrapper of c++ vector of {@link ISelectionMeshEntity}. */
  public SelectionMeshEntityVector: any;

  /** @internal wrapper of c++ vector of {@link ShapeInfo}. */
  public ShapeInfoVector: any;

  /** @internal wrapper of c++ vector of {@link IVec4}. */
  public Vec4Vector: any;

  /** @internal wrapper of c++ vector of {@link IVec2}. */
  public Vec2Vector: any;

  /** @internal wrapper of c++ vector of `int`. */
  public IntVector: any;

  /** @internal wrapper of c++ vector of 'string'. */
  public StringVector: any;

  /** @internal wrapper of c++ map of 'int, string'. */
  public IntStringMap: any;

  /** Implementation of {@link IPromise} type. */
  IPromise: IPromise;

  /** Implementation of {@link CameraOrientation}. */
  CameraOrientation: typeof CameraOrientation;

  private _myBaseFolder: string;
  private _myBasePrefix: string;
  private _myCanvasId: string;
  private _myIsWasmLoaded: boolean;
  private _myToPreferPthread: boolean;
  private _myIsPthreadLoaded: boolean;
  private mainScriptUrlOrBlob: string;
  private _myTraceLevel: number;
  private _myEventManager: ViewerEventManager;
  private _myWasmViewer: WasmViewer | null;
  private _myUnitManager: UnitManager | null;
  private _myViewManager: ViewManager | null;
  private _myRenderParams: RenderingParams | null;
  private _mySelectionManager: SelectionManager | null;
  private _myInteractiveContext: InteractiveContext | null;
  private _myInteractiveAspectsManager: InteractiveAspectsManager | null;
  private _myInteractiveCaeMeshManager: InteractiveCaeMeshManager | null;
  private _myInteractiveShapeManager: InteractiveShapeManager | null;
  private _myInteractiveColorScaleManager: InteractiveColorScaleManager | null;
  private _myInteractiveClippingManager: InteractiveClippingManager | null;
  private _myInteractiveScaleRulerManager: InteractiveScaleRulerManager | null;
  private _myInteractiveTextLabelManager: InteractiveTextLabelManager | null;
  private _myInteractiveDimensionManager: InteractiveDimensionManager | null;
  private _myInteractiveViewCubeManager: InteractiveViewCubeManager | null;
  private _myInteractiveAxesManager: InteractiveAxesManager | null;
  private _myRootView: Subview | null;
  private _myFocusView: Subview | null;
  private _myFocusCamera: Camera | null;
  private _myScaleRuler: InteractiveScaleRuler | null;
  private _myViewCube: InteractiveViewCube | null;

  private _myImageMap: Map<string, HTMLImageElement>;
  private _myImageCanvas: HTMLCanvasElement | null;
  private _myToTraceImages: boolean;

  /** Memory heap (forward declaration, exported by Module). */
  private HEAPU8: Uint8Array;

  /** Entry to emulated File System API (forward declaration, exported by Emscripten Module). */
  private FS: any;

  /** Emulated File System helper (forward declaration, exported by Emscripten Module). */
  private PATH: any;

  /**
   * Main constructor.
   * @param {string} theBaseDataPath path to folder that contains
   *        'wasm32' + 'wasm32-pthread' folders (WASM files),
   *        'fonts' folder with default font files.
   */
  constructor(theBaseDataPath: string) {
    this._myImageMap = new Map<string, HTMLImageElement>();
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
    this.canvas = document.getElementById(this._myCanvasId) as HTMLCanvasElement;
    this.canvas.tabIndex = -1;
    this.canvas.onclick = (theEvent): void => { this.canvas.focus(); };
    const aViewer = this;
    Object.defineProperty(this.canvas, 'scViewer', { get: function(): SCwebviewer { return aViewer; }});

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
  public get isWasmLoaded(): boolean { return this._myIsWasmLoaded; }

  /** Return TRUE if pthread version should be loaded when supported and allowed by Browser. */
  public get toPreferPthread(): boolean { return this._myToPreferPthread; }

  /** Check if pthread version of viewer is loaded. */
  public get isPthreadLoaded(): boolean { return this._myIsPthreadLoaded; }

  /**
   * Get gravity level for logging messages.
   * @return {MessageGravity} lower message gravity to log
   */
  public get traceLevel(): MessageGravity { return this._myTraceLevel; }

  /** Flag to enable extra verbose logging. */
  public get toTraceImages(): boolean { return this._myToTraceImages; }

  /**
   * Get subview manager.
   * @return {ViewManager} subview manager object
   */
  public get viewManager(): ViewManager | null { return this._myViewManager; }

  /**
   * Get Camera manager.
   * @return {Camera} Camera manager object
   */
  public get camera(): Camera | null { return this._myFocusCamera; }

  /**
   * Get root view object (either main view itself or composer of subviews).
   * @return {Subview} View object
   */
  public get rootView(): Subview | null { return this._myRootView; }

  /**
   * Get view object receiving focus.
   * @return {Subview} View object
   */
  public get focusView(): Subview | null { return this._myFocusView; }

  /**
   * Get Rendering Parameters manager.
   * @return {RenderingParams} Rendering Parameters manager object
   */
  public get renderingParams(): RenderingParams | null { return this._myRenderParams; }

  /**
   * Get unit conversion manager.
   * @return {UnitManager} unit conversion manager
   */
  public get unitManager(): UnitManager | null { return this._myUnitManager; }

  /**
   * Get Object manager.
   * @return {InteractiveContext} Object manager
   */
  public get interactiveContext(): InteractiveContext | null { return this._myInteractiveContext; }

  /**
   * Get selection manager.
   * @return {SelectionManager} selection manager
   */
  public get selectionManager(): SelectionManager | null { return this._mySelectionManager; }

  /**
   * Get event manager - an interface for subscribing and unsubscribing to various callbacks (like selection changes).
   * @return {ViewerEventManager} event manager
   */
  public get eventManager(): ViewerEventManager { return this._myEventManager; }

  /**
   * @internal Get aspects manager.
   * @return {InteractiveAspectsManager} aspects manager
   */
  public get interactiveAspectsManager(): InteractiveAspectsManager | null { return this._myInteractiveAspectsManager; }

  /**
   * @internal Get CAE mesh manager.
   * @return {InteractiveCaeMeshManager} CAE mesh manager
   */
  public get interactiveCaeMeshManager(): InteractiveCaeMeshManager | null { return this._myInteractiveCaeMeshManager; }

  /**
   * @internal Get shape manager.
   * @return {InteractiveShapeManager} shape manager
   */
  public get interactiveShapeManager(): InteractiveShapeManager | null { return this._myInteractiveShapeManager; }

  /**
   * @internal Get colorscale manager.
   * @return {InteractiveColorScaleManager} colorscale manager
   */
  public get interactiveColorScaleManager(): InteractiveColorScaleManager | null { return this._myInteractiveColorScaleManager; }

  /**
   * @internal Get clipping box manager.
   * @return {InteractiveClippingManager} clipping box manager
   */
  public get interactiveClippingManager(): InteractiveClippingManager | null { return this._myInteractiveClippingManager; }

  /**
   * @internal Get scale ruler manager.
   * @return {InteractiveScaleRulerManager} scale ruler manager
   */
  public get interactiveScaleRulerManager(): InteractiveScaleRulerManager | null { return this._myInteractiveScaleRulerManager; }

  /**
   * @internal Get text label manager.
   * @return {InteractiveTextLabelManager} text label manager
   */
  public get interactiveTextLabelManager(): InteractiveTextLabelManager | null { return this._myInteractiveTextLabelManager; }

  /**
   * @internal Get dimension manager.
   * @return {InteractiveDimensionManager} dimensioon manager
   */
  public get interactiveDimensionManager(): InteractiveDimensionManager | null { return this._myInteractiveDimensionManager; }

   /**
   * @internal Get viewcube manager.
   * @return {InteractiveViewCubeManager} viewcube manager
   */
  public get interactiveViewCubeManager(): InteractiveViewCubeManager | null { return this._myInteractiveViewCubeManager; }

  /**
   * @internal Get axes manager.
   * @return {InteractiveAxesManager} axes manager
   */
  public get interactiveAxesManager(): InteractiveAxesManager | null { return this._myInteractiveAxesManager; }

  /**
   * Get InteractiveScaleRuler object.
   * @return {InteractiveScaleRuler} InteractiveScaleRuler object
   */
  public get scaleRuler(): InteractiveScaleRuler | null { return this._myScaleRuler; }

  /**
   * Get InteractiveViewCube object.
   * @return {InteractiveViewCube} InteractiveViewCube object
   */
  public get viewCube(): InteractiveViewCube | null { return this._myViewCube; }

  /** @internal generate screenshot (short-cut for diagnostics). */
  private get doScreenshot(): boolean { return this.makeScreenshot('viewer.png', -1, -1); }

  /**
   * Set gravity level for logging messages.
   * @param {MessageGravity} theGravity lower message gravity to log
   */
  public set traceLevel(theGravity: MessageGravity) {
    // Messages are filtered by both {@link SCwebviewer.printMessage} and `Message_Printer::send()`.
    // Propagating filter level to Message_Printer might improve performance by skipping extra function calls for too verbose messages.
    this._myTraceLevel = theGravity.valueOf();
    if (this._myIsWasmLoaded) {
      this._cppSetTraceLevel(theGravity.valueOf());
    }
  }

  /** Flag to enable extra verbose logging. */
  public set toTraceImages(theToEnable: boolean) { this._myToTraceImages = theToEnable; }

  /**
   * Set if pthread version should be loaded when supported by Browser.
   * Should be set BEFORE SCwebviewer.init()!
   */
  public set toPreferPthread(theToPrefer: boolean) { this._myToPreferPthread = theToPrefer; }

  /** Check browser support and return TRUE if WASM supported. */
  public isWasmSupported(): boolean {
    try {
      if (typeof WebAssembly === 'object' && typeof WebAssembly.instantiate === 'function') {
        const aDummyModule = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00));
        if (aDummyModule instanceof WebAssembly.Module) {
          return (new WebAssembly.Instance(aDummyModule) instanceof WebAssembly.Instance);
        }
      }
    } catch (theErr) {
      // skip errors
    }
    return false;
  }

  /** Check browser support and return TRUE if WASM64 supported. */
  public isWasm64Supported(): boolean {
    try {
      if (typeof WebAssembly === 'object' && typeof WebAssembly.instantiate === 'function') {
        // this WAT might look wrong, but enough for checking WASM64 support
        // '(module (memory i64 1) )'
        const aDummyModule = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x05, 0x03, 0x01, 0x04, 0x01));
        if (aDummyModule instanceof WebAssembly.Module) {
          return (new WebAssembly.Instance(aDummyModule) instanceof WebAssembly.Instance);
        }
      }
    } catch (theErr) {
      // skip errors
    }
    return false;
  }

  /** Check if multithreading is supported and allowed by Browser. */
  public isPthreadAllowed(): boolean {
    return window.Worker !== undefined && window.Atomics !== undefined &&
           window.SharedArrayBuffer !== undefined;
  }

  /** Init {@link SCwebviewer} Module and load wasm file. */
  public async init(): Promise<void> {
    return new Promise(async (resolve, reject) => {
      try {
        await this._loadModule('Viewer-wasm.js');

        // load standard fonts
        await this._loadFonts();

        this._myWasmViewer = this._cppGetViewer();

        this._myUnitManager = this._myWasmViewer.getManager(ManagerType.UNIT_MANAGER) as UnitManager;
        UnitManagerExtensions.init(this._myUnitManager);

        this._myRenderParams = this._myWasmViewer.getManager(ManagerType.RENDERING_PARAMS) as RenderingParams;
        this._myInteractiveAspectsManager = this._myWasmViewer.getManager(ManagerType.ASPECTS_MANAGER) as InteractiveAspectsManager;
        this._myInteractiveCaeMeshManager = this._myWasmViewer.getManager(ManagerType.CAEMESH_MANAGER) as InteractiveCaeMeshManager;
        this._myInteractiveShapeManager = this._myWasmViewer.getManager(ManagerType.SHAPE_MANAGER) as InteractiveShapeManager;
        InteractiveShapeManagerExtensions.init(this, this._myInteractiveShapeManager);
        this._myInteractiveColorScaleManager = this._myWasmViewer.getManager(ManagerType.COLORSCALE_MANAGER) as InteractiveColorScaleManager;
        this._myInteractiveClippingManager = this._myWasmViewer.getManager(ManagerType.CLIPPING_MANAGER) as InteractiveClippingManager;
        this._myInteractiveScaleRulerManager = this._myWasmViewer.getManager(ManagerType.SCALERULER_MANAGER) as InteractiveScaleRulerManager;
        this._myInteractiveTextLabelManager = this._myWasmViewer.getManager(ManagerType.TEXTLABEL_MANAGER) as InteractiveTextLabelManager;
        this._myInteractiveDimensionManager = this._myWasmViewer.getManager(ManagerType.DIMENSION_MANAGER) as InteractiveDimensionManager;
        this._myInteractiveViewCubeManager = this._myWasmViewer.getManager(ManagerType.VIEWCUBE_MANAGER) as InteractiveViewCubeManager;
        this._myInteractiveAxesManager = this._myWasmViewer.getManager(ManagerType.AXES_MANAGER) as InteractiveAxesManager;

        this._myInteractiveContext = this._myWasmViewer.getManager(ManagerType.INTERACTIVE_CONTEXT) as InteractiveContext;
        InteractiveContextExtensions.init(this, this._myInteractiveContext);

        this._mySelectionManager = this._myWasmViewer.getManager(ManagerType.SELECTION_MANAGER) as SelectionManager;
        SelectionManagerExtensions.init(this, this._mySelectionManager);

        this._myViewManager = this._myWasmViewer.getManager(ManagerType.VIEW_MANAGER) as ViewManager;
        ViewManagerExtensions.init(this, this._myViewManager);

        this._myFocusCamera = new Camera(this, -1);
        this._myRootView = new Subview();
        this._myRootView.wrap(this, 0);
        this._myFocusView = new Subview();
        this._myFocusView.wrap(this, -1);
        this._myScaleRuler = new InteractiveScaleRuler(this);
        this._myViewCube = new InteractiveViewCube(this);
        resolve();
      } catch (err) {
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
  public async registerFontFromUrl(theFontUrl: string,
                                   theFontName: string = '',
                                   theToOverride: boolean = false): Promise<void> {
    const aPaths = theFontUrl.split('/');
    const aFileName = `/fonts/${aPaths.pop()}`;
    await this.uploadUrl(theFontUrl, aFileName, false);
    this._cppRegisterFont(aFileName, theFontName, theToOverride);
  }

  /**
   * Load and register SVG image to be accessible by viewer.
   * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
   */
  public async registerImageFromUrl(theImageURl: string,
                                    theImageId: string = '',
                                    theIsScalable: boolean | undefined = undefined): Promise<void> {
    let aViewer = this;
    return new Promise<void>((theResolve, theReject) => {
      let anSvgImg = new Image();
      anSvgImg.onload = function(): void {
        if (aViewer.registerImage(anSvgImg, theImageId !== '' ? theImageId : theImageURl, theIsScalable)) {
          theResolve();
        } else {
          theReject();
        }
      };
      anSvgImg.onerror = function(): void {
        theReject();
      };
      anSvgImg.src = theImageURl;
    });
  }

  /**
   * Register image to be accessible by viewer.
   * This interface is designed for handling scalable image formats like SVG, but will handle bitmaps as well.
   */
  public registerImage(theImage: HTMLImageElement | null,
                       theImageId: string,
                       theIsScalable: boolean | undefined = undefined): boolean {
    if (theImage == null) {
      if (this._myImageMap.delete(theImageId)) {
        if (theImageId.startsWith('/')) { this.unlinkFile(theImageId); }
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
    (theImage as any).isScalable = isScalable;
    if (this._myToTraceImages) {
      const aNativeSize = theImage.naturalWidth !== theImage.width || theImage.naturalHeight !== theImage.height ?
                          ` [native: ${theImage.naturalWidth}x${theImage.naturalHeight}]` : '';
      const aScalStr = isScalable ? ' [scalable]' : ' [non-scalable]';
      this.printMessage(`Registered image '${theImageId}' ${theImage.width}x${theImage.height}${aScalStr}${aNativeSize}`,
                        MessageGravity.Trace.valueOf());
    }
    this._myImageMap.set(theImageId, theImage);

    // create a dummy file
    if (theImageId.startsWith('/')) {
      this.preloadFile(Uint8Array.of(0x0), theImageId, false);
    }
    return true;
  }

  /** Return TRUE if preloaded image is defined. */
  public isImageRegistered(theImageId: string): boolean { return this._myImageMap.has(theImageId); }

  /** Return TRUE if preloaded image is scalable (e.g. SVG). */
  public isImageScalable(theImageId: string): IVec2 {
    let anImg: HTMLImageElement | undefined = this._myImageMap.get(theImageId);
    return anImg && (anImg as any).isScalable;
  }

  /** Return dimensions of registered image. */
  public getImageSize(theImageId: string, theIsNatural: boolean = false): IVec2 {
    let anImg: HTMLImageElement | undefined = this._myImageMap.get(theImageId);
    if (!anImg) { return {x: 0, y: 0}; }
    return theIsNatural ?
           {x: anImg.naturalWidth, y: anImg.naturalHeight} :
           {x: anImg.width, y: anImg.height};
  }

  /** Render registered image and return raw RGBA data buffer to be released by Module._free(). */
  public renderImage(theImageId: string, theW: number, theH: number): number {
    let anImg: HTMLImageElement | undefined = this._myImageMap.get(theImageId);
    if (!anImg) {
      this.printMessage(`Error: Unable to render unregistered image '${theImageId}'`,
                        MessageGravity.Fail.valueOf());
      // @c throw new Error("Unregistered image '" + theImageId + "'");
      return 0;
    }

    // create a canvas to draw pixels
    if (this._myImageCanvas == null) { this._myImageCanvas = document.createElement('canvas'); }
    const aCtx: CanvasRenderingContext2D | null = this._myImageCanvas.getContext('2d', { willReadFrequently: true });
    if (!aCtx) { throw new Error('Unable to create 2D canvas'); }

    let aSize = { dx: theW, dy: theH };
    if (theW === 0 && theH === 0) { aSize = { dx: anImg.width, dy: anImg.height }; }
    this._myImageCanvas.width = aSize.dx;
    this._myImageCanvas.height = aSize.dy;

    aCtx.drawImage(anImg, 0, 0, aSize.dx, aSize.dy);
    let anImgData = aCtx.getImageData(0, 0, aSize.dx, aSize.dy);
    let aRawMem = this._allocateArrayU8(new Uint8Array(anImgData.data));
    if (this._myToTraceImages) {
      this.printMessage(`Rendered image '${theImageId}' ${anImg.width}x${anImg.height} at ${aSize.dx}x${aSize.dy}`,
                        MessageGravity.Trace.valueOf());
    }
    return aRawMem;
  }

  /**
   * Dump fonts information.
   * @param {string} theFontName font name to find or empty string to print all fonts
   * @param {boolean} theIsStrict flag to find exact font name
   * @param {boolean} theToPrintDetails flag to print details or only font name
   */
  public dumpFontsInfo(theFontName: string = '',
                       theIsStrict: boolean = false,
                       theToPrintDetails: boolean = true): void {
    this._cppDumpFontsInfo(theFontName, theIsStrict, theToPrintDetails);
  }

  /** Request View update (using `requestAnimationFrame()` or similar method). */
  public requestViewUpdate(): void { this._myWasmViewer?.requestViewUpdate(); }

  /** Resize rendering window. */
  public resizeViewport(): void { this._myWasmViewer?.resizeViewport(); }

  /**
   * Set cubemap background. File will be loaded asynchronously.
   * @param {string} theImagePath image path to load
   * @param {boolean} theLightmapOnly
   * @return {Promise} promise with image loading results
   */
  public async setBackgroundCubemap(theImagePath: string,
                                    theLightmapOnly: boolean): Promise<void> {
    const aFilePath = '/emulated/cubemap.jpg';
    await this.uploadUrl(theImagePath, aFilePath, true);
    this._myRenderParams?.setBackgroundCubemap(aFilePath, theLightmapOnly);
  }

  /** Print memory stats */
  public dumpMemInfo(): void {
    this._myWasmViewer?.dumpMemInfo();
  }

  /**
   * Get screenshot.
   * @param {theWidth}  theWidth  of screenshot (`-1` - take size of canvas)
   * @param {theHeight} theHeight of screenshot (`-1` - take size of canvas)
   * @return string in the format image/png or empty
   */
  public getScreenshot(theWidth: number, theHeight: number): string {
    const aWidth = theWidth === -1 ? this.canvas.width : theWidth;
    const aHeight = theHeight === -1 ? this.canvas.height : theHeight;

    const aByteSize = aWidth * aHeight * 4;
    const aDataBuffer = this._malloc(aByteSize);

    // make screenshot
    if (!this._myWasmViewer?.makeScreenshot(aWidth, aHeight, aDataBuffer)) {
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
  public makeScreenshot(theFilename: string, theWidth: number, theHeight: number): boolean {
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
  public makeFileLink(theFileName: string, theMime: string = 'application/octet-stream'): boolean {
    const aData = this.FS.readFile(theFileName);

    const aBlob = new Blob([aData], {type: theMime});
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
  public _allocateArrayU8(theArray: Uint8Array): number {
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
  public preloadFile(theBuffer: Uint8Array, theFilePath: string, theToPreload: boolean): Promise<boolean> {
    return new Promise(
      (theResolve, theReject) => {
        let aDestDir = this.PATH.dirname(theFilePath);
        this.FS.mkdirTree(aDestDir);
        this.FS.writeFile(theFilePath, theBuffer);
        if (theToPreload) {
          // decode image
          this.FS.createPreloadedFile(
            !theFilePath.startsWith('/') ? this.FS.cwd() : '/',
            theFilePath, theBuffer, true, true,
            () => { theResolve(true); },
            () => { theReject(new Error(`Preload '${theFilePath}' failed`)); },
            true); // file is already created
        } else {
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
  public uploadArrayBuffer(theArray: ArrayBuffer, theFilePath: string, theToPreload: boolean): Promise<boolean> {
    let aPathSplit = theFilePath.split('/');
    let aFileName = theFilePath;
    if (aPathSplit.length > 1) {
      aFileName = aPathSplit[aPathSplit.length - 1];
    }

    let aDataArray = new Uint8Array(theArray);
    this.printMessage(`uploading file '${aFileName}' of size ${aDataArray.length} bytes to '${theFilePath}'...`,
      MessageGravity.Trace.valueOf());
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
  public uploadUrl(theFileUrl: string, theFilePath: string, theToPreload: boolean): Promise<boolean> {
    let aPathSplit = theFileUrl.split('/');
    let aFileName = theFileUrl;
    if (aPathSplit.length > 1) {
      aFileName = aPathSplit[aPathSplit.length - 1];
    }

    let aFilePath = theFilePath;
    if (aFilePath === '') {
      aFilePath = aFileName;
    }

    const aCheckStatusFunc = function (theResponse: any): any {
      if (!theResponse.ok) {
        throw new Error(`HTTP ${theResponse.status} - ${theResponse.statusText} (URL: '${theFileUrl}')`);
      }
      return theResponse;
    };

    return new Promise(
      (theResolve, theReject) => {
        fetch(theFileUrl)
          .then((theResponse) => aCheckStatusFunc(theResponse) && theResponse.arrayBuffer())
          .then((theBuffer) => {
            let aDataArray = new Uint8Array(theBuffer);
            this.printMessage(`uploading file '${aFileName} ' of size ${aDataArray.length} bytes to '${aFilePath}'...`,
              MessageGravity.Trace.valueOf());
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
  public unlinkFile(theFilePath: string): boolean {
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
  public checkFile(theFilePath: string): boolean {
    return this.FS.analyzePath(theFilePath, true).exists;
  }

  // #region Module interface
  /**
   * C++ `std::cout` callback redirecting to Terminal.
   * @param {string} theText text to print
   */
  public print(theText: string): void { this.printMessage(theText, MessageGravity.Info); }

  /**
   * C++ `std::cerr` callback redirecting to Terminal.
   * @param {string} theText text to print
   */
  public printErr(theText: string): void { this.printMessage(theText, MessageGravity.Fail); }

  /**
   * C++ `Message::Send()` callback redirecting to Terminal.
   * @param {string} theText text to print
   * @param {number} theGravity message gravity within 0..4 range
   */
  public printMessage(theText: string, theGravity: MessageGravity): void {
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
  public locateFile(thePath: string, thePrefix: string): string {
    return this._myBasePrefix + thePath;
  }

  /** WebAssembly module callback on runtime initialization. */
  public onRuntimeInitialized(): void {
    // @c console.log(" @@ onRuntimeInitialized()" + Object.getOwnPropertyNames (this));
  }

  // #endregion

  // #region C++ methods exported by Module (forward declarations)

  /**
   * @internal Memory allocation within C++ heap (forward declaration).
   * @param {number} theSize number of bytes to allocation
   * @return {number} offset within the heap
   */
  public _malloc(theSize: number): number { throw new Error('Method not implemented'); }

  /**
   * @internal Memory deallocation within C++ heap (forward declaration).
   * @param {number} theOffset offset within the heap
   */
  public _free(theOffset: number): number { throw new Error('Method not implemented'); }

  /**
   * Set gravity level for C++ OCCT messenger.
   * @param {number} theGravity lower message gravity to log
   */
  private _cppSetTraceLevel(theGravity: number): void { throw new Error('Method not implemented'); }

  /**
   * Register font.
   * @param {string} theFontPath the font path
   * @param {string} theFontName font name or empty string to keep name from file itself
   * @param {boolean} theToOverride override existing font family
   */
  private _cppRegisterFont(theFontPath: string, theFontName: string, theToOverride: boolean): void {
    throw new Error('Method not implemented');
  }

  /**
   * Dump fonts information.
   * @param {string} theFontName font name to find or empty string to print all fonts
   * @param {boolean} theIsStrict flag to find exact font name
   * @param {boolean} theToPrintDetails flag to print details or only font name
   */
  private _cppDumpFontsInfo(theFontName: string,
                            theIsStrict: boolean,
                            theToPrintDetails: boolean): void {
    throw new Error('Method not implemented');
  }

  /**
   * Call callback by its pointer.
   * @param {number} theCallback pointer to {@link IPromise} object
   */
  private _cppWasmCallBack(theCallback: number): void { throw new Error('Method not implemented'); }

  /**
   * Get global Viewer instance.
   * @return {WasmViewer} viewer instance
   */
  private _cppGetViewer(): WasmViewer { throw new Error('Method not implemented'); }

  // #endregion

  /** Import WASM module. */
  private async _importModule(theModuleName: string, theWasmCfg: string): Promise<any> {
    this._myBasePrefix = `${this._myBaseFolder}${theWasmCfg}/`;
    this.mainScriptUrlOrBlob = `${this._myBasePrefix}${theModuleName}`;

    const isPThreaded: boolean = theWasmCfg.includes('pthread');
    if (!isPThreaded) {
      let ret = await import( /* webpackIgnore: true */ this._myBasePrefix + theModuleName);
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
    let AMod = await import( /* webpackIgnore: true */ URL.createObjectURL(aBlob));
    this._myIsPthreadLoaded = true;
    this._myIsWasmLoaded = true;
    return AMod;
  }

  /** Load WASM module. */
  private async _loadModule(theModuleName: string): Promise<void> {
    // ["wasm64-pthread", "wasm64", "wasm32-pthread", "wasm32"]
    const aWasmCfgList: string[] = ViewerBuildConfig.wasmConfigs;
    let aLastErr = '';
    for (let aWasmCfgIter of aWasmCfgList) {
      const isPThreaded: boolean = aWasmCfgIter.includes('pthread');
      const isWasm64: boolean = aWasmCfgIter.includes('wasm64');
      if (isPThreaded && !(this._myToPreferPthread && this.isPthreadAllowed())) {
        continue; // unsupported or ignored
      }
      if (isWasm64 && !this.isWasm64Supported()) {
        continue; // unsupported or ignored
      }

      let aBackProps: any = null;
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
      } catch (err) {
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
  private async _loadFonts(): Promise<void> {
    const aFontsFolder = `${this._myBaseFolder}fonts/`;
    const aMonoFamily: string[] = ['UbuntuMono-R.ttf'];
    // skip italic and bold-italic to spare downloads - they could be synthesized
    // @c "OpenSans-Italic.ttf"
    const aSansFamily: string[] = ['OpenSans-Regular.ttf', 'OpenSans-Bold.ttf'];
    const aCjkFamily: string[] = ['HarmonyOS_Sans_SC_Regular.woff'];

    for (let aFontIter of aMonoFamily) {
      try {
        await this.registerFontFromUrl(aFontsFolder + aFontIter, 'monospace');
      } catch (err) {
        // skip errors
      }
    }

    for (let aFontIter of aSansFamily) {
      try {
        await this.registerFontFromUrl(aFontsFolder + aFontIter, 'sans-serif');
      } catch (err) {
        // skip errors
      }
    }

    // CJK fonts are large, it might be better loading them only when GUI settings are set to Chinese
    for (let aFontIter of aCjkFamily) {
      try {
        await this.registerFontFromUrl(aFontsFolder + aFontIter, 'cjk');
      } catch (err) {
        // skip errors
      }
    }
  }
};
