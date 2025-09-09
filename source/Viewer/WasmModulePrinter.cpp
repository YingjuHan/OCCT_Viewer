#include "WasmModulePrinter.h"

#include <TCollection_AsciiString.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>

//! Print message to Module.printMessage callback.
EM_JS(void, jsPrintMessage, (const char* theText, int theGravity), {
    if (Module.printMessage != undefined && Module.printMessage != null) {
        Module.printMessage(UTF8ToString(theText), theGravity);
    } else if (Module.print != undefined && Module.print != null) {
        Module.print(UTF8ToString(theText));
    } else {
        // @c console.info (UTF8ToString(theText));
    }
});
#endif

namespace Viewer {

// =======================================================================
// function : WasmModulePrinter
// =======================================================================
WasmModulePrinter::WasmModulePrinter(const Message_Gravity theTraceLevel)
{
    SetTraceLevel(theTraceLevel);
}

// =======================================================================
// function : ~WasmModulePrinter
// =======================================================================
WasmModulePrinter::~WasmModulePrinter()
{
}

// =======================================================================
// function : send
// =======================================================================
void WasmModulePrinter::send(const TCollection_AsciiString& theString, const Message_Gravity theGravity) const
{
    if (theGravity >= myTraceLevel) {
#ifdef __EMSCRIPTEN__
        jsPrintMessage(theString.ToCString(), (int)theGravity);
#else
        (void)theString;
#endif
    }
}

} // namespace

