#include <SimMesh.h>
#include <SimMesh_Model.h>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <functional>
#include <map>
#include <set>
#include <vector>

namespace {

//! Structure for storing mesh entities' numbers.
struct Numbers {
    int  nbNodes      = 0;
    int  nbTris       = 0;
    int  nbEdges      = 0;
    bool hasNormals   = true;
    bool hasTexCoords = true;
};

//! Structure to store group IDs.
struct GroupIds {
    int triGroupId  = 0;
    int edgeGroupId = 0;
};

//! Structure to hold vertex index and face;
struct IndFace {
    int         vertInd = -1;
    TopoDS_Face face;
};

//! Structure to hold min and max values;
struct IntRange {
    int min = INT_MAX;
    int max = INT_MIN;

    void Update(const int theVal)
    {
        min = std::min(min, theVal);
        max = std::max(max, theVal);
    }
};

//! Organizes the given mesh edges into a set of connected chains.
static std::vector<SimMesh_IdList> CreatePolylines(const Handle(SimMesh_Model)& theModel,
                                                   const SimMesh_IdList&        theEdges)
{
    // Create the connectivity map (vertex ID -> list of edge ID)
    // and create the set of all edges.
    std::map<SimMesh_Id, SimMesh_IdList> aMapVS;
    std::set<SimMesh_Id>                 anEdgesSet;
    for (SimMesh_Id anEdgeId : theEdges) {
        const SimMesh_Edge& anEdge = theModel->GetEdge(anEdgeId);
        aMapVS[anEdge[0]].push_back(anEdgeId);
        aMapVS[anEdge[1]].push_back(anEdgeId);
        anEdgesSet.insert(anEdgeId);
    }

    // While a set is not empty get the first edge and build a chain
    // in both directions till the end or multi-connection.
    std::vector<SimMesh_IdList> aLines;
    while (!anEdgesSet.empty()) {
        SimMesh_Id aStartEdgeId = *anEdgesSet.begin();
        aLines.emplace_back();
        SimMesh_IdList& aLastLine = aLines.back();

        // go in back direction to find a start of chain
        SimMesh_Id aCurrEdgeId = aStartEdgeId;
        for (;;) {
            const SimMesh_Edge&   aCurrEdge  = theModel->GetEdge(aCurrEdgeId);
            const SimMesh_IdList& aVertEdges = aMapVS[aCurrEdge[0]];
            if (aVertEdges.size() != 2) {
                break;
            }
            aCurrEdgeId = aVertEdges[0] == aCurrEdgeId ? aVertEdges[1] : aVertEdges[0];
            if (aCurrEdgeId == aStartEdgeId) {
                break;
            }
        }

        // go forward and create a new chain
        aStartEdgeId = aCurrEdgeId;
        for (;;) {
            anEdgesSet.erase(aCurrEdgeId);
            const SimMesh_Edge& aCurrEdge   = theModel->GetEdge(aCurrEdgeId);
            SimMesh_IdList&     aVertEdges0 = aMapVS[aCurrEdge[0]];
            aVertEdges0.RemoveItem(aCurrEdgeId);
            aLastLine.push_back(aCurrEdge[0]);

            SimMesh_IdList& aVertEdges1 = aMapVS[aCurrEdge[1]];
            SimMesh_Id      aNextEdgeId = aVertEdges1.size() != 2       ? SimMesh_InvalidId
                                        : aVertEdges1[0] == aCurrEdgeId ? aVertEdges1[1]
                                                                        : aVertEdges1[0];
            if (aNextEdgeId == SimMesh_InvalidId || aNextEdgeId == aStartEdgeId) {
                aVertEdges1.RemoveItem(aCurrEdgeId);
                aLastLine.push_back(aCurrEdge[1]);
                break;
            }
            aCurrEdgeId = aNextEdgeId;
        }
    }
    return aLines;
}

//! Counts mesh entities' numbers in the shape.
static void CountMeshEntityNumbers(const TopoDS_Shape& theShape, Numbers& theNumbers)
{
    TopLoc_Location aFaceLoc;
    TopLoc_Location aEdgeLoc;
    for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next()) {
        const TopoDS_Face&                aFace    = TopoDS::Face(aFaceIter.Current());
        const Handle(Poly_Triangulation)& aPolyTri = BRep_Tool::Triangulation(aFace, aFaceLoc);
        if (!aPolyTri.IsNull()) {
            theNumbers.nbNodes += aPolyTri->NbNodes();
            theNumbers.nbTris += aPolyTri->NbTriangles();
            theNumbers.hasNormals   = theNumbers.hasNormals && aPolyTri->HasNormals();
            theNumbers.hasTexCoords = theNumbers.hasTexCoords && aPolyTri->HasUVNodes();
        }
        for (TopExp_Explorer aEdgeIter(aFace, TopAbs_EDGE); aEdgeIter.More(); aEdgeIter.Next()) {
            const TopoDS_Edge&                         aEdge    = TopoDS::Edge(aEdgeIter.Current());
            const Handle(Poly_PolygonOnTriangulation)& aPolygon = BRep_Tool::PolygonOnTriangulation(aEdge,
                                                                                                    aPolyTri,
                                                                                                    aFaceLoc);
            if (!aPolygon.IsNull()) {
                theNumbers.nbEdges += aPolygon->NbNodes() - 1;
            }
        }
    }
    for (TopExp_Explorer aEdgeIter(theShape, TopAbs_EDGE, TopAbs_FACE); aEdgeIter.More(); aEdgeIter.Next()) {
        const TopoDS_Edge&            aEdge    = TopoDS::Edge(aEdgeIter.Current());
        const Handle(Poly_Polygon3D)& aPolygon = BRep_Tool::Polygon3D(aEdge, aEdgeLoc);
        if (!aPolygon.IsNull()) {
            theNumbers.nbNodes += aPolygon->NbNodes();
            theNumbers.nbEdges += aPolygon->NbNodes() - 1;
        }
    }
    if (theNumbers.nbTris == 0) {
        theNumbers.hasNormals   = false;
        theNumbers.hasTexCoords = false;
    }
}

