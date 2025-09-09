

#ifndef RWCAETEST_SCVREADMESHPROPERTIES_HEADERFILE
#define RWCAETEST_SCVREADMESHPROPERTIES_HEADERFILE

#include <RWCaeTest/MeshIOCommand.h>

namespace RWCaeTest {

//! Command for loading CAE mesh properties from files.
class ScvReadMeshProperties : public MeshIOCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvreadmeshproperties";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvreadmeshproperties prsName *.msh|*.pos
Load mesh properties from file and append them to existing mesh.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvReadMeshProperties();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_prsName;
    TCollection_AsciiString m_fileName;
};

}

#endif // RWCAETEST_SCVREADMESHPROPERTIES_HEADERFILE
