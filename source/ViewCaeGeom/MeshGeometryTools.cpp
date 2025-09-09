#include <ViewCaeGeom/MeshGeometryTools.h>

#include <ViewCaeGeom/MeshGeometryBlock.h>
#include <ViewCaeGeom/MeshGeometryClip.h>
#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>
#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <RWCae/MeshTools.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>

namespace {
static constexpr const char* NAME_ID   = "id";
static constexpr const char* NAME_TYPE = "type";

//! Get a mesh geometry from map (or create a new one).
bool RestoreGeom(const Handle(RWCae::MeshData)&         theMeshData,
                 const Handle(ViewCaeGeom::MeshBvhData)& theBvhData,
                 const TCollection_AsciiString&           theId,
                 ViewCaeGeom::MeshGeometryType           theType,
                 ViewCaeGeom::MapOfMeshGeometries&       theGeoms,
                 Handle(ViewCaeGeom::MeshGeometry)&      theBaseGeom,
                 Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                 bool&                                    theIsChanged)
{
    bool toCreate = false;
    if (theGeom.IsNull()) {
        toCreate    = !theGeoms.Find(theId, theGeom);
        theBaseGeom = theGeom;
    } else if (theGeom->GetChild().IsNull() || theGeom->GetChild()->Id() != theId) {
        theGeom->RemoveChildren();
        toCreate = true;
    } else {
        theGeom = theGeom->GetChild();
    }

    if (toCreate) {
        Handle(ViewCaeGeom::MeshGeometry) aGeom = ViewCaeGeom::MeshGeometryFactory::Create(theMeshData,
                                                                                             theBvhData,
                                                                                             theType);
        if (aGeom.IsNull()) {
            Message::SendFail() << "MeshGeometryTools::RestoreFromJson: unknown geometry type '" << (int)theType << "'";
            return false;
        }

        aGeom->SetId(theId);

        if (theGeom.IsNull()) {
            theGeoms.Bind(theId, aGeom);
            theGeom     = aGeom;
            theBaseGeom = theGeom;
        } else {
            theGeom->Append(aGeom);
            theGeom = aGeom;
        }

        theIsChanged = true;
    }

    return true;
}
}

