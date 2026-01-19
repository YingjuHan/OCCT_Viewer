#include <ViewBase/InteractiveOwner.h>

#include <ViewBase/BaseSelection.h>
#include <ViewBase/IInteractiveObject.h>

#include <Graphic3d_ArrayOfPoints.hxx>
#include <Prs3d_PointAspect.hxx>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

//! Callback on object mouse click.
EM_JS(bool, jsOnObjectMouseClick, (const char *theObjId, int thePart, int theSub, int theX, int theY, int theFlags), {
    const aPtr = typeof theObjId === "bigint"
        ? (typeof bigintToI53Checked === "function" ? bigintToI53Checked(theObjId) : Number(theObjId))
        : theObjId;
    return Module.eventManager ?.onObjectMouseClick(UTF8ToString(aPtr), thePart, theSub, theX, theY, theFlags);
});
#endif

namespace ViewBase
{

    //! Dummy presentable object.
    //! Added as a child for AIS_InteractiveObject to show parts on the topmost Z-layer.
    class InteractiveOwner::LayeredAisObject : public AIS_InteractiveObject
    {
    public:
        void Compute(const Handle(PrsMgr_PresentationManager) &,
                     const Handle(Prs3d_Presentation) &,
                     const Standard_Integer) override
        {
        }

        void ComputeSelection(const Handle(SelectMgr_Selection) &, const Standard_Integer) override {}
    };

    // ================================================================
    // Function : FindPrsOwnerById
    // ================================================================
    Handle(InteractiveOwner) InteractiveOwner::FindPrsOwnerById(const AIS_InteractiveContext &theCtx,
                                                                const TCollection_AsciiString &theObjId)
    {
        if (theObjId.IsEmpty())
        {
            return Handle(InteractiveOwner)();
        }

        for (AIS_DataMapOfIOStatus::Iterator anObjIter = theCtx.ObjectIterator(); anObjIter.More(); anObjIter.Next())
        {
            const Handle(AIS_InteractiveObject) &anObj = anObjIter.Key();

            const InteractiveOwner *anOwner = dynamic_cast<InteractiveOwner *>(anObj->GetOwner().get());
            const InteractiveOwner *aChild = anOwner != nullptr ? anOwner->FindChildById(theObjId) : nullptr;
            if (aChild != nullptr)
            {
                return aChild;
            }
        }

        return Handle(InteractiveOwner)();
    }

    // =======================================================================
    // function : Next
    // =======================================================================
    void InteractiveOwner::Iterator::Next()
    {
        if (myOwnIter.More())
        {
            myOwnIter.Next();
            if (myOwnIter.More())
            {
                return;
            }
        }

        myCurrent = nullptr;
        myOwnIter = OwnerList::Iterator();
        for (; myObjIter.More(); myObjIter.Next())
        {
            const Handle(AIS_InteractiveObject) &anObj = myObjIter.Key();
            myCurrent = dynamic_cast<ViewBase::InteractiveOwner *>(anObj->GetOwner().get());
            if (myCurrent != nullptr)
            {
                myObjIter.Next();
                // consider only one-level children
                myOwnIter = myCurrent->ChildIterator();
                break;
            }
        }
    }

    // =======================================================================
    // function : GetPickTolerance
    // =======================================================================
    double InteractiveOwner::GetPickTolerance(const Handle(StdSelect_ViewerSelector3d) & theSelector, const gp_Pnt &thePoint)
    {
        const Handle(Graphic3d_Camera) &aCamera = theSelector->GetManager().Camera();
        if (aCamera.IsNull())
        {
            return 0.0;
        }

        Graphic3d_Vec2i aWinSize;
        theSelector->GetManager().WindowSize(aWinSize.x(), aWinSize.y());
        const double aPixelSize = Max(1.0 / double(aWinSize.x()), 1.0 / double(aWinSize.y()));
        double aScaleFactor = aCamera->IsOrthographic() ? aCamera->Scale() : 2.0 * Tan(aCamera->FOVy() * M_PI / 360.0);
        aScaleFactor *= aPixelSize;
        const double aSensFactor = theSelector->DepthToleranceType() == SelectMgr_TypeOfDepthTolerance_SensitivityFactor
                                       ? theSelector->Sensitivity()
                                       : theSelector->DepthTolerance();
        if (aCamera->IsOrthographic())
        {
            return aScaleFactor * aSensFactor;
        }

        const double aDistFromEye = Abs((thePoint.XYZ() - aCamera->Eye().XYZ()).Dot(aCamera->Direction().XYZ()));
        return aDistFromEye * aScaleFactor * aSensFactor;
    }

