#include "SelectionMeshEntity.h"

#include "EmscriptenHelper.h"

#include <ViewCae/MeshPresentationVector.h>

#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshBinReader.h>

#include <Message.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

using MeshElemType = RWCae::MeshElemType;

// ================================================================
// Function : SelectionMeshEntity
// ================================================================
SelectionMeshEntity::SelectionMeshEntity(const ViewCae::MeshSubEntity&                   theSubEntity,
                                         const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner)
{
    if (!theSubEntity.IsDefined()) {
        return;
    }

    entityId = theSubEntity.Index();

    prsId = theSubEntity.RefMeshPrs().ToCString();
    geomId = theSubEntity.RefMeshGeom().ToCString();
    isElemSurf = theSubEntity.IsElemSurf();

    if (theSubEntity.IsNode()) {
        fillNodeInfo(theOwner);
    } else {
        fillElementInfo(theOwner);
    }

    fillPrsInfo(theOwner);
}

// ================================================================
// Function : fillNodeInfo
// ================================================================
void SelectionMeshEntity::fillNodeInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner)
{
    nbNodes = 1;

    Handle(ViewCae::MeshPresentation) aPrs = theOwner->Presentation(prsId.c_str());
    if (aPrs.IsNull()) {
        return;
    }

    Handle(RWCae::MeshData) aMeshData = aPrs->GetMeshGeometry()->GetMeshData();
    if (aMeshData.IsNull()) {
        return;
    }

    if (entityId < 0 || entityId >= aMeshData->Nodes().Size()) {
        Message::SendFail() << "Error: Can't get node '" << entityId << "'";
        entityId = -1;
        return;
    }

    entityType = int(ViewCae::MeshEntityType::NODE);

    pos = Vec3(Graphic3d_Vec3d(aMeshData->Nodes()[entityId]));

    int aCurTimeStep = theOwner->ActiveTimeStep();

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theOwner->GetMeshData()->Properties()) {
        if (aProp.IsNull() || aProp->IsElemental() || entityId >= aProp->NbValues()
            || (aProp->TimeStep() != aCurTimeStep && aCurTimeStep != -1) || !aProp->IsLoaded()) {
            continue;
        }

        MeshPropertyValuesInfo& aPropInfo =
            (*(propsInfo.emplace(aProp->Id().ToCString(), MeshPropertyValuesInfo()).first)).second;

        aPropInfo.description = aProp->Description().ToCString();

        const int aNbRawComponents = aProp->NbRawComponents();

        for (int i = 0; i < aNbRawComponents; i++) {
            aPropInfo.components.push_back(MeshComponentValueInfo());
            MeshComponentValueInfo& aCompInfo = aPropInfo.components[aPropInfo.components.size() - 1];

            const Handle(RWCae::MeshScalarPropertyBase)& aComp = aProp->GetRawComponent(i);

            double aValue = aProp->GetRawComponent(i)->Value(RWCae::MeshPropertyEntityInfo(entityId));
            if (RWCae::IsNaN(aValue)) {
                aValue = RealLast();
            }

            aCompInfo.value        = aValue;
            aCompInfo.description  = aComp->Description().ToCString();
            aCompInfo.displayUnits = aComp->DisplayUnits().ToCString();
        }
    }
}

// ================================================================
// Function : fillElementInfo
// ================================================================
void SelectionMeshEntity::fillElementInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner)
{
    Handle(ViewCae::MeshPresentation) aPrs = theOwner->Presentation(prsId.c_str());
    if (aPrs.IsNull()) {
        return;
    }

    Handle(RWCae::MeshData) aMeshData = aPrs->GetMeshGeometry()->GetMeshData();
    if (aMeshData.IsNull()) {
        return;
    }

    if (entityId >= aMeshData->IndexedElements().Size()) {
        Message::SendFail() << "Error: Can't get element '" << entityId << "'";
        entityId = -1;
        return;
    }

    RWCae::MeshData::IndexedElement anElem = aMeshData->IndexedElements()[entityId];
    fillElementInfo(theOwner, aMeshData, anElem);
}

// ================================================================
// Function : fillElementInfo
// ================================================================
void SelectionMeshEntity::fillElementInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner,
                                          const Handle(RWCae::MeshData)&                 theMeshData,
                                          const RWCae::MeshData::IndexedElement&         theElem)
{
    auto compute = [this, &theOwner, &theElem](const auto& theAdaptor) {
        entityType = int(theAdaptor.GetElementType()) + ViewCae::MeshEntityTypeElemLower;
        fillElementTInfo(theAdaptor, theOwner, theElem.Index);
        return true;
    };

    RWCae::MeshAdaptorsIteratorAllElems::Switch(theElem.Type, compute, theMeshData);
}