namespace ViewCaeGeom {
// =======================================================================
// function : DumpToJson
// =======================================================================
bool MeshGeometryTools::DumpToJson(const Handle(MeshGeometry)& theGeom, std::string& theJsonStr)
{
    std::ostringstream           aStr;
    rapidjson::OStreamWrapper    aStrStream(aStr);
    RWBase::JsonHelper::Writer aWriter(aStrStream);

    aWriter.StartArray();

    for (MeshGeometry* aGeom : MeshGeomChildren(theGeom.get())) {
        aWriter.StartObject();

        aWriter.Key(NAME_ID);
        aWriter.String(aGeom->Id().ToCString());

        aWriter.Key(NAME_TYPE);
        aWriter.Int((int)aGeom->Type());

        aGeom->ToJson(aWriter);

        aWriter.EndObject();
    }

    aWriter.EndArray();

    aStr.flush();
    if (!aStr.good()) {
        Message::SendFail() << "MeshGeometryTools::DumpToJson: cannot write mesh geometry parameters";
        return false;
    }

    theJsonStr = aStr.str();

    return true;
}

// =======================================================================
// function : RestoreFromJson
// =======================================================================
bool MeshGeometryTools::RestoreFromJson(const std::string&               theJsonStr,
                                        const Handle(RWCae::MeshData)& theMeshData,
                                        const Handle(MeshBvhData)&       theBvhData,
                                        int                              theTime,
                                        MapOfMeshGeometries&             theGeoms,
                                        Handle(MeshGeometry)&            theGeom,
                                        bool&                            theIsChanged)
{
    rapidjson::Document aReader;
    if (!RWBase::JsonHelper::ParseFromString(theJsonStr, aReader)) {
        Message::SendFail() << "MeshGeometryTools::RestoreFromJson: cannot parse json";
        return false;
    }

    theIsChanged = false;

    Handle(MeshGeometry) aCurGeom;

    for (rapidjson::Value::ConstValueIterator anIter = aReader.Begin(); anIter != aReader.End(); ++anIter) {
        const RWBase::JsonHelper::Value* anIdField  = RWBase::JsonHelper::FindStringMember(*anIter, NAME_ID);
        const RWBase::JsonHelper::Value* aTypeField = RWBase::JsonHelper::FindIntMember(*anIter, NAME_TYPE);
        if (anIdField == nullptr || aTypeField == nullptr) {
            Message::SendFail() << "MeshGeometryTools::RestoreFromJson: cannot parse required fields";
            return false;
        }

        TCollection_AsciiString anId  = anIdField->GetString();
        MeshGeometryType        aType = (MeshGeometryType)aTypeField->GetInt();

        if (!RestoreGeom(theMeshData, theBvhData, anId, aType, theGeoms, theGeom, aCurGeom, theIsChanged)) {
            return false;
        }

        if (!aCurGeom->FromJson(*anIter, theMeshData, theTime, theIsChanged)) {
            Message::SendFail() << "MeshGeometryTools::RestoreFromJson: cannot parse parameters for geometry '"
                                << (int)aType << "'";
            return false;
        }
    }

    if (!aCurGeom->GetChild().IsNull()) {
        aCurGeom->RemoveChildren();
        theIsChanged = true;
    }

    if (theIsChanged) {
        theGeom->SetToUpdate();
    }

    return true;
}

// =======================================================================
// function : GatherUsedProps
// =======================================================================
void MeshGeometryTools::GatherUsedProps(const MapOfMeshGeometries&                          theGeoms,
                                        NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps)
{
    auto processGeom = [&theProps](const Handle(MeshGeometry)& theGeom) {
        if (Handle(MeshGeometryDeformed) aGeomDef = Handle(MeshGeometryDeformed)::DownCast(theGeom)) {
            if (!aGeomDef->DeformedProperty().IsNull()) {
                theProps.Add(aGeomDef->DeformedProperty());
            }
        } else if (Handle(MeshGeometryIso) aGeomIso = Handle(MeshGeometryIso)::DownCast(theGeom)) {
            if (aGeomIso->Property().IsDefined()) {
                theProps.Add(aGeomIso->Property().Property);
            }
        }
    };

    for (MapOfMeshGeometries::Iterator anIter(theGeoms); anIter.More(); anIter.Next()) {
        for (MeshGeometry* aGeom : MeshGeomChildren(anIter.Value().get())) {
            processGeom(aGeom);
        }
    }
}

// =======================================================================
// function : Perform
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::Perform()
{
    if (!generateElementValues()) {
        return false;
    }

    MapOfContours* aCachedMapOfContours = myCachedContours->ChangeSeek((int)myElem->Type);
    if (aCachedMapOfContours == nullptr) {
        aCachedMapOfContours = myCachedContours->Bound((int)myElem->Type, MapOfContours());
    }

    ListOfContours  aPolyListOfContours;
    ListOfContours* aCachedListOfContours = aCachedMapOfContours->ChangeSeek(myValueMask);
    if (aCachedListOfContours == nullptr) {
        // Don't cache contours for polyhedral elements
        if (myElem->Type != RWCae::MeshElemType::Polyhedron && myElem->Type != RWCae::MeshElemType::Polygon) {
            aCachedListOfContours = aCachedMapOfContours->Bound(myValueMask, ListOfContours());
        } else {
            aCachedListOfContours = &aPolyListOfContours;
        }

        if (myValueMask.IsZero()) {
            generateWholeElementContours(aCachedListOfContours);
        } else if (!generateContours(aCachedListOfContours)) {
            aCachedMapOfContours->UnBind(myValueMask);
            return false;
        }
    }

    generateCutPoints(*aCachedListOfContours);

    return true;
}

// =======================================================================
// function : generateElementValues
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::generateElementValues()
{
    RWCae::MeshElemEntityId aNbNodes = myElem->NbNodes();

    myValueMask.Init(aNbNodes);

    myAvValue = 0.0;

    int aNbPosValues = 0;
    int aNbNegValues = 0;

    myNodeValues.resize(aNbNodes);

    for (RWCae::MeshElemEntityId i = 0; i < aNbNodes; i++) {
        double aValue = myNodeValueFunc(i);
        if (RWCae::IsNaN(aValue)) {
            return false;
        }

        if (aValue < myCutValue - myConfusion) {
            ++aNbNegValues;
            myValueMask.Set(i, 1);
        } else if (aValue > myCutValue + myConfusion) {
            ++aNbPosValues;
            myValueMask.Set(i, 0);
        } else {
            aValue = myCutValue;
        }

        myAvValue += aValue;

        myNodeValues[i] = aValue;
    }

    myAvValue /= aNbNodes;
    if (Abs(myAvValue - myCutValue) <= myConfusion) {
        myAvValue = myCutValue + myConfusion + myConfusion;
    }

    return aNbPosValues != aNbNodes && aNbNegValues != aNbNodes;
}

// =======================================================================
// function : generateWholeElementContours
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::generateWholeElementContours(ListOfContours* theListOfContours)
{
    RWCae::MeshElemEntityId aNbFaces = myAdaptor->NbFaces(*myElem);

    Graphic3d_Vec2ub anEdge;
    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
        NCollection_List<Graphic3d_Vec2ub> aContour;

        RWCae::MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, aFaceIter);
        RWCae::MeshElemEntityId aPrevNode    = myAdaptor->FaceNodeIndex(*myElem, aFaceIter, 0);
        for (RWCae::MeshElemEntityId aNodeIter = 1; aNodeIter <= aNbFaceNodes; aNodeIter++) {
            RWCae::MeshElemEntityId aNode = myAdaptor->FaceNodeIndex(*myElem, aFaceIter, aNodeIter % aNbFaceNodes);

            anEdge.SetValues(aPrevNode, aNode);
            aContour.Append(anEdge);

            aPrevNode = aNode;
        }
        theListOfContours->Append(aContour);
    }
}