//! Adds vertices from a triangulation into mesh model.
static void AddVerticesToMeshModel(const Handle(Poly_Triangulation)& thePolyTri,
                                   const gp_Trsf&                    theTrsf,
                                   const bool                        theReversed,
                                   const Handle(SimMesh_Model)&      theModel,
                                   const Numbers&                    theNumbers)
{
    for (int aNodeIter = 1; aNodeIter <= thePolyTri->NbNodes(); ++aNodeIter) {
        const gp_Pnt aNode = thePolyTri->Node(aNodeIter).Transformed(theTrsf);
        SimMesh_Id aVertId = theModel->AddVertex(SimMesh_Vertex((float)aNode.X(), (float)aNode.Y(), (float)aNode.Z()));
        if (theNumbers.hasNormals && thePolyTri->HasNormals()) {
            gp_Dir aNorm = thePolyTri->Normal(aNodeIter).Transformed(theTrsf);
            if (theReversed) {
                aNorm.Reverse();
            }
            theModel->ChangeVertexAttrib(aVertId).ChangeNormal().SetValues((float)aNorm.X(),
                                                                           (float)aNorm.Y(),
                                                                           (float)aNorm.Z());
        }
        if (theNumbers.hasTexCoords && thePolyTri->HasUVNodes()) {
            gp_Pnt2d anUV = thePolyTri->UVNode(aNodeIter);
            theModel->ChangeVertexAttrib(aVertId).ChangeTexCoord().SetValues((float)anUV.X(), (float)anUV.Y());
        }
    }
}

//! Adds triangles from a triangulation into mesh model.
static void AddTrianglesToMeshModel(const Handle(Poly_Triangulation)& thePolyTri,
                                    const bool                        theReversed,
                                    const Handle(SimMesh_Model)&      theModel,
                                    const int                         theVertexOffset,
                                    const int                         theTriGroupId)
{
    for (int aTriaIter = 1; aTriaIter <= thePolyTri->NbTriangles(); ++aTriaIter) {
        const Poly_Triangle& aPolyTria = thePolyTri->Triangle(aTriaIter);
        SimMesh_Triangle     aTria(aPolyTria(1), aPolyTria(2), aPolyTria(3));
        aTria += SimMesh_Vec3i(theVertexOffset);
        if (theReversed) {
            std::swap(aTria[1], aTria[2]);
        }
        SimMesh_Id aTriaId = theModel->AddTriangle(aTria);
        theModel->SetTriangleGroup(aTriaId, theTriGroupId);
    }
}

