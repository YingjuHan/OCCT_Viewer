
#ifndef _SCVMESHREMOVEPROPS_HEADERFILE
#define _SCVMESHREMOVEPROPS_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for removing mesh properties.
class ScvMeshRemoveProps : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshremoveprops";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshremoveprops prsName
Remove all mesh properties.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshRemoveProps();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;
};

}

#endif // _SCVMESHREMOVEPROPS_HEADERFILE
