#include "InteractiveAspectsManager.h"

#include "WasmViewer.h"
#include "Utils/ViewCorner.h"

#include <ViewBase/IInteractiveObject.h>

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveAspectsManager
// ================================================================
InteractiveAspectsManager::InteractiveAspectsManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : GetZLayer
// ================================================================
int InteractiveAspectsManager::GetZLayer(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? aPrs->ZLayer() : Graphic3d_ZLayerId_UNKNOWN;
}

// ================================================================
// Function : SetZLayer
// ================================================================
void InteractiveAspectsManager::SetZLayer(const ObjectId& theObjId, const int theLayer)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetZLayer (" << theObjId << ", " << theLayer
                            << ") called for non-existing object";
        return;
    } else if (aPrs->ZLayer() == theLayer) {
        return;
    } else if (theLayer != Graphic3d_ZLayerId_Default && theLayer != Graphic3d_ZLayerId_Top
               && theLayer != Graphic3d_ZLayerId_Topmost && theLayer != Graphic3d_ZLayerId_TopOSD
               && theLayer != Graphic3d_ZLayerId_BotOSD) {
        Message::SendFail() << "Error: InteractiveContext::SetZLayer (" << theObjId << ", " << theLayer
                            << ") called for invalid Z-Layer";
        return;
    }

    aPrs->SetZLayer(theLayer);
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetTransparency
// ================================================================
float InteractiveAspectsManager::GetTransparency(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? (float)aPrs->Transparency() : 0.0f;
}

// ================================================================
// Function : SetTransparency
// ================================================================
void InteractiveAspectsManager::SetTransparency(const ObjectId& theObjId, const float theTransp)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetTransparency (" << theObjId
                            << ") called for non-existing object";
        return;
    } else if ((float)aPrs->Transparency() == theTransp) {
        return;
    }

    aPrs->SetTransparency(theTransp);
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetBaseColor
// ================================================================
ColorTools::ColorString InteractiveAspectsManager::GetBaseColor(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull() || !aPrs->HasColor()) {
        return ColorTools::ColorString();
    }

    Quantity_Color aColor;
    aPrs->Color(aColor);
    return Quantity_ColorRGBA::ColorToHex(Quantity_ColorRGBA(aColor)).ToCString();
}

// ================================================================
// Function : SetBaseColor
// ================================================================
void InteractiveAspectsManager::SetBaseColor(const ObjectId& theObjId, const ColorTools::ColorString& theHexColor)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetBaseColor (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    if (theHexColor.empty() && aPrs->HasColor()) {
        aPrs->UnsetColor();
        if (aPrs->ToBeUpdated()) {
            myWebViewer->GetContext()->Redisplay(aPrs, false);
        }
        invalidateViewer(aPrs);
    } else if (!theHexColor.empty()) {
        Quantity_ColorRGBA aColor;
        if (!ColorTools::ParseHexColor(theHexColor, aColor)) {
            return;
        }
        aPrs->SetColor(aColor.GetRGB());
        if (aPrs->ToBeUpdated()) {
            myWebViewer->GetContext()->Redisplay(aPrs, false);
        }
        invalidateViewer(aPrs);
    }
}

// ================================================================
// Function : GetLineWidth
// ================================================================
float InteractiveAspectsManager::GetLineWidth(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() && aPrs->HasWidth() ? float(aPrs->Width()) : 1.0f;
}

// ================================================================
// Function : SetLineWidth
// ================================================================
void InteractiveAspectsManager::SetLineWidth(const ObjectId& theObjId, float theWidth)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetLineWidth (" << theObjId
                            << ") called for non-existing object";
        return;
    } else if (theWidth <= 0.0f) {
        Message::SendFail() << "Error: InteractiveContext::SetLineWidth() is called with invalid line width '"
                            << theWidth << "'";
        return;
    }

    const double aNewWidth  = Round(theWidth);
    const double anOldWidth = aPrs->HasWidth() ? aPrs->Width() : 1.0;
    if (aNewWidth == anOldWidth) {
        return;
    }

    aPrs->SetWidth(aNewWidth);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    }
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetLineType
// ================================================================
int InteractiveAspectsManager::GetLineType(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? aPrs->Attributes()->LineAspect()->Aspect()->LineType() : -1;
}

