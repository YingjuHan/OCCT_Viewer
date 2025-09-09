#include "ViewManager.h"

#include "WasmViewer.h"
#include "Utils/ViewCorner.h"

#include <AIS_AnimationCamera.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
    #include <emscripten/html5.h>
#endif

namespace Viewer {
#ifdef __EMSCRIPTEN__
//! Callback fullscreen view mode changes.
EM_JS(bool, jsOnSubviewFullscreen, (int theViewId, bool isFullscreen), {
    return Module.eventManager.onSubviewFullscreen(theViewId, isFullscreen);
});
#endif

//! Sentry class to configure smooth camera transaction.
class ViewManager::AnimatedCamera {
public:
    //! Main constructor.
    AnimatedCamera(const opencascade::handle<AIS_AnimationCamera>& theAnim,
                   const opencascade::handle<V3d_View>&            theView,
                   const float                                     theAnimDuration)
        : myAnim(theAnim)
    {
        if (theAnimDuration <= 0.00001f) {
            return;
        }

        myAnim->SetView(theView);
        myAnim->SetStartPts(0.0);
        myAnim->SetOwnDuration(theAnimDuration);
        myAnim->SetCameraStart(new Graphic3d_Camera(theView->Camera()));

        myCamBack = theView->Camera();
        theView->SetCamera(new Graphic3d_Camera(myCamBack));
    }

    //! Destructor.
    ~AnimatedCamera() { Flush(); }

    //! Flush camera changes.
    void Flush() noexcept
    {
        if (myCamBack.IsNull()) {
            return;
        }

        const Handle(V3d_View)&  aView   = myAnim->View();
        Handle(Graphic3d_Camera) aCamNew = aView->Camera();
        aView->SetCamera(myCamBack);

        const Graphic3d_Mat4d anOrientMat1 = myCamBack->OrientationMatrix();
        const Graphic3d_Mat4d anOrientMat2 = aCamNew->OrientationMatrix();
        if (anOrientMat1 != anOrientMat2) {
            myAnim->SetCameraEnd(new Graphic3d_Camera(aCamNew));
            myAnim->StartTimer(0.0, 1.0, true, false);
            aView->Invalidate();
        }

        myCamBack.Nullify();
    }

    //! Copy constructor.
    AnimatedCamera(const AnimatedCamera&) = delete;
    //! Copy assignment.
    AnimatedCamera& operator=(const AnimatedCamera&) = delete;

private:
    Handle(AIS_AnimationCamera) myAnim;
    Handle(Graphic3d_Camera)    myCamBack;
};

// ================================================================
// Function : BackupSubviewParams
// ================================================================
void ViewLockParams::BackupSubviewParams(const Handle(Graphic3d_CView)& theView)
{
    isSubviewActive = theView->IsActive();
    subviewSize     = theView->SubviewSize();
    subviewOffset   = theView->SubviewOffset();
    subviewMargins  = theView->SubviewMargins();
}

// ================================================================
// Function : RestoreSubviewParams
// ================================================================
void ViewLockParams::RestoreSubviewParams(const Handle(Graphic3d_CView)& theView)
{
    theView->SetSubviewSize(subviewSize);
    theView->SetSubviewOffset(subviewOffset);
    theView->SetSubviewMargins(subviewMargins);
}

// ================================================================
// Function : ViewManager
// ================================================================
ViewManager::ViewManager(WasmViewer* theViewer) : IManager(theViewer), mySubviewBorderColor(Quantity_NOC_BLACK)
{
    static constexpr int nbMaxViews = 32; // limit comes from V3d_Viewer limitation
    myViewParams.resize(nbMaxViews);
}

// ================================================================
// Function : GetSceneMinMax
// ================================================================
BndBox3 ViewManager::GetSceneMinMax(int theViewId, bool theSelection) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetSceneMinMax (" << theViewId << ") called for undefined View";
        return BndBox3();
    }

    const Bnd_Box aBndBox = theSelection ? myWebViewer->GetContext()->BoundingBoxOfSelection(aView)
                                         : aView->View()->MinMaxValues();
    if (aBndBox.IsVoid()) {
        return BndBox3();
    }

    return BndBox3(aBndBox);
}

// ================================================================
// Function : IsFitMinMax
// ================================================================
bool ViewManager::IsFitMinMax(int theViewId, const BndBox3& theMinMax) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::IsFittedView (" << theViewId << ") called for undefined View";
        return false;
    }

    if (!theMinMax.IsValid()) {
        return false;
    }

    const Bnd_Box aBndBox = theMinMax;

    const Handle(Graphic3d_Camera)& aCam    = aView->Camera();
    Handle(Graphic3d_Camera)        aCamFit = new Graphic3d_Camera(aCam);
    aView->FitMinMax(aCamFit, aBndBox, FIT_ALL_MARGIN);

    // fit all algorithm is not 100% stable - so compute some precision to compare equal camera values
    const double aFitTol = (aBndBox.CornerMax().XYZ() - aBndBox.CornerMin().XYZ()).Modulus() * 0.000001;
    return aCamFit->Center().IsEqual(aCam->Center(), aFitTol) && Abs(aCamFit->Scale() - aCam->Scale()) < aFitTol
        && Abs(aCamFit->Distance() - aCam->Distance()) < aFitTol;
}

// ================================================================
// Function : FitMinMax
// ================================================================
void ViewManager::FitMinMax(int theViewId, const BndBox3& theMinMax, bool theToAnimate)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::FitMinMax (" << theViewId << ") called for undefined View";
        return;
    }

    if (!theMinMax.IsValid()) {
        return;
    }

    AnimatedCamera anAnimSentry(myWebViewer->ViewAnimation(),
                                aView,
                                theToAnimate ? myWebViewer->GetAnimationDuration() : 0.0f);

    const Bnd_Box aBndBox = theMinMax;
    aView->FitMinMax(aView->Camera(), aBndBox, FIT_ALL_MARGIN);
    aView->Invalidate();
}

