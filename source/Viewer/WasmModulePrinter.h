#ifndef _WASMMODULEPRINTER_HEADERFILE
#define _WASMMODULEPRINTER_HEADERFILE

#include <Message_Printer.hxx>

namespace Viewer {

//! Auxiliary printer to a Module.printMessage callback accepting text and gravity.
class WasmModulePrinter : public Message_Printer {
public:
    //! Main constructor.
    Standard_EXPORT WasmModulePrinter(const Message_Gravity theTraceLevel = Message_Info);

    //! Destructor.
    Standard_EXPORT virtual ~WasmModulePrinter();

protected:
    //! Puts a message.
    Standard_EXPORT virtual void send(const TCollection_AsciiString& theString,
                                      const Message_Gravity          theGravity) const Standard_OVERRIDE;
};

} // namespace

#endif // _WASMMODULEPRINTER_HEADERFILE