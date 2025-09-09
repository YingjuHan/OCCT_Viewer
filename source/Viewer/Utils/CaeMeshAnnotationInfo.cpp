#include "CaeMeshAnnotationInfo.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InitFromAnnotation
// ================================================================
void CaeMeshAnnotationInfo::InitFromAnnotation(const ViewCae::MeshAnnotation& theAnnotation)
{
    isVisible   = theAnnotation.IsVisible();
    toShowText  = theAnnotation.IsText();
    toShowIcons = theAnnotation.IsIcon();
    text.clear();
    icons.clear();
    for (const ViewCae::MeshAnnotationData& aData : theAnnotation.Data()) {
        text.push_back(aData.Text.ToCString());
        icons.push_back(aData.Icon.ToCString());
    }
    pos.SetXYZ(theAnnotation.Pos().XYZ());
}

// ================================================================
// Function : UpdateAnnotation
// ================================================================
void CaeMeshAnnotationInfo::UpdateAnnotation(ViewCae::MeshAnnotation& theAnnotation) const
{
    theAnnotation.SetVisible(isVisible);
    unsigned int aType = 0;
    if (toShowText) {
        aType |= (unsigned int)ViewCae::MeshAnnotationType::Text;
    }
    if (toShowIcons) {
        aType |= (unsigned int)ViewCae::MeshAnnotationType::Icon;
    }
    theAnnotation.SetType(aType);

    std::vector<ViewCae::MeshAnnotationData> aData;
    for (int i = 0; i < (int)text.size(); i++) {
        aData.push_back(ViewCae::MeshAnnotationData(text[i].c_str(), icons[i].c_str()));
    }
    theAnnotation.SetData(aData);

    theAnnotation.SetPos(pos.XYZ());
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CaeMeshAnnotationInfo)
{
    emscripten::value_object<CaeMeshAnnotationInfo>("CaeMeshAnnotationInfo")
        .field("isVisible", &CaeMeshAnnotationInfo::isVisible)
        .field("text", &CaeMeshAnnotationInfo::text)
        .field("icons", &CaeMeshAnnotationInfo::icons)
        .field("toShowText", &CaeMeshAnnotationInfo::toShowText)
        .field("toShowIcons", &CaeMeshAnnotationInfo::toShowIcons)
        .field("pos", &CaeMeshAnnotationInfo::pos);
}
#endif

} // namespace