// ================================================================
// Function : fillElementTInfo
// ================================================================
template<class Elem_T>
void SelectionMeshEntity::fillElementTInfo(const RWCae::MeshAdaptor<Elem_T>&              theMesh,
                                           const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner,
                                           int                                              theElemIndex)
{
    const Elem_T&  anElem  = theMesh.GetElement(theElemIndex);
    Graphic3d_Vec3 aCenter = theMesh.ElementCenter(anElem);
    pos                    = Vec3(Graphic3d_Vec3d(aCenter));

    nbNodes = anElem.NbNodes();

    auto getInterpolatedValue = [this, &anElem](const Handle(RWCae::MeshScalarPropertyBase)& theComp) -> double {
        double anInterpolatedValue = 0.0;
        int    aNbNodes            = 0;

        for (RWCae::MeshElemEntityId i = 0; i < anElem.NbNodes(); i++) {
            RWCae::MeshPropertyEntityInfo anInfo = theComp->IsElemental()
                                                       ? RWCae::MeshPropertyEntityInfo(entityId, anElem[i])
                                                       : RWCae::MeshPropertyEntityInfo(anElem[i]);

            double aPropValue = theComp->Value(anInfo);
            if (!RWCae::IsNaN(aPropValue)) {
                anInterpolatedValue += aPropValue;
                aNbNodes++;
            }
        }

        anInterpolatedValue = (aNbNodes > 0) ? (anInterpolatedValue / aNbNodes) : RealLast();
        return anInterpolatedValue;
    };

    int aCurTimeStep = theOwner->ActiveTimeStep();

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theOwner->GetMeshData()->Properties()) {
        if (aProp.IsNull() || (aProp->TimeStep() != aCurTimeStep && aCurTimeStep != -1) || !aProp->IsLoaded()) {
            continue;
        }

        MeshPropertyValuesInfo& aPropInfo =
            (*(propsInfo.emplace(aProp->Id().ToCString(), MeshPropertyValuesInfo()).first)).second;

        aPropInfo.description = aProp->Description().ToCString();

        const int aNbRawComponents = aProp->NbRawComponents();

        for (int aCompIter = 0; aCompIter < aNbRawComponents; aCompIter++) {
            aPropInfo.components.push_back(MeshComponentValueInfo());
            MeshComponentValueInfo& aCompInfo = aPropInfo.components[aPropInfo.components.size() - 1];

            const Handle(RWCae::MeshScalarPropertyBase)& aComp = aProp->GetRawComponent(aCompIter);

            aCompInfo.value        = getInterpolatedValue(aComp);
            aCompInfo.description  = aComp->Description().ToCString();
            aCompInfo.displayUnits = aComp->DisplayUnits().ToCString();
        }
    }
}

// ================================================================
// Function : fillPrsInfo
// ================================================================
void SelectionMeshEntity::fillPrsInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner)
{
    if (prsId.empty()) {
        return;
    }

    auto propAdded = [&propsInfo = propsInfo](const Handle(RWCae::MeshNVecProperty)& theProp) {
        return propsInfo.find(theProp->Id().ToCString()) != propsInfo.end();
    };

    RWCae::MeshPropertyComponent anActiveProp = theOwner->Property(prsId.c_str());
    if (!anActiveProp.Property.IsNull() && anActiveProp.ComponentIndex != -1 && propAdded(anActiveProp.Property)) {
        auto aCompInfo = prsPropsInfo.emplace((int)MeshPropertyUsageType::COLOR_MAP, MeshPropertyInfo()).first;
        (*aCompInfo).second.componentIndex = anActiveProp.ComponentIndex;
        (*aCompInfo).second.propertyId     = anActiveProp.Property->Id().ToCString();
    }

    Handle(ViewCae::MeshPresentation) aPrs = theOwner->Presentation(prsId.c_str());

    if (Handle(ViewCae::MeshPresentationVector) aVectorPrs = Handle(ViewCae::MeshPresentationVector)::DownCast(
            aPrs)) {
        if (!aVectorPrs->VectorPartParams()->VectorProperty().IsNull()
            && propAdded(aVectorPrs->VectorPartParams()->VectorProperty())) {
            auto aCompInfo = prsPropsInfo.emplace((int)MeshPropertyUsageType::VECTORS, MeshPropertyInfo()).first;
            (*aCompInfo).second.propertyId = aVectorPrs->VectorPartParams()->VectorProperty()->Id().ToCString();
        }
    }

    for (Handle(ViewCaeGeom::MeshGeometry) aGeom = aPrs->GetMeshGeometry(); !aGeom.IsNull();
         aGeom                                    = aGeom->GetChild()) {
        if (Handle(ViewCaeGeom::MeshGeometryDeformed) aDefGeom = Handle(ViewCaeGeom::MeshGeometryDeformed)::DownCast(
                aGeom)) {
            if (!aDefGeom->DeformedProperty().IsNull() && propAdded(aDefGeom->DeformedProperty())) {
                auto aCompInfo =
                    prsPropsInfo.emplace((int)MeshPropertyUsageType::DEFORMATION, MeshPropertyInfo()).first;
                (*aCompInfo).second.propertyId = aDefGeom->DeformedProperty()->Id().ToCString();
            }
        } else if (Handle(ViewCaeGeom::MeshGeometryIso) anIsoGeom = Handle(ViewCaeGeom::MeshGeometryIso)::DownCast(
                       aGeom)) {
            RWCae::MeshPropertyComponent anIsoProp = anIsoGeom->Property();
            if (!anIsoProp.Property.IsNull() && anIsoProp.ComponentIndex != -1 && propAdded(anIsoProp.Property)) {
                auto aCompInfo =
                    prsPropsInfo.emplace((int)MeshPropertyUsageType::ISO_SURFACE, MeshPropertyInfo()).first;
                (*aCompInfo).second.componentIndex = anIsoProp.ComponentIndex;
                (*aCompInfo).second.propertyId     = anIsoProp.Property->Id().ToCString();
            }
        }
    }
}

