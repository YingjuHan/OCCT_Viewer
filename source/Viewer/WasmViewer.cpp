#include "WasmViewer.h"

#include "OCCTViewerConfig.h"

#include "InteractiveContext.h"

#include "InteractiveAspectsManager.h"
#include "InteractiveAxesManager.h"
#include "InteractiveCaeMeshManager.h"
#include "InteractiveClippingManager.h"
#include "InteractiveColorScaleManager.h"
#include "InteractiveDimensionManager.h"
#include "InteractiveScaleRulerManager.h"
#include "InteractiveShapeManager.h"
#include "InteractiveTextLabelManager.h"
#include "InteractiveViewCubeManager.h"
#include "SelectionManager.h"
#include "ViewManager.h"
#include "RenderingParams.h"
#include "UnitManager.h"

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/BaseSelection.h>
#include <ViewBase/BaseViewer.h>
#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveOwner.h>
#include <ViewBase/StatsActivator.h>

#include <RWBase/IPromise.h>
#include <RWBase/TaskManager.h>

#include <ViewCae/ColorScaleTools.h>

#include <Font_FontMgr.hxx>
#include <OpenGl_FrameStats.hxx>
#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>
#include <Standard_Version.hxx>
#include <Wasm_Window.hxx>

// #ifndef OCC_VERSION_GIT_HASH
//     #include <Standard_GitVersion.hxx>
// #else
//     #define OCC_GITVERSION_HASH   OCC_VERSION_GIT_HASH
//     #define OCC_GITVERSION_STRING OCC_VERSION_STRING_EXT
// #endif

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

// predefined canvas element id
#define THE_CANVAS_ID "#ViewerCanvas"

namespace Viewer {
#ifdef __EMSCRIPTEN__
//! Update canvas bounding rectangle.
EM_JS(void, occJSUpdateBoundingClientRect, (), { Module._myCanvasRect = Module.canvas.getBoundingClientRect(); });

//! Get canvas bounding top.
EM_JS(int, occJSGetBoundingClientTop, (), { return Math.round(Module._myCanvasRect.top); });

//! Get canvas bounding left.
EM_JS(int, occJSGetBoundingClientLeft, (), { return Math.round(Module._myCanvasRect.left); });

//! Callback for OnSelectionChanged().
EM_JS(bool, jsOnSelectedObjectsChanged, (), { return Module.eventManager.onSelectedObjectsChanged(); });

//! Callback for changing of detected object on mouse move.
EM_JS(bool, jsOnDetectedObjectChanged, (), { return Module.eventManager.onDetectedObjectChanged(); });

//! Callback for OnObjectDragged().
EM_JS(bool, jsOnObjectDragged, (int thePartId, int theSubPartId, int theAction), {
    return Module.eventManager.onObjectDragged(thePartId, theSubPartId, theAction);
});

//! Callback for a click on the view.
EM_JS(void, jsOnSubviewClicked, (int theViewId), { Module.eventManager.onSubviewClick(theViewId); });

// ================================================================
// Function : onMouseEvent
// ================================================================
EM_BOOL WasmViewer::onMouseEvent(int theEventType, const EmscriptenMouseEvent* theEvent)
{
    if (myMainView.IsNull()) {
        return EM_FALSE;
    }
    if (theEventType == EMSCRIPTEN_EVENT_MOUSEENTER) {
        if (myToAllowHighlightBack != -1) {
            SetAllowHighlight(myToAllowHighlightBack == 1);
            myToAllowHighlightBack = -1;
        }
        return EM_FALSE;
    } else if (theEventType == EMSCRIPTEN_EVENT_MOUSELEAVE) {
        myToAllowHighlightBack = ToAllowHighlight() ? 1 : 0;
        SetAllowHighlight(false);
        return EM_FALSE;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());
    if (theEventType == EMSCRIPTEN_EVENT_MOUSEMOVE || theEventType == EMSCRIPTEN_EVENT_MOUSEUP) {
        // these events are bound to EMSCRIPTEN_EVENT_TARGET_WINDOW, and coordinates should be converted
        occJSUpdateBoundingClientRect();
        EmscriptenMouseEvent anEvent = *theEvent;
        anEvent.targetX -= occJSGetBoundingClientLeft();
        anEvent.targetY -= occJSGetBoundingClientTop();
        aWindow->ProcessMouseEvent(*this, theEventType, &anEvent);
        return EM_FALSE;
    }

    if (theEventType == EMSCRIPTEN_EVENT_MOUSEDOWN && PressedMouseButtons() == Aspect_VKeyMouse_NONE
        && !myMainView->Subviews().IsEmpty()) {
        const Graphic3d_Vec2d aNewPos2d = aWindow->ConvertPointToBacking(
            Graphic3d_Vec2d(theEvent->targetX, theEvent->targetY));
        const Graphic3d_Vec2i aNewPos2i   = Graphic3d_Vec2i(aNewPos2d + Graphic3d_Vec2d(0.5));
        Handle(V3d_View)      aPickedView = myMainView->PickSubview(aNewPos2i);
        if (myViewManager->IsSubviewClickable(aPickedView->View()->Identification())) {
            jsOnSubviewClicked(aPickedView->View()->Identification());
            return EM_TRUE;
        }

        // switch input focus to another subview
        if (!aPickedView.IsNull() && aPickedView != myFocusView) {
            OnSubviewChanged(myContext, myFocusView, aPickedView);
        }
    }

    if (theEventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        // workaround incomplete dragging event
        auto toAbortDragging = [](const EmscriptenMouseEvent* theEv) -> bool {
            return theEv->ctrlKey == EM_TRUE || theEv->shiftKey == EM_TRUE || theEv->altKey == EM_TRUE
                || theEv->metaKey == EM_TRUE || (theEv->buttons & 0x2) != 0 || (theEv->buttons & 0x4) != 0;
        };
        if (toAbortDragging(theEvent)) {
            myUI.Dragging.ToAbort = true;
        }
    }

    aWindow->ProcessMouseEvent(*this, theEventType, theEvent);
    // Make preventDefault for mousedown to avoid dragging issue in chromium
    return theEventType == EMSCRIPTEN_EVENT_MOUSEDOWN ? EM_TRUE : EM_FALSE;
}

// ================================================================
// Function : onWheelEvent
// ================================================================
EM_BOOL WasmViewer::onWheelEvent(int theEventType, const EmscriptenWheelEvent* theEvent)
{
    if (myMainView.IsNull() || theEventType != EMSCRIPTEN_EVENT_WHEEL) {
        return EM_FALSE;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());

    if (!myMainView->Subviews().IsEmpty()) {
        // switch input focus to another subview
        const Graphic3d_Vec2d aNewPos2d = aWindow->ConvertPointToBacking(
            Graphic3d_Vec2d(theEvent->mouse.targetX, theEvent->mouse.targetY));
        const Graphic3d_Vec2i aNewPos2i   = Graphic3d_Vec2i(aNewPos2d + Graphic3d_Vec2d(0.5));
        Handle(V3d_View)      aPickedView = myMainView->PickSubview(aNewPos2i);
        if (myViewManager->IsSubviewClickable(aPickedView->View()->Identification())) {
            Graphic3d_Vec2i aWinSize;
            aWindow->Size(aWinSize.x(), aWinSize.y());
            if (aNewPos2i.x() < 0 || aNewPos2i.x() > aWinSize.x() || aNewPos2i.y() < 0
                || aNewPos2i.y() > aWinSize.y()) {
                return EM_FALSE;
            }
            return EM_TRUE;
        }

        if (!aPickedView.IsNull() && aPickedView != myFocusView) {
            OnSubviewChanged(myContext, myFocusView, aPickedView);
        }
    }

    return aWindow->ProcessWheelEvent(*this, theEventType, theEvent) ? EM_TRUE : EM_FALSE;
}

// ================================================================
// Function : onTouchEvent
// ================================================================
EM_BOOL WasmViewer::onTouchEvent(int theEventType, const EmscriptenTouchEvent* theEvent)
{
    if (myMainView.IsNull()) {
        return EM_FALSE;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());
    return aWindow->ProcessTouchEvent(*this, theEventType, theEvent) ? EM_TRUE : EM_FALSE;
}

// ================================================================
// Function : onKeyDownEvent
// ================================================================
EM_BOOL WasmViewer::onKeyDownEvent(int theEventType, const EmscriptenKeyboardEvent* theEvent)
{
    if (myMainView.IsNull() || theEventType != EMSCRIPTEN_EVENT_KEYDOWN) { // EMSCRIPTEN_EVENT_KEYPRESS
        return EM_FALSE;
    }

    const Aspect_VKey aVKey = Wasm_Window::VirtualKeyFromNative(theEvent->keyCode);
    if (aVKey == Aspect_VKey_Control || aVKey == Aspect_VKey_Shift || aVKey == Aspect_VKey_Alt
        || aVKey == Aspect_VKey_Meta) {
        // workaround incomplete dragging event
        myUI.Dragging.ToAbort = true;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());
    return aWindow->ProcessKeyEvent(*this, theEventType, theEvent) ? EM_TRUE : EM_FALSE;
}

// ================================================================
// Function : onFocusChange
// ================================================================
EM_BOOL WasmViewer::onFocusChange(int theEventType, const EmscriptenFocusEvent* theEvent)
{
    (void)theEventType;
    (void)theEvent;
    if (myMainView.IsNull()) {
        return EM_FALSE;
    }
    ResetViewInput();
    return EM_TRUE;
}

// ================================================================
// Function : onKeyUpEvent
// ================================================================
EM_BOOL WasmViewer::onKeyUpEvent(int theEventType, const EmscriptenKeyboardEvent* theEvent)
{
    if (myMainView.IsNull() || theEventType != EMSCRIPTEN_EVENT_KEYUP) {
        return EM_FALSE;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());
    return aWindow->ProcessKeyEvent(*this, theEventType, theEvent) ? EM_TRUE : EM_FALSE;
}

//! Emscripten wrapper for IPromise.
struct IPromiseWrapper : public emscripten::wrapper<RWBase::IPromise> {
    EMSCRIPTEN_WRAPPER(IPromiseWrapper);
    virtual void resolve() override { return call<void>("resolve"); }
    virtual void reject(const std::string& theErr) override { return call<void>("reject", theErr); }
    virtual ~IPromiseWrapper() override {}
};

//! Return global Viewer instance
static WasmViewer* wasmViewer_cppGetViewer()
{
    return WasmViewer::GetWasmViewerInstance().get();
}

// Module exports
EMSCRIPTEN_BINDINGS(ViewerModule)
{
    emscripten::class_<RWBase::IPromise>("IPromise")
        .function("resolve", &RWBase::IPromise::resolve, emscripten::pure_virtual())
        .function("reject", &RWBase::IPromise::reject, emscripten::pure_virtual())
        .allow_subclass<IPromiseWrapper>("IPromiseWrapper");

    emscripten::function("_cppSetTraceLevel", &WasmViewer::_cppSetTraceLevel);
    emscripten::function("_cppRegisterFont", &WasmViewer::_cppRegisterFont);
    emscripten::function("_cppDumpFontsInfo", &WasmViewer::_cppDumpFontsInfo);
    emscripten::function("_cppWasmCallBack", &WasmViewer::_cppWasmCallBack);
    emscripten::function("_cppGetViewer", &wasmViewer_cppGetViewer, emscripten::allow_raw_pointers());
}

EMSCRIPTEN_BINDINGS(WasmViewer)
{
    emscripten::class_<WasmViewer>("WasmViewer")
        .function("resizeViewport", &WasmViewer::ResizeViewport)
        .function("requestViewUpdate", &WasmViewer::RequestViewUpdate)
        .function("getManager", &WasmViewer::GetManager, emscripten::allow_raw_pointers())
        .function("dumpMemInfo", &WasmViewer::DumpMemInfo)
        .function("makeScreenshot", &WasmViewer::MakeScreenshot);
}

#endif

//! Fill in default mouse gestures.
static void addGestureMapDefault(AIS_MouseGestureMap& theMap, bool theIsMultiselection)
{
    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton, AIS_MouseGesture_RotateOrbit);
    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton | (Standard_UInteger)Aspect_VKeyFlags_CTRL,
                AIS_MouseGesture_Zoom);
    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton | (Standard_UInteger)Aspect_VKeyFlags_SHIFT,
                AIS_MouseGesture_Pan);
    if (theIsMultiselection) {
        theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton | (Standard_UInteger)Aspect_VKeyFlags_ALT,
                    AIS_MouseGesture_SelectRectangle);
        theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton | (Standard_UInteger)Aspect_VKeyFlags_ALT
                        | (Standard_UInteger)Aspect_VKeyFlags_SHIFT,
                    AIS_MouseGesture_SelectRectangle);
    }

    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_RightButton, AIS_MouseGesture_Zoom);
    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_RightButton | (Standard_UInteger)Aspect_VKeyFlags_CTRL,
                AIS_MouseGesture_RotateOrbit);

    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_MiddleButton, AIS_MouseGesture_Pan);
    theMap.Bind((Standard_UInteger)Aspect_VKeyMouse_MiddleButton | (Standard_UInteger)Aspect_VKeyFlags_CTRL,
                AIS_MouseGesture_Pan);
}