//! Adds edges from a polygon on triangulation into mesh model.
static void AddEdgesToMeshModel(const Handle(Poly_PolygonOnTriangulation)& thePolygon,
                                const bool                                 theReversed,
                                const Handle(SimMesh_Model)&               theModel,
                                const int                                  theVertexOffset,
                                const int                                  theEdgeGroupId)
{
    for (int aNodeInd = 1; aNodeInd < thePolygon->NbNodes(); ++aNodeInd) {
        SimMesh_Edge aMeshEdge(thePolygon->Node(aNodeInd), thePolygon->Node(aNodeInd + 1));
        aMeshEdge += SimMesh_Vec2i(theVertexOffset);
        if (theReversed) {
            std::swap(aMeshEdge[0], aMeshEdge[1]);
        }
        SimMesh_Id aMeshEdgeId = theModel->AddEdge(aMeshEdge);
        theModel->SetEdgeGroup(aMeshEdgeId, theEdgeGroupId);
    }
}

//! Adds edges from a polygon into mesh model.
static void AddEdgesToMeshModel(const Handle(Poly_Polygon3D)& thePolygon,
                                const gp_Trsf&                theTrsf,
                                const bool                    theReversed,
                                const Handle(SimMesh_Model)&  theModel,
                                const int                     theEdgeGroupId)
{
    const int                 aVertexOffset = static_cast<int>(theModel->GetNbVertices() - 1);
    const TColgp_Array1OfPnt& aNodes        = thePolygon->Nodes();
    for (int aNodeInd = 1; aNodeInd <= thePolygon->NbNodes(); ++aNodeInd) {
        const gp_Pnt aNode = aNodes(aNodeInd).Transformed(theTrsf);
        theModel->AddVertex(SimMesh_Vertex((float)aNode.X(), (float)aNode.Y(), (float)aNode.Z()));
    }
    for (int aNodeInd = 1; aNodeInd < thePolygon->NbNodes(); ++aNodeInd) {
        SimMesh_Edge aMeshEdge(aNodeInd, aNodeInd + 1);
        aMeshEdge += SimMesh_Vec2i(aVertexOffset);
        if (theReversed) {
            std::swap(aMeshEdge[0], aMeshEdge[1]);
        }
        aMeshEdge.SetBeam(true);
        SimMesh_Id aMeshEdgeId = theModel->AddEdge(aMeshEdge);
        theModel->SetEdgeGroup(aMeshEdgeId, theEdgeGroupId);
    }
}

//! Adds mesh entities from a shape face into mesh model.
static void AddFaceToMeshModel(const TopoDS_Face&           theFace,
                               const Handle(SimMesh_Model)& theModel,
                               const Numbers&               theNumbers,
                               GroupIds&                    theGroupIds)
{
    TopLoc_Location                   aFaceLoc;
    const Handle(Poly_Triangulation)& aPolyTri = BRep_Tool::Triangulation(theFace, aFaceLoc);
    if (aPolyTri.IsNull()) {
        return;
    }

    const int  aVertexOffset  = static_cast<int>(theModel->GetNbVertices() - 1);
    const bool isFaceReversed = theFace.Orientation() == TopAbs_REVERSED;
    AddVerticesToMeshModel(aPolyTri, aFaceLoc.Transformation(), isFaceReversed, theModel, theNumbers);
    AddTrianglesToMeshModel(aPolyTri, isFaceReversed, theModel, aVertexOffset, theGroupIds.triGroupId);

    // Add edges from polygons on triangulation.
    for (TopExp_Explorer aEdgeIter(theFace.Oriented(TopAbs_FORWARD), TopAbs_EDGE); aEdgeIter.More();
         aEdgeIter.Next(), ++theGroupIds.edgeGroupId) {
        const TopoDS_Edge&                         aEdge    = TopoDS::Edge(aEdgeIter.Current());
        const Handle(Poly_PolygonOnTriangulation)& aPolygon = BRep_Tool::PolygonOnTriangulation(aEdge,
                                                                                                aPolyTri,
                                                                                                aFaceLoc);
        if (!aPolygon.IsNull()) {
            const bool isEdgeReversed = aEdge.Orientation() == TopAbs_REVERSED;
            AddEdgesToMeshModel(aPolygon, isEdgeReversed, theModel, aVertexOffset, theGroupIds.edgeGroupId);
        }
    }
}

