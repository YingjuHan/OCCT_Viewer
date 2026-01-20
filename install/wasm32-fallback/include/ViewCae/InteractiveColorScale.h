#ifndef ViewerCAE_INTERACTIVECOLORSCALE_HEADERFILE
#define ViewerCAE_INTERACTIVECOLORSCALE_HEADERFILE

#include <ViewCae/InteractiveCaeMesh.h>

#include <ViewBase/UnitManager.h>

#include <AIS_ColorScale.hxx>
#include <NCollection_IncAllocator.hxx>

namespace ViewCae {
class InteractiveColorScaleOwner; // forward declared class due to cyclic dependency

//! Display modes for InteractiveColorScale.
enum class ColorScaleDispMode { Full, Hidden, Frame };

//! Interactive object for color scale presentation.
//! This subclass extends AIS_ColorScale presentation with extra properties and customized look.
class InteractiveColorScale : public AIS_ColorScale, public ViewBase::IInteractiveObject {
    InteractiveColorScale()                             = delete;
    InteractiveColorScale(const InteractiveColorScale&) = delete;
    InteractiveColorScale& operator=(const InteractiveColorScale&) = delete;

public:
    //! Picking regions.
    enum class SelectionPart {
        ColorBar = 0,
        RangeTipMin,
        RangeTipMax,
        LabelMin,
        LabelMax,
        OutOfRangeMin,
        OutOfRangeMax,
    };

public:
    //! Default constructor.
    Standard_EXPORT explicit InteractiveColorScale(const Handle(ViewCae::InteractiveColorScaleOwner)& theNewId);

    //! Destructor.
    Standard_EXPORT virtual ~InteractiveColorScale();

    //! Return the mesh owner of current interactive object.
    const Handle(ViewCae::InteractiveColorScaleOwner)& MeshOwner() const { return myMeshOwner; }

    //! Attach this color scale to mesh for specified property.
    Standard_EXPORT void Attach(const Handle(InteractiveCaeMesh)&                          theMeshIO,
                                const RWCae::MeshPropertyComponent&                      theProp,
                                const std::function<void(const Handle(MeshColorMapper)&)>& theOnPropertyUpdateCallback);

    //! Compute a color map.
    Standard_EXPORT void UpdateColorMap(bool theToUpdateRange);

    //! Return base breadth.
    float GetBaseBreadth() const { return myBaseBreadth; }

    //! Set base breadth.
    void SetBaseBreadth(float theBreadth)
    {
        if (myBaseBreadth != theBreadth) {
            myBaseBreadth = theBreadth;
            SetToUpdate();
        }
    }

    //! Return base height.
    float GetBaseHeight() const { return myBaseHeight; }

    //! Set base height.
    void SetBaseHeight(float theHeight)
    {
        if (myBaseHeight != theHeight) {
            myBaseHeight = theHeight;
            SetToUpdate();
        }
    }

    //! Return max base height (as ratio for window height if < 1.0 and in pixels otherwise).
    float GetMaxBaseHeight() const { return myMaxBaseHeight; }

    //! Set max base height (as ratio for window height if < 1.0 and in pixels otherwise).
    void SetMaxBaseHeight(float theHeight)
    {
        if (myMaxBaseHeight != theHeight) {
            myMaxBaseHeight = theHeight;
            SetToUpdate();
        }
    }

    //! Return TRUE if empty custom labels should be skipped within legend; FALSE by default.
    //! Affects only custom labels equal to number of custom colors and discrete style.
    bool ToSkipEmptyLabels() const { return myToSkipEmptyLabels; }

    //! Set if empty custom labels should be skipped within legend.
    void SetSkipEmptyLabels(bool theToSkip)
    {
        if (myToSkipEmptyLabels != theToSkip) {
            myToSkipEmptyLabels = theToSkip;
            SetToUpdate();
        }
    }

    //! Sets the range of colorscale values.
    //! If theMin is negative when the scale is logarithmic, the scale will use symlog mode.
    Standard_EXPORT virtual void SetRange(const Standard_Real theMin, const Standard_Real theMax) override;

    //! Return default range lower value, 0.0 by default.
    double GetDefaultRangeMin() const { return myDefRangeMin; }

