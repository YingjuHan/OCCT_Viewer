#include <SimMesh_Model.h>

#include <Message.hxx>
#include <NCollection_LocalArray.hxx>
#include <Precision.hxx>

#include <algorithm>
#include <functional>

// =======================================================================
// function : SimMesh_Model
// =======================================================================
SimMesh_Model::SimMesh_Model()
{
    //
}

// =======================================================================
// function : SimMesh_Model
// =======================================================================
SimMesh_Model::SimMesh_Model(size_t theNbVertsReserve, size_t theNbTriaReserve, size_t theNbEdgesReserve)
{
    myVertices.reserve(theNbVertsReserve);
    myVertDataLists.reserve(theNbVertsReserve);
    myTriangles.reserve(theNbTriaReserve);
    myEdges.reserve(theNbEdgesReserve);
}

// =======================================================================
// function : AddVertex
// =======================================================================
SimMesh_Id SimMesh_Model::AddVertex(const SimMesh_Vertex& theVert)
{
    const SimMesh_Id aVertId = (SimMesh_Id)myVertices.size();
    myVertices.push_back(theVert);

    myVertDataLists.push_back(SimMesh_VertexDataList());
    Standard_ASSERT_VOID(myVertDataLists.size() == myVertices.size(), "");
    SimMesh_Attributes anAttrib(myIsVertNormals, myIsVertColors, myIsVertTCoords);
    myVertDataLists[aVertId].emplace_back(anAttrib);

    myBndBox.Clear();
    return aVertId;
}

// =======================================================================
// function : AddTriangle
// =======================================================================
SimMesh_Id SimMesh_Model::AddTriangle(const SimMesh_Triangle& theTria, bool theToLink)
{
    const SimMesh_Id anEntId = (SimMesh_Id)myTriangles.size();
    myTriangles.push_back(theTria);
    if (theTria[0] != theTria[1] && theTria[1] != theTria[2] && theTria[2] != theTria[0]) {
        if (theToLink) {
            const SimMesh_Triangle& aTria = myTriangles.back();
            Standard_ASSERT_VOID(aTria[0] < myVertDataLists.size() && !myVertDataLists[aTria[0]].empty(), "");
            Standard_ASSERT_VOID(aTria[1] < myVertDataLists.size() && !myVertDataLists[aTria[1]].empty(), "");
            Standard_ASSERT_VOID(aTria[2] < myVertDataLists.size() && !myVertDataLists[aTria[2]].empty(), "");
            myVertDataLists[aTria[0]].begin()->Triangles.push_back(anEntId);
            myVertDataLists[aTria[1]].begin()->Triangles.push_back(anEntId);
            myVertDataLists[aTria[2]].begin()->Triangles.push_back(anEntId);
        }
        if (myIsTriaColors) {
            myTriaColors.emplace_back();
        }
    } else {
        myTriangles.back().SetValid(false);
    }
    return anEntId;
}

// =======================================================================
// function : AddEdge
// =======================================================================
SimMesh_Id SimMesh_Model::AddEdge(const SimMesh_Edge& theEdge, bool theToLink)
{
    const SimMesh_Id anEdgeId = (SimMesh_Id)myEdges.size();
    myEdges.push_back(theEdge);
    if (theEdge[0] != theEdge[1]) {
        if (theToLink) {
            const SimMesh_Edge& anEdge = myEdges[anEdgeId];
            Standard_ASSERT_VOID(anEdge[0] < myVertDataLists.size() && !myVertDataLists[anEdge[0]].empty(), "");
            Standard_ASSERT_VOID(anEdge[1] < myVertDataLists.size() && !myVertDataLists[anEdge[1]].empty(), "");
            myVertDataLists[anEdge[0]].begin()->Edges.push_back(anEdgeId);
            myVertDataLists[anEdge[1]].begin()->Edges.push_back(anEdgeId);
        }
    } else {
        myEdges.back().SetValid(false);
    }
    return anEdgeId;
}

// =======================================================================
// function : InvalidateFreeVertices
// =======================================================================
void SimMesh_Model::InvalidateFreeVertices()
{
    for (SimMesh_Id aVIter = 0; aVIter < GetNbVertices(); ++aVIter) {
        bool isEmpty = true;
        for (const auto& aVData : GetVertexDataList(aVIter)) {
            if (!aVData.Triangles.empty() || !aVData.Edges.empty()) {
                isEmpty = false;
                break;
            }
        }
        if (isEmpty) {
            myVertices[aVIter].SetValid(false);
        }
    }
}

// =======================================================================
// function : GetTriangleNormal
// =======================================================================
const SimMesh_Vec3d& SimMesh_Model::GetTriangleNormal(SimMesh_Id theTriaId, double& theArea) const
{
    const SimMesh_Triangle& aTria = myTriangles[theTriaId];
    if (aTria.IsDirty()) {
        // compute normal to save it in the triangle
        const SimMesh_Vec3d aVert1(myVertices[aTria[0]]);
        const SimMesh_Vec3d aVert2(myVertices[aTria[1]]);
        const SimMesh_Vec3d aVert3(myVertices[aTria[2]]);

        const SimMesh_Vec3d aVec12 = aVert2 - aVert1;
        const SimMesh_Vec3d aVec13 = aVert3 - aVert1;

        aTria.ChangeNormal() = SimMesh_Vec3d::Cross(aVec12, aVec13);
        aTria.ChangeArea()   = aTria.GetNormal().Modulus();
        if (aTria.GetArea() > DBL_MIN) {
            aTria.ChangeNormal() /= aTria.GetArea();
            aTria.ChangeArea() *= 0.5;
        }
        aTria.SetDirty(false);
    }
    theArea = aTria.GetArea();
    return aTria.GetNormal();
}

