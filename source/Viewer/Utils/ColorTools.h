#ifndef _COLORTOOLS_HEADERFILE
#define _COLORTOOLS_HEADERFILE

#include <Message.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <string>

namespace Viewer {
//! Color-related tools.
class ColorTools {
public:
    //! RGBA color definition in form of a hex-encoded string.
    using ColorString = std::string;

    //! Parse hex color string and print error message on wrong input.
    static bool ParseHexColor(const ColorString& theHexColor, Quantity_ColorRGBA& theColor)
    {
        if (!Quantity_ColorRGBA::ColorFromHex(theHexColor.c_str(), theColor)) {
            if (!Quantity_Color::ColorFromName(theHexColor.c_str(), theColor.ChangeRGB())) {
                Message::SendFail() << "Error: invalid color format: '" << theHexColor
                                    << "', please use valid hex format: '#xxxxxxxx' '";
                return false;
            }
        }
        return true;
    }
};

} // namespace

#endif // _COLORTOOLS_HEADERFILE