#ifndef ViewBASETEST_PRSBASECOMMANDS_HEADERFILE
#define ViewBASETEST_PRSBASECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace ViewBaseTest {

//! DRAW Harness factory for commands managing @link ViewBase @endlink presentations.
class PrsBaseCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theCommands);
};

}

#endif // ViewBASETEST_PRSBASECOMMANDS_HEADERFILE