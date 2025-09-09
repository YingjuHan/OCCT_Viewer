#include <ViewCae/InteractiveCaeMeshOwner.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveCaeMeshPrsMode.h>
#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationStreamlines.h>
#include <ViewCae/MeshPresentationTools.h>
#include <ViewCae/MeshPresentationVector.h>

#include <ViewCaeGeom/MeshGeometryBlock.h>
#include <ViewCaeGeom/MeshGeometryClip.h>
#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>
#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <RWCae/MeshAdaptorsIterator.h>

namespace {
//! Update time step of deformed property.
bool updateDeformedPropTimeStep(const Handle(ViewCaeGeom::MeshGeometryDeformed)& theDefGeom,
                                const Handle(RWCae::MeshData)&                  theMeshData,
                                int                                               theTimeStep)
{
    const Handle(RWCae::MeshNVecProperty)& aPrevProp = theDefGeom->DeformedProperty();
    if (aPrevProp.IsNull()) {
        return true;
    }

    const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(aPrevProp->Id(), theTimeStep);
    if (aNewProp.IsNull()) {
        return false;
    }

    theDefGeom->SetDeformedProperty(aNewProp);

    return true;
}

//! Update time step of iso property.
bool updateIsoPropTimeStep(const Handle(ViewCaeGeom::MeshGeometryIso)& theIsoGeom,
                           const Handle(RWCae::MeshData)&             theMeshData,
                           int                                          theTimeStep)
{
    const RWCae::MeshPropertyComponent& aPrevProp = theIsoGeom->Property();
    if (aPrevProp.Property.IsNull()) {
        return true;
    }

    const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(aPrevProp.Property->Id(),
                                                                                     theTimeStep);
    if (aNewProp.IsNull()) {
        return false;
    }

    theIsoGeom->SetProperty(RWCae::MeshPropertyComponent(aNewProp, aPrevProp.ComponentIndex), false);

    return true;
}

//! Update time step of vector property.
bool updateVectorPropTimeStep(const Handle(ViewCae::MeshPresentationVector)& theVectorPrs,
                              const Handle(RWCae::MeshData)&                theMeshData,
                              int                                             theTimeStep)
{
    const Handle(RWCae::MeshNVecProperty)& aPrevProp = theVectorPrs->VectorPartParams()->VectorProperty();
    if (aPrevProp.IsNull()) {
        return true;
    }

    const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(aPrevProp->Id(), theTimeStep);
    if (aNewProp.IsNull()) {
        return false;
    }

    theVectorPrs->VectorPartParams()->SetVectorProperty(aNewProp);

    return true;
}

//! Update time step of streamlines property.
bool updateStreamlinesPropTimeStep(const Handle(ViewCae::MeshPresentationStreamlines)& thePrs,
                                   const Handle(RWCae::MeshData)&                     theMeshData,
                                   int                                                  theTimeStep)
{
    const Handle(RWCae::MeshNVecProperty)& aPrevProp = thePrs->StreamlinesPartParams()->StreamlinesProperty();
    if (aPrevProp.IsNull()) {
        return true;
    }

    const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(aPrevProp->Id(), theTimeStep);
    if (aNewProp.IsNull()) {
        return false;
    }

    thePrs->StreamlinesPartParams()->SetStreamlinesProperty(aNewProp);

    return true;
}

}

namespace ViewCae {

// =======================================================================
// function : InteractiveCaeMeshOwner
// =======================================================================
InteractiveCaeMeshOwner::InteractiveCaeMeshOwner(const Handle(RWCae::MeshData)& theMeshData)
    : InteractiveOwner(theMeshData->Id())
{
    myPresentations = new MeshPresentations(theMeshData);
    myPropertyData  = new MeshPropertyData();
    SetPriority(5);
}

// =======================================================================
// function : ~InteractiveCaeMeshOwner
// =======================================================================
InteractiveCaeMeshOwner::~InteractiveCaeMeshOwner()
{
    //
}

// =======================================================================
// function : Clone
// =======================================================================
Handle(ViewBase::InteractiveOwner) InteractiveCaeMeshOwner::Clone(const TCollection_AsciiString&)
{
    return nullptr;
}

// =======================================================================
// function : CreatePrs
// =======================================================================
const TCollection_AsciiString& InteractiveCaeMeshOwner::CreatePrs(MeshPresentationType           theType,
                                                                  const TCollection_AsciiString& theId)
{
    int aPropId = myPropertyData->AddProperty(nullptr);

    Handle(ViewCaeGeom::MeshGeometry) aGeom = new ViewCaeGeom::MeshGeometryBase(GetMeshBvhData(), GetMeshData());

    Handle(MeshPresentation) aPrs;
    if (theType == MeshPresentationType::Scalar) {
        aPrs = new MeshPresentationScalar(myPropertyData, aPropId, aGeom, theId, true);
    } else if (theType == MeshPresentationType::Vector) {
        aPrs = new MeshPresentationVector(myPropertyData, aPropId, aGeom, theId, true);
    } else if (theType == MeshPresentationType::Streamlines) {
        aPrs = new MeshPresentationStreamlines(myPropertyData, aPropId, aGeom, theId, true);
    }

    Geometries().Bind(aGeom->Id(), aGeom);

    if (myToUseGenGeom) {
        aPrs->CreateGenGeometry();
    }

    aPrs->SetActiveTimeStep(myActiveTimeStep);

    aPrs->SetSelectionMode(GetSelectionMode());

    Presentations().Append(aPrs);
    return aPrs->Id();
}

// =======================================================================
// function : RemovePresentation
// =======================================================================
void InteractiveCaeMeshOwner::RemovePresentation(const TCollection_AsciiString& thePrsId)
{
    for (ListOfMeshPresentations::Iterator anIter(Presentations()); anIter.More(); anIter.Next()) {
        const MeshPresentation* aPrs = dynamic_cast<const MeshPresentation*>(anIter.Value().get());
        if (aPrs->Id() == thePrsId) {
            myPropertyData->RemoveProperty(aPrs->ActiveProperty());
        }
    }

    myPresentations->RemovePresentation(thePrsId);
}

// =======================================================================
// function : Presentation
// =======================================================================
Handle(MeshPresentation) InteractiveCaeMeshOwner::Presentation(const TCollection_AsciiString& thePrsId) const
{
    return Handle(MeshPresentation)::DownCast(myPresentations->Presentation(thePrsId));
}

// =======================================================================
// function : PresentationByIndex
// =======================================================================
Handle(MeshPresentation) InteractiveCaeMeshOwner::PresentationByIndex(int theIndex) const
{
    int anIndex = 0;
    for (const Handle(MeshPresentationBase)& aPrs : Presentations()) {
        if (anIndex++ == theIndex) {
            return Handle(MeshPresentation)::DownCast(aPrs);
        }
    }
    return nullptr;
}

// ================================================================
// Function : GetPrsParamsJson
// ================================================================
std::string InteractiveCaeMeshOwner::GetPrsParamsJson(const TCollection_AsciiString& thePrsId,
                                                      MeshPartType                   thePart) const
{
    std::string aRes = "";

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return aRes;
    }

