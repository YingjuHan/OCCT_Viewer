#include <ViewBase/InteractiveTextLabel.h>

#include <Font_FTFont.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Text.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace ViewBase
{

    InteractiveTextLabel::InteractiveTextLabel(const Handle(ViewBase::InteractiveTextLabelOwner) & theNewId,
                                               const Handle(Prs3d_TextAspect) & theDefTextStyle)
    {
        myOwner = theNewId;
        theNewId->SetSelectablePointer(this);
        theNewId->GetLayeredPresentation(Handle(PrsMgr_PresentationManager)(), 0, true, Graphic3d_ZLayerId_Default);

        myViewOffset = Graphic3d_Vec2i(defaultOffset);
        myInfiniteState = true;

        myLabTrsfPers = new Graphic3d_TransformPers(Graphic3d_TransModeFlags_ZoomRotatePers, gp::Origin());

        myDrawer->SetZLayer(Graphic3d_ZLayerId_Topmost);

        myDrawer->TextAspect()->SetFont(Font_NOF_SANS_SERIF);
        if (!theDefTextStyle.IsNull())
        {
            *myDrawer->TextAspect()->Aspect() = *theDefTextStyle->Aspect();
        }
        myDrawer->TextAspect()->Aspect()->SetTextFontAspect(Font_FontAspect_Regular);
        myDrawer->TextAspect()->SetHeight(ViewBase::BaseDefaultDrawer::m_defTextHeight);
        myDrawer->TextAspect()->SetColor(Quantity_NOC_BLACK);

        myDrawer->SetupOwnShadingAspect();
        myDrawer->ShadingAspect()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
        myDrawer->ShadingAspect()->SetColor(Quantity_NOC_WHITE);
        SetDrawRectangle(true);

        myDrawer->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_EMPTY, Quantity_NOC_WHITE, 1.0f));
        myTopAttachAsp = new Graphic3d_AspectMarker3d(Aspect_TOM_EMPTY, Quantity_NOC_WHITE, 1.0f);
        SetAttachMarker(Aspect_TOM_O_PLUS);

        myDrawer->SetLineAspect(new Prs3d_LineAspect(Quantity_NOC_WHITE, Aspect_TOL_SOLID, 1.0f));
        myDrawer->SetHiddenLineAspect(new Prs3d_LineAspect(Quantity_NOC_WHITE, Aspect_TOL_DASH, 1.0f));

        myShadowAsp = new Graphic3d_Aspects();
        *myShadowAsp = *myDrawer->ShadingAspect()->Aspect();
        myShadowAsp->SetColor(Quantity_NOC_BLACK);

        myIconAsp = new Graphic3d_Aspects();
        myIconAsp->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
        myIconAsp->SetColor(Quantity_NOC_WHITE);
        myIconAsp->SetAlphaMode(Graphic3d_AlphaMode_MaskBlend, 0.285f);
        myIconAsp->SetTextureMapOn(true);

        myDynHilightDrawer = new Prs3d_Drawer();
        myDynHilightDrawer->SetLink(myDrawer);
        myDynHilightDrawer->SetZLayer(Graphic3d_ZLayerId_Topmost);
        myDynHilightDrawer->SetColor(Quantity_NOC_CYAN);
        myDynHilightDrawer->SetTransparency(0.5f);
        myDynHilightDrawer->SetupOwnShadingAspect();
        myDynHilightDrawer->ShadingAspect()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
        myDynHilightDrawer->ShadingAspect()->SetColor(myDynHilightDrawer->Color());
        myDynHilightDrawer->ShadingAspect()->SetTransparency(myDynHilightDrawer->Transparency());

        myTextElem = new Graphic3d_Text((float)myDrawer->TextAspect()->Height());
    }

    InteractiveTextLabel::~InteractiveTextLabel()
    {
        //
    }

    bool InteractiveTextLabel::OnPixelScaleChange(const Handle(AIS_InteractiveObject) & theObj, float theRatio)
    {
        if (!myIconTexture.IsNull() && myIconTexture->SetResolutionScale(theRatio))
        {
            SynchronizeAspects(); // update texture
        }
        return IInteractiveObject::OnPixelScaleChange(theObj, theRatio);
    }

    void InteractiveTextLabel::UpdateAspects()
    {
        InteractiveTextLabelOwner *anOwner = dynamic_cast<InteractiveTextLabelOwner *>(myOwner.get());
        if (anOwner == nullptr || !HasInteractiveContext())
        {
            return;
        }

        const Quantity_Color aSelColor = InteractiveContext()->HighlightStyle(Prs3d_TypeOfHighlight_Selected)->Color();
        const Quantity_Color aMainColor = Quantity_NOC_WHITE;

        myShadowAsp->SetColor(anOwner->IsSelected() ? aSelColor : Quantity_NOC_BLACK);
        AttachPointAspect(false)->SetColor(anOwner->IsSelected() ? aSelColor : aMainColor);
        AttachPointAspect(true)->SetColor(anOwner->IsSelected() ? aSelColor : aMainColor);
        AttachLineAspect(false)->SetColor(anOwner->IsSelected() ? aSelColor : aMainColor);
        AttachLineAspect(true)->SetColor(anOwner->IsSelected() ? aSelColor : aMainColor);
    }

    void InteractiveTextLabel::drawPoint(const Handle(Prs3d_Presentation) & thePrs,
                                         const Handle(Graphic3d_Aspects) & theAspect)
    {
        if (!ToDrawAttachPoint())
        {
            return;
        }

        const gp_Pnt aPnt = HasAttachPoint() ? myAttachPoint : Position();

        Handle(Graphic3d_ArrayOfPoints) aPoint = new Graphic3d_ArrayOfPoints(1);
        aPoint->AddVertex(aPnt);

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(theAspect);
        aGroup->AddPrimitiveArray(aPoint);
    }

    void InteractiveTextLabel::drawAttachLine(const Handle(Prs3d_Presentation) & thePrs,
                                              const Handle(Graphic3d_Aspects) & theAspect)
    {
        if (!HasAttachPoint())
        {
            return;
        }

        const gp_Pnt aPnt1 = myAttachPoint;
        const gp_Pnt aPnt2 = Position();
        if (aPnt1.IsEqual(aPnt2, Precision::Confusion()))
        {
            return;
        }

        static constexpr int nbSegNodes2 = 2;

        Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments(nbSegNodes2);
        aSegs->AddVertex(aPnt1);
        aSegs->AddVertex(aPnt2);

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(theAspect);
        aGroup->AddPrimitiveArray(aSegs);
    }

    Handle(Graphic3d_ArrayOfTriangles) InteractiveTextLabel::DrawRectangle(int theExp)
    {
        const Graphic3d_Vec2d aSize = myTextBBoxSize;

        gp_Ax2 anOrient = myOrientation3D;
        anOrient.SetLocation(myTextBBoxPos);
        gp_Trsf aLabTrsf;
        aLabTrsf.SetTransformation(anOrient);

        const double anOff = myRectPadding + theExp;

        static constexpr int nbQVerts4 = 4;
        static constexpr int nbQTriInd6 = 6;

        static constexpr int qn1 = 1;
        static constexpr int qn2 = 2;
        static constexpr int qn3 = 3;
        static constexpr int qn4 = 4;

        Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(nbQVerts4,
                                                                                  nbQTriInd6,
                                                                                  Graphic3d_ArrayFlags_None);
        aTris->AddVertex(gp_Pnt(0.0 - anOff, 0.0 - anOff, 0.0).Transformed(aLabTrsf));
        aTris->AddVertex(gp_Pnt(0.0 - anOff, aSize.y() + anOff, 0.0).Transformed(aLabTrsf));
        aTris->AddVertex(gp_Pnt(aSize.x() + anOff, aSize.y() + anOff, 0.0).Transformed(aLabTrsf));
        aTris->AddVertex(gp_Pnt(aSize.x() + anOff, 0.0 - anOff, 0.0).Transformed(aLabTrsf));
        aTris->AddQuadTriangleEdges(qn1, qn2, qn3, qn4);
        return aTris;
    }

    void InteractiveTextLabel::drawRectangle(const Handle(Prs3d_Presentation) & thePrs)
    {
        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetTransformPersistence(myLabTrsfPers);

        Handle(Graphic3d_ArrayOfTriangles) aTrisShadow = DrawRectangle(1);

        if (!ToDrawRectangle())
        {
            aGroup->SetPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
            aGroup->AddPrimitiveArray(aTrisShadow);
            return; // just expand bounding box
        }

        Handle(Graphic3d_ArrayOfTriangles) aTris = DrawRectangle();
        aGroup->SetPrimitivesAspect(myShadowAsp);
        aGroup->AddPrimitiveArray(aTrisShadow);
        aGroup->SetPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
        aGroup->AddPrimitiveArray(aTris);
    }

    gp_Pnt InteractiveTextLabel::getTextPoint() const
    {
        static constexpr double halfMult = 0.5;

        gp_Pnt aPos = gp::Origin();
        const double aPad = ToDrawAttachPoint() ? myPointPadding : 0.0;
        Graphic3d_Vec2d anIcon(myIconSize);
        switch (myDrawer->TextAspect()->HorizontalJustification())
        {
        case Graphic3d_HTA_LEFT:
            aPos.SetX(aPad + anIcon.x());
            break;
        case Graphic3d_HTA_RIGHT:
            aPos.SetX(-aPad - anIcon.x());
            break;
        case Graphic3d_HTA_CENTER:
            aPos.SetX(anIcon.x() * halfMult);
            break;
        }
        switch (myDrawer->TextAspect()->VerticalJustification())
        {
        case Graphic3d_VTA_BOTTOM:
            aPos.SetY(aPad);
            break;
        case Graphic3d_VTA_TOP:
            aPos.SetY(-aPad);
            break;
        case Graphic3d_VTA_CENTER:
            break;
        case Graphic3d_VTA_TOPFIRSTLINE:
            break;
        }
        return aPos;
    }

    bool InteractiveTextLabel::calculateLabelParams(gp_Pnt &thePos,
                                                    Graphic3d_Vec2d &theTextSize,
                                                    Graphic3d_Vec2d &theRectSize,
                                                    const Handle(Graphic3d_Text) & theText,
                                                    const Graphic3d_RenderingParams &theRendParams,
                                                    const Handle(Graphic3d_AspectText3d) & theAsp)
    {
        Font_FTFontParams aFontParams;
        aFontParams.PointSize = (unsigned int)theText->Height();
        aFontParams.Resolution = theRendParams.BaseResolution;
        aFontParams.FontHinting = theRendParams.FontHinting;
        Handle(Font_FTFont) aFont = Font_FTFont::FindAndCreate(theAsp->Font(), theAsp->GetTextFontAspect(), aFontParams);
        if (aFont.IsNull())
        {
            return false;
        }

        const double aPntOffset = (ToDrawAttachPoint() ? myPointPadding : 0.0f);

        const Font_Rect aBndBox = aFont->BoundingBox(theText->Text(),
                                                     theText->HorizontalAlignment(),
                                                     theText->VerticalAlignment());

        theTextSize.SetValues(Abs(aBndBox.Width()), Abs(aBndBox.Height()));

        theRectSize.x() = theTextSize.x() + double(myIconSize.x());
        theRectSize.y() = Max(theTextSize.y(), double(myIconSize.y()));

        static constexpr double halfMult = 0.5;
        switch (theText->HorizontalAlignment())
        {
        case Graphic3d_HTA_LEFT:
            thePos.SetX(thePos.X() - aPntOffset);
            break;
        case Graphic3d_HTA_RIGHT:
            thePos.SetX(thePos.X() + theRectSize.x() + aPntOffset);
            break;
        case Graphic3d_HTA_CENTER:
            thePos.SetX(thePos.X() + theRectSize.x() * halfMult);
            break;
        }
        switch (theText->VerticalAlignment())
        {
        case Graphic3d_VTA_BOTTOM:
            thePos.SetY(thePos.Y() - aFont->Descender() - aPntOffset);
            break;
        case Graphic3d_VTA_TOP:
            thePos.SetY(thePos.Y() + theRectSize.y() + aPntOffset);
            break;
        case Graphic3d_VTA_CENTER:
            thePos.SetY(thePos.Y() + theRectSize.y() * halfMult);
            break;
        case Graphic3d_VTA_TOPFIRSTLINE:
            break;
        }
        return true;
    }

    void InteractiveTextLabel::updateText()
    {
        static constexpr double halfMult = 0.5;

        if (myWrappingLength > 0.0f)
        {
            if (myFormatter.IsNull())
            {
                myFormatter = new Font_TextFormatter();
            }
            myFormatter->SetWrapping(myWrappingLength);
        }

        gp_Pnt aPos = getTextPoint();

        myTextElem->SetHeight((float)myDrawer->TextAspect()->Height());
        myTextElem->SetText(myText.ToExtString());
        myTextElem->SetPosition(aPos);
        myTextElem->SetHorizontalAlignment(myDrawer->TextAspect()->HorizontalJustification());
        myTextElem->SetVerticalAlignment(myDrawer->TextAspect()->VerticalJustification());
        myTextElem->SetTextFormatter(myFormatter);

        Graphic3d_Vec2d aTextSize(0.0);
        myTextBBoxPos.SetCoord(0.0, 0.0, 0.0);
        myTextBBoxSize.SetValues(0.0, 0.0);
        const Graphic3d_RenderingParams &aRendParams = GetContext()->CurrentViewer()->DefaultRenderingParams();
        calculateLabelParams(myTextBBoxPos,
                             aTextSize,
                             myTextBBoxSize,
                             myTextElem,
                             aRendParams,
                             myDrawer->TextAspect()->Aspect());

        const double anIconGap = double(myIconSize.y()) - Abs(aTextSize.y());
        if (anIconGap > 0.0)
        {
            if (myTextElem->VerticalAlignment() == Graphic3d_VTA_TOP)
            {
                aPos.SetY(aPos.Y() - anIconGap * halfMult);
            }
            else if (myTextElem->VerticalAlignment() == Graphic3d_VTA_BOTTOM)
            {
                aPos.SetY(aPos.Y() + anIconGap * halfMult);
            }
            myTextElem->SetPosition(aPos);
        }
    }

    void InteractiveTextLabel::drawText(const Handle(Prs3d_Presentation) & thePrs)
    {
        Handle(Graphic3d_Group) aTextGroup = thePrs->NewGroup();
        aTextGroup->SetTransformPersistence(myLabTrsfPers);
        aTextGroup->SetGroupPrimitivesAspect(myDrawer->TextAspect()->Aspect());
        aTextGroup->AddText(myTextElem);
    }

    void InteractiveTextLabel::drawIcon(const Handle(Prs3d_Presentation) & thePrs)
    {
        if (myIconTexture.IsNull())
        {
            return;
        }

        static constexpr int aGap = 0;
        static constexpr double halfMult = 0.5;

        const Graphic3d_Vec2d aSize = myTextBBoxSize;

        gp_XYZ aPos = getTextPoint().XYZ();
        switch (myTextElem->HorizontalAlignment())
        {
        case Graphic3d_HTA_LEFT:
            aPos.SetX(aPos.X() - double(myIconSize.x()));
            break;
        case Graphic3d_HTA_RIGHT:
            break;
        case Graphic3d_HTA_CENTER:
            aPos.SetX(aPos.X() - (aSize.x() + double(myIconSize.x())) * halfMult);
            break;
        }
        switch (myTextElem->VerticalAlignment())
        {
        case Graphic3d_VTA_BOTTOM:
            break;
        case Graphic3d_VTA_TOPFIRSTLINE:
        case Graphic3d_VTA_TOP:
            aPos.SetY(aPos.Y() - myIconSize.y());
            break;
        case Graphic3d_VTA_CENTER:
            break;
        }

        static constexpr int nbQVerts4 = 4;
        static constexpr int nbQTriInd6 = 6;

        static constexpr int qn1 = 1;
        static constexpr int qn2 = 2;
        static constexpr int qn3 = 3;
        static constexpr int qn4 = 4;

        Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(nbQVerts4,
                                                                                  nbQTriInd6,
                                                                                  Graphic3d_ArrayFlags_VertexTexel);
        aTris->AddVertex(aPos + gp_XYZ(0.0 + aGap, 0.0 - aGap, 0.0), gp_Pnt2d(0.0, 0.0));
        aTris->AddVertex(aPos + gp_XYZ(0.0 + aGap, myIconSize.y() + aGap, 0.0), gp_Pnt2d(0.0, 1.0));
        aTris->AddVertex(aPos + gp_XYZ(myIconSize.x() + aGap, myIconSize.y() + aGap, 0.0), gp_Pnt2d(1.0, 1.0));
        aTris->AddVertex(aPos + gp_XYZ(myIconSize.x() + aGap, 0.0 - aGap, 0.0), gp_Pnt2d(1.0, 0.0));
        aTris->AddQuadTriangleEdges(qn1, qn2, qn3, qn4);

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetTransformPersistence(myLabTrsfPers);
        aGroup->SetGroupPrimitivesAspect(myIconAsp);
        aGroup->AddPrimitiveArray(aTris);
    }

    void InteractiveTextLabel::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                       const Handle(Prs3d_Presentation) & thePrs,
                                       const Standard_Integer theMode)
    {
        if (!AcceptDisplayMode(theMode))
        {
            return;
        }

        myLabTrsfPers->SetAnchorPoint(Position());

        myIconSize = Graphic3d_Vec2i(0);
        if (!myIconTexture.IsNull())
        {
            myIconSize = myIconTexture->BaseSize();
            myIconTexture->SetResolutionScale(myDevicePixelRatio);
        }

        const bool toUseExtPrs = myDrawer->ZLayer() == Graphic3d_ZLayerId_Topmost;

        Handle(PrsMgr_Presentation) aDefPrs;
        if (Handle(InteractiveTextLabelOwner) anOwner = Handle(InteractiveTextLabelOwner)::DownCast(myOwner))
        {
            aDefPrs = anOwner->GetLayeredPresentation(thePrsMgr, theMode, toUseExtPrs, Graphic3d_ZLayerId_Default);
        }

        updateText();

        drawPoint(toUseExtPrs ? aDefPrs : thePrs, AttachPointAspect(false));
        drawAttachLine(toUseExtPrs ? aDefPrs : thePrs, AttachLineAspect(false));
        if (toUseExtPrs)
        {
            drawPoint(thePrs, AttachPointAspect(true));
            drawAttachLine(thePrs, AttachLineAspect(true));
        }
        drawRectangle(thePrs);
        drawText(thePrs);
        drawIcon(thePrs);

        // workaround OCCT bug with AIS_InteractiveContext::Load() -> ::Display()
        // not updating selection object considering HasGroupTransformPersistence() changes
        InteractiveContext()->SelectionManager()->UpdateSelection(this);
    }

    void InteractiveTextLabel::ComputeSelection(const Handle(SelectMgr_Selection) & theSel, const Standard_Integer theMode)
    {
        if (theMode != 0)
        {
            return;
        }

        Handle(InteractiveTextLabelOwner) anOwner = Handle(InteractiveTextLabelOwner)::DownCast(myOwner);
        if (anOwner.IsNull())
        {
            return;
        }

        anOwner->SetSelectablePointer(this);

        Handle(Graphic3d_ArrayOfTriangles) aTris = DrawRectangle();
        Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(anOwner);
        aSens->SetTransformPersistence(myLabTrsfPers);
        aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
        theSel->Add(aSens);
    }

}