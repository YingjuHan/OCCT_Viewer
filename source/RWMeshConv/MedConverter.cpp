#include <RWMeshConv/MedConverter.h>

#include <RWMeshConv/MeshLoader.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshBinReader.h>
#include <RWCae/MeshBinWriter.h>
#include <RWCae/MeshTools.h>

#include <BinTools.hxx>
#include <BRep_Builder.hxx>
#include <Message.hxx>
#include <OSD_File.hxx>
#include <OSD_FileSystem.hxx>
#include <OSD_Directory.hxx>
#include <OSD_Protection.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopoDS_Iterator.hxx>

namespace RWMeshConv {

// ================================================================
// Function : MedConverter
// ================================================================
MedConverter::MedConverter()
    : myInputFormat(FileFormat::UNKNOWN),
      myOutputFormat(FileFormat::UNKNOWN),
      myConvStatus(ConversionStatus::NotDone)
{
    //
}

// ================================================================
// Function : Destructor
// ================================================================
MedConverter::~MedConverter()
{
    //
}

// ================================================================
// Function : IsInputFormatSupported
// ================================================================
bool MedConverter::IsInputFormatSupported(const FileFormat theFormat)
{
    return theFormat == FileFormat::MDB || theFormat == FileFormat::MED || theFormat == FileFormat::GMSH22;
}

// ================================================================
// Function : IsOutputFormatSupported
// ================================================================
bool MedConverter::IsOutputFormatSupported(const FileFormat theFormat)
{
    return (theFormat == FileFormat::MDB);
}

// ================================================================
// Function : Perform
// ================================================================
bool MedConverter::Perform()
{
    if (myInputPath.empty() || myOutputPath.empty()) {
        Message::SendFail() << "RWMeshConv::Converter::Perform() invoked with invalid input/output paths";
        return false;
    }

    if (!myPropsTempFolder.empty()) {
        if (!checkAndCreateFolder(myPropsTempFolder.c_str())) {
            Message::SendFail()
                << "RWMeshConv::Converter::Perform() failed to create folder for temporary fields files";
            return false;
        }
    }

    // Import the input file into MeshData container
    Handle(RWCae::MeshData) aMeshData = readMesh();
    if (aMeshData.IsNull()) {
        Message::SendFail() << "RWMeshConv::Converter::Perform() failed to import the input file";
        return false;
    }

    if (myToSortMeshData) {
        RWCae::MeshTools::SortMeshData(aMeshData);
    }

    // Write MeshData container to the output file
    bool aRes = writeMesh(aMeshData);
    if (aRes) {
        Message::SendInfo() << "RWMeshConv::Converter::Perform() completed successfully.";
    } else {
        Message::SendFail() << "RWMeshConv::Converter::Perform() operation was not done.";
    }

    if (!myPropsTempFolder.empty()) {
        for (const Handle(RWCae::MeshNVecProperty)& aProp : aMeshData->Properties()) {
            if (aProp->RefFilePath().IsEmpty()) {
                continue;
            }
            OSD_File aFile(aProp->RefFilePath());
            if (aFile.Exists()) {
                aFile.Remove();
            }
        }
    }

    return aRes;
}

// ================================================================
// Function : readMesh
// ================================================================
Handle(RWCae::MeshData) MedConverter::readMesh()
{
    try {
        OCC_CATCH_SIGNALS
        RWMeshConv::MeshLoader aLoader;
        aLoader.SetReadNodeVecAsScalar(myToReadNodeVecAsScalar);
        aLoader.SetDumpPropsToTempFolder(myPropsTempFolder.c_str());

        Handle(RWCae::MeshData) aMeshData;
        switch (myInputFormat) {
            case FileFormat::MDB: {
                aMeshData = aLoader.OpenMdbMesh(myInputPath.c_str());
                break;
            }
            case FileFormat::MED: {
                // aMeshData = aLoader.OpenMedMesh(myInputPath.c_str(),
                //                                 myMappingPath.c_str(),
                //                                 myMissingFieldsPath.c_str());
                break;
            }
            case FileFormat::GMSH22: {
                aMeshData = aLoader.OpenGmshMesh(myInputPath.c_str());
                break;
            }
            case FileFormat::BREP: {
                aMeshData = aLoader.OpenBrepMesh(myInputPath.c_str(), true);
                break;
            }
            case FileFormat::UNKNOWN: {
                aMeshData = aLoader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),
                                             myInputPath.c_str(),
                                             myMappingPath.c_str(),
                                             myMissingFieldsPath.c_str());
                break;
            }
        }
        if (aLoader.HasMissingFields()) {
            myConvStatus = ConversionStatus::MissingFields;
        }
        return aMeshData;
    } catch (const Standard_Failure& aFailure) {
        Message::SendFail() << "RWMeshConv::MeshLoader::OpenMesh() failed with a Standard_Failure exception:\n"
                            << aFailure;
        return Handle(RWCae::MeshData)();
    } catch (const std::exception& e) {
        Message::SendFail() << "RWMeshConv::MeshLoader::OpenMesh() failed with a STL exception:\n" << e.what();
        return Handle(RWCae::MeshData)();
    }

    return Handle(RWCae::MeshData)();
}

