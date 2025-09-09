#ifndef _INTERACTIVECLIPPINGMANAGER_HEADERFILE
#define _INTERACTIVECLIPPINGMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing Interactive clipping box.
class InteractiveClippingManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveClippingManager(WasmViewer* theViewer);

    //! Create interactive object.
    Standard_EXPORT void CreateClipping(const ObjectId& theObjId, const int theViewId);

    //! Return clipping box configuration - corner point.
    Standard_EXPORT Vec3 GetClippingCorner(const ObjectId& theObjId, bool theIsRel) const;

    //! Set clipping box configuration - corner point.
    Standard_EXPORT void SetClippingCorner(const ObjectId& theObjId, const Vec3& thePnt, bool theIsRel);

    //! Return clipping box configuration - extension.
    Standard_EXPORT Vec3 GetClippingExtent(const ObjectId& theObjId, bool theIsRel) const;

    //! Set clipping box configuration - extension.
    Standard_EXPORT void SetClippingExtent(const ObjectId& theObjId, const Vec3& theExt, bool theIsRel);

    //! Return clipping box configuration.
    Standard_EXPORT BndBox3 GetClippingBox(const ObjectId& theObjId, bool theIsRel) const;

    //! Set clipping box configuration.
    Standard_EXPORT void SetClippingBox(const ObjectId& theObjId, const BndBox3& theBox, bool theIsRel);

    //! Return clipping flag.
    Standard_EXPORT bool IsClippingOn(const ObjectId& theObjId) const;

    //! Set clipping flag.
    Standard_EXPORT void SetClippingOn(const ObjectId& theObjId, const bool theToClip);

    //! Return clipping box flipping flag.
    Standard_EXPORT bool ToFlipClipping(const ObjectId& theObjId) const;

    //! Set clipping box flipping flag.
    Standard_EXPORT void SetFlipClipping(const ObjectId& theObjId, const bool theToFlip);

    //! Return capping flag.
    Standard_EXPORT bool IsCappingOn(const ObjectId& theObjId) const;

    //! Set capping flag.
    Standard_EXPORT void SetCappingOn(const ObjectId& theObjId, const bool theToUse);

    //! Return TRUE for using object's color for capping.
    Standard_EXPORT bool ToUseObjectMaterial(const ObjectId& theObjId) const;

    //! Set flag for using object's color for capping.
    Standard_EXPORT void SetUseObjectMaterial(const ObjectId& theObjId, const bool theToUse);

    //! Return shaded presentation flag.
    Standard_EXPORT bool IsShaded(const ObjectId& theObjId) const;

    //! Set shaded presentation flag.
    Standard_EXPORT void SetShaded(const ObjectId& theObjId, const bool theToShade);

    //! Return clipping box configuration - number of clipping planes per dimension (0, -1, 1, or 2),
    //! where -1 means clipping in opposite direction.
    Standard_EXPORT Vec3 GetNbPlanesXYZ(const ObjectId& theObjId) const;

    //! Set clipping box configuration - number of clipping planes per dimension.
    Standard_EXPORT void SetNbPlanesXYZ(const ObjectId& theObjId, const Vec3& theVec);
};

} // namespace

#endif // _INTERACTIVECLIPPINGMANAGER_HEADERFILE