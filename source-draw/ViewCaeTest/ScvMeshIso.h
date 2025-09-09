#ifndef _SCVMESHISO_HEADERFILE
#define _SCVMESHISO_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting iso surface value for CAE mesh.
class ScvMeshIso : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshiso";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshiso prsName {-prs index} [-add value] [-clear]
                   [-propNodal Prop Comp] [-propElemental Prop Comp]
Set mesh iso surface value.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshIso();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    NCollection_List<double> m_isoValues;
    bool                     m_toClear     = false;
    int                      m_prop        = -1;
    int                      m_comp        = -1;
    bool                     m_isElemental = false;
};

}

#endif // _SCVMESHISO_HEADERFILE