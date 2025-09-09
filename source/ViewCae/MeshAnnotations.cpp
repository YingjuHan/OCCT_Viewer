#include <ViewCae/MeshAnnotations.h>

#include <ViewCae/InteractiveCaeMesh.h>

#include <ViewBase/ScalableTexture2D.h>

#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfTriangleFans.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_Text.hxx>
#include <Graphic3d_Texture2D.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <V3d_Viewer.hxx>

namespace {
constexpr Graphic3d_HorizontalTextAlignment DEFAULT_HOR_TEXT_ALIGNMENT  = Graphic3d_HTA_LEFT;
constexpr Graphic3d_VerticalTextAlignment   DEFAULT_VERT_TEXT_ALIGNMENT = Graphic3d_VTA_CENTER;

constexpr float DATA_OFFSET      = 10.0f;
constexpr float FILL_MARGIN      = 2.0f;
constexpr float TEXTURE_SIZE     = 35.0f;
constexpr float SPACING          = 5.0f;
constexpr float HIGHLIGHT_MARGIN = 2.0f;
}

namespace ViewCae {
// =======================================================================
// function : HilightWithColor
// =======================================================================
void MeshAnnotationLineOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                               const Handle(Prs3d_Drawer)&               theStyle,
                                               const Standard_Integer                    theMode)
{
    InteractiveCaeMesh* anObj = dynamic_cast<InteractiveCaeMesh*>(mySelectable->Parent());
    if (anObj == nullptr) {
        InteractiveOwner::HilightWithColor(thePrsMgr, theStyle, theMode);
        return;
    }
    if (!anObj->IsEnabled()) {
        return;
    }

    const bool isSelection = !thePrsMgr->IsImmediateModeOn();

    Handle(Prs3d_Presentation) aPrs = GetHighlightPresentation(thePrsMgr, theStyle, isSelection);
    aPrs->SetZLayer(Graphic3d_ZLayerId_Topmost);

    Handle(Graphic3d_ArrayOfSegments) aHighPrim = new Graphic3d_ArrayOfSegments(4, 8, Graphic3d_ArrayFlags_None);
    aHighPrim->AddVertex(myRect.Left - HIGHLIGHT_MARGIN, myRect.Bottom - HIGHLIGHT_MARGIN, 0.0f);
    aHighPrim->AddVertex(myRect.Right + HIGHLIGHT_MARGIN, myRect.Bottom - HIGHLIGHT_MARGIN, 0.0f);
    aHighPrim->AddVertex(myRect.Right + HIGHLIGHT_MARGIN, myRect.Top + HIGHLIGHT_MARGIN, 0.0f);
    aHighPrim->AddVertex(myRect.Left - HIGHLIGHT_MARGIN, myRect.Top + HIGHLIGHT_MARGIN, 0.0f);
    aHighPrim->AddEdges(1, 2);
    aHighPrim->AddEdges(2, 3);
    aHighPrim->AddEdges(3, 4);
    aHighPrim->AddEdges(4, 1);

    for (const gp_Pnt& aPos : myPositions) {
        Handle(Graphic3d_Group) aGroup = aPrs->NewGroup();
        aGroup->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_ZoomRotatePers, aPos));
        aGroup->SetGroupPrimitivesAspect(theStyle->ShadingAspect()->Aspect());
        aGroup->AddPrimitiveArray(aHighPrim);
    }

    if (!isSelection) {
        thePrsMgr->AddToImmediateList(aPrs);
    } else {
        aPrs->Display();
    }
}

// =======================================================================
// function : Unhilight
// =======================================================================
void MeshAnnotationLineOwner::Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Standard_Integer                    theMode)
{
    if (Handle(Prs3d_Presentation) aPrs = mySelectable->GetSelectPresentation(Handle(PrsMgr_PresentationManager)())) {
        aPrs->Erase();
    }
    if (Handle(Prs3d_Presentation) aPrs = mySelectable->GetHilightPresentation(Handle(PrsMgr_PresentationManager)())) {
        aPrs->Erase();
    }
    SelectMgr_EntityOwner::Unhilight(thePrsMgr, theMode);
}