// =======================================================================
// function : GetBndBox
// =======================================================================
const Bnd_B3f& SimMesh_Model::GetBndBox() const
{
    if (myBndBox.IsVoid()) {
        for (const SimMesh_Vertex& aVertex : myVertices) {
            if (aVertex.IsValid()) {
                myBndBox.Add(gp_XYZ(aVertex.x(), aVertex.y(), aVertex.z()));
            }
        }
    }
    return myBndBox;
}

// =======================================================================
// function : markNeighborhood
// =======================================================================
void SimMesh_Model::markNeighborhood(SimMesh_Id theVertId, uint8_t theMark)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& aTriangles = aVData.Triangles;
        const size_t          aSize      = aTriangles.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEntId                = aTriangles[aNeighIter];
            myTriangles[anEntId].ChangeMark() = theMark;
        }

        const SimMesh_IdList& anEdges     = aVData.Edges;
        const size_t          anEdgesSize = anEdges.size();
        for (size_t aNeighIter = 0; aNeighIter < anEdgesSize; ++aNeighIter) {
            SimMesh_Id anEdgeId            = anEdges[aNeighIter];
            myEdges[anEdgeId].ChangeMark() = theMark;
        }
    }
}

// =======================================================================
// function : markNeighborhoodDelta
// =======================================================================
void SimMesh_Model::markNeighborhoodDelta(SimMesh_Id theVertId, uint8_t theDelta)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& aTriangles = aVData.Triangles;
        const size_t          aSize      = aTriangles.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEntId = aTriangles[aNeighIter];
            myTriangles[anEntId].ChangeMark() += theDelta;
        }

        const SimMesh_IdList& anEdges     = aVData.Edges;
        const size_t          anEdgesSize = anEdges.size();
        for (size_t aNeighIter = 0; aNeighIter < anEdgesSize; ++aNeighIter) {
            SimMesh_Id anEdgeId = anEdges[aNeighIter];
            myEdges[anEdgeId].ChangeMark() += theDelta;
        }
    }
}

// =======================================================================
// function : gatherUnmarkedNeighborTriangles
// =======================================================================
void SimMesh_Model::gatherUnmarkedNeighborTriangles(SimMesh_Id theVertId, SimMesh_IdList& theTriangles)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& aTriangles = aVData.Triangles;
        const size_t          aSize      = aTriangles.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEntId = aTriangles[aNeighIter];
            if (myTriangles[anEntId].GetMark() == 0) {
                theTriangles.push_back(anEntId);
                myTriangles[anEntId].ChangeMark() = 1;
            }
        }
    }
}

// =======================================================================
// function : gatherUnmarkedNeighborEdges
// =======================================================================
void SimMesh_Model::gatherUnmarkedNeighborEdges(SimMesh_Id theVertId, SimMesh_IdList& theEdges)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& anEdges = aVData.Edges;
        const size_t          aSize   = anEdges.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEdgeId = anEdges[aNeighIter];
            if (myEdges[anEdgeId].GetMark() == 0) {
                theEdges.push_back(anEdgeId);
                myEdges[anEdgeId].ChangeMark() = 1;
            }
        }
    }
}

// =======================================================================
// function : splitMarkedNeighborTriangles
// =======================================================================
void SimMesh_Model::splitMarkedNeighborTriangles(SimMesh_Id      theVertId,
                                                 uint8_t         thePivot,
                                                 SimMesh_IdList& theTrianglesLower,
                                                 SimMesh_IdList& theTrianglesHigher)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& aTriangles = aVData.Triangles;
        const size_t          aSize      = aTriangles.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEntId = aTriangles[aNeighIter];
            if (myTriangles[anEntId].GetMark() == 0) {
                continue;
            }
            if (myTriangles[anEntId].GetMark() < thePivot) {
                theTrianglesLower.push_back(anEntId);
            } else {
                theTrianglesHigher.push_back(anEntId);
            }
            myTriangles[anEntId].ChangeMark() = 0;
        }
    }
}

// =======================================================================
// function : splitMarkedNeighborEdges
// =======================================================================
void SimMesh_Model::splitMarkedNeighborEdges(SimMesh_Id      theVertId,
                                             uint8_t         thePivot,
                                             SimMesh_IdList& theEdgesLower,
                                             SimMesh_IdList& theEdgesHigher)
{
    Standard_ASSERT_VOID(theVertId < GetNbVertices(), "");
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        const SimMesh_IdList& anEdges = aVData.Edges;
        const size_t          aSize   = anEdges.size();
        for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
            SimMesh_Id anEdgeId = anEdges[aNeighIter];
            if (myEdges[anEdgeId].GetMark() == 0) {
                continue;
            }
            if (myEdges[anEdgeId].GetMark() < thePivot) {
                theEdgesLower.push_back(anEdgeId);
            } else {
                theEdgesHigher.push_back(anEdgeId);
            }
            myEdges[anEdgeId].ChangeMark() = 0;
        }
    }
}

// =======================================================================
// function : markTrianglesVertices
// =======================================================================
void SimMesh_Model::markTrianglesVertices(const SimMesh_IdList& theTriangles, const bool theMark)
{
    const size_t aSize = theTriangles.size();
    for (size_t aTriIter = 0; aTriIter < aSize; ++aTriIter) {
        const SimMesh_Triangle& aTria = myTriangles[theTriangles[aTriIter]];
        for (int aTriVertIter = 0; aTriVertIter < 3; ++aTriVertIter) {
            myVertices[aTria[aTriVertIter]].SetMarked(theMark);
        }
    }
}

