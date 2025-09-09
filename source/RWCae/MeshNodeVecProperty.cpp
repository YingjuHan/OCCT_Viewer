#include <RWCae/MeshNodeVecProperty.h>

namespace RWCae {

// ================================================================
// Function : Constructor
// ================================================================
MeshNodeVecProperty::MeshNodeVecProperty(const TCollection_AsciiString& theName, const int theIncrement)
    : MeshScalarPropertyBase(theName, MeshPropertyBase::MappingType::Elemental),
      myValues(theIncrement),
      myElementalProperty(new MeshScalarProperty(theName, MeshPropertyBase::MappingType::Elemental, theIncrement)),
      myNodalProperty(new MeshScalarProperty(theName, MeshScalarProperty::MappingType::Nodal, theIncrement))
{
}

// ================================================================
// Function : Init
// ================================================================
void MeshNodeVecProperty::Init(int                  theElementalSize,
                               int                  theNodalSize,
                               bool                 theToReplaceNodal,
                               bool                 theToReplaceElemental,
                               MeshPropertyElemType theType)
{
    myValues.Init(theElementalSize, theType);
    myIsLoaded = theElementalSize > 0;

    if (theToReplaceNodal) {
        myNodalProperty->Init(theNodalSize, theType);
    } else {
        myNodalProperty.Nullify();
    }

    if (theToReplaceElemental) {
        myElementalProperty->Init(theElementalSize, theType);
    } else {
        myElementalProperty.Nullify();
    }
}

// ================================================================
// Function : HasValue
// ================================================================
bool MeshNodeVecProperty::HasValue(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return false;
    }

    return myValues.HasValue(theInfo);
}

// ================================================================
// Function : SetValue
// ================================================================
void MeshNodeVecProperty::SetValue(const MeshPropertyEntityInfo& theInfo, double theValue, bool toComputeRange)
{
    if (!myIsLoaded) {
        return;
    }

    myValues.SetValue(theInfo, theValue);
    if (toComputeRange) {
        SetExternalValue(theValue);
    }
    checkForIdentical(theInfo);
}

// ================================================================
// Function : SetRawValue
// ================================================================
void MeshNodeVecProperty::SetRawValue(const MeshPropertyEntityInfo& theInfo, const char* theValue, bool toComputeRange)
{
    if (!myIsLoaded) {
        return;
    }

    myValues.SetRawValue(theInfo, theValue);
    if (toComputeRange) {
        SetExternalValue(myValues.Value(theInfo));
    }
    checkForIdentical(theInfo);
}

// ================================================================
// Function : Value
// ================================================================
double MeshNodeVecProperty::Value(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return myValues.NaNValue();
    }

    return myValues.Value(theInfo);
}

// ================================================================
// Function : RawValue
// ================================================================
const char* MeshNodeVecProperty::RawValue(const MeshPropertyEntityInfo& theInfo) const
{
    if (!myIsLoaded) {
        return myValues.RawNaNValue();
    }

    return myValues.RawValue(theInfo);
}

// ================================================================
// Function : Unload
// ================================================================
void MeshNodeVecProperty::Unload()
{
    myValues.Clear();
    myIsLoaded = false;
}

// ================================================================
// Function : SubIndices
// ================================================================
void MeshNodeVecProperty::SubIndices(int theIndex, std::vector<int>& theIndices)
{
    myValues.SubIndices(theIndex, theIndices);
}

// ================================================================
// Function : NbSubIndices
// ================================================================
int MeshNodeVecProperty::NbSubIndices(int theIndex) const
{
    return myValues.NbSubIndices(theIndex);
}

// ================================================================
// Function : checkForIdentical
// ================================================================
void MeshNodeVecProperty::checkForIdentical(const MeshPropertyEntityInfo& theInfo)
{
    // Try to fill the internal elemental property until we find an element with
    // non-equal nodal values
    if (!myElementalProperty.IsNull()) {
        if (!myElementalProperty->HasValue(theInfo)) {
            myElementalProperty->SetRawValue(theInfo, myValues.RawValue(theInfo));
        } else if (::strncmp(myValues.RawValue(theInfo), myElementalProperty->RawValue(theInfo), myValues.ElemSize())
                   != 0) {
            myElementalProperty.Nullify();
        }
    }

    // Try to fill the internal nodal property until we find a node with
    // different property values coming from two elements sharing the node
    if (!myNodalProperty.IsNull()) {
        RWCae::MeshPropertyEntityInfo anInfo(theInfo.SubIndex);
        if (!myNodalProperty->HasValue(anInfo)) {
            myNodalProperty->SetRawValue(anInfo, myValues.RawValue(theInfo));
        } else if (::strncmp(myValues.RawValue(theInfo), myNodalProperty->RawValue(anInfo), myValues.ElemSize()) != 0) {
            myNodalProperty.Nullify();
        }
    }
}

} // namespace