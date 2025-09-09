#include <ViewDim/InteractiveRadiusDimension.h>

namespace ViewDim
{

    // ================================================================
    // Function : InteractiveRadiusDimension
    // ================================================================
    InteractiveRadiusDimension::InteractiveRadiusDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId)
        : PrsDim_RadiusDimension(gp_Circ(gp_Ax2(), 0.0))
    {
        myOwner = theNewId;
        theNewId->SetSelectablePointer(this);
    }

    // ================================================================
    // Function : ~InteractiveRadiusDimension
    // ================================================================
    InteractiveRadiusDimension::~InteractiveRadiusDimension()
    {
        //
    }

    // ================================================================
    // Function : Compute
    // ================================================================
    void InteractiveRadiusDimension::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode)
    {
        InteractiveDimensionOwner::UpdatePickTolerance(*this);

        PrsDim_RadiusDimension::Compute(thePrsMgr, thePrs, theMode);
    }
}