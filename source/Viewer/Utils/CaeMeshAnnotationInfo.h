#ifndef _CAEMESHANNOTATIONINFO_HEADERFILE
#define _CAEMESHANNOTATIONINFO_HEADERFILE

#include "VecN.h"

#include <ViewCae/MeshAnnotations.h>

namespace Viewer {
//! Information about cae mesh annotation.
class CaeMeshAnnotationInfo {
public:
    bool isVisible   = true;
    bool toShowText  = true;
    bool toShowIcons = false;

    std::vector<std::string> text;
    std::vector<std::string> icons;

    Vec3 pos;

public:
    //! Empty constructor.
    CaeMeshAnnotationInfo() {}

    //! Copy parameters from mesh annotation.
    void InitFromAnnotation(const ViewCae::MeshAnnotation& theAnnotation);

    //! Update mesh annotation.
    void UpdateAnnotation(ViewCae::MeshAnnotation& theAnnotation) const;
};

} // namespace

#endif // _CAEMESHANNOTATIONINFO_HEADERFILE

