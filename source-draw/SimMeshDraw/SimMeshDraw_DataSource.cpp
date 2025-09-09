#include <SimMeshDraw_DataSource.h>
#include <SimMesh_Model.h>

//================================================================
// Function : Constructor
// Purpose  :
//================================================================
SimMeshDraw_DataSource::SimMeshDraw_DataSource(const Handle(SimMesh_Model)& theModel) : myModel(theModel)
{
    if (!myModel.IsNull()) {
        for (SimMesh_Id i = 0; i < myModel->GetNbVertices(); i++) {
            if (myModel->IsVertexValid(i)) {
                myNodes.Add(static_cast<int>(i));
            }
        }

        for (SimMesh_Id i = 0; i < myModel->GetNbTriangles(); i++) {
            if (myModel->IsTriangleValid(i)) {
                myTriangles.Add(static_cast<int>(i));
            }
        }

        for (SimMesh_Id i = 0; i < myModel->GetNbEdges(); i++) {
            if (myModel->IsEdgeValid(i)) {
                myTriangles.Add(static_cast<int>(myModel->GetNbTriangles() + i));
            }
        }
    }
}

//================================================================
// Function : GetGeom
// Purpose  :
//================================================================
bool SimMeshDraw_DataSource::GetGeom(const int             theID,
                                     const bool            theIsElement,
                                     TColStd_Array1OfReal& theCoords,
                                     int&                  theNbNodes,
                                     MeshVS_EntityType&    theType) const
{
    if (myModel.IsNull()) {
        return false;
    }

    if (theIsElement) {
        int aNbTri      = (int)myModel->GetNbTriangles();
        int aNbEdges    = (int)myModel->GetNbEdges();
        theType         = theID < aNbTri ? MeshVS_ET_Face : MeshVS_ET_Link;
        SimMesh_Id anID = theType == MeshVS_ET_Face ? theID : theID - aNbTri;
        if (theID < 0 || theID >= aNbTri + aNbEdges || (theType == MeshVS_ET_Face && !myModel->IsTriangleValid(anID))
            || (theType == MeshVS_ET_Link && !myModel->IsEdgeValid(anID))) {
            return false;
        }
        if (theType == MeshVS_ET_Face) {
            theNbNodes                    = 3;
            const SimMesh_Triangle& aTria = myModel->GetTriangle(anID);
            for (int i = 0, k = 1; i < 3; i++) {
                const SimMesh_Vertex& aVert = myModel->GetVertex(aTria[i]);
                for (int j = 0; j < 3; j++, k++) {
                    theCoords(k) = aVert[j];
                }
            }
        } else {
            theNbNodes                 = 2;
            const SimMesh_Edge& anEdge = myModel->GetEdge(anID);
            for (int i = 0, k = 1; i < 2; i++) {
                const SimMesh_Vertex& aVert = myModel->GetVertex(anEdge[i]);
                for (int j = 0; j < 3; j++, k++) {
                    theCoords(k) = aVert[j];
                }
            }
        }
    } else {
        SimMesh_Id anId = theID;
        if (anId >= myModel->GetNbVertices() || !myModel->IsVertexValid(anId)) {
            return false;
        }
        theType    = MeshVS_ET_Node;
        theNbNodes = 1;

        const SimMesh_Vertex& aVert = myModel->GetVertex(anId);
        theCoords(1)                = aVert[0];
        theCoords(2)                = aVert[1];
        theCoords(3)                = aVert[2];
    }
    return true;
}

//================================================================
// Function : GetGeomType
// Purpose  :
//================================================================
bool SimMeshDraw_DataSource::GetGeomType(const int theID, const bool theIsElement, MeshVS_EntityType& theType) const
{
    if (theIsElement) {
        int aNbTri      = (int)myModel->GetNbTriangles();
        int aNbEdges    = (int)myModel->GetNbEdges();
        theType         = theID < aNbTri ? MeshVS_ET_Face : MeshVS_ET_Link;
        SimMesh_Id anID = theType == MeshVS_ET_Face ? theID : theID - aNbTri;
        return theID >= 0 && theID < aNbTri + aNbEdges
            && ((theType == MeshVS_ET_Face && myModel->IsTriangleValid(anID))
                || (theType == MeshVS_ET_Link && myModel->IsEdgeValid(anID)));
    } else {
        theType = MeshVS_ET_Node;
        return true;
    }
}

//================================================================
// Function : GetAddr
// Purpose  :
//================================================================
Standard_Address SimMeshDraw_DataSource::GetAddr(const int /*theID*/, const bool /*theIsElement*/) const
{
    return myModel.get();
}

