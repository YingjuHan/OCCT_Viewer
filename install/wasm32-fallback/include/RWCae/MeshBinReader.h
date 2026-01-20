#ifndef _MESHBINREADER_HEADERFILE
#define _MESHBINREADER_HEADERFILE

#include <RWBase/JsonHelper.h>

#include <RWCae/MeshBinIOBase.h>
#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshScalarProperty.h>

#include <NCollection_Map.hxx>

#include <functional>
#include <memory>

namespace RWCae
{

    //! Reader for CAE mesh @link RWCae::MeshData @endlink binary storage format.
    class MeshBinReader : public MeshBinIOBase
    {
    public:
        typedef rapidjson::Document::ValueType JsonValue;

    public:
        //! Main constructor.
        Standard_EXPORT MeshBinReader();

        //! Main constructor.
        Standard_EXPORT explicit MeshBinReader(const Handle(MeshData) & theMeshData);

        //! Destructor.
        Standard_EXPORT virtual ~MeshBinReader();

        //! Perform reading from the file.
        Standard_EXPORT bool Perform(const TCollection_AsciiString &theFileName);

        //! Perform reading from the stream.
        Standard_EXPORT bool Perform(std::istream &theStream, const TCollection_AsciiString &theFileName);

        //! Return asset generator name.
        const TCollection_AsciiString &GetGenerator() const { return myGenerator; }

        //! Return asset format subversion.
        int GetFormatSubversion() const { return myFormatSubversion; }

        //! Return map of updated properties.
        const NCollection_Map<Handle(RWCae::MeshNVecProperty)> &UpdatedProps() const { return myUpdatedProps; }

    protected:
        //! Read offsets of Json/Bin parts.
        Standard_EXPORT void readDataOffsets(std::istream &theStream);

        //! Read Json part.
        Standard_EXPORT bool readJson(std::istream &theStream);

        //! Read stored asset info from Json part.
        Standard_EXPORT bool readAsset(const JsonValue *theValue);

        //! Read stored compression info from Json part.
        Standard_EXPORT bool readCompressionInfo(const JsonValue *theValue);

        //! Read stored buffers info from Json part.
        Standard_EXPORT bool readBuffers(const JsonValue *theValue);

        //! Read mesh info from Json part.
        Standard_EXPORT bool readMesh(std::istream &theStream, const JsonValue *theValue);

        //! Read mesh geometry (nodes+elements) from Json part.
        Standard_EXPORT bool readGeometry(std::istream &theStream, const JsonValue *theValue);

        //! Read mesh AABB from Json part.
        Standard_EXPORT bool readBounds(std::istream &theStream, const JsonValue *theValue);

        //! Read mesh blocks from Json part.
        Standard_EXPORT bool readBlocks(std::istream &theStream, const JsonValue *theValue);

        //! Read nodes info from Json part and fill the mesh with data from binary part.
        Standard_EXPORT bool readNodes(std::istream &theStream, const JsonValue *theValue);

        //! Read elements info from Json part.
        Standard_EXPORT bool readElement(std::istream &theStream, const JsonValue *theValue);

        //! Read elements info from Json part.
        Standard_EXPORT bool readElement(const std::shared_ptr<std::vector<char>> &theBuffer, MeshDataBufferType theType);

        //! Read array of properties from Json part.
        Standard_EXPORT bool readProperties(std::istream &theStream, const JsonValue *theValue);

        //! Read property info from Json part and fill the mesh with data from binary part.
        Standard_EXPORT bool readProperty(std::istream &theStream, const JsonValue *theValue);

        //! Read property id field from Json part.
        Standard_EXPORT void readPropertyIdField(const JsonValue *theValue, const Handle(MeshPropertyBase) & theProp);

        //! Read property description field from Json part.
        Standard_EXPORT void readPropertyDescriptionField(const JsonValue *theValue,
                                                          const Handle(MeshPropertyBase) & theProp);

        //! Read property time fields from Json part.
        Standard_EXPORT void readPropertyTimeField(const JsonValue *theValue, const Handle(MeshNVecProperty) & theProp);

        //! Read property enabled field from Json part.
        Standard_EXPORT void readPropertyIsEnabledField(const JsonValue *theValue,
                                                        const Handle(MeshScalarPropertyBase) & theProp);

        //! Read property elements type field from Json part.
        Standard_EXPORT void readPropertyElemTypeField(const JsonValue *theValue,
                                                       const Handle(MeshScalarPropertyBase) & theProp);

        //! Read property min/max fields from Json part.
        Standard_EXPORT void readPropertyMinMaxField(const JsonValue *theValue,
                                                     const Handle(MeshScalarPropertyBase) & theProp);

        //! Read property display units from Json part.
        Standard_EXPORT void readPropertyUnitsField(const JsonValue *theValue,
                                                    const Handle(MeshScalarPropertyBase) & theProp);

        //! Read property group field from Json part.
        Standard_EXPORT void readPropertyGroupsField(const JsonValue *theValue, const Handle(MeshGroupProperty) & theProp);

        //! Read property components fields from Json part.
        Standard_EXPORT bool readPropertyComponents(std::istream &theStream,
                                                    const JsonValue *theValue,
                                                    const Handle(MeshNVecProperty) & theProp,
                                                    const MeshDataBufferType theType);

