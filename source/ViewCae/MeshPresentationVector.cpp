#include <ViewCae/MeshPresentationVector.h>

#include <ViewCae/CaeMeshTools.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>

#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>

#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshTools.h>

#include <gp_Pnt.hxx>

namespace {
static constexpr const char* NAME_PROPID    = "vectorpropid";
static constexpr const char* NAME_DISTRTYPE = "distrtype";
static constexpr const char* NAME_RATIO     = "ratio";
}

namespace ViewCae {
// =======================================================================
// function : ToJson
// =======================================================================
void MeshPrsVectorPartParams::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    MeshPrsPartParams::ToJson(theJson);

    theJson.Key(NAME_PROPID);
    theJson.String(!myVectorProperty.IsNull() ? myVectorProperty->Id().ToCString() : "");

    theJson.Key(NAME_DISTRTYPE);
    theJson.Int((int)myVectorDistrType);

    theJson.Key(NAME_RATIO);
    theJson.Double(mySamplingRatio);
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshPrsVectorPartParams::FromJson(const RWBase::JsonHelper::Value& theJson,
                                       const Handle(RWCae::MeshData)&   theMeshData,
                                       int                                theTime,
                                       bool&                              theIsChanged)
{
    if (!MeshPrsPartParams::FromJson(theJson, theMeshData, theTime, theIsChanged)) {
        return false;
    }

    const RWBase::JsonHelper::Value* aVecPropIdField = RWBase::JsonHelper::FindStringMember(theJson, NAME_PROPID);
    const RWBase::JsonHelper::Value* aDistrTypeField = RWBase::JsonHelper::FindIntMember(theJson, NAME_DISTRTYPE);
    const RWBase::JsonHelper::Value* aRatioField     = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_RATIO);
    if (aVecPropIdField == nullptr || aDistrTypeField == nullptr || aRatioField == nullptr) {
        return false;
    }

    const Handle(RWCae::MeshNVecProperty)& aVecProp = theMeshData->GetPropertyById(aVecPropIdField->GetString(),
                                                                                     theTime);
    if (aVecProp != myVectorProperty) {
        myVectorProperty = aVecProp;
        RWCae::MeshTools::LoadMeshProperty(theMeshData, aVecProp);
        theIsChanged = true;
    }

    ViewCae::MeshVectorDistrType aDistrType = (ViewCae::MeshVectorDistrType)aDistrTypeField->GetInt();
    if (aDistrType != myVectorDistrType) {
        myVectorDistrType = aDistrType;
        theIsChanged      = true;
    }

    double aRatio = aRatioField->GetDouble();
    if (!::IsEqual(aRatio, mySamplingRatio)) {
        mySamplingRatio = aRatio;
        theIsChanged    = true;
    }

    return true;
}

// =======================================================================
// function : MeshPresentationVector
// =======================================================================
MeshPresentationVector::MeshPresentationVector(const Handle(MeshPropertyData)&           thePropData,
                                               int                                       thePropIndex,
                                               const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                               const TCollection_AsciiString&            theId,
                                               bool                                      theToUpdateGeomId)
    : MeshPresentation(thePropData, thePropIndex, theGeometry, theId, theToUpdateGeomId)
{
    myPrsVisProps = new MeshPrsVectorVisProps();

    myPartsParams.Bind((int)ViewCae::MeshPartType::Vectors,
                       MeshPrsPartParamsFactory::Create(nullptr, ViewCae::MeshPartType::Vectors));
}

// =======================================================================
// function : ~MeshPresentationVector
// =======================================================================
MeshPresentationVector::~MeshPresentationVector()
{
    //
}

// =======================================================================
// function : UpdateAspects
// =======================================================================
void MeshPresentationVector::UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    MeshPresentation::UpdateAspects(theOwner);

    const MeshPrsVectorVisProps* aPrsProps = dynamic_cast<MeshPrsVectorVisProps*>(myPrsVisProps.get());

    RWCae::MeshPropertyComponent aProp        = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                       : RWCae::MeshPropertyComponent();
    Handle(MeshColorMapper)        aColorMapper = !myPropData.IsNull() ? myPropData->ColorMapper(aProp) : nullptr;

    bool toEnableTextures = !aProp.Property.IsNull() && !aColorMapper.IsNull() && aProp.Property->HasScalarComponents();
    myDrawer->SetVectorTextureMapOn(toEnableTextures);

    const float aTextureTransp           = !aColorMapper.IsNull() ? aColorMapper->ColorTextureTransparency() : 0.0f;
    Handle(Graphic3d_Texture2D) aTexture = !aColorMapper.IsNull() ? aColorMapper->ColorTexture() : nullptr;
    myDrawer->UpdateAspectTexture(myDrawer->FillAreaAspectVectors(), aTexture, aTextureTransp);

    getParts()(MeshPartType::Vectors)->SetShow(aPrsProps->IsVisible());

    updateEffectiveScale(theOwner);
    updateUsedProps();
}

// ================================================================
// Function : updateEffectiveScale
// ================================================================
void MeshPresentationVector::updateEffectiveScale(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    const MeshPrsVectorVisProps* aPrsProps = dynamic_cast<MeshPrsVectorVisProps*>(myPrsVisProps.get());

    double anEffectiveScale = aPrsProps->VectorScaleCoeff();

    Handle(RWCae::MeshNVecProperty) aVecProp = VectorPartParams()->VectorProperty();
    if (!aVecProp.IsNull() && anEffectiveScale != 0.0) {
        anEffectiveScale *= RWCae::MeshTools::ComputeVectorTrsfRatio(theOwner->GetMeshData(), aVecProp);
    }
    setEffectiveScale(anEffectiveScale);
}