    //! Return default range upper value, 1.0 by default.
    double GetDefaultRangeMax() const { return myDefRangeMax; }

    //! Set default range of values and set display range to match it.
    void SetDefaultRange(double theMin, double theMax)
    {
        SetRange(theMin, theMax);
        myDefRangeMin = GetMin();
        myDefRangeMax = GetMax();
        SetToUpdate();
    }

    //! Return effective mapping range lower value, 0.0 by default.
    double GetMappedRangeFrom() const
    {
        double aDispRange = GetMax() - GetMin();
        double aMapMin    = GetMin() + mySelRangeFrom * aDispRange;
        double aFullRange = myDefRangeMax - myDefRangeMin;
        if (aFullRange <= 0.0) {
            return mySelRangeFrom;
        }
        return (aMapMin - myDefRangeMin) / aFullRange;
    }

    //! Return effective mapping range upper value, 1.0 by default.
    Standard_EXPORT double GetMappedRangeTo() const;

    //! Return selected range lower value, 0.0 by default.
    double GetSelectedRangeFrom() const { return mySelRangeFrom; }

    //! Set selected range lower value.
    void SetSelectedRangeFrom(double theVal)
    {
        if (mySelRangeFrom == theVal) {
            return;
        }
        mySelRangeFrom = theVal;
        SetToUpdate();
    }

    //! Return selected range upper value, 1.0 by default.
    double GetSelectedRangeTo() const { return mySelRangeTo; }

    //! Set selected range upper value.
    void SetSelectedRangeTo(double theVal)
    {
        if (mySelRangeTo == theVal) {
            return;
        }
        mySelRangeTo = theVal;
        SetToUpdate();
    }

    //! Set selected range.
    void SetSelectedRange(double theLower, double theUpper)
    {
        if (mySelRangeFrom == theLower && mySelRangeTo == theUpper) {
            return;
        }
        mySelRangeFrom = theLower;
        mySelRangeTo   = theUpper;
        SetToUpdate();
    }

    //! Return color for outer range.
    const Quantity_ColorRGBA& GetOutOfRangeColor() const { return myOutOfRangeAsp->InteriorColorRGBA(); }

    //! Set color for outer range.
    void SetOutOfRangeColor(const Quantity_ColorRGBA& theColor)
    {
        myOutOfRangeAsp->SetInteriorColor(theColor);
        if (mySelRangeFrom != 0.0 || mySelRangeTo != 1.0) {
            SetToUpdate();
        }
    }

    //! Returns TRUE if outer range should be displayed as opaque or transparent gray.
    bool IsOpaqueOutOfRange() const { return GetOutOfRangeColor().Alpha() > 0.99f; }

    //! Sets if outer range should be displayed as opaque or transparent gray.
    Standard_EXPORT void SetOpaqueOutOfRange(const bool theIsOpaque);

    //! Returns the user specified color from color map with index (starts at 1).
    //! Returns default color if index is out of range in color map.
    Quantity_ColorRGBA GetIntervalColor(const int theIndex) const
    {
        if (GetColorType() == Aspect_TOCSD_USER) {
            if (theIndex <= 0 || theIndex > myCustomColors.Length()) {
                return Quantity_ColorRGBA();
            }
            return myCustomColors.Value(theIndex);
        }
        return Quantity_ColorRGBA(AIS_ColorScale::GetIntervalColor(theIndex));
    }

    //! Return custom RGBA colors.
    const NCollection_Sequence<Quantity_ColorRGBA>& GetCustomColors() const { return myCustomColors; }

    //! Set custom RGBA colors.
    void SetCustomColors(const NCollection_Sequence<Quantity_ColorRGBA>& theColors)
    {
        myCustomColors = theColors;

        Aspect_SequenceOfColor aColors;
        for (const Quantity_ColorRGBA& aColIter : theColors) {
            aColors.Append(aColIter.GetRGB());
        }
        AIS_ColorScale::SetColors(aColors);
    }

    //! Set custom RGB colors.
    void SetColors(const Aspect_SequenceOfColor& theColors)
    {
        AIS_ColorScale::SetColors(theColors);
        myCustomColors.Clear();
        for (const Quantity_Color& aColIter : theColors) {
            myCustomColors.Append(Quantity_ColorRGBA(aColIter));
        }
    }

