
#include <SimMeshDraw.h>

#include <SimMesh.h>
#include <SimMeshDraw_DataSource.h>
#include <SimMeshDraw_DrawableMesh.h>
#include <SimMesh_Model.h>
#include <SimMesh_Persistence.h>
#include <SimMesh_Simplifier.h>

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <BRep_Builder.hxx>
#include <DBRep.hxx>
#include <Draw.hxx>
#include <Draw_PluginMacro.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <MeshVS_Mesh.hxx>
#include <MeshVS_MeshEntityOwner.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <MeshVS_SelectionModeFlags.hxx>
#include <TopoDS_Face.hxx>
#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
//=======================================================================
//function : SMModel
//purpose  :
//=======================================================================
static int SMModel(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    TopoDS_Shape aShape = DBRep::Get(theArgVec[2]);
    if (aShape.IsNull()) {
        theDI << "Syntax error: '" << theArgVec[2] << "' is not a shape";
        return 1;
    }

    Handle(SimMesh_Model) aModel = SimMesh::ModelFromShape(aShape);
    if (aModel.IsNull()) {
        theDI << "Error: empty shape";
        return 1;
    }
    Handle(SimMeshDraw_DrawableMesh) aDM = new SimMeshDraw_DrawableMesh(aModel);
    Draw::Set(theArgVec[1], aDM);

    return 0;
}

//=======================================================================
//function : SMSave
//purpose  :
//=======================================================================
static int SMSave(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Syntax error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    if (!SimMesh_Persistence::Save(aModel, theArgVec[2])) {
        theDI << "Error: failure saving model to the file " << theArgVec[2];
        return 1;
    }
    return 0;
}

//=======================================================================
//function : SMLoad
//purpose  :
//=======================================================================
static int SMLoad(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    Handle(SimMesh_Model) aModel = SimMesh_Persistence::Load(theArgVec[1]);
    if (aModel.IsNull()) {
        theDI << "Error: cannot load model from file " << theArgVec[1];
        return 1;
    }

    Handle(SimMeshDraw_DrawableMesh) aDM = new SimMeshDraw_DrawableMesh(aModel);
    Draw::Set(theArgVec[2], aDM);
    return 0;
}

//=======================================================================
//function : SMToShape
//purpose  :
//=======================================================================
static int SMToShape(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[2]));
    if (aDM.IsNull()) {
        theDI << "Syntax error: '" << theArgVec[2] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    TopoDS_Shape aShape = SimMesh::ShapeFromModel(aModel);
    if (aShape.IsNull()) {
        theDI << "Error: empty model";
        return 1;
    }
    DBRep::Set(theArgVec[1], aShape);

    return 0;
}

// ==============================================================================
// function : GetSimplifier
// purpose  :
// ==============================================================================
static SimMesh_Simplifier& GetSimplifier()
{
    static SimMesh_Simplifier aTool;
    return aTool;
}

