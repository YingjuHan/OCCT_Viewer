#ifndef _SCVMESHANNOTATION_HEADERFILE
#define _SCVMESHANNOTATION_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for displaying/hiding CAE mesh annotations.
class ScvMeshAnnotation : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshannotation";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshannotation prsName {-prs index} [{-minmax|+minmax}] [{-groups|+groups}]
Enable/disable displaying of specified mesh annotations.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshAnnotation();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    int m_toShowMinMax = -1;
    int m_toShowGroups = -1;
};

}

#endif // _SCVMESHANNOTATION_HEADERFILE
