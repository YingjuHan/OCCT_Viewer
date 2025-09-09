#include <SimMesh_Quadric.h>
#include <SimMesh_Simplifier.h>

#include <Precision.hxx>
#include <Standard_ShortReal.hxx>

namespace {
//! A value of confusion of points with single precision.
static constexpr float SIMMESH_SIMPLIFIER_SHORT_PRECISION = 1e-7f;

//! The penalty for moving a point of a free boundary.
static constexpr double SIMMESH_SIMPLIFIER_BOUNDARY_PENALTY = 100.;
}

// =======================================================================
// function : SimMesh_Simplifier
// =======================================================================
SimMesh_Simplifier::SimMesh_Simplifier()
{
}

// =======================================================================
// function : Perform
// =======================================================================
bool SimMesh_Simplifier::Perform()
{
    if (myModel.IsNull()) {
        return false;
    }

    myModel->InvalidateFreeVertices();
    createQuadrics();
    addBoundaryConstraints();

    if (myAllowedError < gp::Resolution() && myTargetRatio < gp::Resolution()) {
        myTargetRatio = 0.5;
    }
    myNbTargetEntities = int64_t(myNbValidEntities * myTargetRatio);
    return perform(true);
}

// =======================================================================
// function : calcTriangleQuadric
// =======================================================================
void SimMesh_Simplifier::calcTriangleQuadric(SimMesh_Id theTriaId, SimMesh_Quadric& theQuadric)
{
    double                  anArea;
    const SimMesh_Vec3d&    aNorm  = myModel->GetTriangleNormal(theTriaId, anArea);
    const SimMesh_Triangle& aTria  = myModel->GetTriangle(theTriaId);
    const SimMesh_Vertex&   aVert0 = myModel->GetVertex(aTria[0]);
    double                  aD     = -(aNorm.Dot(SimMesh_Vec3d(aVert0)));
    theQuadric.Init(aNorm.x(), aNorm.y(), aNorm.z(), aD, anArea);
}

// =======================================================================
// function : createQuadrics
// =======================================================================
void SimMesh_Simplifier::createQuadrics()
{
    for (SimMesh_Id aVIter = 0; aVIter < myModel->GetNbVertices(); ++aVIter) {
        if (myModel->IsVertexValid(aVIter)) {
            SimMesh_VertexDataList& aVIL = myModel->ChangeVertexDataList(aVIter);
            for (SimMesh_VertexData& aVData : aVIL) {
                aVData.Quadric.Clear();
            }
        }
    }

    for (SimMesh_Id aTriIter = 0; aTriIter < myModel->GetNbTriangles(); ++aTriIter) {
        if (!myModel->IsTriangleValid(aTriIter)) {
            continue;
        }
        const SimMesh_Triangle& aTria = myModel->GetTriangle(aTriIter);

        SimMesh_Quadric aQ;
        calcTriangleQuadric(aTriIter, aQ);
        aQ *= aQ.GetArea(); // weight by area

        for (unsigned int j = 0; j < 3; j++) {
            SimMesh_VertexDataList& aVDataList = myModel->ChangeVertexDataList(aTria[j]);
            auto                    itVData    = aVDataList.ChangeByTriangleId(aTriIter);
            if (itVData != aVDataList.end()) {
                itVData->Quadric += aQ;
            }
        }
    }
}

// =======================================================================
// function : restoreVertexPairInfos
// =======================================================================
void SimMesh_Simplifier::restoreVertexPairInfos()
{
    for (SimMesh_VertexPair& aPair : myVertexPairs) {
        if (aPair.IsInHeap() && aPair.HasBackup()) {
            myVertexPairHeap.Update(&aPair, aPair.BackupKey);
            aPair.ClearBackup();
        }
    }
}

