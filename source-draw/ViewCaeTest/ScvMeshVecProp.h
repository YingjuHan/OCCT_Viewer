#ifndef _SCVMESHVECPROP_HEADERFILE
#define _SCVMESHVECPROP_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting vector mesh property.
class ScvMeshVecProp : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshvecprop";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshvecprop prsName {-prs index} [{-prop id|-unset}]
                       [-scale value] [-volume|-surface] [-ratio value]
                       [-info]
Set vector mesh property.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshVecProp();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    double                        m_scale          = -1.0;
    bool                          m_toUnset        = false;
    bool                          m_toShowInfo     = false;
    ViewCae::MeshVectorDistrType m_distrType      = ViewCae::MeshVectorDistrType::Volume;
    bool                          m_toSetDistrType = false;
    double                        m_samplingRatio  = -1.0;
};

}

#endif // _SCVMESHVECPROP_HEADERFILE

