#ifdef __EMSCRIPTEN__

    #include "WasmModulePrinter.h"
    #include "WasmViewer.h"

    #include "OCCTViewerConfig.h"

    #include <RWBase/TaskManager.h>

    #include <Message.hxx>
    #include <Message_Messenger.hxx>
    #include <Message_PrinterSystemLog.hxx>
    #include <OSD_MemInfo.hxx>
    #include <Standard_Version.hxx>

    #include <emscripten.h>
    #include <emscripten/bind.h>
    #include <emscripten/html5.h>
    #include <emscripten/threading.h>

//! Return value of Module._myTraceLevel property.
EM_JS(int, jsGetTraceLevel, (), { return Module._myTraceLevel; });

EMSCRIPTEN_KEEPALIVE int main()
{
    const Message_Gravity aGravity = (Message_Gravity)jsGetTraceLevel();

    // replace default printer into std::cout (Message_PrinterOStream) by
    // a printer into a custom callback Module.printMessage accepting message gravity
    Message::DefaultMessenger()->ChangePrinters().Clear();
    Handle(Viewer::WasmModulePrinter) aJSModulePrinter = new Viewer::WasmModulePrinter(aGravity);
    Message::DefaultMessenger()->AddPrinter(aJSModulePrinter);

    // force logging into JavaScript console
    // @c Handle(Message_PrinterSystemLog) aJSConsolePrinter =
    // @ c  new Message_PrinterSystemLog ("OccViewer", Message_Trace);
    // @c Message::DefaultMessenger()->AddPrinter (aJSConsolePrinter);

    RWBase::TaskManager::Instance()->OnRunMain(RWBase::TaskManager::MainEmscriptenLoop());
    Message::SendTrace() << "Emscripten SDK " << __EMSCRIPTEN_major__ << "." << __EMSCRIPTEN_minor__ << "."
                         << __EMSCRIPTEN_tiny__;

    // static constexpr int    hashCutLen = 7;
    // TCollection_AsciiString aHashViewer(OCCTViewer_GIT_HASH);
    // TCollection_AsciiString aHashOcct("OGG Version");
    // if (aHashViewer.Length() > hashCutLen) {
    //     aHashViewer = aHashViewer.SubString(1, hashCutLen);
    // }
    // if (aHashOcct.Length() > hashCutLen) {
    //     aHashOcct = aHashOcct.SubString(1, hashCutLen);
    // }

    Message::SendInfo() << OCCTViewer_PACKAGE << " " << OCCTViewer_VER_COMPLETE
    #if defined(__EMSCRIPTEN_PTHREADS__)
                        << " | pthreads ON (" << RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch() << ")"
    #else
                        << " | pthreads OFF"
    #endif
    #if defined(__LP64__)
                        << " | wasm64"
    #else
                        << " | wasm32"
    #endif
    #ifndef NDEBUG
                        << " | DEBUG.";
    #else
                        << ".";
    #endif

    Viewer::WasmViewer::Init(NULL, NULL);
    Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
    return 0;
}

#endif // __EMSCRIPTEN__