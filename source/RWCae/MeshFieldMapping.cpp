#include <RWCae/MeshFieldMapping.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>
#include <RWGltf_GltfJsonParser.hxx>

#include <set>

namespace {
//! Initial mapping version
constexpr int MAPPING_VERSION_MAJOR_0 = 0;
constexpr int MAPPING_VERSION_MINOR_0 = 0;

//! Changes:
//! - version and analysis global attributes added
constexpr int MAPPING_VERSION_MAJOR_1 = 1;

//! Changes:
//! - id and calculationPoint attributes added to each mapping entry
constexpr int MAPPING_VERSION_MINOR_1 = 1;

//! Changes:
//! - optional sourceUnits attribute added to each mapping entry
constexpr int MAPPING_VERSION_MINOR_2 = 2;

//! Changes:
//! - optional resultTypes global array added
constexpr int MAPPING_VERSION_MINOR_3 = 3;

//! Changes:
//! - optional "options" array added to resultType object
// @c constexpr int MAPPING_VERSION_MINOR_4 = 4;

constexpr int MAPPING_VERSION_INVALID = -1;

// ================================================================
// Function : readStrArray
// ================================================================
bool readStrArray(const rapidjson::Value& theSrc, std::vector<std::string>& theDst)
{
    Standard_ASSERT_RETURN(theSrc.IsArray(), "a JSON array expected but non-array value received", false);
    for (rapidjson::Value::ConstValueIterator itr = theSrc.Begin(); itr != theSrc.End(); ++itr) {
        theDst.push_back(itr->GetString());
    }
    return true;
};

} // namespace

