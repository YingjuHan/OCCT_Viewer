
#ifndef _VTKBASEXMLREADER_HEADERFILE
#define _VTKBASEXMLREADER_HEADERFILE

#include <RWVtk/VtkBaseReader.h>
#include <RWVtk/VtkDataArray.h>

#include <LDOMParser.hxx>

#include <vector>

namespace RWVtk {
//! Base reader class for VTK data stored in XML format.
class VtkBaseXMLReader : public VtkBaseReader {
public:
    //! Constructor.
    Standard_EXPORT VtkBaseXMLReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkBaseXMLReader();

    //! Perform reading from the stream.
    Standard_EXPORT virtual bool PerformStream(std::istream&                  theStream,
                                               const TCollection_AsciiString& theFileName) override;

public:
    //! Struct for header of uncompressed binary data.
    union DataArrayHeader {
        uint32_t      Data32;
        uint64_t      Data64;
        unsigned char RawData[8];
    };

    //! Type of stored data.
    enum class DataFormat { Binary, Ascii, Appended };

    //! Byte order of stored data.
    enum class ByteOrder { LittleEndian, BigEndian };

    //! Struct for "DataArray" XML element.
    struct DataArray {
        TCollection_AsciiString              Name;
        TCollection_AsciiString              Type;
        std::vector<TCollection_AsciiString> Components;
        DataFormat                           Format;
        Handle(VtkAbstractArray)             Data;
        size_t                               Offset = 0;

        void Reset()
        {
            Name.Clear();
            Type.Clear();
            Components.clear();
            Format = DataFormat::Binary;
            Data.Nullify();
            Offset = 0;
        }
    };

protected:
    //! Helper function that attempts to get an XML attribute using two aliases.
    static LDOMString getXmlAttribute(const LDOM_Element& theElem,
                                      const LDOMString&   theName1,
                                      const LDOMString&   theName2)
    {
        LDOMString aResult = theElem.getAttribute(theName1);
        if (aResult.Type() == LDOMBasicString::LDOM_NULL) {
            aResult = theElem.getAttribute(theName2);
        }
        return aResult;
    }

    //! Return type of data.
    virtual TCollection_AsciiString dataType() const = 0;

    //! Perform reading of element data.
    Standard_EXPORT virtual bool perform() { return true; }

    //! Read header of XML file.
    Standard_EXPORT bool readHeader();

    //! Read attributes of root element of XML file.
    Standard_EXPORT bool readRootAttributes(const LDOM_Element& theRoot);

    //! Read data elements of XML file.
    Standard_EXPORT bool readDataElements(const LDOM_Element& theRoot);

    //! Return children of XML element by name.
    Standard_EXPORT std::vector<LDOM_Element> getChildrenElementsByTagName(const LDOM_Element& theElem,
                                                                           const char*         theName);

    //! Return first child of XML element by its name.
    Standard_EXPORT LDOM_Element getFirstElement(const LDOM_Element& theElem, const char* theName);

    //! Read 'DataArray' XML element.
    Standard_EXPORT DataArray readDataArray(const LDOM_Element& theElem);

    //! Read header of 'DataArray' XML element.
    Standard_EXPORT bool readDataArrayHeader(const LDOM_Element& theElem, DataArray& theDataArray);

protected:
    LDOM_Document myDOM;
    LDOM_Element  myDataElement;
    LDOMString    myAppendedData;

    std::string myRawAppendedData;
    std::string myCompressor;
    bool myAppendedIsRaw = false;

    int       myHeaderSize = 4;
    ByteOrder myByteOrder  = ByteOrder::LittleEndian;
};

}

#endif // _VTKBASEXMLREADER_HEADERFILE
