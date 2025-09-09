
#ifndef RWCAETEST_SCVMESHINFO_HEADERFILE
#define RWCAETEST_SCVMESHINFO_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace RWCaeTest {

//! Command for printing CAE mesh statistics.
class ScvMeshInfo : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshinfo";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshinfo prsName [-recompute]
Print mesh statistics (AABB, numbers of entities of different types).
  -recompute option forces recomputation of AABB of a mesh, otherwise a cached AABB is used.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshInfo();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_prsName;
    bool                    m_toRecomputeAABB = false;
};

}

#endif // RWCAETEST_SCVMESHINFO_HEADERFILE
