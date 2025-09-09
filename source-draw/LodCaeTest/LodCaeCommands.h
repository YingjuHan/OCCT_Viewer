#ifndef _LODCAECOMMANDS_HEADERFILE
#define _LODCAECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace LodCaeTest {

//! DRAW Harness factory for LOD related commands.
class LodCaeCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

}

#endif