// ================================================================
// Function : SelectionMeshEntity
// ================================================================
SelectionMeshEntity::SelectionMeshEntity(const TCollection_AsciiString&  thePrsId,
                                         const ViewCae::MeshAnnotation& theAnnotation,
                                         int                             theLineIndex)
{
    entityId   = theAnnotation.Index();
    entityType = int(ViewCae::MeshEntityType::LABEL_GROUP);
    prsId      = thePrsId.ToCString();
    lineIndex  = theLineIndex;
    pos        = Vec3(theAnnotation.Pos().XYZ());
    nbNodes    = 0;
}

// ================================================================
// Function : UpdateSelectedEntity
// ================================================================
void SelectionMeshEntity::UpdateSelectedEntity(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner,
                                               const Handle(AIS_InteractiveContext)&            theCtx) const
{
    if (entityId == -1 || entityType == int(ViewCae::MeshEntityType::UNKNOWN)) {
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = theOwner->Presentation(prsId.c_str());
    if (aPrs.IsNull()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometry) aPrsGeom = getPrsGeom(aPrs);
    if (aPrsGeom.IsNull()) {
        return;
    }

    ViewCae::MeshSubEntity aSubEntity;

    if (entityType == int(ViewCae::MeshEntityType::NODE)) {
        aSubEntity.SetNode(aPrs, aPrsGeom, Graphic3d_Vec3(pos), entityId);
    } else {
        RWCae::MeshElemType anElemType = (RWCae::MeshElemType)(entityType - ViewCae::MeshEntityTypeElemLower);
        aSubEntity.SetElem(aPrs,
                           aPrsGeom,
                           Graphic3d_Vec3(pos),
                           ViewCaeGeom::MeshGeomElem(entityId, anElemType, isElemSurf));
    }

    ViewCae::MeshSubEntity aPrevSubEntity = aSubEntity;

    if (!refMeshDataPath.empty()) {
        RWCae::MeshBinReader aReader;
        if (!aReader.Perform(refMeshDataPath.c_str())) {
            Message::SendFail() << "Error: Can't read custom mesh data for selected element '" << entityId << "'";
            return;
        }

        if (!aSubEntity.SetRefMeshData(aReader.GetMeshData())) {
            Message::SendFail() << "Error: Can't set custom mesh data for selected element '" << entityId << "'";
            return;
        }
    }

    ViewCae::InteractiveCaeMeshOwner::MapOfSelectedEntities& aSelected = theOwner->SelectedSubEntities();
    if (aSelected.Contains(aPrevSubEntity)) {
        aSelected.Remove(aPrevSubEntity);
        aSelected.Add(aSubEntity);
    } else {
        aSelected.Add(aSubEntity);
    }

    theOwner->HighlightSubEntities(theCtx->MainPrsMgr(), true);
}


// ================================================================
// Function : getPrsGeom
// ================================================================
Handle(ViewCaeGeom::MeshGeometry) SelectionMeshEntity::getPrsGeom(
    const Handle(ViewCae::MeshPresentation)& thePrs) const
{
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(thePrs->GetMeshGeometry().get())) {
        if (aGeom->Id() == geomId.c_str()) {
            return aGeom;
        }
    }

    for (int aPart = 0; aPart < ViewCae::MeshNbPartTypes; aPart++) {
        Handle(ViewCaeGeom::MeshGeometry) aGeom = thePrs->GetGenGeometry(aPart);
        if (!aGeom.IsNull() && aGeom->Id() == geomId.c_str()) {
            return aGeom;
            break;
        }
    }

    return nullptr;
}

