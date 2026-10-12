#include <RWMeshConv/MeshLoader.h>

#include <RWCae/MeshBinReader.h>

#include <RWMeshConv/GmshLoader.h>
#include <RWMed/RWMedReader.h>
#include <RWVtk/VtkMultiBlockDataReader.h>
#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkSeriesReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkLegacyReader.h>
#ifdef OCCTV_USE_VTK
#include <RWVtk/VtkNativeReader.h>
#endif

#include <BinTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <Message.hxx>
#include <OSD_FileSystem.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace {
//! Function to check if specified data stream starts with specified header.
static bool IsDataStartsWithHeader(const char (&theData)[2048], const char* theHeader)
{
    return ::strncmp(theData, theHeader, strlen(theHeader)) == 0;
}
}

namespace RWMeshConv {

// ================================================================
// Function : FileFormatFromName
// ================================================================
static std::string FileFormatFromName(const std::string& theName, bool theIsAll)
{
    TCollection_AsciiString aFileName(theName.c_str());
    aFileName.LowerCase();
    if (aFileName.EndsWith(".vtk")) { return "VTK"; }
    if (aFileName.EndsWith(".rmed") || aFileName.EndsWith(".med")) {
        return "MED";
    }

    if (aFileName.EndsWith(".vtu")) {
        return "VTU";
    }

    if (aFileName.EndsWith(".vtp")) {
        return "VTP";
    }

    if (aFileName.EndsWith(".vtm")) {
        return "VTM";
    }

    if (aFileName.EndsWith(".vtm.series")) {
        return "VTMSERIES";
    }

    if (!theIsAll) {
        return "";
    }
    if (aFileName.EndsWith(".pos") || aFileName.EndsWith(".msh")) {
        return "GMSH";
    }
    return "";
}

// ================================================================
// Function : ProbeFileFormat
// ================================================================
std::string MeshLoader::ProbeFileFormat(const std::string& theName,
                                        const char*        theBuffer,
                                        int                theDataLen,
                                        bool               theIsExtra)
{
    if (theDataLen <= 0) {
        return FileFormatFromName(theName, true);
    }

    char aBytes[2048] = {};
    std::copy(theBuffer, theBuffer + Min(theDataLen, (int)sizeof(aBytes)), aBytes);

    if (IsDataStartsWithHeader(aBytes, "# vtk DataFile Version")) {
        return "VTK";
    } else if (IsDataStartsWithHeader(aBytes, "$MeshFormat")) {
        return "GMSH";
    } else if (IsDataStartsWithHeader(aBytes, RWCae::MeshImportExport::MESH_DATA_HEADER)) {
        size_t anOffset = 0;
        for (; anOffset < sizeof(aBytes); ++anOffset) {
            if (::strncmp(aBytes + anOffset, "JSON", 0x4) == 0) {
                break;
            }
        }
        if (anOffset >= sizeof(aBytes)) {
            return "MDB";
        }

        const char* aProbGeom = ::strstr(aBytes + anOffset, "geometry");
        const char* aProbProp = ::strstr(aBytes + anOffset, "properties");
        if (aProbGeom != nullptr || aProbProp == nullptr) {
            return "MDB";
        }
        return "MDBPROP";
    } else if (!theIsExtra) {
        return FileFormatFromName(theName, false);
    }

    if (IsDataStartsWithHeader(aBytes, "DBRep_DrawableShape") ||  //
        IsDataStartsWithHeader(aBytes, "CASCADE Topology V") ||   //
        IsDataStartsWithHeader(aBytes, "\nCASCADE Topology V") || //
        IsDataStartsWithHeader(aBytes, "\nOpen CASCADE Topology V")) {
        const bool isBinary = IsDataStartsWithHeader(aBytes, "\nOpen CASCADE Topology V");
        return isBinary ? "BINBREP" : "BREP";
    }

    return FileFormatFromName(theName, false);
}

// ================================================================
// Function : OpenMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::OpenMesh(const Handle(OSD_FileSystem)&  theFileSystem,
                                               const TCollection_AsciiString& theFilePath,
                                               const TCollection_AsciiString& theMappingFilePath,
                                               const TCollection_AsciiString& theArtifactFilePath)
{
    myHasMissingFields = false;
    myVtkReport = RWVtk::VtkImportReport();
    myVtkReport.source = theFilePath.ToCString();

    // parse header
    char aBytes[2048] = {};
    {
        std::shared_ptr<std::istream> aFile = theFileSystem->OpenIStream(theFilePath, std::ios::in | std::ios::binary);
        if (aFile.get() == nullptr) {
            myVtkReport.errors.push_back("Input file is not accessible: " + myVtkReport.source);
            if (myToReportMessages) { Message::SendFail() << "[ERR_MESH_LOAD] " << myVtkReport.errors.front(); }
            return nullptr;
        }
        aFile->read(aBytes, sizeof(aBytes) - 1);
    }

    const std::string aForm = ProbeFileFormat(theFilePath.ToCString(), aBytes, (int)sizeof(aBytes), true);
    if (aForm == "VTK") {
        return openVtkMesh(theFilePath, theMappingFilePath, aForm);
    }
    if (aForm == "GMSH") {
        return OpenGmshMesh(theFilePath);
    } else if (aForm == "MDB" || aForm == "MDBPROP") {
        return OpenMdbMesh(theFilePath);
    }
    else if (aForm == "MED") {
        return OpenMedMesh(theFilePath, theMappingFilePath, theArtifactFilePath);
    }
     else if (aForm == "VTU") {
        return OpenVtuMesh(theFilePath, theMappingFilePath);
    } else if (aForm == "VTP") {
        return OpenVtpMesh(theFilePath, theMappingFilePath);
    } else if (aForm == "VTM") {
        return OpenVtmMesh(theFilePath, theMappingFilePath);
    } else if (aForm == "VTMSERIES") {
        return OpenVtmSeriesMesh(theFilePath, theMappingFilePath);
    } else if (aForm == "BREP" || aForm == "BINBREP") {
        return OpenBrepMesh(theFilePath, aForm == "BINBREP");
    }

    Message::SendFail() << "[ERR_MESH_FORMAT] file '" << theFilePath << "' has unsupported mesh format '" << aBytes << "'";
    return nullptr;
}