    protected:
        //! Read nodes from binary part.
        Standard_EXPORT bool readNodes(const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read nodes from binary part without compression.
        Standard_EXPORT bool readNodesUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read nodes from binary part using draco compression.
        Standard_EXPORT bool readNodesDraco(const std::shared_ptr<std::vector<char>> &theBuffer);

    protected:
        //! Read typed elements from binary part.
        Standard_EXPORT bool readElements(const std::shared_ptr<std::vector<char>> &theBuffer, MeshElements &theElements);

        //! Read typed elements from binary part without compression.
        template <class Elem_T>
        Standard_EXPORT bool readElementsUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                      NCollection_Vector<Elem_T> &theElements,
                                                      uint64_t &theOffset);

        //! Read typed elements from binary part without compression.
        Standard_EXPORT bool readElementsUncompressed(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                      MeshElements &theElements);

        //! Read typed elements from binary part using draco compression.
        Standard_EXPORT bool readElementsDraco(const std::shared_ptr<std::vector<char>> &theBuffer,
                                               MeshElements &theElements);

        //! Read typed elements from binary part without compression (MDB version < 12).
        Standard_EXPORT bool readElementsUncompressedVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                           MeshElements &theElements,
                                                           MeshElements &theAuxElements);

        //! Read typed elements from binary part using draco compression (MDB version < 12).
        Standard_EXPORT bool readElementsDracoVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                    MeshElements &theElements,
                                                    MeshElements &theAuxElements);

        //! Decode mesh element node index.
        Standard_EXPORT void decodeElementNode(uint32_t theEncodedIndex, int32_t &theIndex, int32_t &theLastIndex) const;

        //! Read 2d elements from binary part (MDB version < 2).
        Standard_EXPORT bool readElements2dVer11(const std::shared_ptr<std::vector<char>> &theBuffer,
                                                 MeshElements &theQuads,
                                                 MeshElements &theTriangles);

    protected:
        //! Compute indexed elements using stored mesh elements.
        Standard_EXPORT void fillIndexedElements();

    protected:
        //! Read property data from binary part.
        Standard_EXPORT bool readPropertyBinData(const Handle(MeshScalarPropertyBase) & theProp,
                                                 MeshDataBufferType theType,
                                                 std::istream &theStream,
                                                 unsigned int theBufferId);

    protected:
        //! Read scalar property data from binary part.
        Standard_EXPORT bool readPropertyBinDataScalar(const Handle(MeshScalarProperty) & theProp,
                                                       const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read scalar property data from binary part without compression.
        Standard_EXPORT bool readPropertyBinDataScalarUncompressed(const Handle(MeshScalarProperty) & theProp,
                                                                   const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read scalar property data from binary part using draco compression.
        Standard_EXPORT bool readPropertyBinDataScalarDraco(const Handle(MeshScalarProperty) & theProp,
                                                            const std::shared_ptr<std::vector<char>> &theBuffer);

    protected:
        //! Read scalar property data from binary part.
        Standard_EXPORT bool readPropertyBinDataNodeVec(const Handle(MeshNodeVecProperty) & theProp,
                                                        const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read scalar property data from binary part without compression.
        Standard_EXPORT bool readPropertyBinDataNodeVecUncompressed(const Handle(MeshNodeVecProperty) & theProp,
                                                                    const std::shared_ptr<std::vector<char>> &theBuffer);

        //! Read scalar property data from binary part using draco compression.
        Standard_EXPORT bool readPropertyBinDataNodeVecDraco(const Handle(MeshNodeVecProperty) & theProp,
                                                             const std::shared_ptr<std::vector<char>> &theBuffer);

    protected:
        //! Read group property data from binary part.
        Standard_EXPORT bool readPropertyBinDataGroup(const Handle(MeshGroupProperty) & theProp,
                                                      const std::shared_ptr<std::vector<char>> &theBuffer);

    protected:
        //! Read data from binary part.
        Standard_EXPORT std::shared_ptr<std::vector<char>> readBinData(std::istream &theStream,
                                                                       uint64_t theOffset,
                                                                       uint64_t theLength);

        //! Check version.
        Standard_EXPORT bool checkVersion() const;

        //! Check if old version with uint32_t offsets should be read.
        bool toRead32BitsOffsets() const { return myVersionMajor == MeshImportExport::MESH_DATA_VERSION_MAJOR_2; }

        //! Check if MDB file contains split compressed parts that can be read in parallel.
        bool toSupportParallelDraco() const { return myVersionMajor >= MeshImportExport::MESH_DATA_VERSION_MAJOR_9; }

        //! Check if MDB file contains old compressed float parts (< 10 version).
        bool toReadDracoVersion9() const { return myVersionMajor < MeshImportExport::MESH_DATA_VERSION_MAJOR_10; }

        //! Return size of offsets fields.
        size_t offsetSize() const { return toRead32BitsOffsets() ? sizeof(uint32_t) : sizeof(uint64_t); }

    protected:
        //! Information about buffer.
        struct BufferData
        {
            uint64_t length;
            uint64_t offset;

            BufferData() : length(0), offset(0) {}
            BufferData(uint64_t theLength, uint64_t theOffset) : length(theLength), offset(theOffset) {}
        };

    protected:
        uint16_t myVersionMajor = 0;
        uint16_t myVersionMinor = 0;
        int myFormatSubversion = 0;
        NCollection_Vector<BufferData> myBufferData;
        TCollection_AsciiString myGenerator;

        std::shared_ptr<rapidjson::Document> myReader;

        NCollection_Map<Handle(RWCae::MeshNVecProperty)> myUpdatedProps;
    };

}

#endif // _MESHBINREADER_HEADERFILE