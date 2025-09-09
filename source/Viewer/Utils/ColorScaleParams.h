#ifndef _COLORSCALEPARAMS_HEADERFILE
#define _COLORSCALEPARAMS_HEADERFILE

#include <ViewCae/InteractiveColorScale.h>

#include "ColorTools.h"
#include "VecN.h"
#include "ViewCorner.h"
#include <vector>

namespace Viewer {
//! Parameters for color scale object.
struct ColorScaleParams {
    Vec2  defaultRange      = Vec2(Graphic3d_Vec2d(0.0, 1.0));
    Vec2  displayRange      = Vec2(Graphic3d_Vec2d(0.0, 1.0));
    Vec2  selectedRange     = Vec2(Graphic3d_Vec2d(0.0, 1.0));
    Vec2  position          = Vec2(Graphic3d_Vec2i(0, 0));
    Vec2  size              = Vec2(Graphic3d_Vec2i(0, 0));
    float maxSize           = 1.0;
    int   nbIntervals       = 10;
    int   labelSize         = 12;
    bool  isSmooth          = false;
    bool  isReversed        = false;
    bool  toSkipEmptyLabels = false;

    bool                                 isCustomColors = false;
    ColorTools::ColorString              outOfRangeColor;
    ColorTools::ColorString              minColor;
    ColorTools::ColorString              maxColor;
    std::vector<ColorTools::ColorString> customColors;
    std::vector<std::string>             customLabels;

    std::string title;
    std::string displayUnits;

    std::string propertyId        = "";
    int         propertyComponent = -1;

    bool                    toHighlightIntervals = true;
    ColorTools::ColorString intervalHighlightColor;

    int screenSidePos = (int)ViewCorner::RIGHT_CENTER;
    bool isLogarithmic = false;

public:
    //! Copy parameters from presentation.
    void InitFromPresentation(const Handle(ViewCae::InteractiveColorScale)& thePrs);

    //! Update interactive object.
    void UpdatePresentation(const Handle(ViewCae::InteractiveColorScale)& thePrs) const;

private:
    //! Update interactive object custom colors.
    void UpdatePrsColors(const Handle(ViewCae::InteractiveColorScale)& thePrs) const;
};

} // namespace

#endif // _COLORSCALEPARAMS_HEADERFILE