#include <ViewCae/InteractiveCaeMesh.h>

#include <ViewCae/InteractiveCaeMeshSelMode.h>
#include <ViewCae/MeshColorScaleMapper.h>
#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationStreamlines.h>
#include <ViewCae/MeshPresentationVector.h>

#include <RWCae/MeshBinReader.h>
#include <RWCae/MeshBinWriter.h>
#include <RWCae/MeshTools.h>

#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_Text.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_TextAspect.hxx>
#include <V3d_View.hxx>

namespace {
constexpr int           MARKER_IMAGE_SIZE = 5;
constexpr unsigned char MARKER_IMAGE[]    = {
    0,   255, 255, 255, 0,   // 1
    255, 255, 255, 255, 255, // 2
    255, 255, 255, 255, 255, // 3
    255, 255, 255, 255, 255, // 4
    0,   255, 255, 255, 0    // 5
};

//! Wrapper for Graphic3d_MarkerImage.
struct CaeMesh_MarkerImage : public Graphic3d_MarkerImage {
    //! Constructor from existing pixmap.
    CaeMesh_MarkerImage(const opencascade::handle<Image_PixMap>& theImage, const TCollection_AsciiString& theKey)
        : Graphic3d_MarkerImage(theKey, theKey + "A", theImage)
    {
    }
};
}

namespace ViewCae {

// ================================================================
// Function : InteractiveCaeMesh
// ================================================================
InteractiveCaeMesh::InteractiveCaeMesh(const Handle(ViewCae::InteractiveCaeMeshOwner)& theNewId)
{
    myOwner     = theNewId;
    myMeshOwner = theNewId;
    myMeshOwner->SetSelectablePointer(this);
    myMeshOwner->GetTopmostPresentation(Handle(PrsMgr_PresentationManager)(), 0, true);

    initDefaultAttributes();

    myAnimation = new CaeMeshAnimation(this);
}

// ================================================================
// Function : ~InteractiveCaeMesh
// ================================================================
InteractiveCaeMesh::~InteractiveCaeMesh()
{
    //
}

// ================================================================
// Function : initDefaultAttributes
// ================================================================
void InteractiveCaeMesh::initDefaultAttributes()
{
    myDrawer = new InteractiveCaeMeshDrawer();

    Handle(InteractiveCaeMeshDrawer) aHighlightDrawer = new InteractiveCaeMeshDrawer();
    Quantity_Color                   aHiColor(Quantity_NOC_GRAY90);
    aHighlightDrawer->SetModulateTexture(true);
    aHighlightDrawer->SetShadingColor(aHiColor);
    aHighlightDrawer->SetEdgesColor(aHiColor);
    aHighlightDrawer->SetPointsColor(aHiColor);
    aHighlightDrawer->SetVectorsColor(aHiColor);
    aHighlightDrawer->SetStreamlinesColor(aHiColor);
    aHighlightDrawer->SetShadingTransparency(0.8f);
    aHighlightDrawer->SetColor(aHiColor);
    aHighlightDrawer->SetTransparency(0.8f);
    myDynHilightDrawer = aHighlightDrawer;

    Handle(InteractiveCaeMeshDrawer) aSelectionDrawer = new InteractiveCaeMeshDrawer();
    Quantity_Color                   aSelColor(Quantity_NOC_GRAY80);
    aSelectionDrawer->SetModulateTexture(true);
    aSelectionDrawer->SetShadingColor(aSelColor);
    aSelectionDrawer->SetEdgesColor(aSelColor);
    aSelectionDrawer->SetPointsColor(aSelColor);
    aSelectionDrawer->SetVectorsColor(aSelColor);
    aSelectionDrawer->SetStreamlinesColor(aSelColor);
    aSelectionDrawer->SetShadingTransparency(0.8f);
    aSelectionDrawer->SetZLayer(Graphic3d_ZLayerId_Top);
    aSelectionDrawer->SetColor(aSelColor);
    aSelectionDrawer->SetTransparency(0.8f);
    myHilightDrawer = aSelectionDrawer;

    Handle(Image_PixMap) aPixMap = new Image_PixMap();
    aPixMap->InitZero(Image_Format_Alpha, MARKER_IMAGE_SIZE, MARKER_IMAGE_SIZE);
    std::copy(MARKER_IMAGE, MARKER_IMAGE + MARKER_IMAGE_SIZE * MARKER_IMAGE_SIZE, aPixMap->ChangeData());

    Handle(CaeMesh_MarkerImage)      aMarkerImage  = new CaeMesh_MarkerImage(aPixMap, "CaeMesh_MarkerImage");
    Handle(Graphic3d_AspectMarker3d) aMarkerAspect = new Graphic3d_AspectMarker3d();
    aMarkerAspect->SetMarkerImage(aMarkerImage);
    aMarkerAspect->SetMarkerType(Aspect_TOM_USERDEFINED);

    my2DGridLineAspect = new Graphic3d_AspectLine3d(Quantity_Color(Quantity_NOC_GRAY90), Aspect_TOL_DOT, 0.3);
    my2DGridTextAspect = new Prs3d_TextAspect();
    my2DGridTextAspect->SetFont(Font_NOF_SANS_SERIF);
    my2DGridTextAspect->Aspect()->SetColor(Quantity_NOC_YELLOW);
    my2DGridTextAspect->Aspect()->SetTextFontAspect(Font_FontAspect_Regular);
    my2DYAxisTextAspect = new Prs3d_TextAspect();
    my2DYAxisTextAspect->DeepCopyFrom(my2DGridTextAspect);
    my2DYAxisTextAspect->SetAngle(90);
}

// ================================================================
// Function : CreatePresentation
// ================================================================
const TCollection_AsciiString& InteractiveCaeMesh::CreatePresentation(MeshPresentationType           theType,
                                                                      const TCollection_AsciiString& theId)
{
    const TCollection_AsciiString& anId = myMeshOwner->CreatePrs(theType, theId);

    Handle(MeshPresentation) aPrs = Presentation(anId);

    Graphic3d_PolygonOffset anOffset;
    anOffset.Factor = -1.0f;
    anOffset.Units  = -1.0f;

    aPrs->HighlightDrawer(MeshHighlightingType::None)->UpdateAspects(myDrawer);

    Handle(InteractiveCaeMeshDrawer) aHiDrawer = aPrs->HighlightDrawer(MeshHighlightingType::Highlight);
    aHiDrawer->UpdateAspects(myDynHilightDrawer);
    aHiDrawer->ShadingAspect()->Aspect()->SetPolygonOffset(anOffset);

    Handle(InteractiveCaeMeshDrawer) aSelDrawer = aPrs->HighlightDrawer(MeshHighlightingType::Selection);
    aSelDrawer->UpdateAspects(myHilightDrawer);
    aSelDrawer->ShadingAspect()->Aspect()->SetPolygonOffset(anOffset);

    UpdateAspects();
    SetDeferredUpdate();
    RecomputeSelection();

    if (myAnimation->IsRunning()) {
        Presentation(anId)->SetCachePrimitives(true, false);
    }

    return anId;
}

// ================================================================
// Function : RemovePresentation
// ================================================================
void InteractiveCaeMesh::RemovePresentation(const TCollection_AsciiString& thePrsId)
{
    myMeshOwner->RemovePresentation(thePrsId);
    if (myMeshOwner->ToUseTestCachePrs()) {
        myMeshOwner->TestCachePrs()->RemovePresentation(thePrsId);
    }
    SetDeferredUpdate();
    RecomputeSelection();
    updateColorMappers();
}

// ================================================================
// Function : SetPartGenMeshData
// ================================================================
void InteractiveCaeMesh::SetPartGenMeshData(const TCollection_AsciiString&   thePrsId,
                                            int                              thePartId,
                                            const Handle(RWCae::MeshData)& theMeshData)
{
    myMeshOwner->SetPartGenMeshData(thePrsId, thePartId, theMeshData);
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    aPrs->ClearPart((MeshPartType)thePartId, ActiveTimeStep(), false, false);

    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// =======================================================================
// function : SetColorMapper
// =======================================================================
void InteractiveCaeMesh::SetColorMapper(const Handle(MeshColorMapper)&        theColorMapper,
                                        const RWCae::MeshPropertyComponent& theProp)
{
    if (!theColorMapper.IsNull()) {
        theColorMapper->SetOnTextureUpdateCallback([this]() {
            this->SynchronizeAspects();
        });
    }

    myMeshOwner->PropertyData()->SetColorMapper(theColorMapper, theProp);

    UpdateAspects();
}

// =======================================================================
// function : SetProperty
// =======================================================================
void InteractiveCaeMesh::SetProperty(const TCollection_AsciiString&           thePrsId,
                                     const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    Handle(RWCae::MeshNVecProperty) aPrevProp = Property(thePrsId).Property;

    if (aPrevProp == theProperty) {
        return;
    }

    bool prevToEnableTrianglesTextures = !aPrevProp.IsNull();
    bool prevToEnableNodesTextures     = prevToEnableTrianglesTextures && !aPrevProp->IsElemental();

    myMeshOwner->SetProperty(thePrsId, theProperty);

    bool toUpdate = onPropertyUpdate(thePrsId, prevToEnableTrianglesTextures, prevToEnableNodesTextures);

    UpdateAspects();

    if (toUpdate) {
        SetDeferredUpdate();
        RecomputeSelection();
    }
}

// =======================================================================
// function : SetPropertyComponentIndex
// =======================================================================
void InteractiveCaeMesh::SetPropertyComponentIndex(const TCollection_AsciiString& thePrsId, const int theIndex)
{
    if (myMeshOwner->Property(thePrsId).ComponentIndex == theIndex) {
        return;
    }

    Handle(RWCae::MeshNVecProperty) aPrevProp = Property(thePrsId).Property;
    if (aPrevProp.IsNull() || !aPrevProp->EnabledComponents().Contains(theIndex)) {
        return;
    }
    bool prevToEnableTrianglesTextures = !aPrevProp.IsNull();
    bool prevToEnableNodesTextures     = prevToEnableTrianglesTextures && !aPrevProp->IsElemental();

    myMeshOwner->SetPropertyComponentIndex(thePrsId, theIndex);

    bool toUpdate = onPropertyUpdate(thePrsId, prevToEnableTrianglesTextures, prevToEnableNodesTextures);

    UpdateAspects();

    if (toUpdate) {
        SetDeferredUpdate();
        RecomputeSelection();
    }
}

// =======================================================================
// function : onPropertyUpdate
// =======================================================================
bool InteractiveCaeMesh::onPropertyUpdate(const TCollection_AsciiString& thePrsId,
                                          bool                           wasTriangleTexturesEnabled,
                                          bool                           wasNodesTexturesEnabled)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);