// =======================================================================
// function : markEdgesVertices
// =======================================================================
void SimMesh_Model::markEdgesVertices(const SimMesh_IdList& theEdges, const bool theMark)
{
    const size_t aSize = theEdges.size();
    for (size_t anEdgeIter = 0; anEdgeIter < aSize; ++anEdgeIter) {
        const SimMesh_Edge& anEdge = myEdges[theEdges[anEdgeIter]];
        for (int anEdgeVertIter = 0; anEdgeVertIter < 2; ++anEdgeVertIter) {
            myVertices[anEdge[anEdgeVertIter]].SetMarked(theMark);
        }
    }
}

// =======================================================================
// function : gatherUnmarkedTrianglesVertices
// =======================================================================
void SimMesh_Model::gatherUnmarkedTrianglesVertices(const SimMesh_IdList& theTriangles, SimMesh_IdList& theVerts)
{
    const size_t aSize = theTriangles.size();
    for (size_t aTriIter = 0; aTriIter < aSize; ++aTriIter) {
        const SimMesh_Triangle& aTria = myTriangles[theTriangles[aTriIter]];
        for (int aTriVertIter = 0; aTriVertIter < 3; ++aTriVertIter) {
            const SimMesh_Id aVertId = aTria[aTriVertIter];
            if (!myVertices[aVertId].IsMarked()) {
                theVerts.push_back(aVertId);
                myVertices[aVertId].SetMarked(true);
            }
        }
    }
}

// =======================================================================
// function : gatherUnmarkedEdgesVertices
// =======================================================================
void SimMesh_Model::gatherUnmarkedEdgesVertices(const SimMesh_IdList& theEdges, SimMesh_IdList& theVerts)
{
    const size_t aSize = theEdges.size();
    for (size_t anEdgeIter = 0; anEdgeIter < aSize; ++anEdgeIter) {
        const SimMesh_Edge& anEdge = myEdges[theEdges[anEdgeIter]];
        for (int anEdgeVertIter = 0; anEdgeVertIter < 2; ++anEdgeVertIter) {
            const SimMesh_Id aVertId = anEdge[anEdgeVertIter];
            if (!myVertices[aVertId].IsMarked()) {
                theVerts.push_back(aVertId);
                myVertices[aVertId].SetMarked(true);
            }
        }
    }
}

// =======================================================================
// function : gatherEdgeNeighbors
// =======================================================================
void SimMesh_Model::gatherEdgeNeighbors(SimMesh_Id      theVert1,
                                        SimMesh_Id      theVert2,
                                        SimMesh_IdList& theTriangles,
                                        SimMesh_IdList& theEdges)
{
    markNeighborhood(theVert1, 1);
    markNeighborhood(theVert2, 0);
    gatherUnmarkedNeighborTriangles(theVert1, theTriangles);
    gatherUnmarkedNeighborEdges(theVert1, theEdges);
}

// =======================================================================
// function : gatherNeighboringTriangles
// =======================================================================
void SimMesh_Model::gatherNeighboringTriangles(SimMesh_Id theVert1, SimMesh_Id theVert2, SimMesh_IdList& theTriangles)
{
    markNeighborhood(theVert1, 0);
    markNeighborhood(theVert2, 1);
    gatherUnmarkedNeighborTriangles(theVert1, theTriangles);
    markNeighborhood(theVert2, 0);
    gatherUnmarkedNeighborTriangles(theVert2, theTriangles);
}

// =======================================================================
// function : gatherAdjacentVertices
// =======================================================================
void SimMesh_Model::gatherAdjacentVertices(SimMesh_Id theVertId, SimMesh_IdList& theVerts)
{
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        markTrianglesVertices(aVData.Triangles, false);
        markEdgesVertices(aVData.Edges, false);
    }
    myVertices[theVertId].SetMarked(true); // don't want to include vertex in the star
    for (const SimMesh_VertexData& aVData : myVertDataLists[theVertId]) {
        gatherUnmarkedTrianglesVertices(aVData.Triangles, theVerts);
        gatherUnmarkedEdgesVertices(aVData.Edges, theVerts);
    }
}

// =======================================================================
// function : CompactVertices
// =======================================================================
void SimMesh_Model::CompactVertices()
{
    SimMesh_Id aNewId = 0;
    for (SimMesh_Id anOldId = 0; anOldId < GetNbVertices(); ++anOldId) {
        if (!myVertices[anOldId].IsValid()) {
            continue;
        }

        if (aNewId == anOldId) {
            ++aNewId;
            continue;
        }

        // just swap values as old vertices will be freed later
        std::swap(myVertices[aNewId], myVertices[anOldId]);
        std::swap(myVertDataLists[aNewId], myVertDataLists[anOldId]);

        for (SimMesh_VertexData& aVData : myVertDataLists[aNewId]) {
            SimMesh_IdList& aTriList = aVData.Triangles;
            const size_t    aSize    = aTriList.size();
            for (size_t aNeighIter = 0; aNeighIter < aSize; ++aNeighIter) {
                myTriangles[aTriList[aNeighIter]].RemapVertex(anOldId, aNewId);
            }

            SimMesh_IdList& anEdgeList  = aVData.Edges;
            const size_t    anEdgesSize = anEdgeList.size();
            for (size_t aNeighIter = 0; aNeighIter < anEdgesSize; ++aNeighIter) {
                myEdges[anEdgeList[aNeighIter]].RemapVertex(anOldId, aNewId);
            }
        }
        ++aNewId;
    }

    // cut the tail
    myVertDataLists.erase(myVertDataLists.begin() + aNewId, myVertDataLists.end());
    myVertices.erase(myVertices.begin() + aNewId, myVertices.end());
}

