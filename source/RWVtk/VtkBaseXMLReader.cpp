#include <RWVtk/VtkBaseXMLReader.h>
#include "VtkBinaryDecoder.h"

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>
#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using Bytes = RWVtk::detail::Bytes;

size_t componentSize(const TCollection_AsciiString& type)
{
    if (type == "Int8" || type == "UInt8") { return 1; }
    if (type == "Int16" || type == "UInt16") { return 2; }
    if (type == "Int32" || type == "UInt32" || type == "Float32") { return 4; }
    if (type == "Int64" || type == "UInt64" || type == "Float64") { return 8; }
    throw std::runtime_error("Unsupported VTK numeric type");
}
}

namespace RWVtk {
int VtkBaseXMLReader::readCount(const LDOM_Element& element, const char* name, const char* alias, bool optional)
{
    const TCollection_AsciiString text=getXmlAttribute(element,name,alias);
    if (optional && text.IsEmpty()) { return 0; }
    size_t used=0; const auto value=std::stoll(text.ToCString(),&used);
    if (used!=(size_t)text.Length() || value<0 || value>std::numeric_limits<int>::max()) { throw std::runtime_error(std::string("Invalid VTK count: ")+name); }
    return (int)value;
}

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

    myHeaderSize = 4;
    myByteOrder = ByteOrder::LittleEndian;
    myCompressor.clear();
    myRawAppendedData.clear();
    myAppendedIsRaw = false;
    myAppendedData = LDOMString();
    myDataElement = LDOM_Element();

    std::string document;
    try { document=myBudget->Read(theStream); } catch (const std::exception& error) { return fail(error.what()); }
    // Raw appended bytes are outside XML. Parse the XML envelope separately.
    const size_t begin = document.find("<AppendedData");
    if (begin != std::string::npos) {
        const size_t tagEnd = document.find('>', begin);
        const size_t end = document.rfind("</AppendedData>");
        if (tagEnd == std::string::npos || end == std::string::npos || end <= tagEnd) {
            return fail("Malformed AppendedData envelope");
        }
        const size_t marker = document.find('_', tagEnd + 1);
        if (marker == std::string::npos || marker >= end) { return fail("Missing appended data marker"); }
        myRawAppendedData = document.substr(marker + 1, end - marker - 1);
        document.erase(tagEnd + 1, end - tagEnd - 1);
        document.shrink_to_fit();
    }
    if(document.find('\0')!=std::string::npos) {return fail("NUL in VTK XML envelope");}
    std::istringstream xml(document);
    LDOMParser aParser;

    if (aParser.parse(xml)) {
        TCollection_AsciiString aData;
        TCollection_AsciiString anError = aParser.GetError(aData);
        return fail(std::string(anError.ToCString()) + ": " + aData.ToCString());
    }

    myDOM = aParser.getDocument();

    if (!readHeader()) {
        return myReport.errors.empty() ? fail("Invalid VTK XML header") : false;
    }

    try {
        if (!perform()) { return myReport.errors.empty() ? fail("Invalid VTK dataset") : false; }
    } catch (const std::exception& error) { return fail(error.what()); }

    return true;
}

// =======================================================================
// function : readHeader
// =======================================================================
bool VtkBaseXMLReader::readHeader()
{
    LDOM_Element aRoot = myDOM.getDocumentElement();
    if (aRoot.isNull()) {
        return fail("Root element is null");
    }

    TCollection_AsciiString aRootName = aRoot.getTagName();
    if (aRootName != "VTKFile") {
        return fail(std::string("Invalid root name : '") + aRootName.ToCString() + "'");
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
        return fail(std::string("Invalid data type: '") + aType.ToCString() + "'");
    }

    if (aByteOrder == "BigEndian") {
        myByteOrder = ByteOrder::BigEndian;
    } else if (aByteOrder == "LittleEndian") {
        myByteOrder = ByteOrder::LittleEndian;
    } else {
        return fail(std::string("Invalid byte order: '") + aByteOrder.ToCString() + "'");
    }

    myCompressor = aCompressor.ToCString();
    if (!myCompressor.empty() && myCompressor != "vtkZLibDataCompressor") {
        return fail("Unsupported VTK compressor: " + myCompressor);
    }

    if (aHeaderType == "UInt32") {
        myHeaderSize = sizeof(uint32_t);
    } else if (aHeaderType == "UInt64") {
        myHeaderSize = sizeof(uint64_t);
    } else if (!aHeaderType.IsEmpty()) {
        return fail(std::string("Unsupported header type '") + aHeaderType.ToCString() + "'");
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
            const TCollection_AsciiString encoding = getXmlAttribute(anElement, "encoding", "Encoding");
            if (encoding != "raw" && encoding != "base64") { return fail("Unsupported appended encoding"); }
            myAppendedIsRaw = encoding == "raw";
        }
    }

    if (myDataElement.isNull()) {
        return fail(std::string("'") + dataType().ToCString() + "' element not found");
    }

    return true;
}

