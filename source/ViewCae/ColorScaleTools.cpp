
#include <ViewCae/ColorScaleTools.h>

#include <ViewCae/InteractiveColorScaleOwner.h>

#include <V3d_View.hxx>

namespace ViewCae {

// ================================================================
// Function : UpdateColorScales
// ================================================================
void ColorScaleTools::UpdateColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                        const Handle(AIS_InteractiveObject)&  theUpdatedObj,
                                        const ViewOffsets&                    theViewOffsets,
                                        bool                                  theIsDragged)
{
    // Two separate sets of offsets are needed for proper logic
    // Offsets given by viewer
    NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i> aViewOffsets = theViewOffsets;
    // Offsets produced by colorscale interaction.
    NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i> aColorscaleOffsets;
    // Colorscale placement logic changes corner offsets, add them
    for (Aspect_TypeOfTriedronPosition aPos : {Aspect_TOTP_TOP,
                                               Aspect_TOTP_BOTTOM,
                                               Aspect_TOTP_LEFT,
                                               Aspect_TOTP_RIGHT,
                                               Aspect_TOTP_LEFT_LOWER,
                                               Aspect_TOTP_LEFT_UPPER,
                                               Aspect_TOTP_RIGHT_LOWER,
                                               Aspect_TOTP_RIGHT_UPPER}) {
        aColorscaleOffsets.Bound(aPos, Graphic3d_Vec2i(0));
    }

    Handle(InteractiveColorScale) anUpdateObj = Handle(InteractiveColorScale)::DownCast(theUpdatedObj);
    MapOfOrientedColorScales      anOrientedColorScales;
    computeOrientedColorScales(theCtx, anUpdateObj, aViewOffsets, anOrientedColorScales);

    if (anOrientedColorScales.IsEmpty()) {
        return;
    }

    Graphic3d_Vec2i aWinSize;
    double          aResRatio = 1.0;
    {
        MapOfOrientedColorScales::Iterator anIter(anOrientedColorScales);
        Handle(V3d_View) aView = ViewBase::IInteractiveObject::GetVisibleInAnyView(anIter.Value().First());
        aWinSize               = aView->Window()->Dimensions();
        aResRatio              = aView->RenderingParams().ResolutionRatio();
    }

    MapOfSideToSortedColorScales aSideToSortedColorscales;
    applySortedColorScaleSideOffset(anUpdateObj,
                                    aWinSize,
                                    aResRatio,
                                    aViewOffsets,
                                    anOrientedColorScales,
                                    aSideToSortedColorscales);

    for (MapOfSideToSortedColorScales::Iterator anIter(aSideToSortedColorscales); anIter.More(); anIter.Next()) {
        computeSortedColorScaleTopBottomOffset(aColorscaleOffsets, anIter);
    }

    for (MapOfSideToSortedColorScales::Iterator anIter(aSideToSortedColorscales); anIter.More(); anIter.Next()) {
        computeSortedColorScales(aViewOffsets, aColorscaleOffsets, aWinSize, anIter);
    }

    for (MapOfSideToSortedColorScales::Iterator anIter(aSideToSortedColorscales); anIter.More(); anIter.Next()) {
        sortColorScales(theIsDragged ? anUpdateObj : nullptr, theCtx, anIter, aWinSize);
    }
}

