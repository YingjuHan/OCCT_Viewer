#include <ViewDim/InteractiveDimensionOwner.h>

#include <Units.hxx>

namespace ViewDim
{

    // =======================================================================
    // function : InteractiveDimensionOwner
    // =======================================================================
    InteractiveDimensionOwner::InteractiveDimensionOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        //
    }

    // =======================================================================
    // function : Clone
    // =======================================================================
    Handle(ViewBase::InteractiveOwner) InteractiveDimensionOwner::Clone(const TCollection_AsciiString &)
    {
        return nullptr;
    }

    // =======================================================================
    // function : UpdateFlyout
    // =======================================================================
    void InteractiveDimensionOwner::UpdateFlyout(PrsDim_Dimension &theDim)
    {
        if (theDim.IsTextPositionCustom())
        {
            return;
        }

        static constexpr double defFly = 0.015;

        const double aFly = Units::FromSI(defFly, theDim.Attributes()->DimLengthModelUnits().ToCString());
        theDim.SetFlyout(aFly);
    }

    // =======================================================================
    // function : UpdatePickTolerance
    // =======================================================================
    void InteractiveDimensionOwner::UpdatePickTolerance(PrsDim_Dimension &theDim)
    {
        static constexpr double defTol = 0.001;

        const double aTol = Units::FromSI(defTol, theDim.Attributes()->DimLengthModelUnits().ToCString());
        theDim.SetSelToleranceForText2d(aTol);
    }

    // =======================================================================
    // function : HandleMouseClick
    // =======================================================================
    Standard_Boolean InteractiveDimensionOwner::HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                                 Aspect_VKeyMouse theButton,
                                                                 Aspect_VKeyFlags theModifiers,
                                                                 bool theIsDoubleClick)
    {
        if (InteractiveOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick))
        {
            return true;
        }

        return false;
    }

    // =======================================================================
    // function : HilightWithColor
    // =======================================================================
    void InteractiveDimensionOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                     const Handle(Prs3d_Drawer) & theStyle,
                                                     const Standard_Integer theMode)
    {
        InteractiveOwner::HilightWithColor(thePrsMgr, theStyle, theMode);
    }

    // =======================================================================
    // function : Unhilight
    // =======================================================================
    void InteractiveDimensionOwner::Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                              const Standard_Integer theMode)
    {
        InteractiveOwner::Unhilight(thePrsMgr, theMode);
    }

} // namespace