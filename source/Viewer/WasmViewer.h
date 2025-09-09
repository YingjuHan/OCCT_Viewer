#ifndef _WASMVIEWER_HEADERFILE
#define _WASMVIEWER_HEADERFILE

#include "ViewManager.h"
#include "SelectionManager.h"

#include <ViewBase/StatsActivator.h>

#include <RWBase/IPromise.h>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <Message.hxx>
#include <V3d_View.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
#else
    #include <ViewerTest_EventManager.hxx>
#endif

#include <array>
#include <memory>

//! @namespace Viewer
//! Viewer is a C++ WebAssembly module with TypeScript bindings
//! for embedding 3D Viewer (`Viewer::WasmViewer`) into Web applications.
//!
//! TypeScript API has a dedicated documentation, which is generated as part of NPM module.
namespace Viewer {

//! Managers enumeration.
enum class ManagerType {
    UNIT_MANAGER = 0,
    VIEW_MANAGER,
    RENDERING_PARAMS,
    INTERACTIVE_CONTEXT,
    SELECTION_MANAGER,
    ASPECTS_MANAGER,
    CAEMESH_MANAGER,
    SHAPE_MANAGER,
    COLORSCALE_MANAGER,
    CLIPPING_MANAGER,
    SCALERULER_MANAGER,
    TEXTLABEL_MANAGER,
    DIMENSION_MANAGER,
    VIEWCUBE_MANAGER,
    AXES_MANAGER,
    NB_MANAGERS
};

//! Entry class managing 3D Viewer within Emscripten canvas.
class WasmViewer
#ifdef __EMSCRIPTEN__
    : public Standard_Transient,
      public AIS_ViewController {
    using baseViewController = AIS_ViewController;
#else
    : public ViewerTest_EventManager {
    using baseViewController = ViewerTest_EventManager;
#endif
    friend class Viewer::ViewManager;

    class GlFrameStats;
    class EnabledFilter;

    //! @name C++ global methods
public:
    //! Init global instance.
    Standard_EXPORT static Handle(WasmViewer)& Init(const Handle(V3d_View)&               theView,
                                                    const Handle(AIS_InteractiveContext)& theCtx);

    //! Return global viewer instance.
    Standard_EXPORT static Handle(WasmViewer)& GetWasmViewerInstance();

    //! @name WebAssembly module exports
public:
    //! Set message log gravity.
    Standard_EXPORT static void _cppSetTraceLevel(int theGravity);

    //! Register font.
    //! This should be done before displaying any text labels in 3D viewer for proper rendering.
    //! @param[in] theFontFile font file
    //! @param[in] theFontName font name or empty string to keep name from file itself
    //! @param[in] theToOverride override existing font family
    Standard_EXPORT static void _cppRegisterFont(const std::string& theFontFile,
                                                 const std::string& theFontName,
                                                 bool               theToOverride);

    //! Dump registered fonts info.
    //! @param[in] theFontName font name to find or empty string to print all fonts
    //! @param{in} theIsStrict flag to find exact font name
    //! @param{in} theToPrintDetails flag to print details or only font name
    Standard_EXPORT static void _cppDumpFontsInfo(const std::string& theFontName,
                                                  const bool         theIsStrict,
                                                  const bool         theToPrintDetails);

    //! Call callback by its pointer.
    //! @param[in] theCallback pointer to RWBase::IPromise object
    Standard_EXPORT static void _cppWasmCallBack(uintptr_t theCallback);

    //! @name TypeScript properties
public:
    //! Resize window.
    Standard_EXPORT void ResizeViewport();

    //! Force specified device pixel ratio (for testing).
    Standard_EXPORT void SetDevicePixelRatio(float theRatio);

    //! Request view redrawing (using requestAnimationFrame() or similar method).
    Standard_EXPORT void RequestViewUpdate();

    //! Get units manager.
    Viewer::IManager* GetManager(int theType)
    {
        if (theType < 0 || theType >= (int)myManagers.size()) {
            TCollection_AsciiString aMsg = "Internal error: wrong manager index ";
            aMsg += theType;
            throw Standard_OutOfRange(aMsg.ToCString());
        }

        return myManagers[theType].get();
    }

    //! Print memory usage info.
    Standard_EXPORT void DumpMemInfo();

    //! Make screenshot and fill the buffer and return true if success.
    //! @param[in] theWidth width of image (-1 - actual size of canvas)
    //! @param[in] theHeight height of image (-1 - actual size of canvas)
    //! @param[in] theBuffer preallocated buffer to write image
    Standard_EXPORT bool MakeScreenshot(int theWidth, int theHeight, uintptr_t theBuffer);

    //! @name C++ public properties
public:
    //! Default constructor.
    Standard_EXPORT WasmViewer(const Handle(V3d_View)& theView, const Handle(AIS_InteractiveContext)& theCtx);

    //! Destructor.
    Standard_EXPORT virtual ~WasmViewer();

    //! Return interactive context.
    const Handle(AIS_InteractiveContext)& GetContext() const { return myContext; }

    //! Set context for view.
    void SetContext(const Handle(AIS_InteractiveContext)& theCtx) { myContext = theCtx; }

    //! Return main viewer.
    Handle(V3d_Viewer) Viewer() const { return myMainView->Viewer(); }

    //! Return main view.
    const Handle(V3d_View)& MainView() const { return myMainView; }

    //! Return view receiving input focus.
    const Handle(V3d_View)& FocusView() const { return myFocusView; }

    //! Set view receiving input focus.
    Standard_EXPORT void SetFocusView(const Handle(V3d_View)& theView);

    //! Update view controller to focus view parameters.
    Standard_EXPORT void ConfigureFocusView();

    //! Return view.
    const Handle(V3d_View)& View() const { return myMainView; }

    //! Return view background color.
    const Quantity_Color& BackgroundColor() const { return myBackgroundColor; }

    //! Set view background color.
    void SetBackgroundColor(const Quantity_Color& theColor) { myBackgroundColor = theColor; }

    //! Return text style for OSD elements.
    const Handle(Prs3d_TextAspect)& DefaultTextStyle() const { return myTextStyle; }

    //! Return device pixel ratio for handling high DPI displays.
    float DevicePixelRatio() const { return myDevicePixelRatio; }

    //! Return last dragged owner.
    const Handle(SelectMgr_EntityOwner)& LastDraggedOwner() const { return myLastDragOwner; }

    //! Return last dragged object.
    const Handle(AIS_InteractiveObject)& LastDraggedObject() const { return myLastDragObject; }

    //! Main application entry point.
    Standard_EXPORT bool init();

    //! Return canvas element id on HTML page.
    const TCollection_AsciiString& CanvasId() const { return myCanvasId; }

    //! Get (sub)view manager.
    const std::shared_ptr<ViewManager>& GetViewManagerShared() const { return myViewManager; }

    //! Customize debug info within frame stats.
    Standard_EXPORT void SetGlViewFrameStats(const Handle(V3d_View)& theView);

    //! Return view info object.
    const Handle(ViewBase::StatsActivator)& GetStatsActivator() const { return myStatsActivator; }

    //! Return default camera animation duration.
    float GetAnimationDuration() const { return 0.5f; }

    //! Mark all 2d objects to update their position/size on the next draw call.
    void Invalidate2dObjects(const Handle(AIS_InteractiveObject)& theUpdatedObj = nullptr)
    {
        my2dObjectsUpdated    = false;
        myLastUpdated2dObject = theUpdatedObj;
    }

    //! @name C++ public properties
public:
    //! Callback called by handleMoveTo() on Selection in 3D Viewer.
    //! This method is expected to be called from rendering thread.
    Standard_EXPORT virtual void OnSelectionChanged(const Handle(AIS_InteractiveContext)& theCtx,
                                                    const Handle(V3d_View)&               theView) Standard_OVERRIDE;

    //! Callback called by handleMoveTo() on object detection in 3D Viewer.
    //! This method is expected to be called from rendering thread.
    Standard_EXPORT virtual void OnObjectDetected(const Handle(AIS_InteractiveContext)& theCtx,
                                                  const Handle(V3d_View)&               theView) Standard_OVERRIDE;

    //! @name implementation details
private:
    //! Define default text style.
    Standard_EXPORT static Handle(Prs3d_TextAspect) createDefaultTextStyle();

private:
#if !defined(__EMSCRIPTEN__)
    using ViewerTest_EventManager::Context; // hide visibility
#endif

    //! Create window.
    Standard_EXPORT void initWindow();

    //! Flush events and redraw view.
    Standard_EXPORT void redrawView();

    //! Updating objects on various stages of the event loop.
    Standard_EXPORT virtual void updateObjectsBeforeRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                           const Handle(V3d_View)&               theView,
                                                           AIS_RedrawProgress theStage) Standard_OVERRIDE;

    //! Resize View.
    virtual void ProcessConfigure(bool theIsResized) Standard_OVERRIDE;

    //! Handle view redraw.
    virtual void HandleViewEvents(const Handle(AIS_InteractiveContext)& theCtx,
                                  const Handle(V3d_View)&               theView) Standard_OVERRIDE;

    //! Handle view redraw.
    virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                  const Handle(V3d_View)&               theView) Standard_OVERRIDE;