// =======================================================================
// function : Compute
// =======================================================================
void MeshAnnotations::Compute(const Handle(AIS_InteractiveContext)&     theCtx,
                              const Handle(Prs3d_Presentation)&         thePrs,
                              const Handle(Prs3d_Presentation)&         theTopPrs,
                              const Handle(Prs3d_TextAspect)&           theTextAspect,
                              const Handle(Graphic3d_AspectFillArea3d)& theFillAspect,
                              const Handle(Graphic3d_AspectMarker3d)&   theTopPointAspect,
                              const Handle(Graphic3d_AspectMarker3d)&   thePointAspect,
                              float                                     theResolutionRatio) const
{
    if (theCtx.IsNull() || myAnnotations.empty()) {
        return;
    }

    Handle(Font_FTFont) aFont = getFont(theCtx, theTextAspect);
    if (aFont.IsNull()) {
        return;
    }

    Handle(Graphic3d_Group) aTopPointGroup = theTopPrs->NewGroup();
    Handle(Graphic3d_Group) aPointGroup    = thePrs->NewGroup();

    auto processAnnotationInst = [&](const MeshAnnotation&                               theAnnotation,
                                     const gp_Pnt&                                       thePos,
                                     const std::vector<std::pair<Font_Rect, Font_Rect>>& theAnnSizes) {
        Handle(Graphic3d_ArrayOfPoints) aPoint = new Graphic3d_ArrayOfPoints(1);
        aPoint->AddVertex(thePos);
        aTopPointGroup->SetPrimitivesAspect(theTopPointAspect);
        aTopPointGroup->AddPrimitiveArray(aPoint);
        aPointGroup->SetPrimitivesAspect(thePointAspect);
        aPointGroup->AddPrimitiveArray(aPoint);

        for (int i = 0; i < (int)theAnnotation.Data().size(); i++) {
            float aHeight = Max(theAnnSizes[i].first.Height(), theAnnSizes[i].second.Height());
            if (aHeight == 0.0f) {
                continue;
            }

            computeIcon(theAnnotation, thePos, i, theAnnSizes[i].second, theTopPrs, theResolutionRatio);
            computeText(theAnnotation, thePos, i, theAnnSizes[i].first, theTopPrs, theTextAspect, theFillAspect);
        }
    };

    for (const MeshAnnotation& anAnnotation : myAnnotations) {
        if (!anAnnotation.IsVisible()) {
            continue;
        }

        std::vector<std::pair<Font_Rect, Font_Rect>> aSizes;
        computeAnnotationSizes(anAnnotation, aFont, aSizes);

        for (const gp_Pnt& aPos : anAnnotation.Positions()) {
            processAnnotationInst(anAnnotation, aPos, aSizes);
        }
    }
}

// =======================================================================
// function : calculateTextSize
// =======================================================================
Font_Rect MeshAnnotations::calculateTextSize(const TCollection_AsciiString& theText,
                                             const Handle(Font_FTFont)&     theFont) const
{
    const NCollection_String aText(theText.ToCString());
    return theFont->BoundingBox(aText, DEFAULT_HOR_TEXT_ALIGNMENT, DEFAULT_VERT_TEXT_ALIGNMENT);
}

// =======================================================================
// function : getFont
// =======================================================================
Handle(Font_FTFont) MeshAnnotations::getFont(const Handle(AIS_InteractiveContext)& theCtx,
                                             const Handle(Prs3d_TextAspect)&       theTextAspect) const
{
    if (!myFont.IsNull()) {
        return myFont;
    }

    const Graphic3d_RenderingParams& aRendParams = theCtx->CurrentViewer()->DefaultRenderingParams();

    Font_FTFontParams aFontParams;
    aFontParams.PointSize   = (unsigned int)theTextAspect->Height();
    aFontParams.Resolution  = aRendParams.BaseResolution;
    aFontParams.FontHinting = aRendParams.FontHinting;

    myFont = Font_FTFont::FindAndCreate(theTextAspect->Aspect()->Font(),
                                        theTextAspect->Aspect()->GetTextFontAspect(),
                                        aFontParams);
    return myFont;
}

