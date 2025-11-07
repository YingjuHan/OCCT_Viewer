#include <ViewCae/InteractiveColorScale.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveColorScaleOwner.h>
#include <ViewCae/MeshColorScaleMapper.h>

#include <ViewBase/BaseDefaultDrawer.h>

#include <Graphic3d_ArrayOfPolylines.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_Text.hxx>
#include <V3d_View.hxx>
#include <Select3D_SensitivePoint.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>

namespace ViewCae {

//! For altering behaviour of symlog near zero region. Could be useful to increase if all values are close to 0.
static constexpr double aSymLogNormConstant = 1;

//! Method to add colored quad into array of triangles.
static void addColoredQuadEx(const Handle(Graphic3d_ArrayOfTriangles)& theTris,
                             const Graphic3d_Vec2i&                    theXY, // left bottom
                             const Graphic3d_Vec2i&                    theSize,
                             const Quantity_ColorRGBA&                 theColBot,
                             const Quantity_ColorRGBA&                 theColTop)
{
    static constexpr double ubMax = 255.0;

    Graphic3d_Vec4ub aColBot(Standard_Byte(theColBot.GetRGB().Red() * ubMax),
                             Standard_Byte(theColBot.GetRGB().Green() * ubMax),
                             Standard_Byte(theColBot.GetRGB().Blue() * ubMax),
                             Standard_Byte(theColBot.Alpha() * ubMax));
    Graphic3d_Vec4ub aColTop(Standard_Byte(theColTop.GetRGB().Red() * ubMax),
                             Standard_Byte(theColTop.GetRGB().Green() * ubMax),
                             Standard_Byte(theColTop.GetRGB().Blue() * ubMax),
                             Standard_Byte(theColTop.Alpha() * ubMax));

    const int aVertIndex = theTris->VertexNumber() + 1;
    theTris->AddVertex(gp_Pnt(theXY.x(), theXY.y() + theSize.y(), 0.0), aColTop);
    theTris->AddVertex(gp_Pnt(theXY.x(), theXY.y(), 0.0), aColBot);
    theTris->AddVertex(gp_Pnt(theXY.x() + theSize.x(), theXY.y(), 0.0), aColBot);
    theTris->AddVertex(gp_Pnt(theXY.x() + theSize.x(), theXY.y() + theSize.y(), 0.0), aColTop);
    theTris->AddEdges(aVertIndex, aVertIndex + 1, aVertIndex + 2);
    theTris->AddEdges(aVertIndex, aVertIndex + 2, aVertIndex + 3);
}

//! Return quad sensitive.
static Handle(Select3D_SensitivePrimitiveArray) getQuadSensitivity(const Handle(SelectMgr_EntityOwner)& theOwnerId,
                                                                   const Graphic3d_Vec2i& theXY, // left bottom
                                                                   const Graphic3d_Vec2i& theSize)
{
    static constexpr int nbQVerts4  = 4;
    static constexpr int nbQTriInd6 = 6;

    Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(nbQVerts4,
                                                                              nbQTriInd6,
                                                                              Graphic3d_ArrayFlags_None);
    addColoredQuadEx(aTris,
                     theXY,
                     theSize,
                     Quantity_ColorRGBA(Quantity_NOC_WHITE),
                     Quantity_ColorRGBA(Quantity_NOC_WHITE));
    Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(theOwnerId);
    aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
    return aSens;
}

// ================================================================
// Function : InteractiveColorScale
// ================================================================
InteractiveColorScale::InteractiveColorScale(const Handle(ViewCae::InteractiveColorScaleOwner)& theNewId)
{
    myOwner     = theNewId;
    myMeshOwner = theNewId;

    SetTextHeight(ViewBase::BaseDefaultDrawer::m_defTextHeight);

    Graphic3d_MaterialAspect anOutMat(Graphic3d_NameOfMaterial_Transparent);
    myOutOfRangeAsp = new Graphic3d_AspectFillArea3d(Aspect_IS_SOLID,
                                                     Quantity_NOC_WHITE,
                                                     Quantity_NOC_BLACK,
                                                     Aspect_TOL_SOLID,
                                                     1.0f,
                                                     anOutMat,
                                                     anOutMat);
    myOutOfRangeAsp->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
    myOutOfRangeAsp->SetInteriorColor(Quantity_ColorRGBA(Graphic3d_Vec4(0.5f, 0.5f, 0.5f, 1.0f)));
    myOutOfRangeAsp->SetAlphaMode(Graphic3d_AlphaMode_MaskBlend, 0.5f);

    myDrawer->SetupOwnShadingAspect();
    myDrawer->ShadingAspect()->Aspect()->SetAlphaMode(Graphic3d_AlphaMode_MaskBlend, 0.1f);

    myDynHilightDrawer = new Prs3d_Drawer();
    myDynHilightDrawer->Link(myDrawer);
    myDynHilightDrawer->SetColor(Quantity_NOC_CYAN1);
    myDynHilightDrawer->SetTransparency(0.5f);
    myDynHilightDrawer->SetDisplayMode((int)ColorScaleDispMode::Frame);
    myDynHilightDrawer->SetZLayer(Graphic3d_ZLayerId_TopOSD);

    myIntervalHighlightColor = Quantity_NOC_YELLOW;

    updateSize(false);
}

// ================================================================
// Function : ~InteractiveColorScale
// ================================================================
InteractiveColorScale::~InteractiveColorScale()
{
    //
}

// ================================================================
// Function : Attach
// ================================================================
void InteractiveColorScale::Attach(const Handle(InteractiveCaeMesh)&                          theMeshIO,
                                   const RWCae::MeshPropertyComponent&                      theProp,
                                   const std::function<void(const Handle(MeshColorMapper)&)>& theCallback)
{
    Handle(MeshColorScaleMapper) aColorMapper = new MeshColorScaleMapper(this);
    aColorMapper->SetOnPropertyUpdateCallback(theCallback);
    theMeshIO->SetColorMapper(aColorMapper, theProp);
    myMeshOwner->SetColorMapper(aColorMapper);
}

// ================================================================
// Function : UpdateColorMap
// ================================================================
void InteractiveColorScale::UpdateColorMap(bool theToUpdateRange)
{
    if (myMeshOwner->ColorMapper().IsNull()) {
        return;
    }

    myMeshOwner->ColorMapper()->UpdateColorTexture();
    if (!HasInteractiveContext()) {
        return;
    }

    // bad luck - could we keep id of attached mesh objects to update them?
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = InteractiveContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (anObj->DisplayStatus() != PrsMgr_DisplayStatus_Displayed) {
            continue;
        }

        if (ViewCae::InteractiveCaeMesh* anIIObj = dynamic_cast<ViewCae::InteractiveCaeMesh*>(anObj.get())) {
            anIIObj->UpdateAspects();
            if (theToUpdateRange) {
                anIIObj->UpdateMinMaxValues(myMeshOwner->ColorMapper());
            }
            InvalidateViewer(anObj);
        }
    }
}

// ================================================================
// Function : SetColor
// ================================================================
void InteractiveColorScale::SetColor(const Quantity_Color& theColor)
{
    if (hasOwnColor && myDrawer->Color().IsEqual(theColor)) {
        return;
    }

    AIS_ColorScale::SetColor(theColor);
    SetToUpdate();
}

// ================================================================
// Function : UnsetColor
// ================================================================
void InteractiveColorScale::UnsetColor()
{
    if (!hasOwnColor) {
        return;
    }

    AIS_ColorScale::UnsetColor();
    SetToUpdate();
}

// ================================================================
// Function : updateTextAspectEx
// ================================================================
void InteractiveColorScale::updateTextAspectEx()
{
    // update text aspect
    if (!myDrawer->HasOwnTextAspect()) {
        const Quantity_Color aFgColor(hasOwnColor ? myDrawer->Color() : Quantity_NOC_WHITE);
        myDrawer->SetTextAspect(new Prs3d_TextAspect());
        *myDrawer->TextAspect()->Aspect() = *myDrawer->Link()->TextAspect()->Aspect();
        myDrawer->TextAspect()->Aspect()->SetColor(aFgColor);
        myDrawer->TextAspect()->Aspect()->SetColorSubTitle(Quantity_NOC_BLACK);
        myDrawer->TextAspect()->Aspect()->SetTextDisplayType(Aspect_TODT_SHADOW);
    }

    const Handle(Prs3d_TextAspect) aTextAsp = myDrawer->TextAspect();
    aTextAsp->SetHeight(GetTextHeight());

    Graphic3d_HorizontalTextAlignment anHJust = Graphic3d_HTA_CENTER;
    if (!IsHorizontal()) {
        switch (GetLabelPosition()) {
            case Aspect_TOCSP_NONE:
            case Aspect_TOCSP_LEFT: anHJust = Graphic3d_HTA_RIGHT; break;
            case Aspect_TOCSP_RIGHT: anHJust = Graphic3d_HTA_LEFT; break;
            case Aspect_TOCSP_CENTER: anHJust = Graphic3d_HTA_LEFT; break;
        }
    }
    aTextAsp->SetHorizontalJustification(anHJust);

    aTextAsp->SetVerticalJustification(Graphic3d_VTA_BOTTOM);
    aTextAsp->Aspect()->SetTextZoomable(false);
}

// ================================================================
// Function : Compute
// ================================================================
void InteractiveColorScale::Compute(const Handle(PrsMgr_PresentationManager)&,
                                    const Handle(Prs3d_Presentation)& thePrs,
                                    const Standard_Integer            theMode)
{
    if (!IsValidCorner(GetPositionSide())) {
        applyPositionSideFix();
    }
    updateOrientationTransform();
    updateInternalYOffset();
    updateSize(false);
    updateTextAspectEx();

    if (theMode == (int)ColorScaleDispMode::Frame) {
        constexpr int anOuterFrameOffset = 2;

        const int anXLeft  = GetBarLeftX();
        const int aBreadth = GetBreadth();
        drawFrameEx(thePrs,
                    Graphic3d_Vec2i(anXLeft - anOuterFrameOffset,
                                    myBarPos.barBottom - anOuterFrameOffset + myBarPosCorrection),
                    Graphic3d_Vec2i(aBreadth + anOuterFrameOffset * 2, myBarPos.barHeight + anOuterFrameOffset * 2),
                    myDrawer->Color());
        return;
    }

    if (theMode != (int)ColorScaleDispMode::Full) {
        return;
    }

    Handle(Graphic3d_Group) aLabelsGroup = thePrs->NewGroup();
    aLabelsGroup->SetGroupPrimitivesAspect(myDrawer->TextAspect()->Aspect());

    // draw title
    drawTitleEx(aLabelsGroup);

    // draw colors
    drawColorBarOutOfRange(thePrs);
    drawColorBarEx(thePrs);

    // draw Labels
    if (GetLabelPosition() != Aspect_TOCSP_NONE) {
        drawLabelsEx(aLabelsGroup);
    }

    // range tips
    if (GetLabelType() != Aspect_TOCSD_USER) {
        drawRangeTips(thePrs);
    }
}

