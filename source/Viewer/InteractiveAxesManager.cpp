
#include "InteractiveAxesManager.h"

#include <ViewCae/InteractiveSphericalAxes.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveSphericalAxesManager
// ================================================================
InteractiveAxesManager::InteractiveAxesManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateSphericalAxes
// ================================================================
void InteractiveAxesManager::CreateSphericalAxes(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);
    Handle(ViewCae::InteractiveSphericalAxes) aPrs = new ViewCae::InteractiveSphericalAxes(
        new ViewBase::InteractiveOwner(theObjId.c_str()));
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Displayed);
}

// ================================================================
// Function : GetAxesPosition
// ================================================================
Vec3 InteractiveAxesManager::GetAxesPosition(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetAxesPosition (" << theObjId << ") called for non-existing object";
        return Vec3::InvalidPoint();
    }
    return Vec3(aPrs->GetPosition());
}

// ================================================================
// Function : SetAxesPosition
// ================================================================
void InteractiveAxesManager::SetAxesPosition(const ObjectId& theObjId, const Vec3& thePos)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetAxesPosition (" << theObjId << ") called for non-existing object";
        return;
    }
    aPrs->SetPosition(thePos.XYZ());
}

// ================================================================
// Function : GetAxesScale
// ================================================================
double InteractiveAxesManager::GetAxesScale(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetAxesScale (" << theObjId << ") called for non-existing object";
        return 1.0;
    }
    return aPrs->GetScale();
}

// ================================================================
// Function : SetAxesScale
// ================================================================
void InteractiveAxesManager::SetAxesScale(const ObjectId& theObjId, double theScale)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetAxesScale (" << theObjId << ") called for non-existing object";
        return;
    }
    aPrs->SetScale(theScale);
}

// ================================================================
// Function : GetAngularArrowDir
// ================================================================
std::string InteractiveAxesManager::GetAngularArrowDir(const ObjectId& theObjId, const std::string& theAxisId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetAngularArrowDir (" << theObjId << ") called for non-existing object";
        return "";
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: GetAngularArrowDir (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return "";
    }
    return ViewCae::ArrowOrientationConverter::GetString(aPrs->GetAngularArrowDir(anAxisEnum));
}

// ================================================================
// Function : SetAngularArrowDir
// ================================================================
void InteractiveAxesManager::SetAngularArrowDir(const ObjectId&    theObjId,
                                                const std::string& theAxisId,
                                                const std::string& theDir)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetAngularArrowDir (" << theObjId << ") called for non-existing object";
        return;
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: SetAngularArrowDir (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return;
    }

    ViewCae::ArrowOrientation aDirEnum = ViewCae::ArrowOrientationConverter::GetValue(theDir.c_str());
    if (aDirEnum == ViewCae::ArrowOrientation::Unknown) {
        Message::SendFail() << "Error: SetAngularArrowDir (" << theObjId << ") called for invalid orientation '"
                            << theDir << "'";
        return;
    }
    aPrs->SetAngularArrowDir(anAxisEnum, aDirEnum);
}

// ================================================================
// Function : GetAngularArrowAxis
// ================================================================
std::string InteractiveAxesManager::GetAngularArrowAxis(const ObjectId& theObjId, const std::string& theAxisId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetAngularArrowAxis (" << theObjId << ") called for non-existing object";
        return "";
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: GetAngularArrowAxis (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return "";
    }
    return ViewCae::AxisTypeConverter::GetString(aPrs->GetAngularArrowAxis(anAxisEnum));
}

// ================================================================
// Function : SetAngularArrowAxis
// ================================================================
void InteractiveAxesManager::SetAngularArrowAxis(const ObjectId&    theObjId,
                                                 const std::string& theAxisId,
                                                 const std::string& theZeroAxis)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetAngularArrowAxis (" << theObjId << ") called for non-existing object";
        return;
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: SetAngularArrowAxis (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return;
    }

    ViewCae::AxisType aZeroAxisEnum = ViewCae::AxisTypeConverter::GetValue(theZeroAxis.c_str());
    if (aZeroAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: SetAngularArrowAxis (" << theObjId << ") called for invalid attachment axis '"
                            << theZeroAxis << "'";
        return;
    }
    aPrs->SetAngularArrowAxis(anAxisEnum, aZeroAxisEnum);
}

// ================================================================
// Function : GetAngularArrowLabel
// ================================================================
std::string InteractiveAxesManager::GetAngularArrowLabel(const ObjectId& theObjId, const std::string& theAxisId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetAngularArrowLabel (" << theObjId << ") called for non-existing object";
        return "";
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: GetAngularArrowLabel (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return "";
    }
    return TCollection_AsciiString(aPrs->GetAngularArrowLabel(anAxisEnum)).ToCString();
}

// ================================================================
// Function : SetAngularArrowLabel
// ================================================================
void InteractiveAxesManager::SetAngularArrowLabel(const ObjectId&    theObjId,
                                                  const std::string& theAxisId,
                                                  const std::string& theLabel)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetAngularArrowLabel (" << theObjId << ") called for non-existing object";
        return;
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxisId.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: SetAngularArrowLabel (" << theObjId << ") called for invalid axis '" << theAxisId
                            << "'";
        return;
    }
    aPrs->SetAngularArrowLabel(anAxisEnum, theLabel.c_str());
}

// ================================================================
// Function : GetOrientation
// ================================================================
std::string InteractiveAxesManager::GetOrientation(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetOrientation (" << theObjId << ") called for non-existing object";
        return "";
    }

    return ViewCae::AxisTypeConverter::GetString(aPrs->GetOrientation());
}

// ================================================================
// Function : SetOrientation
// ================================================================
void InteractiveAxesManager::SetOrientation(const ObjectId& theObjId, const std::string& theAxis)
{
    Handle(ViewCae::InteractiveSphericalAxes) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetOrientation (" << theObjId << ") called for non-existing object";
        return;
    }

    ViewCae::AxisType anAxisEnum = ViewCae::AxisTypeConverter::GetValue(theAxis.c_str());
    if (anAxisEnum == ViewCae::AxisType::Unknown) {
        Message::SendFail() << "Error: SetOrientation (" << theObjId << ") called for invalid axis '" << theAxis << "'";
        return;
    }
    aPrs->SetOrientation(anAxisEnum);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveAxesManager)
{
    emscripten::class_<InteractiveAxesManager, emscripten::base<IManager>>("InteractiveAxesManager")
        .function("createSphericalAxes", &InteractiveAxesManager::CreateSphericalAxes)
        .function("getAxesPosition", &InteractiveAxesManager::GetAxesPosition)
        .function("setAxesPosition", &InteractiveAxesManager::SetAxesPosition)
        .function("getAxesScale", &InteractiveAxesManager::GetAxesScale)
        .function("setAxesScale", &InteractiveAxesManager::SetAxesScale)
        .function("getAngularArrowDir", &InteractiveAxesManager::GetAngularArrowDir)
        .function("setAngularArrowDir", &InteractiveAxesManager::SetAngularArrowDir)
        .function("getAngularArrowAxis", &InteractiveAxesManager::GetAngularArrowAxis)
        .function("setAngularArrowAxis", &InteractiveAxesManager::SetAngularArrowAxis)
        .function("getAngularArrowLabel", &InteractiveAxesManager::GetAngularArrowLabel)
        .function("setAngularArrowLabel", &InteractiveAxesManager::SetAngularArrowLabel)
        .function("getOrientation", &InteractiveAxesManager::GetOrientation)
        .function("setOrientation", &InteractiveAxesManager::SetOrientation);
}
#endif

} // namespace