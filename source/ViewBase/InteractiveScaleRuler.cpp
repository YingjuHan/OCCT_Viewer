#include <ViewBase/InteractiveScaleRuler.h>

#include <ViewBase/InteractiveScaleRulerOwner.h>

namespace ViewBase
{

    InteractiveScaleRuler::InteractiveScaleRuler(const Handle(ViewBase::InteractiveScaleRulerOwner) & theNewId,
                                                 const Handle(Prs3d_TextAspect) & theTextStyle)
    {
        theNewId->SetSelectablePointer(this);
        myOwner = theNewId;
        myDrawer->SetTextAspect(theTextStyle);
        myViewOffset = myTransformPersistence->Offset2d();
    }

    InteractiveScaleRuler::~InteractiveScaleRuler()
    {
        //
    }

    bool InteractiveScaleRuler::OnPixelScaleChange(const Handle(AIS_InteractiveObject) & theObj, float theRatio)
    {
        IInteractiveObject::OnPixelScaleChange(theObj, theRatio);
        if (HasInteractiveContext())
        {
            InteractiveContext()->Redisplay(this, false);
        }
        return true;
    }

}