#include "CameraViewPoint.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
// ================================================================
// Function : CameraViewPoint
// ================================================================
CameraViewPoint::CameraViewPoint() : eye(), up(), direction(), distance(0.0), scale(0.0)
{
    //
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CameraViewPoint)
{
    emscripten::value_object<CameraViewPoint>("CameraViewPoint")
        .field("eye", &CameraViewPoint::eye)
        .field("up", &CameraViewPoint::up)
        .field("direction", &CameraViewPoint::direction)
        .field("distance", &CameraViewPoint::distance)
        .field("scale", &CameraViewPoint::scale);
}
#endif

} // namespace
