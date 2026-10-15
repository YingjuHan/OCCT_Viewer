#ifndef _INTERACTIVEDIMENSIONOWNER_HEADERFILE
#define _INTERACTIVEDIMENSIONOWNER_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

#include <PrsDim_Dimension.hxx>

namespace ViewDim
{

    //! Class represents owner for dimension object.
    class InteractiveDimensionOwner : public ViewBase::InteractiveOwner
    {
    public:
        //! Constructor.
        Standard_EXPORT explicit InteractiveDimensionOwner(const TCollection_AsciiString &theId);

        //! Return copy of this presentation; default implementation returns NULL.
        Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theNewId) override;

        //! Handle mouse button click event for dimension.
        Standard_EXPORT virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                                  Aspect_VKeyMouse theVButton,
                                                                  Aspect_VKeyFlags theVModifiers,
                                                                  bool theIsDoubleClick) override;

        //! Compute highlighting for dimension.
        Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                      const Handle(Prs3d_Drawer) & theHiStyle,
                                                      const Standard_Integer theMode) override;

        //! Clear highlighting for dimension.
        Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                               const Standard_Integer theMode) override;

    public:
        //! Update flyout on dimension change.
        static void UpdateFlyout(PrsDim_Dimension &theDim);

        //! Update picking tolerance on dimension change.
        static void UpdatePickTolerance(PrsDim_Dimension &theDim);
    };
}
#endif