// ================================================================
// Function : writeMesh
// ================================================================
bool MedConverter::writeMesh(const Handle(RWCae::MeshData)& theMeshData)
{
    FileFormat aFormat = myOutputFormat;
    if (myOutputFormat == FileFormat::UNKNOWN) {
        aFormat = FileFormat::MDB;
        TCollection_AsciiString aNameCase(myOutputPath.c_str());
        aNameCase.LowerCase();
        if (aNameCase.EndsWith(".brep")) {
            aFormat = FileFormat::BREP;
        }
    }

    try {
        OCC_CATCH_SIGNALS
        switch (aFormat) {
            case FileFormat::UNKNOWN:
            case FileFormat::MDB: {
                RWCae::MeshBinWriter aWriter(theMeshData);
                if (aWriter.Perform(myOutputPath.c_str())) {
                    myConvStatus = ConversionStatus::Done;
                    Message::SendInfo() << "RWMeshConv::Converter::writeMesh() wrote a dataset to '" << myOutputPath
                                        << "'";
                    return true;
                }

                Message::SendFail() << "RWMeshConv::Converter::writeMesh() failed to write a dataset to '"
                                    << myOutputPath << "'";
                return false;
            }
            case FileFormat::BREP: {
                return writeBrepMesh(theMeshData);
            }
            default: {
                Message::SendFail() << "RWMeshConv::Converter::Perform() invoked for unsupported output format";
                return false;
            }
        }
    } catch (const Standard_Failure& aFailure) {
        Message::SendFail() << "RWMeshConv::Converter::writeMesh() failed with a Standard_Failure exception:\n"
                            << aFailure;
    } catch (const std::exception& e) {
        Message::SendFail() << "RWMeshConv::Converter::writeMesh() failed with a STL exception:\n" << e.what();
    }

    return false;
}

//! Convert mesh into triangulation.
template<class Elem_T>
static int MeshToPolyTris(const RWCae::MeshAdaptor<Elem_T>& theMeshAdaptor, const Handle(Poly_Triangulation)& theTris)
{
    RWCae::MeshEntityId aNbElems = theMeshAdaptor.NbElements();
    if (aNbElems == 0) {
        return 0;
    }

    gp_Pnt aNode;
    int    aNodeIndex = 0;

    RWCae::MeshEntityId aNbNodes = theMeshAdaptor.NbNodes();
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter) {
        theMeshAdaptor.NodePosition(aNode, aNodeIter);
        theTris->SetNode(++aNodeIndex, aNode);
    }

    int aTriIndex = 0;

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theMeshAdaptor.GetElement(anElemIter);

        RWCae::MeshElemEntityId aNbFaces = theMeshAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(theMeshAdaptor, anElem, aFaceIter);
                 aTrgIter.More();
                 aTrgIter.Next()) {
                const Graphic3d_Vec3u& aTrg = aTrgIter.Triangle();
                theTris->SetTriangle(++aTriIndex, Poly_Triangle(aTrg.x(), aTrg.y(), aTrg.z()));
            }
        }
    }

    return aTriIndex;
}