//! Creates mapping of groups to triangles in the mesh model
static std::map<int, SimMesh_IdList> MapGroupsToTris(const Handle(SimMesh_Model)& theModel, IntRange& theGroupIdRange)
{
    std::map<int, SimMesh_IdList> aMapGroupTris;
    for (SimMesh_Id aTriaId = 0; aTriaId < theModel->GetNbTriangles(); ++aTriaId) {
        if (theModel->IsTriangleValid(aTriaId)) {
            int aGroupId = theModel->GetTriangleGroup(aTriaId);
            aMapGroupTris[aGroupId].push_back(aTriaId);
            theGroupIdRange.Update(aGroupId);
        }
    }
    return aMapGroupTris;
}

//! Creates mapping of groups to edges in the mesh model
static std::map<int, SimMesh_IdList> MapGroupsToEdges(const Handle(SimMesh_Model)& theModel, IntRange& theGroupIdRange)
{
    std::map<int, SimMesh_IdList> aMapGroupEdges;
    for (SimMesh_Id aMeshEdgeId = 0; aMeshEdgeId < theModel->GetNbEdges(); ++aMeshEdgeId) {
        if (theModel->IsEdgeValid(aMeshEdgeId)) {
            int aGroupId = theModel->GetEdgeGroup(aMeshEdgeId);
            aMapGroupEdges[aGroupId].push_back(aMeshEdgeId);
            theGroupIdRange.Update(aGroupId);
        }
    }
    return aMapGroupEdges;
}

//! Makes triangulation in the face from triangles group of a mesh model.
static void MakeFaceFromTrianglesGroup(const Handle(SimMesh_Model)&   theModel,
                                       const SimMesh_IdList&          theTris,
                                       const std::vector<SimMesh_Id>& theFaceVertIds,
                                       std::function<int(SimMesh_Id)> theGetVertexIndex,
                                       const TopoDS_Face&             theFace)
{
    // Create triangulation.
    Handle(Poly_Triangulation) aPoly = new Poly_Triangulation();

    // Create triangles with generation of vertex indices.
    const int aNbTri = int(theTris.size());
    aPoly->ResizeTriangles(aNbTri, false);
    for (int i = 0; i < aNbTri; ++i) {
        const SimMesh_Triangle& aTria = theModel->GetTriangle(theTris[i]);
        aPoly->SetTriangle(i + 1,
                           Poly_Triangle(theGetVertexIndex(aTria[0]),
                                         theGetVertexIndex(aTria[1]),
                                         theGetVertexIndex(aTria[2])));
    }
    const int aNbNodes = int(theFaceVertIds.size());

    // Set triangulation properties.
    aPoly->SetDoublePrecision(false);
    aPoly->ResizeNodes(aNbNodes, false);
    if (theModel->IsVertexTextureCoords()) {
        aPoly->AddUVNodes();
    }
    if (theModel->IsVertexNormals()) {
        aPoly->AddNormals();
    }
    aPoly->SetMeshPurpose(Poly_MeshPurpose_Presentation);
    aPoly->Deflection(theModel->GetMaxError());

    // Add nodes.
    for (int i = 0; i < aNbNodes; i++) {
        SimMesh_Id            aVertId = theFaceVertIds[i];
        const SimMesh_Vertex& aVert   = theModel->GetVertex(aVertId);
        aPoly->SetNode(i + 1, gp_Pnt(aVert.x(), aVert.y(), aVert.z()));

        if (theModel->IsVertexNormals()) {
            const SimMesh_Attributes& anAttr = theModel->GetVertexAttrib(aVertId);
            aPoly->SetNormal(i + 1, anAttr.GetNormal());
        }

        if (theModel->IsVertexTextureCoords()) {
            const SimMesh_Attributes& anAttr    = theModel->GetVertexAttrib(aVertId);
            const SimMesh_Vec2f&      aTexCoord = anAttr.GetTexCoord();
            aPoly->SetUVNode(i + 1, gp_Pnt2d(aTexCoord.x(), aTexCoord.y()));
        }
    }
    BRep_Builder::UpdateFace(theFace, aPoly);
}

//! Makes polygons 3D from mesh lines.
static void MakePolygons3DFromMeshLines(const Handle(SimMesh_Model)&       theModel,
                                        const std::vector<SimMesh_IdList>& theLines,
                                        TopoDS_Compound&                   theResShape)
{
    for (const auto& aLine : theLines) {
        int                aNbNodes = int(aLine.size());
        TColgp_Array1OfPnt aPntArray(1, aNbNodes);
        for (int i = 0; i < aNbNodes; ++i) {
            const SimMesh_Vertex& aVert = theModel->GetVertex(aLine[i]);
            aPntArray[i + 1].SetCoord(aVert.x(), aVert.y(), aVert.z());
        }
        Handle(Poly_Polygon3D) aPol = new Poly_Polygon3D(aPntArray);
        TopoDS_Edge            anEdge;
        BRep_Builder::MakeEdge(anEdge, aPol);
        BRep_Builder::Add(theResShape, anEdge);
    }
}

