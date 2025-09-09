#include <RWCaeTest/DrawableMeshData.h>

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/CaeMeshTools.h>

#include <BRep_Tool.hxx>
#include <DBRep.hxx>
#include <Draw.hxx>
#include <Draw_Display.hxx>
#include <Message.hxx>
#include <ViewerTest.hxx>

namespace RWCaeTest {

// =======================================================================
// function : DrawableMeshData
// =======================================================================
DrawableMeshData::DrawableMeshData(const Handle(RWCae::MeshData)& theData)
    : DBRep_DrawableShape(TopoDS_Shape(),
                          Draw_vert,
                          Draw_jaune,
                          Draw_rouge,
                          Draw_bleu,
                          DBRep::Parameters().Size,
                          DBRep::Parameters().NbIsos,
                          DBRep::Parameters().Discretization),
      myMeshData(theData)
{
    DBRep_Params& aParams = DBRep::Parameters();
    DisplayTriangulation(aParams.DispTriangles);
    DisplayPolygons(aParams.DisplayPolygons);
    DisplayHLR(aParams.WithHLR, aParams.WithRg1, aParams.WithRgN, aParams.WithHid, aParams.HLRAngle);
}

// =======================================================================
// function : ~DrawableMeshData
// =======================================================================
DrawableMeshData::~DrawableMeshData()
{
    //
}

// =======================================================================
// function : Whatis
// =======================================================================
void DrawableMeshData::Whatis(Draw_Interpretor& theDI) const
{
    std::stringstream aStr;
    PrintInfo(aStr);
    theDI << aStr.str().c_str();
}

// =======================================================================
// function : PrintInfo
// =======================================================================
void DrawableMeshData::PrintInfo(Standard_OStream& theStream) const
{
    theStream << "MeshData";
    myMeshData->PrintStats(theStream);
}

// =======================================================================
// function : DrawOn
// =======================================================================
void DrawableMeshData::DrawOn(Draw_Display& theDisp) const
{
    Handle(ViewCaeGeom::MeshBvhData)  aBvhData = new ViewCaeGeom::MeshBvhData();
    Handle(ViewCaeGeom::MeshGeometry) aGeom    = new ViewCaeGeom::MeshGeometryBase(aBvhData, myMeshData);

    if (Handle(Graphic3d_ArrayOfSegments) aSegs3d =
            ViewCae::CaeMeshTools::CreateMeshEdges(aGeom, ViewCae::MeshShadedType::VOLUME_SURFACE)) {
        DrawSegments(theDisp, aSegs3d, Draw_bleu);
    } else if (Handle(Graphic3d_ArrayOfSegments) aSegs2d =
                   ViewCae::CaeMeshTools::CreateMeshEdges(aGeom, ViewCae::MeshShadedType::FACES)) {
        DrawSegments(theDisp, aSegs2d, Draw_bleu);
    }

    if (Handle(Graphic3d_ArrayOfSegments) aSharpSegs3d =
            ViewCae::CaeMeshTools::CreateMeshSharpEdges(aGeom, ViewCae::MeshShadedType::VOLUME_SURFACE)) {
        DrawSegments(theDisp, aSharpSegs3d, Draw_rouge);
    } else if (Handle(Graphic3d_ArrayOfSegments) aSharpSegs2d =
                   ViewCae::CaeMeshTools::CreateMeshSharpEdges(aGeom, ViewCae::MeshShadedType::FACES)) {
        DrawSegments(theDisp, aSharpSegs2d, Draw_rouge);
    }
}

// =======================================================================
// function : Display
// =======================================================================
bool DrawableMeshData::Display(const PrsMgr_DisplayStatus           theStatus,
                               const Handle(AIS_InteractiveObject)& theOldPrs) const
{
    const Handle(AIS_InteractiveContext)& aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull() || theStatus == PrsMgr_DisplayStatus_None) {
        return false;
    }

    Handle(ViewCae::InteractiveCaeMeshOwner) anOwner = new ViewCae::InteractiveCaeMeshOwner(myMeshData);
    if (Handle(ViewCae::InteractiveCaeMesh) anOldPrs = Handle(ViewCae::InteractiveCaeMesh)::DownCast(theOldPrs)) {
        anOwner->SetSelectionMode(anOldPrs->MeshOwner()->GetSelectionMode());
    }

    Handle(ViewCae::InteractiveCaeMesh) aMeshPrs = new ViewCae::InteractiveCaeMesh(anOwner);
    const TCollection_AsciiString        anId     = aMeshPrs->MeshOwner()->Id();
    aMeshPrs->SetOnColorMapperChangeCallback(
        [anId](const RWCae::MeshPropertyComponent& theProp, const Handle(ViewCae::MeshColorMapper)& theMapper) {
            (void)anId;
            (void)theProp;
            (void)theMapper;
        });

    aMeshPrs->MeshOwner()->SetUseTestCachePrs(myToTestCacheGenGeom);

    if (theStatus == PrsMgr_DisplayStatus_Erased) {
        if (!ViewBaseTest::ViewerCommand::RegisterObject(myMeshData->Id(), aMeshPrs)) {
            return false;
        }
        aCtx->Display(aMeshPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
        aMeshPrs->CreatePresentation(ViewCae::MeshPresentationType::Scalar);
        return true;
    }

    aMeshPrs->CreatePresentation(ViewCae::MeshPresentationType::Scalar);
    aMeshPrs->ProcessRedraw(aCtx, ViewerTest::CurrentView(), AIS_RedrawProgress_BeforeUpdate);
    ViewerTest::Display(myMeshData->Id(), aMeshPrs, false);
    for (const Handle(PrsMgr_PresentableObject)& aChildIter : aMeshPrs->Children()) {
        Handle(AIS_InteractiveObject) aChildObj = Handle(AIS_InteractiveObject)::DownCast(aChildIter);
        ViewerTest::GetAISContext()->Display(aChildObj, false);
    }
    return true;
}

}

