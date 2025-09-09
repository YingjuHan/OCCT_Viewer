#include <ViewCae/MeshColorScaleMapper.h>

#include <ViewCae/CaeMeshTools.h>
#include <ViewCae/InteractiveColorScaleOwner.h>

#include <Image_SupportedFormats.hxx>

namespace ViewCae {

// =======================================================================
// function : MeshColorScaleMapper
// =======================================================================
MeshColorScaleMapper::MeshColorScaleMapper(const Handle(InteractiveColorScale)& theColorScaleIO)
    : MeshColorMapper(),
      myColorScaleIO(theColorScaleIO),
      myIsGroupPropAttached(false),
      myIsVisible(true)
{
    //
}

// =======================================================================
// function : SetMeshProperty
// =======================================================================
void MeshColorScaleMapper::SetMeshProperty(const Handle(RWCae::MeshNVecProperty)& theProperty,
                                           int                                      theComponent,
                                           bool                                     theToUpdate)
{
    myMeshProperty          = theProperty;
    myMeshPropertyComponent = theComponent;

    if (!theToUpdate) {
        return;
    }

    SynchronizeValues();
    UpdateColorTexture();

    if (myOnPropertyUpdateCallback != nullptr) {
        myOnPropertyUpdateCallback(this);
    }
}

// =======================================================================
// function : ValuesRange
// =======================================================================
Bnd_Range MeshColorScaleMapper::ValuesRange() const
{
    Bnd_Range aRes = MeshColorMapper::ValuesRange();
    if (aRes.IsVoid()) {
        return aRes;
    }

    const double aSCRangeFrom = myColorScaleIO->GetMappedRangeFrom();
    const double aSCRangeTo   = myColorScaleIO->GetMappedRangeTo();

    double aMinValue;
    double aMaxValue;
    aRes.GetBounds(aMinValue, aMaxValue);
    double aSize = aMaxValue - aMinValue;

    aMaxValue = aMinValue + aSize * aSCRangeTo;
    aMinValue = aMinValue + aSize * aSCRangeFrom;
    aRes      = Bnd_Range(aMinValue, aMaxValue);
    return aRes;
}

// =======================================================================
// function : SetVisible
// =======================================================================
void MeshColorScaleMapper::SetVisible(bool theIsVisible)
{
    if (myIsVisible == theIsVisible) {
        return;
    }

    myIsVisible = theIsVisible;
    if (myColorScaleIO->HasInteractiveContext()) {
        myColorScaleIO->InteractiveContext()->SetDisplayMode(myColorScaleIO, myIsVisible ? 0 : 1, Standard_False);
        myColorScaleIO->SetToUpdate();
    }

    if (myOnPropertyUpdateCallback != nullptr) {
        myOnPropertyUpdateCallback(this);
    }
}

// =======================================================================
// function : SynchronizeValues
// =======================================================================
void MeshColorScaleMapper::SynchronizeValues()
{
    if (myColorScaleIO.IsNull()) {
        return;
    }

    if (myMeshProperty.IsNull()) {
        myIsVisible = true;
        if (myColorScaleIO->HasInteractiveContext()) {
            myColorScaleIO->InteractiveContext()->SetDisplayMode(myColorScaleIO, 1, Standard_False);
            myColorScaleIO->MeshOwner()->SetSelectionMode(1);
            myColorScaleIO->SetToUpdate();
        }
        return;
    }

    const Handle(RWCae::MeshScalarPropertyBase)& aComp = myMeshProperty->GetRawComponent(myMeshPropertyComponent);

    if (Handle(RWCae::MeshGroupProperty) aGroupProp = Handle(RWCae::MeshGroupProperty)::DownCast(aComp)) {
        setForGroupProp(aGroupProp);
    } else {
        setForScalarProp(aComp);
    }

    TCollection_AsciiString aNVecTitle = !myMeshProperty->Description().IsEmpty() ? myMeshProperty->Description()
                                                                                  : myMeshProperty->Name();
    TCollection_AsciiString aCompTitle = !aComp->Description().IsEmpty() ? aComp->Description() : aComp->Name();
    TCollection_AsciiString aTitle     = aCompTitle.IsEmpty() ? aNVecTitle
                                       : aNVecTitle.IsEmpty() ? aCompTitle
                                                              : aNVecTitle + " : " + aCompTitle;
    myColorScaleIO->SetDisplayUnits(aComp->DisplayUnits());
    myColorScaleIO->SetTitle(aTitle);
    if (myColorScaleIO->HasInteractiveContext()) {
        myColorScaleIO->InteractiveContext()->SetDisplayMode(myColorScaleIO, myIsVisible ? 0 : 1, Standard_False);
        myColorScaleIO->MeshOwner()->SetSelectionMode(myIsVisible ? 0 : 1);
    }
    myColorScaleIO->SetToUpdate();
}

// =======================================================================
// function : setForGroupProp
// =======================================================================
void MeshColorScaleMapper::setForGroupProp(const Handle(RWCae::MeshGroupProperty)& theProp)
{
    double aValueRangeMin = 0.0;
    double aValueRangeMax = 0.0;
    theProp->ValueRange().GetBounds(aValueRangeMin, aValueRangeMax);
    int aMin = (int)Round(aValueRangeMin);
    int aMax = (int)Round(aValueRangeMax);
    myColorScaleIO->SetDefaultRange(aMin, aMax);

    int aNbIntervals = aMax - aMin + 1;
    myColorScaleIO->SetNumberOfIntervals(aNbIntervals);
    myColorScaleIO->SetSmoothTransition(false);
    myColorScaleIO->SetReversed(true); // list groups from top-to-down
    myColorScaleIO->SetColorType(Aspect_TOCSD_USER);

    Aspect_SequenceOfColor aCustomColors;

    float aHueOffset = 360.0f / aNbIntervals;
    for (int i = 0; i < aNbIntervals; i++) {
        Graphic3d_Vec3 aColorVec(i * aHueOffset, 1.0f, 1.0f);
        Quantity_Color aColor(Quantity_Color::Convert_HLS_To_LinearRGB(aColorVec));
        aCustomColors.Append(aColor);
    }
    myColorScaleIO->SetColors(aCustomColors);
    myColorScaleIO->SetLabelAtBorder(false);

    myColorScaleIO->SetLabelType(Aspect_TOCSD_USER);
    TColStd_SequenceOfExtendedString aLabels;
    for (int i = aMin; i <= aMax; i++) {
        aLabels.Append(theProp->GetGroupName(i));
    }
    myColorScaleIO->SetLabels(aLabels);

    myIsGroupPropAttached = true;
}

// =======================================================================
// function : setForScalarProp
// =======================================================================
void MeshColorScaleMapper::setForScalarProp(const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    double aValueRangeMin = 0.0;
    double aValueRangeMax = 0.0;
    theProp->ValueRange().GetBounds(aValueRangeMin, aValueRangeMax);
    double aSCRangeFrom = 0.0;
    double aSCRangeTo   = 1.0;
    double aMin         = aValueRangeMin + aSCRangeFrom * (aValueRangeMax - aValueRangeMin);
    double aMax         = aValueRangeMin + aSCRangeTo * (aValueRangeMax - aValueRangeMin);
    myColorScaleIO->SetSelectedRange(aSCRangeFrom, aSCRangeTo);
    myColorScaleIO->SetDefaultRange(aMin, aMax);

    if (myIsGroupPropAttached) {
        myColorScaleIO->SetNumberOfIntervals(3);
        myColorScaleIO->SetSmoothTransition(true);
        myColorScaleIO->SetColorType(Aspect_TOCSD_USER);
        Aspect_SequenceOfColor aCustomColors;
        aCustomColors.Append(Quantity_NOC_BLUE);
        aCustomColors.Append(Quantity_NOC_GREEN);
        aCustomColors.Append(Quantity_NOC_RED);
        myColorScaleIO->SetColors(aCustomColors);
        myColorScaleIO->SetLabelAtBorder(true);
        myColorScaleIO->SetLabelType(Aspect_TOCSD_AUTO);
    }

    myIsGroupPropAttached = false;
}

// =======================================================================
// function : ColorTextureTransparency
// =======================================================================
float MeshColorScaleMapper::ColorTextureTransparency() const
{
    float aMinBlendAlpha  = 1.0f;
    bool  hasMinMaskAlpha = false;
    for (const Quantity_ColorRGBA& aColIter : myColorScaleIO->GetCustomColors()) {
        if (aColIter.Alpha() < 0.01f) {
            hasMinMaskAlpha = true;
        } else {
            aMinBlendAlpha = Min(aMinBlendAlpha, aColIter.Alpha());
        }
    }
    if (myColorScaleIO->GetOutOfRangeColor().Alpha() < 0.01f) {
        hasMinMaskAlpha = true;
    } else {
        aMinBlendAlpha = Min(aMinBlendAlpha, myColorScaleIO->GetOutOfRangeColor().Alpha());
    }

    if (aMinBlendAlpha < 0.99f) {
        return 1.0f - aMinBlendAlpha;
    }

    return hasMinMaskAlpha ? 1.0f : 0.0f;
}

//! Set pixel color.
static void setPixmapColor(Image_PixMap& thePixmap, const Graphic3d_Vec4& theColor, const int theRow, const int theCol)
{
    static constexpr float byteMax255f = 255.0f;

    Graphic3d_Vec4ub& aColor0 = thePixmap.ChangeValue<Graphic3d_Vec4ub>(theRow, theCol);

    aColor0 = Graphic3d_Vec4ub(Quantity_ColorRGBA::Convert_LinearRGB_To_sRGB(theColor) * byteMax255f);
}

// =======================================================================
// function : computeIntervalColors
// =======================================================================
void MeshColorScaleMapper::computeIntervalColors(NCollection_Array1<Graphic3d_Vec4>& theColors,
                                                 int                                 theNbIntervals) const
{
    const bool toReverse = myColorScaleIO->IsReversed() && myColorScaleIO->GetLabelType() != Aspect_TOCSD_USER;
    for (int anIntIter = 1; anIntIter <= theNbIntervals; ++anIntIter) {
        bool toHighlightInterval = myColorScaleIO->ToHighlightIntervals()
                                && myColorScaleIO->LastDetectedInterval() == anIntIter;
        Quantity_ColorRGBA aColor = !toHighlightInterval ? myColorScaleIO->GetIntervalColor(anIntIter)
                                                         : Quantity_ColorRGBA(myColorScaleIO->IntervalHighlightColor());
        const int          anIdx  = !toReverse ? anIntIter : theNbIntervals - anIntIter + 1;
        theColors[anIdx]          = Graphic3d_Vec4(aColor);
    }
    theColors[theNbIntervals + 1] = theColors[theNbIntervals]; // to check the math
}

// =======================================================================
// function : updateColorTexture
// =======================================================================
void MeshColorScaleMapper::updateColorTexture()
{
    const int            aNbColors = CaeMeshTools::NbColorscaleColors;
    Handle(Image_PixMap) anImage   = myColorTexture->GetImage(new Image_SupportedFormats());
    if (anImage.IsNull() || !anImage->InitTrash(Image_Format_RGBA, aNbColors, m_nbImgRows2)) {
        return;
    }

    const bool isUserType = (myColorScaleIO->GetColorType() == Aspect_TypeOfColorScaleData::Aspect_TOCSD_USER);
    const bool isSmooth   = myColorScaleIO->IsSmoothTransition();

    const double aSCRangeFrom = myColorScaleIO->GetMappedRangeFrom();
    const double aSCRangeTo   = myColorScaleIO->GetMappedRangeTo();

    const Graphic3d_Vec4 aColorOutOfRangeF = myColorScaleIO->GetOutOfRangeColor();

    const int aPrevNbIntervals = myColorScaleIO->GetNumberOfIntervals();
    const int aNbIntervals     = !isUserType && isSmooth ? 5 : aPrevNbIntervals;
    myColorScaleIO->SetNumberOfIntervals(aNbIntervals);

    NCollection_Array1<Graphic3d_Vec4> aColors(1, aNbIntervals + 1);
    computeIntervalColors(aColors, aNbIntervals);

    for (size_t aColIter = 0; aColIter < size_t(aNbColors); ++aColIter) {
        const double aValue       = double(aColIter) / double(aNbColors);
        const double aCurInterval = (aValue - aSCRangeFrom) / (aSCRangeTo - aSCRangeFrom)
                                  * (!isUserType && isSmooth ? aNbIntervals - 1 : aNbIntervals);
        double      anIntervalD = 0.0;
        const float aRatio      = float(modf(aCurInterval, &anIntervalD));
        const int   anInterval  = int(anIntervalD) + 1;

        Graphic3d_Vec4 aColVal;
        if (aValue < aSCRangeFrom || aValue > aSCRangeTo) {
            aColVal = aColorOutOfRangeF;
        } else if (isUserType && isSmooth) {
            if ((anInterval == 1 && aRatio < m_half) || (anInterval == aNbIntervals && aRatio > m_half)) {
                aColVal = aColors[anInterval];
            } else if (aRatio < m_half) {
                aColVal = aColors[anInterval - 1] * (m_half - aRatio) + aColors[anInterval] * (aRatio + m_half);
            } else {
                aColVal = aColors[anInterval] * (m_oneAndHalf - aRatio) + aColors[anInterval + 1] * (aRatio - m_half);
            }
        } else if (isSmooth) {
            aColVal = aColors[anInterval] * (1.0f - aRatio) + aColors[anInterval + 1] * aRatio;
        } else {
            aColVal = aColors[anInterval];
        }

        setPixmapColor(*anImage, aColVal, 0, aColIter);
    }
    for (size_t aColIter = 0; aColIter < anImage->SizeX(); ++aColIter) {
        setPixmapColor(*anImage, aColorOutOfRangeF, 1, aColIter);
    }

    myColorScaleIO->SetNumberOfIntervals(aPrevNbIntervals);
    myColorTexture->GetParams()->SetFilter(isSmooth ? Graphic3d_TOTF_NEAREST : Graphic3d_TOTF_BILINEAR);
}

} // namespace