//================================================================
// Function : GetNodesByElement
// Purpose  :
//================================================================
bool SimMeshDraw_DataSource::GetNodesByElement(const int                theID,
                                               TColStd_Array1OfInteger& theNodeIDs,
                                               int&                     theNbNodes) const
{
    if (myModel.IsNull()) {
        return false;
    }

    int               aNbTri   = (int)myModel->GetNbTriangles();
    int               aNbEdges = (int)myModel->GetNbEdges();
    MeshVS_EntityType aType    = theID < aNbTri ? MeshVS_ET_Face : MeshVS_ET_Link;
    SimMesh_Id        anID     = aType == MeshVS_ET_Face ? theID : theID - aNbTri;
    if (theID < 0 || theID >= aNbTri + aNbEdges || (aType == MeshVS_ET_Face && !myModel->IsTriangleValid(anID))
        || (aType == MeshVS_ET_Link && !myModel->IsEdgeValid(anID))) {
        return false;
    }

    if (aType == MeshVS_ET_Face) {
        theNbNodes = 3;
        if (theNodeIDs.Length() >= 3) {
            const SimMesh_Triangle& aTria = myModel->GetTriangle(anID);
            int                     aLow  = theNodeIDs.Lower();
            theNodeIDs[aLow]              = static_cast<int>(aTria[0]);
            theNodeIDs[aLow + 1]          = static_cast<int>(aTria[1]);
            theNodeIDs[aLow + 2]          = static_cast<int>(aTria[2]);
            return true;
        }
    } else {
        theNbNodes = 2;
        if (theNodeIDs.Length() >= 2) {
            const SimMesh_Edge& anEdge = myModel->GetEdge(anID);
            int                 aLow   = theNodeIDs.Lower();
            theNodeIDs[aLow]           = static_cast<int>(anEdge[0]);
            theNodeIDs[aLow + 1]       = static_cast<int>(anEdge[1]);
            return true;
        }
    }
    return false;
}

//================================================================
// Function : GetAllNodes
// Purpose  :
//================================================================
const TColStd_PackedMapOfInteger& SimMeshDraw_DataSource::GetAllNodes() const
{
    return myNodes;
}

//================================================================
// Function : GetAllElements
// Purpose  :
//================================================================
const TColStd_PackedMapOfInteger& SimMeshDraw_DataSource::GetAllElements() const
{
    return myTriangles;
}

//================================================================
// Function : GetNormal
// Purpose  :
//================================================================
bool SimMeshDraw_DataSource::GetNormal(const int theId,
                                       const int theMax,
                                       double&   theNx,
                                       double&   theNy,
                                       double&   theNz) const
{
    if (myModel.IsNull()) {
        return false;
    }

    SimMesh_Id anID = theId;
    if (anID >= myModel->GetNbTriangles() || theMax < 3 || !myModel->IsTriangleValid(anID)) {
        return false;
    }

    const SimMesh_Triangle& aTria  = myModel->GetTriangle(anID);
    const SimMesh_Vertex&   aVert0 = myModel->GetVertex(aTria[0]);
    const SimMesh_Vertex&   aVert1 = myModel->GetVertex(aTria[1]);
    const SimMesh_Vertex&   aVert2 = myModel->GetVertex(aTria[2]);
    SimMesh_Vec3f           aN     = SimMesh_Vec3f::Cross(aVert1 - aVert0, aVert2 - aVert0);
    aN.Normalize();
    theNx = aN[0];
    theNy = aN[1];
    theNz = aN[2];
    return true;
}

//================================================================
// Function : GetNodeNormal
// Purpose  :
//================================================================
bool SimMeshDraw_DataSource::GetNodeNormal(const int theRanknode,
                                           const int theElementId,
                                           double&   theNx,
                                           double&   theNy,
                                           double&   theNz) const
{
    if (myModel.IsNull() || !myModel->IsVertexNormals()) {
        return false;
    }

    SimMesh_Id anID = theElementId;
    if (anID >= myModel->GetNbTriangles() || !myModel->IsTriangleValid(anID) || theRanknode < 1 || theRanknode > 3) {
        return false;
    }

    const SimMesh_Triangle&       aTria      = myModel->GetTriangle(anID);
    const SimMesh_VertexDataList& aVDataList = myModel->GetVertexDataList(aTria[theRanknode - 1]);
    auto                          aVIter     = aVDataList.FindByTriangleId(anID);
    if (aVIter == aVDataList.end()) {
        return false;
    }
    const SimMesh_Vec3f& aN = aVIter->Attrib.GetNormal();
    theNx                   = aN[0];
    theNy                   = aN[1];
    theNz                   = aN[2];
    return true;
}
