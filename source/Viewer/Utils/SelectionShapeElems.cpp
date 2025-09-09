#include "SelectionShapeElems.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : Clear
// ================================================================
void SelectionShapeElems::Clear()
{
    groups.clear();
    compSolids.clear();
    solids.clear();
    shells.clear();
    faces.clear();
    wires.clear();
    edges.clear();
    verts.clear();
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(DetectedShapeElem)
{
    emscripten::value_object<DetectedShapeElem>("DetectedShapeElem")
        .field("entityId", &DetectedShapeElem::entityId)
        .field("entityType", &DetectedShapeElem::entityType);
}
EMSCRIPTEN_BINDINGS(SelectionShapeElems)
{
    emscripten::value_object<SelectionShapeElems>("SelectionShapeElems")
        .field("groups", &SelectionShapeElems::groups)
        .field("compSolids", &SelectionShapeElems::compSolids)
        .field("solids", &SelectionShapeElems::solids)
        .field("shells", &SelectionShapeElems::shells)
        .field("faces", &SelectionShapeElems::faces)
        .field("wires", &SelectionShapeElems::wires)
        .field("edges", &SelectionShapeElems::edges)
        .field("verts", &SelectionShapeElems::verts);
}
#endif

} // namespace