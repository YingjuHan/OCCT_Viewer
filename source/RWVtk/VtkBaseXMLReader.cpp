#include <RWVtk/VtkBaseXMLReader.h>

#include <FSD_Base64.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>

namespace RWVtk {

// ================================================================
// function : VtkBaseXMLReader
// ================================================================
VtkBaseXMLReader::VtkBaseXMLReader() : VtkBaseReader()
{
    //
}

// ================================================================
// function : ~VtkBaseXMLReader
// ================================================================
VtkBaseXMLReader::~VtkBaseXMLReader()
{
    //
}

// =======================================================================
// function : PerformStream
// =======================================================================
bool VtkBaseXMLReader::PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName)
{
    if (!VtkBaseReader::PerformStream(theStream, theFileName)) {
        return false;
    }

    LDOMParser aParser;

    if (aParser.parse(*myIStream)) {
        TCollection_AsciiString aData;
        TCollection_AsciiString anError = aParser.GetError(aData);
        Message::SendFail() << "Error: " << anError << ": " << aData;
        return false;
    }

    myDOM = aParser.getDocument();

    if (!readHeader()) {
        return false;
    }

    if (!perform()) {
        return false;
    }

    return true;
}

// =======================================================================
// function : readHeader
// =======================================================================
bool VtkBaseXMLReader::readHeader()
{
    LDOM_Element aRoot = myDOM.getDocumentElement();
    if (aRoot.isNull()) {
        Message::SendFail() << "Error: Root element is null";
        return false;
    }

    TCollection_AsciiString aRootName = aRoot.getTagName();
    if (aRootName != "VTKFile") {
        Message::SendFail() << "Error: Invalid root name : '" << aRootName << "'";
        return false;
    }

    if (!readRootAttributes(aRoot)) {
        return false;
    }

    if (!readDataElements(aRoot)) {
        return false;
    }

    return true;
}

// =======================================================================
// function : readRootAttributes
// =======================================================================
bool VtkBaseXMLReader::readRootAttributes(const LDOM_Element& theRoot)
{
    TCollection_AsciiString aType;
    TCollection_AsciiString aByteOrder;
    TCollection_AsciiString aHeaderType;
    TCollection_AsciiString aCompressor;

    LDOM_NodeList aChildList = theRoot.GetAttributesList();
    for (int i = 0; i < aChildList.getLength(); i++) {
        TCollection_AsciiString anItemName  = aChildList.item(i).getNodeName();
        TCollection_AsciiString anItemValue = aChildList.item(i).getNodeValue();

        if (anItemName == "type") {
            aType = anItemValue;
        } else if (anItemName == "byte_order") {
            aByteOrder = anItemValue;
        } else if (anItemName == "header_type") {
            aHeaderType = anItemValue;
        } else if (anItemName == "compressor") {
            aCompressor = anItemValue;
        }
    }

    if (aType != dataType()) {
        Message::SendFail() << "Error: Invalid data type: '" << aType << "'";
        return false;
    }

    if (aByteOrder == "BigEndian") {
        myByteOrder = ByteOrder::BigEndian;
        Message::SendFail() << "Error: BigEndian byte order is not supported";
        return false;
    } else if (aByteOrder == "LittleEndian") {
        myByteOrder = ByteOrder::LittleEndian;
    } else {
        Message::SendFail() << "Error: Invalid byte order: '" << aByteOrder << "'";
        return false;
    }

    if (aCompressor != "") {
        Message::SendFail() << "Error: Compression is not supported";
        return false;
    }

    if (aHeaderType == "UInt32") {
        myHeaderSize = sizeof(uint32_t);
    } else if (aHeaderType == "UInt64") {
        myHeaderSize = sizeof(uint64_t);
    } else if (!aHeaderType.IsEmpty()) {
        Message::SendFail() << "Error: Unsupported header type '" << aHeaderType << "'";
        return false;
    }

    return true;
}

// =======================================================================
// function : readDataElements
// =======================================================================
bool VtkBaseXMLReader::readDataElements(const LDOM_Element& theRoot)
{
    for (LDOM_Node aNode = theRoot.getFirstChild(); !aNode.isNull(); aNode = aNode.getNextSibling()) {
        LDOM_Element anElement = (const LDOM_Element&)aNode;

        TCollection_AsciiString anElementName = anElement.getTagName();
        if (anElementName == dataType()) {
            myDataElement = anElement;
        } else if (anElementName == "AppendedData") {
            myAppendedData = anElement.getFirstChild().getNodeValue();
            if (getXmlAttribute(anElement, "encoding", "Encoding") == "raw") {
                Message::SendFail() << "Error: Unsupported encoding type 'raw'";
                return false;
            }
        }
    }

    if (myDataElement.isNull()) {
        Message::SendFail() << "Error: '" << dataType() << "' element not found";
        return false;
    }

    return true;
}

