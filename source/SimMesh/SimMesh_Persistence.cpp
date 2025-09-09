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

