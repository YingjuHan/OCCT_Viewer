#include <ViewCae/MeshPresentationScalar.h>

#include <ViewCae/CaeMeshTools.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/MeshAdaptorSensitive.h>

#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>

#include <RWCae/MeshAdaptorsIterator.h>

namespace {
static constexpr ViewCae::MeshPartType PARTS_LIST[] = {ViewCae::MeshPartType::Triangles3d,
                                                        ViewCae::MeshPartType::Triangles2d,
                                                        ViewCae::MeshPartType::Edges3d,
                                                        ViewCae::MeshPartType::Edges2d,
                                                        ViewCae::MeshPartType::SharpEdges3d,
                                                        ViewCae::MeshPartType::SharpEdges2d,
                                                        ViewCae::MeshPartType::FreeEdges3d,
                                                        ViewCae::MeshPartType::FreeEdges2d,
                                                        ViewCae::MeshPartType::Points,
                                                        ViewCae::MeshPartType::Elems0d,
                                                        ViewCae::MeshPartType::Centroids3d,
                                                        ViewCae::MeshPartType::Centroids2d,
                                                        ViewCae::MeshPartType::MinMaxAnnotations,
                                                        ViewCae::MeshPartType::GroupsInfo,
                                                        ViewCae::MeshPartType::GeneralCells};

static constexpr const char* NAME_MIN_VALUE = "min";
static constexpr const char* NAME_MAX_VALUE = "max";
}

namespace ViewCae {
// =======================================================================
// function : ToJson
// =======================================================================
void MeshPrsMinMaxPartParams::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    MeshPrsPartParams::ToJson(theJson);

    theJson.Key(NAME_MIN_VALUE);
    theJson.Double(myMinMaxRange.x());
    theJson.Key(NAME_MAX_VALUE);
    theJson.Double(myMinMaxRange.y());
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshPrsMinMaxPartParams::FromJson(const RWBase::JsonHelper::Value& theJson,
                                       const Handle(RWCae::MeshData)&   theMeshData,
                                       int                                theTime,
                                       bool&                              theIsChanged)
{
    if (!MeshPrsPartParams::FromJson(theJson, theMeshData, theTime, theIsChanged)) {
        return false;
    }

    const RWBase::JsonHelper::Value* aMinField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_MIN_VALUE);
    const RWBase::JsonHelper::Value* aMaxField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_MAX_VALUE);
    if (aMinField == nullptr || aMaxField == nullptr) {
        return false;
    }
    double aMin = aMinField->GetDouble();
    double aMax = aMaxField->GetDouble();

    if (!::IsEqual(aMin, myMinMaxRange.x()) || !::IsEqual(aMax, myMinMaxRange.y())) {
        myMinMaxRange = Graphic3d_Vec2d(aMin, aMax);
        theIsChanged  = true;
    }

    return true;
}

// =======================================================================
// function : MeshPresentationScalar
// =======================================================================
MeshPresentationScalar::MeshPresentationScalar(const Handle(MeshPropertyData)&           thePropData,
                                               int                                       thePropIndex,
                                               const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                               const TCollection_AsciiString&            theId,
                                               bool                                      theToUpdateGeomId)
    : MeshPresentation(thePropData, thePropIndex, theGeometry, theId, theToUpdateGeomId)
{
    myPrsVisProps = new MeshPrsVisScalarProps();

    for (MeshPartType aTypeIter : PARTS_LIST) {
        myPartsParams.Bind((int)aTypeIter, MeshPrsPartParamsFactory::Create(nullptr, aTypeIter));
    }
}

// =======================================================================
// function : ~MeshPresentationScalar
// =======================================================================
MeshPresentationScalar::~MeshPresentationScalar()
{
    //
}

