#ifndef DrawBase_DRAWPRINTERS_HEADERFILE
#define DrawBase_DRAWPRINTERS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <gp_Pnt.hxx>

namespace DrawBase {

namespace DrawPrinters {
    //! Print a space-separated XYZ triplet.
    template<class T>
    inline Draw_Interpretor& operator<<(Draw_Interpretor& theDI, const NCollection_Vec3<T>& theVec)
    {
        theDI << theVec.x() << " " << theVec.y() << " " << theVec.z();
        return theDI;
    }

    //! Print a space-separated XYZ triplet.
    inline Draw_Interpretor& operator<<(Draw_Interpretor& theDI, const gp_Pnt& theVec)
    {
        theDI << theVec.X() << " " << theVec.Y() << " " << theVec.Z();
        return theDI;
    }

    //! Print string.
    inline Draw_Interpretor& operator<<(Draw_Interpretor& theDI, const std::string& theString)
    {
        theDI << theString.c_str();
        return theDI;
    }

    //! Print RGB color.
    inline Draw_Interpretor& operator<<(Draw_Interpretor& theDI, const Quantity_Color& theCol)
    {
        if (Quantity_Color(theCol.Name()).IsEqual(theCol)) {
            theDI << Quantity_Color::StringName(theCol.Name());
        } else {
            theDI << Quantity_Color::ColorToHex(theCol, false);
        }
        return theDI;
    }

    //! Print RGBA color.
    inline Draw_Interpretor& operator<<(Draw_Interpretor& theDI, const Quantity_ColorRGBA& theCol)
    {
        if (Abs(theCol.Alpha() - 1.0) <= gp::Resolution()
            && Quantity_Color(theCol.GetRGB().Name()).IsEqual(theCol.GetRGB())) {
            theDI << Quantity_Color::StringName(theCol.GetRGB().Name());
        } else {
            theDI << Quantity_ColorRGBA::ColorToHex(theCol, false);
        }
        return theDI;
    }
}

}

#endif // DrawBase_DRAWPRINTERS_HEADERFILE