// ================================================================
// Function : SetLineType
// ================================================================
void InteractiveAspectsManager::SetLineType(const ObjectId& theObjId, int theType)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetLineType (" << theObjId
                            << ") called for non-existing object";
        return;
    } else if (theType <= Aspect_TOL_EMPTY || theType >= Aspect_TOL_USERDEFINED) {
        Message::SendFail() << "Error: InteractiveContext::SetLineType() is called with invalid line type '" << theType
                            << "'";
        return;
    }

    if (theType == aPrs->Attributes()->LineAspect()->Aspect()->LineType()) {
        return;
    }
    aPrs->Attributes()->SetOwnLineAspects();
    aPrs->Attributes()->LineAspect()->Aspect()->SetLineType((Aspect_TypeOfLine)theType);
    aPrs->SynchronizeAspects();
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetTextHeight
// ================================================================
double InteractiveAspectsManager::GetTextHeight(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? aPrs->Attributes()->TextAspect()->Height() : 0.0;
}

// ================================================================
// Function : SetTextHeight
// ================================================================
void InteractiveAspectsManager::SetTextHeight(const ObjectId& theObjId, double theHeight)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: SetTextHeight (" << theObjId << ") called for non-existing object";
        return;
    } else if (theHeight <= 0) {
        Message::SendFail() << "Error: SetTextHeight() is called with invalid height '" << theHeight << "'";
        return;
    }

    if (theHeight == aPrs->Attributes()->TextAspect()->Height()) {
        return;
    }
    if (!aPrs->Attributes()->HasOwnTextAspect()) {
        aPrs->Attributes()->SetTextAspect(new Prs3d_TextAspect());
        *aPrs->Attributes()->TextAspect()->Aspect() = *aPrs->Attributes()->Link()->TextAspect()->Aspect();
    }
    aPrs->Attributes()->TextAspect()->SetHeight(theHeight);
    myWebViewer->GetContext()->Redisplay(aPrs, false);
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetTextFont
// ================================================================
std::string InteractiveAspectsManager::GetTextFont(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? aPrs->Attributes()->TextAspect()->Aspect()->Font().ToCString() : "";
}

// ================================================================
// Function : SetTextFont
// ================================================================
void InteractiveAspectsManager::SetTextFont(const ObjectId& theObjId, const std::string& theFont)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: SetTextFont (" << theObjId << ") called for non-existing object";
        return;
    }

    const TCollection_AsciiString aNewFont(theFont.c_str());
    if (aNewFont == aPrs->Attributes()->TextAspect()->Aspect()->Font()) {
        return;
    }
    if (!aPrs->Attributes()->HasOwnTextAspect()) {
        aPrs->Attributes()->SetTextAspect(new Prs3d_TextAspect());
        *aPrs->Attributes()->TextAspect()->Aspect() = *aPrs->Attributes()->Link()->TextAspect()->Aspect();
    }
    aPrs->Attributes()->TextAspect()->SetFont(aNewFont.ToCString());
    myWebViewer->GetContext()->Redisplay(aPrs, false);
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetTextFontAspect
// ================================================================
int InteractiveAspectsManager::GetTextFontAspect(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? aPrs->Attributes()->TextAspect()->Aspect()->TextFontAspect() : 0;
}

// ================================================================
// Function : SetTextFontAspect
// ================================================================
void InteractiveAspectsManager::SetTextFontAspect(const ObjectId& theObjId, const int theAspect)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: SetTextFontAspect (" << theObjId << ", " << theAspect
                            << ") called for non-existing object";
        return;
    } else if (theAspect < Font_FontAspect_Regular || theAspect > Font_FontAspect_BoldItalic) {
        Message::SendFail() << "Error: SetTextFontAspect (" << theObjId << ", " << theAspect
                            << ") called with invalid aspect";
        return;
    }

    if (theAspect == aPrs->Attributes()->TextAspect()->Aspect()->TextFontAspect()) {
        return;
    }
    if (!aPrs->Attributes()->HasOwnTextAspect()) {
        aPrs->Attributes()->SetTextAspect(new Prs3d_TextAspect());
        *aPrs->Attributes()->TextAspect()->Aspect() = *aPrs->Attributes()->Link()->TextAspect()->Aspect();
    }
    aPrs->Attributes()->TextAspect()->Aspect()->SetTextFontAspect((Font_FontAspect)theAspect);
    myWebViewer->GetContext()->Redisplay(aPrs, false);
    invalidateViewer(aPrs);
}