// =======================================================================
// function : drawRangeTips
// =======================================================================
void InteractiveColorScale::drawRangeTips(const Handle(Prs3d_Presentation)& thePrs)
{
    // range tips
    Handle(Graphic3d_Group) aRangeGroup = thePrs->NewGroup();
    aRangeGroup->SetGroupPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
    {
        Handle(RangeTipOwner) aRangeOwner = new RangeTipOwner("", RangeTipOwner::RangeSide::MinSide);
        aRangeOwner->SetSize(static_cast<float>(GetBreadth()));
        aRangeOwner->SetPoint(GetRangeTipMin());
        aRangeGroup->AddPrimitiveArray(aRangeOwner->GetTriangles());
    }
    {
        Handle(RangeTipOwner) aRangeOwner = new RangeTipOwner("", RangeTipOwner::RangeSide::MaxSide);
        aRangeOwner->SetSize(static_cast<float>(GetBreadth()));
        aRangeOwner->SetPoint(GetRangeTipMax());
        aRangeGroup->AddPrimitiveArray(aRangeOwner->GetTriangles());
    }
}

// =======================================================================
// function : drawColorBarOutOfRange
// =======================================================================
void InteractiveColorScale::drawColorBarOutOfRange(const Handle(Prs3d_Presentation)& thePrs)
{
    if (GetOutOfRangeColor().Alpha() < 0.01f) {
        return;
    }

    const int aFromHeight = int(GetRangeTipY(mySelRangeFrom) - GetRangeTipY(0.0));
    const int aToHeight   = int(GetRangeTipY(1.0) - GetRangeTipY(mySelRangeTo));
    if (aFromHeight == 0 && aToHeight == 0) {
        return;
    }

    Handle(Graphic3d_ArrayOfTriangles) aTrisOut = new Graphic3d_ArrayOfTriangles(2 * 4,
                                                                                 2 * 2 * 3,
                                                                                 Graphic3d_ArrayFlags_None);
    if (aFromHeight != 0) {
        addColoredQuadEx(aTrisOut,
                         Graphic3d_Vec2i(GetBarLeftX(), myBarPos.barBottom + myBarPosCorrection),
                         Graphic3d_Vec2i(GetBreadth(), aFromHeight),
                         Quantity_ColorRGBA(Quantity_NOC_WHITE),
                         Quantity_ColorRGBA(Quantity_NOC_WHITE));
    }
    if (aToHeight != 0) {
        addColoredQuadEx(aTrisOut,
                         Graphic3d_Vec2i(GetBarLeftX(), myBarPos.barTop - aToHeight + myBarPosCorrection),
                         Graphic3d_Vec2i(GetBreadth(), aToHeight),
                         Quantity_ColorRGBA(Quantity_NOC_WHITE),
                         Quantity_ColorRGBA(Quantity_NOC_WHITE));
    }

    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(myOutOfRangeAsp);
    aGroup->AddPrimitiveArray(aTrisOut);
}

// =======================================================================
// function : drawColorBarEx
// =======================================================================
void InteractiveColorScale::drawColorBarEx(const Handle(Prs3d_Presentation)& thePrs)
{
    NCollection_Sequence<Quantity_ColorRGBA> aColors;

    int       aNbIntervals = GetNumberOfIntervals();
    const int aNbToSkip    = collectColorBarColors(aColors, aNbIntervals);

    const double aStepY = double(myBarPos.barInnerHeight) / double(Max(aNbIntervals - aNbToSkip, 1));
    if (aStepY <= 0.0) {
        return;
    }

    Handle(Graphic3d_ArrayOfTriangles) aTriangles;
    if (IsSmoothTransition() && GetColorType() == Aspect_TOCSD_USER) {
        // Smooth custom intervals, so that the color in the center of interval is equal to specified one
        // (thus the halves of first and last intervals have solid color)
        drawSmoothUserColorBarEx(aColors, aTriangles, aNbIntervals, aStepY);
    } else if (IsSmoothTransition()) {
        // smooth transition between standard colors - without solid color
        // regions at the beginning and end of full color range
        drawSmoothAutoColorBarEx(aColors, aTriangles, aNbIntervals);
    } else {
        // no color smoothing
        drawDiscreteColorBarEx(aColors, aTriangles, aStepY);
    }

    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
    aGroup->AddPrimitiveArray(aTriangles);

    const int anXLeft       = GetBarLeftX();
    const int aColorBreadth = GetBreadth();

    const Quantity_Color aFgColor(hasOwnColor ? myDrawer->Color() : Quantity_NOC_WHITE);
    drawFrameEx(thePrs,
                Graphic3d_Vec2i(anXLeft - 1 + 1, myBarPos.barBottom - 1 - 1 + myBarPosCorrection),
                Graphic3d_Vec2i(aColorBreadth + 2, myBarPos.barHeight + 2),
                myDrawer->TextAspect()->Aspect()->ColorSubTitle()); // shadow

    drawFrameEx(thePrs,
                Graphic3d_Vec2i(anXLeft - 1, myBarPos.barBottom - 1 + myBarPosCorrection),
                Graphic3d_Vec2i(aColorBreadth + 2, myBarPos.barHeight + 2),
                aFgColor); // foreground
}

// =======================================================================
// function : collectColorBarColors
// =======================================================================
int InteractiveColorScale::collectColorBarColors(NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                 int&                                      theNbIntervals)
{
    NCollection_Array1<bool> aCustLabels;
    int                      aNbToSkip = 0;

    const int aPrevNbIntervals = GetNumberOfIntervals();
    if (IsSmoothTransition() && GetColorType() == Aspect_TOCSD_AUTO) {
        static constexpr int aNbSmoothIntervals = 5; // 4 quads
        SetNumberOfIntervals(aNbSmoothIntervals);
    } else if (myToSkipEmptyLabels && !IsSmoothTransition() && GetLabelType() == Aspect_TOCSD_USER
               && Labels().Size() == GetNumberOfIntervals()) {
        aCustLabels.Resize(1, GetNumberOfIntervals(), false);
        int anIntervalIter = 0;
        for (const TCollection_ExtendedString& aLabIter : Labels()) {
            aCustLabels[++anIntervalIter] = !aLabIter.IsEmpty();
            if (aLabIter.IsEmpty()) {
                ++aNbToSkip;
            }
        }
    }
    theNbIntervals = GetNumberOfIntervals();

    for (int anIntervalIter = 1; anIntervalIter <= theNbIntervals; ++anIntervalIter) {
        if (!aCustLabels.IsEmpty() && !aCustLabels[anIntervalIter]) {
            continue;
        } else if (IsReversed()) {
            theColors.Prepend(GetIntervalColor(anIntervalIter));
        } else {
            theColors.Append(GetIntervalColor(anIntervalIter));
        }
    }
    SetNumberOfIntervals(aPrevNbIntervals);

    return aNbToSkip;
}

// =======================================================================
// function : drawSmoothUserColorBarEx
// =======================================================================
void InteractiveColorScale::drawSmoothUserColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                     Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                     int                                             theNbIntervals,
                                                     double                                          theStepY)
{
    const int anXLeft       = GetBarLeftX();
    const int aColorBreadth = GetBreadth();

    theTriangles = new Graphic3d_ArrayOfTriangles((theColors.Length() + 1) * 4,      // quads
                                                  (theColors.Length() + 1) * 2 * 3,  // quads as triangles
                                                  Graphic3d_ArrayFlags_VertexColor); // per-vertex colors
    Quantity_ColorRGBA aColor1(theColors.Value(1)), aColor2;

    int       aSizeY        = int(theStepY / 2);
    const int anYBottom     = myBarPos.barInnerBottom + aSizeY;
    int       anYBottomIter = anYBottom;
    addColoredQuadEx(theTriangles,
                     Graphic3d_Vec2i(anXLeft, myBarPos.barInnerBottom + myBarPosCorrection),
                     Graphic3d_Vec2i(aColorBreadth, aSizeY),
                     aColor1,
                     aColor1);
    for (int aColorIter = 0; aColorIter < theNbIntervals - 1; ++aColorIter) {
        aColor1 = theColors.Value(aColorIter + 1);
        aColor2 = theColors.Value(aColorIter + 2);
        aSizeY  = anYBottom + int((aColorIter + 1) * theStepY) - anYBottomIter;
        addColoredQuadEx(theTriangles,
                         Graphic3d_Vec2i(anXLeft, anYBottomIter + myBarPosCorrection),
                         Graphic3d_Vec2i(aColorBreadth, aSizeY),
                         aColor1,
                         aColor2);
        anYBottomIter += aSizeY;
    }
    aColor2 = theColors.Value(theNbIntervals);
    aSizeY  = myBarPos.barInnerBottom + myBarPos.barInnerHeight - anYBottomIter;
    addColoredQuadEx(theTriangles,
                     Graphic3d_Vec2i(anXLeft, anYBottomIter + myBarPosCorrection),
                     Graphic3d_Vec2i(aColorBreadth, aSizeY),
                     aColor2,
                     aColor2);
}