//! Fill in mouse button gestures.
static void changeGestureMapButton(AIS_MouseGestureMap&      theMap,
                                   Viewer::MouseGesture theGest,
                                   Aspect_VKeyMouse          theButton)
{
    switch (theGest) {
        case Viewer::MouseGesture::NONE: {
            theMap.UnBind((Standard_UInteger)theButton);
            break;
        }
        case Viewer::MouseGesture::ROTATE: {
            theMap.Bind((Standard_UInteger)theButton, AIS_MouseGesture_RotateOrbit);
            break;
        }
        case Viewer::MouseGesture::ROTATEVIEW: {
            theMap.Bind((Standard_UInteger)theButton, AIS_MouseGesture_RotateView);
            break;
        }
        case Viewer::MouseGesture::PAN: {
            theMap.Bind((Standard_UInteger)theButton, AIS_MouseGesture_Pan);
            break;
        }
        case Viewer::MouseGesture::ZOOM: {
            theMap.Bind((Standard_UInteger)theButton, AIS_MouseGesture_Zoom);
            break;
        }
    }
}

//! Return default mouse gesture map.
static AIS_MouseGestureMap getGestureMapDefault(Viewer::MouseGesture theLeftGest,
                                                Viewer::MouseGesture theMiddleGest,
                                                Viewer::MouseGesture theRightGest,
                                                bool                      theIsMultiselection)
{
    AIS_MouseGestureMap aMap;
    addGestureMapDefault(aMap, theIsMultiselection);

    // addendum
    aMap.Bind(Aspect_VKeyMouse_RightButton, AIS_MouseGesture_Pan);

    changeGestureMapButton(aMap, theLeftGest, Aspect_VKeyMouse_LeftButton);
    changeGestureMapButton(aMap, theRightGest, Aspect_VKeyMouse_RightButton);
    changeGestureMapButton(aMap, theMiddleGest, Aspect_VKeyMouse_MiddleButton);
    return aMap;
}

// ================================================================
// Function : GetWasmViewerInstance
// ================================================================
Handle(WasmViewer)& WasmViewer::GetWasmViewerInstance()
{
    static Handle(WasmViewer) aViewer;
    return aViewer;
}

// ================================================================
// Function : Init
// ================================================================
Handle(WasmViewer)& WasmViewer::Init(const Handle(V3d_View)& theView, const Handle(AIS_InteractiveContext)& theCtx)
{
    GetWasmViewerInstance() = new WasmViewer(theView, theCtx);
    GetWasmViewerInstance()->init();
    return GetWasmViewerInstance();
}

// ================================================================
// Function : createDefaultTextStyle
// ================================================================
Handle(Prs3d_TextAspect) WasmViewer::createDefaultTextStyle()
{
    static constexpr int defFontHeight = 12;

    Handle(Prs3d_TextAspect) aStyle = new Prs3d_TextAspect();
    aStyle->SetFont(Font_NOF_MONOSPACE);
    aStyle->SetHeight(defFontHeight);
    aStyle->Aspect()->SetColor(Quantity_NOC_GRAY95);
    aStyle->Aspect()->SetColorSubTitle(Quantity_NOC_BLACK);
    aStyle->Aspect()->SetDisplayType(Aspect_TODT_SHADOW);
    aStyle->Aspect()->SetTextFontAspect(Font_FA_Bold);
    aStyle->Aspect()->SetTextZoomable(false);
    aStyle->SetHorizontalJustification(Graphic3d_HTA_LEFT);
    aStyle->SetVerticalJustification(Graphic3d_VTA_BOTTOM);
    return aStyle;
}