    // =======================================================================
    // function : InteractiveOwner
    // =======================================================================
    InteractiveOwner::InteractiveOwner(const TCollection_AsciiString &theId,
                                       const SelectionPart &thePart,
                                       const Standard_Integer thePriority)
        : SelectMgr_EntityOwner(Handle(SelectMgr_SelectableObject)(), thePriority),
          myId(theId),
          myPart(thePart)
    {
        //
    }

    // =======================================================================
    // function : ~InteractiveOwner
    // =======================================================================
    InteractiveOwner::~InteractiveOwner()
    {
        //
    }

    // ================================================================
    // Function : FindChildById
    // ================================================================
    const InteractiveOwner *InteractiveOwner::FindChildById(const TCollection_AsciiString &theObjId) const
    {
        if (theObjId.IsEmpty())
        {
            return nullptr;
        }
        else if (myId.IsEqual(theObjId))
        {
            return this;
        }

        for (const Handle(InteractiveOwner) &aChildIter : Children())
        {
            if (aChildIter->Id().IsEqual(theObjId))
            {
                return aChildIter.get();
            }
        }
        return nullptr;
    }

    // ================================================================
    // Function : SetSelectionMode
    // ================================================================
    void InteractiveOwner::SetSelectionMode(const int theMode)
    {
        Standard_ASSERT_RAISE(AcceptSelectionMode(theMode), "Error! Unsupported selection mode.");
        mySelMode = theMode;

        for (const Handle(InteractiveOwner) &aChildIter : Children())
        {
            aChildIter->SetSelectionMode(theMode);
        }
    }

    // =======================================================================
    // function : GetHighlightZLayer
    // =======================================================================
    Graphic3d_ZLayerId InteractiveOwner::GetHighlightZLayer(const Handle(Prs3d_Drawer) & theStyle, bool theIsSelection) const
    {
        if (mySelectable == nullptr)
        {
            return theStyle->ZLayer() != Graphic3d_ZLayerId_UNKNOWN ? theStyle->ZLayer() : Graphic3d_ZLayerId_Topmost;
        }

        if (theStyle->ZLayer() == Graphic3d_ZLayerId_Top)
        {
            if (mySelectable->ZLayer() == Graphic3d_ZLayerId_Topmost || mySelectable->ZLayer() == Graphic3d_ZLayerId_TopOSD || mySelectable->ZLayer() == Graphic3d_ZLayerId_BotOSD)
            {
                return mySelectable->ZLayer();
            }
        }

        if (theStyle->ZLayer() != Graphic3d_ZLayerId_UNKNOWN)
        {
            return theStyle->ZLayer();
        }

        if (!theIsSelection)
        {
            if (mySelectable->ZLayer() == Graphic3d_ZLayerId_Default)
            {
                return Graphic3d_ZLayerId_Top;
            }
            else if (mySelectable->ZLayer() == Graphic3d_ZLayerId_Top)
            {
                return Graphic3d_ZLayerId_Topmost;
            }
        }

        return mySelectable->ZLayer();
    }

    // =======================================================================
    // function : PrepareHighlightPresentation
    // =======================================================================
    void InteractiveOwner::PrepareHighlightPresentation(const Handle(Prs3d_Presentation) & thePrs,
                                                        const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                        const Handle(Prs3d_Drawer) & theStyle,
                                                        const bool theIsSelection)
    {
        if (thePrs.IsNull())
        {
            return;
        }

        thePrs->Clear();
        thePrs->SetIsForHighlight(true);
        thePrs->SetInfiniteState(true);

        thePrs->CStructure()->ViewAffinity = thePrsMgr->StructureManager()->ObjectAffinity(mySelectable);
        const Graphic3d_ZLayerId aZLayer = GetHighlightZLayer(theStyle, theIsSelection);
        if (thePrs->GetZLayer() != aZLayer)
        {
            thePrs->SetZLayer(aZLayer);
        }

        if (theIsSelection && aZLayer == mySelectable->ZLayer() && !mySelectable->Presentations().IsEmpty())
        {
            Graphic3d_DisplayPriority aDispPr = mySelectable->Presentations().First()->DisplayPriority();
            thePrs->SetDisplayPriority((Graphic3d_DisplayPriority)Min(aDispPr + 1, Graphic3d_DisplayPriority_Topmost));
        }
    }

    // =======================================================================
    // function : GetHighlightPresentation
    // =======================================================================
    Handle(Prs3d_Presentation) InteractiveOwner::GetHighlightPresentation(
        const Handle(PrsMgr_PresentationManager) & thePrsMgr,
        const Handle(Prs3d_Drawer) & theStyle,
        const bool theIsSelection)
    {
        Handle(Prs3d_Presentation) aPrs = !theIsSelection ? mySelectable->GetHilightPresentation(thePrsMgr)
                                                          : mySelectable->GetSelectPresentation(thePrsMgr);
        if (!thePrsMgr.IsNull())
        {
            PrepareHighlightPresentation(aPrs, thePrsMgr, theStyle, theIsSelection);
        }
        return aPrs;
    }

