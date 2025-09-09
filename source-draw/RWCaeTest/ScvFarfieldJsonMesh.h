
#ifndef RWCAETEST_SCVFARFIELDJSONMESH_HEADERFILE
#define RWCAETEST_SCVFARFIELDJSONMESH_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWCaeTest/MeshIOCommand.h>

namespace RWCaeTest {

//! Command for reading JSON EM farfield data and generating a mesh.
//! The JSON file must be as follows:
//! @include farfield_sample.json
class ScvFarfieldJsonMesh : public MeshIOCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvfarfieldjsonmesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvfarfieldjsonmesh meshName inputFile.json columnName [-2d|-3d]=-3d [-norm]=1.0
                    [-time {no|fileName|<index> <description>]=no
Creates a test mesh by parsing a JSON file with farfield data.
columnName indicates the column in the JSON table from which to derive
absolute value for polar coordinates.
Options:
  -2d                     create a flat 2D farfield plot presentation;
  -3d                     create a 3D presentation;
  -norm                   a maximum radius value, which all node positions are normalized to;
  -time                   specify how time step info should be added to MeshData:
    no                    don't add time step info to MeshData (use time step index = -1);
    fileName              extract time step index and description from the given
                          input JSON file name `prefix_<time_step_index>_<time_step_description>_suffix.json`;
    <index> <description> add time step with specified ID and description to MeshData.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvFarfieldJsonMesh();

    //! Process all arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

public:
    struct FarfieldTriplet {
        double Abs   = 0.0;
        double Theta = 0.0;
        double Phi   = 0.0;
    };

    struct PropInfo {
        size_t                  Stride    = 0;
        int                     PropIndex = 0;
        Bnd_Range               PropRange;
        TCollection_AsciiString PropUnits;
        bool                    RangeShift = false;
    };

    enum class FarfieldPrsType { Type3D, Type2D };

private:
    //! Returns whether the requested presentation type is 2D.
    bool is2D() const { return m_prsType == FarfieldPrsType::Type2D; }

    //! Parse -time argument
    bool parseTime(int& theArgIter, int& theTimeStep, TCollection_AsciiString& theDescription);

    //! Parse JSON file.
    bool parseFile(std::vector<FarfieldTriplet>& theResult, PropInfo& thePropInfo) const;

    //! Create MeshData object.
    void createMeshData(Handle(RWCae::MeshData)&          theResult,
                        const std::vector<FarfieldTriplet>& theParsedValues,
                        const PropInfo&                     thePropInfo) const;

private:
    FarfieldPrsType         m_prsType = FarfieldPrsType::Type3D;
    TCollection_AsciiString m_colName;
    double                  m_norm     = 1.0;
    int                     m_timeStep = -1;
    TCollection_AsciiString m_timeDescr;
};

}

#endif // RWCAETEST_SCVFARFIELDJSONMESH_HEADERFILE