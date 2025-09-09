
#include <RWCae/MeshBinReader.h>

#include <RWBase/TaskManager.h>

#include <RWCae/MeshDracoCompressionHelper.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>
#include <OSD_Timer.hxx>
#include <RWGltf_GltfJsonParser.hxx>

namespace
{
    static constexpr int ELEM_DEC_FIRST_OFFSETS = 0;
    static constexpr int ELEM_DEC_OFFSETS = 1;
    static constexpr int ELEM_DEC_NODES = 2;
    static constexpr int ELEM_DEC_FACES = 3;
    static constexpr int ELEM_DEC_GI = 4;

    //! Struct that contains decompressed info about binary data part.
    template <int N>
    struct MeshDecodedPart
    {
        MeshDecodedPart() : decoders(N) {}

        std::vector<RWCae::MeshAttributeDecoder> decoders;
        size_t decoderDataOffset = 0;
        bool isOk = false;
    };

    //! Decode binary data parts.
    template <int N>
    bool ProcessDecodedParts(std::vector<MeshDecodedPart<N>> &theParts,
                             bool theToSupportParallel,
                             bool theIsParallel,
                             bool theToReadDracoVer9,
                             const std::shared_ptr<std::vector<char>> &theBuffer,
                             size_t &theNbValues)
    {
        if (theToSupportParallel)
        {
            uint32_t aNbParts = *reinterpret_cast<uint32_t *>(theBuffer->data());
            theParts.resize(aNbParts);
            size_t aHeaderOffset = sizeof(aNbParts) + sizeof(uint64_t) * aNbParts;
            size_t anOffset = sizeof(aNbParts);

            theParts[0].decoderDataOffset = aHeaderOffset;
            for (size_t i = 1; i < aNbParts; i++)
            {
                theParts[i].decoderDataOffset = *reinterpret_cast<uint64_t *>(&(*theBuffer)[anOffset]) + theParts[i - 1].decoderDataOffset;
                anOffset += sizeof(uint64_t);
            }
        }
        else
        {
            theParts.resize(1);
        }

        auto processPart = [&theBuffer, &theParts, theToReadDracoVer9](int, int theObjIndex)
        {
            size_t anOffset = theParts[theObjIndex].decoderDataOffset;

            bool aRes = !theParts[theObjIndex].decoders.empty();
            for (RWCae::MeshAttributeDecoder &aDecoder : theParts[theObjIndex].decoders)
            {
                aDecoder.SetUseOriginRangeParams(theToReadDracoVer9);
                aRes = aDecoder.Decode(theBuffer->data(), theBuffer->size(), anOffset) && aRes;
            }
            theParts[theObjIndex].isOk = aRes;
        };

        RWBase::TaskManager::Instance()->RunInParallel(0, (int)theParts.size(), processPart, !theIsParallel);

        theNbValues = 0;
        for (const MeshDecodedPart<N> &aPart : theParts)
        {
            if (!aPart.isOk)
            {
                return false;
            }
            theNbValues += aPart.decoders[0].NbValues();
        }

        return true;
    }

    //! Append fixed mesh element to elements container.
    void AppendMeshElem(RWCae::MeshElements &theElems,
                        const std::vector<int32_t> &theNodes,
                        RWCae::MeshElemEntityId theNbNodes,
                        int theGI)
    {
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theNbNodes; ++aNodeIter)
        {
            theElems.Nodes().SetValue(theElems.Nodes().Size(), (RWCae::MeshEntityId)theNodes[aNodeIter]);
        }
        theElems.GlobalIndices().SetValue(theElems.GlobalIndices().Size(), theGI);
    }
}

namespace RWCae
{

    //! Find member of the object in a safe way.
    static const MeshBinReader::JsonValue *findObjectMember(const MeshBinReader::JsonValue &theObject, const char *theName)
    {
        if (!theObject.IsObject())
        {
            return nullptr;
        }

        rapidjson::Document::ConstMemberIterator anIter = theObject.FindMember(theName);
        return anIter != theObject.MemberEnd() ? &anIter->value : nullptr;
    }

    //! Find member of the object in a safe way.
    static const MeshBinReader::JsonValue *findObjectMember(const MeshBinReader::JsonValue &theObject,
                                                            MeshDataJsonElement theElement)
    {
        return findObjectMember(theObject, MeshDataJsonElementConvertor::GetString(theElement));
    }

    // ================================================================
    // Function : MeshBinReader
    // ================================================================
    MeshBinReader::MeshBinReader() : MeshBinIOBase(new MeshData())
    {
    }

    // ================================================================
    // Function : MeshBinReader
    // ================================================================
    MeshBinReader::MeshBinReader(const Handle(MeshData) & theMeshData) : MeshBinIOBase(theMeshData)
    {
    }

    // ================================================================
    // Function : ~MeshBinReader
    // ================================================================
    MeshBinReader::~MeshBinReader()
    {
    }

    // ================================================================
    // Function : Perform
    // ================================================================
    bool MeshBinReader::Perform(const TCollection_AsciiString &theFileName)
    {
        const Handle(OSD_FileSystem) &aFileSystem = OSD_FileSystem::DefaultFileSystem();
        std::shared_ptr<std::istream> aFile = aFileSystem->OpenIStream(theFileName, std::ios::in | std::ios::binary);
        if (aFile.get() == nullptr)
        {
            Message::SendFail() << "MeshBinReader: Input file '" << theFileName << "' is not accessible";
            return false;
        }
        return Perform(*aFile, theFileName);
    }

    //! Read and validate magic keyword in MDB file header.
    static bool CheckMdbMagic(std::istream &theStream)
    {
        const int aHeaderLength = static_cast<int>(std::strlen(MeshImportExport::MESH_DATA_HEADER));

        NCollection_Array1<char> aBuf(0, aHeaderLength);
        theStream.read(&aBuf.ChangeFirst(), aHeaderLength);
        aBuf[aHeaderLength] = '\0';

        const TCollection_AsciiString aStoredHeader(&aBuf.First());
        if (!aStoredHeader.IsEqual(MeshImportExport::MESH_DATA_HEADER))
        {
            Message::SendFail() << "MeshBinReader: File header '" << aStoredHeader << "' is incorrect";
            return false;
        }
        return true;
    }

    // ================================================================
    // Function : Perform
    // ================================================================
    bool MeshBinReader::Perform(std::istream &theStream, const TCollection_AsciiString &theFileName)
    {
        myFileName = theFileName;
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinReader: Stream '" << myFileName << "' is missing or invalid";
            return false;
        }

        OSD_Timer aLoadingTimer;
        aLoadingTimer.Start();
        if (!CheckMdbMagic(theStream))
        {
            return false;
        }

        char aHeaderVerBuf[4] = {};
        theStream.read(aHeaderVerBuf, sizeof(uint32_t));
        myVersionMajor = *reinterpret_cast<uint16_t *>(aHeaderVerBuf);
        myVersionMinor = *reinterpret_cast<uint16_t *>(aHeaderVerBuf + sizeof(uint16_t));
        if (!checkVersion())
        {
            Message::SendFail() << "MeshBinReader: Unsupported MDB file version '" << myVersionMajor << "."
                                << myVersionMinor << "'; current version is '" << MeshImportExport::MESH_DATA_VERSION_MAJOR
                                << "." << MeshImportExport::MESH_DATA_VERSION_MINOR << "'";
            return false;
        }

        char aHeaderLenBuf[8] = {};
        theStream.read(aHeaderLenBuf, offsetSize());
        const uint64_t aTotalLength = toRead32BitsOffsets() ? *reinterpret_cast<uint32_t *>(aHeaderLenBuf)
                                                            : *reinterpret_cast<uint64_t *>(aHeaderLenBuf);
        (void)aTotalLength;

        readDataOffsets(theStream);

        if (!readJson(theStream))
        {
            return false;
        }

