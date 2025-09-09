#include <ViewCae/MeshSensPart.h>

#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/MeshAdaptorSensitive.h>
#include <ViewCae/MeshPresentationScalar.h>

#include <RWCae/MeshAdaptorsIterator.h>

namespace ViewCae {
// ================================================================
// Function : MeshSensParts
// ================================================================
MeshSensParts::MeshSensParts()
{
    myParts.resize(MeshNbSensPartTypes);

    myParts[(int)MeshSensPartType::Entities3d]  = new MeshSensPartEntity<MeshSensPartType::Entities3d>();
    myParts[(int)MeshSensPartType::Entities2d]  = new MeshSensPartEntity<MeshSensPartType::Entities2d>();
    myParts[(int)MeshSensPartType::Entities0d]  = new MeshSensPartEntity<MeshSensPartType::Entities0d>();
    myParts[(int)MeshSensPartType::Nodes]       = new MeshSensPartEntity<MeshSensPartType::Nodes>();
    myParts[(int)MeshSensPartType::Annotations] = new MeshSensPartAnnotations();
}

// ================================================================
// Function : ~MeshSensParts
// ================================================================
MeshSensParts::~MeshSensParts()
{
    for (MeshSensPart*& aPart : myParts) {
        delete aPart;
    }
}

// ================================================================
// Function : HideAll
// ================================================================
void MeshSensParts::DeselectAll()
{
    for (MeshSensPart*& aPart : myParts) {
        aPart->SetSelect(false);
    }
}

// ================================================================
// Function : ClearAll
// ================================================================
void MeshSensParts::ClearAll()
{
    for (int i = 0; i < MeshNbSensPartTypes; i++) {
        if (i == (int)MeshSensPartType::Annotations) {
            continue;
        }

        myParts[i]->Clear();
    }
}

//! @name MeshSensPartEntity.
//! @{

// ================================================================
// Function : Update
// ================================================================
template<MeshSensPartType T>
void MeshSensPartEntity<T>::Update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                                   MeshPresentation*                         thePrs,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    if (!myIsUpdated && myToSelect) {
        mySensitives.Clear();

        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theMeshGeom.get())) {
            const Handle(RWCae::MeshData)& aMeshData = aGeom->GetMeshData();
            if (aMeshData.IsNull()) {
                continue;
            }

            update(theOwner, thePrs, aGeom);
        }

        myIsUpdated = true;
    }

    for (MapOfSensitiveEntities::Iterator anIter(mySensitives); anIter.More(); anIter.Next()) {
        for (const Handle(Select3D_SensitiveEntity)& aSensIter : anIter.Value()) {
            aSensIter->Set(theOwner);
        }
    }
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshSensPartEntity<MeshSensPartType::Entities3d>::update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                                                              MeshPresentation*                         thePrs,
                                                              const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    MeshPresentationScalar* aPrs = dynamic_cast<MeshPresentationScalar*>(thePrs);

    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();

    std::vector<Handle(Select3D_SensitiveEntity)> aSensitives;

    auto compute = [&aSensitives, &theOwner, &theMeshGeom, &thePrs](const auto& theAdaptor) {
        if (theAdaptor.NbElements() > 0) {
            aSensitives.push_back(new MeshAdaptorSensitiveElement(theOwner, theAdaptor, theMeshGeom.get(), thePrs));
        }
    };

    if (aPrs->ScalarVisProps()->IsVolumeAsSurface()) {
        RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData, true);
    } else {
        RWCae::MeshAdaptorsIterator3dElems::For(compute, aMeshData);
    }

    if (!aSensitives.empty()) {
        mySensitives.Bind(theMeshGeom, aSensitives);
    }
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshSensPartEntity<MeshSensPartType::Entities2d>::update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                                                              MeshPresentation*                         thePrs,
                                                              const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();

    std::vector<Handle(Select3D_SensitiveEntity)> aSensitives;

    auto compute = [&aSensitives, &theOwner, &theMeshGeom, &thePrs](const auto& theAdaptor) {
        if (theAdaptor.NbElements() > 0) {
            aSensitives.push_back(new MeshAdaptorSensitiveElement(theOwner, theAdaptor, theMeshGeom.get(), thePrs));
        }
    };

    RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData);

    if (!aSensitives.empty()) {
        mySensitives.Bind(theMeshGeom, aSensitives);
    }
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshSensPartEntity<MeshSensPartType::Entities0d>::update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                                                              MeshPresentation*                         thePrs,
                                                              const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();

    std::vector<Handle(Select3D_SensitiveEntity)> aSensitives;

    if (aMeshData->NbElems0d() > 0) {
        RWCae::MeshAdaptor0d           anAdaptor(*aMeshData);
        Handle(Select3D_SensitiveEntity) aSens = new MeshAdaptorSens0d(theOwner, anAdaptor, theMeshGeom.get(), thePrs);
        aSensitives.push_back(aSens);
    }

    if (!aSensitives.empty()) {
        mySensitives.Bind(theMeshGeom, aSensitives);
    }
}

