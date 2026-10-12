
#include <ViewCae/MeshPart.h>
#include <ViewCae/MeshPartGeneralCells.h>

#include <ViewCae/CaeMeshStreamlinesBuilder.h>
#include <ViewCae/CaeMeshTools.h>
#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationStreamlines.h>
#include <ViewCae/MeshPresentationVector.h>

#include <ViewBase/UnitManager.h>
#include <ViewBase/BaseDefaultDrawer.h>

#include <OpenGl_Group.hxx>
#include <OpenGl_Structure.hxx>

namespace ViewCae {
// ================================================================
// Function : MeshParts
// ================================================================
MeshParts::MeshParts()
{
    myParts.resize(MeshNbPartTypes);

    myParts[(int)MeshPartType::Triangles3d]       = new MeshPartPrimArray<MeshPartType::Triangles3d>();
    myParts[(int)MeshPartType::Triangles2d]       = new MeshPartPrimArray<MeshPartType::Triangles2d>();
    myParts[(int)MeshPartType::Edges3d]           = new MeshPartPrimArray<MeshPartType::Edges3d>();
    myParts[(int)MeshPartType::Edges2d]           = new MeshPartPrimArray<MeshPartType::Edges2d>();
    myParts[(int)MeshPartType::SharpEdges3d]      = new MeshPartPrimArray<MeshPartType::SharpEdges3d>();
    myParts[(int)MeshPartType::SharpEdges2d]      = new MeshPartPrimArray<MeshPartType::SharpEdges2d>();
    myParts[(int)MeshPartType::FreeEdges3d]       = new MeshPartPrimArray<MeshPartType::FreeEdges3d>();
    myParts[(int)MeshPartType::FreeEdges2d]       = new MeshPartPrimArray<MeshPartType::FreeEdges2d>();
    myParts[(int)MeshPartType::Points]            = new MeshPartPrimArray<MeshPartType::Points>();
    myParts[(int)MeshPartType::Elems0d]           = new MeshPartPrimArray<MeshPartType::Elems0d>();
    myParts[(int)MeshPartType::Centroids3d]       = new MeshPartPrimArray<MeshPartType::Centroids3d>();
    myParts[(int)MeshPartType::Centroids2d]       = new MeshPartPrimArray<MeshPartType::Centroids2d>();
    myParts[(int)MeshPartType::Vectors]           = new MeshPartVectors();
    myParts[(int)MeshPartType::MinMaxAnnotations] = new MeshPartAnnotations<MeshPartType::MinMaxAnnotations>();
    myParts[(int)MeshPartType::GroupsInfo]        = new MeshPartAnnotations<MeshPartType::GroupsInfo>();
    myParts[(int)MeshPartType::Streamlines]       = new MeshPartStreamlines();
    myParts[(int)MeshPartType::GeneralCells]      = new MeshPartGeneralCells();
}

// ================================================================
// Function : ~MeshParts
// ================================================================
MeshParts::~MeshParts()
{
    //
}

// ================================================================
// Function : HideAll
// ================================================================
void MeshParts::HideAll()
{
    for (const Handle(MeshPart)& aPart : myParts) {
        aPart->SetShow(false);
    }
}

// ================================================================
// Function : ClearAll
// ================================================================
void MeshParts::ClearAll()
{
    for (int i = 0; i < MeshNbPartTypes; i++) {
        if (i == (int)MeshPartType::GroupsInfo) {
            continue;
        }

        myParts[i]->Clear();
    }
}

//! @name MeshPartPrimArray.
//! @{

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Triangles3d>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar*   aPrs   = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());
    Handle(MeshPrsVisScalarProps)   aProps = aPrs->ScalarVisProps();
    const InteractiveCaeMeshPrsMode aMode  = aProps->PresentationMode();

    constexpr float aShrinkDef  = 0.8f;
    const float     aShrink     = aMode == InteractiveCaeMeshPrsMode::ShadedMeshShrinked ? aShrinkDef : 1.0f;
    MeshShadedType  aShadedType = aProps->IsVolumeAsSurface() ? MeshShadedType::VOLUME_SURFACE : MeshShadedType::VOLUME;

    myPrimArray = CaeMeshTools::CreateShadedMesh(aPrs->GetMeshGeometry((int)MeshPartType::Triangles3d),
                                                 aPrs->GetGeomScalarProps(),
                                                 false,
                                                 aShadedType,
                                                 aShrink);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Triangles3d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Volume,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Triangles2d>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar*   aPrs  = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());
    const InteractiveCaeMeshPrsMode aMode = aPrs->ScalarVisProps()->PresentationMode();

    constexpr float aShrinkDef  = 0.8f;
    const float     aShrink     = aMode == InteractiveCaeMeshPrsMode::ShadedMeshShrinked ? aShrinkDef : 1.0f;
    MeshShadedType  aShadedType = MeshShadedType::FACES;

    myPrimArray = CaeMeshTools::CreateShadedMesh(aPrs->GetMeshGeometry((int)MeshPartType::Triangles2d),
                                                 aPrs->GetGeomScalarProps(),
                                                 false,
                                                 aShadedType,
                                                 aShrink);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Triangles2d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Face,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Edges3d>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar* aPrs   = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());
    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();

    MeshShadedType aShadedType = aProps->IsVolumeAsSurface() ? MeshShadedType::VOLUME_SURFACE : MeshShadedType::VOLUME;

    myPrimArray = CaeMeshTools::CreateMeshEdges(aPrs->GetMeshGeometry((int)MeshPartType::Edges3d), aShadedType);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Edges3d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                               const Handle(MeshPrsPartParams)&          theParams,
                                                               const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Volume,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Edges2d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateMeshEdges(thePrs->GetMeshGeometry((int)MeshPartType::Edges2d),
                                                MeshShadedType::FACES);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Edges2d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                               const Handle(MeshPrsPartParams)&          theParams,
                                                               const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Face,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::SharpEdges3d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateMeshSharpEdges(thePrs->GetMeshGeometry((int)MeshPartType::SharpEdges3d),
                                                     MeshShadedType::VOLUME_SURFACE);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::SharpEdges3d>::ExtractMeshData(
    const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
    const Handle(MeshPrsPartParams)&          theParams,
    const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Volume,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::SharpEdges2d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateMeshSharpEdges(thePrs->GetMeshGeometry((int)MeshPartType::SharpEdges2d),
                                                     MeshShadedType::FACES);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::SharpEdges2d>::ExtractMeshData(
    const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
    const Handle(MeshPrsPartParams)&          theParams,
    const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Face,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::FreeEdges3d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateMeshFreeEdges(thePrs->GetMeshGeometry((int)MeshPartType::FreeEdges3d),
                                                    MeshShadedType::VOLUME_SURFACE);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::FreeEdges3d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Volume,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::FreeEdges2d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateMeshFreeEdges(thePrs->GetMeshGeometry((int)MeshPartType::FreeEdges2d),
                                                    MeshShadedType::FACES);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::FreeEdges2d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Face,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Points>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar* aPrs   = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());
    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    bool toShowFreeNodes = (aProps->ElementTypes() & (unsigned int)InteractiveCaeMeshElementType::FreeNode) != 0;

    myPrimArray = CaeMeshTools::CreateNodes(aPrs->GetMeshGeometry((int)MeshPartType::Points),
                                            aPrs->GetGeomScalarProps(),
                                            toShowFreeNodes);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Points>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                              const Handle(MeshPrsPartParams)&          theParams,
                                                              const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractPoints(theGeom, theParams->Property(), theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Elems0d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateElems0d(thePrs->GetMeshGeometry((int)MeshPartType::Elems0d),
                                              thePrs->GetGeomScalarProps());
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Elems0d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                               const Handle(MeshPrsPartParams)&          theParams,
                                                               const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Element0d,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Centroids3d>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar* aPrs   = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());
    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    MeshShadedType aShadedType = aProps->IsVolumeAsSurface() ? MeshShadedType::VOLUME_SURFACE : MeshShadedType::VOLUME;
    myPrimArray                = CaeMeshTools::CreateCentroids(aPrs->GetMeshGeometry((int)MeshPartType::Centroids3d),
                                                aPrs->GetGeomScalarProps(),
                                                aShadedType);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Centroids3d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Volume,
                                                   theGenGeom);
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartPrimArray<MeshPartType::Centroids2d>::update(const Handle(MeshPresentation)& thePrs)
{
    myPrimArray = CaeMeshTools::CreateCentroids(thePrs->GetMeshGeometry((int)MeshPartType::Centroids2d),
                                                thePrs->GetGeomScalarProps(),
                                                MeshShadedType::FACES);
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartPrimArray<MeshPartType::Centroids2d>::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                                   const Handle(MeshPrsPartParams)&          theParams,
                                                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    return ViewCae::CaeMeshTools::ExtractElements(theGeom,
                                                   theParams->Property(),
                                                   InteractiveCaeMeshElementType::Face,
                                                   theGenGeom);
}

