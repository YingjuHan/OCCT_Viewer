#ifndef RWSHAPETEST_RWSHAPECOMMANDS_HEADERFILE
#define RWSHAPETEST_RWSHAPECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace RWShapeTest {

//! DRAW Harness factory for B-Rep shape file storage commands.
class RWShapeCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

}

#endif // RWSHAPETEST_RWSHAPECOMMANDS_HEADERFILE