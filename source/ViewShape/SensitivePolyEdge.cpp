#include <ViewShape/SensitivePolyEdge.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <Select3D_SensitiveSegment.hxx>
#include <StdSelect_BRepSelectionTool.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : SensitivePolyEdge
    // ================================================================
    SensitivePolyEdge::SensitivePolyEdge(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                         const TopoDS_Edge &theEdge,
                                         const Handle(TColgp_HArray1OfPnt) & thePoints)
        : Select3D_SensitivePoly(theOwnerId, thePoints, true),
          myEdge(theEdge)
    {
        mySFactor = 3;
    }

    // ================================================================
    // Function : ~SensitivePolyEdge
    // ================================================================
    SensitivePolyEdge::~SensitivePolyEdge()
    {
        //
    }

    // ================================================================
    // Function : GetEdgeSensitive
    // ================================================================
    Handle(SensitivePolyEdge) SensitivePolyEdge::GetEdgeSensitive(const TopoDS_Edge &theEdge,
                                                                  const Handle(SelectMgr_EntityOwner) & theOwner,
                                                                  const Handle(Prs3d_Drawer) & theDrawer,
                                                                  const double theDefl,
                                                                  const Handle(SelectMgr_Selection) & theTmpSel)
    {
        const int aNbPOnEdge = 9;
        const double aMaxParam = 500.0;

        Handle(Select3D_SensitiveEntity) aSens;
        StdSelect_BRepSelectionTool::GetEdgeSensitive(theEdge,
                                                      theOwner,
                                                      theTmpSel,
                                                      theDefl,
                                                      theDrawer->DeviationAngle(),
                                                      aNbPOnEdge,
                                                      aMaxParam,
                                                      aSens);

        // put originating into sensitive (create inefficient copy)
        if (Handle(Select3D_SensitivePoly) aDefPolySens = Handle(Select3D_SensitivePoly)::DownCast(aSens))
        {
            Handle(TColgp_HArray1OfPnt) aPnts;
            aDefPolySens->Points3D(aPnts);
            Handle(SensitivePolyEdge) aSensEdge = new SensitivePolyEdge(theOwner, theEdge, aPnts);
            return aSensEdge;
        }
        else if (Handle(Select3D_SensitiveSegment) aDefSegSens = Handle(Select3D_SensitiveSegment)::DownCast(aSens))
        {
            Handle(TColgp_HArray1OfPnt) aPnts = new TColgp_HArray1OfPnt(1, 2);
            aPnts->SetValue(1, aDefSegSens->StartPoint());
            aPnts->SetValue(2, aDefSegSens->EndPoint());
            Handle(SensitivePolyEdge) aSensEdge = new SensitivePolyEdge(theOwner, theEdge, aPnts);
            return aSensEdge;
        }
        else if (!aSens.IsNull())
        {
            // @c return aSens; // shouldn't happen
            return Handle(SensitivePolyEdge)();
        }
        return Handle(SensitivePolyEdge)();
    }

    // ================================================================
    // Function : Matches
    // ================================================================
    Standard_Boolean SensitivePolyEdge::Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                SelectBasics_PickResult &thePickResult)
    {
        const InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());
        if (anOwner != nullptr && anOwner->GetSelectionMode() != (int)InteractiveShapeSelMode::Global)
        {
            if (mySubshapeIndex != -1 && anOwner->GetDisabledElems().Contains(ShapeSelElemType::Edge, mySubshapeIndex))
            {
                return false;
            }
        }

        return Select3D_SensitivePoly::Matches(theMgr, thePickResult);
    }

} // namespace ViewShape