#include "UnitManager.h"

#include "WasmViewer.h"

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/UnitManager.h>

#include <ViewDim/InteractiveDimensionOwner.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
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
        .function("systemFromSI", &UnitManager::SystemFromSI);
}
#endif

} // namespace

