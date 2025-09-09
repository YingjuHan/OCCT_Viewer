#ifndef _SCVMESHSLICE_HEADERFILE
#define _SCVMESHSLICE_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting slice plane for CAE mesh.
class ScvMeshSlice : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshslice";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshslice prsName {-prs index} [-add A B C D] [-clear]
Set mesh slice planes.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshSlice();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    Graphic3d_SequenceOfHClipPlane m_slices;
    bool                           m_toClear = false;
};

}

#endif // _SCVMESHSLICE_HEADERFILE

