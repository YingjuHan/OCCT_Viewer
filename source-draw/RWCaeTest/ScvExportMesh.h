
#ifndef RWCAETEST_SCVEXPORTMESH_HEADERFILE
#define RWCAETEST_SCVEXPORTMESH_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <RWCae/MeshBinWriter.h>

#include <RWCaeTest/MeshIOCommand.h>

namespace RWCaeTest {

//! Command for exporting CAE mesh files.
class ScvExportMesh : public MeshIOCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvexportmesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvexportmesh prsName fileName [{-geom|+geom}] [{-props|+props}] [-skipGeomData] [-skipPropData]
                               [+propById id [-timeStepRange firstTimeStep lastTimeStep]]
                               [-singlePrecision] [-dumpTime]
                               [-draco nodesBits propsBits nbParts] [-parallel]
Export mesh to binary format. Arguments are:
    prsName            a name of presentation to write;
    fileName           an output filename;
    {-geom|+geom}      turn on/off the writing of geometry;
    {-props|+props}    turn on/off the writing of properties;
    [-skipGeomData]    turn off the writing of geometry binary data;
    [-skipPropData]    turn off the writing of properties binary data;
    [+propById id]     write property by Id, all other will be skipped;
    [+timeStepRange    firstTimeStep lastTimeStep] additional argument
                       for propById: used to filter by time steps (integer values);
    [-singlePrecision] write double property values as float values;
    [-draco]           use draco compression for binary data;
    [-parallel]        export file in parallel mode;
    [-dumpTime]        write total export time.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvExportMesh();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    int m_toIsSinglePrecision = -1;

    bool m_toUseDracoCompression = false;
    int  m_dracoNodesQuantBits   = -1;
    int  m_dracoPropsQuantBits   = -1;
    int  m_dracoNbParts          = 1;

    bool m_toDumpTime = false;
};

}

#endif // RWCAETEST_SCVEXPORTMESH_HEADERFILE
