#ifndef _MESHSCALARPROPERTYBASE_HEADERFILE
#define _MESHSCALARPROPERTYBASE_HEADERFILE

#include <RWCae/MeshPropertyBase.h>
#include <RWCae/MeshPropertyData.h>

#include <Bnd_Range.hxx>

namespace RWCae {
//! Type of scalar property.
enum class MeshScalarPropertyBaseType { Scalar, NodeVec, Group };

//! Base class for mesh property containing single value per entity.
class MeshScalarPropertyBase : public MeshPropertyBase {
public:
    //! Default constructor.
    Standard_EXPORT MeshScalarPropertyBase(const TCollection_AsciiString& theName, const MappingType theMapType);

public:
    //! Return number of property values.
    virtual int NbValues() const = 0;

    //! Return TRUE if value for specified index has been defined (e.g. not Nan).
    virtual bool HasValue(const MeshPropertyEntityInfo& theInfo) const = 0;

    //! Return value for specified index.
    virtual double Value(const MeshPropertyEntityInfo& theInfo) const = 0;

    //! Return pointer to value for specified index.
    virtual const char* RawValue(const MeshPropertyEntityInfo& theInfo) const = 0;

    //! Set value for specified index.
    virtual void SetValue(const MeshPropertyEntityInfo& theInfo, double theValue, bool toComputeRange = true) = 0;

    //! Set value for specified index by its pointer.
    virtual void SetRawValue(const MeshPropertyEntityInfo& theInfo,
                             const char*                   theValue,
                             bool                          toComputeRange = true) = 0;

    //! Return size of elements in bytes.
    virtual size_t ElemSize() const = 0;

    //! Return type of elements.
    virtual MeshPropertyElemType ElemType() const = 0;

    //! Return property base type.
    virtual MeshScalarPropertyBaseType BaseType() const = 0;

public:
    //! Update value range with a value without storing it.
    void SetExternalValue(double theValue)
    {
        if (!IsNaN(theValue)) {
            myValueRange.Add(theValue);
        }
    }

    //! Return value range.
    const Bnd_Range& ValueRange() const { return myValueRange; }

    //! Set (override) lower and upper values of the property.
    void SetValueRange(const Bnd_Range& theRange) { myValueRange = theRange; }

    //! Check if this property should be processed.
    bool IsEnabled() const { return myIsEnabled; }

    //! Enable/disable this property for processing.
    void SetEnabled(bool theIsEnabled) { myIsEnabled = theIsEnabled; }

    //! Return the measurement units used to display the property values.
    const TCollection_AsciiString& DisplayUnits() const { return myDisplayUnits; }

    //! Set the measurement units used to display the property values.
    void SetDisplayUnits(const TCollection_AsciiString& theUnits) { myDisplayUnits = theUnits; }

protected:
    TCollection_AsciiString myDisplayUnits; //!< the measurement units for displaying the property's values
    Bnd_Range               myValueRange;   //!< values range
    bool myIsEnabled; //!< if true, indicates this property should be available to the end users, else it's hidden
};

} // namespace

#endif // _MESHSCALARPROPERTYBASE_HEADERFILE  