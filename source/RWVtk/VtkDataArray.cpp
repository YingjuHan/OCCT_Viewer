
#include <RWVtk/VtkDataArray.h>

#include <Standard_ArrayStreamBuffer.hxx>

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
    return static_cast<int>(getElement(theIndex, theComp));
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
    return static_cast<int>(getElement(theIndex, theComp));
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

    ET aValue;

    while (aStream >> aValue) {
        myData.Append(static_cast<T>(aValue));
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
        myData.Append(static_cast<T>(Strtod(aCheckStr.ToCString(), nullptr)));
    }
}

}