// ================================================================
// Function : addDefaultActionHotKeys
// ================================================================
void WasmViewer::addDefaultActionHotKeys()
{
    addActionHotKeys2(Aspect_VKey_NavForward, Aspect_VKey_W, Aspect_VKey_W | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavBackward, Aspect_VKey_S, Aspect_VKey_S | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavSlideLeft, Aspect_VKey_A, Aspect_VKey_A | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavSlideRight, Aspect_VKey_D, Aspect_VKey_D | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavRollCCW, Aspect_VKey_Q, Aspect_VKey_Q | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavRollCW, Aspect_VKey_E, Aspect_VKey_E | Aspect_VKeyFlags_SHIFT);

    addActionHotKeys2(Aspect_VKey_NavSpeedIncrease,
                      Aspect_VKey_Plus,
                      Aspect_VKey_Plus | Aspect_VKeyFlags_SHIFT,
                      Aspect_VKey_Equal,
                      Aspect_VKey_NumpadAdd);
    myNavKeyMap2.Bind(Aspect_VKey_NumpadAdd | Aspect_VKeyFlags_SHIFT, Aspect_VKey_NavSpeedIncrease);
    addActionHotKeys2(Aspect_VKey_NavSpeedDecrease,
                      Aspect_VKey_Minus,
                      Aspect_VKey_Minus | Aspect_VKeyFlags_SHIFT,
                      Aspect_VKey_NumpadSubtract,
                      Aspect_VKey_NumpadSubtract | Aspect_VKeyFlags_SHIFT);

    addActionHotKeys2(Aspect_VKey_NavLookUp, Aspect_VKey_Up);
    addActionHotKeys2(Aspect_VKey_NavLookDown, Aspect_VKey_Down);
    addActionHotKeys2(Aspect_VKey_NavLookLeft, Aspect_VKey_Left);
    addActionHotKeys2(Aspect_VKey_NavLookRight, Aspect_VKey_Right);
    addActionHotKeys2(Aspect_VKey_NavSlideLeft, Aspect_VKey_Left | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavSlideRight, Aspect_VKey_Right | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavSlideUp, Aspect_VKey_Up | Aspect_VKeyFlags_SHIFT);
    addActionHotKeys2(Aspect_VKey_NavSlideDown, Aspect_VKey_Down | Aspect_VKeyFlags_SHIFT);
}

// ================================================================
// Function : WasmViewer
// ================================================================
WasmViewer::WasmViewer(const Handle(V3d_View)& theView, const Handle(AIS_InteractiveContext)& theCtx)
#if !defined(__EMSCRIPTEN__)
    : ViewerTest_EventManager(theView, theCtx),
#else
    : Standard_Transient(),
#endif
      myMainView(theView),
      myFocusView(theView),
      myContext(theCtx),
      myBackgroundColor(Quantity_NOC_BLACK),
      //
      myStatsActivator(new ViewBase::StatsActivator())
{
    myMinCamDistance = 0.001; // in meters
    myTextStyle      = createDefaultTextStyle();

    myMouseSelectionSchemes.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton
                                     | (Standard_UInteger)Aspect_VKeyFlags_CTRL,
                                 AIS_SelectionScheme_XOR);
    myMouseSelectionSchemes.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton
                                     | (Standard_UInteger)Aspect_VKeyFlags_ALT
                                     | (Standard_UInteger)Aspect_VKeyFlags_CTRL,
                                 AIS_SelectionScheme_XOR);
    myMouseDefaultSchemesMap = myMouseSelectionSchemes;

    SetRotationMode(AIS_RotationMode_PickLast);
    // @c SetLockOrbitZUp (true);

    addDefaultActionHotKeys();

    const ViewLockParams aDefViewParams;
    myMouseGestureMap = getGestureMapDefault(aDefViewParams.leftMouseGesture,
                                             aDefViewParams.middleMouseGesture,
                                             aDefViewParams.rightMouseGesture,
                                             true);

    myViewManager.reset(new Viewer::ViewManager(this));
    mySelManager.reset(new Viewer::SelectionManager(this));

    myManagers[(int)ManagerType::UNIT_MANAGER].reset(new Viewer::UnitManager(this));
    myManagers[(int)ManagerType::VIEW_MANAGER] = myViewManager;
    myManagers[(int)ManagerType::RENDERING_PARAMS].reset(new Viewer::RenderingParams(this));
    myManagers[(int)ManagerType::INTERACTIVE_CONTEXT].reset(new Viewer::InteractiveContext(this));
    myManagers[(int)ManagerType::SELECTION_MANAGER] = mySelManager;
    myManagers[(int)ManagerType::ASPECTS_MANAGER].reset(new Viewer::InteractiveAspectsManager(this));
    myManagers[(int)ManagerType::CAEMESH_MANAGER].reset(new Viewer::InteractiveCaeMeshManager(this));
    myManagers[(int)ManagerType::SHAPE_MANAGER].reset(new Viewer::InteractiveShapeManager(this));
    myManagers[(int)ManagerType::COLORSCALE_MANAGER].reset(new Viewer::InteractiveColorScaleManager(this));
    myManagers[(int)ManagerType::CLIPPING_MANAGER].reset(new Viewer::InteractiveClippingManager(this));
    myManagers[(int)ManagerType::SCALERULER_MANAGER].reset(new Viewer::InteractiveScaleRulerManager(this));
    myManagers[(int)ManagerType::TEXTLABEL_MANAGER].reset(new Viewer::InteractiveTextLabelManager(this));
    myManagers[(int)ManagerType::DIMENSION_MANAGER].reset(new Viewer::InteractiveDimensionManager(this));
    myManagers[(int)ManagerType::VIEWCUBE_MANAGER].reset(new Viewer::InteractiveViewCubeManager(this));
    myManagers[(int)ManagerType::AXES_MANAGER].reset(new Viewer::InteractiveAxesManager(this));
}

// ================================================================
// Function : ~WasmViewer
// ================================================================
WasmViewer::~WasmViewer()
{
    //
}

// ================================================================
// Function : init
// ================================================================
bool WasmViewer::init()
{
    initWindow();
    initViewer();
    if (!myContext.IsNull()) {
        initPixelScaleRatio();
    }
#ifdef __EMSCRIPTEN__

    if (myMainView.IsNull()) {
        return false;
    }

    myMainView->MustBeResized();
    myMainView->Redraw();
#endif
    return !myMainView.IsNull();
}

// ================================================================
// Function : initWindow
// ================================================================
void WasmViewer::initWindow()
{
#ifdef __EMSCRIPTEN__
    myDevicePixelRatio = emscripten_get_device_pixel_ratio();
    myCanvasId         = THE_CANVAS_ID;

    const char*   aTargetId    = myCanvasId.ToCString();
    const EM_BOOL toUseCapture = EM_TRUE;

    auto aMouseCallback = [](int theEventType, const EmscriptenMouseEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onMouseEvent(theEventType, theEvent);
    };
    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, aMouseCallback);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, aMouseCallback);
    emscripten_set_mousedown_callback(aTargetId, this, toUseCapture, aMouseCallback);
    emscripten_set_dblclick_callback(aTargetId, this, toUseCapture, aMouseCallback);
    emscripten_set_click_callback(aTargetId, this, toUseCapture, aMouseCallback);
    emscripten_set_mouseleave_callback(aTargetId, this, toUseCapture, aMouseCallback);
    emscripten_set_mouseenter_callback(aTargetId, this, toUseCapture, aMouseCallback);

    auto aWheelCallback = [](int theEventType, const EmscriptenWheelEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onWheelEvent(theEventType, theEvent);
    };
    emscripten_set_wheel_callback(aTargetId, this, toUseCapture, aWheelCallback);

    auto aTouchCallback = [](int theEventType, const EmscriptenTouchEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onTouchEvent(theEventType, theEvent);
    };
    emscripten_set_touchstart_callback(aTargetId, this, toUseCapture, aTouchCallback);
    emscripten_set_touchend_callback(aTargetId, this, toUseCapture, aTouchCallback);
    emscripten_set_touchmove_callback(aTargetId, this, toUseCapture, aTouchCallback);
    emscripten_set_touchcancel_callback(aTargetId, this, toUseCapture, aTouchCallback);

    auto aKeyDownCallback = [](int theEventType, const EmscriptenKeyboardEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onKeyDownEvent(theEventType, theEvent);
    };
    emscripten_set_keydown_callback(aTargetId, this, toUseCapture, aKeyDownCallback);

    auto aKeyUpCallback = [](int theEventType, const EmscriptenKeyboardEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onKeyUpEvent(theEventType, theEvent);
    };
    emscripten_set_keyup_callback(aTargetId, this, toUseCapture, aKeyUpCallback);

    auto aFocusChangeCallback = [](int theEventType, const EmscriptenFocusEvent* theEvent, void* theView) -> EM_BOOL {
        return ((WasmViewer*)theView)->onFocusChange(theEventType, theEvent);
    };
    emscripten_set_focusout_callback(aTargetId, this, toUseCapture, aFocusChangeCallback);
#endif

    my2dObjectsUpdated = false; // suppress dummy warning
}