// =======================================================================
// function : computeAnnotationSizes
// =======================================================================
void MeshAnnotations::computeAnnotationSizes(const MeshAnnotation&                         theAnnotation,
                                             const Handle(Font_FTFont)&                    theFont,
                                             std::vector<std::pair<Font_Rect, Font_Rect>>& theSizes) const
{
    float aTotalHeight = 0.0f;
    for (const MeshAnnotationData& aData : theAnnotation.Data()) {
        Font_Rect aTextSize = {};
        if (theAnnotation.IsText() && !aData.Text.IsEmpty()) {
            aTextSize = calculateTextSize(aData.Text, theFont);
            aTextSize.Top += FILL_MARGIN * 2.0f;
            aTextSize.Right += FILL_MARGIN * 2.0f;
        }
        Font_Rect anIconSize = {};
        if (theAnnotation.IsIcon() && !aData.Icon.IsEmpty() && !getTextureAspect(aData.Icon).IsNull()) {
            anIconSize.Top   = TEXTURE_SIZE;
            anIconSize.Right = TEXTURE_SIZE;
        }

        const float aHeight = Max(aTextSize.Height(), anIconSize.Height());
        if (aHeight > 0.0f) {
            aTotalHeight += aHeight + SPACING;
        }

        theSizes.push_back(std::make_pair(aTextSize, anIconSize));
    }

    if (aTotalHeight == 0.0f) {
        return;
    }

    float aHeightOffset = aTotalHeight + DATA_OFFSET;
    for (std::pair<Font_Rect, Font_Rect>& aSizes : theSizes) {
        const float aTextHeight  = aSizes.first.Height();
        const float anIconHeight = aSizes.second.Height();
        const float aHeight      = Max(aTextHeight, anIconHeight);
        if (aHeight == 0.0f) {
            continue;
        }

        Graphic3d_Vec2 aPos(DATA_OFFSET, aHeightOffset - SPACING - aHeight * 0.5f);
        aHeightOffset -= aHeight + SPACING;

        if (anIconHeight > 0.0f) {
            aSizes.second.Top    = aPos.y() + anIconHeight * 0.5f;
            aSizes.second.Bottom = aPos.y() - anIconHeight * 0.5f;
            aSizes.second.Left += aPos.x();
            aSizes.second.Right += aPos.x();
            aPos.x() += aSizes.second.Width() + SPACING;
        }

        if (aTextHeight > 0.0f) {
            aSizes.first.Top    = aPos.y() + aTextHeight * 0.5f;
            aSizes.first.Bottom = aPos.y() - aTextHeight * 0.5f;
            aSizes.first.Left += aPos.x();
            aSizes.first.Right += aPos.x();
        }
    }
}

// =======================================================================
// function : getTextureAspect
// =======================================================================
Handle(Graphic3d_AspectFillArea3d) MeshAnnotations::getTextureAspect(const TCollection_AsciiString& theIcon) const
{
    Handle(Graphic3d_AspectFillArea3d) aTextureAspect;

    if (!myTextureAspects.Find(theIcon, aTextureAspect)) {
        Handle(ViewBase::ScalablePixMap) aPixmap = new ViewBase::ScalablePixMap(theIcon);
        if (aPixmap->BaseSize() != Graphic3d_Vec2i(0)) {
            Handle(ViewBase::ScalableTexture2D) aTexture = new ViewBase::ScalableTexture2D(aPixmap);

            aTextureAspect = new Graphic3d_AspectFillArea3d();
            aTextureAspect->SetInteriorStyle(Aspect_IS_SOLID);
            aTextureAspect->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
            aTextureAspect->SetColor(Quantity_NOC_WHITE);
            aTextureAspect->SetAlphaMode(Graphic3d_AlphaMode_MaskBlend, 0.285f);
            aTextureAspect->SetTextureMap(aTexture);
            aTextureAspect->SetTextureMapOn();
            myTextureAspects.Bind(theIcon, aTextureAspect);
        }
    }

    return aTextureAspect;
}