//! Check if shape defined triangulation.
static bool IsShapeTriangulated(const TopoDS_Shape& theShape, const TCollection_AsciiString& theFilePath)
{
    int aNbTris      = 0;
    int aNbTriNodes  = 0;
    int aNbNullFaces = 0;

    TopLoc_Location aLoc;
    for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next()) {
        const Handle(Poly_Triangulation)& aTris = BRep_Tool::Triangulation(TopoDS::Face(aFaceIter.Value()), aLoc);
        if (aTris.IsNull()) {
            ++aNbNullFaces;
            continue;
        }

        aNbTris += aTris->NbTriangles();
        aNbTriNodes += aTris->NbNodes();
    }

    if (aNbTris == 0 || aNbTriNodes == 0) {
        Message::SendFail() << "[ERR_MESH_LOAD] BREP file '" << theFilePath << "' doesn't define triangulations";
        return false;
    } else if (aNbNullFaces != 0) {
        Message::SendWarning() << "Warning: skipping " << aNbNullFaces << " faces without triangles from '"
                               << theFilePath << "'";
    }
    return true;
}

// ================================================================
// Function : OpenBrepMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::OpenBrepMesh(const TCollection_AsciiString& theFilePath, bool theIsBinary)
{
    myHasMissingFields = false;

    std::shared_ptr<std::istream> aFile = OSD_FileSystem::DefaultFileSystem()->OpenIStream(theFilePath.ToCString(),
                                                                                           std::ios::in
                                                                                               | std::ios::binary);
    if (aFile.get() == nullptr) {
        Message::SendFail() << "[ERR_MESH_LOAD] unable to open file '" << theFilePath << "'";
        return Handle(RWCae::MeshData)();
    }

    TopoDS_Shape aShape;
    BRep_Builder aBuilder;
    theIsBinary ? BinTools::Read(aShape, *aFile) : BRepTools::Read(aShape, *aFile, aBuilder);
    if (aShape.IsNull()) {
        Message::SendFail() << "[ERR_MESH_LOAD] unable to read BREP file '" << theFilePath << "'";
        return Handle(RWCae::MeshData)();
    } else if (!IsShapeTriangulated(aShape, theFilePath)) {
        return Handle(RWCae::MeshData)();
    }

    TopLoc_Location aLoc;

    Handle(RWCae::MeshData) aMesh = new RWCae::MeshData();
    for (TopExp_Explorer aFaceIter(aShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next()) {
        const Handle(Poly_Triangulation)& aTris = BRep_Tool::Triangulation(TopoDS::Face(aFaceIter.Value()), aLoc);
        if (aTris.IsNull()) {
            continue;
        }

        const int aFirstNode = aMesh->Nodes().Length();
        for (int aNodeIter = 1; aNodeIter <= aTris->NbNodes(); ++aNodeIter) {
            gp_Pnt aNode = aTris->Node(aNodeIter);
            aNode.Transform(aLoc.Transformation());
            aMesh->AddNodePosition(aNode.XYZ());
        }

        Graphic3d_Vec3i              anElem(0, 0, 0);
        RWCae::MeshElementTriangle aTrg;
        for (int aTriIter = 1; aTriIter <= aTris->NbTriangles(); ++aTriIter) {
            aTris->Triangle(aTriIter).Get(anElem.x(), anElem.y(), anElem.z());

            aTrg[0] = anElem[0] + aFirstNode - 1;
            aTrg[1] = anElem[1] + aFirstNode - 1;
            aTrg[2] = anElem[2] + aFirstNode - 1;
            aMesh->AddElement(aTrg);
        }
    }

    RWCae::MeshElement0d anElem0;
    for (TopExp_Explorer aVertIter(aShape, TopAbs_VERTEX, TopAbs_EDGE); aVertIter.More(); aVertIter.Next()) {
        const gp_Pnt aNode = BRep_Tool::Pnt(TopoDS::Vertex(aVertIter.Value()));
        anElem0[0]         = aMesh->AddNodePosition(aNode.XYZ());
        aMesh->AddElement(anElem0);
    }

    return aMesh;
}

// ================================================================
// Function : OpenMdbMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::OpenMdbMesh(const TCollection_AsciiString& theFilePath)
{
    myHasMissingFields = false;

    RWCae::MeshBinReader aBinReader;
    aBinReader.SetOptions(myOptions);
    aBinReader.SetPropsToProcess(myPropsToProcess);
    if (!aBinReader.Perform(theFilePath)) {
        Message::SendFail() << "[ERR_MESH_FORMAT] '" << theFilePath << "' is not a valid mesh binary file";
        return nullptr;
    }
    return aBinReader.GetMeshData();
}

// ================================================================
// Function : OpenGmshMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::OpenGmshMesh(const TCollection_AsciiString& theFilePath)
{
    myHasMissingFields = false;

    RWMeshConv::GmshLoader aLoader;
    if (!aLoader.Perform(theFilePath)) {
        Message::SendFail() << "[ERR_MESH_FORMAT] '" << theFilePath << "' is not a valid Gmsh mesh/postpro file";
        return nullptr;
    }

    const RWMeshConv::GmshSubMeshMap& aMeshes = aLoader.Meshes();
    if (aMeshes.IsEmpty()) {
        Message::SendFail() << "[ERR_MESH_LOAD] '" << theFilePath << "' doesn't contain any meshes";
        return nullptr;
    }

    RWMeshConv::GmshSubMeshMap::Iterator anIter(aMeshes);
    return anIter.Value()->Data;
}

// ================================================================
// Function : OpenMedMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::OpenMedMesh(const TCollection_AsciiString& theFilePath,
                                                  const TCollection_AsciiString& theMappingFilePath,
                                                  const TCollection_AsciiString& theArtifactFilePath)
{
    myHasMissingFields = false;

    RWMed::RWMedReader aMedReader;
    aMedReader.SetReadNodeVecAsScalar(myToReadNodeVecAsScalar);
    aMedReader.SetDumpPropsToTempFolder(myPropsTempFolder);
    if (!aMedReader.Perform(theFilePath.ToCString(), theMappingFilePath.ToCString(), theArtifactFilePath.ToCString())
        || aMedReader.Meshes().empty()) {
        return nullptr;
    }

    myHasMissingFields = aMedReader.GetStatus() == RWMed::RWMedReader::ReaderStatus::MissingFields;
    return aMedReader.Meshes().front();
}

