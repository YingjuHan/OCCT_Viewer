#include "UnitManager.h"

#include "WasmViewer.h"

#include <NCollection_Mat4.hxx>
#include <cmath>
#include <Precision.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_EulerSequence.hxx>
#include <gp_Pnt.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/UnitManager.h>

#include <ViewDim/InteractiveDimensionOwner.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
namespace {
gp_Quaternion toQuat(const Vec4& theRot)
{
    return gp_Quaternion(theRot.X(), theRot.Y(), theRot.Z(), theRot.W());
}

gp_Vec toVec(const Vec3& theVec)
{
    return gp_Vec(theVec.X(), theVec.Y(), theVec.Z());
}

gp_Trsf toTrsf(const Location& theLoc)
{
    gp_Trsf aTrsf;
    const double aScale = (std::abs(theLoc.scale) <= Precision::Confusion()) ? 1.0 : theLoc.scale;
    aTrsf.SetRotationPart(toQuat(theLoc.rotation));
    aTrsf.SetTranslationPart(toVec(theLoc.translation));
    if (std::abs(aScale - 1.0) > Precision::Confusion()) {
        aTrsf.SetScaleFactor(aScale);
    }
    return aTrsf;
}

Mat4 toMat4(const gp_Trsf& theTrsf)
{
    NCollection_Mat4<double> aMat;
    theTrsf.GetMat4(aMat);
    Mat4 aRes;
    aRes.row1 = Vec4(aMat.GetValue(0, 0), aMat.GetValue(0, 1), aMat.GetValue(0, 2), aMat.GetValue(0, 3));
    aRes.row2 = Vec4(aMat.GetValue(1, 0), aMat.GetValue(1, 1), aMat.GetValue(1, 2), aMat.GetValue(1, 3));
    aRes.row3 = Vec4(aMat.GetValue(2, 0), aMat.GetValue(2, 1), aMat.GetValue(2, 2), aMat.GetValue(2, 3));
    aRes.row4 = Vec4(aMat.GetValue(3, 0), aMat.GetValue(3, 1), aMat.GetValue(3, 2), aMat.GetValue(3, 3));
    return aRes;
}
} // namespace

#ifdef __EMSCRIPTEN__
//! Callback for units changes.
EM_JS(bool, jsOnUnitsChanged, (int theQuantType, int theSection), {
    return Module.eventManager.onUnitsChanged(theQuantType, theSection);
});
#endif

// ================================================================
// Function : UnitManager
// ================================================================
UnitManager::UnitManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : FormatUnitName
// ================================================================
std::string UnitManager::FormatUnitName(const std::string& theUnit) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->FormatUnitName(theUnit.c_str()).ToCString();
}

// ================================================================
// Function : FindQuantity
// ================================================================
int UnitManager::FindQuantity(const std::string& theUnit) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return (int)aMgr->FindQuantity(theUnit.c_str());
}

// ================================================================
// Function : GetQuantityName
// ================================================================
std::string UnitManager::GetQuantityName(int theQuantity) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: GetQuantityName(" << theQuantity << ") is called with invalid quantity type";
        return std::string();
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->GetQuantityName((ViewBase::QuantityType)theQuantity).ToCString();
}

// ================================================================
// Function : GetQuantityUnit
// ================================================================
std::string UnitManager::GetQuantityUnit(int theQuantity, int theSec) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: GetQuantityUnit(" << theQuantity << ", " << theSec
                            << ") is called with invalid quantity type";
        return std::string();
    } else if (!ViewBase::UnitManager::IsSectionEnum(theSec)) {
        Message::SendFail() << "Error: GetQuantityUnit(" << theQuantity << ", " << theSec
                            << ") is called with invalid section type";
        return std::string();
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->GetQuantityUnit((ViewBase::QuantityType)theQuantity, (ViewBase::UnitSection)theSec).ToCString();
}