    Handle(RWCae::MeshScalarPropertyBase) aProp = Property(thePrsId).GetScalarComp();

    bool toEnableTrianglesTextures = !aProp.IsNull();
    bool toEnableNodesTextures     = toEnableTrianglesTextures && !aProp->IsElemental();

    bool toUpdate = false;
    if (wasTriangleTexturesEnabled || toEnableTrianglesTextures) {
        aPrs->ClearPart(MeshPartType::Triangles3d);
        aPrs->ClearPart(MeshPartType::Triangles2d);
        aPrs->ClearPart(MeshPartType::Edges3d);
        aPrs->ClearPart(MeshPartType::Edges2d);
        aPrs->ClearPart(MeshPartType::Centroids3d);
        aPrs->ClearPart(MeshPartType::Centroids2d);
        aPrs->ClearPart(MeshPartType::Elems0d);
        toUpdate = true;
    }
    if (wasNodesTexturesEnabled || toEnableNodesTextures) {
        aPrs->ClearPart(MeshPartType::Points);
        toUpdate = true;
    }
    if (Handle(MeshPrsVisScalarProps) aScalarProps = Handle(MeshPrsVisScalarProps)::DownCast(aPrs->VisProps())) {
        aPrs->ClearPart(MeshPartType::MinMaxAnnotations);
        toUpdate = toUpdate || aScalarProps->ToShowMinMax();

        if (aScalarProps->ToShowGroupsInfo()) {
            const Handle(MeshPartGroupsInfoAnnotations)& aGroupInfo = Handle(MeshPartGroupsInfoAnnotations)::DownCast(
                aPrs->ChangeParts()(MeshPartType::GroupsInfo));
            aGroupInfo->Annotations().UnhilightAll(InteractiveContext()->MainPrsMgr());
        }
        aPrs->ClearPart(MeshPartType::GroupsInfo);
        toUpdate = toUpdate || aScalarProps->ToShowGroupsInfo();
        if (Handle(RWCae::MeshGroupProperty) aGroupProp = Handle(RWCae::MeshGroupProperty)::DownCast(aProp)) {
            Handle(MeshPresentationScalar)::DownCast(aPrs)->FillGroupsInfo();
        }
    }

    updateColorMappers();

    return toUpdate;
}

// =======================================================================
// function : ElementTypes
// =======================================================================
unsigned int InteractiveCaeMesh::ElementTypes(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return 0;
    }

    Handle(MeshPrsVisScalarProps) aVisProps = aPrs->ScalarVisProps();
    return aVisProps->ElementTypes();
}

// =======================================================================
// function : SetElementTypes
// =======================================================================
void InteractiveCaeMesh::SetElementTypes(const TCollection_AsciiString& thePrsId, unsigned int theElementTypes)
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisScalarProps) aVisProps = aPrs->ScalarVisProps();
    if (aVisProps->ElementTypes() == theElementTypes) {
        return;
    }

    MeshSensParts& aSensParts = aPrs->ChangeSensParts();

    bool prevToShowFreeNodes = (aVisProps->ElementTypes() & (unsigned int)InteractiveCaeMeshElementType::FreeNode) != 0;

    unsigned int aChangedTypes = aVisProps->ElementTypes() ^ theElementTypes;

    aVisProps->SetElementTypes(theElementTypes);

    if ((aChangedTypes & (unsigned int)InteractiveCaeMeshElementType::Element0d) != 0) {
        aPrs->ClearPart(MeshPartType::Elems0d, -1, true, false);
        aSensParts(MeshSensPartType::Entities0d)->Clear();
    }
    if ((aChangedTypes & (unsigned int)InteractiveCaeMeshElementType::Face) != 0) {
        aPrs->ClearPart(MeshPartType::Triangles2d, -1, true, false);
        aPrs->ClearPart(MeshPartType::Edges2d, -1, true, false);
        aSensParts(MeshSensPartType::Entities2d)->Clear();
    }
    if ((aChangedTypes & (unsigned int)InteractiveCaeMeshElementType::Volume) != 0) {
        aPrs->ClearPart(MeshPartType::Triangles3d, -1, true, false);
        aPrs->ClearPart(MeshPartType::Edges3d, -1, true, false);
        aSensParts(MeshSensPartType::Entities3d)->Clear();
    }
    if ((aChangedTypes & (unsigned int)InteractiveCaeMeshElementType::Node) != 0) {
        aPrs->ClearPart(MeshPartType::Points, -1, true, false);
        aSensParts(MeshSensPartType::Nodes)->Clear();
    }

    bool toShowFreeNodes = (theElementTypes & (unsigned int)InteractiveCaeMeshElementType::FreeNode) != 0;

    if (prevToShowFreeNodes != toShowFreeNodes) {
        myMeshOwner->SetBoundingBoxToUpdate();
    }

    aPrs->ClearPart(MeshPartType::MinMaxAnnotations);

    UpdateAspects();

    SetDeferredUpdate();

    RecomputeSelection();
}

// =======================================================================
// function : SetDeformedProperty
// =======================================================================
void InteractiveCaeMesh::SetDeformedProperty(const TCollection_AsciiString&           thePrsId,
                                             const Handle(RWCae::MeshNVecProperty)& theDeformedProperty)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    if (DeformedProperty(thePrsId) == theDeformedProperty) {
        return;
    }

    if (!theDeformedProperty.IsNull() && (!theDeformedProperty->IsNodal() || !theDeformedProperty->IsVector3())) {
        return;
    }

    myMeshOwner->SetDeformedProperty(thePrsId, theDeformedProperty);

    myMeshOwner->SetBoundingBoxToUpdate();

    UpdateAspects();
    SetDeferredUpdate();

    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// =======================================================================
// function : SetDeformedScaleCoeff
// =======================================================================
void InteractiveCaeMesh::SetDeformedScaleCoeff(const TCollection_AsciiString& thePrsId, double theDeformedScaleCoeff)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    if (DeformedProperty(thePrsId).IsNull()) {
        return;
    }

    if (Abs(DeformedScaleCoeff(thePrsId) - theDeformedScaleCoeff) < Precision::Confusion()) {
        return;
    }

    myMeshOwner->SetDeformedScaleCoeff(thePrsId, theDeformedScaleCoeff);

    myMeshOwner->SetBoundingBoxToUpdate();

    UpdateAspects();
    SetDeferredUpdate();

    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// =======================================================================
// function : GetAvailableVec3Properties
// =======================================================================
std::vector<TCollection_AsciiString> InteractiveCaeMesh::GetAvailableVec3Properties() const
{
    std::vector<TCollection_AsciiString>  aRes;
    const Handle(RWCae::MeshData)&      aMeshData = GetMeshData();
    RWCae::MeshNVecProperties::Iterator anIter(aMeshData->Properties());
    for (int i = 0; anIter.More(); anIter.Next(), i++) {
        const Handle(RWCae::MeshNVecProperty)& aVecProperty = anIter.Value();
        if (!aVecProperty.IsNull() && aVecProperty->HasScalarComponents() && aVecProperty->IsVector3()) {
            aRes.push_back(aVecProperty->Id());
        }
    }

    return aRes;
}