// =======================================================================
// function : generateContours
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::generateContours(ListOfContours* theListOfContours)
{
    if (myAdaptor->NbFaces(*myElem) == 1) {
        return generateContours2d(theListOfContours);
    }

    EdgeElemMap                      anEdges;
    std::vector<std::vector<double>> aFaceNodesValues;
    gatherIntersectedEdges(&anEdges, &aFaceNodesValues);
    while (!anEdges.IsEmpty()) {
        NCollection_List<Graphic3d_Vec2ub> aContourPnts;

        int anIter = 1;
        while (anIter > 0) {
            MeshElemEdge aCurEdge = anEdges.FindKey(anIter);
            aContourPnts.Append(anEdges.FindFromIndex(anIter).second);

            aCurEdge = anEdges.FindFromIndex(anIter).first;
            anEdges.RemoveFromIndex(anIter);
            if (!aCurEdge.IsValid) {
                aContourPnts.Clear();
                break;
            }

            anIter = anEdges.FindIndex(aCurEdge);
            if (anIter == 0) {
                aContourPnts.Clear();
                break;
            }

            anEdges.RemoveFromIndex(anIter);

            RWCae::MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, aCurEdge.Face);

            anIter = getFaceAdjEdge(anEdges, aFaceNodesValues, aNbFaceNodes, &aCurEdge);
        }

        if (aContourPnts.Size() > 1) {
            theListOfContours->Append(aContourPnts);
        }
    }

    return true;
}

// =======================================================================
// function : gatherIntersectedEdges
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::gatherIntersectedEdges(
    EdgeElemMap*                      theEdges,
    std::vector<std::vector<double>>* theFaceNodesValues)
{
    NCollection_DataMap<Graphic3d_Vec2u, MeshElemEdge, EdgeHasher> aProcessedEdges;

    Graphic3d_Vec2u  aProcEdge;
    Graphic3d_Vec2ub anIndEdge;

    RWCae::MeshElemEntityId aNbFaces = myAdaptor->NbFaces(*myElem);
    theFaceNodesValues->resize(aNbFaces);
    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
        RWCae::MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, aFaceIter);

        (*theFaceNodesValues)[aFaceIter].resize(aNbFaceNodes);
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
            RWCae::MeshElemEntityId aNode1 = myAdaptor->FaceNodeIndex(*myElem, aFaceIter, aNodeIter);
            RWCae::MeshElemEntityId aNode2 = myAdaptor->FaceNodeIndex(*myElem,
                                                                        aFaceIter,
                                                                        (aNodeIter + 1) % aNbFaceNodes);

            double aVal1 = myNodeValues[aNode1];
            double aVal2 = myNodeValues[aNode2];

            (*theFaceNodesValues)[aFaceIter][aNodeIter] = aVal1;

            bool arePntsOpposite = (aVal1 < myCutValue + myConfusion && aVal2 > myCutValue - myConfusion)
                                || (aVal1 > myCutValue - myConfusion && aVal2 < myCutValue + myConfusion);
            if (!arePntsOpposite || Abs(aVal2 - aVal1) < myConfusion) {
                continue;
            }

            if ((aVal1 == myCutValue && (aVal2 - myCutValue) * (myAvValue - myCutValue) < 0.0)
                || (aVal2 == myCutValue && (aVal1 - myCutValue) * (myAvValue - myCutValue) < 0.0)) {
                continue;
            }

            MeshElemEdge anEdge(aFaceIter, aNodeIter);

            anIndEdge.SetValues(aNode1, aNode2);
            theEdges->Add(anEdge, std::make_pair(MeshElemEdge(), anIndEdge));

            aProcEdge.SetValues((*myElem)[aNode1], (*myElem)[aNode2]);

            const MeshElemEdge* anAdjEdge = aProcessedEdges.Seek(aProcEdge);
            if (anAdjEdge == nullptr) {
                aProcessedEdges.Bind(aProcEdge, anEdge);
                continue;
            }

            std::pair<MeshElemEdge, Graphic3d_Vec2ub>* anAdj = theEdges->ChangeSeek(*anAdjEdge);
            if (anAdj != nullptr) {
                anAdj->first = anEdge;
            }
            anAdj = theEdges->ChangeSeek(anEdge);
            if (anAdj != nullptr) {
                anAdj->first = *anAdjEdge;
            }
        }
    }
}

