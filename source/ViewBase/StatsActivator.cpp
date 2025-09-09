#include <ViewBase/StatsActivator.h>

#include <ViewBase/InteractiveOwner.h>

#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Message.hxx>
#include <OSD_Timer.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <V3d_View.hxx>

namespace ViewBase
{

    //! Redefined entity owner that is highlighted when owner is detected,
    //! even if Interactive Context highlighted on last detection procedure.
    class StatsActivator::StatsActivatorOwner : public SelectMgr_EntityOwner
    {
    public:
        //! Main constructor.
        explicit StatsActivatorOwner(const Handle(StatsActivator) & theObject)
            : SelectMgr_EntityOwner((const Handle(SelectMgr_SelectableObject) &)theObject,
                                    ViewBase::AverageOwnerSelectionPriority5)
        {
            //
        }

        //! Handle mouse button click event.
        virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i &, Aspect_VKeyMouse, Aspect_VKeyFlags, bool) override
        {
            if (myTimer.IsStarted())
            {
                if (myTimer.ElapsedTime() > myMaxTimerThreshold)
                {
                    myClickCount = myMaxClickCount;
                    myTimer.Restart();
                }
            }
            else
            {
                myTimer.Start();
            }
            if (--myClickCount <= 0)
            {
                StatsActivator *aPrs = dynamic_cast<StatsActivator *>(mySelectable);
                AIS_InteractiveContext *aCtx = aPrs != nullptr ? aPrs->InteractiveContext() : nullptr;
                Handle(V3d_View) aView = aCtx != nullptr ? aCtx->LastActiveView() : Handle(V3d_View)();
                if (!aView.IsNull())
                {
                    bool wasToShowStats = aView->RenderingParams().ToShowStats;

                    aView->ChangeRenderingParams().ToShowStats = !wasToShowStats;
                    if (!wasToShowStats)
                    {
                        aPrs->UpdateExtraInfo();
                    }
                }
                myClickCount = myMaxClickCount;
            }
            return true;
        }

    private:
        OSD_Timer myTimer;
        const double myMaxTimerThreshold = 1.5;
        const int myMaxClickCount = 5;
        int myClickCount = 5; //!< click count to show statistic
    };

    StatsActivator::StatsActivator() : myExtraInfo(new TCollection_HAsciiString()), mySize(m_defStatSize)
    {
        SetInfiniteState(true);
        myTransformPersistence = new Graphic3d_TransformPers(Graphic3d_TMF_2d, Aspect_TOTP_LEFT_LOWER);
        myDrawer->SetZLayer(Graphic3d_ZLayerId_BotOSD);
        Handle(Graphic3d_AspectFillArea3d) anAspect = new Graphic3d_AspectFillArea3d();
        anAspect->SetInteriorStyle(Aspect_IS_EMPTY);
        myDrawer->SetupOwnShadingAspect();
        myDrawer->ShadingAspect()->SetAspect(anAspect);
    }

    StatsActivator::~StatsActivator()
    {
        //
    }

    void StatsActivator::UpdateExtraInfo()
    {
        myExtraInfo->Clear();
        if (!HasInteractiveContext())
        {
            return;
        }
    }

    static Handle(Graphic3d_ArrayOfTriangles) getTriangles(int theSize)
    {
        static constexpr int nbQVerts4 = 4;
        static constexpr int nbQTriInd6 = 6;

        static constexpr int qn1 = 1;
        static constexpr int qn2 = 2;
        static constexpr int qn3 = 3;
        static constexpr int qn4 = 4;

        const double aSize = double(theSize) / 2.0;

        Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(nbQVerts4, nbQTriInd6);
        aTris->AddVertex(gp_Pnt(-aSize, -aSize, 0.0));
        aTris->AddVertex(gp_Pnt(-aSize, aSize, 0.0));
        aTris->AddVertex(gp_Pnt(aSize, aSize, 0.0));
        aTris->AddVertex(gp_Pnt(aSize, -aSize, 0.0));
        aTris->AddQuadTriangleEdges(qn1, qn2, qn3, qn4);
        return aTris;
    }

    void StatsActivator::Compute(const Handle(PrsMgr_PresentationManager) &,
                                 const Handle(Prs3d_Presentation) & thePrs,
                                 const Standard_Integer theMode)
    {
        thePrs->SetInfiniteState(true);
        if (theMode != 0)
        {
            return;
        }

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->AddPrimitiveArray(getTriangles(mySize));
        aGroup->SetGroupPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
    }

    void StatsActivator::ComputeSelection(const Handle(SelectMgr_Selection) & theSel, const Standard_Integer theMode)
    {
        if (theMode != 0)
        {
            return;
        }

        Handle(Graphic3d_ArrayOfTriangles) aTris = getTriangles(mySize);
        Handle(StatsActivatorOwner) anOwner = new StatsActivatorOwner(this);
        Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(anOwner);
        aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
        theSel->Add(aSens);
    }

} // namespace
