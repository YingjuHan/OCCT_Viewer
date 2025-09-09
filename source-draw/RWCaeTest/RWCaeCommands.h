
#ifndef RWCAETEST_RWCAECOMMANDS_HEADERFILE
#define RWCAETEST_RWCAECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace RWCaeTest {

//! DRAW Harness factory for CAE mesh file storage commands.
class RWCaeCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

}

#endif // RWCAETEST_RWCAECOMMANDS_HEADERFILE