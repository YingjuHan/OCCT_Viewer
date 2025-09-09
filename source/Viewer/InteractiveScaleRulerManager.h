#ifndef _INTERACTIVESCALERULERMANAGER_HEADERFILE
#define _INTERACTIVESCALERULERMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing Interactive scale ruler.
class InteractiveScaleRulerManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveScaleRulerManager(WasmViewer* theViewer);

    //! Create interactive view cube presentation.
    Standard_EXPORT void CreateScaleRuler(const ObjectId& theObjId);

    //! Return scale ruler base size.
    Standard_EXPORT Vec2 GetBaseSize(const ObjectId& theObjId) const;

    //! Set scale ruler base size.
    Standard_EXPORT void SetBaseSize(const ObjectId& theObjId, const Vec2& theSize);
};

} // namespace

#endif // _INTERACTIVESCALERULERMANAGER_HEADERFILE

