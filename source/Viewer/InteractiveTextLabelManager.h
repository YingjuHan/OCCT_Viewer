#ifndef _INTERACTIVETEXTLABELMANAGER_HEADERFILE
#define _INTERACTIVETEXTLABELMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {

//! Base text label parameters.
struct TextLabelParams {
    std::string text;
    std::string icon;
    Vec3        position;
    Vec3        attachPoint;
    int         attachMarker      = Aspect_TOM_EMPTY;
    int         attachMarkerScale = 1;
    float       pointPadding      = 0.0f;
    float       rectanglePadding  = 0.0f;
    bool        toDrawRectangle   = false;
};

//! Class provides public interface for managing Interactive text labels.
class InteractiveTextLabelManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveTextLabelManager(WasmViewer* theViewer);

    //! Create interactive text label presentation.
    Standard_EXPORT void CreateTextLabel(const ObjectId& theObjId);

    //! Return text label parameters.
    Standard_EXPORT TextLabelParams GetLabelParams(const ObjectId& theObjId) const;

    //! Set text label parameters.
    Standard_EXPORT void SetLabelParams(const ObjectId& theObjId, const TextLabelParams& theParams);
};

} // namespace

#endif // _INTERACTIVETEXTLABELMANAGER_HEADERFILE