// =======================================================================
// function : UpdateAspects
// =======================================================================
void MeshPresentationScalar::UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    MeshPresentation::UpdateAspects(theOwner);

    RWCae::MeshPropertyComponent aProp        = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                       : RWCae::MeshPropertyComponent();
    Handle(MeshColorMapper)        aColorMapper = !myPropData.IsNull() ? myPropData->ColorMapper(aProp) : nullptr;

    bool isGroup                   = !aProp.Property.IsNull() && aProp.Property->HasGroupComponents();
    bool toEnableTextures          = !aProp.Property.IsNull() && !aColorMapper.IsNull();
    bool toEnableTrianglesTextures = toEnableTextures && (!isGroup || aProp.Property->IsElemental());
    bool toEnableElems0dTextures   = toEnableTextures && aProp.Property->IsElemental();
    bool toEnableNodesTextures     = toEnableTextures && !aProp.Property->IsElemental();
    myDrawer->SetFillAreaTextureMapOn(toEnableTrianglesTextures);
    myDrawer->SetElem0dTextureMapOn(toEnableElems0dTextures);
    myDrawer->SetPointTextureMapOn(toEnableNodesTextures);

    updateElementsVisibility();
    updateElementsSelectability();
    updatePolygonOffsets();
    updateUsedProps();

    const float aTextureTransp           = !aColorMapper.IsNull() ? aColorMapper->ColorTextureTransparency() : 0.0f;
    Handle(Graphic3d_Texture2D) aTexture = !aColorMapper.IsNull() ? aColorMapper->ColorTexture() : nullptr;
    myDrawer->UpdateAspectTexture(myDrawer->FillAreaAspect3d(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->FillAreaAspect2d(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->Elem0dAspect(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->CentroidAspect3d(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->CentroidAspect2d(), aTexture, aTextureTransp);
    myDrawer->UpdateAspectTexture(myDrawer->PointAspect()->Aspect(), aTexture, aTextureTransp);

    Graphic3d_Vec2d aRangeMinMax(-RealLast(), RealLast());
    if (!aColorMapper.IsNull()) {
        Bnd_Range aRange = aColorMapper->ValuesRange();
        aRange.GetBounds(aRangeMinMax[0], aRangeMinMax[1]);
    }
    GetPartParams<MeshPrsMinMaxPartParams>((int)MeshPartType::MinMaxAnnotations)->SetMinMaxRange(aRangeMinMax);
}

// ================================================================
// Function : updateElementsVisibility
// ================================================================
void MeshPresentationScalar::updateElementsVisibility()
{
    const MeshPrsVisScalarProps* aPrsProps = dynamic_cast<MeshPrsVisScalarProps*>(myPrsVisProps.get());

    const bool toShow0d         = aPrsProps->ToShow0d();
    const bool toShow2d         = aPrsProps->ToShow2d();
    const bool toShow3d         = aPrsProps->ToShow3d();
    const bool toShowNodes      = aPrsProps->ToShowNodes();
    const bool toShowCentr      = aPrsProps->ToShowCentroids();
    const bool toShowSharpEdges = aPrsProps->ToShowSharpEdges();
    const bool toShowFreeEdges  = aPrsProps->ToShowFreeEdges();

    myDrawer->FillAreaAspect3d()->SetInteriorStyle(toShow3d ? Aspect_IS_SOLID : Aspect_IS_EMPTY);
    myDrawer->CentroidAspect3d()->SetMarkerType(toShowCentr && toShow3d ? Aspect_TOM_POINT : Aspect_TOM_EMPTY);
    myDrawer->LineAspect3d()->SetLineType(toShow3d ? Aspect_TOL_SOLID : Aspect_TOL_EMPTY);

    myDrawer->FillAreaAspect2d()->SetInteriorStyle(toShow2d ? Aspect_IS_SOLID : Aspect_IS_EMPTY);
    myDrawer->CentroidAspect2d()->SetMarkerType(toShowCentr && toShow2d ? Aspect_TOM_POINT : Aspect_TOM_EMPTY);
    myDrawer->LineAspect2d()->SetLineType(toShow2d ? Aspect_TOL_SOLID : Aspect_TOL_EMPTY);

    myDrawer->PointAspect()->Aspect()->SetMarkerType(toShowNodes ? Aspect_TOM_POINT : Aspect_TOM_EMPTY);
    myDrawer->Elem0dAspect()->SetMarkerType(toShow0d ? Aspect_TOM_PLUS : Aspect_TOM_EMPTY);

    MeshParts& aParts = getParts();

    if (!aPrsProps->IsVisible()) {
        aParts.HideAll();
        return;
    }

    const InteractiveCaeMeshPrsMode aMode = aPrsProps->PresentationMode();

    const bool toShowTris  = aMode != InteractiveCaeMeshPrsMode::MeshEdges;
    const bool toShowEdges = aMode == InteractiveCaeMeshPrsMode::MeshEdges
                          || aMode == InteractiveCaeMeshPrsMode::ShadedMeshEdges;

    aParts(MeshPartType::Triangles3d)->SetShow(toShow3d && toShowTris);
    aParts(MeshPartType::Triangles2d)->SetShow(toShow2d && toShowTris);
    aParts(MeshPartType::Edges3d)->SetShow(toShow3d && toShowEdges);
    aParts(MeshPartType::Edges2d)->SetShow(toShow2d && toShowEdges);
    aParts(MeshPartType::SharpEdges3d)->SetShow(toShow3d && toShowSharpEdges);
    aParts(MeshPartType::SharpEdges2d)->SetShow(toShow2d && toShowSharpEdges);
    aParts(MeshPartType::FreeEdges3d)->SetShow(toShow3d && toShowFreeEdges);
    aParts(MeshPartType::FreeEdges2d)->SetShow(toShow2d && toShowFreeEdges);
    aParts(MeshPartType::Elems0d)->SetShow(toShow0d);
    aParts(MeshPartType::Points)->SetShow(toShowNodes);
    aParts(MeshPartType::Centroids3d)->SetShow(toShow3d && toShowCentr);
    aParts(MeshPartType::Centroids2d)->SetShow(toShow2d && toShowCentr);
    aParts(MeshPartType::MinMaxAnnotations)->SetShow(aPrsProps->ToShowMinMax());
    aParts(MeshPartType::GroupsInfo)->SetShow(aPrsProps->ToShowGroupsInfo());
    aParts(MeshPartType::GeneralCells)->SetShow(true);
}

// ================================================================
// Function : updateElementsSelectability
// ================================================================
void MeshPresentationScalar::updateElementsSelectability()
{
    const MeshPrsVisScalarProps* aPrsProps = dynamic_cast<MeshPrsVisScalarProps*>(myPrsVisProps.get());

    if (!aPrsProps->IsVisible()) {
        mySensitives.DeselectAll();
        return;
    }

    const bool toShow0d    = aPrsProps->ToShow0d();
    const bool toShow2d    = aPrsProps->ToShow2d();
    const bool toShow3d    = aPrsProps->ToShow3d();
    const bool toShowNodes = aPrsProps->ToShowNodes();

    const bool toSelPrs   = (mySelMode & (int)InteractiveCaeMeshSelMode::Presentation) != 0;
    const bool toSelElems = (mySelMode & (int)InteractiveCaeMeshSelMode::Elements) != 0;
    const bool toSelNodes = (mySelMode & (int)InteractiveCaeMeshSelMode::Nodes) != 0;

    mySensitives(MeshSensPartType::Entities3d)->SetSelect(toShow3d && (toSelPrs || toSelElems));
    mySensitives(MeshSensPartType::Entities2d)->SetSelect(toShow2d && (toSelPrs || toSelElems));
    mySensitives(MeshSensPartType::Entities0d)->SetSelect(toShow0d && (toSelPrs || toSelElems));
    mySensitives(MeshSensPartType::Nodes)->SetSelect((toShowNodes && toSelPrs) || toSelNodes);
    mySensitives(MeshSensPartType::Annotations)->SetSelect(getParts()(MeshPartType::GroupsInfo)->ToShow());
}

// ================================================================
// Function : updatePolygonOffsets
// ================================================================
void MeshPresentationScalar::updatePolygonOffsets()
{
    // With enabled polygon offset triangles of 3d elements can overlap with each other on their boundaries.
    // Due to this, multiple line artifacts will appear on the surface of 3D mesh.
    // So, polygon offset should be disabled for simple shading and enabled for edge shading
    // (because edges should be drawn above a surface and line artifacts are hidden under edges in this case).
    const MeshPrsVisScalarProps*    aPrsProps = dynamic_cast<MeshPrsVisScalarProps*>(myPrsVisProps.get());
    const InteractiveCaeMeshPrsMode aPrsMode  = aPrsProps->PresentationMode();

    MeshParts& aParts = getParts();

    bool isEdgeShading = aPrsMode == InteractiveCaeMeshPrsMode::ShadedMeshEdges;
    bool isShrinked    = aPrsMode == InteractiveCaeMeshPrsMode::ShadedMeshShrinked;
    bool isFreeEdges   = aParts(MeshPartType::FreeEdges3d)->ToShow() || aParts(MeshPartType::FreeEdges2d)->ToShow();
    bool isSharpEdges  = aParts(MeshPartType::SharpEdges3d)->ToShow() || aParts(MeshPartType::SharpEdges2d)->ToShow();
    myDrawer->SetPolygonOffsetOn(isEdgeShading || isShrinked || isFreeEdges || isSharpEdges);
}

// ================================================================
// Function : updateUsedProps
// ================================================================
void MeshPresentationScalar::updateUsedProps()
{
    for (int i = 0; i < MeshNbPartTypes; i++) {
        myUsedPerPartProps[i].Clear();
    }

    RWCae::MeshPropertyComponent aProp = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                : RWCae::MeshPropertyComponent();
    if (aProp.IsDefined()) {
        const TCollection_AsciiString&       aSrcPropId = aProp.Property->Id();
        std::vector<TCollection_AsciiString> aPropsToAdd{aSrcPropId};
        addDependencyPropIds(aPropsToAdd, {aProp.Property});

        for (const TCollection_AsciiString& aPropId : aPropsToAdd) {
            addUsedPerPartProp(MeshPartType::Triangles3d, aPropId);
            addUsedPerPartProp(MeshPartType::Triangles2d, aPropId);
            addUsedPerPartProp(MeshPartType::GeneralCells, aPropId);
            addUsedPerPartProp(MeshPartType::MinMaxAnnotations, aPropId);
            if (aProp.Property->IsNodal()) {
                addUsedPerPartProp(MeshPartType::Points, aPropId);
            } else {
                addUsedPerPartProp(MeshPartType::Elems0d, aPropId);
                addUsedPerPartProp(MeshPartType::Centroids2d, aPropId);
                addUsedPerPartProp(MeshPartType::Centroids3d, aPropId);
            }
            if (aProp.Property->HasGroupComponents()) {
                addUsedPerPartProp(MeshPartType::GroupsInfo, aPropId);
            }
        }
    }

    Handle(RWCae::MeshNVecProperty) aDeformedProp;
    Handle(RWCae::MeshNVecProperty) anIsoProp;

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            aDeformedProp = !aDefGeom->DeformedProperty().IsNull() && aDefGeom->DeformedProperty()->IsVector3()
                                 && aDefGeom->DeformedProperty()->IsNodal()
                              ? aDefGeom->DeformedProperty()
                              : nullptr;
        } else if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            anIsoProp = anIsoGeom->Property().IsDefined() ? anIsoGeom->Property().Property : nullptr;
        }
    }

    if (!aDeformedProp.IsNull()) {
        const TCollection_AsciiString& aPropId = aDeformedProp->Id();
        for (MeshPartType aTypeIter : PARTS_LIST) {
            addUsedPerPartProp(aTypeIter, aPropId);
        }
    }

    if (!anIsoProp.IsNull()) {
        const TCollection_AsciiString& aPropId = anIsoProp->Id();
        for (MeshPartType aTypeIter : PARTS_LIST) {
            addUsedPerPartProp(aTypeIter, aPropId);
        }
    }
}