    //! Schedule processing of window input events with the next repaint event.
    virtual void ProcessInput() Standard_OVERRIDE;

    //! Handle DPI changes.
    virtual void ProcessDpiChange() Standard_OVERRIDE;

    //! Handle mouse click.
    virtual bool UpdateMouseClick(const Graphic3d_Vec2i& thePoint,
                                  Aspect_VKeyMouse       theButton,
                                  Aspect_VKeyFlags       theModifiers,
                                  bool                   theIsDoubleClick) Standard_OVERRIDE;

    //! Handle mouse button click.
    virtual bool UpdateMouseButtons(const Graphic3d_Vec2i& thePoint,
                                    Aspect_VKeyMouse       theButtons,
                                    Aspect_VKeyFlags       theModifiers,
                                    bool                   theIsEmulated) Standard_OVERRIDE;

    //! Handle mouse movements.
    virtual bool UpdateMousePosition(const Graphic3d_Vec2i& thePoint,
                                     Aspect_VKeyMouse       theButtons,
                                     Aspect_VKeyFlags       theModifiers,
                                     bool                   theIsEmulated) Standard_OVERRIDE;

    //! Handle key down event.
    virtual void KeyDown(Aspect_VKey theKey, double theTime, double thePressure) Standard_OVERRIDE;