//! Convert mesh 2d elements into triangulation.
Handle(Poly_Triangulation) Mesh2dToPolyTris(const Handle(RWCae::MeshData)& theMeshData, bool theIsSurface)
{
    int  aNbTris2d     = 0;
    auto computeNbTrgs = [&aNbTris2d](const auto& theAdaptor) {
        aNbTris2d += static_cast<int>(theAdaptor.NbTriangles());
    };

    RWCae::MeshAdaptorsIterator2dElems::For(computeNbTrgs, theMeshData, theIsSurface);

    if (aNbTris2d == 0) {
        return nullptr;
    }

    Handle(Poly_Triangulation) aTris2d = new Poly_Triangulation(theMeshData->NbNodes(), aNbTris2d, false);

    auto computeTrg = [&aTris2d](const auto& theAdaptor) {
        MeshToPolyTris(theAdaptor, aTris2d);
    };

    RWCae::MeshAdaptorsIterator2dElems::For(computeTrg, theMeshData, theIsSurface);

    return aTris2d;
}

// ================================================================
// Function : writeBrepMesh
// ================================================================
bool MedConverter::writeBrepMesh(const Handle(RWCae::MeshData)& theMeshData)
{
    Handle(Poly_Triangulation) aTris2d = Mesh2dToPolyTris(theMeshData, false);
    Handle(Poly_Triangulation) aTris3d = Mesh2dToPolyTris(theMeshData, true);

    TopoDS_Compound aComp;
    BRep_Builder().MakeCompound(aComp);
    if (!aTris2d.IsNull()) {
        TopoDS_Face aFace;
        BRep_Builder().MakeFace(aFace, aTris2d);
        BRep_Builder().Add(aComp, aFace);
    }
    if (!aTris3d.IsNull()) {
        TopoDS_Face aFace;
        BRep_Builder().MakeFace(aFace, aTris3d);
        BRep_Builder().Add(aComp, aFace);
    }
    if (theMeshData->NbElems0d() > 0) {
        TopoDS_Compound aComp0d;
        BRep_Builder().MakeCompound(aComp0d);

        RWCae::MeshAdaptor0d anAdaptor(*theMeshData);
        RWCae::MeshEntityId  aNb0d = anAdaptor.NbElements();
        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNb0d; anElemIter++) {
            const RWCae::MeshEntityId aNodeIndex = anAdaptor.GetElement(anElemIter)[0];
            const Graphic3d_Vec3        aNode      = theMeshData->Nodes()[aNodeIndex];
            TopoDS_Vertex               aVert;
            BRep_Builder().MakeVertex(aVert, gp_Pnt(aNode.x(), aNode.y(), aNode.z()), 0.0);
            BRep_Builder().Add(aComp0d, aVert);
        }
        BRep_Builder().Add(aComp, aComp0d);
    }

    TopoDS_Shape aRes = aComp;
    if (aComp.NbChildren() == 0) {
        Message::SendFail() << "RWMeshConv::Converter::writeBrepMesh() nothing to export to BREP";
        return false;
    } else if (aComp.NbChildren() == 1) {
        aRes = TopoDS_Iterator(aComp).Value();
    }

    if (!BinTools::Write(aRes, myOutputPath.c_str())) {
        Message::SendFail() << "RWMeshConv::Converter::writeBrepMesh() failed to write a dataset to BREP '"
                            << myOutputPath << "'";
        return false;
    }

    myConvStatus = ConversionStatus::Done;
    return true;
}

// ================================================================
// Function : checkAndCreateFolder
// ================================================================
bool MedConverter::checkAndCreateFolder(const TCollection_AsciiString& thePath)
{
    OSD_Path      aPath(thePath);
    OSD_Directory aDir(aPath);

    if (aDir.Exists()) {
        return true;
    }

    OSD_Path aParentPath = aPath;
    aParentPath.UpTrek();
    OSD_Directory aParentDir(aParentPath);
    while (!aParentDir.Exists() && aParentPath.TrekLength() > 0) {
        aParentPath.UpTrek();
        aParentDir.SetPath(aParentPath);
    }
    if (aParentPath.TrekLength() == 0) {
        return false;
    }

    OSD_Protection aParentProt = aParentDir.Protection();

    OSD_SingleProtection aSystemProt = aParentProt.System();
    aParentProt.Add(aSystemProt, OSD_RWD);
    aParentProt.SetSystem(aSystemProt);
    OSD_SingleProtection aUserProt = aParentProt.User();
    aParentProt.Add(aUserProt, OSD_RWD);
    aParentProt.SetUser(aUserProt);

    aDir.Build(aParentProt);
    return aDir.Exists();
}

} // namespace