#include <ViewCae/MeshPresentations.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/MeshPresentationTools.h>

namespace ViewCae {

// =======================================================================
// function : MeshPresentations
// =======================================================================
MeshPresentations::MeshPresentations(const Handle(RWCae::MeshData)& theMeshData) : myMeshData(theMeshData)
{
    myBvhData = new ViewCaeGeom::MeshBvhData();
}

// =======================================================================
// function : ~MeshPresentations
// =======================================================================
MeshPresentations::~MeshPresentations()
{
    release();
}

// =======================================================================
// function : release
// =======================================================================
void MeshPresentations::release() noexcept
{
    if (!myBvhData.IsNull()) {
        myBvhData->Clear();
    }
    myPresentations.Clear();
    for (ViewCaeGeom::MapOfMeshGeometries::Iterator anIter(myGeometries); anIter.More(); anIter.Next()) {
        anIter.Value()->Clear();
    }
    myGeometries.Clear();
}

// =======================================================================
// function : RemovePresentation
// =======================================================================
void MeshPresentations::RemovePresentation(const TCollection_AsciiString& thePrsId)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom;

    for (ListOfMeshPresentations::Iterator anIter(myPresentations); anIter.More(); anIter.Next()) {
        if (anIter.Value()->Id() == thePrsId) {
            aMeshGeom = anIter.Value()->GetMeshGeometry();
            myPresentations.Remove(anIter);
            break;
        }
    }

    bool hasGeom = false;
    for (const Handle(MeshPresentationBase)& aPrs : myPresentations) {
        if (aPrs->GetMeshGeometry() == aMeshGeom) {
            hasGeom = true;
            break;
        }
    }

    if (!hasGeom) {
        OnRemoveMeshGeometry(aMeshGeom);
        aMeshGeom->Clear();
        myGeometries.UnBind(aMeshGeom->Id());
    }
}

// ================================================================
// Function : onRemoveMeshGeometry
// ================================================================
void MeshPresentations::OnRemoveMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theMeshGeom.get())) {
        myBvhData->RemoveBvhBoxSet(aGeom);
    }
}

// =======================================================================
// function : Presentation
// =======================================================================
Handle(MeshPresentationBase) MeshPresentations::Presentation(const TCollection_AsciiString& thePrsId) const
{
    for (const Handle(MeshPresentationBase)& aPrs : myPresentations) {
        if (aPrs->Id() == thePrsId) {
            return aPrs;
        }
    }
    return nullptr;
}

// ================================================================
// Function : GetPartMeshData
// ================================================================
Handle(RWCae::MeshData) MeshPresentations::GetPartGenMeshData(const std::string& theData)
{
    Handle(RWCae::MeshData) aRes;

    Handle(MeshPresentationBase) aPrs;
    int                          aPart     = 0;
    bool                         isChanged = false;
    if (!MeshPresentationTools::RestoreFromJson(theData,
                                                myMeshData,
                                                myBvhData,
                                                myPresentations,
                                                myGeometries,
                                                aPrs,
                                                aPart,
                                                isChanged)) {
        return aRes;
    }

    // Unload not used props
    if (isChanged && myToCacheProperties) {
        NCollection_Map<Handle(RWCae::MeshNVecProperty)> aUsedProps;
        MeshPresentationTools::GatherUsedProps(myPresentations, myGeometries, aUsedProps);
        for (const Handle(RWCae::MeshNVecProperty)& aProp : myMeshData->Properties()) {
            if (aUsedProps.Contains(aProp)) {
                continue;
            }

            if (aProp->IsLoaded() && aProp->CanBeLoaded()) {
                aProp->Unload();
            }
        }
    }

    Handle(ViewCaeGeom::MeshGeometryGen) aGenGeom = aPrs->GetGenGeometry(aPart);
    if (aGenGeom.IsNull()) {
        return aRes;
    }

    aRes = aGenGeom->GetMeshData();
    return aRes;
}

// ================================================================
// Function : SetPartGenMeshData
// ================================================================
void MeshPresentations::SetPartGenMeshData(const TCollection_AsciiString&   thePrsId,
                                           int                              thePartId,
                                           const Handle(RWCae::MeshData)& theMeshData)
{
    Handle(MeshPresentationBase) aPrs = Presentation(thePrsId);
    if (aPrs.IsNull()) {
        return;
    }

    Handle(ViewCaeGeom::MeshGeometryGen) aGenGeom = aPrs->GetGenGeometry(thePartId);
    if (aGenGeom.IsNull()) {
        return;
    }

    aGenGeom->SetMeshData(theMeshData);
}

// ================================================================
// Function : GetMeshEntityInfo
// ================================================================
Handle(RWCae::MeshData) MeshPresentations::GetMeshEntityInfo(const std::string&    thePrsInfo,
                                                               const Graphic3d_Vec3& thePos,
                                                               int                   theEntityId,
                                                               int                   theEntityType,
                                                               bool                  theIsSurf)
{
    Handle(RWCae::MeshData) aRes;
    MeshEntityType aType = static_cast<MeshEntityType>(theEntityType);
    if (aType == MeshEntityType::UNKNOWN) {
        return aRes;
    }

    Handle(MeshPresentationBase) aPrs;
    int                          aPart     = 0;
    bool                         isChanged = false;
    if (!MeshPresentationTools::RestoreFromJson(thePrsInfo,
                                                myMeshData,
                                                myBvhData,
                                                myPresentations,
                                                myGeometries,
                                                aPrs,
                                                aPart,
                                                isChanged)) {
        return aRes;
    }

    Handle(ViewCaeGeom::MeshGeometryGen) aGenGeom = aPrs->GetGenGeometry(aPart);
    if (aGenGeom.IsNull()) {
        return aRes;
    }

    MeshSubEntity anEntity;

    if (aType == MeshEntityType::NODE) {
        anEntity.SetNode(aPrs, aGenGeom, thePos, theEntityId);
    } else {
        RWCae::MeshElemType anElemType = (RWCae::MeshElemType)(theEntityType - 1);
        anEntity.SetElem(aPrs, aGenGeom, thePos, ViewCaeGeom::MeshGeomElem(theEntityId, anElemType, theIsSurf));
    }

    aRes = anEntity.RefMeshData();

    return aRes;
}

} // namespace