// =======================================================================
// function : drawSmoothAutoColorBarEx
// =======================================================================
void InteractiveColorScale::drawSmoothAutoColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                     Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                     int                                             theNbIntervals)
{
    const int anXLeft       = GetBarLeftX();
    const int aColorBreadth = GetBreadth();

    const int    aNbQuads   = theNbIntervals - 1;
    const double aQuadSizeF = double(myBarPos.barInnerHeight) / double(aNbQuads);

    theTriangles = new Graphic3d_ArrayOfTriangles(aNbQuads * 4, aNbQuads * 2 * 3, Graphic3d_ArrayFlags_VertexColor);
    int anYIter  = myBarPos.barInnerBottom;
    for (int aQuadIter = 0; aQuadIter < aNbQuads; ++aQuadIter) {
        int anYBot = anYIter;
        anYIter    = (int)Round(double(myBarPos.barInnerBottom) + double(aQuadIter + 1) * aQuadSizeF);
        if (aQuadIter + 1 == aNbQuads) {
            anYIter = myBarPos.barInnerBottom + myBarPos.barInnerHeight;
        }
        if (anYIter - anYBot <= 0) {
            continue;
        }
        addColoredQuadEx(theTriangles,
                         Graphic3d_Vec2i(anXLeft, anYBot + myBarPosCorrection),
                         Graphic3d_Vec2i(aColorBreadth, anYIter - anYBot),
                         theColors.Value(aQuadIter + 1),
                         theColors.Value(aQuadIter + 2));
    }
}

// =======================================================================
// function : drawDiscreteColorBarEx
// =======================================================================
void InteractiveColorScale::drawDiscreteColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                   Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                   double                                          theStepY)
{
    const int anXLeft       = GetBarLeftX();
    const int aColorBreadth = GetBreadth();

    theTriangles = new Graphic3d_ArrayOfTriangles(theColors.Length() * 4,
                                                  theColors.Length() * 2 * 3,
                                                  Graphic3d_ArrayFlags_VertexColor);

    int anYBottomIter = myBarPos.barInnerBottom;
    for (int aColorIter = 0; aColorIter < theColors.Length(); ++aColorIter) {
        const Quantity_ColorRGBA& aColor = theColors.Value(aColorIter + 1);

        const int aSizeY = myBarPos.barInnerBottom + int((aColorIter + 1) * theStepY) - anYBottomIter;
        addColoredQuadEx(theTriangles,
                         Graphic3d_Vec2i(anXLeft, anYBottomIter + myBarPosCorrection),
                         Graphic3d_Vec2i(aColorBreadth, aSizeY),
                         aColor,
                         aColor);
        anYBottomIter += aSizeY;
    }
}

// =======================================================================
// function : GetLabelEx
// =======================================================================
TCollection_ExtendedString InteractiveColorScale::GetLabelEx(const int theIndex) const
{
    double aVal = IsLabelAtBorder() ? GetIntervalValueEx(theIndex - 1)
                                    : (0.5 * (GetIntervalValueEx(theIndex - 1) + GetIntervalValueEx(theIndex)));
    if (!myDisplayUnits.IsEmpty()) {
        aVal = ViewBase::UnitManager::Instance()->ConvertAnyFromSI(aVal, myDisplayUnits);
    }

    std::stringstream aBuf;
    aBuf.precision(ViewBase::BaseDefaultDrawer::m_defFloatPrec);
    aBuf.imbue(std::locale::classic()); // use always C locale
    aBuf << std::defaultfloat << aVal;
    return TCollection_ExtendedString(aBuf.str().c_str(), true);
}

// =======================================================================
// function : drawLabelsEx
// =======================================================================
void InteractiveColorScale::drawLabelsEx(const Handle(Graphic3d_Group)& theGroup)
{
    TColStd_SequenceOfExtendedString aLabels;
    if (!collectLabelsEx(aLabels)) {
        return;
    }

    const int aNbLabels    = aLabels.Size();
    const int aNbIntervals = IsLabelAtBorder() ? aNbLabels - 1 : aNbLabels;
    // length is static for horizontal color scale, actual label length is unchecked
    const int    anExpectedTextSpace = (IsHorizontal() ? 6 : 1) * GetTextHeight();
    const double aStepY              = double(myBarPos.barInnerHeight) / double(aNbIntervals);
    if (aStepY <= 0.0) {
        return;
    }

    int aFilter = 0;
    {
        const int aSpc = myBarPos.barInnerHeight
                       - ((Min(aNbLabels, 2) + Abs(aNbLabels - aNbIntervals - 1)) * anExpectedTextSpace);
        if (aSpc <= 0) {
            return;
        }

        const double aVal    = double(aNbLabels) * anExpectedTextSpace / aSpc;
        double       anIPart = 0.0;
        double       anFPart = std::modf(aVal, &anIPart);
        aFilter              = (int)anIPart + (anFPart != 0 ? 1 : 0);
    }
    if (aFilter <= 0) {
        return;
    }

    drawVertLabelsEx(aLabels, aStepY, aFilter, theGroup);

    // expand bounding box to workaround picking of labels,
    // as text entities adds only anchor point to presentation box
    const int          anXLeft = GetTextLabelX();
    Graphic3d_BndBox4f aBox    = theGroup->BoundingBox();
    aBox.Add(Graphic3d_Vec4(Graphic3d_Vec4i(anXLeft - GetBreadth() * 2, myBarPos.barBottom, 0, 0)));
    aBox.Add(Graphic3d_Vec4(Graphic3d_Vec4i(anXLeft + GetBreadth() * 2, myBarPos.barTop, 0, 0)));
    theGroup->ChangeBoundingBox() = aBox;
}

// =======================================================================
// function : collectLabelsEx
// =======================================================================
bool InteractiveColorScale::collectLabelsEx(TColStd_SequenceOfExtendedString& theLabels) const
{
    if (GetLabelType() == Aspect_TOCSD_USER) {
        const bool toSkipEmpty = myToSkipEmptyLabels && !IsSmoothTransition()
                              && Labels().Size() == GetNumberOfIntervals();
        for (const TCollection_ExtendedString& aLabIter : Labels()) {
            if (!toSkipEmpty || !aLabIter.IsEmpty()) {
                theLabels.Append(aLabIter);
            }
        }
    } else {
        const int aNbLabels = IsLabelAtBorder() ? GetNumberOfIntervals() + 1 : GetNumberOfIntervals();
        for (int aLabIter = 1; aLabIter <= aNbLabels; ++aLabIter) {
            theLabels.Append(GetLabelEx(aLabIter));
        }
        if (theLabels.IsEmpty()) {
            return false;
        }
    }
    if (IsReversed()) {
        theLabels.Reverse();
    }

    return true;
}

// =======================================================================
// function : drawVertLabelsEx
// =======================================================================
void InteractiveColorScale::drawVertLabelsEx(const TColStd_SequenceOfExtendedString& theLabels,
                                             double                                  theStepY,
                                             int                                     theFilter,
                                             const Handle(Graphic3d_Group)&          theGroup)
{
    const int  aNbLabels = theLabels.Size();
    const int  anXLeft   = GetTextLabelX();
    int        i1        = 0;
    int        i2        = aNbLabels - 1;
    int        aLast1    = i1;
    int        aLast2    = i2;
    const int  anYBottom = IsLabelAtBorder() ? myBarPos.barInnerBottom : myBarPos.barInnerBottom + int(theStepY / 2);
    const bool doFlushBorderLabel                          = IsLabelAtBorder() && IsHorizontal();
    Graphic3d_HorizontalTextAlignment anUnflushedAlignment = myDrawer->TextAspect()->HorizontalJustification();
    const auto                        drawLabel            = [&](int i, Graphic3d_HorizontalTextAlignment theAlign) {
        drawTextEx(theGroup,
                   theLabels.Value(i + 1),
                   Graphic3d_Vec2i(anXLeft, anYBottom + int(i * theStepY) + myBarPosCorrection),
                   theAlign,
                   Graphic3d_VTA_CENTER);
    };
    while (i2 - i1 >= theFilter || (i2 == 0 && i1 == 0)) {
        int aPos1 = i1;
        int aPos2 = aNbLabels - 1 - i2;
        if (theFilter && !(aPos1 % theFilter)) {
            Graphic3d_HorizontalTextAlignment anAlign = (doFlushBorderLabel && i1 == 0) ? Graphic3d_HTA_LEFT
                                                                                        : anUnflushedAlignment;
            drawLabel(i1, anAlign);
            aLast1 = i1;
        }
        if (theFilter && !(aPos2 % theFilter)) {
            Graphic3d_HorizontalTextAlignment anAlign = (doFlushBorderLabel && i2 == aNbLabels - 1)
                                                          ? Graphic3d_HTA_RIGHT
                                                          : anUnflushedAlignment;
            drawLabel(i2, anAlign);
            aLast2 = i2;
        }
        i1++;
        i2--;
    }
    int aPos = i1;
    int i0   = -1;
    while (aPos <= i2 && i0 == -1) {
        if (theFilter && !(aPos % theFilter) && Abs(aPos - aLast1) >= theFilter && Abs(aPos - aLast2) >= theFilter) {
            i0 = aPos;
        }
        ++aPos;
    }

    if (i0 != -1)
        drawLabel(i0, anUnflushedAlignment);
}

// =======================================================================
// function : drawFrameEx
// =======================================================================
void InteractiveColorScale::drawFrameEx(const Handle(Prs3d_Presentation)& thePrs,
                                        const Graphic3d_Vec2i&            theXY,
                                        const Graphic3d_Vec2i&            theSize,
                                        const Quantity_Color&             theColor)
{
    Handle(Graphic3d_ArrayOfPolylines) aPrim = new Graphic3d_ArrayOfPolylines(5);
    aPrim->AddVertex(theXY.x(), theXY.y(), 0.0);
    aPrim->AddVertex(theXY.x() + theSize.x(), theXY.y(), 0.0);
    aPrim->AddVertex(theXY.x() + theSize.x(), theXY.y() + theSize.y(), 0.0);
    aPrim->AddVertex(theXY.x(), theXY.y() + theSize.y(), 0.0);
    aPrim->AddVertex(theXY.x(), theXY.y(), 0.0);

    Handle(Graphic3d_AspectLine3d) anAspect = new Graphic3d_AspectLine3d(theColor, Aspect_TOL_SOLID, 1.0);
    Handle(Graphic3d_Group)        aGroup   = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(anAspect);
    aGroup->AddPrimitiveArray(aPrim);
}