    //! Return display unit.
    const TCollection_AsciiString& DisplayUnits() const { return myDisplayUnits; }

    //! Set display unit.
    void SetDisplayUnits(const TCollection_AsciiString& theUnits)
    {
        if (myDisplayUnits != theUnits) {
            myDisplayUnits = theUnits;
            SetToUpdate();
        }
    }

    //! Return true if a special color is used for detected interval.
    bool ToHighlightIntervals() const { return myToHighlightIntervals; }

    //! Set to use a special color for detected interval.
    void SetToHighlightIntervals(bool theToHighlight) { myToHighlightIntervals = theToHighlight; }

    //! Return color used for detected interval.
    Quantity_Color IntervalHighlightColor() const { return myIntervalHighlightColor; }

    //! Set color used for detected interval.
    void SetIntervalHighlightColor(const Quantity_Color& theColor) { myIntervalHighlightColor = theColor; }

    //! Return last detected interval.
    int LastDetectedInterval() const { return myLastDetectedInterval; }

    //! Set last detected interval.
    void SetLastDetectedInterval(int theIndex) { myLastDetectedInterval = theIndex; }

    //! Sets true if the color scale has logarithmic intervals.
    Standard_EXPORT virtual void SetLogarithmic(const Standard_Boolean isLogarithmic) override;

    //! Returns true if the scale is using the "symmetric log" function.
    bool IsSymLog() const { return myIsLogSymmetrical; }

    //! Takes normalized mesh value (which is a value in range [0, 1] and calculated as
    //! (MeshValue - GetMin()) / (GetMax() - GetMin()) ) and maps it to color range value
    //! (which is a value in range [0, 1]).
    Standard_EXPORT double LogTransformMeshValueToRange(double theNormMeshValue) const;

    //! Transforms color range value to normalized mesh value (which can be restored to
    //! real mesh value as GetMin() + aNormMV * (GetMax() - GetMin()) ).
    Standard_EXPORT double LogTransformRangeToMeshValue(double theRangeValue) const;

    //! Symmetric log is a logarithm function alternative for data in ranges that span
    //! both negative and positive numbers.
    //! It approaches log(x) and -log(-x) at +∞ and -∞. Its graph goes through (0, 0).
    Standard_EXPORT static double SymLog(double theArg);

    //! Inverse function of the symmetric log.
    Standard_EXPORT static double InvSymLog(double theArg);

    //! Plain logarithm.
    static double Log(double theArg) { return std::log(theArg); }

    //! Exponentiates the argument.
    static double InvLog(double theArg) { return std::exp(theArg); }

    //! Log function with choice of the correct formula.
    double LogScale(double theArg) const { return myIsLogSymmetrical ? SymLog(theArg) : Log(theArg); }

    //! Inverse log function with choice of the correct formula.
    double InvLogScale(double theArg) const { return myIsLogSymmetrical ? InvSymLog(theArg) : InvLog(theArg); }

    //! Get the orientation axis in floating point form.
    //! (1.0, 0.0) when horizontal, (0.0, 1.0) when vertical.
    Graphic3d_Vec2d GetOrientationAxis() const
    {
        return IsHorizontal() ? Graphic3d_Vec2d(1.0, 0.0) : Graphic3d_Vec2d(0.0, 1.0);
    }

    //! Get the orientation axis in integer form.
    //! (1, 0) when horizontal, (0, 1) when vertical.
    Graphic3d_Vec2i GetOrientationAxisI() const
    {
        return IsHorizontal() ? Graphic3d_Vec2i(1, 0) : Graphic3d_Vec2i(0, 1);
    }

    //! Returns true if the colorscale is horizontal.
    bool IsHorizontal() const
    {
        if (myTransformPersistence.IsNull())
            return false;

        Aspect_TypeOfTriedronPosition aPos = myTransformPersistence->Corner2d();
        return (aPos == Aspect_TOTP_BOTTOM) || (aPos == Aspect_TOTP_TOP);
    }

