#ifndef _INTERACTIVECOLORSCALEMANAGER_HEADERFILE
#define _INTERACTIVECOLORSCALEMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/ColorScaleParams.h"

#include <ViewCae/MeshColorScaleMapper.h>

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing Interactive color scale objects.
class InteractiveColorScaleManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveColorScaleManager(WasmViewer* theViewer);

    //! Create interactive color scale presentation.
    Standard_EXPORT void createColorScale(const ObjectId& theObjId);

    //! Attach color scale to mesh object for specified property.
    Standard_EXPORT void attachColorScale(const ObjectId&    theColorScaleId,
                                          const ObjectId&    theMeshId,
                                          const std::string& thePropertyId,
                                          const int          theComponentIndex);

    //! Return color scale parameters.
    Standard_EXPORT ColorScaleParams getColorScaleParams(const ObjectId& theObjId);

    //! Set color scale parameters and update its presentation.
    Standard_EXPORT void setColorScaleParams(const ObjectId& theObjId, const ColorScaleParams& theParams);

    //! Get ids of color scale objects attached to mesh.
    Standard_EXPORT std::vector<ObjectId> getMeshAttachedColorScales(const ObjectId& theMeshObjId);

    using ViewOffsets = NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i>;

    //! Update size and position of visible colorscale objects in according to the defined viewer offsets.
    Standard_EXPORT static void UpdateColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                                  const Handle(AIS_InteractiveObject)&  theUpdatedObj  = nullptr,
                                                  const ViewOffsets&                    theViewOffsets = ViewOffsets(),
                                                  bool                                  theIsDragged   = false);

    //! Return corresponding color mapper.
    Standard_EXPORT Handle(ViewCae::MeshColorScaleMapper) getRelatedColorMapper(
        const Handle(ViewCae::InteractiveColorScale)& theColorScaleObj) const;

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

    using ListOfColorScales        = NCollection_List<Handle(ViewCae::InteractiveColorScale)>;
    using VectorOfColorScales      = std::vector<Handle(ViewCae::InteractiveColorScale)>;
    using MapOfOrientedColorScales = NCollection_DataMap<Aspect_TypeOfTriedronPosition, ListOfColorScales>;
    using MapOfSortedColorScales   = NCollection_DataMap<Graphic3d_Vec3i, VectorOfColorScales, Vec3iHasher>;

    //! Get colorscales sorted by view corners.
    Standard_EXPORT static void computeOrientedColorScales(const Handle(AIS_InteractiveContext)&          theCtx,
                                                           const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
                                                           ViewOffsets&              theViewOffsets,
                                                           MapOfOrientedColorScales& theRes);

    //! Get colorscales sorted by view offsets.
    Standard_EXPORT static void computeSortedColorScales(const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
                                                         const ViewOffsets&                             theViewOffsets,
                                                         const MapOfOrientedColorScales::Iterator&      theColorScales,
                                                         MapOfSortedColorScales&                        theRes);

    //! Return offset for specified view position.
    Standard_EXPORT static Graphic3d_Vec2i getOffset(Aspect_TypeOfTriedronPosition thePos,
                                                     const ViewOffsets&            theViewOffsets);

    //! Sort colorscales according to its position.
    Standard_EXPORT static void sortColorScales(const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
                                                const Handle(AIS_InteractiveContext)&          theCtx,
                                                Aspect_TypeOfTriedronPosition                  thePos,
                                                const MapOfSortedColorScales&                  theSorted,
                                                const Graphic3d_Vec2i&                         theWinSize);
};

} // namespace

#endif // _INTERACTIVECOLORSCALEMANAGER_HEADERFILE