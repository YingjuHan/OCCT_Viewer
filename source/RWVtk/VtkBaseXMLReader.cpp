#include <RWVtk/VtkBaseXMLReader.h>

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
using Bytes = std::vector<unsigned char>;

Bytes decodeBase64(const char* data, size_t size)
{
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    Bytes result;
    char group[4]; int count = 0;
    for (size_t i = 0; i < size; ++i) {
        const unsigned char ch = static_cast<unsigned char>(data[i]);
        if (std::isspace(ch)) { continue; }
        group[count++] = (char)ch;
        if (count != 4) { continue; }
        unsigned digits[4] = {};
        for (int n = 0; n < 4; ++n) {
            const char* digit = std::strchr(alphabet, group[n]);
            if (group[n] != '=' && (group[n] == 0 || !digit)) { throw std::runtime_error("Invalid base64 digit"); }
            if (group[n] != '=') { digits[n] = (unsigned)(digit - alphabet); }
        }
        if (group[0] == '=' || group[1] == '=' || (group[2] == '=' && group[3] != '=')
            || (group[2] == '=' && (digits[1] & 15)) || (group[3] == '=' && group[2] != '=' && (digits[2] & 3))) {
            throw std::runtime_error("Invalid base64 padding");
        }
        result.push_back((unsigned char)((digits[0] << 2) | (digits[1] >> 4)));
        if (group[2] != '=') { result.push_back((unsigned char)((digits[1] << 4) | (digits[2] >> 2))); }
        if (group[3] != '=') { result.push_back((unsigned char)((digits[2] << 6) | digits[3])); }
        count = 0;
    }
    if (count) { throw std::runtime_error("Truncated base64 quartet"); }
    return result;
}

uint64_t readWord(const Bytes& bytes, size_t offset, size_t size, bool bigEndian)
{
    if (offset > bytes.size() || size > bytes.size() - offset) {
        throw std::runtime_error("Truncated VTK binary header");
    }
    uint64_t value = 0;
    for (size_t i = 0; i < size; ++i) {
        value |= static_cast<uint64_t>(bytes[offset + i]) << (8 * (bigEndian ? size - 1 - i : i));
    }
    return value;
}

Bytes decodePayload(const Bytes& bytes, size_t headerSize, bool bigEndian, bool compressed, bool exact)
{
    if (!compressed) {
        const uint64_t length = readWord(bytes, 0, headerSize, bigEndian);
        if (length > bytes.size() - headerSize) { throw std::runtime_error("Truncated VTK binary array"); }
        if (exact && length != bytes.size() - headerSize) { throw std::runtime_error("Extra inline VTK binary bytes"); }
        return Bytes(bytes.begin() + headerSize, bytes.begin() + headerSize + static_cast<size_t>(length));
    }
    const uint64_t blocks = readWord(bytes, 0, headerSize, bigEndian);
    const uint64_t blockSize = readWord(bytes, headerSize, headerSize, bigEndian);
    const uint64_t lastSize = readWord(bytes, headerSize * 2, headerSize, bigEndian);
    if (blocks > bytes.size() / headerSize || blocks > (std::numeric_limits<size_t>::max() / headerSize) - 3
        || (blocks && (blockSize == 0 || lastSize > blockSize))) {
        throw std::runtime_error("Invalid VTK compression header");
    }
    size_t offset = static_cast<size_t>(3 + blocks) * headerSize;
    Bytes result;
    for (uint64_t i = 0; i < blocks; ++i) {
        const uint64_t packedSize = readWord(bytes, static_cast<size_t>(3 + i) * headerSize, headerSize, bigEndian);
        const uint64_t unpackedSize = i + 1 == blocks && lastSize ? lastSize : blockSize;
        if (offset > bytes.size() || packedSize > bytes.size() - offset
            || unpackedSize > std::numeric_limits<uLongf>::max()
            || unpackedSize > result.max_size() - result.size()) {
            throw std::runtime_error("Invalid VTK compressed block length");
        }
        const size_t start = result.size();
        result.resize(start + static_cast<size_t>(unpackedSize));
        uLongf actual = static_cast<uLongf>(unpackedSize);
        if (uncompress(result.data() + start, &actual, bytes.data() + offset,
                       static_cast<uLong>(packedSize)) != Z_OK || actual != unpackedSize) {
            throw std::runtime_error("Cannot decompress VTK zlib block");
        }
        offset += static_cast<size_t>(packedSize);
    }
    if (exact && offset != bytes.size()) { throw std::runtime_error("Extra inline compressed bytes"); }
    return result;
}

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

    std::string document((std::istreambuf_iterator<char>(theStream)), std::istreambuf_iterator<char>());
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
    }
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

    if (!readDataArrayHeader(theElem, aRes)) {
        return aRes;
    }

    try {
        const size_t valueSize = componentSize(aRes.Type);
        std::string text;
        if (aRes.Format == DataFormat::Appended) {
            if (aRes.Offset >= myRawAppendedData.size()) { throw std::runtime_error("Appended offset out of range"); }
            text = myRawAppendedData.substr(aRes.Offset);
        } else {
            for (LDOM_Node node = theElem.getFirstChild(); !node.isNull(); node = node.getNextSibling()) {
                const TCollection_AsciiString value = node.getNodeValue();
                text += value.ToCString();
            }
        }
        if (aRes.Format == DataFormat::Ascii) {
            aRes.Data = VtkAbstractArray::CreateAscii(aRes.Type, text.data(), static_cast<int>(text.size()),
                                                     static_cast<int>(aRes.Components.size()));
        } else {
            Bytes encoded = aRes.Format == DataFormat::Appended && myAppendedIsRaw
                                ? Bytes(text.begin(), text.end()) : decodeBase64(text.data(), text.size());
            Bytes payload = decodePayload(encoded, myHeaderSize, myByteOrder == ByteOrder::BigEndian,
                                          !myCompressor.empty(), aRes.Format != DataFormat::Appended);
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