    //! Sets the colorscale's orientation.
    void SetHorizontal(bool theIsHorizontal)
    {
        Aspect_TypeOfTriedronPosition aPos = theIsHorizontal ? Aspect_TOTP_BOTTOM : Aspect_TOTP_LEFT;
        SetPositionSide(aPos);
    }

    //! Get the side of the view to which the colorscale is attached to.
    //! Returns one of: Aspect_TOTP_LEFT, Aspect_TOTP_RIGHT
    //!                 Aspect_TOTP_TOP, Aspect_TOTP_BOTTOM.
    Aspect_TypeOfTriedronPosition GetPositionSide() const { return myTransformPersistence->Corner2d(); }

    //! Set the side of the view to which the colorscale is attached to.
    //! Accepts one of: Aspect_TOTP_LEFT, Aspect_TOTP_RIGHT
    //!                 Aspect_TOTP_TOP, Aspect_TOTP_BOTTOM.
    void SetPositionSide(const Aspect_TypeOfTriedronPosition thePos)
    {
        Aspect_TypeOfTriedronPosition aNewPos = IsValidCorner(thePos) ? thePos
                                                                      : InteractiveColorScale::FixPositionSide(thePos);
        myTransformPersistence->SetCorner2d(aNewPos);
        SetToUpdate();
    }

    //! Returns true if the argument is one of:
    //! Aspect_TOTP_LEFT, Aspect_TOTP_RIGHT,
    //! Aspect_TOTP_TOP or Aspect_TOTP_BOTTOM.
    static bool IsValidCorner(Aspect_TypeOfTriedronPosition theCorner)
    {
        switch (theCorner) {
            case Aspect_TOTP_LEFT:
            case Aspect_TOTP_RIGHT:
            case Aspect_TOTP_TOP:
            case Aspect_TOTP_BOTTOM: return true;
            default: return false;
        }
    }

    //! Corrects invalid corner value for compatibility with existing code that sets incorrect values.
    static Aspect_TypeOfTriedronPosition FixPositionSide(const Aspect_TypeOfTriedronPosition thePosSide)
    {
        Aspect_TypeOfTriedronPosition aNewSide = thePosSide;
        switch (aNewSide) {
            case Aspect_TOTP_LEFT_LOWER: aNewSide = Aspect_TOTP_LEFT; break;
            case Aspect_TOTP_RIGHT_LOWER: aNewSide = Aspect_TOTP_RIGHT; break;
            case Aspect_TOTP_LEFT_UPPER:
            case Aspect_TOTP_RIGHT_UPPER:
            case Aspect_TOTP_CENTER: aNewSide = Aspect_TOTP_LEFT; break;
            default: break;
        }
        return aNewSide;
    }

    //! Update window size used to offset bar from screen center.
    Standard_EXPORT void NotifyWindowSize(Graphic3d_Vec2i theWinSize) { myKnownWindowSize = theWinSize; }

public:
    //! Handle updates.
    Standard_EXPORT virtual AIS_RedrawProgressResult ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                                   const Handle(V3d_View)&               theView,
                                                                   const AIS_RedrawProgress theStage) override;

    //! Update presentation to new pixel scale ratio.
    Standard_EXPORT virtual bool OnPixelScaleChange(const Handle(AIS_InteractiveObject)& theObj,
                                                    float                                theRatio) override;

    //! Return name of this type.
    virtual const char* TypeName() const override { return "InteractiveColorScale"; }

public:
    //! Sets color settings.
    Standard_EXPORT virtual void SetColor(const Quantity_Color& theColor) override;

    //! Removes color settings.
    Standard_EXPORT virtual void UnsetColor() override;

    //! Return true if specified display mode is supported.
    virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override
    {
        return theMode == (int)ColorScaleDispMode::Full || theMode == (int)ColorScaleDispMode::Hidden
            || theMode == (int)ColorScaleDispMode::Frame;
    }

    //! Handle dragging.
    Standard_EXPORT virtual Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                             const Handle(V3d_View)&               theView,
                                                             const Handle(SelectMgr_EntityOwner)&  theOwner,
                                                             const Graphic3d_Vec2i&                theDragFrom,
                                                             const Graphic3d_Vec2i&                theDragTo,
                                                             const AIS_DragAction                  theAction) override;

    //! Set transform persistence.
    virtual void SetTransformPersistence(const Handle(Graphic3d_TransformPers)& theTrsfPers) override
    {
        AIS_ColorScale::SetTransformPersistence(theTrsfPers);
        updateSize(true);
        SetToUpdate();
    }

    //! Clears or invalidates dynamic highlight presentation.
    Standard_EXPORT virtual void ClearDynamicHighlight(const Handle(PrsMgr_PresentationManager)& theMgr) override;

