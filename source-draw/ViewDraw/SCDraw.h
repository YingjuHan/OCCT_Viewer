#ifndef SCPRSDRAW_DRAW_HEADERFILE
#define SCPRSDRAW_DRAW_HEADERFILE

#include <Viewer/WasmViewer.h>

#include <DrawBase/DrawCommand.h>

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

//! @namespace ViewDraw
//! ViewDraw is a C++ library implementing [Draw Harness](https://dev.opencascade.org/project/draw-harness) plugin.
//! This plugin could be used for testing viewer functionality and as CLI for automated actions
//! like conversion of CAD/CAE models into Web-friendly format.
namespace ViewDraw {

//! This  class provides commands for testing SCwebviewer.
class SCDraw {
public:
    DEFINE_STANDARD_ALLOC

    //! Add commands to Draw_Interpretor.
    static void Factory(Draw_Interpretor& theDI);
};

}

#endif // SCPRSDRAW_DRAW_HEADERFILE
