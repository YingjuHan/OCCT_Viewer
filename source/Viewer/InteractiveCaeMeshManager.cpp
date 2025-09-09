#include "InteractiveCaeMeshManager.h"

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/InteractiveColorScaleOwner.h>
#include <ViewCae/MeshColorScaleMapper.h>

#include <RWBase/EmulatedFileSystem.h>

#include <RWCae/MeshBinWriter.h>
#include <RWCae/MeshGroupProperty.h>

#include <RWMeshConv/MeshLoader.h>
#include <RWMed/RWMedReader.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include "Utils/EmscriptenHelper.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

#ifdef __EMSCRIPTEN__
// clang-format off
//! Callback for OnColorMapperChanged().
EM_JS(bool, jsOnColorMapperChanged, (const char* theObjId,
                                     const char* thePropId,
                                     int theCompInd,
                                     const char* theCScaleId),
{
    return Module.eventManager.onColorMapperChanged(UTF8ToString(theObjId),
                                                    UTF8ToString(thePropId),
                                                    theCompInd,
                                                    UTF8ToString(theCScaleId));
});
//! Callback for mesh animation change.
EM_JS(void, jsOnMeshAnimationChanged, (const char* theObjId,
                                       int theActiveTimeStep,
                                       bool theIsChanged,
                                       bool theIsRunning),
{
    Module.eventManager.onMeshAnimationChanged(UTF8ToString(theObjId),
                                               theActiveTimeStep,
                                               theIsChanged,
                                               theIsRunning);
});
//! Callback for mesh properties request.
EM_JS(void, jsOnMeshPropertiesRequested, (const char* theObjId,
                                       int theTimeStep,
                                       const char* thePropsToLoad),
{
    Module.eventManager.onMeshPropertiesRequested(UTF8ToString(theObjId),
                                                  theTimeStep,
                                                  UTF8ToString(thePropsToLoad));
});
//! Callback for mesh geometry request.
EM_JS(void, jsOnMeshGenGeomRequested, (const char* theObjId,
                                       const char* thePrsId,
                                       int thePartId,
                                       const char* theParameters),
{
    Module.eventManager.onMeshGenGeomRequested(UTF8ToString(theObjId),
                                               UTF8ToString(thePrsId),
                                               thePartId,
                                               UTF8ToString(theParameters));
});
// clang-format on
#endif

using MeshNVecProperty = RWCae::MeshNVecProperty;

// ================================================================
// Function : InteractiveCaeMeshManager
// ================================================================
InteractiveCaeMeshManager::InteractiveCaeMeshManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : createPresentation
// ================================================================
std::string InteractiveCaeMeshManager::createPresentation(const ObjectId&    theMeshId,
                                                          int                theType,
                                                          const std::string& thePrsId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theMeshId)) {
        Message::SendFail() << "Error: createPresentation (" << theMeshId << ") called for non-existing mesh object";
        return "";
    }

    return aMesh->CreatePresentation((ViewCae::MeshPresentationType)theType, thePrsId.c_str()).ToCString();
}

// ================================================================
// Function : removePresentation
// ================================================================
void InteractiveCaeMeshManager::removePresentation(const ObjectId& theMeshId, const std::string& thePrsId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theMeshId)) {
        Message::SendFail() << "Error: removePresentation (" << theMeshId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: removePresentation (" << theMeshId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->RemovePresentation(thePrsId.c_str());
}

// ================================================================
// Function : getPresentations
// ================================================================
std::vector<std::string> InteractiveCaeMeshManager::getPresentations(const ObjectId& theMeshId)
{
    std::vector<std::string>             aRes;
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theMeshId)) {
        Message::SendFail() << "Error: getPresentations (" << theMeshId << ") called for non-existing mesh object";
        return aRes;
    }

    for (const Handle(ViewCae::MeshPresentationBase)& aPrs : aMesh->Presentations()) {
        aRes.push_back(aPrs->Id().ToCString());
    }

    return aRes;
}

