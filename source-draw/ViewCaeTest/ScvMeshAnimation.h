#ifndef _SCVMESHANIMATION_HEADERFILE
#define _SCVMESHANIMATION_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for mesh annimation.
class ScvMeshAnimation : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshanim";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshanim prsName [-time timestep] [-info]
                    [-start|-restart|-stop]
                    [-duration value] [-loop]
Command for animation of mesh properties.
  [-time timestep]  Set mesh active time step;
  [-info]           Print available time steps;
  [-start]          Start animation from the curent time step;
  [-restart]        Start animation from the first time step;
  [-stop]           Stop animation;
  [-duration]       Set total animation duration in seconds;
  [-loop]           Set animation looping.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshAnimation();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

protected:
    int    m_timeStep    = -2;
    int    m_toStartAnim = -1;
    double m_duration    = -1.0;
    bool   m_toShowInfo  = false;
    int    m_toLoop      = -1;
};

}

#endif // _SCVMESHANIMATION_HEADERFILE