// =======================================================================
// function : drawTextEx
// =======================================================================
void InteractiveColorScale::drawTextEx(const Handle(Graphic3d_Group)&          theGroup,
                                       const TCollection_ExtendedString&       theText,
                                       const Graphic3d_Vec2i&                  theXY,
                                       const Graphic3d_HorizontalTextAlignment theHorizAlignment,
                                       const Graphic3d_VerticalTextAlignment   theVertAlignment)
{
    const Handle(Prs3d_TextAspect)& anAspect = myDrawer->TextAspect();

    Handle(Graphic3d_Text) aText = new Graphic3d_Text((float)anAspect->Height());
    aText->SetPosition(gp_Pnt(theXY.x(), theXY.y(), 0.0));
    aText->SetText(theText.ToExtString());
    aText->SetHorizontalAlignment(theHorizAlignment);
    aText->SetVerticalAlignment(theVertAlignment);

    theGroup->AddText(aText);
}

// =======================================================================
// function : drawTitleEx
// =======================================================================
void InteractiveColorScale::drawTitleEx(const Handle(Graphic3d_Group)& theGroup)
{
    if (GetTitle().IsEmpty()) {
        return;
    }

    TCollection_ExtendedString aTitle = GetTitle();
    if (!myDisplayUnits.IsEmpty()) {
        aTitle = aTitle + " [" + ViewBase::UnitManager::Instance()->FormatUnitName(myDisplayUnits) + "]";
    }

    int anXLeft = GetLabelPosition() == Aspect_TOCSP_RIGHT
                    ? GetXPosition()
                    : (GetLabelPosition() == Aspect_TOCSP_LEFT ? -GetXPosition() : 0);

    int                               anYTop  = 0;
    Graphic3d_HorizontalTextAlignment anAlign = Graphic3d_HTA_LEFT;

    if (IsHorizontal()) {
        anYTop                                 = myBarPos.barBottom;
        anAlign                                = Graphic3d_HTA_LEFT;
        Aspect_TypeOfTriedronPosition aPosSide = GetPositionSide();
        if (aPosSide == Aspect_TOTP_BOTTOM) {
            anXLeft += GetSpacing();
        } else if (aPosSide == Aspect_TOTP_TOP) {
            anXLeft += GetBarLeftX() + GetBreadth() + GetSpacing();
        }
    } else {
        anYTop  = myBarPos.barTop + myBarPos.barYOffset;
        anAlign = myDrawer->TextAspect()->HorizontalJustification();
    }
    anYTop += myBarPosCorrection;
    drawTextEx(theGroup, aTitle, Graphic3d_Vec2i(anXLeft, anYTop), anAlign, Graphic3d_VTA_BOTTOM);
}

// ================================================================
// Function : updateSize
// ================================================================
void InteractiveColorScale::updateSize(bool theToInvalidate)
{
    Aspect_TypeOfColorScalePosition aLabPos = Aspect_TOCSP_NONE;

    int aBreadth = 0;
    int aHeight  = 0;
    int anOffset = 0;

    computeSizes(aBreadth, aHeight, aLabPos, anOffset);

    if (theToInvalidate && (aBreadth != GetBreadth() || aHeight != GetHeight() || aLabPos != GetLabelPosition())) {
        SetToUpdate();
    }

    if (aHeight != 0) {
        SetHeight(aHeight);
    }
    // doesn't work due to 20 pixels limit in AIS_ColorScale
    if (aBreadth != 0) {
        SetBreadth(aBreadth);
    }

    SetLabelPosition(aLabPos);

    static constexpr int halfSize2 = 2;
    myBarPos.topY                  = GetYPosition() + anOffset;
    if (IsHorizontal()) {
        myBarPos.titleOffset = 0;
    } else {
        myBarPos.titleOffset = !GetTitle().IsEmpty() ? (GetTextHeight() + GetSpacing()) * titleRowsCount() : 0;
    }
    myBarPos.barYOffset = GetTextHeight() / halfSize2 + halfSize2 * GetSpacing(); // a half-label offset
    myBarPos.barBottom  = myBarPos.topY + myBarPos.barYOffset;
    myBarPos.barTop     = myBarPos.topY + GetHeight() - myBarPos.titleOffset - myBarPos.barYOffset;
    myBarPos.barHeight  = myBarPos.barTop - myBarPos.barBottom;

    myBarPos.barInnerBottom = myBarPos.barBottom + int(double(myBarPos.barHeight) * mySelRangeFrom);
    myBarPos.barInnerTop    = myBarPos.barBottom + int(double(myBarPos.barHeight) * mySelRangeTo);
    myBarPos.barInnerHeight = int(double(myBarPos.barHeight) * (mySelRangeTo - mySelRangeFrom));
}

// ================================================================
// Function : computeSizes
// ================================================================
void InteractiveColorScale::computeSizes(int&                             theBreadth,
                                         int&                             theHeight,
                                         Aspect_TypeOfColorScalePosition& theLabPos,
                                         int&                             theOffset) const
{
    theLabPos = Aspect_TOCSP_RIGHT;

    theBreadth     = (int)myBaseBreadth;
    theHeight      = (int)myBaseHeight;
    int aMaxHeight = (int)myMaxBaseHeight;

    if (myBaseHeight >= 2.0f) {
        theHeight = (int)myBaseHeight;
    }
    if (myMaxBaseHeight >= 2.0f) {
        aMaxHeight = (int)myMaxBaseHeight;
    }
    if ((myBaseHeight < 2.0f || myMaxBaseHeight < 2.0f) && HasInteractiveContext()) {
        Handle(V3d_View) aView        = IInteractiveObject::GetVisibleInAnyView(this);
        Graphic3d_Vec2i  anOffset     = !myTransformPersistence.IsNull() && myTransformPersistence->IsTrihedronOr2d()
                                          ? myTransformPersistence->Offset2d()
                                          : Graphic3d_Vec2i();
        const double     anOffsetProj = anOffset.Dot(GetOrientationAxisI());
        if (!aView.IsNull()) {
            Graphic3d_Vec2d aWinSize = Graphic3d_Vec2d(aView->Window()->Dimensions())
                                     / aView->RenderingParams().ResolutionRatio();
            // window size projected to color scale's axis
            const double aWinSizeProj = aWinSize.Dot(GetOrientationAxis());

            if (myBaseHeight < 2.0f) {
                theHeight = (int)Round(myBaseHeight * float(aWinSizeProj - anOffsetProj));
            }
            if (myMaxBaseHeight < 2.0f) {
                aMaxHeight = (int)Round(myMaxBaseHeight * float(aWinSizeProj));
            }
        }
    }
    theOffset = 0;
    if (theHeight > aMaxHeight) {
        static constexpr int halfFactor2 = 2;

        theOffset = (theHeight - aMaxHeight) / halfFactor2;
        theHeight = aMaxHeight;
    }
    if (!myTransformPersistence.IsNull() && myTransformPersistence->IsTrihedronOr2d()) {
        if ((myTransformPersistence->Corner2d() & Aspect_TOTP_RIGHT) != 0) {
            theLabPos = Aspect_TOCSP_LEFT;
        }
    }
}

// =======================================================================
// function : OnBeforeRedraw
// =======================================================================
AIS_RedrawProgressResult InteractiveColorScale::ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                              const Handle(V3d_View)&               theView,
                                                              const AIS_RedrawProgress              theStage)
{
    updateSize(true);
    return AIS_InteractiveObject::ProcessRedraw(theCtx, theView, theStage);
}

// ================================================================
// Function : OnPixelScaleChange
// ================================================================
bool InteractiveColorScale::OnPixelScaleChange(const Handle(AIS_InteractiveObject)& theObj, float theRatio)
{
    IInteractiveObject::OnPixelScaleChange(theObj, theRatio);
    updateSize(true);
    if (HasInteractiveContext()) {
        InteractiveContext()->Redisplay(this, false);
    }
    return true;
}

// ================================================================
// Function : BarDragOwner
// ================================================================
InteractiveColorScale::BarDragOwner::BarDragOwner(const TCollection_AsciiString& theId)
    : ViewBase::InteractiveOwner(theId, ViewBase::InteractiveOwner::SelectionPart(), (int)PickPriority::ColorBar)
{
    myAspect = new Graphic3d_AspectFillArea3d();
    myAspect->SetInteriorStyle(Aspect_IS_SOLID);
    myAspect->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
}

// ================================================================
// Function : BarDragOwner::HandleMouseClick
// ================================================================
Standard_Boolean InteractiveColorScale::BarDragOwner::HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                                       Aspect_VKeyMouse       theButton,
                                                                       Aspect_VKeyFlags       theModifiers,
                                                                       bool                   theIsDoubleClick)
{
    if (InteractiveOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick)) {
        return true;
    }
    if (GetInterval() == -1) {
        return false;
    }
    if (theButton != Aspect_VKeyMouse_LeftButton || theModifiers != Aspect_VKeyFlags_NONE || theIsDoubleClick) {
        return false;
    }

    Handle(InteractiveColorScale) anObj = Handle(InteractiveColorScale)::DownCast(mySelectable);

    // flip transparency
    NCollection_Sequence<Quantity_ColorRGBA> aColors = anObj->GetCustomColors();
    Quantity_ColorRGBA&                      aCol    = aColors.ChangeValue(GetInterval());
    aCol.SetAlpha(aCol.Alpha() < 1.0f ? 1.0f : 0.0f);
    anObj->SetCustomColors(aColors);

    anObj->InteractiveContext()->RecomputePrsOnly(anObj, false, true);
    anObj->UpdateColorMap(false);
    return true;
}

// ================================================================
// Function : BarDragOwner::HilightWithColor
// ================================================================
void InteractiveColorScale::BarDragOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                           const Handle(Prs3d_Drawer)&               theStyle,
                                                           const Standard_Integer                    theMode)
{
    if (!thePrsMgr->IsImmediateModeOn()) {
        return;
    }

    Handle(InteractiveColorScale) anObj = Handle(InteractiveColorScale)::DownCast(mySelectable);
    if (anObj.IsNull()) {
        return;
    }

    InteractiveOwner::HilightWithColor(thePrsMgr, theStyle, theMode);

    if (GetInterval() == -1) {
        return;
    }

    Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(4, 6, Graphic3d_ArrayFlags_None);
    aTris->AddVertex(myXY.x(), myXY.y() + mySize.y(), 0.0);
    aTris->AddVertex(myXY.x(), myXY.y(), 0.0);
    aTris->AddVertex(myXY.x() + mySize.x(), myXY.y(), 0.0);
    aTris->AddVertex(myXY.x() + mySize.x(), myXY.y() + mySize.y(), 0.0);
    aTris->AddQuadTriangleEdges(1, 2, 3, 4);

    Handle(Prs3d_Presentation) aHilightPrs = GetHighlightPresentation(thePrsMgr, theStyle, false);
    aHilightPrs->Clear();
    aHilightPrs->CStructure()->ViewAffinity = thePrsMgr->StructureManager()->ObjectAffinity(mySelectable);
    aHilightPrs->SetTransformPersistence(mySelectable->TransformPersistence());
    aHilightPrs->SetTransformation(mySelectable->TransformationGeom());
    aHilightPrs->SetDisplayPriority(Graphic3d_DisplayPriority_Highlight);

    myAspect->SetInteriorColor(anObj->IntervalHighlightColor());

    Handle(Graphic3d_Group) aGroup = aHilightPrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(myAspect);
    aGroup->AddPrimitiveArray(aTris);

    thePrsMgr->AddToImmediateList(aHilightPrs);

    anObj->SetLastDetectedInterval(GetInterval());

    if (anObj->ToHighlightIntervals()) {
        anObj->UpdateColorMap(false);
    }

    InvalidateViewer(anObj);
}