// ================================================================
// Function : openVtkMesh
// ================================================================
Handle(RWCae::MeshData) MeshLoader::openVtkMesh(const TCollection_AsciiString& theFilePath,
                                               const TCollection_AsciiString& theMappingFilePath,
                                               const std::string& theFormat)
{
    myHasMissingFields = false;
    myVtkReport = RWVtk::VtkImportReport();
    myVtkReport.source = theFilePath.ToCString();
    myVtkReport.format = theFormat;
    Handle(RWCae::MeshData) aMesh;

    auto record = [&](const RWVtk::VtkImportReport& theReport, const char* theBackend, bool theSucceeded) {
        myVtkReport.attempts.push_back({theBackend, theSucceeded, theReport.errors, theReport.warnings});
        auto& anAttempt = myVtkReport.attempts.back();
        if (!theSucceeded && anAttempt.errors.empty()) { anAttempt.errors.push_back("Reader returned no mesh"); }
        myVtkReport.errors = anAttempt.errors;
        myVtkReport.warnings = anAttempt.warnings;
        myVtkReport.backend = theBackend;
    };
    auto read = [&](auto& theReader, auto theGetMesh) {
        theReader.SetMappingFile(theMappingFilePath);
        theReader.SetToReportMessages(false);
        bool isRead = false;
        RWVtk::VtkImportReport aReport;
        try {
            isRead = theReader.Perform(theFilePath);
            if (isRead) { aMesh = theGetMesh(); }
            aReport = theReader.Report();
        } catch (const Standard_Failure& anError) {
            aReport = theReader.Report();
            aReport.errors.push_back(anError.GetMessageString());
        } catch (const std::exception& anError) {
            aReport = theReader.Report();
            aReport.errors.push_back(anError.what());
        }
        record(aReport, "portable", isRead && !aMesh.IsNull() && aReport.errors.empty());
        if (!myVtkReport.attempts.back().succeeded) { aMesh.Nullify(); }
    };
    if (theFormat == "VTK") {
        RWVtk::VtkLegacyReader aReader;
        read(aReader, [&]() { return aReader.GetMeshData(); });
    } else if (theFormat == "VTU") {
        RWVtk::VtkUnstructuredGridReader aReader;
        read(aReader, [&]() { return aReader.GetMeshData(); });
    } else if (theFormat == "VTP") {
        RWVtk::VtkPolyDataReader aReader;
        read(aReader, [&]() { return aReader.GetMeshData(); });
    } else if (theFormat == "VTM") {
        RWVtk::VtkMultiBlockDataReader aReader;
        read(aReader, [&]() { return aReader.GetMergedMeshData(); });
    } else if (theFormat == "VTMSERIES") {
        RWVtk::VtkSeriesReader aReader;
        read(aReader, [&]() { return aReader.GetMergedMeshData(); });
    }
#ifdef OCCTV_USE_VTK
    // Native fallback does not implement mappings or composite/time-series semantics.
    if (aMesh.IsNull() && theMappingFilePath.IsEmpty()
        && (theFormat == "VTK" || theFormat == "VTU" || theFormat == "VTP")) {
        RWVtk::VtkNativeReader aReader;
        const bool isRead = aReader.Perform(theFilePath);
        record(aReader.Report(), "native-vtk", isRead);
        if (isRead) { aMesh = aReader.GetMeshData(); }
    }
#endif
    myVtkReport.succeeded = !aMesh.IsNull();
    if (myVtkReport.succeeded) {
        // Counts describe the published mesh, not a rejected partial attempt.
        myVtkReport.points = aMesh->NbNodes();
        myVtkReport.cells = aMesh->NbIndexedElems();
        myVtkReport.fields = aMesh->Properties().Size();
        myVtkReport.errors.clear();
    } else {
        myVtkReport.errors.clear();
        for (const auto& anAttempt : myVtkReport.attempts) {
            for (const auto& anError : anAttempt.errors) {
                myVtkReport.errors.push_back(anAttempt.backend + ": " + anError);
            }
        }
    }
    if (myToReportMessages) {
        for (const auto& anError : myVtkReport.errors) {
            Message::SendFail() << "[ERR_VTK_IMPORT] " << myVtkReport.source << ": " << anError;
        }
        for (const auto& aWarning : myVtkReport.warnings) {
            Message::SendWarning() << "[VTK_IMPORT] " << myVtkReport.source << ": " << aWarning;
        }
    }
    return aMesh;
}

