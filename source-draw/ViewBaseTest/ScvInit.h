#ifndef ViewBASETEST_SCVINIT_HEADERFILE
#define ViewBASETEST_SCVINIT_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace ViewBaseTest {

//! Command for Viewer initialization @link Viewer::WasmViewer @endlink.
class ScvInit : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvinit";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvinit [ViewName] [-pixelRatio Ratio] [...]
Initialize viewer (vinit wrapper).
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvInit();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_args       = "vinit";
    double                  m_pixelRatio = -1.0;
};

}

#endif // ViewBASETEST_SCVINIT_HEADERFILE