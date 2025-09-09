
#ifndef _SCVMESHRESETCOUNTERS_HEADERFILE
#define _SCVMESHRESETCOUNTERS_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <ViewCae/MeshPresentationBase.h>
#include <ViewCae/MeshPropertyData.h>

namespace ViewCaeTest {

//! Command for resetting static counters for CAE meshes.
class ScvMeshResetCounters : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshresetcounters";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshresetcounters
Reset mesh static counters.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    ScvMeshResetCounters() {}

    //! Execute command.
    virtual bool Perform() override
    {
        ViewCae::MeshPresentationBase::ResetStaticCounter();
        ViewCae::MeshPropertyData::ResetStaticCounter();
        ViewCaeGeom::MeshGeometry::ResetStaticCounter();
        RWCae::MeshData::ResetStaticCounter();
        RWCae::MeshPropertyBase::ResetStaticCounter();
        return true;
    }
};

}

#endif // _SCVMESHRESETCOUNTERS_HEADERFILE
