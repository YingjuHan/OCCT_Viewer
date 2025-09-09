#include <ViewBase/InteractiveScaleRulerOwner.h>
#include <ViewBase/InteractiveScaleRuler.h>

namespace ViewBase
{

    InteractiveScaleRulerOwner::InteractiveScaleRulerOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        myIsInfinite = true;
        SetPriority(ViewBase::AverageOwnerSelectionPriority5);
    }

    Handle(ViewBase::InteractiveOwner) InteractiveScaleRulerOwner::Clone(const TCollection_AsciiString &)
    {
        return nullptr;
    }

    Standard_Boolean InteractiveScaleRulerOwner::HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                                  Aspect_VKeyMouse theButton,
                                                                  Aspect_VKeyFlags theModifiers,
                                                                  bool theIsDoubleClick)
    {
        if (InteractiveOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick))
        {
            return true;
        }

        return true;
    }

}