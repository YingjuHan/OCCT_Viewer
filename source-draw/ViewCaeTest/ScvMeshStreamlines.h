#ifndef _SCVMESHSTREAMLINES_HEADERFILE
#define _SCVMESHSTREAMLINES_HEADERFILE

#include <ViewCae/CaeMeshStreamlinesBuilder.h>

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting streamlines presentation parameters.
class ScvMeshStreamlines : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshstreamlines";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshstreamlines prsName {-prs index} [-prop id]
                   [-regular 0|1] [-nbStreamlines value] [-uStreamlines value] [-vStreamlines value]
                   [-planeOrigin X Y Z] [-planeSide X Y Z] [-planeUp X Y Z]
                   [-stepCoeff val] [-lengthCoeff val] [-thickness val]
                   [-anim {0|1}] [-animSpeed val] [-animTrail val] [-animFreq val]
Set streamlines presentation parameters.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshStreamlines();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    ViewCae::MeshStreamlinesPlane m_plane;
    bool                           m_toSetPlaneOrigin = false;
    bool                           m_toSetPlaneSide   = false;
    bool                           m_toSetPlaneUp     = false;

    int    m_isRegular     = -1;
    int    m_nbStreamlines = -1;
    int    m_uStreamlines  = -1;
    int    m_vStreamlines  = -1;
    double m_stepCoeff     = -1.0;
    double m_lengthCoeff   = -1.0;
    double m_thickness     = -1.0;

    double m_animSpeed = -1.0;
    double m_animTrail = -1.0;
    int    m_animFreq  = -1;
    int    m_toAnim    = -1;
};

}

#endif // _SCVMESHSTREAMLINES_HEADERFILE