// ================================================================
// Function : initPixelScaleRatio
// ================================================================
void WasmViewer::initPixelScaleRatio()
{
    SetTouchToleranceScale(myDevicePixelRatio);
    if (!myMainView.IsNull()) {
        Graphic3d_RenderingParams aParams = myMainView->Viewer()->DefaultRenderingParams();
        aParams.BaseResolution            = DEF_FONT_RESOLUTION;
        aParams.Resolution = static_cast<unsigned int>(Round(double(DEF_FONT_RESOLUTION) * myDevicePixelRatio));
        myContext->CurrentViewer()->SetDefaultRenderingParams(aParams);
        myMainView->Camera()->SetResolutionRatio(aParams.ResolutionRatio());
        for (const Handle(V3d_View)& aViewIter : myMainView->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().BaseResolution = aParams.BaseResolution;
            aViewIter->ChangeRenderingParams().Resolution     = aParams.Resolution;
            aViewIter->Camera()->SetResolutionRatio(aParams.ResolutionRatio());
        }
    }
    if (myContext.IsNull()) {
        return;
    }

    // extend tolerance to make touchscreen picking simpler to catch small items
    myContext->SetPixelTolerance(int(Round(myDevicePixelRatio * DEF_PIXEL_TOLERANCE)));
    // use smaller tolerance for considering objects/entities close by depth
    myContext->MainSelector()->SetDepthTolerance(SelectMgr_TypeOfDepthTolerance_UniformPixels, 1.0);

    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myContext->ObjectIterator(); anObjIter.More(); anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (ViewBase::IInteractiveObject* anIIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get())) {
            if (anIIObj->OnPixelScaleChange(anObj, myDevicePixelRatio) && myContext->IsDisplayed(anObj)) {
                myContext->Redisplay(anObj, false);
            }
        }
    }
}

// ================================================================
// Function : ProcessDpiChange
// ================================================================
void WasmViewer::ProcessDpiChange()
{
    if (!myMainView.IsNull()) {
        myDevicePixelRatio = (float)myMainView->Window()->DevicePixelRatio();
    }
    initPixelScaleRatio();
    baseViewController::ProcessDpiChange();
}

// ================================================================
// Function : UpdateMouseClick
// ================================================================
bool WasmViewer::UpdateMouseClick(const Graphic3d_Vec2i& thePoint,
                                  Aspect_VKeyMouse       theButton,
                                  Aspect_VKeyFlags       theModifiers,
                                  bool                   theIsDoubleClick)
{
    bool isProcessed = AIS_ViewController::UpdateMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick);
    if (isProcessed) {
        // workaround incomplete dragging event
        myUI.Dragging.ToAbort = true;
    }
    return isProcessed;
}

// ================================================================
// Function : UpdateMousePosition
// ================================================================
bool WasmViewer::UpdateMouseButtons(const Graphic3d_Vec2i& thePoint,
                                    Aspect_VKeyMouse       theButtons,
                                    Aspect_VKeyFlags       theModifiers,
                                    bool                   theIsEmulated)
{
    return AIS_ViewController::UpdateMouseButtons(thePoint, theButtons, theModifiers, theIsEmulated);
}

// ================================================================
// Function : UpdateMousePosition
// ================================================================
bool WasmViewer::UpdateMousePosition(const Graphic3d_Vec2i& thePoint,
                                     Aspect_VKeyMouse       theButtons,
                                     Aspect_VKeyFlags       theModifiers,
                                     bool                   theIsEmulated)
{
    bool toUpdateView = AIS_ViewController::UpdateMousePosition(thePoint, theButtons, theModifiers, theIsEmulated);
    if (myFocusView.IsNull() || myMouseModifiers != Aspect_VKeyFlags_NONE) {
        return toUpdateView;
    }

    return toUpdateView;
}

// ================================================================
// Function : ProcessInput
// ================================================================
void WasmViewer::ProcessInput()
{
    if (myMainView.IsNull()) {
        return;
    }

#ifdef __EMSCRIPTEN__
    // Queue redrawView() callback to redraw canvas after all user input is flushed by browser.
    // Redrawing viewer on every single message would be a pointless waste of resources,
    // as user will see only the last drawn frame due to WebGL implementation details.
    if (++myNbUpdateRequests2 == 1) {
        auto aRedrawView = [](void* theView) {
            return ((WasmViewer*)theView)->redrawView();
        };
        emscripten_async_call(aRedrawView, this, -1);
    }
#else
    // handle synchronously
    ProcessExpose();
#endif
}

// ================================================================
// Function : redrawView
// ================================================================
void WasmViewer::redrawView()
{
    myNbUpdateRequests2 = 0;
    if (!myFocusView.IsNull()) {
        FlushViewEvents(myContext, myFocusView, true);
    }
}

// ================================================================
// Function : HandleViewEvents
// ================================================================
void WasmViewer::HandleViewEvents(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView)
{
    if (Handle(ViewBase::BaseSelection) aBaseSel = Handle(ViewBase::BaseSelection)::DownCast(
            myContext->Selection())) {
        // temporary workaround
        aBaseSel->SetSelectionScheme(myGL.Selection.Scheme);
    }

    baseViewController::HandleViewEvents(theCtx, theView);
}

// ================================================================
// Function : updateObjectsBeforeRedraw
// ================================================================
void WasmViewer::updateObjectsBeforeRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                           const Handle(V3d_View)&               theView,
                                           AIS_RedrawProgress                    theStage)
{
    if (theStage == AIS_RedrawProgress_BeforeRedraw) {
        update2dObjects();
    }
    baseViewController::updateObjectsBeforeRedraw(theCtx, theView, theStage);
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void WasmViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView)
{
    baseViewController::handleViewRedraw(theCtx, theView);
#ifdef __EMSCRIPTEN__
    if (myToAskNextFrame) {
        // ask more frames
        if (++myNbUpdateRequests2 == 1) {
            auto aRedrawView = [](void* theView) {
                return ((WasmViewer*)theView)->redrawView();
            };
            emscripten_async_call(aRedrawView, this, -1);
        }
    }
#endif
}

// ================================================================
// Function : navigationKeyModifierSwitch2
// ================================================================
bool WasmViewer::navigationKeyModifierSwitch2(unsigned int theModifOld, unsigned int theModifNew, double theTimeStamp)
{
    bool hasActions = false;
    for (unsigned int aKeyIter = 0; aKeyIter < Aspect_VKey_ModifiersLower; ++aKeyIter) {
        if (!myKeys.IsKeyDown(aKeyIter)) {
            continue;
        }

        Aspect_VKey anActionOld = Aspect_VKey_UNKNOWN;
        Aspect_VKey anActionNew = Aspect_VKey_UNKNOWN;
        myNavKeyMap2.Find(aKeyIter | theModifOld, anActionOld);
        myNavKeyMap2.Find(aKeyIter | theModifNew, anActionNew);
        if (anActionOld == anActionNew) {
            continue;
        }

        if (anActionOld != Aspect_VKey_UNKNOWN) {
            myKeys.KeyUp(anActionOld, theTimeStamp);
        }
        if (anActionNew != Aspect_VKey_UNKNOWN) {
            hasActions = true;
            myKeys.KeyDown(anActionNew, theTimeStamp);
        }
    }
    return hasActions;
}

// =======================================================================
// function : KeyDown
// =======================================================================
void WasmViewer::KeyDown(Aspect_VKey theKey, double theTime, double thePressure)
{
    const unsigned int aModifOld = myKeys.Modifiers();
    AIS_ViewController::KeyDown(theKey, theTime, thePressure);

    const unsigned int aModifNew = myKeys.Modifiers();
    if (aModifNew != aModifOld && navigationKeyModifierSwitch2(aModifOld, aModifNew, theTime)) {
        // modifier key just pressed
    }

    Aspect_VKey anAction = Aspect_VKey_UNKNOWN;
    if (myNavKeyMap2.Find(theKey | myKeys.Modifiers(), anAction) && anAction != Aspect_VKey_UNKNOWN) {
        AIS_ViewController::KeyDown(anAction, theTime, thePressure);
    }
}

// =======================================================================
// function : KeyUp
// =======================================================================
void WasmViewer::KeyUp(Aspect_VKey theKey, double theTime)
{
    const unsigned int aModifOld = myKeys.Modifiers();
    AIS_ViewController::KeyUp(theKey, theTime);

    Aspect_VKey anAction = Aspect_VKey_UNKNOWN;
    if (myNavKeyMap2.Find(theKey | myKeys.Modifiers(), anAction) && anAction != Aspect_VKey_UNKNOWN) {
        AIS_ViewController::KeyUp(anAction, theTime);
        processKeyPress(anAction);
    }

    const unsigned int aModifNew = myKeys.Modifiers();
    if (aModifNew != aModifOld && navigationKeyModifierSwitch2(aModifOld, aModifNew, theTime)) {
        // modifier key released
    }

    processKeyPress(theKey | aModifNew);
}