namespace RWCae {

// ================================================================
// Function : MeshFieldMappingReader
// ================================================================
MeshFieldMappingReader::MeshFieldMappingReader()
{
    //
}

// ================================================================
// Function : Perform
// ================================================================
bool MeshFieldMappingReader::Perform(const TCollection_AsciiString& theFileName)
{
    const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
    std::shared_ptr<std::istream> aFile       = aFileSystem->OpenIStream(theFileName, std::ios::in | std::ios::binary);
    if (aFile.get() == nullptr) {
        Message::SendFail() << "Error: Input file '" << theFileName << "' is not accessible";
        return false;
    }
    return Perform(*aFile, theFileName);
}

// ================================================================
// Function : Perform
// ================================================================
bool MeshFieldMappingReader::Perform(std::istream& theStream, const TCollection_AsciiString& theFileName)
{
    if (!theStream.good()) {
        Message::SendFail() << "Error: Stream '" << theFileName << "' is missing or invalid";
        return false;
    }

    myReader.reset(new rapidjson::Document());
    rapidjson::IStreamWrapper aFileStream(theStream);
    rapidjson::ParseResult    aRes =
        myReader->ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                              rapidjson::UTF8<>,
                              rapidjson::IStreamWrapper>(aFileStream);
    if (aRes.IsError()) {
        if (aRes.Code() == rapidjson::kParseErrorDocumentEmpty) {
            Message::SendFail() << "Error: Stream '" << theFileName << "' is not a JSON document";
            return false;
        }
        TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
        Message::SendFail() << "Error: Stream '" << theFileName << "' defines invalid JSON document!\n"
                            << anErrDesc << " [at offset " << (int)aRes.Offset() << "].";
        return false;
    }

    // Read the major and minor version components.
    int aVersionMaj = 0;
    int aVersionMin = 0;
    if (!readMappingVersion(aVersionMaj, aVersionMin)) {
        Message::SendFail() << "Error: Cannot detect JSON mapping file version";
        return false;
    }

    Message::SendInfo() << "Reading mapping file version '" << aVersionMaj << "." << aVersionMin << "'";

    if (aVersionMaj == MAPPING_VERSION_MAJOR_0) {
        return readMappingResults0(*myReader);
    } else if (aVersionMaj == MAPPING_VERSION_MAJOR_1 && aVersionMin == MAPPING_VERSION_MINOR_0) {
        return readMappingResults10(*myReader);
    } else if (aVersionMaj == MAPPING_VERSION_MAJOR_1 && aVersionMin == MAPPING_VERSION_MINOR_1) {
        return readMappingResults11(*myReader);
    } else if (aVersionMaj == MAPPING_VERSION_MAJOR_1 && aVersionMin == MAPPING_VERSION_MINOR_2) {
        return readMappingResults12(*myReader);
    } else if (aVersionMaj == MAPPING_VERSION_MAJOR_1 && aVersionMin == MAPPING_VERSION_MINOR_3) {
        return readMappingResults13(*myReader);
    } else if (aVersionMaj == MAPPING_VERSION_MAJOR_1) {
        return readMappingResults14(*myReader);
    }

    Message::SendFail() << "Error: Unsupported JSON mapping file version";
    return false;
}

// ================================================================
// Function : readMappingVersion
// ================================================================
bool MeshFieldMappingReader::readMappingVersion(int& theVersionMajor, int& theVersionMinor)
{
    if (!myReader->IsObject()) {
        theVersionMajor = MAPPING_VERSION_MAJOR_0;
        theVersionMinor = MAPPING_VERSION_MINOR_0;
        return true;
    }

    if (!myReader->HasMember("version")) {
        return false;
    }

    auto getComp = [](std::istringstream& theInput, std::string& theToken) {
        std::getline(theInput, theToken, '.');
        if (theInput.fail()) {
            return MAPPING_VERSION_INVALID;
        }
        try {
            return std::stoi(theToken);
        } catch (const std::exception&) {
            return MAPPING_VERSION_INVALID;
        }
    };

    std::istringstream aVerStr((*myReader)["version"].GetString());
    std::string        aVerComp;
    theVersionMajor = getComp(aVerStr, aVerComp);
    theVersionMinor = getComp(aVerStr, aVerComp);

    return theVersionMajor >= MAPPING_VERSION_INVALID && theVersionMinor >= MAPPING_VERSION_INVALID;
}

// ================================================================
// Function : readMappingResults0
// ================================================================
bool MeshFieldMappingReader::readMappingResults0(const rapidjson::Document::ValueType& theValue)
{
    myMappingInfo.resize(theValue.Capacity());

    int  i    = 0;
    bool isOk = true;
    for (rapidjson::Value::ConstValueIterator itr = theValue.Begin(); isOk && itr != theValue.End(); ++itr, ++i) {
        MeshFieldMappingInfo& aFMI = myMappingInfo[i];

        aFMI.FieldDescription = itr->GetObject()["description"].GetString();
        aFMI.ValueType        = MeshMappingFieldTypeConvertor::GetValue(itr->GetObject()["valueType"].GetString());
        aFMI.FieldMask        = itr->GetObject()["fieldMask"].GetString();

        aFMI.NormalizeFlag = itr->GetObject().HasMember("normalize") && itr->GetObject()["normalize"].GetBool();

        isOk = isOk && ::readStrArray(itr->GetObject()["componentMask"], aFMI.ComponentMasks);
        isOk = isOk
            && (!itr->GetObject().HasMember("componentUnit")
                || (::readStrArray(itr->GetObject()["componentUnit"], aFMI.ComponentUnits)
                    && aFMI.ComponentMasks.size() == aFMI.ComponentUnits.size()));
    }

    if (!isOk) {
        myMappingInfo.clear();
    }

    return isOk;
}

// ================================================================
// Function : readMappingResults10
// ================================================================
bool MeshFieldMappingReader::readMappingResults10(const rapidjson::Document::ValueType& theValue)
{
    if (!theValue.HasMember("analysis") || !theValue["analysis"].IsString()) {
        Message::SendFail() << "Error: Analysis type string not found in JSON mapping file";
        return false;
    }
    myAnalysisType = theValue["analysis"].GetString();

    if (!theValue.HasMember("results") || !theValue["results"].IsArray()) {
        Message::SendFail() << "Error: Results array not found in JSON mapping file";
        return false;
    }

    return readMappingResults0(theValue["results"]);
}

// ================================================================
// Function : readMappingResults11
// ================================================================
bool MeshFieldMappingReader::readMappingResults11(const rapidjson::Document::ValueType& theValue)
{
    if (!readMappingResults10(theValue)) {
        return false;
    }

    const rapidjson::Document::ValueType& anArray = theValue["results"];

    int                   i = 0;
    std::set<std::string> aUniqueIds;
    for (rapidjson::Value::ConstValueIterator itr = anArray.Begin(); itr != anArray.End(); ++itr, ++i) {
        MeshFieldMappingInfo& aFMI = myMappingInfo[i];

        aFMI.FieldId = itr->GetObject()["id"].GetString();
        // We require that a non-empty field ID must be unique within a mapping file format v1.1
        // in order to manipulate solver results in a non-ambiguous way. Having duplicated IDs
        // mean an ill-formed v1.1 mapping file so we abort reading with an error in such a case.
        if (!aFMI.FieldId.empty()) {
            if (aUniqueIds.find(aFMI.FieldId) != aUniqueIds.cend()) {
                Message::SendFail() << "Error: Mapping file contains duplicated id '" << aFMI.FieldId.c_str() << "'";
                return false;
            } else {
                aUniqueIds.insert(aFMI.FieldId);
            }
        }
        aFMI.CalcPoint = MeshMappingCalcPointConvertor::GetValue(itr->GetObject()["calculationPoint"].GetString());
    }

    return true;
}

// ================================================================
// Function : readMappingResults12
// ================================================================
bool MeshFieldMappingReader::readMappingResults12(const rapidjson::Document::ValueType& theValue)
{
    if (!readMappingResults11(theValue)) {
        return false;
    }

    const rapidjson::Document::ValueType& anArray = theValue["results"];

    int  i    = 0;
    bool isOk = true;
    for (rapidjson::Value::ConstValueIterator itr = anArray.Begin(); itr != anArray.End(); ++itr, ++i) {
        MeshFieldMappingInfo& aFMI = myMappingInfo[i];
        isOk                       = isOk
            && (!itr->GetObject().HasMember("sourceUnits")
                || (::readStrArray(itr->GetObject()["sourceUnits"], aFMI.SourceUnits)
                    && aFMI.ComponentMasks.size() == aFMI.SourceUnits.size()));
    }

    if (!isOk) {
        myMappingInfo.clear();
    }

    return isOk;
}

// ================================================================
// Function : readMappingResults13
// ================================================================
bool MeshFieldMappingReader::readMappingResults13(const rapidjson::Document::ValueType& theValue)
{
    if (!readMappingResults12(theValue)) {
        return false;
    }

    // resultTypes array is optional
    if (!theValue.HasMember("resultTypes")) {
        return true;
    }

    if (!theValue["resultTypes"].IsArray()) {
        Message::SendFail() << "Error: Result types is not an array";
        return false;
    }

    const rapidjson::Document::ValueType& anArray = theValue["resultTypes"];

    myResultTypes.resize(anArray.Size());

    bool                  isOk = true;
    std::set<std::string> aUniqueIds;
    for (rapidjson::SizeType i = 0; i < anArray.Size(); ++i) {
        const rapidjson::Document::ValueType& aJsonVal = anArray[i];
        if (!aJsonVal.IsObject()) {
            Message::SendFail() << "Error: 'resultTypes' should be an array of objects";
            isOk = false;
            break;
        } else if (!aJsonVal.HasMember("resultType") || !aJsonVal.HasMember("description")
                   || !aJsonVal.HasMember("format")) {
            Message::SendFail() << "Error: Result type attributes are missing or incorrect";
            isOk = false;
            break;
        }

        MeshResultType& aResType = myResultTypes[i];
        aResType.TypeId          = aJsonVal["resultType"].GetString();
        aResType.Description     = aJsonVal["description"].GetString();
        aResType.Format          = aJsonVal["format"].GetString();
        if (aUniqueIds.find(aResType.TypeId) != aUniqueIds.cend()) {
            Message::SendFail() << "Error: Mapping file contains duplicated result type id '" << aResType.TypeId.c_str()
                                << "'";
            isOk = false;
        } else {
            aUniqueIds.insert(aResType.TypeId);
        }
    }

    if (!isOk) {
        myResultTypes.clear();
    }

    return isOk;
}

// ================================================================
// Function : readMappingResults14
// ================================================================
bool MeshFieldMappingReader::readMappingResults14(const rapidjson::Document::ValueType& theValue)
{
    if (!readMappingResults13(theValue)) {
        return false;
    }

    // resultTypes array is empty --> no options to read
    if (myResultTypes.empty()) {
        return true;
    }

    const rapidjson::Document::ValueType& anArray = theValue["resultTypes"];
    if (!anArray.IsArray()) {
        Message::SendFail() << "Error: 'resultTypes' is not an array";
        return false;
    } else if (size_t(anArray.Size()) != myResultTypes.size()) {
        Message::SendFail() << "Error: unexpected length " << anArray.Size() << " of 'resultTypes' array (expected "
                            << myResultTypes.size() << ")";
        return false;
    }

    bool isOk = true;
    for (rapidjson::SizeType i = 0; i < anArray.Size(); ++i) {
        const rapidjson::Document::ValueType& aJsonVal = anArray[i];
        if (!aJsonVal.IsObject()) {
            Message::SendFail() << "Error: 'resultTypes' should be an array of objects";
            isOk = false;
            break;
        } else if (!aJsonVal.HasMember("options")) {
            continue;
        }

        const rapidjson::Document::ValueType& anOptions = aJsonVal["options"];
        if (!anOptions.IsObject()) {
            Message::SendFail() << "Error: resultTypes.options is not an object";
            isOk = false;
            break;
        }

        // Collect all "key" : "value" pairs from "options" object in a map.
        // The meaning of such options is application-defined.
        MeshResultType& aResType = myResultTypes[i];
        for (rapidjson::Document::ConstMemberIterator anOptionsIt = anOptions.MemberBegin();
             anOptionsIt != anOptions.MemberEnd();
             ++anOptionsIt) {
            isOk = isOk && anOptionsIt->value.IsString();
            if (!isOk) {
                Message::SendFail() << "Error: Result type option '" << anOptionsIt->name.GetString()
                                    << "' is not a string";
                break;
            }

            isOk = isOk && (aResType.Options.find(anOptionsIt->name.GetString()) == aResType.Options.end());
            if (!isOk) {
                Message::SendFail() << "Error: Duplicated Result type option '" << anOptionsIt->name.GetString() << "'";
                break;
            }

            aResType.Options.insert(
                std::pair<std::string, std::string>(anOptionsIt->name.GetString(), anOptionsIt->value.GetString()));
        }

        if (!isOk) {
            aResType.Options.clear();
            break;
        }
    }

    return isOk;
}

// ================================================================
// Function : MeshFieldMappingWriter
// ================================================================
MeshFieldMappingWriter::MeshFieldMappingWriter(const std::vector<MeshFieldMappingInfo>& theMappingInfo)
    : myMappingInfo(theMappingInfo)
{
    //
}

// ================================================================
// Function : Perform
// ================================================================
bool MeshFieldMappingWriter::Perform(const TCollection_AsciiString& theFileName)
{
    const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
    std::shared_ptr<std::ostream> aFile       = aFileSystem->OpenOStream(theFileName, std::ios::out | std::ios::binary);
    if (!aFile) {
        Message::SendFail() << "Error: File '" << theFileName << "' cannot be written";
        return false;
    }
    return Perform(*aFile, theFileName);
}

// ================================================================
// Function : Perform
// ================================================================
bool MeshFieldMappingWriter::Perform(std::ostream& theStream, const TCollection_AsciiString& theFileName)
{
    if (!theStream.good()) {
        Message::SendFail() << "Error: File '" << theFileName << "' cannot be written";
        return false;
    }
    rapidjson::OStreamWrapper                    aFileStream(theStream);
    rapidjson::Writer<rapidjson::OStreamWrapper> aWriter(aFileStream);

    aWriter.StartArray();

    for (size_t i = 0; i < myMappingInfo.size(); i++) {
        const MeshFieldMappingInfo& aFMI = myMappingInfo[i];
        aWriter.StartObject();

        aWriter.Key("description");
        aWriter.String(aFMI.FieldDescription.c_str());

        aWriter.Key("valueType");
        aWriter.String(MeshMappingFieldTypeConvertor::GetString(aFMI.ValueType));

        aWriter.Key("fieldMask");
        aWriter.String(aFMI.FieldMask.c_str());

        aWriter.Key("componentMask");
        aWriter.StartArray();
        for (const std::string& aCompMask : aFMI.ComponentMasks) {
            aWriter.String(aCompMask.c_str());
        }
        aWriter.EndArray();

        if (!aFMI.ComponentUnits.empty()) {
            aWriter.Key("componentUnit");
            aWriter.StartArray();
            for (const std::string& aCompUnit : aFMI.ComponentUnits) {
                aWriter.String(aCompUnit.c_str());
            }
            aWriter.EndArray();
        }

        if (aFMI.NormalizeFlag) {
            aWriter.Key("normalize");
            aWriter.Bool(true);
        }

        aWriter.EndObject();
    }

    aWriter.EndArray();

    theStream.flush();
    if (!theStream.good()) {
        Message::SendFail() << "Error: File '" << theFileName << "' cannot be written";
        return false;
    }

    return true;
}

}