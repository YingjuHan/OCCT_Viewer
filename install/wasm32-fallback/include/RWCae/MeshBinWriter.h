#ifndef _MESHBINWRITER_HEADERFILE
#define _MESHBINWRITER_HEADERFILE

#include <RWCae/MeshBinIOBase.h>
#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <Message_ProgressRange.hxx>
#include <NCollection_Map.hxx>

#include <memory>

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

namespace RWCae
{

    //! Writer for CAE mesh @link RWCae::MeshData @endlink binary storage format.
    //!
    //! Binary file consists of 3 sections:
    //!
    //!  1) Binary header section - contains
    //!    - header string
    //!    - version number (4 bytes)
    //!    - total file length (4 bytes)
    //!
    //!  2) JSON section - contains
    //!    - "JSON" header (4 bytes)
    //!    - size of the section in bytes (4 bytes)
    //!    - JSON string (see below)
    //!
    //!  3) Binary section - contains
    //!    - "BIN" header (4 bytes)
    //!    - size of the section in bytes (4 bytes)
    //!    - binary data
    //!
    //! Stored JSON has the next structure:
    //! @include MdbJsonSample.json
    class MeshBinWriter : public MeshBinIOBase
    {
    public:
        //! Main constructor.
        Standard_EXPORT explicit MeshBinWriter(const Handle(MeshData) & theMeshData);

        //! Destructor.
        Standard_EXPORT virtual ~MeshBinWriter();

        //! Perform exporting to the file.
        Standard_EXPORT bool Perform(const TCollection_AsciiString &theFileName,
                                     const Message_ProgressRange &theProgressRange = Message_ProgressRange());

        //! Perform exporting to the stream.
        Standard_EXPORT bool Perform(std::ostream &theStream,
                                     const TCollection_AsciiString &theFileName,
                                     const Message_ProgressRange &theProgressRange = Message_ProgressRange());

        //! Force write double property values as float values.
        void SetToForceSinglePrecision(bool theToForce) { myToForceSinglePrecision = theToForce; }

        //! Enable bin data compression using draco.
        void SetUseDracoCompression(const MeshDracoCompressionInfo &theInfo)
        {
            myCompressionType = MeshCompressionType::Draco;
            myDracoCompressionInfo = theInfo;
        }

    protected:
        //! Write header.
        Standard_EXPORT bool writeHeader(std::ostream &theStream);

        //! Write Json part.
        Standard_EXPORT bool writeJSON(std::ostream &theStream, const Message_ProgressRange &theProgressRange);

        //! Write array of buffers info to Json.
        Standard_EXPORT void writeBuffers(std::ostream &theStream);

        //! Write array of buffers info to Json.
        Standard_EXPORT void writeBufferArray();

        //! Write auxiliary info to Json.
        Standard_EXPORT void writeAsset();

        //! Write compression info to Json.
        Standard_EXPORT void writeCompressionInfo();

        //! Write mesh info to Json.
        Standard_EXPORT void writeMesh(const Message_ProgressRange &theProgressRange);

        //! Write mesh geometry (nodes+elements) info to Json.
        Standard_EXPORT void writeGeometry();

        //! Write mesh elements info to Json.
        Standard_EXPORT void writeElements();

        //! Write mesh AABB to Json.
        Standard_EXPORT void writeBounds();

        //! Write mesh blocks to Json.
        Standard_EXPORT void writeBlocks();

        //! Write array of properties info to Json.
        Standard_EXPORT void writeProperties();

        //! Write property info to Json.
        Standard_EXPORT void writeProperty(const Handle(MeshNVecProperty) & theProperty, int theIndex);

        //! Write property components info to Json.
        Standard_EXPORT void writePropertyComponents(const Handle(MeshNVecProperty) & theProperty,
                                                     int theIndex,
                                                     MeshDataBufferType theBufType);

        //! Write property values range to Json.
        Standard_EXPORT void writePropertyRange(const Handle(MeshScalarPropertyBase) & theProp,
                                                MeshPropertyElemType theElemType);

        //! Write group property info to Json.
        Standard_EXPORT void writeGroupProperty(const Handle(MeshGroupProperty) & theProperty);

        //! Write total size and sizes of Json/binary parts.
        Standard_EXPORT bool updateSizes(std::ostream &theStream);

    protected:
        //! Write binary part.
        Standard_EXPORT bool writeBinData(std::ostream &theStream, const Message_ProgressRange &theProgressRange);

        //! Write buffers to binary part.
        Standard_EXPORT bool writeBinDataBuffers(std::ostream &theStream, const Message_ProgressRange &theProgressRange);