// ================================================================
// Function : getMeshPresentationType
// ================================================================
int InteractiveCaeMeshManager::getMeshPresentationType(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPresentationType (" << theObjId
                            << ") called for non-existing mesh object";
        return 0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshPresentationType (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 0;
    }

    return (int)aPrs->Type();
}

// ================================================================
// Function : getMeshPresentationMode
// ================================================================
int InteractiveCaeMeshManager::getMeshPresentationMode(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPresentationMode (" << theObjId
                            << ") called for non-existing mesh object";
        return 0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshPresentationMode (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 0;
    }

    return (int)aMesh->PresentationMode(thePrsId.c_str());
}

// ================================================================
// Function : setMeshPresentationMode
// ================================================================
void InteractiveCaeMeshManager::setMeshPresentationMode(const ObjectId&    theObjId,
                                                        const std::string& thePrsId,
                                                        int                theMode)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshPresentationMode (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    if (theMode < 0 || theMode >= ViewCae::InteractiveCaeMeshNbPrsModes) {
        Message::SendFail() << "Error: setMeshPresentationMode (" << theObjId << ", " << theMode
                            << ") called with unknown presentation mode";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshPresentationMode (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetPresentationMode(thePrsId.c_str(), (ViewCae::InteractiveCaeMeshPrsMode)theMode);
}

// ================================================================
// Function : getMeshProperties
// ================================================================
std::vector<CaeMeshPropInfo> InteractiveCaeMeshManager::getMeshProperties(const ObjectId& theObjId) const
{
    std::vector<CaeMeshPropInfo>         aRes;
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshProperties (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    const Handle(RWCae::MeshData)& aMeshData = aMesh->GetMeshData();

    NCollection_Map<TCollection_AsciiString> anAddedProps;
    for (const Handle(MeshNVecProperty)& aProp : aMeshData->Properties()) {
        if (anAddedProps.Add(aProp->Id())) {
            aRes.push_back(CaeMeshPropInfo(aMeshData, aProp->Id()));
        }
    }

    return aRes;
}

// ================================================================
// Function : getMeshProperty
// ================================================================
CaeMeshPropInfo InteractiveCaeMeshManager::getMeshProperty(const ObjectId&    theObjId,
                                                           const std::string& thePropertyId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshProperty (" << theObjId << ") called for non-existing mesh object";
        return CaeMeshPropInfo();
    }

    return CaeMeshPropInfo(aMesh->GetMeshData(), thePropertyId.c_str());
}

// ================================================================
// Function : getMeshPropertyComponent
// ================================================================
CaeMeshPropCompInfo InteractiveCaeMeshManager::getMeshPropertyComponent(const ObjectId&    theObjId,
                                                                        const std::string& thePropertyId,
                                                                        int                theComponentIndex) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPropertyComponent (" << theObjId
                            << ") called for non-existing mesh object";
        return CaeMeshPropCompInfo();
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetMeshData()->GetPropertyById(thePropertyId.c_str());

    if (aProperty.IsNull()) {
        Message::SendFail() << "Error: getMeshPropertyComponent (" << theObjId << ") called for non-existing property "
                            << thePropertyId;
        return CaeMeshPropCompInfo();
    }

    return CaeMeshPropCompInfo(aProperty, theComponentIndex);
}

// ================================================================
// Function : getActiveMeshProperty
// ================================================================
std::string InteractiveCaeMeshManager::getActiveMeshProperty(const ObjectId&    theObjId,
                                                             const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getActiveMeshProperty (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getActiveMeshProperty (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return "";
    }

    RWCae::MeshPropertyComponent aProp = aMesh->Property(thePrsId.c_str());
    return !aProp.Property.IsNull() ? aProp.Property->Id().ToCString() : "";
}

// ================================================================
// Function : setActiveMeshProperty
// ================================================================
void InteractiveCaeMeshManager::setActiveMeshProperty(const ObjectId&    theObjId,
                                                      const std::string& thePrsId,
                                                      const std::string& thePropId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setActiveMeshProperty (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    const Handle(MeshNVecProperty)& aProp = thePropId != "" ? aMesh->GetPropertyByActiveTimeStep(thePropId.c_str())
                                                            : nullptr;

    if (thePropId != "" && aProp.IsNull()) {
        Message::SendFail() << "Error: setActiveMeshProperty (" << theObjId << ") called for non-existing property "
                            << thePropId;
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setActiveMeshProperty (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetProperty(thePrsId.c_str(), aProp);
}

// ================================================================
// Function : getActiveMeshComponentIndex
// ================================================================
int InteractiveCaeMeshManager::getActiveMeshComponentIndex(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getActiveMeshComponentIndex (" << theObjId
                            << ") called for non-existing mesh object";
        return -1;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getActiveMeshComponentIndex (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return -1;
    }

    return aMesh->Property(thePrsId.c_str()).ComponentIndex;
}

// ================================================================
// Function : setActiveMeshComponentIndex
// ================================================================
void InteractiveCaeMeshManager::setActiveMeshComponentIndex(const ObjectId&    theObjId,
                                                            const std::string& thePrsId,
                                                            int                theIndex)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setActiveMeshComponentIndex (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->Property(thePrsId.c_str()).Property;

    if (aProperty.IsNull() || !aProperty->EnabledComponents().Contains(theIndex)) {
        Message::SendFail() << "Error: setActiveMeshComponentIndex (" << theObjId
                            << ") called for non-existing component " << theIndex;
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setActiveMeshComponentIndex (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetPropertyComponentIndex(thePrsId.c_str(), theIndex);
}

// ================================================================
// Function : getMeshElemTypes
// ================================================================
int InteractiveCaeMeshManager::getMeshElemTypes(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshElemTypes (" << theObjId << ") called for non-existing mesh object";
        return 0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshElemTypes (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 0;
    }

    return aMesh->ElementTypes(thePrsId.c_str());
}

// ================================================================
// Function : setMeshElemTypes
// ================================================================
void InteractiveCaeMeshManager::setMeshElemTypes(const ObjectId&    theObjId,
                                                 const std::string& thePrsId,
                                                 int                theElemTypes)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshElemTypes (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshElemTypes (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetElementTypes(thePrsId.c_str(), theElemTypes);
}

// ================================================================
// Function : getMeshBox
// ================================================================
BndBox3 InteractiveCaeMeshManager::getMeshBox(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshBox (" << theObjId << ") called for non-existing mesh object";
        return BndBox3();
    }

    return BndBox3(aMesh->GetMeshData()->BoundingBox());
}

// ================================================================
// Function : getMeshPropertyIdByName
// ================================================================
std::string InteractiveCaeMeshManager::getMeshPropertyIdByName(const ObjectId&    theObjId,
                                                               const std::string& thePropertyName) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPropertyIdByName (" << theObjId
                            << ") called for non-existing mesh object";
        return "";
    }

    NCollection_List<TCollection_AsciiString> anIds = aMesh->GetMeshData()->GetPropertyIdsByName(
        thePropertyName.c_str());
    if (anIds.IsEmpty()) {
        Message::SendFail() << "Error: cannot find id for property with name '" << thePropertyName << "' for mesh '"
                            << theObjId << "'";
        return "";
    }
    if (anIds.Size() > 1) {
        Message::SendWarning() << "Warning: several properties with name '" << thePropertyName << "' for mesh '"
                               << theObjId << "' was found; using the first one";
    }

    return anIds.First().ToCString();
}

// ================================================================
// Function : getAvailableMeshVec3Props
// ================================================================
std::vector<std::string> InteractiveCaeMeshManager::getAvailableMeshVec3Props(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getAvailableMeshVec3Props (" << theObjId
                            << ") called for non-existing mesh object";
        return std::vector<std::string>();
    }

    std::vector<TCollection_AsciiString> aProps = aMesh->GetAvailableVec3Properties();

    NCollection_Map<TCollection_AsciiString> anAddedProps;

    std::vector<std::string> aRes;
    for (TCollection_AsciiString& aProp : aProps) {
        if (anAddedProps.Add(aProp)) {
            aRes.push_back(aProp.ToCString());
        }
    }

    return aRes;
}

// ================================================================
// Function : getMeshDeformedProp
// ================================================================
std::string InteractiveCaeMeshManager::getMeshDeformedProp(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshDeformedProp (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshDeformedProp (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return "";
    }

    Handle(MeshNVecProperty) aProp = aMesh->DeformedProperty(thePrsId.c_str());

    return !aProp.IsNull() ? aProp->Id().ToCString() : "";
}

// ================================================================
// Function : setMeshDeformedProp
// ================================================================
void InteractiveCaeMeshManager::setMeshDeformedProp(const ObjectId&    theObjId,
                                                    const std::string& thePrsId,
                                                    const std::string& thePropertyId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshDeformedProp (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetPropertyByActiveTimeStep(thePropertyId.c_str());
    if (aProperty.IsNull() && !thePropertyId.empty()) {
        Message::SendFail() << "Error: setMeshDeformedProp (" << theObjId << ") called for non-existing property "
                            << thePropertyId;
        return;
    }

    if ((!aProperty->IsVector3() || !aProperty->IsNodal()) && !thePropertyId.empty()) {
        Message::SendFail() << "Error: setMeshDeformedProp (" << theObjId << ") called for non-deformed property"
                            << thePropertyId;
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshDeformedProp (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetDeformedProperty(thePrsId.c_str(), aProperty);
}

// ================================================================
// Function : getMeshDeformedScaleCoeff
// ================================================================
double InteractiveCaeMeshManager::getMeshDeformedScaleCoeff(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshDeformedScaleCoeff (" << theObjId
                            << ") called for non-existing mesh object";
        return 0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshDeformedScaleCoeff (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 0;
    }

    return aMesh->DeformedScaleCoeff(thePrsId.c_str());
}

// ================================================================
// Function : setMeshDeformedScaleCoeff
// ================================================================
void InteractiveCaeMeshManager::setMeshDeformedScaleCoeff(const ObjectId&    theObjId,
                                                          const std::string& thePrsId,
                                                          double             theDefScaleCoeff)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshDeformedScaleCoeff (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshDeformedScaleCoeff (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetDeformedScaleCoeff(thePrsId.c_str(), theDefScaleCoeff);
}

// ================================================================
// Function : getMeshVecProp
// ================================================================
std::string InteractiveCaeMeshManager::getMeshVecProp(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshVecProp (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshVecProp (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return "";
    }

    Handle(MeshNVecProperty) aProp = aMesh->VectorProperty(thePrsId.c_str());

    return !aProp.IsNull() ? aProp->Id().ToCString() : "";
}

// ================================================================
// Function : setMeshVecProp
// ================================================================
void InteractiveCaeMeshManager::setMeshVecProp(const ObjectId&    theObjId,
                                               const std::string& thePrsId,
                                               const std::string& thePropertyId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshVecProp (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshVecProp (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    if (thePropertyId.empty()) {
        aMesh->SetVectorProperty(thePrsId.c_str(), nullptr);
        return;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetPropertyByActiveTimeStep(thePropertyId.c_str());
    if (aProperty.IsNull()) {
        Message::SendFail() << "Error: setMeshVecProp (" << theObjId << ") called for non-existing property "
                            << thePropertyId;
        return;
    }

    if (!aProperty->IsVector3() || !aProperty->HasScalarComponents()) {
        Message::SendFail() << "Error: setMeshVecProp (" << theObjId << ") called for non-vector property"
                            << thePropertyId;
        return;
    }
    aMesh->SetVectorProperty(thePrsId.c_str(), aProperty);
}

// ================================================================
// Function : getMeshVecPropScaleCoeff
// ================================================================
double InteractiveCaeMeshManager::getMeshVecPropScaleCoeff(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshVecPropScaleCoeff (" << theObjId
                            << ") called for non-existing mesh object";
        return 1;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshVecPropScaleCoeff (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 1;
    }

    return aMesh->VectorScaleCoeff(thePrsId.c_str());
}

// ================================================================
// Function : setMeshVecPropScaleCoeff
// ================================================================
void InteractiveCaeMeshManager::setMeshVecPropScaleCoeff(const ObjectId&    theObjId,
                                                         const std::string& thePrsId,
                                                         double             theScaleCoeff)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshVecPropScaleCoeff (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshVecPropScaleCoeff (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetVectorScaleCoeff(thePrsId.c_str(), theScaleCoeff);
}

// ================================================================
// Function : getMeshVecPropDistrType
// ================================================================
int InteractiveCaeMeshManager::getMeshVecPropDistrType(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshVecPropDistrType (" << theObjId
                            << ") called for non-existing mesh object";
        return (int)ViewCae::MeshVectorDistrType::Volume;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshVecPropDistrType (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return (int)ViewCae::MeshVectorDistrType::Volume;
    }

    return (int)aMesh->VectorDistrType(thePrsId.c_str());
}

// ================================================================
// Function : setMeshVecPropDistrType
// ================================================================
void InteractiveCaeMeshManager::setMeshVecPropDistrType(const ObjectId&    theObjId,
                                                        const std::string& thePrsId,
                                                        int                theType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshVecPropScaleCoeff (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshVecPropDistrType (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetVectorDistrType(thePrsId.c_str(), (ViewCae::MeshVectorDistrType)theType);
}

// ================================================================
// Function : getMeshVecSamplingRatio
// ================================================================
double InteractiveCaeMeshManager::getMeshVecSamplingRatio(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshVecSamplingRatio (" << theObjId
                            << ") called for non-existing mesh object";
        return 1.0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshVecSamplingRatio (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 1.0;
    }

    return aMesh->VectorSamplingRatio(thePrsId.c_str());
}

// ================================================================
// Function : setMeshVecSamplingRatio
// ================================================================
void InteractiveCaeMeshManager::setMeshVecSamplingRatio(const ObjectId&    theObjId,
                                                        const std::string& thePrsId,
                                                        double             theRatio)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshVecSamplingRatio (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshVecSamplingRatio (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetVectorSamplingRatio(thePrsId.c_str(), theRatio);
}

// ================================================================
// Function : getMeshUsedGroups
// ================================================================
std::map<int, std::string> InteractiveCaeMeshManager::getMeshUsedGroups(const ObjectId&    theObjId,
                                                                        const std::string& thePropertyId) const
{
    std::map<int, std::string> aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshUsedGroups (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetMeshData()->GetPropertyById(thePropertyId.c_str());
    if (aProperty.IsNull()) {
        Message::SendFail() << "Error: getMeshUsedGroups (" << theObjId << ") called for non-existing property "
                            << thePropertyId;
        return aRes;
    }

    Handle(RWCae::MeshGroupProperty) aGroup = Handle(RWCae::MeshGroupProperty)::DownCast(
        aProperty->GetRawComponent(0));
    if (aGroup.IsNull()) {
        Message::SendFail() << "Error: getMeshUsedGroups (" << theObjId << ") called for non-group property "
                            << thePropertyId;
        return aRes;
    }

    aRes = aGroup->GetUsedGroups();
    return aRes;
}

// ================================================================
// Function : isMeshVolumeAsSurface
// ================================================================
bool InteractiveCaeMeshManager::isMeshVolumeAsSurface(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: isMeshVolumeAsSurface (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: isMeshVolumeAsSurface (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return false;
    }

    return aMesh->IsVolumeAsSurface(thePrsId.c_str());
}

// ================================================================
// Function : setMeshVolumeAsSurface
// ================================================================
void InteractiveCaeMeshManager::setMeshVolumeAsSurface(const ObjectId&    theObjId,
                                                       const std::string& thePrsId,
                                                       bool               theIsVolumeAsSurface)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshVolumeAsSurface (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshVolumeAsSurface (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetVolumeAsSurface(thePrsId.c_str(), theIsVolumeAsSurface);
}

// ================================================================
// Function : isMeshPrsVisible
// ================================================================
bool InteractiveCaeMeshManager::isMeshPrsVisible(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: isMeshPrsVisible (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: isMeshPrsVisible (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return false;
    }

    return aMesh->IsPrsVisible(thePrsId.c_str());
}

// ================================================================
// Function : setMeshPrsVisible
// ================================================================
void InteractiveCaeMeshManager::setMeshPrsVisible(const ObjectId&    theObjId,
                                                  const std::string& thePrsId,
                                                  bool               theIsVisible)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshPrsVisible (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshPrsVisible (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetPrsVisible(thePrsId.c_str(), theIsVisible);
}

// ================================================================
// Function : getMeshShadingColor
// ================================================================
ColorTools::ColorString InteractiveCaeMeshManager::getMeshShadingColor(const ObjectId&    theObjId,
                                                                       const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshShadingColor (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshShadingColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return "";
    }
    Quantity_ColorRGBA aColor(aMesh->ShadingColor(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType));
    return Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
}

// ================================================================
// Function : setMeshShadingColor
// ================================================================
void InteractiveCaeMeshManager::setMeshShadingColor(const ObjectId&                theObjId,
                                                    const std::string&             thePrsId,
                                                    const ColorTools::ColorString& theColor, int theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshShadingColor (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Quantity_ColorRGBA aColor;
    if (!ColorTools::ParseHexColor(theColor, aColor)) {
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshShadingColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetShadingColor(thePrsId.c_str(), aColor.GetRGB(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : getMeshEdgesColor
// ================================================================
ColorTools::ColorString InteractiveCaeMeshManager::getMeshEdgesColor(const ObjectId&    theObjId,
                                                                     const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshEdgesColor (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshEdgesColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return "";
    }

    Quantity_ColorRGBA aColor(aMesh->EdgesColor(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType));
    return Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
}

// ================================================================
// Function : setMeshEdgesColor
// ================================================================
void InteractiveCaeMeshManager::setMeshEdgesColor(const ObjectId&                theObjId,
                                                  const std::string&             thePrsId,
                                                  const ColorTools::ColorString& theColor, int theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshEdgesColor (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Quantity_ColorRGBA aColor;
    if (!ColorTools::ParseHexColor(theColor, aColor)) {
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshEdgesColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetEdgesColor(thePrsId.c_str(), aColor.GetRGB(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : getMeshPointsColor
// ================================================================
ColorTools::ColorString InteractiveCaeMeshManager::getMeshPointsColor(const ObjectId&    theObjId,
                                                                      const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPointsColor (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshPointsColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return "";
    }
    Quantity_ColorRGBA aColor = Quantity_ColorRGBA(aMesh->PointsColor(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType));
    return Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
}

// ================================================================
// Function : setMeshPointsColor
// ================================================================
void InteractiveCaeMeshManager::setMeshPointsColor(const ObjectId&                theObjId,
                                                   const std::string&             thePrsId,
                                                   const ColorTools::ColorString& theColor, int theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshPointsColor (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Quantity_ColorRGBA aColor;
    if (!ColorTools::ParseHexColor(theColor, aColor)) {
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshPointsColor (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetPointsColor(thePrsId.c_str(), aColor.GetRGB(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : getMeshEdgesWidth
// ================================================================
float InteractiveCaeMeshManager::getMeshEdgesWidth(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshEdgesWidth (" << theObjId << ") called for non-existing mesh object";
        return 1;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshEdgesWidth (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 1;
    }

    return aMesh->EdgesWidth(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : setMeshEdgesWidth
// ================================================================
void InteractiveCaeMeshManager::setMeshEdgesWidth(const ObjectId& theObjId, const std::string& thePrsId, float theWidth, int theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshEdgesWidth (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshEdgesWidth (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetEdgesWidth(thePrsId.c_str(), theWidth, (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : getMeshPointsSize
// ================================================================
float InteractiveCaeMeshManager::getMeshPointsSize(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshPointsSize (" << theObjId << ") called for non-existing mesh object";
        return 1;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshPointsSize (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 1;
    }

    return aMesh->PointsSize(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : setMeshPointsSize
// ================================================================
void InteractiveCaeMeshManager::setMeshPointsSize(const ObjectId& theObjId, const std::string& thePrsId, float theSize, int theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshPointsSize (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshPointsSize (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetPointsSize(thePrsId.c_str(), theSize, (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : getMeshShadingTransparency
// ================================================================
float InteractiveCaeMeshManager::getMeshShadingTransparency(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshShadingTransparency (" << theObjId
                            << ") called for non-existing mesh object";
        return 0.0;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshShadingTransparency (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return 0.0;
    }

    return aMesh->ShadingTransparency(thePrsId.c_str(), (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : setMeshShadingTransparency
// ================================================================
void InteractiveCaeMeshManager::setMeshShadingTransparency(const ObjectId&    theObjId,
                                                           const std::string& thePrsId,
                                                           float              theValue,
                                                           int                theHiType)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshShadingTransparency (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshShadingTransparency (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetShadingTransparency(thePrsId.c_str(), theValue, (ViewCae::MeshHighlightingType)theHiType);
}

// ================================================================
// Function : LoadMeshProperties
// ================================================================
bool InteractiveCaeMeshManager::LoadMeshProperties(const Handle(AIS_InteractiveContext)& theCtx,
                                                   const ObjectId&                       theObjId,
                                                   const std::string&                    theFilePath,
                                                   const int                             theOptions)
{
    Handle(ViewCae::InteractiveCaeMeshOwner) aMesh;
    if (!ViewBase::InteractiveOwner::FindPrsOwnerById(aMesh, *theCtx, theObjId.c_str())) {
        Message::SendFail() << "Error: LoadMeshProperties (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(theFilePath.c_str());
    if (aFileSystem.IsNull()) {
        return false;
    }

    NCollection_Map<Handle(RWCae::MeshNVecProperty)> anUpdatedProps;

    int anOptions = theOptions;
    if (aMesh->ToCacheProperties()) {
        anOptions |= (int)RWCae::MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE;
    }
    RWMeshConv::MeshLoader aLoader(anOptions);
    if (!aLoader.LoadMeshProperties(aMesh->GetMeshData(), aFileSystem, theFilePath.c_str(), anUpdatedProps)) {
        Message::SendFail() << "Error: Cannot load mesh properties from '" << theFilePath << "'";
        return false;
    }

    aMesh->OnPropertiesUpdated(anUpdatedProps);

    return true;
}

// ================================================================
// Function : loadMeshProperties
// ================================================================
bool InteractiveCaeMeshManager::loadMeshProperties(const ObjectId& theObjId, const std::string& theFilePath)
{
    int anOptions = (int)RWCae::MeshDataImportExportOptions::PARALLEL_PROCESSING;
    return LoadMeshProperties(myWebViewer->GetContext(), theObjId, theFilePath, anOptions);
}

// ================================================================
// Function : removeMeshProperties
// ================================================================
void InteractiveCaeMeshManager::removeMeshProperties(const ObjectId& theObjId, bool theWithMetadata)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: removeMeshProperties (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    aMesh->RemoveProperties(theWithMetadata);
}

// ================================================================
// Function : probeFileFormat
// ================================================================
std::string InteractiveCaeMeshManager::probeFileFormat(const std::string& theName,
                                                       uintptr_t          theBuffer,
                                                       int                theDataLen,
                                                       bool               theToFree)
{
    std::string aFormat = RWMeshConv::MeshLoader::ProbeFileFormat(theName, (Standard_CString)theBuffer, theDataLen);
    if (theToFree) {
        free((void*)theBuffer);
    }
    return aFormat;
}

// ================================================================
// Function : openModelMesh
// ================================================================
bool InteractiveCaeMeshManager::openModelMesh(const ObjectId&    theObjId,
                                              const std::string& theFilePath,
                                              bool               theToCacheProps)
{
    int anOptions = (int)RWCae::MeshDataImportExportOptions::DEFAULTS;
    if (theToCacheProps) {
        anOptions = (int)RWCae::MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE
                  | (int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA;
    }

    anOptions |= (int)RWCae::MeshDataImportExportOptions::PARALLEL_PROCESSING;

    bool aRes = OpenMesh(myWebViewer->GetContext(), theObjId, theFilePath, "", "", anOptions);
    if (aRes) {
        Handle(ViewCae::InteractiveCaeMeshOwner) aMesh = Handle(ViewCae::InteractiveCaeMeshOwner)::DownCast(
            FindPrsOwner(theObjId));
        aMesh->SetCacheProperties(theToCacheProps);
    }

    return aRes;
}

// ================================================================
// Function : OpenMesh
// ================================================================
bool InteractiveCaeMeshManager::OpenMesh(const Handle(AIS_InteractiveContext)& theCtx,
                                         const ObjectId&                       theObjId,
                                         const std::string&                    theFilePath,
                                         const std::string&                    theMappingFilePath,
                                         const std::string&                    theArtifactFilePath,
                                         const int                             theOptions)
{
    InteractiveContext::RemoveObject(theCtx, theObjId);

    const TCollection_AsciiString aFileName(theFilePath.c_str());

    Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(aFileName);
    if (aFileSystem.IsNull()) {
        return false;
    }

    RWMeshConv::MeshLoader  aLoader(theOptions);
    Handle(RWCae::MeshData) aMeshData =
        aLoader.OpenMesh(aFileSystem, aFileName, theMappingFilePath.c_str(), theArtifactFilePath.c_str());
    if (aMeshData.IsNull()) {
        return false;
    }

    aMeshData->SetId(theObjId.c_str());

    Handle(ViewCae::InteractiveCaeMeshOwner) anOwner  = new ViewCae::InteractiveCaeMeshOwner(aMeshData);
    Handle(ViewCae::InteractiveCaeMesh)      aMeshPrs = new ViewCae::InteractiveCaeMesh(anOwner);

#ifdef __EMSCRIPTEN__
    const TCollection_AsciiString anId = aMeshPrs->MeshOwner()->Id();
    aMeshPrs->SetOnColorMapperChangeCallback(
        [anId](const RWCae::MeshPropertyComponent& theProp, const Handle(ViewCae::MeshColorMapper)& theMapper) {
            OnColorMapperChanged(anId, theProp, theMapper);
        });

    aMeshPrs->Animation()->SetOnAnimationUpdateCallback(
        [anId](int theActiveTimeStep, bool theIsChanged, bool theIsRunning) {
            jsOnMeshAnimationChanged(anId.ToCString(), theActiveTimeStep, theIsChanged, theIsRunning);
        });

    aMeshPrs->SetOnPropertiesRequestedCallback([anId](int theTimeStep, const TCollection_AsciiString& thePropsToLoad) {
        jsOnMeshPropertiesRequested(anId.ToCString(), theTimeStep, thePropsToLoad.ToCString());
    });

    aMeshPrs->SetOnGenGeomRequestedCallback(
        [anId](const TCollection_AsciiString& thePrsId, int thePartId, const std::string& theParameters) {
            jsOnMeshGenGeomRequested(anId.ToCString(), thePrsId.ToCString(), thePartId, theParameters.c_str());
        });

#endif
    theCtx->Display(aMeshPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
    return true;
}

// ================================================================
// Function : saveModelMesh
// ================================================================
bool InteractiveCaeMeshManager::saveModelMesh(const ObjectId& theObjId, const std::string& theFile) const
{
    Handle(ViewCae::InteractiveCaeMeshOwner) aMesh;
    if (!FindPrsOwner(aMesh, theObjId)) {
        Message::SendFail() << "Error: saveModelMesh (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    RWCae::MeshBinWriter aWriter(aMesh->GetMeshData());
    if (!aWriter.Perform(theFile.c_str())) {
        return false;
    }
    return true;
}

// ================================================================
// Function : OnColorMapperChanged
// ================================================================
void InteractiveCaeMeshManager::OnColorMapperChanged(const TCollection_AsciiString&           theObjId,
                                                     const RWCae::MeshPropertyComponent&    theProp,
                                                     const Handle(ViewCae::MeshColorMapper)& theMapper)
{
#ifdef __EMSCRIPTEN__
    Handle(ViewCae::MeshColorScaleMapper) aMapper = Handle(ViewCae::MeshColorScaleMapper)::DownCast(theMapper);
    jsOnColorMapperChanged(theObjId.ToCString(),
                           !theProp.Property.IsNull() ? theProp.Property->Id().ToCString() : "",
                           !theProp.Property.IsNull() ? theProp.ComponentIndex : -1,
                           !aMapper.IsNull() && !aMapper->ColorScaleObject().IsNull()
                               ? aMapper->ColorScaleObject()->MeshOwner()->Id().ToCString()
                               : "");
#else
    (void)theObjId;
    (void)theProp;
    (void)theMapper;
#endif
}

// ================================================================
// Function : hasClipPlane
// ================================================================
bool InteractiveCaeMeshManager::hasClipPlane(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: hasClipPlane (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: hasClipPlane (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return false;
    }

    return !aMesh->ClipPlane(thePrsId.c_str()).IsNull();
}

// ================================================================
// Function : getClipPlane
// ================================================================
Vec4 InteractiveCaeMeshManager::getClipPlane(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getClipPlane (" << theObjId << ") called for non-existing mesh object";
        return Vec4();
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getClipPlane (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return Vec4();
    }

    const Handle(Graphic3d_ClipPlane)& aClipPlane = aMesh->ClipPlane(thePrsId.c_str());
    if (aClipPlane.IsNull()) {
        Message::SendFail() << "Error: mesh '" << theObjId << "' does not have a clip plane";
        return Vec4();
    }
    Graphic3d_Vec4d anEq = aClipPlane->GetEquation();
    Vec4            aRes(anEq.x(), anEq.y(), anEq.z(), anEq.w());
    return aRes;
}

// ================================================================
// Function : setClipPlane
// ================================================================
void InteractiveCaeMeshManager::setClipPlane(const ObjectId& theObjId, const std::string& thePrsId, const Vec4& theEq)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setClipPlane (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setClipPlane (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    Handle(Graphic3d_ClipPlane) aClipPlane = new Graphic3d_ClipPlane(
        Graphic3d_Vec4d(theEq.x(), theEq.y(), theEq.z(), theEq.w()));
    aMesh->SetClipPlane(thePrsId.c_str(), aClipPlane);
}

// ================================================================
// Function : unsetClipPlane
// ================================================================
void InteractiveCaeMeshManager::unsetClipPlane(const ObjectId& theObjId, const std::string& thePrsId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: unsetClipPlane (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: unsetClipPlane (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    aMesh->SetClipPlane(thePrsId.c_str(), nullptr);
}

// ================================================================
// Function : getSlicePlanes
// ================================================================
std::vector<Vec4> InteractiveCaeMeshManager::getSlicePlanes(const ObjectId& theObjId, const std::string& thePrsId) const
{
    std::vector<Vec4>                    aRes;
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getSlicePlanes (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getSlicePlanes (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return aRes;
    }

    Graphic3d_SequenceOfHClipPlane aSlicePlanes = aMesh->SlicePlanes(thePrsId.c_str());
    for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(aSlicePlanes); anIter.More(); anIter.Next()) {
        Graphic3d_Vec4d anEq = anIter.Value()->GetEquation();
        aRes.push_back(Vec4(anEq.x(), anEq.y(), anEq.z(), anEq.w()));
    }
    return aRes;
}

// ================================================================
// Function : setSlicePlanes
// ================================================================
void InteractiveCaeMeshManager::setSlicePlanes(const ObjectId&          theObjId,
                                               const std::string&       thePrsId,
                                               const std::vector<Vec4>& theEqs)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setSlicePlanes (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setSlicePlanes (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    Graphic3d_SequenceOfHClipPlane aSlicePlanes;
    for (const Vec4& anEq : theEqs) {
        Handle(Graphic3d_ClipPlane) aSlicePlane = new Graphic3d_ClipPlane(
            Graphic3d_Vec4d(anEq.x(), anEq.y(), anEq.z(), anEq.w()));
        aSlicePlanes.Append(aSlicePlane);
    }
    aMesh->SetSlicePlanes(thePrsId.c_str(), aSlicePlanes);
}

// ================================================================
// Function : getIsoPropId
// ================================================================
std::string InteractiveCaeMeshManager::getIsoPropId(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getIsoPropId (" << theObjId << ") called for non-existing mesh object";
        return "";
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getIsoPropId (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return "";
    }

    RWCae::MeshPropertyComponent aComp = aMesh->IsoProperty(thePrsId.c_str());
    return !aComp.Property.IsNull() ? aComp.Property->Id().ToCString() : "";
}

// ================================================================
// Function : getIsoPropComp
// ================================================================
int InteractiveCaeMeshManager::getIsoPropComp(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getIsoPropComp (" << theObjId << ") called for non-existing mesh object";
        return -1;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getIsoPropComp (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return -1;
    }

    RWCae::MeshPropertyComponent aComp = aMesh->IsoProperty(thePrsId.c_str());
    return !aComp.Property.IsNull() ? aComp.ComponentIndex : -1;
}

// ================================================================
// Function : setIsoProp
// ================================================================
void InteractiveCaeMeshManager::setIsoProp(const ObjectId&    theObjId,
                                           const std::string& thePrsId,
                                           const std::string& thePropertyId,
                                           int                theCompIndex)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setIsoProp (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setIsoProp (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetPropertyByActiveTimeStep(thePropertyId.c_str());
    if (aProperty.IsNull() && !thePropertyId.empty()) {
        Message::SendFail() << "Error: setIsoProp (" << theObjId << ") called for non-existing property "
                            << thePropertyId;
        return;
    }

    if ((aProperty.IsNull() || !aProperty->EnabledComponents().Contains(theCompIndex)) && !thePropertyId.empty()) {
        Message::SendFail() << "Error: setIsoProp (" << theObjId << ") called for non-existing component "
                            << theCompIndex;
        return;
    }

    aMesh->SetIsoProperty(thePrsId.c_str(), RWCae::MeshPropertyComponent(aProperty, theCompIndex));
}

// ================================================================
// Function : getIsoValues
// ================================================================
std::vector<double> InteractiveCaeMeshManager::getIsoValues(const ObjectId& theObjId, const std::string& thePrsId) const
{
    std::vector<double>                  aRes;
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getIsoValues (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getIsoValues (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return aRes;
    }

    aRes = aMesh->IsoValues(thePrsId.c_str());
    return aRes;
}

// ================================================================
// Function : setIsoValues
// ================================================================
void InteractiveCaeMeshManager::setIsoValues(const ObjectId&            theObjId,
                                             const std::string&         thePrsId,
                                             const std::vector<double>& theValues)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setIsoValues (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setIsoValues (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    aMesh->SetIsoValues(thePrsId.c_str(), theValues);
}

// ================================================================
// Function : toShowPrsMinMax
// ================================================================
bool InteractiveCaeMeshManager::toShowPrsMinMax(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: toShowPrsMinMax (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: toShowPrsMinMax (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return false;
    }

    return aMesh->ToShowPrsMinMax(thePrsId.c_str());
}

// ================================================================
// Function : setShowPrsMinMax
// ================================================================
void InteractiveCaeMeshManager::setShowPrsMinMax(const ObjectId& theObjId, const std::string& thePrsId, bool theToShow)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setShowPrsMinMax (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setShowPrsMinMax (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetShowPrsMinMax(thePrsId.c_str(), theToShow);
}

// ================================================================
// Function : toShowGroupsInfo
// ================================================================
bool InteractiveCaeMeshManager::toShowGroupsInfo(const ObjectId& theObjId, const std::string& thePrsId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: toShowGroupsInfo (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: toShowGroupsInfo (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return false;
    }

    return aMesh->ToShowGroupsInfo(thePrsId.c_str());
}

// ================================================================
// Function : setShowGroupsInfo
// ================================================================
void InteractiveCaeMeshManager::setShowGroupsInfo(const ObjectId& theObjId, const std::string& thePrsId, bool theToShow)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setShowGroupsInfo (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setShowGroupsInfo (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    aMesh->SetShowGroupsInfo(thePrsId.c_str(), theToShow);
}

// ================================================================
// Function : getGroupInfo
// ================================================================
CaeMeshAnnotationInfo InteractiveCaeMeshManager::getGroupInfo(const ObjectId&    theObjId,
                                                              const std::string& thePrsId,
                                                              int                theGroupId) const
{
    CaeMeshAnnotationInfo aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getGroupInfo (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getGroupInfo (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return aRes;
    }

    Handle(RWCae::MeshGroupProperty) aGroup = Handle(RWCae::MeshGroupProperty)::DownCast(
        aMesh->Property(thePrsId.c_str()).GetScalarComp());
    if (aGroup.IsNull()) {
        Message::SendFail() << "Error: getGroupInfo (" << theObjId << ") called for non-group property";
        return aRes;
    }

    const Handle(ViewCae::MeshPartGroupsInfoAnnotations)& aGroupsInfo =
        Handle(ViewCae::MeshPartGroupsInfoAnnotations)::DownCast(
            aPrs->ChangeParts()(ViewCae::MeshPartType::GroupsInfo));

    const ViewCae::MeshAnnotation* anAnnotation = aGroupsInfo->Annotations().Annotation(theGroupId);
    if (anAnnotation == nullptr) {
        Message::SendFail() << "Error: getGroupInfo (" << theObjId << ") called for empty group '" << thePrsId << "'";
        return aRes;
    }

    aRes.InitFromAnnotation(*anAnnotation);

    return aRes;
}

// ================================================================
// Function : setGroupInfo
// ================================================================
void InteractiveCaeMeshManager::setGroupInfo(const ObjectId&              theObjId,
                                             const std::string&           thePrsId,
                                             int                          theGroupId,
                                             const CaeMeshAnnotationInfo& theInfo)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setGroupInfo (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setGroupInfo (" << theObjId << ") called for non-existing mesh presentation '"
                            << thePrsId << "'";
        return;
    }

    Handle(RWCae::MeshGroupProperty) aGroup = Handle(RWCae::MeshGroupProperty)::DownCast(
        aMesh->Property(thePrsId.c_str()).GetScalarComp());
    if (aGroup.IsNull()) {
        Message::SendFail() << "Error: setGroupInfo (" << theObjId << ") called for non-group property";
        return;
    }

    const Handle(ViewCae::MeshPartGroupsInfoAnnotations)& aGroupsInfo =
        Handle(ViewCae::MeshPartGroupsInfoAnnotations)::DownCast(
            aPrs->ChangeParts()(ViewCae::MeshPartType::GroupsInfo));

    ViewCae::MeshAnnotation* anAnnotation = aGroupsInfo->Annotations().Annotation(theGroupId);
    if (anAnnotation == nullptr) {
        Message::SendFail() << "Error: setGroupInfo (" << theObjId << ") called for empty group '" << thePrsId << "'";
        return;
    }

    theInfo.UpdateAnnotation(*anAnnotation);
    aMesh->SetToUpdate();
    aMesh->RecomputeSelection();
}

// ================================================================
// Function : getMeshInfo
// ================================================================
CaeMeshInfo InteractiveCaeMeshManager::getMeshInfo(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshInfo (" << theObjId << ") called for non-existing mesh object";
        return CaeMeshInfo();
    }

    return CaeMeshInfo(aMesh->GetMeshData());
}

// ================================================================
// Function : getMeshBlocks
// ================================================================
std::string InteractiveCaeMeshManager::getMeshBlocks(const ObjectId& theObjId) const
{
    std::string aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshBlocks (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    aRes = aMesh->GetMeshData()->Blocks()->ToJson().ToCString();
    return aRes;
}

// ================================================================
// Function : getVisibleMeshBlocks
// ================================================================
std::string InteractiveCaeMeshManager::getVisibleMeshBlocks(const ObjectId& theObjId, const std::string& thePrsId) const
{
    std::string aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getVisibleMeshBlocks (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getVisibleMeshBlocks (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return aRes;
    }

    Handle(RWCae::MeshDataBlock) aBlocks = aMesh->VisibleBlocks(thePrsId.c_str());
    if (aBlocks.IsNull()) {
        return aRes;
    }

    aRes = aBlocks->ToJson().ToCString();

    return aRes;
}

// ================================================================
// Function : setVisibleMeshBlocks
// ================================================================
void InteractiveCaeMeshManager::setVisibleMeshBlocks(const ObjectId&    theObjId,
                                                     const std::string& thePrsId,
                                                     const std::string& theBlocks,
                                                     bool               theVisible)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setVisibleMeshBlocks (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(thePrsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setVisibleMeshBlocks (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    Handle(RWCae::MeshDataBlock) aBlocks = new RWCae::MeshDataBlock("");
    if (!aBlocks->FromJson(theBlocks.c_str())) {
        Message::SendFail() << "Error: setVisibleMeshBlocks (" << theObjId << ") called for invalid JSON '" << theBlocks
                            << "'";
        return;
    }

    aMesh->SetBlocksVisible(thePrsId.c_str(), aBlocks, theVisible);
}

// ================================================================
// Function : getMeshActiveTimeStep
// ================================================================
int InteractiveCaeMeshManager::getMeshActiveTimeStep(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshActiveTimeStep (" << theObjId << ") called for non-existing mesh object";
        return -1;
    }

    return aMesh->ActiveTimeStep();
}

// ================================================================
// Function : setMeshActiveTimeStep
// ================================================================
void InteractiveCaeMeshManager::setMeshActiveTimeStep(const ObjectId& theObjId, int theTimeStep)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshActiveTimeStep (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    std::vector<std::pair<int, double>> aTimeSteps = aMesh->MeshOwner()->AvailableTimeSteps();

    bool isValid = false;
    for (const std::pair<int, double>& aTimeStep : aTimeSteps) {
        if (theTimeStep == aTimeStep.first) {
            isValid = true;
            break;
        }
    }

    if (!isValid) {
        Message::SendFail() << "Error: setMeshActiveTimeStep (" << theObjId << ") called for invalid time step";
        return;
    }

    aMesh->SetActiveTimeStep(theTimeStep);
}

// ================================================================
// Function : getAvailableMeshTimeSteps
// ================================================================
std::vector<Vec2> InteractiveCaeMeshManager::getAvailableMeshTimeSteps(const ObjectId& theObjId) const
{
    std::vector<Vec2>                    aRes;
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getAvailableMeshTimeSteps (" << theObjId
                            << ") called for non-existing mesh object";
        return aRes;
    }

    std::vector<std::pair<int, double>> aTimeSteps = aMesh->MeshOwner()->AvailableTimeSteps();
    aRes.resize(aTimeSteps.size());
    for (int i = 0; i < (int)aTimeSteps.size(); i++) {
        const std::pair<int, double>& aTimeStep = aTimeSteps[i];
        aRes[i].SetValues((double)aTimeStep.first, aTimeStep.second);
    }

    return aRes;
}

// ================================================================
// Function : setAvailableMeshTimeSteps
// ================================================================
void InteractiveCaeMeshManager::setAvailableMeshTimeSteps(const ObjectId&          theObjId,
                                                          const std::vector<Vec2>& theTimeSteps)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setAvailableMeshTimeSteps (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    std::vector<std::pair<int, double>> aTimeSteps;
    aTimeSteps.resize(theTimeSteps.size());
    for (int i = 0; i < (int)theTimeSteps.size(); i++) {
        aTimeSteps[i].first  = (int)theTimeSteps[i].X();
        aTimeSteps[i].second = theTimeSteps[i].Y();
    }

    aMesh->MeshOwner()->SetOwnTimeSteps(aTimeSteps);
}

// ================================================================
// Function : startMeshAnimation
// ================================================================
void InteractiveCaeMeshManager::startMeshAnimation(const ObjectId& theObjId, bool theToRestart)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: startMeshAnimation (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->StartTimer(theToRestart ? -1.0 : 0.0, 1.0, false);
}

// ================================================================
// Function : stopMeshAnimation
// ================================================================
void InteractiveCaeMeshManager::stopMeshAnimation(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: stopMeshAnimation (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->Stop();
}

// ================================================================
// Function : continueMeshAnimation
// ================================================================
void InteractiveCaeMeshManager::continueMeshAnimation(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: continueMeshAnimation (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->Continue();
}

// ================================================================
// Function : isMeshAnimationActive
// ================================================================
bool InteractiveCaeMeshManager::isMeshAnimationActive(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: isMeshAnimationActive (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    return !aMesh->Animation()->IsStopped();
}

// ================================================================
// Function : getMeshAnimationDuration
// ================================================================
double InteractiveCaeMeshManager::getMeshAnimationDuration(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshAnimationDuration (" << theObjId
                            << ") called for non-existing mesh object";
        return 0.0;
    }

    return aMesh->Animation()->OwnDuration();
}

// ================================================================
// Function : setMeshAnimationDuration
// ================================================================
void InteractiveCaeMeshManager::setMeshAnimationDuration(const ObjectId& theObjId, double theDur)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshAnimationDuration (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->SetDuration(theDur);
}

// ================================================================
// Function : getMeshAnimationLoopPlayback
// ================================================================
bool InteractiveCaeMeshManager::getMeshAnimationLoopPlayback(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshAnimationLoopPlayback (" << theObjId
                            << ") called for non-existing mesh object";
        return false;
    }

    return aMesh->Animation()->IsLoopPlayback();
}

// ================================================================
// Function : setMeshAnimationLoopPlayback
// ================================================================
void InteractiveCaeMeshManager::setMeshAnimationLoopPlayback(const ObjectId& theObjId, bool theIsLoopPlayback)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshAnimationLoopPlayback (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->SetLoopPlayback(theIsLoopPlayback);
}

// ================================================================
// Function : getMeshAnimationUniformSteps
// ================================================================
bool InteractiveCaeMeshManager::getMeshAnimationUniformSteps(const ObjectId& theObjId) const
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshAnimationUniformSteps (" << theObjId
                            << ") called for non-existing mesh object";
        return false;
    }

    return aMesh->Animation()->IsUniformSteps();
}

// ================================================================
// Function : setMeshAnimationUniformSteps
// ================================================================
void InteractiveCaeMeshManager::setMeshAnimationUniformSteps(const ObjectId& theObjId, bool theIsUniform)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshAnimationUniformSteps (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    aMesh->Animation()->SetUniformSteps(theIsUniform);
}

// ================================================================
// Function : getMeshStreamlinesInfo
// ================================================================
CaeMeshStreamlinesInfo InteractiveCaeMeshManager::getMeshStreamlinesInfo(const ObjectId&    theObjId,
                                                                         const std::string& thePrsId) const
{
    CaeMeshStreamlinesInfo aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: getMeshStreamlinesInfo (" << theObjId << ") called for non-existing mesh object";
        return aRes;
    }

    TCollection_AsciiString aPrsId(thePrsId.c_str());

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(aPrsId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: getMeshStreamlinesInfo (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return aRes;
    }

    const Handle(RWCae::MeshNVecProperty)& aProp = aMesh->StreamlinesProperty(aPrsId);

    aRes.property      = !aProp.IsNull() ? aProp->Id().ToCString() : "";
    aRes.nbStreamlines = aMesh->NbStreamlines(aPrsId);

    ViewCae::MeshStreamlinesPlane aPlane = aMesh->StreamlinesPlane(aPrsId);
    aRes.planeOrigin.SetXYZ(aPlane.Origin.XYZ());
    aRes.planeSide.SetXYZ(aPlane.Side.XYZ());
    aRes.planeUp.SetXYZ(aPlane.Up.XYZ());

    aRes.stepCoeff   = aMesh->StreamlinesStepCoeff(aPrsId);
    aRes.lengthCoeff = aMesh->StreamlinesLengthCoeff(aPrsId);

    aRes.thickness = aMesh->StreamlinesThickness(aPrsId);

    return aRes;
}

// ================================================================
// Function : setMeshStreamlinesInfo
// ================================================================
void InteractiveCaeMeshManager::setMeshStreamlinesInfo(const ObjectId&               theObjId,
                                                       const std::string&            thePrsId,
                                                       const CaeMeshStreamlinesInfo& theInfo)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshStreamlinesInfo (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    TCollection_AsciiString aPrsId(thePrsId.c_str());

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(aPrsId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshStreamlinesInfo (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return;
    }

    const Handle(MeshNVecProperty)& aProperty = aMesh->GetPropertyByActiveTimeStep(theInfo.property.c_str());
    if (aProperty.IsNull() && !theInfo.property.empty()) {
        Message::SendFail() << "Error: setMeshStreamlinesInfo (" << theObjId << ") called for non-existing property "
                            << theInfo.property;
        return;
    }

    aMesh->SetStreamlinesProperty(aPrsId, !theInfo.property.empty() ? aProperty : nullptr);

    aMesh->SetNbStreamlines(aPrsId, theInfo.nbStreamlines);

    ViewCae::MeshStreamlinesPlane aPlane;
    aPlane.Origin.SetXYZ(theInfo.planeOrigin.XYZ());
    aPlane.Side.SetXYZ(theInfo.planeSide.XYZ());
    aPlane.Up.SetXYZ(theInfo.planeUp.XYZ());

    aMesh->SetStreamlinesPlane(aPrsId, aPlane);

    aMesh->SetStreamlinesStepCoeff(aPrsId, theInfo.stepCoeff);
    aMesh->SetStreamlinesLengthCoeff(aPrsId, theInfo.lengthCoeff);

    aMesh->SetStreamlinesThickness(aPrsId, theInfo.thickness);
}

// ================================================================
// Function : setUseMeshGenGeometry
// ================================================================
void InteractiveCaeMeshManager::setUseMeshGenGeometry(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setUseMeshGenGeometry (" << theObjId << ") called for non-existing mesh object";
        return;
    }

    aMesh->MeshOwner()->SetUseGenGeom(true);
}

// ================================================================
// Function : setUseMeshTestCacheGenGeometry
// ================================================================
void InteractiveCaeMeshManager::setUseMeshTestCacheGenGeometry(const ObjectId& theObjId)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setUseMeshTestCacheGenGeometry (" << theObjId
                            << ") called for non-existing mesh object";
        return;
    }

    aMesh->MeshOwner()->SetUseTestCachePrs(true);
}

// ================================================================
// Function : setMeshGenGeometry
// ================================================================
bool InteractiveCaeMeshManager::setMeshGenGeometry(const ObjectId&    theObjId,
                                                   const std::string& thePrsId,
                                                   int                thePartId,
                                                   const std::string& theFilePath)
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theObjId)) {
        Message::SendFail() << "Error: setMeshGenGeometry (" << theObjId << ") called for non-existing mesh object";
        return false;
    }

    TCollection_AsciiString aPrsId(thePrsId.c_str());

    Handle(ViewCae::MeshPresentation) aPrs = aMesh->Presentation(aPrsId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setMeshGenGeometry (" << theObjId
                            << ") called for non-existing mesh presentation '" << thePrsId << "'";
        return false;
    }

    const TCollection_AsciiString aFileName(theFilePath.c_str());

    Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(aFileName);
    if (aFileSystem.IsNull()) {
        return false;
    }

    int anOptions = (int)RWCae::MeshDataImportExportOptions::PARALLEL_PROCESSING;

    RWMeshConv::MeshLoader  aLoader(anOptions);
    Handle(RWCae::MeshData) aMeshData = aLoader.OpenMesh(aFileSystem, aFileName);
    if (aMeshData.IsNull()) {
        Message::SendWarning() << "Error: setMeshGenGeometry (" << theObjId << ") - cannot load mesh data";
    }

    aMesh->SetPartGenMeshData(aPrsId, thePartId, aMeshData);
    return true;
}

// Module exports
#ifdef __EMSCRIPTEN__
// Register geometry-related cae mesh functions
void registerMeshGeometryFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshDeformedProp", &InteractiveCaeMeshManager::getMeshDeformedProp)
        .function("setMeshDeformedProp", &InteractiveCaeMeshManager::setMeshDeformedProp)
        .function("getMeshDeformedScaleCoeff", &InteractiveCaeMeshManager::getMeshDeformedScaleCoeff)
        .function("setMeshDeformedScaleCoeff", &InteractiveCaeMeshManager::setMeshDeformedScaleCoeff)
        .function("hasClipPlane", &InteractiveCaeMeshManager::hasClipPlane)
        .function("getClipPlane", &InteractiveCaeMeshManager::getClipPlane)
        .function("setClipPlane", &InteractiveCaeMeshManager::setClipPlane)
        .function("unsetClipPlane", &InteractiveCaeMeshManager::unsetClipPlane)
        .function("getSlicePlanes", &InteractiveCaeMeshManager::getSlicePlanes)
        .function("setSlicePlanes", &InteractiveCaeMeshManager::setSlicePlanes)
        .function("getIsoPropId", &InteractiveCaeMeshManager::getIsoPropId)
        .function("getIsoPropComp", &InteractiveCaeMeshManager::getIsoPropComp)
        .function("setIsoProp", &InteractiveCaeMeshManager::setIsoProp)
        .function("getIsoValues", &InteractiveCaeMeshManager::getIsoValues)
        .function("setIsoValues", &InteractiveCaeMeshManager::setIsoValues)
        .function("getMeshBlocks", &InteractiveCaeMeshManager::getMeshBlocks)
        .function("getVisibleMeshBlocks", &InteractiveCaeMeshManager::getVisibleMeshBlocks)
        .function("setVisibleMeshBlocks", &InteractiveCaeMeshManager::setVisibleMeshBlocks);
}

// Register aspects-related cae mesh functions
void registerMeshAspectsFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("isMeshPrsVisible", &InteractiveCaeMeshManager::isMeshPrsVisible)
        .function("setMeshPrsVisible", &InteractiveCaeMeshManager::setMeshPrsVisible)
        .function("getMeshShadingColor", &InteractiveCaeMeshManager::getMeshShadingColor)
        .function("setMeshShadingColor", &InteractiveCaeMeshManager::setMeshShadingColor)
        .function("getMeshEdgesColor", &InteractiveCaeMeshManager::getMeshEdgesColor)
        .function("setMeshEdgesColor", &InteractiveCaeMeshManager::setMeshEdgesColor)
        .function("getMeshPointsColor", &InteractiveCaeMeshManager::getMeshPointsColor)
        .function("setMeshPointsColor", &InteractiveCaeMeshManager::setMeshPointsColor)
        .function("getMeshEdgesWidth", &InteractiveCaeMeshManager::getMeshEdgesWidth)
        .function("setMeshEdgesWidth", &InteractiveCaeMeshManager::setMeshEdgesWidth)
        .function("getMeshPointsSize", &InteractiveCaeMeshManager::getMeshPointsSize)
        .function("setMeshPointsSize", &InteractiveCaeMeshManager::setMeshPointsSize)
        .function("getMeshShadingTransparency", &InteractiveCaeMeshManager::getMeshShadingTransparency)
        .function("setMeshShadingTransparency", &InteractiveCaeMeshManager::setMeshShadingTransparency)
        .function("getMeshPresentationMode", &InteractiveCaeMeshManager::getMeshPresentationMode)
        .function("setMeshPresentationMode", &InteractiveCaeMeshManager::setMeshPresentationMode);
}

// Register display-related cae mesh functions
void registerMeshDisplayFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshPresentationMode", &InteractiveCaeMeshManager::getMeshPresentationMode)
        .function("setMeshPresentationMode", &InteractiveCaeMeshManager::setMeshPresentationMode)
        .function("getMeshElemTypes", &InteractiveCaeMeshManager::getMeshElemTypes)
        .function("setMeshElemTypes", &InteractiveCaeMeshManager::setMeshElemTypes)
        .function("toShowPrsMinMax", &InteractiveCaeMeshManager::toShowPrsMinMax)
        .function("setShowPrsMinMax", &InteractiveCaeMeshManager::setShowPrsMinMax);
}

// Register presentation-related cae mesh functions
void registerMeshPrsFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("createPresentation", &InteractiveCaeMeshManager::createPresentation)
        .function("removePresentation", &InteractiveCaeMeshManager::removePresentation)
        .function("getPresentations", &InteractiveCaeMeshManager::getPresentations)
        .function("getMeshPresentationType", &InteractiveCaeMeshManager::getMeshPresentationType)
        .function("isMeshVolumeAsSurface", &InteractiveCaeMeshManager::isMeshVolumeAsSurface)
        .function("setMeshVolumeAsSurface", &InteractiveCaeMeshManager::setMeshVolumeAsSurface)
        .function("getMeshStreamlinesInfo", &InteractiveCaeMeshManager::getMeshStreamlinesInfo)
        .function("setMeshStreamlinesInfo", &InteractiveCaeMeshManager::setMeshStreamlinesInfo);
}

// Register property-related cae mesh functions
void registerMeshPropsFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshProperties", &InteractiveCaeMeshManager::getMeshProperties)
        .function("getMeshProperty", &InteractiveCaeMeshManager::getMeshProperty)
        .function("getMeshPropertyComponent", &InteractiveCaeMeshManager::getMeshPropertyComponent)
        .function("loadMeshProperties", &InteractiveCaeMeshManager::loadMeshProperties)
        .function("removeMeshProperties", &InteractiveCaeMeshManager::removeMeshProperties)
        .function("getActiveMeshProperty", &InteractiveCaeMeshManager::getActiveMeshProperty)
        .function("setActiveMeshProperty", &InteractiveCaeMeshManager::setActiveMeshProperty)
        .function("getActiveMeshComponentIndex", &InteractiveCaeMeshManager::getActiveMeshComponentIndex)
        .function("setActiveMeshComponentIndex", &InteractiveCaeMeshManager::setActiveMeshComponentIndex)
        .function("getMeshPropertyIdByName", &InteractiveCaeMeshManager::getMeshPropertyIdByName)
        .function("getAvailableMeshVec3Props", &InteractiveCaeMeshManager::getAvailableMeshVec3Props);
}

// Register vector-related cae mesh functions
void registerMeshVecFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshVecProp", &InteractiveCaeMeshManager::getMeshVecProp)
        .function("setMeshVecProp", &InteractiveCaeMeshManager::setMeshVecProp)
        .function("getMeshVecPropScaleCoeff", &InteractiveCaeMeshManager::getMeshVecPropScaleCoeff)
        .function("setMeshVecPropScaleCoeff", &InteractiveCaeMeshManager::setMeshVecPropScaleCoeff)
        .function("getMeshVecPropDistrType", &InteractiveCaeMeshManager::getMeshVecPropDistrType)
        .function("setMeshVecPropDistrType", &InteractiveCaeMeshManager::setMeshVecPropDistrType)
        .function("getMeshVecSamplingRatio", &InteractiveCaeMeshManager::getMeshVecSamplingRatio)
        .function("setMeshVecSamplingRatio", &InteractiveCaeMeshManager::setMeshVecSamplingRatio);
}

// Register IO-related cae mesh functions
void registerMeshRWFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("probeFileFormat", &InteractiveCaeMeshManager::probeFileFormat)
        .function("openModelMesh", &InteractiveCaeMeshManager::openModelMesh)
        .function("saveModelMesh", &InteractiveCaeMeshManager::saveModelMesh)
        .function("getMeshBox", &InteractiveCaeMeshManager::getMeshBox)
        .function("getMeshInfo", &InteractiveCaeMeshManager::getMeshInfo)
        .function("setUseMeshGenGeometry", &InteractiveCaeMeshManager::setUseMeshGenGeometry)
        .function("setUseMeshTestCacheGenGeometry", &InteractiveCaeMeshManager::setUseMeshTestCacheGenGeometry)
        .function("setMeshGenGeometry", &InteractiveCaeMeshManager::setMeshGenGeometry);
}

// Register groups-related cae mesh functions
void registerMeshGroupsFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshUsedGroups", &InteractiveCaeMeshManager::getMeshUsedGroups)
        .function("toShowGroupsInfo", &InteractiveCaeMeshManager::toShowGroupsInfo)
        .function("setShowGroupsInfo", &InteractiveCaeMeshManager::setShowGroupsInfo)
        .function("getGroupInfo", &InteractiveCaeMeshManager::getGroupInfo)
        .function("setGroupInfo", &InteractiveCaeMeshManager::setGroupInfo);
}

// Register animation-related cae mesh functions
void registerMeshAnimFuncs(emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>>& theClass)
{
    theClass.function("getMeshActiveTimeStep", &InteractiveCaeMeshManager::getMeshActiveTimeStep)
        .function("setMeshActiveTimeStep", &InteractiveCaeMeshManager::setMeshActiveTimeStep)
        .function("getAvailableMeshTimeSteps", &InteractiveCaeMeshManager::getAvailableMeshTimeSteps)
        .function("setAvailableMeshTimeSteps",
                  emscripten::optional_override(
                      [](InteractiveCaeMeshManager& self, const std::string& theObjId, emscripten::val theVal) {
                          std::vector<Vec2> aVal;
                          EmscriptenHelper::FromJSArray(aVal, theVal);
                          self.setAvailableMeshTimeSteps(theObjId, aVal);
                      }))
        .function("startMeshAnimation", &InteractiveCaeMeshManager::startMeshAnimation)
        .function("stopMeshAnimation", &InteractiveCaeMeshManager::stopMeshAnimation)
        .function("continueMeshAnimation", &InteractiveCaeMeshManager::continueMeshAnimation)
        .function("getMeshAnimationDuration", &InteractiveCaeMeshManager::getMeshAnimationDuration)
        .function("setMeshAnimationDuration", &InteractiveCaeMeshManager::setMeshAnimationDuration)
        .function("isMeshAnimationActive", &InteractiveCaeMeshManager::isMeshAnimationActive)
        .function("getMeshAnimationLoopPlayback", &InteractiveCaeMeshManager::getMeshAnimationLoopPlayback)
        .function("setMeshAnimationLoopPlayback", &InteractiveCaeMeshManager::setMeshAnimationLoopPlayback)
        .function("getMeshAnimationUniformSteps", &InteractiveCaeMeshManager::getMeshAnimationUniformSteps)
        .function("setMeshAnimationUniformSteps", &InteractiveCaeMeshManager::setMeshAnimationUniformSteps);
}

EMSCRIPTEN_BINDINGS(InteractiveCaeMeshManager)
{
    emscripten::register_vector<double>("DoubleVector");
    emscripten::register_vector<std::string>("StringVector");
    emscripten::register_vector<Vec4>("Vec4Vector");
    emscripten::register_vector<Vec2>("Vec2Vector");
    emscripten::register_vector<int>("IntVector");
    emscripten::register_map<int, std::string>("IntStringMap");

    emscripten::class_<InteractiveCaeMeshManager, emscripten::base<IManager>> aCaeMeshManager(
        "InteractiveCaeMeshManager");

    registerMeshGeometryFuncs(aCaeMeshManager);
    registerMeshAspectsFuncs(aCaeMeshManager);
    registerMeshDisplayFuncs(aCaeMeshManager);
    registerMeshPrsFuncs(aCaeMeshManager);
    registerMeshPropsFuncs(aCaeMeshManager);
    registerMeshVecFuncs(aCaeMeshManager);
    registerMeshRWFuncs(aCaeMeshManager);
    registerMeshGroupsFuncs(aCaeMeshManager);
    registerMeshAnimFuncs(aCaeMeshManager);
}
#endif

} // namespace