// ==============================================================================
// function : processKeyPress
// ==============================================================================
bool WasmViewer::processKeyPress(Aspect_VKey theKey)
{
    switch (theKey) {
        case Aspect_VKey_F: {
            static constexpr double aMargin = 0.01;
            myFocusView->FitAll(aMargin, false);
            RequestViewUpdate();
            return true;
        }
#ifndef __EMSCRIPTEN__ // selection still picks first item in browser
        case Aspect_VKey_Comma: {
            myContext->HilightNextDetected(myFocusView);
            RequestViewUpdate();
            return true;
        }
        case Aspect_VKey_Period: {
            myContext->HilightPreviousDetected(myFocusView);
            RequestViewUpdate();
            return true;
        }
#endif
        default: break;
    }
    return false;
}

// ================================================================
// Function : OnSelectionChanged
// ================================================================
void WasmViewer::OnSelectionChanged(const Handle(AIS_InteractiveContext)&, const Handle(V3d_View)&)
{
#ifdef __EMSCRIPTEN__
    jsOnSelectedObjectsChanged();
#endif
}

// ================================================================
// Function : OnObjectDetected
// ================================================================
void WasmViewer::OnObjectDetected(const Handle(AIS_InteractiveContext)&, const Handle(V3d_View)&)
{
#ifdef __EMSCRIPTEN__
    jsOnDetectedObjectChanged();
#endif
}

// =======================================================================
// function : contextLazyMoveTo
// =======================================================================
void WasmViewer::contextLazyMoveTo(const Handle(AIS_InteractiveContext)& theCtx,
                                   const Handle(V3d_View)&               theView,
                                   const Graphic3d_Vec2i&                thePnt)
{
    if (myPrevMoveTo == thePnt || myHasHlrOnBeforeRotation) {
        return; // ignore highlighting in-between rotation of HLR view
    }

    try {
        OCC_CATCH_SIGNALS
        baseViewController::contextLazyMoveTo(theCtx, theView, thePnt);
    } catch (const Standard_Failure& theErr) {
        Message::SendFail() << "WasmViewer MoveTo FAILED:\n" << theErr;
    }
}

// ================================================================
// Function : PickPoint
// ================================================================
bool WasmViewer::PickPoint(gp_Pnt&                               thePnt,
                           const Handle(AIS_InteractiveContext)& theCtx,
                           const Handle(V3d_View)&               theView,
                           const Graphic3d_Vec2i&                theCursor,
                           bool                                  theToStickToPickRay)
{
    ResetPreviousMoveTo();

    const Handle(StdSelect_ViewerSelector3d)& aSelector = theCtx->MainSelector();
    aSelector->Pick(theCursor.x(), theCursor.y(), theView);
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        Handle(AIS_InteractiveObject) aDetObj = Handle(AIS_InteractiveObject)::DownCast(
            aSelector->Picked(aPickIter)->Selectable());
        if (!aDetObj->TransformPersistence().IsNull() && aDetObj->TransformPersistence()->IsTrihedronOr2d()) {
            continue;
        } else if (aDetObj->Transparency() >= 0.75f) {
            continue;
        }

        const SelectMgr_SortCriterion& aPicked = aSelector->PickedData(aPickIter);
        if (theToStickToPickRay && !Precision::IsInfinite(aPicked.Depth)) {
            thePnt = aSelector->GetManager().DetectedPoint(aPicked.Depth);
        } else {
            thePnt = aSelector->PickedPoint(aPickIter);
        }

        return !Precision::IsInfinite(thePnt.X()) && !Precision::IsInfinite(thePnt.Y())
            && !Precision::IsInfinite(thePnt.Z());
    }
    return false;
}

// ================================================================
// Function : GravityPoint
// ================================================================
gp_Pnt WasmViewer::GravityPoint(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView)
{
    if (myRotationMode == AIS_RotationMode_PickLast && !theCtx->DetectedOwner().IsNull()
        && theCtx->DetectedOwner()->HasSelectable()) {
        const SelectMgr_SelectableObject* aDetObj = theCtx->DetectedOwner()->Selectable().get();
        if (!aDetObj->TransformPersistence().IsNull() && aDetObj->TransformPersistence()->IsTrihedronOr2d()) {
            const Graphic3d_Vec2i aViewPort = theView->Window()->Dimensions();
            const Graphic3d_Vec2i aCursor   = aViewPort / 2;
            gp_Pnt                aPnt;
            if (PickPoint(aPnt, theCtx, theView, aCursor, myToStickToRayOnRotation)) {
                return aPnt;
            }
            return theCtx->GravityPoint(theView);
        }
    }

    return AIS_ViewController::GravityPoint(theCtx, theView);
}

// ================================================================
// Function : OnObjectDragged
// ================================================================
void WasmViewer::OnObjectDragged(const Handle(AIS_InteractiveContext)& theCtx,
                                 const Handle(V3d_View)&               theView,
                                 AIS_DragAction                        theAction)
{
    if (myIsLockedDrag && theAction != AIS_DragAction_Start) {
        if (theAction == AIS_DragAction_Stop || theAction == AIS_DragAction_Abort) {
            myIsLockedDrag = false;
            myDragObject.Nullify();
        }
        return;
    }

    AIS_ViewController::OnObjectDragged(theCtx, theView, theAction);
    if (!myDragObject.IsNull()) {
        myLastDragOwner  = myDragOwner;
        myLastDragObject = myDragObject;
    } else if (theAction == AIS_DragAction_Start && !theCtx->DetectedOwner().IsNull()) {
        const AIS_InteractiveObject* aDetObj = dynamic_cast<AIS_InteractiveObject*>(
            theCtx->DetectedOwner()->SelectablePointer());
        // if user pressed mouse over some detectable 2D object,
        // imitate fake dragging to suppress camera rotation
        if (aDetObj != nullptr && !aDetObj->TransformPersistence().IsNull()
            && (aDetObj->TransformPersistence()->Mode() & Graphic3d_TMF_2d) != 0) {
            myIsLockedDrag = true;
            myDragObject   = aDetObj;
        }
    }

    int anAction = theAction;
    if (theAction == AIS_DragAction_Start) {
        myIsDragStarted = false; // postpone Start event till the first Update
    } else if (!myIsDragStarted && theAction == AIS_DragAction_Update
               && !myGL.Dragging.PointTo.IsEqual(myGL.Dragging.PointStart)) {
        myIsDragStarted = true;
        anAction        = AIS_DragAction_Start;
    }

    if (theAction == AIS_DragAction_Update && !myDragObject.IsNull()) {
        Invalidate2dObjects(myDragObject);
    }

    ViewBase::InteractiveOwner::SelectionPart aPart;
    if (ViewBase::InteractiveOwner* anOwner = dynamic_cast<ViewBase::InteractiveOwner*>(myLastDragOwner.get())) {
        aPart = anOwner->GetSelectionPart();
    }
#ifdef __EMSCRIPTEN__
    if (!myIsLockedDrag && myIsDragStarted) {
        jsOnObjectDragged(aPart.Id, aPart.SubId, (int)anAction);
    }
#else
    (void)aPart;
    (void)anAction;
#endif
}

