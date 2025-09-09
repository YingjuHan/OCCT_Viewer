#ifndef _INTERACTIVEASPECTSMANAGER_HEADERFILE
#define _INTERACTIVEASPECTSMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ColorTools.h"
#include "Utils/ObjectId.h"

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing general presentation aspects of Interactive Objects.
class InteractiveAspectsManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveAspectsManager(WasmViewer* theViewer);

    //! Return object's ZLayer within Graphic3d_ZLayerId enumeration.
    Standard_EXPORT int GetZLayer(const ObjectId& theObjId) const;

    //! Set object's ZLayer within Graphic3d_ZLayerId enumeration.
    Standard_EXPORT void SetZLayer(const ObjectId& theObjId, const int theLayer);

    //! Return object's transparency within [0.0, 1.0] range, or 0.0 if unspecified.
    Standard_EXPORT float GetTransparency(const ObjectId& theObjId) const;

    //! Set object's transparency.
    Standard_EXPORT void SetTransparency(const ObjectId& theObjId, const float theTransp);

    //! Return object's color or empty string if unspecified.
    Standard_EXPORT ColorTools::ColorString GetBaseColor(const ObjectId& theObjId) const;

    //! Set object's color or empty string if unspecified.
    Standard_EXPORT void SetBaseColor(const ObjectId& theObjId, const ColorTools::ColorString& theHexColor);

    //! Return object's line width.
    Standard_EXPORT float GetLineWidth(const ObjectId& theObjId) const;

    //! Set object's line width.
    Standard_EXPORT void SetLineWidth(const ObjectId& theObjId, const float theWidth);

    //! Return object's line type.
    Standard_EXPORT int GetLineType(const ObjectId& theObjId) const;

    //! Set object's line type.
    Standard_EXPORT void SetLineType(const ObjectId& theObjId, const int theType);

    //! Return text height.
    Standard_EXPORT double GetTextHeight(const ObjectId& theObjId) const;

    //! Set text height.
    Standard_EXPORT void SetTextHeight(const ObjectId& theObjId, const double theHeight);

    //! Return text label font.
    Standard_EXPORT std::string GetTextFont(const ObjectId& theObjId) const;

    //! Set text label font.
    Standard_EXPORT void SetTextFont(const ObjectId& theObjId, const std::string& theFont);

    //! Return text font aspect (Font_FontAspect).
    Standard_EXPORT int GetTextFontAspect(const ObjectId& theObjId) const;

    //! Set text font aspect (Font_FontAspect).
    Standard_EXPORT void SetTextFontAspect(const ObjectId& theObjId, const int theAspect);

    //! Return text label justification (Viewer::ViewCorner).
    Standard_EXPORT int GetTextJustification(const ObjectId& theObjId) const;

    //! Set text label justification (Viewer::ViewCorner).
    Standard_EXPORT void SetTextJustification(const ObjectId& theObjId, const int theCorner);
};

} // namespace

#endif // _INTERACTIVEASPECTSMANAGER_HEADERFILE