// =======================================================================
// function : computeVertexPairInfo
// =======================================================================
void SimMesh_Simplifier::computeVertexPairInfo(SimMesh_VertexPair* thePair)
{
    const SimMesh_Id      aVertId1 = thePair->Vert1;
    const SimMesh_Id      aVertId2 = thePair->Vert2;
    const SimMesh_Vertex& aVert1   = myModel->GetVertex(aVertId1);
    const SimMesh_Vertex& aVert2   = myModel->GetVertex(aVertId2);

    double anErr = 0.0; // error introduced by this contraction

    // Check distance between vertices
    thePair->Distance = (aVert1 - aVert2).Modulus();
    if (thePair->Distance < SIMMESH_SIMPLIFIER_SHORT_PRECISION) {
        // For a null-length contraction use simple treatment
        thePair->TargetType = SimMesh_TargetType::UseFirst;
        thePair->TargetVec  = SimMesh_Vec3d(aVert1);
        // force this pair be the first among others with zero error
        anErr = -Precision::Confusion();
    } else {
        // Check if there is at least one surrounding triangle
        bool isTriangle = false;
        for (const auto& aVData : myModel->GetVertexDataList(aVertId1)) {
            if (!aVData.Triangles.empty()) {
                isTriangle = true;
                break;
            }
        }
        if (isTriangle) {
            anErr = computeErrorByQuadrics(thePair);
        }

        // Check if there is at least one surrounding edge
        bool isEdge = false;
        for (const auto& aVData : myModel->GetVertexDataList(aVertId1)) {
            if (!aVData.Edges.empty()) {
                isEdge = true;
                break;
            }
        }
        if (isEdge) {
            anErr = std::max(anErr, computeErrorByEdges(thePair));
        }
    }

    myVertexPairHeap.Update(thePair, (float)-anErr);
}

// =======================================================================
// function : computeErrorByQuadrics
// =======================================================================
double SimMesh_Simplifier::computeErrorByQuadrics(SimMesh_VertexPair* thePair)
{
    double           anErr    = 0.0;
    const SimMesh_Id aVertId1 = thePair->Vert1;
    const SimMesh_Id aVertId2 = thePair->Vert2;

    // Compute summary quadric
    SimMesh_Quadric aQ;
    myModel->GetVertexDataList(aVertId1).SumQuadric(aQ);
    myModel->GetVertexDataList(aVertId2).SumQuadric(aQ);

    // Choose optimal position for the merged vertex and evaluate error
    thePair->TargetType = SimMesh_TargetType::Undefined;
    if (aQ.Optimize(thePair->TargetVec)) {
        thePair->TargetType = SimMesh_TargetType::Optimized;
        anErr               = aQ.Evaluate(thePair->TargetVec);
    }

    if (thePair->TargetType == SimMesh_TargetType::Undefined) {
        // Fall back on one of endpoints
        const SimMesh_Vec3d aPos1(myModel->GetVertex(aVertId1));
        const SimMesh_Vec3d aPos2(myModel->GetVertex(aVertId2));
        const double        anErr1 = aQ.Evaluate(aPos1);
        const double        anErr2 = aQ.Evaluate(aPos2);
        if (anErr1 <= anErr2) {
            thePair->TargetType = SimMesh_TargetType::UseFirst;
            thePair->TargetVec  = aPos1;
            anErr               = anErr1;
        } else {
            thePair->TargetType = SimMesh_TargetType::UseSecond;
            thePair->TargetVec  = aPos2;
            anErr               = anErr2;
        }
    }

    if (anErr > 0.) {
        anErr /= aQ.GetArea(); // weight by area
        anErr = Sqrt(anErr);   // turn to geometric error
    } else {
        anErr = 0.;
    }
    return anErr;
}