// =======================================================================
// function : gatherIntersectedEdges
// =======================================================================
template<class Elem_T>
int MeshGeometryTools::MeshElementCutter<Elem_T>::getFaceAdjEdge(
    const EdgeElemMap&                      theEdges,
    const std::vector<std::vector<double>>& theFaceNodesValues,
    RWCae::MeshElemEntityId               theNbFaceNodes,
    MeshElemEdge*                           theEdge)
{
    bool isLeft = theFaceNodesValues[theEdge->Face][theEdge->Id] < myCutValue
               || theFaceNodesValues[theEdge->Face][(theEdge->Id + 1) % theNbFaceNodes] > myCutValue;

    RWCae::MeshElemEntityId aCurId = theEdge->Id + theNbFaceNodes;

    int anIterLeft  = 0;
    int anIterRight = 0;
    for (RWCae::MeshElemEntityId anEdgeIter = 1; anEdgeIter < theNbFaceNodes && (anIterLeft == 0 || anIterRight == 0);
         anEdgeIter++) {
        if (anIterLeft == 0) {
            theEdge->Id = (aCurId - anEdgeIter) % theNbFaceNodes;
            anIterLeft  = theEdges.FindIndex(*theEdge);
        }
        if (anIterRight == 0) {
            theEdge->Id = (aCurId + anEdgeIter) % theNbFaceNodes;
            anIterRight = theEdges.FindIndex(*theEdge);
        }
    }

    return anIterLeft == anIterRight ? anIterLeft
         : anIterLeft == 0           ? anIterRight
         : anIterRight == 0          ? anIterLeft
         : isLeft                    ? anIterLeft
                                     : anIterRight;
}

// =======================================================================
// function : generateContours2d
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::generateContours2d(ListOfContours* theListOfContours)
{
    Graphic3d_Vec2ub aNodeToAdd;
    bool             isFirstNode = true;

    RWCae::MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, 0);

    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
        RWCae::MeshElemEntityId aNode1 = myAdaptor->FaceNodeIndex(*myElem, 0, aNodeIter);
        RWCae::MeshElemEntityId aNode2 = myAdaptor->FaceNodeIndex(*myElem, 0, (aNodeIter + 1) % aNbFaceNodes);

        double aVal1 = myNodeValues[aNode1];
        double aVal2 = myNodeValues[aNode2];

        bool arePntsOpposite = (aVal1 < myCutValue + myConfusion && aVal2 > myCutValue - myConfusion)
                            || (aVal1 > myCutValue - myConfusion && aVal2 < myCutValue + myConfusion);
        if (!arePntsOpposite || Abs(aVal2 - aVal1) < myConfusion) {
            continue;
        }

        if ((aVal1 == myCutValue && (aVal2 - myCutValue) * (myAvValue - myCutValue) < 0.0)
            || (aVal2 == myCutValue && (aVal1 - myCutValue) * (myAvValue - myCutValue) < 0.0)) {
            continue;
        }

        if (isFirstNode) {
            aNodeToAdd.SetValues(aNode1, aNode2);
        } else {
            NCollection_List<Graphic3d_Vec2ub>& aContour = theListOfContours->Append(
                NCollection_List<Graphic3d_Vec2ub>());
            aContour.Append(aNodeToAdd);
            aNodeToAdd.SetValues(aNode1, aNode2);
            aContour.Append(aNodeToAdd);
        }
        isFirstNode = !isFirstNode;
    }

    return true;
}

// =======================================================================
// function : generateCutPoints
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::generateCutPoints(const ListOfContours& theListOfContours)
{
    static constexpr int nbBaseNodes = 2;

    myCutNodes.clear();
    myCutNodes.reserve(theListOfContours.Size());

    for (ListOfContours::Iterator aContoursIter(theListOfContours); aContoursIter.More(); aContoursIter.Next()) {
        const NCollection_List<Graphic3d_Vec2ub>& aContour = aContoursIter.Value();

        myCutNodes.push_back(std::vector<MeshInterpolatedNode>());
        myCutNodes.back().reserve(aContour.Extent());

        for (NCollection_List<Graphic3d_Vec2ub>::Iterator aContourIter(aContour); aContourIter.More();
             aContourIter.Next()) {
            Graphic3d_Vec2u anEdge((*myElem)[aContourIter.Value().x()], (*myElem)[aContourIter.Value().y()]);
            double          aValue1 = myNodeValues[aContourIter.Value().x()];
            double          aValue2 = myNodeValues[aContourIter.Value().y()];
            if (aValue1 > aValue2 + myConfusion) {
                std::swap(aValue1, aValue2);
                std::swap(anEdge.x(), anEdge.y());
            }
            double aValue = aValue2 - aValue1 < myConfusion ? 0.0 : (myCutValue - aValue1) / (aValue2 - aValue1);
            MeshInterpolatedNode aLerpPnt(nbBaseNodes);
            aLerpPnt.AddBaseNode(anEdge.x(), 1.0 - aValue);
            aLerpPnt.AddBaseNode(anEdge.y(), aValue);

            myCutNodes.back().push_back(aLerpPnt);
        }
    }
}