    if (!MeshPresentationTools::DumpToJson(aPrs, thePart, aRes)) {
        return "";
    }

    return aRes;
}

// ================================================================
// Function : SetUseTestCacheOwner
// ================================================================
void InteractiveCaeMeshOwner::SetUseTestCachePrs(bool theToUse)
{
    if (theToUse == ToUseTestCachePrs()) {
        return;
    }

    SetUseGenGeom(theToUse);

    if (!theToUse) {
        myTestCachePrs.Nullify();
        return;
    }

    myTestCachePrs = new MeshPresentations(GetMeshData());
    myTestCachePrs->SetCacheProperties(myPresentations->ToCacheProperties());
}

// =======================================================================
// function : Property
// =======================================================================
RWCae::MeshPropertyComponent InteractiveCaeMeshOwner::Property(const TCollection_AsciiString& thePrsId) const
{
    return myPropertyData->Property(Presentation(thePrsId)->ActiveProperty());
}

// =======================================================================
// function : SetProperty
// =======================================================================
void InteractiveCaeMeshOwner::SetProperty(const TCollection_AsciiString&           thePrsId,
                                          const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    const Handle(MeshPresentation)& aPrs = Presentation(thePrsId);
    myPropertyData->SetProperty(theProperty, aPrs->ActiveProperty());
    aPrs->SetToUpdate(true, true);
}

// =======================================================================
// function : SetPropertyComponentIndex
// =======================================================================
void InteractiveCaeMeshOwner::SetPropertyComponentIndex(const TCollection_AsciiString& thePrsId, const int theIndex)
{
    const Handle(MeshPresentation)& aPrs = Presentation(thePrsId);
    myPropertyData->SetPropertyComponentIndex(theIndex, aPrs->ActiveProperty());
    aPrs->SetToUpdate(true, true);
}

// ================================================================
// Function : DeformedProperty
// ================================================================
Handle(RWCae::MeshNVecProperty) InteractiveCaeMeshOwner::DeformedProperty(
    const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return nullptr;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            return aDefGeom->DeformedProperty();
        }
    }

    return nullptr;
}

// ================================================================
// Function : SetDeformedProperty
// ================================================================
void InteractiveCaeMeshOwner::SetDeformedProperty(const TCollection_AsciiString&           thePrsId,
                                                  const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            if (!theProperty.IsNull()) {
                aDefGeom->SetDeformedProperty(theProperty);
            } else {
                myPresentations->OnRemoveMeshGeometry(aPrs->GetMeshGeometry());
                aDefGeom->SetDeformedProperty(nullptr);
                aPrs->GetMeshGeometry()->Remove(aDefGeom->Id());
            }

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }

    if (theProperty.IsNull()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometryDeformed) aDefGeom = new ViewCaeGeom::MeshGeometryDeformed(GetMeshBvhData());
    aPrs->AppendMeshGeometry(aDefGeom, true);
    aDefGeom->SetDeformedProperty(theProperty);
    aDefGeom->SetDeformedScaleCoeff(0.0);

    updatePresentations(aPrs->GetMeshGeometry());
}

// ================================================================
// Function : DeformedScaleCoeff
// ================================================================
double InteractiveCaeMeshOwner::DeformedScaleCoeff(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return 0.0;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            return aDefGeom->DeformedScaleCoeff();
        }
    }

    return 0.0;
}

// ================================================================
// Function : SetDeformedScaleCoeff
// ================================================================
void InteractiveCaeMeshOwner::SetDeformedScaleCoeff(const TCollection_AsciiString& thePrsId, double theCoeff)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            aDefGeom->SetDeformedScaleCoeff(theCoeff);

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }
}

// ================================================================
// Function : ClipPlane
// ================================================================
Handle(Graphic3d_ClipPlane) InteractiveCaeMeshOwner::ClipPlane(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return nullptr;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryClip* aClipGeom = dynamic_cast<ViewCaeGeom::MeshGeometryClip*>(aGeom)) {
            return aClipGeom->ClipPlane();
        }
    }

    return nullptr;
}

// ================================================================
// Function : SetClipPlane
// ================================================================
void InteractiveCaeMeshOwner::SetClipPlane(const TCollection_AsciiString&     thePrsId,
                                           const Handle(Graphic3d_ClipPlane)& theClipPlane)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryClip* aClipGeom = dynamic_cast<ViewCaeGeom::MeshGeometryClip*>(aGeom)) {
            if (!theClipPlane.IsNull()) {
                aClipGeom->SetClipPlane(theClipPlane);
            } else {
                myPresentations->OnRemoveMeshGeometry(aClipGeom);
                aClipGeom->SetClipPlane(nullptr);
                aPrs->GetMeshGeometry()->Remove(aClipGeom->Id());
            }

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }

    if (theClipPlane.IsNull()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometryClip) aClipGeom = new ViewCaeGeom::MeshGeometryClip(
        myPresentations->GetMeshBvhData());
    aPrs->AppendMeshGeometry(aClipGeom, true);
    aClipGeom->SetClipPlane(theClipPlane);

    updatePresentations(aPrs->GetMeshGeometry());
}

// ================================================================
// Function : ToCutOnClipPlane
// ================================================================
bool InteractiveCaeMeshOwner::ToCutOnClipPlane(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryClip* aClipGeom = dynamic_cast<ViewCaeGeom::MeshGeometryClip*>(aGeom)) {
            return aClipGeom->ToCut();
        }
    }

    return false;
}

// ================================================================
// Function : SetCutOnClipPlane
// ================================================================
void InteractiveCaeMeshOwner::SetCutOnClipPlane(const TCollection_AsciiString& thePrsId, bool theToCut)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryClip* aClipGeom = dynamic_cast<ViewCaeGeom::MeshGeometryClip*>(aGeom)) {
            aClipGeom->SetCut(theToCut);

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }
}