    //! Handle key up event.
    virtual void KeyUp(Aspect_VKey theKey, double theTime) Standard_OVERRIDE;

    //! Pick closest point under mouse cursor.
    virtual bool PickPoint(gp_Pnt&                               thePnt,
                           const Handle(AIS_InteractiveContext)& theCtx,
                           const Handle(V3d_View)&               theView,
                           const Graphic3d_Vec2i&                theCursor,
                           bool                                  theToStickToPickRay) Standard_OVERRIDE;

    //! Compute rotation gravity center point depending on rotation mode.
    virtual gp_Pnt GravityPoint(const Handle(AIS_InteractiveContext)& theCtx,
                                const Handle(V3d_View)&               theView) Standard_OVERRIDE;

    //! Handle object dragging.
    virtual void OnObjectDragged(const Handle(AIS_InteractiveContext)& theCtx,
                                 const Handle(V3d_View)&               theView,
                                 AIS_DragAction                        theAction) Standard_OVERRIDE;

    //! Callback called on Selection of another (sub)view.
    //! This method is expected to be called from rendering thread.
    virtual void OnSubviewChanged(const Handle(AIS_InteractiveContext)& theCtx,
                                  const Handle(V3d_View)&               theOldView,
                                  const Handle(V3d_View)&               theNewView) Standard_OVERRIDE;

    //! Handle zoom event myGL.ZoomActions.
    //! This method is expected to be called from rendering thread.
    virtual void handleZoom(const Handle(V3d_View)&   theView,
                            const Aspect_ScrollDelta& theParams,
                            const gp_Pnt*             thePnt) Standard_OVERRIDE;

    //! Handle mouse picking.
    virtual void contextLazyMoveTo(const Handle(AIS_InteractiveContext)& theCtx,
                                   const Handle(V3d_View)&               theView,
                                   const Graphic3d_Vec2i&                thePnt) Standard_OVERRIDE;

    //! Initialize pixel scale ratio.
    Standard_EXPORT void initPixelScaleRatio();

    //! Update position/size of 2d objects.
    Standard_EXPORT void update2dObjects();

private:
    //! Define default hotkeys.
    Standard_EXPORT void addDefaultActionHotKeys();

    //! Register hot-keys for specified Action.
    void addActionHotKeys2(Aspect_VKey  theAction,
                           unsigned int theHotKey1 = 0,
                           unsigned int theHotKey2 = 0,
                           unsigned int theHotKey3 = 0,
                           unsigned int theHotKey4 = 0)
    {
        if (theHotKey1 != 0) {
            myNavKeyMap2.Bind(theHotKey1, theAction);
        }
        if (theHotKey2 != 0) {
            myNavKeyMap2.Bind(theHotKey2, theAction);
        }
        if (theHotKey3 != 0) {
            myNavKeyMap2.Bind(theHotKey3, theAction);
        }
        if (theHotKey4 != 0) {
            myNavKeyMap2.Bind(theHotKey4, theAction);
        }
    }