// ================================================================
// Function : compute
// ================================================================
template<MeshPartType T>
void MeshPartPrimArray<T>::compute(const Handle(AIS_InteractiveContext)&,
                                   const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                                   const Handle(Prs3d_Presentation)&       thePrs,
                                   const Handle(Prs3d_Presentation)&,
                                   float) const
{
    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    if (MeshPartPrimArray<T>::Type() == MeshPartType::Triangles3d) {
        // need to check also CAE clipping options
        // @c aGroup->SetClosed(true);
    }
    aGroup->SetPrimitivesAspect(theDrawer->Aspect(T));
    aGroup->AddPrimitiveArray(myPrimArray);
}

//! @}

//! @name MeshPartVectors.
//! @{

// ================================================================
// Function : update
// ================================================================
void MeshPartVectors::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationVector*   aPrs    = dynamic_cast<const MeshPresentationVector*>(thePrs.get());
    Handle(MeshPrsVectorPartParams) aParams = aPrs->VectorPartParams();

    myVectors = new MeshVectorsPrimitiveArray(aPrs->GetMeshGeometry((int)MeshPartType::Vectors),
                                              aPrs->GetGeomScalarProps(),
                                              aPrs->GetGeomVecProps(),
                                              aParams->VectorDistrType(),
                                              aPrs->EffectiveScale(),
                                              aParams->SamplingRatio());
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
bool MeshPartVectors::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                      const Handle(MeshPrsPartParams)&          theParams,
                                      const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    const MeshPrsVectorPartParams* aParams = dynamic_cast<const MeshPrsVectorPartParams*>(theParams.get());
    if (aParams == nullptr) {
        return false;
    }

    Handle(RWCae::MeshData) aRes = MeshVectorsPrimitiveArray::ExtractVectorNodes(theGeom,
                                                                                   aParams->Property(),
                                                                                   aParams->VectorProperty(),
                                                                                   aParams->VectorDistrType(),
                                                                                   aParams->SamplingRatio());
    if (aRes.IsNull()) {
        return false;
    }

    theGenGeom->SetMeshData(aRes);
    return true;
}