// =======================================================================
// function : readDataArray
// =======================================================================
VtkBaseXMLReader::DataArray VtkBaseXMLReader::readDataArray(const LDOM_Element& theElem)
{
    DataArray aRes;

    if (!readDataArrayHeader(theElem, aRes)) {
        return aRes;
    }

    const char* aBase64Data;

    if (aRes.Format == DataFormat::Binary || aRes.Format == DataFormat::Ascii) {
        LDOMString aDataString = theElem.getFirstChild().getNodeValue();
        aBase64Data            = aDataString.GetString();
    } else {
        aBase64Data = myAppendedData.GetString() + 1 + aRes.Offset;
    }

    if (aRes.Format == DataFormat::Ascii) {
        aRes.Data = VtkAbstractArray::CreateAscii(aRes.Type,
                                                  aBase64Data,
                                                  int(std::strlen(aBase64Data)),
                                                  int(aRes.Components.size()));
        return aRes;
    }

    constexpr double aByteToBase64Coeff = 4.0 / 3.0;

    Handle(NCollection_Buffer) aData = FSD_Base64::Decode(aBase64Data, (int)Ceiling(myHeaderSize * aByteToBase64Coeff));
    if (aData.IsNull()) {
        Message::SendFail() << "Error: Fail to allocate memory";
        return aRes;
    }

    DataArrayHeader aHeader;
    for (int i = 0; i < myHeaderSize; i++) {
        aHeader.RawData[i] = aData->Data()[i];
    }

    constexpr int  aInt32Size = 4;
    const uint64_t aDataSize  = myHeaderSize == aInt32Size ? aHeader.Data32 : aHeader.Data64;

    uint64_t aNbSymbols = (uint64_t)Ceiling(((uint64_t)(int64_t)myHeaderSize + aDataSize) * aByteToBase64Coeff);

    aData = FSD_Base64::Decode(aBase64Data, (size_t)aNbSymbols);
    if (aData.IsNull()) {
        Message::SendFail() << "Error: Fail to allocate memory";
        return aRes;
    }

    aRes.Data = VtkAbstractArray::CreateBinary(aRes.Type, aData, myHeaderSize, int(aRes.Components.size()));
    return aRes;
}

// =======================================================================
// function : readDataArrayHeader
// =======================================================================
bool VtkBaseXMLReader::readDataArrayHeader(const LDOM_Element& theElem, VtkBaseXMLReader::DataArray& theDataArray)
{
    theDataArray.Name = getXmlAttribute(theElem, "Name", "name");

    theDataArray.Type = getXmlAttribute(theElem, "Type", "type");
    if (theDataArray.Type.IsEmpty()) {
        Message::SendFail() << "Error: 'DataArray -> type' attribute not found";
        return false;
    }

    int aNbComponents = 1;
    getXmlAttribute(theElem, "NumberOfComponents", "numberOfComponents").GetInteger(aNbComponents);
    if (aNbComponents <= 0) {
        Message::SendFail() << "Error: Invalid number of components '" << aNbComponents << "'";
        return false;
    }
    theDataArray.Components.resize(aNbComponents, "");

    TCollection_AsciiString aFormat = getXmlAttribute(theElem, "format", "Format");
    if (aFormat == "binary") {
        theDataArray.Format = DataFormat::Binary;
    } else if (aFormat == "ascii") {
        theDataArray.Format = DataFormat::Ascii;
    } else if (aFormat == "appended") {
        theDataArray.Format = DataFormat::Appended;
    } else {
        Message::SendFail() << "Error: Invalid points format '" << aFormat << "'";
        return false;
    }

    getXmlAttribute(theElem, "offset", "Offset").GetInteger(theDataArray.Offset);
    return true;
}

// =======================================================================
// function : getChildrenElementsByTagName
// =======================================================================
std::vector<LDOM_Element> VtkBaseXMLReader::getChildrenElementsByTagName(const LDOM_Element& theElem,
                                                                         const char*         theName)
{
    std::vector<LDOM_Element> aRes;
    if (theElem.isNull() || !theElem.hasChildNodes()) {
        return aRes;
    }

    for (LDOM_Element anElement = theElem.GetChildByTagName(theName); !anElement.isNull();
         anElement              = anElement.GetSiblingByTagName()) {
        aRes.push_back(anElement);
    }

    return aRes;
}

// =======================================================================
// function : getFirstElement
// =======================================================================
LDOM_Element VtkBaseXMLReader::getFirstElement(const LDOM_Element& theElem, const char* theName)
{
    std::vector<LDOM_Element> anElements = getChildrenElementsByTagName(theElem, theName);
    return !anElements.empty() ? anElements[0] : LDOM_Element();
}

}