// ==============================================================================
// function : SMPerform
// purpose  :
// ==============================================================================
static int SMPerform(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    Handle(SimMeshDraw_DrawableMesh) aDM;

    double aTargetSizeRatio = 0.;
    double anAllowedError   = 0.;
    int    aNbIterLimit     = 0;
    bool   isAggregation    = true;
    bool   isResume         = false;
    bool   isAllowInverse   = true;

    for (int anArgIter = 1; anArgIter < theArgNb; ++anArgIter) {
        TCollection_AsciiString anArgCase(theArgVec[anArgIter]);
        anArgCase.LowerCase();
        if (anArgIter + 1 < theArgNb && (anArgCase == "-targetratio" || anArgCase == "-ratio")) {
            aTargetSizeRatio = Draw::Atof(theArgVec[++anArgIter]);
        } else if (anArgIter + 1 < theArgNb && (anArgCase == "-maxerror" || anArgCase == "-error")) {
            anAllowedError = Draw::Atof(theArgVec[++anArgIter]);
        } else if (anArgIter + 1 < theArgNb && (anArgCase == "-iternumlimit" || anArgCase == "-limit")) {
            aNbIterLimit = Draw::Atoi(theArgVec[++anArgIter]);
        } else if (anArgCase == "-resume") {
            isResume = true;
        } else if (anArgCase == "-aggregation" || anArgCase == "-aggr") {
            isAggregation = Draw::ParseOnOffIterator(theArgNb, theArgVec, anArgIter);
        } else if (anArgCase == "-allowinverse" || anArgCase == "-inverse") {
            isAllowInverse = Draw::ParseOnOffIterator(theArgNb, theArgVec, anArgIter);
        } else if (aDM.IsNull()) {
            aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[anArgIter]));
            if (aDM.IsNull()) {
                theDI << "Syntax error: '" << theArgVec[anArgIter] << "' is not a mesh model";
                return 1;
            }
        } else {
            theDI << "Syntax error: unknown argument '" << theArgVec[anArgIter] << "'";
            return 1;
        }
    }
    if (isResume && !aDM.IsNull()) {
        theDI << "Syntax error: do not pass a model in resume mode";
        return 1;
    }
    if (!isResume && aDM.IsNull()) {
        theDI << "Syntax error: model is not given";
        return 1;
    }

    SimMesh_Simplifier& aTool = GetSimplifier();
    aTool.SetNbIterLimit(aNbIterLimit);
    bool isOK = false;
    if (isResume) {
        isOK = aTool.ResumePerform();
    } else {
        Handle(SimMesh_Model) aModel = aDM->GetModel();
        aTool.Initialize(aModel);
        aTool.SetAggregationMode(isAggregation);
        aTool.SetAllowInversion(isAllowInverse);
        aTool.SetTargetRatio(aTargetSizeRatio);
        aTool.SetAllowedError(anAllowedError);
        isOK = aTool.Perform();
    }
    if (isOK) {
        theDI << "Maximal_error " << aTool.GetMaxError();
        if (aTool.NbInversions() > 0) {
            theDI << "\nWarning: " << aTool.NbInversions() << " inversions occurred";
        }
        if (aNbIterLimit > 0 && aTool.IsTargetGot()) {
            theDI << "\nSimplification target has been got";
        }
    } else {
        theDI << "Error: Simplification failed";
        return 1;
    }

    return 0;
}

//=======================================================================
//function : SMDisplay
//purpose  :
//=======================================================================
static int SMDisplay(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 2) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // make sure context is ready
    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull()) {
        ViewerTest::ViewerInit();
        aCtx = ViewerTest::GetAISContext();
        if (aCtx.IsNull()) {
            theDI << "Error: AIS context is not found! Run the command 'vinit'";
            return 1;
        }
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    // remove an old interactive object if any
    TCollection_AsciiString aName(theArgVec[1]);
    if (GetMapOfAIS().IsBound2(aName)) {
        Handle(AIS_InteractiveObject) anObj = GetMapOfAIS().Find2(aName);
        aCtx->Remove(anObj, false);
        GetMapOfAIS().UnBind2(aName);
    }

    // create MeshVS mesh
    Handle(SimMeshDraw_DataSource) aDS    = new SimMeshDraw_DataSource(aModel);
    Handle(MeshVS_Mesh)            aVMesh = new MeshVS_Mesh();
    aVMesh->SetDataSource(aDS);
    aVMesh->AddBuilder(new MeshVS_MeshPrsBuilder(aVMesh), true);
    Quantity_Color aIntColor     = Quantity_NOC_LIGHTYELLOW;
    Quantity_Color aBackIntColor = Quantity_NOC_YELLOW2;
    Quantity_Color aEdgeColor    = Quantity_NOC_GRAY20;
    Quantity_Color aBeamColor    = Quantity_NOC_GREEN;
    aVMesh->GetDrawer()->SetInteger(MeshVS_DA_MaxFaceNodes, 3);
    aVMesh->GetDrawer()->SetMaterial(MeshVS_DA_FrontMaterial,
                                     Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial_Metalized));
    aVMesh->GetDrawer()->SetMaterial(MeshVS_DA_BackMaterial,
                                     Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial_Metalized));
    aVMesh->GetDrawer()->SetColor(MeshVS_DA_InteriorColor, aIntColor);
    aVMesh->GetDrawer()->SetColor(MeshVS_DA_BackInteriorColor, aBackIntColor);
    aVMesh->GetDrawer()->SetBoolean(MeshVS_DA_ShowEdges, true);
    aVMesh->GetDrawer()->SetBoolean(MeshVS_DA_DisplayNodes, false);
    aVMesh->GetDrawer()->SetBoolean(MeshVS_DA_SmoothShading, true);
    aVMesh->GetDrawer()->SetColor(MeshVS_DA_BeamColor, aBeamColor);
    aVMesh->GetDrawer()->SetColor(MeshVS_DA_EdgeColor, aEdgeColor);
    aVMesh->GetDrawer()->SetDouble(MeshVS_DA_EdgeWidth, 0.5);
    aVMesh->SetDisplayMode(MeshVS_DMF_Shading);
    aVMesh->SetMeshSelMethod(MeshVS_MSM_PRECISE);

    // display mesh
    aCtx->Display(aVMesh, MeshVS_DMF_Shading, MeshVS_SMF_Face, true);
    GetMapOfAIS().Bind(aVMesh, aName);
    return 0;
}