// =======================================================================
// function : VectorProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) InteractiveCaeMesh::VectorProperty(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return nullptr;
    }

    return aPrs->VectorPartParams()->VectorProperty();
}

// =======================================================================
// function : SetVectorProperty
// =======================================================================
void InteractiveCaeMesh::SetVectorProperty(const TCollection_AsciiString&           thePrsId,
                                           const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVectorPartParams) aProps = aPrs->VectorPartParams();

    if (aProps->VectorProperty() == theProperty) {
        return;
    }

    if (theProperty.IsNull() || !theProperty->HasScalarComponents() || !theProperty->IsVector3()) {
        return;
    }

    aProps->SetVectorProperty(theProperty);
    aPrs->SetToUpdate(true, true);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : VectorScaleCoeff
// =======================================================================
double InteractiveCaeMesh::VectorScaleCoeff(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    return !aPrs.IsNull() ? aPrs->VectorVisProps()->VectorScaleCoeff() : 1.0;
}

// =======================================================================
// function : SetVectorScaleCoeff
// =======================================================================
void InteractiveCaeMesh::SetVectorScaleCoeff(const TCollection_AsciiString& thePrsId, double theCoeff)
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVectorVisProps) aProps = aPrs->VectorVisProps();
    if (::IsEqual(aProps->VectorScaleCoeff(), theCoeff)) {
        return;
    }

    aProps->SetVectorScaleCoeff(theCoeff);
    aPrs->ClearPart(MeshPartType::Vectors, -1, true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : VectorDistrType
// =======================================================================
MeshVectorDistrType InteractiveCaeMesh::VectorDistrType(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return MeshVectorDistrType::Volume;
    }

    return aPrs->VectorPartParams()->VectorDistrType();
}

// =======================================================================
// function : SetVectorDistrType
// =======================================================================
void InteractiveCaeMesh::SetVectorDistrType(const TCollection_AsciiString& thePrsId, MeshVectorDistrType theType)
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVectorPartParams) aProps = aPrs->VectorPartParams();
    if (aProps->VectorDistrType() == theType) {
        return;
    }

    aProps->SetVectorDistrType(theType);
    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : VectorSamplingRatio
// =======================================================================
double InteractiveCaeMesh::VectorSamplingRatio(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return 1.0;
    }

    return aPrs->VectorPartParams()->SamplingRatio();
}

// =======================================================================
// function : SetVectorSamplingRatio
// =======================================================================
void InteractiveCaeMesh::SetVectorSamplingRatio(const TCollection_AsciiString& thePrsId, double theRatio)
{
    Handle(MeshPresentationVector) aPrs = Presentation<MeshPresentationVector>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVectorPartParams) aProps = aPrs->VectorPartParams();
    if (::IsEqual(aProps->SamplingRatio(), theRatio)) {
        return;
    }

    aProps->SetSamplingRatio(theRatio);
    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) InteractiveCaeMesh::StreamlinesProperty(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return nullptr;
    }

    return aPrs->StreamlinesPartParams()->StreamlinesProperty();
}

// =======================================================================
// function : SetStreamlinesProperty
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesProperty(const TCollection_AsciiString&           thePrsId,
                                                const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (aProps->StreamlinesProperty() == theProperty) {
        return;
    }

    if (theProperty.IsNull() || !theProperty->HasScalarComponents() || !theProperty->IsVector3()
        || !theProperty->IsNodal()) {
        return;
    }

    aProps->SetStreamlinesProperty(theProperty);
    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : IsRegularStreamlines
// =======================================================================
bool InteractiveCaeMesh::IsRegularStreamlines(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    return aPrs->StreamlinesPartParams()->IsRegular();
}

// =======================================================================
// function : SetIsRegularStreamlines
// =======================================================================
void InteractiveCaeMesh::SetIsRegularStreamlines(const TCollection_AsciiString& thePrsId, bool theIsRegular)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (aProps->IsRegular() == theIsRegular) {
        return;
    }

    aProps->SetIsRegular(theIsRegular);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : NbStreamlines
// =======================================================================
int InteractiveCaeMesh::NbStreamlines(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0;
    }

    return aPrs->StreamlinesPartParams()->NbStreamlines();
}

// =======================================================================
// function : SetNbStreamlines
// =======================================================================
void InteractiveCaeMesh::SetNbStreamlines(const TCollection_AsciiString& thePrsId, int theNbStreamlines)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (aProps->NbStreamlines() == theNbStreamlines) {
        return;
    }

    aProps->SetNbStreamlines(theNbStreamlines);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : UVStreamlines
// =======================================================================
Graphic3d_Vec2i InteractiveCaeMesh::UVStreamlines(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return Graphic3d_Vec2i();
    }

    return aPrs->StreamlinesPartParams()->UVStreamlines();
}

// =======================================================================
// function : SetUVStreamlines
// =======================================================================
void InteractiveCaeMesh::SetUVStreamlines(const TCollection_AsciiString& thePrsId,
                                          const Graphic3d_Vec2i&         theUVStreamlines)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (aProps->UVStreamlines() == theUVStreamlines) {
        return;
    }

    aProps->SetUVStreamlines(theUVStreamlines);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesPlane
// =======================================================================
MeshStreamlinesPlane InteractiveCaeMesh::StreamlinesPlane(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return MeshStreamlinesPlane();
    }

    return aPrs->StreamlinesPartParams()->Plane();
}

// =======================================================================
// function : SetStreamlinesPlane
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesPlane(const TCollection_AsciiString& thePrsId,
                                             const MeshStreamlinesPlane&    thePlane)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (aProps->Plane().IsEqual(thePlane)) {
        return;
    }

    aProps->SetPlane(thePlane);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesStepCoeff
// =======================================================================
double InteractiveCaeMesh::StreamlinesStepCoeff(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0;
    }

    return aPrs->StreamlinesPartParams()->StepCoeff();
}

// =======================================================================
// function : SetStreamlinesStepCoeff
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesStepCoeff(const TCollection_AsciiString& thePrsId, double theStepCoeff)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (::IsEqual(aProps->StepCoeff(), theStepCoeff)) {
        return;
    }

    aProps->SetStepCoeff(theStepCoeff);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesLengthCoeff
// =======================================================================
double InteractiveCaeMesh::StreamlinesLengthCoeff(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0;
    }

    return aPrs->StreamlinesPartParams()->LengthCoeff();
}

// =======================================================================
// function : SetStreamlinesLengthCoeff
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesLengthCoeff(const TCollection_AsciiString& thePrsId, double theLengthCoeff)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesPartParams) aProps = aPrs->StreamlinesPartParams();
    if (::IsEqual(aProps->LengthCoeff(), theLengthCoeff)) {
        return;
    }

    aProps->SetLengthCoeff(theLengthCoeff);

    aPrs->SetToUpdate(true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesThickness
// =======================================================================
double InteractiveCaeMesh::StreamlinesThickness(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0;
    }

    return aPrs->StreamlinesVisProps()->Thickness();
}

// =======================================================================
// function : SetStreamlinesThickness
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesThickness(const TCollection_AsciiString& thePrsId, double theThickness)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesVisProps) aProps = aPrs->StreamlinesVisProps();
    if (::IsEqual(aProps->Thickness(), theThickness)) {
        return;
    }

    aProps->SetThickness(theThickness);
    aPrs->ClearPart(MeshPartType::Streamlines, -1, true, false);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : StreamlinesAnimationSpeed
// =======================================================================
float InteractiveCaeMesh::StreamlinesAnimationSpeed(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0f;
    }

    return aPrs->StreamlinesVisProps()->AnimationSpeed();
}

// =======================================================================
// function : SetStreamlinesAnimationSpeed
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesAnimationSpeed(const TCollection_AsciiString& thePrsId, float theSpeed)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesVisProps) aProps = aPrs->StreamlinesVisProps();
    if (::IsEqual(aProps->AnimationSpeed(), theSpeed)) {
        return;
    }

    aProps->SetAnimationSpeed(theSpeed);

    Handle(MeshPartStreamlines) anInfo = Handle(MeshPartStreamlines)::DownCast(
        aPrs->ChangeParts()(MeshPartType::Streamlines));
    if (anInfo->AnimationData() != nullptr) {
        anInfo->ChangeAnimationData()->Speed = theSpeed;
    }
}

