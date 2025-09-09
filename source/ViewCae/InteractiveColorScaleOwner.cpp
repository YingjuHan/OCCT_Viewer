#include <ViewCae/InteractiveColorScaleOwner.h>

namespace ViewCae {

// =======================================================================
// function : InteractiveColorScaleOwner
// =======================================================================
InteractiveColorScaleOwner::InteractiveColorScaleOwner(const TCollection_AsciiString& theId) : InteractiveOwner(theId)
{
    SetPriority(ViewBase::AverageOwnerSelectionPriority5);
}

// =======================================================================
// function : Clone
// =======================================================================
Handle(ViewBase::InteractiveOwner) InteractiveColorScaleOwner::Clone(const TCollection_AsciiString&)
{
    return nullptr;
}

} // namespace