// ================================================================
// Function : updateParts
// ================================================================
void MeshPresentationScalar::updateParts(MeshParts& theParts)
{
    for (MeshPartType aTypeIter : PARTS_LIST) {
        theParts(aTypeIter)->Update(this);
    }
}

// ================================================================
// Function : needToUpdateParts
// ================================================================
bool MeshPresentationScalar::needToUpdateParts(const MeshParts& theParts) const
{
    for (MeshPartType aTypeIter : PARTS_LIST) {
        if (theParts(aTypeIter)->NeedToUpdate()) {
            return true;
        }
    }

    return false;
}

// ================================================================
// Function : updateSensitiveSets
// ================================================================
void MeshPresentationScalar::updateSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    if (!myToUseGenGeom) {
        Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = myGeometry->GetLastNotModifierChild();
        mySensitives(MeshSensPartType::Entities3d)->Update(theOwner, this, aMeshGeom);
        mySensitives(MeshSensPartType::Entities2d)->Update(theOwner, this, aMeshGeom);
        mySensitives(MeshSensPartType::Entities0d)->Update(theOwner, this, aMeshGeom);
        mySensitives(MeshSensPartType::Nodes)->Update(theOwner, this, aMeshGeom);
        mySensitives(MeshSensPartType::Annotations)->Update(theOwner, this, aMeshGeom);
    } else {
        mySensitives(MeshSensPartType::Entities3d)
            ->Update(theOwner, this, myGenGeometry.Find((int)MeshPartType::Triangles3d));
        mySensitives(MeshSensPartType::Entities2d)
            ->Update(theOwner, this, myGenGeometry.Find((int)MeshPartType::Triangles2d));
        mySensitives(MeshSensPartType::Entities0d)
            ->Update(theOwner, this, myGenGeometry.Find((int)MeshPartType::Elems0d));
        mySensitives(MeshSensPartType::Nodes)->Update(theOwner, this, myGenGeometry.Find((int)MeshPartType::Points));
        mySensitives(MeshSensPartType::Annotations)->Update(theOwner, this, nullptr);
    }
}

