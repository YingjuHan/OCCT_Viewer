
#include <Draw_Color.hxx>
#include <Draw_Display.hxx>
#include <SimMeshDraw_DrawableMesh.h>
#include <SimMesh_Model.h>
#include <TColStd_PackedMapOfInteger.hxx>

const Draw_ColorKind TriangleColor = Draw_saumon;
const Draw_ColorKind EdgeColor     = Draw_vert;
const Draw_ColorKind NodeColor     = Draw_jaune;

//================================================================
// Function : SimMeshDraw_DrawableMesh
// Purpose  :
//================================================================
SimMeshDraw_DrawableMesh::SimMeshDraw_DrawableMesh(const Handle(SimMesh_Model)& theModel) : myModel(theModel)
{
}

//================================================================
// Function : DrawOn
// Purpose  :
//================================================================
void SimMeshDraw_DrawableMesh::DrawOn(Draw_Display& theDisp) const
{
    TColStd_PackedMapOfInteger anElNodes;

    // Triangles
    theDisp.SetColor(TriangleColor);
    for (SimMesh_Id aTriaId = 0; aTriaId < myModel->GetNbTriangles(); ++aTriaId) {
        if (myModel->IsTriangleValid(aTriaId)) {
            const SimMesh_Triangle& aTria = myModel->GetTriangle(aTriaId);
            anElNodes.Add(static_cast<int>(aTria[0]));
            const SimMesh_Vertex& aVert1 = myModel->GetVertex(aTria[0]);
            gp_Pnt                aPnt1(aVert1.x(), aVert1.y(), aVert1.z());
            theDisp.MoveTo(aPnt1);
            for (int i = 1; i < 3; ++i) {
                anElNodes.Add(static_cast<int>(aTria[i]));
                const SimMesh_Vertex& aVert2 = myModel->GetVertex(aTria[i]);
                gp_Pnt                aPnt2(aVert2.x(), aVert2.y(), aVert2.z());
                theDisp.DrawTo(aPnt2);
                theDisp.MoveTo(aPnt2);
            }
            theDisp.DrawTo(aPnt1);
            theDisp.MoveTo(aPnt1);
        }
    }

    // Edges
    theDisp.SetColor(EdgeColor);
    for (SimMesh_Id anEdgeId = 0; anEdgeId < myModel->GetNbEdges(); ++anEdgeId) {
        if (myModel->IsEdgeValid(anEdgeId)) {
            const SimMesh_Edge& anEdge = myModel->GetEdge(anEdgeId);
            anElNodes.Add(static_cast<int>(anEdge[0]));
            const SimMesh_Vertex& aVert1 = myModel->GetVertex(anEdge[0]);
            gp_Pnt                aPnt1(aVert1.x(), aVert1.y(), aVert1.z());
            theDisp.MoveTo(aPnt1);
            anElNodes.Add(static_cast<int>(anEdge[1]));
            const SimMesh_Vertex& aVert2 = myModel->GetVertex(anEdge[1]);
            gp_Pnt                aPnt2(aVert2.x(), aVert2.y(), aVert2.z());
            theDisp.DrawTo(aPnt2);
        }
    }

    // Free vertices
    for (SimMesh_Id aVertId = 0; aVertId < myModel->GetNbVertices(); ++aVertId) {
        if (myModel->IsVertexValid(aVertId)) {
            if (!anElNodes.Contains(static_cast<int>(aVertId))) {
                theDisp.SetColor(NodeColor);
                const SimMesh_Vertex& aVert = myModel->GetVertex(aVertId);
                gp_Pnt                aPnt(aVert.x(), aVert.y(), aVert.z());
                theDisp.MoveTo(aPnt);
                theDisp.DrawMarker(aPnt, Draw_Circle, 2);
            }
        }
    }
}

//================================================================
// Function : Whatis
// Purpose  :
//================================================================
void SimMeshDraw_DrawableMesh::Whatis(Draw_Interpretor& theI) const
{
    theI << "SimMesh_Model";
}

//================================================================
// Function : Dump
// Purpose  :
//================================================================
void SimMeshDraw_DrawableMesh::Dump(Standard_OStream& theS) const
{
    gp_XYZ aPMin   = myModel->GetBndBox().CornerMin();
    gp_XYZ aPMax   = myModel->GetBndBox().CornerMax();
    int    nbVert  = 0;
    int    nbTria  = 0;
    int    nbEdges = 0;
    for (SimMesh_Id aVertId = 0; aVertId < myModel->GetNbVertices(); ++aVertId) {
        if (myModel->IsVertexValid(aVertId)) {
            ++nbVert;
        }
    }
    for (SimMesh_Id aTriaId = 0; aTriaId < myModel->GetNbTriangles(); ++aTriaId) {
        if (myModel->IsTriangleValid(aTriaId)) {
            ++nbTria;
        }
    }
    for (SimMesh_Id anEdgeId = 0; anEdgeId < myModel->GetNbEdges(); ++anEdgeId) {
        if (myModel->IsEdgeValid(anEdgeId)) {
            ++nbEdges;
        }
    }
    theS << "Nb vertices: " << nbVert << " (total " << myModel->GetNbVertices() << ")" << std::endl
         << "Nb triangles: " << nbTria << " (total " << myModel->GetNbTriangles() << ")" << std::endl
         << "Nb edges: " << nbEdges << " (total " << myModel->GetNbEdges() << ")" << std::endl
         << "Has vertex normals: " << (myModel->IsVertexNormals() ? "true" : "false") << std::endl
         << "Has vertex colors: " << (myModel->IsVertexColors() ? "true" : "false") << std::endl
         << "Has vertex texture coords: " << (myModel->IsVertexTextureCoords() ? "true" : "false") << std::endl
         << "Has triangle colors: " << (myModel->IsTriangleColors() ? "true" : "false") << std::endl
         << "Bnd box: " << aPMin.X() << " " << aPMin.Y() << " " << aPMin.Z() << " " << aPMax.X() << " " << aPMax.Y()
         << " " << aPMax.Z();
}

//================================================================
// Function : Copy
// Purpose  :
//================================================================
Handle(Draw_Drawable3D) SimMeshDraw_DrawableMesh::Copy() const
{
    return new SimMeshDraw_DrawableMesh(myModel);
}
