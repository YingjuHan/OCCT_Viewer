#ifndef _VIEWMANAGER_HEADERFILE
#define _VIEWMANAGER_HEADERFILE

#include <V3d_View.hxx>
#include <Quantity_ColorRGBA.hxx>

#include "IManager.h"
#include "Utils/CameraViewPoint.h"
#include "Utils/ColorTools.h"
#include "Utils/VecN.h"

#include <vector>

namespace Viewer {
//! Left mouse gesture configuration.
enum class MouseGesture { NONE = 0, ROTATE = 1, ROTATEVIEW = 2, PAN = 3, ZOOM = 4 };

//! Stereoscopic modes.
enum class StereoMode { MONO = 0, REDCYAN = 1, ROWINTERLACED = 2, ROWINTERLACEDINV = 3 };

//! Per-view parameters.
struct ViewLockParams {
    Graphic3d_Vec2d subviewSize;
    Graphic3d_Vec2d subviewOffset;
    Graphic3d_Vec2i subviewMargins;
    MouseGesture    leftMouseGesture    = MouseGesture::ROTATE;
    MouseGesture    middleMouseGesture  = MouseGesture::PAN;
    MouseGesture    rightMouseGesture   = MouseGesture::ROTATEVIEW;
    bool            isSubviewActive     = true;
    bool            isSubviewFullScreen = false;
    bool            isSubviewClickable  = false;
    bool            toAllowZooming      = true;
    bool            toAllowPanning      = true;
    bool            toAllowRotation     = true;
    bool            toLockOrbitZUp      = false;

    //! Copy parameters from specified subview.
    Standard_EXPORT void BackupSubviewParams(const Handle(Graphic3d_CView)& theView);

    //! Copy parameters into specified subview.
    Standard_EXPORT void RestoreSubviewParams(const Handle(Graphic3d_CView)& theView);
};

//! Class provides public interface for managing (sub)views.
class ViewManager : public IManager {
public:
    //! Default margin for fit all operation.
    static constexpr double FIT_ALL_MARGIN = 0.01;

public:
    //! Main constructor
    Standard_EXPORT explicit ViewManager(WasmViewer* theViewer);

    //! Return OrbitAcceleration coefficient.
    Standard_EXPORT float GetOrbitAcceleration() const;

    //! Set OrbitAcceleration coefficient.
    Standard_EXPORT void SetOrbitAcceleration(float theOrbitAcceleration);

    //! Return max camera distance multiplier.
    Standard_EXPORT float GetMaxCamDistanceMultiplier() const;

    //! Set max camera distance multiplier.
    //! @param[in] theMaxCamDistanceMultiplier max camera distance multiplier value
    //!                                        if it equals 0 or negative then disable the max zoom limit
    Standard_EXPORT void SetMaxCamDistanceMultiplier(float theMaxCamDistanceMultiplier);

    //! Get subview border color.
    Standard_EXPORT ColorTools::ColorString GetSubviewBorderColor() const;

    //! Set subview border color.
    Standard_EXPORT void SetSubviewBorderColor(const ColorTools::ColorString& theHexColor);

    //! Returns fullscreen mode state.
    Standard_EXPORT bool IsFullscreen() const;

    //! Enable/disable fullscreen mode.
    //! WARNING! This should be called in combination with user gesture (mouse click / button click) in browser!
    Standard_EXPORT void SetFullscreen(bool theToEnable);

public:
    //! Return scene bounding box if camera precisely fits into the box.
    Standard_EXPORT BndBox3 GetSceneMinMax(int theViewId, bool theSelection) const;

    //! Return TRUE if camera precisely fits into the box.
    Standard_EXPORT bool IsFitMinMax(int theViewId, const BndBox3& theMinMax) const;

    //! Fit AABB into view.
    Standard_EXPORT void FitMinMax(int theViewId, const BndBox3& theMinMax, bool theToAnimate);

    //! Fit selected or fit all visible objects into view.
    Standard_EXPORT void FitAllAuto(int theViewId, bool theIsAuto);

    //! Set camera view orientation (V3d_TypeOfOrientation )
    Standard_EXPORT void SetOrientation(int theViewId, int theOrientation, bool theToAnimate);

    //! Get current view params.
    Standard_EXPORT CameraViewPoint GetViewPoint(int theViewId);

    //! Restore view params.
    Standard_EXPORT void SetViewPoint(int theViewId, const CameraViewPoint& theParams);

public:
    //! Return per-view lock parameters.
    const std::vector<ViewLockParams>& getViewLockParam() const { return myViewParams; }

    //! Returns rotation status.
    Standard_EXPORT bool ToAllowRotation(int theViewId) const;

    //! Enable/disable rotation.
    Standard_EXPORT void SetAllowRotation(int theViewId, bool theToEnable);

    //! Return TRUE if camera up orientation within AIS_NavigationMode_Orbit
    //! rotation mode should be forced Z up; FALSE by default.
    Standard_EXPORT bool ToLockOrbitZUp(int theViewId) const;

    //! Set if camera up orientation within AIS_NavigationMode_Orbit rotation mode should be forced Z up.
    Standard_EXPORT void SetLockOrbitZUp(int theViewId, bool theToForceUp);

    //! Returns pan status.
    Standard_EXPORT bool ToAllowPanning(int theViewId) const;

    //! Enable/disable panning.
    Standard_EXPORT void SetAllowPanning(int theViewId, bool theToEnable);

    //! Returns zoom status.
    Standard_EXPORT bool ToAllowZooming(int theViewId) const;