// ================================================================
// Function : SetQuantityUnit
// ================================================================
void UnitManager::SetQuantityUnit(int theQuantity, int theSec, const std::string& theUnit)
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: SetQuantityUnit(" << theQuantity << ", " << theSec << ", " << theUnit
                            << ") is called with invalid quantity type";
        return;
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    if (theSec == (int)ViewBase::UnitSection::System) {
        aMgr->SetSystemUnit((ViewBase::QuantityType)theQuantity, theUnit.c_str());
    } else if (theSec == (int)ViewBase::UnitSection::Display) {
        aMgr->SetDisplayUnit((ViewBase::QuantityType)theQuantity, theUnit.c_str());
    } else {
        Message::SendFail() << "Error: SetQuantityUnit(" << theQuantity << ", " << theSec << ", " << theUnit
                            << ") is called with invalid section type";
        return;
    }

    if (ViewBase::BaseDefaultDrawer* aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer*>(
            myWebViewer->GetContext()->DefaultDrawer().get())) {
        aDefDrawer->UpdateUnits();
    }

    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*myWebViewer->GetContext()); anOwnIter.More();
         anOwnIter.Next()) {
        if (dynamic_cast<ViewDim::InteractiveDimensionOwner*>(anOwnIter.Current()) != nullptr) {
            anOwnIter.CurrentObject()->SetToUpdate();
        }
    }

#ifdef __EMSCRIPTEN__
    jsOnUnitsChanged(theQuantity, theSec);
#endif
}

// ================================================================
// Function : Convert
// ================================================================
double UnitManager::Convert(const double theVal, const std::string& theFromUnit, const std::string& theToUnit) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->Convert(theVal, theFromUnit.c_str(), theToUnit.c_str());
}

// ================================================================
// Function : ConvertAnyFromSI
// ================================================================
double UnitManager::ConvertAnyFromSI(const double theVal, const std::string& theToUnit) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->ConvertAnyFromSI(theVal, theToUnit.c_str());
}

// ================================================================
// Function : ConvertAnyToSI
// ================================================================
double UnitManager::ConvertAnyToSI(const double theVal, const std::string& theFromUnit) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->ConvertAnyToSI(theVal, theFromUnit.c_str());
}

// ================================================================
// Function : SystemToDisplay
// ================================================================
double UnitManager::SystemToDisplay(int theQuantity, double theSysVal) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: SystemToDisplay(" << theQuantity << ", " << theSysVal
                            << ") is called with invalid quantity type";
        return 0.0;
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->SystemToDisplay((ViewBase::QuantityType)theQuantity, theSysVal);
}

// ================================================================
// Function : SystemFromDisplay
// ================================================================
double UnitManager::SystemFromDisplay(int theQuantity, double theDispVal) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: SystemFromDisplay(" << theQuantity << ", " << theDispVal
                            << ") is called with invalid quantity type";
        return 0.0;
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->SystemFromDisplay((ViewBase::QuantityType)theQuantity, theDispVal);
}

// ================================================================
// Function : SystemToSI
// ================================================================
double UnitManager::SystemToSI(int theQuantity, double theSysVal) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: SystemToSI(" << theQuantity << ", " << theSysVal
                            << ") is called with invalid quantity type";
        return 0.0;
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->SystemToSI((ViewBase::QuantityType)theQuantity, theSysVal);
}

// ================================================================
// Function : SystemFromSI
// ================================================================
double UnitManager::SystemFromSI(int theQuantity, double theSIVal) const
{
    if (!ViewBase::UnitManager::IsQuantityType(theQuantity)) {
        Message::SendFail() << "Error: SystemFromSI(" << theQuantity << ", " << theSIVal
                            << ") is called with invalid quantity type";
        return 0.0;
    }

    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    return aMgr->SystemFromSI((ViewBase::QuantityType)theQuantity, theSIVal);
}

// ================================================================
// Function : TrsfMultiply
// ================================================================
Location UnitManager::TrsfMultiply(const Location& theLoc1, const Location& theLoc2) const
{
    gp_Trsf aTrsf1 = toTrsf(theLoc1);
    gp_Trsf aTrsf2 = toTrsf(theLoc2);
    gp_Trsf aRes   = aTrsf1.Multiplied(aTrsf2);
    return Location(aRes);
}

// ================================================================
// Function : TrsfInvert
// ================================================================
Location UnitManager::TrsfInvert(const Location& theLoc) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    aTrsf.Invert();
    return Location(aTrsf);
}

// ================================================================
// Function : TrsfGetMat4
// ================================================================
Mat4 UnitManager::TrsfGetMat4(const Location& theLoc) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    return toMat4(aTrsf);
}

