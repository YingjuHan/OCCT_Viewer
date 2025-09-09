#ifndef _INTERACTIVEDIMENSIONMANAGER_HEADERFILE
#define _INTERACTIVEDIMENSIONMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/DimensionParams.h"
#include "Utils/ObjectId.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {

//! Class provides public interface for managing Interactive dimension labels.
class InteractiveDimensionManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveDimensionManager(WasmViewer* theViewer);

    //! Create empty interactive length dimension presentation.
    Standard_EXPORT void CreateLengthDimension(const ObjectId& theObjId);

    //! Create empty interactive radius dimension presentation.
    Standard_EXPORT void CreateRadiusDimension(const ObjectId& theObjId);

    //! Create empty interactive angle dimension presentation.
    Standard_EXPORT void CreateAngleDimension(const ObjectId& theObjId);

    //! Create interactive length dimension presentation between two points.
    Standard_EXPORT bool BuildLengthDimension(const ObjectId& theObjId,
                                              const Vec3&     thePnt1,
                                              const Vec3&     thePnt2,
                                              const Ax2&      thePln);

    //! Create interactive radius dimension presentation.
    Standard_EXPORT bool BuildRadiusDimension(const ObjectId& theObjId,
                                              const Ax2&      theAx2,
                                              const double    theRadius,
                                              const Vec3&     theAnchor);

    //! Create interactive angle dimension presentation from 3 points.
    Standard_EXPORT bool BuildAngleDimension(const ObjectId& theObjId,
                                             const Vec3&     thePnt1,
                                             const Vec3&     thePnt2,
                                             const Vec3&     thePnt3);

    //! Return interactive dimension parameters.
    Standard_EXPORT DimensionParams GetDimensionParams(const ObjectId& theObjId) const;

    //! Set interactive dimension parameters.
    Standard_EXPORT void SetDimensionParams(const ObjectId& theObjId, const DimensionParams& theParams);

    //! Return interactive dimension text position.
    Standard_EXPORT Vec3 GetDimensionTextPosition(const ObjectId& theObjId) const;

    //! Set interactive dimension text position.
    Standard_EXPORT void SetDimensionTextPosition(const ObjectId& theObjId, const Vec3& thePnt);
};

} // namespace

#endif // _INTERACTIVEDIMENSIONMANAGER_HEADERFILE