// =======================================================================
// function : StreamlinesAnimationTrail
// =======================================================================
float InteractiveCaeMesh::StreamlinesAnimationTrail(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0f;
    }

    return aPrs->StreamlinesVisProps()->AnimationTrail();
}

// =======================================================================
// function : SetStreamlinesAnimationTrail
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesAnimationTrail(const TCollection_AsciiString& thePrsId, float theTrail)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesVisProps) aProps = aPrs->StreamlinesVisProps();
    if (::IsEqual(aProps->AnimationTrail(), theTrail)) {
        return;
    }

    aProps->SetAnimationTrail(theTrail);

    Handle(MeshPartStreamlines) anInfo = Handle(MeshPartStreamlines)::DownCast(
        aPrs->ChangeParts()(MeshPartType::Streamlines));
    if (anInfo->AnimationData() != nullptr) {
        anInfo->ChangeAnimationData()->Trail = theTrail;
    }
}

// =======================================================================
// function : StreamlinesAnimationFrequency
// =======================================================================
int InteractiveCaeMesh::StreamlinesAnimationFrequency(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0;
    }

    return aPrs->StreamlinesVisProps()->AnimationFrequency();
}

// =======================================================================
// function : SetStreamlinesAnimationFrequency
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesAnimationFrequency(const TCollection_AsciiString& thePrsId, int theFreq)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesVisProps) aProps = aPrs->StreamlinesVisProps();
    if (::IsEqual(aProps->AnimationFrequency(), theFreq)) {
        return;
    }

    aProps->SetAnimationFrequency(theFreq);

    Handle(MeshPartStreamlines) anInfo = Handle(MeshPartStreamlines)::DownCast(
        aPrs->ChangeParts()(MeshPartType::Streamlines));
    if (anInfo->AnimationData() != nullptr) {
        anInfo->ChangeAnimationData()->Frequency = theFreq;
    }
}

// =======================================================================
// function : IsStreamlinesAnimated
// =======================================================================
bool InteractiveCaeMesh::IsStreamlinesAnimated(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0;
    }

    return aPrs->StreamlinesVisProps()->IsAnimated();
}

// =======================================================================
// function : SetStreamlinesAnimated
// =======================================================================
void InteractiveCaeMesh::SetStreamlinesAnimated(const TCollection_AsciiString& thePrsId, bool theIsAnimated)
{
    Handle(MeshPresentationStreamlines) aPrs = Presentation<MeshPresentationStreamlines>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsStreamlinesVisProps) aProps = aPrs->StreamlinesVisProps();
    if (::IsEqual(aProps->IsAnimated(), theIsAnimated)) {
        return;
    }

    aProps->SetAnimated(theIsAnimated);

    Handle(MeshPartStreamlines) anInfo = Handle(MeshPartStreamlines)::DownCast(
        aPrs->ChangeParts()(MeshPartType::Streamlines));
    if (anInfo->AnimationData() != nullptr) {
        anInfo->ChangeAnimationData()->IsAnimated = theIsAnimated;
    }
}

// =======================================================================
// function : SetElementType
// =======================================================================
ViewCae::InteractiveCaeMeshPrsMode InteractiveCaeMesh::PresentationMode(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return InteractiveCaeMeshPrsMode::FlatShaded;
    }

    return aPrs->ScalarVisProps()->PresentationMode();
}

// ================================================================
// Function : SetPresentationMode
// ================================================================
void InteractiveCaeMesh::SetPresentationMode(const TCollection_AsciiString& thePrsId, InteractiveCaeMeshPrsMode theMode)
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    if (aProps->PresentationMode() == theMode) {
        return;
    }

    if (aProps->PresentationMode() == InteractiveCaeMeshPrsMode::ShadedMeshShrinked
        || theMode == InteractiveCaeMeshPrsMode::ShadedMeshShrinked) {
        aPrs->ClearPart(MeshPartType::Triangles3d, -1, true, false);
        aPrs->ClearPart(MeshPartType::Triangles2d, -1, true, false);
    }

    aProps->SetPresentationMode(theMode);

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : IsVolumeAsSurface
// =======================================================================
bool InteractiveCaeMesh::IsVolumeAsSurface(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    return aPrs->ScalarVisProps()->IsVolumeAsSurface();
}

// ================================================================
// Function : SetVolumeAsSurface
// ================================================================
void InteractiveCaeMesh::SetVolumeAsSurface(const TCollection_AsciiString& thePrsId, bool theIsVolumeAsSurface)
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    if (aProps->IsVolumeAsSurface() == theIsVolumeAsSurface) {
        return;
    }

    aProps->SetVolumeAsSurface(theIsVolumeAsSurface);

    aPrs->ClearPart(MeshPartType::Triangles3d);
    aPrs->ClearPart(MeshPartType::Centroids3d);
    if (aProps->PresentationMode() == InteractiveCaeMeshPrsMode::ShadedMeshEdges) {
        aPrs->ClearPart(MeshPartType::Edges3d);
    }
    aPrs->ChangeSensParts()(MeshSensPartType::Entities3d)->Clear();
    RecomputeSelection();

    UpdateAspects();
    SetDeferredUpdate();
}

// =======================================================================
// function : IsPrsVisible
// =======================================================================
bool InteractiveCaeMesh::IsPrsVisible(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    return aPrs->VisProps()->IsVisible();
}

// ================================================================
// Function : SetPrsVisible
// ================================================================
void InteractiveCaeMesh::SetPrsVisible(const TCollection_AsciiString& thePrsId, bool theIsVisible)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisProps) aProps = aPrs->VisProps();
    if (aProps->IsVisible() == theIsVisible) {
        return;
    }

    aProps->SetVisible(theIsVisible);

    aPrs->SetToUpdate(true, true);
    aPrs->ChangeSensParts().ClearAll();

    UpdateAspects();
    SetDeferredUpdate();
    RecomputeSelection();

    updateColorMappers();
}

// =======================================================================
// function : ToShowPrsMinMax
// =======================================================================
bool InteractiveCaeMesh::ToShowPrsMinMax(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    return aPrs->ScalarVisProps()->ToShowMinMax();
}

// ================================================================
// Function : SetShowPrsMinMax
// ================================================================
void InteractiveCaeMesh::SetShowPrsMinMax(const TCollection_AsciiString& thePrsId, bool theToShow)
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    if (aProps->ToShowMinMax() == theToShow) {
        return;
    }

    aProps->SetShowMinMax(theToShow);
    aPrs->ClearPart(MeshPartType::MinMaxAnnotations);

    UpdateAspects();
    SetDeferredUpdate();
}

// ================================================================
// Function : UpdateMinMaxValues
// ================================================================
void InteractiveCaeMesh::UpdateMinMaxValues(const Handle(MeshColorMapper)& theColorMapper)
{
    if (theColorMapper.IsNull()) {
        return;
    }

    RWCae::MeshPropertyComponent aProp(theColorMapper->MeshProperty(), theColorMapper->MeshPropertyComp());
    TColStd_PackedMapOfInteger     aProps;
    myMeshOwner->PropertyData()->PropertyUsedIndices(aProp, aProps);

    bool toUpdate = false;

    for (const Handle(MeshPresentationBase)& aPrs : Presentations()) {
        Handle(MeshPresentationScalar) aScalarPrs = Handle(MeshPresentationScalar)::DownCast(aPrs);
        if (aScalarPrs.IsNull()) {
            continue;
        }

        if (aProps.Contains(aScalarPrs->ActiveProperty())) {
            aScalarPrs->ClearPart(MeshPartType::MinMaxAnnotations);
            toUpdate = toUpdate || aScalarPrs->ChangeParts()(MeshPartType::MinMaxAnnotations)->ToShow();
        }
    }

    if (toUpdate) {
        UpdateAspects();
        SetDeferredUpdate();
    }
}

// =======================================================================
// function : ToShowGroupsInfo
// =======================================================================
bool InteractiveCaeMesh::ToShowGroupsInfo(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    return aPrs->ScalarVisProps()->ToShowGroupsInfo();
}

