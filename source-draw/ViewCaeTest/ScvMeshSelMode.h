

#ifndef _SCVMESHSELMODE_HEADERFILE
#define _SCVMESHSELMODE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewCae/InteractiveCaeMesh.h>

namespace ViewCaeTest {

//! Command for setting CAE mesh selection mode.
class ScvMeshSelMode : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshselmode";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshselmode prsName [-presentation] [-element] [-node]
Set mesh selection mode.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshSelMode();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    Handle(ViewCae::InteractiveCaeMeshOwner) m_mesh;

    int m_selMode = 0;
};

}

#endif // _SCVMESHSELMODE_HEADERFILE

