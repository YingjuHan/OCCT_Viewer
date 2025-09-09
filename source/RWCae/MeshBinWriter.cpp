#include <RWCae/MeshBinWriter.h>

#include <RWBase/TaskManager.h>

#include <RWCae/MeshDracoCompressionHelper.h>
#include <RWCae/MeshTools.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>

#include <functional>

namespace
{
    //! Struct that contains compressed info about binary data part.
    template <int N>
    struct MeshEncodedPart
    {
        MeshEncodedPart() : encoders(N) {}

        std::vector<RWCae::MeshAttributeEncoder> encoders;
        bool isOk = false;
    };

    //! Encode binary data parts and write them to stream.
    template <int N>
    uint64_t ProcessEncodedParts(std::vector<MeshEncodedPart<N>> &theParts,
                                 bool theIsParallel,
                                 const std::function<void(int, int)> &theEncodeFunc,
                                 std::ostream &theStream)
    {
        uint64_t aLength = 0;

        uint32_t aNbParts = (uint32_t)theParts.size();

        RWBase::TaskManager::Instance()->RunInParallel(0, (int)aNbParts, theEncodeFunc, !theIsParallel);

        theStream.write((const char *)&aNbParts, sizeof(aNbParts));
        aLength += sizeof(aNbParts);
        for (const MeshEncodedPart<N> &aPart : theParts)
        {
            if (!aPart.isOk)
            {
                return 0;
            }
            uint64_t anEncodedSize = 0;
            for (int i = 0; i < N; i++)
            {
                anEncodedSize += aPart.encoders[i].EncodedSize();
            }
            theStream.write((const char *)&anEncodedSize, sizeof(anEncodedSize));
            aLength += sizeof(anEncodedSize);
        }

        for (MeshEncodedPart<N> &aPart : theParts)
        {
            for (int i = 0; i < N; i++)
            {
                aPart.encoders[i].Write(theStream, aLength);
            }
        }

        return aLength;
    }

}

namespace RWCae
{

    // ================================================================
    // Function : MeshBinWriter
    // ================================================================
    MeshBinWriter::MeshBinWriter(const Handle(MeshData) & theMeshData) : MeshBinIOBase(theMeshData)
    {
    }

    // ================================================================
    // Function : ~MeshBinWriter
    // ================================================================
    MeshBinWriter::~MeshBinWriter()
    {
    }

    // ================================================================
    // Function : Perform
    // ================================================================
    bool MeshBinWriter::Perform(const TCollection_AsciiString &theFileName, const Message_ProgressRange &theProgressRange)
    {
        const Handle(OSD_FileSystem) &aFileSystem = OSD_FileSystem::DefaultFileSystem();
        std::shared_ptr<std::ostream> aFile = aFileSystem->OpenOStream(theFileName, std::ios::out | std::ios::binary);
        if (!aFile)
        {
            Message::SendFail() << "Error: File '" << theFileName << "' cannot be written";
            return false;
        }
        return Perform(*aFile, theFileName, theProgressRange);
    }

    // ================================================================
    // Function : Perform
    // ================================================================
    bool MeshBinWriter::Perform(std::ostream &theStream,
                                const TCollection_AsciiString &theFileName,
                                const Message_ProgressRange &theProgressRange)
    {
        if (theProgressRange.UserBreak())
        {
            return false;
        }

        myFileName = theFileName;
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinWriter: Stream '" << myFileName << "' can not be written";
            return false;
        }

        if (!writeHeader(theStream))
        {
            return false;
        }

        if (!writeJSON(theStream, theProgressRange))
        {
            return false;
        }

        if (!writeBinData(theStream, theProgressRange))
        {
            return false;
        }