// ================================================================
// Function : FitAllAuto
// ================================================================
void ViewManager::FitAllAuto(int theViewId, bool theIsAuto)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::FitAllAuto (" << theViewId << ") called for undefined View";
        return;
    }

    if (theIsAuto) {
        myWebViewer->FitAllAuto(myWebViewer->GetContext(), aView);
    } else {
        aView->FitAll(FIT_ALL_MARGIN, false);
    }
    aView->Invalidate();
}

// ================================================================
// Function : SetOrientation
// ================================================================
void ViewManager::SetOrientation(int theViewId, int theOrientation, bool theToAnimate)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetOrientation (" << theViewId << ") called for undefined View";
        return;
    }

    AnimatedCamera anAnimSentry(myWebViewer->ViewAnimation(),
                                aView,
                                theToAnimate ? myWebViewer->GetAnimationDuration() : 0.0f);
    aView->SetProj((V3d_TypeOfOrientation)theOrientation);
    myWebViewer->FitAllAuto(myWebViewer->GetContext(), aView);
    aView->Invalidate();
}

// ================================================================
// Function : GetViewPoint
// ================================================================
CameraViewPoint ViewManager::GetViewPoint(int theViewId)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetViewPoint (" << theViewId << ") called for undefined View";
        return CameraViewPoint();
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    CameraViewPoint                 aRet;
    aRet.eye       = Vec3(aCamera->Eye().XYZ());
    aRet.up        = Vec3(aCamera->Up().XYZ());
    aRet.direction = Vec3(aCamera->Direction().XYZ());
    aRet.distance  = aCamera->Distance();
    aRet.scale     = aCamera->Scale();
    return aRet;
}

// ================================================================
// Function : SetViewPoint
// ================================================================
void ViewManager::SetViewPoint(int theViewId, const CameraViewPoint& theParams)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetViewPoint (" << theViewId << ") called for undefined View";
        return;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    aCamera->SetScale(theParams.scale);
    aCamera->SetDirectionFromEye(theParams.direction.XYZ());
    aCamera->SetDistance(theParams.distance);
    aCamera->MoveEyeTo(theParams.eye.XYZ());
    aCamera->SetUp(theParams.up.XYZ());
    aCamera->OrthogonalizeUp();
    aView->Invalidate();
}

// ================================================================
// Function : GetSubviewBorderColor
// ================================================================
ColorTools::ColorString ViewManager::GetSubviewBorderColor() const
{
    Quantity_ColorRGBA      aColor(mySubviewBorderColor);
    TCollection_AsciiString aHex = Quantity_ColorRGBA::ColorToHex(aColor);
    return aHex.ToCString();
}

// ================================================================
// Function : SetSubviewBorderColor
// ================================================================
void ViewManager::SetSubviewBorderColor(const ColorTools::ColorString& theHexColor)
{
    Quantity_ColorRGBA aColor;
    if (!ColorTools::ParseHexColor(theHexColor, aColor)) {
        return;
    }

    mySubviewBorderColor = aColor.GetRGB();
    if (myWebViewer->MainView()->View()->IsSubviewComposer()) {
        myWebViewer->MainView()->SetBackgroundColor(aColor.GetRGB());
        myWebViewer->MainView()->Invalidate();
    }
}

// ================================================================
// Function : ToAllowRotation
// ================================================================
bool ViewManager::ToAllowRotation(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId >= (int)myViewParams.size() || myViewParams[aViewId].toAllowRotation;
}

// ================================================================
// Function : SetAllowRotation
// ================================================================
void ViewManager::SetAllowRotation(int theViewId, bool theToEnable)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetAllowRotation (" << theViewId << ") called for undefined View";
        return;
    }

    const int aViewId                     = aView->View()->Identification();
    myViewParams[aViewId].toAllowRotation = theToEnable;
    if (aView == myWebViewer->FocusView()) {
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : GetOrbitAcceleration
// ================================================================
float ViewManager::GetOrbitAcceleration() const
{
    return myWebViewer->OrbitAcceleration();
}

// ================================================================
// Function : SetOrbitAcceleration
// ================================================================
void ViewManager::SetOrbitAcceleration(float theOrbitAcceleration)
{
    myWebViewer->SetOrbitAcceleration(theOrbitAcceleration);
}

// ================================================================
// Function : ToLockOrbitZUp
// ================================================================
bool ViewManager::ToLockOrbitZUp(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId < (int)myViewParams.size() && myViewParams[aViewId].toLockOrbitZUp;
}

// ================================================================
// Function : SetLockOrbitZUp
// ================================================================
void ViewManager::SetLockOrbitZUp(int theViewId, bool theToForceUp)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetLockOrbitZUp (" << theViewId << ") called for undefined View";
        return;
    }

    const int aViewId                    = aView->View()->Identification();
    myViewParams[aViewId].toLockOrbitZUp = theToForceUp;
    if (aView == myWebViewer->FocusView()) {
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : ToAllowPanning
// ================================================================
bool ViewManager::ToAllowPanning(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId >= (int)myViewParams.size() || myViewParams[aViewId].toAllowPanning;
}

// ================================================================
// Function : SetAllowPanning
// ================================================================
void ViewManager::SetAllowPanning(int theViewId, bool theToEnable)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetAllowPanning (" << theViewId << ") called for undefined View";
        return;
    }

    const int aViewId                    = aView->View()->Identification();
    myViewParams[aViewId].toAllowPanning = theToEnable;
    if (aView == myWebViewer->FocusView()) {
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : ToAllowZooming
// ================================================================
bool ViewManager::ToAllowZooming(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId >= (int)myViewParams.size() || myViewParams[aViewId].toAllowZooming;
}

// ================================================================
// Function : SetAllowZooming
// ================================================================
void ViewManager::SetAllowZooming(int theViewId, bool theToEnable)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetAllowZooming (" << theViewId << ") called for undefined View";
        return;
    }

    const int aViewId                    = aView->View()->Identification();
    myViewParams[aViewId].toAllowZooming = theToEnable;
    if (aView == myWebViewer->FocusView()) {
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : getMouseGesture
// ================================================================
int ViewManager::getMouseGesture(int theViewId, int theButton) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return static_cast<int>(Viewer::MouseGesture::NONE);
    }

    const int aViewId = aView->View()->Identification();
    if (aViewId >= (int)myViewParams.size()) {
        return static_cast<int>(Viewer::MouseGesture::NONE);
    }

    switch (theButton) {
        case MOUSE_BTN0: return (int)myViewParams[aViewId].leftMouseGesture;
        case MOUSE_BTN1: return (int)myViewParams[aViewId].middleMouseGesture;
        case MOUSE_BTN2: return (int)myViewParams[aViewId].rightMouseGesture;
        default: break;
    }

    return static_cast<int>(Viewer::MouseGesture::NONE);
}