// ================================================================
// Function : BarDragOwner::Unhilight
// ================================================================
void InteractiveColorScale::BarDragOwner::Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                    const Standard_Integer                    theMode)
{
    InteractiveOwner::Unhilight(thePrsMgr, theMode);

    Handle(InteractiveColorScale) anObj = Handle(InteractiveColorScale)::DownCast(mySelectable);
    if (anObj.IsNull() || GetInterval() == -1) {
        return;
    }

    anObj->SetLastDetectedInterval(-1);

    if (anObj->ToHighlightIntervals() && anObj->HasInteractiveContext()) {
        anObj->UpdateColorMap(false);
    }

    InvalidateViewer(anObj);
}

// ================================================================
// Function : OutOfRangeOwner
// ================================================================
InteractiveColorScale::OutOfRangeOwner::OutOfRangeOwner(const TCollection_AsciiString& theId) : BarDragOwner(theId)
{
    SetPriority((int)PickPriority::OuterRangeFlip);
}

// ================================================================
// Function : OutOfRangeOwner::HandleMouseClick
// ================================================================
Standard_Boolean InteractiveColorScale::OutOfRangeOwner::HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                                          Aspect_VKeyMouse       theButton,
                                                                          Aspect_VKeyFlags       theModifiers,
                                                                          bool                   theIsDoubleClick)
{
    if (BarDragOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick)) {
        return true;
    }
    if (theButton != Aspect_VKeyMouse_LeftButton || theModifiers != Aspect_VKeyFlags_NONE || theIsDoubleClick) {
        return false;
    }

    Handle(InteractiveColorScale) anObj = Handle(InteractiveColorScale)::DownCast(mySelectable);
    anObj->SetOpaqueOutOfRange(!anObj->IsOpaqueOutOfRange());
    anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
    anObj->InteractiveContext()->RecomputePrsOnly(anObj, false, true);
    anObj->UpdateColorMap(false);
    return true;
}

// ================================================================
// Function : RangeFlipOwner
// ================================================================
InteractiveColorScale::RangeFlipOwner::RangeFlipOwner(const TCollection_AsciiString& theId)
    : ViewBase::InteractiveOwner(theId, ViewBase::InteractiveOwner::SelectionPart(), (int)PickPriority::RangeFlip)
{
    //
}

// ================================================================
// Function : RangeFlipOwner::HandleMouseClick
// ================================================================
Standard_Boolean InteractiveColorScale::RangeFlipOwner::HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                                         Aspect_VKeyMouse       theButton,
                                                                         Aspect_VKeyFlags       theModifiers,
                                                                         bool                   theIsDoubleClick)
{
    if (InteractiveOwner::HandleMouseClick(thePoint, theButton, theModifiers, theIsDoubleClick)) {
        return true;
    }
    if (theButton != Aspect_VKeyMouse_LeftButton || theModifiers != Aspect_VKeyFlags_NONE || theIsDoubleClick) {
        return false;
    }

    Handle(InteractiveColorScale) anObj = Handle(InteractiveColorScale)::DownCast(mySelectable);
    anObj->SetReversed(!anObj->IsReversed());
    anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
    anObj->InteractiveContext()->RecomputePrsOnly(anObj, false, true);
    anObj->UpdateColorMap(false);
    return true;
}

// ================================================================
// Function : RangeTipOwner
// ================================================================
InteractiveColorScale::RangeTipOwner::RangeTipOwner(const TCollection_AsciiString& theId, RangeSide theSideId)
    : ViewBase::InteractiveOwner(theId, ViewBase::InteractiveOwner::SelectionPart(), (int)PickPriority::RangeTip),
      myTipSide(theSideId)
{
    static constexpr int nbTriNodes3 = 3;

    myAspect = new Graphic3d_AspectLine3d(Quantity_NOC_CYAN, Aspect_TOL_SOLID, 1.0);
    myTris   = new Graphic3d_ArrayOfTriangles(nbTriNodes3, 0, Graphic3d_ArrayFlags_AttribsMutable);
}

// ================================================================
// Function : RangeTipOwner::SetPoint
// ================================================================
void InteractiveColorScale::RangeTipOwner::SetPoint(const Graphic3d_Vec2i& thePnt)
{
    static constexpr int triNode1 = 1;
    static constexpr int triNode2 = 2;
    static constexpr int triNode3 = 3;

    myPoint = thePnt;

    const Graphic3d_Vec2 aPnt(thePnt);
    myTris->SetVertice(triNode1, aPnt.x(), aPnt.y(), 0.0f);
    myTris->SetVertice(triNode2,
                       aPnt.x() + int(mySize * 0.5f),
                       aPnt.y() + int(myTipSide == RangeSide::MaxSide ? mySize * 0.5f : -mySize * 0.5f),
                       0.0f);
    myTris->SetVertice(triNode3,
                       aPnt.x() - int(mySize * 0.5f),
                       aPnt.y() + int(myTipSide == RangeSide::MaxSide ? mySize * 0.5f : -mySize * 0.5f),
                       0.0f);
    myTris->Attributes()->Invalidate();
}

// ================================================================
// Function : RangeTipOwner::HilightWithColor
// ================================================================
void InteractiveColorScale::RangeTipOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                            const Handle(Prs3d_Drawer)&               theStyle,
                                                            const Standard_Integer)
{
    if (!thePrsMgr->IsImmediateModeOn()) {
        return;
    }

    SetPoint(myPoint);

    Handle(Prs3d_Presentation) aHilightPrs = GetHighlightPresentation(thePrsMgr, theStyle, false);
    aHilightPrs->Clear();
    aHilightPrs->CStructure()->ViewAffinity = thePrsMgr->StructureManager()->ObjectAffinity(mySelectable);
    aHilightPrs->SetTransformPersistence(mySelectable->TransformPersistence());
    aHilightPrs->SetTransformation(mySelectable->TransformationGeom());
    aHilightPrs->SetDisplayPriority(Graphic3d_DisplayPriority_Highlight);

    Handle(Graphic3d_Group) aGroup = aHilightPrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(myAspect);
    aGroup->AddPrimitiveArray(myTris);

    thePrsMgr->AddToImmediateList(aHilightPrs);
}

// ================================================================
// Function : ComputeSelection
// ================================================================
void InteractiveColorScale::ComputeSelection(const Handle(SelectMgr_Selection)& theSel, const Standard_Integer theMode)
{
    if (theMode != (int)ColorScaleDispMode::Full) {
        return;
    }

    if (myMeshOwner->GetSelectionMode() != 0) {
        return;
    }

    // draggable color bar
    if (!IsSmoothTransition() && GetColorType() == Aspect_TOCSD_USER) {
        computeSelDragBarUserColors(theSel);
    } else {
        computeSelDragBar(theSel);
    }

    // outer range transparency flippers
    computeSelOutRangeFlips(theSel);

    // draggable range tips
    computeSelDragRangeTips(theSel);

    // flippable range labels
    computeSelFlipRange(theSel);
}

// ================================================================
// Function : computeSelDragBarUserColors
// ================================================================
void InteractiveColorScale::computeSelDragBarUserColors(const Handle(SelectMgr_Selection)& theSel)
{
    if (IsSmoothTransition() || GetColorType() != Aspect_TOCSD_USER) {
        return;
    }

    const int aNbIntervals = GetNumberOfIntervals();
    int       aNbToSkip    = 0;

    NCollection_Array1<bool> aCustLabels;
    if (ToSkipEmptyLabels() && Labels().Size() == GetNumberOfIntervals()) {
        aCustLabels.Resize(1, GetNumberOfIntervals(), false);
        int anIntervalIter = 0;
        for (const TCollection_ExtendedString& aLabIter : Labels()) {
            aCustLabels[++anIntervalIter] = !aLabIter.IsEmpty();
            if (aLabIter.IsEmpty()) {
                ++aNbToSkip;
            }
        }
    }

    NCollection_Sequence<int> aColors;
    for (int anIntervalIter = 1; anIntervalIter <= aNbIntervals; ++anIntervalIter) {
        if (!aCustLabels.IsEmpty() && !aCustLabels[anIntervalIter]) {
            continue;
        } else if (IsReversed()) {
            aColors.Prepend(anIntervalIter);
        } else {
            aColors.Append(anIntervalIter);
        }
    }

    const TCollection_AsciiString& anObjId = myMeshOwner->Id();

    const double aStepY        = double(myBarPos.barInnerHeight) / double(Max(aNbIntervals - aNbToSkip, 1));
    int          anYBottomIter = myBarPos.barInnerBottom;
    for (int aColorIter = 0; aColorIter < aColors.Length(); ++aColorIter) {
        const int aSizeY = myBarPos.barInnerBottom + int((aColorIter + 1) * aStepY) - anYBottomIter;
        if (aSizeY < 10) {
            continue; // avoid too small ranges
        }

        Handle(BarDragOwner) aDragOwner = new BarDragOwner(anObjId);
        aDragOwner->SetSelectablePointer(this);
        aDragOwner->SetSelectionPart(
            ViewBase::InteractiveOwner::SelectionPart((int)SelectionPart::ColorBar, aColors.Value(aColorIter + 1)));
        aDragOwner->SetPosSize(Graphic3d_Vec2i(GetBarLeftX(), anYBottomIter + myBarPosCorrection),
                               Graphic3d_Vec2i(GetBreadth(), aSizeY));

        Handle(Select3D_SensitivePrimitiveArray) aBarSens =
            getQuadSensitivity(aDragOwner,
                               Graphic3d_Vec2i(GetBarLeftX(), anYBottomIter + myBarPosCorrection),
                               Graphic3d_Vec2i(GetBreadth(), aSizeY));
        theSel->Add(aBarSens);

        anYBottomIter += aSizeY;
    }
}