// ================================================================
// Function : ComputeMinMaxPoints
// ================================================================
bool MeshPresentationScalar::ComputeMinMaxPoints(NCollection_Vec2<Graphic3d_Vec3>& theMinMaxPnts,
                                                 NCollection_Vec2<double>&         theMinMaxValues) const
{
    RWCae::MeshPropertyComponent aComp        = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                       : RWCae::MeshPropertyComponent();
    Handle(MeshColorMapper)        aColorMapper = !myPropData.IsNull() ? myPropData->ColorMapper(aComp) : nullptr;
    if (aColorMapper.IsNull()) {
        return false;
    }

    Bnd_Range aRange = aColorMapper->ValuesRange();
    if (aRange.IsVoid()) {
        return false;
    }
    Graphic3d_Vec2d aRangeMinMax;
    aRange.GetBounds(aRangeMinMax[0], aRangeMinMax[1]);

    Handle(ViewCaeGeom::MeshGeometry) aGeom = !myToUseGenGeom ? myGeometry->GetLastNotModifierChild()
                                                               : GetGenGeometry((int)MeshPartType::MinMaxAnnotations);

    return ComputeMinMaxPoints(aGeom, myGeomScalarProps, aRangeMinMax, theMinMaxPnts, theMinMaxValues);
}