// =======================================================================
// function : SplitVertices
// =======================================================================
void SimMesh_Model::SplitVertices()
{
    for (SimMesh_Id aVertId = 0; aVertId < GetNbVertices(); ++aVertId) {
        if (!myVertices[aVertId].IsValid()) {
            continue;
        }
        SimMesh_VertexDataList& aVDataList     = myVertDataLists[aVertId];
        const size_t            aVDataListSize = aVDataList.size();
        if (aVDataListSize <= 1) {
            continue;
        }
        const SimMesh_Vertex& aVertex = myVertices[aVertId];

        // leave the first data, and start with the second
        for (size_t aVDataInd = 1; aVDataInd < aVDataListSize; ++aVDataInd) {
            SimMesh_VertexData& aVData = aVDataList[aVDataInd];

            // create a copy of the vertex
            const SimMesh_Id aNewVertId = (SimMesh_Id)myVertices.size();
            myVertices.push_back(aVertex);

            // remap vertex ID in triangles
            for (SimMesh_Id anEntId : aVData.Triangles) {
                myTriangles[anEntId].RemapVertex(aVertId, aNewVertId);
            }

            // remap vertex ID in edges
            for (SimMesh_Id anEdgeId : aVData.Edges) {
                myEdges[anEdgeId].RemapVertex(aVertId, aNewVertId);
            }

            // move vertex data
            myVertDataLists.push_back(SimMesh_VertexDataList());
            myVertDataLists[aNewVertId].emplace_back(std::move(aVData));
        }
        // remove the copied vertex data
        aVDataList.erase(aVDataList.begin() + 1, aVDataList.end());
    }
}

// =======================================================================
// function : removeEntity
// =======================================================================
template<class TEntity>
void SimMesh_Model::removeEntity(SimMesh_Id theEntId, std::vector<TEntity>& theContainer)
{
    SimMesh_IdList SimMesh_VertexData::*anEntListPtr = std::is_same<TEntity, SimMesh_Triangle>::value
                                                         ? &SimMesh_VertexData::Triangles
                                                         : &SimMesh_VertexData::Edges;

    TEntity& anEntity = theContainer[theEntId];
    anEntity.SetValid(false);

    int aNbFound = 0;
    for (int i = 0; i < anEntity.Length(); ++i) {
        // remove this entity ID from vertices data
        SimMesh_VertexDataList& aVDataList = myVertDataLists[anEntity[i]];
        for (SimMesh_VertexData& aVData : aVDataList) {
            SimMesh_IdList& anEntList = aVData.*anEntListPtr;
            if (anEntList.RemoveItem(theEntId)) {
                (void)aNbFound;
                ++aNbFound;
            }
        }
        // remove vertex data with empty triangle and edge lists
        aVDataList.erase(std::remove_if(aVDataList.begin(),
                                        aVDataList.end(),
                                        [](const SimMesh_VertexData& theVData) {
                                            return theVData.Triangles.empty() && theVData.Edges.empty();
                                        }),
                         aVDataList.end());
        if (aVDataList.empty()) {
            // data list is empty, so mark vertex as not valid
            myVertices[anEntity[i]].SetValid(false);
        }
    }
}

// =======================================================================
// function : createContraction
// =======================================================================
void SimMesh_Model::createContraction(SimMesh_Id                     theVert1,
                                      SimMesh_Id                     theVert2,
                                      SimMesh_VertexPairContraction& theContraction)
{
    theContraction.Vert1 = theVert1;
    theContraction.Vert2 = theVert2;
    theContraction.DeltaTriangles.clear();
    theContraction.DeadTriangles.clear();
    theContraction.DeltaEdges.clear();
    theContraction.DeadEdges.clear();

    // Mark the neighborhood of (theVert1,theVert2) such that each triangle is
    // tagged with the number of times the vertices theVert1,theVert2 occur in it.
    // Possible values are 1 or 2.
    markNeighborhood(theVert2, 0);
    markNeighborhood(theVert1, 1);
    markNeighborhoodDelta(theVert2, 1);

    // Now partition the neighborhood of (theVert1,theVert2) into those triangles
    // which degenerate during contraction and those which are merely reshaped.
    splitMarkedNeighborTriangles(theVert1, 2, theContraction.DeltaTriangles, theContraction.DeadTriangles);
    theContraction.DeltaTrianglesPivot = theContraction.DeltaTriangles.size();
    splitMarkedNeighborTriangles(theVert2, 2, theContraction.DeltaTriangles, theContraction.DeadTriangles);

    // And the same with edges
    splitMarkedNeighborEdges(theVert1, 2, theContraction.DeltaEdges, theContraction.DeadEdges);
    theContraction.DeltaEdgesPivot = theContraction.DeltaEdges.size();
    splitMarkedNeighborEdges(theVert2, 2, theContraction.DeltaEdges, theContraction.DeadEdges);
}