// =======================================================================
// function : fillNodesPositionMap
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::fillNodesPositionMap(const Handle(MeshGeometry)& theGeom,
                                                                        NodePositionMap*            theNodes) const
{
    Graphic3d_Vec3 aNode;
    for (RWCae::MeshElemEntityId i = 0; i < myElem->NbNodes(); i++) {
        myAdaptor->NodePosition(aNode, (*myElem)[i]);
        theGeom->ModifyNode((*myElem)[i], aNode);
        theNodes->Bind((*myElem)[i], aNode);
    }
}

// =======================================================================
// function : AddCutSurf
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::AddCutSurf(const Handle(MeshGeometry)&      theGeom,
                                                              const Handle(RWCae::MeshData)& theOutMeshData,
                                                              MeshGeometry::NodesMap*          theGenNodes,
                                                              MeshGeometry::ElementsMap*       theGenElems,
                                                              AddedNodeMap*                    theAddedNodes,
                                                              RWCae::MeshEntityId            theLocalIndex) const
{
    static constexpr RWCae::MeshElemEntityId aNbTrgNodes  = 3;
    static constexpr RWCae::MeshElemEntityId aNbQuadNodes = 4;

    NodePositionMap aNodes;
    fillNodesPositionMap(theGeom, &aNodes);

    // Since the list of contours is cached, the triangulation result depends
    // on element evaluation order and depends on BVH building algorithm.
    for (const std::vector<MeshInterpolatedNode>& aContour : myCutNodes) {
        if (aContour.size() < aNbTrgNodes) {
            continue;
        }

        std::vector<RWCae::MeshEntityId> aPnts(aContour.size());
        RWCae::MeshElemEntityId          aNbAddedNodes = 0;

        for (size_t aNodeIter = 0; aNodeIter < aContour.size(); aNodeIter++) {
            if (MeshInterpolatedNode::IsEqual(aContour[aNodeIter], aContour[(aNodeIter + 1) % aContour.size()])) {
                continue;
            }
            if (!theAddedNodes->Find(aContour[aNodeIter], aPnts[aNbAddedNodes])) {
                Graphic3d_Vec3 aPnt1 = aNodes.Find(aContour[aNodeIter].BaseNodes()[0].first);
                Graphic3d_Vec3 aPnt2 = aNodes.Find(aContour[aNodeIter].BaseNodes()[1].first);
                Graphic3d_Vec3 aPnt  = aPnt1 * (float)aContour[aNodeIter].BaseNodes()[0].second
                                    + aPnt2 * (float)aContour[aNodeIter].BaseNodes()[1].second;

                aPnts[aNbAddedNodes] = theOutMeshData->NbNodes();
                theAddedNodes->Bind(aContour[aNodeIter], aPnts[aNbAddedNodes]);
                theGenNodes->Bind(aPnts[aNbAddedNodes], std::make_pair(theGeom.get(), aContour[aNodeIter]));
                theOutMeshData->AddNodePosition(aPnt);
            }
            aNbAddedNodes++;
        }

        if (aNbAddedNodes < aNbTrgNodes) {
            continue;
        }

        RWCae::MeshElemType aType;
        if (aNbAddedNodes == aNbTrgNodes) {
            RWCae::MeshElementTriangle aSurfElem(aPnts.data());
            aType = aSurfElem.Type;
            theOutMeshData->AddElement(aSurfElem);
        } else if (aNbAddedNodes == aNbQuadNodes) {
            RWCae::MeshElementQuad aSurfElem(aPnts.data());
            aType = aSurfElem.Type;
            theOutMeshData->AddElement(aSurfElem);
        } else {
            RWCae::MeshElementPolygon aSurfElem(aPnts.data(), aNbAddedNodes);
            aType = aSurfElem.Type;
            theOutMeshData->AddElement(aSurfElem);
        }

        theGenElems->Bind(ViewCaeGeom::MeshGeomElem(theOutMeshData->NbElems(aType) - 1, aType, false),
                          std::make_pair(theGeom.get(),
                                         ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false)));
    }
}