// ================================================================
// Function : ComputeMinMaxPoints
// ================================================================
bool MeshPresentationScalar::ComputeMinMaxPoints(const Handle(ViewCaeGeom::MeshGeometry)&     theGeom,
                                                 const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                                 const Graphic3d_Vec2d&                        theRange,
                                                 NCollection_Vec2<Graphic3d_Vec3>&             theMinMaxPnts,
                                                 NCollection_Vec2<double>&                     theMinMaxValues)
{
    theMinMaxValues[0] = RealLast();
    theMinMaxValues[1] = -RealLast();

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theGeom.get())) {
        const Handle(RWCae::MeshData)& aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        RWCae::MeshPropertyComponent aProp;
        if (!theGeomProps.Find(aGeom->Id(), aProp) || aProp.Property.IsNull()) {
            continue;
        }
        const Handle(RWCae::MeshScalarPropertyBase) aScalarProp = aProp.GetScalarComp();

        if (aScalarProp->IsNodal()) {
            computeMinMaxPointsNodal(aMeshData, aGeom, aScalarProp, theMinMaxPnts, theMinMaxValues, theRange);
        } else {
            computeMinMaxPointsElemental(aMeshData, aGeom, aScalarProp, theMinMaxPnts, theMinMaxValues, theRange);
        }
    }

    return theMinMaxValues[0] <= theMinMaxValues[1];
}

// ================================================================
// Function : computeMinMaxPointsNodal
// ================================================================
void MeshPresentationScalar::computeMinMaxPointsNodal(const Handle(RWCae::MeshData)&               theMeshData,
                                                      const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                      const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                      NCollection_Vec2<Graphic3d_Vec3>&              theMinMaxPnts,
                                                      NCollection_Vec2<double>&                      theMinMaxValues,
                                                      const Graphic3d_Vec2d&                         theRangeMinMax)
{
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < theMeshData->NbNodes(); aNodeIter++) {
        Graphic3d_Vec3 aNode = theMeshData->Nodes()[aNodeIter];
        theGeom->ModifyNode(aNodeIter, aNode);
        if (!theGeom->AcceptNode(aNodeIter, aNode)) {
            continue;
        }
        const double aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(aNodeIter));
        if (RWCae::IsNaN(aValue)) {
            continue;
        }

        if (aValue < theMinMaxValues[0] && aValue >= theRangeMinMax[0]) {
            theMinMaxValues[0] = aValue;
            theMinMaxPnts[0]   = aNode;
        }

        if (aValue > theMinMaxValues[1] && aValue <= theRangeMinMax[1]) {
            theMinMaxValues[1] = aValue;
            theMinMaxPnts[1]   = aNode;
        }
    }
}

// ================================================================
// Function : computeMinMaxPointsElemental
// ================================================================
void MeshPresentationScalar::computeMinMaxPointsElemental(const Handle(RWCae::MeshData)&               theMeshData,
                                                          const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                          const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                          NCollection_Vec2<Graphic3d_Vec3>&              theMinMaxPnts,
                                                          NCollection_Vec2<double>& theMinMaxValues,
                                                          const Graphic3d_Vec2d&    theRangeMinMax)
{
    auto compute = [&theGeom, &theProp, &theMinMaxPnts, &theMinMaxValues, &theRangeMinMax](const auto& theAdaptor) {
        computeMinMaxPointsElementalT(theAdaptor, theGeom, theProp, theMinMaxPnts, theMinMaxValues, theRangeMinMax);
    };

    RWCae::MeshAdaptorsIteratorAllElems::For(compute, theMeshData);
}