#ifdef __EMSCRIPTEN__
//! Register SelectionMeshEntity class
void registerSelectionMeshEntity()
{
    emscripten::value_object<SelectionMeshEntity>("SelectionMeshEntity")
        .field("entityId", &SelectionMeshEntity::entityId)
        .field("entityType", &SelectionMeshEntity::entityType)
        .field("nbNodes", &SelectionMeshEntity::nbNodes)
        .field("prsId", &SelectionMeshEntity::prsId)
        .field("geomId", &SelectionMeshEntity::geomId)
        .field("pos", &SelectionMeshEntity::pos)
        .field("isElemSurf", &SelectionMeshEntity::isElemSurf)
        .field("refMeshDataPath", &SelectionMeshEntity::refMeshDataPath)
        .field("propsInfo",
               emscripten::optional_override([](const SelectionMeshEntity& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSMap(theStruct.propsInfo);
               }),
               emscripten::optional_override([](SelectionMeshEntity& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSMap(theStruct.propsInfo, theVal);
               }))
        .field("prsPropsInfo",
               emscripten::optional_override([](const SelectionMeshEntity& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSMap(theStruct.prsPropsInfo);
               }),
               emscripten::optional_override([](SelectionMeshEntity& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSMap(theStruct.prsPropsInfo, theVal);
               }))
        .field("lineIndex", &SelectionMeshEntity::lineIndex);
}

//! Register MeshPropertyValuesInfo class
void registerMeshPropertyValuesInfo()
{
    emscripten::value_object<MeshPropertyValuesInfo>("MeshPropertyValuesInfo")
        .field("description", &MeshPropertyValuesInfo::description)
        .field("components",
               emscripten::optional_override([](const MeshPropertyValuesInfo& theStruct) -> emscripten::val {
                   return EmscriptenHelper::ToJSArray(theStruct.components);
               }),
               emscripten::optional_override([](MeshPropertyValuesInfo& theStruct, emscripten::val theVal) {
                   EmscriptenHelper::FromJSArray(theStruct.components, theVal);
               }));
}

EMSCRIPTEN_BINDINGS(SelectionMeshEntity)
{
    emscripten::register_vector<SelectionMeshEntity>("SelectionMeshEntityVector");
    emscripten::register_vector<MeshComponentValueInfo>("MeshComponentValueInfoVector");
    emscripten::register_map<std::string, MeshPropertyValuesInfo>("MeshPropertyValuesInfoMap");
    emscripten::register_map<int, MeshPropertyInfo>("MeshPropertyInfoMap");

    emscripten::enum_<ViewCae::MeshEntityType>("MeshEntityType")
        .value("Unknown", ViewCae::MeshEntityType::UNKNOWN)
        .value("Node", ViewCae::MeshEntityType::NODE)
        .value("Element0d", ViewCae::MeshEntityType::ELEMENT_0D)
        .value("Triangle", ViewCae::MeshEntityType::TRIANGLE)
        .value("Quadrangle", ViewCae::MeshEntityType::QUADRANGLE)
        .value("Polygon", ViewCae::MeshEntityType::POLYGON)
        .value("Tetrahedron", ViewCae::MeshEntityType::TETRAHEDRON)
        .value("Pyramid", ViewCae::MeshEntityType::PYRAMID)
        .value("Prism", ViewCae::MeshEntityType::PRISM)
        .value("Hexahedron", ViewCae::MeshEntityType::HEXAHEDRON)
        .value("Polyhedron", ViewCae::MeshEntityType::POLYHEDRON)
        .value("LabelGroup", ViewCae::MeshEntityType::LABEL_GROUP);
    emscripten::value_object<MeshComponentValueInfo>("MeshComponentValueInfo")
        .field("value", &MeshComponentValueInfo::value)
        .field("displayUnits", &MeshComponentValueInfo::displayUnits)
        .field("description", &MeshComponentValueInfo::description);
    emscripten::value_object<MeshPropertyInfo>("MeshPropertyInfo")
        .field("propertyId", &MeshPropertyInfo::propertyId)
        .field("componentIndex", &MeshPropertyInfo::componentIndex);

    registerSelectionMeshEntity();
    registerMeshPropertyValuesInfo();
}
#endif

} // namespace