    //! Handle navigation keys.
    Standard_EXPORT bool navigationKeyModifierSwitch2(unsigned int theModifOld,
                                                      unsigned int theModifNew,
                                                      double       theTimeStamp);

    //! Handle hot-key.
    Standard_EXPORT bool processKeyPress(Aspect_VKey theKey);

    //! Create 3D Viewer.
    Standard_EXPORT bool initViewer();

    //! Wrap wasm viewer.
    Standard_EXPORT bool initViewerWasm();

    //! Setup view rendering parameters.
    Standard_EXPORT void setupRenderingParams(const Handle(V3d_View)& theView);

    //! Dump WebGL context information.
    Standard_EXPORT void dumpGlInfo(bool theIsBasic, bool theIsTrace);

    //! Setup highlighting.
    Standard_EXPORT void initHighlightStyles();

    //! On view updated.
    Standard_EXPORT void onViewUpdated();

    //! @name Emscripten callbacks
private:
#ifdef __EMSCRIPTEN__
    //! Mouse event.
    EM_BOOL onMouseEvent(int theEventType, const EmscriptenMouseEvent* theEvent);

    //! Scroll event.
    EM_BOOL onWheelEvent(int theEventType, const EmscriptenWheelEvent* theEvent);

    //! Touch event.
    EM_BOOL onTouchEvent(int theEventType, const EmscriptenTouchEvent* theEvent);

    //! Key down event.
    EM_BOOL onKeyDownEvent(int theEventType, const EmscriptenKeyboardEvent* theEvent);

    //! Key up event.
    EM_BOOL onKeyUpEvent(int theEventType, const EmscriptenKeyboardEvent* theEvent);

    //! on focus change event.
    EM_BOOL onFocusChange(int theEventType, const EmscriptenFocusEvent* theEvent);
#endif

private:
    //! Default text resolution.
    static constexpr int DEF_FONT_RESOLUTION = 96;
    //! Default number of MSAA samples.
    static constexpr int DEF_MSAA = 4;
    //! Picking tolerance.
    static constexpr double DEF_PIXEL_TOLERANCE = 2.0;

private:
    Handle(V3d_View)               myMainView;        //!< 3D view
    Handle(V3d_View)               myFocusView;       //!< 3D view with input focus
    Handle(AIS_InteractiveContext) myContext;         //!< interactive context
    Handle(Prs3d_TextAspect)       myTextStyle;       //!< text style for OSD elements
    Quantity_Color                 myBackgroundColor; //!< view background color

    TCollection_AsciiString myCanvasId;                  //!< canvas element id on HTML page
    Graphic3d_Vec2i         myWinSizeOld;                //!< cached window size
    Standard_ShortReal      myMaxCamDistance    = 10.0f; //!< zoom distance limit
    Standard_ShortReal      myDevicePixelRatio  = 1.0f;  //!< device pixel ratio for handling high DPI displays
    unsigned int            myNbUpdateRequests2 = 0;     //!< counter for unhandled update requests
    Standard_Integer        myToAllowHighlightBack =
        -1; //!< to allow highlight backing store state (-1-not set, 1 allow, 0 not allow)

    std::array<std::shared_ptr<IManager>, (int)ManagerType::NB_MANAGERS> myManagers;

    std::shared_ptr<ViewManager> myViewManager; //!< subview manager
    std::shared_ptr<SelectionManager> mySelManager; //!< selection manager

    Handle(ViewBase::StatsActivator) myStatsActivator; //!< view info object

    AIS_MouseSelectionSchemeMap myMouseDefaultSchemesMap;        //!< using for backup of myMouseSelectionSchemes
    NCollection_DataMap<unsigned int, Aspect_VKey> myNavKeyMap2; //!< map of Hot-Key (key+modifiers) to Action

    Handle(SelectMgr_EntityOwner) myLastDragOwner;        //!< last dragged owner
    Handle(AIS_InteractiveObject) myLastDragObject;       //!< last dragged object
    Standard_Boolean              myIsDragStarted = false;
    Standard_Boolean              myIsLockedDrag = false; //!< dragging event was suppressed

    bool                          my2dObjectsUpdated = false;
    Handle(AIS_InteractiveObject) myLastUpdated2dObject;

    bool myDummyVar = false; //!< dummy variable to suppress `function could be made const` warning
};

} // namespace

#endif // _WASMVIEWER_HEADERFILE