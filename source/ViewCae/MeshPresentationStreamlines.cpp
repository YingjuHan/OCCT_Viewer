#include <ViewCae/MeshPresentationStreamlines.h>

#include <ViewCae/InteractiveCaeMeshOwner.h>

#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>

#include <RWCae/MeshTools.h>

namespace {
static constexpr const char* NAME_PROPID        = "streamlinespropid";
static constexpr const char* NAME_ORIGIN        = "origin";
static constexpr const char* NAME_SIDE          = "side";
static constexpr const char* NAME_UP            = "up";
static constexpr const char* NAME_STEPCOEFF     = "stepcoeff";
static constexpr const char* NAME_LENGTHCOEFF   = "lengthcoeff";
static constexpr const char* NAME_ISREGULAR     = "isregular";
static constexpr const char* NAME_NBSTREAMLINES = "nbstreamlines";
static constexpr const char* NAME_UVSTREAMLINES = "uvstreamlines";
}

namespace ViewCae {
using JsonHelper = RWBase::JsonHelper;

// =======================================================================
// function : ToJson
// =======================================================================
void MeshPrsStreamlinesPartParams::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    MeshPrsPartParams::ToJson(theJson);

    theJson.Key(NAME_PROPID);
    theJson.String(!myStreamlinesProperty.IsNull() ? myStreamlinesProperty->Id().ToCString() : "");

    auto dumpVec = [&theJson](const gp_XYZ& theVec, const char* theName) {
        theJson.Key(theName);
        theJson.StartArray();
        theJson.Double(theVec.X());
        theJson.Double(theVec.Y());
        theJson.Double(theVec.Z());
        theJson.EndArray();
    };

    dumpVec(myPlane.Origin.XYZ(), NAME_ORIGIN);
    dumpVec(myPlane.Side.XYZ(), NAME_SIDE);
    dumpVec(myPlane.Up.XYZ(), NAME_UP);

    theJson.Key(NAME_STEPCOEFF);
    theJson.Double(myStepCoeff);

    theJson.Key(NAME_LENGTHCOEFF);
    theJson.Double(myLengthCoeff);

    theJson.Key(NAME_ISREGULAR);
    theJson.Bool(myIsRegular);

    theJson.Key(NAME_NBSTREAMLINES);
    theJson.Int(myNbStreamlines);

