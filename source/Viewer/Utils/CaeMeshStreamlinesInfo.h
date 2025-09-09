#ifndef _CAEMESHSTREAMLINESINFO_HEADERFILE
#define _CAEMESHSTREAMLINESINFO_HEADERFILE

#include "VecN.h"

namespace Viewer {
//! Information about CAE mesh streamlines presentation.
struct CaeMeshStreamlinesInfo {
    std::string property = "";
    Vec3        planeOrigin;
    Vec3        planeSide;
    Vec3        planeUp;
    int         nbStreamlines = 0;
    double      stepCoeff     = 1.0;
    double      lengthCoeff   = 1.0;
    double      thickness     = 0.0;

    //! Constructor.
    CaeMeshStreamlinesInfo()
    {
        planeOrigin.SetValues(0.0, 0.0, 0.0);
        planeSide.SetValues(0.0, 1.0, 0.0);
        planeUp.SetValues(0.0, 0.0, 1.0);
    }
};
} // namespace

#endif // _CAEMESHSTREAMLINESINFO_HEADERFILE
