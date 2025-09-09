#include <ViewBase/InteractiveViewCubeOwner.h>
#include <ViewBase/InteractiveViewCube.h>

namespace ViewBase
{
    InteractiveViewCubeOwner::InteractiveViewCubeOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        myIsInfinite = true;
        SetPriority(ViewBase::AverageOwnerSelectionPriority5);
    }

    Handle(ViewBase::InteractiveOwner) InteractiveViewCubeOwner::Clone(const TCollection_AsciiString &theId)
    {
        (void) theId;
        return nullptr;
    }
}