// ================================================================
// Function : SetShowGroupsInfo
// ================================================================
void InteractiveCaeMesh::SetShowGroupsInfo(const TCollection_AsciiString& thePrsId, bool theToShow)
{
    Handle(MeshPresentationScalar) aPrs = Presentation<MeshPresentationScalar>(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(MeshPrsVisScalarProps) aProps = aPrs->ScalarVisProps();
    if (aProps->ToShowGroupsInfo() == theToShow) {
        return;
    }

    aProps->SetShowGroupsInfo(theToShow);

    UpdateAspects();
    SetDeferredUpdate();
    RecomputeSelection();
}

// ================================================================
// Function : SetClipPlane
// ================================================================
void InteractiveCaeMesh::SetClipPlane(const TCollection_AsciiString&     thePrsId,
                                      const Handle(Graphic3d_ClipPlane)& theClipPlane)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    if (myMeshOwner->ClipPlane(thePrsId) == theClipPlane) {
        return;
    }

    myMeshOwner->SetClipPlane(thePrsId, theClipPlane);

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetCutOnClipPlane
// ================================================================
void InteractiveCaeMesh::SetCutOnClipPlane(const TCollection_AsciiString& thePrsId, bool theToCut)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    if (myMeshOwner->ToCutOnClipPlane(thePrsId) == theToCut) {
        return;
    }

    myMeshOwner->SetCutOnClipPlane(thePrsId, theToCut);

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetSlicePlanes
// ================================================================
void InteractiveCaeMesh::SetSlicePlanes(const TCollection_AsciiString&        thePrsId,
                                        const Graphic3d_SequenceOfHClipPlane& theSlicePlanes)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    myMeshOwner->SetSlicePlanes(thePrsId, theSlicePlanes);

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetIsoProperty
// ================================================================
void InteractiveCaeMesh::SetIsoProperty(const TCollection_AsciiString&        thePrsId,
                                        const RWCae::MeshPropertyComponent& theIsoProperty)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    myMeshOwner->SetIsoProperty(thePrsId, theIsoProperty);

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetIsoValues
// ================================================================
void InteractiveCaeMesh::SetIsoValues(const TCollection_AsciiString& thePrsId, const std::vector<double>& theIsoValues)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    myMeshOwner->SetIsoValues(thePrsId, theIsoValues);

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetBlocksVisible
// ================================================================
void InteractiveCaeMesh::SetBlocksVisible(const TCollection_AsciiString&        thePrsId,
                                          const Handle(RWCae::MeshDataBlock)& theBlocks,
                                          bool                                  theVisible)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    if (!myMeshOwner->SetBlocksVisible(thePrsId, theBlocks, theVisible)) {
        return;
    }

    UpdateAspects();
    SetDeferredUpdate();
    aPrs->ChangeSensParts().ClearAll();
    RecomputeSelection();
}

// ================================================================
// Function : SetActiveTimeStep
// ================================================================
void InteractiveCaeMesh::SetActiveTimeStep(const int theTimeStep)
{
    if (ActiveTimeStep() == theTimeStep) {
        return;
    }

    bool aRes = myMeshOwner->SetActiveTimeStep(theTimeStep);

    UpdateAspects();
    SetDeferredUpdate();

    for (const Handle(MeshPresentationBase)& aPrs : myMeshOwner->Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->ChangeSensParts().ClearAll();
    }
    RecomputeSelection();

    for (const Handle(MeshPresentationBase)& aPrs : myMeshOwner->Presentations()) {
        Handle(RWCae::MeshNVecProperty) aProp = Property(aPrs->Id()).Property;
        if (!aProp.IsNull() && aProp->HasGroupComponents()) {
            Handle(MeshPresentationScalar)::DownCast(aPrs)->FillGroupsInfo();
        }
    }

    if (aRes && myMeshOwner->ToCacheProperties()) {
        // for now unload all properties of other time steps
        for (const Handle(RWCae::MeshNVecProperty)& aProp : GetMeshData()->Properties()) {
            if (aProp->IsLoaded() && aProp->CanBeLoaded() && aProp->TimeStep() != theTimeStep) {
                aProp->Unload();
            }
        }
    }
}

// ================================================================
// Function : SetCacheTimeSteps
// ================================================================
void InteractiveCaeMesh::SetCacheTimeSteps(bool theToCache, bool theToClearCached)
{
    for (const Handle(MeshPresentationBase)& aPrs : myMeshOwner->Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->SetCachePrimitives(theToCache, theToClearCached);
    }
}

// ================================================================
// Function : OnAnimationToggled
// ================================================================
void InteractiveCaeMesh::OnAnimationToggled(bool theIsRunning)
{
    for (const Handle(MeshPresentationBase)& aPrs : myMeshOwner->Presentations()) {
        Handle(MeshPresentationStreamlines) aStreamlinesPrs = Handle(MeshPresentationStreamlines)::DownCast(aPrs);
        if (aStreamlinesPrs.IsNull()) {
            continue;
        }

        aStreamlinesPrs->StreamlinesVisProps()->SetAnimationPaused(theIsRunning);

        Handle(MeshPartStreamlines) anInfo = Handle(MeshPartStreamlines)::DownCast(
            aStreamlinesPrs->ChangeParts()(MeshPartType::Streamlines));
        if (anInfo->AnimationData() != nullptr) {
            anInfo->ChangeAnimationData()->IsPaused = theIsRunning;
        }
    }
}

// ================================================================
// Function : OnPropertyDependencyUpdate
// ================================================================
void InteractiveCaeMesh::OnPropertyDependencyUpdate()
{
    UpdateAspects();
    SetDeferredUpdate();
    ProcessRedraw(InteractiveContext(), Handle(V3d_View)(), AIS_RedrawProgress_BeforeUpdate);
}

// ================================================================
// Function : getDrawer
// ================================================================
Handle(InteractiveCaeMeshDrawer) InteractiveCaeMesh::getDrawer(MeshHighlightingType theHiType) const
{
    switch (theHiType) {
        case MeshHighlightingType::Highlight: return Handle(InteractiveCaeMeshDrawer)::DownCast(myDynHilightDrawer);
        case MeshHighlightingType::Selection: return Handle(InteractiveCaeMeshDrawer)::DownCast(myHilightDrawer);
        case MeshHighlightingType::None: return Handle(InteractiveCaeMeshDrawer)::DownCast(myDrawer);
        default: return Handle(InteractiveCaeMeshDrawer)::DownCast(myDrawer);
    }
}

// ================================================================
// Function : ShadingColor
// ================================================================
Quantity_Color InteractiveCaeMesh::ShadingColor(const TCollection_AsciiString& thePrsId,
                                                MeshHighlightingType           theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->ShadingColor();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->ShadingColor() : Quantity_NOC_WHITE;
}

// ================================================================
// Function : SetShadingColor
// ================================================================
void InteractiveCaeMesh::SetShadingColor(const TCollection_AsciiString& thePrsId,
                                         const Quantity_Color&          theColor,
                                         MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetShadingColor(theColor);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())->HighlightDrawer(theHiType)->SetShadingColor(theColor);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetShadingColor(theColor);
        UpdateAspects();
    }
}

// ================================================================
// Function : EdgesColor
// ================================================================
Quantity_Color InteractiveCaeMesh::EdgesColor(const TCollection_AsciiString& thePrsId,
                                              MeshHighlightingType           theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->EdgesColor();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->EdgesColor() : Quantity_NOC_WHITE;
}

// ================================================================
// Function : SetEdgesColor
// ================================================================
void InteractiveCaeMesh::SetEdgesColor(const TCollection_AsciiString& thePrsId,
                                       const Quantity_Color&          theColor,
                                       MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetEdgesColor(theColor);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())->HighlightDrawer(theHiType)->SetEdgesColor(theColor);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetEdgesColor(theColor);
        UpdateAspects();
    }
}

// ================================================================
// Function : PointsColor
// ================================================================
Quantity_Color InteractiveCaeMesh::PointsColor(const TCollection_AsciiString& thePrsId,
                                               MeshHighlightingType           theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->PointsColor();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->PointsColor() : Quantity_NOC_WHITE;
}

// ================================================================
// Function : SetPointsColor
// ================================================================
void InteractiveCaeMesh::SetPointsColor(const TCollection_AsciiString& thePrsId,
                                        const Quantity_Color&          theColor,
                                        MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetPointsColor(theColor);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())->HighlightDrawer(theHiType)->SetPointsColor(theColor);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetPointsColor(theColor);
        UpdateAspects();
    }
}

// ================================================================
// Function : EdgesWidth
// ================================================================
float InteractiveCaeMesh::EdgesWidth(const TCollection_AsciiString& thePrsId, MeshHighlightingType theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->EdgesWidth();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->EdgesWidth() : 1.0f;
}

// ================================================================
// Function : SetEdgesWidth
// ================================================================
void InteractiveCaeMesh::SetEdgesWidth(const TCollection_AsciiString& thePrsId,
                                       const float                    theWidth,
                                       MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetEdgesWidth(theWidth);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())->HighlightDrawer(theHiType)->SetEdgesWidth(theWidth);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetEdgesWidth(theWidth);
        UpdateAspects();
    }
}

// ================================================================
// Function : PointsSize
// ================================================================
float InteractiveCaeMesh::PointsSize(const TCollection_AsciiString& thePrsId, MeshHighlightingType theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->PointsSize();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->PointsSize() : 1.0f;
}