    theJson.Key(NAME_UVSTREAMLINES);
    theJson.StartArray();
    theJson.Int(myUVStreamlines.x());
    theJson.Int(myUVStreamlines.y());
    theJson.EndArray();
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshPrsStreamlinesPartParams::FromJson(const RWBase::JsonHelper::Value& theJson,
                                            const Handle(RWCae::MeshData)&   theMeshData,
                                            int                                theTime,
                                            bool&                              theIsChanged)
{
    if (!MeshPrsPartParams::FromJson(theJson, theMeshData, theTime, theIsChanged)) {
        return false;
    }

    const JsonHelper::Value* aPropIdField = JsonHelper::FindStringMember(theJson, NAME_PROPID);
    if (aPropIdField == nullptr) {
        return false;
    }

    const Handle(RWCae::MeshNVecProperty)& aProp = theMeshData->GetPropertyById(aPropIdField->GetString(), theTime);
    if (aProp != myStreamlinesProperty) {
        myStreamlinesProperty = aProp;
        RWCae::MeshTools::LoadMeshProperty(theMeshData, aProp);
        theIsChanged = true;
    }

    return paramsFromJson(theJson, theIsChanged);
}

// =======================================================================
// function : paramsFromJson
// =======================================================================
bool MeshPrsStreamlinesPartParams::paramsFromJson(const RWBase::JsonHelper::Value& theJson, bool& theIsChanged)
{
    if (!planeFromJson(theJson, theIsChanged)) {
        return false;
    }

    const JsonHelper::Value* aStepCoeffField     = JsonHelper::FindDoubleMember(theJson, NAME_STEPCOEFF);
    const JsonHelper::Value* aLengthCoeffField   = JsonHelper::FindDoubleMember(theJson, NAME_LENGTHCOEFF);
    const JsonHelper::Value* aNbStreamlinesField = JsonHelper::FindIntMember(theJson, NAME_NBSTREAMLINES);
    if (aStepCoeffField == nullptr || aLengthCoeffField == nullptr || aNbStreamlinesField == nullptr) {
        return false;
    }

    double aStepCoeff = aStepCoeffField->GetDouble();
    if (aStepCoeff != myStepCoeff) {
        myStepCoeff  = aStepCoeff;
        theIsChanged = true;
    }

    double aLengthCoeff = aLengthCoeffField->GetDouble();
    if (aLengthCoeff != myLengthCoeff) {
        myLengthCoeff = aLengthCoeff;
        theIsChanged  = true;
    }

    int aNbStreamlines = aNbStreamlinesField->GetInt();
    if (aNbStreamlines != myNbStreamlines) {
        myNbStreamlines = aNbStreamlines;
        theIsChanged    = true;
    }

    // Optional parameters. If missing, random distribution of initial points is used.
    const JsonHelper::Value* anIsRegularField    = JsonHelper::FindBoolMember(theJson, NAME_ISREGULAR);
    const JsonHelper::Value* aUVStreamlinesField = JsonHelper::FindIntMember(theJson, NAME_UVSTREAMLINES);

    bool anIsRegular = (anIsRegularField == nullptr ? false : anIsRegularField->GetBool());
    if (anIsRegular != myIsRegular) {
        myIsRegular  = anIsRegular;
        theIsChanged = true;
    }

    constexpr int   DEFAULTUVSTREAMLINES = 10;
    Graphic3d_Vec2i aUVStreamlines(DEFAULTUVSTREAMLINES);
    if (aUVStreamlinesField != nullptr) {
        vec2iFromJson(aUVStreamlinesField, aUVStreamlines);
    }
    if (aUVStreamlines != myUVStreamlines) {
        myUVStreamlines = aUVStreamlines;
        theIsChanged    = true;
    }

    return true;
}

// =======================================================================
// function : planeFromJson
// =======================================================================
bool MeshPrsStreamlinesPartParams::planeFromJson(const RWBase::JsonHelper::Value& theJson, bool& theIsChanged)
{
    const JsonHelper::Value* aPlaneOriginField = JsonHelper::FindArrayMember(theJson, NAME_ORIGIN);
    const JsonHelper::Value* aPlaneSideField   = JsonHelper::FindArrayMember(theJson, NAME_SIDE);
    const JsonHelper::Value* aPlaneUpField     = JsonHelper::FindArrayMember(theJson, NAME_UP);

    if (aPlaneOriginField == nullptr || aPlaneSideField == nullptr || aPlaneUpField == nullptr) {
        return false;
    }

    ViewCae::MeshStreamlinesPlane aPlane;
    vecFromJson(aPlaneOriginField, aPlane.Origin);
    vecFromJson(aPlaneSideField, aPlane.Side);
    vecFromJson(aPlaneUpField, aPlane.Up);
    if (!aPlane.IsEqual(myPlane)) {
        myPlane      = aPlane;
        theIsChanged = true;
    }

    return true;
}

// =======================================================================
// function : MeshPresentationStreamlines
// =======================================================================
MeshPresentationStreamlines::MeshPresentationStreamlines(const Handle(MeshPropertyData)&           thePropData,
                                                         int                                       thePropIndex,
                                                         const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                                         const TCollection_AsciiString&            theId,
                                                         bool                                      theToUpdateGeomId)
    : MeshPresentation(thePropData, thePropIndex, theGeometry, theId, theToUpdateGeomId)
{
    myPrsVisProps = new MeshPrsStreamlinesVisProps();

    myPartsParams.Bind((int)ViewCae::MeshPartType::Streamlines,
                       MeshPrsPartParamsFactory::Create(theGeometry->GetFirstParent()->GetMeshData(),
                                                        ViewCae::MeshPartType::Streamlines));
}

// =======================================================================
// function : ~MeshPresentationStreamlines
// =======================================================================
MeshPresentationStreamlines::~MeshPresentationStreamlines()
{
    //
}

// =======================================================================
// function : UpdateAspects
// =======================================================================
void MeshPresentationStreamlines::UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    MeshPresentation::UpdateAspects(theOwner);

    const MeshPrsStreamlinesVisProps* aPrsProps = dynamic_cast<MeshPrsStreamlinesVisProps*>(myPrsVisProps.get());

    RWCae::MeshPropertyComponent aProp        = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                       : RWCae::MeshPropertyComponent();
    Handle(MeshColorMapper)        aColorMapper = !myPropData.IsNull() ? myPropData->ColorMapper(aProp) : nullptr;

    bool toEnableTextures = !aProp.Property.IsNull() && !aColorMapper.IsNull() && aProp.Property->HasScalarComponents();
    myDrawer->SetStreamlinesTextureMapOn(toEnableTextures);

