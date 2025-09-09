

#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>

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
        Message::SendFail() << "Error: Invalid number of elements '" << myNbElements << "'";
        return false;
    }

    (*myStoredElements)[myCurPiece].resize(myNbElements, 0);

    return true;
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
        Message::SendFail() << "Error: 'UnstructuredGrid/Piece/Cells' element not found";
        return false;
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
    MeshElements aMeshElements;
    parseElements(theElem, aMeshElements);

    if (aMeshElements.Connectivity.Data.IsNull() || aMeshElements.Offsets.Data.IsNull()
        || aMeshElements.Types.Data.IsNull()) {
        Message::SendFail() << "Error: Failed to read cells data";
        return false;
    }

    if (aMeshElements.Offsets.Data->NbElements() != myNbElements
        || aMeshElements.Types.Data->NbElements() != myNbElements) {
        Message::SendFail() << "Error: Incorrect number of cells '" << aMeshElements.Offsets.Data->NbElements() << ", "
                            << aMeshElements.Types.Data->NbElements() << "', should be '" << myNbElements << "'";
        return false;
    }

    NCollection_DataMap<int, int> aSkippedElements;

    parseCells(aMeshElements, aSkippedElements);

    myNbProcessed += myNbElements;

    if (!aSkippedElements.IsEmpty()) {
        Message::SendInfo() << "Skipped elements: ";
        for (NCollection_DataMap<int, int>::Iterator anIter(aSkippedElements); anIter.More(); anIter.Next()) {
            Message::SendInfo() << "Type = " << anIter.Key() << ", Nb elements = " << anIter.Value();
        }
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

        const Handle(VtkAbstractArray)& aData = theElems.Connectivity.Data;

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
                addPolyhedron(aData, theElems.Faces.Data, aCellBegin, aCellEnd, aFacesBegin, i);
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
    if (aNbFaces < 1) {
        Message::SendWarning() << "Error: Polyhedron without faces, skipped";
        return;
    }

    std::vector<RWCae::MeshElemEntityId> aFaces;
    std::vector<int>                       aFaceNodes;
    for (int aFaceId = 0, aFaceOffset = 1; aFaceId < aNbFaces; aFaceId++) {
        int aNbFaceNodes = theFacesArray->GetInt(theFacesBegin + aFaceOffset, 0);
        aFaces.push_back(aNbFaceNodes);
        aFaceOffset++;
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
            int aNode = aFaceNodes[(aFaceNode + aMinNode) % aNbFaceNodes];

            const int* aMappedNode = aNodesMap.Seek(aNode);
            if (aMappedNode == nullptr) {
                Message::SendWarning() << "Error: Invalid polyhedron face node index, skipped";
                return;
            }
            aFaces.push_back(*aMappedNode);
        }
    }
    anElem.SetFaces(aFaces.data(), static_cast<uint16_t>(aFaces.size()), false);

    myMeshData->AddElement(anElem);
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

}