// ================================================================
// Function : computeSelDragBar
// ================================================================
void InteractiveColorScale::computeSelDragBar(const Handle(SelectMgr_Selection)& theSel)
{
    if (myTransformPersistence.IsNull() || !myTransformPersistence->IsTrihedronOr2d()) {
        return;
    }

    Handle(BarDragOwner) aDragOwner = new BarDragOwner(myMeshOwner->Id());
    aDragOwner->SetSelectablePointer(this);
    aDragOwner->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart((int)SelectionPart::ColorBar, -1));

    Handle(Select3D_SensitivePrimitiveArray) aBarSens =
        getQuadSensitivity(aDragOwner,
                           Graphic3d_Vec2i(GetBarLeftX(), myBarPos.barBottom + myBarPosCorrection),
                           Graphic3d_Vec2i(GetBreadth(), myBarPos.barHeight));
    theSel->Add(aBarSens);
}

// ================================================================
// Function : computeSelOutRangeFlips
// ================================================================
void InteractiveColorScale::computeSelOutRangeFlips(const Handle(SelectMgr_Selection)& theSel)
{
    const TCollection_AsciiString& anObjId = myMeshOwner->Id();
    if (GetRangeTipY(mySelRangeFrom) != GetRangeTipY(0.0)) {
        const int               aFromHeight    = int(GetRangeTipY(mySelRangeFrom) - GetRangeTipY(0.0));
        Handle(OutOfRangeOwner) aFlipOwnerFrom = new OutOfRangeOwner(anObjId);
        aFlipOwnerFrom->SetSelectablePointer(this);
        aFlipOwnerFrom->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
            !IsReversed() ? (int)SelectionPart::OutOfRangeMin : (int)SelectionPart::OutOfRangeMax));

        Handle(Select3D_SensitivePrimitiveArray) aBarSensFrom =
            getQuadSensitivity(aFlipOwnerFrom,
                               Graphic3d_Vec2i(GetBarLeftX(), myBarPos.barBottom + myBarPosCorrection),
                               Graphic3d_Vec2i(GetBreadth(), aFromHeight));
        theSel->Add(aBarSensFrom);
    }
    if (GetRangeTipY(mySelRangeTo) != GetRangeTipY(1.0)) {
        const int               aToHeight    = int(GetRangeTipY(1.0) - GetRangeTipY(mySelRangeTo));
        Handle(OutOfRangeOwner) aFlipOwnerTo = new OutOfRangeOwner(anObjId);
        aFlipOwnerTo->SetSelectablePointer(this);
        aFlipOwnerTo->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
            !IsReversed() ? (int)SelectionPart::OutOfRangeMax : (int)SelectionPart::OutOfRangeMin));

        Handle(Select3D_SensitivePrimitiveArray) aBarSensTo =
            getQuadSensitivity(aFlipOwnerTo,
                               Graphic3d_Vec2i(GetBarLeftX(), myBarPos.barTop - aToHeight + myBarPosCorrection),
                               Graphic3d_Vec2i(GetBreadth(), aToHeight));
        theSel->Add(aBarSensTo);
    }
}

// ================================================================
// Function : computeSelDragRangeTips
// ================================================================
void InteractiveColorScale::computeSelDragRangeTips(const Handle(SelectMgr_Selection)& theSel)
{
    const TCollection_AsciiString& anObjId = myMeshOwner->Id();
    if (GetLabelType() == Aspect_TOCSD_USER) {
        return;
    }

    // draggable range tips
    Handle(RangeTipOwner) aRangeOwnerMin = new RangeTipOwner(anObjId, RangeTipOwner::RangeSide::MinSide);
    aRangeOwnerMin->SetSize(static_cast<float>(GetBreadth()));
    aRangeOwnerMin->SetPoint(GetRangeTipMin());
    aRangeOwnerMin->SetSelectablePointer(this);
    aRangeOwnerMin->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
        !IsReversed() ? (int)SelectionPart::RangeTipMin : (int)SelectionPart::RangeTipMax));
    Handle(Select3D_SensitivePoint) aSensMin = new Select3D_SensitivePoint(aRangeOwnerMin,
                                                                           aRangeOwnerMin->GetPoint3d());
    aSensMin->SetSensitivityFactor(GetBreadth() / 2);
    theSel->Add(aSensMin);

    Handle(RangeTipOwner) aRangeOwnerMax = new RangeTipOwner(anObjId, RangeTipOwner::RangeSide::MaxSide);
    aRangeOwnerMax->SetSize(static_cast<float>(GetBreadth()));
    aRangeOwnerMax->SetPoint(GetRangeTipMax());
    aRangeOwnerMax->SetSelectablePointer(this);
    aRangeOwnerMax->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
        !IsReversed() ? (int)SelectionPart::RangeTipMax : (int)SelectionPart::RangeTipMin));
    Handle(Select3D_SensitivePoint) aSensMax = new Select3D_SensitivePoint(aRangeOwnerMax,
                                                                           aRangeOwnerMax->GetPoint3d());
    aSensMax->SetSensitivityFactor(GetBreadth() / 2);
    theSel->Add(aSensMax);
}

// ================================================================
// Function : computeSelFlipRange
// ================================================================
void InteractiveColorScale::computeSelFlipRange(const Handle(SelectMgr_Selection)& theSel)
{
    const TCollection_AsciiString& anObjId = myMeshOwner->Id();
    // @c if (GetLabelType() == Aspect_TOCSD_USER) { return; }

    const int aLeftLabFlip = GetTextLabelX() + (GetLabelPosition() == Aspect_TOCSP_LEFT ? -GetSpacing() : GetSpacing());

    Handle(RangeFlipOwner) aFlipOwnerMin = new RangeFlipOwner(anObjId);
    aFlipOwnerMin->SetSelectablePointer(this);
    aFlipOwnerMin->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
        !IsReversed() ? (int)SelectionPart::LabelMin : (int)SelectionPart::LabelMax));

    Handle(Select3D_SensitivePoint) aSensFlipMin =
        new Select3D_SensitivePoint(aFlipOwnerMin, gp_Pnt(aLeftLabFlip, GetRangeTipMin().y(), 0.0));
    theSel->Add(aSensFlipMin);

    Handle(RangeFlipOwner) aFlipOwnerMax = new RangeFlipOwner(anObjId);
    aFlipOwnerMax->SetSelectablePointer(this);
    aFlipOwnerMax->SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart(
        !IsReversed() ? (int)SelectionPart::LabelMax : (int)SelectionPart::LabelMin));

    Handle(Select3D_SensitivePoint) aSensFlipMax =
        new Select3D_SensitivePoint(aFlipOwnerMax, gp_Pnt(aLeftLabFlip, GetRangeTipMax().y(), 0.0));
    theSel->Add(aSensFlipMax);
}

// ================================================================
// Function : ProcessDragging
// ================================================================
Standard_Boolean InteractiveColorScale::ProcessDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                        const Handle(V3d_View)&               theView,
                                                        const Handle(SelectMgr_EntityOwner)&  theOwner,
                                                        const Graphic3d_Vec2i&                theDragFrom,
                                                        const Graphic3d_Vec2i&                theDragTo,
                                                        const AIS_DragAction                  theAction)
{
    if (Handle(RangeTipOwner) aRangeOwner = Handle(RangeTipOwner)::DownCast(theOwner)) {
        return ProcessRangeDragging(theContext, theView, aRangeOwner, theDragFrom, theDragTo, theAction);
    }

    if (Handle(BarDragOwner) aBarOwner = Handle(BarDragOwner)::DownCast(theOwner)) {
        return ProcessBarDragging(theContext, theView, aBarOwner, theDragFrom, theDragTo, theAction);
    }

    return false;
}

// ================================================================
// Function : ProcessRangeDragging
// ================================================================
bool InteractiveColorScale::ProcessRangeDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                 const Handle(V3d_View)&               theView,
                                                 const Handle(RangeTipOwner)&          theOwner,
                                                 const Graphic3d_Vec2i&,
                                                 const Graphic3d_Vec2i& theDragTo,
                                                 const AIS_DragAction   theAction)
{
    switch (theAction) {
        case AIS_DragAction_Start: {
            return true;
        }
        case AIS_DragAction_Update: {
            UpdateRangeDragging(theContext, theView, theOwner, theDragTo);
            return true;
        }
        case AIS_DragAction_Stop:
            theContext->RecomputeSelectionOnly(this);
            // fallthrough
        case AIS_DragAction_Abort: {
            theContext->RecomputePrsOnly(this, false, true);
            UpdateColorMap(true);
            return true;
        }
        default: return false;
    }
}

// ================================================================
// Function : setupBarDragging
// ================================================================
void InteractiveColorScale::setupBarDragging(const Handle(V3d_View)& theView, const Graphic3d_Vec2i& theDragFrom)
{
    myDragOffsetStart = myTransformPersistence->Offset2d();
    myDragCornerStart = myTransformPersistence->Corner2d();
    myDragDiffStart   = theDragFrom - IInteractiveObject::GetPointOnScreen2d(this, theView, Graphic3d_Vec2i(0, 0));
    if ((myDragCornerStart & Aspect_TOTP_RIGHT) != 0) {
        myDragDiffStart.x() = -myDragDiffStart.x();
    }
    if ((myDragCornerStart & Aspect_TOTP_BOTTOM) != 0) {
        myDragDiffStart.y() = -myDragDiffStart.y();
    }
    // If myDragDiffStart would be outside the bar, snap the bar so that its middle is under the cursor
    const int              aSide = IsHorizontal() ? 1 : 0;
    const Standard_Integer aMin  = GetBarSideSpace();
    const Standard_Integer aMax  = aMin + GetBreadth();
    if (myDragDiffStart[aSide] > aMax || myDragDiffStart[aSide] < aMin) {
        myDragDiffStart[aSide] = (aMin + aMax) / 2;
    }
}