    //! Enable/disable zoom.
    Standard_EXPORT void SetAllowZooming(int theViewId, bool theToEnable);

    //! Returns active mouse gesture within Viewer::MouseGesture enum.
    Standard_EXPORT int getMouseGesture(int theViewId, int theButton) const;

    //! Sets active mouse gesture Viewer3::MouseGesture enum.
    Standard_EXPORT void setMouseGesture(int theViewId, int theButton, int theGesture);

public:
    //! Return TRUE if perspective projection is active.
    Standard_EXPORT bool IsPerspectiveProjection(int theViewId) const;

    //! Enable/disable perspective projection.
    Standard_EXPORT void SetPerspectiveProjection(int theViewId, bool theIsPerspective);

    //! Return perspective projection FOVy angle in degrees.
    Standard_EXPORT double GetPerspectiveProjectionFOVy(int theViewId) const;

    //! Set perspective projection FOVy angle in degrees between the upper and lower sides of the viewing frustum.
    Standard_EXPORT void SetPerspectiveProjectionFOVy(int theViewId, double theFOVy);

    //! Return stereoscopic mode.
    Standard_EXPORT int GetStereoMode(int theViewId) const;

    //! Set stereoscopic mode.
    Standard_EXPORT void SetStereoMode(int theViewId, int theMode);

public:
    //! Define new subview.
    Standard_EXPORT int CreateSubview(const Viewer::Vec2& theSize);

    //! Remove subview.
    Standard_EXPORT void RemoveSubview(int theViewId);

    //! Remove all subview.
    Standard_EXPORT void RemoveAllSubviews();

    //! Return TRUE if view with specified Id exists.
    Standard_EXPORT bool IsDefined(int theViewId) const;

    //! Return TRUE if view with specified Id is in active state.
    Standard_EXPORT bool IsActive(int theViewId) const;

    //! Activate or deactivate view with specified Id.
    Standard_EXPORT void SetActive(int theViewId, bool theActive);

    //! Return view id that receives input focus.
    Standard_EXPORT int GetInputFocus() const;

    //! Return TRUE if view with specified Id receives input focus.
    Standard_EXPORT bool HasInputFocus(int theViewId) const;

    //! Set view with specified Id to receive input focus.
    Standard_EXPORT void SetInputFocus(int theViewId);

    //! Return TRUE if view serves as a composer to subviews.
    Standard_EXPORT bool IsSubviewComposer(int theViewId) const;

    //! Set composer flag.
    Standard_EXPORT void SetSubviewComposer(int theViewId, bool theComposer);

    //! Return subview size; (1.0,1.0) by default which means filling in full parent view.
    Standard_EXPORT Viewer::Vec2 GetSubviewSize(int theViewId) const;

    //! Set subview size.
    Standard_EXPORT void SetSubviewSize(int theViewId, const Viewer::Vec2& theSize);

    //! Return subview corner (Viewer::ViewCorner); ViewCorner_LeftUpper by default.
    Standard_EXPORT int GetSubviewCorner(int theViewId) const;

    //! Set subview corner (Viewer::ViewCorner).
    Standard_EXPORT void SetSubviewCorner(int theViewId, int theCorner);

    //! Return subview offset; (0,0) by default.
    Standard_EXPORT Viewer::Vec2 GetSubviewOffset(int theViewId) const;

    //! Set subview offset.
    Standard_EXPORT void SetSubviewOffset(int theViewId, const Viewer::Vec2& theOffset);

    //! Return subview margins in pixels; (0,0) by default.
    Standard_EXPORT Viewer::Vec2 GetSubviewMargins(int theViewId) const;

    //! Set subview margins in pixels.
    Standard_EXPORT void SetSubviewMargins(int theViewId, const Viewer::Vec2& theMargins);

    //! Returns fullscreen mode state.
    Standard_EXPORT bool IsSubviewFullscreen(int theViewId) const;

    //! Enable/disable fullscreen mode.
    Standard_EXPORT void SetSubviewFullscreen(int theViewId, bool theToEnable);

    //! Returns clickable property state.
    Standard_EXPORT bool IsSubviewClickable(int theViewId) const;

    //! Enable/disable clickable.
    Standard_EXPORT void SetSubviewClickable(int theViewId, bool theToEnable);

public:
    using ViewOffsets = NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i>;

    //! Return defined viewer offsets.
    const ViewOffsets& GetViewOffsets() const { return myViewOffsets; }

    //! Set viewer offset for specified corner @sa Viewer::ViewCorner.
    Standard_EXPORT void setViewOffset(const int theSide, const Vec2& theOffset);

    //! Reset all viewer offsets.
    Standard_EXPORT void unsetViewOffsets();

    //! Mark all 2d objects to update their position/size on the next draw call.
    Standard_EXPORT void invalidate2dObjects(const ObjectId& theObjId);

private:
    //! Hide objects.
    void hideObjects(const AIS_ListOfInteractive& theObjects);

private:
    // three mouse buttons
    static constexpr int MOUSE_BTN0 = 0, MOUSE_BTN1 = 1, MOUSE_BTN2 = 2;

    class AnimatedCamera;

private:
    std::vector<ViewLockParams> myViewParams;
    Quantity_Color              mySubviewBorderColor; //!< view border color (between subviews)
    Standard_Boolean            myIsFullscreen = false;
    ViewOffsets                 myViewOffsets;
};

} // namespace

#endif // _VIEWMANAGER_HEADERFILE