// ================================================================
// Function : ConfigureFocusView
// ================================================================
void WasmViewer::ConfigureFocusView()
{
    if (myFocusView.IsNull()) {
        return;
    }

    const int aViewId = myFocusView->View()->Identification();
    SetAllowZooming(myViewManager->ToAllowZooming(aViewId));
    SetAllowPanning(myViewManager->ToAllowPanning(aViewId));
    SetAllowRotation(myViewManager->ToAllowRotation(aViewId));
    SetLockOrbitZUp(myViewManager->ToLockOrbitZUp(aViewId));

    static constexpr int mBtnId0 = 0;
    static constexpr int mBtnId1 = 1;
    static constexpr int mBtnId2 = 2;

    myMouseGestureMap =
        getGestureMapDefault((Viewer::MouseGesture)myViewManager->getMouseGesture(aViewId, mBtnId0),
                             (Viewer::MouseGesture)myViewManager->getMouseGesture(aViewId, mBtnId1),
                             (Viewer::MouseGesture)myViewManager->getMouseGesture(aViewId, mBtnId2),
                             mySelManager->ToAllowMultiselection());

    const bool isMultiSelection = mySelManager->ToAllowMultiselection();
    const AIS_SelectionScheme aToggleScheme = isMultiSelection ? AIS_SelectionScheme_XOR
                                                               : AIS_SelectionScheme_Replace;
    const AIS_SelectionScheme aAddScheme = isMultiSelection ? AIS_SelectionScheme_Add
                                                            : AIS_SelectionScheme_Replace;

    myMouseSelectionSchemes = myMouseDefaultSchemesMap;
    for (Standard_UInteger aModif :
         {(Standard_UInteger)Aspect_VKeyFlags_CTRL,
          (Standard_UInteger)Aspect_VKeyFlags_CTRL | (Standard_UInteger)Aspect_VKeyFlags_ALT,
           (Standard_UInteger)Aspect_VKeyFlags_SHIFT}) {
        myMouseSelectionSchemes.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton | aModif, aToggleScheme);
    }
    myMouseSelectionSchemes.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton
                                     | (Standard_UInteger)Aspect_VKeyFlags_ALT,
                                 AIS_SelectionScheme_Replace);
    myMouseSelectionSchemes.Bind((Standard_UInteger)Aspect_VKeyMouse_LeftButton
                                     | (Standard_UInteger)Aspect_VKeyFlags_SHIFT
                                     | (Standard_UInteger)Aspect_VKeyFlags_ALT,
                                 aAddScheme);
}

// ================================================================
// Function : SetFocusView
// ================================================================
void WasmViewer::SetFocusView(const Handle(V3d_View)& theView)
{
    const int aViewId = theView->View()->Identification();
    (void)aViewId;
    myFocusView = theView;
    ConfigureFocusView();
}

// ================================================================
// Function : OnSubviewChanged
// ================================================================
void WasmViewer::OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
                                  const Handle(V3d_View)&,
                                  const Handle(V3d_View)& theNewView)
{
    if (myMainView == theNewView && !myMainView->Subviews().IsEmpty() && myMainView->View()->IsSubviewComposer()) {
        return; // disallow activating a composer
    }
    SetFocusView(theNewView);
}

// ================================================================
// Function : setupRenderingParams
// ================================================================
void WasmViewer::setupRenderingParams(const Handle(V3d_View)& theView)
{
    theView->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
    theView->Camera()->SetFOV2d(360.0f);
    theView->Camera()->SetFOVy(45.0f);

    Graphic3d_RenderingParams& aParams = theView->ChangeRenderingParams();
    aParams.StatsPosition->SetCorner2d(Aspect_TOTP_RIGHT_UPPER);

    const int aMaxMsaa =
#ifdef __EMSCRIPTEN__
        DEF_MSAA; // this method is called before V3d_View::SetWindow()
#else
        theView->Viewer()->Driver()->InquireLimit(Graphic3d_TypeOfLimit_MaxMsaa);
#endif

    const bool toAntialias = myDevicePixelRatio <= 1.25f && aMaxMsaa >= DEF_MSAA;
    aParams.NbMsaaSamples  = toAntialias ? DEF_MSAA : 0;
    // @c aParams.RenderResolutionScale = toAntialias ? 2.0f : 1.0f;
    aParams.Resolution = static_cast<unsigned int>(
        Round(static_cast<double>(DEF_FONT_RESOLUTION) * myDevicePixelRatio));
    aParams.ToShowStats    = false;
    aParams.CollectedStats = Graphic3d_RenderingParams::PerfCounters(
        Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Structures
        | Graphic3d_RenderingParams::PerfCounters_Groups | Graphic3d_RenderingParams::PerfCounters_Triangles
        | Graphic3d_RenderingParams::PerfCounters_EstimMem);
    aParams.StatsTextAspect = myTextStyle->Aspect();
    aParams.StatsTextHeight = (int)myTextStyle->Height();

    Graphic3d_RenderingParams aDefParams = myMainView->Viewer()->DefaultRenderingParams();
    aDefParams.RenderResolutionScale     = aParams.RenderResolutionScale;
    aDefParams.NbMsaaSamples             = aParams.NbMsaaSamples;
    aDefParams.ToShowStats               = aParams.ToShowStats;
    aDefParams.CollectedStats            = aParams.CollectedStats;
    aDefParams.StatsPosition             = aParams.StatsPosition;
    aDefParams.StatsTextAspect           = aParams.StatsTextAspect;
    aDefParams.StatsTextHeight           = aParams.StatsTextHeight;
    myMainView->Viewer()->SetDefaultRenderingParams(aDefParams);

    // disable immediate layers as they will not work with MSAA in WebGL
    const std::array<Graphic3d_ZLayerId, 2> aTopLayers = {Graphic3d_ZLayerId_Top, Graphic3d_ZLayerId_Topmost};
    for (Graphic3d_ZLayerId aLayerIter : aTopLayers) {
        Graphic3d_ZLayerSettings aLayer = theView->Viewer()->ZLayerSettings(aLayerIter);
        aLayer.SetImmediate(false);
        theView->Viewer()->SetZLayerSettings(aLayerIter, aLayer);
    }
}

//! Class extends performance counters overlay with extra info.
class WasmViewer::GlFrameStats : public OpenGl_FrameStats {
public:
    explicit GlFrameStats(const Handle(TCollection_HAsciiString)& theExtraInfo) : myExtraInfo(theExtraInfo)
    {
        // static constexpr int    hashCut7 = 7;
        // TCollection_AsciiString aHashViewer(Viewer_GIT_HASH);
        // TCollection_AsciiString aHashOcct(OCC_GITVERSION_HASH);
        // if (aHashViewer.Length() > hashCut7) {
        //     aHashViewer = aHashViewer.SubString(1, hashCut7);
        // }
        // if (aHashOcct.Length() > hashCut7) {
        //     aHashOcct = aHashOcct.SubString(1, hashCut7);
        // }
        myData += TCollection_AsciiString(OCCTViewer_PACKAGE " " OCCTViewer_VER_COMPLETE "-")
                + ", threads: "
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
                + RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
#else
                + "OFF";
#endif
//                 + "\nocct-" OCC_GITVERSION_STRING "-" + aHashOcct + "\n";
    }

public:
    virtual TCollection_AsciiString FormatStats(Graphic3d_RenderingParams::PerfCounters theFlags) const override
    {
        return myData + myExtraInfo->String() + OpenGl_FrameStats::FormatStats(theFlags);
    }

private:
    Handle(TCollection_HAsciiString) myExtraInfo;
    TCollection_AsciiString          myData;
};

//! Selection filter for enabled objects.
class WasmViewer::EnabledFilter : public SelectMgr_Filter {
public:
    //! Empty constructor.
    EnabledFilter() {}
    //! Filter only enabled objects.
    virtual Standard_Boolean IsOk(const Handle(SelectMgr_EntityOwner)& theObj) const
    {
        ViewBase::IInteractiveObject* aPrs = dynamic_cast<ViewBase::IInteractiveObject*>(theObj->Selectable().get());
        return aPrs == nullptr || aPrs->IsEnabled();
    }
};

// ================================================================
// Function : initViewerWasm
// ================================================================
bool WasmViewer::initViewerWasm()
{
#ifdef __EMSCRIPTEN__
    Handle(Aspect_DisplayConnection) aDisp;
    Handle(OpenGl_GraphicDriver)     aDriver = new OpenGl_GraphicDriver(aDisp, false);
    if (!aDriver->InitContext()) {
        Message::DefaultMessenger()->Send(TCollection_AsciiString("Error: EGL initialization failed"), Message_Fail);
        return false;
    }

    Handle(V3d_Viewer) aViewer = new V3d_Viewer(aDriver);
    aViewer->SetComputedMode(false);
    ViewBase::BaseViewer::SetupLightSources(aViewer);

    myMainView = new V3d_View(aViewer);
    myMainView->SetImmediateUpdate(false);
    myFocusView = myMainView;
    setupRenderingParams(myMainView);

    Handle(Wasm_Window) aWindow = new Wasm_Window(THE_CANVAS_ID);
    aWindow->Size(myWinSizeOld.x(), myWinSizeOld.y());
    myMainView->SetWindow(aWindow);
    myContext = new AIS_InteractiveContext(aViewer);
#else
    myDummyVar         = false; // suppress dummy warning
#endif
    return true;
}

