#ifndef _SCVMESHSIMPLIFY_HEADERFILE
#define _SCVMESHSIMPLIFY_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <LodCae/Simplifier.h>

namespace LodCaeTest {

//! Command for simplification of CAE mesh data objects.
class ScvMeshSimplify : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshsimplify";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshsimplify inputName outputName
                    [-targetRatio ratio]=0.5
                    [-maxError value]=0
                    [-aggregation [-allowInverse]]
                    [-elemType {2d|surf|preferSurf}]=preferSurf
Produces simplified mesh model from the input one.
 -targetRatio  defines the expected ratio of the number of output mesh elements
               to the number of input elements;
               zero value turns off this option.
 -maxError     defines max allowed error. The algorithm stops if further
               simplification will lead to exceeding this error;
               zero value turns off this option.
 -aggregation  turns on aggregation mode, when distinct mesh regions are
               permitted to join together.
 -allowInverse allows inversion of triangles in aggregation mode.
 -elemType     defines the source element type. It can be '2d', 'surf' or 'preferSurf';
               if 'preferSurf' then any non-empty container is taken perferring surface elements.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshSimplify();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse element type.
    bool ParseElemType(const char* theStr);

private:
    TCollection_AsciiString m_inputName;
    TCollection_AsciiString m_outputName;

    double m_targetRatio  = 0.;
    double m_maxError     = 0.;
    bool   m_aggregation  = false;
    bool   m_allowInverse = false;

    LodCae::SourceElemType m_elemType = LodCae::SourceElemType::PreferSurface;
};
}

#endif // RWCAETEST_SCVREADMESH_HEADERFILE