//! Makes polygons on triangulations from mesh lines.
static void MakePolygonsOnTriFromMeshLines(const std::vector<SimMesh_IdList>&         theLines,
                                           const std::map<SimMesh_Id, IndFace>&       theVrtFaceMap,
                                           const TopoDS_Face&                         theFace,
                                           TopTools_IndexedDataMapOfShapeListOfShape& theFacesEdges)
{
    for (const auto& aLine : theLines) {
        int                     aNbNodes = int(aLine.size());
        TColStd_Array1OfInteger aNodeIndices(1, aNbNodes);
        for (int i = 0; i < aNbNodes; ++i) {
            aNodeIndices[i + 1] = theVrtFaceMap.find(aLine[i])->second.vertInd;
        }
        Handle(Poly_PolygonOnTriangulation) aPol = new Poly_PolygonOnTriangulation(aNodeIndices);
        TopLoc_Location                     aLoc;
        Handle(Poly_Triangulation)          aPolyTri = BRep_Tool::Triangulation(theFace, aLoc);
        TopoDS_Edge                         anEdge;
        BRep_Builder::MakeEdge(anEdge, aPol, aPolyTri, aLoc);
        theFacesEdges.ChangeFromKey(theFace).Append(anEdge);
    }
}

//! Makes polygons from edges group of a mesh model. Detects a face to which belongs each edge
//! and creates polygon on the corresponding triangulation. If an edge does not belong to a face
//! then create polygon 3D.
static void MakePolygonsFromEdgesGroup(const Handle(SimMesh_Model)&               theModel,
                                       const SimMesh_IdList&                      theMeshEdges,
                                       const std::map<SimMesh_Id, IndFace>&       theVrtFaceMap,
                                       TopTools_IndexedDataMapOfShapeListOfShape& theFacesEdges,
                                       TopoDS_Compound&                           theResShape)
{
    // Check if edge is on face.
    TopoDS_Face aFace;
    SimMesh_Id  aVertId = theModel->GetEdge(theMeshEdges[0])[0];
    auto        aFound  = theVrtFaceMap.find(aVertId);
    if (aFound != theVrtFaceMap.end()) {
        aFace = aFound->second.face;
        // Confirm that all vertices are on the same face
        for (size_t anInd = 0; anInd < theMeshEdges.size() && !aFace.IsNull(); ++anInd) {
            const SimMesh_Edge& aMeshEdge = theModel->GetEdge(theMeshEdges[anInd]);
            for (SimMesh_Id anId : {aMeshEdge[0], aMeshEdge[1]}) {
                aFound = theVrtFaceMap.find(anId);
                if (aFound == theVrtFaceMap.end() || aFound->second.face != aFace) {
                    aFace.Nullify();
                }
            }
        }
        Standard_ASSERT_VOID(!aFace.IsNull(), "");
    }

    // Create polylines from a set of edges using connectivity.
    std::vector<SimMesh_IdList> aLines = CreatePolylines(theModel, theMeshEdges);

    if (aFace.IsNull()) {
        MakePolygons3DFromMeshLines(theModel, aLines, theResShape);
    } else {
        MakePolygonsOnTriFromMeshLines(aLines, theVrtFaceMap, aFace, theFacesEdges);
    }
}

//! Adds faces to the result shape.
static void AddFacesToResult(const TopTools_IndexedDataMapOfShapeListOfShape& theFacesEdges,
                             TopoDS_Compound&                                 theResShape)
{
    for (int aFaceIter = 1; aFaceIter <= theFacesEdges.Extent(); ++aFaceIter) {
        const TopoDS_Shape&         aFace   = theFacesEdges.FindKey(aFaceIter);
        const TopTools_ListOfShape& anEdges = theFacesEdges.FindFromIndex(aFaceIter);
        if (!anEdges.IsEmpty()) {
            TopoDS_Wire aWire = BRep_Builder::MakeWire();
            for (const TopoDS_Shape& anEdge : anEdges) {
                BRep_Builder::Add(aWire, anEdge);
            }
            BRep_Builder::Add(const_cast<TopoDS_Shape&>(aFace), aWire);
        }
        BRep_Builder::Add(theResShape, aFace);
    }
}
}

