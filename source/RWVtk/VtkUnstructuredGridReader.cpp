

#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <map>
#include <set>

namespace {

//! From vtkCellType
enum class VtkCellType {
    VTK_VERTEX               = 1,
    VTK_TRIANGLE             = 5,
    VTK_POLYGON              = 7,
    VTK_QUAD                 = 9,
    VTK_TETRA                = 10,
    VTK_HEXAHEDRON           = 12,
    VTK_WEDGE                = 13,
    VTK_PYRAMID              = 14,
    VTK_QUADRATIC_TRIANGLE   = 22,
    VTK_QUADRATIC_QUAD       = 23,
    VTK_QUADRATIC_TETRA      = 24,
    VTK_QUADRATIC_HEXAHEDRON = 25,
    VTK_QUADRATIC_WEDGE      = 26,
    VTK_QUADRATIC_PYRAMID    = 27,
    VTK_POLYHEDRON           = 42
};

//! Fill nodes of mesh element with fixed number of nodes and add to mesh data.
template<class Elem_T>
void AddFixedElem(Elem_T&                                  theElem,
                  const Handle(RWCae::MeshData)&         theMeshData,
                  const Handle(RWVtk::VtkAbstractArray)& theConnectivityArray,
                  int                                      theCellBegin,
                  int                                      theNodesOffset)
{
    for (RWCae::MeshElemEntityId i = 0; i < Elem_T::NbFixedNodes; i++) {
        theElem[i] = theConnectivityArray->GetInt(theCellBegin + i, 0) + theNodesOffset;
    }
    theMeshData->AddElement(theElem);
}

}

