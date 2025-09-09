#ifndef _VIEWCORNER_HEADERFILE
#define _VIEWCORNER_HEADERFILE

#include <Aspect_TypeOfTriedronPosition.hxx>
#include <Graphic3d_HorizontalTextAlignment.hxx>
#include <Graphic3d_VerticalTextAlignment.hxx>

namespace Viewer {
//! View corner
enum class ViewCorner : int {
    FREE3D        = -1,
    CENTER_CENTER = 0,
    CENTER_UPPER  = 1,
    CENTER_LOWER  = 2,
    LEFT_CENTER   = 4,
    LEFT_UPPER    = 5,
    LEFT_LOWER    = 6,
    RIGHT_CENTER  = 8,
    RIGHT_UPPER   = 9,
    RIGHT_LOWER   = 10,
};

//! Return ViewCorner.
inline ViewCorner occToViewCorner(Aspect_TypeOfTriedronPosition thePos);

//! Convert ViewCorner to OCCT enum.
inline Aspect_TypeOfTriedronPosition occFromViewCorner(ViewCorner thePos);

//! Convert view corner into horizontal text alignment.
inline Graphic3d_HorizontalTextAlignment viewCornerToHAlignment(ViewCorner thePos);

//! Convert view corner into vertical text alignment.
inline Graphic3d_VerticalTextAlignment viewCornerToVAlignment(ViewCorner thePos);

//! Convert text alignment into view corner.
inline ViewCorner viewCornerFromTextAlignment(Graphic3d_HorizontalTextAlignment theHPos,
                                              Graphic3d_VerticalTextAlignment   theVPos);

} // namespace

#include "ViewCorner.lxx"

#endif // _VIEWCORNER_HEADERFILE

