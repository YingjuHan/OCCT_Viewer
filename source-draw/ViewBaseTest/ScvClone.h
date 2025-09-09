#ifndef ViewBASETEST_SCVCLONE_HEADERFILE
#define ViewBASETEST_SCVCLONE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

namespace ViewBaseTest {

//! Command for cloning @link ViewBase::IInteractiveObject @endlink.
class ScvClone : public ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvclone";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvclone prsName newName
Make a copy of existing presentation.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvClone();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_oldName;
    TCollection_AsciiString m_newName;
};

}

#endif // ViewBASETEST_SCVCLONE_HEADERFILE