    // =======================================================================
    // function : GetLayeredPresentation
    // =======================================================================
    Handle(PrsMgr_Presentation) InteractiveOwner::GetLayeredPresentation(
        const Handle(PrsMgr_PresentationManager) & thePrsMgr,
        const int theDispMode,
        const bool theToCreate,
        const Graphic3d_ZLayerId theLayer)
    {
        AIS_InteractiveObject *aSelObj = dynamic_cast<AIS_InteractiveObject *>(mySelectable);
        if (aSelObj == nullptr)
        {
            return Handle(PrsMgr_Presentation)();
        }

        if (aSelObj->Children().IsEmpty())
        {
            if (!theToCreate)
            {
                return Handle(PrsMgr_Presentation)();
            }

            Handle(PrsMgr_PresentableObject) aTopPrsObj = new LayeredAisObject();
            aSelObj->AddChild(aTopPrsObj);
        }
        if (thePrsMgr.IsNull())
        {
            return Handle(PrsMgr_Presentation)();
        }

        Handle(LayeredAisObject) aChildAis = Handle(LayeredAisObject)::DownCast(aSelObj->Children().First());
        if (aChildAis.IsNull())
        {
            return Handle(PrsMgr_Presentation)();
        }

        Handle(PrsMgr_Presentation) aTopPrs = thePrsMgr->Presentation(aChildAis, theDispMode, theToCreate, aSelObj);
        if (aTopPrs.IsNull())
        {
            return aTopPrs;
        }

        aTopPrs->Clear();
        aTopPrs->SetInfiniteState(myIsInfinite);
        if (aTopPrs->GetZLayer() != theLayer)
        {
            aTopPrs->SetZLayer(theLayer);
            aChildAis->Attributes()->SetZLayer(theLayer);
        }
        aTopPrs->CStructure()->ViewAffinity = thePrsMgr->StructureManager()->ObjectAffinity(aSelObj);
        aTopPrs->SetUpdateStatus(false);
        return aTopPrs;
    }

    // =======================================================================
    // function : GetCurrentSelectionScheme
    // =======================================================================
    AIS_SelectionScheme InteractiveOwner::GetCurrentSelectionScheme() const
    {
        const AIS_InteractiveObject *aSelObj = dynamic_cast<AIS_InteractiveObject *>(mySelectable);

        const BaseSelection *aBaseSel = aSelObj->HasInteractiveContext()
                                            ? dynamic_cast<BaseSelection *>(aSelObj->InteractiveContext()->Selection().get())
                                            : nullptr;
        return aBaseSel != nullptr ? aBaseSel->GetSelectionScheme() : AIS_SelectionScheme_UNKNOWN;
    }

    // =======================================================================
    // function : HilightWithColor
    // =======================================================================
    void InteractiveOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                            const Handle(Prs3d_Drawer) & theStyle,
                                            const Standard_Integer theMode)
    {
        SelectMgr_EntityOwner::HilightWithColor(thePrsMgr, theStyle, theMode);
    }

    // =======================================================================
    // function : Unhilight
    // =======================================================================
    void InteractiveOwner::Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr, const Standard_Integer theMode)
    {
        SelectMgr_EntityOwner::Unhilight(thePrsMgr, theMode);
    }

    // =======================================================================
    // function : HandleMouseClick
    // =======================================================================
    Standard_Boolean InteractiveOwner::HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                        Aspect_VKeyMouse theButton,
                                                        Aspect_VKeyFlags theModifiers,
                                                        bool theIsDoubleClick)
    {
        ViewBase::IInteractiveObject *anObj = dynamic_cast<ViewBase::IInteractiveObject *>(mySelectable);
        Handle(AIS_InteractiveObject) aSelObj = Handle(AIS_InteractiveObject)::DownCast(mySelectable);
        const char *anObjStrId = myId.ToCString();
#ifdef __EMSCRIPTEN__
        unsigned int aFlags = theModifiers | theButton;
        if (theIsDoubleClick)
        {
            static constexpr unsigned int bitDoubleClick = 1 << 20;
            aFlags |= bitDoubleClick;
        }
        if (!jsOnObjectMouseClick(anObjStrId, myPart.Id, myPart.SubId, thePoint.x(), thePoint.y(), (int)aFlags))
        {
            return true;
        }
#else
        (void)anObjStrId;
#endif

        if (anObj != nullptr)
        {
            return anObj->HandleMouseClick(aSelObj, thePoint, theButton, theModifiers, theIsDoubleClick);
        }
        return false;
    }

}