// ================================================================
// Function : SetPointsSize
// ================================================================
void InteractiveCaeMesh::SetPointsSize(const TCollection_AsciiString& thePrsId,
                                       const float                    theSize,
                                       MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetPointsSize(theSize);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())->HighlightDrawer(theHiType)->SetPointsSize(theSize);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetPointsSize(theSize);
        UpdateAspects();
    }
}

// ================================================================
// Function : ShadingTransparency
// ================================================================
float InteractiveCaeMesh::ShadingTransparency(const TCollection_AsciiString& thePrsId,
                                              MeshHighlightingType           theHiType) const
{
    if (thePrsId.IsEmpty()) {
        return getDrawer(theHiType)->ShadingTransparency();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->HighlightDrawer(theHiType)->ShadingTransparency() : 0.0f;
}

// ================================================================
// Function : SetShadingTransparency
// ================================================================
void InteractiveCaeMesh::SetShadingTransparency(const TCollection_AsciiString& thePrsId,
                                                const float                    theValue,
                                                MeshHighlightingType           theHiType)
{
    if (thePrsId.IsEmpty()) {
        Handle(InteractiveCaeMeshDrawer) aDrawer = getDrawer(theHiType);
        aDrawer->SetShadingTransparency(theValue);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            dynamic_cast<MeshPresentation*>(aPrsBase.get())
                ->HighlightDrawer(theHiType)
                ->SetShadingTransparency(theValue);
        }
        UpdateAspects();
    }

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (!aPrs.IsNull()) {
        aPrs->HighlightDrawer(theHiType)->SetShadingTransparency(theValue);
        UpdateAspects();
    }
}

// ================================================================
// Function : ProcessRedraw
// ================================================================
AIS_RedrawProgressResult InteractiveCaeMesh::ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                           const Handle(V3d_View)&               theView,
                                                           const AIS_RedrawProgress              theStage)
{
    unsigned int aRes = 0;
    // updating aspects before redrawing frame shouldn't be that expensive?
    if (theStage == AIS_RedrawProgress_BeforeRedraw) {
        UpdateAspects();

        if (!myAnimation.IsNull() && !myAnimation->IsStopped()) {
            myAnimation->UpdateTimer();
            if (!myAnimation->IsPaused()) {
                aRes |= (unsigned int)AIS_RedrawProgressResult_NeedAskNextFrame;
            }
        }

        // request a new frame if any streamlines presentation is animated
        for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
            MeshPresentationStreamlines* aPrs = dynamic_cast<MeshPresentationStreamlines*>(aPrsBase.get());
            if (aPrs == nullptr) {
                continue;
            }
            const MeshParts& aParts = aPrs->ChangeParts();

            Handle(MeshPartStreamlines) aStreamlinesPart = Handle(MeshPartStreamlines)::DownCast(
                aParts(MeshPartType::Streamlines));
            if (aStreamlinesPart->ToShow() && aStreamlinesPart->AnimationData() != nullptr
                && aStreamlinesPart->AnimationData()->IsAnimated && !aStreamlinesPart->AnimationData()->IsPaused) {
                aRes |= (unsigned int)AIS_RedrawProgressResult_NeedAskNextFrame;
                break;
            }
        }
    }

    if (myIsDeferredUpdate && theStage == AIS_RedrawProgress_BeforeUpdate) {
        if (!myMeshOwner->ToUseGenGeom()) {
            processDeferredProps();
        } else {
            processDeferredGeoms();
        }
    }

    aRes |= (unsigned int)AIS_InteractiveObject::ProcessRedraw(theCtx, theView, theStage);
    return (AIS_RedrawProgressResult)aRes;
}

// ================================================================
// Function : ClearDynamicHighlight
// ================================================================
void InteractiveCaeMesh::ClearDynamicHighlight(const Handle(PrsMgr_PresentationManager)& theMgr)
{
    AIS_InteractiveObject::ClearDynamicHighlight(theMgr);
    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        if (auto aPrs = dynamic_cast<MeshPresentation*>(aPrsBase.get())) {
            if (aPrs->HighlightType() == MeshHighlightingType::Highlight) {
                aPrs->SetHighlightType(MeshHighlightingType::None);
            }
        }
    }
}

// ================================================================
// Function : UpdateAspects
// ================================================================
void InteractiveCaeMesh::UpdateAspects()
{
    for (const Handle(MeshPresentationBase)& aPrs : myMeshOwner->Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->UpdateAspects(myMeshOwner);
    }

    SynchronizeAspects();
}

// ================================================================
// Function : drawGridText
// ================================================================
void InteractiveCaeMesh::drawGridText(const Handle(Prs3d_Presentation)& thePrs,
                                      const Handle(Prs3d_TextAspect)&   theAspect,
                                      const TCollection_AsciiString&    theString,
                                      const gp_Pnt&                     thePos,
                                      Graphic3d_HorizontalTextAlignment theHAlign,
                                      Graphic3d_VerticalTextAlignment   theVAlign,
                                      const float                       theHeight)
{
    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();

    Handle(Graphic3d_Text) aText = new Graphic3d_Text(theHeight);
    aText->SetText(TCollection_ExtendedString(theString));
    aText->SetPosition(thePos);
    aText->SetHorizontalAlignment(theHAlign);
    aText->SetVerticalAlignment(theVAlign);

    aGroup->SetGroupPrimitivesAspect(theAspect->Aspect());
    aGroup->AddText(aText);
}

// ================================================================
// Function : Compute
// ================================================================
void InteractiveCaeMesh::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                 const Handle(Prs3d_Presentation)&         thePrs,
                                 const Standard_Integer                    theMode)
{
    thePrs->SetInfiniteState(myMeshOwner->IsInfiniteState());
    if (!AcceptDisplayMode(theMode)) {
        return;
    }

    Handle(PrsMgr_Presentation) aTopPrs = myMeshOwner->GetTopmostPresentation(thePrsMgr, theMode, true);

    for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
        Handle(MeshPresentation) aPrs   = Handle(MeshPresentation)::DownCast(aPrsBase);
        const MeshParts&         aParts = aPrs->GetParts();
        for (int i = 0; i < MeshNbPartTypes; i++) {
            aParts((MeshPartType)i)->Compute(InteractiveContext(), aPrs->Drawer(), thePrs, aTopPrs, myDevicePixelRatio);
        }
    }

    // 2D coordinate grid for cases when flat mesh is used as 2D graph.
    compute2DGrid(thePrs);

    InteractiveContext()->SelectionManager()->UpdateSelection(this);
}

// ================================================================
// Function : ComputeSelection
// ================================================================
void InteractiveCaeMesh::ComputeSelection(const Handle(SelectMgr_Selection)& theSel, const Standard_Integer theMode)
{
    if (theMode != 0) {
        return;
    }

    if ((myMeshOwner->GetSelectionMode() & (int)InteractiveCaeMeshSelMode::Box) != 0) {
        const Bnd_Box& aBox = GetMeshData()->BoundingBox();

        Handle(MeshSensitiveBox) aSensitiveBox = new MeshSensitiveBox(myMeshOwner, aBox);
        theSel->Add(aSensitiveBox);
        return;
    }

    for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
        Handle(MeshPresentation) aPrs  = Handle(MeshPresentation)::DownCast(aPrsBase);
        const MeshSensParts&     aSens = aPrs->GetSensParts(myMeshOwner);
        for (int i = 0; i < MeshNbSensPartTypes; i++) {
            aSens((MeshSensPartType)i)->Compute(theSel);
        }
    }
}

// ================================================================
// Function : RemoveProperties
// ================================================================
void InteractiveCaeMesh::RemoveProperties(bool theWithMetadata)
{
    if (theWithMetadata) {
        for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
            Handle(MeshPresentation) aPrs = Handle(MeshPresentation)::DownCast(aPrsBase);
            SetProperty(aPrs->Id(), nullptr);
            SetDeformedProperty(aPrs->Id(), nullptr);
            SetVectorProperty(aPrs->Id(), nullptr);
            SetIsoProperty(aPrs->Id(), RWCae::MeshPropertyComponent());
        }

        GetMeshData()->ChangeProperties().Clear();

        myMeshOwner->SetActiveTimeStep(-1);
        UpdateAspects();

        myIsDeferredUpdate = false;
        SetToUpdate();
    } else {
        for (const Handle(RWCae::MeshNVecProperty)& aProp : GetMeshData()->Properties()) {
            aProp->Unload();
        }
        SetDeferredUpdate();
    }
}