// ================================================================
// Function : compute
// ================================================================
void MeshPartVectors::compute(const Handle(AIS_InteractiveContext)&,
                              const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                              const Handle(Prs3d_Presentation)&       thePrs,
                              const Handle(Prs3d_Presentation)&,
                              float) const
{
    Handle(OpenGl_Group) aGroup = Handle(OpenGl_Group)::DownCast(thePrs->NewGroup());
    aGroup->SetGroupPrimitivesAspect(theDrawer->Aspect(MeshPartType::Vectors));
    aGroup->AddElement(new MeshVectorsElement(myVectors));
    const Bnd_Box aBox = myVectors->BndBox();
    if (!aBox.IsVoid()) {
        aGroup->SetMinMaxValues(aBox.CornerMin().X(),
                                aBox.CornerMin().Y(),
                                aBox.CornerMin().Z(),
                                aBox.CornerMax().X(),
                                aBox.CornerMax().Y(),
                                aBox.CornerMax().Z());
    }
}

//! @}

//! @name MeshPartAnnotations.
//! @{

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartAnnotations<MeshPartType::MinMaxAnnotations>::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationScalar* aPrs = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());

    myAnnotations.Clear();
    NCollection_Vec2<Graphic3d_Vec3> aMinMaxPnts;
    NCollection_Vec2<double>         aMinMaxValues;
    if (!aPrs->ComputeMinMaxPoints(aMinMaxPnts, aMinMaxValues)) {
        return;
    }

    const Handle(ViewBase::UnitManager)& aUnitMgr = ViewBase::UnitManager::Instance();
    const TCollection_AsciiString&        aUnits =
        aPrs->PropertyData()->Property(aPrs->ActiveProperty()).GetScalarComp()->DisplayUnits();

    constexpr int aNbPass = 2;

    bool isEqual = false;

    for (int i = 0; i < aNbPass && !isEqual; i++) {
        TCollection_AsciiString aText;
        if (i == 0 && aMinMaxPnts[0].IsEqual(aMinMaxPnts[1])) {
            aText   = "MinMax";
            isEqual = true;
        } else {
            aText = (i == 0) ? "Min" : "Max";
        }
        if (!aUnits.IsEmpty()) {
            aMinMaxValues[i] = aUnitMgr->ConvertAnyFromSI(aMinMaxValues[i], aUnits);
        }

        std::stringstream aBuf;
        aBuf.precision(ViewBase::BaseDefaultDrawer::m_defFloatPrec);
        aBuf.imbue(std::locale::classic()); // use always C locale
        aBuf << std::defaultfloat << aMinMaxValues[i];

        aText = aText + ": " + aBuf.str().c_str();
        if (!aUnits.IsEmpty()) {
            aText = aText + " " + aUnitMgr->FormatUnitName(aUnits);
        }
        MeshAnnotation anAnnotation(gp_Pnt(aMinMaxPnts[i].x(), aMinMaxPnts[i].y(), aMinMaxPnts[i].z()),
                                    (int)MeshAnnotationType::Text,
                                    1);
        anAnnotation.SetText(aText);
        myAnnotations.AddAnnotation(anAnnotation);
    }
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartAnnotations<MeshPartType::MinMaxAnnotations>::ExtractMeshData(
    const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
    const Handle(MeshPrsPartParams)&          theParams,
    const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    const MeshPrsMinMaxPartParams* aParams = dynamic_cast<const MeshPrsMinMaxPartParams*>(theParams.get());
    if (aParams == nullptr) {
        return false;
    }

    theGenGeom->SetMeshData(new RWCae::MeshData());

    const RWCae::MeshPropertyComponent& aProp = aParams->Property();
    if (!aProp.IsDefined()) {
        return true;
    }

    ViewCaeGeom::MapOfMeshGeomScalarProps aProps;

    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theGeom->GetLastNotModifierChild();
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        if (aProps.IsBound(aGeom->Id())) {
            continue;
        }
        Handle(RWCae::MeshNVecProperty) aGProp = aGeom->CalculateProperty(aProp.Property);
        aProps.Bind(aGeom->Id(),
                    !aGProp.IsNull() ? RWCae::MeshPropertyComponent(aGProp, aProp.ComponentIndex)
                                     : RWCae::MeshPropertyComponent());
    }

    NCollection_Vec2<Graphic3d_Vec3> aMinMaxPnts;
    NCollection_Vec2<double>         aMinMaxValues;
    if (!MeshPresentationScalar::ComputeMinMaxPoints(aMeshGeom,
                                                     aProps,
                                                     aParams->MinMaxRange(),
                                                     aMinMaxPnts,
                                                     aMinMaxValues)) {
        return true;
    }

    constexpr int aNbPoints = 2;

    Handle(RWCae::MeshNVecProperty) aResProp =
        new RWCae::MeshNVecProperty(aProp.Property->Name(), 1, RWCae::MeshPropertyBase::MappingType::Nodal);
    aResProp->Init<RWCae::MeshScalarProperty>();
    aResProp->SetId(aProp.Property->Id());
    Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
        aResProp->GetRawComponent(0));
    aComp->Init(aNbPoints);
    aComp->SetId(aProp.GetScalarComp()->Id());
    theGenGeom->GetMeshData()->ChangeProperties().Append(aResProp);

    for (int i = 0; i < aNbPoints; i++) {
        theGenGeom->GetMeshData()->AddNodePosition(aMinMaxPnts[i]);
        aComp->SetValue(RWCae::MeshPropertyEntityInfo(i), aMinMaxValues[i]);
    }
    aResProp->UpdateComponents(false);

    return true;
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshPartAnnotations<MeshPartType::GroupsInfo>::update(const Handle(MeshPresentation)& thePrs)
{
    if (myAnnotations.Annotations().empty()) {
        return;
    }

    const MeshPresentationScalar* aPrs = dynamic_cast<const MeshPresentationScalar*>(thePrs.get());

    MapOfGroupAreas aGroupAreas;
    aPrs->ComputeGroupsAreas(aGroupAreas);

    const Handle(RWCae::MeshData)& aMeshData = aPrs->GetMeshGeometry((int)MeshPartType::GroupsInfo)->GetMeshData();

    for (MeshAnnotation& anAnnotation : myAnnotations.Annotations()) {
        int aGroupIndex = anAnnotation.Index();

        const NCollection_List<TColStd_PackedMapOfInteger>* aListOfGroups = aGroupAreas.Seek(aGroupIndex);
        if (aListOfGroups == nullptr) {
            continue;
        }
        std::vector<gp_Pnt> aPositions;

        for (const TColStd_PackedMapOfInteger& aGroup : *aListOfGroups) {
            if (aGroup.IsEmpty()) {
                continue;
            }

            int aMinNodeIdx = MeshPresentationScalar::GetGroupCenterNode(aGroup, aMeshData);

            const Graphic3d_Vec3& aNode = aMeshData->Nodes()[aMinNodeIdx];
            aPositions.push_back(gp_Pnt(aNode.x(), aNode.y(), aNode.z()));
        }

        anAnnotation.SetPositions(aPositions);
    }
}

