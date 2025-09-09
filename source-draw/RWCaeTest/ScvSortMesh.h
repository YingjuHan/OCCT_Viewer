
#ifndef RWCAETEST_SCVSORTMESH_HEADERFILE
#define RWCAETEST_SCVSORTMESH_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace RWCaeTest {

//! Command for sorting of CAE mesh nodes and elements.
class ScvSortMesh : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvsortmesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvsortmesh prsName
Sort CAE mesh nodes and elements.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvSortMesh();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_prsName;
};

}

#endif // RWCAETEST_SCVSORTMESH_HEADERFILE