// =======================================================================
// function : readDataArray
// =======================================================================
VtkBaseXMLReader::DataArray VtkBaseXMLReader::readDataArray(const LDOM_Element& theElem)
{
    DataArray aRes;
    try { VtkResourceBudget::Consume(myBudget->Arrays,1,myLimits.MaxArrays,"arrays"); }
    catch (const std::exception& error) { fail(error.what()); return aRes; }
    if (!readDataArrayHeader(theElem, aRes)) {
        return aRes;
    }

    try {
        const size_t valueSize = componentSize(aRes.Type);
        std::string text;
        std::string_view source;
        if (aRes.Format == DataFormat::Appended) {
            if (aRes.Offset >= myRawAppendedData.size()) { throw std::runtime_error("Appended offset out of range"); }
            source = std::string_view(myRawAppendedData).substr(aRes.Offset);
        } else {
            for (LDOM_Node node = theElem.getFirstChild(); !node.isNull(); node = node.getNextSibling()) {
                const TCollection_AsciiString value = node.getNodeValue();
                text += value.ToCString();
            }
        }
        if (aRes.Format != DataFormat::Appended) { source=text; }
        if (aRes.Format == DataFormat::Ascii) {
            uint64_t values=0; bool token=false;
            for (unsigned char c : source) { if (std::isspace(c)) {token=false;} else if (!token) {++values;token=true;} }
            if (source.size()>(size_t)std::numeric_limits<int>::max() || values>(uint64_t)std::numeric_limits<int>::max()) { throw std::runtime_error("VTK array exceeds native index range"); }
            myBudget->Array(values*valueSize);
            aRes.Data = VtkAbstractArray::CreateAscii(aRes.Type, text.data(), static_cast<int>(text.size()),
                                                     static_cast<int>(aRes.Components.size()));
        } else {
            detail::BinaryCursor cursor(source,aRes.Format==DataFormat::Appended && myAppendedIsRaw);
            Bytes payload=detail::Decode(cursor,myHeaderSize,myByteOrder==ByteOrder::BigEndian,!myCompressor.empty(),*myBudget);
            if (aRes.Format!=DataFormat::Appended && !cursor.End()) {throw std::runtime_error("Extra inline VTK binary bytes");}
            if (payload.size()/valueSize>(size_t)std::numeric_limits<int>::max()) {throw std::runtime_error("VTK array exceeds native index range");}
            if (payload.size() % (valueSize * aRes.Components.size()) != 0) {
                throw std::runtime_error("VTK array byte length does not match component count");
            }
            const uint16_t endianProbe = 1;
            const bool hostBig = *reinterpret_cast<const unsigned char*>(&endianProbe) == 0;
            if (hostBig != (myByteOrder == ByteOrder::BigEndian)) {
                for (size_t i = 0; i < payload.size(); i += valueSize) {
                    std::reverse(payload.begin() + i, payload.begin() + i + valueSize);
                }
            }
            Handle(NCollection_Buffer) buffer = new NCollection_Buffer(NCollection_BaseAllocator::CommonBaseAllocator(), payload.size());
            if (!payload.empty()) { std::memcpy(buffer->ChangeData(), payload.data(), payload.size()); }
            aRes.Data = VtkAbstractArray::CreateBinary(aRes.Type, buffer, 0, static_cast<int>(aRes.Components.size()));
        }
        if (aRes.Data.IsNull()) { throw std::runtime_error("Unsupported VTK array type"); }
    } catch (const std::exception& error) {
        fail(std::string("Array '") + aRes.Name.ToCString() + "': " + error.what());
        aRes.Data.Nullify();
    }
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
        return fail("'DataArray -> type' attribute not found");
    }

    int aNbComponents = 1;
    const TCollection_AsciiString componentCount = getXmlAttribute(theElem, "NumberOfComponents", "numberOfComponents");
    if (!componentCount.IsEmpty()) {
        try {
            size_t used = 0; const auto count = std::stoll(componentCount.ToCString(), &used);
            if (used != (size_t)componentCount.Length() || count < 1 || count > std::numeric_limits<int>::max()) { return fail("Invalid NumberOfComponents"); }
            aNbComponents = (int)count;
        } catch (const std::exception&) { return fail("Invalid NumberOfComponents"); }
    }
    if (aNbComponents <= 0) {
        return fail(std::string("Invalid number of components '") + std::to_string(aNbComponents) + "'");
    }
    if ((uint64_t)aNbComponents>myLimits.MaxComponents) { return fail("VTK resource limit: components"); }
    theDataArray.Components.resize(aNbComponents, "");
    for (int i = 0; i < aNbComponents; ++i) {
        const std::string key = "ComponentName" + std::to_string(i);
        theDataArray.Components[i] = TCollection_AsciiString(theElem.getAttribute(key.c_str()));
    }

    TCollection_AsciiString aFormat = getXmlAttribute(theElem, "format", "Format");
    if (aFormat == "binary") {
        theDataArray.Format = DataFormat::Binary;
    } else if (aFormat == "ascii") {
        theDataArray.Format = DataFormat::Ascii;
    } else if (aFormat == "appended") {
        theDataArray.Format = DataFormat::Appended;
    } else {
        return fail(std::string("Invalid points format '") + aFormat.ToCString() + "'");
    }

    const TCollection_AsciiString offset = getXmlAttribute(theElem, "offset", "Offset");
    if (theDataArray.Format == DataFormat::Appended && offset.IsEmpty()) { return fail("Missing appended offset"); }
    if (!offset.IsEmpty()) {
        try {
            if (offset.ToCString()[0] == '-') { return fail("Negative appended offset"); }
            size_t used = 0;
            theDataArray.Offset = std::stoull(offset.ToCString(), &used);
            if (used != static_cast<size_t>(offset.Length())) { return fail("Invalid appended offset"); }
        } catch (const std::exception&) { return fail("Invalid appended offset"); }
    }
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