// ================================================================
// Function : applySortedColorScaleSideOffset
// ================================================================
void ColorScaleTools::applySortedColorScaleSideOffset(const Handle(InteractiveColorScale)& theUpdatedObj,
                                                      const Graphic3d_Vec2i&               theWinSize,
                                                      const double                         theResRatio,
                                                      const ViewOffsets&                   theViewOffsets,
                                                      const MapOfOrientedColorScales&      theColorscales,
                                                      MapOfSideToSortedColorScales&        theResult)
{
    // First pass side offset calculation
    // Colorscales are given their side offset, then depending on how many colorscales
    // got merged my side offset proximity, a deadzone will be enacted during second pass.
    for (MapOfOrientedColorScales::Iterator anIter(theColorscales); anIter.More(); anIter.Next()) {
        MapOfSortedColorScales* aSortedColorScales = theResult.ChangeSeek(anIter.Key());
        if (aSortedColorScales == nullptr) {
            aSortedColorScales = theResult.Bound(anIter.Key(), MapOfSortedColorScales());
        }
        computeSortedColorScaleSideOffset(theUpdatedObj,
                                          theViewOffsets,
                                          anIter,
                                          theWinSize,
                                          theResRatio,
                                          0,
                                          *aSortedColorScales);
    }

    // Compute deadzone

    // Decide which axis needs dead zones
    // True when dragging vertical colorscales horizontally could shrink horizontal colorscales to invalid sizes.
    // False when vice versa.
    bool isDeadzoneOnHorizontalDrag = theWinSize.x() > theWinSize.y();

    // Might be horizontal or vertical
    int aDragDeadZone = computeCenterDragDeadzone(isDeadzoneOnHorizontalDrag, theResult, theResRatio);

    // Second pass, a deadzone is passed depending on colorscale orientation
    for (MapOfOrientedColorScales::Iterator anIter(theColorscales); anIter.More(); anIter.Next()) {
        bool      isVertical    = ((anIter.Key() & Aspect_TOTP_LEFT) != 0) || ((anIter.Key() & Aspect_TOTP_RIGHT) != 0);
        const int aUsedDeadZone = (isVertical == isDeadzoneOnHorizontalDrag) ? aDragDeadZone : 0;

        MapOfSortedColorScales* aSortedColorScales = theResult.ChangeSeek(anIter.Key());
        aSortedColorScales->Clear();
        computeSortedColorScaleSideOffset(theUpdatedObj,
                                          theViewOffsets,
                                          anIter,
                                          theWinSize,
                                          theResRatio,
                                          aUsedDeadZone,
                                          *aSortedColorScales);
    }
}

// ================================================================
// Function : computeCenterDragDeadzone
// ================================================================
int ColorScaleTools::computeCenterDragDeadzone(const bool                          theIsDeadzoneHorizontal,
                                               const MapOfSideToSortedColorScales& theColorScales,
                                               const double                        theResRatio)
{
    // Number of colorscales that could potentially be "squeezed" between dragged colorscales
    size_t                                                    aMaxAlignedColorScales = 0;
    static constexpr int                                      aSideArraySize         = 2;
    std::array<Aspect_TypeOfTriedronPosition, aSideArraySize> aSandwichedSides{Aspect_TOTP_LEFT, Aspect_TOTP_RIGHT};
    std::array<Aspect_TypeOfTriedronPosition, aSideArraySize> anOuterSides{Aspect_TOTP_TOP, Aspect_TOTP_BOTTOM};
    Graphic3d_Vec2i                                           aSandwichedAxis = Graphic3d_Vec2i(0, 1);
    if (theIsDeadzoneHorizontal) {
        aSandwichedAxis = Graphic3d_Vec2i(1, 0);
        std::swap(aSandwichedSides, anOuterSides);
    }

    for (Aspect_TypeOfTriedronPosition aSide : aSandwichedSides) {
        const MapOfSortedColorScales* aSortedMap = theColorScales.Seek(aSide);
        if (aSortedMap == nullptr)
            continue;

        for (MapOfSortedColorScales::Iterator anIter(*aSortedMap); anIter.More(); anIter.Next()) {
            const size_t aNewSize = anIter.Value().size();
            if (aNewSize > aMaxAlignedColorScales)
                aMaxAlignedColorScales = aNewSize;
        }
    }

    // Width of furthermost colorscale on outer side
    Standard_Integer aColorscaleWidthCorrection = 0;
    for (Aspect_TypeOfTriedronPosition aSide : anOuterSides) {
        int                           aMaxSideOffset = 0;
        const MapOfSortedColorScales* aSortedMap     = theColorScales.Seek(aSide);
        if (aSortedMap == nullptr)
            continue;

        for (MapOfSortedColorScales::Iterator anIter(*aSortedMap); anIter.More(); anIter.Next()) {
            updateDragDeadZoneCorrection(anIter, aMaxSideOffset, aSandwichedAxis, aColorscaleWidthCorrection);
        }
    }

    int aDragDeadZone = 0;
    // Deadzone to ensure 90 px minimum per colorscale present
    static constexpr int aColorscaleSpaceAllocationPx = 90 / 2;
    if (aMaxAlignedColorScales != 0)
        aDragDeadZone = (int)((double)(aMaxAlignedColorScales * aColorscaleSpaceAllocationPx
                                       + aColorscaleWidthCorrection)
                              * theResRatio);
    return aDragDeadZone;
}

