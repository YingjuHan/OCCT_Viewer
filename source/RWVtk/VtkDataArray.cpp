
#include <RWVtk/VtkDataArray.h>

#include <Standard_ArrayStreamBuffer.hxx>
#include <cmath>
#include <limits>
#include <cerrno>

namespace {
template<typename T>
int checkedInt(T theValue)
{
    const long double aValue = static_cast<long double>(theValue);
    if (!std::isfinite(aValue) || aValue < std::numeric_limits<int>::min()
        || aValue > std::numeric_limits<int>::max() || std::trunc(aValue) != aValue) {
        throw std::out_of_range("VTK integer value exceeds supported range or is not integral");
    }
    return static_cast<int>(theValue);
}
}

namespace RWVtk {

// ================================================================
// Function : GetFloat
// ================================================================
template<typename T>
float VtkBinaryArray<T>::GetFloat(int theIndex, int theComp) const
{
    return static_cast<float>(getElement(theIndex, theComp));
}

// ================================================================
// Function : GetDouble
// ================================================================
template<typename T>
double VtkBinaryArray<T>::GetDouble(int theIndex, int theComp) const
{
    return static_cast<double>(getElement(theIndex, theComp));
}

// ================================================================
// Function : GetInt
// ================================================================
template<typename T>
int VtkBinaryArray<T>::GetInt(int theIndex, int theComp) const
{
    return checkedInt(getElement(theIndex, theComp));
}

// ================================================================
// Function : CreateBinary
// ================================================================
Handle(VtkAbstractArray) VtkAbstractArray::CreateBinary(const TCollection_AsciiString&    theType,
                                                        const Handle(NCollection_Buffer)& theData,
                                                        int                               theOffset,
                                                        int                               theNbComponents)
{
    if (theType == "Float32") {
        return new VtkBinaryArray<float>(theData, theOffset, theNbComponents);
    }
    if (theType == "Float64") {
        return new VtkBinaryArray<double>(theData, theOffset, theNbComponents);
    }
    if (theType == "Int8") {
        return new VtkBinaryArray<int8_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "UInt8") {
        return new VtkBinaryArray<uint8_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "Int16") {
        return new VtkBinaryArray<int16_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "UInt16") {
        return new VtkBinaryArray<uint16_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "Int32") {
        return new VtkBinaryArray<int32_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "UInt32") {
        return new VtkBinaryArray<uint32_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "Int64") {
        return new VtkBinaryArray<int64_t>(theData, theOffset, theNbComponents);
    }
    if (theType == "UInt64") {
        return new VtkBinaryArray<uint64_t>(theData, theOffset, theNbComponents);
    }
    return nullptr;
}

// ================================================================
// Function : VtkAsciiArray
// ================================================================
template<typename T>
VtkAsciiArray<T>::VtkAsciiArray(const char* theData, int theLength, int theNbComponents)
    : VtkAbstractArray(theNbComponents)
{
    readFromIStream<T>(theData, theLength);
}

// ================================================================
// Function : VtkAsciiArray
// ================================================================
template<>
VtkAsciiArray<int8_t>::VtkAsciiArray(const char* theData, int theLength, int theNbComponents)
    : VtkAbstractArray(theNbComponents)
{
    // data for char type should be read as short
    readFromIStream<short>(theData, theLength);
}

// ================================================================
// Function : VtkAsciiArray
// ================================================================
template<>
VtkAsciiArray<uint8_t>::VtkAsciiArray(const char* theData, int theLength, int theNbComponents)
    : VtkAbstractArray(theNbComponents)
{
    // data for unsigned char type should be read as short
    readFromIStream<short>(theData, theLength);
}

// ================================================================
// Function : VtkAsciiArray
// ================================================================
template<>
VtkAsciiArray<float>::VtkAsciiArray(const char* theData, int theLength, int theNbComponents)
    : VtkAbstractArray(theNbComponents)
{
    // data for float type can contain additional string values like "inf", "-inf", "nan"
    readRealFromIStream(theData, theLength);
}

// ================================================================
// Function : VtkAsciiArray
// ================================================================
template<>
VtkAsciiArray<double>::VtkAsciiArray(const char* theData, int theLength, int theNbComponents)
    : VtkAbstractArray(theNbComponents)
{
    // data for double type can contain additional string values like "inf", "-inf", "nan"
    readRealFromIStream(theData, theLength);
}

// ================================================================
// Function : GetFloat
// ================================================================
template<typename T>
float VtkAsciiArray<T>::GetFloat(int theIndex, int theComp) const
{
    return static_cast<float>(getElement(theIndex, theComp));
}

// ================================================================
// Function : GetDouble
// ================================================================
template<typename T>
double VtkAsciiArray<T>::GetDouble(int theIndex, int theComp) const
{
    return static_cast<double>(getElement(theIndex, theComp));
}

// ================================================================
// Function : GetInt
// ================================================================
template<typename T>
int VtkAsciiArray<T>::GetInt(int theIndex, int theComp) const
{
    return checkedInt(getElement(theIndex, theComp));
}

// ================================================================
// Function : CreateAscii
// ================================================================
Handle(VtkAbstractArray) VtkAbstractArray::CreateAscii(const TCollection_AsciiString& theType,
                                                       const char*                    theData,
                                                       int                            theLength,
                                                       int                            theNbComponents)
{
    if (theType == "Float32") {
        return new VtkAsciiArray<float>(theData, theLength, theNbComponents);
    }
    if (theType == "Float64") {
        return new VtkAsciiArray<double>(theData, theLength, theNbComponents);
    }
    if (theType == "Int8") {
        return new VtkAsciiArray<int8_t>(theData, theLength, theNbComponents);
    }
    if (theType == "UInt8") {
        return new VtkAsciiArray<uint8_t>(theData, theLength, theNbComponents);
    }
    if (theType == "Int16") {
        return new VtkAsciiArray<int16_t>(theData, theLength, theNbComponents);
    }
    if (theType == "UInt16") {
        return new VtkAsciiArray<uint16_t>(theData, theLength, theNbComponents);
    }
    if (theType == "Int32") {
        return new VtkAsciiArray<int32_t>(theData, theLength, theNbComponents);
    }
    if (theType == "UInt32") {
        return new VtkAsciiArray<uint32_t>(theData, theLength, theNbComponents);
    }
    if (theType == "Int64") {
        return new VtkAsciiArray<int64_t>(theData, theLength, theNbComponents);
    }
    if (theType == "UInt64") {
        return new VtkAsciiArray<uint64_t>(theData, theLength, theNbComponents);
    }
    return nullptr;
}

// ================================================================
// Function : readFromIStream
// ================================================================
template<typename T>
template<typename ET>
void VtkAsciiArray<T>::readFromIStream(const char* theData, int theLength)
{
    Standard_ArrayStreamBuffer aStreamBuffer(theData, theLength);
    std::istream               aStream(&aStreamBuffer);
    aStream.imbue(std::locale::classic());

    std::string token;
    while (aStream >> token) {
        size_t used = 0;
        long double aValue;
        if (std::numeric_limits<T>::is_signed) { aValue = std::stoll(token, &used); }
        else {
            if (token[0] == '-') { throw std::out_of_range("Negative unsigned VTK value"); }
            aValue = std::stoull(token, &used);
        }
        if (used != token.size()) { throw std::invalid_argument("Invalid VTK ASCII integer token"); }
        const long double aNumber = static_cast<long double>(aValue);
        if (aNumber < std::numeric_limits<T>::lowest() || aNumber > std::numeric_limits<T>::max()) {
            throw std::out_of_range("VTK ASCII value exceeds declared type range");
        }
        myData.Append(static_cast<T>(aValue));
    }
    if (!aStream.eof()) { throw std::invalid_argument("Invalid VTK ASCII integer token"); }
    if (myNbComponents <= 0 || myData.Length() % myNbComponents != 0) {
        throw std::invalid_argument("Invalid VTK ASCII component count");
    }
}

// ================================================================
// Function : readRealFromIStream
// ================================================================
template<typename T>
void VtkAsciiArray<T>::readRealFromIStream(const char* theData, int theLength)
{
    Standard_ArrayStreamBuffer aStreamBuffer(theData, theLength);
    std::istream               aStream(&aStreamBuffer);
    aStream.imbue(std::locale::classic());

    TCollection_AsciiString aCheckStr;

    while (aStream >> aCheckStr) {
        char* anEnd = nullptr;
        errno = 0;
        const double aValue = Strtod(aCheckStr.ToCString(), &anEnd);
        if (anEnd == aCheckStr.ToCString() || *anEnd != '\0') {
            throw std::invalid_argument("Invalid VTK ASCII real token");
        }
        if ((errno == ERANGE && !std::isfinite(aValue))
            || (std::isfinite(aValue) && std::abs(aValue) > std::numeric_limits<T>::max())) {
            throw std::out_of_range("VTK ASCII real exceeds declared type range");
        }
        myData.Append(static_cast<T>(aValue));
    }
    if (myNbComponents <= 0 || myData.Length() % myNbComponents != 0) {
        throw std::invalid_argument("Invalid VTK ASCII component count");
    }
}

}