// ================================================================
// Function : setMouseGesture
// ================================================================
void ViewManager::setMouseGesture(int theViewId, int theButton, int theGest)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::setMouseGesture (" << theViewId << ", " << theButton << ", "
                            << theGest << ") called for undefined View";
        return;
    } else if (theButton < MOUSE_BTN0 || theButton > MOUSE_BTN2) {
        Message::SendFail() << "Error: ViewManager::setMouseGesture (" << theViewId << ", " << theButton << ", "
                            << theGest << ") called for invalid mouse button";
        return;
    } else if (theGest != static_cast<int>(Viewer::MouseGesture::NONE) &&       //
               theGest != static_cast<int>(Viewer::MouseGesture::ROTATE) &&     //
               theGest != static_cast<int>(Viewer::MouseGesture::ROTATEVIEW) && //
               theGest != static_cast<int>(Viewer::MouseGesture::PAN) &&        //
               theGest != static_cast<int>(Viewer::MouseGesture::ZOOM)) {
        Message::SendFail() << "Error: ViewManager::setMouseGesture (" << theViewId << ", " << theButton << ", "
                            << theGest << ") called for invalid gesture";
        return;
    }

    const int aViewId = aView->View()->Identification();
    switch (theButton) {
        case MOUSE_BTN0: myViewParams[aViewId].leftMouseGesture = (Viewer::MouseGesture)theGest; break;
        case MOUSE_BTN1: myViewParams[aViewId].middleMouseGesture = (Viewer::MouseGesture)theGest; break;
        case MOUSE_BTN2: myViewParams[aViewId].rightMouseGesture = (Viewer::MouseGesture)theGest; break;
        default: break;
    }
    if (aView == myWebViewer->FocusView()) {
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : GetMaxCamDistanceMultiplier
// ================================================================
float ViewManager::GetMaxCamDistanceMultiplier() const
{
    return myWebViewer->myMaxCamDistance;
}

// ================================================================
// Function : SetMaxCamDistanceMultiplier
// ================================================================
void ViewManager::SetMaxCamDistanceMultiplier(float theMaxCamDistanceMultiplier)
{
    myWebViewer->myMaxCamDistance = Max(0.0f, theMaxCamDistanceMultiplier);
}

// ================================================================
// Function : IsPerspectiveProjection
// ================================================================
bool ViewManager::IsPerspectiveProjection(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    return aCamera->ProjectionType() != Graphic3d_Camera::Projection_Orthographic;
}

// ================================================================
// Function : SetPerspectiveProjection
// ================================================================
void ViewManager::SetPerspectiveProjection(int theViewId, bool theIsPerspective)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetPerspectiveProjection (" << theViewId
                            << ") called for undefined View";
        return;
    }

    if (IsPerspectiveProjection(theViewId) == theIsPerspective) {
        return;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    aCamera->SetProjectionType(theIsPerspective ? Graphic3d_Camera::Projection_Perspective
                                                : Graphic3d_Camera::Projection_Orthographic);
    aView->Invalidate();
}

// ================================================================
// Function : GetPerspectiveProjectionFOVy
// ================================================================
double ViewManager::GetPerspectiveProjectionFOVy(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::PerspectiveProjectionFOVy (" << theViewId
                            << ") called for undefined View";
        return 0.0;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    return aCamera->FOVy();
}

// ================================================================
// Function : SetPerspectiveProjectionFOVy
// ================================================================
void ViewManager::SetPerspectiveProjectionFOVy(int theViewId, double theFOVy)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetPerspectiveProjectionFOVy (" << theViewId
                            << ") called for undefined View";
        return;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    if (aCamera->FOVy() == theFOVy) {
        return;
    }

    aCamera->SetFOVy(theFOVy);
    if (aCamera->ProjectionType() != Graphic3d_Camera::Projection_Orthographic) {
        aView->Invalidate();
    }
}

// ================================================================
// Function : GetStereoMode
// ================================================================
int ViewManager::GetStereoMode(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();
    if (aCamera->ProjectionType() != Graphic3d_Camera::Projection_Stereo) {
        return static_cast<int>(StereoMode::MONO);
    }

    StereoMode aMode = StereoMode::MONO;
    switch (aView->RenderingParams().StereoMode) {
        case Graphic3d_StereoMode_Anaglyph: {
            aMode = StereoMode::REDCYAN;
            break;
        }
        case Graphic3d_StereoMode_RowInterlaced: {
            aMode = aView->RenderingParams().ToReverseStereo ? StereoMode::ROWINTERLACEDINV : StereoMode::ROWINTERLACED;
            break;
        }
        default: {
            aMode = StereoMode::MONO;
            break;
        }
    }
    return static_cast<int>(aMode);
}

// ================================================================
// Function : SetStereoMode
// ================================================================
void ViewManager::SetStereoMode(int theViewId, int theMode)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetStereoMode (" << theViewId << ", " << theMode
                            << ") called for undefined View";
        return;
    }

    const Handle(Graphic3d_Camera)& aCamera = aView->Camera();

    const bool toReverse = (theMode == static_cast<int>(StereoMode::ROWINTERLACEDINV));
    switch (theMode) {
        case static_cast<int>(StereoMode::MONO): {
            if (aCamera->ProjectionType() != Graphic3d_Camera::Projection_Stereo) {
                return;
            }

            aCamera->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
            aView->ChangeRenderingParams().ToReverseStereo = toReverse;
            break;
        }
        case static_cast<int>(StereoMode::REDCYAN): {
            if (aCamera->ProjectionType() == Graphic3d_Camera::Projection_Stereo
                && aView->RenderingParams().StereoMode == Graphic3d_StereoMode_Anaglyph) {
                return;
            }

            aView->ChangeRenderingParams().StereoMode      = Graphic3d_StereoMode_Anaglyph;
            aView->ChangeRenderingParams().ToReverseStereo = toReverse;
            aCamera->SetProjectionType(Graphic3d_Camera::Projection_Stereo);
            break;
        }
        case static_cast<int>(StereoMode::ROWINTERLACED):
        case static_cast<int>(StereoMode::ROWINTERLACEDINV): {
            if (aCamera->ProjectionType() == Graphic3d_Camera::Projection_Stereo
                && aView->RenderingParams().StereoMode == Graphic3d_StereoMode_RowInterlaced
                && aView->RenderingParams().ToReverseStereo == toReverse) {
                return;
            }

            aView->ChangeRenderingParams().StereoMode      = Graphic3d_StereoMode_RowInterlaced;
            aView->ChangeRenderingParams().ToReverseStereo = toReverse;
            aCamera->SetProjectionType(Graphic3d_Camera::Projection_Stereo);
            break;
        }
        default: {
            Message::SendFail() << "Error: ViewManager::SetStereoMode (" << theViewId << ", " << theMode
                                << ") called with invalid stere mode";
            return;
        }
    }

    aView->Invalidate();
}

// ================================================================
// Function : CreateSubview
// ================================================================
int ViewManager::CreateSubview(const Viewer::Vec2& theSize)
{
    const Handle(V3d_Viewer)& aViewer = myWebViewer->GetContext()->CurrentViewer();
    Handle(V3d_View)          aView   = new V3d_View(aViewer);
    aView->SetImmediateUpdate(false);
    aView->SetBackgroundColor(myWebViewer->BackgroundColor());

    aView->Camera()->SetProjectionType(myWebViewer->MainView()->Camera()->ProjectionType());
    aView->Camera()->SetFOV2d(360.0f);
    aView->Camera()->SetFOVy(45.0f);
    aView->SetWindow(myWebViewer->MainView(), theSize);
    aView->Invalidate();
    myWebViewer->MainView()->Invalidate();

    const int aNewViewId = aView->View()->Identification();
    if (aNewViewId >= (int)myViewParams.size()) {
        myViewParams.resize(aNewViewId + 1);
    }
    myViewParams[aNewViewId] = ViewLockParams();
    myWebViewer->SetFocusView(aView);

    myWebViewer->MainView()->View()->SetSubviewComposer(true);
    myWebViewer->MainView()->SetBackgroundColor(mySubviewBorderColor);
    myWebViewer->MainView()->ChangeRenderingParams().NbMsaaSamples         = 0;
    myWebViewer->MainView()->ChangeRenderingParams().RenderResolutionScale = 1.0f;
    myWebViewer->SetGlViewFrameStats(aView);

    aView->SetBackgroundCubeMap(myWebViewer->MainView()->View()->IBLCubeMap(), true, false);
    aView->SetBgGradientStyle(Aspect_GFM_NONE, false);

    Message::SendInfo() << "Created subview #" << aNewViewId << " of size " << theSize.x() << "x" << theSize.y();

    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        Handle(Graphic3d_ViewAffinity)       anAffinity =
            myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(anObj);
        if (!anAffinity.IsNull() && !anAffinity->IsVisible(aNewViewId)) {
            // synchronize View affinity with Graphic3d_CView::HiddenObjects() to suppress picking
            myWebViewer->GetContext()->SetViewAffinity(anObj, aView, false);
        }
    }

    return aNewViewId;
}