// =======================================================================
// function : AddCutElem
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::AddCutElem(const Handle(MeshGeometry)&      theGeom,
                                                              const Handle(RWCae::MeshData)& theOutMeshData,
                                                              const NodesAdjElems&             theNodesAdjElems,
                                                              MeshGeometry::NodesMap*          theGenNodes,
                                                              MeshGeometry::ElementsMap*       theGenElems,
                                                              AddedNodeMap*                    theAddedNodes,
                                                              RWCae::MeshEntityId            theLocalIndex) const
{
    EdgeToLerpPoint aContourMapEdges;
    NodeToLerpPoint aContourMapNodes;
    for (size_t i = 0; i < myCutNodes.size(); i++) {
        for (size_t j = 0; j < myCutNodes[i].size(); j++) {
            const MeshInterpolatedNode& aNode = myCutNodes[i][j];
            if (aNode.BaseNodes()[0].second > 1.0 - 1e-3) {
                aContourMapNodes.Bind(aNode.BaseNodes()[0].first, aNode);
            } else if (aNode.BaseNodes()[1].second > 1.0 - 1e-3) {
                aContourMapNodes.Bind(aNode.BaseNodes()[1].first, aNode);
            } else {
                aContourMapEdges.Bind(Graphic3d_Vec2u(aNode.BaseNodes()[0].first, aNode.BaseNodes()[1].first), aNode);
            }
        }
    }

    NodePositionMap aNodes;
    fillNodesPositionMap(theGeom, &aNodes);

    ElemFaces aFaces;

    RWCae::MeshElemEntityId aNbFaces = myAdaptor->NbFaces(*myElem);

    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
        extractCutElemFace(theNodesAdjElems, aContourMapEdges, aContourMapNodes, aFaceIter, aNbFaces == 1, &aFaces);
    }

    if (aFaces.empty()) {
        return false;
    }

    std::vector<RWCae::MeshEntityId> anElemNodes;
    AddedNodeMap                       aLerpToId;
    EntityToElemId                     anIdToIdx;
    addCutElemNodes(theGeom, theOutMeshData, aNodes, theGenNodes, theAddedNodes, &anElemNodes, &aLerpToId, &anIdToIdx);

    if (aNbFaces == 1) {
        addCutElem2d(theGeom, theOutMeshData, aFaces, aLerpToId, theLocalIndex, theGenElems);
        return true;
    }

    extractCutElemCutFace(aFaces);

    addCutElem3d(theGeom, theOutMeshData, anElemNodes, aFaces, aLerpToId, anIdToIdx, theLocalIndex, theGenElems);

    return true;
}

// =======================================================================
// function : addCutElemNodes
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::addCutElemNodes(const Handle(MeshGeometry)&         theGeom,
                                                                   const Handle(RWCae::MeshData)&    theOutMeshData,
                                                                   const NodePositionMap&              theNodes,
                                                                   MeshGeometry::NodesMap*             theGenNodes,
                                                                   AddedNodeMap*                       theAddedNodes,
                                                                   std::vector<RWCae::MeshEntityId>* theElemNodes,
                                                                   AddedNodeMap*                       theLerpToId,
                                                                   EntityToElemId*                     theIdToIdx) const
{
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < myElem->NbNodes(); aNodeIter++) {
        if (!myValueMask.IsSet(aNodeIter, 0)) {
            continue;
        }

        MeshInterpolatedNode aNode;
        aNode.AddBaseNode((*myElem)[aNodeIter], 1.0);

        RWCae::MeshEntityId aNewNodeId;
        if (!theAddedNodes->Find(aNode, aNewNodeId)) {
            aNewNodeId = theOutMeshData->NbNodes();
            theAddedNodes->Bind(aNode, aNewNodeId);
            theGenNodes->Bind(aNewNodeId, std::make_pair(theGeom.get(), aNode));
            theOutMeshData->AddNodePosition(theNodes.Find((*myElem)[aNodeIter]));
        }

        theElemNodes->push_back(aNewNodeId);
        theLerpToId->Bind(aNode, aNewNodeId);
        theIdToIdx->Bind(aNewNodeId, static_cast<RWCae::MeshElemEntityId>(theElemNodes->size() - 1));
    }

    for (const std::vector<MeshInterpolatedNode>& aContourNodes : myCutNodes) {
        for (const MeshInterpolatedNode& aNode : aContourNodes) {
            RWCae::MeshEntityId aNewNodeId;
            if (!theAddedNodes->Find(aNode, aNewNodeId)) {
                Graphic3d_Vec3 aPnt1 = theNodes.Find(aNode.BaseNodes()[0].first);
                Graphic3d_Vec3 aPnt2 = theNodes.Find(aNode.BaseNodes()[1].first);
                Graphic3d_Vec3 aPnt  = aPnt1 * (float)aNode.BaseNodes()[0].second
                                    + aPnt2 * (float)aNode.BaseNodes()[1].second;

                aNewNodeId = theOutMeshData->NbNodes();
                theAddedNodes->Bind(aNode, aNewNodeId);
                theGenNodes->Bind(aNewNodeId, std::make_pair(theGeom.get(), aNode));
                theOutMeshData->AddNodePosition(aPnt);
            }

            if (!theLerpToId->Bind(aNode, aNewNodeId)) {
                continue;
            }

            theElemNodes->push_back(aNewNodeId);
            theIdToIdx->Bind(aNewNodeId, static_cast<RWCae::MeshElemEntityId>(theElemNodes->size() - 1));
        }
    }
}

