#include <RWMeshConv/GmshLoader.h>

#include <Standard_CLocaleSentry.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_Timer.hxx>

#include <algorithm>

namespace RWMeshConv {

// ================================================================
// Function : Constructor
// ================================================================
GmshLoader::GmshLoader()
    : myToSplitSubMeshes(false),
      mySectionTypes((unsigned int)GmshSectionType::Geometry | (unsigned int)GmshSectionType::Property)
{
    // to be considered - setup measurement units to read mesh geometry correctly
}

// ================================================================
// Function : Constructor
// ================================================================
GmshLoader::GmshLoader(const Handle(RWCae::MeshData)& theMeshData) : GmshLoader()
{
    myMeshMap.Add(Graphic3d_Vec3i(0, 0, 0), new GmshSubMesh(theMeshData));
}

// =======================================================================
// function : GmshLoader
// =======================================================================
GmshLoader::~GmshLoader()
{
    //
}

// =======================================================================
// function : collectMeshStats
// =======================================================================
GmshLoader::MeshStats GmshLoader::collectMeshStats()
{
    MeshStats       aStats;
    TopLoc_Location aDummyLoc;
    for (GmshSubMeshMap::Iterator anIter(myMeshMap); anIter.More(); anIter.Next()) {
        const Handle(GmshSubMesh)&       aSubMesh  = anIter.Value();
        const Handle(RWCae::MeshData)& aMeshData = aSubMesh->Data;
        if (!aMeshData.IsNull()) {
            aStats.nbNodes += aMeshData->NbNodes();
            aStats.nbElems += aMeshData->NbElems2d();
            aStats.nbVolElems += aMeshData->NbElems3d();
            aStats.nbSubMeshes++;
        }
    }
    return aStats;
}

// ================================================================
// Function : performMesh
// ================================================================
bool GmshLoader::performMesh(const TCollection_AsciiString& theFile, const Message_ProgressRange& theProgress)
{
    OSD_Timer aLoadingTimer;
    aLoadingTimer.Start();

    Standard_CLocaleSentry aLocaleSentry;
    FileSentry             aFile(theFile);
    if (aFile.File == NULL) {
        Message::SendFail() << "[ERR_MESH_LOAD] file '" << theFile << "' is not found";
        return false;
    }

    // determine length of file
    const int64_t aFileLen = aFile.FileLen;
    if (aFileLen <= 0L) {
        Message::SendFail() << "[ERR_MESH_LOAD] file '" << theFile << "' is empty";
        return false;
    }

    const int aNbMiBTotal  = int(aFileLen / (1024 * 1024));
    int       aNbMiBPassed = 0;

    Message_ProgressScope aPSentry(theProgress, "Reading text Gmsh file", aNbMiBTotal);

    // Let GmshLoaderContext fill myMeshMap with loaded data
    GmshLoaderContext aCtx(aFile, myCoordSysConverter, myToSplitSubMeshes, mySectionTypes, myMeshMap);
    if (!aCtx.ReadLine() || !aCtx.CheckKeyWord(aFile.Line, "$MeshFormat")) {
        Message::SendFail() << "[ERR_MESH_FORMAT] missing Gmsh file header in '" << theFile << "'";
        return false;
    }

    if (!readHeader(aCtx)) {
        return false;
    }

    bool hasFailed = false;
    if (!readSections(aCtx, aPSentry, aNbMiBPassed, hasFailed)) {
        return false;
    }

    aCtx.PrintMessages();

    performPostprocessing(aCtx);

    if (hasFailed) {
        return false;
    }

    const MeshStats aMeshStats = collectMeshStats();

    for (; aNbMiBPassed < aNbMiBTotal; ++aNbMiBPassed) {
        aPSentry.Next();
    }

    aLoadingTimer.Stop();

    TCollection_AsciiString aStats = TCollection_AsciiString("[") + aMeshStats.nbNodes + " nodes] ["
                                   + aMeshStats.nbElems + " 2d elements]";
    if (aMeshStats.nbVolElems != 0) {
        aStats += TCollection_AsciiString("[") + aMeshStats.nbVolElems + " 3d elements]";
    }

    Message::SendInfo(TCollection_AsciiString("Mesh ") + theFile + "\n" + aStats + "\n[read in "
                      + aLoadingTimer.ElapsedTime() + " s]");

    return true;
}

// ================================================================
// Function : parseHeader
// ================================================================
bool GmshLoader::parseHeader(const char* theStr, int& theVerMaj, int& theVerMin, int& theFileType, int& theFloatSize)
{
    std::string       aStr(theStr);
    std::stringstream ss(aStr);

    char aDot;
    return !(ss >> theVerMaj >> aDot >> theVerMin >> theFileType >> theFloatSize).bad() && aDot == '.';
}

// ================================================================
// Function : readHeader
// ================================================================
bool GmshLoader::readHeader(GmshLoaderContext& theCtx)
{
    int aVerMaj    = 0;
    int aVerMin    = 0;
    int aFileType  = 0;
    int aFloatSize = 0;

    if (!theCtx.ReadLine() || !parseHeader(theCtx.File().Line, aVerMaj, aVerMin, aFileType, aFloatSize)
        || aFloatSize != int(sizeof(double))) {
        Message::SendFail() << "[ERR_MESH_FORMAT] unsupported Gmsh file version in '" << theCtx.File().FileName << "'";
        return false;
    } else if (aFileType != 0) {
        Message::SendFail() << "[ERR_MESH_FORMAT] unsupported Gmsh file format in '" << theCtx.File().FileName << "'";
        return false;
    }
    if (!theCtx.ReadLine() || !theCtx.CheckKeyWord(theCtx.File().Line, "$EndMeshFormat")) {
        Message::SendFail() << "[ERR_MESH_FORMAT] broken Gmsh file header in '" << theCtx.File().FileName << "'";
        return false;
    }
    return true;
}

// ================================================================
// Function : readSections
// ================================================================
bool GmshLoader::readSections(GmshLoaderContext&     theCtx,
                              Message_ProgressScope& thePSEntry,
                              int&                   theNbMiBPassed,
                              bool&                  hasFailed)
{
    OSD_Timer aTimer;
    aTimer.Start();

    for (GmshSection aSection = GmshSection::UNDEFINED; !hasFailed && theCtx.ReadLine();) {
        if (aTimer.ElapsedTime() > 1.0) {
            if (!thePSEntry.More()) {
                return false;
            }

            const int aNbMiBRead = int(theCtx.File().Position / (1024 * 1024));
            for (; theNbMiBPassed < aNbMiBRead; ++theNbMiBPassed) {
                thePSEntry.Next();
            }
            aTimer.Reset();
            aTimer.Start();
        }

        if (theCtx.File().Line[0] == '$') {
            hasFailed = !theCtx.ParseKeyWord(aSection);
            continue;
        }

        readSection(theCtx, aSection, hasFailed);
    }

    return true;
}

// ================================================================
// Function : readSection
// ================================================================
void GmshLoader::readSection(GmshLoaderContext& theCtx, GmshSection theSection, bool& hasFailed)
{
    switch (theSection) {
        case GmshSection::Nodes: {
            hasFailed = !theCtx.ReadNode();
            break;
        }
        case GmshSection::Elements: {
            hasFailed = !theCtx.ReadElement();
            break;
        }
        case GmshSection::NodeData: {
            hasFailed = !theCtx.ReadNodeData();
            break;
        }
        case GmshSection::ElementData: {
            hasFailed = !theCtx.ReadElementData();
            break;
        }
        case GmshSection::ElementNodeData: {
            hasFailed = !theCtx.ReadElementNodeData();
            break;
        }
        case GmshSection::InterpolationScheme:
        case GmshSection::PhysicalNames:
        case GmshSection::MeshFormat:
        case GmshSection::Periodic:
        case GmshSection::UNKNOWN:
        case GmshSection::UNDEFINED: {
            break;
        }
    }
}

// ================================================================
// Function : performPostprocessing
// ================================================================
void GmshLoader::performPostprocessing(const GmshLoaderContext& theCtx)
{
    for (GmshSubMeshMap::Iterator aMeshIter(theCtx.Meshes()); aMeshIter.More(); aMeshIter.Next()) {
        const Handle(GmshSubMesh)& aSubmesh = aMeshIter.Value();

        const RWCae::MeshNVecProperties& aProperties = aSubmesh->Data->Properties();
        for (const Handle(RWCae::MeshNVecProperty)& aPropIter : aProperties) {
            aPropIter->UpdateComponents();
        }

        aSubmesh->Data->UpdateCachedMinMax();
    }

    RWCae::PropertyRanges aPropsLimits;
    for (GmshSubMeshMap::Iterator aMeshIter(theCtx.Meshes()); aMeshIter.More(); aMeshIter.Next()) {
        RWCae::MeshNVecProperties& aProperties = aMeshIter.ChangeValue()->Data->ChangeProperties();
        RWCae::MeshTools::fillPropertiesRange(aProperties, aPropsLimits);
    }

    // update upper/lower limits considering all time steps
    for (GmshSubMeshMap::Iterator aMeshIter(theCtx.Meshes()); aMeshIter.More(); aMeshIter.Next()) {
        const RWCae::MeshNVecProperties& aProperties = aMeshIter.Value()->Data->Properties();
        RWCae::MeshTools::restorePropertiesRange(aProperties, aPropsLimits);
    }
}

} // namespace