// ================================================================
// Function : SlicePlanes
// ================================================================
Graphic3d_SequenceOfHClipPlane InteractiveCaeMeshOwner::SlicePlanes(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return Graphic3d_SequenceOfHClipPlane();
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometrySlice* aSliceGeom = dynamic_cast<ViewCaeGeom::MeshGeometrySlice*>(aGeom)) {
            return aSliceGeom->SlicePlanes();
        }
    }

    return Graphic3d_SequenceOfHClipPlane();
}

// ================================================================
// Function : SetSlicePlanes
// ================================================================
void InteractiveCaeMeshOwner::SetSlicePlanes(const TCollection_AsciiString&        thePrsId,
                                             const Graphic3d_SequenceOfHClipPlane& theSlicePlanes)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometrySlice* aSliceGeom = dynamic_cast<ViewCaeGeom::MeshGeometrySlice*>(aGeom)) {
            if (!theSlicePlanes.IsEmpty()) {
                aSliceGeom->SetSlicePlanes(theSlicePlanes);
            } else {
                myPresentations->OnRemoveMeshGeometry(aSliceGeom);
                aSliceGeom->SetSlicePlanes(Graphic3d_SequenceOfHClipPlane());
                aPrs->GetMeshGeometry()->Remove(aSliceGeom->Id());
            }

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }

    if (theSlicePlanes.IsEmpty()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometrySlice) aSliceGeom = new ViewCaeGeom::MeshGeometrySlice(
        myPresentations->GetMeshBvhData());
    aPrs->AppendMeshGeometry(aSliceGeom, true);
    aSliceGeom->SetSlicePlanes(theSlicePlanes);

    updatePresentations(aPrs->GetMeshGeometry());
}

// ================================================================
// Function : IsoProperty
// ================================================================
RWCae::MeshPropertyComponent InteractiveCaeMeshOwner::IsoProperty(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return RWCae::MeshPropertyComponent();
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            return anIsoGeom->Property();
        }
    }

    return RWCae::MeshPropertyComponent();
}

// ================================================================
// Function : SetIsoProperty
// ================================================================
void InteractiveCaeMeshOwner::SetIsoProperty(const TCollection_AsciiString&        thePrsId,
                                             const RWCae::MeshPropertyComponent& theIsoProperty)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            if (!theIsoProperty.Property.IsNull()) {
                anIsoGeom->SetProperty(theIsoProperty);
            } else {
                myPresentations->OnRemoveMeshGeometry(anIsoGeom);
                anIsoGeom->SetProperty(theIsoProperty);
                aPrs->GetMeshGeometry()->Remove(anIsoGeom->Id());
            }

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }

    if (theIsoProperty.Property.IsNull()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometryIso) anIsoGeom = new ViewCaeGeom::MeshGeometryIso(
        myPresentations->GetMeshBvhData());
    aPrs->AppendMeshGeometry(anIsoGeom, true);
    anIsoGeom->SetProperty(theIsoProperty);

    updatePresentations(aPrs->GetMeshGeometry());
}

// ================================================================
// Function : IsoValues
// ================================================================
std::vector<double> InteractiveCaeMeshOwner::IsoValues(const TCollection_AsciiString& thePrsId) const
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return std::vector<double>();
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            return anIsoGeom->IsoValues();
        }
    }

    return std::vector<double>();
}

// ================================================================
// Function : SetIsoValues
// ================================================================
void InteractiveCaeMeshOwner::SetIsoValues(const TCollection_AsciiString& thePrsId,
                                           const std::vector<double>&     theIsoValues)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            anIsoGeom->SetIsoValues(theIsoValues);

            updatePresentations(aPrs->GetMeshGeometry());
            return;
        }
    }
}

// ================================================================
// Function : VisibleBlocks
// ================================================================
Handle(RWCae::MeshDataBlock) InteractiveCaeMeshOwner::VisibleBlocks(const TCollection_AsciiString& thePrsId) const
{
    Handle(RWCae::MeshDataBlock) aRes = new RWCae::MeshDataBlock(GetMeshData()->Blocks()->Name());

    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return aRes;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryBlock* aBlockGeom = dynamic_cast<ViewCaeGeom::MeshGeometryBlock*>(aGeom)) {
            return aBlockGeom->GetEnabled();
        }
    }

    return aRes;
}

// ================================================================
// Function : SetBlocksVisible
// ================================================================
bool InteractiveCaeMeshOwner::SetBlocksVisible(const TCollection_AsciiString&        thePrsId,
                                               const Handle(RWCae::MeshDataBlock)& theBlocks,
                                               bool                                  theVisible)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return false;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
        if (ViewCaeGeom::MeshGeometryBlock* aBlockGeom = dynamic_cast<ViewCaeGeom::MeshGeometryBlock*>(aGeom)) {
            if (!aBlockGeom->SetEnabled(theBlocks, theVisible)) {
                return false;
            }
            if (aBlockGeom->IsAllEnabled()) {
                aPrs->GetMeshGeometry()->Remove(aBlockGeom->Id());
            }

            updatePresentations(aPrs->GetMeshGeometry());
            return true;
        }
    }

    Handle(ViewCaeGeom::MeshGeometryBlock) aBlockGeom = new ViewCaeGeom::MeshGeometryBlock(GetMeshBvhData(),
                                                                                             GetMeshData()->Blocks());
    aBlockGeom->SetEnabled(theBlocks, theVisible);
    if (aBlockGeom->IsAllEnabled()) {
        return false;
    }

    aPrs->AppendMeshGeometry(aBlockGeom, true);

    updatePresentations(aPrs->GetMeshGeometry());
    return true;
}

// ================================================================
// Function : SetActiveTimeStep
// ================================================================
bool InteractiveCaeMeshOwner::SetActiveTimeStep(const int theTimeStep)
{
    myActiveTimeStep = theTimeStep;

    const Handle(RWCae::MeshData)& aMeshData = GetMeshData();

    bool aRes = myPropertyData->UpdatePropertiesTimeStep(GetMeshData(), myActiveTimeStep);

    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        Handle(MeshPresentation) aPrs = Handle(MeshPresentation)::DownCast(aPrsBase);
        if (aPrs.IsNull()) {
            continue;
        }
        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aPrs->GetMeshGeometry().get())) {
            if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(
                    aGeom)) {
                aRes = updateDeformedPropTimeStep(aDefGeom, aMeshData, myActiveTimeStep) && aRes;
            } else if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
                aRes = updateIsoPropTimeStep(anIsoGeom, aMeshData, myActiveTimeStep) && aRes;
            }
        }

        if (Handle(MeshPresentationVector) aVectorPrs = Handle(MeshPresentationVector)::DownCast(aPrs)) {
            aRes = updateVectorPropTimeStep(aVectorPrs, aMeshData, myActiveTimeStep) && aRes;
        }

        if (Handle(MeshPresentationStreamlines) aStreamlinesPrs = Handle(MeshPresentationStreamlines)::DownCast(aPrs)) {
            aRes = updateStreamlinesPropTimeStep(aStreamlinesPrs, aMeshData, myActiveTimeStep) && aRes;
        }

        aPrs->SetActiveTimeStep(theTimeStep);
        aPrs->SetToUpdate(false, true);
    }

    return aRes;
}