// ================================================================
// Function : RemoveSubview
// ================================================================
void ViewManager::RemoveSubview(int theViewId)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::RemoveView (" << theViewId << ") called for undefined View";
        return;
    }
    if (aView == myWebViewer->MainView()) {
        Message::SendFail() << "Error: ViewManager::RemoveView (" << theViewId << ") called for main view";
        return;
    }

    const int aViewId = aView->View()->Identification();
    if (aViewId < (int)myViewParams.size()) {
        myViewParams[aViewId] = ViewLockParams();
    }
    aView->Remove();
    if (myWebViewer->MainView()->Subviews().IsEmpty()) {
        myWebViewer->MainView()->View()->SetSubviewComposer(false);
        myWebViewer->MainView()->SetBackgroundColor(myWebViewer->BackgroundColor());
        myWebViewer->MainView()->ChangeRenderingParams().NbMsaaSamples =
            myWebViewer->Viewer()->DefaultRenderingParams().NbMsaaSamples;
        myWebViewer->MainView()->ChangeRenderingParams().RenderResolutionScale =
            myWebViewer->Viewer()->DefaultRenderingParams().RenderResolutionScale;
    }
    if (myWebViewer->FocusView() == aView) {
        myWebViewer->SetFocusView(myWebViewer->MainView());
    }

    // move objects visible only within removed View into Erased state
    AIS_ListOfInteractive anObjListToHide;
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        Handle(Graphic3d_ViewAffinity)       anAffinity =
            myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(anObj);
        if (anAffinity.IsNull() || !anAffinity->IsVisible(aViewId)) {
            continue;
        }

        int hasVisibleViews = 0;
        for (const Handle(V3d_View)& aViewIter : myWebViewer->MainView()->Viewer()->DefinedViews()) {
            if (anAffinity->IsVisible(aViewIter->View()->Identification())) {
                ++hasVisibleViews;
                break;
            }
        }
        if (hasVisibleViews == 0) {
            anObjListToHide.Append(anObj);
        }
    }
    hideObjects(anObjListToHide);
}

