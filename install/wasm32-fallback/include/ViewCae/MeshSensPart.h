#ifndef MESHSENSITIVEPART_HEADERFILE
#define MESHSENSITIVEPART_HEADERFILE

#include <ViewCae/MeshAnnotations.h>

#include <ViewCaeGeom/MeshGeometry.h>

#include <Select3D_SensitiveSet.hxx>

#include <vector>

namespace ViewCae {
DEFINE_STANDARD_HANDLE(InteractiveCaeMeshOwner, Dummy); // forward declared class due to cyclic dependency
DEFINE_STANDARD_HANDLE(MeshPresentation, Dummy);        // forward declared class due to cyclic dependency

//! Type of mesh presentation sensitive part.
enum class MeshSensPartType { Entities3d, Entities2d, Entities0d, Nodes, Annotations };
constexpr int MeshNbSensPartTypes = (int)MeshSensPartType::Annotations + 1;

//! Base class for mesh presentation sensitive part.
class MeshSensPart {
public:
    //! Destructor.
    virtual ~MeshSensPart() {}

    //! Return selection flag.
    bool ToSelect() const { return myToSelect; }

    //! Set selection flag.
    void SetSelect(bool theToSelect) { myToSelect = theToSelect; }

    //! Clear sensitive part to recompute it.
    virtual void Clear()
    {
        myIsUpdated = false;
        clear();
    }

    //! Recompute sensitive part if needed.
    virtual void Update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                        MeshPresentation*                         thePrs,
                        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom) = 0;

    //! Add sensitive part to selection if needed.
    virtual void Compute(const Handle(SelectMgr_Selection)& theSel) const
    {
        if (myToSelect) {
            compute(theSel);
        }
    }

protected:
    //! Clear sensitive part.
    virtual void clear() = 0;

    //! Add sensitive part to selection.
    virtual void compute(const Handle(SelectMgr_Selection)& theSel) const = 0;

protected:
    bool myIsUpdated = false;
    bool myToSelect  = false;
};

//! Template class for mesh sensitive part of mesh entities.
template<MeshSensPartType T>
class MeshSensPartEntity : public MeshSensPart {
public:
    //! Destructor.
    virtual ~MeshSensPartEntity() {}

    //! Recompute sensitive part if needed.
    virtual void Update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                        MeshPresentation*                         thePrs,
                        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom) override;

    //! Return type of mesh sensitive part.
    static MeshSensPartType Type() { return T; }

protected:
    //! Clear sensitive part.
    virtual void clear() override { mySensitives.Clear(); };

    //! Recompute sensitive part.
    void update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                MeshPresentation*                         thePrs,
                const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Add sensitive part to selection.
    virtual void compute(const Handle(SelectMgr_Selection)& theSel) const override;

protected:
    typedef NCollection_DataMap<Handle(ViewCaeGeom::MeshGeometry), std::vector<Handle(Select3D_SensitiveEntity)>>
                           MapOfSensitiveEntities;
    MapOfSensitiveEntities mySensitives;
};

//! Mesh sensitive part of mesh annotations.
class MeshSensPartAnnotations : public MeshSensPart {
public:
    //! Destructor.
    virtual ~MeshSensPartAnnotations() {}

    //! Recompute sensitive part if needed.
    virtual void Update(const Handle(InteractiveCaeMeshOwner)&    theOwner,
                        MeshPresentation*                         thePrs,
                        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom) override;

    //! Return type of mesh sensitive part.
    static MeshSensPartType Type() { return MeshSensPartType::Annotations; }

protected:
    //! Clear sensitive part.
    virtual void clear() override { mySensitives.Clear(); };

    //! Add sensitive part to selection.
    virtual void compute(const Handle(SelectMgr_Selection)& theSel) const override;

protected:
    typedef NCollection_Map<Handle(Select3D_SensitiveSet)> MapOfSensitiveAnnotations;
    MapOfSensitiveAnnotations                              mySensitives;
};

//! Container for mesh sensitive parts.
class MeshSensParts {
public:
    //! Constructor.
    Standard_EXPORT MeshSensParts();

    //! Destructor.
    Standard_EXPORT ~MeshSensParts();

    //! Return mesh sensitive part by type.
    const MeshSensPart* operator()(MeshSensPartType theType) const { return myParts[(int)theType]; }

    //! Return casted mesh sensitive part by type.
    template<class T>
    T* GetPart()
    {
        return dynamic_cast<T*>(myParts[(int)T::Type()]);
    }

    //! Return mesh sensitive part by type.
    MeshSensPart* operator()(MeshSensPartType theType) { return myParts[(int)theType]; }

    //! Return casted mesh sensitivepart by type.
    template<class T>
    const T* GetPart() const
    {
        return dynamic_cast<const T*>(myParts[(int)T::Type()]);
    }

    //! Set selection flag of all defined parts to false.
    Standard_EXPORT void DeselectAll();

    //! Clear all defined parts to recompute it.
    Standard_EXPORT void ClearAll();

protected:
    std::vector<MeshSensPart*> myParts;
};

} // namespace

#endif // MESHSENSITIVEPART_HEADERFILE