// ================================================================
// Function : AvailableTimeSteps
// ================================================================
const std::vector<std::pair<int, double>>& InteractiveCaeMeshOwner::AvailableTimeSteps()
{
    if (myIsOwnTimeSteps) {
        return myTimeSteps;
    }

    myTimeSteps.clear();

    TColStd_PackedMapOfInteger anAddedTimeSteps;
    for (const Handle(RWCae::MeshNVecProperty)& aProp : GetMeshData()->Properties()) {
        if (aProp->HasTimeStep() && anAddedTimeSteps.Add(aProp->TimeStep())) {
            myTimeSteps.push_back(std::make_pair(aProp->TimeStep(), aProp->Time()));
        }
    }

    auto sortTimeSteps = [](const std::pair<int, double>& theTime1, const std::pair<int, double>& theTime2) -> bool {
        return theTime1.first < theTime2.first;
    };

    std::sort(myTimeSteps.begin(), myTimeSteps.end(), sortTimeSteps);
    return myTimeSteps;
}

// =======================================================================
// function : SetOwnTimeSteps
// =======================================================================
void InteractiveCaeMeshOwner::SetOwnTimeSteps(const std::vector<std::pair<int, double>>& theTimeSteps)
{
    myIsOwnTimeSteps = !theTimeSteps.empty();

    myTimeSteps = theTimeSteps;

    auto sortTimeSteps = [](const std::pair<int, double>& theTime1, const std::pair<int, double>& theTime2) -> bool {
        return theTime1.first < theTime2.first;
    };

    std::sort(myTimeSteps.begin(), myTimeSteps.end(), sortTimeSteps);
}

// ================================================================
// Function : BoundingBox
// ================================================================
const Bnd_Box& InteractiveCaeMeshOwner::BoundingBox()
{
    if (myBoundingBoxUpdated) {
        return myBoundingBox;
    }

    myBoundingBox.SetVoid();

    auto processGeom = [&aBox = myBoundingBox](const Handle(MeshPresentation)&           thePrs,
                                               const Handle(ViewCaeGeom::MeshGeometry)& theGeom) {
        Handle(MeshPrsVisScalarProps) aScalarProps = Handle(MeshPrsVisScalarProps)::DownCast(thePrs->VisProps());

        bool toProcessFreeNodes = !aScalarProps.IsNull() ? (aScalarProps->ElementTypes()
                                                            & (unsigned int)InteractiveCaeMeshElementType::FreeNode)
                                                               != 0
                                                         : true;

        const Handle(RWCae::MeshData)& aMeshData = theGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            return;
        }

        int aNodeLower = aMeshData->Nodes().Lower();
        int aNodeUpper = aMeshData->Nodes().Upper();

        for (int aNodeIter = aNodeLower; aNodeIter <= aNodeUpper; ++aNodeIter) {
            if (!toProcessFreeNodes && aMeshData->IsNodeFree(aNodeIter)) {
                continue;
            }
            Graphic3d_Vec3 aNode = aMeshData->Nodes().Value(aNodeIter);
            theGeom->ModifyNode(aNodeIter, aNode);
            aBox.Add(gp_Pnt(aNode.x(), aNode.y(), aNode.z()));
        }
    };

    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        Handle(MeshPresentation) aPrs = Handle(MeshPresentation)::DownCast(aPrsBase);
        if (myToUseGenGeom) {
            const NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>& aGeoms = aPrs->GetGenGeometries();
            for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(aGeoms);
                 anIter.More();
                 anIter.Next()) {
                processGeom(aPrs, anIter.Value());
            }
        } else {
            processGeom(aPrs, aPrs->GetMeshGeometry());
        }
    }

    myBoundingBoxUpdated = true;

    return myBoundingBox;
}

// ================================================================
// Function : OnPropertiesUpdated
// ================================================================
void InteractiveCaeMeshOwner::OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                                  bool theCachedProps)
{
    if (theProps.IsEmpty()) {
        return;
    }

    for (Handle(MeshPresentationBase)& aPrs : Presentations()) {
        Handle(MeshPresentation)::DownCast(aPrs)->OnPropertiesUpdated(theProps, theCachedProps);
    }
}

// ================================================================
// Function : SetSelectionMode
// ================================================================
void InteractiveCaeMeshOwner::SetSelectionMode(const int theMode)
{
    if (mySelMode == theMode) {
        return;
    }
    InteractiveOwner::SetSelectionMode(theMode);

    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        dynamic_cast<MeshPresentation*>(aPrsBase.get())->SetSelectionMode(theMode);
    }

    Handle(InteractiveCaeMesh) anObj = Handle(InteractiveCaeMesh)::DownCast(mySelectable);
    if (!anObj.IsNull() && anObj->HasInteractiveContext()) {
        anObj->UpdateAspects();
        anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
    }
}

// ================================================================
// Function : EnableBoxSelectionMode
// ================================================================
void InteractiveCaeMeshOwner::EnableBoxSelectionMode(bool theToEnable)
{
    int aSelMode = GetSelectionMode();
    aSelMode     = theToEnable ? (aSelMode | (int)InteractiveCaeMeshSelMode::Box)
                               : (aSelMode & ~(int)InteractiveCaeMeshSelMode::Box);
    InteractiveOwner::SetSelectionMode(aSelMode);

    Handle(InteractiveCaeMesh) anObj = Handle(InteractiveCaeMesh)::DownCast(mySelectable);
    if (!anObj.IsNull() && anObj->HasInteractiveContext()) {
        anObj->UpdateAspects();
        anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
    }
}

// ================================================================
// Function : GetPrsSelectionMode
// ================================================================
int InteractiveCaeMeshOwner::GetPrsSelectionMode(const TCollection_AsciiString& thePrsId)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    return !aPrs.IsNull() ? aPrs->SelectionMode() : 0;
}