        //! Write buffer to binary part.
        Standard_EXPORT uint64_t writeBinDataBuffer(std::ostream &theStream,
                                                    int theBuffIdx,
                                                    const Message_ProgressRange &theProgressRange);

    protected:
        //! Write mesh data nodes.
        Standard_EXPORT uint64_t writeNodes(std::ostream &theStream);

        //! Write mesh data nodes without compression.
        Standard_EXPORT uint64_t writeNodesUncompressed(std::ostream &theStream);

        //! Write mesh data nodes using draco compression.
        Standard_EXPORT uint64_t writeNodesDraco(std::ostream &theStream);

    protected:
        //! Write mesh data elements.
        Standard_EXPORT uint64_t writeMeshElements(std::ostream &theStream,
                                                   const MeshElements &theElems,
                                                   const Message_ProgressRange &theProgressRange);

        //! Write mesh data elements without compression.
        template <class Elem_T>
        Standard_EXPORT uint64_t writeMeshElementsVectorUncompressed(std::ostream &theStream,
                                                                     const NCollection_Vector<Elem_T> &theElems,
                                                                     const Message_ProgressRange &theProgressRange);

        //! Write mesh data elements without compression.
        Standard_EXPORT uint64_t writeMeshElementsUncompressed(std::ostream &theStream,
                                                               const MeshElements &theElems,
                                                               const Message_ProgressRange &theProgressRange);

        //! Write mesh data elements using draco compression.
        Standard_EXPORT uint64_t writeMeshElementsDraco(std::ostream &theStream,
                                                        const MeshElements &theElems,
                                                        const Message_ProgressRange &theProgressRange);

        //! Encode mesh element node index.
        Standard_EXPORT void encodeElementNode(int32_t theIndex, uint32_t &theEncodedIndex, int32_t &theLastIndex) const;

    protected:
        //! Write property to binary part.
        Standard_EXPORT uint64_t writeBinDataProperty(std::ostream &theStream,
                                                      int thePropIndex,
                                                      int theCompIndex,
                                                      MeshDataBufferType theType);

        //! Write scalar property to binary part.
        Standard_EXPORT uint64_t writeBinDataScalarProperty(std::ostream &theStream, int thePropIndex, int theCompIndex);

        //! Write scalar property to binary part without compression.
        Standard_EXPORT uint64_t writeBinDataScalarPropertyUncompressed(std::ostream &theStream,
                                                                        int thePropIndex,
                                                                        int theCompIndex);

        //! Write scalar property to binary part using draco compression.
        Standard_EXPORT uint64_t writeBinDataScalarPropertyDraco(std::ostream &theStream,
                                                                 int thePropIndex,
                                                                 int theCompIndex);

    protected:
        //! Write nodal elemental property to binary part.
        Standard_EXPORT uint64_t writeBinDataNodalElementalProperty(std::ostream &theStream,
                                                                    int thePropIndex,
                                                                    int theCompIndex);

        //! Write nodal elemental property to binary part without compression.
        Standard_EXPORT uint64_t writeBinDataNodalElementalPropertyUncompressed(std::ostream &theStream,
                                                                                int thePropIndex,
                                                                                int theCompIndex);

        //! Write nodal elemental property to binary part using draco compression.
        Standard_EXPORT uint64_t writeBinDataNodalElementalPropertyDraco(std::ostream &theStream,
                                                                         int thePropIndex,
                                                                         int theCompIndex);

    protected:
        //! Write group property to binary part.
        Standard_EXPORT uint64_t writeBinDataGroupProperty(std::ostream &theStream, int thePropIndex, int theCompIndex);

    protected:
        //! Information about type of buffer and its index.
        struct BufferData
        {
            MeshDataBufferType type = MeshDataBufferType::NODE;
            int index = -1;
            int subIndex = -1;
            uint64_t length = 0;

            BufferData() {}

            explicit BufferData(MeshDataBufferType theType, int theIndex = -1, int theSubIndex = -1)
                : type(theType),
                  index(theIndex),
                  subIndex(theSubIndex)
            {
            }
        };

    protected:
        NCollection_Vector<BufferData> myBufferData;
        size_t myBufferArrayOffset = 0;

        std::shared_ptr<rapidjson::Writer<rapidjson::OStreamWrapper,
                                          rapidjson::UTF8<>,
                                          rapidjson::UTF8<>,
                                          rapidjson::CrtAllocator,
                                          rapidjson::kWriteDefaultFlags | rapidjson::kWriteNanAndInfFlag>>
            myWriter;

        NCollection_Map<int> myPropsToUnload;

        bool myToForceSinglePrecision = false;
    };

}

#endif // _MESHBINWRITER_HEADERFILE
