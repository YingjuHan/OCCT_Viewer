#include "WasmModulePrinter.h"

#include <TCollection_AsciiString.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>

    #if defined(_LP64)
//! Cast 64-bit pointer (BigInt) into double (Number).
EM_JS(double, jsPtrToNumber, (const char* thePtr), { return bigintToI53Checked(thePtr); });

//! Cast Number -> 64-bit pointer (BigInt).
EM_JS(const char*, jsNumberToPtr, (double thePtr), { return BigInt(thePtr); });
    #else
//! Cast 32-bit pointer (Number) into double (Number).
EM_JS(double, jsPtrToNumber, (const char* thePtr), { return thePtr; });

//! Cast Number -> 32-bit pointer (Number).
EM_JS(const char*, jsNumberToPtr, (double thePtr), { return thePtr; });
    #endif

//! Wrap UTF8ToString() for casting BigInt -> Number.
EM_JS(const char*, jsUTF8ToString, (const char* theText), {
    const aPtr = jsPtrToNumber(theText);
    return UTF8ToString(aPtr);
});

//! Print message to Module.printMessage callback.
EM_JS(void, jsPrintMessage, (const char* theText, int theGravity), {
    if (Module.printMessage != undefined && Module.printMessage != null) {
        Module.printMessage(jsUTF8ToString(theText), theGravity);
    } else if (Module.print != undefined && Module.print != null) {
        Module.print(jsUTF8ToString(theText));
    } else {
        // @c console.info (jsUTF8ToString(theText));
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