// =======================================================================
// function : getAdjElemsMask
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::getAdjElemsMask(const NodesAdjElems&      theNodesAdjElems,
                                                                   RWCae::MeshElemEntityId theFaceIter,
                                                                   RWCae::MeshElemEntityId theNbFaceNodes,
                                                                   BitMaskSet<1>*            theMask) const
{
    theMask->Init(static_cast<int>(theNodesAdjElems.second.size()));
    theMask->SetAll();
    NCollection_Map<RWCae::MeshEntityId> anAdjFace;
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theNbFaceNodes; aNodeIter++) {
        const BitMaskSet<1>& aNodeAdjElems =
            theNodesAdjElems.first[myAdaptor->FaceNodeIndex(*myElem, theFaceIter, aNodeIter)];
        theMask->Intersected(aNodeAdjElems);
    }
}

// =======================================================================
// function : getFirstAdjElem
// =======================================================================
template<class Elem_T>
bool MeshGeometryTools::MeshElementCutter<Elem_T>::getFirstAdjElem(const NodesAdjElems&   theNodesAdjElems,
                                                                   const BitMaskSet<1>&   theMask,
                                                                   RWCae::MeshEntityId* theIdx) const
{
    if (theMask.IsZero()) {
        return false;
    }

    for (int i = 0; i < theMask.NbValues(); i++) {
        if (!theMask.IsSet(i)) {
            continue;
        }

        RWCae::MeshEntityId anAdjElem = theNodesAdjElems.second[i];
        RWCae::MeshElemType anAdjType = myAdaptor->GetMeshData()->IndexedElements()[anAdjElem].Type;
        if ((int)anAdjType >= RWCae::MeshElem3dLowerType && (int)anAdjType <= RWCae::MeshElem3dUpperType) {
            *theIdx = anAdjElem;
            return true;
        }
    }

    return false;
}

// =======================================================================
// function : extractCutElemFace
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::extractCutElemFace(const NodesAdjElems&      theNodesAdjElems,
                                                                      const EdgeToLerpPoint&    theContourMapEdges,
                                                                      const NodeToLerpPoint&    theContourMapNodes,
                                                                      RWCae::MeshElemEntityId theFaceIter,
                                                                      bool                      theIs2d,
                                                                      ElemFaces*                theFaces) const
{
    RWCae::MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, theFaceIter);

    theFaces->push_back(std::make_pair(std::vector<MeshInterpolatedNode>(), false));
    std::vector<MeshInterpolatedNode>& aFace = theFaces->back().first;
    aFace.reserve(aNbFaceNodes);

    int aNbAddedContourNodes = 0;
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
        RWCae::MeshElemEntityId aCurNodeIndex = myAdaptor->FaceNodeIndex(*myElem, theFaceIter, aNodeIter);
        RWCae::MeshEntityId     aCurNodeId    = (*myElem)[aCurNodeIndex];
        RWCae::MeshEntityId aNextNodeId = myAdaptor->FaceNode(*myElem, theFaceIter, (aNodeIter + 1) % aNbFaceNodes);
        if (myValueMask.IsSet(aCurNodeIndex, 0)) {
            MeshInterpolatedNode aLerpPnt;
            aLerpPnt.AddBaseNode(aCurNodeId, 1.0);
            aFace.push_back(aLerpPnt);
        }

        const MeshInterpolatedNode* aLerpPntEdge = theContourMapEdges.Seek(Graphic3d_Vec2u(aCurNodeId, aNextNodeId));
        if (aLerpPntEdge != nullptr) {
            aFace.push_back(*aLerpPntEdge);
            aNbAddedContourNodes++;
        }

        const MeshInterpolatedNode* aLerpPnt = theContourMapNodes.Seek(aNextNodeId);
        if (aLerpPnt != nullptr) {
            aFace.push_back(*aLerpPnt);
            aNbAddedContourNodes++;
        }
    }

    static constexpr size_t aMinNbNodes = 3;
    if (aFace.size() < aMinNbNodes) {
        theFaces->pop_back();
        return;
    }

    if (theIs2d) {
        return;
    }

    BitMaskSet<1> aMask;
    getAdjElemsMask(theNodesAdjElems, theFaceIter, aNbFaceNodes, &aMask);

    RWCae::MeshEntityId anAdjElem = 0;
    if (!getFirstAdjElem(theNodesAdjElems, aMask, &anAdjElem)) {
        theFaces->back().second = true;
    } else if (aNbAddedContourNodes >= aNbFaceNodes) {
        theFaces->pop_back();
        myAdjElemToCheck.push_back(anAdjElem);
    }
}