// ================================================================
// Function : updateDragDeadZoneCorrection
// ================================================================
void ColorScaleTools::updateDragDeadZoneCorrection(MapOfSortedColorScales::Iterator& theColorscaleIter,
                                                   int&                              theCurrentMaxOffset,
                                                   const Graphic3d_Vec2i&            theProjAxis,
                                                   int&                              theResultWidthCorrection)
{
    int aSideOffset = theColorscaleIter.Key().z();
    if (aSideOffset > theCurrentMaxOffset) {
        theCurrentMaxOffset = aSideOffset;
        for (const Handle(InteractiveColorScale)& aScale : theColorscaleIter.Value()) {
            Bnd_Box anObjBndBox;
            aScale->BoundingBox(anObjBndBox);
            const gp_XYZ          aRange = anObjBndBox.CornerMax().XYZ() - anObjBndBox.CornerMin().XYZ();
            const Graphic3d_Vec2i anObjScreenSize((int)aRange.X(), (int)aRange.Y());
            int                   aSize = anObjScreenSize.Dot(theProjAxis);
            if (aSize > theResultWidthCorrection) {
                theResultWidthCorrection = aSize;
            }
        }
    }
}

// ================================================================
// Function : computeOrientedColorScales
// ================================================================
void ColorScaleTools::computeOrientedColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                                 const Handle(InteractiveColorScale)&  theUpdatedObj,
                                                 ViewOffsets&                          theViewOffsets,
                                                 MapOfOrientedColorScales&             theRes)
{
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = theCtx->ObjectIterator(); anObjIter.More(); anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)&   anObj = anObjIter.Key();
        const Handle(Graphic3d_TransformPers)& aPers = anObj->TransformPersistence();
        if (aPers.IsNull() || !aPers->IsTrihedronOr2d() || anObj->DisplayStatus() != PrsMgr_DisplayStatus_Displayed
            || ViewBase::IInteractiveObject::GetVisibleInAnyView(anObj).IsNull()) {
            continue;
        }

        Aspect_TypeOfTriedronPosition aPos = aPers->Corner2d();
        if (Handle(InteractiveColorScale) aColorScale = Handle(InteractiveColorScale)::DownCast(anObj)) {
            if (aColorScale->DisplayMode() == 1) {
                continue;
            }

            aPos = aColorScale->GetPositionSide();

            // Handle invalid cases
            if (!InteractiveColorScale::IsValidCorner(aPos)) {
                aColorScale->SetHorizontal(false);
                aPos = aColorScale->GetPositionSide();
            }

            ListOfColorScales* aColorScales = theRes.ChangeSeek(aPos);
            if (aColorScales == nullptr) {
                aColorScales = theRes.Bound(aPos, ListOfColorScales());
            }
            if (aColorScale == theUpdatedObj) {
                aColorScales->Append(aColorScale);
            } else {
                aColorScales->Prepend(aColorScale);
            }
        } else if (ViewBase::IInteractiveObject* anIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get())) {
            Graphic3d_Vec2i anObjOffset = anIObj->Get2dObjectSize();
            if (anObjOffset == Graphic3d_Vec2i(0)) {
                continue;
            }

            Graphic3d_Vec2i* anOffset = theViewOffsets.ChangeSeek(aPos);
            if (anOffset == nullptr) {
                anOffset = theViewOffsets.Bound(aPos, Graphic3d_Vec2i(0));
            }
            anObjOffset = anObjOffset.cwiseMax(*anOffset);
            anOffset->SetValues(anObjOffset.x(), anObjOffset.y());
        }
    }
}

