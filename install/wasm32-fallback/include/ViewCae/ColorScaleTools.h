#ifndef SCPRSCAE_COLORSCALETOOLS_HEADERFILE
#define SCPRSCAE_COLORSCALETOOLS_HEADERFILE

#include <ViewCae/InteractiveColorScale.h>

namespace ViewCae {

//! Class with helper methods for color scales.
class ColorScaleTools {
public:
    using ViewOffsets = NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i>;

    //! Update size and position of visible colorscale objects in according to the defined viewer offsets.
    Standard_EXPORT static void UpdateColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                                  const Handle(AIS_InteractiveObject)&  theUpdatedObj  = nullptr,
                                                  const ViewOffsets&                    theViewOffsets = ViewOffsets(),
                                                  bool                                  theIsDragged   = false);

protected:
    //! Hasher for Graphic3d_Vec3i.
    class Vec3iHasher {
    public:
        //! Hash value, for Map interface.
        static int HashCode(const Graphic3d_Vec3i& theElem, const int theUpper)
        {
            unsigned int aHashCode = 0;

            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.x(), theUpper);
            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.y(), theUpper);
            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.z(), theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! Matching two instances, for Map interface.
        static bool IsEqual(const Graphic3d_Vec3i& theElem1, const Graphic3d_Vec3i& theElem2)
        {
            return theElem1.x() == theElem2.x() && theElem1.y() == theElem2.y() && theElem1.z() == theElem2.z();
        }
    };

    using ListOfColorScales            = NCollection_List<Handle(InteractiveColorScale)>;
    using VectorOfColorScales          = std::vector<Handle(InteractiveColorScale)>;
    using MapOfOrientedColorScales     = NCollection_DataMap<Aspect_TypeOfTriedronPosition, ListOfColorScales>;
    using MapOfSortedColorScales       = NCollection_DataMap<Graphic3d_Vec3i, VectorOfColorScales, Vec3iHasher>;
    using MapOfSideToSortedColorScales = NCollection_DataMap<Aspect_TypeOfTriedronPosition, MapOfSortedColorScales>;

    //! Get colorscales sorted by view corners.
    Standard_EXPORT static void computeOrientedColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                                           const Handle(InteractiveColorScale)&  theUpdatedObj,
                                                           ViewOffsets&                          theViewOffsets,
                                                           MapOfOrientedColorScales&             theRes);

    //! Compute offset from the colorscales' side of screen.
    Standard_EXPORT static void computeSortedColorScaleSideOffset(
        const Handle(InteractiveColorScale)&      theUpdatedObj,
        const ViewOffsets&                        theViewOffsets,
        const MapOfOrientedColorScales::Iterator& theColorScales,
        const Graphic3d_Vec2i&                    theWinSize,
        const double                              theResRatio,
        const int                                 theCenterDeadZone,
        MapOfSortedColorScales&                   theRes);

    //! Update the argument offsets in a way that when they are used,
    //! vertical and horizontal colorscales do not intersect.
    Standard_EXPORT static void computeSortedColorScaleTopBottomOffset(
        ViewOffsets&                                  theOffsets,
        const MapOfSideToSortedColorScales::Iterator& theColorScales);

    //! Get colorscales sorted by view offsets.
    Standard_EXPORT static void computeSortedColorScales(const ViewOffsets&                      theViewOffsets,
                                                         const ViewOffsets&                      theColorScaleOffsets,
                                                         const Graphic3d_Vec2i&                  theWinSize,
                                                         MapOfSideToSortedColorScales::Iterator& theColorScales);

    //! Return offset for specified view position.
    Standard_EXPORT static Graphic3d_Vec2i getOffset(Aspect_TypeOfTriedronPosition thePos,
                                                     const ViewOffsets&            theViewOffsets);

    //! Return reference to offset for specified view position.
    Standard_EXPORT static Graphic3d_Vec2i& changeOffset(Aspect_TypeOfTriedronPosition thePos,
                                                         ViewOffsets&                  theViewOffsets);

    //! Sort colorscales according to its position.
    Standard_EXPORT static void sortColorScales(const Handle(InteractiveColorScale)&          theUpdatedObj,
                                                const Handle(AIS_InteractiveContext)&         theCtx,
                                                const MapOfSideToSortedColorScales::Iterator& theIter,
                                                const Graphic3d_Vec2i&                        theWinSize);

    //! Computes colorscales' side offset with account for moving colorscales
    //! away from the center so that others don't have invalid sizes.
    Standard_EXPORT static void applySortedColorScaleSideOffset(const Handle(InteractiveColorScale)& theUpdatedObj,
                                                                const Graphic3d_Vec2i&               theWinSize,
                                                                const double                         theResRatio,
                                                                const ViewOffsets&                   theViewOffsets,
                                                                const MapOfOrientedColorScales&      theColorscales,
                                                                MapOfSideToSortedColorScales&        theResult);

    //! Compute half-width of deadzone measured from screen centerline to which the colorscales cannot get dragged to.
    Standard_EXPORT static int computeCenterDragDeadzone(const bool                          theIsDeadzoneHorizontal,
                                                         const MapOfSideToSortedColorScales& theColorScales,
                                                         const double                        theResRatio);

    //! Update the width correction in the deadzone calculation process.
    Standard_EXPORT static void updateDragDeadZoneCorrection(MapOfSortedColorScales::Iterator& theColorscaleIter,
                                                             int&                              theCurrentMaxOffset,
                                                             const Graphic3d_Vec2i&            theProjAxis,
                                                             int& theResultWidthCorrection);
};

} // namespace

#endif // SCPRSCAE_COLORSCALETOOLS_HEADERFILE