// =======================================================================
// function : computeIcon
// =======================================================================
void MeshAnnotations::computeIcon(const MeshAnnotation&             theAnnotation,
                                  const gp_Pnt&                     thePos,
                                  int                               theLine,
                                  const Font_Rect&                  theSize,
                                  const Handle(Prs3d_Presentation)& theTopPrs,
                                  float                             theResolutionRatio) const
{
    if (theSize.Height() == 0.0f) {
        return;
    }

    const MeshAnnotationData& aData = theAnnotation.Data()[theLine];

    const Handle(Graphic3d_AspectFillArea3d)& aTextureAspect = myTextureAspects.Find(aData.Icon);

    Handle(ViewBase::ScalableTexture2D)::DownCast(aTextureAspect->TextureMap())
        ->SetResolutionScale(theResolutionRatio);

    Handle(Graphic3d_Group) aTextureGroup = theTopPrs->NewGroup();
    aTextureGroup->SetPrimitivesAspect(aTextureAspect);
    aTextureGroup->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_ZoomRotatePers, thePos));

    Handle(Graphic3d_ArrayOfTriangleFans) aTriangles =
        new Graphic3d_ArrayOfTriangleFans(4, 0, false, false, false, true);
    aTriangles->AddVertex(gp_Pnt((double)theSize.Left, (double)theSize.Bottom, 0.0), gp_Pnt2d(0.0, 0.0));
    aTriangles->AddVertex(gp_Pnt((double)theSize.Right, (double)theSize.Bottom, 0.0), gp_Pnt2d(1.0, 0.0));
    aTriangles->AddVertex(gp_Pnt((double)theSize.Right, (double)theSize.Top, 0.0), gp_Pnt2d(1.0, 1.0));
    aTriangles->AddVertex(gp_Pnt((double)theSize.Left, (double)theSize.Top, 0.0), gp_Pnt2d(0.0, 1.0));
    aTextureGroup->AddPrimitiveArray(aTriangles);
}

// =======================================================================
// function : computeText
// =======================================================================
void MeshAnnotations::computeText(const MeshAnnotation&                     theAnnotation,
                                  const gp_Pnt&                             thePos,
                                  int                                       theLine,
                                  const Font_Rect&                          theSize,
                                  const Handle(Prs3d_Presentation)&         theTopPrs,
                                  const Handle(Prs3d_TextAspect)&           theTextAspect,
                                  const Handle(Graphic3d_AspectFillArea3d)& theFillAspect) const
{
    const MeshAnnotationData& aData = theAnnotation.Data()[theLine];

    if (!theAnnotation.IsText() || aData.Text.IsEmpty()) {
        return;
    }

    Handle(Graphic3d_Group) aGroup = theTopPrs->NewGroup();
    aGroup->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_ZoomRotatePers, thePos));
    if (myToDrawTextBack) {
        aGroup->SetPrimitivesAspect(theFillAspect);

        Handle(Graphic3d_ArrayOfTriangleFans) aTriangles = new Graphic3d_ArrayOfTriangleFans(4);
        aTriangles->AddVertex(theSize.Left, theSize.Bottom, 0.0f);
        aTriangles->AddVertex(theSize.Right, theSize.Bottom, 0.0f);
        aTriangles->AddVertex(theSize.Right, theSize.Top, 0.0f);
        aTriangles->AddVertex(theSize.Left, theSize.Top, 0.0f);
        aGroup->AddPrimitiveArray(aTriangles);
    }

    aGroup->SetPrimitivesAspect(theTextAspect->Aspect());

    Handle(Graphic3d_Text) aText = new Graphic3d_Text((float)theTextAspect->Height());
    aText->SetText(aData.Text);
    aText->SetPosition(gp_Pnt((double)(theSize.Left + FILL_MARGIN), (double)(theSize.Bottom + theSize.Top) * 0.5, 0.0));
    aText->SetOwnAnchorPoint(false);
    aText->SetHorizontalAlignment(DEFAULT_HOR_TEXT_ALIGNMENT);
    aText->SetVerticalAlignment(DEFAULT_VERT_TEXT_ALIGNMENT);
    aGroup->AddText(aText);
}

