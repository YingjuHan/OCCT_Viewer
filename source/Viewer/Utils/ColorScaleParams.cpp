

#include "ColorScaleParams.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InitFromPresentation
// ================================================================
void ColorScaleParams::InitFromPresentation(const Handle(ViewCae::InteractiveColorScale)& thePrs)
{
    Vec2 aPos;
    thePrs->GetPosition(aPos.x(), aPos.y());
    position     = Vec2(Graphic3d_Vec2i(aPos));
    title        = TCollection_AsciiString(thePrs->GetTitle()).ToCString();
    displayUnits = thePrs->DisplayUnits().ToCString();
    {
        const TColStd_SequenceOfExtendedString& aLabels = thePrs->Labels();
        customLabels.reserve(aLabels.Length());
        for (const TCollection_ExtendedString& anIter : aLabels) {
            customLabels.push_back(TCollection_AsciiString(anIter).ToCString());
        }
        toSkipEmptyLabels = thePrs->ToSkipEmptyLabels();
    }

    Graphic3d_Vec2d aColScaleSize(thePrs->GetBaseBreadth(), thePrs->GetBaseHeight());
    size            = Vec2(aColScaleSize);
    maxSize         = thePrs->GetMaxBaseHeight();
    labelSize       = thePrs->GetTextHeight();
    nbIntervals     = thePrs->GetNumberOfIntervals();
    isSmooth        = thePrs->IsSmoothTransition();
    isReversed      = thePrs->IsReversed();
    outOfRangeColor = Quantity_ColorRGBA::ColorToHex(thePrs->GetOutOfRangeColor()).ToCString();
    isCustomColors  = thePrs->GetColorType() == Aspect_TOCSD_USER;
    if (!isCustomColors) {
        Quantity_Color aMinColor;
        Quantity_Color aMaxColor;
        thePrs->ColorRange(aMinColor, aMaxColor);
        minColor = Quantity_ColorRGBA::ColorToHex(Quantity_ColorRGBA(aMinColor)).ToCString();
        maxColor = Quantity_ColorRGBA::ColorToHex(Quantity_ColorRGBA(aMaxColor)).ToCString();
    } else {
        const NCollection_Sequence<Quantity_ColorRGBA>& aCustomColors = thePrs->GetCustomColors();
        customColors.reserve(aCustomColors.Length());
        for (const Quantity_ColorRGBA& anIter : aCustomColors) {
            customColors.push_back(Quantity_ColorRGBA::ColorToHex(anIter).ToCString());
        }
    }

    defaultRange  = Vec2(Graphic3d_Vec2d(thePrs->GetDefaultRangeMin(), thePrs->GetDefaultRangeMax()));
    displayRange  = Vec2(Graphic3d_Vec2d(thePrs->GetMin(), thePrs->GetMax()));
    selectedRange = Vec2(Graphic3d_Vec2d(thePrs->GetSelectedRangeFrom(), thePrs->GetSelectedRangeTo()));

    toHighlightIntervals = thePrs->ToHighlightIntervals();
    intervalHighlightColor =
        Quantity_ColorRGBA::ColorToHex(Quantity_ColorRGBA(thePrs->IntervalHighlightColor())).ToCString();
}

// ================================================================
// Function : UpdatePresentation
// ================================================================
void ColorScaleParams::UpdatePresentation(const Handle(ViewCae::InteractiveColorScale)& thePrs) const
{
    thePrs->SetTitle(title.c_str());
    thePrs->SetDisplayUnits(displayUnits.c_str());
    {
        TColStd_SequenceOfExtendedString aLabels;
        for (const std::string& aLabIter : customLabels) {
            aLabels.Append(TCollection_ExtendedString(aLabIter.c_str(), true));
        }
        thePrs->SetLabels(aLabels);
        thePrs->SetSkipEmptyLabels(toSkipEmptyLabels);
    }

    double aPrevMappedRangeFrom = thePrs->GetMappedRangeFrom();
    double aPrevMappedRangeTo   = thePrs->GetMappedRangeTo();

    thePrs->SetDefaultRange(defaultRange[0], defaultRange[1]); // supposed to be read-only!
    thePrs->SetRange(displayRange[0], displayRange[1]);
    if (selectedRange[0] > -Precision::Confusion() && selectedRange[1] < 1.0 + Precision::Confusion()
        && selectedRange[0] < selectedRange[1] - Precision::Confusion()) {
        thePrs->SetSelectedRange(selectedRange[0], selectedRange[1]);
    } else {
        Message::SendFail() << "Error: invalid color scale selected range (" << selectedRange[0] << ", "
                            << selectedRange[1] << ")";
    }

    double aMappedRangeFrom = thePrs->GetMappedRangeFrom();
    double aMappedRangeTo   = thePrs->GetMappedRangeTo();

    bool wasRangeUpdated = Abs(aMappedRangeFrom - aPrevMappedRangeFrom) > RealEpsilon()
                        || Abs(aMappedRangeTo - aPrevMappedRangeTo) > RealEpsilon();

    thePrs->SetPosition(int(position.x()), int(position.y()));
    thePrs->SetBaseBreadth(float(size.x()));
    thePrs->SetBaseHeight(float(size.y()));
    thePrs->SetMaxBaseHeight(maxSize);
    thePrs->SetTextHeight(labelSize);
    thePrs->SetNumberOfIntervals(nbIntervals);
    thePrs->SetSmoothTransition(isSmooth);
    thePrs->SetReversed(isReversed);
    thePrs->SetToHighlightIntervals(toHighlightIntervals);
    thePrs->SetPositionSide(occFromViewCorner((ViewCorner)screenSidePos));

    thePrs->SetLogarithmic(isLogarithmic);

    UpdatePrsColors(thePrs);

    thePrs->SetToUpdate();
    thePrs->UpdateColorMap(wasRangeUpdated);
}

