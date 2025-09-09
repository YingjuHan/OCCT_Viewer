#ifndef _INTERACTIVETEXTLABELOWNER_HEADERFILE
#define _INTERACTIVETEXTLABELOWNER_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

namespace ViewBase
{
    class InteractiveTextLabelOwner : public InteractiveOwner
    {
    public:
        Standard_EXPORT explicit InteractiveTextLabelOwner(const TCollection_AsciiString &theId);

        //! Return copy of this presentation; default implementation returns NULL.
        Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theNewId) override;

        //! Handle mouse button click event for text label.
        Standard_EXPORT virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                                  Aspect_VKeyMouse theButton,
                                                                  Aspect_VKeyFlags theModifiers,
                                                                  bool theIsDoubleClick) override;

        //! Compute highlighting for text label.
        Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                      const Handle(Prs3d_Drawer) & theStyle,
                                                      const Standard_Integer theMode) override;

        //! Clear highlighting for text label.
        Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                               const Standard_Integer theMode) override;
    };
}

#endif // _INTERACTIVETEXTLABELOWNER_HEADERFILE