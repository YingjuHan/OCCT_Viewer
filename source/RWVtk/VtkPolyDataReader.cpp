

#include <RWVtk/VtkPolyDataReader.h>

#include <BRepMesh_Triangulator.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <TColStd_SequenceOfInteger.hxx>

namespace RWVtk {

// ================================================================
// function : VtkPolyDataReader
// ================================================================
VtkPolyDataReader::VtkPolyDataReader() : VtkUnstructuredDataReader()
{
    //
}

// ================================================================
// function : ~VtkPolyDataReader
// ================================================================
VtkPolyDataReader::~VtkPolyDataReader()
{
    //
}

// =======================================================================
// function : readMeshInfo
// =======================================================================
bool VtkPolyDataReader::readMeshInfo(const LDOM_Element& theElem)
{
    if (!VtkUnstructuredDataReader::readMeshInfo(theElem)) {
        return false;
    }

    getXmlAttribute(theElem, "NumberOfVerts", "numberOfVerts").GetInteger(myNbVerts);
    getXmlAttribute(theElem, "NumberOfLines", "numberOfLines").GetInteger(myNbLines);
    getXmlAttribute(theElem, "NumberOfStrips", "numberOfStrips").GetInteger(myNbStrips);
    getXmlAttribute(theElem, "NumberOfPolys", "numberOfPolys").GetInteger(myNbPolys);

    (*myStoredElements)[myCurPiece].resize(myNbVerts + myNbLines + myNbStrips + myNbPolys, 0);

    return true;
}

// =======================================================================
// function : readMeshGeometry
// =======================================================================
bool VtkPolyDataReader::readMeshGeometry(const LDOM_Element& theElem)
{
    if (!VtkUnstructuredDataReader::readMeshGeometry(theElem)) {
        return false;
    }

    LDOM_Element aVerts = getFirstElement(theElem, "Verts");
    if (!aVerts.isNull() && myNbVerts > 0) {
        if (!parseVerts(aVerts)) {
            return false;
        }
    }

    LDOM_Element aLines = getFirstElement(theElem, "Lines");
    if (!aLines.isNull() && myNbLines > 0) {
        Message::SendInfo() << "1d elements are not supported";
    }

    LDOM_Element aStrips = getFirstElement(theElem, "Strips");
    if (!aStrips.isNull() && myNbStrips > 0) {
        if (!parseStrips(aStrips)) {
            return false;
        }
    }

    LDOM_Element aPolys = getFirstElement(theElem, "Polys");
    if (!aPolys.isNull() && myNbPolys > 0) {
        if (!parsePolys(aPolys)) {
            return false;
        }
    }

    return true;
}

// =======================================================================
// function : parseVerts
// =======================================================================
bool VtkPolyDataReader::parseVerts(const LDOM_Element& theElem)
{
    MeshElements aMeshElements;
    parseElements(theElem, aMeshElements);

    if (aMeshElements.Connectivity.Data.IsNull() || aMeshElements.Offsets.Data.IsNull()) {
        Message::SendFail() << "Error: Failed to read verts data";
        return false;
    }

    if (aMeshElements.Offsets.Data->NbElements() != myNbVerts) {
        Message::SendFail() << "Error: Incorrect number of verts '" << aMeshElements.Offsets.Data->NbElements()
                            << "', should be '" << myNbVerts << "'";
        return false;
    }

    int aCellBegin = 0;
    int aCurOffset = 0;

    RWCae::MeshElement0d anElem;
    for (int i = 0; i < myNbVerts; i++, aCellBegin = aCurOffset) {
        aCurOffset = aMeshElements.Offsets.Data->GetInt(i, 0);
        for (int aVertIter = aCellBegin; aVertIter < aCurOffset; aVertIter++) {
            uint32_t aNode = aMeshElements.Connectivity.Data->GetInt(aVertIter, 0) + myNodesOffset;
            anElem[0]      = aNode;
            myMeshData->AddElement(anElem);
            (*myStoredElements)[myCurPiece][myNbProcessed + i]++;
        }
    }

    myNbProcessed += myNbVerts;

    return true;
}

// =======================================================================
// function : parseVerts
// =======================================================================
bool VtkPolyDataReader::parseStrips(const LDOM_Element& theElem)
{
    MeshElements aMeshElements;
    parseElements(theElem, aMeshElements);

    if (aMeshElements.Connectivity.Data.IsNull() || aMeshElements.Offsets.Data.IsNull()) {
        Message::SendFail() << "Error: Failed to read strips data";
        return false;
    }

    if (aMeshElements.Offsets.Data->NbElements() != myNbStrips) {
        Message::SendFail() << "Error: Incorrect number of strips '" << aMeshElements.Offsets.Data->NbElements()
                            << "', should be '" << myNbStrips << "'";
        return false;
    }

    constexpr int aNbTrgPnts = 3;

    int aCellBegin = 0;
    int aCurOffset = 0;
    for (int i = 0; i < myNbStrips; i++, aCellBegin = aCurOffset) {
        aCurOffset = aMeshElements.Offsets.Data->GetInt(i, 0);
        if (aCurOffset - aCellBegin < aNbTrgPnts) {
            continue;
        }

        int aVert1 = aMeshElements.Connectivity.Data->GetInt(aCellBegin, 0) + myNodesOffset;
        int aVert2 = aMeshElements.Connectivity.Data->GetInt(aCellBegin + 1, 0) + myNodesOffset;
        for (int aVertIter = aCellBegin + 2; aVertIter < aCurOffset; aVertIter++) {
            int aVert3 = aMeshElements.Connectivity.Data->GetInt(aVertIter, 0) + myNodesOffset;
            addElemTriangle(aVert1, aVert2, aVert3, i);

            aVert1 = aVert2;
            aVert2 = aVert3;
        }
    }

    myNbProcessed += myNbStrips;

    return true;
}

// =======================================================================
// function : parsePolys
// =======================================================================
bool VtkPolyDataReader::parsePolys(const LDOM_Element& theElem)
{
    MeshElements aMeshElements;
    parseElements(theElem, aMeshElements);

    if (aMeshElements.Connectivity.Data.IsNull() || aMeshElements.Offsets.Data.IsNull()) {
        Message::SendFail() << "Error: Failed to read polys data";
        return false;
    }

    if (aMeshElements.Offsets.Data->NbElements() != myNbPolys) {
        Message::SendFail() << "Error: Incorrect number of polys '" << aMeshElements.Offsets.Data->NbElements()
                            << "', should be '" << myNbPolys << "'";
        return false;
    }

    constexpr int aNbTrgPnts = 3;

    int aCellBegin = 0;
    int aCurOffset = 0;
    for (int i = 0; i < myNbPolys; i++, aCellBegin = aCurOffset) {
        aCurOffset = aMeshElements.Offsets.Data->GetInt(i, 0);
        if (aCurOffset - aCellBegin < aNbTrgPnts) {
            continue;
        }
        addPolygon(aMeshElements.Connectivity.Data, aCellBegin, aCurOffset, i);
    }

    myNbProcessed += myNbPolys;

    return true;
}

}           