// =======================================================================
// function : performContraction
// =======================================================================
void SimMesh_Model::performContraction(const SimMesh_VertexPairContraction& theContraction,
                                       const SimMesh_TargetType             theTargetType,
                                       const SimMesh_Vec3d&                 theTargetPos)
{
    const SimMesh_Id aVertId1 = theContraction.Vert1;
    const SimMesh_Id aVertId2 = theContraction.Vert2;

    // Merge vertex data from vertex 2 to vertex 1
    mergeVertexData(theContraction, theTargetType, theTargetPos);

    // Remove dead triangles and edges.
    NCollection_LocalArray<SimMesh_Id, 20> aLeftVertices(theContraction.DeadTriangles.size() + 1);
    int                                    aNbLeftVert = 0;
    removeDeadEntities(theContraction, aLeftVertices, aNbLeftVert);

    // Update changed triangles and edges.
    updateChangedEntities(theContraction);

    // Remove non-beam edges that are left without face support.
    removeEdgesLeftWithoutFace(aLeftVertices, aNbLeftVert);

    // Set new position of the vertex 1.
    if (myVertices[aVertId1].IsValid()) {
        // vertex 1 remains in the mesh
        if (theTargetType == SimMesh_TargetType::Optimized) {
            // update position from optimized vector
            myVertices[aVertId1] = SimMesh_Vertex(theTargetPos);
        } else if (theTargetType == SimMesh_TargetType::UseSecond) {
            // use position of the second vertex
            myVertices[aVertId1].SetCoords(myVertices[aVertId2]);
        }
    }

    // invalidate second vertex
    myVertices[aVertId2].SetValid(false);
    myBndBox.Clear();
}

// =======================================================================
// function : mergeVertexData
// =======================================================================
void SimMesh_Model::mergeVertexData(const SimMesh_VertexPairContraction& theContraction,
                                    const SimMesh_TargetType             theTargetType,
                                    const SimMesh_Vec3d&                 theTargetPos)
{
    const SimMesh_Id        aVertId1     = theContraction.Vert1;
    const SimMesh_Id        aVertId2     = theContraction.Vert2;
    SimMesh_VertexDataList& aVDataList1  = myVertDataLists[aVertId1];
    SimMesh_VertexDataList& aVDataList2  = myVertDataLists[aVertId2];
    double                  anInterParam = 0.;
    if (theTargetType == SimMesh_TargetType::Optimized) {
        const SimMesh_Vec3d& aVec1    = SimMesh_Vec3d(myVertices[aVertId1]);
        const SimMesh_Vec3d& aVec2    = SimMesh_Vec3d(myVertices[aVertId2]);
        SimMesh_Vec3d        aVec12   = aVec2 - aVec1;
        double               aSqMod12 = aVec12.SquareModulus();
        if (aSqMod12 > Precision::Confusion() * Precision::Confusion()) {
            anInterParam = aVec12.Dot(theTargetPos - aVec1) / aSqMod12;
            anInterParam = Max(0., Min(anInterParam, 1.));
        }
    }
    aVDataList1.Merge(aVDataList2, theContraction.DeadTriangles, theContraction.DeadEdges, theTargetType, anInterParam);
    aVDataList2.clear();
}

// =======================================================================
// function : removeDeadEntities
// =======================================================================
void SimMesh_Model::removeDeadEntities(const SimMesh_VertexPairContraction& theContraction,
                                       SimMesh_Id                           theLeftVertices[],
                                       int&                                 theNbLeftVert)
{
    const SimMesh_Id aVertId1 = theContraction.Vert1;
    const SimMesh_Id aVertId2 = theContraction.Vert2;

    // remove dead triangles
    theLeftVertices[theNbLeftVert++] = aVertId1;
    for (SimMesh_Id anEntId : theContraction.DeadTriangles) {
        const SimMesh_Triangle& aTrg     = GetTriangle(anEntId);
        theLeftVertices[theNbLeftVert++] = aTrg.OppositeNode(aVertId1, aVertId2);
        removeEntity(anEntId, myTriangles);
    }

    // remove dead edges
    for (SimMesh_Id anEdgeId : theContraction.DeadEdges) {
        removeEntity(anEdgeId, myEdges);
    }
}

// =======================================================================
// function : updateChangedEntities
// =======================================================================
void SimMesh_Model::updateChangedEntities(const SimMesh_VertexPairContraction& theContraction)
{
    const SimMesh_Id aVertId1 = theContraction.Vert1;
    const SimMesh_Id aVertId2 = theContraction.Vert2;

    // update changed triangles
    for (size_t aDeltaIter = 0; aDeltaIter < theContraction.DeltaTrianglesPivot; ++aDeltaIter) {
        const SimMesh_Id anEntId = theContraction.DeltaTriangles[aDeltaIter];
        myTriangles[anEntId].SetDirty(true);
    }
    for (size_t aDeltaIter = theContraction.DeltaTrianglesPivot; aDeltaIter < theContraction.DeltaTriangles.size();
         ++aDeltaIter) {
        const SimMesh_Id anEntId = theContraction.DeltaTriangles[aDeltaIter];
        myTriangles[anEntId].RemapVertex(aVertId2, aVertId1);
    }

    // update changed edges
    for (size_t aDeltaIter = theContraction.DeltaEdgesPivot; aDeltaIter < theContraction.DeltaEdges.size();
         ++aDeltaIter) {
        const SimMesh_Id anEdgeId = theContraction.DeltaEdges[aDeltaIter];
        myEdges[anEdgeId].RemapVertex(aVertId2, aVertId1);
    }
}