// =======================================================================
// function : ModelFromShape
// =======================================================================
Handle(SimMesh_Model) SimMesh::ModelFromShape(const TopoDS_Shape& theShape)
{
    if (theShape.IsNull()) {
        return Handle(SimMesh_Model)();
    }

    Numbers aNumbers;
    CountMeshEntityNumbers(theShape, aNumbers);
    if (aNumbers.nbNodes < 3 || (aNumbers.nbTris + aNumbers.nbEdges) < 1) {
        return Handle(SimMesh_Model)();
    }

    // Fill in the new model
    Handle(SimMesh_Model) aModel = new SimMesh_Model(aNumbers.nbNodes, aNumbers.nbTris, aNumbers.nbEdges);
    aModel->SetVertexNormals(aNumbers.hasNormals);
    aModel->SetVertexColors(false);
    aModel->SetVertexTextureCoords(aNumbers.hasTexCoords);
    aModel->SetTriangleColors(false);
    GroupIds aGroupIds;

    // Treat topo faces
    for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next(), ++aGroupIds.triGroupId) {
        const TopoDS_Face& aFace = TopoDS::Face(aFaceIter.Current());
        AddFaceToMeshModel(aFace, aModel, aNumbers, aGroupIds);
    }

    // Treat separate edges
    for (TopExp_Explorer aEdgeIter(theShape, TopAbs_EDGE, TopAbs_FACE); aEdgeIter.More();
         aEdgeIter.Next(), ++aGroupIds.edgeGroupId) {
        const TopoDS_Edge&            aEdge = TopoDS::Edge(aEdgeIter.Current());
        TopLoc_Location               aEdgeLoc;
        const Handle(Poly_Polygon3D)& aPolygon = BRep_Tool::Polygon3D(aEdge, aEdgeLoc);
        if (!aPolygon.IsNull()) {
            const bool isEdgeReversed = aEdge.Orientation() == TopAbs_REVERSED;
            AddEdgesToMeshModel(aPolygon, aEdgeLoc.Transformation(), isEdgeReversed, aModel, aGroupIds.edgeGroupId);
        }
    }

    return aModel;
}

// =======================================================================
// function : ExtractTriangulation
// =======================================================================
Handle(Poly_Triangulation) SimMesh::ExtractTriangulation(const Handle(SimMesh_Model)& theModel)
{
    if (theModel.IsNull()) {
        return Handle(Poly_Triangulation)();
    }

    theModel->CompactVertices();
    theModel->SplitVertices();

    // Count valid triangles
    const size_t aNbResVerts = theModel->GetNbVertices();
    int          aNbResTris  = 0;
    for (SimMesh_Id aTriIter = 0; aTriIter < theModel->GetNbTriangles(); ++aTriIter) {
        if (theModel->IsTriangleValid(aTriIter)) {
            ++aNbResTris;
        }
    }
    if (aNbResTris == 0 || aNbResVerts == 0) {
        return Handle(Poly_Triangulation)();
    }

    // Fill in the new triangulation
    Handle(Poly_Triangulation) aResTri = new Poly_Triangulation();
    aResTri->SetDoublePrecision(false);
    aResTri->ResizeNodes(static_cast<int>(aNbResVerts), false);
    aResTri->ResizeTriangles(aNbResTris, false);
    if (theModel->IsVertexTextureCoords()) {
        aResTri->AddUVNodes();
    }
    if (theModel->IsVertexNormals()) {
        aResTri->AddNormals();
    }
    aResTri->Deflection(theModel->GetMaxError());

    for (SimMesh_Id aVertIter = 0; aVertIter < aNbResVerts; ++aVertIter) {
        const SimMesh_Vertex& aVert = theModel->GetVertex(aVertIter);
        aResTri->SetNode((int)(aVertIter + 1), gp_Pnt(aVert.x(), aVert.y(), aVert.z()));
        if (theModel->IsVertexNormals()) {
            const SimMesh_Vec3f& aVertNorm = theModel->GetVertexAttrib(aVertIter).GetNormal();
            aResTri->SetNormal((int)(aVertIter + 1), aVertNorm);
        }
        if (theModel->IsVertexTextureCoords()) {
            const SimMesh_Vec2f& aTexCoord = theModel->GetVertexAttrib(aVertIter).GetTexCoord();
            aResTri->SetUVNode((int)(aVertIter + 1), gp_Pnt2d(aTexCoord.x(), aTexCoord.y()));
        }
    }
    for (SimMesh_Id aTriIter = 0, aTriResIndex = 1; aTriIter < theModel->GetNbTriangles(); ++aTriIter) {
        const SimMesh_Triangle& aTri = theModel->GetTriangle(aTriIter);
        if (theModel->IsTriangleValid(aTriIter)) {
            Standard_ASSERT_VOID(aTri[0] < aNbResVerts && aTri[1] < aNbResVerts && aTri[2] < aNbResVerts,
                                 "Invalid triangle node index");
            aResTri->SetTriangle(int(aTriResIndex++),
                                 Poly_Triangle(int(aTri[0] + 1), int(aTri[1] + 1), int(aTri[2] + 1)));
        }
    }

    return aResTri;
}

