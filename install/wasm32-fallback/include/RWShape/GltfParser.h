#ifndef _GLTFPARSER_HEADERFILE
#define _GLTFPARSER_HEADERFILE

#ifndef HAVE_RAPIDJSON
#define HAVE_RAPIDJSON // OCCT should be built with USE_RAPIDJSON option
#endif
#include <RWGltf_GltfJsonParser.hxx>

namespace RWShape
{

    //! glTF parser that processes extra entities in glTF file.
    class GltfParser : public RWGltf_GltfJsonParser
    {
    public:
        Standard_EXPORT explicit GltfParser(TopTools_SequenceOfShape &theRootShapes);

        //! Return TRUE if data format is binary.
        Standard_EXPORT bool IsBinary() const { return myIsBinary; }

        //! Parse glTF data.
        Standard_EXPORT bool ParseGltfFromMemory(std::istream &theStream, const TCollection_AsciiString &theFile);

        //! Parse extras section.
        Standard_EXPORT void ParseExtras();

        //! Parse glTF data header.
        Standard_EXPORT bool ParseGltfHeaderFromMemory(std::istream &theStream, const TCollection_AsciiString &theFile);
    };

}

#endif // _GLTFPARSER_HEADERFILE