// ================================================================
// Function : computeSortedColorScaleSideOffset
// ================================================================
void ColorScaleTools::computeSortedColorScaleSideOffset(const Handle(InteractiveColorScale)&      theUpdatedObj,
                                                        const ViewOffsets&                        theOffsets,
                                                        const MapOfOrientedColorScales::Iterator& theColorScales,
                                                        const Graphic3d_Vec2i&                    theWinSize,
                                                        const double                              theResRatio,
                                                        const int                                 theCenterDeadZone,
                                                        MapOfSortedColorScales&                   theRes)
{
    Aspect_TypeOfTriedronPosition aPos           = theColorScales.Key();
    int                           aSide          = aPos == Aspect_TOTP_LEFT || aPos == Aspect_TOTP_RIGHT ? 0 : 1;
    int                           aMinSideOffset = getOffset(aPos, theOffsets)[aSide];

    auto updateOffset = [&theRes](const Handle(InteractiveColorScale)& theObj, int& theOffset) {
        int aMin = theOffset + theObj->GetXPosition();
        int aMax = aMin + theObj->GetBreadth();
        for (MapOfSortedColorScales::Iterator aSortedIter(theRes); aSortedIter.More(); aSortedIter.Next()) {
            if (aMin < aSortedIter.Key().z() + aMax - aMin && aMax > aSortedIter.Key().z()) {
                theOffset = aSortedIter.Key().z();
                break;
            }
        }
    };

    const int aDeadzoneBoundary = (int)((theWinSize[aSide] / 2 - theCenterDeadZone) / theResRatio);

    const ListOfColorScales& aColorScales = theColorScales.Value();
    for (const Handle(InteractiveColorScale)& anObj : aColorScales) {
        Graphic3d_Vec3i anOffsets(0);
        anOffsets.z() = Max(anObj->Get2dViewOffset()[aSide], aMinSideOffset);
        if (anOffsets.z() > aDeadzoneBoundary) {
            anOffsets.z() = Max(aDeadzoneBoundary, aMinSideOffset);
        }
        if (anObj == theUpdatedObj) {
            updateOffset(anObj, anOffsets.z());
        }
        VectorOfColorScales* aRangedColorScales = theRes.ChangeSeek(anOffsets);
        if (aRangedColorScales == nullptr) {
            aRangedColorScales = theRes.Bound(anOffsets, VectorOfColorScales());
        }
        aRangedColorScales->push_back(anObj);
    }
}

