#include "DimensionParams.h"

#include <NCollection_String.hxx>
#include <Message.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InitFromPresentation
// ================================================================
void DimensionParams::InitFromPresentation(const Handle(PrsDim_Dimension)& thePrs)
{
    if (thePrs->DisplaySpecialSymbol() == PrsDim_DisplaySpecialSymbol_No || thePrs->SpecialSymbol() == ' '
        || thePrs->SpecialSymbol() == '\0') {
        specialSymbol.clear();
        isSpecialSymbolAfter = false;
    } else {
        const Standard_ExtCharacter  aSymbExt[2] = {thePrs->SpecialSymbol(), '\0'};
        const NCollection_Utf8String aSymb(aSymbExt, 1);
        specialSymbol        = aSymb.ToCString();
        isSpecialSymbolAfter = thePrs->DisplaySpecialSymbol() == PrsDim_DisplaySpecialSymbol_After;
    }

    realValue = thePrs->GetValue();
    textValue = TCollection_AsciiString(thePrs->GetCustomValue()).ToCString();

    plane = Ax2(thePrs->GetPlane().Position().Ax2());
}

// ================================================================
// Function : UpdatePresentation
// ================================================================
void DimensionParams::UpdatePresentation(const Handle(PrsDim_Dimension)& thePrs) const
{
    if (specialSymbol.length() > 1) {
        Message::SendFail() << "Error, DimensionParams: invalid special symbol '" << specialSymbol << "'";
        return;
    } else if (!specialSymbol.empty()) {
        const NCollection_Utf16String aSymb(specialSymbol.c_str(), 1);
        thePrs->SetSpecialSymbol(*aSymb.ToCString());
        thePrs->SetDisplaySpecialSymbol(isSpecialSymbolAfter ? PrsDim_DisplaySpecialSymbol_After
                                                             : PrsDim_DisplaySpecialSymbol_Before);
    } else {
        thePrs->SetSpecialSymbol(' ');
        thePrs->SetDisplaySpecialSymbol(PrsDim_DisplaySpecialSymbol_No);
    }

    if (!textValue.empty()) {
        thePrs->SetCustomValue(TCollection_ExtendedString(textValue.c_str(), true));
    } else if (!thePrs->GetCustomValue().IsEmpty()) {
        thePrs->SetCustomValue(TCollection_ExtendedString());
        thePrs->SetComputedValue();
    }

    const Ax2 aRefAx2(thePrs->GetPlane().Position().Ax2());
    if (!plane.location.IsEqual(aRefAx2.location) || !plane.direction.IsEqual(aRefAx2.direction)
        || !plane.xDirection.IsEqual(aRefAx2.xDirection)) {
        thePrs->SetCustomPlane(gp_Pln(aRefAx2.GetAx2()));
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(DimensionParams)
{
    emscripten::value_object<DimensionParams>("DimensionParams")
        .field("plane", &DimensionParams::plane)
        .field("realValue", &DimensionParams::realValue)
        .field("textValue", &DimensionParams::textValue)
        .field("specialSymbol", &DimensionParams::specialSymbol)
        .field("isSpecialSymbolAfter", &DimensionParams::isSpecialSymbolAfter);
}
#endif

} // namespace