// =======================================================================
// function : extractCutElemCutFace
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::extractCutElemCutFace(ElemFaces& theFaces) const
{
    static constexpr size_t aMinNbNodes = 3;

    for (const std::vector<MeshInterpolatedNode>& aContourNodes : myCutNodes) {
        if (aContourNodes.size() < aMinNbNodes) {
            continue;
        }
        theFaces.push_back(std::make_pair(std::vector<MeshInterpolatedNode>(), false));
        std::vector<MeshInterpolatedNode>& aFace = theFaces.back().first;
        aFace.reserve(aContourNodes.size());

        for (size_t aNodeIter = 0; aNodeIter < aContourNodes.size(); aNodeIter++) {
            if (MeshInterpolatedNode::IsEqual(aContourNodes[aNodeIter],
                                              aContourNodes[(aNodeIter + 1) % aContourNodes.size()])) {
                continue;
            }
            aFace.push_back(aContourNodes[aNodeIter]);
        }

        if (aFace.size() < aMinNbNodes) {
            theFaces.pop_back();
            continue;
        }

        theFaces.back().second = true;
    }
}

// =======================================================================
// function : addCutElem2d
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::addCutElem2d(const Handle(MeshGeometry)&      theGeom,
                                                                const Handle(RWCae::MeshData)& theOutMeshData,
                                                                const ElemFaces&                 theFaces,
                                                                const AddedNodeMap&              theLerpToId,
                                                                RWCae::MeshEntityId            theLocalIndex,
                                                                MeshGeometry::ElementsMap*       theGenElems) const
{
    std::vector<RWCae::MeshEntityId> aNodes(theFaces.front().first.size());
    for (size_t i = 0; i < aNodes.size(); i++) {
        aNodes[i] = theLerpToId.Find(theFaces.front().first[i]);
    }

    RWCae::MeshElementPolygon anElem(aNodes.data(), static_cast<RWCae::MeshElemEntityId>(aNodes.size()));
    theOutMeshData->AddElement(anElem);

    theGenElems->Bind(ViewCaeGeom::MeshGeomElem(theOutMeshData->NbElems(anElem.Type) - 1, anElem.Type, false),
                      std::make_pair(theGeom.get(), ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false)));
}

// =======================================================================
// function : addCutElem3d
// =======================================================================
template<class Elem_T>
void MeshGeometryTools::MeshElementCutter<Elem_T>::addCutElem3d(const Handle(MeshGeometry)&      theGeom,
                                                                const Handle(RWCae::MeshData)& theOutMeshData,
                                                                const std::vector<RWCae::MeshEntityId>& theElemNodes,
                                                                const ElemFaces&                          theFaces,
                                                                const AddedNodeMap&                       theLerpToId,
                                                                const EntityToElemId&                     theIdToIdx,
                                                                RWCae::MeshEntityId                     theLocalIndex,
                                                                MeshGeometry::ElementsMap* theGenElems) const
{
    std::vector<RWCae::MeshElemEntityId>          anElemFaces;
    std::vector<std::vector<RWCae::MeshEntityId>> anOuterFaces;
    std::vector<RWCae::MeshEntityId>              aNodes;
    for (const auto& aFace : theFaces) {
        aNodes.resize(aFace.first.size());
        for (size_t i = 0; i < aFace.first.size(); i++) {
            aNodes[i] = theLerpToId.Find(aFace.first[i]);
        }

        anElemFaces.push_back(static_cast<RWCae::MeshElemEntityId>(aNodes.size()));
        for (RWCae::MeshEntityId aNode : aNodes) {
            anElemFaces.push_back(theIdToIdx.Find(aNode));
        }

        if (aFace.second) {
            anOuterFaces.push_back(aNodes);
        }
    }

    RWCae::MeshElementPolyhedron anElem(theElemNodes.data(),
                                          static_cast<int>(theElemNodes.size()),
                                          anElemFaces.data(),
                                          static_cast<uint16_t>(anElemFaces.size()));

    int anElemGI = theOutMeshData->AddElement(anElem);

    theGenElems->Bind(ViewCaeGeom::MeshGeomElem(theOutMeshData->NbElems(anElem.Type) - 1, anElem.Type, false),
                      std::make_pair(theGeom.get(), ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false)));

    for (const auto& aFace : anOuterFaces) {
        RWCae::MeshElementPolygon anOuterElem(aFace.data(),
                                                static_cast<RWCae::MeshElemEntityId>(aFace.size()),
                                                anElemGI);
        theOutMeshData->AddSurfaceElement(anOuterElem);

        theGenElems->Bind(
            ViewCaeGeom::MeshGeomElem(theOutMeshData->NbSurfElems(anOuterElem.Type) - 1, anOuterElem.Type, true),
            std::make_pair(theGeom.get(), ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false)));
    }

    theOutMeshData->SetToUpdateReconstructedSurface(false);
}

template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElement0d>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementTriangle>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementTriangle2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementQuad>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementQuad2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPolygon>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementTetrahedron>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementTetrahedron2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPyramid>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPyramid2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPrism>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPrism2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementHexahedron>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementHexahedron2nd>;
template ViewCaeGeom_ExportTemplate class MeshGeometryTools::MeshElementCutter<RWCae::MeshElementPolyhedron>;

} // namespace               
