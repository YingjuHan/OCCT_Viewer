#include <RWCae/MeshScalarPropertyBase.h>

namespace RWCae {

// ================================================================
// Function : Constructor
// ================================================================
MeshScalarPropertyBase::MeshScalarPropertyBase(const TCollection_AsciiString& theName, const MappingType theMapType)
    : MeshPropertyBase(theName, theMapType),
      myIsEnabled(true)
{
    //
}

}