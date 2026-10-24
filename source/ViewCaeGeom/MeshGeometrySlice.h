#ifndef _MESHGEOMETRYSLICE_HEADERFILE
#define _MESHGEOMETRYSLICE_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_SequenceOfHClipPlane.hxx>
#include <array>

namespace ViewCaeGeom {
struct MeshCutCancellation;
//! Wall times partition a rebuilt native single-plane section (not cache hits).
struct MeshSliceWork {
    double BvhMs=0, PrepareMs=0, CutMs=0, MergeMs=0;
    // MergeMs includes these disjoint subphases plus loop/timer overhead.
    double SharedPointMs=0, AppendMs=0, FallbackMs=0;
    size_t Candidates=0, Fallbacks=0;
    size_t Unsupported=0, NearPlane=0, Warped=0, Degenerate=0, NearEndpoint=0;
};
//! Endpoints index the slice mesh nodes; source element indexes the original mesh.
struct MeshSliceLine {
    std::array<RWCae::MeshEntityId, 2> Nodes;
    RWCae::MeshEntityId SourceElement;
};

//! Mesh geometry for mesh slice.
//! It generates section faces for volume cells and section lines for surface cells.
class MeshGeometrySlice : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometrySlice(const Handle(MeshBvhData)& theBvhData);

    Standard_EXPORT ~MeshGeometrySlice() override;
    void SetCancellation(const std::shared_ptr<MeshCutCancellation>& token) { myCancellation=token; }
    const MeshSliceWork& LastWork() const { return myLastWork; }
    size_t WorkGeneration() const { return myWorkGeneration; }
    //! Same conservative BVH plane test as cutting, without creating output primitives.
    Standard_EXPORT static size_t CountPlaneCandidates(const Handle(MeshBvhData)& bvh,
        const Handle(MeshGeometry)& source, const Graphic3d_Vec4d& plane);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Slice; }

    //! Return active slice planes.
    const Graphic3d_SequenceOfHClipPlane& SlicePlanes() const { return mySlicePlanes; }

    //! Generate the slice if necessary and return surface-mesh intersections.
    const std::vector<MeshSliceLine>& Lines()
    {
        GetMeshData();
        return myLines;
    }

    //! Nodal values index slice nodes; elemental values index Lines(), with generated-node subindices for NodeVec.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateLineProperty(
        const Handle(RWCae::MeshNVecProperty)& theProp) const;

    //! Set active slice planes.
    void SetSlicePlanes(const Graphic3d_SequenceOfHClipPlane& theSlicePlanes)
    {
        mySlicePlanes = theSlicePlanes;
        SetToUpdate();
    }

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override;

protected:
    Graphic3d_SequenceOfHClipPlane mySlicePlanes;
    std::vector<MeshSliceLine> myLines;
    std::shared_ptr<MeshCutCancellation> myCancellation;
    MeshSliceWork myLastWork;
    size_t myWorkGeneration=0;
};

} // namespace

#endif // _MESHGEOMETRYSLICE_HEADERFILE