// =======================================================================
// function : computeErrorByEdges
// =======================================================================
double SimMesh_Simplifier::computeErrorByEdges(SimMesh_VertexPair* thePair)
{
    const SimMesh_Id    aVertId1 = thePair->Vert1;
    const SimMesh_Id    aVertId2 = thePair->Vert2;
    const SimMesh_Vec3d aPos1(myModel->GetVertex(aVertId1));
    const SimMesh_Vec3d aPos2(myModel->GetVertex(aVertId2));

    if (thePair->TargetType == SimMesh_TargetType::Undefined) {
        // Choose the best position among 3: the end points and the middle
        SimMesh_VertexPair aPairs[3] = {*thePair, *thePair, *thePair};
        aPairs[0].TargetType         = SimMesh_TargetType::Optimized;
        aPairs[0].TargetVec          = (aPos1 + aPos2) * 0.5;
        aPairs[1].TargetType         = SimMesh_TargetType::UseFirst;
        aPairs[1].TargetVec          = aPos1;
        aPairs[2].TargetType         = SimMesh_TargetType::UseSecond;
        aPairs[2].TargetVec          = aPos2;
        double anErr                 = thePair->Distance;
        int    iMin                  = 0;
        for (int i = 0; i < 3; ++i) {
            double anErr1 = computeErrorByEdges(&aPairs[i]);
            if (anErr > anErr1) {
                anErr = anErr1;
                iMin  = i;
            }
        }
        *thePair = aPairs[iMin];
        return anErr;
    }

    double anErr1 = computeErrorByMovement(aVertId1, aPos1, thePair->TargetVec, aVertId2);
    double anErr2 = computeErrorByMovement(aVertId2, aPos2, thePair->TargetVec, aVertId1);
    return Max(anErr1, anErr2);
}

// =======================================================================
// function : computeErrorByMovement
// =======================================================================
double SimMesh_Simplifier::computeErrorByMovement(const SimMesh_Id     theVertId,
                                                  const SimMesh_Vec3d& theOldPos,
                                                  const SimMesh_Vec3d& theNewPos,
                                                  const SimMesh_Id     theVertIdExclude)
{
    double       anErr   = 0.;
    const double aSqPrec = Precision::SquareConfusion();
    for (const SimMesh_VertexData& aVData : myModel->GetVertexDataList(theVertId)) {
        for (SimMesh_Id anEdgeId : aVData.Edges) {
            const SimMesh_Edge& anEdge     = myModel->GetEdge(anEdgeId);
            const SimMesh_Id    aVertOppId = anEdge.OppositeNode(theVertId);
            if (aVertOppId == theVertIdExclude) {
                continue;
            }
            const SimMesh_Vec3d aPosOpp(myModel->GetVertex(aVertOppId));
            const SimMesh_Vec3d aNewEdgeVec(theNewPos - aPosOpp);
            const SimMesh_Vec3d anOldEdgeVec(theOldPos - aPosOpp);
            const double        aNewLen = aNewEdgeVec.SquareModulus();
            if (aNewLen > aSqPrec) {
                const double anErr1 = SimMesh_Vec3d::Cross(anOldEdgeVec, aNewEdgeVec).SquareModulus() / aNewLen;
                anErr               = std::max(anErr, anErr1);
            }
        }
    }
    anErr = Sqrt(anErr);
    return anErr;
}

