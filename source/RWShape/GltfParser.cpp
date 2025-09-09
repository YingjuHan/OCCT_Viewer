#include <RWShape/GltfParser.h>

#include <Message.hxx>

namespace RWShape
{

    static constexpr int GLTF_VERSION2 = 2;
    static constexpr int GLTF_KEY_LEN = 4; // "glTF"

    // ================================================================
    // Function : GltfParser
    // ================================================================
    GltfParser::GltfParser(TopTools_SequenceOfShape &theRootShapes) : RWGltf_GltfJsonParser(theRootShapes)
    {
    }

    // ================================================================
    // Function : ParseExtras
    // ================================================================
    void GltfParser::ParseExtras()
    {
        const RWGltf_JsonValue *anAsset = myGltfRoots[RWGltf_GltfRootElement_Asset].Root();
        if (anAsset == nullptr || !anAsset->IsObject())
        {
            return;
        }

        rapidjson::Document::ConstMemberIterator anExtrasIter = anAsset->FindMember("extras");
        if (anExtrasIter == anAsset->MemberEnd())
        {
            return;
        }

        const RWGltf_JsonValue *anExtras = &anExtrasIter->value;
        if (myMetadata == nullptr)
        {
            return;
        }

        for (ConstMemberIterator aChildIter = anExtras->MemberBegin(); aChildIter != anExtras->MemberEnd(); ++aChildIter)
        {
            if (!aChildIter->name.IsString())
            {
                continue;
            }

            const TCollection_AsciiString aKey(aChildIter->name.GetString());
            if (aChildIter->value.IsString())
            {
                myMetadata->Add(aKey, aChildIter->value.GetString());
            }
            if (aChildIter->value.IsNumber())
            {
                myMetadata->Add(aKey, aChildIter->value.GetDouble());
            }
        }
    }

    // ================================================================
    // Function : ParseGltfHeaderFromMemory
    // ================================================================
    bool GltfParser::ParseGltfHeaderFromMemory(std::istream &theStream, const TCollection_AsciiString &theFile)
    {
        bool isBinaryFile = false;
        char aGlbHeader[12] = {};
        theStream.read(aGlbHeader, sizeof(aGlbHeader));
        int64_t aBinBodyOffset = 0;
        int64_t aBinBodyLen = 0;
        int64_t aJsonBodyOffset = 0;
        uint32_t aVer = 0;
        uint32_t aLen = 0;
        // @c uint32_t JSONValue = -1;

        if (::strncmp(aGlbHeader, "glTF", GLTF_KEY_LEN) == 0)
        {
            isBinaryFile = true;
            aVer = *(const uint32_t *)(aGlbHeader + 4);
            aLen = *(const uint32_t *)(aGlbHeader + 8);
        }

        if (!isBinaryFile)
        {
            theStream.seekg(0, std::ios_base::beg);
            return true;
        }
        else if (aVer != GLTF_VERSION2)
        {
            Message::SendFail() << "File '" << theFile << "' is written using unknown version " << int(aVer);
            return false;
        }

        static constexpr int maxNbChunks2 = 2;
        for (int aChunkIter = 0; !theStream.eof() && aChunkIter < maxNbChunks2; ++aChunkIter)
        {
            char aChunkHeader2[8] = {};
            if (int64_t(theStream.tellg()) + int64_t(sizeof(aChunkHeader2)) > int64_t(aLen))
            {
                break;
            }

            theStream.read(aChunkHeader2, sizeof(aChunkHeader2));
            if (!theStream.good())
            {
                Message::SendFail() << "File '" << theFile << "' is written using unsupported format";
                return false;
            }

            const uint32_t *aChunkLen = (const uint32_t *)(aChunkHeader2 + 0);
            const uint32_t *aChunkType = (const uint32_t *)(aChunkHeader2 + 4);
            if (*aChunkType == 0x4E4F534A)
            {
                aJsonBodyOffset = int64_t(theStream.tellg());
            }
            else if (*aChunkType == 0x004E4942)
            {
                aBinBodyOffset = int64_t(theStream.tellg());
                aBinBodyLen = int64_t(*aChunkLen);
            }

            if (*aChunkLen != 0)
            {
                theStream.seekg(*aChunkLen, std::ios_base::cur);
            }
        }

        theStream.seekg((std::streamoff)aJsonBodyOffset, std::ios_base::beg);
        SetBinaryFormat(aBinBodyOffset, aBinBodyLen);
        return true;
    }

    // ================================================================
    // Function : ParseGltfFromMemory
    // ================================================================
    bool GltfParser::ParseGltfFromMemory(std::istream &theStream, const TCollection_AsciiString &theFile)
    {
        if (!ParseGltfHeaderFromMemory(theStream, theFile))
        {
            return false;
        }

        rapidjson::ParseResult aRes;
        rapidjson::IStreamWrapper aFileStream(theStream);
        if (IsBinary())
        {
            aRes = ParseStream<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseStopWhenDoneFlag,
                               rapidjson::UTF8<>,
                               rapidjson::IStreamWrapper>(aFileStream);
        }
        else
        {
            aRes = ParseStream<rapidjson::kParseFullPrecisionFlag>(aFileStream);
        }
        if (aRes.IsError())
        {
            if (aRes.Code() == rapidjson::kParseErrorDocumentEmpty)
            {
                Message::SendFail(TCollection_AsciiString("File '") + theFile + "' is empty");
                return false;
            }
            TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
            Message::SendFail(TCollection_AsciiString("File '") + theFile + "' defines invalid JSON document!\n" + anErrDesc + " [at offset " + (int)aRes.Offset() + "].");
            return false;
        }

        if (!Parse(Message_ProgressRange()))
        {
            return false;
        }

        ParseExtras();
        return true;
    }

}