// =======================================================================
// function : removeEdgesLeftWithoutFace
// =======================================================================
void SimMesh_Model::removeEdgesLeftWithoutFace(const SimMesh_Id theLeftVertices[], const int theNbVert)
{
    SimMesh_IdList aRemovedEdges(6);

    auto gatherNonBeamEdges = [this, &aRemovedEdges](const SimMesh_VertexData& theVData) {
        for (SimMesh_Id anEdgeId : theVData.Edges) {
            if (!myEdges[anEdgeId].IsBeam()) {
                aRemovedEdges.push_back(anEdgeId);
            }
        }
    };
    for (int aVertNum = 0; aVertNum < theNbVert; ++aVertNum) {
        SimMesh_Id aVertId = theLeftVertices[aVertNum];
        if (!myVertices[aVertId].IsValid()) {
            continue;
        }
        for (const SimMesh_VertexData& aVData : myVertDataLists[aVertId]) {
            if (!aVData.Triangles.empty() || aVData.Edges.empty()) {
                continue;
            }
            gatherNonBeamEdges(aVData);
        }
    }
    for (SimMesh_Id anEdgeId : aRemovedEdges) {
        removeEntity(anEdgeId, myEdges);
    }
}

// =======================================================================
// function : IsModelConsistent
// =======================================================================
bool SimMesh_Model::IsModelConsistent(Standard_OStream* theStr) const
{
    bool isOK = checkEntitiesConsistency(theStr, myTriangles);
    return (theStr || isOK) && checkEntitiesConsistency(theStr, myEdges);
}

// =======================================================================
// function : checkEntitiesConsistency
// =======================================================================
template<class TEntity>
bool SimMesh_Model::checkEntitiesConsistency(Standard_OStream* theStr, const std::vector<TEntity>& theContainer) const
{
    using LambdaType = std::function<void(SimMesh_Id, Standard_CString, SimMesh_Id)>;
    auto printMsg = theStr ? LambdaType{[&theStr](SimMesh_Id theEntId, Standard_CString theMsg, SimMesh_Id theVertId) {
        *theStr << (std::is_same<TEntity, SimMesh_Triangle>::value ? "triangle" : "edge") << " " << theEntId << theMsg
                << theVertId << "\n";
    }}
                           : LambdaType{[](SimMesh_Id, Standard_CString, SimMesh_Id) {
                             }};

    auto entityFinder = std::is_same<TEntity, SimMesh_Triangle>::value ? &SimMesh_VertexDataList::FindByTriangleId
                                                                       : &SimMesh_VertexDataList::FindByEdgeId;

    bool isOK = true;
    for (SimMesh_Id anEntId = 0; anEntId < theContainer.size() && (theStr || isOK); ++anEntId) {
        const TEntity& anEntity = theContainer[anEntId];
        if (!anEntity.IsValid()) {
            continue;
        }
        for (int i = 0; i < anEntity.Length() && (theStr || isOK); ++i) {
            SimMesh_Id            aVertId = anEntity[i];
            const SimMesh_Vertex& aVert   = GetVertex(aVertId);
            if (!aVert.IsValid()) {
                isOK = false;
                printMsg(anEntId, " refers to invalid vertex ", aVertId);
            }
            const SimMesh_VertexDataList& aVDataList = GetVertexDataList(aVertId);
            if ((aVDataList.*entityFinder)(anEntId) == aVDataList.end()) {
                isOK = false;
                printMsg(anEntId, " lost back link with vertex ", aVertId);
            }
        }
    }
    return isOK;
}

#include <SimMesh_Persistence.h>
#include <SimMesh_Model.h>

#include <Message.hxx>
#include <OSD_FileSystem.hxx>