// ================================================================
// Function : RecomputeSelection
// ================================================================
void InteractiveCaeMesh::RecomputeSelection()
{
    if (!HasInteractiveContext()) {
        return;
    }

    myMeshOwner->Unhilight(InteractiveContext()->MainPrsMgr(), 0);

    for (Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        Handle(MeshPresentation)                     aPrs        = Handle(MeshPresentation)::DownCast(aPrsBase);
        const Handle(MeshPartGroupsInfoAnnotations)& aGroupsInfo = Handle(MeshPartGroupsInfoAnnotations)::DownCast(
            aPrs->ChangeParts()(MeshPartType::GroupsInfo));
        if (!aGroupsInfo->Annotations().Annotations().empty()) {
            aGroupsInfo->Annotations().UnhilightAll(InteractiveContext()->MainPrsMgr());
        }
    }

    InteractiveContext()->SelectionManager()->Selector()->RemovePicked(this);
}

// ================================================================
// Function : UsedProps
// ================================================================
void InteractiveCaeMesh::UsedProps(int                                       theTimeStep,
                                   NCollection_Map<TCollection_AsciiString>& theComputedProps,
                                   NCollection_Map<TCollection_AsciiString>& theNotComputedProps) const
{
    theComputedProps.Clear();
    theNotComputedProps.Clear();

    for (Handle(MeshPresentationBase)& aPrs : Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->UsedProps(theTimeStep, theComputedProps, theNotComputedProps);
    }
}

// ================================================================
// Function : GetNotLoadedProps
// ================================================================
void InteractiveCaeMesh::GetNotLoadedProps(std::vector<Handle(RWCae::MeshNVecProperty)>& theProps,
                                           std::vector<Handle(RWCae::MeshNVecProperty)>& theCachedProps,
                                           std::vector<TCollection_AsciiString>&           theNotExistedProps) const
{
    const Handle(RWCae::MeshData)& aMeshData = GetMeshData();

    int aCurTimeStep = ActiveTimeStep();

    NCollection_Map<TCollection_AsciiString> aComputedProps;
    NCollection_Map<TCollection_AsciiString> aNotComputedProps;
    for (Handle(MeshPresentationBase)& aPrs : Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->UsedProps(aCurTimeStep, aComputedProps, aNotComputedProps);
    }

    for (NCollection_Map<TCollection_AsciiString>::Iterator anIter(aNotComputedProps); anIter.More(); anIter.Next()) {
        const Handle(RWCae::MeshNVecProperty)& aProp = aMeshData->GetPropertyById(anIter.Key(), aCurTimeStep);
        if (aProp.IsNull()) {
            theNotExistedProps.push_back(anIter.Key());
        } else if (!aProp->IsLoaded() && !aProp->CanBeLoaded()) {
            theProps.push_back(aProp);
        } else if (!aProp->IsLoaded() && aProp->CanBeLoaded()) {
            theCachedProps.push_back(aProp);
        }
    }
}

// ================================================================
// Function : HasNotLoadedGeom
// ================================================================
bool InteractiveCaeMesh::HasNotLoadedGeom() const
{
    if (!myMeshOwner->ToUseGenGeom()) {
        return false;
    }

    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        MeshPresentation* aPrs = dynamic_cast<MeshPresentation*>(aPrsBase.get());
        const NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>& aGenGeoms = aPrs->GetGenGeometries();
        for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(aGenGeoms); anIter.More();
             anIter.Next()) {
            const Handle(MeshPart)& aPart = aPrs->ChangeParts()((MeshPartType)anIter.Key());
            if (aPart->NeedToUpdate() && !anIter.Value()->IsLoaded()) {
                return true;
            }
        }
    }

    return false;
}

// ================================================================
// Function : SetDeferredUpdate
// ================================================================
void InteractiveCaeMesh::SetDeferredUpdate()
{
    bool isLoaded = true;
    if (!myMeshOwner->ToUseGenGeom()) {
        std::vector<Handle(RWCae::MeshNVecProperty)> aProps;
        std::vector<Handle(RWCae::MeshNVecProperty)> aCachedProps;
        std::vector<TCollection_AsciiString>           aNotExistedProps;
        GetNotLoadedProps(aProps, aCachedProps, aNotExistedProps);

        isLoaded = aProps.empty() && aCachedProps.empty() && aNotExistedProps.empty();
    } else {
        for (Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            MeshPresentation* aPrs   = dynamic_cast<MeshPresentation*>(aPrsBase.get());
            const MeshParts&  aParts = aPrs->ChangeParts();
            const NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>& aGenGeoms = aPrs->GetGenGeometries();
            for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(aGenGeoms);
                 anIter.More() && isLoaded;
                 anIter.Next()) {
                const Handle(MeshPart)& aPart = aParts((MeshPartType)anIter.Key());
                isLoaded = !aPart->NeedToUpdate() || (aPart->NeedToUpdate() && anIter.Value()->IsLoaded());
            }
            if (!isLoaded) {
                break;
            }
        }
    }

    myIsDeferredUpdate = !isLoaded;
    for (PrsMgr_Presentations::Iterator aPrsIter(myPresentations); aPrsIter.More(); aPrsIter.Next()) {
        aPrsIter.ChangeValue()->SetUpdateStatus(isLoaded);
    }
}

// ================================================================
// Function : processDeferredProps
// ================================================================
void InteractiveCaeMesh::processDeferredProps()
{
    // Gather properties that are required for calculation but not loaded.
    std::vector<Handle(RWCae::MeshNVecProperty)> aProps;
    std::vector<Handle(RWCae::MeshNVecProperty)> aCachedProps;
    std::vector<TCollection_AsciiString>           aNotExistedProps;
    GetNotLoadedProps(aProps, aCachedProps, aNotExistedProps);

    // Load properties that are cached in FS.
    if (!aCachedProps.empty()) {
        const Handle(RWCae::MeshData)&                   aMeshData = GetMeshData();
        NCollection_Map<Handle(RWCae::MeshNVecProperty)> anUpdatedProps;
        for (Handle(RWCae::MeshNVecProperty)& aProp : aCachedProps) {
            if (aProp->IsLoaded()) {
                // LoadMeshProperty would report success, but we don't need
                // to put an already loaded property into updated props array.
                continue;
            }
            if (!RWCae::MeshTools::LoadMeshProperty(aMeshData, aProp)) {
                aProps.push_back(aProp);
            } else {
                anUpdatedProps.Add(aProp);
            }
        }
        myMeshOwner->OnPropertiesUpdated(anUpdatedProps, true);
    }

    // Request properties that are not cached in FS or don't exist.
    if (!aProps.empty() || !aNotExistedProps.empty()) {
        // Clear update status, keep deferred update status.
        for (PrsMgr_Presentations::Iterator aPrsIter(myPresentations); aPrsIter.More(); aPrsIter.Next()) {
            aPrsIter.ChangeValue()->SetUpdateStatus(false);
        }

        TCollection_AsciiString aPropsStr = "";
        for (const Handle(RWCae::MeshNVecProperty)& aProp : aProps) {
            if (!aPropsStr.IsEmpty()) {
                aPropsStr += "\n";
            }
            aPropsStr += aProp->Id();
        }
        for (const TCollection_AsciiString& aPropId : aNotExistedProps) {
            if (!aPropsStr.IsEmpty()) {
                aPropsStr += "\n";
            }
            aPropsStr += aPropId;
        }

        if (myOnPropertiesRequested != nullptr) {
            myOnPropertiesRequested(ActiveTimeStep(), aPropsStr);
        }
    } else {
        // Set update status if all required properties are available.
        myIsDeferredUpdate = false;
        SetToUpdate();
    }
}

// ================================================================
// Function : processDeferredGeoms
// ================================================================
void InteractiveCaeMesh::processDeferredGeoms()
{
    for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
        MeshPresentation* aPrs   = dynamic_cast<MeshPresentation*>(aPrsBase.get());
        const MeshParts&  aParts = aPrs->ChangeParts();
        const NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>& aGenGeoms = aPrs->GetGenGeometries();
        for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(aGenGeoms); anIter.More();
             anIter.Next()) {
            const Handle(MeshPart)& aPart = aParts((MeshPartType)anIter.Key());
            if (!aPart->NeedToUpdate()) {
                continue;
            }

            if (anIter.Value()->IsLoaded()) {
                continue;
            }

            std::string aPrsParams = myMeshOwner->GetPrsParamsJson(aPrs->Id(), (MeshPartType)anIter.Key());
            if (aPrsParams.empty()) {
                continue;
            }

            if (myOnGenGeomRequested != nullptr) {
                myOnGenGeomRequested(aPrs->Id(), anIter.Key(), aPrsParams);
            }

            if (!myMeshOwner->ToUseTestCachePrs()) {
                return;
            }

            Handle(RWCae::MeshData) aGenMeshData = myMeshOwner->TestCachePrs()->GetPartGenMeshData(aPrsParams);
            if (aGenMeshData.IsNull()) {
                continue;
            }

            // Try to emulate test client-server interaction - dump and restore mesh data instead of direct assigning
            std::ostringstream     aTestCacheStrOStream;
            RWCae::MeshBinWriter aWriter(aGenMeshData);
            aWriter.Perform(aTestCacheStrOStream, "");

            std::istringstream     aTestCacheStrIStream(aTestCacheStrOStream.str());
            RWCae::MeshBinReader aReader;
            aReader.Perform(aTestCacheStrIStream, "");

            SetPartGenMeshData(aPrs->Id(), anIter.Key(), aReader.GetMeshData());
            myAnimation->Continue();
        }
    }

    myIsDeferredUpdate = false;
    SetToUpdate();
}

