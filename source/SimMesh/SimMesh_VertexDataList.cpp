#include <SimMesh_VertexDataList.h>

// =======================================================================
// function : SumQuadric
// =======================================================================
void SimMesh_VertexDataList::SumQuadric(SimMesh_Quadric& theSum) const
{
    size_t aSize = size();
    Standard_ASSERT_VOID(aSize > 0, "");
    const SimMesh_VertexData* aVData = data();
    for (size_t i = 0; i < aSize; ++i) {
        theSum += aVData[i].Quadric;
    }
}

// =======================================================================
// function : ChangeByTriangleId
// =======================================================================
SimMesh_VertexDataList::iterator SimMesh_VertexDataList::ChangeByTriangleId(const SimMesh_Id theTriaId)
{
    iterator anIter = begin();
    for (; anIter != end(); ++anIter) {
        if (anIter->Triangles.Contains(theTriaId)) {
            break;
        }
    }
    return anIter;
}

// =======================================================================
// function : FindByTriangleId
// =======================================================================
SimMesh_VertexDataList::const_iterator SimMesh_VertexDataList::FindByTriangleId(const SimMesh_Id theTriaId) const
{
    const_iterator anIter = begin();
    for (; anIter != end(); ++anIter) {
        if (anIter->Triangles.Contains(theTriaId)) {
            break;
        }
    }
    return anIter;
}

// =======================================================================
// function : ChangeByEdgeId
// =======================================================================
SimMesh_VertexDataList::iterator SimMesh_VertexDataList::ChangeByEdgeId(const SimMesh_Id theEdgeId)
{
    iterator anIter = begin();
    for (; anIter != end(); ++anIter) {
        if (anIter->Edges.Contains(theEdgeId)) {
            break;
        }
    }
    return anIter;
}

// =======================================================================
// function : FindByEdgeId
// =======================================================================
SimMesh_VertexDataList::const_iterator SimMesh_VertexDataList::FindByEdgeId(const SimMesh_Id theEdgeId) const
{
    const_iterator anIter = begin();
    for (; anIter != end(); ++anIter) {
        if (anIter->Edges.Contains(theEdgeId)) {
            break;
        }
    }
    return anIter;
}

// =======================================================================
// function : Merge
// =======================================================================
void SimMesh_VertexDataList::Merge(SimMesh_VertexDataList&  theOther,
                                   const SimMesh_IdList&    theDeadTrias,
                                   const SimMesh_IdList&    theDeadEdges,
                                   const SimMesh_TargetType theTargetType,
                                   const double             theInterParam)
{
    mergeEdges(theOther, theDeadEdges);
    mergeTriangles(theOther, theDeadTrias, theTargetType, theInterParam);

    // Move remaining vertex data from the other list to this one
    insert(cend(), theOther.begin(), theOther.end());
}

// =======================================================================
// function : mergeEdges
// =======================================================================
void SimMesh_VertexDataList::mergeEdges(SimMesh_VertexDataList& theOther, const SimMesh_IdList& theDeadEdges)
{
    auto moveRemainingEdges = [&theDeadEdges](SimMesh_VertexDataList::iterator theIter1,
                                              SimMesh_VertexDataList::iterator theIter2) {
        for (SimMesh_Id anEdgeId2 : theIter2->Edges) {
            if (!theDeadEdges.Contains(anEdgeId2)) {
                theIter1->Edges.push_back(anEdgeId2);
            }
        }
    };
    // Iterate by dead edges
    for (SimMesh_Id anEdgeId : theDeadEdges) {
        // Find vertex data containing the dead edge
        SimMesh_VertexDataList::iterator aVIter1 = ChangeByEdgeId(anEdgeId);
        SimMesh_VertexDataList::iterator aVIter2 = theOther.ChangeByEdgeId(anEdgeId);

        if (aVIter1 != end()) {
            // Remove dead edge ID from the edge list of this vertex data
            aVIter1->Edges.RemoveItem(anEdgeId);
        }

        if (aVIter2 != theOther.end()) {
            // Remove dead edge ID from the edge list of the other vertex data
            aVIter2->Edges.RemoveItem(anEdgeId);

            if (aVIter1 != end()) {
                // Move remaining edges from other vertex data to this one
                moveRemainingEdges(aVIter1, aVIter2);
                aVIter2->Edges.clear();
            }

            if (aVIter2->Edges.empty() && aVIter2->Triangles.empty()) {
                theOther.erase(aVIter2);
            }
        }

        if (aVIter1 != end() && aVIter1->Triangles.empty() && aVIter1->Edges.empty()) {
            erase(aVIter1);
        }
    }
}

// =======================================================================
// function : mergeTriangles
// =======================================================================
void SimMesh_VertexDataList::mergeTriangles(SimMesh_VertexDataList&  theOther,
                                            const SimMesh_IdList&    theDeadTrias,
                                            const SimMesh_TargetType theTargetType,
                                            const double             theInterParam)
{
    // Dead triangles are the triangles common between the two vertices

    auto moveRemainingTriangles = [&theDeadTrias](SimMesh_VertexDataList::iterator theIter1,
                                                  SimMesh_VertexDataList::iterator theIter2) {
        for (SimMesh_Id aTriaId2 : theIter2->Triangles) {
            if (!theDeadTrias.Contains(aTriaId2)) {
                theIter1->Triangles.push_back(aTriaId2);
            }
        }
    };
    auto mergeVertexData = [&theTargetType, &theInterParam](SimMesh_VertexDataList::iterator theIter1,
                                                            SimMesh_VertexDataList::iterator theIter2) {
        if (theTargetType == SimMesh_TargetType::UseSecond) {
            theIter1->Attrib  = theIter2->Attrib;
            theIter1->Quadric = theIter2->Quadric;
        } else if (theTargetType == SimMesh_TargetType::Optimized) {
            theIter1->Attrib.Interpolate(theIter2->Attrib, theInterParam);
        }
    };
    // Iterate by dead triangles
    for (SimMesh_Id aTriaId : theDeadTrias) {
        // Find vertex data containing the dead triangle
        SimMesh_VertexDataList::iterator aVIter1 = ChangeByTriangleId(aTriaId);
        SimMesh_VertexDataList::iterator aVIter2 = theOther.ChangeByTriangleId(aTriaId);

        if (aVIter1 != end()) {
            // Remove dead triangle ID from the triangle list of this vertex data
            aVIter1->Triangles.RemoveItem(aTriaId);
        }

        if (aVIter2 != theOther.end()) {
            // Remove dead triangle ID from the triangle list of the other vertex data
            aVIter2->Triangles.RemoveItem(aTriaId);

            if (aVIter1 != end()) {
                // Add quadric of the other vertex data to this one
                aVIter1->Quadric += aVIter2->Quadric;

                // Move remaining triangles from other vertex data to this one
                moveRemainingTriangles(aVIter1, aVIter2);

                // Move remaining edges from other vertex data to this one
                aVIter1->Edges.insert(aVIter1->Edges.end(), aVIter2->Edges.begin(), aVIter2->Edges.end());

                // Merge attributes and quadric
                mergeVertexData(aVIter1, aVIter2);

                aVIter2->Edges.clear();
                aVIter2->Triangles.clear();
            }

            if (aVIter2->Triangles.empty()) {
                // Remove vertex data from the other list
                theOther.erase(aVIter2);
            }
        }

        if (aVIter1 != end() && aVIter1->Triangles.empty() && aVIter1->Edges.empty()) {
            erase(aVIter1);
        }
    }
}

