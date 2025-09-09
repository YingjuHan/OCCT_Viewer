#include <ViewBase/InteractiveClippingOwner.h>

#include <ViewBase/InteractiveClipping.h>

namespace ViewBase
{

    InteractiveClippingOwner::InteractiveClippingOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        myIsInfinite = true;
        SetPriority(ViewBase::AverageOwnerSelectionPriority5);
    }

    Handle(ViewBase::InteractiveOwner) InteractiveClippingOwner::Clone(const TCollection_AsciiString &)
    {
        return nullptr;
    }

} // namespace ViewBase