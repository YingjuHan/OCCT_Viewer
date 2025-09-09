

#ifndef SCWEBVIEWER_INTERACTIVEAXESMANAGER_HEADERFILE
#define SCWEBVIEWER_INTERACTIVEAXESMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

namespace Viewer {
//! Class provides public interface for managing Interactive spherical axes.
class InteractiveAxesManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveAxesManager(WasmViewer* theViewer);

    //! Creates a spherical axes object with specified ID.
    Standard_EXPORT void CreateSphericalAxes(const ObjectId& theObjId);

    //! Returns position of the center of the coordinate system.
    Standard_EXPORT Vec3 GetAxesPosition(const ObjectId& theObjId);

    //! Sets the position of the center of the coordinate system.
    Standard_EXPORT void SetAxesPosition(const ObjectId& theObjId, const Vec3& thePos);

    //! Gets the axes scale.
    Standard_EXPORT double GetAxesScale(const ObjectId& theObjId);

    //! Sets the axes scale.
    Standard_EXPORT void SetAxesScale(const ObjectId& theObjId, double theScale);

    //! Return the angular arrow direction.
    Standard_EXPORT std::string GetAngularArrowDir(const ObjectId& theObjId, const std::string& theAxisId);

    //! Set the angular arrow direction.
    Standard_EXPORT void SetAngularArrowDir(const ObjectId&    theObjId,
                                            const std::string& theAxisId,
                                            const std::string& theDir);

    //! Return the attachment axis (zero axis) of the angular arrow.
    Standard_EXPORT std::string GetAngularArrowAxis(const ObjectId& theObjId, const std::string& theAxisId);

    //! Set the attachment axis (zero axis) of the angular arrow.
    Standard_EXPORT void SetAngularArrowAxis(const ObjectId&    theObjId,
                                             const std::string& theAxisId,
                                             const std::string& theZeroAxis);

    //! Return the angular arrow text label.
    Standard_EXPORT std::string GetAngularArrowLabel(const ObjectId& theObjId, const std::string& theAxisId);

    //! Set the angular arrow text label.
    Standard_EXPORT void SetAngularArrowLabel(const ObjectId&    theObjId,
                                              const std::string& theAxisId,
                                              const std::string& theLabel);

    //! Return the orientation axis of the longitude/latitude circles.
    Standard_EXPORT std::string GetOrientation(const ObjectId& theObjId);

    //! Set the orientation axis of the longitude/latitude circles.
    Standard_EXPORT void SetOrientation(const ObjectId& theObjId, const std::string& theAxis);
};

//! TypeDoc/Markdown documentation link between SCPrsBase::InteractiveSphericalAxes and
//! and its managing class for TypeScript.
using InteractiveSphericalAxes = Viewer::InteractiveAxesManager;

} // namespace

#endif // SCWEBVIEWER_INTERACTIVEAXESMANAGER_HEADERFILE