// ================================================================
// Function : clear
// ================================================================
template<>
void MeshPartAnnotations<MeshPartType::GroupsInfo>::clear(bool theWithAuxData)
{
    if (theWithAuxData) {
        myAnnotations.Clear();
    }
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
template<>
bool MeshPartAnnotations<MeshPartType::GroupsInfo>::ExtractMeshData(
    const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
    const Handle(MeshPrsPartParams)&          theParams,
    const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    MapOfGroupAreas aGroupAreas;
    MeshPresentationScalar::ComputeGroupsAreas(theGeom, theParams->Property(), aGroupAreas);

    theGenGeom->SetMeshData(new RWCae::MeshData());
    if (aGroupAreas.IsEmpty()) {
        return true;
    }

    int aNbGroupNodes = 0;
    for (MapOfGroupAreas::Iterator anIter(aGroupAreas); anIter.More(); anIter.Next()) {
        for (const TColStd_PackedMapOfInteger& aGroup : anIter.Value()) {
            if (!aGroup.IsEmpty()) {
                aNbGroupNodes++;
            }
        }
    }

    Handle(RWCae::MeshNVecProperty) aResProp =
        new RWCae::MeshNVecProperty(theParams->Property().Property->Name(),
                                      1,
                                      RWCae::MeshPropertyBase::MappingType::Nodal);
    aResProp->Init<RWCae::MeshGroupProperty>();
    aResProp->SetId(theParams->Property().Property->Id());
    Handle(RWCae::MeshGroupProperty) aComp = Handle(RWCae::MeshGroupProperty)::DownCast(
        aResProp->GetRawComponent(0));
    aComp->Init(aNbGroupNodes);
    theGenGeom->GetMeshData()->ChangeProperties().Append(aResProp);

    const Handle(RWCae::MeshData)& aMeshData = theGeom->GetMeshData();

    for (MapOfGroupAreas::Iterator anIter(aGroupAreas); anIter.More(); anIter.Next()) {
        for (const TColStd_PackedMapOfInteger& aGroup : anIter.Value()) {
            if (aGroup.IsEmpty()) {
                continue;
            }

            int aMinNodeIdx = MeshPresentationScalar::GetGroupCenterNode(aGroup, aMeshData);

            const Graphic3d_Vec3& aNode    = aMeshData->Nodes()[aMinNodeIdx];
            RWCae::MeshEntityId aNodeIdx = theGenGeom->GetMeshData()->AddNodePosition(aNode);
            aComp->SetValue(RWCae::MeshPropertyEntityInfo(aNodeIdx), anIter.Key());
        }
    }
    aResProp->UpdateComponents(false);

    return true;
}

// ================================================================
// Function : compute
// ================================================================
template<MeshPartType T>
void MeshPartAnnotations<T>::compute(const Handle(AIS_InteractiveContext)&   theCtx,
                                     const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                                     const Handle(Prs3d_Presentation)&       thePrs,
                                     const Handle(Prs3d_Presentation)&       theTopPrs,
                                     float                                   thePixelRatio) const
{
    myAnnotations.Compute(theCtx,
                          thePrs,
                          theTopPrs,
                          theDrawer->TextAspect(),
                          theDrawer->AnnotationFillAreaAspect(),
                          theDrawer->AnnotationTopPointAspect(),
                          theDrawer->AnnotationPointAspect(),
                          thePixelRatio);
}

//! @}

//! @name MeshPartVectors.
//! @{

// ================================================================
// Function : update
// ================================================================
void MeshPartStreamlines::update(const Handle(MeshPresentation)& thePrs)
{
    const MeshPresentationStreamlines* aPrs = dynamic_cast<const MeshPresentationStreamlines*>(thePrs.get());

    Handle(MeshPrsStreamlinesPartParams) aParams   = aPrs->StreamlinesPartParams();
    Handle(MeshPrsStreamlinesVisProps)   aVisProps = aPrs->StreamlinesVisProps();

    Handle(ViewCaeGeom::MeshGeometry) aGeom = aPrs->GetMeshGeometry((int)MeshPartType::Streamlines);
    if (aGeom.IsNull()) {
        return;
    }
    RWCae::MeshPropertyComponent aBaseProp = aParams->Property();
    RWCae::MeshPropertyComponent aProp     = aGeom->CalculatePropertyComponent(aBaseProp.Property,
                                                                             aBaseProp.ComponentIndex);

    CaeMeshStreamlinesBuilder aBuilder(aGeom, aProp, aParams->StreamlinesProperty(), aParams->Plane());
    aBuilder.SetIsRegular(aParams->IsRegular());
    if (aParams->IsRegular()) {
        aBuilder.SetUVStreamlines(aParams->UVStreamlines());
    } else {
        aBuilder.SetNbStreamlines(aParams->NbStreamlines());
    }
    aBuilder.SetStepCoeff(aParams->StepCoeff());
    aBuilder.SetLengthCoeff(aParams->LengthCoeff());
    aBuilder.SetThickness(aVisProps->Thickness());

    const Handle(RWCae::MeshData)& aMeshData = aPrs->GetMeshGeometry()->GetFirstParent()->GetMeshData();
    if (!aMeshData.IsNull()) {
        aBuilder.SetBox(aMeshData->BoundingBox());
    }
    myStreamlines = aBuilder.Create();

    if (!myStreamlines.IsNull()) {
        MeshStreamlinesAnimationData* anAnimData = ChangeAnimationData();
        anAnimData->Speed                        = aVisProps->AnimationSpeed();
        anAnimData->Trail                        = aVisProps->AnimationTrail();
        anAnimData->Frequency                    = aVisProps->AnimationFrequency();
        anAnimData->IsAnimated                   = aVisProps->IsAnimated();
        anAnimData->IsPaused                     = aVisProps->IsAnimationPaused();
    }
}

// ================================================================
// Function : ExtractMeshData
// ================================================================
bool MeshPartStreamlines::ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                          const Handle(MeshPrsPartParams)&          theParams,
                                          const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    const MeshPrsStreamlinesPartParams* aParams = dynamic_cast<const MeshPrsStreamlinesPartParams*>(theParams.get());
    if (aParams == nullptr) {
        return false;
    }

    CaeMeshStreamlinesBuilder aBuilder(theGeom, aParams->Property(), aParams->StreamlinesProperty(), aParams->Plane());
    aBuilder.SetIsRegular(aParams->IsRegular());
    if (aParams->IsRegular()) {
        aBuilder.SetUVStreamlines(aParams->UVStreamlines());
    } else {
        aBuilder.SetNbStreamlines(aParams->NbStreamlines());
    }
    aBuilder.SetStepCoeff(aParams->StepCoeff());
    aBuilder.SetLengthCoeff(aParams->LengthCoeff());

    Handle(RWCae::MeshData) aRes = aBuilder.ExtractStreamlines();
    if (aRes.IsNull()) {
        return false;
    }

    theGenGeom->SetMeshData(aRes);
    return true;
}