// ================================================================
// Function : updateUsedProps
// ================================================================
void MeshPresentationVector::updateUsedProps()
{
    myUsedPerPartProps[(int)MeshPartType::Vectors].Clear();

    // Record used props to mark their dependencies as used.
    std::vector<Handle(RWCae::MeshNVecProperty)> aUsedProps;

    Handle(RWCae::MeshNVecProperty) aDeformedProp;
    Handle(RWCae::MeshNVecProperty) anIsoProp;

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            anIsoProp = anIsoGeom->Property().IsDefined() ? anIsoGeom->Property().Property : nullptr;
        } else if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(
                       aGeom)) {
            aDeformedProp = !aDefGeom->DeformedProperty().IsNull() && aDefGeom->DeformedProperty()->IsVector3()
                                 && aDefGeom->DeformedProperty()->IsNodal()
                              ? aDefGeom->DeformedProperty()
                              : nullptr;
        }
    }

    if (!anIsoProp.IsNull()) {
        addUsedPerPartProp(MeshPartType::Vectors, anIsoProp->Id());
        aUsedProps.push_back(anIsoProp);
    }

    if (!aDeformedProp.IsNull()) {
        addUsedPerPartProp(MeshPartType::Vectors, aDeformedProp->Id());
        aUsedProps.push_back(aDeformedProp);
    }

    Handle(RWCae::MeshNVecProperty) aVecProp = VectorPartParams()->VectorProperty();
    if (aVecProp.IsNull() || !aVecProp->HasScalarComponents() || !aVecProp->IsVector3()) {
        return;
    }

    addUsedPerPartProp(MeshPartType::Vectors, aVecProp->Id());
    aUsedProps.push_back(aVecProp);

    RWCae::MeshPropertyComponent aProp = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                : RWCae::MeshPropertyComponent();
    if (aProp.IsDefined() && aProp.Property->HasScalarComponents()
        && (aProp.Property->IsNodal() != aVecProp->IsElemental())) {
        addUsedPerPartProp(MeshPartType::Vectors, aProp.Property->Id());
        aUsedProps.push_back(aProp.Property);
    }

    // Add all dependencies of used props.
    std::vector<TCollection_AsciiString> anAdditionalDependencies;
    addDependencyPropIds(anAdditionalDependencies, aUsedProps);
    for (const TCollection_AsciiString& aDepId : anAdditionalDependencies) {
        addUsedPerPartProp(MeshPartType::Vectors, aDepId);
    }
}

// ================================================================
// Function : updateParts
// ================================================================
void MeshPresentationVector::updateParts(MeshParts& theParts)
{
    theParts(MeshPartType::Vectors)->Update(this);
}

// ================================================================
// Function : needToUpdateParts
// ================================================================
bool MeshPresentationVector::needToUpdateParts(const MeshParts& theParts) const
{
    return theParts(MeshPartType::Vectors)->NeedToUpdate();
}

// ================================================================
// Function : fillProperties
// ================================================================
void MeshPresentationVector::fillProperties()
{
    if (myGeomPropsUpdated) {
        return;
    }

    Handle(RWCae::MeshNVecProperty) aVecProp = VectorPartParams()->VectorProperty();
    if (!myToUseGenGeom) {
        Handle(ViewCaeGeom::MeshGeometry) aGeom = myGeometry->GetLastNotModifierChild();
        while (!aGeom.IsNull()) {
            Handle(RWCae::MeshNVecProperty) aProp = aGeom->CalculateProperty(aVecProp);
            myGeomVecProps.Bind(aGeom->Id(), aProp);
            aGeom = aGeom->GetChild();
        }
    } else {
        for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(myGenGeometry);
             anIter.More();
             anIter.Next()) {
            if (myGeomVecProps.IsBound(anIter.Value()->Id())) {
                continue;
            }

            Handle(RWCae::MeshNVecProperty) aProp = anIter.Value()->CalculateProperty(aVecProp);
            myGeomVecProps.Bind(anIter.Value()->Id(), aProp);
        }
    }

    MeshPresentation::fillProperties();
}

// ================================================================
// Function : updateSensitiveSets
// ================================================================
void MeshPresentationVector::updateSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    (void)theOwner;
}

// ================================================================
// Function : OnPropertiesUpdated
// ================================================================
void MeshPresentationVector::OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                                 bool theCachedProps)
{
    MeshPresentation::OnPropertiesUpdated(theProps, theCachedProps);

    bool toUpdatePart = false;

    Handle(RWCae::MeshNVecProperty) aVecProp = VectorPartParams()->VectorProperty();
    if (!aVecProp.IsNull() && theProps.Contains(aVecProp)) {
        myGeomPropsUpdated = false;
        myGeomVecProps.Clear();
        toUpdatePart = true;
    }

    MeshParts* aPart = myPrimitives.ChangeSeek(myActiveTimeStep);
    if (aPart == nullptr) {
        return;
    }

    Handle(RWCae::MeshNVecProperty) aProp = myPropData->Property(myPropIndex).Property;
    if (!aProp.IsNull() && theProps.Contains(aProp)) {
        toUpdatePart = true;
    }

    if (toUpdatePart) {
        (*aPart)(MeshPartType::Vectors)->Clear();
        SetToUpdateGenGeometry((int)MeshPartType::Vectors);
    }
}

// ================================================================
// Function : onGeomUpdated
// ================================================================
void MeshPresentationVector::onGeomUpdated(const TCollection_AsciiString& theGeomId)
{
    MeshPresentation::onGeomUpdated(theGeomId);
    if (theGeomId.IsEmpty()) {
        myGeomVecProps.Clear();
    } else {
        myGeomVecProps.UnBind(theGeomId);
    }
}

} // namespace