// ================================================================
// Function : SetPrsSelectionMode
// ================================================================
void InteractiveCaeMeshOwner::SetPrsSelectionMode(const TCollection_AsciiString& thePrsId, const int theMode)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    aPrs->SetSelectionMode(theMode);

    Handle(InteractiveCaeMesh) anObj = Handle(InteractiveCaeMesh)::DownCast(mySelectable);
    if (!anObj.IsNull() && anObj->HasInteractiveContext()) {
        anObj->UpdateAspects();
        anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
    }
}

// =======================================================================
// function : HilightWithColor
// =======================================================================
void InteractiveCaeMeshOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                               const Handle(Prs3d_Drawer)&,
                                               const Standard_Integer)
{
    if (thePrsMgr->IsImmediateModeOn()) {
        SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart((int)MeshEntityType::UNKNOWN));
    }

    InteractiveCaeMesh* anObj = dynamic_cast<InteractiveCaeMesh*>(mySelectable);
    if (anObj == nullptr || !anObj->IsEnabled()) {
        return;
    } else if (!anObj->HasInteractiveContext() || anObj->InteractiveContext()->SelectionManager().IsNull()
               || anObj->InteractiveContext()->SelectionManager()->Selector().IsNull()) {
        mySelectedEntities.Clear();
        myDetectedEntity.Reset();
        return;
    }

    const bool            isSelection = !thePrsMgr->IsImmediateModeOn();
    AIS_SelectionScheme   aSelScheme  = GetCurrentSelectionScheme();
    MapOfSelectedEntities anOldSel;
    if (isSelection) {
        MarkSelected(true);
        for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
            auto aPrs = dynamic_cast<MeshPresentation*>(aPrsBase.get());
            if (aPrs == nullptr) {
                continue;
            }
            aPrs->SetHighlightType(MeshHighlightingType::None);
        }
        anObj->UpdateAspects();
        if (aSelScheme == AIS_SelectionScheme_XOR) {
            std::swap(anOldSel, mySelectedEntities);
        } else if (aSelScheme != AIS_SelectionScheme_Add) {
            mySelectedEntities.Clear();
        }
    } else {
        myDetectedEntity.Reset();
    }

    if ((mySelMode & (int)InteractiveCaeMeshSelMode::Box) != 0) {
        return;
    }

    if (!myToHighlightEntities) {
        return;
    }

    detectSubEntities(anObj->InteractiveContext(),
                      !isSelection ? &myDetectedEntity : nullptr,
                      isSelection ? &mySelectedEntities : nullptr);
    applySelectionScheme(anOldSel, aSelScheme);

    HighlightSubEntities(thePrsMgr, isSelection);
}

// =======================================================================
// function : Unhilight
// =======================================================================
void InteractiveCaeMeshOwner::Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Standard_Integer                    theMode)
{
    mySelectedEntities.Clear();
    myDetectedEntity.Reset();

    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        auto aPrs = dynamic_cast<MeshPresentation*>(aPrsBase.get());
        if (aPrs == nullptr) {
            continue;
        }
        aPrs->SetHighlightType(MeshHighlightingType::None);
    }

    InteractiveCaeMesh* anObj = dynamic_cast<InteractiveCaeMesh*>(mySelectable);
    if (anObj != nullptr) {
        MarkSelected(false);
        anObj->UpdateAspects();

        if (Handle(Prs3d_Presentation) aPrs = anObj->GetSelectPresentation(Handle(PrsMgr_PresentationManager)())) {
            aPrs->Erase();
        }
        if (Handle(Prs3d_Presentation) aPrs = anObj->GetHilightPresentation(Handle(PrsMgr_PresentationManager)())) {
            aPrs->Erase();
        }
    }
    SelectMgr_EntityOwner::Unhilight(thePrsMgr, theMode);
}

// =======================================================================
// function : applySelectionScheme
// =======================================================================
void InteractiveCaeMeshOwner::applySelectionScheme(const MapOfSelectedEntities& thePrevSelected,
                                                   AIS_SelectionScheme          theSelScheme)
{
    if (theSelScheme == AIS_SelectionScheme_XOR && thePrevSelected.Size() > 0) {
        for (MapOfSelectedEntities::Iterator anIter(thePrevSelected); anIter.More(); anIter.Next()) {
            if (mySelectedEntities.Contains(anIter.Value())) {
                mySelectedEntities.Remove(anIter.Value());
            } else {
                mySelectedEntities.Add(anIter.Value());
            }
        }
    }
}

// =======================================================================
// function : detectSubEntities
// =======================================================================
void InteractiveCaeMeshOwner::detectSubEntities(const Handle(AIS_InteractiveContext)& theCtx,
                                                MeshSubEntity*                        theDet,
                                                MapOfSelectedEntities*                theSel) const
{
    const Handle(SelectMgr_ViewerSelector)& aSelector = theCtx->MainSelector();

    bool isPointSel = aSelector->GetManager().GetActiveSelectionType() == SelectMgr_SelectionType_Point;
    if (isPointSel && theCtx->DetectedOwner() != this) {
        return;
    }

    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        if (aSelector->Picked(aPickIter) != this) {
            continue;
        }

        const Handle(Select3D_SensitiveEntity)& anEntity    = aSelector->PickedEntity(aPickIter);
        const SelectMgr_SortCriterion&          aPickedData = aSelector->PickedData(aPickIter);
        Graphic3d_Vec3 aPnt((float)aPickedData.Point.X(), (float)aPickedData.Point.Y(), (float)aPickedData.Point.Z());

        DetectEntity(anEntity, aPnt, theDet, theSel);

        if (theDet != nullptr || isPointSel) {
            return;
        }
    }
}