// ================================================================
// Function : computeSortedColorScaleTopBottomOffset
// ================================================================
void ColorScaleTools::computeSortedColorScaleTopBottomOffset(
    ViewOffsets&                                  theOffsets,
    const MapOfSideToSortedColorScales::Iterator& theColorScales)
{
    Aspect_TypeOfTriedronPosition aPos = theColorScales.Key();
    for (MapOfSortedColorScales::Iterator anIter(theColorScales.Value()); anIter.More(); anIter.Next()) {
        int aSide = aPos == Aspect_TOTP_LEFT || aPos == Aspect_TOTP_RIGHT ? 0 : 1;

        Aspect_TypeOfTriedronPosition aTopAsp = aSide == 0 ? Aspect_TOTP_TOP : Aspect_TOTP_RIGHT;
        Aspect_TypeOfTriedronPosition aBotAsp = aSide == 0 ? Aspect_TOTP_BOTTOM : Aspect_TOTP_LEFT;

        Graphic3d_Vec2i& aCornerTopOffset = changeOffset((Aspect_TypeOfTriedronPosition)(aPos | aTopAsp), theOffsets);
        Graphic3d_Vec2i& aCornerBotOffset = changeOffset((Aspect_TypeOfTriedronPosition)(aPos | aBotAsp), theOffsets);

        const VectorOfColorScales& aColorScales = anIter.Value();
        for (const Handle(InteractiveColorScale)& anObj : aColorScales) {
            Graphic3d_Vec3i anOffsets = anIter.Key();
            // Update top and bottom offsets for interaction with other colorscales.
            Bnd_Box anObjBndBox;
            anObj->BoundingBox(anObjBndBox);
            const gp_XYZ          aRange = anObjBndBox.CornerMax().XYZ() - anObjBndBox.CornerMin().XYZ();
            const Graphic3d_Vec2i anObjScreenSize((int)aRange.X(), (int)aRange.Y());
            Standard_Integer      anOffsetSpaceReserve           = anObjScreenSize[aSide];
            const int             aCornerOffsetWithReservedSpace = anOffsets.z() + anOffsetSpaceReserve;
            if (aCornerOffsetWithReservedSpace > aCornerTopOffset[aSide]) {
                aCornerTopOffset[aSide] = aCornerOffsetWithReservedSpace;
            }
            if (aCornerOffsetWithReservedSpace > aCornerBotOffset[aSide]) {
                aCornerBotOffset[aSide] = aCornerOffsetWithReservedSpace;
            }
        }
    }
}

// ================================================================
// Function : computeSortedColorScales
// ================================================================
void ColorScaleTools::computeSortedColorScales(const ViewOffsets&                      theViewOffsets,
                                               const ViewOffsets&                      theColorscaleOffsets,
                                               const Graphic3d_Vec2i&                  theWinSize,
                                               MapOfSideToSortedColorScales::Iterator& theColorScales)
{
    Aspect_TypeOfTriedronPosition aPos    = theColorScales.Key();
    MapOfSortedColorScales&       aResult = theColorScales.ChangeValue();

    int aSide = aPos == Aspect_TOTP_LEFT || aPos == Aspect_TOTP_RIGHT ? 0 : 1;
    int anUp  = (aSide + 1) % 2;

    Aspect_TypeOfTriedronPosition aTopAsp = aSide == 0 ? Aspect_TOTP_TOP : Aspect_TOTP_RIGHT;
    Aspect_TypeOfTriedronPosition aBotAsp = aSide == 0 ? Aspect_TOTP_BOTTOM : Aspect_TOTP_LEFT;

    int       aMinTopOffset                  = getOffset(aTopAsp, theViewOffsets)[anUp];
    int       aMinBotOffset                  = getOffset(aBotAsp, theViewOffsets)[anUp];
    bool      isPreferUsingVerticalSpace     = theWinSize.x() > theWinSize.y();
    bool      doColorScaleInsetting          = ((anUp == 1) == isPreferUsingVerticalSpace);
    const int aTopOffsetFromOtherColorscales = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aTopAsp),
                                                         theColorscaleOffsets)[anUp];
    const int aBotOffsetFromOtherColorscales = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aBotAsp),
                                                         theColorscaleOffsets)[anUp];
    if (!doColorScaleInsetting && aMinTopOffset < aTopOffsetFromOtherColorscales) {
        aMinTopOffset = aTopOffsetFromOtherColorscales;
    }
    if (!doColorScaleInsetting && aMinBotOffset < aBotOffsetFromOtherColorscales) {
        aMinBotOffset = aBotOffsetFromOtherColorscales;
    }
    Graphic3d_Vec2i aCornerTopOffset = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aTopAsp), theViewOffsets);
    Graphic3d_Vec2i aCornerBotOffset = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aBotAsp), theViewOffsets);

    MapOfSortedColorScales aNewMap;
    for (MapOfSortedColorScales::Iterator anIter(aResult); anIter.More(); anIter.Next()) {
        VectorOfColorScales aVec      = anIter.Value();
        Graphic3d_Vec3i     anOffsets = anIter.Key();
        // Z offset is already calculated
        anOffsets.x() = Max(anOffsets.z() < aCornerTopOffset[aSide] ? aCornerTopOffset[anUp] : aMinTopOffset,
                            aMinTopOffset);
        anOffsets.y() = Max(anOffsets.z() < aCornerBotOffset[aSide] ? aCornerBotOffset[anUp] : aMinBotOffset,
                            aMinBotOffset);

        aNewMap.Bound(anOffsets, aVec);
    }

    aResult = aNewMap;
}