// ================================================================
// Function : updateColorMappers
// ================================================================
void InteractiveCaeMesh::updateColorMappers()
{
    const MeshPropertyData::MapOfColorMappers& aMappers = myMeshOwner->PropertyData()->UsedColorMappers();
    for (MeshPropertyData::MapOfColorMappers::Iterator anIter(aMappers); anIter.More(); anIter.Next()) {
        Handle(MeshColorScaleMapper) aMapper = Handle(MeshColorScaleMapper)::DownCast(anIter.Value());
        if (aMapper.IsNull()) {
            continue;
        }
        bool isVisible = false;
        for (const Handle(MeshPresentationBase)& aPrsBase : myMeshOwner->Presentations()) {
            Handle(MeshPresentation)       aPrs  = Handle(MeshPresentation)::DownCast(aPrsBase);
            RWCae::MeshPropertyComponent aProp = myMeshOwner->PropertyData()->Property(aPrs->ActiveProperty());
            if (aProp == anIter.Key() && aPrs->VisProps()->IsVisible()) {
                isVisible = true;
                break;
            }
        }

        aMapper->SetVisible(isVisible);
    }
}

// ================================================================
// Function : compute2DGrid
// ================================================================
void InteractiveCaeMesh::compute2DGrid(const Handle(Prs3d_Presentation)& thePrs)
{
    if (!myToShow2DGrid) {
        return;
    }

    // Get mesh bounding box
    const Bnd_Box aBounds = myMeshOwner->GetMeshData()->BoundingBox();
    const double  aMinX   = aBounds.CornerMin().X();
    const double  aMaxX   = aBounds.CornerMax().X();
    const double  aMinY   = aBounds.CornerMin().Y();
    const double  aMaxY   = aBounds.CornerMax().Y();
    const double  aZ      = aBounds.CornerMin().Z();

    const size_t aTickCountX = my2DAxesNames.TicksX.size();
    const size_t aTickCountY = my2DAxesNames.TicksY.size();

    const auto lerp = [](double theMin, double theMax, size_t theParam, size_t theTickCount) -> double {
        return theMin + (theMax - theMin) * (static_cast<double>(theParam) / (theTickCount - 1));
    };

    // Calculate number of primitives for the grid
    const size_t aGridVertexCount  = 2 * (aTickCountX + aTickCountY);
    const size_t aGridSegmentCount = aTickCountX + aTickCountY + 2;

    Handle(Graphic3d_ArrayOfSegments) aGridSegments = new Graphic3d_ArrayOfSegments(static_cast<int>(aGridVertexCount),
                                                                                    static_cast<int>(
                                                                                        2 * aGridSegmentCount),
                                                                                    Graphic3d_ArrayFlags_None);

    // Generate vertices.
    for (size_t anIter = 0; anIter < aTickCountX; anIter++) {
        aGridSegments->AddVertex(lerp(aMinX, aMaxX, anIter, aTickCountX), aMinY, aZ);
    }
    for (size_t anIter = 0; anIter < aTickCountX; anIter++) {
        aGridSegments->AddVertex(lerp(aMinX, aMaxX, anIter, aTickCountX), aMaxY, aZ);
    }
    for (size_t anIter = 0; anIter < aTickCountY; anIter++) {
        aGridSegments->AddVertex(aMinX, lerp(aMinY, aMaxY, anIter, aTickCountY), aZ);
    }
    for (size_t anIter = 0; anIter < aTickCountY; anIter++) {
        aGridSegments->AddVertex(aMaxX, lerp(aMinY, aMaxY, anIter, aTickCountY), aZ);
    }

    // Generate segments
    for (size_t anIter = 0; anIter < aTickCountX; anIter++) {
        aGridSegments->AddEdges(static_cast<int>(anIter + 1), static_cast<int>(anIter + aTickCountX + 1));
    }
    for (size_t anIter = 0; anIter < aTickCountY; anIter++) {
        const size_t aVertexStartY = 2 * aTickCountX;
        aGridSegments->AddEdges(static_cast<int>(aVertexStartY + anIter + 1),
                                static_cast<int>(aVertexStartY + anIter + aTickCountY + 1));
    }

    Handle(Graphic3d_Group) aGridGroup = thePrs->NewGroup();
    aGridGroup->SetGroupPrimitivesAspect(my2DGridLineAspect);
    aGridGroup->AddPrimitiveArray(aGridSegments);

    const float  aTextHeight       = static_cast<float>(my2DGridTextAspect->Height());
    const double anYLabelsOffset   = -0.05;
    const double anAxisLabelOffset = 0.4;

    for (size_t anIter = 0; anIter < aTickCountX; anIter++) {
        Graphic3d_HorizontalTextAlignment aHAlign = anIter == 0 ? Graphic3d_HTA_LEFT : Graphic3d_HTA_CENTER;
        drawGridText(thePrs,
                     my2DGridTextAspect,
                     my2DAxesNames.TicksX.at(anIter).c_str(),
                     {lerp(aMinX, aMaxX, anIter, aTickCountX), aMinY, aZ},
                     aHAlign,
                     Graphic3d_VTA_TOP,
                     aTextHeight);
    }
    for (size_t anIter = 0; anIter < aTickCountY; anIter++) {
        Graphic3d_VerticalTextAlignment aVAlign = anIter == 0 ? Graphic3d_VTA_BOTTOM : Graphic3d_VTA_CENTER;
        drawGridText(thePrs,
                     my2DGridTextAspect,
                     my2DAxesNames.TicksY.at(anIter).c_str(),
                     {aMinX + anYLabelsOffset, lerp(aMinY, aMaxY, anIter, aTickCountY), aZ},
                     Graphic3d_HTA_RIGHT,
                     aVAlign,
                     aTextHeight);
    }

    const gp_Pnt anAxesNamePosX{(aMinX + aMaxX) / 2, -anAxisLabelOffset, aZ};
    const gp_Pnt anAxesNamePosY{-anAxisLabelOffset, (aMinY + aMaxY) / 2, aZ};

    drawGridText(thePrs,
                 my2DGridTextAspect,
                 my2DAxesNames.NameX.c_str(),
                 anAxesNamePosX,
                 Graphic3d_HTA_CENTER,
                 Graphic3d_VTA_TOP,
                 aTextHeight);
    drawGridText(thePrs,
                 my2DYAxisTextAspect,
                 my2DAxesNames.NameY.c_str(),
                 anAxesNamePosY,
                 Graphic3d_HTA_CENTER,
                 Graphic3d_VTA_BOTTOM,
                 aTextHeight);

    // Here we need to extend the presentation bounding box size by some value that should depend on the text height.
    // The problem is that the two units are not comparable since zooming in will enlarge the 3D space visually
    // while text stays the same size on the screen.
    // Here we provide a multiplier that does conversion from text size in pixels to 3D coordinate units.
    // It is the amount of 3D space that roughly corresponds to 1 pixel in some reasonable range of zoom levels.
    static constexpr float aPixelSize = 0.015f;
    // An offset that depends on text height.
    const float aTH = static_cast<float>(my2DGridTextAspect->Height()) * aPixelSize;
    // Shorthands.
    const gp_Pnt& aPosX = anAxesNamePosX;
    const gp_Pnt& aPosY = anAxesNamePosY;

    // Expand grid group bounding box past label anchor points.
    // This is done so that when FitAll is activated in the viewer,
    // the furthermost text (axes names) doesn't get clipped by framebuffer edge.
    Graphic3d_BndBox4f aBox = aGridGroup->BoundingBox();
    for (const Graphic3d_Vec4d& aDummyPoint : {Graphic3d_Vec4d{aPosX.X(), aPosX.Y() - aTH, aPosX.Z(), 0},
                                               Graphic3d_Vec4d{aPosY.X() - aTH, aPosY.Y(), aPosY.Z(), 0},
                                               Graphic3d_Vec4d{aPosX.X(), aMaxY - aPosX.Y() + aTH, aPosX.Z(), 0},
                                               Graphic3d_Vec4d{aMaxX - aPosY.X() + aTH, aPosY.Y(), aPosY.Z(), 0}}) {
        aBox.Add(Graphic3d_Vec4(aDummyPoint));
    }
    aGridGroup->ChangeBoundingBox() = aBox;
}

} // namespace