// =======================================================================
// function : checkInversion
// =======================================================================
bool SimMesh_Simplifier::checkInversion(SimMesh_VertexPair* thePair, double& theDeltaArea)
{
    // Check that for each surrounding triangle the target vertex position
    // does not invert the normal vector.
    // Return true if at least one condition fails.
    // By the way compute the change of local area.
    myTmpTriangles.clear();
    myModel->gatherNeighboringTriangles(thePair->Vert1, thePair->Vert2, myTmpTriangles);
    const SimMesh_Vertex aNewPos((float)thePair->TargetVec[0],
                                 (float)thePair->TargetVec[1],
                                 (float)thePair->TargetVec[2]);
    double               anInitArea  = 0.;
    double               aNewArea    = 0.;
    const double         aSqPrec     = Precision::SquareConfusion();
    bool                 isInversion = false;
    for (size_t aTriIter = 0; aTriIter < myTmpTriangles.size(); ++aTriIter) {
        const SimMesh_Id        aTriaId   = myTmpTriangles[aTriIter];
        const SimMesh_Triangle& aTria     = myModel->GetTriangle(aTriaId);
        int                     anOppInd1 = -1, anOppInd2 = -1;
        for (int i = 0; i < 3; ++i) {
            if (aTria[i] == thePair->Vert1 || aTria[i] == thePair->Vert2) {
                anOppInd1 = (i + 1) % 3;
                anOppInd2 = (i + 2) % 3;
                break;
            }
        }
        Standard_ASSERT_VOID(anOppInd1 >= 0, "Opposite edge is not found");
        if (anOppInd1 < 0) {
            continue;
        }

        double               aTriArea = 0.0;
        const SimMesh_Vec3d& aTriNorm = myModel->GetTriangleNormal(aTriaId, aTriArea);

        const bool isDeadTriangle = (aTria[anOppInd1] == thePair->Vert1 || aTria[anOppInd1] == thePair->Vert2
                                     || aTria[anOppInd2] == thePair->Vert1 || aTria[anOppInd2] == thePair->Vert2);
        if (!isDeadTriangle) {
            const SimMesh_Vertex& anOppVert1 = myModel->GetVertex(aTria[anOppInd1]);
            const SimMesh_Vertex& anOppVert2 = myModel->GetVertex(aTria[anOppInd2]);
            const SimMesh_Vec3d   aVecE(anOppVert2 - anOppVert1);
            const SimMesh_Vec3d   aVecNewPos(aNewPos - anOppVert1);
            const SimMesh_Vec3d   aNewTriNorm = SimMesh_Vec3d::Cross(aVecE, aVecNewPos);
            double                aTriNewArea = aNewTriNorm.Modulus() * 0.5;
            if (aTriArea > aSqPrec && (aTriNewArea < aSqPrec || aNewTriNorm.Dot(aTriNorm) < 0.)) {
                isInversion = true;
            }
            aNewArea += aTriNewArea;
        }
        anInitArea += aTriArea;
    }
    theDeltaArea = Abs(anInitArea - aNewArea);
    return isInversion;
}

// =======================================================================
// function : iterateConnectedVertexPairs
// =======================================================================
void SimMesh_Simplifier::iterateConnectedVertexPairs(std::function<void(SimMesh_Id, SimMesh_Id)> theFunc)
{
    SimMesh_IdList aStar(10);
    for (SimMesh_Id aVertIter = 0; aVertIter < myModel->GetNbVertices(); ++aVertIter) {
        if (!myModel->IsVertexValid(aVertIter)) {
            continue;
        }
        aStar.clear();
        myModel->gatherAdjacentVertices(aVertIter, aStar);
        const size_t aStarSize = aStar.size();
        for (size_t j = 0; j < aStarSize; ++j) {
            if (aVertIter >= aStar[j]) { // ignore order of vertices in a pair
                continue;
            }
            theFunc(aVertIter, aStar[j]);
        }
    }
}

// =======================================================================
// function : addContractionsFromEdges
// =======================================================================
float SimMesh_Simplifier::addContractionsFromEdges()
{
    float aMinLength = ShortRealLast();
    iterateConnectedVertexPairs([this, &aMinLength](SimMesh_Id theVert1, SimMesh_Id theVert2) {
        myVertexPairs.emplace_back();
        SimMesh_VertexPair* pPair = &myVertexPairs.back();

        pPair->Vert1 = theVert1;
        pPair->Vert2 = theVert2;

        myVertexPairLinks[pPair->Vert1].push_back(pPair);
        myVertexPairLinks[pPair->Vert2].push_back(pPair);

        computeVertexPairInfo(pPair);
        aMinLength = std::min(aMinLength, pPair->Distance);
    });
    return aMinLength;
}