// ================================================================
// Function : getOffset
// ================================================================
Graphic3d_Vec2i ColorScaleTools::getOffset(Aspect_TypeOfTriedronPosition thePos, const ViewOffsets& theViewOffsets)
{
    Graphic3d_Vec2i aRes(0);
    theViewOffsets.Find(thePos, aRes);
    return aRes;
}

// ================================================================
// Function : changeOffset
// ================================================================
Graphic3d_Vec2i& ColorScaleTools::changeOffset(Aspect_TypeOfTriedronPosition thePos, ViewOffsets& theViewOffsets)
{
    Graphic3d_Vec2i* aRes = theViewOffsets.ChangeSeek(thePos);
    return *aRes;
}

// ================================================================
// Function : sortColorScales
// ================================================================
void ColorScaleTools::sortColorScales(const Handle(InteractiveColorScale)&          theUpdatedObj,
                                      const Handle(AIS_InteractiveContext)&         theCtx,
                                      const MapOfSideToSortedColorScales::Iterator& theIter,
                                      const Graphic3d_Vec2i&                        theWinSize)
{
    const Aspect_TypeOfTriedronPosition aPos          = theIter.Key();
    const MapOfSortedColorScales&       aSortedScales = theIter.Value();
    const int                           aSide         = aPos == Aspect_TOTP_LEFT || aPos == Aspect_TOTP_RIGHT ? 0 : 1;
    const int                           anUp          = (aSide + 1) % 2;
    for (MapOfSortedColorScales::Iterator aSortedIter(aSortedScales); aSortedIter.More(); aSortedIter.Next()) {
        VectorOfColorScales& aCSVector = aSortedIter.ChangeValue();

        auto sortByIds = [](const Handle(InteractiveColorScale)& theObj1,
                            const Handle(InteractiveColorScale)& theObj2) -> bool {
            return theObj1->MeshOwner()->Id() < theObj2->MeshOwner()->Id();
        };
        std::sort(aCSVector.begin(), aCSVector.end(), sortByIds);
        int i = 0;
        for (const Handle(InteractiveColorScale)& anObj : aCSVector) {
            double aSize = (double(theWinSize[anUp]) / anObj->DevicePixelRatio() - aSortedIter.Key().x()
                            - aSortedIter.Key().y())
                         / aCSVector.size();

            Graphic3d_Vec2d anOffset(anObj->Get2dViewOffset());
            anOffset[aSide] = aSortedIter.Key().z();
            anOffset[anUp]  = aSortedIter.Key().y() + i * aSize;
            anObj->Set2dViewOffset(Graphic3d_Vec2i(anOffset));
            anObj->SetBaseHeight((float)aSize);
            Graphic3d_Vec2i aWinSizeUnscaled(Graphic3d_Vec2d(theWinSize) / anObj->DevicePixelRatio());
            anObj->NotifyWindowSize(aWinSizeUnscaled);

            anObj->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(anOffset));
            theCtx->RecomputePrsOnly(anObj, false, true);
            if (anObj != theUpdatedObj) {
                theCtx->RecomputeSelectionOnly(anObj);
            }
            ++i;
        }
    }
}

} // namespace