// ================================================================
// Function : computeMinMaxPointsT
// ================================================================
template<class Elem_T>
void MeshPresentationScalar::computeMinMaxPointsElementalT(const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                                           const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                           const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                           NCollection_Vec2<Graphic3d_Vec3>&              theMinMaxPnts,
                                                           NCollection_Vec2<double>& theMinMaxValues,
                                                           const Graphic3d_Vec2d&    theRangeMinMax)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return;
    }

    Handle(RWCae::MeshNodeVecProperty) aNodeVecProp = Handle(RWCae::MeshNodeVecProperty)::DownCast(theProp);
    bool                                 isNodeVec    = !aNodeVecProp.IsNull();

    NCollection_Vec2<int> aResMinMaxIdx(-1);

    auto checkMinMax = [&](int theIndex, int theSubIndex) {
        const double aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(theIndex, theSubIndex));
        if (RWCae::IsNaN(aValue)) {
            return;
        }

        if (aValue < theMinMaxValues[0] && aValue >= theRangeMinMax[0]) {
            theMinMaxValues[0] = aValue;
            aResMinMaxIdx[0]   = theSubIndex;
        }

        if (aValue > theMinMaxValues[1] && aValue <= theRangeMinMax[1]) {
            theMinMaxValues[1] = aValue;
            aResMinMaxIdx[1]   = theSubIndex;
        }
    };

    for (RWCae::MeshEntityId anElemIdx = 0; anElemIdx < aNbElems; anElemIdx++) {
        const Elem_T& anElem = theMesh.GetElement(anElemIdx);
        if (!theGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        if (!isNodeVec) {
            checkMinMax(anElem.GlobalIndex(), anElemIdx);
            continue;
        }

        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
            checkMinMax(anElem.GlobalIndex(), anElem[aNodeIter]);
        }
    }

    generateMinMaxPointsElementalT(theMesh, theGeom, theMinMaxPnts, aResMinMaxIdx, isNodeVec);
}

// ================================================================
// Function : generateMinMaxPointsElementalT
// ================================================================
template<class Elem_T>
void MeshPresentationScalar::generateMinMaxPointsElementalT(const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                                            const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                            NCollection_Vec2<Graphic3d_Vec3>&         theMinMaxPnts,
                                                            const NCollection_Vec2<int>&              theResMinMaxIdx,
                                                            bool                                      isNodeVec)
{
    for (int i = 0; i < 2; i++) {
        if (theResMinMaxIdx[i] == -1) {
            continue;
        }
        Graphic3d_Vec3 aResPnt(0.0);
        if (isNodeVec) {
            theMesh.NodePosition(aResPnt, theResMinMaxIdx[i]);
            theGeom->ModifyNode(theResMinMaxIdx[i], aResPnt);
            theMinMaxPnts[i] = aResPnt;
            continue;
        }

        Graphic3d_Vec3 aNode;
        int            aNbNodes = 0;
        const Elem_T&  anElem   = theMesh.GetElement(theResMinMaxIdx[i]);
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
            RWCae::MeshEntityId aNodeIdx = anElem[aNodeIter];

            theMesh.NodePosition(aNode, aNodeIdx);
            theGeom->ModifyNode(aNodeIdx, aNode);
            aResPnt += aNode;
            aNbNodes++;
        }

        if (aNbNodes > 0) {
            aResPnt /= static_cast<float>(aNbNodes);
        }

        theMinMaxPnts[i] = aResPnt;
    }
}

// ================================================================
// Function : FillGroupsInfo
// ================================================================
void MeshPresentationScalar::FillGroupsInfo()
{
    const Handle(MeshPartGroupsInfoAnnotations)& aGroupsInfo = Handle(MeshPartGroupsInfoAnnotations)::DownCast(
        getParts()(MeshPartType::GroupsInfo));
    aGroupsInfo->Clear();

    RWCae::MeshPropertyComponent     aProp      = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                         : RWCae::MeshPropertyComponent();
    Handle(RWCae::MeshGroupProperty) aGroupProp = Handle(RWCae::MeshGroupProperty)::DownCast(aProp.GetScalarComp());
    if (aGroupProp.IsNull()) {
        return;
    }

    std::map<int, std::string> aUsedGroups = aGroupProp->GetUsedGroups();
    for (const std::pair<const int, std::string>& aGroup : aUsedGroups) {
        MeshAnnotation anAnnotation(gp_Pnt(), (int)MeshAnnotationType::Text, 1);
        anAnnotation.SetText(aGroup.second.c_str());
        anAnnotation.SetIndex(aGroup.first);
        aGroupsInfo->Annotations().AddAnnotation(anAnnotation);
    }
}

// ================================================================
// Function : ComputeGroupsAreas
// ================================================================
void MeshPresentationScalar::ComputeGroupsAreas(MapOfGroupAreas& theAreas) const
{
    Handle(ViewCaeGeom::MeshGeometry) aGeom = myGeometry;
    RWCae::MeshPropertyComponent     aProp = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                    : RWCae::MeshPropertyComponent();
    if (myToUseGenGeom) {
        aGeom          = GetGenGeometry((int)MeshPartType::GroupsInfo);
        aProp.Property = !aGeom.IsNull() ? aGeom->CalculateProperty(aProp.Property) : nullptr;
    }
    ComputeGroupsAreas(aGeom, aProp, theAreas);
}