namespace {
//! Phrase to write in the file for recognition.
static const std::string& EtalonPhrase()
{
    static const std::string EtalonPhrase("SimMesh_Model");
    return EtalonPhrase;
}

//! Writes header to the output stream.
static void writeHeader(const std::shared_ptr<std::ostream> theFile,
                        const int32_t                       theVersion,
                        const Handle(SimMesh_Model)&        theModel)
{
    // Identification info
    int32_t aPhraseLen = (int32_t)EtalonPhrase().size();
    theFile->write((Standard_Character*)&theVersion, sizeof(int32_t));
    theFile->write((Standard_Character*)&aPhraseLen, sizeof(int32_t));
    theFile->write(EtalonPhrase().c_str(), aPhraseLen);

    // Numbers
    int32_t aNbVert  = (int32_t)theModel->GetNbVertices();
    int32_t aNbTria  = (int32_t)theModel->GetNbTriangles();
    int32_t aNbEdges = (int32_t)theModel->GetNbEdges();
    theFile->write((Standard_Character*)&aNbVert, sizeof(int32_t));
    theFile->write((Standard_Character*)&aNbTria, sizeof(int32_t));
    theFile->write((Standard_Character*)&aNbEdges, sizeof(int32_t));

    // Flags
    bool hasNormals    = theModel->IsVertexNormals();
    bool hasTCoords    = theModel->IsVertexTextureCoords();
    bool hasColors     = theModel->IsVertexColors();
    bool hasTriaColors = theModel->IsTriangleColors();
    theFile->write((Standard_Character*)&hasNormals, sizeof(bool));
    theFile->write((Standard_Character*)&hasTCoords, sizeof(bool));
    theFile->write((Standard_Character*)&hasColors, sizeof(bool));
    theFile->write((Standard_Character*)&hasTriaColors, sizeof(bool));
}

//! Writes vertices, triangles and edges to the output stream.
static void writeMeshEntities(const std::shared_ptr<std::ostream> theFile, const Handle(SimMesh_Model)& theModel)
{
    // Vertices
    for (SimMesh_Id i = 0; i < theModel->GetNbVertices(); ++i) {
        const SimMesh_Vertex& aVert = theModel->GetVertex(i);
        theFile->write((Standard_Character*)&aVert, sizeof(SimMesh_Vertex));
    }

    // Triangles
    for (SimMesh_Id i = 0; i < theModel->GetNbTriangles(); ++i) {
        const SimMesh_Triangle& aTria = theModel->GetTriangle(i);
        theFile->write((Standard_Character*)&aTria, sizeof(SimMesh_Triangle));
    }

    // Edges
    for (SimMesh_Id i = 0; i < theModel->GetNbEdges(); ++i) {
        const SimMesh_Edge& aMeshEdge = theModel->GetEdge(i);
        theFile->write((Standard_Character*)&aMeshEdge, sizeof(SimMesh_Edge));
    }
}

//! Writes vertex data list to the output stream.
static void writeVertexDataList(const std::shared_ptr<std::ostream> theFile,
                                const SimMesh_VertexDataList&       theVDataList,
                                const bool                          theHasNormals,
                                const bool                          theHasTCoords,
                                const bool                          theHasColors)
{
    int32_t aNbData = (int32_t)theVDataList.size();
    theFile->write((Standard_Character*)&aNbData, sizeof(int32_t));
    for (int j = 0; j < aNbData; ++j) {
        const SimMesh_VertexData& aVData = theVDataList[j];

        int32_t aVDataNbTrias = (int32_t)aVData.Triangles.size();
        theFile->write((Standard_Character*)&aVDataNbTrias, sizeof(int32_t));
        for (int k = 0; k < aVDataNbTrias; ++k) {
            theFile->write((Standard_Character*)&aVData.Triangles[k], sizeof(SimMesh_Id));
        }

        int32_t aVDataNbEdges = (int32_t)aVData.Edges.size();
        theFile->write((Standard_Character*)&aVDataNbEdges, sizeof(int32_t));
        for (int k = 0; k < aVDataNbEdges; ++k) {
            theFile->write((Standard_Character*)&aVData.Edges[k], sizeof(SimMesh_Id));
        }

        const SimMesh_Attributes& anAttr = aVData.Attrib;
        if (theHasNormals) {
            const SimMesh_Vec3f& aNorm = anAttr.GetNormal();
            theFile->write((Standard_Character*)&aNorm, sizeof(SimMesh_Vec3f));
        }
        if (theHasColors) {
            const SimMesh_Color& aColor = anAttr.GetColor();
            theFile->write((Standard_Character*)&aColor, sizeof(SimMesh_Color));
        }
        if (theHasTCoords) {
            const SimMesh_Vec2f& aTCoord = anAttr.GetTexCoord();
            theFile->write((Standard_Character*)&aTCoord, sizeof(SimMesh_Vec2f));
        }
    }
}

//! Writes attributes to the output stream.
static void writeAttributes(const std::shared_ptr<std::ostream> theFile, const Handle(SimMesh_Model)& theModel)
{
    // Vertex data lists
    for (SimMesh_Id i = 0; i < theModel->GetNbVertices(); ++i) {
        const SimMesh_VertexDataList& aVDataList = theModel->GetVertexDataList(i);
        writeVertexDataList(theFile,
                            aVDataList,
                            theModel->IsVertexNormals(),
                            theModel->IsVertexTextureCoords(),
                            theModel->IsVertexColors());
    }

    // Triangle colors
    if (theModel->IsTriangleColors()) {
        for (SimMesh_Id j = 0; j < theModel->GetNbTriangles(); ++j) {
            const SimMesh_Color& aColor = theModel->GetTriangleColor(j);
            theFile->write((Standard_Character*)&aColor, sizeof(SimMesh_Color));
        }
    }
}

//! Read header from the stream.
static Handle(SimMesh_Model) readHeader(const std::shared_ptr<std::istream> theFile, const int32_t theVersion)
{
    // Identification info
    int32_t aVersion = 0;
    theFile->read((Standard_Character*)&aVersion, sizeof(int32_t));
    if (aVersion == 0 || aVersion > theVersion) {
        Message::SendFail() << "Error: unsupported mesh file version '" << aVersion << "' (supported version: '"
                            << theVersion << "')";
        return Handle(SimMesh_Model)();
    }
    int32_t aPhraseLen = 0;
    theFile->read((Standard_Character*)&aPhraseLen, sizeof(int32_t));
    std::string aPhrase(aPhraseLen, 0);
    theFile->read(&aPhrase[0], aPhraseLen);
    if (aPhrase != EtalonPhrase()) {
        Message::SendFail() << "Error: bad file content";
        return Handle(SimMesh_Model)();
    }

    // Numbers
    int32_t aNbVert = 0, aNbTria = 0, aNbEdges = 0;
    theFile->read((Standard_Character*)&aNbVert, sizeof(int32_t));
    theFile->read((Standard_Character*)&aNbTria, sizeof(int32_t));
    theFile->read((Standard_Character*)&aNbEdges, sizeof(int32_t));

    // Flags
    bool hasNormals = false, hasTCoords = false, hasColors = false, hasTriaColors = false;
    theFile->read((Standard_Character*)&hasNormals, sizeof(bool));
    theFile->read((Standard_Character*)&hasTCoords, sizeof(bool));
    theFile->read((Standard_Character*)&hasColors, sizeof(bool));
    theFile->read((Standard_Character*)&hasTriaColors, sizeof(bool));

    Handle(SimMesh_Model) aModel = new SimMesh_Model(aNbVert, aNbTria, aNbEdges);
    aModel->SetVertexNormals(hasNormals);
    aModel->SetVertexTextureCoords(hasTCoords);
    aModel->SetVertexColors(hasColors);
    aModel->SetTriangleColors(hasTriaColors);
    return aModel;
}

//! Reads vertices, triangles and edges from the stream.
static void readMeshEntities(const std::shared_ptr<std::istream> theFile, const Handle(SimMesh_Model)& theModel)
{
    // Vertices
    for (SimMesh_Id i = 0; i < theModel->GetNbVertices(); ++i) {
        SimMesh_Vertex aVert(0., 0., 0.);
        theFile->read((Standard_Character*)&aVert, sizeof(SimMesh_Vertex));
        theModel->AddVertex(aVert);
    }

    // Triangles
    for (SimMesh_Id i = 0; i < theModel->GetNbTriangles(); ++i) {
        SimMesh_Triangle aTria(0, 0, 0);
        theFile->read((Standard_Character*)&aTria, sizeof(SimMesh_Triangle));
        theModel->AddTriangle(aTria, false);
    }

    // Edges
    for (SimMesh_Id i = 0; i < theModel->GetNbEdges(); ++i) {
        SimMesh_Edge anEdge(0, 0);
        theFile->read((Standard_Character*)&anEdge, sizeof(SimMesh_Edge));
        theModel->AddEdge(anEdge, false);
    }
}

//! Reads vertex data list from the input stream.
static void readVertexDataList(const std::shared_ptr<std::istream> theFile,
                               SimMesh_VertexDataList&             theVDataList,
                               const bool                          theHasNormals,
                               const bool                          theHasTCoords,
                               const bool                          theHasColors)
{
    int32_t aNbData = 0;
    theFile->read((Standard_Character*)&aNbData, sizeof(int32_t));
    theVDataList.reserve(aNbData);

    for (int j = 0; j < aNbData; ++j) {
        if (j > 0) {
            // The data list with index 0 is created in SimMesh_Model::AddVertex().
            SimMesh_Attributes anAttrib(theHasNormals, theHasColors, theHasTCoords);
            theVDataList.emplace_back(anAttrib);
        }
        SimMesh_VertexData& aVData = theVDataList[j];

        int32_t aVDataNbTrias = 0;
        theFile->read((Standard_Character*)&aVDataNbTrias, sizeof(int32_t));
        aVData.Triangles.resize(aVDataNbTrias);
        for (int k = 0; k < aVDataNbTrias; ++k) {
            theFile->read((Standard_Character*)&aVData.Triangles[k], sizeof(SimMesh_Id));
        }

        int32_t aVDataNbEdges = 0;
        theFile->read((Standard_Character*)&aVDataNbEdges, sizeof(int32_t));
        aVData.Edges.resize(aVDataNbEdges);
        for (int k = 0; k < aVDataNbEdges; ++k) {
            theFile->read((Standard_Character*)&aVData.Edges[k], sizeof(SimMesh_Id));
        }

        SimMesh_Attributes& anAttr = aVData.Attrib;
        if (theHasNormals) {
            SimMesh_Vec3f& aNorm = anAttr.ChangeNormal();
            theFile->read((Standard_Character*)&aNorm, sizeof(SimMesh_Vec3f));
        }
        if (theHasColors) {
            SimMesh_Color& aColor = anAttr.ChangeColor();
            theFile->read((Standard_Character*)&aColor, sizeof(SimMesh_Color));
        }
        if (theHasTCoords) {
            SimMesh_Vec2f& aTCoord = anAttr.ChangeTexCoord();
            theFile->read((Standard_Character*)&aTCoord, sizeof(SimMesh_Vec2f));
        }
    }
}

//! Reads attributes from the stream.
static void readAttributes(const std::shared_ptr<std::istream> theFile, const Handle(SimMesh_Model)& theModel)
{
    // Vertex data lists
    for (SimMesh_Id i = 0; i < theModel->GetNbVertices(); ++i) {
        SimMesh_VertexDataList& aVDataList = theModel->ChangeVertexDataList(i);
        readVertexDataList(theFile,
                           aVDataList,
                           theModel->IsVertexNormals(),
                           theModel->IsVertexTextureCoords(),
                           theModel->IsVertexColors());
    }

    // Triangle colors
    if (theModel->IsTriangleColors()) {
        for (SimMesh_Id j = 0; j < theModel->GetNbTriangles(); ++j) {
            SimMesh_Color& aColor = theModel->ChangeTriangleColor(j);
            theFile->read((Standard_Character*)&aColor, sizeof(SimMesh_Color));
        }
    }
}
}

