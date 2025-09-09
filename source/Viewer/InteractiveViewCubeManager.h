#ifndef _INTERACTIVEVIEWCUBEMANAGER_HEADERFILE
#define _INTERACTIVEVIEWCUBEMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing Interactive view cube.
class InteractiveViewCubeManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveViewCubeManager(WasmViewer* theViewer);

    //! Create interactive view cube presentation.
    Standard_EXPORT void CreateViewCube(const ObjectId& theObjId);

    //! Return view cube base size.
    Standard_EXPORT float GetViewCubeBaseSize(const ObjectId& theObjId) const;

    //! Set view cube base size.
    Standard_EXPORT void SetViewCubeBaseSize(const ObjectId& theObjId, const float theSize);

    //! Get view cube side label within V3d_TypeOfOrientation enumeration subset.
    Standard_EXPORT std::string GetBoxSideLabel(const ObjectId& theObjId, const int theSide) const;

    //! Set view cube side label.
    Standard_EXPORT void SetBoxSideLabel(const ObjectId& theObjId, const int theSide, const std::string& theLabel);
};

} // namespace

#endif // _INTERACTIVEVIEWCUBEMANAGER_HEADERFILE