//=======================================================================
//function : SMShowEdges
//purpose  :
//=======================================================================

static int SMShowEdges(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull()) {
        theDI << "Error: no interactive context";
        return 1;
    }

    TCollection_AsciiString       aName(theArgVec[1]);
    Handle(AIS_InteractiveObject) anObj;
    GetMapOfAIS().Find2(aName, anObj);
    Handle(MeshVS_Mesh) aVMesh = Handle(MeshVS_Mesh)::DownCast(anObj);
    if (aVMesh.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not an interactive mesh";
        return 1;
    }

    bool aFlag = Draw::Atoi(theArgVec[2]) != 0;
    aVMesh->GetDrawer()->SetBoolean(MeshVS_DA_ShowEdges, aFlag);
    aCtx->Redisplay(aVMesh, true);
    return 0;
}

//=======================================================================
//function : SMShowNodes
//purpose  :
//=======================================================================

static int SMShowNodes(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull()) {
        theDI << "Error: no interactive context";
        return 1;
    }

    TCollection_AsciiString       aName(theArgVec[1]);
    Handle(AIS_InteractiveObject) anObj;
    GetMapOfAIS().Find2(aName, anObj);
    Handle(MeshVS_Mesh) aVMesh = Handle(MeshVS_Mesh)::DownCast(anObj);
    if (aVMesh.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not an interactive mesh";
        return 1;
    }

    bool aFlag = Draw::Atoi(theArgVec[2]) != 0;
    aVMesh->GetDrawer()->SetBoolean(MeshVS_DA_DisplayNodes, aFlag);
    aCtx->Redisplay(aVMesh, true);
    return 0;
}

//=======================================================================
//function : SMListSel
//purpose  :
//=======================================================================
static int SMListSel(Draw_Interpretor& theDI, int /*theArgNb*/, const char** /*theArgVec*/)
{
    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();
    if (aCtx.IsNull()) {
        theDI << "Error: no interactive context";
        return 1;
    }

    for (aCtx->InitSelected(); aCtx->MoreSelected(); aCtx->NextSelected()) {
        Handle(MeshVS_MeshEntityOwner) anOwner = Handle(MeshVS_MeshEntityOwner)::DownCast(aCtx->SelectedOwner());
        if (!anOwner.IsNull()) {
            SimMesh_Model* aModel = (SimMesh_Model*)anOwner->Owner();

            int anID = anOwner->Type() == MeshVS_ET_Face ? anOwner->ID()
                                                         : anOwner->ID() - int(aModel->GetNbTriangles());
            theDI << anID << (anOwner->Type() == MeshVS_ET_Link ? "s" : "") << " ";
        }
    }
    return 0;
}

