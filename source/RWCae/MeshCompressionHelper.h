#ifndef _MESHCOMPRESSIONHELPER_HEADERFILE
#define _MESHCOMPRESSIONHELPER_HEADERFILE

#include <NCollection_DoubleMap.hxx>
#include <TCollection_AsciiString.hxx>

namespace RWCae
{
    //! Compression type.
    //! WARNING! Enumeration is used in MeshBinReader storage format,
    //! old values should not be changed.
    enum class MeshCompressionType
    {
        None = 0,
        Draco = 1
    };

    //! @internal Mapper between MeshCompressionType and string.
    class MeshCompressionTypeConvertor
    {
    public:
        static const char *GetString(MeshCompressionType theType)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return aMap.Find1((int)theType).ToCString();
        }

        static MeshCompressionType GetType(const char *theString)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return (MeshCompressionType)aMap.Find2(theString);
        }

    private:
        static NCollection_DoubleMap<int, TCollection_AsciiString> fillMap()
        {
            NCollection_DoubleMap<int, TCollection_AsciiString> aMap;
            aMap.Bind((int)MeshCompressionType::None, "none");
            aMap.Bind((int)MeshCompressionType::Draco, "draco");
            return aMap;
        }
    };

    //! Draco compression info.
    class MeshDracoCompressionInfo
    {
    public:
        //! Constructor.
        explicit MeshDracoCompressionInfo(int theNodesQuantBits = -1, int thePropsQuantBits = -1, int theNbParts = 16)
            : myNodesQuantBits(theNodesQuantBits),
              myPropsQuantBits(thePropsQuantBits),
              myNbParts(theNbParts)
        {
        }

        //! Return quantization bits for nodes.
        int NodesQuantBits() const { return myNodesQuantBits; }

        //! Return name for quantization bits for nodes.
        static const char *NodesQuantBitsStr() { return "nodesquantbits"; }

        //! Return quantization bits for properties.
        int PropsQuantBits() const { return myPropsQuantBits; }

        //! Return name for quantization bits for properties.
        static const char *PropsQuantBitsStr() { return "propsquantbits"; }

        //! Return number of parts to which compressed binary data will be split.
        int NbParts() const { return myNbParts; }

        //! Return number of parts to which compressed binary data of specified size will be split.
        int NbParts(int theSize) const
        {
            constexpr int aMinPartSize = 1000;
            return Max(Min(myNbParts, theSize / aMinPartSize), 1);
        }

    protected:
        int myNodesQuantBits = -1;
        int myPropsQuantBits = -1;
        int myNbParts = 16;
    };

}

#endif // _MESHCOMPRESSIONHELPER_HEADERFILE