// ================================================================
// Function : GetTextJustification
// ================================================================
int InteractiveAspectsManager::GetTextJustification(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    return !aPrs.IsNull() ? (
               int)Viewer::viewCornerFromTextAlignment(aPrs->Attributes()->TextAspect()->HorizontalJustification(),
                                                            aPrs->Attributes()->TextAspect()->VerticalJustification())
                          : 0;
}

// ================================================================
// Function : SetTextJustification
// ================================================================
void InteractiveAspectsManager::SetTextJustification(const ObjectId& theObjId, int theCorner)
{
    Handle(AIS_InteractiveObject) aPrs = FindAisObject(theObjId);
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: SetTextJustification (" << theObjId << ") called for non-existing object";
        return;
    } else if (theCorner < 0 || theCorner > static_cast<int>(ViewCorner::RIGHT_LOWER)) {
        Message::SendFail() << "Error: SetTextJustification() is called with invalid corner '" << theCorner << "'";
        return;
    }

    Graphic3d_HorizontalTextAlignment aHJust = Viewer::viewCornerToHAlignment((ViewCorner)theCorner);
    Graphic3d_VerticalTextAlignment   aVJust = Viewer::viewCornerToVAlignment((ViewCorner)theCorner);
    if (aHJust == aPrs->Attributes()->TextAspect()->HorizontalJustification()
        && aVJust == aPrs->Attributes()->TextAspect()->VerticalJustification()) {
        return;
    }

    if (!aPrs->Attributes()->HasOwnTextAspect()) {
        aPrs->Attributes()->SetTextAspect(new Prs3d_TextAspect());
        *aPrs->Attributes()->TextAspect()->Aspect() = *aPrs->Attributes()->Link()->TextAspect()->Aspect();
    }
    aPrs->Attributes()->TextAspect()->SetHorizontalJustification(aHJust);
    aPrs->Attributes()->TextAspect()->SetVerticalJustification(aVJust);
    myWebViewer->GetContext()->Redisplay(aPrs, false);
    invalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveAspectsManager)
{
    emscripten::class_<InteractiveAspectsManager, emscripten::base<IManager>>("InteractiveAspectsManager")
        .function("getZLayer", &InteractiveAspectsManager::GetZLayer)
        .function("setZLayer", &InteractiveAspectsManager::SetZLayer)
        .function("getTransparency", &InteractiveAspectsManager::GetTransparency)
        .function("setTransparency", &InteractiveAspectsManager::SetTransparency)
        .function("getBaseColor", &InteractiveAspectsManager::GetBaseColor)
        .function("setBaseColor", &InteractiveAspectsManager::SetBaseColor)
        .function("getLineWidth", &InteractiveAspectsManager::GetLineWidth)
        .function("setLineWidth", &InteractiveAspectsManager::SetLineWidth)
        .function("getLineType", &InteractiveAspectsManager::GetLineType)
        .function("setLineType", &InteractiveAspectsManager::SetLineType)
        .function("getTextHeight", &InteractiveAspectsManager::GetTextHeight)
        .function("setTextHeight", &InteractiveAspectsManager::SetTextHeight)
        .function("getTextFont", &InteractiveAspectsManager::GetTextFont)
        .function("setTextFont", &InteractiveAspectsManager::SetTextFont)
        .function("getTextFontAspect", &InteractiveAspectsManager::GetTextFontAspect)
        .function("setTextFontAspect", &InteractiveAspectsManager::SetTextFontAspect)
        .function("getTextJustification", &InteractiveAspectsManager::GetTextJustification)
        .function("setTextJustification", &InteractiveAspectsManager::SetTextJustification);
}
#endif

} // namespace

