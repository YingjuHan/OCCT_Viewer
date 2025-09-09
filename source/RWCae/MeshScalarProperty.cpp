#include <RWCae/MeshScalarProperty.h>

namespace RWCae {

// ================================================================
// Function : Constructor
// ================================================================
MeshScalarProperty::MeshScalarProperty(const TCollection_AsciiString& theName,
                                       const MappingType              theMapType,
                                       const int                      theIncrement)
    : MeshScalarPropertyBase(theName, theMapType),
      myValues(theIncrement)
{
}

// ================================================================
// Function : Init
// ================================================================
void MeshScalarProperty::Init(int theSize, MeshPropertyElemType theType)
{
    myValues.Init(theSize, theType);
    myIsLoaded = theSize > 0;
}

// ================================================================
// Function : HasValue
// ================================================================
bool MeshScalarProperty::HasValue(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return false;
    }

    return myValues.HasValue(theInfo);
}

// ================================================================
// Function : SetValue
// ================================================================
void MeshScalarProperty::SetValue(const MeshPropertyEntityInfo& theInfo, double theValue, bool toComputeRange)
{
    if (!myIsLoaded) {
        return;
    }

    myValues.SetValue(theInfo, theValue);
    if (toComputeRange) {
        SetExternalValue(theValue);
    }
}

// ================================================================
// Function : SetRawValue
// ================================================================
void MeshScalarProperty::SetRawValue(const MeshPropertyEntityInfo& theInfo, const char* theValue, bool toComputeRange)
{
    if (!myIsLoaded) {
        return;
    }

    myValues.SetRawValue(theInfo, theValue);
    if (toComputeRange) {
        SetExternalValue(myValues.Value(theInfo));
    }
}

// ================================================================
// Function : Value
// ================================================================
double MeshScalarProperty::Value(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return myValues.NaNValue();
    }

    return myValues.Value(theInfo);
}

// ================================================================
// Function : RawValue
// ================================================================
const char* MeshScalarProperty::RawValue(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return myValues.RawNaNValue();
    }

    return myValues.RawValue(theInfo);
}

// ================================================================
// Function : AppendValue
// ================================================================
void MeshScalarProperty::AppendValue(double theValue, bool toComputeRange)
{
    myValues.AppendValue(theValue);
    if (toComputeRange) {
        SetExternalValue(theValue);
    }
    myIsLoaded = true;
}

// ================================================================
// Function : Unload
// ================================================================
void MeshScalarProperty::Unload()
{
    myValues.Clear();
    myIsLoaded = false;
}

} // namespace