
#ifndef RWSHAPETEST_SCVEXPLODE_HEADERFILE
#define RWSHAPETEST_SCVEXPLODE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWShapeConv/ShapeLoader.h>

namespace RWShapeTest {

//! Command for exploding shape into subshapes.
class ScvExplode : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvexplode";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvexplode prsName [-depth ExplodeDepth]=-1 [-propagateId 0|1]=1 [-genId 0|1]=1
Explode shape to sub-shapes (previous objects will be removed)
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvExplode();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArg) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_prsName;

    RWShapeConv::ShapeLoader::ExplodeParams m_expParams;
};

}

#endif // RWSHAPETEST_SCVEXPLODE_HEADERFILE