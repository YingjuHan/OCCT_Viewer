#ifndef _VTKDATAARRAY_HEADERFILE
#define _VTKDATAARRAY_HEADERFILE

#include <NCollection_Buffer.hxx>
#include <NCollection_Vector.hxx>
#include <TCollection_AsciiString.hxx>
#include <cstring>
#include <stdexcept>

namespace RWVtk {

//! Base class for storing VTK array.
class VtkAbstractArray : public Standard_Transient {
public:
    //! Return float value for specified index and component.
    virtual float GetFloat(int theIndex, int theComp) const = 0;

    //! Return double value for specified index and component.
    virtual double GetDouble(int theIndex, int theComp) const = 0;

    //! Return int value for specified index and component.
    virtual int GetInt(int theIndex, int theComp) const = 0;

    //! Return number of elements.
    virtual int NbElements() const = 0;

public:
    //! Create array from binary data.
    Standard_EXPORT static Handle(VtkAbstractArray) CreateBinary(const TCollection_AsciiString&    theType,
                                                                 const Handle(NCollection_Buffer)& theData,
                                                                 int                               theOffset,
                                                                 int                               theNbComponents);

    //! Create array from ascii data.
    Standard_EXPORT static Handle(VtkAbstractArray) CreateAscii(const TCollection_AsciiString& theType,
                                                                const char*                    theData,
                                                                int                            theLength,
                                                                int                            theNbComponents);

protected:
    //! Constructor.
    explicit VtkAbstractArray(int theNbComponents) : myNbComponents(theNbComponents) {}

protected:
    int myNbComponents = 1;
};

//! VTK typed array created from binary data.
template<typename T>
class VtkBinaryArray : public VtkAbstractArray {
public:
    //! Constructor.
    VtkBinaryArray(const Handle(NCollection_Buffer)& theData, int theOffset, int theNbComponents)
        : VtkAbstractArray(theNbComponents),
          myData(theData),
          myOffset(theOffset)
    {
    }

    //! Return float value for specified index and component.
    Standard_EXPORT virtual float GetFloat(int theIndex, int theComp) const override;

    //! Return double value for specified index and component.
    Standard_EXPORT virtual double GetDouble(int theIndex, int theComp) const override;

    //! Return int value for specified index and component.
    Standard_EXPORT virtual int GetInt(int theIndex, int theComp) const override;

    //! Return number of elements.
    virtual int NbElements() const override
    {
        return static_cast<int>((myData->Size() - myOffset) / myNbComponents / sizeof(T));
    }

protected:
    //! Return element for specified index and component.
    T getElement(int theIndex, int theComp) const
    {
        if (theIndex < 0 || theIndex >= NbElements() || theComp < 0 || theComp >= myNbComponents) {
            throw std::out_of_range("VTK array index out of range");
        }
        T result;
        std::memcpy(&result, myData->Data() + myOffset + sizeof(T) * (myNbComponents * theIndex + theComp), sizeof(T));
        return result;
    }

protected:
    Handle(NCollection_Buffer) myData;
    int                        myOffset = 0;
};

//! VTK typed array created from ascii data.
template<typename T>
class VtkAsciiArray : public VtkAbstractArray {
public:
    //! Constructor.
    Standard_EXPORT VtkAsciiArray(const char* theData, int theLength, int theNbComponents);

    //! Return float value for specified index and component.
    Standard_EXPORT virtual float GetFloat(int theIndex, int theComp) const override;

    //! Return double value for specified index and component.
    Standard_EXPORT virtual double GetDouble(int theIndex, int theComp) const override;

    //! Return int value for specified index and component.
    Standard_EXPORT virtual int GetInt(int theIndex, int theComp) const override;

    //! Return number of elements.
    virtual int NbElements() const override { return myData.Length() / myNbComponents; }

protected:
    //! Return element for specified index and component.
    T getElement(int theIndex, int theComp) const
    {
        if (theIndex < 0 || theIndex >= NbElements() || theComp < 0 || theComp >= myNbComponents) {
            throw std::out_of_range("VTK array index out of range");
        }
        return myData[theIndex * myNbComponents + theComp];
    }

    //! Read data from buffer with ascii characters.
    template<typename ET>
    void readFromIStream(const char* theData, int theLength);

    //! Read data from buffer with ascii characters for float/double types.
    void readRealFromIStream(const char* theData, int theLength);

protected:
    NCollection_Vector<T> myData;
};

}

#endif // _VTKDATAARRAY_HEADERFILE