// =======================================================================
// function : DetectEntity
// =======================================================================
void InteractiveCaeMeshOwner::DetectEntity(const Handle(Select3D_SensitiveEntity)& theEnt,
                                           const Graphic3d_Vec3&                   thePnt,
                                           MeshSubEntity*                          theDet,
                                           MapOfSelectedEntities*                  theSel) const
{
    if (Handle(MeshAdaptorSensTriangle) aSensTriangle = Handle(MeshAdaptorSensTriangle)::DownCast(theEnt)) {
        detectSubElementsT(aSensTriangle, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensTriangle2nd) aSensTriangle2nd = Handle(MeshAdaptorSensTriangle2nd)::DownCast(
                   theEnt)) {
        detectSubElementsT(aSensTriangle2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensQuad) aSensQuad = Handle(MeshAdaptorSensQuad)::DownCast(theEnt)) {
        detectSubElementsT(aSensQuad, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensQuad2nd) aSensQuad2nd = Handle(MeshAdaptorSensQuad2nd)::DownCast(theEnt)) {
        detectSubElementsT(aSensQuad2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPolygon) aSensPolygon = Handle(MeshAdaptorSensPolygon)::DownCast(theEnt)) {
        detectSubElementsT(aSensPolygon, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSens0d) aSens0d = Handle(MeshAdaptorSens0d)::DownCast(theEnt)) {
        detectSubElementsT(aSens0d, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensTetrahedron) aSensTetra = Handle(MeshAdaptorSensTetrahedron)::DownCast(theEnt)) {
        detectSubElementsT(aSensTetra, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensTetrahedron2nd) aSensTetra2nd = Handle(MeshAdaptorSensTetrahedron2nd)::DownCast(
                   theEnt)) {
        detectSubElementsT(aSensTetra2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPyramid) aSensPyr = Handle(MeshAdaptorSensPyramid)::DownCast(theEnt)) {
        detectSubElementsT(aSensPyr, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPyramid2nd) aSensPyr2nd = Handle(MeshAdaptorSensPyramid2nd)::DownCast(theEnt)) {
        detectSubElementsT(aSensPyr2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPrism) aSensPrism = Handle(MeshAdaptorSensPrism)::DownCast(theEnt)) {
        detectSubElementsT(aSensPrism, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPrism2nd) aSensPrism2nd = Handle(MeshAdaptorSensPrism2nd)::DownCast(theEnt)) {
        detectSubElementsT(aSensPrism2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensHexahedron) aSensHexa = Handle(MeshAdaptorSensHexahedron)::DownCast(theEnt)) {
        detectSubElementsT(aSensHexa, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensHexahedron2nd) aSensHexa2nd = Handle(MeshAdaptorSensHexahedron2nd)::DownCast(
                   theEnt)) {
        detectSubElementsT(aSensHexa2nd, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensPolyhedron) aSensPoly = Handle(MeshAdaptorSensPolyhedron)::DownCast(theEnt)) {
        detectSubElementsT(aSensPoly, thePnt, theDet, theSel);
    } else if (Handle(MeshAdaptorSensitiveNode) aSensNode = Handle(MeshAdaptorSensitiveNode)::DownCast(theEnt)) {
        detectSubNodes(aSensNode, thePnt, theDet, theSel);
    }
}

// =======================================================================
// function : detectSubElementsT
// =======================================================================
template<class Elem_T>
void InteractiveCaeMeshOwner::detectSubElementsT(
    const opencascade::handle<MeshAdaptorSensitiveElement<Elem_T>>& theSens,
    const Graphic3d_Vec3&                                           thePnt,
    MeshSubEntity*                                                  theDet,
    MapOfSelectedEntities*                                          theSel) const
{
    if (theSens->GetDetectedElement() != -1) {
        const int anElemIndex = theSens->GetDetectedElement();
        detectSubElementT(theSens, thePnt, anElemIndex, theDet, theSel);
        return;
    }

    if (theSens->GetDetectedElements().IsEmpty()) {
        return;
    }

    for (TColStd_PackedMapOfInteger::Iterator anIter(theSens->GetDetectedElements()); anIter.More(); anIter.Next()) {
        const int anElemIndex = anIter.Key();
        detectSubElementT(theSens, thePnt, anElemIndex, theDet, theSel);
    }
}

// =======================================================================
// function : detectSubElementT
// =======================================================================
template<class Elem_T>
void InteractiveCaeMeshOwner::detectSubElementT(const opencascade::handle<MeshAdaptorSensitiveElement<Elem_T>>& theSens,
                                                const Graphic3d_Vec3&                                           thePnt,
                                                RWCae::MeshEntityId  theElemIdx,
                                                MeshSubEntity*         theDet,
                                                MapOfSelectedEntities* theSel) const
{
    const MeshPresentation*           aPrs  = theSens->MeshPrs();
    const ViewCaeGeom::MeshGeometry* aGeom = theSens->MeshGeom();

    MeshSubEntity aSubEntity;

    ViewCaeGeom::MeshGeomElem anElemInfo(theElemIdx,
                                          theSens->Adaptor().GetElementType(),
                                          theSens->Adaptor().IsSurface());

    int aSelMode = aPrs->SelectionMode();
    if ((aSelMode & (int)InteractiveCaeMeshSelMode::Elements) != 0) {
        aSubEntity.SetElem(aPrs, aGeom, thePnt, anElemInfo);
    } else if ((aSelMode & (int)InteractiveCaeMeshSelMode::Presentation) != 0) {
        aSubEntity.SetPresentationViaElem(aPrs, aGeom, thePnt, anElemInfo);
    } else {
        return;
    }

    if (theSel != nullptr) {
        if (!aSubEntity.IsPresentation()) {
            retrieveEntityFromTestCachePrs(aPrs, aSubEntity);
        }
        theSel->Add(aSubEntity);
    } else {
        *theDet = aSubEntity;
    }
}

// =======================================================================
// function : detectSubNodes
// =======================================================================
void InteractiveCaeMeshOwner::detectSubNodes(const Handle(MeshAdaptorSensitiveNode)& theSens,
                                             const Graphic3d_Vec3&                   thePnt,
                                             MeshSubEntity*                          theDet,
                                             MapOfSelectedEntities*                  theSel) const
{
    if (theSens->GetDetectedNode() != -1) {
        const int aNodeIndex = theSens->GetDetectedNode();
        detectSubNode(theSens, thePnt, aNodeIndex, theDet, theSel);
        return;
    }

    if (theSens->GetDetectedElements().IsEmpty()) {
        return;
    }

    for (TColStd_PackedMapOfInteger::Iterator anIter(theSens->GetDetectedElements()); anIter.More(); anIter.Next()) {
        const int aNodeIndex = anIter.Key();
        detectSubNode(theSens, thePnt, aNodeIndex, theDet, theSel);
    }
}