Handle(RWCae::MeshData) MeshLoader::OpenVtuMesh(const TCollection_AsciiString& theFilePath,
                                              const TCollection_AsciiString& theMappingFilePath)
{
    return openVtkMesh(theFilePath, theMappingFilePath, "VTU");
}

Handle(RWCae::MeshData) MeshLoader::OpenVtpMesh(const TCollection_AsciiString& theFilePath,
                                              const TCollection_AsciiString& theMappingFilePath)
{
    return openVtkMesh(theFilePath, theMappingFilePath, "VTP");
}

Handle(RWCae::MeshData) MeshLoader::OpenVtmMesh(const TCollection_AsciiString& theFilePath,
                                              const TCollection_AsciiString& theMappingFilePath)
{
    return openVtkMesh(theFilePath, theMappingFilePath, "VTM");
}

Handle(RWCae::MeshData) MeshLoader::OpenVtmSeriesMesh(const TCollection_AsciiString& theFilePath,
                                                    const TCollection_AsciiString& theMappingFilePath)
{
    return openVtkMesh(theFilePath, theMappingFilePath, "VTMSERIES");
}

// ================================================================
// Function : LoadMeshProperties
// ================================================================
bool MeshLoader::LoadMeshProperties(const Handle(RWCae::MeshData)&                    theMesh,
                                    const Handle(OSD_FileSystem)&                       theFileSystem,
                                    const TCollection_AsciiString&                      theFilePath,
                                    NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theUpdatedProps)
{
    // parse header
    char aBytes[2048] = {};
    {
        std::shared_ptr<std::istream> aFile = theFileSystem->OpenIStream(theFilePath, std::ios::in | std::ios::binary);
        if (aFile.get() == nullptr) {
            Message::SendFail() << "[ERR_MESH_LOAD] the file '" << theFilePath << "' cannot be opened";
            return false;
        }
        aFile->read(aBytes, sizeof(aBytes) - 1);
    }
    const std::string aForm = ProbeFileFormat(theFilePath.ToCString(), aBytes, (int)sizeof(aBytes));
    if (aForm == "GMSH") {
        RWMeshConv::GmshLoader aLoader(theMesh);
        aLoader.SetSectionTypesToRead((unsigned int)RWMeshConv::GmshSectionType::Property);
        if (!aLoader.Perform(theFilePath)) {
            Message::SendFail() << "[ERR_MESH_FORMAT] '" << theFilePath << "' is not a valid Gmsh mesh/postpro file";
            return false;
        }
    } else if (aForm == "MDB" || aForm == "MDBPROP") {
        RWCae::MeshBinReader aBinReader(theMesh);
        myOptions |= (unsigned int)RWCae::MeshDataImportExportOptions::SKIP_GEOMETRY;
        aBinReader.SetOptions(myOptions);
        if (!aBinReader.Perform(theFilePath)) {
            Message::SendFail() << "[ERR_MESH_FORMAT] '" << theFilePath << "' is not a valid mesh binary file";
            return false;
        }
        theUpdatedProps = aBinReader.UpdatedProps();
    } else {
        Message::SendFail() << "[ERR_MESH_FORMAT] file '" << theFilePath << "' has unsupported mesh format '" << aBytes << "'";
        return false;
    }

    return true;
}
}

