

#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <map>
#include <set>
#include <RWCae/MeshCellTopology.h>
#include <cmath>

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

    myNbElements = readCount(theElem,"NumberOfCells","numberOfCells");
    VtkResourceBudget::Consume(myBudget->Cells,myNbElements,std::min(myLimits.MaxCells,(uint64_t)std::numeric_limits<int>::max()),"cells");
    if (myNbElements < 0) {
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

    for (const char* association : {"PointData", "CellData"}) {
        const auto section = getFirstElement(theElem, association);
        const bool nodal = std::string(association) == "PointData";
        const char* role = nodal ? "RationalWeights" : "HigherOrderDegrees";
        TCollection_AsciiString selected = section.isNull() ? TCollection_AsciiString() : TCollection_AsciiString(section.getAttribute(role));
        for (const auto& element : getChildrenElementsByTagName(section,"DataArray")) {
            const TCollection_AsciiString name = getXmlAttribute(element,"Name","name");
            if (name != (selected.IsEmpty() ? TCollection_AsciiString(role) : selected)) { continue; }
            auto array = readDataArray(element);
            if (array.Data.IsNull() || array.Components.size() != (nodal ? 1u : 3u)
                || array.Data->NbElements() != (nodal ? myNbNodes : myNbElements)) { return fail("Invalid cell order/rational weight array"); }
            for (int n = 0; n < array.Data->NbElements(); ++n) {
                for (int c = 0; c < (int)array.Components.size(); ++c) {
                    const double v = array.Data->GetDouble(n,c);
                    if (!std::isfinite(v) || (nodal ? v <= 0 : v < 0 || v > 1024 || std::trunc(v) != v)) { return fail("Invalid higher-order degree or rational weight"); }
                }
            }
            (nodal ? myWeights : myDegrees) = array;
            myTopologyAttributes.emplace(std::string(association)+":"+name.ToCString(),array);
        }
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

    const bool modern = !aMeshElements.FaceConnectivity.Data.IsNull() || !aMeshElements.FaceEnds.Data.IsNull()
        || !aMeshElements.PolyhedronFaces.Data.IsNull() || !aMeshElements.PolyhedronEnds.Data.IsNull();
    const bool legacy = !aMeshElements.Faces.Data.IsNull() || !aMeshElements.FaceOffsets.Data.IsNull();
    if (modern && legacy) { return fail("Ambiguous old and XML 2.3 polyhedron face arrays"); }
    auto scalar = [](const DataArray& array) { return !array.Data.IsNull() && array.Components.size() == 1; };
    if (modern) {
        const auto& fc = aMeshElements.FaceConnectivity; const auto& fo = aMeshElements.FaceEnds;
        const auto& pf = aMeshElements.PolyhedronFaces; const auto& po = aMeshElements.PolyhedronEnds;
        if (!scalar(fc) || !scalar(fo) || !scalar(pf) || !scalar(po) || po.Data->NbElements() != myNbElements) {
            return fail("Incomplete/non-scalar XML 2.3 polyhedron face arrays");
        }
        int begin = 0;
        for (int f = 0; f < fo.Data->NbElements(); ++f) {
            const int end = fo.Data->GetInt(f,0);
            if (end < begin || end-begin < 3 || end > fc.Data->NbElements()) { return fail("Invalid XML 2.3 face offset"); }
            for (int n = begin; n < end; ++n) {
                const int id = fc.Data->GetInt(n,0);
                if (id < 0 || id >= myNbNodes) { return fail("XML 2.3 face point index out of range"); }
            }
            begin = end;
        }
        if (begin != fc.Data->NbElements()) { return fail("Unused XML 2.3 face connectivity"); }
        begin = 0; std::vector<bool> referenced(fo.Data->NbElements(),false);
        for (int i = 0; i < myNbElements; ++i) {
            const int end = po.Data->GetInt(i,0);
            const bool polyhedron = aMeshElements.Types.Data->GetInt(i,0) == 42;
            if (end < begin || end > pf.Data->NbElements() || (polyhedron ? end-begin < 4 : end != begin)) {
                return fail("Invalid XML 2.3 polyhedron offset");
            }
            for (int n = begin; n < end; ++n) {
                const int id = pf.Data->GetInt(n,0);
                if (id < 0 || id >= fo.Data->NbElements()) { return fail("XML 2.3 face index out of range"); }
                referenced[id] = true;
            }
            begin = end;
        }
        if (begin != pf.Data->NbElements() || std::find(referenced.begin(),referenced.end(),false) != referenced.end()) {
            return fail("Unused XML 2.3 polyhedron face data");
        }
    } else if (legacy) {
        if (!scalar(aMeshElements.Faces) || !scalar(aMeshElements.FaceOffsets)
            || aMeshElements.FaceOffsets.Data->NbElements() != myNbElements) { return fail("Invalid old polyhedron face arrays"); }
        int end = 0;
        for (int i = 0; i < myNbElements; ++i) {
            const int offset = aMeshElements.FaceOffsets.Data->GetInt(i,0);
            if (aMeshElements.Types.Data->GetInt(i,0) == 42) {
                if (offset <= end || offset > aMeshElements.Faces.Data->NbElements()) { return fail("Invalid polyhedron face offset"); }
                end = offset;
            } else if (offset != -1) { return fail("Non-polyhedron must have faceoffset -1"); }
        }
        if (end != aMeshElements.Faces.Data->NbElements()) { return fail("Unused old polyhedron face data"); }
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
        const int faceBegin = aFacesBegin;
        int aCellType = theElems.Types.Data->GetInt(i, 0);

        int aCellEnd = theElems.Offsets.Data->GetInt(i, 0);

        if (aCellEnd < aCellBegin || aCellEnd > theElems.Connectivity.Data->NbElements()) {
            fail("Invalid VTK connectivity offset");
            return;
        }
        const Handle(VtkAbstractArray)& aData = theElems.Connectivity.Data;

        RWCae::MeshData::GenericCell topology; topology.Type = aCellType;
        if (!myDegrees.Data.IsNull()) { for (int c = 0; c < 3; ++c) { topology.Degrees.push_back(myDegrees.Data->GetInt(i,c)); } }
        if (!RWCae::MeshCellTopology::ValidSize(aCellType,aCellEnd-aCellBegin,topology.Degrees)) {
            fail("VTK cell connectivity has incorrect type/node count/degrees"); return;
        }
        for (int node = aCellBegin; node < aCellEnd; ++node) {
            const int id = aData->GetInt(node, 0);
            if (id < 0 || id >= myNbNodes) { fail("VTK connectivity point index out of range"); return; }
        }

        for (int n = aCellBegin; n < aCellEnd; ++n) {
            const int id = aData->GetInt(n,0); topology.Nodes.push_back(id+myNodesOffset);
            if (!myWeights.Data.IsNull()) { topology.RationalWeights.push_back(myWeights.Data->GetDouble(id,0)); }
        }
        if (aCellType == 42) {
            const std::set<RWCae::MeshEntityId> members(topology.Nodes.begin(),topology.Nodes.end());
            auto appendFace = [&](const DataArray& array, int begin, int end) {
                if (begin < 0 || end-begin < 3 || end > array.Data->NbElements()) { throw std::runtime_error("Invalid polyhedron face length"); }
                std::vector<RWCae::MeshEntityId> face;
                for (int n = begin; n < end; ++n) {
                    const auto id = array.Data->GetInt(n,0)+myNodesOffset;
                    if (!members.count(id)) { throw std::runtime_error("Polyhedron face point is not in cell connectivity"); }
                    face.push_back(id);
                }
                topology.Faces.push_back(std::move(face));
            };
            if (!theElems.FaceConnectivity.Data.IsNull()) {
                const auto& fc = theElems.FaceConnectivity; const auto& fo = theElems.FaceEnds;
                const auto& pf = theElems.PolyhedronFaces; const auto& po = theElems.PolyhedronEnds;
                if (fo.Data.IsNull() || pf.Data.IsNull() || po.Data.IsNull() || po.Data->NbElements()!=myNbElements) { fail("Incomplete XML 2.3 polyhedron face arrays"); return; }
                const int end = po.Data->GetInt(i,0);
                if (end-aFacesBegin < 4 || end > pf.Data->NbElements()) { fail("Invalid polyhedron face references"); return; }
                for (int n = aFacesBegin; n < end; ++n) {
                    const int f = pf.Data->GetInt(n,0);
                    if (f < 0 || f >= fo.Data->NbElements()) { fail("Invalid polyhedron face index"); return; }
                    appendFace(fc,f ? fo.Data->GetInt(f-1,0) : 0,fo.Data->GetInt(f,0));
                }
                aFacesBegin = end;
            } else {
                const auto& faces = theElems.Faces; const auto& offsets = theElems.FaceOffsets;
                if (faces.Data.IsNull() || offsets.Data.IsNull() || offsets.Data->NbElements()!=myNbElements) { fail("Invalid or missing VTK polyhedron face arrays"); return; }
                const int end = offsets.Data->GetInt(i,0);
                if (end <= aFacesBegin || end > faces.Data->NbElements()) { fail("Invalid polyhedron face offset"); return; }
                int at = aFacesBegin; const int count = faces.Data->GetInt(at++,0);
                if (count < 4 || count > end-at) { fail("Invalid polyhedron face count"); return; }
                for (int f = 0; f < count; ++f) {
                    if (at >= end) { fail("Truncated polyhedron face stream"); return; }
                    const int size = faces.Data->GetInt(at++,0);
                    if (size < 3 || size > end-at) { fail("Invalid polyhedron face length"); return; }
                    appendFace(faces,at,at+size); at += size;
                }
                if (at != end) { fail("Unused polyhedron face data"); return; }
                aFacesBegin = end;
            }
        }
        const bool generic = !theElems.FaceConnectivity.Data.IsNull() || myGeneralLayout || !topology.Degrees.empty() || !topology.RationalWeights.empty()
            || aCellType == 0 || aCellType == 2 || aCellType == 3 || aCellType == 4 || aCellType == 6 || aCellType == 8 || aCellType == 11
            || aCellType == 15 || aCellType == 16 || aCellType == 21 || (aCellType >= 28 && aCellType != 42);
        if (generic) {
            myMeshData->AddGenericCell(topology); (*myStoredElements)[myCurPiece][myNbProcessed+i]++;
        } else switch (aCellType) {
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
                    || theElems.FaceOffsets.Data->GetInt(i, 0) < aFacesBegin
                    || theElems.FaceOffsets.Data->GetInt(i, 0) > theElems.Faces.Data->NbElements()) {
                    fail("Invalid or missing VTK polyhedron face arrays");
                    return;
                }
                addPolyhedron(aData, theElems.Faces.Data, aCellBegin, aCellEnd, faceBegin,
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
