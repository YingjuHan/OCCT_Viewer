#include <ViewDim/InteractiveLengthDimension.h>

namespace ViewDim
{

    // ================================================================
    // Function : InteractiveLengthDimension
    // ================================================================
    InteractiveLengthDimension::InteractiveLengthDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId)
    {
        myOwner = theNewId;
        theNewId->SetSelectablePointer(this);
    }

    // ================================================================
    // Function : ~InteractiveLengthDimension
    // ================================================================
    InteractiveLengthDimension::~InteractiveLengthDimension()
    {
        //
    }

    // ================================================================
    // Function : Compute
    // ================================================================
    void InteractiveLengthDimension::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode)
    {
        InteractiveDimensionOwner::UpdateFlyout(*this);
        InteractiveDimensionOwner::UpdatePickTolerance(*this);

        PrsDim_LengthDimension::Compute(thePrsMgr, thePrs, theMode);
    }
}