// ================================================================
// Function : RemoveAllSubviews
// ================================================================
void ViewManager::RemoveAllSubviews()
{
    NCollection_Sequence<Handle(V3d_View)> aSubviews = myWebViewer->MainView()->Subviews();
    if (aSubviews.IsEmpty()) {
        return;
    }

    for (const Handle(V3d_View)& aSubviewIter : aSubviews) {
        const int aSubviewId = aSubviewIter->View()->Identification();
        if (aSubviewId < (int)myViewParams.size()) {
            myViewParams[aSubviewId] = ViewLockParams();
        }
        aSubviewIter->Remove();
    }
    myWebViewer->MainView()->View()->SetSubviewComposer(false);
    myWebViewer->MainView()->SetBackgroundColor(myWebViewer->BackgroundColor());
    myWebViewer->MainView()->ChangeRenderingParams().NbMsaaSamples =
        myWebViewer->Viewer()->DefaultRenderingParams().NbMsaaSamples;
    myWebViewer->MainView()->ChangeRenderingParams().RenderResolutionScale =
        myWebViewer->Viewer()->DefaultRenderingParams().RenderResolutionScale;
    myWebViewer->SetFocusView(myWebViewer->MainView());

    // move objects visible only within removed Views into Erased state
    AIS_ListOfInteractive anObjListToHide;
    const int             aMainViewId = myWebViewer->MainView()->View()->Identification();
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        Handle(Graphic3d_ViewAffinity)       anAffinity =
            myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(anObj);
        if (!anAffinity.IsNull() && !anAffinity->IsVisible(aMainViewId)) {
            anObjListToHide.Append(anObj);
        }
    }
    hideObjects(anObjListToHide);
}

// ================================================================
// Function : hideObjects
// ================================================================
void ViewManager::hideObjects(const AIS_ListOfInteractive& theObjects)
{
    for (const Handle(AIS_InteractiveObject)& anObjIter : theObjects) {
        Handle(Graphic3d_ViewAffinity) anAffinity =
            myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(anObjIter);
        myWebViewer->GetContext()->Erase(anObjIter, false);
        anAffinity->SetVisible(true); // remove View affinity
    }
}

// ================================================================
// Function : IsDefined
// ================================================================
bool ViewManager::IsDefined(int theViewId) const
{
    return !findView(theViewId).IsNull();
}

// ================================================================
// Function : IsActive
// ================================================================
bool ViewManager::IsActive(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    return !aView.IsNull() && aView->View()->IsActive();
}

// ================================================================
// Function : SetActive
// ================================================================
void ViewManager::SetActive(int theViewId, bool theActive)
{
    const Handle(V3d_Viewer)& aViewer = myWebViewer->GetContext()->CurrentViewer();
    Handle(V3d_View)          aView   = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetActive (" << theViewId << ") called for undefined View";
        return;
    }

    if (theActive) {
        aViewer->SetViewOn(aView);
    } else {
        aViewer->SetViewOff(aView);
    }
}

// ================================================================
// Function : GetInputFocus
// ================================================================
int ViewManager::GetInputFocus() const
{
    return myWebViewer->FocusView()->View()->Identification();
}

// ================================================================
// Function : HasInputFocus
// ================================================================
bool ViewManager::HasInputFocus(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    return !aView.IsNull() && aView == myWebViewer->FocusView();
}

// ================================================================
// Function : SetInputFocus
// ================================================================
void ViewManager::SetInputFocus(int theViewId)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetInputFocus (" << theViewId << ") called for undefined View";
        return;
    }

    myWebViewer->SetFocusView(aView);
}

// ================================================================
// Function : IsSubviewComposer
// ================================================================
bool ViewManager::IsSubviewComposer(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    return !aView.IsNull() && aView->View()->IsSubviewComposer();
}

// ================================================================
// Function : SetSubviewComposer
// ================================================================
void ViewManager::SetSubviewComposer(int theViewId, bool theComposer)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetSubviewComposer (" << theViewId << ") called for undefined View";
        return;
    }

    if (aView->ParentView() != nullptr) {
        Message::SendFail() << "Error: ViewManager::SetSubviewComposer (" << theViewId << ") called for subview";
        return;
    }
    aView->View()->SetSubviewComposer(theComposer);
}

// ================================================================
// Function : GetSubviewSize
// ================================================================
Viewer::Vec2 ViewManager::GetSubviewSize(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetSubviewSize (" << theViewId << ") called for undefined View";
        return Viewer::Vec2();
    } else if (aView->ParentView() == nullptr) {
        return Viewer::Vec2(aView->Window()->Dimensions());
    }

    const int aViewId = aView->View()->Identification();
    return Viewer::Vec2(myViewParams[aViewId].isSubviewFullScreen ? myViewParams[aViewId].subviewSize
                                                                       : aView->View()->SubviewSize());
}

// ================================================================
// Function : SetSubviewSize
// ================================================================
void ViewManager::SetSubviewSize(int theViewId, const Viewer::Vec2& theSize)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetSubviewize (" << theViewId << ") called for undefined View";
        return;
    } else if (aView->ParentView() == nullptr) {
        Message::SendFail() << "Error: ViewManager::SetSubviewSize() is allowed only for subviews";
        return;
    }

    const int aViewId = aView->View()->Identification();
    if (myViewParams[aViewId].isSubviewFullScreen) {
        myViewParams[aViewId].subviewSize = theSize;
    } else if (aView->View()->SubviewSize() != theSize) {
        aView->View()->SetSubviewSize(theSize);
        aView->MustBeResized();
        aView->View()->Invalidate();
        aView->ParentView()->Invalidate();
    }
}

// ================================================================
// Function : GetSubviewCorner
// ================================================================
int ViewManager::GetSubviewCorner(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetSubviewCorner (" << theViewId << ") called for undefined View";
        return static_cast<int>(ViewCorner::LEFT_UPPER);
    }

    if (aView->ParentView() == nullptr) {
        return static_cast<int>(ViewCorner::LEFT_UPPER);
    }
    return (int)Viewer::occToViewCorner(aView->View()->SubviewCorner());
}

// ================================================================
// Function : SetSubviewCorner

// ================================================================
void ViewManager::SetSubviewCorner(int theViewId, int theCorner)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetSubviewCorner (" << theViewId << ") called for undefined View";
        return;
    } else if (aView->ParentView() == nullptr) {
        Message::SendFail() << "Error: ViewManager::SetSubviewCorner() is allowed only for subviews";
        return;
    } else if (theCorner < 0 || theCorner > static_cast<int>(ViewCorner::RIGHT_LOWER)) {
        Message::SendFail() << "Error: ViewManager::SetSubviewCorner() is called with invalid corner '" << theCorner
                            << "'";
        return;
    }

    Aspect_TypeOfTriedronPosition aCorner = occFromViewCorner((ViewCorner)theCorner);
    if (aView->View()->SubviewCorner() != aCorner) {
        aView->View()->SetSubviewCorner(aCorner);
        aView->MustBeResized();
        aView->View()->Invalidate();
        aView->ParentView()->Invalidate();
    }
}

