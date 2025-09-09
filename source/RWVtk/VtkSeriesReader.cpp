#include <RWVtk/VtkSeriesReader.h>

#include <RWCae/MeshTools.h>

#include <RWVtk/VtkMultiBlockDataReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>
#include <OSD_Path.hxx>
#include <RWGltf_GltfJsonParser.hxx>

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
            Message::SendFail() << "Error: Stream '" << myFileName << "' does not contain a JSON document";
            return false;
        }
        TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
        Message::SendFail() << "Error: Stream '" << myFileName << "' defines invalid JSON document!\n"
                            << anErrDesc << " [at offset " << (int)aRes.Offset() << "].";
        return false;
    }

    const JsonValue* aFiles = findObjectMember(*myReader.get(), "files");
    if (!readFiles(aFiles)) {
        Message::SendFail() << "Error: Cannot read 'files' property";
        return false;
    }

    return true;
}

// ================================================================
// Function : readFiles
// ================================================================
bool VtkSeriesReader::readFiles(const JsonValue* theValue)
{
    if (theValue == nullptr) {
        return false;
    }

    int aTimeStep = -1;

    for (rapidjson::Value::ConstValueIterator anIter = theValue->Begin(); anIter != theValue->End(); ++anIter) {
        const JsonValue* aFileNameJson = findObjectMember(*anIter, "name");
        if (aFileNameJson == nullptr) {
            return false;
        }

        TCollection_AsciiString aFileName = aFileNameJson->GetString();

        VtkMultiBlockDataReader aVtmReader;
        aVtmReader.SetMappingInfo(myMappingInfo);
        aVtmReader.SetTimeStep(++aTimeStep);
        if (anIter != theValue->Begin()) {
            aVtmReader.SetMeshDataTree(myAccumMeshDataTreeRoot);
        }
        if (!aVtmReader.Perform(myPath + aFileName)) {
            Message::SendFail() << "Error: Cannot read '" << aFileName << "' file";
            return false;
        }
        if (anIter == theValue->Begin()) {
            myAccumMeshDataTreeRoot = aVtmReader.GetMeshDataTree();
        }
    }

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

    RWCae::PropertyRanges      aPropsLimits;
    RWCae::MeshNVecProperties& aProperties = myMergedMeshData->ChangeProperties();
    RWCae::MeshTools::fillPropertiesRange(aProperties, aPropsLimits);
    RWCae::MeshTools::restorePropertiesRange(aProperties, aPropsLimits);
}

}  