// ================================================================
// Function : TrsfGetEulerAngles
// ================================================================
Vec3 UnitManager::TrsfGetEulerAngles(const Location& theLoc, int theSeq) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    gp_Quaternion aRot = aTrsf.GetRotation();
    Standard_Real anAlpha = 0.0;
    Standard_Real aBeta = 0.0;
    Standard_Real aGamma = 0.0;
    aRot.GetEulerAngles(static_cast<gp_EulerSequence>(theSeq), anAlpha, aBeta, aGamma);
    return Vec3(gp_XYZ(anAlpha, aBeta, aGamma));
}

// ================================================================
// Function : TrsfSetEulerAngles
// ================================================================
Location UnitManager::TrsfSetEulerAngles(const Location& theLoc, const Vec3& theAngles, int theSeq) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    gp_Quaternion aRot;
    aRot.SetEulerAngles(static_cast<gp_EulerSequence>(theSeq), theAngles.X(), theAngles.Y(), theAngles.Z());
    aTrsf.SetRotationPart(aRot);
    return Location(aTrsf);
}

// ================================================================
// Function : TrsfRotateAroundAxis
// ================================================================
Location UnitManager::TrsfRotateAroundAxis(const Vec3& theDir, double theAngle, const Vec3& theOrigin) const
{
    gp_Vec aDirVec = toVec(theDir);
    if (aDirVec.SquareMagnitude() <= Precision::Confusion() * Precision::Confusion()) {
        return Location();
    }

    gp_Ax1 anAxis(gp_Pnt(theOrigin.X(), theOrigin.Y(), theOrigin.Z()), gp_Dir(aDirVec));
    gp_Trsf aTrsf;
    aTrsf.SetRotation(anAxis, theAngle);
    return Location(aTrsf);
}

// ================================================================
// Function : TrsfTransformPnt
// ================================================================
Vec3 UnitManager::TrsfTransformPnt(const Location& theLoc, const Vec3& thePnt) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    gp_Pnt aPnt(thePnt.X(), thePnt.Y(), thePnt.Z());
    aPnt.Transform(aTrsf);
    return Vec3(aPnt.XYZ());
}

// ================================================================
// Function : TrsfTransformDir
// ================================================================
Vec3 UnitManager::TrsfTransformDir(const Location& theLoc, const Vec3& theDir) const
{
    gp_Trsf aTrsf = toTrsf(theLoc);
    gp_Vec aVec(theDir.X(), theDir.Y(), theDir.Z());
    aVec.Transform(aTrsf);
    return Vec3(aVec.XYZ());
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(UnitManager)
{
    emscripten::class_<UnitManager, emscripten::base<IManager>>("UnitManager")
        .function("formatUnitName", &UnitManager::FormatUnitName)
        .function("findQuantity", &UnitManager::FindQuantity)
        .function("getQuantityName", &UnitManager::GetQuantityName)
        .function("getQuantityUnit", &UnitManager::GetQuantityUnit)
        .function("setQuantityUnit", &UnitManager::SetQuantityUnit)
        .function("convert", &UnitManager::Convert)
        .function("convertAnyFromSI", &UnitManager::ConvertAnyFromSI)
        .function("convertAnyToSI", &UnitManager::ConvertAnyToSI)
        .function("systemToDisplay", &UnitManager::SystemToDisplay)
        .function("systemFromDisplay", &UnitManager::SystemFromDisplay)
        .function("systemToSI", &UnitManager::SystemToSI)
        .function("systemFromSI", &UnitManager::SystemFromSI)
        .function("trsfMultiply", &UnitManager::TrsfMultiply)
        .function("trsfInvert", &UnitManager::TrsfInvert)
        .function("trsfGetMat4", &UnitManager::TrsfGetMat4)
        .function("trsfGetEulerAngles", &UnitManager::TrsfGetEulerAngles)
        .function("trsfSetEulerAngles", &UnitManager::TrsfSetEulerAngles)
        .function("trsfRotateAroundAxis", &UnitManager::TrsfRotateAroundAxis)
        .function("trsfTransformPnt", &UnitManager::TrsfTransformPnt)
        .function("trsfTransformDir", &UnitManager::TrsfTransformDir);
}
#endif

} // namespace