namespace RWVtk {

// ================================================================
// function : VtkUnstructuredGridReader
// ================================================================
VtkUnstructuredGridReader::VtkUnstructuredGridReader() : VtkUnstructuredDataReader()
{
    //
}

// ================================================================
// function : ~VtkUnstructuredGridReader
// ================================================================
VtkUnstructuredGridReader::~VtkUnstructuredGridReader()
{
    //
}

// =======================================================================
// function : readMeshInfo
// =======================================================================
bool VtkUnstructuredGridReader::readMeshInfo(const LDOM_Element& theElem)
{
    if (!VtkUnstructuredDataReader::readMeshInfo(theElem)) {
        return false;
    }

    getXmlAttribute(theElem, "NumberOfCells", "numberOfCells").GetInteger(myNbElements);
    if (myNbElements <= 0) {
        return fail(std::string("Invalid number of elements '") + std::to_string(myNbElements) + "'");
    }

    if (myToReadMeshGeometry) { (*myStoredElements)[myCurPiece].assign(myNbElements, 0); }
    else if ((*myStoredElements)[myCurPiece].size() != (size_t)myNbElements) {
        return fail("VTK time step cell count differs from the original mesh");
    }

    return myReport.errors.empty();
}

// =======================================================================
// function : readMeshGeometry
// =======================================================================
bool VtkUnstructuredGridReader::readMeshGeometry(const LDOM_Element& theElem)
{
    if (!VtkUnstructuredDataReader::readMeshGeometry(theElem)) {
        return false;
    }

    LDOM_Element aCells = getFirstElement(theElem, "Cells");
    if (aCells.isNull()) {
        return fail("'UnstructuredGrid/Piece/Cells' element not found");
    }

    if (!parseCells(aCells)) {
        return false;
    }

    return true;
}

// =======================================================================
// function : parseCells
// =======================================================================
bool VtkUnstructuredGridReader::parseCells(const LDOM_Element& theElem)
{
    // VTK XML 2.3 stores shared faces separately from polyhedron-to-face IDs.
    // Recognize this concrete encoding as a backend gap, while incomplete or
    // malformed arrays remain validation failures and cannot trigger fallback.
    int modernFaces = 0;
    std::set<std::string> names;
    for (const auto& element : getChildrenElementsByTagName(theElem, "DataArray")) {
        const TCollection_AsciiString name = getXmlAttribute(element, "Name", "name");
        if (!names.insert(name.ToCString()).second) { return fail("Duplicate VTK topology array name"); }
        if (name == "face_connectivity" || name == "face_offsets"
            || name == "polyhedron_to_faces" || name == "polyhedron_offsets") {
            ++modernFaces;
            const auto array = readDataArray(element);
            if (array.Data.IsNull() || array.Components.size() != 1
                || (array.Type != "Int32" && array.Type != "Int64")) { return fail("Invalid XML 2.3 polyhedron face array"); }
        }
    }
    if (modernFaces) {
        return fail(modernFaces == 4 ? "VTK XML 2.3 polyhedron face encoding requires native cells"
            : "Incomplete XML 2.3 polyhedron face arrays");
    }
    MeshElements aMeshElements;
    parseElements(theElem, aMeshElements);

    if (aMeshElements.Connectivity.Data.IsNull() || aMeshElements.Offsets.Data.IsNull()
        || aMeshElements.Types.Data.IsNull()) {
        return fail("Failed to read cells data");
    }
    if (aMeshElements.Connectivity.Components.size() != 1 || aMeshElements.Offsets.Components.size() != 1
        || aMeshElements.Types.Components.size() != 1) {
        return fail("VTK cell arrays must have one component");
    }

    if (aMeshElements.Offsets.Data->NbElements() != myNbElements
        || aMeshElements.Types.Data->NbElements() != myNbElements) {
        return fail(std::string("Incorrect number of cells '") + std::to_string(aMeshElements.Offsets.Data->NbElements()) + ", " + std::to_string(aMeshElements.Types.Data->NbElements()) + "', should be '" + std::to_string(myNbElements) + "'");
    }

    NCollection_DataMap<int, int> aSkippedElements;

    parseCells(aMeshElements, aSkippedElements);

    if (!myReport.errors.empty()) { return false; }

    myNbProcessed += myNbElements;

    if (!aSkippedElements.IsEmpty()) {
        for (NCollection_DataMap<int, int>::Iterator anIter(aSkippedElements); anIter.More(); anIter.Next()) {
            fail("Unsupported VTK cell type " + std::to_string(anIter.Key()) + " ("
                 + std::to_string(anIter.Value()) + " cells)");
        }
        return false;
    }

    return true;
}

// =======================================================================
// function : parseCells
// =======================================================================
void VtkUnstructuredGridReader::parseCells(const MeshElements&            theElems,
                                           NCollection_DataMap<int, int>& theSkippedElements)
{
    int aCellBegin  = 0;
    int aFacesBegin = 0;
    for (int i = 0; i < myNbElements; i++) {
        int aCellType = theElems.Types.Data->GetInt(i, 0);

        int aCellEnd = theElems.Offsets.Data->GetInt(i, 0);

        if (aCellEnd <= aCellBegin || aCellEnd > theElems.Connectivity.Data->NbElements()) {
            fail("Invalid VTK connectivity offset");
            return;
        }
        if (aCellEnd - aCellBegin > std::numeric_limits<RWCae::MeshElemEntityId>::max()
            || (aCellType == (int)VtkCellType::VTK_POLYGON && aCellEnd - aCellBegin < 3)) {
            fail("Unsupported VTK cell node count");
            return;
        }

        const Handle(VtkAbstractArray)& aData = theElems.Connectivity.Data;

        static const std::map<int, int> nodeCounts = {{1,1},{5,3},{22,6},{9,4},{23,8},{10,4},{24,10},
                                                    {14,5},{27,13},{13,6},{26,15},{12,8},{25,20}};
        const auto expected = nodeCounts.find(aCellType);
        if (expected != nodeCounts.end() && aCellEnd - aCellBegin != expected->second) {
            fail("VTK cell connectivity has incorrect node count");
            return;
        }
        for (int node = aCellBegin; node < aCellEnd; ++node) {
            const int id = aData->GetInt(node, 0);
            if (id < 0 || id >= myNbNodes) { fail("VTK connectivity point index out of range"); return; }
        }

        switch (aCellType) {
            case (int)VtkCellType::VTK_VERTEX: addElem0d(aData, aCellBegin, i); break;
            case (int)VtkCellType::VTK_TRIANGLE: addTriangle(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_TRIANGLE: addTriangle(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_QUAD: addQuad(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_QUAD: addQuad(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_POLYGON: addPolygon(aData, aCellBegin, aCellEnd, i); break;
            case (int)VtkCellType::VTK_TETRA: addTetrahedron(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_TETRA: addTetrahedron(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_PYRAMID: addPyramid(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_PYRAMID: addPyramid(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_WEDGE: addPrism(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_WEDGE: addPrism(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_HEXAHEDRON: addHexahedron(aData, aCellBegin, i, false); break;
            case (int)VtkCellType::VTK_QUADRATIC_HEXAHEDRON: addHexahedron(aData, aCellBegin, i, true); break;
            case (int)VtkCellType::VTK_POLYHEDRON:
                if (theElems.Faces.Data.IsNull() || theElems.FaceOffsets.Data.IsNull()
                    || theElems.FaceOffsets.Data->NbElements() != myNbElements
                    || theElems.Faces.Components.size() != 1 || theElems.FaceOffsets.Components.size() != 1
                    || theElems.FaceOffsets.Data->GetInt(i, 0) <= aFacesBegin
                    || theElems.FaceOffsets.Data->GetInt(i, 0) > theElems.Faces.Data->NbElements()) {
                    fail("Invalid or missing VTK polyhedron face arrays");
                    return;
                }
                addPolyhedron(aData, theElems.Faces.Data, aCellBegin, aCellEnd, aFacesBegin,
                              theElems.FaceOffsets.Data->GetInt(i, 0), i);
                if (!myReport.errors.empty()) { return; }
                break;
            default: {
                int* aNbSkipped = theSkippedElements.ChangeSeek(aCellType);
                if (aNbSkipped == nullptr) {
                    aNbSkipped = theSkippedElements.Bound(aCellType, 0);
                }
                (*aNbSkipped)++;
                break;
            }
        }

        aCellBegin = aCellEnd;

        if (!theElems.FaceOffsets.Data.IsNull()) {
            aFacesBegin = std::max(aFacesBegin, theElems.FaceOffsets.Data->GetInt(i, 0));
        }
    }
    if (aCellBegin != theElems.Connectivity.Data->NbElements()) {
        fail("Unused VTK connectivity entries");
    }
}

// =======================================================================
// function : addElem0d
// =======================================================================
void VtkUnstructuredGridReader::addElem0d(const Handle(VtkAbstractArray)& theConnectivityArray,
                                          int                             theCellBegin,
                                          int                             theElemIndex)
{
    RWCae::MeshElement0d anElem;
    anElem[0] = theConnectivityArray->GetInt(theCellBegin, 0) + myNodesOffset;
    myMeshData->AddElement(anElem);
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addTetrahedron
// =======================================================================
void VtkUnstructuredGridReader::addTetrahedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                               int                             theCellBegin,
                                               int                             theElemIndex,
                                               bool                            theIs2nd)
{
    if (!theIs2nd) {
        RWCae::MeshElementTetrahedron anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    } else {
        RWCae::MeshElementTetrahedron2nd anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    }
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addPyramid
// =======================================================================
void VtkUnstructuredGridReader::addPyramid(const Handle(VtkAbstractArray)& theConnectivityArray,
                                           int                             theCellBegin,
                                           int                             theElemIndex,
                                           bool                            theIs2nd)
{
    if (!theIs2nd) {
        RWCae::MeshElementPyramid anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    } else {
        RWCae::MeshElementPyramid2nd anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    }
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addPrism
// =======================================================================
void VtkUnstructuredGridReader::addPrism(const Handle(VtkAbstractArray)& theConnectivityArray,
                                         int                             theCellBegin,
                                         int                             theElemIndex,
                                         bool                            theIs2nd)
{
    if (!theIs2nd) {
        RWCae::MeshElementPrism anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    } else {
        RWCae::MeshElementPrism2nd anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    }
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addHexahedron
// =======================================================================
void VtkUnstructuredGridReader::addHexahedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                              int                             theCellBegin,
                                              int                             theElemIndex,
                                              bool                            theIs2nd)
{
    if (!theIs2nd) {
        RWCae::MeshElementHexahedron anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    } else {
        RWCae::MeshElementHexahedron2nd anElem;
        AddFixedElem(anElem, myMeshData, theConnectivityArray, theCellBegin, myNodesOffset);
    }
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addPolyhedron
// =======================================================================
void VtkUnstructuredGridReader::addPolyhedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                              const Handle(VtkAbstractArray)& theFacesArray,
                                              int                             theCellBegin,
                                              int                             theCellEnd,
                                              int                             theFacesBegin,
                                              int                             theFacesEnd,
                                              int                             theElemIndex)
{
    int aNbNodes = theCellEnd - theCellBegin;

    NCollection_DataMap<int, int> aNodesMap;

    RWCae::MeshElementPolyhedron anElem(aNbNodes);
    for (int i = 0; i < aNbNodes; i++) {
        int aNode = theConnectivityArray->GetInt(theCellBegin + i, 0) + myNodesOffset;
        aNodesMap.Bind(aNode, i);
        anElem[i] = aNode;
    }

    int aNbFaces = theFacesArray->GetInt(theFacesBegin, 0);
    if (aNbFaces < 4 || aNbFaces > std::numeric_limits<RWCae::MeshElemEntityId>::max()) {
        fail("Invalid VTK polyhedron face count");
        return;
    }

    std::vector<RWCae::MeshElemEntityId> aFaces;
    std::vector<int>                       aFaceNodes;
    int aFaceOffset = 1;
    for (int aFaceId = 0; aFaceId < aNbFaces; aFaceId++) {
        if (theFacesBegin + aFaceOffset >= theFacesEnd) { fail("Truncated VTK polyhedron face stream"); return; }
        int aNbFaceNodes = theFacesArray->GetInt(theFacesBegin + aFaceOffset, 0);
        if (aNbFaceNodes < 3 || aNbFaceNodes > aNbNodes) {
            fail("Invalid VTK polyhedron face node count");
            return;
        }
        aFaces.push_back(aNbFaceNodes);
        aFaceOffset++;
        if (aNbFaceNodes > theFacesEnd - theFacesBegin - aFaceOffset) {
            fail("Truncated VTK polyhedron face connectivity");
            return;
        }
        aFaceNodes.resize(aNbFaceNodes);

        int aMinNode  = 0;
        int aMinIndex = -1;
        for (int aFaceNode = 0; aFaceNode < aNbFaceNodes; aFaceNode++, aFaceOffset++) {
            int aNode             = theFacesArray->GetInt(theFacesBegin + aFaceOffset, 0) + myNodesOffset;
            aFaceNodes[aFaceNode] = aNode;
            if (aMinIndex == -1 || aNode < aMinNode) {
                aMinIndex = aFaceNode;
                aMinNode  = aNode;
            }
        }

        // Face nodes start from the node with min index
        for (int aFaceNode = 0; aFaceNode < aNbFaceNodes; aFaceNode++) {
            int aNode = aFaceNodes[(aFaceNode + aMinIndex) % aNbFaceNodes];

            const int* aMappedNode = aNodesMap.Seek(aNode);
            if (aMappedNode == nullptr) {
                fail("VTK polyhedron face node is absent from cell connectivity");
                return;
            }
            aFaces.push_back(*aMappedNode);
        }
    }
    if (theFacesBegin + aFaceOffset != theFacesEnd) { fail("Unused VTK polyhedron face entries"); return; }
    anElem.SetFaces(aFaces.data(), static_cast<uint16_t>(aFaces.size()), false);

    myMeshData->AddElement(anElem);
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

}
