#ifndef MESHPRESENTATIONS_HEADERFILE
#define MESHPRESENTATIONS_HEADERFILE

#include <ViewCae/MeshPresentationBase.h>

#include <ViewCaeGeom/MeshBvhData.h>

#include <RWCae/MeshData.h>

namespace ViewCae {
//! Storage class for cae mesh presentations.
class MeshPresentations : public Standard_Transient {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentations(const Handle(RWCae::MeshData)& theMeshData);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentations();

    //! Returns assigned mesh data.
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMeshData; }

    //! Returns auxiliary BVH data.
    const Handle(ViewCaeGeom::MeshBvhData)& GetMeshBvhData() const { return myBvhData; }

    //! Returns list of used mesh geometry chains.
    ViewCaeGeom::MapOfMeshGeometries& Geometries() { return myGeometries; }

    //! Returns list of used mesh geometry chains.
    const ViewCaeGeom::MapOfMeshGeometries& Geometries() const { return myGeometries; }

    //! Return list of mesh presentations.
    ListOfMeshPresentations& Presentations() { return myPresentations; }

    //! Return list of mesh presentations.
    const ListOfMeshPresentations& Presentations() const { return myPresentations; }

    //! Return mesh presentation by its id.
    Standard_EXPORT Handle(MeshPresentationBase) Presentation(const TCollection_AsciiString& thePrsId) const;

    //! Remove mesh presentation by its id.
    Standard_EXPORT void RemovePresentation(const TCollection_AsciiString& thePrsId);

    //! Return mesh data for presentation, defined by Json parameters.
    Standard_EXPORT Handle(RWCae::MeshData) GetPartGenMeshData(const std::string& theData);

    //! Set custom mesh data for presentation.
    Standard_EXPORT void SetPartGenMeshData(const TCollection_AsciiString&   thePrsId,
                                            int                              thePartId,
                                            const Handle(RWCae::MeshData)& theMeshData);

    //! Gather information about original mesh entity and return as separate mesh data.
    Standard_EXPORT Handle(RWCae::MeshData) GetMeshEntityInfo(const std::string&    thePrsInfo,
                                                                const Graphic3d_Vec3& thePos,
                                                                int                   theEntityId,
                                                                int                   theEntityType,
                                                                bool                  theIsSurf);

    //! Return true if properties data is stored in MDB file and can be loaded/unloaded on demand.
    bool ToCacheProperties() const { return myToCacheProperties; }

    //! Define if properties data is stored in MDB file and can be loaded/unloaded on demand.
    void SetCacheProperties(bool theToCache) { myToCacheProperties = theToCache; }

    //! Called when any mesh geometry was removed.
    Standard_EXPORT void OnRemoveMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

protected:
    //! Release resources.
    Standard_EXPORT void release() noexcept;

protected:
    Handle(RWCae::MeshData)         myMeshData;
    Handle(ViewCaeGeom::MeshBvhData) myBvhData;
    ViewCaeGeom::MapOfMeshGeometries myGeometries;
    ListOfMeshPresentations           myPresentations;
    bool                              myToCacheProperties = false;
};

} // namespace

#endif // MESHPRESENTATIONS_HEADERFILE