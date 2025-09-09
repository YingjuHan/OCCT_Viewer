#include <RWShapeTest/DrawableShapeData.h>

#include <ViewBaseTest/ViewerCommand.h>

#include <RWBase/ConditionPromise.h>
#include <RWBase/TaskManager.h>

#include <RWShapeConv/ShapeLoader.h>

#include <ViewShape/InteractiveShape.h>

#include <BRep_Tool.hxx>
#include <DBRep.hxx>
#include <Draw.hxx>
#include <Draw_Display.hxx>
#include <Message.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <ViewerTest.hxx>

namespace RWShapeTest {

// =======================================================================
// function : DrawableShapeData
// =======================================================================
DrawableShapeData::DrawableShapeData(const Handle(RWShape::ShapeData)& theData)
    : DBRep_DrawableShape(theData->GetSingleShape(),
                          Draw_vert,
                          Draw_jaune,
                          Draw_rouge,
                          Draw_bleu,
                          DBRep::Parameters().Size,
                          DBRep::Parameters().NbIsos,
                          DBRep::Parameters().Discretization),
      myShapeData(theData)
{
    DBRep_Params& aParams = DBRep::Parameters();
    DisplayTriangulation(aParams.DispTriangles);
    DisplayPolygons(aParams.DisplayPolygons);
    DisplayHLR(aParams.WithHLR, aParams.WithRg1, aParams.WithRgN, aParams.WithHid, aParams.HLRAngle);
}

// =======================================================================
// function : ~DrawableShapeData
// =======================================================================
DrawableShapeData::~DrawableShapeData()
{
    //
}

// =======================================================================
// function : Whatis
// =======================================================================
void DrawableShapeData::Whatis(Draw_Interpretor& theDI) const
{
    std::stringstream aStr;
    PrintInfo(aStr);
    theDI << aStr.str().c_str();
}

// =======================================================================
// function : PrintInfo
// =======================================================================
void DrawableShapeData::PrintInfo(Standard_OStream& theStream) const
{
    int aNbTris  = 0;
    int aNbNodes = 0;
    for (const TopoDS_Shape& aShapeIter : myShapeData->Shapes()) {
        for (TopExp_Explorer aFaceIter(aShapeIter, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next()) {
            TopLoc_Location    aLoc;
            const TopoDS_Face& aFace = TopoDS::Face(aFaceIter.Value());
            if (const Handle(Poly_Triangulation)& aTris = BRep_Tool::Triangulation(aFace, aLoc)) {
                aNbTris += aTris->NbTriangles();
                aNbNodes += aTris->NbNodes();
            }
        }
    }

    theStream << "ShapeData[" << aNbNodes << " nodes][" << aNbTris << " triangles]";
}

// =======================================================================
// function : DrawOn
// =======================================================================
void DrawableShapeData::DrawOn(Draw_Display& theDisp) const
{
    static constexpr double halfMult = 0.5;
    if (!myShapeData->IsLoaded()) {
        gp_Pnt aPnt;
        if (!myShapeData->BoundingBox().IsVoid()) {
            aPnt = (myShapeData->BoundingBox().CornerMin().XYZ() + myShapeData->BoundingBox().CornerMax().XYZ())
                 * halfMult;
        }

        const Draw_Color aColor(Draw_rouge);
        theDisp.SetColor(aColor);
        theDisp.DrawString(aPnt, "Deferred Shape");
        return;
    }
    DBRep_DrawableShape::DrawOn(theDisp);
}

// =======================================================================
// function : Display
// =======================================================================
bool DrawableShapeData::Display(const PrsMgr_DisplayStatus           theStatus,
                                const Handle(AIS_InteractiveObject)& theOldPrs) const
{
    const Handle(AIS_InteractiveContext)& aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull() || theStatus == PrsMgr_DisplayStatus_None) {
        return false;
    }

    Handle(ViewShape::InteractiveShapeOwner) anOwner = new ViewShape::InteractiveShapeOwner(myShapeData);
    if (Handle(ViewShape::InteractiveShape) anOldPrs = Handle(ViewShape::InteractiveShape)::DownCast(theOldPrs)) {
        anOwner->SetPresentationMode(anOldPrs->MeshOwner()->PresentationMode());
        anOwner->SetSelectionMode(anOldPrs->MeshOwner()->GetSelectionMode());
    }

    Handle(ViewShape::InteractiveShape) aPrs = Handle(ViewShape::InteractiveShape)::DownCast(
        anOwner->CreatePresentation());
    if (theStatus == PrsMgr_DisplayStatus_Erased) {
        if (!ViewBaseTest::ViewerCommand::RegisterObject(myShapeData->Id(), aPrs)) {
            return false;
        }
        aCtx->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
        return true;
    }

    ViewerTest::Display(myShapeData->Id(), aPrs, false);
    return true;
}

// =======================================================================
// function : LoadData
// =======================================================================
bool DrawableShapeData::LoadData(const NCollection_List<Handle(RWShapeTest::DrawableShapeData)>& theObjects,
                                 const bool                                                        theIsMulti,
                                 const bool                                                        theIsAsync)
{
    if (theObjects.IsEmpty()) {
        return false;
    }

    RWBase::TaskManager::Instance()->SetMultithreadingMode(theIsMulti);
    if (!theIsAsync) {
        for (const Handle(RWShapeTest::DrawableShapeData)& anObjIter : theObjects) {
            if (!RWShapeConv::ShapeLoader::LoadShape(anObjIter->ShapeData(), nullptr)) {
                Message::SendFail() << "Syntax error: object '" << anObjIter->ShapeData()->Id() << "' cannot be loaded";
                return false;
            }
        }
        return true;
    }

    std::deque<std::shared_ptr<RWBase::ConditionPromise>> aCallbackList;
    for (const Handle(RWShapeTest::DrawableShapeData)& anObjIter : theObjects) {
        std::shared_ptr<RWBase::ConditionPromise> anObjCallback = std::make_shared<RWBase::ConditionPromise>();
        if (!RWShapeConv::ShapeLoader::LoadShape(anObjIter->ShapeData(), anObjCallback.get())) {
            if (!anObjCallback->Error().empty()) {
                Message::SendFail() << "Error: " << anObjCallback->Error() << "\n";
            }
            Message::SendFail() << "Syntax error: object cannot be loaded";
            return false;
        }
        aCallbackList.push_back(anObjCallback);
    }

    // emulate main loop until end of computation
    while (!aCallbackList.empty()) {
        for (std::deque<std::shared_ptr<RWBase::ConditionPromise>>::iterator aCallbackIter = aCallbackList.begin();
             aCallbackIter != aCallbackList.end();
             ++aCallbackIter) {
            const std::shared_ptr<RWBase::ConditionPromise>& aCallback = *aCallbackIter;
            if (!aCallback->ChangeCondition().Check()) {
                RWBase::TaskManager::Instance()->RunOnce();
                // @c OSD::MilliSecSleep (1);
                continue;
            } else if (!aCallback->Error().empty()) {
                Message::SendFail() << "Error: " << aCallback->Error();
                return false;
            }
            aCallbackList.erase(aCallbackIter);
            break;
        }
    }

    for (const Handle(RWShapeTest::DrawableShapeData)& anObjIter : theObjects) {
        Draw::Set(anObjIter->ShapeData()->Id().ToCString(), anObjIter);
    }
    return true;
}

}

