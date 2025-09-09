
#ifndef _PRSCAECOMMANDS_HEADERFILE
#define _PRSCAECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace ViewCaeTest {

//! DRAW Harness factory for commands managing @link ViewCae::InteractiveCaeMesh @endlink presentation.
class PrsCaeCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

}

#endif // _PRSCAECOMMANDS_HEADERFILE