        return updateSizes(theStream);
    }

    // ================================================================
    // Function : writeHeader
    // ================================================================
    bool MeshBinWriter::writeHeader(std::ostream &theStream)
    {
        theStream.write(MeshImportExport::MESH_DATA_HEADER, strlen(MeshImportExport::MESH_DATA_HEADER));

        theStream.write((const char *)&MeshImportExport::MESH_DATA_VERSION_MAJOR, 2);

        theStream.write((const char *)&MeshImportExport::MESH_DATA_VERSION_MINOR, 2);

        myFileOffsets.totalOffset = static_cast<uint64_t>(theStream.tellp());

        uint64_t aTotalSize = 0;
        theStream.write((const char *)&aTotalSize, sizeof(uint64_t));

        theStream.flush();
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinWriter: Stream '" << myFileName << "' cannot be written";
            return false;
        }

        return true;
    }

    // ================================================================
    // Function : writeJSON
    // ================================================================
    bool MeshBinWriter::writeJSON(std::ostream &theStream, const Message_ProgressRange &theProgressRange)
    {
        myFileOffsets.jsonOffset = static_cast<uint64_t>(theStream.tellp());

        uint64_t aJsonSize = 0;
        theStream.write((const char *)&aJsonSize, sizeof(uint64_t));
        uint32_t aContentType = 0x4E4F534A; // "JSON"
        theStream.write((const char *)&aContentType, sizeof(uint32_t));

        myFileOffsets.totalLength += sizeof(uint64_t) + sizeof(uint32_t);

        rapidjson::OStreamWrapper aFileStream(theStream);
        myWriter.reset(new rapidjson::Writer<rapidjson::OStreamWrapper,
                                             rapidjson::UTF8<>,
                                             rapidjson::UTF8<>,
                                             rapidjson::CrtAllocator,
                                             rapidjson::kWriteDefaultFlags | rapidjson::kWriteNanAndInfFlag>(aFileStream));

        myWriter->StartObject();

        writeAsset();

        writeMesh(theProgressRange);

        writeBuffers(theStream);

        myWriter->EndObject();

        theStream.flush();
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinWriter: Stream '" << myFileName << "' cannot be written";
            return false;
        }

        myFileOffsets.jsonLength = static_cast<uint64_t>(theStream.tellp()) - myFileOffsets.jsonOffset - sizeof(uint64_t) - sizeof(uint32_t);
        myFileOffsets.totalLength += myFileOffsets.jsonLength;
        return true;
    }

    // ================================================================
    // Function : writeAsset
    // ================================================================
    void MeshBinWriter::writeAsset()
    {
        static constexpr int format_subversion = 0; // incremented on minor changes
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::ASSET));
        myWriter->StartObject();
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::GENERATOR));
        myWriter->String("MeshBinWriter");
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::SUBVERSION));
        myWriter->Int(format_subversion);

        writeCompressionInfo();

        myWriter->EndObject();
    }

    // ================================================================
    // Function : writeCompressionInfo
    // ================================================================
    void MeshBinWriter::writeCompressionInfo()
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::COMPRESSION));
        myWriter->StartObject();

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::COMPRESSION_CODEC));
        myWriter->String(MeshCompressionTypeConvertor::GetString(myCompressionType));

        if (myCompressionType == MeshCompressionType::Draco)
        {
            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::COMPRESSION_DRACO));
            myWriter->StartObject();

            myWriter->Key(MeshDracoCompressionInfo::NodesQuantBitsStr());
            myWriter->Int(myDracoCompressionInfo.NodesQuantBits());
            myWriter->Key(MeshDracoCompressionInfo::PropsQuantBitsStr());
            myWriter->Int(myDracoCompressionInfo.PropsQuantBits());

            myWriter->EndObject();
        }

        myWriter->EndObject();
    }

    // ================================================================
    // Function : writeMesh
    // ================================================================
    void MeshBinWriter::writeMesh(const Message_ProgressRange &theProgressRange)
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::MESH));
        myWriter->StartObject();

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::MESH_NAME));
        myWriter->String(myMeshData->Name().ToCString());

        if (!theProgressRange.UserBreak() && !CheckOption(MeshDataImportExportOptions::SKIP_GEOMETRY))
        {
            writeGeometry();
        }
        if (!theProgressRange.UserBreak() && !CheckOption(MeshDataImportExportOptions::SKIP_PROPERTIES))
        {
            writeProperties();
        }

        myWriter->EndObject();
    }

    // ================================================================
    // Function : writeGeometry
    // ================================================================
    void MeshBinWriter::writeGeometry()
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::GEOMETRY));
        myWriter->StartObject();

        writeBounds();

        writeBlocks();

        bool toSkipData = CheckOption(MeshDataImportExportOptions::SKIP_GEOMETRY_BINARY_DATA);

        if (myMeshData->NbNodes() > 0 && !toSkipData)
        {
            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::NODES));
            myWriter->Uint(myBufferData.Size());
            myBufferData.Append(BufferData(MeshDataBufferType::NODE));
        }

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::ELEMENTS));
        myWriter->StartArray();

        if (!toSkipData)
        {
            writeElements();
        }

        myWriter->EndArray();

        myWriter->EndObject();
    }

    // ================================================================
    // Function : writeElements
    // ================================================================
    void MeshBinWriter::writeElements()
    {
        auto addElement = [this](uint32_t theNbElems, MeshDataBufferType theType)
        {
            if (theNbElems == 0)
            {
                return;
            }
            myWriter->StartObject();
            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::ELEMENT_TYPE));
            myWriter->String(MeshDataBufferTypeConvertor::GetString(theType));
            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::ELEMENT_INDICES));
            myWriter->Uint(myBufferData.Size());
            myBufferData.Append(BufferData(theType));
            myWriter->EndObject();
        };

        // 0d
        addElement(myMeshData->NbElems(MeshElemType::Elem0d), MeshDataBufferType::ELEMENT_0D);

        // 2d
        addElement(myMeshData->NbElems(MeshElemType::Triangle), MeshDataBufferType::ELEMENT_TRIANGLE);
        addElement(myMeshData->NbElems(MeshElemType::Triangle2nd), MeshDataBufferType::ELEMENT_TRIANGLE_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Quad), MeshDataBufferType::ELEMENT_QUAD);
        addElement(myMeshData->NbElems(MeshElemType::Quad2nd), MeshDataBufferType::ELEMENT_QUAD_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Polygon), MeshDataBufferType::ELEMENT_POLYGON);

        // 3d
        addElement(myMeshData->NbElems(MeshElemType::Tetrahedron), MeshDataBufferType::ELEMENT_TETRAHEDRON);
        addElement(myMeshData->NbElems(MeshElemType::Tetrahedron2nd), MeshDataBufferType::ELEMENT_TETRAHEDRON_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Pyramid), MeshDataBufferType::ELEMENT_PYRAMID);
        addElement(myMeshData->NbElems(MeshElemType::Pyramid2nd), MeshDataBufferType::ELEMENT_PYRAMID_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Prism), MeshDataBufferType::ELEMENT_PRISM);
        addElement(myMeshData->NbElems(MeshElemType::Prism2nd), MeshDataBufferType::ELEMENT_PRISM_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Hexahedron), MeshDataBufferType::ELEMENT_HEXAHEDRON);
        addElement(myMeshData->NbElems(MeshElemType::Hexahedron2nd), MeshDataBufferType::ELEMENT_HEXAHEDRON_2ND);
        addElement(myMeshData->NbElems(MeshElemType::Polyhedron), MeshDataBufferType::ELEMENT_POLYHEDRON);

        // surface
        addElement(myMeshData->NbSurfElems(MeshElemType::Triangle), MeshDataBufferType::ELEMENT_SURFTRIANGLE);
        addElement(myMeshData->NbSurfElems(MeshElemType::Triangle2nd), MeshDataBufferType::ELEMENT_SURFTRIANGLE_2ND);
        addElement(myMeshData->NbSurfElems(MeshElemType::Quad), MeshDataBufferType::ELEMENT_SURFQUAD);
        addElement(myMeshData->NbSurfElems(MeshElemType::Quad2nd), MeshDataBufferType::ELEMENT_SURFQUAD_2ND);
        addElement(myMeshData->NbSurfElems(MeshElemType::Polygon), MeshDataBufferType::ELEMENT_SURFPOLYGON);
    }

    // ================================================================
    // Function : writeBounds
    // ================================================================
    void MeshBinWriter::writeBounds()
    {
        const Bnd_Box &anAABB = myMeshData->BoundingBox();

        if (!anAABB.IsVoid())
        {
            const gp_Pnt aCornerMax(anAABB.CornerMax());
            const gp_Pnt aCornerMin(anAABB.CornerMin());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BOUNDS_MAX));
            myWriter->StartArray();
            myWriter->Double(aCornerMax.X());
            myWriter->Double(aCornerMax.Y());
            myWriter->Double(aCornerMax.Z());
            myWriter->EndArray();

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BOUNDS_MIN));
            myWriter->StartArray();
            myWriter->Double(aCornerMin.X());
            myWriter->Double(aCornerMin.Y());
            myWriter->Double(aCornerMin.Z());
            myWriter->EndArray();
        }
    }

    // ================================================================
    // Function : writeBlocks
    // ================================================================
    void MeshBinWriter::writeBlocks()
    {
        std::function<void(const Handle(MeshDataBlock) &)> writeBlockNode = [&](const Handle(MeshDataBlock) & theNode)
        {
            myWriter->StartObject();

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BLOCK_NAME));
            myWriter->String(theNode->Name().ToCString());

            if (theNode->HasChildren())
            {
                myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BLOCK_CHILDREN));
                myWriter->StartArray();
                for (const Handle(MeshDataBlock) &aChild : theNode->Children())
                {
                    writeBlockNode(aChild);
                }
                myWriter->EndArray();
            }
            else
            {
                myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BLOCK_RANGE));
                myWriter->StartArray();
                myWriter->Int(theNode->Range().NodeLower);
                myWriter->Int(theNode->Range().NodeUpper);
                myWriter->Int(theNode->Range().ElemLower);
                myWriter->Int(theNode->Range().ElemUpper);
                myWriter->EndArray();
            }

            myWriter->EndObject();
        };

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BLOCKS));
        writeBlockNode(myMeshData->Blocks());
    }

    // ================================================================
    // Function : writeProperties
    // ================================================================
    void MeshBinWriter::writeProperties()
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTIES));
        myWriter->StartArray();

        for (int i = 0; i < myMeshData->Properties().Size(); ++i)
        {
            writeProperty(myMeshData->Properties()[i], i);
        }

        myWriter->EndArray();
    }

    // ================================================================
    // Function : writeProperty
    // ================================================================
    void MeshBinWriter::writeProperty(const Handle(MeshNVecProperty) & theProperty, int theIndex)
    {
        if (!shouldPropBeProcessed(theProperty))
        {
            return;
        }

        MeshDataBufferType aCompType = theProperty->HasNodeVecComponents() ? MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL
                                       : theProperty->HasGroupComponents() ? MeshDataBufferType::PROPERTY_GROUP
                                                                           : MeshDataBufferType::PROPERTY_SCALAR;

        myWriter->StartObject();

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_COMP_TYPE));
        myWriter->String(MeshDataBufferTypeConvertor::GetString(aCompType));

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_ID));
        myWriter->String(theProperty->Id().ToCString());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_NAME));
        myWriter->String(theProperty->Name().ToCString());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_DESCRIPTION));
        myWriter->String(theProperty->Description().ToCString());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_IS_ELEMENTAL));
        myWriter->Bool(theProperty->IsElemental());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_NB_COMPONENTS));
        myWriter->Uint(theProperty->NbComponents());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_TIMESTAMP));
        myWriter->Double(theProperty->Time());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_TIMESTEP));
        myWriter->Double(theProperty->TimeStep());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_TYPE));
        myWriter->Uint((int)theProperty->Type());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_TIME_DESCRIPTION));
        myWriter->String(theProperty->TimeStepDescription().ToCString());

        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_TIME_UNITS));
        myWriter->String(theProperty->TimeDisplayUnits().ToCString());

        writePropertyComponents(theProperty, theIndex, aCompType);

        myWriter->EndObject();
    }

    // ================================================================
    // Function : writePropertyComponents
    // ================================================================
    void MeshBinWriter::writePropertyComponents(const Handle(MeshNVecProperty) & theProperty,
                                                int theIndex,
                                                MeshDataBufferType theBufType)
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_COMPONENTS));
        myWriter->StartArray();

        const int aNbRawComponents = theProperty->NbRawComponents();
        const int aNbAuxComponents = aNbRawComponents - theProperty->NbComponents();
        const bool toCompressPropsUsingDraco = myCompressionType == MeshCompressionType::Draco && myDracoCompressionInfo.PropsQuantBits() != -1;

        for (int i = 0; i < aNbRawComponents; i++)
        {
            myWriter->StartObject();

            const Handle(MeshScalarPropertyBase) &aComp = theProperty->GetRawComponent(i);

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_ID));
            myWriter->String(aComp->Id().ToCString());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_NAME));
            myWriter->String(aComp->Name().ToCString());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_DESCRIPTION));
            myWriter->String(aComp->Description().ToCString());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_IS_ENABLED));
            myWriter->Bool(aComp->IsEnabled());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_ELEM_TYPE));
            // Draco compression works only with float values
            MeshPropertyElemType anElemType = !myToForceSinglePrecision && !toCompressPropsUsingDraco
                                                  ? aComp->ElemType()
                                                  : MeshPropertyElemType::Float;
            myWriter->String(MeshPropertyElemTypeConvertor::GetString(anElemType));

            writePropertyRange(aComp, anElemType);

            if (!aComp->DisplayUnits().IsEmpty())
            {
                myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_UNITS));
                myWriter->String(aComp->DisplayUnits().ToCString());
            }

            // We don't write the values of auto-generated components when compression is used,
            // they are recalculated by the reader after decoding.
            bool skipAuxComponent = (toCompressPropsUsingDraco && i < aNbAuxComponents);
            bool toWriteBinaryData = !CheckOption(MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA) && ((aComp->IsLoaded() && aComp->NbValues() > 0) || aComp->CanBeLoaded()) && !skipAuxComponent;
            if (toWriteBinaryData)
            {
                myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_VALUES));
                myWriter->Uint(myBufferData.Size());
                myBufferData.Append(BufferData(theBufType, theIndex, i));
            }

            if (theBufType == MeshDataBufferType::PROPERTY_GROUP)
            {
                writeGroupProperty(Handle(MeshGroupProperty)::DownCast(aComp));
            }

            myWriter->EndObject();
        }
        myWriter->EndArray();
    }

    // ================================================================
    // Function : writePropertyRange
    // ================================================================
    void MeshBinWriter::writePropertyRange(const Handle(MeshScalarPropertyBase) & theProp, MeshPropertyElemType theElemType)
    {
        double aValueMin = RealLast();
        double aValueMax = RealLast();
        if (theProp->ValueRange().GetBounds(aValueMin, aValueMax))
        {
            if (theElemType == MeshPropertyElemType::Float)
            {
                aValueMin = (float)aValueMin;
                aValueMax = (float)aValueMax;
            }
            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_MIN));
            myWriter->Double(aValueMin);

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_MAX));
            myWriter->Double(aValueMax);
        }
    }

    // ================================================================
    // Function : writeGroupProperty
    // ================================================================
    void MeshBinWriter::writeGroupProperty(const Handle(MeshGroupProperty) & theProperty)
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_GROUPS));
        myWriter->StartArray();
        for (int i = 0; i < MeshGroupProperty::MaxNbGroups; i++)
        {
            myWriter->StartObject();

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_GROUP_ID));
            myWriter->Uint(i);

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_GROUP_NAME));
            myWriter->String(theProperty->GetGroupName(i).ToCString());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::PROPERTY_GROUP_NB_ENTITIES));
            myWriter->Int(theProperty->GetGroupNbEntities(i));

            myWriter->EndObject();
        }
        myWriter->EndArray();
    }

    // ================================================================
    // Function : writeBuffers
    // ================================================================
    void MeshBinWriter::writeBuffers(std::ostream &theStream)
    {
        myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BUFFERS));
        myBufferArrayOffset = static_cast<uint64_t>(theStream.tellp()) + 1;
        writeBufferArray();
    }

    // ================================================================
    // Function : writeBufferArray
    // ================================================================
    void MeshBinWriter::writeBufferArray()
    {
        // Buffers length and offset are written in two steps.
        // On the first step when dumping main Json and offsets are not known,
        // values are written as fixed-size strings filled with '0' chars.
        // The second dump is performed after binary data was written
        // and offset was calculated.
        static constexpr int aNbDigits = 20;
        myWriter->StartArray();

        std::string aStr;

        uint64_t anOffset = 0;
        for (int i = 0; i < myBufferData.Size(); i++)
        {
            BufferData aBufferData = myBufferData[i];

            myWriter->StartObject();

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BUFFER_LENGTH));
            std::ostringstream aLengthStrStream;
            aLengthStrStream << std::setfill('0') << std::setw(aNbDigits) << aBufferData.length;
            aStr = aLengthStrStream.str();
            myWriter->String(aStr.c_str());

            myWriter->Key(MeshDataJsonElementConvertor::GetString(MeshDataJsonElement::BUFFER_OFFSET));
            std::ostringstream anOffsetStrStream;
            anOffsetStrStream << std::setfill('0') << std::setw(aNbDigits) << anOffset;
            aStr = anOffsetStrStream.str();
            myWriter->String(aStr.c_str());

            myWriter->EndObject();
            anOffset += aBufferData.length;
        }
        myWriter->EndArray();
    }

    // ================================================================
    // Function : writeBinData
    // ================================================================
    bool MeshBinWriter::writeBinData(std::ostream &theStream, const Message_ProgressRange &theProgressRange)
    {
        myFileOffsets.binDataOffset = static_cast<uint64_t>(theStream.tellp());
        uint64_t aBinDataOffset = 0;
        theStream.write((const char *)&aBinDataOffset, sizeof(uint64_t));
        uint32_t aContentType = 0x004E4942; // "BIN"
        theStream.write((const char *)&aContentType, sizeof(uint32_t));

        myFileOffsets.totalLength += sizeof(uint64_t) + sizeof(uint32_t);

        if (theProgressRange.UserBreak() || !writeBinDataBuffers(theStream, theProgressRange))
        {
            Message::SendFail() << "MeshBinWriter: Cannot write bin data";
            return false;
        }

        theStream.flush();
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinWriter: Stream '" << myFileName << "' cannot be written";
            return false;
        }

        myFileOffsets.binDataLength = static_cast<uint64_t>(theStream.tellp()) - myFileOffsets.binDataOffset - sizeof(uint64_t) - sizeof(uint32_t);
        myFileOffsets.totalLength += myFileOffsets.binDataLength;
        return true;
    }

    // ================================================================
    // Function : writeBinDataBuffers
    // ================================================================
    bool MeshBinWriter::writeBinDataBuffers(std::ostream &theStream, const Message_ProgressRange &theProgressRange)
    {
        for (int aBuffIter = 0; aBuffIter < myBufferData.Size() && !theProgressRange.UserBreak(); ++aBuffIter)
        {
            uint64_t aLength = writeBinDataBuffer(theStream, aBuffIter, theProgressRange);
            if (aLength == 0 || theProgressRange.UserBreak())
            {
                return false;
            }

            myBufferData[aBuffIter].length = aLength;
        }

        return true;
    }

    // ================================================================
    // Function : writeBinDataBuffer
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataBuffer(std::ostream &theStream, int theBuffIdx, const Message_ProgressRange &thePR)
    {
        auto writeElems = [this, &theStream, &theData = myMeshData, &thePR](MeshElemType theType)
        {
            return writeMeshElements(theStream, theData->Elements(theType), thePR);
        };
        auto writeSurfElems = [this, &theStream, &theData = myMeshData, &thePR](MeshElemType theType)
        {
            return writeMeshElements(theStream, theData->SurfaceElements(theType), thePR);
        };

        const MeshDataBufferType aType = myBufferData[theBuffIdx].type;
        const int anIndex = myBufferData[theBuffIdx].index;
        const int aSubIndex = myBufferData[theBuffIdx].subIndex;

        switch (aType)
        {
        case MeshDataBufferType::NODE:
            return writeNodes(theStream);
        case MeshDataBufferType::ELEMENT_0D:
            return writeElems(MeshElemType::Elem0d);
        case MeshDataBufferType::ELEMENT_TRIANGLE:
            return writeElems(MeshElemType::Triangle);
        case MeshDataBufferType::ELEMENT_TRIANGLE_2ND:
            return writeElems(MeshElemType::Triangle2nd);
        case MeshDataBufferType::ELEMENT_QUAD:
            return writeElems(MeshElemType::Quad);
        case MeshDataBufferType::ELEMENT_QUAD_2ND:
            return writeElems(MeshElemType::Quad2nd);
        case MeshDataBufferType::ELEMENT_POLYGON:
            return writeElems(MeshElemType::Polygon);
        case MeshDataBufferType::ELEMENT_TETRAHEDRON:
            return writeElems(MeshElemType::Tetrahedron);
        case MeshDataBufferType::ELEMENT_TETRAHEDRON_2ND:
            return writeElems(MeshElemType::Tetrahedron2nd);
        case MeshDataBufferType::ELEMENT_PYRAMID:
            return writeElems(MeshElemType::Pyramid);
        case MeshDataBufferType::ELEMENT_PYRAMID_2ND:
            return writeElems(MeshElemType::Pyramid2nd);
        case MeshDataBufferType::ELEMENT_PRISM:
            return writeElems(MeshElemType::Prism);
        case MeshDataBufferType::ELEMENT_PRISM_2ND:
            return writeElems(MeshElemType::Prism2nd);
        case MeshDataBufferType::ELEMENT_HEXAHEDRON:
            return writeElems(MeshElemType::Hexahedron);
        case MeshDataBufferType::ELEMENT_HEXAHEDRON_2ND:
            return writeElems(MeshElemType::Hexahedron2nd);
        case MeshDataBufferType::ELEMENT_POLYHEDRON:
            return writeElems(MeshElemType::Polyhedron);
        case MeshDataBufferType::ELEMENT_SURFTRIANGLE:
            return writeSurfElems(MeshElemType::Triangle);
        case MeshDataBufferType::ELEMENT_SURFTRIANGLE_2ND:
            return writeSurfElems(MeshElemType::Triangle2nd);
        case MeshDataBufferType::ELEMENT_SURFQUAD:
            return writeSurfElems(MeshElemType::Quad);
        case MeshDataBufferType::ELEMENT_SURFQUAD_2ND:
            return writeSurfElems(MeshElemType::Quad2nd);
        case MeshDataBufferType::ELEMENT_SURFPOLYGON:
            return writeSurfElems(MeshElemType::Polygon);
        case MeshDataBufferType::PROPERTY_SCALAR:
            // fallthrough
        case MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL:
            // fallthrough
        case MeshDataBufferType::PROPERTY_GROUP:
            return writeBinDataProperty(theStream, anIndex, aSubIndex, aType);
        case MeshDataBufferType::ELEMENT_1D:
            // not implemented
            // fallthrough
        case MeshDataBufferType::ELEMENT_INDEXED:
            // indexed elements are no longer written and should be computed during reading
            // fallthrough
        default:
            return 0;
        }
    }

    // ================================================================
    // Function : writeNodes
    // ================================================================
    uint64_t MeshBinWriter::writeNodes(std::ostream &theStream)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return writeNodesUncompressed(theStream);
        case MeshCompressionType::Draco:
            return writeNodesDraco(theStream);
        default:
            return 0;
        }
    }

    // ================================================================
    // Function : writeNodesUncompressed
    // ================================================================
    uint64_t MeshBinWriter::writeNodesUncompressed(std::ostream &theStream)
    {
        uint64_t aLength = 0;
        for (MeshEntityId i = 0; i < myMeshData->NbNodes(); i++)
        {
            const Graphic3d_Vec3 &aNode = myMeshData->Nodes()[i];
            theStream.write((const char *)aNode.GetData(), sizeof(aNode));
            aLength += sizeof(aNode);
        }
        return aLength;
    }

    // ================================================================
    // Function : writeNodesDraco
    // ================================================================
    uint64_t MeshBinWriter::writeNodesDraco(std::ostream &theStream)
    {
        if (myDracoCompressionInfo.NodesQuantBits() == -1)
        {
            return writeNodesUncompressed(theStream);
        }

        int aNbParts = myDracoCompressionInfo.NbParts(myMeshData->Nodes().Size());
        int aPartSize = (int)Ceiling((double)myMeshData->Nodes().Size() / aNbParts);

        std::vector<MeshEncodedPart<1>> anEncodedParts(aNbParts);

        int aQuantBits = myDracoCompressionInfo.NodesQuantBits();

        auto processPart = [&aMesh = myMeshData, &anEncodedParts, aPartSize, aQuantBits](int, int theIdx)
        {
            int aFirstNodeId = theIdx * aPartSize;
            int aLastNodeId = Min(aFirstNodeId + aPartSize, aMesh->Nodes().Size()) - 1;
            int aNbNodes = aLastNodeId - aFirstNodeId + 1;

            MeshAttributeEncoder &anEncoder = anEncodedParts[theIdx].encoders[0];
            anEncoder.Init(aNbNodes);

            static constexpr int aNbNodeComponents = 3;
            anEncoder.AddAttributeFloat32(aNbNodeComponents);
            Bnd_Box aBox;
            for (int i = 0; i < aNbNodes; i++)
            {
                const Graphic3d_Vec3 &aNode = aMesh->Nodes()[i + aFirstNodeId];
                anEncoder.SetValue(0, i, aNode.GetData());
                aBox.Add(gp_Pnt(aNode.x(), aNode.y(), aNode.z()));
            }

            std::vector<double> aMin = {aBox.CornerMin().X() - ShortRealEpsilon(),
                                        aBox.CornerMin().Y() - ShortRealEpsilon(),
                                        aBox.CornerMin().Z() - ShortRealEpsilon()};
            std::vector<double> aMax = {aBox.CornerMax().X() + ShortRealEpsilon(),
                                        aBox.CornerMax().Y() + ShortRealEpsilon(),
                                        aBox.CornerMax().Z() + ShortRealEpsilon()};

            anEncoder.SetQuantOptions(0, aQuantBits, aMin, aMax);

            anEncodedParts[theIdx].isOk = anEncoder.Encode();
        };

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        return ProcessEncodedParts(anEncodedParts, isParallel, processPart, theStream);
    }

    // ================================================================
    // Function : writeMeshElements
    // ================================================================
    uint64_t MeshBinWriter::writeMeshElements(std::ostream &theStream,
                                              const MeshElements &theElems,
                                              const Message_ProgressRange &theProgressRange)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return writeMeshElementsUncompressed(theStream, theElems, theProgressRange);
        case MeshCompressionType::Draco:
            return writeMeshElementsDraco(theStream, theElems, theProgressRange);
        default:
            return 0;
        }
    }

    // ================================================================
    // Function : writeMeshElementsVectorUncompressed
    // ================================================================
    template <class Elem_T>
    uint64_t MeshBinWriter::writeMeshElementsVectorUncompressed(std::ostream &theStream,
                                                                const NCollection_Vector<Elem_T> &theElems,
                                                                const Message_ProgressRange &theProgressRange)
    {
        uint64_t aLength = 0;
        if (theProgressRange.UserBreak())
        {
            return aLength;
        }
        uint64_t aSize = theElems.Size();
        theStream.write((const char *)&aSize, sizeof(aSize));
        aLength += sizeof(aSize);
        if (aSize > 0)
        {
            for (Elem_T aValue : theElems)
            {
                theStream.write((const char *)&aValue, sizeof(aValue));
            }
            aLength += aSize * sizeof(Elem_T);
        }

        return aLength;
    }

    // ================================================================
    // Function : writeMeshElementsUncompressed
    // ================================================================
    uint64_t MeshBinWriter::writeMeshElementsUncompressed(std::ostream &theStream,
                                                          const MeshElements &theElems,
                                                          const Message_ProgressRange &theProgressRange)
    {
        uint64_t aLength = 0;

        aLength += writeMeshElementsVectorUncompressed(theStream, theElems.NodesOffsets(), theProgressRange);
        aLength += writeMeshElementsVectorUncompressed(theStream, theElems.Nodes(), theProgressRange);
        aLength += writeMeshElementsVectorUncompressed(theStream, theElems.FacesOffsets(), theProgressRange);
        aLength += writeMeshElementsVectorUncompressed(theStream, theElems.Faces(), theProgressRange);
        aLength += writeMeshElementsVectorUncompressed(theStream, theElems.GlobalIndices(), theProgressRange);

        return aLength;
    }

    //! Init draco encoders for mesh elements.
    void InitMeshElementsDracoEncoders(std::vector<RWCae::MeshAttributeEncoder> &theEncoders,
                                       const MeshElements &theElems,
                                       uint32_t theFirstElemId,
                                       uint32_t theNbElems)
    {
        MeshAttributeEncoder &anEncoderFirstOffsets = theEncoders[0];
        anEncoderFirstOffsets.Init(1);
        if (theElems.ToStoreNodesOffsets())
        {
            anEncoderFirstOffsets.AddAttributeUInt64(1);
        }
        if (theElems.ToStoreFaces())
        {
            anEncoderFirstOffsets.AddAttributeUInt64(1);
        }

        MeshAttributeEncoder &anEncoderOffsets = theEncoders[1];
        anEncoderOffsets.Init(theNbElems - 1);
        if (theElems.ToStoreNodesOffsets())
        {
            anEncoderOffsets.AddAttributeUInt8(1);
        }
        if (theElems.ToStoreFaces())
        {
            anEncoderOffsets.AddAttributeUInt16(1);
        }

        MeshElemEntityOffset aNbNodes = theNbElems * theElems.FixedElemSize();
        if (theElems.ToStoreNodesOffsets())
        {
            aNbNodes = theElems.NodesOffsets()[theFirstElemId + theNbElems - 1] - (theFirstElemId > 0 ? theElems.NodesOffsets()[theFirstElemId - 1] : 0);
        }
        MeshAttributeEncoder &anEncoderNodes = theEncoders[2];
        anEncoderNodes.Init(static_cast<int>(aNbNodes));
        anEncoderNodes.AddAttributeUInt32(1);

        MeshAttributeEncoder &anEncoderFaces = theEncoders[3];
        if (theElems.ToStoreFaces())
        {
            MeshElemEntityOffset aFirstFace = theFirstElemId > 0 ? theElems.FacesOffsets()[theFirstElemId - 1] : 0;
            MeshElemEntityOffset aNbFaces = theElems.FacesOffsets()[theFirstElemId + theNbElems - 1] - aFirstFace;
            anEncoderFaces.Init(static_cast<int>(aNbFaces));
            anEncoderFaces.AddAttributeUInt8(1);
        }

        MeshAttributeEncoder &anEncoderGI = theEncoders[4];
        anEncoderGI.Init(theNbElems);
        anEncoderGI.AddAttributeUInt32(1);
    }

    //! Fill draco encoders for mesh element.
    void EncodeMeshElementDraco(std::vector<RWCae::MeshAttributeEncoder> &theEncoders,
                                const MeshElements &theElems,
                                uint32_t theFirstElemId,
                                uint32_t theElemIter,
                                uint64_t &thePrevNodesOffset,
                                uint32_t &theLastFirstNode,
                                MeshElemEntityOffset &theNodesIter,
                                uint64_t &thePrevFacesOffset,
                                MeshElemEntityOffset &theFacesIter)
    {
        RWCae::MeshAttributeEncoder &anEncoderFirstOffsets = theEncoders[0];
        RWCae::MeshAttributeEncoder &anEncoderOffsets = theEncoders[1];
        RWCae::MeshAttributeEncoder &anEncoderNodes = theEncoders[2];
        RWCae::MeshAttributeEncoder &anEncoderFaces = theEncoders[3];

        uint64_t aNodesOffset = 0;
        if (theElems.ToStoreNodesOffsets())
        {
            aNodesOffset = theElems.NodesOffsets()[theElemIter + theFirstElemId];
            if (theElemIter == 0)
            {
                anEncoderFirstOffsets.SetValue(0, 0, &aNodesOffset);
            }
            else
            {
                uint8_t aNodesOffsetDiff = static_cast<uint8_t>(aNodesOffset - thePrevNodesOffset);
                anEncoderOffsets.SetValue(0, theElemIter - 1, &aNodesOffsetDiff);
            }
        }
        else
        {
            aNodesOffset = thePrevNodesOffset + theElems.FixedElemSize();
        }

        uint32_t aFirstNode = theElems.Nodes()[static_cast<int>(thePrevNodesOffset)];
        int32_t aDiff = aFirstNode - theLastFirstNode;
        theLastFirstNode = aFirstNode;
        uint32_t anEncodedNodeValue = static_cast<uint32_t>((abs(aDiff) << 1) | (aDiff < 0 ? 1 : 0));
        anEncoderNodes.SetValue(0, static_cast<int>(theNodesIter++), &anEncodedNodeValue);
        for (uint64_t aNodeIter = thePrevNodesOffset + 1; aNodeIter < aNodesOffset; aNodeIter++)
        {
            uint32_t aNode = theElems.Nodes()[static_cast<int>(aNodeIter)];
            aDiff = aNode - aFirstNode;
            aFirstNode = aNode;
            anEncodedNodeValue = static_cast<uint32_t>((abs(aDiff) << 1) | (aDiff < 0 ? 1 : 0));
            anEncoderNodes.SetValue(0, static_cast<int>(theNodesIter++), &anEncodedNodeValue);
        }

        thePrevNodesOffset = aNodesOffset;

        if (theElems.ToStoreFaces())
        {
            uint64_t aFacesOffset = theElems.FacesOffsets()[theElemIter + theFirstElemId];
            if (theElemIter == 0)
            {
                anEncoderFirstOffsets.SetValue(1, 0, &aFacesOffset);
            }
            else
            {
                uint16_t aFacesOffsetDiff = static_cast<uint16_t>(aFacesOffset - thePrevFacesOffset);
                anEncoderOffsets.SetValue(1, theElemIter - 1, &aFacesOffsetDiff);
            }

            for (uint64_t aFaceIter = thePrevFacesOffset; aFaceIter < aFacesOffset; aFaceIter++)
            {
                uint8_t aFaceValue = static_cast<uint8_t>(theElems.Faces()[static_cast<int>(aFaceIter)]);
                anEncoderFaces.SetValue(0, static_cast<int>(theFacesIter++), &aFaceValue);
            }

            thePrevFacesOffset = aFacesOffset;
        }
    }

    // ================================================================
    // Function : writeMeshElementsDraco
    // ================================================================
    uint64_t MeshBinWriter::writeMeshElementsDraco(std::ostream &theStream,
                                                   const MeshElements &theElems,
                                                   const Message_ProgressRange &theProgressRange)
    {
        int aNbParts = myDracoCompressionInfo.NbParts(theElems.NbElements());
        uint32_t aPartSize = (uint32_t)Ceiling((double)theElems.NbElements() / aNbParts);

        // 1 - for first nodes offset (uint64_t), first faces offset (uint64_t)
        // 2 - for nodes offsets (uint8_t), faces offsets (uint8_t)
        // 3 - for nodes (uint32_t)
        // 4 - for faces (uint8_t)
        // 5 - for global indices (uint32_t)
        std::vector<MeshEncodedPart<5>> anEncodedParts(aNbParts);

        auto processPart = [&theElems, &anEncodedParts, aPartSize, &theProgressRange](int, int theIdx)
        {
            uint32_t aFirstElemId = theIdx * aPartSize;
            uint32_t aNbElems = std::min(aFirstElemId + aPartSize, theElems.NbElements()) - aFirstElemId;

            std::vector<RWCae::MeshAttributeEncoder> &anEncoders = anEncodedParts[theIdx].encoders;
            InitMeshElementsDracoEncoders(anEncoders, theElems, aFirstElemId, aNbElems);

            uint64_t aPrevNodesOffset = theElems.ToStoreNodesOffsets()
                                            ? (aFirstElemId > 0 ? theElems.NodesOffsets()[aFirstElemId - 1] : 0)
                                            : (aFirstElemId * theElems.FixedElemSize());
            uint64_t aPrevFacesOffset = theElems.ToStoreFaces()
                                            ? (aFirstElemId > 0 ? theElems.FacesOffsets()[aFirstElemId - 1] : 0)
                                            : 0;
            uint32_t aLastFirstNode = 0;
            int32_t aLastGlobalIndex = 0;

            MeshElemEntityOffset aNodesIter = 0;
            MeshElemEntityOffset aFacesIter = 0;
            for (uint32_t anElemIter = 0; anElemIter < aNbElems; ++anElemIter)
            {
                EncodeMeshElementDraco(anEncoders,
                                       theElems,
                                       aFirstElemId,
                                       anElemIter,
                                       aPrevNodesOffset,
                                       aLastFirstNode,
                                       aNodesIter,
                                       aPrevFacesOffset,
                                       aFacesIter);

                int32_t aGlobalIndex = theElems.GlobalIndex(anElemIter + aFirstElemId);
                int32_t aDiff = aGlobalIndex - aLastGlobalIndex;
                aLastGlobalIndex = aGlobalIndex;
                uint32_t anEncodedGlobalIndex = static_cast<uint32_t>((abs(aDiff) << 1) | (aDiff < 0 ? 1 : 0));
                anEncoders[4].SetValue(0, anElemIter, &anEncodedGlobalIndex);
            }

            anEncodedParts[theIdx].isOk = !theProgressRange.UserBreak() && anEncoders[0].Encode();
            anEncodedParts[theIdx].isOk = anEncodedParts[theIdx].isOk && !theProgressRange.UserBreak() && anEncoders[1].Encode();
            anEncodedParts[theIdx].isOk = anEncodedParts[theIdx].isOk && !theProgressRange.UserBreak() && anEncoders[2].Encode();
            anEncodedParts[theIdx].isOk = anEncodedParts[theIdx].isOk && !theProgressRange.UserBreak() && anEncoders[3].Encode();
            anEncodedParts[theIdx].isOk = anEncodedParts[theIdx].isOk && !theProgressRange.UserBreak() && anEncoders[4].Encode();
        };

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        return ProcessEncodedParts(anEncodedParts, isParallel, processPart, theStream);
    }

    // ================================================================
    // Function : encodeElementNode
    // ================================================================
    void MeshBinWriter::encodeElementNode(int32_t theIndex, uint32_t &theEncodedIndex, int32_t &theLastIndex) const
    {
        theEncodedIndex = 0;
        if (theIndex < 0)
        {
            return;
        }
        int32_t aDiff = theIndex - theLastIndex;
        theLastIndex = theIndex;
        theEncodedIndex = static_cast<uint32_t>((abs(aDiff) << 1) | (aDiff < 0 ? 1 : 0)) + 1;
    }

    // ================================================================
    // Function : writeBinDataProperty
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataProperty(std::ostream &theStream,
                                                 int thePropIndex,
                                                 int theCompIndex,
                                                 MeshDataBufferType theType)
    {
        uint64_t aRes = 0;

        const Handle(MeshNVecProperty) &aProperty = myMeshData->Properties()[thePropIndex];

        if (!aProperty->IsLoaded() && aProperty->CanBeLoaded())
        {
            if (!MeshTools::LoadMeshProperty(myMeshData, aProperty))
            {
                return aRes;
            }

            myPropsToUnload.Add(thePropIndex);
        }

        if (theType == MeshDataBufferType::PROPERTY_SCALAR)
        {
            aRes = writeBinDataScalarProperty(theStream, thePropIndex, theCompIndex);
        }
        else if (theType == MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL)
        {
            aRes = writeBinDataNodalElementalProperty(theStream, thePropIndex, theCompIndex);
        }
        else if (theType == MeshDataBufferType::PROPERTY_GROUP)
        {
            aRes = writeBinDataGroupProperty(theStream, thePropIndex, theCompIndex);
        }

        if (theCompIndex == aProperty->NbRawComponents() - 1 && myPropsToUnload.Remove(thePropIndex))
        {
            aProperty->Unload();
        }

        return aRes;
    }

    // ================================================================
    // Function : writeBinDataScalarProperty
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataScalarProperty(std::ostream &theStream, int thePropIndex, int theCompIndex)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return writeBinDataScalarPropertyUncompressed(theStream, thePropIndex, theCompIndex);
        case MeshCompressionType::Draco:
            return writeBinDataScalarPropertyDraco(theStream, thePropIndex, theCompIndex);
        default:
            return 0;
        }
    }

    // ================================================================
    // Function : writeBinDataScalarPropertyUncompressed
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataScalarPropertyUncompressed(std::ostream &theStream,
                                                                   int thePropIndex,
                                                                   int theCompIndex)
    {
        uint64_t aLength = 0;

        Handle(MeshScalarProperty) aProperty = Handle(MeshScalarProperty)::DownCast(
            myMeshData->Properties()[thePropIndex]->GetRawComponent(theCompIndex));

        size_t anElemSize = aProperty->ElemSize();

        for (int i = 0; i < aProperty->NbValues(); ++i)
        {
            if (myToForceSinglePrecision)
            {
                const float aValue = (float)aProperty->Value(MeshPropertyEntityInfo(i));
                theStream.write((const char *)&aValue, sizeof(aValue));
                aLength += sizeof(aValue);
            }
            else
            {
                theStream.write(aProperty->RawValue(MeshPropertyEntityInfo(i)), anElemSize);
                aLength += anElemSize;
            }
        }

        return aLength;
    }

    // ================================================================
    // Function : writeBinDataScalarPropertyDraco
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataScalarPropertyDraco(std::ostream &theStream, int thePropIndex, int theCompIndex)
    {
        if (myDracoCompressionInfo.PropsQuantBits() == -1)
        {
            return writeBinDataScalarPropertyUncompressed(theStream, thePropIndex, theCompIndex);
        }

        Handle(MeshScalarProperty) aProperty = Handle(MeshScalarProperty)::DownCast(
            myMeshData->Properties()[thePropIndex]->GetRawComponent(theCompIndex));

        int aNbParts = myDracoCompressionInfo.NbParts(aProperty->NbValues());
        int aPartSize = (int)Ceiling((double)aProperty->NbValues() / aNbParts);

        std::vector<MeshEncodedPart<1>> anEncodedParts(aNbParts);

        int aQuantBits = myDracoCompressionInfo.PropsQuantBits();

        auto processPart = [&aProperty, &anEncodedParts, aPartSize, aQuantBits](int, int theIdx)
        {
            int aFirstValueId = theIdx * aPartSize;
            int aLastValueId = Min(aFirstValueId + aPartSize, aProperty->NbValues()) - 1;
            int aNbValues = aLastValueId - aFirstValueId + 1;

            MeshAttributeEncoder &anEncoder = anEncodedParts[theIdx].encoders[0];
            anEncoder.Init(aNbValues);
            anEncoder.AddAttributeFloat32(1);
            for (int i = 0; i < aNbValues; i++)
            {
                const float aValue = (float)aProperty->Value(MeshPropertyEntityInfo(i + aFirstValueId));
                anEncoder.SetValue(0, i, &aValue);
            }

            std::vector<double> aMin = {0.0};
            std::vector<double> aMax = {0.0};
            aProperty->ValueRange().GetBounds(aMin[0], aMax[0]);
            anEncoder.SetQuantOptions(0, aQuantBits, aMin, aMax);

            anEncodedParts[theIdx].isOk = anEncoder.Encode();
        };

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        return ProcessEncodedParts(anEncodedParts, isParallel, processPart, theStream);
    }

    // ================================================================
    // Function : writeBinDataNodalElementalProperty
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataNodalElementalProperty(std::ostream &theStream, int thePropIndex, int theCompIndex)
    {
        switch (myCompressionType)
        {
        case MeshCompressionType::None:
            return writeBinDataNodalElementalPropertyUncompressed(theStream, thePropIndex, theCompIndex);
        case MeshCompressionType::Draco:
            return writeBinDataNodalElementalPropertyDraco(theStream, thePropIndex, theCompIndex);
        default:
            return 0;
        }
    }

    // ================================================================
    // Function : writeBinDataNodalElementalPropertyUncompressed
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataNodalElementalPropertyUncompressed(std::ostream &theStream,
                                                                           int thePropIndex,
                                                                           int theCompIndex)
    {
        uint64_t aLength = 0;

        const Handle(MeshNodeVecProperty) aProperty = Handle(MeshNodeVecProperty)::DownCast(
            myMeshData->Properties()[thePropIndex]->GetRawComponent(theCompIndex));

        std::vector<int> anIndices;

        size_t anElemSize = aProperty->ElemSize();

        const uint32_t aNbValues = static_cast<uint32_t>(aProperty->NbValues());
        theStream.write((const char *)&aNbValues, sizeof(uint32_t));
        aLength += sizeof(uint32_t);
        for (int i = 0; i < aProperty->NbValues(); ++i)
        {
            aProperty->SubIndices(i, anIndices);

            const uint32_t aValuesSize = static_cast<uint32_t>(anIndices.size());
            theStream.write((const char *)&aValuesSize, sizeof(int32_t));
            aLength += sizeof(int32_t);

            for (int anIndex : anIndices)
            {
                const int32_t aNodeIndex = static_cast<int32_t>(anIndex);
                theStream.write((const char *)&aNodeIndex, sizeof(int32_t));
                aLength += sizeof(int32_t);

                if (myToForceSinglePrecision)
                {
                    const float aNodeValue = (float)aProperty->Value(MeshPropertyEntityInfo(i, anIndex));
                    theStream.write((const char *)&aNodeValue, sizeof(aNodeValue));
                    aLength += sizeof(aNodeValue);
                }
                else
                {
                    theStream.write(aProperty->RawValue(MeshPropertyEntityInfo(i, anIndex)), anElemSize);
                    aLength += anElemSize;
                }
            }
        }

        return aLength;
    }

    // ================================================================
    // Function : writeBinDataNodalElementalPropertyDraco
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataNodalElementalPropertyDraco(std::ostream &theStream,
                                                                    int thePropIndex,
                                                                    int theCompIndex)
    {
        const Handle(MeshNodeVecProperty) aProperty = Handle(MeshNodeVecProperty)::DownCast(
            myMeshData->Properties()[thePropIndex]->GetRawComponent(theCompIndex));

        int aNbParts = myDracoCompressionInfo.NbParts(aProperty->NbValues());
        int aPartSize = (int)Ceiling((double)aProperty->NbValues() / aNbParts);

        constexpr int aNbEncoders = 2;
        std::vector<MeshEncodedPart<aNbEncoders>> anEncodedParts(aNbParts);

        int aQuantBits = myDracoCompressionInfo.PropsQuantBits();
        bool isSingle = myToForceSinglePrecision;

        auto processPart = [&aProperty, &anEncodedParts, aPartSize, aQuantBits, isSingle](int, int theIdx)
        {
            int aFirstValueId = theIdx * aPartSize;
            int aNbValues = Min(aFirstValueId + aPartSize, aProperty->NbValues()) - aFirstValueId;

            uint32_t aTotalNbValues = 0;

            MeshAttributeEncoder &anEncoderSizes = anEncodedParts[theIdx].encoders[0];
            anEncoderSizes.Init(aNbValues);
            anEncoderSizes.AddAttributeUInt32(1);
            for (int i = 0; i < aNbValues; i++)
            {
                const uint32_t aValuesSize = static_cast<uint32_t>(aProperty->NbSubIndices(i + aFirstValueId));
                aTotalNbValues += aValuesSize;
                anEncoderSizes.SetValue(0, i, &aValuesSize);
            }

            if (!anEncoderSizes.Encode())
            {
                return;
            }

            MeshAttributeEncoder &anEncoderValues = anEncodedParts[theIdx].encoders[1];
            anEncoderValues.Init(aTotalNbValues);

            bool toUseSinglePrecision = aQuantBits != -1 || isSingle;

            anEncoderValues.AddAttributeInt32(1);
            toUseSinglePrecision ? anEncoderValues.AddAttributeFloat32(1) : anEncoderValues.AddAttributeFloat64(1);

            uint32_t anEncodedIter = 0;

            std::vector<int> anIndices;

            for (int i = 0; i < aNbValues; i++)
            {
                aProperty->SubIndices(i + aFirstValueId, anIndices);
                for (int anIndex : anIndices)
                {
                    const int32_t aNodeIndex = static_cast<int32_t>(anIndex);
                    anEncoderValues.SetValue(0, anEncodedIter, &aNodeIndex);

                    const double aValueD = aProperty->Value(MeshPropertyEntityInfo(i + aFirstValueId, anIndex));
                    const float aValueF = (float)aValueD;
                    anEncoderValues.SetValue(1, anEncodedIter++, toUseSinglePrecision ? (void *)&aValueF : (void *)&aValueD);
                }
            }

            std::vector<double> aMin = {0.0};
            std::vector<double> aMax = {0.0};
            aProperty->ValueRange().GetBounds(aMin[0], aMax[0]);
            anEncoderValues.SetQuantOptions(1, aQuantBits, aMin, aMax);

            anEncodedParts[theIdx].isOk = anEncoderValues.Encode();
        };

        bool isParallel = CheckOption(MeshDataImportExportOptions::PARALLEL_PROCESSING);
        return ProcessEncodedParts(anEncodedParts, isParallel, processPart, theStream);
    }

    // ================================================================
    // Function : writeBinDataGroupProperty
    // ================================================================
    uint64_t MeshBinWriter::writeBinDataGroupProperty(std::ostream &theStream, int thePropIndex, int theCompIndex)
    {
        const Handle(MeshGroupProperty) aProperty = Handle(MeshGroupProperty)::DownCast(
            myMeshData->Properties()[thePropIndex]->GetRawComponent(theCompIndex));

        uint64_t aLength = 0;
        for (int i = 0; i < aProperty->NbValues(); ++i)
        {
            const MeshGroupProperty::MeshGroupsMaskType aValue = aProperty->GetGroups(i);
            theStream.write((const char *)&aValue, sizeof(MeshGroupProperty::MeshGroupsMaskType));
            aLength += sizeof(MeshGroupProperty::MeshGroupsMaskType);
        }
        return aLength;
    }

    // ================================================================
    // Function : updateSizes
    // ================================================================
    bool MeshBinWriter::updateSizes(std::ostream &theStream)
    {
        theStream.seekp(myBufferArrayOffset, std::ios::beg);
        rapidjson::OStreamWrapper aFileStream(theStream);
        myWriter.reset(new rapidjson::Writer<rapidjson::OStreamWrapper,
                                             rapidjson::UTF8<>,
                                             rapidjson::UTF8<>,
                                             rapidjson::CrtAllocator,
                                             rapidjson::kWriteDefaultFlags | rapidjson::kWriteNanAndInfFlag>(aFileStream));
        writeBufferArray();

        theStream.seekp(myFileOffsets.totalOffset, std::ios::beg);
        theStream.write((const char *)&myFileOffsets.totalLength, sizeof(uint64_t));
        theStream.seekp(myFileOffsets.jsonOffset, std::ios::beg);
        theStream.write((const char *)&myFileOffsets.jsonLength, sizeof(uint64_t));
        theStream.seekp(myFileOffsets.binDataOffset, std::ios::beg);
        theStream.write((const char *)&myFileOffsets.binDataLength, sizeof(uint64_t));

        theStream.flush();
        if (!theStream.good())
        {
            Message::SendFail() << "MeshBinWriter: Stream '" << myFileName << "' cannot be written";
            return false;
        }

        return true;
    }

} // RWCae