// =======================================================================
// function : addContractionsFromVirtualEdges
// =======================================================================
void SimMesh_Simplifier::addContractionsFromVirtualEdges(NCollection_CellFilter<VertexFinder>& theCells,
                                                         const float                           theDistThreshold)
{
    SimMesh_IdList anAvoidVertices(6);
    const float    aSqDistThreshold = myAggregation ? theDistThreshold * theDistThreshold : FLT_MIN;
    for (SimMesh_Id aVertId = 0; aVertId < myModel->GetNbVertices(); ++aVertId) {
        if (!myModel->IsVertexValid(aVertId)) {
            continue;
        }

        // Identify vertices from existing pairs and put them in avoid list
        anAvoidVertices.clear();
        gatherAdjacentVerticesFromVertexPairs(aVertId, anAvoidVertices);

        // Find all (except avoided) vertices located on the distance less than
        // theDistThreshold from the current vertex.
        const SimMesh_Vertex& aVert = myModel->GetVertex(aVertId);
        VertexFinder          aVertexFinder(myModel, aVertId, anAvoidVertices, aSqDistThreshold);
        if (myAggregation) {
            theCells.Inspect(VertexFinder::Shift(aVert, -theDistThreshold),
                             VertexFinder::Shift(aVert, theDistThreshold),
                             aVertexFinder);
        } else {
            theCells.Inspect(aVert, aVertexFinder);
        }

        // Form new pairs for the current vertex
        const SimMesh_IdList& aCollVerts = aVertexFinder.GetGatheredVertices();
        for (SimMesh_Id aVertIdOpp : aCollVerts) {
            myVertexPairs.emplace_back();
            SimMesh_VertexPair* pPair = &myVertexPairs.back();

            pPair->Vert1 = aVertId;
            pPair->Vert2 = aVertIdOpp;

            myVertexPairLinks[pPair->Vert1].push_back(pPair);
            myVertexPairLinks[pPair->Vert2].push_back(pPair);

            computeVertexPairInfo(pPair);
        }
        theCells.Add(aVertId, aVert);
    }
}

// =======================================================================
// function : gatherAdjacentVerticesFromVertexPairs
// =======================================================================
void SimMesh_Simplifier::gatherAdjacentVerticesFromVertexPairs(SimMesh_Id theVertId, SimMesh_IdList& theVerts)
{
    for (const SimMesh_VertexPair* aPair : myVertexPairLinks[theVertId]) {
        const SimMesh_Id anOppVertId = aPair->Vert1 == theVertId ? aPair->Vert2 : aPair->Vert1;
        theVerts.push_back(anOppVertId);
    }
}

// =======================================================================
// function : defineBoundaryConstraint
// =======================================================================
void SimMesh_Simplifier::defineBoundaryConstraint(SimMesh_Id            theVert1,
                                                  SimMesh_Id            theVert2,
                                                  const SimMesh_IdList& theTriangles)
{
    const SimMesh_Vec3d aFrom(myModel->GetVertex(theVert1));
    const SimMesh_Vec3d aDest(myModel->GetVertex(theVert2));
    const SimMesh_Vec3d aVecE = aDest - aFrom;

    const size_t aTrianglesSize = theTriangles.size();
    for (size_t aTriIter = 0; aTriIter < aTrianglesSize; ++aTriIter) {
        const SimMesh_Id     aTriaId  = theTriangles[aTriIter];
        double               anArea   = 0.;
        const SimMesh_Vec3d& aTriNorm = myModel->GetTriangleNormal(aTriaId, anArea);

        SimMesh_Vec3d n2 = SimMesh_Vec3d::Cross(aVecE, aTriNorm);
        n2.Normalize();

        SimMesh_Quadric aQ(n2, -(n2.Dot(aFrom)), anArea);
        aQ += SimMesh_Quadric(aTriNorm, -(aTriNorm.Dot(aFrom)), 0);
        aQ *= anArea * SIMMESH_SIMPLIFIER_BOUNDARY_PENALTY;

        SimMesh_VertexDataList::iterator aVIter = myModel->ChangeVertexDataList(theVert1).ChangeByTriangleId(aTriaId);
        Standard_ASSERT_VOID(aVIter != myModel->GetVertexDataList(theVert1).end(), "");
        aVIter->Quadric += aQ;

        aVIter = myModel->ChangeVertexDataList(theVert2).ChangeByTriangleId(aTriaId);
        Standard_ASSERT_VOID(aVIter != myModel->GetVertexDataList(theVert2).end(), "");
        aVIter->Quadric += aQ;
    }
}