protected:
    //! Compute presentation.
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Handle(Prs3d_Presentation)&         thePrs,
                                         const Standard_Integer                    theMode) override;

    //! Compute selection.
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                  const Standard_Integer             theMode) override;

protected:
    //! Handle size changes.
    Standard_EXPORT void updateSize(bool theToInvalidate);

    //! Return number of text rows (separated with '\n' character) in the color scale title.
    int titleRowsCount() const
    {
        const TCollection_ExtendedString& aTitle = GetTitle();

        int aRowsCount = 1;
        for (int aCharIter = 1; aCharIter <= aTitle.Length(); ++aCharIter) {
            if (aTitle.Value(aCharIter) == '\n') {
                ++aRowsCount;
            }
        }
        return aRowsCount;
    }

    //! Calculate correction variable based on own offset and on screen size.
    void updateInternalYOffset()
    {
        int                  anUp      = IsHorizontal() ? 0 : 1;
        static constexpr int halfSize2 = 2;
        myBarPosCorrection             = myViewOffset[anUp] - myKnownWindowSize[anUp] / halfSize2;
    }

    //! Compute color scale sizes.
    Standard_EXPORT void computeSizes(int&                             theBreadth,
                                      int&                             theHeight,
                                      Aspect_TypeOfColorScalePosition& theLabPos,
                                      int&                             theOffset) const;

    //! Update text aspects.
    Standard_EXPORT void updateTextAspectEx();

    //! Draw range tips.
    Standard_EXPORT void drawRangeTips(const Handle(Prs3d_Presentation)& thePrs);

    //! Draw the title of the colorscale.
    Standard_EXPORT void drawTitleEx(const Handle(Graphic3d_Group)& theGroup);

    //! Draw text labels.
    Standard_EXPORT void drawLabelsEx(const Handle(Graphic3d_Group)& theGroup);

    //! Collect text labels.
    Standard_EXPORT bool collectLabelsEx(TColStd_SequenceOfExtendedString& theLabels) const;

    //! Draw text labels.
    Standard_EXPORT void drawVertLabelsEx(const TColStd_SequenceOfExtendedString& theLabels,
                                          double                                  theStepY,
                                          int                                     theFilter,
                                          const Handle(Graphic3d_Group)&          theGroup);

    //! Draw single text label.
    Standard_EXPORT void drawTextEx(const Handle(Graphic3d_Group)&          theGroup,
                                    const TCollection_ExtendedString&       theText,
                                    const Graphic3d_Vec2i&                  theXY,
                                    const Graphic3d_HorizontalTextAlignment theHorizAlignment,
                                    const Graphic3d_VerticalTextAlignment   theVertAlignment);

    //! Draw color bar frame.
    Standard_EXPORT void drawFrameEx(const Handle(Prs3d_Presentation)& thePrs,
                                     const Graphic3d_Vec2i&            theXY,
                                     const Graphic3d_Vec2i&            theSize,
                                     const Quantity_Color&             theColor);

    //! Draw color bar out of range.
    Standard_EXPORT void drawColorBarOutOfRange(const Handle(Prs3d_Presentation)& thePrs);

    //! Draw color bar.
    Standard_EXPORT void drawColorBarEx(const Handle(Prs3d_Presentation)& thePrs);

    //! Collect color bar colors and return number of skipped intervals.
    Standard_EXPORT int collectColorBarColors(NCollection_Sequence<Quantity_ColorRGBA>& theColors, int& theNbIntervals);

    //! Draw smooth color bar with user colors.
    Standard_EXPORT void drawSmoothUserColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                  Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                  int                                             theNbIntervals,
                                                  double                                          theStepY);

    //! Draw smooth color bar with auto colors.
    Standard_EXPORT void drawSmoothAutoColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                  Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                  int                                             theNbIntervals);

    //! Draw discrete color bar.
    Standard_EXPORT void drawDiscreteColorBarEx(const NCollection_Sequence<Quantity_ColorRGBA>& theColors,
                                                Handle(Graphic3d_ArrayOfTriangles)&             theTriangles,
                                                double                                          theStepY);

    //! Updates the local transformation of the colorscale.
    Standard_EXPORT void updateOrientationTransform();

    //! Fixes corner in own transform persistence.
    void applyPositionSideFix()
    {
        Aspect_TypeOfTriedronPosition aPos = GetPositionSide();
        aPos                               = InteractiveColorScale::FixPositionSide(aPos);
        myTransformPersistence->SetCorner2d(aPos);
    }

    //! Return extra spacing between element
    int GetSpacing() const { return int(5.0f * myDevicePixelRatio); }

    using AIS_ColorScale::GetSize;
    using AIS_ColorScale::SetBreadth;
    using AIS_ColorScale::SetSize;

    //! Compute draggable color bar.
    Standard_EXPORT void computeSelDragBarUserColors(const Handle(SelectMgr_Selection)& theSel);

    //! Compute draggable color bar.
    Standard_EXPORT void computeSelDragBar(const Handle(SelectMgr_Selection)& theSel);

    //! Compute outer range transparency flippers.
    Standard_EXPORT void computeSelOutRangeFlips(const Handle(SelectMgr_Selection)& theSel);

    //! Compute draggable range tips.
    Standard_EXPORT void computeSelDragRangeTips(const Handle(SelectMgr_Selection)& theSel);

    //! Compute flippable range labels.
    Standard_EXPORT void computeSelFlipRange(const Handle(SelectMgr_Selection)& theSel);

    //! Log scale uses linear value transformation before mapping to colors, these values are cached in class variables.
    Standard_EXPORT void updateRangeToLogMappingCoeffs();

    //! Processes the switching of the colorscale orientation when it is dragged to a screen side
    //! where it belongs in a different orientation, i.e. dragging vertical colorscale to the
    //! bottom of view so it turns horizontal.
    Standard_EXPORT void processBarDragOrientationSwitch(const Handle(V3d_View)& theView,
                                                         const Graphic3d_Vec2i&  theDragTo,
                                                         const Graphic3d_Vec2i&  theWinSize,
                                                         const double            theResRatio);

    //! Set up internal variables on start of bar dragging.
    //! This is used on start of bar drag and on orientation switch triggered by dragging.
    Standard_EXPORT void setupBarDragging(const Handle(V3d_View)& theView, const Graphic3d_Vec2i& theDragFrom);

    //! Sets the scale's used function as symmetric log if the argument is true.
    //! Explicit setting of symmetric log from outside of the class is disallowed.
    void setSymLog(bool theIsSymLog) { myIsLogSymmetrical = theIsSymLog; }

