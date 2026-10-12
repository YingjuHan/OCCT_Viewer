#include <RWVtk/VtkSeriesReader.h>

#include <RWCae/MeshTools.h>

#include <RWVtk/VtkMultiBlockDataReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>
#include <OSD_Path.hxx>
#include <RWGltf_GltfJsonParser.hxx>
#include <cmath>

namespace {
//! Find member of the object in a safe way.
static const RWVtk::VtkSeriesReader::JsonValue* findObjectMember(const RWVtk::VtkSeriesReader::JsonValue& theObject,
                                                                   const char*                                theName)
{
    if (!theObject.IsObject()) {
        return nullptr;
    }

    rapidjson::Document::ConstMemberIterator anIter = theObject.FindMember(theName);
    return anIter != theObject.MemberEnd() ? &anIter->value : nullptr;
}

}

namespace RWVtk {

// ================================================================
// function : VtkSeriesReader
// ================================================================
VtkSeriesReader::VtkSeriesReader() : VtkBaseReader()
{
    //
}

// ================================================================
// function : ~VtkSeriesReader
// ================================================================
VtkSeriesReader::~VtkSeriesReader()
{
    //
}

// =======================================================================
// function : PerformStream
// =======================================================================
bool VtkSeriesReader::PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName)
{
    if (!VtkBaseReader::PerformStream(theStream, theFileName)) {
        return false;
    }
    myAccumMeshDataTreeRoot.Nullify();
    myMergedMeshData.Nullify();

    TCollection_AsciiString aFileName;
    OSD_Path::FolderAndFileFromPath(myFileName, myPath, aFileName);

    myReader.reset(new rapidjson::Document());
    rapidjson::IStreamWrapper aFileStream(theStream);
    rapidjson::ParseResult    aRes =
        myReader->ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                              rapidjson::UTF8<>,
                              rapidjson::IStreamWrapper>(aFileStream);
    if (aRes.IsError()) {
        if (aRes.Code() == rapidjson::kParseErrorDocumentEmpty) {
            return fail("VTK series contains no JSON document");
        }
        TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
        return fail(std::string("Invalid VTK series JSON: ") + anErrDesc.ToCString()
                    + " at offset " + std::to_string(aRes.Offset()));
    }

    const JsonValue* aFiles = findObjectMember(*myReader.get(), "files");
    if (!readFiles(aFiles)) {
        return fail("Cannot read 'files' property");
    }

    return true;
}

// ================================================================
// Function : readFiles
// ================================================================
bool VtkSeriesReader::readFiles(const JsonValue* theValue)
{
    if (theValue == nullptr || !theValue->IsArray() || theValue->Empty()) {
        return fail("VTK series files must be a nonempty array");
    }

    int aTimeStep = -1;

    for (rapidjson::Value::ConstValueIterator anIter = theValue->Begin(); anIter != theValue->End(); ++anIter) {
        const JsonValue* aFileNameJson = findObjectMember(*anIter, "name");
        const JsonValue* aTimeJson = findObjectMember(*anIter, "time");
        if (aFileNameJson == nullptr || !aFileNameJson->IsString() || aFileNameJson->GetStringLength() == 0
            || aTimeJson == nullptr || !aTimeJson->IsNumber() || !std::isfinite(aTimeJson->GetDouble())) {
            return fail("VTK series entry requires a filename and finite numeric time");
        }

        TCollection_AsciiString aFileName = aFileNameJson->GetString();

        VtkMultiBlockDataReader aVtmReader;
        aVtmReader.SetToReportMessages(myToReportMessages);
        aVtmReader.SetMappingInfo(myMappingInfo);
        aVtmReader.SetTimeStep(++aTimeStep);
        aVtmReader.SetTimeStamp(aTimeJson->GetDouble());
        if (anIter != theValue->Begin()) {
            aVtmReader.SetMeshDataTree(myAccumMeshDataTreeRoot);
        }
        if (!aVtmReader.Perform(myPath + aFileName)) {
            for (const auto& anError : aVtmReader.Report().errors) { myReport.errors.push_back(anError); }
            myReport.warnings.insert(myReport.warnings.end(), aVtmReader.Report().warnings.begin(), aVtmReader.Report().warnings.end());
            return fail(std::string("Cannot read series file: ") + aFileName.ToCString());
        }
        myReport.warnings.insert(myReport.warnings.end(), aVtmReader.Report().warnings.begin(), aVtmReader.Report().warnings.end());
        if (anIter == theValue->Begin()) {
            myAccumMeshDataTreeRoot = aVtmReader.GetMeshDataTree();
        }
    }

    const auto& aMesh = GetMergedMeshData();
    if (aMesh.IsNull()) { return fail("VTK series contains no mesh"); }
    myReport.points = aMesh->NbNodes();
    myReport.cells = aMesh->NbIndexedElems();
    myReport.fields = aMesh->Properties().Size();

    return true;
}

// ================================================================
// Function : mergeMeshData
// ================================================================
void VtkSeriesReader::mergeMeshData()
{
    if (!myMergedMeshData.IsNull()) {
        return;
    }

    VtkMultiBlockDataReader::MergeMeshData(myAccumMeshDataTreeRoot, myMergedMeshData);
    if (myMergedMeshData.IsNull()) { return; }

    RWCae::PropertyRanges      aPropsLimits;
    RWCae::MeshNVecProperties& aProperties = myMergedMeshData->ChangeProperties();
    RWCae::MeshTools::fillPropertiesRange(aProperties, aPropsLimits);
    RWCae::MeshTools::restorePropertiesRange(aProperties, aPropsLimits);
}

}  
