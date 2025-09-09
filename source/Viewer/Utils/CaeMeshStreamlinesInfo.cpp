#include "CaeMeshStreamlinesInfo.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CaeMeshStreamlinesInfo)
{
    emscripten::value_object<CaeMeshStreamlinesInfo>("CaeMeshStreamlinesInfo")
        .field("property", &CaeMeshStreamlinesInfo::property)
        .field("nbStreamlines", &CaeMeshStreamlinesInfo::nbStreamlines)
        .field("planeOrigin", &CaeMeshStreamlinesInfo::planeOrigin)
        .field("planeSide", &CaeMeshStreamlinesInfo::planeSide)
        .field("planeUp", &CaeMeshStreamlinesInfo::planeUp)
        .field("stepCoeff", &CaeMeshStreamlinesInfo::stepCoeff)
        .field("lengthCoeff", &CaeMeshStreamlinesInfo::lengthCoeff)
        .field("thickness", &CaeMeshStreamlinesInfo::thickness);
}
#endif

} // namespace