// ================================================================
// Function : compute
// ================================================================
void MeshPartStreamlines::compute(const Handle(AIS_InteractiveContext)&,
                                  const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                                  const Handle(Prs3d_Presentation)&       thePrs,
                                  const Handle(Prs3d_Presentation)&,
                                  float) const
{
    if (myStreamlines.IsNull() || !myStreamlines->IsValid()) {
        return;
    }

    Handle(OpenGl_Group) aGroup = Handle(OpenGl_Group)::DownCast(thePrs->NewGroup());
    if (myStreamlines->Type() == Graphic3d_TOPA_POLYLINES) {
        aGroup->SetPrimitivesAspect(theDrawer->StreamlinesLineAspect());
    } else {
        aGroup->SetPrimitivesAspect(theDrawer->StreamlinesTubeAspect());
    }

    OpenGl_Structure*           aStruct = aGroup->GlStruct();
    const OpenGl_GraphicDriver* aDriver = aStruct->GlDriver();

    const MeshStreamlinesAnimationData* anAnimData = AnimationData();

    MeshStreamlinesElement* anArray = new MeshStreamlinesElement(aDriver,
                                                                 myStreamlines->Type(),
                                                                 myStreamlines->Indices(),
                                                                 myStreamlines->Attributes(),
                                                                 myStreamlines->Bounds(),
                                                                 anAnimData);
    aGroup->AddElement(anArray);

    aGroup->Graphic3d_Group::AddPrimitiveArray(myStreamlines->Type(),
                                               myStreamlines->Indices(),
                                               myStreamlines->Attributes(),
                                               myStreamlines->Bounds());
}

//! @}

template ViewCae_ExportTemplate class MeshPartAnnotations<MeshPartType::GroupsInfo>;

} // namespace
