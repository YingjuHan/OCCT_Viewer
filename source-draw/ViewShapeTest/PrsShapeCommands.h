
#ifndef SCPRSSHAPETEST_PRSSHAPECOMMANDS_HEADERFILE
#define SCPRSSHAPETEST_PRSSHAPECOMMANDS_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

namespace ViewShapeTest {

//! DRAW Harness factory for commands managing @link ViewShape::InteractiveShape @endlink presentation.
class PrsShapeCommands {
public:
    DEFINE_STANDARD_ALLOC

    //! Defines all commands.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

}

#endif // SCPRSSHAPETEST_PRSSHAPECOMMANDS_HEADERFILE

