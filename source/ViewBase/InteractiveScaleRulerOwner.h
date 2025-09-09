#ifndef ViewBase_INTERACTIVESCALERULEROWNER_HEADERFILE
#define ViewBase_INTERACTIVESCALERULEROWNER_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

namespace ViewBase {

//! Class represents owner for @link InteractiveScaleRuler @endlink object.
class InteractiveScaleRulerOwner : public InteractiveOwner {
public:
    //! Constructor.
    Standard_EXPORT explicit InteractiveScaleRulerOwner(const TCollection_AsciiString& theId);

    //! Return copy of this presentation; default implementation returns NULL.
    Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString& theNewId) override;

    //! Handle mouse button click event.
    Standard_EXPORT virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                              Aspect_VKeyMouse       theButton,
                                                              Aspect_VKeyFlags       theModifiers,
                                                              bool                   theIsDoubleClick) override;
};

} // namespace

#endif         