// =======================================================================
// function : ShapeFromModel
// =======================================================================
TopoDS_Shape SimMesh::ShapeFromModel(const Handle(SimMesh_Model)& theModel)
{
    if (theModel.IsNull()) {
        return TopoDS_Shape();
    }

    theModel->CompactVertices();
    theModel->SplitVertices();

    // Create mapping of groups to mesh entities
    IntRange                      aTriGroupIdRange;
    IntRange                      anEdgeGroupIdRange;
    std::map<int, SimMesh_IdList> aMapGroupTris  = MapGroupsToTris(theModel, aTriGroupIdRange);
    std::map<int, SimMesh_IdList> aMapGroupEdges = MapGroupsToEdges(theModel, anEdgeGroupIdRange);
    if (aMapGroupTris.empty() && aMapGroupEdges.empty()) {
        return TopoDS_Shape();
    }

    // Create result compound shape
    TopoDS_Compound aResShape = BRep_Builder::MakeCompound();

    // Iterate on face groups, create TopoDS_Face with triangulation from them
    // and map all vertices to pairs (Vertex index, Face).
    std::map<SimMesh_Id, IndFace>             aVrtFaceMap;
    TopTools_IndexedDataMapOfShapeListOfShape aFacesEdges;
    for (int aFaceId = aTriGroupIdRange.min; aFaceId <= aTriGroupIdRange.max; ++aFaceId) {
        TopoDS_Face aFace;
        BRep_Builder().MakeFace(aFace);
        aFacesEdges.Add(aFace, TopTools_ListOfShape());
        auto aFoundTriList = aMapGroupTris.find(aFaceId);
        if (aFoundTriList == aMapGroupTris.end()) {
            continue;
        }
        // Generator of vertex index inside face
        std::vector<SimMesh_Id>        aFaceVertIds;
        std::function<int(SimMesh_Id)> getVertexIndex = [&aVrtFaceMap, &aFaceVertIds, &aFace](SimMesh_Id theVertId) {
            auto aFound = aVrtFaceMap.lower_bound(theVertId); // search for the vertex
            if (aFound == aVrtFaceMap.end() || aVrtFaceMap.key_comp()(theVertId, aFound->first)) {
                aFaceVertIds.push_back(theVertId);
                IndFace aIndface = {(int)aFaceVertIds.size(), aFace};
                aFound           = aVrtFaceMap.insert(aFound, std::make_pair(theVertId, aIndface));
            } else {
                // Check inconsistent data, such that a vertex is shared with other faces.
                // It cannot occur after the call to SplitVertices().
                Standard_ASSERT_VOID(aFound->second.face == aFace, "");
            }
            return aFound->second.vertInd;
        };
        MakeFaceFromTrianglesGroup(theModel, aFoundTriList->second, aFaceVertIds, getVertexIndex, aFace);
    }

    // Iterate on edge groups and make polygons.
    for (int anEdgeId = anEdgeGroupIdRange.min; anEdgeId <= anEdgeGroupIdRange.max; ++anEdgeId) {
        auto aFoundEdgeList = aMapGroupEdges.find(anEdgeId);
        if (aFoundEdgeList == aMapGroupEdges.end()) {
            // empty edge, skip it
            continue;
        }
        MakePolygonsFromEdgesGroup(theModel, aFoundEdgeList->second, aVrtFaceMap, aFacesEdges, aResShape);
    }

    AddFacesToResult(aFacesEdges, aResShape);
    return aResShape;
}

