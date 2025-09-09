#include <ViewDim/InteractiveAngleDimension.h>

namespace ViewDim
{

    // ================================================================
    // Function : InteractiveAngleDimension
    // ================================================================
    InteractiveAngleDimension::InteractiveAngleDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId)
        : PrsDim_AngleDimension(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 0.0, 1.0), gp_Pnt(0.0, 0.0, 0.0))
    {
        myOwner = theNewId;
        theNewId->SetSelectablePointer(this);
    }

    // ================================================================
    // Function : ~InteractiveAngleDimension
    // ================================================================
    InteractiveAngleDimension::~InteractiveAngleDimension()
    {
        //
    }

    // ================================================================
    // Function : Compute
    // ================================================================
    void InteractiveAngleDimension::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                            const Handle(Prs3d_Presentation) & thePrs,
                                            const Standard_Integer theMode)
    {
        InteractiveDimensionOwner::UpdateFlyout(*this);
        InteractiveDimensionOwner::UpdatePickTolerance(*this);

        PrsDim_AngleDimension::Compute(thePrsMgr, thePrs, theMode);
    }
}