//=======================================================================
//function : SMTriDump
//purpose  :
//=======================================================================
static int SMTriDump(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    const int        anId   = Draw::Atoi(theArgVec[2]);
    const SimMesh_Id aTriId = anId;

    if (aTriId >= aModel->GetNbTriangles()) {
        theDI << "Error: ID is out of valid range";
        return 1;
    }

    const SimMesh_Triangle& aTria = aModel->GetTriangle(aTriId);
    theDI << "Triangle " << anId << " is " << (aModel->IsTriangleValid(aTriId) ? "valid" : "invalid") << "\n";
    theDI << "Vertices: " << (int)aTria[0] << " " << (int)aTria[1] << " " << (int)aTria[2] << "\n";
    if (aModel->IsTriangleColors()) {
        const SimMesh_Color& aColor = aModel->GetTriangleColor(aTriId);
        theDI << "Color: " << aColor.R() << " " << aColor.G() << " " << aColor.B() << "\n";
    }
    return 0;
}

//=======================================================================
//function : SMEdgeDump
//purpose  :
//=======================================================================
static int SMEdgeDump(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    const int        anId     = Draw::Atoi(theArgVec[2]);
    const SimMesh_Id anEdgeId = anId;

    if (anEdgeId >= aModel->GetNbEdges()) {
        theDI << "Error: ID is out of valid range";
        return 1;
    }

    const SimMesh_Edge& anEdge = aModel->GetEdge(anEdgeId);
    theDI << "Edge " << anId << " is " << (aModel->IsEdgeValid(anEdgeId) ? "valid" : "invalid") << "\n";
    theDI << "Vertices: " << (int)anEdge[0] << " " << (int)anEdge[1] << "\n";
    return 0;
}

//=======================================================================
//function : SMVertexDump
//purpose  :
//=======================================================================
static int SMVertexDump(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 3) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    int              anId    = Draw::Atoi(theArgVec[2]);
    const SimMesh_Id aVertId = anId;

    if (aVertId >= aModel->GetNbVertices()) {
        theDI << "Error: ID is out of valid range";
        return 1;
    }

    const SimMesh_Vertex&         aVert      = aModel->GetVertex(aVertId);
    const SimMesh_VertexDataList& aVDataList = aModel->GetVertexDataList(aVertId);
    theDI << "Vertex " << anId << " is " << (aModel->IsVertexValid(aVertId) ? "valid" : "invalid") << "\n";
    theDI << "Position: " << aVert[0] << " " << aVert[1] << " " << aVert[2] << "\n";
    if (aModel->IsVertexValid(aVertId)) {
        theDI << "Data list:";
        for (size_t i = 0; i < aVDataList.size(); ++i) {
            const SimMesh_VertexData& aVData = aVDataList[i];
            theDI << "\n" << (int)i << ":";
            if (!aVData.Triangles.empty()) {
                theDI << "\n  Triangles: ";
                for (SimMesh_Id aTriaId : aVData.Triangles) {
                    theDI << (int)aTriaId << " ";
                }
            }
            if (!aVData.Edges.empty()) {
                theDI << "\n  Edges: ";
                for (SimMesh_Id anEdgeId : aVData.Edges) {
                    theDI << (int)anEdgeId << " ";
                }
            }
            if (aVData.Attrib.IsNormal()) {
                const SimMesh_Vec3f& aN = aVData.Attrib.GetNormal();
                theDI << "\n  Normal: " << aN[0] << " " << aN[1] << " " << aN[2];
            }
            if (aVData.Attrib.IsColor()) {
                const SimMesh_Color& aColor = aVData.Attrib.GetColor();
                theDI << "\n  Color: " << aColor.R() << " " << aColor.G() << " " << aColor.B();
            }
            if (aVData.Attrib.IsTexCoord()) {
                const SimMesh_Vec2f& aT = aVData.Attrib.GetTexCoord();
                theDI << "\n  Texture coord: " << aT.x() << " " << aT.y();
            }
        }
    }
    return 0;
}

//=======================================================================
//function : SMCheckModel
//purpose  :
//=======================================================================
static int SMCheckModel(Draw_Interpretor& theDI, int theArgNb, const char** theArgVec)
{
    if (theArgNb != 2) {
        theDI.PrintHelp(theArgVec[0]);
        return 1;
    }

    // get mesh model
    Handle(SimMeshDraw_DrawableMesh) aDM = Handle(SimMeshDraw_DrawableMesh)::DownCast(Draw::Get(theArgVec[1]));
    if (aDM.IsNull()) {
        theDI << "Error: '" << theArgVec[1] << "' is not a mesh model";
        return 1;
    }
    Handle(SimMesh_Model) aModel = aDM->GetModel();

    Standard_SStream aSStr;
    if (!aModel->IsModelConsistent(&aSStr)) {
        theDI << aSStr.str().c_str() << "\n";
        theDI << "Error: there are inconsistencies";
    }
    return 0;
}