// =======================================================================
// function : ComputeSelection
// =======================================================================
void MeshAnnotations::ComputeSelection(const Handle(ViewBase::InteractiveOwner)&      theOwner,
                                       const Handle(Prs3d_TextAspect)&                 theTextAspect,
                                       NCollection_Map<Handle(Select3D_SensitiveSet)>& theSensitiveSets) const
{
    if (myAnnotations.empty()) {
        return;
    }

    Handle(InteractiveCaeMesh) anObj = Handle(InteractiveCaeMesh)::DownCast(theOwner->Selectable());
    if (anObj.IsNull() || !anObj->HasInteractiveContext()) {
        return;
    }

    Handle(Font_FTFont) aFont = getFont(anObj->InteractiveContext(), theTextAspect);
    if (aFont.IsNull()) {
        return;
    }

    for (MeshAnnotation& anAnnotation : myAnnotations) {
        anAnnotation.LineOwners().Clear();

        if (!anAnnotation.IsVisible()) {
            continue;
        }

        std::vector<std::pair<Font_Rect, Font_Rect>> aSizes;
        computeAnnotationSizes(anAnnotation, myFont, aSizes);

        for (int i = 0; i < (int)anAnnotation.Data().size(); i++) {
            computeSelection(theOwner, anAnnotation, aSizes[i].first, aSizes[i].second, i, theSensitiveSets);
        }
    }
}

// =======================================================================
// function : computeSelection
// =======================================================================
void MeshAnnotations::computeSelection(const Handle(ViewBase::InteractiveOwner)&      theOwner,
                                       MeshAnnotation&                                 theAnnotation,
                                       const Font_Rect&                                theTextSize,
                                       const Font_Rect&                                theIconSize,
                                       int                                             theLineIndex,
                                       NCollection_Map<Handle(Select3D_SensitiveSet)>& theSensitiveSets) const
{
    float aHeight = Max(theTextSize.Height(), theIconSize.Height());
    if (aHeight == 0.0f) {
        return;
    }

    Font_Rect aLineRect;
    if (theTextSize.Height() == 0.0f) {
        aLineRect = theIconSize;
    } else if (theIconSize.Height() == 0.0f) {
        aLineRect = theTextSize;
    } else {
        aLineRect.Left   = Min(theTextSize.Left, theIconSize.Left);
        aLineRect.Right  = Max(theTextSize.Right, theIconSize.Right);
        aLineRect.Bottom = Min(theTextSize.Bottom, theIconSize.Bottom);
        aLineRect.Top    = Max(theTextSize.Top, theIconSize.Top);
    }

    Handle(MeshAnnotationLineOwner) aLineOwner = new MeshAnnotationLineOwner(theOwner->Id(),
                                                                             aLineRect,
                                                                             theAnnotation.Positions());
    aLineOwner->SetSelectablePointer(
        dynamic_cast<AIS_InteractiveObject*>(theOwner->Selectable()->Children().First().get()));

    theAnnotation.LineOwners().Bind(theLineIndex, aLineOwner);

    Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(4, 6, Graphic3d_ArrayFlags_None);
    aTris->AddVertex(aLineRect.Left, aLineRect.Bottom, 0.0f);
    aTris->AddVertex(aLineRect.Right, aLineRect.Bottom, 0.0f);
    aTris->AddVertex(aLineRect.Right, aLineRect.Top, 0.0f);
    aTris->AddVertex(aLineRect.Left, aLineRect.Top, 0.0f);
    aTris->AddQuadTriangleEdges(1, 2, 3, 4);

    for (const gp_Pnt& aPos : theAnnotation.Positions()) {
        Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(aLineOwner);
        aSens->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_ZoomRotatePers, aPos));
        aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
        theSensitiveSets.Add(aSens);
    }
}

// =======================================================================
// function : UnhilightAll
// =======================================================================
void MeshAnnotations::UnhilightAll(const Handle(PrsMgr_PresentationManager)& thePrsMgr)
{
    for (MeshAnnotation& anAnnotation : myAnnotations) {
        for (NCollection_DataMap<int, Handle(MeshAnnotationLineOwner)>::Iterator aLineIter(anAnnotation.LineOwners());
             aLineIter.More();
             aLineIter.Next()) {
            aLineIter.Value()->Unhilight(thePrsMgr, 0);
        }
    }
}

// =======================================================================
// function : Annotation
// =======================================================================
MeshAnnotation* MeshAnnotations::Annotation(int theIndex) const
{
    for (MeshAnnotation& anAnnotation : myAnnotations) {
        if (anAnnotation.Index() == theIndex) {
            return &anAnnotation;
        }
    }

    return nullptr;
}

} // namespace