// ================================================================
// Function : UpdatePrsColors
// ================================================================
void ColorScaleParams::UpdatePrsColors(const Handle(ViewCae::InteractiveColorScale)& thePrs) const
{
    {
        Quantity_ColorRGBA anOutColor;
        if (!ColorTools::ParseHexColor(outOfRangeColor, anOutColor)) {
            return;
        }
        thePrs->SetOutOfRangeColor(anOutColor);
    }

    {
        Quantity_ColorRGBA anIntervalHighlightColor;
        if (!ColorTools::ParseHexColor(intervalHighlightColor, anIntervalHighlightColor)) {
            return;
        }
        thePrs->SetIntervalHighlightColor(anIntervalHighlightColor.GetRGB());
    }

    thePrs->SetColorType(isCustomColors ? Aspect_TOCSD_USER : Aspect_TOCSD_AUTO);
    if (isCustomColors) {
        NCollection_Sequence<Quantity_ColorRGBA> aCustomColors;
        for (int i = 0; i < nbIntervals; ++i) {
            if (i >= static_cast<int>(customColors.size())) {
                aCustomColors.Append(Quantity_ColorRGBA(Quantity_NOC_BLACK));
                continue;
            }

            Quantity_ColorRGBA             aColor;
            const ColorTools::ColorString& aColorStr = customColors[i];
            if (!ColorTools::ParseHexColor(aColorStr, aColor)) {
                return;
            }
            aCustomColors.Append(aColor);
        }
        thePrs->SetCustomColors(aCustomColors);
    } else {
        Quantity_ColorRGBA aMinColor;
        Quantity_ColorRGBA aMaxColor;
        if (!ColorTools::ParseHexColor(minColor, aMinColor)) {
            return;
        }
        if (!ColorTools::ParseHexColor(maxColor, aMaxColor)) {
            return;
        }
        thePrs->SetColorRange(aMinColor.GetRGB(), aMaxColor.GetRGB());
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(ColorScaleParams)
{
    emscripten::value_object<ColorScaleParams>("ColorScaleParams")
        .field("defaultRange", &ColorScaleParams::defaultRange)
        .field("displayRange", &ColorScaleParams::displayRange)
        .field("selectedRange", &ColorScaleParams::selectedRange)
        .field("isSmooth", &ColorScaleParams::isSmooth)
        .field("isReversed", &ColorScaleParams::isReversed)
        .field("toSkipEmptyLabels", &ColorScaleParams::toSkipEmptyLabels)
        .field("nbIntervals", &ColorScaleParams::nbIntervals)
        .field("labelSize", &ColorScaleParams::labelSize)
        .field("pos", &ColorScaleParams::position)
        .field("size", &ColorScaleParams::size)
        .field("maxSize", &ColorScaleParams::maxSize)
        .field("isCustomColors", &ColorScaleParams::isCustomColors)
        .field("outOfRangeColor", &ColorScaleParams::outOfRangeColor)
        .field("minColor", &ColorScaleParams::minColor)
        .field("maxColor", &ColorScaleParams::maxColor)
        .field("customColors", &ColorScaleParams::customColors)
        .field("customLabels", &ColorScaleParams::customLabels)
        .field("title", &ColorScaleParams::title)
        .field("displayUnits", &ColorScaleParams::displayUnits)
        .field("propertyId", &ColorScaleParams::propertyId)
        .field("propertyComponent", &ColorScaleParams::propertyComponent)
        .field("toHighlightIntervals", &ColorScaleParams::toHighlightIntervals)
        .field("intervalHighlightColor", &ColorScaleParams::intervalHighlightColor);
}
#endif

} // namespace