        {
            Message_Messenger::StreamBuffer aStats = Message::SendInfo();
            aStats << "Mesh " << myFileName + "\n";
            myMeshData->PrintStats(aStats);
            aStats << "\n[read in " << aLoadingTimer.ElapsedTime() << " s]";
        }
        return true;
    }

    // ================================================================
    // Function : readDataOffsets
    // ================================================================
    void MeshBinReader::readDataOffsets(std::istream &theStream)
    {
        int aNbChunks = 2;
        for (int aChunkIter = 0; !theStream.eof() && aChunkIter < aNbChunks; ++aChunkIter)
        {
            char aChunkBuf[12] = {};
            theStream.read(aChunkBuf, sizeof(uint32_t) + offsetSize());
            const uint64_t aChunkLength = toRead32BitsOffsets() ? *reinterpret_cast<uint32_t *>(aChunkBuf)
                                                                : *reinterpret_cast<uint64_t *>(aChunkBuf);
            const uint32_t aChunkType = *reinterpret_cast<uint32_t *>(aChunkBuf + offsetSize());
            if (aChunkType == 0x4E4F534A)
            { // "JSON"
                myFileOffsets.jsonLength = aChunkLength;
                myFileOffsets.jsonOffset = (uint64_t)theStream.tellg();
            }
            else if (aChunkType == 0x004E4942)
            { // "BIN"
                myFileOffsets.binDataLength = aChunkLength;
                myFileOffsets.binDataOffset = (uint64_t)theStream.tellg();
            }
            theStream.seekg(aChunkLength, std::ios::cur);
        }
    }

    // ================================================================
    // Function : readJson
    // ================================================================
    bool MeshBinReader::readJson(std::istream &theStream)
    {
        theStream.seekg(myFileOffsets.jsonOffset, std::ios::beg);

        myReader.reset(new rapidjson::Document());
        rapidjson::IStreamWrapper aFileStream(theStream);
        rapidjson::ParseResult aRes = myReader->ParseStream<
            rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag,
            rapidjson::UTF8<>,
            rapidjson::IStreamWrapper>(aFileStream);
        if (aRes.IsError())
        {
            if (aRes.Code() == rapidjson::kParseErrorDocumentEmpty)
            {
                Message::SendFail() << "MeshBinReader: Stream '" << myFileName << "' does not contain a JSON part";
                return false;
            }
            TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
            Message::SendFail() << "MeshBinReader: Stream '" << myFileName << "' defines invalid JSON document!\n"
                                << anErrDesc << " [at offset " << (int)aRes.Offset() << "].";
            return false;
        }

        const JsonValue *anAsset = findObjectMember(*myReader.get(), MeshDataJsonElement::ASSET);
        if (!readAsset(anAsset))
        {
            Message::SendFail() << "MeshBinReader: Cannot read '"
                                << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::ASSET) << "' property";
            return false;
        }

        const JsonValue *aBuffers = findObjectMember(*myReader.get(), MeshDataJsonElement::BUFFERS);
        if (!readBuffers(aBuffers))
        {
            Message::SendFail() << "MeshBinReader: Cannot read '"
                                << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BUFFERS) << "' property";
            return false;
        }

        const JsonValue *aMesh = findObjectMember(*myReader.get(), MeshDataJsonElement::MESH);
        if (!readMesh(theStream, aMesh))
        {
            Message::SendFail() << "MeshBinReader: Cannot read '"
                                << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::MESH) << "' property";
            return false;
        }

        return true;
    }

    // ================================================================
    // Function : readAsset
    // ================================================================
    bool MeshBinReader::readAsset(const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return false;
        }

        const JsonValue *aName = findObjectMember(*theValue, MeshDataJsonElement::GENERATOR);
        if (aName != nullptr)
        {
            myGenerator = aName->GetString();
        }
        const JsonValue *aSubver = findObjectMember(*theValue, MeshDataJsonElement::SUBVERSION);
        if (aSubver != nullptr && aSubver->IsInt())
        {
            myFormatSubversion = aSubver->GetInt();
        }

        const JsonValue *aCompression = findObjectMember(*theValue, MeshDataJsonElement::COMPRESSION);
        if (!readCompressionInfo(aCompression))
        {
            Message::SendFail() << "MeshBinReader: Cannot read '"
                                << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::COMPRESSION)
                                << "' property";
            return false;
        }

        return true;
    }

    // ================================================================
    // Function : readCompressionInfo
    // ================================================================
    bool MeshBinReader::readCompressionInfo(const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            // Compression info is an optional field
            return true;
        }

        const JsonValue *aType = findObjectMember(*theValue, MeshDataJsonElement::COMPRESSION_CODEC);
        if (aType == nullptr || !aType->IsString())
        {
            return false;
        }

        myCompressionType = MeshCompressionTypeConvertor::GetType(aType->GetString());
        if (myCompressionType == MeshCompressionType::Draco)
        {
            if (myVersionMajor < MeshImportExport::MESH_DATA_VERSION_MAJOR_6)
            {
                Message::SendFail() << "MeshBinReader: Unsupported compressed MDB file version '" << myVersionMajor << "."
                                    << myVersionMinor << "'; current version is '"
                                    << MeshImportExport::MESH_DATA_VERSION_MAJOR << "."
                                    << MeshImportExport::MESH_DATA_VERSION_MINOR << "'";
                return false;
            }
            const JsonValue *anInfo = findObjectMember(*theValue, MeshDataJsonElement::COMPRESSION_DRACO);
            if (anInfo == nullptr || !anInfo->IsObject())
            {
                return false;
            }

            const JsonValue *aNodesQuantBits = findObjectMember(*anInfo, MeshDracoCompressionInfo::NodesQuantBitsStr());
            if (aNodesQuantBits == nullptr || !aNodesQuantBits->IsInt())
            {
                return false;
            }

            const JsonValue *aPropsQuantBits = findObjectMember(*anInfo, MeshDracoCompressionInfo::PropsQuantBitsStr());
            if (aPropsQuantBits == nullptr || !aPropsQuantBits->IsInt())
            {
                return false;
            }

            myDracoCompressionInfo = MeshDracoCompressionInfo(aNodesQuantBits->GetInt(), aPropsQuantBits->GetInt());
        }

        return true;
    }

    // ================================================================
    // Function : readBuffers
    // ================================================================
    bool MeshBinReader::readBuffers(const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return false;
        }

        static constexpr int aBase = 10;

        for (rapidjson::Value::ConstValueIterator anIter = theValue->Begin(); anIter != theValue->End(); ++anIter)
        {
            const JsonValue *aLength = findObjectMember(*anIter, MeshDataJsonElement::BUFFER_LENGTH);
            const JsonValue *anOffset = findObjectMember(*anIter, MeshDataJsonElement::BUFFER_OFFSET);
            if (aLength == nullptr || anOffset == nullptr)
            {
                return false;
            }
            if (myVersionMajor <= MeshImportExport::MESH_DATA_VERSION_MAJOR_3)
            {
                myBufferData.Append(BufferData(aLength->GetUint64(), anOffset->GetUint64()));
            }
            else
            {
                myBufferData.Append(BufferData(std::strtoull(aLength->GetString(), nullptr, aBase),
                                               std::strtoull(anOffset->GetString(), nullptr, aBase)));
            }
        }
        return true;
    }

    // ================================================================
    // Function : readMesh
    // ================================================================
    bool MeshBinReader::readMesh(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return false;
        }

        const JsonValue *aName = findObjectMember(*theValue, MeshDataJsonElement::MESH_NAME);
        if (aName != nullptr)
        {
            myMeshData->SetName(aName->GetString());
        }

        if (!CheckOption(MeshDataImportExportOptions::SKIP_GEOMETRY))
        {
            const JsonValue *aGeometry = findObjectMember(*theValue, MeshDataJsonElement::GEOMETRY);
            if (!readGeometry(theStream, aGeometry))
            {
                Message::SendFail() << "MeshBinReader: Cannot read '"
                                    << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::GEOMETRY)
                                    << "' property";
                return false;
            }
        }

        if (!CheckOption(MeshDataImportExportOptions::SKIP_PROPERTIES))
        {
            const JsonValue *aProperties = findObjectMember(*theValue, MeshDataJsonElement::PROPERTIES);
            if (!readProperties(theStream, aProperties))
            {
                Message::SendFail() << "MeshBinReader: Cannot read '"
                                    << MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTIES)
                                    << "' property";
                return false;
            }
        }

        return true;
    }

    // ================================================================
    // Function : readGeometry
    // ================================================================
    bool MeshBinReader::readGeometry(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return true; // it is optional field
        }

        if (!CheckOption(MeshDataImportExportOptions::SKIP_GEOMETRY_BINARY_DATA))
        {
            const JsonValue *aNodes = findObjectMember(*theValue, MeshDataJsonElement::NODES);
            if (!readNodes(theStream, aNodes))
            {
                return false;
            }

            const JsonValue *anElements = findObjectMember(*theValue, MeshDataJsonElement::ELEMENTS);
            if (anElements == nullptr)
            {
                return false;
            }

            for (rapidjson::Value::ConstValueIterator anIter = anElements->Begin(); anIter != anElements->End(); ++anIter)
            {
                if (!readElement(theStream, anIter))
                {
                    return false;
                }
            }

            fillIndexedElements();
        }

        // Read precomputed mesh AABB AFTER reading the geometry else the cached AABB is reset
        if (!readBounds(theStream, theValue))
        {
            return false;
        }

        // Read precomputed mesh AABB AFTER reading the geometry else the cached AABB is reset
        if (!readBlocks(theStream, theValue))
        {
            return false;
        }

        return true;
    }

    // ================================================================
    // Function : readBounds
    // ================================================================
    bool MeshBinReader::readBounds(std::istream &, const JsonValue *theValue)
    {
        // Unset a cached AABB
        myMeshData->SetCachedMinMax(Bnd_Box());

        const JsonValue *aBoundsMinField = findObjectMember(*theValue, MeshDataJsonElement::BOUNDS_MIN);
        const JsonValue *aBoundsMaxField = findObjectMember(*theValue, MeshDataJsonElement::BOUNDS_MAX);
        // Both min/max corners are missing - OK (we might be reading an older format subversion)
        if (aBoundsMinField == nullptr && aBoundsMaxField == nullptr)
        {
            return true;
        }

        auto readCoords = [](const JsonValue *theSrc, gp_Pnt &theDst)
        {
            constexpr rapidjson::SizeType aNbCoords = 3;
            if (theSrc == nullptr || !theSrc->IsArray() || theSrc->Size() != aNbCoords)
            {
                return false;
            }
            for (rapidjson::SizeType i = 0; i < aNbCoords; i++)
            {
                theDst.SetCoord(static_cast<int>(i) + 1, (*theSrc)[i].GetDouble());
            }
            return true;
        };

        gp_Pnt aCornerMin;
        gp_Pnt aCornerMax;

        // readCoords() returning false means broken JSON structure
        if (!readCoords(aBoundsMinField, aCornerMin) || !readCoords(aBoundsMaxField, aCornerMax))
        {
            return false;
        }

        Bnd_Box anAABB(aCornerMin, aCornerMax);
        myMeshData->SetCachedMinMax(anAABB);

        return true;
    }

    // ================================================================
    // Function : readBlocks
    // ================================================================
    bool MeshBinReader::readBlocks(std::istream &, const JsonValue *theValue)
    {
        const JsonValue *aBlocksField = findObjectMember(*theValue, MeshDataJsonElement::BLOCKS);
        if (aBlocksField == nullptr)
        {
            return true;
        }

        Handle(MeshDataBlock) aRootNode;

        std::function<bool(const JsonValue *theSrc, Handle(MeshDataBlock) &)> readBlockNode =
            [&](const JsonValue *theSrc, Handle(MeshDataBlock) & theNode) -> bool
        {
            const JsonValue *aBlockNameField = findObjectMember(*theSrc, MeshDataJsonElement::BLOCK_NAME);
            if (aBlockNameField == nullptr)
            {
                return false;
            }

            TCollection_AsciiString aBlockName = aBlockNameField->GetString();
            theNode = new MeshDataBlock(aBlockName);

            const JsonValue *aBlockChildrenField = findObjectMember(*theSrc, MeshDataJsonElement::BLOCK_CHILDREN);

            if (aBlockChildrenField == nullptr)
            {
                const JsonValue *aBlockRangeField = findObjectMember(*theSrc, MeshDataJsonElement::BLOCK_RANGE);
                if (aBlockRangeField == nullptr)
                {
                    return false;
                }

                int aRange[4] = {-1, -1, -1, -1};
                int i = 0;
                for (rapidjson::Value::ConstValueIterator anIter = aBlockRangeField->Begin();
                     anIter != aBlockRangeField->End() && i < 4;
                     ++anIter, ++i)
                {
                    aRange[i] = anIter->GetInt();
                }
                theNode->Range().SetValues(aRange[0], aRange[1], aRange[2], aRange[3]);

                return true;
            }

            for (rapidjson::Value::ConstValueIterator anIter = aBlockChildrenField->Begin();
                 anIter != aBlockChildrenField->End();
                 ++anIter)
            {
                Handle(MeshDataBlock) aChild;
                if (!readBlockNode(anIter, aChild))
                {
                    return false;
                }
                theNode->AppendChild(aChild);
            }

            return true;
        };

        if (!readBlockNode(aBlocksField, aRootNode))
        {
            return false;
        }

        myMeshData->SetBlocks(aRootNode);

        return true;
    }

    // ================================================================
    // Function : readNodes
    // ================================================================
    bool MeshBinReader::readNodes(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return true;
        }

        const unsigned int aNodesBufferId = theValue->GetUint();
        if (aNodesBufferId >= static_cast<unsigned int>(myBufferData.Size()))
        {
            return false;
        }

        std::shared_ptr<std::vector<char>> aNodesBuffer = readBinData(theStream,
                                                                      myBufferData[aNodesBufferId].offset,
                                                                      myBufferData[aNodesBufferId].length);
        if (aNodesBuffer.get() == nullptr)
        {
            return false;
        }

        return readNodes(aNodesBuffer);
    }

    // ================================================================
    // Function : readNodes
    // ================================================================
    bool MeshBinReader::readNodes(const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return readNodesUncompressed(theBuffer);
        case MeshCompressionType::Draco:
            return readNodesDraco(theBuffer);
        default:
            break;
        }

        return false;
    }

    // ================================================================
    // Function : readNodesUncompressed
    // ================================================================
    bool MeshBinReader::readNodesUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        const size_t aNbNodes = theBuffer->size() / sizeof(Graphic3d_Vec3);
        Graphic3d_Vec3 aNode;
        for (size_t i = 0; i < aNbNodes; i++)
        {
            size_t anOffset = i * sizeof(Graphic3d_Vec3);

            aNode.x() = *reinterpret_cast<float *>(&(*theBuffer)[anOffset]);
            anOffset += sizeof(float);
            aNode.y() = *reinterpret_cast<float *>(&(*theBuffer)[anOffset]);
            anOffset += sizeof(float);
            aNode.z() = *reinterpret_cast<float *>(&(*theBuffer)[anOffset]);
            myMeshData->AddNodePosition(aNode);
        }

        return true;
    }

    // ================================================================
    // Function : readNodesDraco
    // ================================================================
    bool MeshBinReader::readNodesDraco(const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        if (myDracoCompressionInfo.NodesQuantBits() == -1)
        {
            return readNodesUncompressed(theBuffer);
        }

        std::vector<MeshDecodedPart<1>> aDecodedParts;

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        size_t aNbNodesTotal = 0;
        if (!ProcessDecodedParts(aDecodedParts,
                                 toSupportParallelDraco(),
                                 isParallel,
                                 toReadDracoVersion9(),
                                 theBuffer,
                                 aNbNodesTotal))
        {
            return false;
        }

        if (aNbNodesTotal == 0)
        {
            return true;
        }

        for (int aPartId = 0; aPartId < (int)aDecodedParts.size(); aPartId++)
        {
            MeshAttributeDecoder &aDecoder = aDecodedParts[aPartId].decoders[0];
            const size_t aNbNodes = aDecoder.NbValues();
            Graphic3d_Vec3 aNode;
            for (size_t i = 0; i < aNbNodes; i++)
            {
                aDecoder.GetValue(0, static_cast<int>(i), aNode.ChangeData());
                myMeshData->AddNodePosition(aNode);
            }
        }

        return true;
    }

    // ================================================================
    // Function : readElement
    // ================================================================
    bool MeshBinReader::readElement(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return false;
        }

        const JsonValue *aTypeField = findObjectMember(*theValue, MeshDataJsonElement::ELEMENT_TYPE);
        const JsonValue *aBufferIdField = findObjectMember(*theValue, MeshDataJsonElement::ELEMENT_INDICES);
        if (aTypeField == nullptr || aBufferIdField == nullptr)
        {
            return false;
        }
        MeshDataBufferType aType = MeshDataBufferTypeConvertor::GetType(aTypeField->GetString());
        if (aType == MeshDataBufferType::ELEMENT_INDEXED)
        {
            // indexed elements should be computed using stored mesh elements
            return true;
        }

        unsigned int aBufferId = aBufferIdField->GetUint();

        std::shared_ptr<std::vector<char>> aBuffer = readBinData(theStream,
                                                                 myBufferData[aBufferId].offset,
                                                                 myBufferData[aBufferId].length);
        if (aBuffer.get() == nullptr)
        {
            return false;
        }

        return readElement(aBuffer, aType);
    }

    // ================================================================
    // Function : readElement
    // ================================================================
    bool MeshBinReader::readElement(const std::shared_ptr<std::vector<char>> &theBuffer, MeshDataBufferType theType)
    {
        auto readElems = [this, &theBuffer, &theData = myMeshData](MeshElemType theElemType)
        {
            return readElements(theBuffer, theData->ChangeElements(theElemType));
        };
        auto readSurfElems = [this, &theBuffer, &theData = myMeshData](MeshElemType theElemType)
        {
            theData->SetToUpdateReconstructedSurface(false);
            return readElements(theBuffer, theData->ChangeSurfaceElements(theElemType));
        };

        switch (theType)
        {
        case MeshDataBufferType::ELEMENT_0D:
            return readElems(MeshElemType::Elem0d);
        case MeshDataBufferType::ELEMENT_2D:
            return readElements2dVer11(theBuffer,
                                       myMeshData->ChangeElements(MeshElemType::Quad),
                                       myMeshData->ChangeElements(MeshElemType::Triangle));
        case MeshDataBufferType::ELEMENT_TRIANGLE:
            return readElems(MeshElemType::Triangle);
        case MeshDataBufferType::ELEMENT_TRIANGLE_2ND:
            return readElems(MeshElemType::Triangle2nd);
        case MeshDataBufferType::ELEMENT_QUAD:
            return readElems(MeshElemType::Quad);
        case MeshDataBufferType::ELEMENT_QUAD_2ND:
            return readElems(MeshElemType::Quad2nd);
        case MeshDataBufferType::ELEMENT_POLYGON:
            return readElems(MeshElemType::Polygon);
        case MeshDataBufferType::ELEMENT_TETRAHEDRON:
            return readElems(MeshElemType::Tetrahedron);
        case MeshDataBufferType::ELEMENT_TETRAHEDRON_2ND:
            return readElems(MeshElemType::Tetrahedron2nd);
        case MeshDataBufferType::ELEMENT_PYRAMID:
            return readElems(MeshElemType::Pyramid);
        case MeshDataBufferType::ELEMENT_PYRAMID_2ND:
            return readElems(MeshElemType::Pyramid2nd);
        case MeshDataBufferType::ELEMENT_PRISM:
            return readElems(MeshElemType::Prism);
        case MeshDataBufferType::ELEMENT_PRISM_2ND:
            return readElems(MeshElemType::Prism2nd);
        case MeshDataBufferType::ELEMENT_HEXAHEDRON:
            return readElems(MeshElemType::Hexahedron);
        case MeshDataBufferType::ELEMENT_HEXAHEDRON_2ND:
            return readElems(MeshElemType::Hexahedron2nd);
        case MeshDataBufferType::ELEMENT_POLYHEDRON:
            return readElems(MeshElemType::Polyhedron);
        case MeshDataBufferType::ELEMENT_SURFACE:
        {
            myMeshData->SetToUpdateReconstructedSurface(false);
            return readElements2dVer11(theBuffer,
                                       myMeshData->ChangeSurfaceElements(MeshElemType::Quad),
                                       myMeshData->ChangeSurfaceElements(MeshElemType::Triangle));
        }
        case MeshDataBufferType::ELEMENT_SURFTRIANGLE:
            return readSurfElems(MeshElemType::Triangle);
        case MeshDataBufferType::ELEMENT_SURFTRIANGLE_2ND:
            return readSurfElems(MeshElemType::Triangle2nd);
        case MeshDataBufferType::ELEMENT_SURFQUAD:
            return readSurfElems(MeshElemType::Quad);
        case MeshDataBufferType::ELEMENT_SURFQUAD_2ND:
            return readSurfElems(MeshElemType::Quad2nd);
        case MeshDataBufferType::ELEMENT_SURFPOLYGON:
            return readSurfElems(MeshElemType::Polygon);
        case MeshDataBufferType::ELEMENT_1D:
            // not implemented
        default:
            return false;
        }
    }

    // ================================================================
    // Function : readProperties
    // ================================================================
    bool MeshBinReader::readProperties(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return true; // it is optional field
        }

        for (rapidjson::Value::ConstValueIterator anIter = theValue->Begin(); anIter != theValue->End(); ++anIter)
        {
            if (!readProperty(theStream, anIter))
            {
                return false;
            }
        }

        return true;
    }

    // ================================================================
    // Function : readProperty
    // ================================================================
    bool MeshBinReader::readProperty(std::istream &theStream, const JsonValue *theValue)
    {
        if (theValue == nullptr)
        {
            return false;
        }

        const JsonValue *aCompTypeField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_COMP_TYPE);
        const JsonValue *aNameField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_NAME);
        const JsonValue *aNbComponentsField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_NB_COMPONENTS);
        const JsonValue *aTypeField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_TYPE);
        const JsonValue *anIsElementalField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_IS_ELEMENTAL);

        if (aCompTypeField == nullptr || aNameField == nullptr || aNbComponentsField == nullptr || aTypeField == nullptr || anIsElementalField == nullptr)
        {
            return false;
        }

        const MeshDataBufferType aCompType = MeshDataBufferTypeConvertor::GetType(aCompTypeField->GetString());
        const TCollection_AsciiString aName = aNameField->GetString();
        const int aNbComponents = static_cast<int>(aNbComponentsField->GetUint());
        const MeshNVecPropertyType aType = (MeshNVecPropertyType)aTypeField->GetUint();
        const MeshPropertyBase::MappingType aMapType = anIsElementalField->GetBool()
                                                           ? MeshPropertyBase::MappingType::Elemental
                                                           : MeshPropertyBase::MappingType::Nodal;

        Handle(MeshNVecProperty) aProp = new MeshNVecProperty(aName, aNbComponents, aMapType, aType);
        readPropertyIdField(theValue, aProp);
        readPropertyTimeField(theValue, aProp);

        if (!shouldPropBeProcessed(aProp))
        {
            return true;
        }

        const Handle(MeshNVecProperty) &aStoredProp = myMeshData->GetPropertyById(aProp->Id(), aProp->TimeStep());
        if (!aStoredProp.IsNull() && aStoredProp->NbComponents() == aProp->NbComponents() && aStoredProp->MapType() == aProp->MapType() && aStoredProp->Type() == aProp->Type())
        {
            aProp = aStoredProp;

            myUpdatedProps.Add(aProp);
        }
        else
        {
            myMeshData->ChangeProperties().Append(aProp);

            if (aCompType == MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL)
            {
                aProp->Init<MeshNodeVecProperty>();
            }
            else if (aCompType == MeshDataBufferType::PROPERTY_GROUP)
            {
                aProp->Init<MeshGroupProperty>();
            }
            else
            {
                aProp->Init<MeshScalarProperty>();
            }
        }

        readPropertyDescriptionField(theValue, aProp);

        if (!readPropertyComponents(theStream, theValue, aProp, aCompType))
        {
            return false;
        }

        // Need to recalculate auto-generated components after decoding as they aren't loaded
        // from an encoded stream.
        aProp->UpdateComponents(myCompressionType != MeshCompressionType::None);

        return true;
    }

    // ================================================================
    // Function : readPropertyIdField
    // ================================================================
    void MeshBinReader::readPropertyIdField(const JsonValue *theValue, const Handle(MeshPropertyBase) & theProp)
    {
        const JsonValue *aPropertyIdField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_ID);
        if (aPropertyIdField != nullptr)
        {
            theProp->SetId(aPropertyIdField->GetString());
        }
    }

    // ================================================================
    // Function : readPropertyDescriptionField
    // ================================================================
    void MeshBinReader::readPropertyDescriptionField(const JsonValue *theValue, const Handle(MeshPropertyBase) & theProp)
    {
        const JsonValue *aDescriptionField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_DESCRIPTION);
        TCollection_AsciiString aDescription(aDescriptionField != nullptr ? aDescriptionField->GetString() : "");
        if (aDescription.IsEmpty())
        {
            aDescription = theProp->Name();
        }
        theProp->SetDescription(aDescription);
    }

    // ================================================================
    // Function : readPropertyTimeField
    // ================================================================
    void MeshBinReader::readPropertyTimeField(const JsonValue *theValue, const Handle(MeshNVecProperty) & theProp)
    {
        const JsonValue *aTimeStampField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_TIMESTAMP);
        const JsonValue *aTimeStepField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_TIMESTEP);
        if (aTimeStampField != nullptr && aTimeStepField != nullptr)
        {
            theProp->SetTime(aTimeStampField->GetDouble(), static_cast<int>(aTimeStepField->GetDouble()));
        }

        const JsonValue *aTimeStepDescriptionField = findObjectMember(*theValue,
                                                                      MeshDataJsonElement::PROPERTY_TIME_DESCRIPTION);
        const JsonValue *aTimeUnitsField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_TIME_UNITS);
        if (aTimeStepDescriptionField != nullptr && aTimeUnitsField != nullptr)
        {
            theProp->SetTimeStepDescription(aTimeStepDescriptionField->GetString());
            theProp->SetTimeDisplayUnits(aTimeUnitsField->GetString());
        }
    }

    // ================================================================
    // Function : readPropertyIsEnabledField
    // ================================================================
    void MeshBinReader::readPropertyIsEnabledField(const JsonValue *theValue, const Handle(MeshScalarPropertyBase) & theProp)
    {
        const JsonValue *anIsEnabledField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_IS_ENABLED);
        if (anIsEnabledField != nullptr)
        {
            theProp->SetEnabled(anIsEnabledField->GetBool());
        }
    }

    // ================================================================
    // Function : readPropertyElemTypeField
    // ================================================================
    void MeshBinReader::readPropertyElemTypeField(const JsonValue *theValue, const Handle(MeshScalarPropertyBase) & theProp)
    {
        const JsonValue *aField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_ELEM_TYPE);
        if (aField != nullptr)
        {
            MeshPropertyElemType aType = MeshPropertyElemTypeConvertor::GetType(aField->GetString());
            if (Handle(MeshScalarProperty) aScalarProp = Handle(MeshScalarProperty)::DownCast(theProp))
            {
                aScalarProp->Init(0, aType);
            }
            else if (Handle(MeshNodeVecProperty) aNodeVecProp = Handle(MeshNodeVecProperty)::DownCast(theProp))
            {
                aNodeVecProp->Init(0, 0, false, false, aType);
            }
        }
    }

    // ================================================================
    // Function : readPropertyMinMaxField
    // ================================================================
    void MeshBinReader::readPropertyMinMaxField(const JsonValue *theValue, const Handle(MeshScalarPropertyBase) & theProp)
    {
        const JsonValue *aPropertyMinField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_MIN);
        const JsonValue *aPropertyMaxField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_MAX);
        if (aPropertyMinField != nullptr && aPropertyMaxField != nullptr)
        {
            if (theProp->ElemType() == MeshPropertyElemType::Float)
            {
                theProp->SetValueRange(
                    Bnd_Range((float)aPropertyMinField->GetDouble(), (float)aPropertyMaxField->GetDouble()));
            }
            else
            {
                theProp->SetValueRange(Bnd_Range(aPropertyMinField->GetDouble(), aPropertyMaxField->GetDouble()));
            }
        }
    }

    // ================================================================
    // Function : readPropertyUnitsField
    // ================================================================
    void MeshBinReader::readPropertyUnitsField(const JsonValue *theValue, const Handle(MeshScalarPropertyBase) & theProp)
    {
        const JsonValue *aUnitsField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_UNITS);
        if (aUnitsField != nullptr)
        {
            theProp->SetDisplayUnits(aUnitsField->GetString());
        }
    }

    // ================================================================
    // Function : readPropertyComponents
    // ================================================================
    bool MeshBinReader::readPropertyComponents(std::istream &theStream,
                                               const JsonValue *theValue,
                                               const Handle(MeshNVecProperty) & theProp,
                                               const MeshDataBufferType theType)
    {
        const JsonValue *aPropCompField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_COMPONENTS);
        if (aPropCompField == nullptr)
        {
            return false;
        }

        const int aNbComponents = theProp->NbRawComponents();
        int aCompIndex = 0;
        for (rapidjson::Value::ConstValueIterator anIter = aPropCompField->Begin();
             anIter != aPropCompField->End() && aCompIndex < aNbComponents;
             ++anIter, ++aCompIndex)
        {
            if (anIter == nullptr)
            {
                continue;
            }

            const Handle(MeshScalarPropertyBase) &aComp = theProp->GetRawComponent(aCompIndex);

            const JsonValue *aNameField = findObjectMember(*anIter, MeshDataJsonElement::PROPERTY_NAME);
            if (aNameField == nullptr)
            {
                continue;
            }
            aComp->SetName(aNameField->GetString());

            readPropertyIdField(anIter, aComp);
            readPropertyDescriptionField(anIter, aComp);
            readPropertyIsEnabledField(anIter, aComp);
            readPropertyElemTypeField(anIter, aComp);
            readPropertyMinMaxField(anIter, aComp);
            readPropertyUnitsField(anIter, aComp);

            const JsonValue *aBufferIdField = findObjectMember(*anIter, MeshDataJsonElement::PROPERTY_VALUES);

            bool haveBinaryData = aBufferIdField != nullptr;
            bool toLoadBinaryData = !CheckOption(MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA) && haveBinaryData;

            bool toTempLoadGroupProp = theType == MeshDataBufferType::PROPERTY_GROUP && haveBinaryData && !toLoadBinaryData && myVersionMajor < MeshImportExport::MESH_DATA_VERSION_MAJOR_11;

            if (toLoadBinaryData || toTempLoadGroupProp)
            {
                unsigned int aBufferId = aBufferIdField->GetUint();
                if (!readPropertyBinData(aComp, theType, theStream, aBufferId))
                {
                    return false;
                }
            }

            if (theType == MeshDataBufferType::PROPERTY_GROUP)
            {
                readPropertyGroupsField(anIter, Handle(MeshGroupProperty)::DownCast(aComp));
            }

            if (toTempLoadGroupProp)
            {
                aComp->Unload();
            }

            if (haveBinaryData && CheckOption(MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE))
            {
                aComp->SetRefFilePath(myFileName);
            }
        }

        return true;
    }

    // ================================================================
    // Function : readPropertyGroupsField
    // ================================================================
    void MeshBinReader::readPropertyGroupsField(const JsonValue *theValue, const Handle(MeshGroupProperty) & theProp)
    {
        const JsonValue *aPropertyGroupsField = findObjectMember(*theValue, MeshDataJsonElement::PROPERTY_GROUPS);
        if (aPropertyGroupsField != nullptr)
        {
            for (rapidjson::Value::ConstValueIterator anIter = aPropertyGroupsField->Begin();
                 anIter != aPropertyGroupsField->End();
                 ++anIter)
            {
                if (anIter == nullptr)
                {
                    continue;
                }

                const JsonValue *aPropertyGroupIdField = findObjectMember(*anIter, MeshDataJsonElement::PROPERTY_GROUP_ID);
                const JsonValue *aPropertyGroupNameField = findObjectMember(*anIter,
                                                                            MeshDataJsonElement::PROPERTY_GROUP_NAME);
                if (aPropertyGroupIdField != nullptr && aPropertyGroupNameField != nullptr)
                {
                    theProp->SetGroupName(aPropertyGroupIdField->GetUint(), aPropertyGroupNameField->GetString());
                }
                const JsonValue *aPropertyGroupNbEntitiesField =
                    findObjectMember(*anIter, MeshDataJsonElement::PROPERTY_GROUP_NB_ENTITIES);
                if (aPropertyGroupIdField != nullptr && aPropertyGroupNbEntitiesField != nullptr)
                {
                    theProp->SetGroupNbEntities(aPropertyGroupIdField->GetUint(), aPropertyGroupNbEntitiesField->GetInt());
                }
            }
        }
    }

    // ================================================================
    // Function : readElements
    // ================================================================
    bool MeshBinReader::readElements(const std::shared_ptr<std::vector<char>> &theBuffer, MeshElements &theElements)
    {
        if (myVersionMajor < MeshImportExport::MESH_DATA_VERSION_MAJOR_12)
        {
            MeshElements aDummy;
            switch (myCompressionType)
            {
            case MeshCompressionType::None:
                return readElementsUncompressedVer11(theBuffer, theElements, aDummy);
            case MeshCompressionType::Draco:
                return readElementsDracoVer11(theBuffer, theElements, aDummy);
            default:
                break;
            }
        }
        else
        {
            switch (myCompressionType)
            {
            case MeshCompressionType::None:
                return readElementsUncompressed(theBuffer, theElements);
            case MeshCompressionType::Draco:
                return readElementsDraco(theBuffer, theElements);
            default:
                break;
            }
        }

        return false;
    }

    // ================================================================
    // Function : readElementsUncompressed
    // ================================================================
    template <class Elem_T>
    bool MeshBinReader::readElementsUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                 NCollection_Vector<Elem_T> &theElements,
                                                 uint64_t &theOffset)
    {
        uint64_t aSize = *reinterpret_cast<const uint64_t *>(&(*theBuffer)[theOffset]);
        theOffset += sizeof(aSize);
        if (aSize == 0)
        {
            return true;
        }

        theElements.SetValue(aSize - 1, Elem_T());
        for (uint64_t i = 0; i < aSize; i++)
        {
            Elem_T aValue = *reinterpret_cast<const Elem_T *>(&(*theBuffer)[i * sizeof(Elem_T) + theOffset]);
            theElements.SetValue(i, aValue);
        }

        theOffset += aSize * sizeof(Elem_T);

        return true;
    }

    // ================================================================
    // Function : readElementsUncompressed
    // ================================================================
    bool MeshBinReader::readElementsUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                 MeshElements &theElements)
    {
        uint64_t anOffset = 0;
        readElementsUncompressed(theBuffer, theElements.NodesOffsets(), anOffset);
        readElementsUncompressed(theBuffer, theElements.Nodes(), anOffset);
        readElementsUncompressed(theBuffer, theElements.FacesOffsets(), anOffset);
        readElementsUncompressed(theBuffer, theElements.Faces(), anOffset);
        readElementsUncompressed(theBuffer, theElements.GlobalIndices(), anOffset);

        return true;
    }

    //! Init mesh elements from draco decoders.
    void InitMeshElementsDraco(const std::vector<MeshDecodedPart<5>> &theDecodedParts,
                               MeshElements &theElements,
                               uint32_t &theNbElems,
                               uint64_t &theNbNodes,
                               uint64_t &theNbFaces)
    {
        for (int aPartId = 0; aPartId < (int)theDecodedParts.size(); aPartId++)
        {
            const MeshAttributeDecoder &aDecoderNodes = theDecodedParts[aPartId].decoders[2];
            const MeshAttributeDecoder &aDecoderFaces = theDecodedParts[aPartId].decoders[3];
            const MeshAttributeDecoder &aDecoderGI = theDecodedParts[aPartId].decoders[4];

            theNbElems += static_cast<uint32_t>(aDecoderGI.NbValues());
            theNbNodes += aDecoderNodes.NbValues();
            theNbFaces += aDecoderFaces.NbValues();
        }

        if (theElements.ToStoreNodesOffsets())
        {
            theElements.NodesOffsets().SetValue(theNbElems - 1, 0);
        }
        theElements.Nodes().SetValue(static_cast<int>(theNbNodes - 1), 0);
        if (theElements.ToStoreFaces())
        {
            theElements.FacesOffsets().SetValue(theNbElems - 1, 0);
            theElements.Faces().SetValue(static_cast<int>(theNbFaces - 1), 0);
        }
        theElements.GlobalIndices().SetValue(theNbElems - 1, -1);
    }

    //! Decode mesh element from draco decoders.
    void DecodeMeshElementDraco(const std::vector<RWCae::MeshAttributeDecoder> &theDecoders,
                                MeshElements &theElements,
                                uint32_t &theNbProcElems,
                                uint32_t theElemIter,
                                uint64_t &thePrevNodesOffset,
                                uint32_t &theLastFirstNode,
                                MeshElemEntityOffset &theNodesIter,
                                uint64_t &thePrevFacesOffset,
                                MeshElemEntityOffset &theFacesIter)
    {
        uint64_t aNodesOffset = 0;
        if (theElements.ToStoreNodesOffsets())
        {
            if (theElemIter == 0)
            {
                theDecoders[ELEM_DEC_FIRST_OFFSETS].GetValue(0, 0, &aNodesOffset);
            }
            else
            {
                uint8_t aVal = 0;
                theDecoders[ELEM_DEC_OFFSETS].GetValue(0, theElemIter - 1, &aVal);
                aNodesOffset = thePrevNodesOffset + aVal;
            }
            theElements.NodesOffsets().SetValue(theNbProcElems + theElemIter, aNodesOffset);
        }
        else
        {
            aNodesOffset = thePrevNodesOffset + theElements.FixedElemSize();
        }

        uint32_t aFirstNode = 0;
        theDecoders[ELEM_DEC_NODES].GetValue(0, static_cast<int>(theNodesIter++), &aFirstNode);
        int32_t aDiff = static_cast<int32_t>(aFirstNode >> 1) * ((aFirstNode & 1) == 1 ? -1 : 1);
        uint32_t aDecFirstNode = aDiff + theLastFirstNode;
        theLastFirstNode = aDecFirstNode;
        theElements.Nodes().SetValue(static_cast<int>(thePrevNodesOffset), aDecFirstNode);
        for (uint64_t aNodeIter = thePrevNodesOffset + 1; aNodeIter < aNodesOffset; aNodeIter++)
        {
            uint32_t aNode = 0;
            theDecoders[ELEM_DEC_NODES].GetValue(0, static_cast<int>(theNodesIter++), &aNode);
            aDiff = static_cast<int32_t>(aNode >> 1) * ((aNode & 1) == 1 ? -1 : 1);
            uint32_t aDecNode = aDiff + aDecFirstNode;
            aDecFirstNode = aDecNode;
            theElements.Nodes().SetValue(static_cast<int>(aNodeIter), aDecNode);
        }

        thePrevNodesOffset = aNodesOffset;

        if (theElements.ToStoreFaces())
        {
            uint64_t aFacesOffset = 0;
            if (theElemIter == 0)
            {
                theDecoders[ELEM_DEC_FIRST_OFFSETS].GetValue(1, 0, &aFacesOffset);
            }
            else
            {
                uint16_t aVal = 0;
                theDecoders[ELEM_DEC_OFFSETS].GetValue(1, theElemIter - 1, &aVal);
                aFacesOffset = thePrevFacesOffset + aVal;
            }
            theElements.FacesOffsets().SetValue(theNbProcElems + theElemIter, aFacesOffset);

            for (uint64_t aFaceIter = thePrevFacesOffset; aFaceIter < aFacesOffset; aFaceIter++)
            {
                uint8_t aFaceValue = 0;
                theDecoders[ELEM_DEC_FACES].GetValue(0, static_cast<int>(theFacesIter++), &aFaceValue);
                theElements.Faces().SetValue(static_cast<int>(aFaceIter), aFaceValue);
            }

            thePrevFacesOffset = aFacesOffset;
        }
    }

    // ================================================================
    // Function : readElementsDraco
    // ================================================================
    bool MeshBinReader::readElementsDraco(const std::shared_ptr<std::vector<char>> &theBuffer, MeshElements &theElements)
    {
        static constexpr int aNbDecs = 5;

        std::vector<MeshDecodedPart<aNbDecs>> aDecodedParts;

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        size_t aNbValuesTotal = 0;
        if (!ProcessDecodedParts(aDecodedParts, toSupportParallelDraco(), isParallel, false, theBuffer, aNbValuesTotal))
        {
            return false;
        }

        if (aNbValuesTotal == 0)
        {
            return true;
        }

        uint32_t aNbElems = 0;
        uint64_t aNbNodes = 0;
        uint64_t aNbFaces = 0;
        InitMeshElementsDraco(aDecodedParts, theElements, aNbElems, aNbNodes, aNbFaces);

        uint32_t aNbProcElems = 0;
        for (int aPartId = 0; aPartId < (int)aDecodedParts.size(); aPartId++)
        {
            std::vector<RWCae::MeshAttributeDecoder> &aDecoders = aDecodedParts[aPartId].decoders;

            aNbElems = static_cast<uint32_t>(aDecoders[ELEM_DEC_GI].NbValues());

            uint64_t aPrevNodesOffset = theElements.ToStoreNodesOffsets()
                                            ? (aNbProcElems > 0 ? theElements.NodesOffsets()[aNbProcElems - 1] : 0)
                                            : (aNbProcElems * theElements.FixedElemSize());
            uint64_t aPrevFacesOffset = theElements.ToStoreFaces()
                                            ? (aNbProcElems > 0 ? theElements.FacesOffsets()[aNbProcElems - 1] : 0)
                                            : 0;
            int aLastGlobalIndex = 0;
            uint32_t aLastFirstNode = 0;

            MeshElemEntityOffset aNodesIter = 0;
            MeshElemEntityOffset aFacesIter = 0;
            for (uint32_t anElemIter = 0; anElemIter < aNbElems; anElemIter++)
            {
                DecodeMeshElementDraco(aDecoders,
                                       theElements,
                                       aNbProcElems,
                                       anElemIter,
                                       aPrevNodesOffset,
                                       aLastFirstNode,
                                       aNodesIter,
                                       aPrevFacesOffset,
                                       aFacesIter);

                uint32_t aGlobalIndex = 0;
                aDecoders[ELEM_DEC_GI].GetValue(0, anElemIter, &aGlobalIndex);
                int32_t aDiff = static_cast<int32_t>(aGlobalIndex >> 1) * ((aGlobalIndex & 1) == 1 ? -1 : 1);
                int aDecGlobalIndex = aDiff + aLastGlobalIndex;
                aLastGlobalIndex = aDecGlobalIndex;
                theElements.GlobalIndices().SetValue(aNbProcElems + anElemIter, aDecGlobalIndex);
            }

            aNbProcElems += aNbElems;
        }

        return true;
    }

    // ================================================================
    // Function : readElements2dVer11
    // ================================================================
    bool MeshBinReader::readElements2dVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                            MeshElements &theQuads,
                                            MeshElements &theTriangles)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return readElementsUncompressedVer11(theBuffer, theQuads, theTriangles);
        case MeshCompressionType::Draco:
            return readElementsDracoVer11(theBuffer, theQuads, theTriangles);
        default:
            break;
        }
        return false;
    }

    // ================================================================
    // Function : readElementsUncompressedVer11
    // ================================================================
    bool MeshBinReader::readElementsUncompressedVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                      MeshElements &theElements,
                                                      MeshElements &theAuxElements)
    {
        MeshElemEntityId aNbNodes = theElements.FixedElemSize();

        const size_t anElemNodesSize = static_cast<size_t>(aNbNodes) * sizeof(int32_t);
        const size_t anElemSize = anElemNodesSize + sizeof(int32_t);
        const size_t aNbElements = theBuffer->size() / anElemSize;

        std::vector<int32_t> aNodes(aNbNodes);

        for (size_t i = 0; i < aNbElements; ++i)
        {
            for (MeshElemEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter)
            {
                aNodes[aNodeIter] = *reinterpret_cast<const int32_t *>(
                    &(*theBuffer)[i * anElemSize + aNodeIter * sizeof(int32_t)]);
            }

            int aGlobalIndex = *reinterpret_cast<const int32_t *>(
                &(*theBuffer)[i * anElemSize + aNbNodes * sizeof(int32_t)]);

            // Special case for triangles
            if (aNodes[aNbNodes - 1] == -1)
            {
                for (MeshElemEntityId aNodeIter = 0; aNodeIter < aNbNodes - 1; ++aNodeIter)
                {
                    theAuxElements.Nodes().SetValue(theAuxElements.Nodes().Size(), (MeshEntityId)aNodes[aNodeIter]);
                }
                theAuxElements.GlobalIndices().SetValue(theAuxElements.GlobalIndices().Size(), aGlobalIndex);
            }
            else
            {
                for (MeshElemEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter)
                {
                    theElements.Nodes().SetValue(theElements.Nodes().Size(), (MeshEntityId)aNodes[aNodeIter]);
                }
                theElements.GlobalIndices().SetValue(theElements.GlobalIndices().Size(), aGlobalIndex);
            }
        }

        return true;
    }

    // ================================================================
    // Function : readElementsDracoVer11
    // ================================================================
    bool MeshBinReader::readElementsDracoVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                               MeshElements &theElements,
                                               MeshElements &theAuxElements)
    {
        std::vector<MeshDecodedPart<1>> aDecodedParts;

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        size_t aNbValuesTotal = 0;
        if (!ProcessDecodedParts(aDecodedParts, toSupportParallelDraco(), isParallel, false, theBuffer, aNbValuesTotal))
        {
            return false;
        }

        if (aNbValuesTotal == 0)
        {
            return true;
        }

        MeshElemEntityId aNbNodes = theElements.FixedElemSize();

        std::vector<uint32_t> aData(aNbNodes);
        std::vector<int32_t> aDecData(aNbNodes);
        for (int aPartId = 0; aPartId < (int)aDecodedParts.size(); aPartId++)
        {
            MeshAttributeDecoder &aDecoder = aDecodedParts[aPartId].decoders[0];

            int32_t aLastFirstIndex = 0;
            int32_t aLastGlobalIndex = 0;

            auto decodeElement = [this, &aDecoder, &aData, &aDecData, aNbNodes](int theElemIter)
            {
                if (aNbNodes == 1)
                {
                    return;
                }

                aDecoder.GetValue(1, theElemIter, &aData[1]);
                int32_t aLastIndex = aDecData[0];
                for (int i = 1; i < aNbNodes; ++i)
                {
                    decodeElementNode(aData[i], aDecData[i], aLastIndex);
                }
            };

            for (size_t anElemIter = 0; anElemIter < aDecoder.NbValues(); ++anElemIter)
            {
                aDecoder.GetValue(0, static_cast<int>(anElemIter), aData.data());

                decodeElementNode(aData[0], aDecData[0], aLastFirstIndex);

                decodeElement(static_cast<int>(anElemIter));

                uint32_t aGlobalIndex = 0;
                aDecoder.GetValue(aNbNodes > 1 ? 2 : 1, static_cast<int>(anElemIter), &aGlobalIndex);
                int32_t aDiff = static_cast<int32_t>(aGlobalIndex >> 1) * ((aGlobalIndex & 1) == 1 ? -1 : 1);
                int aDecGlobalIndex = aDiff + aLastGlobalIndex;
                aLastGlobalIndex = aDecGlobalIndex;

                // Special case for triangles
                if (aDecData[aNbNodes - 1] == -1)
                {
                    AppendMeshElem(theAuxElements, aDecData, aNbNodes - 1, aDecGlobalIndex);
                }
                else
                {
                    AppendMeshElem(theElements, aDecData, aNbNodes, aDecGlobalIndex);
                }
            }
        }

        return true;
    }

    // ================================================================
    // Function : decodeElementNode
    // ================================================================
    void MeshBinReader::decodeElementNode(uint32_t theEncodedIndex, int32_t &theIndex, int32_t &theLastIndex) const
    {
        theIndex = -1;
        if (theEncodedIndex == 0)
        {
            return;
        }
        int32_t aDiff = static_cast<int32_t>((theEncodedIndex - 1) >> 1);
        aDiff *= ((theEncodedIndex - 1) & 1) == 1 ? -1 : 1;
        theIndex = aDiff + theLastIndex;
        theLastIndex = theIndex;
    }

    // ================================================================
    // Function : fillIndexedElements
    // ================================================================
    void MeshBinReader::fillIndexedElements()
    {
        MeshDataVector<MeshData::IndexedElement> &anIndElems = myMeshData->ChangeIndexedElements();
        anIndElems.Clear();

        int aNbTotalElems = myMeshData->NbElems0d() + myMeshData->NbElems2d() + myMeshData->NbElems3d();
        if (aNbTotalElems == 0)
        {
            return;
        }

        anIndElems.SetValue(aNbTotalElems - 1, MeshData::IndexedElement());

        for (int anElemType = 0; anElemType < MeshElemNbTypes; anElemType++)
        {
            MeshElemType aType = (MeshElemType)anElemType;

            const MeshElements &anElems = myMeshData->Elements(aType);

            uint32_t aNbElems = anElems.NbElements();
            for (uint32_t i = 0; i < aNbElems; i++)
            {
                int aGlobalIndex = anElems.GlobalIndex(i);
                anIndElems.SetValue(aGlobalIndex, MeshData::IndexedElement(aType, i));
            }
        }
    }

    // ================================================================
    // Function : readPropertyBinData
    // ================================================================
    bool MeshBinReader::readPropertyBinData(const Handle(MeshScalarPropertyBase) & theProp,
                                            MeshDataBufferType theType,
                                            std::istream &theStream,
                                            unsigned int theBufferId)
    {
        std::shared_ptr<std::vector<char>> aBuffer = readBinData(theStream,
                                                                 myBufferData[theBufferId].offset,
                                                                 myBufferData[theBufferId].length);
        if (aBuffer.get() == nullptr)
        {
            return false;
        }

        switch (theType)
        {
        case MeshDataBufferType::PROPERTY_SCALAR:
            return readPropertyBinDataScalar(Handle(MeshScalarProperty)::DownCast(theProp), aBuffer);
        case MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL:
        {
            return readPropertyBinDataNodeVec(Handle(MeshNodeVecProperty)::DownCast(theProp), aBuffer);
        }
        case MeshDataBufferType::PROPERTY_GROUP:
            return readPropertyBinDataGroup(Handle(MeshGroupProperty)::DownCast(theProp), aBuffer);
        default:
            break;
        }
        return false;
    }

    // ================================================================
    // Function : readPropertyBinDataScalar
    // ================================================================
    bool MeshBinReader::readPropertyBinDataScalar(const Handle(MeshScalarProperty) & theProp,
                                                  const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return readPropertyBinDataScalarUncompressed(theProp, theBuffer);
        case MeshCompressionType::Draco:
            return readPropertyBinDataScalarDraco(theProp, theBuffer);
        default:
            break;
        }

        return false;
    }

    // ================================================================
    // Function : readPropertyBinDataScalarUncompressed
    // ================================================================
    bool MeshBinReader::readPropertyBinDataScalarUncompressed(const Handle(MeshScalarProperty) & theProp,
                                                              const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        size_t anElemSize = theProp->ElemSize();
        const size_t aNbElements = theBuffer->size() / anElemSize;
        theProp->Init(static_cast<int>(aNbElements), theProp->ElemType());
        for (size_t i = 0; i < aNbElements; ++i)
        {
            theProp->SetRawValue(MeshPropertyEntityInfo(static_cast<int>(i)),
                                 &(*theBuffer)[static_cast<int>(i * anElemSize)],
                                 false);
        }

        return true;
    }

    // ================================================================
    // Function : readPropertyBinDataScalarDraco
    // ================================================================
    bool MeshBinReader::readPropertyBinDataScalarDraco(const Handle(MeshScalarProperty) & theProp,
                                                       const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        if (myDracoCompressionInfo.PropsQuantBits() == -1)
        {
            return readPropertyBinDataScalarUncompressed(theProp, theBuffer);
        }

        std::vector<MeshDecodedPart<1>> aDecodedParts;

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        size_t aNbValuesTotal = 0;
        if (!ProcessDecodedParts(aDecodedParts,
                                 toSupportParallelDraco(),
                                 isParallel,
                                 toReadDracoVersion9(),
                                 theBuffer,
                                 aNbValuesTotal))
        {
            return false;
        }

        if (aNbValuesTotal == 0)
        {
            return true;
        }

        theProp->Init(static_cast<int>(aNbValuesTotal), theProp->ElemType());

        int aNbProcessedValues = 0;
        for (int aPartId = 0; aPartId < (int)aDecodedParts.size(); aPartId++)
        {
            MeshAttributeDecoder &aDecoder = aDecodedParts[aPartId].decoders[0];

            for (size_t i = 0; i < aDecoder.NbValues(); ++i)
            {
                float aValue = 0.0f;
                aDecoder.GetValue(0, static_cast<int>(i), &aValue);
                theProp->SetRawValue(MeshPropertyEntityInfo(static_cast<int>(i) + aNbProcessedValues),
                                     reinterpret_cast<const char *>(&aValue),
                                     false);
            }

            aNbProcessedValues += static_cast<int>(aDecoder.NbValues());
        }

        return true;
    }

    // ================================================================
    // Function : readPropertyBinDataNodeVec
    // ================================================================
    bool MeshBinReader::readPropertyBinDataNodeVec(const Handle(MeshNodeVecProperty) & theProp,
                                                   const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return readPropertyBinDataNodeVecUncompressed(theProp, theBuffer);
        case MeshCompressionType::Draco:
            return readPropertyBinDataNodeVecDraco(theProp, theBuffer);
        default:
            break;
        }

        return false;
    }

    // ================================================================
    // Function : readPropertyBinDataNodeVecUncompressed
    // ================================================================
    bool MeshBinReader::readPropertyBinDataNodeVecUncompressed(const Handle(MeshNodeVecProperty) & theProp,
                                                               const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        size_t anOffset = 0;
        size_t anElemSize = theProp->ElemSize();
        const int aNbElements = *reinterpret_cast<const uint32_t *>(&(*theBuffer)[anOffset]);
        anOffset += sizeof(uint32_t);
        theProp->Init(aNbElements, 0, false, false, theProp->ElemType());
        for (int i = 0; i < aNbElements; i++)
        {
            const int aNbValues = *reinterpret_cast<const uint32_t *>(&(*theBuffer)[anOffset]);
            anOffset += sizeof(uint32_t);
            theProp->ReserveValueMap(i, aNbValues);
            for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++)
            {
                int aNodeIndex = *reinterpret_cast<const int32_t *>(&(*theBuffer)[anOffset]);
                anOffset += sizeof(int32_t);

                MeshPropertyEntityInfo anInfo(i, aNodeIndex);
                theProp->SetRawValue(anInfo, &(*theBuffer)[anOffset]);
                anOffset += anElemSize;
            }
        }

        return true;
    }

    // ================================================================
    // Function : readPropertyBinDataNodeVecDraco
    // ================================================================
    bool MeshBinReader::readPropertyBinDataNodeVecDraco(const Handle(MeshNodeVecProperty) & theProp,
                                                        const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        constexpr int aNbDecoders = 2;
        std::vector<MeshDecodedPart<aNbDecoders>> aDecodedParts;

        bool isParallelProc = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        size_t aNbValuesTotal = 0;
        if (!ProcessDecodedParts(aDecodedParts,
                                 toSupportParallelDraco(),
                                 isParallelProc,
                                 toReadDracoVersion9(),
                                 theBuffer,
                                 aNbValuesTotal))
        {
            return false;
        }

        if (aNbValuesTotal == 0)
        {
            return true;
        }

        theProp->Init(static_cast<int>(aNbValuesTotal), 0, false, false, theProp->ElemType());

        bool isFloatType = theProp->ElemType() == MeshPropertyElemType::Float;

        int aNbProcessedValues = 0;

        for (int aPartId = 0; aPartId < (int)aDecodedParts.size(); aPartId++)
        {
            MeshAttributeDecoder &aDecoderSizes = aDecodedParts[aPartId].decoders[0];
            MeshAttributeDecoder &aDecoderValues = aDecodedParts[aPartId].decoders[1];

            uint32_t anEncodedIter = 0;

            for (int anElemIter = 0; anElemIter < (int)aDecoderSizes.NbValues(); anElemIter++)
            {
                MeshPropertyEntityInfo anInfo(anElemIter + aNbProcessedValues);

                uint32_t aNbNodes = 0;
                aDecoderSizes.GetValue(0, anElemIter, &aNbNodes);
                theProp->ReserveValueMap(anInfo.Index, aNbNodes);

                for (int aNodeIter = 0; aNodeIter < (int)aNbNodes; aNodeIter++)
                {
                    int32_t aNode = 0;
                    aDecoderValues.GetValue(0, anEncodedIter, &aNode);
                    anInfo.SubIndex = aNode;

                    float aFloatValue = 0.0f;
                    double aDoubleValue = 0.0;
                    aDecoderValues.GetValue(1, anEncodedIter, isFloatType ? (void *)&aFloatValue : (void *)&aDoubleValue);
                    theProp->SetRawValue(anInfo,
                                         reinterpret_cast<const char *>(isFloatType ? (void *)&aFloatValue
                                                                                    : (void *)&aDoubleValue),
                                         false);

                    anEncodedIter++;
                }
            }

            aNbProcessedValues += static_cast<int>(aDecoderSizes.NbValues());
        }

        return true;
    }

    // ================================================================
    // Function : readPropertyBinDataGroup
    // ================================================================
    bool MeshBinReader::readPropertyBinDataGroup(const Handle(MeshGroupProperty) & theProp,
                                                 const std::shared_ptr<std::vector<char>> &theBuffer)
    {
        for (int i = 0; i < MeshGroupProperty::MaxNbGroups; i++)
        {
            theProp->SetGroupNbEntities(i, 0);
        }

        const size_t aNbElements = theBuffer->size() / sizeof(MeshGroupProperty::MeshGroupsMaskType);
        theProp->Init(static_cast<int>(aNbElements));
        for (size_t i = 0; i < aNbElements; ++i)
        {
            MeshGroupProperty::MeshGroupsMaskType aValue = *reinterpret_cast<const MeshGroupProperty::MeshGroupsMaskType *>(
                &(*theBuffer)[i * sizeof(MeshGroupProperty::MeshGroupsMaskType)]);
            theProp->SetGroups(static_cast<int>(i), aValue, false);
        }
        return true;
    }

    // ================================================================
    // Function : readBinData
    // ================================================================
    std::shared_ptr<std::vector<char>> MeshBinReader::readBinData(std::istream &theStream,
                                                                  uint64_t theOffset,
                                                                  uint64_t theLength)
    {
        if (theLength <= 0)
        {
            return nullptr;
        }

        const std::streamoff aPrevPos = theStream.tellg();

        std::shared_ptr<std::vector<char>> aRes = std::make_shared<std::vector<char>>(size_t(theLength));

        // Read data by chunks < 2GB due to the problem in GCC compiler.
        uint64_t anOffset = theOffset + myFileOffsets.binDataOffset;
        uint64_t aLength = std::min(theLength, (uint64_t)IntegerLast());
        for (uint64_t aCurOffset = anOffset; aCurOffset < anOffset + theLength; aCurOffset += aLength)
        {
            theStream.seekg(aCurOffset, std::ios::beg);
            theStream.read(aRes->data() + aCurOffset - anOffset, std::min(aLength, anOffset + theLength - aCurOffset));
        }

        theStream.seekg(aPrevPos, std::ios::beg);
        if (!theStream.good())
        {
            return nullptr;
        }

        return aRes;
    }

    // ================================================================
    // Function : checkVersion
    // ================================================================
    bool MeshBinReader::checkVersion() const
    {
        return myVersionMajor >= MeshImportExport::MESH_DATA_VERSION_MAJOR_2 && myVersionMajor <= MeshImportExport::MESH_DATA_VERSION_MAJOR_14;
    }

} // RWCae