// =======================================================================
// function : detectSubNode
// =======================================================================
void InteractiveCaeMeshOwner::detectSubNode(const Handle(MeshAdaptorSensitiveNode)& theSens,
                                            const Graphic3d_Vec3&                   thePnt,
                                            const int                               theNodeIndex,
                                            MeshSubEntity*                          theDet,
                                            MapOfSelectedEntities*                  theSel) const
{
    const MeshPresentation*           aPrs  = theSens->MeshPrs();
    const ViewCaeGeom::MeshGeometry* aGeom = theSens->MeshGeom();

    MeshSubEntity aSubEntity;

    int aSelMode = aPrs->SelectionMode();
    if ((aSelMode & (int)InteractiveCaeMeshSelMode::Nodes) != 0) {
        aSubEntity.SetNode(aPrs, aGeom, thePnt, theNodeIndex);
    } else if ((aSelMode & (int)InteractiveCaeMeshSelMode::Presentation) != 0) {
        aSubEntity.SetPresentationViaNode(aPrs, aGeom, thePnt, theNodeIndex);
    } else {
        return;
    }

    if (theSel != nullptr) {
        if (!aSubEntity.IsPresentation()) {
            retrieveEntityFromTestCachePrs(aPrs, aSubEntity);
        }
        theSel->Add(aSubEntity);
    } else {
        *theDet = aSubEntity;
    }
}

// =======================================================================
// function : retrieveEntityFromTestCachePrs
// =======================================================================
void InteractiveCaeMeshOwner::retrieveEntityFromTestCachePrs(const MeshPresentation* thePrs,
                                                             MeshSubEntity&          theEntity) const
{
    if (!myToUseGenGeom || !ToUseTestCachePrs()) {
        return;
    }

    int aPartId = -1;
    for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(thePrs->GetGenGeometries());
         anIter.More();
         anIter.Next()) {
        if (anIter.Value()->Id() == theEntity.RefMeshGeom()) {
            aPartId = anIter.Key();
            break;
        }
    }

    if (aPartId == -1) {
        return;
    }

    std::string aPrsInfo = "";
    ViewCae::MeshPresentationTools::DumpToJson(thePrs, (ViewCae::MeshPartType)aPartId, aPrsInfo);

    MeshEntityType anEntType = MeshEntityType::UNKNOWN;

    if (theEntity.IsElement()) {
        anEntType = (MeshEntityType)((int)theEntity.ElemType() + MeshEntityTypeElemLower);
    } else if (theEntity.IsNode()) {
        anEntType = MeshEntityType::NODE;
    }

    Handle(RWCae::MeshData) aSubEntInfo = myTestCachePrs->GetMeshEntityInfo(aPrsInfo,
                                                                              theEntity.Pos(),
                                                                              theEntity.Index(),
                                                                              (int)anEntType,
                                                                              theEntity.IsElemSurf());
    if (!aSubEntInfo.IsNull()) {
        theEntity.SetRefMeshData(aSubEntInfo);
    }
}

// =======================================================================
// function : updateSelectionPart
// =======================================================================
void InteractiveCaeMeshOwner::updateSelectionPart(const MeshSubEntity& theSubEntity)
{
    if (!theSubEntity.IsDefined()) {
        return;
    }

    MeshEntityType aSelType = MeshEntityType::UNKNOWN;

    if (theSubEntity.IsElement()) {
        aSelType = (MeshEntityType)((int)theSubEntity.ElemType() + MeshEntityTypeElemLower);
    } else if (theSubEntity.IsNode()) {
        aSelType = MeshEntityType::NODE;
    }

    SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart((int)aSelType, theSubEntity.Index()));
}

// =======================================================================
// function : highlightSubEntities
// =======================================================================
void InteractiveCaeMeshOwner::HighlightSubEntities(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                   const bool                                theIsSelection)
{
    InteractiveCaeMesh* anObj = dynamic_cast<InteractiveCaeMesh*>(mySelectable);
    if (anObj == nullptr || !anObj->IsEnabled()) {
        return;
    }

    NCollection_DataMap<TCollection_AsciiString, SubEntityPrimitiveArrays> aPrimitives;

    if (theIsSelection) {
        for (MapOfSelectedEntities::Iterator anIter(mySelectedEntities); anIter.More(); anIter.Next()) {
            fillSubEntityPrimitives(anIter.Value(), aPrimitives, theIsSelection);
        }
    } else {
        fillSubEntityPrimitives(myDetectedEntity, aPrimitives, theIsSelection);
    }

    Handle(Prs3d_Drawer)       aStyle = theIsSelection ? anObj->HilightAttributes() : anObj->DynamicHilightAttributes();
    Handle(Prs3d_Presentation) aPrs   = GetHighlightPresentation(thePrsMgr, aStyle, theIsSelection);

    NCollection_DataMap<TCollection_AsciiString, SubEntityPrimitiveArrays>::Iterator iter(aPrimitives);
    while (iter.More())
    {
        HighlightPrimitives(iter.Key(), iter.Value(), aPrs, theIsSelection);
        iter.Next();
    }

    if (!theIsSelection) {
        updateSelectionPart(myDetectedEntity);
        thePrsMgr->AddToImmediateList(aPrs);
    } else {
        aPrs->Display();
    }
}

// =======================================================================
// function : fillSubEntityPrimitives
// =======================================================================
void InteractiveCaeMeshOwner::fillSubEntityPrimitives(const MeshSubEntity&         theSubEntity,
                                                      PrsSubEntityPrimitiveArrays& thePrimitives,
                                                      const bool                   theIsSelection)
{
    if (!theSubEntity.IsDefined()) {
        return;
    }

    const TCollection_AsciiString& aPrsId = theSubEntity.RefMeshPrs();

    SubEntityPrimitiveArrays* aPrims = thePrimitives.ChangeSeek(aPrsId);
    if (aPrims == nullptr) {
        aPrims = thePrimitives.Bound(aPrsId, SubEntityPrimitiveArrays());
    }

    aPrims->Nodes.Append(theSubEntity.Pos());

    if (theSubEntity.IsPresentation()) {
        aPrims->IsWholePrs = true;
    }

    if (!theIsSelection) {
        return;
    }

    const Handle(RWCae::MeshData)& aMeshData = theSubEntity.RefMeshData();

    if (theSubEntity.IsElement()) {
        auto compute = [this, &aPrims](const auto& theAdaptor) {
            RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();
            for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++) {
                fillElementPrimitives(theAdaptor, *aPrims, anElemIter);
            }
        };

        RWCae::MeshAdaptorsIteratorAllElems::For(compute, aMeshData);
    }
}

