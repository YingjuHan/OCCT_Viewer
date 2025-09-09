#ifndef ViewBASETEST_SCVVIEWCUBE_HEADERFILE
#define ViewBASETEST_SCVVIEWCUBE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

namespace ViewBaseTest {

//! Command for managing View Cube interactive object @link ViewBase::InteractiveViewCube @endlink.
class ScvViewCube : public ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvviewcube";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvviewcube Name [-size Size]
Create and display the view cube with the name Name.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvViewCube();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_name;
    double                  m_baseSize = -1.0;
};

}

#endif // ViewBASETEST_SCVVIEWCUBE_HEADERFILE