// ================================================================
// Function : GetSubviewOffset
// ================================================================
Viewer::Vec2 ViewManager::GetSubviewOffset(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetSubviewOffset (" << theViewId << ") called for undefined View";
        return Viewer::Vec2();
    } else if (aView->ParentView() == nullptr) {
        return Viewer::Vec2();
    }

    const int aViewId = aView->View()->Identification();
    return Viewer::Vec2(myViewParams[aViewId].isSubviewFullScreen ? myViewParams[aViewId].subviewOffset
                                                                       : aView->View()->SubviewOffset());
}

// ================================================================
// Function : SetSubviewOffset
// ================================================================
void ViewManager::SetSubviewOffset(int theViewId, const Viewer::Vec2& theOffset)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetSubviewOffset (" << theViewId << ") called for undefined View";
        return;
    } else if (aView->ParentView() == nullptr) {
        Message::SendFail() << "Error: ViewManager::SetSubviewOffset() is allowed only for subviews";
        return;
    }

    const int aViewId = aView->View()->Identification();
    if (myViewParams[aViewId].isSubviewFullScreen) {
        myViewParams[aViewId].subviewOffset = theOffset;
    } else if (aView->View()->SubviewOffset() != theOffset) {
        aView->View()->SetSubviewOffset(theOffset);
        aView->MustBeResized();
        aView->View()->Invalidate();
        aView->ParentView()->Invalidate();
    }
}

// ================================================================
// Function : GetSubviewMargins
// ================================================================
Viewer::Vec2 ViewManager::GetSubviewMargins(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::GetViewMargins (" << theViewId << ") called for undefined View";
        return Viewer::Vec2();
    } else if (aView->ParentView() == nullptr) {
        return Viewer::Vec2();
    }

    const int aViewId = aView->View()->Identification();
    return Viewer::Vec2(myViewParams[aViewId].isSubviewFullScreen ? myViewParams[aViewId].subviewMargins
                                                                       : aView->View()->SubviewMargins());
}

// ================================================================
// Function : SetSubviewMargins
// ================================================================
void ViewManager::SetSubviewMargins(int theViewId, const Viewer::Vec2& theMargins)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetViewMargins (" << theViewId << ") called for undefined View";
        return;
    } else if (aView->ParentView() == nullptr) {
        Message::SendFail() << "Error: ViewManager::SetViewMargins() is allowed only for subviews";
        return;
    }

    const int aViewId = aView->View()->Identification();
    if (myViewParams[aViewId].isSubviewFullScreen) {
        myViewParams[aViewId].subviewMargins = Graphic3d_Vec2i(theMargins);
    } else if (aView->View()->SubviewMargins() != Graphic3d_Vec2i(theMargins)) {
        aView->View()->SetSubviewMargins(Graphic3d_Vec2i(theMargins));
        aView->MustBeResized();
        aView->View()->Invalidate();
        aView->ParentView()->Invalidate();
    }
}

// ================================================================
// Function : IsFullscreen
// ================================================================
bool ViewManager::IsFullscreen() const
{
    return myIsFullscreen;
}

#ifdef __EMSCRIPTEN__
//! Format EMSCRIPTEN_RESULT values
static const char* FormatFullscreenError(EMSCRIPTEN_RESULT theRes)
{
    switch (theRes) {
        case EMSCRIPTEN_RESULT_SUCCESS: {
            return "Success";
        }
        case EMSCRIPTEN_RESULT_DEFERRED: {
            return "Deferred";
        }
        case EMSCRIPTEN_RESULT_NOT_SUPPORTED: {
            return "Not Supported";
        }
        case EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED: {
            return "Failed not deferred";
        }
        case EMSCRIPTEN_RESULT_INVALID_TARGET: {
            return "Invalid Target";
        }
        case EMSCRIPTEN_RESULT_UNKNOWN_TARGET: {
            return "Unknown Target";
        }
        case EMSCRIPTEN_RESULT_INVALID_PARAM: {
            return "Invalid Param";
        }
        case EMSCRIPTEN_RESULT_FAILED: {
            return "Failed";
        }
        case EMSCRIPTEN_RESULT_NO_DATA: {
            return "No Data";
        }
    }
    return nullptr;
}
#endif

// ================================================================
// Function : SetFullscreen
// ================================================================
void ViewManager::SetFullscreen(bool theToEnable)
{
    if (myIsFullscreen == theToEnable) {
        //
    }
#ifdef __EMSCRIPTEN__
    if (theToEnable) {
        EmscriptenFullscreenStrategy aFullMode = {};
        aFullMode.scaleMode                    = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
        aFullMode.canvasResolutionScaleMode    = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
        aFullMode.filteringMode                = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
        EMSCRIPTEN_RESULT aRes = emscripten_request_fullscreen_strategy(myWebViewer->CanvasId().ToCString(),
                                                                        true,
                                                                        &aFullMode);
        // @c EMSCRIPTEN_RESULT aRes = emscripten_request_fullscreen (myWebViewer->CanvasId().ToCString(), true);
        // @c aRes = emscripten_enter_soft_fullscreen (myWebViewer->CanvasId().ToCString(), &aFullMode);
        myIsFullscreen = aRes == EMSCRIPTEN_RESULT_SUCCESS || aRes == EMSCRIPTEN_RESULT_DEFERRED;
        switch (aRes) {
            case EMSCRIPTEN_RESULT_SUCCESS: {
                Message::SendInfo() << "Entering fullscreen mode...";
                break;
            }
            case EMSCRIPTEN_RESULT_DEFERRED: {
                Message::SendInfo() << "Entering fullscreen mode (deferred)...";
                break;
            }
            default: {
                if (const char* anErr = FormatFullscreenError(aRes)) {
                    Message::SendFail() << "Unable to enter fullscreen mode: " << anErr;
                }
                break;
            }
        }
    } else {
        myIsFullscreen = false;
        emscripten_exit_fullscreen();
    }
#else
    myIsFullscreen = theToEnable;
#endif
}

