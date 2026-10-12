#ifndef _MESHPART_HEADERFILE
#define _MESHPART_HEADERFILE

#include <ViewCae/CaeMeshArrowsTools.h>
#include <ViewCae/CaeMeshStreamlinesBuilder.h>
#include <ViewCae/MeshAnnotations.h>

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_ArrayOfPrimitives.hxx>

#include <vector>

namespace ViewCae {
class InteractiveCaeMeshDrawer; // forward declared class due to cyclic dependency
class MeshPresentation;         // forward declared class due to cyclic dependency
class MeshPrsPartParams;        // forward declared class due to cyclic dependency

//! Type of mesh presentation part.
enum class MeshPartType {
    Triangles3d,
    Triangles2d,
    Edges3d,
    Edges2d,
    SharpEdges3d,
    SharpEdges2d,
    FreeEdges3d,
    FreeEdges2d,
    Points,
    Elems0d,
    Centroids3d,
    Centroids2d,
    Vectors,
    MinMaxAnnotations,
    GroupsInfo,
    Streamlines,
    GeneralCells
};
constexpr int MeshNbPartTypes = (int)MeshPartType::GeneralCells + 1;

//! Base class for mesh presentation part.
class MeshPart : public Standard_Transient {
public:
    //! Destructor.
    virtual ~MeshPart() {}

    //! Check if part is needed to show and update.
    bool NeedToUpdate() const { return myToShow && !myIsUpdated; }

    //! Return visibility flag.
    bool ToShow() const { return myToShow; }

    //! Set visibility flag.
    void SetShow(bool theToShow) { myToShow = theToShow; }

    //! Clear part to recompute it.
    virtual void Clear(bool theWithAuxData = true)
    {
        myIsUpdated = false;
        clear(theWithAuxData);
    }

    //! Recompute part if needed.
    virtual void Update(const Handle(MeshPresentation)& thePrs)
    {
        if (myIsUpdated || !myToShow) {
            return;
        }

        update(thePrs);

        myIsUpdated = true;
    }

    //! Add part to presentation if needed.
    virtual void Compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const
    {
        if (myToShow && isDefined()) {
            compute(theCtx, theDrawer, thePrs, theTopPrs, thePixelRatio);
        }
    }

protected:
    //! Clear part.
    virtual void clear(bool theWithAuxData) = 0;

    //! Recompute part.
    virtual void update(const Handle(MeshPresentation)& thePrs) = 0;

    //! Add part to presentation.
    virtual void compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const = 0;

    //! Check if part is defined.
    virtual bool isDefined() const = 0;

protected:
    bool myIsUpdated = false;
    bool myToShow    = false;
};

//! Template class for mesh presentation part that stores array of primitives.
template<MeshPartType T>
class MeshPartPrimArray : public MeshPart {
public:
    //! Destructor.
    virtual ~MeshPartPrimArray() {}

    //! Return type of mesh part.
    static MeshPartType Type() { return T; }

    //! Extract part as a separate mesh data.
    static bool ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                const Handle(MeshPrsPartParams)&          theParams,
                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

protected:
    //! Clear part.
    virtual void clear(bool) override { myPrimArray.Nullify(); }

    //! Recompute part.
    virtual void update(const Handle(MeshPresentation)& thePrs) override;

    //! Add part to presentation.
    virtual void compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const override;

    //! Check if part is defined.
    virtual bool isDefined() const override { return !myPrimArray.IsNull(); }

protected:
    Handle(Graphic3d_ArrayOfPrimitives) myPrimArray;
};

//! Mesh presentation part that stores data for vector presentation.
class MeshPartVectors : public MeshPart {
public:
    //! Destructor.
    virtual ~MeshPartVectors() {}

    //! Return type of mesh part.
    static MeshPartType Type() { return MeshPartType::Vectors; }

    //! Extract part as a separate mesh data.
    static bool ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                const Handle(MeshPrsPartParams)&          theParams,
                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

protected:
    //! Clear part.
    virtual void clear(bool) override { myVectors.Nullify(); }