// ================================================================
// Function : ProcessBarDragging
// ================================================================
bool InteractiveColorScale::ProcessBarDragging(const Handle(AIS_InteractiveContext)& theContext,
                                               const Handle(V3d_View)&               theView,
                                               const Handle(BarDragOwner)&,
                                               const Graphic3d_Vec2i& theDragFrom,
                                               const Graphic3d_Vec2i& theDragTo,
                                               const AIS_DragAction   theAction)
{
    switch (theAction) {
        case AIS_DragAction_Start: {
            setupBarDragging(theView, theDragFrom);
            return true;
        }
        case AIS_DragAction_Update: {
            UpdateBarDragging(theContext, theView, theDragTo);
            return true;
        }
        case AIS_DragAction_Stop: {
            theContext->RecomputeSelectionOnly(this);
            theContext->RecomputePrsOnly(this, false, true);
            return true;
        }
        case AIS_DragAction_Abort: {
            myTransformPersistence->SetOffset2d(myDragOffsetStart);
            myTransformPersistence->SetCorner2d(myDragCornerStart);
            Set2dViewOffset(myDragOffsetStart);
            theContext->RecomputePrsOnly(this, false, true);
            return true;
        }
        default: return false;
    }
}

// ================================================================
// Function : UpdateBarDragging
// ================================================================
void InteractiveColorScale::UpdateBarDragging(const Handle(AIS_InteractiveContext)& theContext,
                                              const Handle(V3d_View)&               theView,
                                              const Graphic3d_Vec2i&                theDragTo)
{
    const double                  aResRatio = theView->RenderingParams().ResolutionRatio();
    Aspect_TypeOfTriedronPosition aCorner   = Aspect_TOTP_CENTER;

    const Graphic3d_Vec2i aWinSize   = theView->Window()->Dimensions();
    Graphic3d_Vec2d       aNewOffset = Graphic3d_Vec2d(myDragOffsetStart) * aResRatio;

    processBarDragOrientationSwitch(theView, theDragTo, aWinSize, aResRatio);

    Graphic3d_Vec2d anOwnAxis = GetOrientationAxis();
    // Drag axis is perpendicular to main axis, rotated 90 degrees counterclockwise
    Graphic3d_Vec2d aDragAxis(anOwnAxis.y(), anOwnAxis.x());
    double          aProjDragTo   = aDragAxis.Dot(Graphic3d_Vec2d(theDragTo));
    double          aProjDragDiff = aDragAxis.Dot(Graphic3d_Vec2d(myDragDiffStart));
    double          aProjWinSize  = aDragAxis.Dot(Graphic3d_Vec2d(aWinSize));

    double aNewOffsetProj = 0.0;

    if (aProjDragTo > aProjWinSize / 2) {
        aNewOffsetProj = aProjWinSize - aProjDragTo - aProjDragDiff;
        aCorner = Aspect_TypeOfTriedronPosition(aCorner | (IsHorizontal() ? Aspect_TOTP_BOTTOM : Aspect_TOTP_RIGHT));
    } else {
        aNewOffsetProj = aProjDragTo - aProjDragDiff;
        aCorner        = Aspect_TypeOfTriedronPosition(aCorner | (IsHorizontal() ? Aspect_TOTP_TOP : Aspect_TOTP_LEFT));
    }
    aNewOffset     = aDragAxis * aNewOffsetProj;
    aNewOffset.x() = Max(aNewOffset.x(), 0.0);
    aNewOffset.y() = Max(aNewOffset.y(), 0.0);
    aNewOffset     = aNewOffset / aResRatio;
    myTransformPersistence->SetCorner2d(aCorner);
    myTransformPersistence->SetOffset2d(Graphic3d_Vec2i(aNewOffset));
    Set2dViewOffset(Graphic3d_Vec2i(aNewOffset));
    theContext->RecomputePrsOnly(this, false, true);

    if (myLastDetectedInterval != -1) {
        myLastDetectedInterval = -1;

        if (Handle(Prs3d_Presentation) aPrs = GetHilightPresentation(Handle(PrsMgr_PresentationManager)())) {
            aPrs->Erase();
        }

        if (myToHighlightIntervals) {
            UpdateColorMap(false);
        }
    }
}

// ================================================================
// Function : processBarDragOrientationSwitch
// ================================================================
void InteractiveColorScale::processBarDragOrientationSwitch(const Handle(V3d_View)& theView,
                                                            const Graphic3d_Vec2i&  theDragTo,
                                                            const Graphic3d_Vec2i&  theWinSize,
                                                            const double            theResRatio)
{
    Aspect_TypeOfTriedronPosition aPos = Aspect_TOTP_CENTER;

    // Fixed zone size
    static constexpr float aZoneSizePx = 50.0f;
    Graphic3d_Vec2i        anOrientationSwitchZone((int)(aZoneSizePx * theResRatio), (int)(aZoneSizePx * theResRatio));

    if (theDragTo.x() < anOrientationSwitchZone.x()) {
        aPos = Aspect_TypeOfTriedronPosition(aPos | Aspect_TOTP_LEFT);
    }
    if (theDragTo.x() > theWinSize.x() - anOrientationSwitchZone.x()) {
        aPos = Aspect_TypeOfTriedronPosition(aPos | Aspect_TOTP_RIGHT);
    }
    if (theDragTo.y() < anOrientationSwitchZone.y()) {
        aPos = Aspect_TypeOfTriedronPosition(aPos | Aspect_TOTP_TOP);
    }
    if (theDragTo.y() > theWinSize.y() - anOrientationSwitchZone.y()) {
        aPos = Aspect_TypeOfTriedronPosition(aPos | Aspect_TOTP_BOTTOM);
    }

    if (aPos != Aspect_TOTP_CENTER) { // Indicates that the bar has been dragged to a orientation switch zone
        bool isNewHorizontal = IsHorizontal();
        switch (aPos) {
            case Aspect_TOTP_TOP:
            case Aspect_TOTP_BOTTOM: isNewHorizontal = true; break;
            case Aspect_TOTP_LEFT:
            case Aspect_TOTP_RIGHT: isNewHorizontal = false; break;
            default: // Corner zones shall be ignored, otherwise rapid switching will occur.
                break;
        }

        if (isNewHorizontal != IsHorizontal()) {
            SetPositionSide(aPos);
            setupBarDragging(theView, theDragTo);
        }
    }
}

// ================================================================
// Function : updateOrientationTransform
// ================================================================
void InteractiveColorScale::updateOrientationTransform()
{
    bool anOldIsHorizontal = (myTransformationPosSide == Aspect_TOTP_TOP)
                          || (myTransformationPosSide == Aspect_TOTP_BOTTOM);
    if (IsHorizontal() == anOldIsHorizontal) {
        return;
    }

    if (IsHorizontal()) {
        gp_Ax2d aSymmetryAxis(gp_Pnt2d(0.0, 0.0), gp_Dir2d(1.0, 1.0));

        gp_Trsf2d aMirrorTrsf2d;
        aMirrorTrsf2d.SetMirror(aSymmetryAxis);
        gp_Trsf aMirrorTrsf3d(aMirrorTrsf2d);

        SetLocalTransformation(aMirrorTrsf3d);
    } else {
        SetLocalTransformation(gp_Trsf());
    }
    if (HasInteractiveContext()) {
        // update transformation of dynamically highlighted presentation with ColorScaleDispMode::Frame display mode
        const Standard_Integer aHiMode = myDynHilightDrawer->DisplayMode();
        InteractiveContext()->MainPrsMgr()->UpdateHighlightTrsf(InteractiveContext()->CurrentViewer(), this, aHiMode);
    }

    myTransformationPosSide = GetPositionSide();
}

// ================================================================
// Function : UpdateRangeDragging
// ================================================================
void InteractiveColorScale::UpdateRangeDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                const Handle(V3d_View)&               theView,
                                                const Handle(RangeTipOwner)&          theOwner,
                                                const Graphic3d_Vec2i&                theDragTo)
{
    const double aResRatio = theView->RenderingParams().ResolutionRatio();

    const Graphic3d_Vec2i aBarBotScreen = IInteractiveObject::GetPointOnScreen2d(
        this,
        theView,
        Graphic3d_Vec2i(Graphic3d_Vec2d(Graphic3d_Vec2i(GetRangeTipX(),
                                                         (int)GetRangeTipY(0.0) + myBarPosCorrection))
                        * aResRatio));
    const Graphic3d_Vec2i aBarTopScreen = IInteractiveObject::GetPointOnScreen2d(
        this,
        theView,
        Graphic3d_Vec2i(Graphic3d_Vec2d(Graphic3d_Vec2i(GetRangeTipX(),
                                                         (int)GetRangeTipY(1.0) + myBarPosCorrection))
                        * aResRatio));

    Graphic3d_Vec2d aBarVec(aBarTopScreen - aBarBotScreen);
    const double    aBarLength = aBarVec.Modulus();
    if (aBarLength <= 0.0) {
        return;
    }

    const Graphic3d_Vec2d aDragVec(theDragTo - aBarBotScreen);
    double                aNewPos = Max(0.0, Min(1.0, aDragVec.Dot(aBarVec) / (aBarLength * aBarLength)));
    if (theOwner->GetTipSide() == RangeTipOwner::RangeSide::MinSide) {
        aNewPos = Max(0.0, Min(mySelRangeTo - 8.0 / aBarLength, aNewPos));
        if (mySelRangeFrom == aNewPos) {
            return;
        }

        mySelRangeFrom = aNewPos;
    } else if (theOwner->GetTipSide() == RangeTipOwner::RangeSide::MaxSide) {
        aNewPos = Min(1.0, Max(mySelRangeFrom + 8.0 / aBarLength, aNewPos));
        if (mySelRangeTo == aNewPos) {
            return;
        }

        mySelRangeTo = aNewPos;
    }

    UpdateColorMap(true);
    Graphic3d_Vec2d aNewPoint = Graphic3d_Vec2d(GetRangeTipX(), GetRangeTipY(aNewPos) + myBarPosCorrection);

    theOwner->SetPoint(Graphic3d_Vec2i(aNewPoint));
    theContext->RecomputePrsOnly(this, false, true);
}