// =======================================================================
// function : InitCommands
// purpose  :
// =======================================================================
void SimMeshDraw::InitCommands(Draw_Interpretor& theCommands)
{
    const char* aGroup = "SimMesh";
    theCommands.Add("SMperform",
                    "SMperform model [-targetRatio Value] [-maxerror Value] [-aggregation [0|1]=1]"
                    "\n                 [-allowInverse [0|1]=1] [-iterNumLimit Num]"
                    "\nor"
                    "\nSMperform [-resume] [-iterNumLimit Num]"
                    "\n\t\t: Perform mesh simplification algorithm. By default no stop criteria are given."
                    "\n\t\t: You need to give target ratio and/or max error."
                    "\n\t\t: By default aggregation mode is 1, allowInverse is 1."
                    "\n\t\t: AllowInverse takes effect only in aggregation mode",
                    __FILE__,
                    SMPerform,
                    aGroup);
    theCommands.Add("SMmodel",
                    "SMmodel new_model shape"
                    "\n\t\t: Build mesh simplification model from shape.",
                    __FILE__,
                    SMModel,
                    aGroup);
    theCommands.Add("SMsave",
                    "SMsave model filename"
                    "\n\t\t: Save mesh simplification model to a file.",
                    __FILE__,
                    SMSave,
                    aGroup);
    theCommands.Add("SMLoad",
                    "SMLoad filename model"
                    "\n\t\t: Load mesh simplification model from a file.",
                    __FILE__,
                    SMLoad,
                    aGroup);
    theCommands.Add("SMtoshape",
                    "SMtoshape new_shape model"
                    "\n\t\t: Create a one-face shape from mesh simplification model.",
                    __FILE__,
                    SMToShape,
                    aGroup);
    theCommands.Add("SMdisplay",
                    "SMdisplay model"
                    "\n\t\t: Display mesh simplification model.",
                    __FILE__,
                    SMDisplay,
                    aGroup);
    theCommands.Add("SMshowedges",
                    "SMshowedges model 0/1"
                    "\n\t\t: Show/hide mesh model edges.",
                    __FILE__,
                    SMShowEdges,
                    aGroup);
    theCommands.Add("SMshownodes",
                    "SMshownodes model 0/1"
                    "\n\t\t: Show/hide mesh model nodes.",
                    __FILE__,
                    SMShowNodes,
                    aGroup);
    theCommands.Add("SMlistsel",
                    "SMlistsel"
                    "\n\t\t: Print selected mesh triangles IDs.",
                    __FILE__,
                    SMListSel,
                    aGroup);
    theCommands.Add("SMtridump",
                    "SMtridump model tri_id"
                    "\n\t\t: Dump the given mesh triangle.",
                    __FILE__,
                    SMTriDump,
                    aGroup);
    theCommands.Add("SMedgedump",
                    "SMedgedump model edge_id"
                    "\n\t\t: Dump the given mesh edge.",
                    __FILE__,
                    SMEdgeDump,
                    aGroup);
    theCommands.Add("SMvertexdump",
                    "SMvertexdump model vertex_id"
                    "\n\t\t: Dump the given mesh vertex.",
                    __FILE__,
                    SMVertexDump,
                    aGroup);
    theCommands.Add("SMcheckmodel",
                    "SMcheckmodel model"
                    "\n\t\t: Check the given model consistency.",
                    __FILE__,
                    SMCheckModel,
                    aGroup);
}

// =======================================================================
// function : Factory
// purpose  :
// =======================================================================
void SimMeshDraw::Factory(Draw_Interpretor& theDI)
{
    SimMeshDraw::InitCommands(theDI);
}

// Declare entry point PLUGINFACTORY
DPLUGIN(SimMeshDraw)