    //! Recompute part.
    virtual void update(const Handle(MeshPresentation)& thePrs) override;

    //! Add part to presentation.
    virtual void compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const override;

    //! Check if part is defined.
    virtual bool isDefined() const override { return !myVectors.IsNull(); }

protected:
    Handle(MeshVectorsPrimitiveArray) myVectors;
};

//! Template class for mesh presentation part that stores array of annotations.
template<MeshPartType T>
class MeshPartAnnotations : public MeshPart {
public:
    //! Destructor.
    virtual ~MeshPartAnnotations() {}

    //! Return type of mesh part.
    static MeshPartType Type() { return T; }

    //! Extract part as a separate mesh data.
    static bool ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                const Handle(MeshPrsPartParams)&          theParams,
                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

    //! Return annotations container.
    MeshAnnotations& Annotations() { return myAnnotations; }

    //! Return annotations container.
    const MeshAnnotations& Annotations() const { return myAnnotations; }

protected:
    //! Clear part.
    virtual void clear(bool) override { myAnnotations.Clear(); }

    //! Recompute part.
    virtual void update(const Handle(MeshPresentation)& thePrs) override;

    //! Add part to presentation.
    virtual void compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const override;

    //! Check if part is defined.
    virtual bool isDefined() const override { return !myAnnotations.Annotations().empty(); }

protected:
    MeshAnnotations myAnnotations;
};

typedef NCollection_DataMap<int, NCollection_List<TColStd_PackedMapOfInteger>> MapOfGroupAreas;

//! Mesh presentation part that stores data for streamlines presentation.
class MeshPartStreamlines : public MeshPart {
public:
    //! Destructor.
    virtual ~MeshPartStreamlines() {}

    //! Return type of mesh part.
    static MeshPartType Type() { return MeshPartType::Streamlines; }

    //! Extract part as a separate mesh data.
    static bool ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                const Handle(MeshPrsPartParams)&          theParams,
                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

    //! Return streamlines animation data.
    const MeshStreamlinesAnimationData* AnimationData() const
    {
        return dynamic_cast<const MeshStreamlinesAnimationData*>(myStreamlines.get());
    }

    //! Return streamlines animation data.
    MeshStreamlinesAnimationData* ChangeAnimationData()
    {
        return dynamic_cast<MeshStreamlinesAnimationData*>(myStreamlines.get());
    }

protected:
    //! Clear part.
    virtual void clear(bool) override { myStreamlines.Nullify(); }

    //! Recompute part.
    virtual void update(const Handle(MeshPresentation)& thePrs) override;

    //! Add part to presentation.
    virtual void compute(const Handle(AIS_InteractiveContext)&   theCtx,
                         const Handle(InteractiveCaeMeshDrawer)& theDrawer,
                         const Handle(Prs3d_Presentation)&       thePrs,
                         const Handle(Prs3d_Presentation)&       theTopPrs,
                         float                                   thePixelRatio) const override;

    //! Check if part is defined.
    virtual bool isDefined() const override { return !myStreamlines.IsNull(); }

protected:
    Handle(Graphic3d_ArrayOfPrimitives) myStreamlines;
};

//! Container for mesh presentation parts.
class MeshParts {
public:
    //! Constructor.
    Standard_EXPORT MeshParts();

    //! Destructor.
    Standard_EXPORT ~MeshParts();

    //! Return mesh part by type.
    const Handle(MeshPart)& operator()(MeshPartType theType) const { return myParts[(int)theType]; }

    //! Set visibility flag of all defined parts to false.
    Standard_EXPORT void HideAll();

    //! Clear all defined parts to recompute it.
    Standard_EXPORT void ClearAll();

protected:
    std::vector<Handle(MeshPart)> myParts;
};

#ifdef Standard_UNUSED
    // suppress CodeCheck warnings considering template exports as 'forward declarations'
    #define ViewCae_ExportTemplate
#endif

typedef MeshPartAnnotations<MeshPartType::GroupsInfo> MeshPartGroupsInfoAnnotations;

} // namespace

#endif // _MESHPART_HEADERFILE