// =======================================================================
// function : FillElementPrimitives
// =======================================================================
void InteractiveCaeMeshOwner::FillElementPrimitives(const Handle(RWCae::MeshData)& theMeshData,
                                                    SubEntityPrimitiveArrays&        thePrimitives,
                                                    RWCae::MeshEntityId            theElemIndex)
{
    RWCae::MeshData::IndexedElement anIndElem = theMeshData->IndexedElements()[theElemIndex];

    auto compute = [this, &thePrimitives, &anIndElem](const auto& theAdaptor) {
        fillElementPrimitives(theAdaptor, thePrimitives, anIndElem.Index);
        return true;
    };

    RWCae::MeshAdaptorsIteratorAllElems::Switch(anIndElem.Type, compute, theMeshData, false);
}

// =======================================================================
// function : fillElementPrimitives
// =======================================================================
template<class Elem_T>
void InteractiveCaeMeshOwner::fillElementPrimitives(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                                    SubEntityPrimitiveArrays&           thePrimitives,
                                                    RWCae::MeshEntityId               theElemIndex)
{
    Graphic3d_Vec3                     aNode(0.0f);
    std::vector<RWCae::MeshEntityId> aNodes;

    const Elem_T& anElem = theAdaptor.GetElement(theElemIndex);

    int aStartIndex = thePrimitives.ElementNodes.Size() + 1;

    theAdaptor.ElementNodes(anElem, aNodes);
    for (RWCae::MeshEntityId aNodeIdx : aNodes) {
        theAdaptor.NodePosition(aNode, aNodeIdx);
        thePrimitives.ElementNodes.Append(aNode);
    }

    RWCae::MeshElemEntityId aNbFaces = theAdaptor.NbFaces(anElem);
    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
        for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(theAdaptor, anElem, aFaceIter);
             aTrgIter.More();
             aTrgIter.Next()) {
            const Graphic3d_Vec3ub& aTrg = aTrgIter.TriangleIndices();

            thePrimitives.ElementTrgs.Append(
                Graphic3d_Vec3i(aStartIndex + aTrg.x(), aStartIndex + aTrg.y(), aStartIndex + aTrg.z()));
        }

        RWCae::MeshElemEntityId aNbFaceNodes = theAdaptor.NbFaceNodes(anElem, aFaceIter);
        for (RWCae::MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < aNbFaceNodes && aNbFaceNodes > 1;
             aFaceNodeIter++) {
            RWCae::MeshElemEntityId aNodeIndex1 = theAdaptor.FaceNodeIndex(anElem, aFaceIter, aFaceNodeIter);
            RWCae::MeshElemEntityId aNodeIndex2 = theAdaptor.FaceNodeIndex(anElem,
                                                                             aFaceIter,
                                                                             (aFaceNodeIter + 1) % aNbFaceNodes);
            thePrimitives.ElementEdges.Append(Graphic3d_Vec2i(aStartIndex + aNodeIndex1, aStartIndex + aNodeIndex2));
        }
    }
}

// =======================================================================
// function : HighlightPrimitives
// =======================================================================
void InteractiveCaeMeshOwner::HighlightPrimitives(const TCollection_AsciiString&    thePrsId,
                                                  const SubEntityPrimitiveArrays&   thePrimitives,
                                                  const Handle(Prs3d_Presentation)& thePrs,
                                                  bool                              theIsSelection)
{
    Handle(MeshPresentation) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    MeshHighlightingType aHiType = theIsSelection ? MeshHighlightingType::Selection : MeshHighlightingType::Highlight;

    const Handle(InteractiveCaeMeshDrawer)& aDrawer = aPrs->HighlightDrawer(aHiType);

    if (thePrimitives.IsWholePrs) {
        if (aHiType != MeshHighlightingType::Highlight || aPrs->HighlightType() != MeshHighlightingType::Selection) {
            aPrs->SetHighlightType(aHiType);
        }
    }

    // Add elements faces
    if (!thePrimitives.ElementNodes.IsEmpty() && !thePrimitives.ElementTrgs.IsEmpty()) {
        int aNbVertices = thePrimitives.ElementNodes.Size();
        int aNbIndices  = thePrimitives.ElementTrgs.Size() * 3;

        Handle(Graphic3d_ArrayOfTriangles) aTriangles = new Graphic3d_ArrayOfTriangles(aNbVertices, aNbIndices);

        for (const Graphic3d_Vec3& aNode : thePrimitives.ElementNodes) {
            aTriangles->AddVertex(aNode);
        }

        for (const Graphic3d_Vec3i& aTrg : thePrimitives.ElementTrgs) {
            aTriangles->AddEdges(aTrg.x(), aTrg.y(), aTrg.z());
        }

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(aDrawer->ShadingAspect()->Aspect());
        aGroup->AddPrimitiveArray(aTriangles);
    }

    // Add elements edges
    if (!thePrimitives.ElementNodes.IsEmpty() && !thePrimitives.ElementEdges.IsEmpty()) {
        int aNbVertices = thePrimitives.ElementNodes.Size();
        int aNbIndices  = thePrimitives.ElementEdges.Size() * 2;

        Handle(Graphic3d_ArrayOfSegments) anEdges = new Graphic3d_ArrayOfSegments(aNbVertices, aNbIndices);

        for (const Graphic3d_Vec3& aNode : thePrimitives.ElementNodes) {
            anEdges->AddVertex(aNode);
        }

        for (const Graphic3d_Vec2i& anEdge : thePrimitives.ElementEdges) {
            anEdges->AddEdges(anEdge.x(), anEdge.y());
        }

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(aDrawer->LineAspect()->Aspect());
        aGroup->AddPrimitiveArray(anEdges);
    }

    // Add nodes
    if (!thePrimitives.Nodes.IsEmpty()) {
        Handle(Graphic3d_ArrayOfPoints) aPoints = new Graphic3d_ArrayOfPoints(thePrimitives.Nodes.Size());

        for (const Graphic3d_Vec3& aNode : thePrimitives.Nodes) {
            aPoints->AddVertex(aNode);
        }

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(aDrawer->PointAspect()->Aspect());
        aGroup->AddPrimitiveArray(aPoints);
    }
}

// =======================================================================
// function : updatePresentations
// =======================================================================
void InteractiveCaeMeshOwner::updatePresentations(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    for (const Handle(MeshPresentationBase)& aPrsBase : Presentations()) {
        Handle(MeshPresentation) aPrs = Handle(MeshPresentation)::DownCast(aPrsBase);
        if (aPrs->HasMeshGeometry(theMeshGeom)) {
            aPrs->SetToUpdate(true, true);
        }
    }
}

} // namespace