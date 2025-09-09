#include <ViewBase/InteractiveTextLabelOwner.h>
#include <ViewBase/InteractiveTextLabel.h>
#include <Prs3d_LineAspect.hxx>

namespace ViewBase
{
    InteractiveTextLabelOwner::InteractiveTextLabelOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        SetPriority(ViewBase::AverageOwnerSelectionPriority5);
    }

    Handle(ViewBase::InteractiveOwner) InteractiveTextLabelOwner::Clone(const TCollection_AsciiString&)
    {
        return nullptr;
    }

    Standard_Boolean InteractiveTextLabelOwner::HandleMouseClick(
        const Graphic3d_Vec2i &thePoint,
        Aspect_VKeyMouse theButton,
        Aspect_VKeyFlags theModifiers,
        bool theIsDoubleClick
    ) {
        if (InteractiveOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick))
        {
            return true;
        }
        return false;
    }

    void InteractiveTextLabelOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                     const Handle(Prs3d_Drawer) & theStyle,
                                                     const Standard_Integer theMode)
    {
        InteractiveTextLabel *anObj = dynamic_cast<InteractiveTextLabel *>(mySelectable);
        if (anObj == nullptr)
        {
            InteractiveOwner::HilightWithColor(thePrsMgr, theStyle, theMode);
            return;
        }
        if (!anObj->IsEnabled())
        {
            return;
        }

        const bool isSelection = !thePrsMgr->IsImmediateModeOn();
        if (isSelection && !anObj->ToDrawRectangle())
        {
            InteractiveOwner::HilightWithColor(thePrsMgr, theStyle, theMode);
            return;
        }
        else if (isSelection)
        {
            anObj->UpdateAspects();
            return;
        }

        Handle(Prs3d_Presentation) aPrs = GetHighlightPresentation(thePrsMgr, theStyle, isSelection);
        {
            Handle(Graphic3d_Group) aGroup = aPrs->NewGroup();
            aGroup->SetTransformPersistence(anObj->LabelTransformPersistence());

            static constexpr int rectExpand2 = 2;

            Handle(Graphic3d_ArrayOfTriangles) aTris = anObj->DrawRectangle(rectExpand2);
            aGroup->SetGroupPrimitivesAspect(theStyle->ShadingAspect()->Aspect());
            aGroup->AddPrimitiveArray(aTris);
        }

        if (!isSelection)
        {
            thePrsMgr->AddToImmediateList(aPrs);
        }
        else
        {
            aPrs->Display();
        }
    }

    void InteractiveTextLabelOwner::Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                              const Standard_Integer theMode)
    {
        InteractiveTextLabel *anObj = dynamic_cast<InteractiveTextLabel *>(mySelectable);
        if (anObj != nullptr)
        {
            anObj->UpdateAspects();
            if (Handle(Prs3d_Presentation) aPrs = anObj->GetSelectPresentation(Handle(PrsMgr_PresentationManager)()))
            {
                aPrs->Erase();
            }
        }
        InteractiveOwner::Unhilight(thePrsMgr, theMode);
    }

}