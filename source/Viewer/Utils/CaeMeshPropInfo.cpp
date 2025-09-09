#include "CaeMeshPropInfo.h"

#include "EmscriptenHelper.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
// ================================================================
// Function : CaeMeshPropCompInfo
// ================================================================
CaeMeshPropCompInfo::CaeMeshPropCompInfo(const Handle(RWCae::MeshNVecProperty)& theProp, int theCompIndex)
{
    if (theCompIndex < 0 || theCompIndex >= theProp->NbRawComponents()) {
        return;
    }

    const Handle(RWCae::MeshScalarPropertyBase)& aComp = theProp->GetRawComponent(theCompIndex);
    if (!aComp->IsEnabled()) {
        return;
    }

    index        = theCompIndex;
    name         = aComp->Name().ToCString();
    description  = aComp->Description().ToCString();
    displayUnits = aComp->DisplayUnits().ToCString();
    aComp->ValueRange().GetBounds(valueRange.x(), valueRange.y());
}

// ================================================================
// Function : CaeMeshPropInfo
// ================================================================
CaeMeshPropInfo::CaeMeshPropInfo(const Handle(RWCae::MeshData)& theMeshData, const TCollection_AsciiString& thePropId)
{
    Handle(RWCae::MeshNVecProperty) aFoundProp;
    for (const Handle(RWCae::MeshNVecProperty)& aProp : theMeshData->Properties()) {
        if (aProp->Id() == thePropId) {
            timeSteps.push_back(aProp->TimeStep());
            if (aProp->IsLoaded() || aProp->CanBeLoaded()) {
                loadedTimeSteps.push_back(aProp->TimeStep());
            }
            aFoundProp = aProp;
        }
    }
    if (aFoundProp.IsNull()) {
        return;
    }

    propId      = thePropId.ToCString();
    name        = aFoundProp->Name().ToCString();
    description = aFoundProp->Description().ToCString();
    isElemental = aFoundProp->IsElemental();
    type        = aFoundProp->Type() == RWCae::MeshNVecPropertyType::ScalarSet ? (int)CaeMeshPropType::SCALAR_SET
                : aFoundProp->Type() == RWCae::MeshNVecPropertyType::Vector    ? (int)CaeMeshPropType::VECTOR
                                                                                 : (int)CaeMeshPropType::TENSOR;
    elemType    = aFoundProp->HasScalarComponents()  ? (int)CaeMeshPropElemType::SCALAR
                : aFoundProp->HasNodeVecComponents() ? (int)CaeMeshPropElemType::NODEVEC
                                                     : (int)CaeMeshPropElemType::GROUP;

    for (int i = 0; i < aFoundProp->NbRawComponents(); i++) {
        CaeMeshPropCompInfo anInfo(aFoundProp, i);
        if (anInfo.index != -1) {
            comps.push_back(anInfo);
        }
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CaeMeshPropInfo)
{
    emscripten::register_vector<CaeMeshPropInfo>("CaeMeshPropInfoVector");
    emscripten::register_vector<CaeMeshPropCompInfo>("CaeMeshPropCompInfoVector");

    emscripten::value_object<CaeMeshPropCompInfo>("CaeMeshPropCompInfo")
        .field("index", &CaeMeshPropCompInfo::index)
        .field("name", &CaeMeshPropCompInfo::name)
        .field("description", &CaeMeshPropCompInfo::description)
        .field("displayUnits", &CaeMeshPropCompInfo::displayUnits)
        .field("valueRange", &CaeMeshPropCompInfo::valueRange);

    emscripten::value_object<CaeMeshPropInfo>("CaeMeshPropInfo")
        .field("propId", &CaeMeshPropInfo::propId)
        .field("timeSteps",
               emscripten::optional_override([](const CaeMeshPropInfo& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSArray(theStruct.timeSteps);
               }),
               emscripten::optional_override([](CaeMeshPropInfo& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSArray(theStruct.timeSteps, theVal);
               }))
        .field("loadedTimeSteps",
               emscripten::optional_override([](const CaeMeshPropInfo& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSArray(theStruct.loadedTimeSteps);
               }),
               emscripten::optional_override([](CaeMeshPropInfo& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSArray(theStruct.loadedTimeSteps, theVal);
               }))
        .field("name", &CaeMeshPropInfo::name)
        .field("description", &CaeMeshPropInfo::description)
        .field("isElemental", &CaeMeshPropInfo::isElemental)
        .field("type", &CaeMeshPropInfo::type)
        .field("elemType", &CaeMeshPropInfo::elemType)
        .field("comps",
               emscripten::optional_override([](const CaeMeshPropInfo& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSArray(theStruct.comps);
               }),
               emscripten::optional_override([](CaeMeshPropInfo& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSArray(theStruct.comps, theVal);
               }));
}
#endif

} // namespace