    const float aTextureTransp           = !aColorMapper.IsNull() ? aColorMapper->ColorTextureTransparency() : 0.0f;
    Handle(Graphic3d_Texture2D) aTexture = !aColorMapper.IsNull() ? aColorMapper->ColorTexture() : nullptr;
    myDrawer->UpdateAspectTexture(myDrawer->StreamlinesTubeAspect(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->StreamlinesLineAspect(), aTexture, aTextureTransp);

    getParts()(MeshPartType::Streamlines)->SetShow(aPrsProps->IsVisible());

    myUsedPerPartProps[(int)MeshPartType::Streamlines].Clear();
    // Record used props to mark their dependencies as used.
    std::vector<Handle(RWCae::MeshNVecProperty)> aUsedProps;

    const Handle(RWCae::MeshNVecProperty)& aStreamlinesProp = StreamlinesPartParams()->StreamlinesProperty();
    if (!aStreamlinesProp.IsNull() && aStreamlinesProp->HasScalarComponents() && aStreamlinesProp->IsVector3()) {
        addUsedPerPartProp(MeshPartType::Streamlines, aStreamlinesProp->Id());
        aUsedProps.push_back(aStreamlinesProp);

        if (aProp.IsDefined() && aProp.Property->HasScalarComponents()) {
            addUsedPerPartProp(MeshPartType::Streamlines, aProp.Property->Id());
            aUsedProps.push_back(aProp.Property);
        }
    }

    Handle(RWCae::MeshNVecProperty) aDefProp;
    Handle(RWCae::MeshNVecProperty) anIsoProp;

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            anIsoProp = anIsoGeom->Property().IsDefined() ? anIsoGeom->Property().Property : nullptr;
        } else if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(
                       aGeom)) {
            aDefProp = !aDefGeom->DeformedProperty().IsNull() && aDefGeom->DeformedProperty()->IsVector3()
                            && aDefGeom->DeformedProperty()->IsNodal()
                         ? aDefGeom->DeformedProperty()
                         : nullptr;
        }
    }

    if (!anIsoProp.IsNull()) {
        addUsedPerPartProp(MeshPartType::Streamlines, anIsoProp->Id());
        aUsedProps.push_back(aStreamlinesProp);
    }

    if (!aDefProp.IsNull()) {
        addUsedPerPartProp(MeshPartType::Streamlines, aDefProp->Id());
        aUsedProps.push_back(aStreamlinesProp);
    }

    // Add all dependencies of used props.
    std::vector<TCollection_AsciiString> anAdditionalDependencies;
    addDependencyPropIds(anAdditionalDependencies, aUsedProps);
    for (const TCollection_AsciiString& aDepId : anAdditionalDependencies) {
        addUsedPerPartProp(MeshPartType::Streamlines, aDepId);
    }
}

// ================================================================
// Function : updateParts
// ================================================================
void MeshPresentationStreamlines::updateParts(MeshParts& theParts)
{
    theParts(MeshPartType::Streamlines)->Update(this);
}

// ================================================================
// Function : needToUpdateParts
// ================================================================
bool MeshPresentationStreamlines::needToUpdateParts(const MeshParts& theParts) const
{
    return theParts(MeshPartType::Streamlines)->NeedToUpdate();
}

// ================================================================
// Function : updateSensitiveSets
// ================================================================
void MeshPresentationStreamlines::updateSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    (void)theOwner;
}

// ================================================================
// Function : OnPropertiesUpdated
// ================================================================
void MeshPresentationStreamlines::OnPropertiesUpdated(
    const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
    bool                                                      theCachedProps)
{
    MeshPresentation::OnPropertiesUpdated(theProps, theCachedProps);

    bool toUpdatePart = false;

    const Handle(RWCae::MeshNVecProperty)& aStreamlinesProp = StreamlinesPartParams()->StreamlinesProperty();
    if (!aStreamlinesProp.IsNull() && theProps.Contains(aStreamlinesProp)) {
        toUpdatePart = true;
    }

    MeshParts* aPart = myPrimitives.ChangeSeek(myActiveTimeStep);
    if (aPart == nullptr) {
        return;
    }

    Handle(RWCae::MeshNVecProperty) aProperty = myPropData->Property(myPropIndex).Property;
    if (!aProperty.IsNull() && theProps.Contains(aProperty)) {
        toUpdatePart = true;
    }

    if (toUpdatePart) {
        (*aPart)(MeshPartType::Streamlines)->Clear();
        SetToUpdateGenGeometry((int)MeshPartType::Streamlines);
    }
}

} // namespace
