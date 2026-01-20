
#ifndef RWCae_MESHSCALARPROPERTY_HEADERFILE
#define RWCae_MESHSCALARPROPERTY_HEADERFILE

#include <RWCae/MeshScalarPropertyBase.h>
#include <RWCae/MeshScalarPropertyData.h>

#include <NCollection_Vector.hxx>

namespace RWCae {
//! Mesh property with single double value per entity.
class MeshScalarProperty : public MeshScalarPropertyBase {
public:
    //! Default constructor.
    Standard_EXPORT MeshScalarProperty(const TCollection_AsciiString& theName,
                                       const MappingType              theMapType,
                                       const int                      theIncrement = 256);

public:
    //! Return number of property values.
    virtual int NbValues() const override { return myValues.NbValues(); }

    //! Return TRUE if value for specified index has been defined (e.g. not Nan).
    Standard_EXPORT virtual bool HasValue(const MeshPropertyEntityInfo& theInfo) const override;

    //! Set value for specified index.
    Standard_EXPORT virtual void SetValue(const MeshPropertyEntityInfo& theInfo,
                                          double                        theValue,
                                          bool                          toComputeRange = true) override;

    //! Set value for specified index by its pointer.
    Standard_EXPORT virtual void SetRawValue(const MeshPropertyEntityInfo& theInfo,
                                             const char*                   theValue,
                                             bool                          toComputeRange = true) override;

    //! Return value for specified index.
    Standard_EXPORT virtual double Value(const MeshPropertyEntityInfo& theInfo) const override;

    //! Return pointer to value for specified index.
    Standard_EXPORT virtual const char* RawValue(const MeshPropertyEntityInfo& theInfo) const override;

    //! Return size of elements in bytes.
    virtual size_t ElemSize() const override { return myValues.ElemSize(); }

    //! Return type of elements.
    virtual MeshPropertyElemType ElemType() const override { return myValues.ElemType(); }

    //! Return property base type.
    virtual MeshScalarPropertyBaseType BaseType() const override { return MeshScalarPropertyBaseType::Scalar; }

    //! Clear values data and mark as not loaded.
    Standard_EXPORT virtual void Unload() override;

public:
    //! Initialize the array of required size with specified value.
    Standard_EXPORT void Init(int theSize, MeshPropertyElemType theType = MeshPropertyElemType::Double);

    //! Append new value.
    Standard_EXPORT void AppendValue(double theValue, bool toComputeRange = true);

    //! Create a new instance of this class.
    static Handle(MeshScalarPropertyBase) NewInstance(MeshPropertyBase::MappingType theType)
    {
        return new MeshScalarProperty("", theType);
    }

protected:
    MeshScalarPropertyData myValues;
};

} // namespace

#endif // RWCae_MESHSCALARPROPERTY_HEADERFILE