// ================================================================
// Function : ClearDynamicHighlight
// ================================================================
void InteractiveColorScale::ClearDynamicHighlight(const Handle(PrsMgr_PresentationManager)& theMgr)
{
    AIS_InteractiveObject::ClearDynamicHighlight(theMgr);

    if (!ToHighlightIntervals()) {
        return;
    }

    if (!HasInteractiveContext()) {
        return;
    }

    Handle(InteractiveColorScale::BarDragOwner) aBarDragOwner = Handle(InteractiveColorScale::BarDragOwner)::DownCast(
        InteractiveContext()->DetectedOwner());
    if (aBarDragOwner.IsNull() || aBarDragOwner->Selectable().IsNull()) {
        return;
    }

    aBarDragOwner->Unhilight(theMgr, 0);
}

// ================================================================
// Function : SetOpaqueOutOfRange
// ================================================================
void InteractiveColorScale::SetOpaqueOutOfRange(const bool theIsOpaque)
{
    if (IsOpaqueOutOfRange() == theIsOpaque) {
        return;
    }

    Quantity_ColorRGBA aColor = GetOutOfRangeColor();
    if (theIsOpaque) {
        myOutOfRangeAlpha = aColor.Alpha();
        aColor.SetAlpha(1.0f);
    }
    if (myOutOfRangeAlpha >= 0.99f) {
        myOutOfRangeAlpha = 0.0f;
    }

    aColor.SetAlpha(theIsOpaque ? 1.0f : myOutOfRangeAlpha);
    SetOutOfRangeColor(aColor);
}

// ================================================================
// Function : GetTextLabelX
// ================================================================
int InteractiveColorScale::GetTextLabelX() const
{
    int                  anXLeft = 0;
    static constexpr int aSize2  = 2;
    if (IsHorizontal()) {
        Aspect_TypeOfTriedronPosition aPosSide = GetPositionSide();
        if (aPosSide == Aspect_TOTP_TOP) {
            anXLeft = GetBarLeftX() - GetSpacing() - GetTextHeight() / aSize2;
        } else {
            anXLeft = GetBarLeftX() + GetBreadth() + GetTextHeight();
        }
    } else {
        anXLeft = GetXPosition() + GetBreadth() + GetSpacing() * aSize2;
        if (GetLabelPosition() == Aspect_TOCSP_LEFT) {
            anXLeft = -anXLeft;
        } else if (GetLabelPosition() == Aspect_TOCSP_CENTER) {
            anXLeft = 0;
        }
    }
    return anXLeft;
}

// ================================================================
// Function : GetIntervalValueEx
// ================================================================
double InteractiveColorScale::GetIntervalValueEx(const int theIndex) const
{
    if (GetNumberOfIntervals() <= 0) {
        return 0.0;
    }

    double aRangeFull      = GetMax() - GetMin();
    double aSelRangeLength = mySelRangeTo - mySelRangeFrom;
    double aScaledRange    = aRangeFull * aSelRangeLength;

    if (IsLogarithmic()) {
        // Normalize to [0, 1] range
        const double aRangeValue           = (double)theIndex / (double)GetNumberOfIntervals();
        const double aRangeValueInSelRange = aRangeValue * aSelRangeLength + mySelRangeFrom;
        const double aNormalizedMeshValue  = LogTransformRangeToMeshValue(aRangeValueInSelRange);
        return GetMin() + aRangeFull * aNormalizedMeshValue;
    }

    if (IsReversed()) {
        double aMin = GetMin() + (1.0 - mySelRangeTo) * aRangeFull;
        return aMin + theIndex * (Abs(aScaledRange) / GetNumberOfIntervals());
    } else {
        double aMin = GetMin() + mySelRangeFrom * aRangeFull;
        return aMin + theIndex * (Abs(aScaledRange) / GetNumberOfIntervals());
    }
}

// ================================================================
// Function : LogTransformRangeToMeshValue
// ================================================================
double InteractiveColorScale::LogTransformRangeToMeshValue(double theRangeValue) const
{
    if (!IsLogarithmic())
        return 0.0;

    theRangeValue = Min(Max(theRangeValue, 0.0), 1.0);

    const double aMin = GetMin();
    const double aMax = GetMax();

    const auto LinearMap = [this](double theArg) {
        return myLogTransformScale * theArg + myLogTransformBias;
    };

    const auto LinearUnmap = [this](double theArg) {
        return (theArg - myLogTransformBias) / myLogTransformScale;
    };

    const double aLogScaleFrom         = LogScale(LinearMap(aMin));
    const double aLogScaleTo           = LogScale(LinearMap(aMax));
    const double aLogScaleRange        = aLogScaleTo - aLogScaleFrom;
    const double aMappedValueFreerange = theRangeValue * aLogScaleRange + aLogScaleFrom;
    const double aValueToUnmap         = InvLogScale(aMappedValueFreerange);
    const double aMeshValue            = LinearUnmap(aValueToUnmap);

    const double aMappedValue = (aMeshValue - aMin) / (aMax - aMin);

    return aMappedValue;
}

// ================================================================
// Function : LogTransformMeshValueToRange
// ================================================================
double InteractiveColorScale::LogTransformMeshValueToRange(double theNormMeshValue) const
{
    if (!IsLogarithmic())
        return 0.0;

    theNormMeshValue = Min(Max(theNormMeshValue, 0.0), 1.0);

    const double aMin = GetMin();
    const double aMax = GetMax();

    const auto LinearMap = [this](double theArg) {
        return myLogTransformScale * theArg + myLogTransformBias;
    };

    const double aMeshValue = aMin + (aMax - aMin) * theNormMeshValue;

    double aValueToMap           = LinearMap(aMeshValue);
    double aMappedValueFreerange = LogScale(aValueToMap);
    double aLogScaleFrom         = LogScale(LinearMap(aMin));
    double aLogScaleTo           = LogScale(LinearMap(aMax));
    double aLogScaleRange        = aLogScaleTo - aLogScaleFrom;
    double aMappedValue          = (aMappedValueFreerange - aLogScaleFrom) / aLogScaleRange;

    // Define behaviour with NaNs and infinities
    if (std::isnan(aMappedValue) || std::isinf(aMappedValue)) {
        if (std::isnan(aMappedValue) || aMappedValue < 0) {
            aMappedValue = 0.0;
        } else { // +infinity
            aMappedValue = 1.0;
        }
    }

    return aMappedValue;
}

// ================================================================
// Function : updateRangeToLogMappingCoeffs
// ================================================================
void InteractiveColorScale::updateRangeToLogMappingCoeffs()
{
    if (!IsLogarithmic())
        return;

    double       aMin = GetMin();
    const double aMax = GetMax();

    // For logarithmic color scale mapping, value is first linearly mapped into [1, aLogNorm] range,
    // then directly mapped by logarithm to a color.
    // For symmetric log mode, the linear mapping range is scaled so that the mesh property value
    // that is the largest by the modulus lies on one end of the [-aLogNorm, aLogNorm] range.

    // Parameter for log norm range
    static constexpr double aLogNorm = 256.0;

    // Calculate range boundaries
    // Initially assume scale is not symmetric log scale
    double aLogRangeBegin            = 1.0;
    double aLogRangeEnd              = aLogNorm;
    double aMeshValueNormRangeLength = aMax - aMin;

    if (IsSymLog()) {
        if (aMin < 0 && aMax > 0) {
            const double aLogMin = LogScale(aMin);
            const double aLogMax = LogScale(aMax);
            if (Abs(aLogMax) > Abs(aLogMin)) {
                aLogRangeBegin            = 0;
                aLogRangeEnd              = aLogNorm;
                aMeshValueNormRangeLength = aMax;
                aMin                      = 0.0;
            } else {
                aLogRangeBegin            = -aLogNorm;
                aLogRangeEnd              = 0;
                aMeshValueNormRangeLength = -aMin;
            }
        } else {
            aLogRangeBegin *= -1.0;
            aLogRangeEnd *= -1.0;
        }
    }

    // Now we've got which range the mesh values map to, then we calculate linear mapping
    // coefficients to squish the values into the above log range.
    const double aMeshValueToLogRangeScale = (aLogRangeEnd - aLogRangeBegin) / aMeshValueNormRangeLength;
    const double aMeshValueToLogRangeBias  = aLogRangeBegin - aMin * aMeshValueToLogRangeScale;

    myLogTransformScale = aMeshValueToLogRangeScale;
    myLogTransformBias  = aMeshValueToLogRangeBias;
    return;
}

// ================================================================
// Function : SetLogarithmic
// ================================================================
void InteractiveColorScale::SetLogarithmic(const Standard_Boolean isLogarithmic)
{
    AIS_ColorScale::SetLogarithmic(isLogarithmic);

    if (IsLogarithmic()) {
        if (GetMin() < 0)
            setSymLog(true);

        updateRangeToLogMappingCoeffs();
    }
}

// ================================================================
// Function : SetRange
// ================================================================
void InteractiveColorScale::SetRange(const Standard_Real theMin, const Standard_Real theMax)
{
    AIS_ColorScale::SetRange(theMin, theMax);

    if (IsLogarithmic()) {
        if (theMin < 0)
            setSymLog(true);

        updateRangeToLogMappingCoeffs();
    }
}

// ================================================================
// Function : SymLog
// ================================================================
double InteractiveColorScale::SymLog(double theArg)
{
    const auto signum = [](double theNum) -> double {
        return (theNum > 0.0) ? 1.0 : ((theNum < 0.0) ? -1.0 : 0.0);
    };
    return signum(theArg) * (std::log(Abs(theArg / aSymLogNormConstant) + 1.0));
}

// ================================================================
// Function : InvSymLog
// ================================================================
double InteractiveColorScale::InvSymLog(double theArg)
{
    const auto signum = [](double theNum) -> double {
        return (theNum > 0.0) ? 1.0 : ((theNum < 0.0) ? -1.0 : 0.0);
    };
    return signum(theArg) * Abs(aSymLogNormConstant) * (std::exp(Abs(theArg)) - 1.0);
}

// ================================================================
// Function : GetMappedRangeTo
// ================================================================
double InteractiveColorScale::GetMappedRangeTo() const
{
    double aDispRange = GetMax() - GetMin();
    double aMapMax    = GetMin() + mySelRangeTo * aDispRange;
    double aFullRange = myDefRangeMax - myDefRangeMin;
    if (aFullRange <= 0.0) {
        return mySelRangeTo;
    }
    return (aMapMax - myDefRangeMin) / aFullRange;
}

} // namespace
