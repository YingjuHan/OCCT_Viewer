#include <RWCae/MeshPropertyBase.h>

#include <Standard_Atomic.hxx>

namespace {
static volatile int g_meshPropCounter = 0;
}

namespace RWCae {

// ================================================================
// Function : Constructor
// ================================================================
MeshPropertyBase::MeshPropertyBase(const TCollection_AsciiString& theName, const MappingType theMapType)
    : myId(generateId()),
      myName(theName),
      myMapType(theMapType)
{
    //
}

// ================================================================
// Function : generateId
// ================================================================
TCollection_AsciiString MeshPropertyBase::generateId()
{
    return TCollection_AsciiString("prop_") + TCollection_AsciiString(Standard_Atomic_Increment(&g_meshPropCounter));
}

// ================================================================
// Function : ResetStaticCounter
// ================================================================
void MeshPropertyBase::ResetStaticCounter()
{
    g_meshPropCounter = 0;
}

} // namespace