// ================================================================
// Function : initViewer
// ================================================================
bool WasmViewer::initViewer()
{
#ifdef __EMSCRIPTEN__
    if (!initViewerWasm()) {
        return false;
    }
#else
    myDevicePixelRatio = (float)myMainView->Window()->DevicePixelRatio();
    // override current state / defaults
    ViewBase::BaseViewer::SetupLightSources(myMainView->Viewer());
    setupRenderingParams(myMainView);
#endif

    Handle(ViewBase::BaseDefaultDrawer) aDefDrawer = Handle(ViewBase::BaseDefaultDrawer)::DownCast(
        myContext->DefaultDrawer());
    if (aDefDrawer.IsNull()) {
        aDefDrawer = new ViewBase::BaseDefaultDrawer();
        myContext->SetDefaultDrawer(aDefDrawer);
        for (int aStyleIter = Prs3d_TypeOfHighlight_Selected; aStyleIter < Prs3d_TypeOfHighlight_NB; ++aStyleIter) {
            myContext->HighlightStyle((Prs3d_TypeOfHighlight)aStyleIter)->SetLink(aDefDrawer);
        }
    }

    myContext->RemoveFilters();
    myContext->AddFilter(new WasmViewer::EnabledFilter());

    myContext->SetSelection(new ViewBase::BaseSelection());

    SetOrbitAcceleration(3.0f);
#ifdef __EMSCRIPTEN__
    const bool toDumpBasicGl = false;
#else
    const bool toDumpBasicGl = true;
#endif
    dumpGlInfo(toDumpBasicGl, false);
    {
        Graphic3d_RenderingParams aDefParams = myMainView->Viewer()->DefaultRenderingParams();
        aDefParams.IsShadowEnabled           = false;
        aDefParams.ShadingModel = myMainView->Viewer()->Driver()->InquireLimit(Graphic3d_TypeOfLimit_HasPBR) == 1
                                    ? Graphic3d_TOSM_PBR
                                    : Graphic3d_TOSM_FRAGMENT;
        myMainView->Viewer()->SetDefaultRenderingParams(aDefParams);

        myMainView->SetBackgroundColor(myBackgroundColor);
        myMainView->ChangeRenderingParams().IsShadowEnabled = aDefParams.IsShadowEnabled;
        myMainView->SetShadingModel(aDefParams.ShadingModel);
    }

    // setup default IBL cubemap
    ViewBase::BaseViewer::SetupCubemapIBL(myMainView);
    initHighlightStyles();
    myContext->Display(myStatsActivator, false);

    // customize debug info
    SetGlViewFrameStats(myMainView);
    return true;
}

// ================================================================
// Function : initHighlightStyles
// ================================================================
void WasmViewer::initHighlightStyles()
{
    // highlight with the same display mode by default
    myContext->HighlightStyle(Prs3d_TypeOfHighlight_Selected)->SetDisplayMode(-1);
    myContext->HighlightStyle(Prs3d_TypeOfHighlight_Dynamic)->SetDisplayMode(-1);
    myContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalSelected)->SetDisplayMode(-1);
    myContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetDisplayMode(-1);

    for (const Prs3d_TypeOfHighlight aStyleIt : {Prs3d_TypeOfHighlight_Dynamic,
                                                 Prs3d_TypeOfHighlight_LocalDynamic,
                                                 Prs3d_TypeOfHighlight_Selected,
                                                 Prs3d_TypeOfHighlight_LocalSelected}) {
        const bool isDyn = aStyleIt == Prs3d_TypeOfHighlight_Dynamic || aStyleIt == Prs3d_TypeOfHighlight_LocalDynamic;

        const Handle(Prs3d_Drawer)& aHiStyle = myContext->HighlightStyle(aStyleIt);
        Quantity_ColorRGBA          aColor(isDyn ? Quantity_NOC_CYAN : Quantity_NOC_YELLOW);
        aColor.SetAlpha(isDyn ? 0.6f : 1.0f);

        Graphic3d_MaterialAspect aMat(Graphic3d_NOM_STONE);
        aMat.SetColor(aColor.GetRGB());
        aMat.SetAlpha(aColor.Alpha());

        Handle(Graphic3d_AspectFillArea3d) anAspect = new Graphic3d_AspectFillArea3d();
        *anAspect                                   = *myContext->DefaultDrawer()->ShadingAspect()->Aspect();
        anAspect->SetFrontMaterial(aMat);
        anAspect->SetInteriorColor(aColor.GetRGB());
        aHiStyle->SetColor(aColor.GetRGB());
        aHiStyle->SetBasicFillAreaAspect(anAspect);
        aHiStyle->SetShadingAspect(new Prs3d_ShadingAspect(anAspect));

        aHiStyle->SetOwnLineAspects();
        aHiStyle->LineAspect()->SetColor(aColor.GetRGB());
        aHiStyle->WireAspect()->SetColor(aColor.GetRGB());
        aHiStyle->FreeBoundaryAspect()->SetColor(aColor.GetRGB());
        aHiStyle->UnFreeBoundaryAspect()->SetColor(aColor.GetRGB());

        aHiStyle->SetupOwnPointAspect();
        aHiStyle->PointAspect()->SetColor(aColor.GetRGB());
    }
}

