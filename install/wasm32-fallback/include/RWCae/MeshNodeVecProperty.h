
#ifndef _MESHNODEVECPROPERTY_HEADERFILE
#define _MESHNODEVECPROPERTY_HEADERFILE

#include <RWCae/MeshNodeVecPropertyData.h>
#include <RWCae/MeshScalarProperty.h>

#include <TColStd_PackedMapOfInteger.hxx>

#include <utility>
#include <vector>

namespace RWCae {
//! Property assigned to Mesh elements and holding individual quantity value for each element's node.
class MeshNodeVecProperty : public MeshScalarPropertyBase {
public:
    //! Create a new instance of this class.
    static Handle(MeshScalarPropertyBase) NewInstance(MeshPropertyBase::MappingType)
    {
        return new MeshNodeVecProperty("");
    }

public:
public:
    //! Default constructor.
    Standard_EXPORT explicit MeshNodeVecProperty(const TCollection_AsciiString& theName, const int theIncrement = 256);

public:
    //! Return number of property values.
    virtual int NbValues() const override { return myValues.NbValues(); }

    //! Return TRUE if value for specified index has been defined (e.g. not Nan).
    Standard_EXPORT virtual bool HasValue(const MeshPropertyEntityInfo& theInfo) const override;

    //! Return value for specified index.
    Standard_EXPORT virtual double Value(const MeshPropertyEntityInfo& theInfo) const override;

    //! Return pointer to value for specified index.
    Standard_EXPORT virtual const char* RawValue(const MeshPropertyEntityInfo& theInfo) const override;

    //! Set value for specified index.
    Standard_EXPORT virtual void SetValue(const MeshPropertyEntityInfo& theInfo,
                                          double                        theValue,
                                          bool                          toComputeRange = true) override;

    //! Set value for specified index by its pointer.
    Standard_EXPORT virtual void SetRawValue(const MeshPropertyEntityInfo& theInfo,
                                             const char*                   theValue,
                                             bool                          toComputeRange = true) override;

    //! Return size of elements in bytes.
    virtual size_t ElemSize() const override { return myValues.ElemSize(); }

    //! Return type of elements.
    virtual MeshPropertyElemType ElemType() const override { return myValues.ElemType(); }

    //! Return property base type.
    virtual MeshScalarPropertyBaseType BaseType() const override { return MeshScalarPropertyBaseType::NodeVec; }

    //! Clear values data and mark as not loaded.
    Standard_EXPORT virtual void Unload() override;

public:
    //! Initialize the array of required size with specified value.
    //! @param theElementalSize number of elements
    //! @param theNodalSize number of shared nodes
    //! @param theToReplaceNodal if true then try to replace this property
    //!                     with scalar nodal property with single value per entity.
    //! @param theToReplaceElemental if true then try to replace this property
    //!                     with scalar elemental property with single value per entity.
    //! @param theType type of elements
    Standard_EXPORT void Init(int                  theElementalSize,
                              int                  theNodalSize,
                              bool                 theToReplaceNodal     = true,
                              bool                 theToReplaceElemental = true,
                              MeshPropertyElemType theType               = MeshPropertyElemType::Double);

    //! Reserve vector for specified value map.
    void ReserveValueMap(int theIndex, int theSize) { myValues.ReserveValueMap(theIndex, theSize); }

    //! Returns an elemental property, holds elemental values if non-null.
    const Handle(MeshScalarProperty) ElementalProperty() const { return myElementalProperty; }

    //! Returns an elemental property, holds nodal values if non-null.
    const Handle(MeshScalarProperty) NodalProperty() const { return myNodalProperty; }

    //! Nullify temporary nodal/elemental scalar properties.
    void NullifyAuxProperties()
    {
        myElementalProperty.Nullify();
        myNodalProperty.Nullify();
    }

    //! Return indices of nodes for specified element.
    Standard_EXPORT void SubIndices(int theIndex, std::vector<int>& theIndices);

    //! Return number of sub indices for specified element.
    Standard_EXPORT int NbSubIndices(int theIndex) const;

protected:
    //! Check for identical node values of specified element and fill temporary properties (if needed).
    Standard_EXPORT void checkForIdentical(const MeshPropertyEntityInfo& theInfo);

protected:
    MeshNodeVecPropertyData    myValues;
    Handle(MeshScalarProperty) myElementalProperty; //!< temporary property holding single value per an element
    Handle(MeshScalarProperty) myNodalProperty;     //!< temporary property holding single value per a node
};

} // namespace

#endif // RWCae_MESHNODEVECPROPERTY_HEADERFILE