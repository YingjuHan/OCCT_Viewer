#ifndef ViewBASETEST_SCVSCALERULER_HEADERFILE
#define ViewBASETEST_SCVSCALERULER_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

namespace ViewBaseTest {

//! Command for displaying Scale Ruler interactive object @link ViewBase::InteractiveScaleRuler @endlink.
class ScvScaleRuler : public ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvscaleruler";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvscaleruler Name [-size Width Height]
Create and display the scale ruler with the name Name.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvScaleRuler();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_name;

    gp_XY m_baseSize = gp_XY(-1.0, -1.0);
};

}

#endif // ViewBASETEST_SCVSCALERULER_HEADERFILE