// ================================================================
// Function : ComputeGroupsAreas
// ================================================================
void MeshPresentationScalar::ComputeGroupsAreas(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const RWCae::MeshPropertyComponent&     theProp,
                                                MapOfGroupAreas&                          theAreas)
{
    const Handle(RWCae::MeshData)& aMeshData = theGeom->GetMeshData();
    if (aMeshData.IsNull()) {
        return;
    }

    Handle(RWCae::MeshGroupProperty) aGroupProp = Handle(RWCae::MeshGroupProperty)::DownCast(
        theProp.GetScalarComp());
    if (aGroupProp.IsNull()) {
        return;
    }

    // vector<vector<int>> seems to be more efficient than vector<map<int>>
    // even if it contains the same nodes several times
    std::vector<std::vector<RWCae::MeshEntityId>> aConnectedPoints(aMeshData->NbNodes(),
                                                                     std::vector<RWCae::MeshEntityId>());

    Handle(RWCae::MeshGroupProperty) aNodalProp;
    generateConnectedPoints(aMeshData, aGroupProp, aNodalProp, aConnectedPoints);

    std::map<int, std::string> aUsedGroups = aGroupProp->GetUsedGroups();
    for (const std::pair<const int, std::string>& aGroup : aUsedGroups) {
        std::vector<bool> aProcessedPoints(aMeshData->NbNodes(), false);

        NCollection_List<TColStd_PackedMapOfInteger>* aGroupsOfNodes =
            theAreas.Bound(aGroup.first, NCollection_List<TColStd_PackedMapOfInteger>());
        NCollection_List<RWCae::MeshEntityId> aQueuedNodes;

        for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aMeshData->NbNodes(); aNodeIter++) {
            if (aProcessedPoints[aNodeIter]) {
                continue;
            }

            RWCae::MeshGroupProperty::MeshGroupsMaskType aNodeGroups = aNodalProp->GetGroups(aNodeIter);
            if (!RWCae::MeshGroupProperty::IsInGroup(aNodeGroups, aGroup.first)) {
                continue;
            }

            aQueuedNodes.Append(aNodeIter);
            aProcessedPoints[aNodeIter] = true;

            TColStd_PackedMapOfInteger& aCurGroupOfNodes = aGroupsOfNodes->Append(TColStd_PackedMapOfInteger());
            processGroupNodes(aGroup.first,
                              aNodalProp,
                              aQueuedNodes,
                              aCurGroupOfNodes,
                              aConnectedPoints,
                              aProcessedPoints);
        }
    }
}

// ================================================================
// Function : generateConnectedPoints
// ================================================================
void MeshPresentationScalar::generateConnectedPoints(
    const Handle(RWCae::MeshData)&                 theMeshData,
    const Handle(RWCae::MeshGroupProperty)&        theProp,
    Handle(RWCae::MeshGroupProperty)&              theNodalProp,
    std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints)
{
    if (theProp->IsElemental()) {
        theNodalProp = new RWCae::MeshGroupProperty(theProp->Name(), RWCae::MeshPropertyBase::MappingType::Nodal);
        theNodalProp->Init(theMeshData->NbNodes());
    } else {
        theNodalProp = theProp;
    }

    auto compute = [&theProp, &theNodalProp, &theConnectedPoints](const auto& theAdaptor) {
        generateConnectedPointsT(theAdaptor, theProp, theNodalProp, theConnectedPoints);
    };

    RWCae::MeshAdaptorsIteratorAllElems::For(compute, theMeshData);
}

// ================================================================
// Function : generateConnectedPointsT
// ================================================================
template<class Elem_T>
void MeshPresentationScalar::generateConnectedPointsT(
    const RWCae::MeshAdaptor<Elem_T>&              theMesh,
    const Handle(RWCae::MeshGroupProperty)&        theProp,
    const Handle(RWCae::MeshGroupProperty)&        theNodalProp,
    std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return;
    }

    auto processElemNode = [&](const Elem_T& theElem, RWCae::MeshEntityId theNodeIdx) {
        std::vector<RWCae::MeshEntityId>& aConnectedPoints = theConnectedPoints[theNodeIdx];

        for (RWCae::MeshElemEntityId aNodeToAddIter = 0; aNodeToAddIter < theElem.NbNodes(); aNodeToAddIter++) {
            RWCae::MeshEntityId aNodeToAddIdx = theElem[aNodeToAddIter];
            if (aNodeToAddIdx != theNodeIdx) {
                aConnectedPoints.push_back(aNodeToAddIdx);
            }
        }
    };

    for (RWCae::MeshEntityId anElemIdx = 0; anElemIdx < aNbElems; anElemIdx++) {
        const Elem_T& anElem = theMesh.GetElement(anElemIdx);

        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
            RWCae::MeshEntityId aNodeIdx = anElem[aNodeIter];

            processElemNode(anElem, aNodeIdx);

            if (theProp->IsElemental()) {
                RWCae::MeshGroupProperty::MeshGroupsMaskType aGroup     = theProp->GetGroups(anElem.GlobalIndex());
                RWCae::MeshGroupProperty::MeshGroupsMaskType aPrevGroup = theNodalProp->GetGroups(aNodeIdx);
                theNodalProp->SetGroups(aNodeIdx, aPrevGroup | aGroup, false, false);
            }
        }
    }
}