// ================================================================
// Function : update
// ================================================================
template<>
void MeshSensPartEntity<MeshSensPartType::Nodes>::update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                                                         MeshPresentation*                         thePrs,
                                                         const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    std::vector<Handle(Select3D_SensitiveEntity)> aSensitives;

    Handle(Select3D_SensitiveEntity) aSens = new MeshAdaptorSensitiveNode(theOwner, theMeshGeom.get(), thePrs);
    aSensitives.push_back(aSens);
    mySensitives.Bind(theMeshGeom, aSensitives);
}

// ================================================================
// Function : compute
// ================================================================
template<MeshSensPartType T>
void MeshSensPartEntity<T>::compute(const Handle(SelectMgr_Selection)& theSel) const
{
    for (MapOfSensitiveEntities::Iterator anIter(mySensitives); anIter.More(); anIter.Next()) {
        for (const Handle(Select3D_SensitiveEntity)& aSensIter : anIter.Value()) {
            theSel->Add(aSensIter);
        }
    }
}

//! @}

//! @name MeshSensPartAnnotations.
//! @{

// ================================================================
// Function : Update
// ================================================================
void MeshSensPartAnnotations::Update(const Handle(InteractiveCaeMeshOwner)& theOwner,
                                     MeshPresentation*                      thePrs,
                                     const Handle(ViewCaeGeom::MeshGeometry)&)
{
    MeshPresentationScalar* aPrs = dynamic_cast<MeshPresentationScalar*>(thePrs);

    if (myToSelect) {
        mySensitives.Clear();

        aPrs->ChangeParts()(MeshPartType::GroupsInfo)->Update(thePrs);

        const Handle(MeshPartGroupsInfoAnnotations)& aGroupsInfo = Handle(MeshPartGroupsInfoAnnotations)::DownCast(
            aPrs->GetParts()(MeshPartType::GroupsInfo));

        aGroupsInfo->Annotations().ComputeSelection(theOwner, thePrs->Drawer()->TextAspect(), mySensitives);

        for (MeshAnnotation& anAnnotation : aGroupsInfo->Annotations().Annotations()) {
            for (NCollection_DataMap<int, Handle(MeshAnnotationLineOwner)>::Iterator anOwnersIter(
                     anAnnotation.LineOwners());
                 anOwnersIter.More();
                 anOwnersIter.Next()) {
                anOwnersIter.ChangeValue()->SetSelectionPart(
                    ViewBase::InteractiveOwner::SelectionPart((int)MeshEntityType::LABEL_GROUP, anAnnotation.Index()));
            }
        }
    }
}

// ================================================================
// Function : compute
// ================================================================
void MeshSensPartAnnotations::compute(const Handle(SelectMgr_Selection)& theSel) const
{
    for (MapOfSensitiveAnnotations::Iterator anIter(mySensitives); anIter.More(); anIter.Next()) {
        theSel->Add(anIter.Value());
    }
}

//! @}

} // namespace