// ================================================================
// Function : IsSubviewFullscreen
// ================================================================
bool ViewManager::IsSubviewFullscreen(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId < (int)myViewParams.size() && myViewParams[aViewId].isSubviewFullScreen;
}

//! Verify ViewManager::SetSubviewFullscreen() parameters.
static bool IsValidFullscreenSubview(const Handle(V3d_View)& theView, int theViewId)
{
    if (theView.IsNull()) {
        Message::SendFail() << "Error: SetSubviewFullscreen (" << theViewId << ") called for undefined View";
        return false;
    } else if (theView->ParentView() == nullptr) {
        Message::SendFail() << "Error: SetSubviewFullscreen (" << theViewId << ") called for root view";
        return false;
    }
    return true;
}

//! Temporary disable subview.
static void SetSubviewMarginsDisabled(const Handle(V3d_View)& theView)
{
    theView->Viewer()->SetViewOff(theView);
    theView->View()->SetSubviewSize(Graphic3d_Vec2d(2.0)); // could be removed after OCCT bugfix
    theView->View()->SetSubviewOffset(Graphic3d_Vec2d(-1.0));
    theView->View()->SetSubviewMargins(Graphic3d_Vec2i(0));
    theView->MustBeResized();
}

//! Enable temporary disabled subview.
static void SetSubviewEnabled(const Handle(V3d_View)& theView)
{
    theView->Viewer()->SetViewOn(theView);
    theView->MustBeResized();
    theView->Invalidate();
}

//! Set fullscreen subview margins.
static void SetSubviewMarginsFullscreen(const Handle(V3d_View)& theView)
{
    theView->View()->SetSubviewSize(Graphic3d_Vec2d(1.0));
    theView->View()->SetSubviewOffset(Graphic3d_Vec2d(0.0));
    theView->View()->SetSubviewMargins(Graphic3d_Vec2i(0));
}

// ================================================================
// Function : SetSubviewFullscreen
// ================================================================
void ViewManager::SetSubviewFullscreen(int theViewId, bool theToEnable)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (!IsValidFullscreenSubview(aView, theViewId)) {
        return;
    }

    const int aViewId = aView->View()->Identification();
    if (myViewParams[aViewId].isSubviewFullScreen == theToEnable) {
        return;
    }

    const Handle(V3d_Viewer)&                    aViewer   = myWebViewer->GetContext()->CurrentViewer();
    const NCollection_Sequence<Handle(V3d_View)> aSubviews = myWebViewer->MainView()->Subviews();
    myViewParams[aViewId].isSubviewFullScreen              = theToEnable;
    if (theToEnable) {
        for (const Handle(V3d_View)& aSubviewIter : aSubviews) {
            const int aSubviewId = aSubviewIter->View()->Identification();
            if (aSubviewIter == aView || !aSubviewIter->View()->IsActive()) {
                continue;
            }
            if (!myViewParams[aSubviewId].isSubviewFullScreen) {
                myViewParams[aSubviewId].BackupSubviewParams(aSubviewIter->View());
            }
            myViewParams[aSubviewId].isSubviewFullScreen = false;
            SetSubviewMarginsDisabled(aSubviewIter);
        }

        if (!aView->View()->IsActive()) {
            aViewer->SetViewOn(aView);
        } else {
            myViewParams[aViewId].BackupSubviewParams(aView->View());
        }
        SetSubviewMarginsFullscreen(aView);
        myWebViewer->SetFocusView(aView);
    } else {
        myViewParams[aViewId].RestoreSubviewParams(aView->View());
        for (const Handle(V3d_View)& aSubviewIter : aSubviews) {
            const int aSubviewId = aSubviewIter->View()->Identification();
            if (aSubviewIter != aView && myViewParams[aSubviewId].isSubviewActive) {
                myViewParams[aSubviewId].RestoreSubviewParams(aSubviewIter->View());
                myViewParams[aSubviewId].isSubviewFullScreen = false;
                SetSubviewEnabled(aSubviewIter);
            }
        }
    }

    aView->MustBeResized();
    aView->Invalidate();
    aView->ParentView()->Invalidate();
#ifdef __EMSCRIPTEN__
    jsOnSubviewFullscreen(aViewId, theToEnable);
#endif
}

// ================================================================
// Function : IsSubviewClickable
// ================================================================
bool ViewManager::IsSubviewClickable(int theViewId) const
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        return false;
    }

    const int aViewId = aView->View()->Identification();
    return aViewId < (int)myViewParams.size() && myViewParams[aViewId].isSubviewClickable;
}

// ================================================================
// Function : SetSubviewClickable
// ================================================================
void ViewManager::SetSubviewClickable(int theViewId, bool theToEnable)
{
    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: ViewManager::SetSubviewClickable (" << theViewId
                            << ") called for undefined View";
        return;
    } else if (aView->ParentView() == nullptr) {
        Message::SendFail() << "Error: ViewManager::SetSubviewClickable (" << theViewId << ") called for root view";
        return;
    }

    const int aViewId                        = aView->View()->Identification();
    myViewParams[aViewId].isSubviewClickable = theToEnable;
}

// ================================================================
// Function : setViewOffset
// ================================================================
void ViewManager::setViewOffset(const int theSide, const Vec2& theOffset)
{
    myViewOffsets.Bind(Viewer::occFromViewCorner((Viewer::ViewCorner)theSide), Graphic3d_Vec2i(theOffset));
    myWebViewer->Invalidate2dObjects();
}

// ================================================================
// Function : unsetViewOffsets
// ================================================================
void ViewManager::unsetViewOffsets()
{
    myViewOffsets.Clear();
    myWebViewer->Invalidate2dObjects();
}

// ================================================================
// Function : invalidate2dObjects
// ================================================================
void ViewManager::invalidate2dObjects(const ObjectId& theObj)
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObj);
    myWebViewer->Invalidate2dObjects(anObj);
}

