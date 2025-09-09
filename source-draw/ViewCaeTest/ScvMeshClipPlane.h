#ifndef _SCVMESHCLIPPLANE_HEADERFILE
#define _SCVMESHCLIPPLANE_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting clip plane for CAE mesh.
class ScvMeshClipPlane : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshclipplane";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshclipplane prsName {-prs index} [-set A B C D] [-unset] [-cut]
Set mesh clip plane.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshClipPlane();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    Handle(Graphic3d_ClipPlane) m_clipPlane;
    int                         m_toCut   = -1;
    bool                        m_toUnset = false;
};

}

#endif // _SCVMESHCLIPPLANE_HEADERFILE