protected:
    //! Picking owners priorities.
    enum class PickPriority {
        ColorBar       = 0,
        OuterRangeFlip = 1,
        RangeFlip      = 4,
        RangeTip       = 5,
    };

    //! Owner of dragging bar.
    class BarDragOwner : public ViewBase::InteractiveOwner {
    public:
        //! Main constructor.
        explicit BarDragOwner(const TCollection_AsciiString& theId);

        //! Destructor.
        virtual ~BarDragOwner() {}

        //! Return interval.
        int GetInterval() const { return myPart.SubId; }

        //! Set position and size.
        void SetPosSize(const Graphic3d_Vec2i& theXY, const Graphic3d_Vec2i& theSize)
        {
            myXY   = theXY;
            mySize = theSize;
        }

        //! Handle mouse button click event.
        virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                  Aspect_VKeyMouse       theButton,
                                                  Aspect_VKeyFlags       theModifiers,
                                                  bool                   theIsDoubleClick) override;

        //! Handle highlighting.
        virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                      const Handle(Prs3d_Drawer)&               theStyle,
                                      const Standard_Integer) override;

        //! Removes highlighting from the owner.
        virtual void Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                               const Standard_Integer                    theMode) override;

    private:
        Graphic3d_Vec2i                    myXY;
        Graphic3d_Vec2i                    mySize;
        Handle(Graphic3d_AspectFillArea3d) myAspect;
    };

    //! Owner for flipping transparency for outer range.
    class OutOfRangeOwner : public BarDragOwner {
    public:
        //! Main constructor.
        explicit OutOfRangeOwner(const TCollection_AsciiString& theId);

        //! Handle mouse button click event.
        virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                  Aspect_VKeyMouse       theButton,
                                                  Aspect_VKeyFlags       theModifiers,
                                                  bool                   theIsDoubleClick) override;
    };

    //! Owner of selected range label to flip range.
    class RangeFlipOwner : public ViewBase::InteractiveOwner {
    public:
        //! Main constructor.
        explicit RangeFlipOwner(const TCollection_AsciiString& theId);

        //! Destructor.
        virtual ~RangeFlipOwner() {}

        //! Handle mouse button click event.
        virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                  Aspect_VKeyMouse       theButton,
                                                  Aspect_VKeyFlags       theModifiers,
                                                  bool                   theIsDoubleClick) override;
    };

    //! Owner of selected range end.
    class RangeTipOwner : public ViewBase::InteractiveOwner {
    public:
        enum class RangeSide {
            MinSide = 1,
            MaxSide = 2,
        };

    public:
        //! Main constructor.
        RangeTipOwner(const TCollection_AsciiString& theId, RangeSide theSideId);

        //! Destructor.
        virtual ~RangeTipOwner() {}

        //! Return tip part.
        RangeSide GetTipSide() const { return myTipSide; }

        //! Get arrow size.
        float GetSize() const { return mySize; }

        //! Set arrow size.
        void SetSize(float theSize) { mySize = theSize; }

        //! Return attached point.
        const Graphic3d_Vec2i& GetPoint() const { return myPoint; }

        //! Return attached point.
        gp_Pnt GetPoint3d() const
        {
            return gp_Pnt(myPoint.x(),
                          myPoint.y() + int(myTipSide == RangeSide::MaxSide ? mySize * 0.25f : -mySize * 0.25f),
                          0.0);
        }

        //! Return triangle array.
        const Handle(Graphic3d_ArrayOfTriangles)& GetTriangles() const { return myTris; }

        //! Set attached point.
        void SetPoint(const Graphic3d_Vec2i& thePnt);

        //! Handle highlighting.
        virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                      const Handle(Prs3d_Drawer)&               theStyle,
                                      const Standard_Integer) override;

    private:
        Graphic3d_Vec2i myPoint;
        float           mySize = 10.0f;
        RangeSide       myTipSide;

        Handle(Graphic3d_AspectLine3d)     myAspect;
        Handle(Graphic3d_ArrayOfTriangles) myTris;
    };

    //! Return range lower tip.
    Graphic3d_Vec2i GetRangeTipMin() const
    {
        return Graphic3d_Vec2i(GetRangeTipX(), (int)GetRangeTipY(mySelRangeFrom) + myBarPosCorrection);
    }

    //! Return range upper tip.
    Graphic3d_Vec2i GetRangeTipMax() const
    {
        return Graphic3d_Vec2i(GetRangeTipX(), (int)GetRangeTipY(mySelRangeTo) + myBarPosCorrection);
    }

    //! Return color bar left X.
    int GetBarLeftX() const
    {
        int anXLeft = GetBarSideSpace();
        return (GetPositionSide() == Aspect_TOTP_RIGHT || GetPositionSide() == Aspect_TOTP_TOP)
                 ? -anXLeft - GetBreadth()
                 : anXLeft;
    }

    //! Return range tip X.
    int GetRangeTipX() const
    {
        int aMiddleX = GetBarLeftX() + GetBreadth() / 2;
        return aMiddleX;
    }

    //! Return range lower tip Y.
    double GetRangeTipY(double thePos) const
    {
        return double(myBarPos.barBottom) + thePos * double(myBarPos.barHeight);
    }

    //! Return text labels start X position.
    Standard_EXPORT int GetTextLabelX() const;

    //! Return space from side of screen.
    int GetBarSideSpace() const
    {
        int aResult = GetXPosition() + GetSpacing();
        if (IsHorizontal() && !GetTitle().IsEmpty()) {
            aResult += (GetSpacing() + GetTextHeight()) * titleRowsCount();
        }
        return aResult;
    }

    //! Return text label for interval.
    Standard_EXPORT TCollection_ExtendedString GetLabelEx(const int theIndex) const;

    //! Return value for interval.
    Standard_EXPORT double GetIntervalValueEx(const int theIndex) const;

    //! Handle range tips dragging.
    Standard_EXPORT virtual bool ProcessRangeDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                      const Handle(V3d_View)&               theView,
                                                      const Handle(RangeTipOwner)&          theOwner,
                                                      const Graphic3d_Vec2i&                theDragFrom,
                                                      const Graphic3d_Vec2i&                theDragTo,
                                                      const AIS_DragAction                  theAction);

    //! Handle color bar dragging.
    Standard_EXPORT virtual bool ProcessBarDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                    const Handle(V3d_View)&               theView,
                                                    const Handle(BarDragOwner)&           theOwner,
                                                    const Graphic3d_Vec2i&                theDragFrom,
                                                    const Graphic3d_Vec2i&                theDragTo,
                                                    const AIS_DragAction                  theAction);

    //! Update color bar dragging.
    Standard_EXPORT void UpdateBarDragging(const Handle(AIS_InteractiveContext)& theContext,
                                           const Handle(V3d_View)&               theView,
                                           const Graphic3d_Vec2i&                theDragTo);

    //! Update range dragging.
    Standard_EXPORT void UpdateRangeDragging(const Handle(AIS_InteractiveContext)& theContext,
                                             const Handle(V3d_View)&               theView,
                                             const Handle(RangeTipOwner)&          theOwner,
                                             const Graphic3d_Vec2i&                theDragTo);