// =======================================================================
// function : addBoundaryConstraints
// =======================================================================
void SimMesh_Simplifier::addBoundaryConstraints()
{
    SimMesh_IdList aTriangles(6);
    SimMesh_IdList anEdges(6);
    iterateConnectedVertexPairs([this, &aTriangles, &anEdges](SimMesh_Id theVert1, SimMesh_Id theVert2) {
        aTriangles.clear();
        anEdges.clear();
        myModel->gatherEdgeNeighbors(theVert1, theVert2, aTriangles, anEdges);
        if (aTriangles.size() == 1 || anEdges.size() > 0) {
            defineBoundaryConstraint(theVert1, theVert2, aTriangles);
        }
    });
}

// =======================================================================
// function : Initialize
// =======================================================================
void SimMesh_Simplifier::Initialize(const Handle(SimMesh_Model)& theModel)
{
    myModel           = theModel;
    myNbValidEntities = 0;

    if (myModel.IsNull()) {
        return;
    }

    if (theModel->GetNbTriangles() > 0) {
        // there are triangles
        for (SimMesh_Id aTriIter = 0; aTriIter < theModel->GetNbTriangles(); ++aTriIter) {
            if (theModel->IsTriangleValid(aTriIter)) {
                ++myNbValidEntities;
            }
        }
    } else {
        // there are edges only
        for (SimMesh_Id anEdgeIter = 0; anEdgeIter < theModel->GetNbEdges(); ++anEdgeIter) {
            if (theModel->IsEdgeValid(anEdgeIter)) {
                ++myNbValidEntities;
            }
        }
    }
}

// =======================================================================
// function : doContraction
// =======================================================================
void SimMesh_Simplifier::doContraction(const SimMesh_VertexPair* thePair)
{
    myNbValidEntities -= (int64_t)(myModel->GetNbTriangles() > 0 ? myContraction.DeadTriangles.size()
                                                                 : myContraction.DeadEdges.size());

    // Update myVertexPairLinks
    updateBeforeContraction();

    // Apply contraction in the model
    myModel->performContraction(myContraction, thePair->TargetType, thePair->TargetVec);

    const SimMesh_Id aVert1 = myContraction.Vert1;
    SimMesh_IdList   aRemovedVertices(6);
    if (myModel->IsVertexValid(aVert1)) {
        // vertex 1 remains in the mesh

        // Must update SimMesh_VertexPair here so that the meshing penalties
        // will be computed with respect to the new mesh rather than the old
        SimMesh_VertexPairList& aPairs = myVertexPairLinks[aVert1];
        for (SimMesh_VertexPair* aPair : aPairs) {
            const SimMesh_Id aVertOpp = (aVert1 == aPair->Vert1 ? aPair->Vert2 : aPair->Vert1);
            if (myModel->IsVertexValid(aVertOpp)) {
                computeVertexPairInfo(aPair);
            } else {
                aRemovedVertices.push_back(aVertOpp);
            }
        }
    } else {
        aRemovedVertices.push_back(aVert1);
    }

    // Remove pairs containing removed vertices
    for (const SimMesh_Id aVertId : aRemovedVertices) {
        for (SimMesh_VertexPair* aPair : myVertexPairLinks[aVertId]) {
            const SimMesh_Id anOppVertId = aPair->Vert1 == aVertId ? aPair->Vert2 : aPair->Vert1;

            myVertexPairLinks[anOppVertId].RemoveItem(aPair);
            myVertexPairHeap.Remove(aPair);
        }
        myVertexPairLinks[aVertId].clear();
    }
}

