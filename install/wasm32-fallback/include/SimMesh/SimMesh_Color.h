

#ifndef SIMMESH_COLOR_HEADERFILE
#define SIMMESH_COLOR_HEADERFILE

#include <SimMesh_Types.h>

//! RGBA color packed into 4 bytes.
class SimMesh_Color : public SimMesh_Vec4uc {
public:
    using SimMesh_Vec4uc::NCollection_Vec4;
    using SimMesh_Vec4uc::operator=;

    //! Empty constructor.
    SimMesh_Color() {}

    //! Constructor.
    SimMesh_Color(float theRed, float theGreen, float theBlue, float theAlpha)
    {
        Set(theRed, theGreen, theBlue, theAlpha);
    }

    //! Return red color.
    float R() const { return byteToFloat(r()); }

    //! Return green color.
    float G() const { return byteToFloat(g()); }

    //! Return blue color.
    float B() const { return byteToFloat(b()); }

    //! Return alpha coefficient.
    float A() const { return byteToFloat(a()); }

    //! Set color values
    void Set(float theRed, float theGreen, float theBlue, float theAlpha)
    {
        r() = floatToByte(theRed);
        g() = floatToByte(theGreen);
        b() = floatToByte(theBlue);
        a() = floatToByte(theAlpha);
    }

private:
    //! Pack float into byte.
    static unsigned char floatToByte(float theVal) { return (unsigned char)((theVal > 1.0f ? 1.0f : theVal) * 255.0f); }

    //! Unpack float from byte.
    static float byteToFloat(unsigned char theByte) { return float(theByte) / 255.0f; }
};

#endif // SIMMESH_COLOR_HEADERFILE