// ================================================================
// Function : processGroupNodes
// ================================================================
void MeshPresentationScalar::processGroupNodes(
    int                                                    theGroupId,
    const Handle(RWCae::MeshGroupProperty)&              theProp,
    NCollection_List<RWCae::MeshEntityId>&               theQueuedNodes,
    TColStd_PackedMapOfInteger&                            theGroupOfNodes,
    const std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints,
    std::vector<bool>&                                     theProcessedPoints)
{
    while (!theQueuedNodes.IsEmpty()) {
        RWCae::MeshEntityId aCurNode = theQueuedNodes.First();
        theGroupOfNodes.Add(aCurNode);
        theQueuedNodes.RemoveFirst();

        const std::vector<RWCae::MeshEntityId>& aConnectedPoints = theConnectedPoints[aCurNode];
        for (RWCae::MeshEntityId aPntIdx : aConnectedPoints) {
            RWCae::MeshGroupProperty::MeshGroupsMaskType aGroup = theProp->GetGroups(aPntIdx);
            if (!RWCae::MeshGroupProperty::IsInGroup(aGroup, theGroupId)) {
                continue;
            }
            if (theProcessedPoints[aPntIdx]) {
                continue;
            }

            theQueuedNodes.Append(aPntIdx);
            theProcessedPoints[aPntIdx] = true;
        }
    }
}

// ================================================================
// Function : GetGroupCenterNode
// ================================================================
int MeshPresentationScalar::GetGroupCenterNode(const TColStd_PackedMapOfInteger& theGroup,
                                               const Handle(RWCae::MeshData)&  theMeshData)
{
    Graphic3d_Vec3 aCenter(0.0f);
    for (TColStd_PackedMapOfInteger::Iterator aNodeIter(theGroup); aNodeIter.More(); aNodeIter.Next()) {
        aCenter += theMeshData->Nodes()[aNodeIter.Key()];
    }
    aCenter /= static_cast<float>(theGroup.Extent());
    int   aMinNodeIdx = -1;
    float aMinDist    = ShortRealLast();
    for (TColStd_PackedMapOfInteger::Iterator aNodeIter(theGroup); aNodeIter.More(); aNodeIter.Next()) {
        float aDist = (theMeshData->Nodes()[aNodeIter.Key()] - aCenter).SquareModulus();
        if (aDist < aMinDist) {
            aMinDist    = aDist;
            aMinNodeIdx = aNodeIter.Key();
        }
    }
    return aMinNodeIdx;
}

// ================================================================
// Function : OnPropertiesUpdated
// ================================================================
void MeshPresentationScalar::OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                                 bool theCachedProps)
{
    MeshPresentation::OnPropertiesUpdated(theProps, theCachedProps);

    Handle(RWCae::MeshNVecProperty) aProp = myPropData->Property(myPropIndex).Property;
    if (aProp.IsNull() || !theProps.Contains(aProp)) {
        return;
    }

    MeshParts* aPart = myPrimitives.ChangeSeek(myActiveTimeStep);
    if (aPart == nullptr) {
        return;
    }

    std::vector<MeshPartType> aPartsToUpdate;
    aPartsToUpdate.push_back(MeshPartType::Triangles3d);
    aPartsToUpdate.push_back(MeshPartType::Triangles2d);
    aPartsToUpdate.push_back(MeshPartType::GeneralCells);
    aPartsToUpdate.push_back(MeshPartType::MinMaxAnnotations);
    if (aProp->IsNodal()) {
        aPartsToUpdate.push_back(MeshPartType::Points);
    } else {
        aPartsToUpdate.push_back(MeshPartType::Elems0d);
        aPartsToUpdate.push_back(MeshPartType::Centroids2d);
        aPartsToUpdate.push_back(MeshPartType::Centroids3d);
    }

    for (MeshPartType aPartType : aPartsToUpdate) {
        (*aPart)(aPartType)->Clear();
        SetToUpdateGenGeometry((int)aPartType);
    }
}

} // namespace