protected:
    Handle(ViewCae::InteractiveColorScaleOwner) myMeshOwner;
    Handle(Graphic3d_AspectFillArea3d)           myOutOfRangeAsp;

    NCollection_Sequence<Quantity_ColorRGBA> myCustomColors;
    TCollection_AsciiString                  myDisplayUnits;

    float myBaseBreadth   = 20.0f;
    float myBaseHeight    = 1.0f;
    float myMaxBaseHeight = 1.0f;

    double myDefRangeMin       = 0.0;
    double myDefRangeMax       = 1.0;
    double mySelRangeFrom      = 0.0;
    double mySelRangeTo        = 1.0;
    float  myOutOfRangeAlpha   = 0.0f;
    bool   myToSkipEmptyLabels = false;

    Quantity_Color myIntervalHighlightColor;
    bool           myToHighlightIntervals = true;
    int            myLastDetectedInterval = -1;

    struct {
        int topY           = 0;
        int titleOffset    = 0;
        int barYOffset     = 0;
        int barBottom      = 0;
        int barTop         = 0;
        int barHeight      = 0;
        int barInnerBottom = 0;
        int barInnerTop    = 0;
        int barInnerHeight = 0;
    } myBarPos; // bar vertical position

    Graphic3d_Vec2i               myDragDiffStart;
    Graphic3d_Vec2i               myDragOffsetStart;
    Aspect_TypeOfTriedronPosition myDragCornerStart = Aspect_TOTP_CENTER;

    //! Determines whether log scale uses symlog function.
    //! Note that IsLogarithmic() needs to also return true for this.
    bool myIsLogSymmetrical = false;

    //! Coefficients of linear transformation used for log scale.
    //! They are specific for a range.
    double myLogTransformScale = 1.0;
    double myLogTransformBias  = 0.0;

    //! Correction to move bar along bar axis.
    //! Is needed because a non-corner transformation persistence does not apply all view offset components.
    int myBarPosCorrection = 0;

    //! Window size known to the colorscale, used for calculating correction.
    //! Unscaled pixels.
    Graphic3d_Vec2i myKnownWindowSize = Graphic3d_Vec2i(0, 0);

    //! The side for which local transformation is currently applied.
    //! Checked within Compute() to update the transform if the colorscale's orientation changed.
    Aspect_TypeOfTriedronPosition myTransformationPosSide = Aspect_TOTP_LEFT;
};

} // namespace

#endif // ViewerCAE_INTERACTIVECOLORSCALE_HEADERFILE