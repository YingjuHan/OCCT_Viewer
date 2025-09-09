#ifndef RWCAETEST_SCVREADMESH_HEADERFILE
#define RWCAETEST_SCVREADMESH_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWCae/MeshImportExport.h>

#include <RWCaeTest/MeshIOCommand.h>

namespace RWCaeTest {

//! Command for reading CAE mesh files.
class ScvReadMesh : public MeshIOCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvreadmesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvreadmesh prsName {model.msh|model.pos|model.rmed}
                    [-display {0|1}]=-1
                    [{-geom|+geom}] [{-props|+props}]
                    [-skipdata|-header] [-skipPropData] [-savePropRefFile]
                    [+propById id [-timeStepRange firstTimeStep lastTimeStep]]
                    [-dumpTime] [-parallel] [-nodeVecAsScalar]
                    [-testCacheGeom {0|1}]=0
Load a mesh model with one or more optional data fields from mesh file.
Supported MSH v2 or MED v3+ (optional) file formats.
[-skipdata|-header]  works for MDB format only; if specified, only JSON header is parsed
                     and the binary mesh/field data is not loaded;
{-geom|+geom}        turn on/off the reading of geometry;
{-props|+props}      turn on/off the reading of properties;
[-skipPropData]      turn off the reading of properties binary data;
[-savePropRefFile]   save in properties reference to MDB file to use it as a cache;
[+propById id]       read property by Id, all other will be skipped;
[+timeStepRange firstTimeStep lastTimeStep] additional argument
                     for propById: used to filter by time steps (integer values);
[-parallel]          read file in parallel mode;
[-dumpTime]          write total read time;
[-nodeVecAsScalar]   enable conversion of nodevec properties to scalar if possible (for MED files only);
[-testCacheGeom]     toggle usage of test custom geometry for mesh presentations.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvReadMesh();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_mappingFilePath;
    TCollection_AsciiString m_artifactFilePath;

    int  m_toDisplay        = -1;
    bool m_toDumpTime       = false;
    bool m_nodeVecAsScalar  = false;
    bool m_testCacheGenGeom = false;
};

}

#endif // RWCAETEST_SCVREADMESH_HEADERFILE