// =======================================================================
// function : updateBeforeContraction
// =======================================================================
void SimMesh_Simplifier::updateBeforeContraction()
{
    const SimMesh_Id aVert1 = myContraction.Vert1;
    const SimMesh_Id aVert2 = myContraction.Vert2;
    myTmpVertices.clear();
    gatherAdjacentVerticesFromVertexPairs(aVert1, myTmpVertices);
    const size_t aLinksSize = myVertexPairLinks[aVert2].size();
    for (size_t i = 0; i < aLinksSize; ++i) {
        SimMesh_VertexPair* aPair   = myVertexPairLinks[aVert2][i];
        SimMesh_Id          aVertId = aPair->Vert1 == aVert2 ? aPair->Vert2 : aPair->Vert1;
        Standard_ASSERT_VOID(aPair->Vert1 == aVert2 || aPair->Vert2 == aVert2, "");
        Standard_ASSERT_VOID(aVertId != aVert2, "");

        if (aVertId == aVert1
            || std::find(myTmpVertices.cbegin(), myTmpVertices.cend(), aVertId) != myTmpVertices.cend()) {
            // remove useless link
            myVertexPairLinks[aVertId].RemoveItem(aPair);
            myVertexPairHeap.Remove(aPair);
        } else {
            // Relink this to aVert1
            aPair->Vert1 = aVert1;
            aPair->Vert2 = aVertId;
            myVertexPairLinks[aVert1].push_back(aPair);
        }
    }

    myVertexPairLinks[aVert2].clear();
}

// =======================================================================
// function : perform
// =======================================================================
bool SimMesh_Simplifier::perform(bool theNewStart)
{
    NCollection_CellFilter<VertexFinder> aCells;
    if (theNewStart && !initVertexPairs(aCells)) {
        return false;
    }

    // The main loop. Make one vertex pair contraction at one iteration.
    bool isOK        = true;
    myWasContraction = false;
    for (; myNbIterLimit == 0 || myIterNum < myNbIterLimit; // Stop on reaching limit of nb iterations
         ++myIterNum) {
        if (myNbTargetEntities > 0 && myNbValidEntities <= myNbTargetEntities) {
            // stop on reaching target number of triangles
            break;
        }

        SimMesh_VertexPair* aCurPair = myVertexPairHeap.Top();
        if (aCurPair == nullptr && !myAggregation) {
            isOK = false;
            break;
        }

        const float anError = aCurPair == nullptr ? 0.f : -aCurPair->HeapKey;
        if (myAllowedError > 0. && anError >= myAllowedError) {
            // stop on reaching the maximal error
            break;
        }

        if (aCurPair == nullptr || anError > myDistThreshold) {
            if (!increaseThersholdAndReinit(aCells)) {
                isOK = false;
                break;
            }
            continue;
        }

        // Check the pair for validity of its contraction.
        bool isInversion = false;
        if (!checkVertexPair(aCurPair, isInversion)) {
            continue;
        }

        // Perform contraction
        contractVertexPair(aCurPair, anError, isInversion);
#ifdef CHECK_MODEL
        Standard_ASSERT_VOID(myModel->IsModelConsistent(), "");
#endif
    }

    if (myNbValidEntities <= myNbTargetEntities) {
        myVertexPairLinks.clear();
        myVertexPairHeap.clear();
        myVertexPairs.clear();
    }
    myModel->setMaxError(myMaxError);
    return isOK;
}