// ================================================================
// Function : SetGlViewFrameStats
// ================================================================
void WasmViewer::SetGlViewFrameStats(const Handle(V3d_View)& theView)
{
    if (Handle(OpenGl_View) aGlView = Handle(OpenGl_View)::DownCast(theView->View())) {
        if (!aGlView->GlWindow().IsNull() && !aGlView->GlWindow()->GetGlContext().IsNull()) {
            Handle(GlFrameStats) aFrameStats = new GlFrameStats(myStatsActivator->ExtraInfo());
            aGlView->GlWindow()->GetGlContext()->SetFrameStats(aFrameStats);
        }
    }
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void WasmViewer::dumpGlInfo(bool theIsBasic, bool theIsTrace)
{
    TColStd_IndexedDataMapOfStringString aGlCapsDict;
    myMainView->DiagnosticInformation(aGlCapsDict,
                                      theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
    if (theIsBasic) {
        TCollection_AsciiString aViewport;
        aGlCapsDict.FindFromKey("Viewport", aViewport);
        aGlCapsDict.Clear();
        aGlCapsDict.Add("Viewport", aViewport);
    }

    TCollection_AsciiString aSubviews;
    for (const Handle(V3d_View)& aChildIter : myMainView->Subviews()) {
        if (!aSubviews.IsEmpty()) {
            aSubviews += "; ";
        }
        aSubviews += TCollection_AsciiString("id: ") + aChildIter->View()->Identification()
                   + " size: " + aChildIter->Window()->Dimensions().x() + "x" + aChildIter->Window()->Dimensions().y()
                   + " topleft: " + aChildIter->View()->SubviewTopLeft().x() + "x"
                   + aChildIter->View()->SubviewTopLeft().y();
    }
    if (!aSubviews.IsEmpty()) {
        aGlCapsDict.Add("Subviews", aSubviews);
    }

    aGlCapsDict.Add("Display scale", TCollection_AsciiString(myDevicePixelRatio));

    // beautify output
    {
        TCollection_AsciiString* aGlVer   = aGlCapsDict.ChangeSeek("GLversion");
        TCollection_AsciiString* aGlslVer = aGlCapsDict.ChangeSeek("GLSLversion");
        if (aGlVer != NULL && aGlslVer != NULL) {
            *aGlVer = *aGlVer + " [GLSL: " + *aGlslVer + "]";
            aGlslVer->Clear();
        }
    }

    TCollection_AsciiString anInfo;
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next()) {
        if (!aValueIter.Value().IsEmpty()) {
            if (!anInfo.IsEmpty()) {
                anInfo += "\n";
            }
            anInfo += aValueIter.Key() + ": " + aValueIter.Value();
        }
    }

    ::Message::Send(anInfo, theIsTrace ? Message_Trace : Message_Info);
}

// ================================================================
// Function : handleZoom
// ================================================================
void WasmViewer::handleZoom(const Handle(V3d_View)& theView, const Aspect_ScrollDelta& theParams, const gp_Pnt* thePnt)
{
    if (theParams.Delta >= 0 || myMaxCamDistance <= 0) {
        AIS_ViewController::handleZoom(theView, theParams, thePnt);
        return;
    }

    const Bnd_Box aViewBox = theView->View()->MinMaxValues(false);
    if (aViewBox.IsVoid()) {
        AIS_ViewController::handleZoom(theView, theParams, thePnt);
        return;
    }

    const Handle(Graphic3d_Camera)& aCam    = theView->Camera();
    const gp_Pnt                    aMinPnt = aViewBox.CornerMin();
    const gp_Pnt                    aMaxPnt = aViewBox.CornerMax();
    const double                    aDiag   = aMinPnt.Distance(aMaxPnt);
    if (aCam->IsOrthographic()) {
        const gp_XYZ aViewDim = aCam->ViewDimensions();
        if (aViewDim.Y() == 0.0) {
            return;
        }

        const double aScale = double(aDiag) / Abs(aViewDim.Y());
        if (aScale <= (1.0 / myMaxCamDistance)) {
            return;
        }
    } else {
        const gp_Pnt aPnt            = (aMinPnt.XYZ() + aMaxPnt.XYZ()) / 2.0;
        const double aMaxCamDistance = aDiag * myMaxCamDistance;
        const double aViewDist       = (aPnt.XYZ() - aCam->Eye().XYZ()).Modulus();
        if (aViewDist > aMaxCamDistance && aViewDist > 0) {
            return;
        }
    }
    AIS_ViewController::handleZoom(theView, theParams, thePnt);
}

// ================================================================
// Function : RequestViewUpdate
// ================================================================
void WasmViewer::RequestViewUpdate()
{
    if (!myMainView.IsNull()) {
        myMainView->Invalidate();
        ProcessInput();
    }
}

// ================================================================
// Function : _cppWasmCallBack
// ================================================================
void WasmViewer::_cppWasmCallBack(uintptr_t theCallback)
{
    if (RWBase::IPromise* aCallBack = (RWBase::IPromise*)theCallback) {
        aCallBack->resolve();
    }
}

// ================================================================
// Function : _cppSetTraceLevel
// ================================================================
void WasmViewer::_cppSetTraceLevel(int theGravity)
{
    for (const Handle(Message_Printer)& aPrinterIter : Message::DefaultMessenger()->Printers()) {
        aPrinterIter->SetTraceLevel((Message_Gravity)theGravity);
    }
}

// ================================================================
// Function : _cppRegisterFont
// ================================================================
void WasmViewer::_cppRegisterFont(const std::string& theFontFile, const std::string& theFontName, bool theToOverride)
{
    Handle(Font_SystemFont) aFont = Font_FontMgr::GetInstance()->CheckFont(theFontFile.c_str());
    if (aFont.IsNull()) {
        Message::DefaultMessenger()->SendFail() << "Error: font '" << theFontFile << "' is not found";
        return;
    }

    if (!theFontName.empty()) {
        Handle(Font_SystemFont) aFont2 = aFont;

        aFont = new Font_SystemFont(theFontName.c_str());
        for (int anAspectIter = 0; anAspectIter < Font_FontAspect_NB; ++anAspectIter) {
            aFont->SetFontPath((Font_FontAspect)anAspectIter,
                               aFont2->FontPath((Font_FontAspect)anAspectIter),
                               aFont2->FontFaceId((Font_FontAspect)anAspectIter));
        }
    }
    Font_FontMgr::GetInstance()->RegisterFont(aFont, theToOverride);
}

// ================================================================
// Function : _cppDumpFontsInfo
// ================================================================
void WasmViewer::_cppDumpFontsInfo(const std::string& theFontName, const bool theIsStrict, const bool theToPrintDetails)
{
    Font_StrictLevel aStrictLevel = theIsStrict ? Font_StrictLevel_Strict : Font_StrictLevel_Aliases;
    Font_FontAspect  aFontAspect  = Font_FA_Undefined;

    Message_Messenger::StreamBuffer anInfo = Message::SendInfo();

    Handle(Font_FontMgr) aMgr = Font_FontMgr::GetInstance();
    if (!theFontName.empty() && theFontName != "*") {
        TCollection_AsciiString aResult;
        if (Handle(Font_SystemFont) aFont = aMgr->FindFont(theFontName.c_str(), aStrictLevel, aFontAspect)) {
            aResult = theToPrintDetails ? aFont->ToString() : aFont->FontName();
        }

        if (!aResult.IsEmpty()) {
            anInfo << aResult;
        } else {
            Message::SendFail() << "Error: font '" << theFontName << "' is not found";
        }
        return;
    }

    const Font_NListOfSystemFont aFonts = aMgr->GetAvailableFonts();

    std::vector<Handle(Font_SystemFont)> aFontsSorted;
    aFontsSorted.reserve(aFonts.Size());
    for (const Handle(Font_SystemFont)& aFontIter : aFonts) {
        aFontsSorted.push_back(aFontIter);
    }

    struct {
        bool operator()(const Handle(Font_SystemFont)& theFontA, const Handle(Font_SystemFont)& theFontB)
        {
            return theFontA->FontKey().IsLess(theFontB->FontKey());
        }
    } aFontComparator;
    std::stable_sort(aFontsSorted.begin(), aFontsSorted.end(), aFontComparator);

    bool isFirst = true;
    for (const Handle(Font_SystemFont)& aFontIter : aFontsSorted) {
        if (!isFirst) {
            anInfo << "\n";
        }

        if (theToPrintDetails) {
            anInfo << aFontIter->ToString();
        } else {
            anInfo << "\"" << aFontIter->FontName() << "\"";
        }
        isFirst = false;
    }
}

// ================================================================
// Function : ProcessConfigure
// ================================================================
void WasmViewer::ProcessConfigure(bool theIsResized)
{
    Invalidate2dObjects();
    baseViewController::ProcessConfigure(theIsResized);
}

// ================================================================
// Function : ResizeViewport
// ================================================================
void WasmViewer::ResizeViewport()
{
    if (myMainView.IsNull()) {
        return;
    }

    Handle(Wasm_Window) aWindow = Handle(Wasm_Window)::DownCast(myMainView->Window());
    if (aWindow.IsNull()) {
        return;
    }

    Graphic3d_Vec2i aWinSizeNew;
    aWindow->DoResize();
    aWindow->Size(aWinSizeNew.x(), aWinSizeNew.y());
#ifdef __EMSCRIPTEN__
    const float aPixelRatio = emscripten_get_device_pixel_ratio();
    if (aWinSizeNew == myWinSizeOld && aPixelRatio == myDevicePixelRatio) {
        // invalidate immediate view anyway as canvas could be cleared
        myMainView->InvalidateImmediate();
        redrawView();
        return;
    }

    myWinSizeOld = aWinSizeNew;
    if (myDevicePixelRatio != aPixelRatio) {
        myDevicePixelRatio = aPixelRatio;
        initPixelScaleRatio();
    }

    onViewUpdated();
    dumpGlInfo(true, true);
#endif
    return;
}

// ================================================================
// Function : SetDevicePixelRatio
// ================================================================
void WasmViewer::SetDevicePixelRatio(float theRatio)
{
    if (myMainView.IsNull()) {
        return;
    }
    if (myDevicePixelRatio == theRatio) {
        return;
    }

    myDevicePixelRatio = theRatio;
    initPixelScaleRatio();

    onViewUpdated();
}

// ================================================================
// Function : onViewUpdated
// ================================================================
void WasmViewer::onViewUpdated()
{
    myMainView->MustBeResized();
    myMainView->Invalidate();
    for (const Handle(V3d_View)& aChildIter : myMainView->Subviews()) {
        aChildIter->Window()->DoResize();
        aChildIter->MustBeResized();
        aChildIter->Invalidate();
    }

    Invalidate2dObjects();
    update2dObjects();
    updateObjectsBeforeRedraw(myContext, myMainView, AIS_RedrawProgress_BeforeUpdate);
    updateObjectsBeforeRedraw(myContext, myMainView, AIS_RedrawProgress_BeforeRedraw);
    for (const Handle(V3d_View)& aChildIter : myMainView->Subviews()) {
        if (aChildIter->View()->IsActive()) {
            aChildIter->Redraw();
        }
    }
    myMainView->Redraw();
}

// ================================================================
// Function : DumpMemInfo
// ================================================================
void WasmViewer::DumpMemInfo()
{
    Message::DefaultMessenger()->SendInfo(OSD_MemInfo::PrintInfo());
}

// ================================================================
// Function : MakeScreenshot
// ================================================================
bool WasmViewer::MakeScreenshot(int theWidth, int theHeight, uintptr_t theBuffer)
{
    Image_PixMap aPixMap;
    aPixMap.InitWrapper(Image_Format_RGBA, (Standard_Byte*)theBuffer, theWidth, theHeight);
    aPixMap.SetTopDown(true);

    V3d_ImageDumpOptions aParams;
    aParams.BufferType = Graphic3d_BT_RGBA;
    aParams.Height     = theHeight;
    aParams.Width      = theWidth;
    return myMainView->ToPixMap(aPixMap, aParams);
}

// ================================================================
// Function : update2dObjects
// ================================================================
void WasmViewer::update2dObjects()
{
    if (my2dObjectsUpdated) {
        return;
    }

    ViewCae::ColorScaleTools::UpdateColorScales(GetContext(),
                                                 myLastUpdated2dObject,
                                                 myViewManager->GetViewOffsets(),
                                                 myLastUpdated2dObject == myDragObject);
    my2dObjectsUpdated = true;
    myLastUpdated2dObject.Nullify();
}

} // namespace