// Module exports
#ifdef __EMSCRIPTEN__
//! Register scene-related functions.
void registerSceneFunctions(emscripten::class_<ViewManager, emscripten::base<IManager>>& theViewManager)
{
    theViewManager.function("getSceneMinMax", &ViewManager::GetSceneMinMax)
        .function("isFitMinMax", &ViewManager::IsFitMinMax)
        .function("fitMinMax", &ViewManager::FitMinMax)
        .function("fitAllAuto", &ViewManager::FitAllAuto)
        .function("setOrientation", &ViewManager::SetOrientation)
        .function("getViewPoint", &ViewManager::GetViewPoint)
        .function("setViewPoint", &ViewManager::SetViewPoint)
        .function("getSubviewBorderColor", &ViewManager::GetSubviewBorderColor)
        .function("setSubviewBorderColor", &ViewManager::SetSubviewBorderColor)
        .function("isFullscreen", &ViewManager::IsFullscreen)
        .function("setFullscreen", &ViewManager::SetFullscreen)
        .function("setViewOffset", &ViewManager::setViewOffset)
        .function("unsetViewOffsets", &ViewManager::unsetViewOffsets)
        .function("invalidate2dObjects", &ViewManager::invalidate2dObjects);
}

//! Register interaction-related functions.
void registerInteractionFunctions(emscripten::class_<ViewManager, emscripten::base<IManager>>& theViewManager)
{
    theViewManager.function("toAllowRotation", &ViewManager::ToAllowRotation)
        .function("setAllowRotation", &ViewManager::SetAllowRotation)
        .function("getOrbitAcceleration", &ViewManager::GetOrbitAcceleration)
        .function("setOrbitAcceleration", &ViewManager::SetOrbitAcceleration)
        .function("toLockOrbitZUp", &ViewManager::ToLockOrbitZUp)
        .function("setLockOrbitZUp", &ViewManager::SetLockOrbitZUp)
        .function("toAllowPanning", &ViewManager::ToAllowPanning)
        .function("setAllowPanning", &ViewManager::SetAllowPanning)
        .function("toAllowZooming", &ViewManager::ToAllowZooming)
        .function("setAllowZooming", &ViewManager::SetAllowZooming)
        .function("getMouseGesture", &ViewManager::getMouseGesture)
        .function("setMouseGesture", &ViewManager::setMouseGesture)
        .function("getMaxCamDistanceMultiplier", &ViewManager::GetMaxCamDistanceMultiplier)
        .function("setMaxCamDistanceMultiplier", &ViewManager::SetMaxCamDistanceMultiplier);
}

//! Register camera-related functions.
void registerCameraFunctions(emscripten::class_<ViewManager, emscripten::base<IManager>>& theViewManager)
{
    theViewManager.function("isPerspectiveProjection", &ViewManager::IsPerspectiveProjection)
        .function("setPerspectiveProjection", &ViewManager::SetPerspectiveProjection)
        .function("getPerspectiveProjectionFOVy", &ViewManager::GetPerspectiveProjectionFOVy)
        .function("setPerspectiveProjectionFOVy", &ViewManager::SetPerspectiveProjectionFOVy)
        .function("getStereoMode", &ViewManager::GetStereoMode)
        .function("setStereoMode", &ViewManager::SetStereoMode);
}

//! Register subview-related functions.
void registerSubviewFunctions(emscripten::class_<ViewManager, emscripten::base<IManager>>& theViewManager)
{
    theViewManager.function("isDefined", &ViewManager::IsDefined)
        .function("createSubview", &ViewManager::CreateSubview)
        .function("removeSubview", &ViewManager::RemoveSubview)
        .function("removeAllSubviews", &ViewManager::RemoveAllSubviews)
        .function("isActive", &ViewManager::IsActive)
        .function("setActive", &ViewManager::SetActive)
        .function("getInputFocus", &ViewManager::GetInputFocus)
        .function("hasInputFocus", &ViewManager::HasInputFocus)
        .function("setInputFocus", &ViewManager::SetInputFocus)
        .function("isSubviewComposer", &ViewManager::IsSubviewComposer)
        .function("setSubviewComposer", &ViewManager::SetSubviewComposer)
        .function("getSubviewSize", &ViewManager::GetSubviewSize)
        .function("setSubviewSize", &ViewManager::SetSubviewSize)
        .function("getSubviewCorner", &ViewManager::GetSubviewCorner)
        .function("setSubviewCorner", &ViewManager::SetSubviewCorner)
        .function("getSubviewOffset", &ViewManager::GetSubviewOffset)
        .function("setSubviewOffset", &ViewManager::SetSubviewOffset)
        .function("getSubviewMargins", &ViewManager::GetSubviewMargins)
        .function("setSubviewMargins", &ViewManager::SetSubviewMargins)
        .function("isSubviewFullscreen", &ViewManager::IsSubviewFullscreen)
        .function("setSubviewFullscreen", &ViewManager::SetSubviewFullscreen)
        .function("isSubviewClickable", &ViewManager::IsSubviewClickable)
        .function("setSubviewClickable", &ViewManager::SetSubviewClickable);
}

EMSCRIPTEN_BINDINGS(ViewManager)
{
    emscripten::enum_<V3d_TypeOfOrientation>("CameraOrientation")
        .value("LEFT", V3d_TypeOfOrientation_Zup_Left)
        .value("RIGHT", V3d_TypeOfOrientation_Zup_Right)
        .value("BOTTOM", V3d_TypeOfOrientation_Zup_Bottom)
        .value("TOP", V3d_TypeOfOrientation_Zup_Top)
        .value("FRONT", V3d_TypeOfOrientation_Zup_Front)
        .value("BACK", V3d_TypeOfOrientation_Zup_Back)
        .value("AXORIGHT", V3d_TypeOfOrientation_Zup_AxoRight)
        .value("AXOLEFT", V3d_TypeOfOrientation_Zup_AxoLeft)
        .value("XnegYneg", V3d_XnegYneg)
        .value("XposYneg", V3d_XposYneg);

    emscripten::class_<ViewManager, emscripten::base<IManager>> aViewManager("ViewManager");
    registerSceneFunctions(aViewManager);
    registerInteractionFunctions(aViewManager);
    registerCameraFunctions(aViewManager);
    registerSubviewFunctions(aViewManager);
}
#endif

} // namespace