// =======================================================================
// function : initVertexPairs
// =======================================================================
bool SimMesh_Simplifier::initVertexPairs(NCollection_CellFilter<VertexFinder>& theCells)
{
    myVertexPairLinks.clear();
    myVertexPairLinks.resize(myModel->GetNbVertices());
    for (SimMesh_VertexPairList& aListIter : myVertexPairLinks) {
        aListIter.reserve(8);
    };
    myVertexPairHeap.clear();
    myVertexPairHeap.reserve(myModel->GetNbVertices());
    myVertexPairs.clear();

    // Get bounding box of the model
    const Bnd_B3f& aBndBox = myModel->GetBndBox();
    const gp_XYZ   aBndVec = aBndBox.CornerMax() - aBndBox.CornerMin();
    myBndExtent            = float(aBndVec.Modulus());

    // Collect all real edges in the model as vertex pairs for contraction.
    myDistThreshold = addContractionsFromEdges();

    // Set the minimal edge length as the initial distance threshold.
    // Then populate cells with vertices creating non-adjacent vertex pairs.
    // If myDistThreshold is too small it means there are degenerated real edges,
    // and they are to be contracted first, so skip creation of virtual edges.
    myDistThreshold = std::max(myDistThreshold, SIMMESH_SIMPLIFIER_SHORT_PRECISION);
    if (!myAggregation || myDistThreshold > SIMMESH_SIMPLIFIER_SHORT_PRECISION) {
        theCells.Reset(myDistThreshold);
        addContractionsFromVirtualEdges(theCells, myAggregation ? myDistThreshold : 0.f);
    }

    myIterNum      = 0;
    myMaxError     = 0.;
    myNbInversions = 0;
    return !myVertexPairs.empty();
}

// =======================================================================
// function : checkVertexPair
// =======================================================================
bool SimMesh_Simplifier::checkVertexPair(SimMesh_VertexPair* thePair, bool& theIsInversion)
{
    // Invalidate the pair in case of triangle inversion
    double anAreaChange = 0.0;
    theIsInversion      = checkInversion(thePair, anAreaChange);
    if (theIsInversion) {
        if (myAggregation && myAllowInversion) {
            // In aggregation mode we allow inversion, but postpone it as a last resort;
            // if the pair has already a backup then the time comes
            if (!thePair->HasBackup()) {
                thePair->Backup();
                myVertexPairHeap.Update(thePair, -myDistThreshold * 0.99f);
                return false;
            }
        } else {
            thePair->Backup();
            myVertexPairHeap.Update(thePair, -myDistThreshold * 2.f);
            return false;
        }
    }
    // Invalidate the pair in case of big change of surface area
    anAreaChange = Sqrt(anAreaChange / M_PI);
    if (anAreaChange > myDistThreshold) {
        float anErr = (float)anAreaChange;
        anErr *= 1.0001f; // ensure to be >myDistThreshold after conversion from double
        thePair->Backup();
        myVertexPairHeap.Update(thePair, -anErr);
        return false;
    }
    return true;
}

// =======================================================================
// function : contractVertexPair
// =======================================================================
void SimMesh_Simplifier::contractVertexPair(const SimMesh_VertexPair* thePair,
                                            const float               theError,
                                            const bool                theIsInversion)
{
    myVertexPairHeap.Extract();
    const SimMesh_Id aVertId1 = thePair->Vert1;
    const SimMesh_Id aVertId2 = thePair->Vert2;
    if (myModel->IsVertexValid(aVertId1) && myModel->IsVertexValid(aVertId2)) {
        myModel->createContraction(aVertId1, aVertId2, myContraction);
        doContraction(thePair);
    }
    if (myMaxError < theError) {
        myMaxError = theError;
    }
    if (theIsInversion) {
        ++myNbInversions;
    }
    myWasContraction = true;
}

// =======================================================================
// function : increaseThersholdAndReinit
// =======================================================================
bool SimMesh_Simplifier::increaseThersholdAndReinit(NCollection_CellFilter<VertexFinder>& theCells)
{
    // Double the threshold.
    myDistThreshold *= 2.f;

    // If the threshold is too high stop the process.
    if (myDistThreshold > myBndExtent) {
        return false;
    }

    if (myAggregation && myWasContraction) {
        // Collect new vertex pairs with increased threshold
        theCells.Reset(myDistThreshold);
        addContractionsFromVirtualEdges(theCells, myDistThreshold);
        restoreVertexPairInfos();
        myWasContraction = false;
    }
    return true;
}
