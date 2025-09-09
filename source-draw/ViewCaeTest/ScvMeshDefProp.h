#ifndef _SCVMESHDEFPROP_HEADERFILE
#define _SCVMESHDEFPROP_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting deformation CAE mesh property.
class ScvMeshDefProp : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshdefprop";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshdefprop prsName {-prs index} [{-index index|-unset}] [-scale value] [-info]
Set deformation mesh property.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshDefProp();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    bool   m_toUnset    = false;
    double m_scale      = -1.0;
    bool   m_toShowInfo = false;
};

}

#endif // _SCVMESHDEFPROP_HEADERFILE