// =======================================================================
// function : Save
// =======================================================================
bool SimMesh_Persistence::Save(const Handle(SimMesh_Model)& theModel, const char* theFileName)
{
    std::shared_ptr<std::ostream> aFile = OSD_FileSystem::DefaultFileSystem()->OpenOStream(theFileName,
                                                                                           std::ios_base::out
                                                                                               | std::ios_base::binary);
    if (aFile == nullptr || !aFile->good()) {
        Message::SendFail() << "Error: cannot create file '" << theFileName << "'";
        return false;
    }

    writeHeader(aFile, CurrentVersion, theModel);
    writeMeshEntities(aFile, theModel);
    writeAttributes(aFile, theModel);

    return aFile->good();
}

// =======================================================================
// function : Load
// =======================================================================
Handle(SimMesh_Model) SimMesh_Persistence::Load(const char* theFileName)
{
    std::shared_ptr<std::istream> aFile = OSD_FileSystem::DefaultFileSystem()->OpenIStream(theFileName,
                                                                                           std::ios_base::out
                                                                                               | std::ios_base::binary);
    if (aFile == nullptr || !aFile->good()) {
        Message::SendFail() << "Error: cannot open file '" << theFileName << "'";
        return Handle(SimMesh_Model)();
    }

    Handle(SimMesh_Model) aModel = readHeader(aFile, CurrentVersion);
    readMeshEntities(aFile, aModel);
    readAttributes(aFile, aModel);

    return aModel;
}